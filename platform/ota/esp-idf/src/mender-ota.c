/**
 * @file      mender-ota.c
 * @brief     Mender OTA interface for ESP-IDF platform
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

#include <esp_ota_ops.h>
#include "mender-log.h"
#include "mender-ota.h"

/**
 * @brief OTA handle
 */
typedef struct {
    esp_partition_t *partition;  /**< Update partition to which the firmware is flashed */
    esp_ota_handle_t ota_handle; /**< OTA handle used to flash the firmware */
} mender_ota_handle_t;

mender_err_t
mender_ota_begin(char *name, size_t size, void **handle) {

    assert(NULL != name);
    assert(NULL != handle);
    esp_err_t err;

    /* Print current file name and size */
    mender_log_info("Start flashing OTA artifact '%s' with size %d", name, size);

    /* Allocate memory to store the OTA handle */
    if (NULL == (*handle = malloc(sizeof(mender_ota_handle_t)))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Check for the next update partition */
    if (NULL == (((mender_ota_handle_t *)(*handle))->partition = (esp_partition_t *)esp_ota_get_next_update_partition(NULL))) {
        mender_log_error("Unable to find next update partition");
        return MENDER_FAIL;
    }
    mender_log_info("Next update partition is '%s', subtype %d at offset 0x%x and with size %d",
                    ((mender_ota_handle_t *)(*handle))->partition->label,
                    ((mender_ota_handle_t *)(*handle))->partition->subtype,
                    ((mender_ota_handle_t *)(*handle))->partition->address,
                    ((mender_ota_handle_t *)(*handle))->partition->size);

    /* Begin OTA with sequential writes */
    if (ESP_OK
        != (err = esp_ota_begin(((mender_ota_handle_t *)(*handle))->partition, OTA_WITH_SEQUENTIAL_WRITES, &((mender_ota_handle_t *)(*handle))->ota_handle))) {
        mender_log_error("esp_ota_begin failed (%s)", esp_err_to_name(err));
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_ota_write(void *handle, void *data, size_t index, size_t length) {

    (void)index;
    esp_err_t err;

    /* Check OTA handle */
    if (NULL == handle) {
        mender_log_error("Invalid OTA handle");
        return MENDER_FAIL;
    }

    /* Write data received to the update partition */
    if (ESP_OK != (err = esp_ota_write(((mender_ota_handle_t *)handle)->ota_handle, data, length))) {
        mender_log_error("esp_ota_write failed (%s)", esp_err_to_name(err));
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_ota_abort(void *handle) {

    /* Check OTA handle */
    if (NULL != handle) {

        /* Abort current OTA */
        esp_ota_abort(((mender_ota_handle_t *)handle)->ota_handle);

        /* Release memory */
        free(handle);
    }

    return MENDER_OK;
}

mender_err_t
mender_ota_end(void *handle) {

    esp_err_t err;

    /* Check OTA handle */
    if (NULL == handle) {
        mender_log_error("Invalid OTA handle");
        return MENDER_FAIL;
    }

    /* Ending current OTA */
    if (ESP_OK != (err = esp_ota_end(((mender_ota_handle_t *)handle)->ota_handle))) {
        if (ESP_ERR_OTA_VALIDATE_FAILED == err) {
            mender_log_error("Image validation failed, image is corrupted");
        } else {
            mender_log_error("esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_ota_set_boot_partition(void *handle) {

    esp_err_t err;

    /* Check OTA handle */
    if (NULL != handle) {

        /* Set new boot partition */
        if (ESP_OK != (err = esp_ota_set_boot_partition(((mender_ota_handle_t *)handle)->partition))) {
            mender_log_error("esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
            return MENDER_FAIL;
        }

        /* Release memory */
        free(handle);
    }

    return MENDER_OK;
}

mender_err_t
mender_ota_mark_app_valid_cancel_rollback(void) {

    mender_err_t ret = MENDER_OK;

    /* Retrieve running version state of the ESP32 */
    esp_ota_img_states_t   img_state;
    const esp_partition_t *partition = esp_ota_get_running_partition();
    if (ESP_OK != esp_ota_get_state_partition(partition, &img_state)) {
        mender_log_error("Unable to get running version state");
    } else {
        /* Validate the image if it is still pending */
        if (ESP_OTA_IMG_PENDING_VERIFY == img_state) {
            if (ESP_OK != esp_ota_mark_app_valid_cancel_rollback()) {
                mender_log_error("Unable to mark application valid, application will rollback");
                ret = MENDER_FAIL;
            } else {
                mender_log_info("Application has been mark valid and rollback canceled");
            }
        }
    }

    return ret;
}

mender_err_t
mender_ota_mark_app_invalid_rollback_and_reboot(void) {

    mender_err_t ret = MENDER_OK;

    /* Retrieve running version state of the ESP32 */
    esp_ota_img_states_t   img_state;
    const esp_partition_t *partition = esp_ota_get_running_partition();
    if (ESP_OK != esp_ota_get_state_partition(partition, &img_state)) {
        mender_log_error("Unable to get running version state");
    } else {
        /* Invalidate the image if it is still pending */
        if (ESP_OTA_IMG_PENDING_VERIFY == img_state) {
            if (ESP_OK != esp_ota_mark_app_invalid_rollback_and_reboot()) {
                mender_log_error("Unable to mark application invalid, application will not reboot");
                ret = MENDER_FAIL;
            } else {
                mender_log_info("Application has been mark invalid and reboot scheduled");
            }
        }
    }

    return ret;
}
