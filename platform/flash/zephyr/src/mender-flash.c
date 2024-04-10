/**
 * @file      mender-flash.c
 * @brief     Mender flash interface for Zephyr platform
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
#include "mender-flash.h"
#include "mender-log.h"

mender_err_t
mender_flash_open(char *name, size_t size, void **handle) {

    assert(NULL != name);
    assert(NULL != handle);
    int result;

    /* Print current file name and size */
    mender_log_info("Start flashing artifact '%s' with size %d", name, size);

    /* Allocate memory to store the flash handle */
    if (NULL == (*handle = malloc(sizeof(struct flash_img_context)))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Begin deployment with sequential writes */
    if ((result = flash_img_init((struct flash_img_context *)*handle)) < 0) {
        mender_log_error("flash_img_init failed (%d)", result);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_write(void *handle, void *data, size_t index, size_t length) {

    (void)index;
    int result;

    /* Check flash handle */
    if (NULL == handle) {
        mender_log_error("Invalid flash handle");
        return MENDER_FAIL;
    }

    /* Write data received to the update partition */
    if ((result = flash_img_buffered_write((struct flash_img_context *)handle, (const uint8_t *)data, length, false)) < 0) {
        mender_log_error("flash_img_buffered_write failed (%d)", result);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_close(void *handle) {

    int result;

    /* Check flash handle */
    if (NULL == handle) {
        mender_log_error("Invalid flash handle");
        return MENDER_FAIL;
    }

    /* Flush data received to the update partition */
    if ((result = flash_img_buffered_write((struct flash_img_context *)handle, NULL, 0, true)) < 0) {
        mender_log_error("flash_img_buffered_write failed (%d)", result);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_set_pending_image(void *handle) {

    int result;

    /* Check flash handle */
    if (NULL != handle) {

        /* Set new boot partition */
        if ((result = boot_request_upgrade(BOOT_UPGRADE_TEST)) < 0) {
            mender_log_error("boot_request_upgrade failed (%d)", result);
            return MENDER_FAIL;
        }

        /* Release memory */
        free(handle);
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_abort_deployment(void *handle) {

    /* Check flash handle */
    if (NULL != handle) {

        /* Release memory */
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
