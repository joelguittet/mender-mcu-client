/**
 * @file      mender-troubleshoot.c
 * @brief     Mender MCU Troubleshoot add-on implementation
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

#include "mender-client.h"
#include "mender-log.h"
#include "mender-scheduler.h"
#include "mender-troubleshoot.h"
#include "mender-troubleshoot-api.h"
#include "mender-troubleshoot-control.h"
#include "mender-troubleshoot-file-transfer.h"
#include "mender-troubleshoot-mender-client.h"
#include "mender-troubleshoot-protomsg.h"
#include "mender-troubleshoot-shell.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

/**
 * @brief Default host
 */
#ifndef CONFIG_MENDER_SERVER_HOST
#define CONFIG_MENDER_SERVER_HOST "https://hosted.mender.io"
#endif /* CONFIG_MENDER_SERVER_HOST */

/**
 * @brief Default troubleshoot healthcheck interval (seconds)
 */
#ifndef CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL
#define CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL (30)
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL */

/**
 * @brief Mender troubleshoot instance
 */
const mender_addon_instance_t mender_troubleshoot_addon_instance
    = { .init = mender_troubleshoot_init, .activate = NULL, .deactivate = mender_troubleshoot_deactivate, .exit = mender_troubleshoot_exit };

/**
 * @brief Mender troubleshoot configuration
 */
static mender_troubleshoot_config_t mender_troubleshoot_config;

/**
 * @brief Mender troubleshoot work handle
 */
static void *mender_troubleshoot_healthcheck_work_handle = NULL;

/**
 * @brief Mender troubleshoot healthcheck work function
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_healthcheck_work_function(void);

/**
 * @brief Callback function to be invoked to perform the treatment of the data from the websocket
 * @param data Received data
 * @param length Received data length
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_data_received_callback(void *data, size_t length);

mender_err_t
mender_troubleshoot_init(void *config, void *callbacks) {

    assert(NULL != config);
    mender_err_t ret;

    /* Save configuration */
    if ((NULL != ((mender_troubleshoot_config_t *)config)->host) && (strlen(((mender_troubleshoot_config_t *)config)->host) > 0)) {
        mender_troubleshoot_config.host = ((mender_troubleshoot_config_t *)config)->host;
    } else {
        mender_troubleshoot_config.host = CONFIG_MENDER_SERVER_HOST;
    }
    if (0 != ((mender_troubleshoot_config_t *)config)->healthcheck_interval) {
        mender_troubleshoot_config.healthcheck_interval = ((mender_troubleshoot_config_t *)config)->healthcheck_interval;
    } else {
        mender_troubleshoot_config.healthcheck_interval = CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL;
    }

    /* Initializations */
    mender_troubleshoot_api_config_t mender_troubleshoot_api_config = { .host = mender_troubleshoot_config.host };
    if (MENDER_OK != (ret = mender_troubleshoot_api_init(&mender_troubleshoot_api_config))) {
        mender_log_error("Unable to initialize troubleshoot API");
        goto END;
    }
    if (MENDER_OK != (ret = mender_troubleshoot_control_init())) {
        mender_log_error("Unable to initialize control handler");
        goto END;
    }
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER
    if (MENDER_OK != (ret = mender_troubleshoot_file_transfer_init(&((mender_troubleshoot_callbacks_t *)callbacks)->file_transfer))) {
        mender_log_error("Unable to initialize file transfer handler");
        goto END;
    }
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER */
    if (MENDER_OK != (ret = mender_troubleshoot_mender_client_init())) {
        mender_log_error("Unable to initialize mender client handler");
        goto END;
    }
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL
    mender_troubleshoot_shell_config_t mender_troubleshoot_shell_config = { .healthcheck_interval = mender_troubleshoot_config.healthcheck_interval };
    if (MENDER_OK != (ret = mender_troubleshoot_shell_init(&mender_troubleshoot_shell_config, &((mender_troubleshoot_callbacks_t *)callbacks)->shell))) {
        mender_log_error("Unable to initialize shell handler");
        goto END;
    }
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */

    /* Create troubleshoot healthcheck work */
    mender_scheduler_work_params_t healthcheck_work_params;
    healthcheck_work_params.function = mender_troubleshoot_healthcheck_work_function;
    healthcheck_work_params.period   = mender_troubleshoot_config.healthcheck_interval;
    healthcheck_work_params.name     = "mender_troubleshoot_healthcheck";
    if (MENDER_OK != (ret = mender_scheduler_work_create(&healthcheck_work_params, &mender_troubleshoot_healthcheck_work_handle))) {
        mender_log_error("Unable to create healthcheck work");
        goto END;
    }

END:

    return ret;
}

mender_err_t
mender_troubleshoot_activate(void) {

    mender_err_t ret;

    /* Activate troubleshoot healthcheck work */
    if (MENDER_OK != (ret = mender_scheduler_work_activate(mender_troubleshoot_healthcheck_work_handle))) {
        mender_log_error("Unable to activate troubleshoot healthcheck work");
        return ret;
    }

    return ret;
}

mender_err_t
mender_troubleshoot_deactivate(void) {

    mender_err_t ret = MENDER_OK;

    /* Deactivate troubleshoot healthcheck work */
    mender_scheduler_work_deactivate(mender_troubleshoot_healthcheck_work_handle);

    /* Check if connection is established */
    if (true == mender_troubleshoot_api_is_connected()) {

#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL

        /* Deactivate shell handler */
        if (MENDER_OK != mender_troubleshoot_shell_close()) {
            mender_log_error("Unable to deactivate shell handler");
        }

#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */

        /* Disconnect the device of the server */
        if (MENDER_OK != (ret = mender_troubleshoot_api_disconnect())) {
            mender_log_error("Unable to disconnect the device of the server");
        }

        /* Release access to the network */
        mender_client_network_release();
    }

    return ret;
}

mender_err_t
mender_troubleshoot_exit(void) {

    /* Delete troubleshoot healthcheck work */
    mender_scheduler_work_delete(mender_troubleshoot_healthcheck_work_handle);
    mender_troubleshoot_healthcheck_work_handle = NULL;

    /* Release handlers */
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL
    mender_troubleshoot_shell_exit();
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */
    mender_troubleshoot_mender_client_exit();
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER
    mender_troubleshoot_file_transfer_exit();
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER */
    mender_troubleshoot_control_exit();

    /* Release API */
    mender_troubleshoot_api_exit();

    /* Release memory */
    mender_troubleshoot_config.healthcheck_interval = 0;

    return MENDER_OK;
}

static mender_err_t
mender_troubleshoot_healthcheck_work_function(void) {

    mender_err_t ret = MENDER_OK;

    /* Check if connection is established */
    if (true == mender_troubleshoot_api_is_connected()) {

#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL

        /* Shell handler healthcheck */
        if (MENDER_OK != (ret = mender_troubleshoot_shell_healthcheck())) {
            mender_log_error("Unable to perform shell healthcheck");
            goto FAIL;
        }

#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */

    } else {

        /* Request access to the network */
        if (MENDER_OK != (ret = mender_client_network_connect())) {
            mender_log_error("Requesting access to the network failed");
            goto END;
        }

        /* Connect the device to the server */
        if (MENDER_OK != (ret = mender_troubleshoot_api_connect(&mender_troubleshoot_data_received_callback))) {
            mender_log_error("Unable to connect the device to the server");
            goto END;
        }
    }

END:

    return ret;

#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL

FAIL:

#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */

    /* Check if connection is established */
    if (true == mender_troubleshoot_api_is_connected()) {

#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL

        /* Deactivate shell handler */
        if (MENDER_OK != mender_troubleshoot_shell_close()) {
            mender_log_error("Unable to deactivate shell handler");
        }

#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */

        /* Disconnect the device of the server */
        if (MENDER_OK != mender_troubleshoot_api_disconnect()) {
            mender_log_error("Unable to disconnect the device of the server");
        }

        /* Release access to the network */
        mender_client_network_release();
    }

    return ret;
}

static mender_err_t
mender_troubleshoot_data_received_callback(void *data, size_t length) {

    assert(NULL != data);
    mender_err_t                    ret = MENDER_OK;
    mender_troubleshoot_protomsg_t *protomsg;
    mender_troubleshoot_protomsg_t *response = NULL;
    void                           *payload  = NULL;

    /* Unpack and decode message */
    if (NULL == (protomsg = mender_troubleshoot_protomsg_unpack(data, length))) {
        mender_log_error("Unable to decode message");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Verify integrity of the message */
    if (NULL == protomsg->hdr) {
        mender_log_error("Invalid message received");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Treatment of the message depending of the proto type */
    switch (protomsg->hdr->proto) {
        case MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_INVALID:
            mender_log_error("Invalid message received");
            ret = MENDER_FAIL;
            break;
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL
        case MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_SHELL:
            ret = mender_troubleshoot_shell_message_handler(protomsg, &response);
            break;
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */
        case MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_MENDER_CLIENT:
            ret = mender_troubleshoot_mender_client_message_handler(protomsg, &response);
            break;
        case MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_CONTROL:
            ret = mender_troubleshoot_control_message_handler(protomsg, &response);
            break;
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER
        case MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_FILE_TRANSFER:
            ret = mender_troubleshoot_file_transfer_message_handler(protomsg, &response);
            break;
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER */
        case MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_PORT_FORWARD:
            mender_log_error("Port Forwarding is not supported");
            ret = MENDER_FAIL;
            break;
        default:
            mender_log_error("Unsupported message received with proto type 0x%04x", protomsg->hdr->proto);
            ret = MENDER_FAIL;
            break;
    }

    /* Check if response is available */
    if (NULL != response) {

        /* Encode and pack the message */
        if (MENDER_OK != (ret = mender_troubleshoot_protomsg_pack(response, &payload, &length))) {
            mender_log_error("Unable to encode response");
            goto FAIL;
        }

        /* Send response */
        if (MENDER_OK != (ret = mender_troubleshoot_api_send(payload, length))) {
            mender_log_error("Unable to send response");
            goto FAIL;
        }
    }

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(protomsg);
    mender_troubleshoot_protomsg_release(response);
    if (NULL != payload) {
        free(payload);
    }

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */
