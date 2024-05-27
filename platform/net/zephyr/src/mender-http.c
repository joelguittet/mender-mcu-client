/**
 * @file      mender-http.c
 * @brief     Mender HTTP interface for Zephyr platform
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

#include <version.h>
#include <zephyr/net/http/client.h>
#include "mender-http.h"
#include "mender-log.h"
#include "mender-net.h"

/**
 * @brief HTTP User-Agent
 */
#define MENDER_HTTP_USER_AGENT "mender-mcu-client/" MENDER_CLIENT_VERSION " (mender-http) zephyr/" KERNEL_VERSION_STRING

/**
 * @brief Receive buffer length
 */
#define MENDER_HTTP_RECV_BUF_LENGTH (512)

/**
 * @brief Request timeout (milliseconds)
 */
#define MENDER_HTTP_REQUEST_TIMEOUT (600000)

/**
 * @brief Request context
 */
typedef struct {
    mender_err_t (*callback)(mender_http_client_event_t, void *, size_t, void *); /**< Callback to be invoked when data are received */
    void *       params;                                                          /**< Callback parameters */
    mender_err_t ret;                                                             /**< Last callback return value */
} mender_http_request_context;

/**
 * @brief Mender HTTP configuration
 */
static mender_http_config_t mender_http_config;

/**
 * @brief HTTP response callback, invoked to handle data received
 * @param response HTTP response structure
 * @param final_call Indicate final call
 * @param user_data User data, used to retrieve request context data
 */
static void mender_http_response_cb(struct http_response *response, enum http_final_call final_call, void *user_data);

/**
 * @brief Convert mender HTTP method to Zephyr HTTP client method
 * @param method Mender HTTP method
 * @return Zephyr HTTP client method if the function succeeds, -1 otherwise
 */
static enum http_method mender_http_method_to_zephyr_http_client_method(mender_http_method_t method);

mender_err_t
mender_http_init(mender_http_config_t *config) {

    assert(NULL != config);
    assert(NULL != config->host);

    /* Save configuration */
    memcpy(&mender_http_config, config, sizeof(mender_http_config_t));

    return MENDER_OK;
}

mender_err_t
mender_http_perform(char *               jwt,
                    char *               path,
                    mender_http_method_t method,
                    char *               payload,
                    char *               signature,
                    mender_err_t (*callback)(mender_http_client_event_t, void *, size_t, void *),
                    void *params,
                    int * status) {

    assert(NULL != path);
    assert(NULL != callback);
    assert(NULL != status);
    mender_err_t                ret;
    struct http_request         request;
    mender_http_request_context request_context;
    char *                      header_fields[5] = { NULL, NULL, NULL, NULL, NULL };
    size_t                      header_index     = 0;
    char *                      host             = NULL;
    char *                      port             = NULL;
    char *                      url              = NULL;
    int                         sock             = -1;

    /* Initialize request */
    memset(&request, 0, sizeof(struct http_request));

    /* Initialize request context */
    request_context.callback = callback;
    request_context.params   = params;
    request_context.ret      = MENDER_OK;

    /* Retrieve host, port and url */
    if (MENDER_OK != (ret = mender_net_get_host_port_url(path, mender_http_config.host, &host, &port, &url))) {
        mender_log_error("Unable to retrieve host/port/url");
        goto END;
    }

    /* Configuration of the client */
    request.method      = mender_http_method_to_zephyr_http_client_method(method);
    request.url         = url;
    request.host        = host;
    request.protocol    = "HTTP/1.1";
    request.payload     = payload;
    request.payload_len = (NULL != payload) ? strlen(payload) : 0;
    request.response    = mender_http_response_cb;
    if (NULL == (request.recv_buf = (uint8_t *)malloc(MENDER_HTTP_RECV_BUF_LENGTH))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    request.recv_buf_len = MENDER_HTTP_RECV_BUF_LENGTH;
    size_t str_length    = strlen("User-Agent: ") + strlen(MENDER_HTTP_USER_AGENT) + strlen("\r\n") + 1;
    if (NULL == (header_fields[header_index] = malloc(str_length))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    snprintf(header_fields[header_index], str_length, "User-Agent: %s\r\n", MENDER_HTTP_USER_AGENT);
    header_index++;
    if (NULL != jwt) {
        str_length = strlen("Authorization: Bearer ") + strlen(jwt) + strlen("\r\n") + 1;
        if (NULL == (header_fields[header_index] = (char *)malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        snprintf(header_fields[header_index], str_length, "Authorization: Bearer %s\r\n", jwt);
        header_index++;
    }
    if (NULL != signature) {
        str_length = strlen("X-MEN-Signature: ") + strlen(signature) + strlen("\r\n") + 1;
        if (NULL == (header_fields[header_index] = (char *)malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        snprintf(header_fields[header_index], str_length, "X-MEN-Signature: %s\r\n", signature);
        header_index++;
    }
    if (NULL != payload) {
        if (NULL == (header_fields[header_index] = strdup("Content-Type: application/json\r\n"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
    }
    request.header_fields = (0 != header_index) ? ((const char **)header_fields) : NULL;

    /* Connect to the server */
    if (MENDER_OK != (ret = mender_net_connect(host, port, &sock))) {
        mender_log_error("Unable to open HTTP client connection");
        goto END;
    }
    if (MENDER_OK != (ret = callback(MENDER_HTTP_EVENT_CONNECTED, NULL, 0, params))) {
        mender_log_error("An error occurred");
        goto END;
    }

    /* Perform HTTP request */
    if (http_client_req(sock, &request, MENDER_HTTP_REQUEST_TIMEOUT, (void *)&request_context) < 0) {
        mender_log_error("Unable to write data");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Check if an error occured during the treatment of data */
    if (MENDER_OK != (ret = request_context.ret)) {
        goto END;
    }

    /* Read HTTP status code */
    if (0 == request.internal.response.http_status_code) {
        mender_log_error("An error occurred, connection has been closed");
        callback(MENDER_HTTP_EVENT_ERROR, NULL, 0, params);
        ret = MENDER_FAIL;
        goto END;
    } else {
        *status = request.internal.response.http_status_code;
    }
    if (MENDER_OK != (ret = callback(MENDER_HTTP_EVENT_DISCONNECTED, NULL, 0, params))) {
        mender_log_error("An error occurred");
        goto END;
    }

END:

    /* Close connection */
    mender_net_disconnect(sock);

    /* Release memory */
    if (NULL != host) {
        free(host);
    }
    if (NULL != port) {
        free(port);
    }
    if (NULL != url) {
        free(url);
    }
    if (NULL != request.recv_buf) {
        free(request.recv_buf);
    }
    for (size_t index = 0; index < sizeof(header_fields) / sizeof(header_fields[0]); index++) {
        if (NULL != header_fields[index]) {
            free(header_fields[index]);
        }
    }

    return ret;
}

mender_err_t
mender_http_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}

static void
mender_http_response_cb(struct http_response *response, enum http_final_call final_call, void *user_data) {

    assert(NULL != response);
    (void)final_call;
    assert(NULL != user_data);

    /* Retrieve request context */
    mender_http_request_context *request_context = (mender_http_request_context *)user_data;

    /* Check if data is available */
    if ((true == response->body_found) && (NULL != response->body_frag_start) && (0 != response->body_frag_len) && (MENDER_OK == request_context->ret)) {

        /* Transmit data received to the upper layer */
        if (MENDER_OK
            != (request_context->ret = request_context->callback(
                    MENDER_HTTP_EVENT_DATA_RECEIVED, (void *)response->body_frag_start, response->body_frag_len, request_context->params))) {
            mender_log_error("An error occurred, stop reading data");
        }
    }
}

static enum http_method
mender_http_method_to_zephyr_http_client_method(mender_http_method_t method) {

    /* Convert method */
    if (MENDER_HTTP_GET == method) {
        return HTTP_GET;
    } else if (MENDER_HTTP_POST == method) {
        return HTTP_POST;
    } else if (MENDER_HTTP_PUT == method) {
        return HTTP_PUT;
    } else if (MENDER_HTTP_PATCH == method) {
        return HTTP_PATCH;
    }

    return -1;
}
