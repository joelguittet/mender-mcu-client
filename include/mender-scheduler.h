/**
 * @file      mender-scheduler.h
 * @brief     Mender scheduler interface
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

#ifndef __MENDER_SCHEDULER_H__
#define __MENDER_SCHEDULER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Work parameters
 */
typedef struct {
    mender_err_t (*function)(void); /**< Work function */
    int32_t period;                 /**< Work period (seconds), negative or null value permits to disable periodic execution */
    char *  name;                   /**< Work name */
} mender_scheduler_work_params_t;

/**
 * @brief Initialization of the scheduler
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_init(void);

/**
 * @brief Function used to register a new work
 * @param work_params Work parameters
 * @param handle Work handle if the function succeeds, NULL otherwise
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_work_create(mender_scheduler_work_params_t *work_params, void **handle);

/**
 * @brief Function used to activate a work
 * @param handle Work handle
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_work_activate(void *handle);

/**
 * @brief Function used to set work period
 * @param handle Work handle
 * @param period Work period (seconds)
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_work_set_period(void *handle, uint32_t period);

/**
 * @brief Function used to trigger execution of the work
 * @param handle Work handle
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_work_execute(void *handle);

/**
 * @brief Function used to deactivate a work
 * @param handle Work handle
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_work_deactivate(void *handle);

/**
 * @brief Function used to delete a work
 * @param handle Work handle
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_work_delete(void *handle);

/**
 * @brief Function used to create a mutex
 * @param handle Mutex handle if the function succeeds, NULL otherwise
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_mutex_create(void **handle);

/**
 * @brief Function used to take a mutex
 * @param handle Mutex handle
 * @param delay_ms Delay to obtain the mutex, -1 to block indefinitely (without a timeout)
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_mutex_take(void *handle, int32_t delay_ms);

/**
 * @brief Function used to give a mutex
 * @param handle Mutex handle
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_mutex_give(void *handle);

/**
 * @brief Function used to delete a mutex
 * @param handle Mutex handle
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_mutex_delete(void *handle);

/**
 * @brief Release mender scheduler
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_scheduler_exit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_SCHEDULER_H__ */
