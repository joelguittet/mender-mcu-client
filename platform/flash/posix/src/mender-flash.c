/**
 * @file      mender-flash.c
 * @brief     Mender flash interface for Posix platform
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
