/**
 * @file      mender-shell.h
 * @brief     Mender shell interface, to be used in mender troubleshoot callbacks (but you can also have your owns)
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

#ifndef __MENDER_SHELL_H__
#define __MENDER_SHELL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Open a new shell
 * @param terminal_width Terminal width
 * @param terminal_height Terminal height
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_shell_open(uint16_t terminal_width, uint16_t terminal_height);

/**
 * @brief Resize the shell
 * @param terminal_width Terminal width
 * @param terminal_height Terminal height
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_shell_resize(uint16_t terminal_width, uint16_t terminal_height);

/**
 * @brief Write data to the shell
 * @param data Data to write
 * @param length Length of the data
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_shell_write(uint8_t *data, size_t length);

/**
 * @brief Close shell
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_shell_close(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_SHELL_H__ */
