/**
 * @file      mender-troubleshoot-api.c
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

#include "mender-api.h"
#include "mender-log.h"
#include "mender-troubleshoot-api.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

#include "mender-websocket.h"

/**
 * @brief Paths of the mender-server APIs
 */
#define MENDER_TROUBLESHOOT_API_PATH_GET_DEVICE_CONNECT "/api/devices/v1/deviceconnect/connect"

/**
 * @brief Mender troubleshoot API config
 */
static mender_troubleshoot_api_config_t mender_troubleshoot_api_config;

/**
 * @brief Mender troubleshoot API connection handle
 */
static void *mender_troubleshoot_api_handle = NULL;

/**
 * @brief Websocket callback used to handle websocket data
 * @param event Websocket client event
 * @param data Data received
 * @param data_length Data length
 * @param params Callback parameters
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_api_websocket_callback(mender_websocket_client_event_t event, void *data, size_t data_length, void *params);

mender_err_t
mender_troubleshoot_api_init(mender_troubleshoot_api_config_t *config) {

    assert(NULL != config);
    assert(NULL != config->host);
    mender_err_t ret;

    /* Save configuration */
    memcpy(&mender_troubleshoot_api_config, config, sizeof(mender_troubleshoot_api_config_t));

    /* Initializations */
    mender_websocket_config_t mender_websocket_config = { .host = mender_troubleshoot_api_config.host };
    if (MENDER_OK != (ret = mender_websocket_init(&mender_websocket_config))) {
        mender_log_error("Unable to initialize websocket");
        return ret;
    }

    return ret;
}

mender_err_t
mender_troubleshoot_api_connect(mender_err_t (*callback)(void *, size_t)) {

    mender_err_t ret;

    /* Open websocket connection */
    if (MENDER_OK
        != (ret = mender_websocket_connect(mender_api_get_authentication_token(),
                                           MENDER_TROUBLESHOOT_API_PATH_GET_DEVICE_CONNECT,
                                           &mender_troubleshoot_api_websocket_callback,
                                           callback,
                                           &mender_troubleshoot_api_handle))) {
        mender_log_error("Unable to open websocket connection");
        goto END;
    }

END:

    return ret;
}

bool
mender_troubleshoot_api_is_connected(void) {

    /* Return connection status */
    return (NULL != mender_troubleshoot_api_handle) ? true : false;
}

mender_err_t
mender_troubleshoot_api_send(void *payload, size_t length) {

    mender_err_t ret;

    /* Send data over websocket connection */
    if (MENDER_OK != (ret = mender_websocket_send(mender_troubleshoot_api_handle, payload, length))) {
        mender_log_error("Unable to send data over websocket connection");
        goto END;
    }

END:

    return ret;
}

mender_err_t
mender_troubleshoot_api_disconnect(void) {

    mender_err_t ret;

    /* Close websocket connection */
    if (MENDER_OK != (ret = mender_websocket_disconnect(mender_troubleshoot_api_handle))) {
        mender_log_error("Unable to close websocket connection");
        goto END;
    }
    mender_troubleshoot_api_handle = NULL;

END:

    return ret;
}

mender_err_t
mender_troubleshoot_api_exit(void) {

    /* Release all modules */
    mender_websocket_exit();

    return MENDER_OK;
}

static mender_err_t
mender_troubleshoot_api_websocket_callback(mender_websocket_client_event_t event, void *data, size_t data_length, void *params) {

    assert(NULL != params);
    mender_err_t (*callback)(void *, size_t) = params;
    mender_err_t ret                         = MENDER_OK;

    /* Treatment depending of the event */
    switch (event) {
        case MENDER_WEBSOCKET_EVENT_CONNECTED:
            /* Nothing to do */
            mender_log_info("Troubleshoot client connected");
            break;
        case MENDER_WEBSOCKET_EVENT_DATA_RECEIVED:
            /* Check input data */
            if ((NULL == data) || (0 == data_length)) {
                mender_log_error("Invalid data received");
                ret = MENDER_FAIL;
                break;
            }
            /* Process input data */
            if (MENDER_OK != (ret = callback(data, data_length))) {
                mender_log_error("Unable to process data");
                break;
            }
            break;
        case MENDER_WEBSOCKET_EVENT_DISCONNECTED:
            /* Nothing to do */
            mender_log_info("Troubleshoot client disconnected");
            break;
        case MENDER_WEBSOCKET_EVENT_ERROR:
            /* Websocket connection fails */
            mender_log_error("An error occurred");
            ret = MENDER_FAIL;
            break;
        default:
            /* Should not occur */
            ret = MENDER_FAIL;
            break;
    }

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */
