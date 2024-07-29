/**
 * @file      mender-storage.c
 * @brief     Mender storage interface for weak platform
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

#include "mender-log.h"
#include "mender-storage.h"

__attribute__((weak)) mender_err_t
mender_storage_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

__attribute__((weak)) mender_err_t
mender_storage_set_authentication_keys(unsigned char *private_key, size_t private_key_length, unsigned char *public_key, size_t public_key_length) {

    (void)private_key;
    (void)private_key_length;
    (void)public_key;
    (void)public_key_length;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_storage_get_authentication_keys(unsigned char **private_key, size_t *private_key_length, unsigned char **public_key, size_t *public_key_length) {

    (void)private_key;
    (void)private_key_length;
    (void)public_key;
    (void)public_key_length;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_storage_delete_authentication_keys(void) {

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_storage_set_deployment_data(char *deployment_data) {

    (void)deployment_data;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_storage_get_deployment_data(char **deployment_data) {

    (void)deployment_data;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_storage_delete_deployment_data(void) {

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
#ifdef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

__attribute__((weak)) mender_err_t
mender_storage_set_device_config(char *device_config) {

    (void)device_config;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_storage_get_device_config(char **device_config) {

    (void)device_config;

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

__attribute__((weak)) mender_err_t
mender_storage_delete_device_config(void) {

    /* Nothing to do */
    return MENDER_NOT_IMPLEMENTED;
}

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

__attribute__((weak)) mender_err_t
mender_storage_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}
