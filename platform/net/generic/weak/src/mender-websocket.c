/**
 * @file      mender-websocket.c
 * @brief     Mender websocket interface for weak platform
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

#include "mender-websocket.h"

__attribute__((weak)) mender_err_t
mender_websocket_init(mender_websocket_config_t *config) {

    (void)config;

    /* Nothing to do */
    return MENDER_OK;
}

__attribute__((weak)) mender_err_t
mender_websocket_connect(
    char *jwt, char *path, mender_err_t (*callback)(mender_websocket_client_event_t, void *, size_t, void *), void *params, void **handle) {

    (void)jwt;
    (void)path;
    (void)callback;
    (void)params;
    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_websocket_send(void *handle, void *payload, size_t length) {

    (void)handle;
    (void)payload;
    (void)length;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_websocket_disconnect(void *handle) {

    (void)handle;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_websocket_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}
