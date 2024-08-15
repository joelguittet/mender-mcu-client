/**
 * @file      mender-troubleshoot-api.h
 * @brief     Mender MCU Troubleshoot add-on API implementation
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

#ifndef __MENDER_TROUBLESHOOT_API_H__
#define __MENDER_TROUBLESHOOT_API_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

/**
 * @brief Mender troubleshoot API configuration
 */
typedef struct {
    char *host; /**< URL of the mender server */
} mender_troubleshoot_api_config_t;

/**
 * @brief Initialization of the API
 * @param config Mender troubleshoot API configuration
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_api_init(mender_troubleshoot_api_config_t *config);

/**
 * @brief Connect the device and make it available to the server
 * @param callback Callback function to be invoked to perform the treatment of the data from the websocket
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_api_connect(mender_err_t (*callback)(void *, size_t));

/**
 * @brief Check if the device troubleshoot add-on is connected to the server
 * @return true if the add-on is connected to the server, false otherwise
 */
bool mender_troubleshoot_api_is_connected(void);

/**
 * @brief Send binary data to the server
 * @param payload Payload to send
 * @param length Length of the payload
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_api_send(void *payload, size_t length);

/**
 * @brief Disconnect the device
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_api_disconnect(void);

/**
 * @brief Release mender troubleshoot API
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_api_exit(void);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_TROUBLESHOOT_API_H__ */
