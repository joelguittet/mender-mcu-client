/**
 * @file      mender-flash.c
 * @brief     Mender flash interface for Zephyr platform
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

#include <dfu/dfu_target.h>
#include <dfu/dfu_target_mcuboot.h>
#include <zephyr/dfu/mcuboot.h>
#include "mender-flash.h"
#include "mender-log.h"

/**
 * @brief MCUboot buffer size (Bytes)
 */
#ifndef CONFIG_MENDER_FLASH_MCUBOOT_BUFFER_SIZE
#define CONFIG_MENDER_FLASH_MCUBOOT_BUFFER_SIZE (512)
#endif /* CONFIG_MENDER_FLASH_MCUBOOT_BUFFER_SIZE */

/**
 * @brief MCUboot image number
 */
#ifndef CONFIG_MENDER_FLASH_MCUBOOT_IMAGE_NUMBER
#define CONFIG_MENDER_FLASH_MCUBOOT_IMAGE_NUMBER (0)
#endif /* CONFIG_MENDER_FLASH_MCUBOOT_IMAGE_NUMBER */

mender_err_t
mender_flash_open(char *name, size_t size, void **handle) {

    assert(NULL != name);
    assert(NULL != handle);
    int result;

    /* Print current file name and size */
    mender_log_info("Start flashing artifact '%s' with size %d", name, size);

    /* Allocate memory to store the MCUboot buffer */
    if (NULL == (*handle = malloc(CONFIG_MENDER_FLASH_MCUBOOT_BUFFER_SIZE))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Set MCUboot buffer */
    if (0 != (result = dfu_target_mcuboot_set_buf(*handle, CONFIG_MENDER_FLASH_MCUBOOT_BUFFER_SIZE))) {
        mender_log_error("dfu_target_mcuboot_set_buf failed (%d)", result);
        return MENDER_FAIL;
    }

    /* Initialize DFU target */
    /* Note that the current implmentation only support upgrading MCUboot based applications */
    /* Other type of DFU targets could be supported later, or using a custom implmentation of mender-flash */
    /* The filename of the artefact can be used to determine the DFU target type */
    if (0 != (result = dfu_target_init(DFU_TARGET_IMAGE_TYPE_MCUBOOT, CONFIG_MENDER_FLASH_MCUBOOT_IMAGE_NUMBER, size, NULL))) {
        mender_log_error("dfu_target_init failed (%d)", result);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_write(void *handle, void *data, size_t index, size_t length) {

    (void)handle;
    (void)index;
    int result;

    /* Write data received to the update partition */
    if ((result = dfu_target_write(data, length)) < 0) {
        mender_log_error("dfu_target_write failed (%d)", result);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_close(void *handle) {

    (void)handle;
    int result;

    /* Release DFU target ressources */
    if (0 != (result = dfu_target_done(true))) {
        mender_log_error("dfu_target_done failed (%d)", result);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_set_pending_image(void *handle) {

    int result;

    /* Schedule the update */
    if (0 != (result = dfu_target_schedule_update(CONFIG_MENDER_FLASH_MCUBOOT_IMAGE_NUMBER))) {
        mender_log_error("dfu_target_schedule_update failed (%d)", result);
        return MENDER_FAIL;
    }

    /* Release memory */
    if (NULL != handle) {
        free(handle);
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_abort_deployment(void *handle) {

    int result;

    /* Abort deployment */
    if (0 != (result = dfu_target_reset())) {
        mender_log_error("dfu_target_reset failed (%d)", result);
        return MENDER_FAIL;
    }

    /* Release memory */
    if (NULL != handle) {
        free(handle);
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_confirm_image(void) {

    int          result;
    mender_err_t ret = MENDER_OK;

    /* Validate the image if it is still pending */
    if (false == mender_flash_is_image_confirmed()) {
        if ((result = boot_write_img_confirmed()) < 0) {
            mender_log_error("Unable to mark application valid, application will rollback (%d)", result);
            ret = MENDER_FAIL;
        } else {
            mender_log_info("Application has been mark valid and rollback canceled");
        }
    }

    return ret;
}

bool
mender_flash_is_image_confirmed(void) {

    /* Check if the image it still pending */
    return boot_is_img_confirmed();
}
