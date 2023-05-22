/**
 * @file      mender-storage.c
 * @brief     Mender storage interface for Zephyr platform
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
#define MENDER_STORAGE_NVS_PRIVATE_KEY       1
#define MENDER_STORAGE_NVS_PUBLIC_KEY        2
#define MENDER_STORAGE_NVS_OTA_ID            3
#define MENDER_STORAGE_NVS_OTA_ARTIFACT_NAME 4
#define MENDER_STORAGE_NVS_DEVICE_CONFIG     5

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
    mender_storage_nvs_handle.sector_count = CONFIG_MENDER_STORAGE_SECTOR_COUNT;

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
    if (NULL == (*private_key = malloc(*private_key_length))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    if (NULL == (*public_key = malloc(*public_key_length))) {
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
mender_storage_set_ota_deployment(char *ota_id, char *ota_artifact_name) {

    assert(NULL != ota_id);
    assert(NULL != ota_artifact_name);

    /* Write ID and artifact name */
    if ((nvs_write(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ID, ota_id, strlen(ota_id) + 1) < 0)
        || (nvs_write(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ARTIFACT_NAME, ota_artifact_name, strlen(ota_artifact_name) + 1) < 0)) {
        mender_log_error("Unable to write OTA ID or artifact name");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_get_ota_deployment(char **ota_id, char **ota_artifact_name) {

    assert(NULL != ota_id);
    assert(NULL != ota_artifact_name);
    size_t  ota_id_length            = 0;
    size_t  ota_artifact_name_length = 0;
    ssize_t ret;

    /* Retrieve length of the ID and artifact name */
    if ((ret = nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ID, NULL, 0)) <= 0) {
        mender_log_info("OTA ID not available");
        return MENDER_NOT_FOUND;
    }
    ota_id_length = (size_t)ret;
    if ((ret = nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ARTIFACT_NAME, NULL, 0)) <= 0) {
        mender_log_info("Artifact name not available");
        return MENDER_NOT_FOUND;
    }
    ota_artifact_name_length = (size_t)ret;

    /* Allocate memory to copy ID and artifact name */
    if (NULL == (*ota_id = malloc(ota_id_length))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    if (NULL == (*ota_artifact_name = malloc(ota_artifact_name_length))) {
        mender_log_error("Unable to allocate memory");
        free(*ota_id);
        *ota_id = NULL;
        return MENDER_FAIL;
    }

    /* Read ID and artifact name */
    if ((nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ID, *ota_id, ota_id_length) < 0)
        || (nvs_read(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ARTIFACT_NAME, *ota_artifact_name, ota_artifact_name_length) < 0)) {
        mender_log_error("Unable to read OTA ID or artifact name");
        free(*ota_id);
        *ota_id = NULL;
        free(*ota_artifact_name);
        *ota_artifact_name = NULL;
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_delete_ota_deployment(void) {

    /* Delete ID and artifact name */
    if ((0 != nvs_delete(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ID))
        || (0 != nvs_delete(&mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ARTIFACT_NAME))) {
        mender_log_error("Unable to delete OTA ID or artifact name");
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
    if (NULL == (*device_config = malloc(device_config_length))) {
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

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */
#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

mender_err_t
mender_storage_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}
