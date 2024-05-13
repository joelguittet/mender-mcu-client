/**
 * @file      mender-storage.c
 * @brief     Mender storage interface for weak platform
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
