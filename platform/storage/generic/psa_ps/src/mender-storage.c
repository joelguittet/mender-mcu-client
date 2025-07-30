/**
 * @file      mender-storage.c
 * @brief     Mender storage interface for PSA PS API platform
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

#include <psa/protected_storage.h>
#include "mender-log.h"
#include "mender-storage.h"

/**
 * @brief Storage UIDs
 */
#ifndef CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA
#define CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA (1)
#endif /* CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA */
#ifndef CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG
#define CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG (2)
#endif /* CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG */

mender_err_t
mender_storage_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

mender_err_t
mender_storage_set_authentication_keys(unsigned char *private_key, size_t private_key_length, unsigned char *public_key, size_t public_key_length) {

    (void)private_key;
    (void)private_key_length;
    (void)public_key;
    (void)public_key_length;

    /* If using PSA API, the mender-tls implementation uses PSA Crypto to manage keys */
    return MENDER_NOT_IMPLEMENTED;
}

mender_err_t
mender_storage_get_authentication_keys(unsigned char **private_key, size_t *private_key_length, unsigned char **public_key, size_t *public_key_length) {

    (void)private_key;
    (void)private_key_length;
    (void)public_key;
    (void)public_key_length;

    /* If using PSA API, the mender-tls implementation uses PSA Crypto to manage keys */
    return MENDER_NOT_IMPLEMENTED;
}

mender_err_t
mender_storage_delete_authentication_keys(void) {

    /* If using PSA API, the mender-tls implementation uses PSA Crypto to manage keys */
    return MENDER_NOT_IMPLEMENTED;
}

mender_err_t
mender_storage_set_deployment_data(char *deployment_data) {

    assert(NULL != deployment_data);
    psa_status_t status;

    /* Write deployment data */
    if (PSA_SUCCESS
        != (status = psa_ps_set(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA, strlen(deployment_data) + 1, deployment_data, PSA_STORAGE_FLAG_NONE))) {
        mender_log_error("Unable to write deployment data (status=%d)", status);
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
    if (PSA_SUCCESS != (status = psa_ps_get_info(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA, &info))) {
        mender_log_info("Deployment data not available (status=%d)", status);
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
        != (status = psa_ps_get(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA, 0, deployment_data_length, *deployment_data, &deployment_data_length))) {
        mender_log_error("Unable to read deployment data (status=%d)", status);
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
    if (PSA_SUCCESS != (status = psa_ps_remove(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEPLOYMENT_DATA))) {
        mender_log_error("Unable to delete deployment data (status=%d)", status);
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
        != (status = psa_ps_set(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG, strlen(device_config) + 1, device_config, PSA_STORAGE_FLAG_NONE))) {
        mender_log_error("Unable to write device configuration (status=%d)", status);
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
    if (PSA_SUCCESS != (status = psa_ps_get_info(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG, &info))) {
        mender_log_info("Device configuration not available (status=%d)", status);
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
        != (status = psa_ps_get(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG, 0, device_config_length, *device_config, &device_config_length))) {
        mender_log_error("Unable to read device configuration (status=%d)", status);
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
    if (PSA_SUCCESS != (status = psa_ps_remove(CONFIG_MENDER_STORAGE_PSA_STORAGE_UID_DEVICE_CONFIG))) {
        mender_log_error("Unable to delete device configuration (status=%d)", status);
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
