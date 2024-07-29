/**
 * @file      mender-flash.h
 * @brief     Mender flash interface, to be used in mender client callbacks (but you can also have your owns)
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

#ifndef __MENDER_FLASH_H__
#define __MENDER_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Open flash device
 * @param name Name of the artifact
 * @param size Size of the artifact
 * @param handle Handle of the deployment to be used with mender flash functions
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_flash_open(char *name, size_t size, void **handle);

/**
 * @brief Write deployment data
 * @param handle Handle from mender_flash_open
 * @param data Data to be written
 * @param index Index of the data to be written
 * @param length Length of the data to be written
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_flash_write(void *handle, void *data, size_t index, size_t length);

/**
 * @brief Close flash device
 * @param handle Handle from mender_flash_open
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_flash_close(void *handle);

/**
 * @brief Set new boot partition to be used at the next boot
 * @param handle Handle from mender_flash_open
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_flash_set_pending_image(void *handle);

/**
 * @brief Abort current deployment
 * @param handle Handle from mender_flash_open
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_flash_abort_deployment(void *handle);

/**
 * @brief Mark image valid and cancel rollback if this is still pending
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_flash_confirm_image(void);

/**
 * @brief Check if image is confirmed
 * @return true if the image is confirmed, false otherwise
 */
bool mender_flash_is_image_confirmed(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_FLASH_H__ */
