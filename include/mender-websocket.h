/**
 * @file      mender-websocket.h
 * @brief     Mender websocket interface
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
