/**
 * @file      mender-ota.c
 * @brief     Mender OTA interface for Posix platform
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

#include <errno.h>
#include <unistd.h>
#include "mender-log.h"
#include "mender-ota.h"

/**
 * @brief Default OTA path (working directory)
 */
#ifndef CONFIG_MENDER_OTA_PATH
#define CONFIG_MENDER_OTA_PATH ""
#endif /* CONFIG_MENDER_OTA_PATH */

/**
 * @brief OTA Files
 */
#define MENDER_OTA_REQUEST_UPGRADE CONFIG_MENDER_OTA_PATH "request_upgrade"

mender_err_t
mender_ota_begin(char *name, size_t size, void **handle) {

    assert(NULL != name);
    assert(NULL != handle);
    char *path = NULL;

    /* Print current file name and size */
    mender_log_info("Start flashing OTA artifact '%s' with size %d", name, size);

    /* Compute path */
    if (NULL == (path = (char *)malloc(strlen(CONFIG_MENDER_OTA_PATH) + strlen(name) + 1))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    sprintf(path, "%s%s", CONFIG_MENDER_OTA_PATH, name);

    /* Begin OTA with sequential writes */
    if (NULL == (*handle = fopen(path, "wb"))) {
        mender_log_error("fopen failed (%d)", errno);
        free(path);
        return MENDER_FAIL;
    }

    /* Release memory */
    free(path);

    return MENDER_OK;
}

mender_err_t
mender_ota_write(void *handle, void *data, size_t index, size_t length) {

    (void)index;

    /* Check OTA handle */
    if (NULL == handle) {
        mender_log_error("Invalid OTA handle");
        return MENDER_FAIL;
    }

    /* Write data received to the update file */
    if (fwrite(data, sizeof(unsigned char), length, handle) != length) {
        mender_log_error("fwrite failed (%d)", length);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_ota_abort(void *handle) {

    /* Check OTA handle */
    if (NULL != handle) {

        /* Release memory */
        fclose(handle);
    }

    return MENDER_OK;
}

mender_err_t
mender_ota_end(void *handle) {

    /* Check OTA handle */
    if (NULL == handle) {
        mender_log_error("Invalid OTA handle");
        return MENDER_FAIL;
    }

    /* Close update file */
    fclose(handle);

    return MENDER_OK;
}

mender_err_t
mender_ota_set_boot_partition(void *handle) {

    FILE *       file;
    mender_err_t ret = MENDER_OK;

    /* Check OTA handle */
    if (NULL != handle) {

        /* Write request update file */
        if (NULL == (file = fopen(MENDER_OTA_REQUEST_UPGRADE, "wb"))) {
            mender_log_error("fopen failed (%d)", errno);
            ret = MENDER_FAIL;
        } else {
            fclose(file);
        }
    }

    return ret;
}

mender_err_t
mender_ota_mark_app_valid_cancel_rollback(void) {

    mender_err_t ret = MENDER_OK;

    /* Validate the image if it is still pending */
    if (!access(MENDER_OTA_REQUEST_UPGRADE, F_OK)) {
        /* Remove request upgrade file */
        if (0 != unlink(MENDER_OTA_REQUEST_UPGRADE)) {
            mender_log_error("Unable to mark application valid, application will rollback (%d)", errno);
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
    if (!access(MENDER_OTA_REQUEST_UPGRADE, F_OK)) {
        exit(EXIT_FAILURE);
    }

    return MENDER_OK;
}
