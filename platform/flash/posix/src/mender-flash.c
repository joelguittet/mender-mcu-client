/**
 * @file      mender-flash.c
 * @brief     Mender flash interface for Posix platform
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
#include "mender-flash.h"
#include "mender-log.h"

/**
 * @brief Default deployment path (working directory)
 */
#ifndef CONFIG_MENDER_FLASH_PATH
#define CONFIG_MENDER_FLASH_PATH ""
#endif /* CONFIG_MENDER_FLASH_PATH */

/**
 * @brief Deployment files
 */
#define MENDER_FLASH_REQUEST_UPGRADE CONFIG_MENDER_FLASH_PATH "request_upgrade"

mender_err_t
mender_flash_open(char *name, size_t size, void **handle) {

    assert(NULL != name);
    assert(NULL != handle);
    char *path = NULL;

    /* Print current file name and size */
    mender_log_info("Start flashing artifact '%s' with size %d", name, size);

    /* Compute path */
    size_t str_length = strlen(CONFIG_MENDER_FLASH_PATH) + strlen(name) + 1;
    if (NULL == (path = (char *)malloc(str_length))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    snprintf(path, str_length, "%s%s", CONFIG_MENDER_FLASH_PATH, name);

    /* Begin deployment with sequential writes */
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
mender_flash_write(void *handle, void *data, size_t index, size_t length) {

    (void)index;

    /* Check flash handle */
    if (NULL == handle) {
        mender_log_error("Invalid flash handle");
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
mender_flash_close(void *handle) {

    /* Check flash handle */
    if (NULL == handle) {
        mender_log_error("Invalid flash handle");
        return MENDER_FAIL;
    }

    /* Close update file */
    fclose(handle);

    return MENDER_OK;
}

mender_err_t
mender_flash_set_pending_image(void *handle) {

    FILE        *file;
    mender_err_t ret = MENDER_OK;

    /* Check flash handle */
    if (NULL != handle) {

        /* Write request update file */
        if (NULL == (file = fopen(MENDER_FLASH_REQUEST_UPGRADE, "wb"))) {
            mender_log_error("fopen failed (%d)", errno);
            ret = MENDER_FAIL;
        } else {
            fclose(file);
        }
    }

    return ret;
}

mender_err_t
mender_flash_abort_deployment(void *handle) {

    /* Check flash handle */
    if (NULL != handle) {

        /* Release memory */
        fclose(handle);
    }

    return MENDER_OK;
}

mender_err_t
mender_flash_confirm_image(void) {

    mender_err_t ret = MENDER_OK;

    /* Validate the image if it is still pending */
    if (false == mender_flash_is_image_confirmed()) {
        /* Remove request upgrade file */
        if (0 != unlink(MENDER_FLASH_REQUEST_UPGRADE)) {
            mender_log_error("Unable to mark application valid, application will rollback (%d)", errno);
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
    return (0 != access(MENDER_FLASH_REQUEST_UPGRADE, F_OK));
}
