/**
 * @file      mender-troubleshoot.h
 * @brief     Mender MCU Troubleshoot add-on implementation
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

#ifndef __MENDER_TROUBLESHOOT_H__
#define __MENDER_TROUBLESHOOT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-addon.h"
#include "mender-utils.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

/**
 * @brief Mender troubleshoot instance
 */
extern const mender_addon_instance_t mender_troubleshoot_addon_instance;

/**
 * @brief Mender troubleshoot configuration
 */
typedef struct {
    int32_t healthcheck_interval; /**< Troubleshoot healthcheck interval, default is 30 seconds, -1 permits to disable periodic execution */
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
 * @param callbacks Mender troubleshoot callbacks (optional)
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_init(void *config, void *callbacks);

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
