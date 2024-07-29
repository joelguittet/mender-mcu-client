/**
 * @file      mender-flash.c
 * @brief     Mender flash interface for weak platform
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

#include "mender-flash.h"

__attribute__((weak)) mender_err_t
mender_flash_open(char *name, size_t size, void **handle) {

    (void)name;
    (void)size;
    (void)handle;

    /* Nothing to do */
    return MENDER_OK;
}

__attribute__((weak)) mender_err_t
mender_flash_write(void *handle, void *data, size_t index, size_t length) {

    (void)handle;
    (void)data;
    (void)index;
    (void)length;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_flash_close(void *handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_flash_set_pending_image(void *handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_flash_abort_deployment(void *handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_flash_confirm_image(void) {

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) bool
mender_flash_is_image_confirmed(void) {

    /* Nothing to do */
    return false;
}
