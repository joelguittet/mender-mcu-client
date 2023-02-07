/**
 * @file      mender-ota.c
 * @brief     Mender OTA interface for Zephyr platform
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

#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/sys/reboot.h>
#include "mender-log.h"
#include "mender-ota.h"

mender_err_t
mender_ota_begin(char *name, size_t size, void **handle) {

    int result;

    /* Print current file name and size */
    mender_log_info("Start flashing OTA artifact '%s' with size %d", name, size);

    /* Begin OTA with sequential writes */
    if (NULL == (*handle = malloc(sizeof(struct flash_img_context)))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    if ((result = flash_img_init((struct flash_img_context *)*handle)) < 0) {
        mender_log_error("flash_img_init failed (%d)", result);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_ota_write(void *handle, void *data, size_t data_length) {

    assert(NULL != handle);
    int result;

    /* Write data received to the update partition */
    if ((result = flash_img_buffered_write((struct flash_img_context *)handle, (const uint8_t *)data, data_length, false)) < 0) {
        mender_log_error("flash_img_buffered_write failed (%d)", result);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_ota_abort(void *handle) {

    assert(NULL != handle);

    /* Release memory */
    free(handle);

    return MENDER_OK;
}

mender_err_t
mender_ota_end(void *handle) {

    assert(NULL != handle);
    int result;

    /* Flush data received to the update partition */
    if ((result = flash_img_buffered_write((struct flash_img_context *)handle, NULL, 0, true)) < 0) {
        mender_log_error("flash_img_buffered_write failed (%d)", result);
        return MENDER_FAIL;
    }

    /* Release memory */
    free(handle);

    return MENDER_OK;
}

mender_err_t
mender_ota_set_boot_partition(void) {

    int result;

    /* Set new boot partition */
    if ((result = boot_request_upgrade(BOOT_UPGRADE_TEST)) < 0) {
        mender_log_error("boot_request_upgrade failed (%d)", result);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_ota_mark_app_valid_cancel_rollback(void) {

    int          result;
    mender_err_t ret = MENDER_OK;

    /* Validate the image if it is still pending */
    if (!boot_is_img_confirmed()) {
        if ((result = boot_write_img_confirmed()) < 0) {
            mender_log_error("Unable to mark application valid, application will rollback (%d)", result);
            ret = MENDER_FAIL;
        } else {
            mender_log_info("Application has been mark valid and rollback canceled");
        }
    }

    return ret;
}

mender_err_t
mender_ota_mark_app_invalid_rollback_and_reboot(void) {

    /* Reboot if validating the image it still pending */
    if (!boot_is_img_confirmed()) {
        sys_reboot(SYS_REBOOT_WARM);
    }

    return MENDER_OK;
}
