/**
 * @file      mender-flash.c
 * @brief     Mender flash interface for ESP-IDF platform
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

#include <esp_ota_ops.h>
#include "mender-flash.h"
#include "mender-log.h"

/**
 * @brief Flash handle
 */
typedef struct {
    const esp_partition_t *partition;  /**< Update partition to which the firmware is flashed */
    esp_ota_handle_t       ota_handle; /**< OTA handle used to flash the firmware */
} mender_flash_handle_t;

mender_err_t
mender_flash_open(char *name, size_t size, void **handle) {

    assert(NULL != name);
    assert(NULL != handle);
    esp_err_t err;

    /* Print current file name and size */
    mender_log_info("Start flashing artifact '%s' with size %d", name, size);

    /* Allocate memory to store the flash handle */
    if (NULL == (*handle = malloc(sizeof(mender_flash_handle_t)))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Check for the next update partition */
    if (NULL == (((mender_flash_handle_t *)(*handle))->partition = esp_ota_get_next_update_partition(NULL))) {
        mender_log_error("Unable to find next update partition");
        return MENDER_FAIL;
    }
    mender_log_info("Next update partition is '%s', subtype %d at offset 0x%x and with size %d",
                    ((mender_flash_handle_t *)(*handle))->partition->label,
                    ((mender_flash_handle_t *)(*handle))->partition->subtype,
                    ((mender_flash_handle_t *)(*handle))->partition->address,
                    ((mender_flash_handle_t *)(*handle))->partition->size);

    /* Begin OTA with sequential writes */
    if (ESP_OK
        != (err
            = esp_ota_begin(((mender_flash_handle_t *)(*handle))->partition, OTA_WITH_SEQUENTIAL_WRITES, &((mender_flash_handle_t *)(*handle))->ota_handle))) {
        mender_log_error("esp_ota_begin failed (%s)", esp_err_to_name(err));
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_write(void *handle, void *data, size_t index, size_t length) {

    (void)index;
    esp_err_t err;

    /* Check flash handle */
    if (NULL == handle) {
        mender_log_error("Invalid flash handle");
        return MENDER_FAIL;
    }

    /* Write data received to the update partition */
    if (ESP_OK != (err = esp_ota_write(((mender_flash_handle_t *)handle)->ota_handle, data, length))) {
        mender_log_error("esp_ota_write failed (%s)", esp_err_to_name(err));
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_abort_deployment(void *handle) {

    /* Check flash handle */
    if (NULL != handle) {

        /* Abort current deployment */
        esp_ota_abort(((mender_flash_handle_t *)handle)->ota_handle);

        /* Release memory */
        free(handle);
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_close(void *handle) {

    esp_err_t err;

    /* Check flash handle */
    if (NULL == handle) {
        mender_log_error("Invalid flash handle");
        return MENDER_FAIL;
    }

    /* Ending current deployment */
    if (ESP_OK != (err = esp_ota_end(((mender_flash_handle_t *)handle)->ota_handle))) {
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
mender_flash_set_pending_image(void *handle) {

    esp_err_t err;

    /* Check flash handle */
    if (NULL != handle) {

        /* Set new boot partition */
        if (ESP_OK != (err = esp_ota_set_boot_partition(((mender_flash_handle_t *)handle)->partition))) {
            mender_log_error("esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
            return MENDER_FAIL;
        }

        /* Release memory */
        free(handle);
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_confirm_image(void) {

    mender_err_t ret = MENDER_OK;

    /* Validate the image if it is still pending */
    if (false == mender_flash_is_image_confirmed()) {
        if (ESP_OK != esp_ota_mark_app_valid_cancel_rollback()) {
            mender_log_error("Unable to mark application valid, application will rollback");
            ret = MENDER_FAIL;
        } else {
            mender_log_info("Application has been mark valid and rollback canceled");
        }
    }

    return ret;
}

bool
mender_flash_is_image_confirmed(void) {

    /* Retrieve running version state of the ESP32 */
    esp_ota_img_states_t   img_state;
    const esp_partition_t *partition = esp_ota_get_running_partition();
    if (ESP_OK != esp_ota_get_state_partition(partition, &img_state)) {
        mender_log_error("Unable to get running version state");
        return false;
    }

    /* Check if the image is still pending */
    return (ESP_OTA_IMG_VALID == img_state);
}
