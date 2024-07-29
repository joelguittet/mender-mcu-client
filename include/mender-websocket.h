/**
 * @file      mender-websocket.h
 * @brief     Mender websocket interface
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

#ifndef __MENDER_WEBSOCKET_H__
#define __MENDER_WEBSOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Mender websocket configuration
 */
typedef struct {
    char *host; /**< URL of the mender server */
} mender_websocket_config_t;

/**
 * @brief Websocket client events
 */
typedef enum {
    MENDER_WEBSOCKET_EVENT_CONNECTED,     /**< Connected to the server */
    MENDER_WEBSOCKET_EVENT_DATA_RECEIVED, /**< Data received from the server */
    MENDER_WEBSOCKET_EVENT_DISCONNECTED,  /**< Disconnected from the server */
    MENDER_WEBSOCKET_EVENT_ERROR          /**< An error occurred */
} mender_websocket_client_event_t;
/**
 * @brief Initialize mender websocket
 * @param config Mender websocket configuration
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_websocket_init(mender_websocket_config_t *config);

/**
 * @brief Perform HTTP request and upgrade connection to websocket
 * @param jwt Token, NULL if not authenticated yet
 * @param path Path of the request
 * @param callback Callback invoked on websocket events
 * @param params Parameters passed to the callback, NULL if not used
 * @param handle Websocket connection handle
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_websocket_connect(
    char *jwt, char *path, mender_err_t (*callback)(mender_websocket_client_event_t, void *, size_t, void *), void *params, void **handle);

/**
 * @brief Send binary data over websocket connection
 * @param handle Websocket connection handle
 * @param payload Payload to send
 * @param length Length of the payload
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_websocket_send(void *handle, void *payload, size_t length);

/**
 * @brief Close the websocket connection
 * @param handle Websocket connection handle
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_websocket_disconnect(void *handle);

/**
 * @brief Release mender websocket
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_websocket_exit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_WEBSOCKET_H__ */
