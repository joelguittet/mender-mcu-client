/**
 * @file      mender-storage.c
 * @brief     Mender storage interface for PSA ITS API platform
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

#include <psa/internal_trusted_storage.h>
#include "mender-log.h"
#include "mender-storage.h"

/**
 * @brief Storage UIDs
 */
#ifndef CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PRIVATE_KEY
#define CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PRIVATE_KEY (1)
#endif /* CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PRIVATE_KEY */
#ifndef CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PUBLIC_KEY
#define CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PUBLIC_KEY (2)
#endif /* CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PUBLIC_KEY */
#ifndef CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA
#define CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA (3)
#endif /* CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA */
#ifndef CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG
#define CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG (4)
#endif /* CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG */

mender_err_t
mender_storage_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

mender_err_t
mender_storage_set_authentication_keys(unsigned char *private_key, size_t private_key_length, unsigned char *public_key, size_t public_key_length) {

    assert(NULL != private_key);
    assert(NULL != public_key);
    psa_status_t status;

    /* Write keys */
    if (PSA_SUCCESS != (status = psa_its_set(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PRIVATE_KEY, private_key_length, private_key, PSA_STORAGE_FLAG_NONE))) {
        mender_log_error("Unable to write authentication keys (%d)", status);
        return MENDER_FAIL;
    }
    if (PSA_SUCCESS != (status = psa_its_set(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PUBLIC_KEY, public_key_length, public_key, PSA_STORAGE_FLAG_NONE))) {
        mender_log_error("Unable to write authentication keys (%d)", status);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_get_authentication_keys(unsigned char **private_key, size_t *private_key_length, unsigned char **public_key, size_t *public_key_length) {

    assert(NULL != private_key);
    assert(NULL != private_key_length);
    assert(NULL != public_key);
    assert(NULL != public_key_length);
    struct psa_storage_info_t info;
    psa_status_t              status;

    /* Retrieve length of the keys */
    if (PSA_SUCCESS != (status = psa_its_get_info(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PRIVATE_KEY, &info))) {
        mender_log_info("Authentication keys are not available (%d)", status);
        return MENDER_NOT_FOUND;
    }
    *private_key_length = info.size;
    if (PSA_SUCCESS != (status = psa_its_get_info(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PUBLIC_KEY, &info))) {
        mender_log_info("Authentication keys are not available (%d)", status);
        return MENDER_NOT_FOUND;
    }
    *public_key_length = info.size;

    /* Allocate memory to copy keys */
    if (NULL == (*private_key = (unsigned char *)malloc(*private_key_length))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    if (NULL == (*public_key = (unsigned char *)malloc(*public_key_length))) {
        mender_log_error("Unable to allocate memory");
        free(*private_key);
        *private_key = NULL;
        return MENDER_FAIL;
    }

    /* Read keys */
    if (PSA_SUCCESS != (status = psa_its_get(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PRIVATE_KEY, 0, *private_key_length, *private_key, private_key_length))) {
        mender_log_error("Unable to read authentication keys (%d)", status);
        free(*private_key);
        *private_key = NULL;
        free(*public_key);
        *public_key = NULL;
        return MENDER_FAIL;
    }
    if (PSA_SUCCESS != (status = psa_its_get(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PUBLIC_KEY, 0, *public_key_length, *public_key, public_key_length))) {
        mender_log_error("Unable to read authentication keys (%d)", status);
        free(*private_key);
        *private_key = NULL;
        free(*public_key);
        *public_key = NULL;
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_delete_authentication_keys(void) {

    /* Erase keys */
    if ((PSA_SUCCESS != psa_its_remove(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PRIVATE_KEY))
        || (PSA_SUCCESS != psa_its_remove(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_PUBLIC_KEY))) {
        mender_log_error("Unable to erase authentication keys");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_set_deployment_data(char *deployment_data) {

    assert(NULL != deployment_data);
    psa_status_t status;

    /* Write deployment data */
    if (PSA_SUCCESS
        != (status = psa_its_set(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA, strlen(deployment_data) + 1, deployment_data, PSA_STORAGE_FLAG_NONE))) {
        mender_log_error("Unable to write deployment data (%d)", status);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_get_deployment_data(char **deployment_data) {

    assert(NULL != deployment_data);
    psa_status_t              status;
    struct psa_storage_info_t info;
    size_t                    deployment_data_length = 0;

    /* Retrieve length of the deployment data */
    if (PSA_SUCCESS != (status = psa_its_get_info(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA, &info))) {
        mender_log_info("Deployment data not available (%d)", status);
        return MENDER_NOT_FOUND;
    }
    deployment_data_length = info.size;

    /* Allocate memory to copy deployment data */
    if (NULL == (*deployment_data = (char *)malloc(deployment_data_length))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Read deployment data */
    if (PSA_SUCCESS
        != (status
            = psa_its_get(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA, 0, deployment_data_length, *deployment_data, &deployment_data_length))) {
        mender_log_error("Unable to read deployment data (%d)", status);
        free(*deployment_data);
        *deployment_data = NULL;
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_delete_deployment_data(void) {

    psa_status_t status;

    /* Delete deployment data */
    if (PSA_SUCCESS != (status = psa_its_remove(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA))) {
        mender_log_error("Unable to delete deployment data (%d)", status);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
#ifdef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

mender_err_t
mender_storage_set_device_config(char *device_config) {

    assert(NULL != device_config);
    psa_status_t status;

    /* Write device configuration */
    if (PSA_SUCCESS
        != (status = psa_its_set(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG, strlen(device_config) + 1, device_config, PSA_STORAGE_FLAG_NONE))) {
        mender_log_error("Unable to write device configuration (%d)", status);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_get_device_config(char **device_config) {

    assert(NULL != device_config);
    psa_status_t              status;
    struct psa_storage_info_t info;
    size_t                    device_config_length = 0;

    /* Retrieve length of the device configuration */
    if (PSA_SUCCESS != (status = psa_its_get_info(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG, &info))) {
        mender_log_info("Device configuration not available (%d)", status);
        return MENDER_NOT_FOUND;
    }
    device_config_length = info.size;

    /* Allocate memory to copy device configuration */
    if (NULL == (*device_config = (char *)malloc(device_config_length))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Read device configuration */
    if (PSA_SUCCESS
        != (status = psa_its_get(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG, 0, device_config_length, *device_config, &device_config_length))) {
        mender_log_error("Unable to read device configuration (%d)", status);
        free(*device_config);
        *device_config = NULL;
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_delete_device_config(void) {

    psa_status_t status;

    /* Delete device configuration */
    if (PSA_SUCCESS != (status = psa_its_remove(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG))) {
        mender_log_error("Unable to delete device configuration (%d)", status);
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
