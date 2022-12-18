/**
 * @file      mender-http.h
 * @brief     Mender HTTP interface
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

#ifndef __MENDER_HTTP_H__
#define __MENDER_HTTP_H__

#include "mender-common.h"

/**
 * @brief Mender HTTP configuration
 */
typedef struct {
    char *host; /**< URL of the mender server */
} mender_http_config_t;

/**
 * @brief HTTP methods
 */
typedef enum {
    MENDER_HTTP_GET,  /**< GET */
    MENDER_HTTP_POST, /**< POST */
    MENDER_HTTP_PUT,  /**< PUT */
    MENDER_HTTP_PATCH /**< PATCH */
} mender_http_method_t;

/**
 * @brief HTTP client events
 */
typedef enum {
    MENDER_HTTP_EVENT_CONNECTED,     /**< Connected to the server */
    MENDER_HTTP_EVENT_DATA_RECEIVED, /**< Data received from the server */
    MENDER_HTTP_EVENT_DISCONNECTED,  /**< Disconnected from the server */
    MENDER_HTTP_EVENT_ERROR          /**< An error occurred */
} mender_http_client_event_t;

/**
 * @brief Initialize mender http
 * @param config Mender HTTP configuration
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_http_init(mender_http_config_t *config);

/**
 * @brief Perform HTTP request
 * @param jwt Token, NULL if not authenticated yet
 * @param path Path of the request
 * @param method Method
 * @param payload Payload, NULL if empty
 * @param signature Signature of the payload, NULL if it is not required
 * @param callback Callback invoked on HTTP events
 * @param params Parameters passed to the callback, NULL if not used
 * @param status Status code
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_http_perform(char *               jwt,
                                 char *               path,
                                 mender_http_method_t method,
                                 char *               payload,
                                 char *               signature,
                                 mender_err_t (*callback)(mender_http_client_event_t, void *, size_t, void *),
                                 void *params,
                                 int * status);

/**
 * @brief Release mender http
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_http_exit(void);

#endif /* __MENDER_HTTP_H__ */
