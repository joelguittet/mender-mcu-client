/**
 * @file      mender-rtos.c
 * @brief     Mender RTOS interface for FreeRTOS platform
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

#if __has_include("FreeRTOS.h")
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"
#else
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#endif /* __has_include("FreeRTOS.h") */
#include "mender-log.h"
#include "mender-rtos.h"

/**
 * @brief Mender RTOS work queue stack
 */
#ifndef MENDER_RTOS_WORK_QUEUE_STACK_SIZE
#define MENDER_RTOS_WORK_QUEUE_STACK_SIZE (12 * 1024)
#endif /* MENDER_RTOS_WORK_QUEUE_STACK_SIZE */

/**
 * @brief Mender RTOS work queue priority
 */
#ifndef MENDER_RTOS_WORK_QUEUE_PRIORITY
#define MENDER_RTOS_WORK_QUEUE_PRIORITY (5)
#endif /* MENDER_RTOS_WORK_QUEUE_PRIORITY */

/**
 * @brief Mender RTOS work queue lenght
 */
#ifndef MENDER_RTOS_WORK_QUEUE_LENGTH
#define MENDER_RTOS_WORK_QUEUE_LENGTH (10)
#endif /* MENDER_RTOS_WORK_QUEUE_LENGTH */

/**
 * @brief Work context
 */
typedef struct {
    mender_rtos_work_params_t params;       /**< Work parameters */
    SemaphoreHandle_t         sem_handle;   /**< Semaphore used to indicate work is pending or executing */
    TimerHandle_t             timer_handle; /**< Timer used to periodically execute work */
} mender_rtos_work_context_t;

/**
 * @brief Function used to handle work context timer when it expires
 * @param handle Timer handler
 */
static void mender_rtos_timer_callback(TimerHandle_t handle);

/**
 * @brief Thread used to handle work queue
 * @param arg Not used
 */
static void mender_rtos_work_queue_thread(void *arg);

/**
 * @brief Work queue handle
 */
static QueueHandle_t mender_rtos_work_queue_handle = NULL;

mender_err_t
mender_rtos_init(void) {

    /* Create and start work queue */
    if (NULL == (mender_rtos_work_queue_handle = xQueueCreate(MENDER_RTOS_WORK_QUEUE_LENGTH, sizeof(mender_rtos_work_context_t *)))) {
        mender_log_error("Unable to create work queue");
        return MENDER_FAIL;
    }
    if (pdPASS
        != xTaskCreate(mender_rtos_work_queue_thread,
                       "mender",
                       (configSTACK_DEPTH_TYPE)(MENDER_RTOS_WORK_QUEUE_STACK_SIZE / sizeof(configSTACK_DEPTH_TYPE)),
                       NULL,
                       MENDER_RTOS_WORK_QUEUE_PRIORITY,
                       NULL)) {
        mender_log_error("Unable to create work queue thread");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_rtos_work_create(mender_rtos_work_params_t *work_params, void **handle) {

    assert(NULL != work_params);
    assert(NULL != work_params->function);
    assert(NULL != work_params->name);
    assert(NULL != handle);

    /* Create work context */
    mender_rtos_work_context_t *work_context = malloc(sizeof(mender_rtos_work_context_t));
    if (NULL == work_context) {
        mender_log_error("Unable to allocate memory");
        goto FAIL;
    }
    memset(work_context, 0, sizeof(mender_rtos_work_context_t));

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
        == (work_context->timer_handle = xTimerCreate(
                work_context->params.name, (1000 * work_context->params.period) / portTICK_PERIOD_MS, pdTRUE, work_context, mender_rtos_timer_callback))) {
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
mender_rtos_work_activate(void *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_rtos_work_context_t *work_context = (mender_rtos_work_context_t *)handle;

    /* Give timer used to protect the work function */
    if (pdTRUE != xSemaphoreGive(work_context->sem_handle)) {
        mender_log_error("Unable to give semaphore");
        return MENDER_FAIL;
    }

    /* Start the timer to handle the work */
    if (pdPASS != xTimerStart(work_context->timer_handle, portMAX_DELAY)) {
        mender_log_error("Unable to start timer");
        return MENDER_FAIL;
    }

    /* Execute the work now */
    mender_rtos_timer_callback(work_context->timer_handle);

    return MENDER_OK;
}

mender_err_t
mender_rtos_work_deactivate(void *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_rtos_work_context_t *work_context = (mender_rtos_work_context_t *)handle;

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

    return MENDER_OK;
}

mender_err_t
mender_rtos_work_delete(void *handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_rtos_work_context_t *work_context = (mender_rtos_work_context_t *)handle;

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
mender_rtos_delay_until_init(unsigned long *handle) {

    assert(NULL != handle);

    /* Get uptime */
    *handle = (unsigned long)xTaskGetTickCount();

    return MENDER_OK;
}

mender_err_t
mender_rtos_delay_until_s(unsigned long *handle, uint32_t delay) {

    /* Sleep */
    vTaskDelayUntil((TickType_t *)handle, (1000 * delay) / portTICK_PERIOD_MS);

    return MENDER_OK;
}

mender_err_t
mender_rtos_mutex_create(void **handle) {

    assert(NULL != handle);

    /* Create mutex */
    if (NULL == (*handle = (void *)xSemaphoreCreateMutex())) {
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_rtos_mutex_take(void *handle, int32_t delay_ms) {

    assert(NULL != handle);

    /* Take mutex */
    if (pdPASS != xSemaphoreTake((SemaphoreHandle_t)handle, (delay_ms >= 0) ? (delay_ms / portTICK_PERIOD_MS) : portMAX_DELAY)) {
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_rtos_mutex_give(void *handle) {

    assert(NULL != handle);

    /* Give mutex */
    if (pdPASS != xSemaphoreGive((SemaphoreHandle_t)handle)) {
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_rtos_mutex_delete(void *handle) {

    assert(NULL != handle);

    /* Release memory */
    vSemaphoreDelete((SemaphoreHandle_t)handle);

    return MENDER_OK;
}

mender_err_t
mender_rtos_exit(void) {

    /* Submit empty work to the work queue, this ask the work queue thread to terminate */
    mender_rtos_work_context_t *work_context = NULL;
    if (pdPASS != xQueueSend(mender_rtos_work_queue_handle, &work_context, portMAX_DELAY)) {
        mender_log_error("Unable to submit empty work to the work queue");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

static void
mender_rtos_timer_callback(TimerHandle_t handle) {

    assert(NULL != handle);

    /* Get work context */
    mender_rtos_work_context_t *work_context = (mender_rtos_work_context_t *)pvTimerGetTimerID(handle);
    assert(NULL != work_context);

    /* Exit if the work is already pending or executing */
    if (pdPASS != xSemaphoreTake(work_context->sem_handle, 0)) {
        mender_log_warning("Work '%s' is already pending or executing", work_context->params.name);
        return;
    }

    /* Submit the work to the work queue */
    if (pdPASS != xQueueSend(mender_rtos_work_queue_handle, &work_context, 0)) {
        mender_log_warning("Unable to submit work '%s' to the work queue", work_context->params.name);
        xSemaphoreGive(work_context->sem_handle);
    }
}

static void
mender_rtos_work_queue_thread(void *arg) {

    (void)arg;
    mender_rtos_work_context_t *work_context = NULL;

    /* Handle work to be executed */
    while (pdPASS == xQueueReceive(mender_rtos_work_queue_handle, &work_context, portMAX_DELAY)) {

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
    vQueueDelete(mender_rtos_work_queue_handle);
    mender_rtos_work_queue_handle = NULL;

    /* Terminate work queue thread */
    vTaskDelete(NULL);
}
