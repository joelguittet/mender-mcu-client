/**
 * @file      mender-rtos.c
 * @brief     Mender RTOS interface for Zephyr platform
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
#include "mender-rtos.h"

/**
 * @brief Mender client task stack
 * @note TODO Should be modified when a dynamic API to create stack will be available
 * @ref https://github.com/zephyrproject-rtos/zephyr/issues/26999
 * @ref https://github.com/zephyrproject-rtos/zephyr/pull/44379
 */
#ifndef MENDER_CLIENT_TASK_STACK_SIZE
#define MENDER_CLIENT_TASK_STACK_SIZE (14 * 1024)
#endif
K_THREAD_STACK_DEFINE(mender_client_task_stack, MENDER_CLIENT_TASK_STACK_SIZE);

/**
 * @brief Generic thread entry wrapper
 * @param p1 First param, used to pass task_function
 * @param p2 Second param, used to pass arg
 * @param p3 Third param, not used
 */
static void mender_rtos_thread_entry(void *p1, void *p2, void *p3);

void
mender_rtos_task_create(void (*task_function)(void *), char *name, size_t stack_size, void *arg, int priority, void **handle) {
    struct k_thread *thread = malloc(sizeof(struct k_thread));
    if (NULL != thread) {
        k_thread_create(
            thread, mender_client_task_stack, MENDER_CLIENT_TASK_STACK_SIZE, mender_rtos_thread_entry, task_function, arg, NULL, priority, K_USER, K_NO_WAIT);
        k_thread_name_set(thread, name);
    }
    *handle = (void *)thread;
}

void
mender_rtos_task_delete(void *handle) {
    if (NULL != handle) {
        k_thread_abort(handle);
    }
}

void
mender_rtos_delay_until_init(unsigned long *handle) {
    assert(NULL != handle);
    *handle = (unsigned long)k_uptime_get();
}

void
mender_rtos_delay_until_s(unsigned long *handle, uint32_t delay) {
    assert(NULL != handle);
    int64_t ms = (1000 * delay) - (k_uptime_get() - *handle);
    k_msleep((ms > 0) ? ((int32_t)ms) : 1);
    *handle = (unsigned long)k_uptime_get();
}

void *
mender_rtos_semaphore_create_mutex(void) {
    struct k_mutex *mutex = malloc(sizeof(struct k_mutex));
    if (NULL != mutex) {
        k_mutex_init(mutex);
    }
    return mutex;
}

void
mender_rtos_semaphore_take(void *handle, int32_t delay_ms) {
    if (delay_ms >= 0) {
        k_mutex_lock((struct k_mutex *)handle, K_MSEC(delay_ms));
    } else {
        k_mutex_lock((struct k_mutex *)handle, K_FOREVER);
    }
}

void
mender_rtos_semaphore_give(void *handle) {
    k_mutex_unlock((struct k_mutex *)handle);
}

void
mender_rtos_semaphore_delete(void *handle) {
    free(handle);
}

static void
mender_rtos_thread_entry(void *p1, void *p2, void *p3) {

    assert(NULL != p1);
    void (*task_function)(void *) = p1;
    (void)p3;

    /* Call task function */
    task_function(p2);
}
