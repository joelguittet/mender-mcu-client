/**
 * @file      mender-ota.h
 * @brief     Mender OTA interface, to be used in mender client callbacks (but you can also have your owns)
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

#ifndef __MENDER_OTA_H__
#define __MENDER_OTA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Begin new OTA deployment
 * @param name Name of the artifact
 * @param size Size of the artifact
 * @param handle Handle of the deployment to be used with mender OTA functions
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_ota_begin(char *name, size_t size, void **handle);

/**
 * @brief Write OTA data
 * @param handle Handle from mender_ota_begin
 * @param data Data to be written
 * @param index Index of the data to be written
 * @param length Length of the data to be written
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_ota_write(void *handle, void *data, size_t index, size_t length);

/**
 * @brief Abort OTA
 * @param handle Handle from mender_ota_begin
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_ota_abort(void *handle);

/**
 * @brief End OTA
 * @param handle Handle from mender_ota_begin
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_ota_end(void *handle);

/**
 * @brief Set new boot partition to be used at the next boot
 * @param handle Handle from mender_ota_begin
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_ota_set_boot_partition(void *handle);

/**
 * @brief Mark application valid and cancel rollback if this is still pending
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_ota_mark_app_valid_cancel_rollback(void);

/**
 * @brief Mark application invalid and perform rollback if this is still pending
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_ota_mark_app_invalid_rollback_and_reboot(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_OTA_H__ */
