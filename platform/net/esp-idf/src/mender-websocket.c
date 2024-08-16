/**
 * @file      mender-websocket.c
 * @brief     Mender websocket interface for ESP-IDF platform, requires ESP-IDF >= 5.0
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

#include <esp_event.h>
#include <esp_websocket_client.h>
#include <esp_crt_bundle.h>
#include "mender-log.h"
#include "mender-utils.h"
#include "mender-websocket.h"

/**
 * @brief Websocket thread stack size (kB)
 */
#ifndef CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE
#define CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE (4)
#endif /* CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE */

/**
 * @brief Websocket thread priority
 */
#ifndef CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY
#define CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY (5)
#endif /* CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY */

/**
 * @brief Websocket buffer size (kB)
 */
#ifndef CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE
#define CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE (1)
#endif /* CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE */

/**
 * @brief Websocket reconnect timeout (milliseconds)
 */
#ifndef CONFIG_MENDER_WEBSOCKET_RECONNECT_TIMEOUT
#define CONFIG_MENDER_WEBSOCKET_RECONNECT_TIMEOUT (3000)
#endif /* CONFIG_MENDER_WEBSOCKET_RECONNECT_TIMEOUT */

/**
 * @brief Websocket network timeout (milliseconds)
 */
#ifndef CONFIG_MENDER_WEBSOCKET_NETWORK_TIMEOUT
#define CONFIG_MENDER_WEBSOCKET_NETWORK_TIMEOUT (3000)
#endif /* CONFIG_MENDER_WEBSOCKET_NETWORK_TIMEOUT */

/**
 * @brief Websocket request timeout (milliseconds)
 */
#ifndef CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT
#define CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT (3000)
#endif /* CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT */

/**
 * @brief Websocket ping interval (seconds)
 */
#ifndef CONFIG_MENDER_WEBSOCKET_PING_INTERVAL
#define CONFIG_MENDER_WEBSOCKET_PING_INTERVAL (60)
#endif /* CONFIG_MENDER_WEBSOCKET_PING_INTERVAL */

/**
 * @brief WebSocket User-Agent
 */
#define MENDER_WEBSOCKET_USER_AGENT "mender-mcu-client/" MENDER_CLIENT_VERSION " (mender-websocket) esp-idf/" IDF_VER

/**
 * @brief Websocket handle
 */
typedef struct {
    esp_websocket_client_handle_t client; /**< Websocket client handle */
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
 * @brief Websocket event callback
 * @param handler_args 
 * @param base Event base
 * @param event_id Event ID
 * @param event_data Websocket event data
 */
static void mender_websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

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
    assert(NULL != callback);
    assert(NULL != handle);
    esp_err_t    err;
    mender_err_t ret    = MENDER_OK;
    char        *url    = NULL;
    char        *bearer = NULL;

    /* Allocate a new handle */
    if (NULL == (*handle = malloc(sizeof(mender_websocket_handle_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(*handle, 0, sizeof(mender_websocket_handle_t));

    /* Save callback and params */
    ((mender_websocket_handle_t *)*handle)->callback = callback;
    ((mender_websocket_handle_t *)*handle)->params   = params;

    /* Compute URL if required */
    if ((false == mender_utils_strbeginwith(path, "ws://")) && (false == mender_utils_strbeginwith(path, "wss://"))) {
        if ((true == mender_utils_strbeginwith(path, "http://")) || (true == mender_utils_strbeginwith(mender_websocket_config.host, "http://"))) {
            size_t str_length = strlen(mender_websocket_config.host) - strlen("http://") + strlen("ws://") + strlen(path) + 1;
            if (NULL == (url = (char *)malloc(str_length))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto FAIL;
            }
            snprintf(url, str_length, "ws://%s%s", mender_websocket_config.host + strlen("http://"), path);
        } else if ((true == mender_utils_strbeginwith(path, "https://")) || (true == mender_utils_strbeginwith(mender_websocket_config.host, "https://"))) {
            size_t str_length = strlen(mender_websocket_config.host) - strlen("https://") + strlen("wss://") + strlen(path) + 1;
            if (NULL == (url = (char *)malloc(str_length))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto FAIL;
            }
            snprintf(url, str_length, "wss://%s%s", mender_websocket_config.host + strlen("https://"), path);
        } else {
            size_t str_length = strlen(mender_websocket_config.host) + strlen(path) + 1;
            if (NULL == (url = (char *)malloc(str_length))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto FAIL;
            }
            snprintf(url, str_length, "%s%s", mender_websocket_config.host, path);
        }
    }

    /* Configuration of the client */
    esp_websocket_client_config_t config = { .uri                  = (NULL != url) ? url : path,
                                             .task_stack           = CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE * 1024,
                                             .task_prio            = CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY,
                                             .buffer_size          = CONFIG_MENDER_WEBSOCKET_BUFFER_SIZE * 1024,
                                             .user_agent           = MENDER_WEBSOCKET_USER_AGENT,
                                             .crt_bundle_attach    = esp_crt_bundle_attach,
                                             .reconnect_timeout_ms = CONFIG_MENDER_WEBSOCKET_RECONNECT_TIMEOUT,
                                             .network_timeout_ms   = CONFIG_MENDER_WEBSOCKET_NETWORK_TIMEOUT,
                                             .ping_interval_sec    = CONFIG_MENDER_WEBSOCKET_PING_INTERVAL };
    if (true == mender_utils_strbeginwith(config.uri, "ws://")) {
        config.transport = WEBSOCKET_TRANSPORT_OVER_TCP;
    } else if (true == mender_utils_strbeginwith(config.uri, "wss://")) {
        config.transport = WEBSOCKET_TRANSPORT_OVER_SSL;
    } else {
        config.transport = WEBSOCKET_TRANSPORT_UNKNOWN;
    }
    if (NULL != jwt) {
        size_t str_length = strlen("Authorization: Bearer ") + strlen(jwt) + strlen("\r\n") + 1;
        if (NULL == (bearer = (char *)malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        snprintf(bearer, str_length, "Authorization: Bearer %s\r\n", jwt);
    }
    config.headers = bearer;

    /* Initialization of the client */
    if (NULL == (((mender_websocket_handle_t *)*handle)->client = esp_websocket_client_init(&config))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    if (ESP_OK
        != (err
            = esp_websocket_register_events(((mender_websocket_handle_t *)*handle)->client, WEBSOCKET_EVENT_ANY, mender_websocket_event_handler, *handle))) {
        mender_log_error("Unable to register websocket callback event handler: %s", esp_err_to_name(err));
        ret = MENDER_FAIL;
        goto END;
    }

    /* Open websocket client connection */
    if (ESP_OK != (err = esp_websocket_client_start(((mender_websocket_handle_t *)*handle)->client))) {
        mender_log_error("Unable to open websocket client connection: %s", esp_err_to_name(err));
        ret = MENDER_FAIL;
        goto FAIL;
    }

    goto END;

FAIL:

    /* Release memory */
    if (NULL != *handle) {
        if (NULL != ((mender_websocket_handle_t *)*handle)->client) {
            esp_websocket_client_destroy(((mender_websocket_handle_t *)*handle)->client);
        }
        free(*handle);
        *handle = NULL;
    }

END:

    /* Release memory */
    if (NULL != bearer) {
        free(bearer);
    }
    if (NULL != url) {
        free(url);
    }

    return ret;
}

mender_err_t
mender_websocket_send(void *handle, void *payload, size_t length) {

    assert(NULL != handle);
    assert(NULL != payload);

    /* Send binary payload */
    if (length
        != esp_websocket_client_send_bin(
            ((mender_websocket_handle_t *)handle)->client, payload, (int)length, pdMS_TO_TICKS(CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT))) {
        mender_log_error("Unable to send data over websocket connection");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_websocket_disconnect(void *handle) {

    assert(NULL != handle);
    esp_err_t err;

    /* Close websocket connection */
    if (ESP_OK != (err = esp_websocket_client_close(((mender_websocket_handle_t *)handle)->client, pdMS_TO_TICKS(CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT)))) {
        mender_log_error("Unable to close websocket connection: %s", esp_err_to_name(err));
        return MENDER_FAIL;
    }

    /* Release memory */
    esp_websocket_client_destroy(((mender_websocket_handle_t *)handle)->client);
    free(handle);

    return MENDER_OK;
}

mender_err_t
mender_websocket_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}

static void
mender_websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    assert(NULL != handler_args);
    (void)base;
    assert(NULL != event_data);
    mender_websocket_handle_t  *handle = (mender_websocket_handle_t *)handler_args;
    esp_websocket_event_data_t *data   = (esp_websocket_event_data_t *)event_data;

    /* Treatment depending of the event */
    switch (event_id) {
        case WEBSOCKET_EVENT_BEFORE_CONNECT:
            /* Nothing to do */
            break;
        case WEBSOCKET_EVENT_CONNECTED:
            /* Connection has been established */
            if (MENDER_OK != handle->callback(MENDER_WEBSOCKET_EVENT_CONNECTED, NULL, 0, handle->params)) {
                mender_log_error("An error occurred");
            }
            break;
        case WEBSOCKET_EVENT_DATA:
            /* Treatment of data received depending of the opcode */
            if (WS_TRANSPORT_OPCODES_PING == data->op_code) {

                /* Send pong message with the same payload */
                esp_websocket_client_send_with_opcode(handle->client,
                                                      WS_TRANSPORT_OPCODES_PONG,
                                                      (const uint8_t *)data->data_ptr,
                                                      data->data_len,
                                                      pdMS_TO_TICKS(CONFIG_MENDER_WEBSOCKET_REQUEST_TIMEOUT));

            } else if (WS_TRANSPORT_OPCODES_BINARY == data->op_code) {

                /* Invoke callback */
                if (MENDER_OK != handle->callback(MENDER_WEBSOCKET_EVENT_DATA_RECEIVED, (void *)data->data_ptr, (size_t)data->data_len, handle->params)) {
                    mender_log_error("An error occurred");
                }
            }
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
        case WEBSOCKET_EVENT_CLOSED:
            /* Connection has been closed */
            if (MENDER_OK != handle->callback(MENDER_WEBSOCKET_EVENT_DISCONNECTED, NULL, 0, handle->params)) {
                mender_log_error("An error occurred");
            }
            break;
        case WEBSOCKET_EVENT_ERROR:
            /* Connection failed */
            mender_log_error("An error occurred");
            handle->callback(MENDER_WEBSOCKET_EVENT_ERROR, NULL, 0, handle->params);
            break;
        default:
            /* Should not occur */
            mender_log_error("Unknown event occurred (event_id=%d)", event_id);
            break;
    }
}
