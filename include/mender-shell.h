/**
 * @file      mender-shell.h
 * @brief     Mender shell interface, to be used in mender troubleshoot callbacks (but you can also have your owns)
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

#ifndef __MENDER_SHELL_H__
#define __MENDER_SHELL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Begin new shell
 * @param terminal_width Terminal width
 * @param terminal_height Terminal height
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_shell_begin(uint16_t terminal_width, uint16_t terminal_height);

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
 * @brief End shell
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_shell_end(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_SHELL_H__ */
