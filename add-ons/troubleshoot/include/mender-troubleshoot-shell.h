/**
 * @file      mender-troubleshoot-shell.h
 * @brief     Mender MCU Troubleshoot add-on shell messages handler implementation
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

#ifndef __MENDER_TROUBLESHOOT_SHELL_H__
#define __MENDER_TROUBLESHOOT_SHELL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-troubleshoot-protomsg.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL

/**
 * @brief Mender troubleshoot shell configuration
 */
typedef struct {
    int32_t healthcheck_interval; /**< Troubleshoot healthcheck interval, default is 30 seconds, -1 permits to disable periodic execution */
} mender_troubleshoot_shell_config_t;

/**
 * @brief Mender troubleshoot shell callbacks
 */
typedef struct {
    mender_err_t (*open)(uint16_t, uint16_t);   /**< Invoked when shell is connected */
    mender_err_t (*resize)(uint16_t, uint16_t); /**< Invoked when shell is resized */
    mender_err_t (*write)(void *, size_t);      /**< Invoked when shell data is received */
    mender_err_t (*close)(void);                /**< Invoked when shell is disconnected */
} mender_troubleshoot_shell_callbacks_t;

/**
 * @brief Initialize mender troubleshoot add-on shell handler
 * @param config Mender troubleshoot shell configuration
 * @param callbacks Mender troubleshoot shell callbacks
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_shell_init(mender_troubleshoot_shell_config_t *config, mender_troubleshoot_shell_callbacks_t *callbacks);

/**
 * @brief Function called to perform the treatment of the shell messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
mender_err_t mender_troubleshoot_shell_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Mender troubleshoot shell handler healthcheck
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_shell_healthcheck(void);

/**
 * @brief Send shell data to the server
 * @param data Data to send to the server for printing in the console
 * @param length Length of data to send to the server
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_shell_print(void *data, size_t length);

/**
 * @brief Close mender troubleshoot add-on shell connection
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_shell_close(void);

/**
 * @brief Release mender troubleshoot add-on shell handler
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_shell_exit(void);

#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_TROUBLESHOOT_SHELL_H__ */
