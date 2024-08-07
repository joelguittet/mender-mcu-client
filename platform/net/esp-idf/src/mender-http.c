/**
 * @file      mender-http.c
 * @brief     Mender HTTP interface for ESP-IDF platform
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
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include "mender-http.h"
#include "mender-log.h"
#include "mender-utils.h"

/**
 * @brief HTTP User-Agent
 */
#define MENDER_HTTP_USER_AGENT "mender-mcu-client/" MENDER_CLIENT_VERSION " (mender-http) esp-idf/" IDF_VER

/**
 * @brief Receive buffer length
 */
#define MENDER_HTTP_RECV_BUF_LENGTH (512)

/**
 * @brief Mender HTTP configuration
 */
static mender_http_config_t mender_http_config;

/**
 * @brief Convert mender HTTP method to ESP HTTP client method
 * @param method Mender HTTP method
 * @return ESP HTTP client method if the function succeeds, HTTP_METHOD_MAX otherwise
 */
static esp_http_client_method_t mender_http_method_to_esp_http_client_method(mender_http_method_t method);

mender_err_t
mender_http_init(mender_http_config_t *config) {

    assert(NULL != config);
    assert(NULL != config->host);

    /* Save configuration */
    memcpy(&mender_http_config, config, sizeof(mender_http_config_t));

    return MENDER_OK;
}

mender_err_t
mender_http_perform(char                *jwt,
                    char                *path,
                    mender_http_method_t method,
                    char                *payload,
                    char                *signature,
                    mender_err_t (*callback)(mender_http_client_event_t, void *, size_t, void *),
                    void *params,
                    int  *status) {

    assert(NULL != path);
    assert(NULL != callback);
    assert(NULL != status);
    esp_err_t                err;
    mender_err_t             ret    = MENDER_OK;
    esp_http_client_handle_t client = NULL;
    char                    *url    = NULL;
    char                    *bearer = NULL;

    /* Compute URL if required */
    if ((false == mender_utils_strbeginwith(path, "http://")) && (false == mender_utils_strbeginwith(path, "https://"))) {
        size_t str_length = strlen(mender_http_config.host) + strlen(path) + 1;
        if (NULL == (url = (char *)malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        snprintf(url, str_length, "%s%s", mender_http_config.host, path);
    }

    /* Configuration of the client */
    esp_http_client_config_t config
        = { .url = (NULL != url) ? url : path, .user_agent = MENDER_HTTP_USER_AGENT, .crt_bundle_attach = esp_crt_bundle_attach, .buffer_size_tx = 2048 };

    /* Initialization of the client */
    if (NULL == (client = esp_http_client_init(&config))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    esp_http_client_set_method(client, mender_http_method_to_esp_http_client_method(method));
    if (NULL != jwt) {
        size_t str_length = strlen("Bearer ") + strlen(jwt) + 1;
        if (NULL == (bearer = (char *)malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        snprintf(bearer, str_length, "Bearer %s", jwt);
        esp_http_client_set_header(client, "Authorization", bearer);
    }
    if (NULL != signature) {
        esp_http_client_set_header(client, "X-MEN-Signature", signature);
    }
    if (NULL != payload) {
        esp_http_client_set_header(client, "Content-Type", "application/json");
    }

    /* Open HTTP client connection */
    if (ESP_OK != (err = esp_http_client_open(client, (NULL != payload) ? (int)strlen(payload) : 0))) {
        mender_log_error("Unable to open HTTP client connection: %s", esp_err_to_name(err));
        ret = MENDER_FAIL;
        goto END;
    }
    if (MENDER_OK != (ret = callback(MENDER_HTTP_EVENT_CONNECTED, NULL, 0, params))) {
        mender_log_error("An error occurred");
        goto END;
    }

    /* Write data if payload is defined */
    if (NULL != payload) {
        if (esp_http_client_write(client, payload, (int)strlen(payload)) < 0) {
            mender_log_error("Unable to write data");
            ret = MENDER_FAIL;
            goto END;
        }
    }

    /* Fetch headers, this returns the content length */
    if (esp_http_client_fetch_headers(client) < 0) {
        mender_log_error("Unable to fetch headers");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Read data until all have been received */
    do {

        char data[MENDER_HTTP_RECV_BUF_LENGTH];
        int  read_length = esp_http_client_read(client, data, sizeof(data));
        if (read_length < 0) {
            mender_log_error("An error occured, unable to read data");
            callback(MENDER_HTTP_EVENT_ERROR, NULL, 0, params);
            ret = MENDER_FAIL;
            goto END;
        } else if (read_length > 0) {
            /* Transmit data received to the upper layer */
            if (MENDER_OK != (ret = callback(MENDER_HTTP_EVENT_DATA_RECEIVED, data, (size_t)read_length, params))) {
                mender_log_error("An error occurred, stop reading data");
                goto END;
            }
        } else {
            if ((ECONNRESET == errno) || (ENOTCONN == errno)) {
                mender_log_error("An error occurred, connection has been closed, errno = %d", errno);
                callback(MENDER_HTTP_EVENT_ERROR, NULL, 0, params);
                ret = MENDER_FAIL;
                goto END;
            }
        }
    } while (false == esp_http_client_is_complete_data_received(client));

    /* Read HTTP status code */
    *status = esp_http_client_get_status_code(client);
    if (MENDER_OK != (ret = callback(MENDER_HTTP_EVENT_DISCONNECTED, NULL, 0, params))) {
        mender_log_error("An error occurred");
        goto END;
    }

END:

    /* Release memory */
    if (NULL != client) {
        esp_http_client_cleanup(client);
    }
    if (NULL != bearer) {
        free(bearer);
    }
    if (NULL != url) {
        free(url);
    }

    return ret;
}

mender_err_t
mender_http_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}

static esp_http_client_method_t
mender_http_method_to_esp_http_client_method(mender_http_method_t method) {

    /* Convert method */
    if (MENDER_HTTP_GET == method) {
        return HTTP_METHOD_GET;
    } else if (MENDER_HTTP_POST == method) {
        return HTTP_METHOD_POST;
    } else if (MENDER_HTTP_PUT == method) {
        return HTTP_METHOD_PUT;
    } else if (MENDER_HTTP_PATCH == method) {
        return HTTP_METHOD_PATCH;
    }

    return HTTP_METHOD_MAX;
}
