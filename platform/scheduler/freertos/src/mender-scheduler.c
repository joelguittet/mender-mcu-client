/**
 * @file      mender-scheduler.c
 * @brief     Mender scheduler interface for FreeRTOS platform
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

#if __has_include("FreeRTOS.h")
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <timers.h>
#else
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#endif /* __has_include("FreeRTOS.h") */
#include "mender-log.h"
#include "mender-scheduler.h"

/**
 * @brief Default work queue stack size (kB)
 */
#ifndef CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE
#define CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE (20)
#endif /* CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE */

/**
 * @brief Default work queue priority
 */
#ifndef CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY
#define CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY (5)
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
    SemaphoreHandle_t              sem_handle;   /**< Semaphore used to indicate work is pending or executing */
    TimerHandle_t                  timer_handle; /**< Timer used to periodically execute work */
    bool                           activated;    /**< Flag indicating the work is activated */
} mender_scheduler_work_context_t;

/**
 * @brief Function used to handle work context timer when it expires
 * @param handle Timer handler
 */
static void mender_scheduler_timer_callback(TimerHandle_t handle);

/**
 * @brief Thread used to handle work queue
 * @param arg Not used
 */
static void mender_scheduler_work_queue_thread(void *arg);

/**
 * @brief Work queue handle
 */
static QueueHandle_t mender_scheduler_work_queue_handle = NULL;

mender_err_t
mender_scheduler_init(void) {

    /* Create and start work queue */
    if (NULL == (mender_scheduler_work_queue_handle = xQueueCreate(CONFIG_MENDER_SCHEDULER_WORK_QUEUE_LENGTH, sizeof(mender_scheduler_work_context_t *)))) {
        mender_log_error("Unable to create work queue");
        return MENDER_FAIL;
    }
    if (pdPASS
        != xTaskCreate(mender_scheduler_work_queue_thread,
                       "mender_scheduler_work_queue",
                       (configSTACK_DEPTH_TYPE)(CONFIG_MENDER_SCHEDULER_WORK_QUEUE_STACK_SIZE * 1024 / sizeof(configSTACK_DEPTH_TYPE)),
                       NULL,
                       CONFIG_MENDER_SCHEDULER_WORK_QUEUE_PRIORITY,
                       NULL)) {
        mender_log_error("Unable to create work queue thread");
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
    if (NULL == (work_context->sem_handle = xSemaphoreCreateBinary())) {
        mender_log_error("Unable to create semaphore");
        goto FAIL;
    }

    /* Create timer to handle the work periodically */
    if (NULL
        == (work_context->timer_handle
            = xTimerCreate(work_context->params.name,
                           (work_context->params.period > 0) ? ((1000 * work_context->params.period) / portTICK_PERIOD_MS) : portMAX_DELAY,
                           pdTRUE,
                           work_context,
                           mender_scheduler_timer_callback))) {
        mender_log_error("Unable to create timer");
        goto FAIL;
    }

    /* Return handle to the new work */
    *handle = (void *)work_context;

    return MENDER_OK;

FAIL:

    /* Release memory */
    if (NULL != work_context) {
        if (NULL != work_context->timer_handle) {
            xTimerDelete(work_context->timer_handle, portMAX_DELAY);
        }
        if (NULL != work_context->sem_handle) {
            vSemaphoreDelete(work_context->sem_handle);
        }
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
    if (pdTRUE != xSemaphoreGive(work_context->sem_handle)) {
        mender_log_error("Unable to give semaphore");
        return MENDER_FAIL;
    }

    /* Check the timer period */
    if (work_context->params.period > 0) {

        /* Start the timer to handle the work */
        if (pdPASS != xTimerStart(work_context->timer_handle, portMAX_DELAY)) {
            mender_log_error("Unable to start timer");
            return MENDER_FAIL;
        }

        /* Execute the work now */
        mender_scheduler_timer_callback(work_context->timer_handle);
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
    if (work_context->params.period > 0) {
        if (pdPASS != xTimerChangePeriod(work_context->timer_handle, (1000 * work_context->params.period) / portTICK_PERIOD_MS, portMAX_DELAY)) {
            mender_log_error("Unable to change timer period");
            return MENDER_FAIL;
        }
    } else {
        if (pdPASS != xTimerStop(work_context->timer_handle, portMAX_DELAY)) {
            mender_log_error("Unable to stop timer");
            return MENDER_FAIL;
        }
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_work_execute(void *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)handle;

    /* Execute the work now */
    mender_scheduler_timer_callback(work_context->timer_handle);

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
        xTimerStop(work_context->timer_handle, portMAX_DELAY);
        while (pdFALSE != xTimerIsTimerActive(work_context->timer_handle)) {
            vTaskDelay(1);
        }

        /* Wait if the work is pending or executing */
        if (pdPASS != xSemaphoreTake(work_context->sem_handle, portMAX_DELAY)) {
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
    xTimerDelete(work_context->timer_handle, portMAX_DELAY);
    vSemaphoreDelete(work_context->sem_handle);
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
    if (NULL == (*handle = (void *)xSemaphoreCreateMutex())) {
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_mutex_take(void *handle, int32_t delay_ms) {

    assert(NULL != handle);

    /* Take mutex */
    if (pdPASS != xSemaphoreTake((SemaphoreHandle_t)handle, (delay_ms >= 0) ? (delay_ms / portTICK_PERIOD_MS) : portMAX_DELAY)) {
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_mutex_give(void *handle) {

    assert(NULL != handle);

    /* Give mutex */
    if (pdPASS != xSemaphoreGive((SemaphoreHandle_t)handle)) {
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_scheduler_mutex_delete(void *handle) {

    assert(NULL != handle);

    /* Release memory */
    vSemaphoreDelete((SemaphoreHandle_t)handle);

    return MENDER_OK;
}

mender_err_t
mender_scheduler_exit(void) {

    /* Submit empty work to the work queue, this ask the work queue thread to terminate */
    mender_scheduler_work_context_t *work_context = NULL;
    if (pdPASS != xQueueSend(mender_scheduler_work_queue_handle, &work_context, portMAX_DELAY)) {
        mender_log_error("Unable to submit empty work to the work queue");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

static void
mender_scheduler_timer_callback(TimerHandle_t handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_scheduler_work_context_t *work_context = (mender_scheduler_work_context_t *)pvTimerGetTimerID(handle);
    assert(NULL != work_context);

    /* Exit if the work is already pending or executing */
    if (pdPASS != xSemaphoreTake(work_context->sem_handle, 0)) {
        mender_log_debug("Work '%s' is not activated, already pending or executing", work_context->params.name);
        return;
    }

    /* Submit the work to the work queue */
    if (pdPASS != xQueueSend(mender_scheduler_work_queue_handle, &work_context, 0)) {
        mender_log_warning("Unable to submit work '%s' to the work queue", work_context->params.name);
        xSemaphoreGive(work_context->sem_handle);
    }
}

static void
mender_scheduler_work_queue_thread(void *arg) {

    (void)arg;
    mender_scheduler_work_context_t *work_context = NULL;

    /* Handle work to be executed */
    while (pdPASS == xQueueReceive(mender_scheduler_work_queue_handle, &work_context, portMAX_DELAY)) {

        /* Check if empty work is received from the work queue, this ask the work queue thread to terminate */
        if (NULL == work_context) {
            goto END;
        }

        /* Call work function */
        if (MENDER_DONE == work_context->params.function()) {

            /* Work is done, stop timer used to execute the work periodically */
            xTimerStop(work_context->timer_handle, portMAX_DELAY);
            while (pdFALSE != xTimerIsTimerActive(work_context->timer_handle)) {
                vTaskDelay(1);
            }
        }

        /* Release semaphore used to protect the work function */
        xSemaphoreGive(work_context->sem_handle);
    }

END:

    /* Release memory */
    vQueueDelete(mender_scheduler_work_queue_handle);
    mender_scheduler_work_queue_handle = NULL;

    /* Terminate work queue thread */
    vTaskDelete(NULL);
}
