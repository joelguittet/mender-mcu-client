/**
 * @file      mender-storage.c
 * @brief     Mender storage interface for Posix platform
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

#include <unistd.h>
#include "mender-log.h"
#include "mender-storage.h"

/**
 * @brief Default storage path (working directory)
 */
#ifndef CONFIG_MENDER_STORAGE_PATH
#define CONFIG_MENDER_STORAGE_PATH ""
#endif /* CONFIG_MENDER_STORAGE_PATH */

/**
 * @brief NVS Files
 */
#define MENDER_STORAGE_NVS_PRIVATE_KEY     CONFIG_MENDER_STORAGE_PATH "key.der"
#define MENDER_STORAGE_NVS_PUBLIC_KEY      CONFIG_MENDER_STORAGE_PATH "pubkey.der"
#define MENDER_STORAGE_NVS_DEPLOYMENT_DATA CONFIG_MENDER_STORAGE_PATH "deployment-data.json"
#define MENDER_STORAGE_NVS_DEVICE_CONFIG   CONFIG_MENDER_STORAGE_PATH "config.json"

mender_err_t
mender_storage_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

mender_err_t
mender_storage_set_authentication_keys(unsigned char *private_key, size_t private_key_length, unsigned char *public_key, size_t public_key_length) {

    assert(NULL != private_key);
    assert(NULL != public_key);
    FILE *f;

    /* Write private key */
    if (NULL == (f = fopen(MENDER_STORAGE_NVS_PRIVATE_KEY, "wb"))) {
        mender_log_error("Unable to write authentication keys");
        return MENDER_FAIL;
    }
    if (fwrite(private_key, sizeof(unsigned char), private_key_length, f) != private_key_length) {
        mender_log_error("Unable to write authentication keys");
        fclose(f);
        return MENDER_FAIL;
    }
    fclose(f);

    /* Write public key */
    if (NULL == (f = fopen(MENDER_STORAGE_NVS_PUBLIC_KEY, "wb"))) {
        mender_log_error("Unable to write authentication keys");
        return MENDER_FAIL;
    }
    if (fwrite(public_key, sizeof(unsigned char), public_key_length, f) != public_key_length) {
        mender_log_error("Unable to write authentication keys");
        fclose(f);
        return MENDER_FAIL;
    }
    fclose(f);

    return MENDER_OK;
}

mender_err_t
mender_storage_get_authentication_keys(unsigned char **private_key, size_t *private_key_length, unsigned char **public_key, size_t *public_key_length) {

    assert(NULL != private_key);
    assert(NULL != private_key_length);
    assert(NULL != public_key);
    assert(NULL != public_key_length);
    long  length;
    FILE *f;

    /* Read private key */
    if (NULL == (f = fopen(MENDER_STORAGE_NVS_PRIVATE_KEY, "rb"))) {
        mender_log_info("Authentication keys are not available");
        return MENDER_NOT_FOUND;
    }
    fseek(f, 0, SEEK_END);
    if ((length = ftell(f)) <= 0) {
        mender_log_info("Authentication keys are not available");
        fclose(f);
        return MENDER_NOT_FOUND;
    }
    *private_key_length = (size_t)length;
    fseek(f, 0, SEEK_SET);
    if (NULL == (*private_key = (unsigned char *)malloc(*private_key_length))) {
        mender_log_error("Unable to allocate memory");
        fclose(f);
        return MENDER_FAIL;
    }
    if (fread(*private_key, sizeof(unsigned char), *private_key_length, f) != *private_key_length) {
        mender_log_error("Unable to read authentication keys");
        fclose(f);
        return MENDER_FAIL;
    }
    fclose(f);

    /* Read public key */
    if (NULL == (f = fopen(MENDER_STORAGE_NVS_PUBLIC_KEY, "rb"))) {
        mender_log_info("Authentication keys are not available");
        free(*private_key);
        *private_key        = NULL;
        *private_key_length = 0;
        return MENDER_NOT_FOUND;
    }
    fseek(f, 0, SEEK_END);
    if ((length = ftell(f)) <= 0) {
        mender_log_info("Authentication keys are not available");
        free(*private_key);
        *private_key        = NULL;
        *private_key_length = 0;
        fclose(f);
        return MENDER_NOT_FOUND;
    }
    *public_key_length = (size_t)length;
    fseek(f, 0, SEEK_SET);
    if (NULL == (*public_key = (unsigned char *)malloc(*public_key_length))) {
        mender_log_error("Unable to allocate memory");
        free(*private_key);
        *private_key        = NULL;
        *private_key_length = 0;
        *public_key_length  = 0;
        fclose(f);
        return MENDER_FAIL;
    }
    if (fread(*public_key, sizeof(unsigned char), *public_key_length, f) != *public_key_length) {
        mender_log_error("Unable to read authentication keys");
        fclose(f);
        return MENDER_FAIL;
    }
    fclose(f);

    return MENDER_OK;
}

mender_err_t
mender_storage_delete_authentication_keys(void) {

    /* Erase keys */
    if ((0 != unlink(MENDER_STORAGE_NVS_PRIVATE_KEY)) || (0 != unlink(MENDER_STORAGE_NVS_PUBLIC_KEY))) {
        mender_log_error("Unable to erase authentication keys");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_set_deployment_data(char *deployment_data) {

    assert(NULL != deployment_data);
    size_t deployment_data_length = strlen(deployment_data);
    FILE  *f;

    /* Write deployment data */
    if (NULL == (f = fopen(MENDER_STORAGE_NVS_DEPLOYMENT_DATA, "wb"))) {
        mender_log_error("Unable to write deployment data");
        return MENDER_FAIL;
    }
    if (fwrite(deployment_data, sizeof(unsigned char), deployment_data_length, f) != deployment_data_length) {
        mender_log_error("Unable to write deployment data");
        fclose(f);
        return MENDER_FAIL;
    }
    fclose(f);

    return MENDER_OK;
}

mender_err_t
mender_storage_get_deployment_data(char **deployment_data) {

    assert(NULL != deployment_data);
    size_t deployment_data_length = 0;
    long   length;
    FILE  *f;

    /* Read deployment data */
    if (NULL == (f = fopen(MENDER_STORAGE_NVS_DEPLOYMENT_DATA, "rb"))) {
        mender_log_info("Deployment data is not available");
        return MENDER_NOT_FOUND;
    }
    fseek(f, 0, SEEK_END);
    if ((length = ftell(f)) <= 0) {
        mender_log_info("Deployment data is not available");
        fclose(f);
        return MENDER_NOT_FOUND;
    }
    deployment_data_length = (size_t)length;
    fseek(f, 0, SEEK_SET);
    if (NULL == (*deployment_data = (char *)malloc(deployment_data_length + 1))) {
        mender_log_error("Unable to allocate memory");
        fclose(f);
        return MENDER_FAIL;
    }
    if (fread(*deployment_data, sizeof(unsigned char), deployment_data_length, f) != deployment_data_length) {
        mender_log_error("Unable to read deployment data");
        fclose(f);
        return MENDER_FAIL;
    }
    (*deployment_data)[deployment_data_length] = '\0';
    fclose(f);

    return MENDER_OK;
}

mender_err_t
mender_storage_delete_deployment_data(void) {

    /* Delete deployment data */
    if (0 != unlink(MENDER_STORAGE_NVS_DEPLOYMENT_DATA)) {
        mender_log_error("Unable to delete deployment data");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
#ifdef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

mender_err_t
mender_storage_set_device_config(char *device_config) {

    assert(NULL != device_config);
    size_t device_config_length = strlen(device_config);
    FILE  *f;

    /* Write device configuration */
    if (NULL == (f = fopen(MENDER_STORAGE_NVS_DEVICE_CONFIG, "wb"))) {
        mender_log_error("Unable to write device configuration");
        return MENDER_FAIL;
    }
    if (fwrite(device_config, sizeof(unsigned char), device_config_length, f) != device_config_length) {
        mender_log_error("Unable to write device configuration");
        fclose(f);
        return MENDER_FAIL;
    }
    fclose(f);

    return MENDER_OK;
}

mender_err_t
mender_storage_get_device_config(char **device_config) {

    assert(NULL != device_config);
    size_t device_config_length = 0;
    long   length;
    FILE  *f;

    /* Read device configuration */
    if (NULL == (f = fopen(MENDER_STORAGE_NVS_DEVICE_CONFIG, "rb"))) {
        mender_log_info("Device configuration not available");
        return MENDER_NOT_FOUND;
    }
    fseek(f, 0, SEEK_END);
    if ((length = ftell(f)) <= 0) {
        mender_log_info("Device configuration not available");
        fclose(f);
        return MENDER_NOT_FOUND;
    }
    device_config_length = (size_t)length;
    fseek(f, 0, SEEK_SET);
    if (NULL == (*device_config = (char *)malloc(device_config_length + 1))) {
        mender_log_error("Unable to allocate memory");
        fclose(f);
        return MENDER_FAIL;
    }
    if (fread(*device_config, sizeof(unsigned char), device_config_length, f) != device_config_length) {
        mender_log_error("Unable to read device configuration");
        fclose(f);
        return MENDER_FAIL;
    }
    (*device_config)[device_config_length] = '\0';
    fclose(f);

    return MENDER_OK;
}

mender_err_t
mender_storage_delete_device_config(void) {

    /* Delete device configuration */
    if (0 != unlink(MENDER_STORAGE_NVS_DEVICE_CONFIG)) {
        mender_log_error("Unable to delete device configuration");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

mender_err_t
mender_storage_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}
