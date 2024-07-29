/**
 * @file      mender-scheduler.c
 * @brief     Mender scheduler interface for weak platform
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

#include "mender-scheduler.h"

__attribute__((weak)) mender_err_t
mender_scheduler_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

__attribute__((weak)) mender_err_t
mender_scheduler_work_create(mender_scheduler_work_params_t *work_params, void **handle) {

    (void)work_params;
    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_scheduler_work_activate(void *handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_scheduler_work_set_period(void *handle, uint32_t period) {

    (void)handle;
    (void)period;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_scheduler_work_execute(void *handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_scheduler_work_deactivate(void *handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_scheduler_work_delete(void *handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_scheduler_mutex_create(void **handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_scheduler_mutex_take(void *handle, int32_t delay_ms) {

    (void)handle;
    (void)delay_ms;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_scheduler_mutex_give(void *handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_scheduler_mutex_delete(void *handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_scheduler_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}
