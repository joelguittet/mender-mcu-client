/**
 * @file      mender-http.c
 * @brief     Mender HTTP interface for weak platform
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

#include "mender-http.h"

__attribute__((weak)) mender_err_t
mender_http_init(mender_http_config_t *config) {

    (void)config;

    /* Nothing to do */
    return MENDER_OK;
}

__attribute__((weak)) mender_err_t
mender_http_perform(char                *jwt,
                    char                *path,
                    mender_http_method_t method,
                    char                *payload,
                    char                *signature,
                    mender_err_t (*callback)(mender_http_client_event_t, void *, size_t, void *),
                    void *params,
                    int  *status) {

    (void)jwt;
    (void)path;
    (void)method;
    (void)payload;
    (void)signature;
    (void)callback;
    (void)params;
    (void)status;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_http_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}
