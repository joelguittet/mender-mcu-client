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
#else
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#endif
#include "mender-rtos.h"

void
mender_rtos_task_create(void (*task_function)(void *), char *name, size_t stack_size, void *arg, uint32_t priority, void **handle) {
    xTaskCreate(task_function, name, (configSTACK_DEPTH_TYPE)(stack_size / sizeof(configSTACK_DEPTH_TYPE)), arg, priority, (TaskHandle_t *)handle);
}

void
mender_rtos_task_delete(void *handle) {
    vTaskDelete(handle);
}

void
mender_rtos_delay_until_init(unsigned long *handle) {
    assert(NULL != handle);
    *handle = (unsigned long)xTaskGetTickCount();
}

void
mender_rtos_delay_until_s(unsigned long *handle, uint32_t delay) {
    vTaskDelayUntil((TickType_t *)handle, (1000 * delay) / portTICK_PERIOD_MS);
}

void *
mender_rtos_semaphore_create_mutex(void) {
    return (void *)xSemaphoreCreateMutex();
}

void
mender_rtos_semaphore_take(void *handle, int32_t delay_ms) {
    if (delay_ms >= 0) {
        xSemaphoreTake((SemaphoreHandle_t)handle, delay_ms / portTICK_PERIOD_MS);
    } else {
        xSemaphoreTake((SemaphoreHandle_t)handle, portMAX_DELAY);
    }
}

void
mender_rtos_semaphore_give(void *handle) {
    xSemaphoreGive((SemaphoreHandle_t)handle);
}

void
mender_rtos_semaphore_delete(void *handle) {
    vSemaphoreDelete((SemaphoreHandle_t)handle);
}
