/**
 * @file      mender-troubleshoot.h
 * @brief     Mender MCU Troubleshoot add-on implementation
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

#ifndef __MENDER_TROUBLESHOOT_H__
#define __MENDER_TROUBLESHOOT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

#include "mender-utils.h"

/**
 * @brief Mender troubleshoot configuration
 */
typedef struct {
    uint32_t healthcheck_interval; /**< Troubleshoot healthcheck interval, default is 30 seconds */
} mender_troubleshoot_config_t;

/**
 * @brief Mender troubleshoot callbacks
 */
typedef struct {
    mender_err_t (*shell_begin)(uint16_t, uint16_t);  /**< Invoked when shell is connected */
    mender_err_t (*shell_resize)(uint16_t, uint16_t); /**< Invoked when shell is resized */
    mender_err_t (*shell_write)(uint8_t *, size_t);   /**< Invoked when shell data is received */
    mender_err_t (*shell_end)(void);                  /**< Invoked when shell is disconnected */
} mender_troubleshoot_callbacks_t;

/**
 * @brief Initialize mender troubleshoot add-on
 * @param config Mender troubleshoot configuration
 * @param callbacks Mender troubleshoot callbacks
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_init(mender_troubleshoot_config_t *config, mender_troubleshoot_callbacks_t *callbacks);

/**
 * @brief Activate mender troubleshoot add-on
 * @note This function connects the device to the server
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_activate(void);

/**
 * @brief Deactivate mender troubleshoot add-on
 * @note This function disconnects the device of the server
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_deactivate(void);

/**
 * @brief Send shell data to the server
 * @param data Data to send to the server for printing in the console
 * @param length Length of data to send to the server
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_shell_print(uint8_t *data, size_t length);

/**
 * @brief Release mender troubleshoot add-on
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_exit(void);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_TROUBLESHOOT_H__ */
