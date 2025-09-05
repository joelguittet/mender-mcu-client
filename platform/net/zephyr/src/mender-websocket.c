/**
 * @file      mender-websocket.c
 * @brief     Mender websocket interface for Zephyr platform
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

#include <errno.h>
#include <version.h>
#include <zephyr/kernel.h>
#include <zephyr/net/websocket.h>
#include "mender-log.h"
#include "mender-net.h"
#include "mender-websocket.h"

/**
 * @brief Check size of the heap memory pool
 */
#if (0 == CONFIG_HEAP_MEM_POOL_SIZE)
#error "CONFIG_HEAP_MEM_POOL_SIZE is not defined"
#endif

/**
 * @brief Websocket thread stack size
 */
#ifndef CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE
#define CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE (4096)
#endif /* CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE */

/**
 * @brief Websocket thread priority
 */
#ifndef CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY
#define CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY (5)
#endif /* CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY */

/**
 * @brief Websocket buffer size
 */
#ifndef CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE
#define CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE (1024)
#endif /* CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE */

/**
 * @brief Websocket connect timeout (milliseconds)
 */
#ifndef CONFIG_MENDER_WEBSOCKET_CONNECT_TIMEOUT
#define CONFIG_MENDER_WEBSOCKET_CONNECT_TIMEOUT (3000)
#endif /* CONFIG_MENDER_WEBSOCKET_CONNECT_TIMEOUT */

/**
 * @brief Websocket request timeout (milliseconds)
 */
#ifndef CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT
#define CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT (3000)
#endif /* CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT */

/**
 * @brief WebSocket User-Agent
 */
#define MENDER_WEBSOCKET_USER_AGENT "mender-mcu-client/" MENDER_CLIENT_VERSION " (mender-websocket) zephyr/" KERNEL_VERSION_STRING

/**
 * @brief Websocket handle
 */
typedef struct {
    int                      sock;          /**< Socket */
    char                    *host;          /**< Host */
    char                    *port;          /**< Port */
    char                    *url;           /**< URL */
    char                   **headers;       /**< Headers */
    struct websocket_request request;       /**< Websocket request */
    int                      client;        /**< Websocket client handle */
    uint8_t                 *data;          /**< Websocket data received from the server */
    size_t                   data_len;      /**< Websocket data length received from the server */
    struct k_thread          thread_handle; /**< Websocket thread handle */
    bool                     abort;         /**< Flag used to indicate connection should be terminated */
    mender_err_t (*callback)(mender_websocket_client_event_t,
                             void *,
                             size_t,
                             void *); /**< Callback function to be invoked to perform the treatment of the events and data from the websocket */
    void *params;                     /**< Callback function parameters */
} mender_websocket_handle_t;

/**
 * @brief Mender websocket configuration
 */
static mender_websocket_config_t mender_websocket_config;

/**
 * @brief Mender websocket thread stack
 */
K_THREAD_STACK_DEFINE(mender_websocket_thread_stack, CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE);

/**
 * @brief Thread used to perform reception of data
 * @param p1 Websocket handle
 * @param p2 Not used
 * @param p3 Not used
 */
static void mender_websocket_thread(void *p1, void *p2, void *p3);

mender_err_t
mender_websocket_init(mender_websocket_config_t *config) {

    assert(NULL != config);
    assert(NULL != config->host);

    /* Save configuration */
    memcpy(&mender_websocket_config, config, sizeof(mender_websocket_config_t));

    return MENDER_OK;
}

mender_err_t
mender_websocket_connect(
    char *jwt, char *path, mender_err_t (*callback)(mender_websocket_client_event_t, void *, size_t, void *), void *params, void **handle) {

    assert(NULL != path);
    assert(NULL != handle);
    mender_err_t ret;
    size_t       header_index = 0;

    /* Allocate a new handle */
    if (NULL == (*handle = malloc(sizeof(mender_websocket_handle_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(*handle, 0, sizeof(mender_websocket_handle_t));
    ((mender_websocket_handle_t *)*handle)->sock = -1;

    /* Save callback and params */
    ((mender_websocket_handle_t *)*handle)->callback = callback;
    ((mender_websocket_handle_t *)*handle)->params   = params;

    /* Retrieve host, port and url */
    if (MENDER_OK
        != (ret = mender_net_get_host_port_url(path,
                                               mender_websocket_config.host,
                                               &((mender_websocket_handle_t *)*handle)->host,
                                               &((mender_websocket_handle_t *)*handle)->port,
                                               &((mender_websocket_handle_t *)*handle)->url))) {
        mender_log_error("Unable to retrieve host/port/url");
        goto FAIL;
    }

    /* Configuration of the client */
    ((mender_websocket_handle_t *)*handle)->request.url  = ((mender_websocket_handle_t *)*handle)->url;
    ((mender_websocket_handle_t *)*handle)->request.host = ((mender_websocket_handle_t *)*handle)->host;
    /* We need to allocate bigger buffer for the websocket data we receive so that the websocket header fits into it */
    if (NULL == (((mender_websocket_handle_t *)*handle)->request.tmp_buf = (uint8_t *)malloc(CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE + 32))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    ((mender_websocket_handle_t *)*handle)->request.tmp_buf_len = CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE + 32;
    if (NULL == (((mender_websocket_handle_t *)*handle)->headers = malloc(3 * sizeof(char *)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(((mender_websocket_handle_t *)*handle)->headers, 0, 3 * sizeof(char *));
    size_t str_length = strlen("User-Agent: ") + strlen(MENDER_WEBSOCKET_USER_AGENT) + strlen("\r\n") + 1;
    if (NULL == (((mender_websocket_handle_t *)*handle)->headers[header_index] = malloc(str_length))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    snprintf(((mender_websocket_handle_t *)*handle)->headers[header_index], str_length, "User-Agent: %s\r\n", MENDER_WEBSOCKET_USER_AGENT);
    header_index++;
    if (NULL != jwt) {
        str_length = strlen("Authorization: Bearer ") + strlen(jwt) + strlen("\r\n") + 1;
        if (NULL == (((mender_websocket_handle_t *)*handle)->headers[header_index] = malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        snprintf(((mender_websocket_handle_t *)*handle)->headers[header_index], str_length, "Authorization: Bearer %s\r\n", jwt);
        header_index++;
    }
    ((mender_websocket_handle_t *)*handle)->request.optional_headers = (const char **)((mender_websocket_handle_t *)*handle)->headers;

    /* Connect to the server */
    if (MENDER_OK
        != (ret = mender_net_connect(
                ((mender_websocket_handle_t *)*handle)->host, ((mender_websocket_handle_t *)*handle)->port, &((mender_websocket_handle_t *)*handle)->sock))) {
        mender_log_error("Unable to open HTTP client connection");
        goto FAIL;
    }

    /* Upgrade to WebSocket connection */
    if ((((mender_websocket_handle_t *)*handle)->client = websocket_connect(
             ((mender_websocket_handle_t *)*handle)->sock, &(((mender_websocket_handle_t *)*handle)->request), CONFIG_MENDER_WEBSOCKET_CONNECT_TIMEOUT, NULL))
        < 0) {
        mender_log_error("Unable to upgrade to websocket connection");
        mender_log_error("errno %d", errno);
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (MENDER_OK != (ret = callback(MENDER_WEBSOCKET_EVENT_CONNECTED, NULL, 0, params))) {
        mender_log_error("An error occurred");
        goto FAIL;
    }

    /* Create and start websocket thread */
    k_thread_create(&((mender_websocket_handle_t *)*handle)->thread_handle,
                    mender_websocket_thread_stack,
                    CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE,
                    mender_websocket_thread,
                    *handle,
                    NULL,
                    NULL,
                    CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY,
                    0,
                    K_NO_WAIT);
    k_thread_name_set(&((mender_websocket_handle_t *)*handle)->thread_handle, "mender_websocket");

    return ret;

FAIL:

    /* Close connection */
    if (NULL != *handle) {
        if (-1 != ((mender_websocket_handle_t *)*handle)->sock) {
            mender_net_disconnect(((mender_websocket_handle_t *)*handle)->sock);
        }
    }

    /* Release memory */
    if (NULL != *handle) {
        if (NULL != ((mender_websocket_handle_t *)*handle)->host) {
            free(((mender_websocket_handle_t *)*handle)->host);
        }
        if (NULL != ((mender_websocket_handle_t *)*handle)->port) {
            free(((mender_websocket_handle_t *)*handle)->port);
        }
        if (NULL != ((mender_websocket_handle_t *)*handle)->url) {
            free(((mender_websocket_handle_t *)*handle)->url);
        }
        if (NULL != ((mender_websocket_handle_t *)*handle)->request.tmp_buf) {
            free(((mender_websocket_handle_t *)*handle)->request.tmp_buf);
        }
        if (NULL != ((mender_websocket_handle_t *)*handle)->headers) {
            header_index = 0;
            while (NULL != ((mender_websocket_handle_t *)*handle)->headers[header_index]) {
                free(((mender_websocket_handle_t *)*handle)->headers[header_index]);
                header_index++;
            }
        }
        free(*handle);
        *handle = NULL;
    }

    return ret;
}

mender_err_t
mender_websocket_send(void *handle, void *payload, size_t length) {

    assert(NULL != handle);
    assert(NULL != payload);
    int sent;

    /* Send binary payload */
    if (length
        != (sent = websocket_send_msg(((mender_websocket_handle_t *)handle)->client,
                                      payload,
                                      length,
                                      WEBSOCKET_OPCODE_DATA_BINARY,
                                      true,
                                      true,
                                      CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT))) {
        mender_log_error("Unable to send data over websocket connection: %d", sent);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_websocket_disconnect(void *handle) {

    assert(NULL != handle);

    /* Close websocket connection */
    ((mender_websocket_handle_t *)handle)->abort = true;
    websocket_disconnect(((mender_websocket_handle_t *)handle)->client);

    /* Wait end of execution of the websocket thread */
    k_thread_join(&((mender_websocket_handle_t *)handle)->thread_handle, K_FOREVER);

    /* Release memory */
    if (NULL != ((mender_websocket_handle_t *)handle)->host) {
        free(((mender_websocket_handle_t *)handle)->host);
    }
    if (NULL != ((mender_websocket_handle_t *)handle)->port) {
        free(((mender_websocket_handle_t *)handle)->port);
    }
    if (NULL != ((mender_websocket_handle_t *)handle)->url) {
        free(((mender_websocket_handle_t *)handle)->url);
    }
    if (NULL != ((mender_websocket_handle_t *)handle)->request.tmp_buf) {
        free(((mender_websocket_handle_t *)handle)->request.tmp_buf);
    }
    if (NULL != ((mender_websocket_handle_t *)handle)->headers) {
        size_t header_index = 0;
        while (NULL != ((mender_websocket_handle_t *)handle)->headers[header_index]) {
            free(((mender_websocket_handle_t *)handle)->headers[header_index]);
            header_index++;
        }
    }
    if (NULL != ((mender_websocket_handle_t *)handle)->data) {
        free(((mender_websocket_handle_t *)handle)->data);
    }
    free(handle);

    return MENDER_OK;
}

mender_err_t
mender_websocket_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}

static void
mender_websocket_thread(void *p1, void *p2, void *p3) {

    assert(NULL != p1);
    mender_websocket_handle_t *handle = (mender_websocket_handle_t *)p1;
    (void)p2;
    (void)p3;
    uint8_t *payload;
    int      received;
    uint32_t message_type = 0;
    uint64_t remaining    = 0;

    /* Allocate payload */
    if (NULL == (payload = (uint8_t *)malloc(CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE))) {
        mender_log_error("Unable to allocate memory");
        goto END;
    }

    /* Perform reception of data from the websocket connection */
    while (false == handle->abort) {

        received = websocket_recv_msg(handle->client, payload, CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE, &message_type, &remaining, SYS_FOREVER_MS);
        if (received < 0) {
            if (-ENOTCONN == received) {
                mender_log_error("Connection has been closed");
                goto END;
            }
            mender_log_error("Unable to receive websocket message: errno=%d", errno);

        } else if (received > 0) {

            /* Perform treatment depending of the opcode */
            if (WEBSOCKET_FLAG_PING == (message_type & WEBSOCKET_FLAG_PING)) {

                /* Send pong message with the same payload */
                websocket_send_msg(handle->client, payload, received, WEBSOCKET_OPCODE_PONG, true, true, CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT);

            } else if (WEBSOCKET_FLAG_BINARY == (message_type & WEBSOCKET_FLAG_BINARY)) {

                /* Check if the whole packet is received once */
                if ((NULL == handle->data) && (0 == remaining)) {

                    /* Invoke callback */
                    if (MENDER_OK != handle->callback(MENDER_WEBSOCKET_EVENT_DATA_RECEIVED, payload, received, handle->params)) {
                        mender_log_error("An error occurred");
                    }

                } else {

                    /* Concatenate data */
                    void *tmp = realloc(handle->data, handle->data_len + received);
                    if (NULL == tmp) {
                        mender_log_error("Unable to allocate memory");
                        if (NULL != handle->data) {
                            free(handle->data);
                            handle->data     = NULL;
                            handle->data_len = 0;
                        }
                        break;
                    }
                    handle->data = tmp;
                    memcpy(handle->data + handle->data_len, payload, received);
                    handle->data_len += received;

                    /* Check if the whole packet has been received */
                    if (0 == remaining) {

                        /* Invoke callback */
                        if (MENDER_OK != handle->callback(MENDER_WEBSOCKET_EVENT_DATA_RECEIVED, handle->data, handle->data_len, handle->params)) {
                            mender_log_error("An error occurred");
                        }

                        /* Release memory */
                        free(handle->data);
                        handle->data     = NULL;
                        handle->data_len = 0;
                    }
                }
            }
        }
    }

END:

    /* Invoke disconnected callback */
    handle->callback(MENDER_WEBSOCKET_EVENT_DISCONNECTED, NULL, 0, handle->params);

    /* Release memory */
    if (NULL != payload) {
        free(payload);
    }
}
