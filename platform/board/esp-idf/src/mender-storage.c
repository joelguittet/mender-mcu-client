/**
 * @file      mender-storage.c
 * @brief     Mender storage interface for ESP-IDF platform
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

#include <nvs_flash.h>
#include "mender-log.h"
#include "mender-storage.h"

/**
 * @brief NVS keys
 */
#define MENDER_STORAGE_NVS_PRIVATE_KEY       "key.der"
#define MENDER_STORAGE_NVS_PUBLIC_KEY        "pubkey.der"
#define MENDER_STORAGE_NVS_OTA_ID            "id"
#define MENDER_STORAGE_NVS_OTA_ARTIFACT_NAME "artifact_name"
#define MENDER_STORAGE_NVS_DEVICE_CONFIG     "config.json"

/**
 * @brief NVS storage handle
 */
static nvs_handle_t mender_storage_nvs_handle;

mender_err_t
mender_storage_init(void) {

    /* Open NVS */
    if (ESP_OK != nvs_open("mender", NVS_READWRITE, &mender_storage_nvs_handle)) {
        mender_log_error("Unable to open NVS storage");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_set_authentication_keys(unsigned char *private_key, size_t private_key_length, unsigned char *public_key, size_t public_key_length) {

    assert(NULL != private_key);
    assert(NULL != public_key);

    /* Write keys */
    if ((ESP_OK != nvs_set_blob(mender_storage_nvs_handle, MENDER_STORAGE_NVS_PRIVATE_KEY, private_key, private_key_length))
        || (ESP_OK != nvs_set_blob(mender_storage_nvs_handle, MENDER_STORAGE_NVS_PUBLIC_KEY, public_key, public_key_length))) {
        mender_log_error("Unable to write authentication keys");
        return MENDER_FAIL;
    }
    if (ESP_OK != nvs_commit(mender_storage_nvs_handle)) {
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

    /* Retrieve length of the keys */
    nvs_get_blob(mender_storage_nvs_handle, MENDER_STORAGE_NVS_PRIVATE_KEY, NULL, private_key_length);
    nvs_get_blob(mender_storage_nvs_handle, MENDER_STORAGE_NVS_PUBLIC_KEY, NULL, public_key_length);
    if ((0 == *private_key_length) || (0 == *public_key_length)) {
        mender_log_info("Authentication keys are not available");
        return MENDER_NOT_FOUND;
    }

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
    if ((ESP_OK != nvs_get_blob(mender_storage_nvs_handle, MENDER_STORAGE_NVS_PRIVATE_KEY, *private_key, private_key_length))
        || (ESP_OK != nvs_get_blob(mender_storage_nvs_handle, MENDER_STORAGE_NVS_PUBLIC_KEY, *public_key, public_key_length))) {
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
    if ((ESP_OK != nvs_erase_key(mender_storage_nvs_handle, MENDER_STORAGE_NVS_PRIVATE_KEY))
        || (ESP_OK != nvs_erase_key(mender_storage_nvs_handle, MENDER_STORAGE_NVS_PUBLIC_KEY))) {
        mender_log_error("Unable to erase authentication keys");
        return MENDER_FAIL;
    }
    if (ESP_OK != nvs_commit(mender_storage_nvs_handle)) {
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
    if ((ESP_OK != nvs_set_str(mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ID, ota_id))
        || (ESP_OK != nvs_set_str(mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ARTIFACT_NAME, ota_artifact_name))) {
        mender_log_error("Unable to write OTA ID or artifact name");
        return MENDER_FAIL;
    }
    if (ESP_OK != nvs_commit(mender_storage_nvs_handle)) {
        mender_log_error("Unable to write OTA ID or artifact name");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_get_ota_deployment(char **ota_id, char **ota_artifact_name) {

    assert(NULL != ota_id);
    assert(NULL != ota_artifact_name);
    size_t ota_id_length            = 0;
    size_t ota_artifact_name_length = 0;

    /* Retrieve length of the ID and artifact name */
    nvs_get_str(mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ID, NULL, &ota_id_length);
    nvs_get_str(mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ARTIFACT_NAME, NULL, &ota_artifact_name_length);
    if ((0 == ota_id_length) || (0 == ota_artifact_name_length)) {
        mender_log_info("OTA ID or artifact name not available");
        return MENDER_NOT_FOUND;
    }

    /* Allocate memory to copy ID and artifact name */
    if (NULL == (*ota_id = (char *)malloc(ota_id_length + 1))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    if (NULL == (*ota_artifact_name = (char *)malloc(ota_artifact_name_length + 1))) {
        mender_log_error("Unable to allocate memory");
        free(*ota_id);
        *ota_id = NULL;
        return MENDER_FAIL;
    }

    /* Read ID and artifact name */
    if ((ESP_OK != nvs_get_str(mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ID, *ota_id, &ota_id_length))
        || (ESP_OK != nvs_get_str(mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ARTIFACT_NAME, *ota_artifact_name, &ota_artifact_name_length))) {
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
    if ((ESP_OK != nvs_erase_key(mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ID))
        || (ESP_OK != nvs_erase_key(mender_storage_nvs_handle, MENDER_STORAGE_NVS_OTA_ARTIFACT_NAME))) {
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
    if (ESP_OK != nvs_set_str(mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEVICE_CONFIG, device_config)) {
        mender_log_error("Unable to write device configuration");
        return MENDER_FAIL;
    }
    if (ESP_OK != nvs_commit(mender_storage_nvs_handle)) {
        mender_log_error("Unable to write device configuration");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_storage_get_device_config(char **device_config) {

    assert(NULL != device_config);
    size_t device_config_length = 0;

    /* Retrieve length of the device configuration */
    nvs_get_str(mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEVICE_CONFIG, NULL, &device_config_length);
    if (0 == device_config_length) {
        mender_log_info("Device configuration not available");
        return MENDER_NOT_FOUND;
    }

    /* Allocate memory to copy device configuration */
    if (NULL == (*device_config = (char *)malloc(device_config_length + 1))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Read device configuration */
    if (ESP_OK != nvs_get_str(mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEVICE_CONFIG, *device_config, &device_config_length)) {
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
    if (ESP_OK != nvs_erase_key(mender_storage_nvs_handle, MENDER_STORAGE_NVS_DEVICE_CONFIG)) {
        mender_log_error("Unable to delete device configuration");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

mender_err_t
mender_storage_exit(void) {

    /* Close NVS storage */
    nvs_close(mender_storage_nvs_handle);

    return MENDER_OK;
}
