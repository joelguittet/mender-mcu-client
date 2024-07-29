/**
 * @file      mender-storage.c
 * @brief     Mender storage interface for Zephyr platform
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

#include <zephyr/drivers/flash.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/storage/flash_map.h>
#include "mender-log.h"
#include "mender-storage.h"

/**
 * @brief NVS storage
 */
#define MENDER_STORAGE_LABEL  storage_partition
#define MENDER_STORAGE_DEVICE FIXED_PARTITION_DEVICE(MENDER_STORAGE_LABEL)
#define MENDER_STORAGE_OFFSET FIXED_PARTITION_OFFSET(MENDER_STORAGE_LABEL)

/**
 * @brief NVS keys
 */
#define MENDER_STORAGE_NVS_PRIVATE_KEY     1
#define MENDER_STORAGE_NVS_PUBLIC_KEY      2
#define MENDER_STORAGE_NVS_DEPLOYMENT_DATA 3
#define MENDER_STORAGE_NVS_DEVICE_CONFIG   4

/**
 * @brief NVS storage handle
 */
static struct nvs_fs mender_storage_nvs_handle;

mender_err_t
mender_storage_init(void) {

    int result;

    /* Get flash info */
    mender_storage_nvs_handle.flash_device = MENDER_STORAGE_DEVICE;
    if (!device_is_ready(mender_storage_nvs_handle.flash_device)) {
        mender_log_error("Flash device not ready");
        return MENDER_FAIL;
    }
    struct flash_pages_info info;
    mender_storage_nvs_handle.offset = MENDER_STORAGE_OFFSET;
    if (0 != flash_get_page_info_by_offs(mender_storage_nvs_handle.flash_device, mender_storage_nvs_handle.offset, &info)) {
        mender_log_error("Unable to get storage page info");
        return MENDER_FAIL;
    }
    mender_storage_nvs_handle.sector_size  = (uint16_t)info.size;
    mender_storage_nvs_handle.sector_count = CONFIG_MENDER_STORAGE_NVS_SECTOR_COUNT;

    /* Mount NVS */
    if (0 != (result = nvs_mount(&mender_storage_nvs_handle))) {
        mender_log_error("Unable to mount NVS storage, result = %d", result);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_set_authentication_keys(unsigned char *private_key, size_t private_key_length, unsigned char *public_key, size_t public_key_length) {

    assert(NULL != private_key);
    assert(NULL != public_key);

    /* Write keys */
    if ((nvs_write(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_PRIVATE_KEY, private_key, private_key_length) < 0)
        || (nvs_write(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_PUBLIC_KEY, public_key, public_key_length) < 0)) {
        mender_log_error("Unable to write authentication keys");
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
    ssize_t ret;

    /* Retrieve length of the keys */
    if ((ret = nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_PRIVATE_KEY, NULL, 0)) <= 0) {
        mender_log_info("Authentication keys are not available");
        return MENDER_NOT_FOUND;
    }
    *private_key_length = (size_t)ret;
    if ((ret = nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_PUBLIC_KEY, NULL, 0)) <= 0) {
        mender_log_info("Authentication keys are not available");
        return MENDER_NOT_FOUND;
    }
    *public_key_length = (size_t)ret;

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
    if ((nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_PRIVATE_KEY, *private_key, *private_key_length) < 0)
        || (nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_PUBLIC_KEY, *public_key, *public_key_length) < 0)) {
        mender_log_error("Unable to read authentication keys");
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
    if ((0 != nvs_delete(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_PRIVATE_KEY))
        || (0 != nvs_delete(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_PUBLIC_KEY))) {
        mender_log_error("Unable to erase authentication keys");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_set_deployment_data(char *deployment_data) {

    assert(NULL != deployment_data);

    /* Write deployment data */
    if (nvs_write(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEPLOYMENT_DATA, deployment_data, strlen(deployment_data) + 1) < 0) {
        mender_log_error("Unable to write deployment data");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_get_deployment_data(char **deployment_data) {

    assert(NULL != deployment_data);
    size_t  deployment_data_length = 0;
    ssize_t ret;

    /* Retrieve length of the deployment data */
    if ((ret = nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEPLOYMENT_DATA, NULL, 0)) <= 0) {
        mender_log_info("Deployment data not available");
        return MENDER_NOT_FOUND;
    }
    deployment_data_length = (size_t)ret;

    /* Allocate memory to copy deployment data */
    if (NULL == (*deployment_data = (char *)malloc(deployment_data_length))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Read deployment data */
    if (nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEPLOYMENT_DATA, *deployment_data, deployment_data_length) < 0) {
        mender_log_error("Unable to read deployment data");
        free(*deployment_data);
        *deployment_data = NULL;
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_delete_deployment_data(void) {

    /* Delete deployment data */
    if (0 != nvs_delete(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEPLOYMENT_DATA)) {
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

    /* Write device configuration */
    if (nvs_write(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEVICE_CONFIG, device_config, strlen(device_config) + 1) < 0) {
        mender_log_error("Unable to write device configuration");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_get_device_config(char **device_config) {

    assert(NULL != device_config);
    size_t  device_config_length = 0;
    ssize_t ret;

    /* Retrieve length of the device configuration */
    if ((ret = nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEVICE_CONFIG, NULL, 0)) <= 0) {
        mender_log_info("Device configuration not available");
        return MENDER_NOT_FOUND;
    }
    device_config_length = (size_t)ret;

    /* Allocate memory to copy device configuration */
    if (NULL == (*device_config = (char *)malloc(device_config_length))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Read device_configuration */
    if (nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEVICE_CONFIG, *device_config, device_config_length) < 0) {
        mender_log_error("Unable to read device configuration");
        free(*device_config);
        *device_config = NULL;
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_delete_device_config(void) {

    /* Delete device configuration */
    if (0 != nvs_delete(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEVICE_CONFIG)) {
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
