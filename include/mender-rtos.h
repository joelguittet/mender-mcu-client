/**
 * @file      mender-rtos.h
 * @brief     Mender RTOS interface
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

#ifndef __MENDER_RTOS_H__
#define __MENDER_RTOS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mender-common.h"

/**
 * @brief Function used to create a new task
 * @param task_function Task function
 * @param name Name of the task
 * @param stack_size Stack size
 * @param arg Arguments
 * @param priority priority of the task
 * @param handle Task handle if the function succeeds, NULL otherwise
 */
void mender_rtos_task_create(void (*task_function)(void *), char *name, size_t stack_size, void *arg, int priority, void **handle);

/**
 * @brief Fucntion used to delete a task
 * @param handle Task handle
 */
void mender_rtos_task_delete(void *handle);

/**
 * @brief Function used to initialize handle to be used with mender_rtos_delay_until_* functions
 * @param handle Delay handle
 */
void mender_rtos_delay_until_init(unsigned long *handle);

/**
 * @brief Function used to make a delay until a specified time
 * @param delay Delay value (seconds)
 */
void mender_rtos_delay_until_s(unsigned long *handle, uint32_t delay);

/**
 * @brief Function used to create a mutex
 * @return Sempahore handle if the function succeeds, NULL otherwise
 */
void *mender_rtos_semaphore_create_mutex(void);

/**
 * @brief Function used to take a semaphore
 * @param handle Semaphore handle
 * @param delay_ms Delay to obtain the semaphore, -1 to block indefinitely (without a timeout)
 */
void mender_rtos_semaphore_take(void *handle, int32_t delay_ms);

/**
 * @brief Function used to give a semaphore
 * @param handle Semaphore handle
 */
void mender_rtos_semaphore_give(void *handle);

/**
 * @brief Function used to delete a semaphore
 * @param handle Semaphore handle
 */
void mender_rtos_semaphore_delete(void *handle);

#ifdef __cplusplus
}
#endif

#endif /* __MENDER_RTOS_H__ */
