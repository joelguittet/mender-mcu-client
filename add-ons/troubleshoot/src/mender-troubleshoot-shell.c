/**
 * @file      mender-troubleshoot-shell.c
 * @brief     Mender MCU Troubleshoot add-on shell message handler implementation
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

#include "mender-log.h"
#include "mender-troubleshoot-api.h"
#include "mender-troubleshoot-msgpack.h"
#include "mender-troubleshoot-shell.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL

/**
 * @brief Message type
 */
#define MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_PING   "ping"
#define MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_PONG   "pong"
#define MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_RESIZE "resize"
#define MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_SHELL  "shell"
#define MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_SPAWN  "new"
#define MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_STOP   "stop"

/**
 * @brief Mender troubleshoot shell config
 */
static mender_troubleshoot_shell_config_t mender_troubleshoot_shell_config;

/**
 * @brief Mender troubleshoot shell callbacks
 */
static mender_troubleshoot_shell_callbacks_t mender_troubleshoot_shell_callbacks;

/**
 * @brief Mender troubleshoot shell session ID
 */
static char *mender_troubleshoot_shell_sid = NULL;

/**
 * @brief Function called to perform the treatment of the shell ping messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_shell_ping_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the shell resize messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_shell_resize_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the shell shell messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_shell_shell_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the shell spawn messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_shell_spawn_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the shell stop messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_shell_stop_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function used to format shell pong message
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_shell_format_pong(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function used to format acknowledgment messages
 * @param protomsg Received proto message
 * @param status Status
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_shell_format_ack(mender_troubleshoot_protomsg_t                      *protomsg,
                                                         mender_troubleshoot_protomsg_hdr_properties_status_t status,
                                                         mender_troubleshoot_protomsg_t                     **response);

/**
 * @brief Function called to send shell ping protomsg
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_shell_send_ping(void);

/**
 * @brief Function called to send shell stop protomsg
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_shell_send_stop(void);

mender_err_t
mender_troubleshoot_shell_init(mender_troubleshoot_shell_config_t *config, mender_troubleshoot_shell_callbacks_t *callbacks) {

    /* Save configuration */
    memcpy(&mender_troubleshoot_shell_config, config, sizeof(mender_troubleshoot_shell_config_t));

    /* Save callbacks */
    if (NULL != callbacks) {
        memcpy(&mender_troubleshoot_shell_callbacks, callbacks, sizeof(mender_troubleshoot_shell_callbacks_t));
    }

    return MENDER_OK;
}

mender_err_t
mender_troubleshoot_shell_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Verify integrity of the message */
    if ((NULL == protomsg->hdr->typ) || (NULL == protomsg->hdr->sid)) {
        mender_log_error("Invalid message received");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Treatment of the message depending of the message type */
    if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_PING)) {
        /* Format pong */
        ret = mender_troubleshoot_shell_ping_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_PONG)) {
        /* Nothing to do */
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_RESIZE)) {
        /* Resize shell */
        ret = mender_troubleshoot_shell_resize_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_SHELL)) {
        /* Shell data */
        ret = mender_troubleshoot_shell_shell_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_SPAWN)) {
        /* Spawn shell */
        ret = mender_troubleshoot_shell_spawn_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_STOP)) {
        /* Stop shell */
        ret = mender_troubleshoot_shell_stop_message_handler(protomsg, response);
    } else {
        /* Not supported */
        mender_log_error("Unsupported message received with message type '%s'", protomsg->hdr->typ);
        ret = MENDER_FAIL;
        goto FAIL;
    }

FAIL:

    return ret;
}

mender_err_t
mender_troubleshoot_shell_healthcheck(void) {

    mender_err_t ret = MENDER_OK;

    /* Check if a session is already opened */
    if (NULL != mender_troubleshoot_shell_sid) {

        /* Send healthcheck ping message over websocket connection */
        if (MENDER_OK != (ret = mender_troubleshoot_shell_send_ping())) {
            mender_log_error("Unable to send healthcheck message to the server");
            goto FAIL;
        }
    }

FAIL:

    return ret;
}

mender_err_t
mender_troubleshoot_shell_print(void *data, size_t length) {

    mender_troubleshoot_protomsg_t *protomsg = NULL;
    mender_err_t                    ret      = MENDER_OK;
    void                           *payload  = NULL;

    /* Check if a session is already opened */
    if (NULL == mender_troubleshoot_shell_sid) {
        mender_log_error("No shell session opened");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Send shell body */
    if (NULL == (protomsg = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == (protomsg->hdr = (mender_troubleshoot_protomsg_hdr_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->hdr, 0, sizeof(mender_troubleshoot_protomsg_hdr_t));
    protomsg->hdr->proto = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_SHELL;
    if (NULL == (protomsg->hdr->typ = strdup(MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_SHELL))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->hdr->sid = strdup(mender_troubleshoot_shell_sid))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->hdr->properties = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
    if (NULL
        == (protomsg->hdr->properties->status
            = (mender_troubleshoot_protomsg_hdr_properties_status_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_status_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    *(protomsg->hdr->properties->status) = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_NORMAL;
    if ((NULL != data) && (0 != length)) {
        if (NULL == (protomsg->body = (mender_troubleshoot_protomsg_body_t *)malloc(sizeof(mender_troubleshoot_protomsg_body_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset(protomsg->body, 0, sizeof(mender_troubleshoot_protomsg_body_t));
        if (NULL == (protomsg->body->data = malloc(length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy(protomsg->body->data, data, length);
        protomsg->body->length = length;
    }

    /* Encode and pack the message */
    if (MENDER_OK != (ret = mender_troubleshoot_protomsg_pack(protomsg, &payload, &length))) {
        mender_log_error("Unable to encode message");
        goto FAIL;
    }

    /* Send message */
    if (MENDER_OK != (ret = mender_troubleshoot_api_send(payload, length))) {
        mender_log_error("Unable to send message");
        goto FAIL;
    }

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(protomsg);
    if (NULL != payload) {
        free(payload);
    }

    return ret;
}

mender_err_t
mender_troubleshoot_shell_close(void) {

    mender_err_t ret = MENDER_OK;

    /* Check if a session is already opened */
    if (NULL != mender_troubleshoot_shell_sid) {

        /* Invoke shell close callback */
        if (NULL != mender_troubleshoot_shell_callbacks.close) {
            if (MENDER_OK != mender_troubleshoot_shell_callbacks.close()) {
                mender_log_error("An error occured");
            }
        }

        /* Send stop message to the server */
        if (MENDER_OK != (ret = mender_troubleshoot_shell_send_stop())) {
            mender_log_error("Unable to send stop message to the server");
        }

        /* Release session ID */
        free(mender_troubleshoot_shell_sid);
        mender_troubleshoot_shell_sid = NULL;
    }

    return ret;
}

mender_err_t
mender_troubleshoot_shell_exit(void) {

    /* Release memory */
    if (NULL != mender_troubleshoot_shell_sid) {
        free(mender_troubleshoot_shell_sid);
        mender_troubleshoot_shell_sid = NULL;
    }

    return MENDER_OK;
}

static mender_err_t
mender_troubleshoot_shell_ping_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_err_t ret = MENDER_OK;

    /* Format pong */
    if (MENDER_OK != (ret = mender_troubleshoot_shell_format_pong(protomsg, response))) {
        mender_log_error("Unable to format pong message");
        goto FAIL;
    }

FAIL:

    return ret;
}

static mender_err_t
mender_troubleshoot_shell_resize_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    (void)response;
    mender_err_t ret = MENDER_OK;

    /* Verify integrity of the message */
    if ((NULL == protomsg->hdr->properties) || (NULL == protomsg->hdr->properties->terminal_width) || (NULL == protomsg->hdr->properties->terminal_height)) {
        mender_log_error("Invalid message received");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Invoke shell resize callback */
    if (NULL != mender_troubleshoot_shell_callbacks.resize) {
        if (MENDER_OK
            != (ret = mender_troubleshoot_shell_callbacks.resize(*protomsg->hdr->properties->terminal_width, *protomsg->hdr->properties->terminal_height))) {
            mender_log_error("An error occured");
            goto FAIL;
        }
    }

FAIL:

    return ret;
}

static mender_err_t
mender_troubleshoot_shell_shell_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    (void)response;
    mender_err_t ret = MENDER_OK;

    /* Verify integrity of the message */
    if (NULL == protomsg->body) {
        mender_log_error("Invalid message received");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->body->data) {
        mender_log_error("Invalid message received");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Invoke shell data write callback */
    if (NULL != mender_troubleshoot_shell_callbacks.write) {
        if (MENDER_OK != (ret = mender_troubleshoot_shell_callbacks.write(protomsg->body->data, protomsg->body->length))) {
            mender_log_error("An error occured");
            goto FAIL;
        }
    }

FAIL:

    return ret;
}

static mender_err_t
mender_troubleshoot_shell_spawn_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_err_t ret = MENDER_OK;

    /* Check is a session is already opened */
    if (NULL != mender_troubleshoot_shell_sid) {
        mender_log_warning("A shell session is already opened");
        goto FAIL;
    }

    /* Start shell session */
    mender_log_info("Starting a new shell session");

    /* Save the session ID */
    if (NULL == (mender_troubleshoot_shell_sid = strdup(protomsg->hdr->sid))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Format acknowledgment */
    if (MENDER_OK
        != (ret = mender_troubleshoot_shell_format_ack(protomsg,
                                                       (MENDER_OK == ret) ? MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_NORMAL
                                                                          : MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_ERROR,
                                                       response))) {
        mender_log_error("Unable to format response");
        goto FAIL;
    }

    /* Invoke shell open callback */
    if (NULL != mender_troubleshoot_shell_callbacks.open) {
        if ((NULL != protomsg->hdr->properties) && (NULL != protomsg->hdr->properties->terminal_width)
            && (NULL != protomsg->hdr->properties->terminal_height)) {
            ret = mender_troubleshoot_shell_callbacks.open(*protomsg->hdr->properties->terminal_width, *protomsg->hdr->properties->terminal_height);
        } else {
            ret = mender_troubleshoot_shell_callbacks.open(0, 0);
        }
        if (MENDER_OK != ret) {
            mender_log_error("An error occured");
            goto FAIL;
        }
    }

FAIL:

    return ret;
}

static mender_err_t
mender_troubleshoot_shell_stop_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_err_t ret = MENDER_OK;

    /* Check is a session is already opened */
    if (NULL == mender_troubleshoot_shell_sid) {
        mender_log_warning("No shell session opened");
        goto FAIL;
    }

    /* Stop shell session */
    mender_log_info("Stopping current shell session");

    /* Invoke shell close callback */
    if (NULL != mender_troubleshoot_shell_callbacks.close) {
        if (MENDER_OK != (ret = mender_troubleshoot_shell_callbacks.close())) {
            mender_log_error("An error occured");
        }
    }

    /* Format acknowledgment */
    if (MENDER_OK
        != (ret = mender_troubleshoot_shell_format_ack(protomsg,
                                                       (MENDER_OK == ret) ? MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_NORMAL
                                                                          : MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_ERROR,
                                                       response))) {
        mender_log_error("Unable to format response");
    }

    /* Release session ID */
    if (NULL != mender_troubleshoot_shell_sid) {
        free(mender_troubleshoot_shell_sid);
        mender_troubleshoot_shell_sid = NULL;
    }

FAIL:

    return ret;
}

static mender_err_t
mender_troubleshoot_shell_format_pong(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Format shell pong message */
    if (NULL == (*response = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(*response, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == ((*response)->hdr = (mender_troubleshoot_protomsg_hdr_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->hdr, 0, sizeof(mender_troubleshoot_protomsg_hdr_t));
    (*response)->hdr->proto = protomsg->hdr->proto;
    if (NULL == ((*response)->hdr->typ = strdup(MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_PONG))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL != protomsg->hdr->sid) {
        if (NULL == ((*response)->hdr->sid = strdup(protomsg->hdr->sid))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
    }

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(*response);
    *response = NULL;

    return ret;
}

static mender_err_t
mender_troubleshoot_shell_format_ack(mender_troubleshoot_protomsg_t                      *protomsg,
                                     mender_troubleshoot_protomsg_hdr_properties_status_t status,
                                     mender_troubleshoot_protomsg_t                     **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Format acknowledgment message */
    if (NULL == (*response = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(*response, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == ((*response)->hdr = (mender_troubleshoot_protomsg_hdr_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->hdr, 0, sizeof(mender_troubleshoot_protomsg_hdr_t));
    (*response)->hdr->proto = protomsg->hdr->proto;
    if (NULL == ((*response)->hdr->typ = strdup(protomsg->hdr->typ))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL != mender_troubleshoot_shell_sid) {
        if (NULL == ((*response)->hdr->sid = strdup(mender_troubleshoot_shell_sid))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
    }
    if (NULL
        == ((*response)->hdr->properties = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
    if (NULL
        == ((*response)->hdr->properties->status
            = (mender_troubleshoot_protomsg_hdr_properties_status_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_status_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    *((*response)->hdr->properties->status) = status;

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(*response);
    *response = NULL;

    return ret;
}

static mender_err_t
mender_troubleshoot_shell_send_ping(void) {

    mender_troubleshoot_protomsg_t *protomsg = NULL;
    mender_err_t                    ret      = MENDER_OK;
    void                           *payload  = NULL;
    size_t                          length   = 0;

    /* Send shell ping message */
    if (NULL == (protomsg = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == (protomsg->hdr = (mender_troubleshoot_protomsg_hdr_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->hdr, 0, sizeof(mender_troubleshoot_protomsg_hdr_t));
    protomsg->hdr->proto = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_SHELL;
    if (NULL == (protomsg->hdr->typ = strdup(MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_PING))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->hdr->sid = strdup(mender_troubleshoot_shell_sid))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->hdr->properties = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
    if (mender_troubleshoot_shell_config.healthcheck_interval > 0) {
        if (NULL == (protomsg->hdr->properties->timeout = (uint32_t *)malloc(sizeof(uint32_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        *protomsg->hdr->properties->timeout = 2 * mender_troubleshoot_shell_config.healthcheck_interval;
    }
    if (NULL
        == (protomsg->hdr->properties->status
            = (mender_troubleshoot_protomsg_hdr_properties_status_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_status_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    *protomsg->hdr->properties->status = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_CONTROL;

    /* Encode and pack the message */
    if (MENDER_OK != (ret = mender_troubleshoot_protomsg_pack(protomsg, &payload, &length))) {
        mender_log_error("Unable to encode message");
        goto FAIL;
    }

    /* Send message */
    if (MENDER_OK != (ret = mender_troubleshoot_api_send(payload, length))) {
        mender_log_error("Unable to send message");
        goto FAIL;
    }

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(protomsg);
    if (NULL != payload) {
        free(payload);
    }

    return ret;
}

static mender_err_t
mender_troubleshoot_shell_send_stop(void) {

    mender_troubleshoot_protomsg_t *protomsg = NULL;
    mender_err_t                    ret      = MENDER_OK;
    void                           *payload  = NULL;
    size_t                          length   = 0;

    /* Send shell stop message */
    if (NULL == (protomsg = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == (protomsg->hdr = (mender_troubleshoot_protomsg_hdr_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->hdr, 0, sizeof(mender_troubleshoot_protomsg_hdr_t));
    protomsg->hdr->proto = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_SHELL;
    if (NULL == (protomsg->hdr->typ = strdup(MENDER_TROUBLESHOOT_SHELL_MESSAGE_TYPE_STOP))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->hdr->sid = strdup(mender_troubleshoot_shell_sid))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->hdr->properties = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
    if (NULL
        == (protomsg->hdr->properties->status
            = (mender_troubleshoot_protomsg_hdr_properties_status_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_status_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    *protomsg->hdr->properties->status = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_ERROR;

    /* Encode and pack the message */
    if (MENDER_OK != (ret = mender_troubleshoot_protomsg_pack(protomsg, &payload, &length))) {
        mender_log_error("Unable to encode message");
        goto FAIL;
    }

    /* Send message */
    if (MENDER_OK != (ret = mender_troubleshoot_api_send(payload, length))) {
        mender_log_error("Unable to send message");
        goto FAIL;
    }

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(protomsg);
    if (NULL != payload) {
        free(payload);
    }

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */
