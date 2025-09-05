/**
 * @file      mender-scheduler.c
 * @brief     Mender scheduler interface for Posix platform
 *
 * Copyright joelguittet and mender-mcu-client contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <math.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "mender-log.h"
#include "mender-scheduler.h"

/**
 * @brief Default work queue stack size
 */
#ifndef CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE
#define CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE (65536)
#endif /* CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE */

/**
 * @brief Default work queue priority
 */
#ifndef CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY
#define CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY (0)
#endif /* CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY */

/**
 * @brief Default work queue length
 */
#ifndef CONFIG_MENDER_SCHEDULER_WORK_QUEUE_LENGTH
#define CONFIG_MENDER_SCHEDULER_WORK_QUEUE_LENGTH (10)
#endif /* CONFIG_MENDER_SCHEDULER_WORK_QUEUE_LENGTH */

/**
 * @brief Work context
 */
typedef struct {
    mender_scheduler_work_params_t params;       /**< Work parameters */
    pthread_mutex_t                sem_handle;   /**< Semaphore used to indicate work is pending or executing */
    timer_t                        timer_handle; /**< Timer used to periodically execute work */
    bool                           activated;    /**< Flag indicating the work is activated */
} mender_scheduler_work_context_t;

/**
 *
 * @brief Work queue parameters
 */
#define MENDER_SCHEDULER_WORK_QUEUE_NAME  "/mender-work-queue"
#define MENDER_SCHEDULER_WORK_QUEUE_PERMS (0644)

/**
 * @brief Function used to handle work context timer when it expires
 * @param timer_data Timer data
 */
static void mender_scheduler_timer_callback(union sigval timer_data);

/**
 * @brief Thread used to handle work queue
 * @param arg Not used
 * @return Not used
 */
static void *mender_scheduler_work_queue_thread(void *arg);

/**
 * @brief Work queue handle
 */
static mqd_t mender_scheduler_work_queue_handle;

/**
 * @brief Work queue thread handle
 */
static pthread_t mender_scheduler_work_queue_thread_handle;

mender_err_t
mender_scheduler_init(void) {

    int ret;

    /* Create and start work queue */
    struct mq_attr mq_attr;
    memset(&mq_attr, 0, sizeof(struct mq_attr));
    mq_attr.mq_maxmsg  = CONFIG_MENDER_SCHEDULER_WORK_QUEUE_LENGTH;
    mq_attr.mq_msgsize = sizeof(mender_scheduler_work_context_t *);
    mq_unlink(MENDER_SCHEDULER_WORK_QUEUE_NAME);
    if ((mender_scheduler_work_queue_handle = mq_open(MENDER_SCHEDULER_WORK_QUEUE_NAME, O_CREAT | O_RDWR, MENDER_SCHEDULER_WORK_QUEUE_PERMS, &mq_attr)) < 0) {
        mender_log_error("Unable to create work queue (errno=%d)", errno);
        return MENDER_FAIL;
    }
    pthread_attr_t pthread_attr;
    if (0 != (ret = pthread_attr_init(&pthread_attr))) {
        mender_log_error("Unable to initialize work queue thread attributes (ret=%d)", ret);
        return MENDER_FAIL;
    }
    if (0
        != (ret = pthread_attr_setstacksize(&pthread_attr,
                                            (CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE > 16384) ? CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE : 16384))) {
        mender_log_error("Unable to set work queue thread stack size (ret=%d)", ret);
        return MENDER_FAIL;
    }
    if (0 != (ret = pthread_create(&mender_scheduler_work_queue_thread_handle, &pthread_attr, mender_scheduler_work_queue_thread, NULL))) {
        mender_log_error("Unable to create work queue thread (ret=%d)", ret);
        return MENDER_FAIL;
    }
    if (0 != (ret = pthread_setschedprio(mender_scheduler_work_queue_thread_handle, CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY))) {
        mender_log_error("Unable to set work queue thread priority (ret=%d)", ret);
        return MENDER_FAIL;
    }

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
    if (0 != pthread_mutex_init(&work_context->sem_handle, NULL)) {
        mender_log_error("Unable to create semaphore");
        goto FAIL;
    }

    /* Create timer to handle the work periodically */
    struct sigevent sev;
    memset(&sev, 0, sizeof(struct sigevent));
    sev.sigev_notify          = SIGEV_THREAD;
    sev.sigev_notify_function = mender_scheduler_timer_callback;
    sev.sigev_value.sival_ptr = work_context;
    if (0 != timer_create(CLOCK_REALTIME, &sev, &work_context->timer_handle)) {
        mender_log_error("Unable to create timer");
        goto FAIL;
    }

    /* Return handle to the new work */
    *handle = (void *)work_context;

    return MENDER_OK;

FAIL:

    /* Release memory */
    if (NULL != work_context) {
        timer_delete(work_context->timer_handle);
        pthread_mutex_destroy(&work_context->sem_handle);
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
    if (0 != pthread_mutex_unlock(&work_context->sem_handle)) {
        mender_log_error("Unable to give semaphore");
        return MENDER_FAIL;
    }

    /* Check the timer period */
    if (work_context->params.period > 0) {

        /* Start the timer to handle the work */
        struct itimerspec its;
        memset(&its, 0, sizeof(struct itimerspec));
        its.it_value.tv_sec    = work_context->params.period;
        its.it_interval.tv_sec = work_context->params.period;
        if (0 != timer_settime(work_context->timer_handle, 0, &its, NULL)) {
            mender_log_error("Unable to start timer");
            return MENDER_FAIL;
        }

        /* Execute the work now */
        union sigval timer_data;
        timer_data.sival_ptr = (void *)work_context;
        mender_scheduler_timer_callback(timer_data);
    }

    /* Indicate the work has been activated */
    work_context->activated = true;

    return MENDER_OK;
}

mender_err_t
mender_scheduler_work_set_period(void *handle, uint32_t period) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)handle;

    /* Set timer period */
    work_context->params.period = period;
    struct itimerspec its;
    memset(&its, 0, sizeof(struct itimerspec));
    if (work_context->params.period > 0) {
        its.it_value.tv_sec    = work_context->params.period;
        its.it_interval.tv_sec = work_context->params.period;
    }
    if (0 != timer_settime(work_context->timer_handle, 0, &its, NULL)) {
        mender_log_error("Unable to set timer period");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_work_execute(void *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)handle;

    /* Execute the work now */
    union sigval timer_data;
    timer_data.sival_ptr = (void *)work_context;
    mender_scheduler_timer_callback(timer_data);

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
        struct itimerspec its;
        memset(&its, 0, sizeof(struct itimerspec));
        if (0 != timer_settime(work_context->timer_handle, 0, &its, NULL)) {
            mender_log_error("Unable to stop timer");
            return MENDER_FAIL;
        }

        /* Wait if the work is pending or executing */
        if (0 != pthread_mutex_lock(&work_context->sem_handle)) {
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
    timer_delete(work_context->timer_handle);
    pthread_mutex_destroy(&work_context->sem_handle);
    if (NULL != work_context->params.name) {
        free(work_context->params.name);
    }
    free(work_context);

    return MENDER_OK;
}

mender_err_t
mender_scheduler_mutex_create(void **handle) {

    assert(NULL != handle);

    /* Create mutex */
    if (NULL == (*handle = malloc(sizeof(pthread_mutex_t)))) {
        return MENDER_FAIL;
    }
    if (0 != pthread_mutex_init(*handle, NULL)) {
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
    if (delay_ms >= 0) {
        struct timespec timeout;
        timeout.tv_sec  = delay_ms / 1000;
        timeout.tv_nsec = (delay_ms % 1000) * 1000000;
        if (0 != pthread_mutex_timedlock((pthread_mutex_t *)handle, &timeout)) {
            return MENDER_FAIL;
        }
    } else {
        if (0 != pthread_mutex_lock((pthread_mutex_t *)handle)) {
            return MENDER_FAIL;
        }
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_mutex_give(void *handle) {

    assert(NULL != handle);

    /* Give mutex */
    if (0 != pthread_mutex_unlock((pthread_mutex_t *)handle)) {
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_mutex_delete(void *handle) {

    assert(NULL != handle);

    /* Release memory */
    pthread_mutex_destroy((pthread_mutex_t *)handle);
    free(handle);

    return MENDER_OK;
}

mender_err_t
mender_scheduler_exit(void) {

    /* Submit empty work to the work queue, this ask the work queue thread to terminate */
    mender_scheduler_work_context_t *work_context = NULL;
    if (0 != mq_send(mender_scheduler_work_queue_handle, (const char *)&work_context, sizeof(mender_scheduler_work_context_t *), 0)) {
        mender_log_error("Unable to submit empty work to the work queue");
        return MENDER_FAIL;
    }

    /* Wait end of execution of the work queue thread */
    pthread_join(mender_scheduler_work_queue_thread_handle, NULL);

    return MENDER_OK;
}

static void
mender_scheduler_timer_callback(union sigval timer_data) {

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)timer_data.sival_ptr;
    assert(NULL != work_context);

    /* Exit if the work is already pending or executing */
    struct timespec timeout;
    memset(&timeout, 0, sizeof(struct timespec));
    if (0 != pthread_mutex_timedlock(&work_context->sem_handle, &timeout)) {
        mender_log_debug("Work '%s' is not activated, already pending or executing", work_context->params.name);
        return;
    }

    /* Submit the work to the work queue */
    if (0 != mq_send(mender_scheduler_work_queue_handle, (const char *)&work_context, sizeof(mender_scheduler_work_context_t *), 0)) {
        mender_log_warning("Unable to submit work '%s' to the work queue", work_context->params.name);
        pthread_mutex_unlock(&work_context->sem_handle);
    }
}

__attribute__((noreturn)) static void *
mender_scheduler_work_queue_thread(void *arg) {

    (void)arg;
    mender_scheduler_work_context_t *work_context = NULL;

    /* Handle work to be executed */
    while (mq_receive(mender_scheduler_work_queue_handle, (char *)&work_context, sizeof(mender_scheduler_work_context_t *), NULL) > 0) {

        /* Check if empty work is received from the work queue, this ask the work queue thread to terminate */
        if (NULL == work_context) {
            goto END;
        }

        /* Call work function */
        if (MENDER_DONE == work_context->params.function()) {

            /* Work is done, stop timer used to execute the work periodically */
            struct itimerspec its;
            memset(&its, 0, sizeof(struct itimerspec));
            if (0 != timer_settime(work_context->timer_handle, 0, &its, NULL)) {
                mender_log_error("Unable to stop timer");
            }
        }

        /* Release semaphore used to protect the work function */
        pthread_mutex_unlock(&work_context->sem_handle);
    }

END:

    /* Release memory */
    mq_close(mender_scheduler_work_queue_handle);
    mq_unlink(MENDER_SCHEDULER_WORK_QUEUE_NAME);

    /* Terminate work queue thread */
    pthread_exit(NULL);
}
