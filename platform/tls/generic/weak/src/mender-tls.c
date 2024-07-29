/**
 * @file      mender-tls.c
 * @brief     Mender TLS interface for weak platform
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

#include "mender-tls.h"

__attribute__((weak)) mender_err_t
mender_tls_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

__attribute__((weak)) mender_err_t
mender_tls_init_authentication_keys(bool recommissioning) {

    (void)recommissioning;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_tls_get_public_key_pem(char **public_key) {

    (void)public_key;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_tls_sign_payload(char *payload, char **signature, size_t *signature_length) {

    (void)payload;
    (void)signature;
    (void)signature_length;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_tls_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}
