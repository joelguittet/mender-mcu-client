/**
 * @file      mender-scheduler.c
 * @brief     Mender scheduler interface for Zephyr platform
 *
 * MIT License
 *
 * Copyright (c) 2022-2023 joelguittet and mender-mcu-client contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <zephyr/kernel.h>
#include "mender-log.h"
#include "mender-scheduler.h"

/**
 * @brief Default work queue stack size (kB)
 */
#ifndef CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE
#define CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE (12)
#endif /* CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE */

/**
 * @brief Default work queue priority
 */
#ifndef CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY
#define CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY (5)
#endif /* CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY */

/**
 * @brief Work context
 */
typedef struct {
    mender_scheduler_work_params_t params;       /**< Work parameters */
    struct k_sem                   sem_handle;   /**< Semaphore used to indicate work is pending or executing */
    struct k_timer                 timer_handle; /**< Timer used to periodically execute work */
    struct k_work                  work_handle;  /**< Work handle used to execute the work function */
    bool                           activated;    /**< Flag indicating the work is activated */
} mender_scheduler_work_context_t;

/**
 * @brief Mender scheduler work queue stack
 */
K_THREAD_STACK_DEFINE(mender_scheduler_work_queue_stack, CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE * 1024);

/**
 * @brief Function used to handle work context timer when it expires
 * @param handle Timer handler
 */
static void mender_scheduler_timer_callback(struct k_timer *handle);

/**
 * @brief Function used to handle work
 * @param handle Work handler
 */
static void mender_scheduler_work_handler(struct k_work *handle);

/**
 * @brief Mender scheduler work queue handle
 */
static struct k_work_q mender_scheduler_work_queue_handle;

mender_err_t
mender_scheduler_init(void) {

    /* Create and start work queue */
    k_work_queue_init(&mender_scheduler_work_queue_handle);
    k_work_queue_start(&mender_scheduler_work_queue_handle,
                       mender_scheduler_work_queue_stack,
                       CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE * 1024,
                       CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY,
                       NULL);
    k_thread_name_set(k_work_queue_thread_get(&mender_scheduler_work_queue_handle), "mender_scheduler_work_queue");

    return MENDER_OK;
}

mender_err_t
mender_scheduler_work_create(mender_scheduler_work_params_t *work_params, void **handle) {

    assert(NULL != work_params);
    assert(NULL != work_params->function);
    assert(NULL != work_params->name);
    assert(NULL != handle);

    /* Create work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)malloc(sizeof(mender_scheduler_work_context_t));
    if (NULL == work_context) {
        mender_log_error("Unable to allocate memory");
        goto FAIL;
    }
    memset(work_context, 0, sizeof(mender_scheduler_work_context_t));

    /* Copy work parameters */
    work_context->params.function = work_params->function;
    work_context->params.period   = work_params->period;
    if (NULL == (work_context->params.name = strdup(work_params->name))) {
        mender_log_error("Unable to allocate memory");
        goto FAIL;
    }

    /* Create semaphore used to protect work function */
    if (0 != k_sem_init(&work_context->sem_handle, 0, 1)) {
        mender_log_error("Unable to create semaphore");
        goto FAIL;
    }

    /* Create timer to handle the work periodically */
    k_timer_init(&work_context->timer_handle, mender_scheduler_timer_callback, NULL);
    k_timer_user_data_set(&work_context->timer_handle, (void *)work_context);

    /* Create work used to execute work function */
    k_work_init(&work_context->work_handle, mender_scheduler_work_handler);

    /* Return handle to the new work context */
    *handle = (void *)work_context;

    return MENDER_OK;

FAIL:

    /* Release memory */
    if (NULL != work_context) {
        if (NULL != work_context->params.name) {
            free(work_context->params.name);
        }
        free(work_context);
    }

    return MENDER_FAIL;
}

mender_err_t
mender_scheduler_work_activate(void *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)handle;

    /* Give semaphore used to protect the work function */
    k_sem_give(&work_context->sem_handle);

    /* Check the timer period */
    if (work_context->params.period > 0) {

        /* Start the timer to handle the work */
        k_timer_start(&work_context->timer_handle, K_NO_WAIT, K_MSEC(1000 * work_context->params.period));
    }

    /* Indicate the work has been activated */
    work_context->activated = true;

    return MENDER_OK;
}

mender_err_t
mender_scheduler_work_execute(void *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)handle;

    /* Execute the work now */
    mender_scheduler_timer_callback(&work_context->timer_handle);

    return MENDER_OK;
}

mender_err_t
mender_scheduler_work_set_period(void *handle, uint32_t period) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)handle;

    /* Set timer period */
    work_context->params.period = period;
    if (work_context->params.period > 0) {
        k_timer_start(&work_context->timer_handle, K_FOREVER, K_MSEC(1000 * work_context->params.period));
    } else {
        k_timer_stop(&work_context->timer_handle);
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_work_deactivate(void *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)handle;

    /* Check if the work was activated */
    if (true == work_context->activated) {

        /* Stop the timer used to periodically execute the work (if it is running) */
        k_timer_stop(&work_context->timer_handle);

        /* Wait if the work is pending or executing */
        if (0 != k_sem_take(&work_context->sem_handle, K_FOREVER)) {
            mender_log_error("Work '%s' is pending or executing", work_context->params.name);
            return MENDER_FAIL;
        }

        /* Indicate the work has been deactivated */
        work_context->activated = false;
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_work_delete(void *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)handle;

    /* Release memory */
    if (NULL != work_context->params.name) {
        free(work_context->params.name);
    }
    free(work_context);

    return MENDER_OK;
}

mender_err_t
mender_scheduler_delay_until_init(unsigned long *handle) {

    assert(NULL != handle);

    /* Get uptime */
    *handle = (unsigned long)k_uptime_get();

    return MENDER_OK;
}

mender_err_t
mender_scheduler_delay_until_s(unsigned long *handle, uint32_t delay) {

    assert(NULL != handle);

    /* Compute sleep time and sleep */
    int64_t ms = (1000 * delay) - (k_uptime_get() - *handle);
    k_msleep((ms > 0) ? ((int32_t)ms) : 1);

    /* Update uptime */
    *handle = (unsigned long)k_uptime_get();

    return MENDER_OK;
}

mender_err_t
mender_scheduler_mutex_create(void **handle) {

    assert(NULL != handle);

    /* Create mutex */
    if (NULL == (*handle = malloc(sizeof(struct k_mutex)))) {
        return MENDER_FAIL;
    }
    if (0 != k_mutex_init((struct k_mutex *)(*handle))) {
        free(*handle);
        *handle = NULL;
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_mutex_take(void *handle, int32_t delay_ms) {

    assert(NULL != handle);

    /* Take mutex */
    if (0 != k_mutex_lock((struct k_mutex *)handle, (delay_ms >= 0) ? K_MSEC(delay_ms) : K_FOREVER)) {
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_mutex_give(void *handle) {

    assert(NULL != handle);

    /* Give mutex */
    if (0 != k_mutex_unlock((struct k_mutex *)handle)) {
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_mutex_delete(void *handle) {

    assert(NULL != handle);

    /* Release memory */
    free(handle);

    return MENDER_OK;
}

mender_err_t
mender_scheduler_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}

static void
mender_scheduler_timer_callback(struct k_timer *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)k_timer_user_data_get(handle);
    assert(NULL != work_context);

    /* Exit if the work is already pending or executing */
    if (0 != k_sem_take(&work_context->sem_handle, K_NO_WAIT)) {
        mender_log_debug("Work '%s' is not activated, already pending or executing", work_context->params.name);
        return;
    }

    /* Submit the work to the work queue */
    if (k_work_submit_to_queue(&mender_scheduler_work_queue_handle, &work_context->work_handle) < 0) {
        mender_log_warning("Unable to submit work '%s' to the work queue", work_context->params.name);
        k_sem_give(&work_context->sem_handle);
    }
}

static void
mender_scheduler_work_handler(struct k_work *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = CONTAINER_OF(handle, mender_scheduler_work_context_t, work_handle);
    assert(NULL != work_context);

    /* Call work function */
    if (MENDER_DONE == work_context->params.function()) {

        /* Work is done, stop timer used to execute the work periodically */
        k_timer_stop(&work_context->timer_handle);
    }

    /* Release semaphore used to protect the work function */
    k_sem_give(&work_context->sem_handle);
}
