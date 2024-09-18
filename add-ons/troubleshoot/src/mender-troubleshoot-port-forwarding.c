/**
 * @file      mender-troubleshoot-port-forwarding.c
 * @brief     Mender MCU Troubleshoot add-on port forwarding message handler implementation
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
#include "mender-troubleshoot-port-forwarding.h"
#include "mender-troubleshoot-msgpack.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING

/**
 * @brief Message type
 */
#define MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_NEW     "new"
#define MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_STOP    "stop"
#define MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_FORWARD "forward"
#define MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_ACK     "ack"
#define MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_ERROR   "error"

/**
 * @brief Connect request
 */
typedef struct {
    char    *remote_host; /**< Remote host */
    uint16_t remote_port; /**< Remote port */
    char    *protocol;    /**< Protocol */
} mender_troubleshoot_port_forwarding_connect_t;

/**
 * @brief Error
 */
typedef struct {
    char *description; /**< Description */
    char *type;        /**< Type */
} mender_troubleshoot_port_forwarding_error_t;

/**
 * @brief State machine used for sending files to the server
 */
enum {
    MENDER_TROUBLESHOOT_FILE_TRANSFER_IDLE,    /**< Idle */
    MENDER_TROUBLESHOOT_FILE_TRANSFER_READING, /**< Reading file and sending chunks to the server */
    MENDER_TROUBLESHOOT_FILE_TRANSFER_EOF      /**< Waiting for the latest ack to close the handle */
} mender_troubleshoot_port_forwarding_state_machine
    = MENDER_TROUBLESHOOT_FILE_TRANSFER_IDLE;

/**
 * @brief Mender troubleshoot port forwarding callbacks
 */
static mender_troubleshoot_port_forwarding_callbacks_t mender_troubleshoot_port_forwarding_callbacks;

/**
 * @brief Mender troubleshoot port forwarding session ID
 */
static char *mender_troubleshoot_port_forwarding_sid = NULL;

/**
 * @brief Mender troubleshoot port forwarding connection ID
 */
static char *mender_troubleshoot_port_forwarding_connection_id = NULL;

/**
 * @brief Connection handle used to store temporary connection reference
 */
static void *mender_troubleshoot_port_forwarding_handle = NULL;

/**
 * @brief Function called to perform the treatment of the port forwarding new messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_port_forwarding_connect_message_handler(mender_troubleshoot_protomsg_t  *protomsg,
                                                                                mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the port forwarding stop messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_port_forwarding_close_message_handler(mender_troubleshoot_protomsg_t  *protomsg,
                                                                              mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the port forwarding forward messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_port_forwarding_forward_message_handler(mender_troubleshoot_protomsg_t  *protomsg,
                                                                                mender_troubleshoot_protomsg_t **response);

/**
 * @brief Unpack and decode connect request
 * @param data Packed data to be decoded
 * @param length Length of the data to be decoded
 * @return Connect if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_port_forwarding_connect_t *mender_troubleshoot_port_forwarding_connect_unpack(void *data, size_t length);

/**
 * @brief Decode connect object
 * @param object Connect object
 * @return Connect if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_port_forwarding_connect_t *mender_troubleshoot_port_forwarding_connect_decode(msgpack_object *object);

/**
 * @brief Release connect
 * @param connect Connect
 */
static void mender_troubleshoot_port_forwarding_connect_release(mender_troubleshoot_port_forwarding_connect_t *connect);

/**
 * @brief Function used to format port forwarding ack message
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_port_forwarding_format_ack(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function used to format port forwarding error message
 * @param protomsg Received proto message
 * @param error Error
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_port_forwarding_format_error(mender_troubleshoot_protomsg_t              *protomsg,
                                                                     mender_troubleshoot_port_forwarding_error_t *error,
                                                                     mender_troubleshoot_protomsg_t             **response);

/**
 * @brief Encode and pack error message
 * @param error Error
 * @param data Packed data encoded
 * @param length Length of the data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_port_forwarding_error_message_pack(mender_troubleshoot_port_forwarding_error_t *error, void **data, size_t *length);

/**
 * @brief Encode error message
 * @param error Error
 * @param data Packed data encoded
 * @param length Length of the data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_port_forwarding_error_message_encode(mender_troubleshoot_port_forwarding_error_t *error, msgpack_object *object);

/**
 * @brief Function called to send port forwarding stop protomsg
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_port_forwarding_send_stop(void);

mender_err_t
mender_troubleshoot_port_forwarding_init(mender_troubleshoot_port_forwarding_callbacks_t *callbacks) {

    /* Save callbacks */
    if (NULL != callbacks) {
        memcpy(&mender_troubleshoot_port_forwarding_callbacks, callbacks, sizeof(mender_troubleshoot_port_forwarding_callbacks_t));
    }

    return MENDER_OK;
}

mender_err_t
mender_troubleshoot_port_forwarding_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Verify integrity of the message */
    if (NULL == protomsg->hdr->typ) {
        mender_log_error("Invalid message received");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Treatment of the message depending of the message type */
    if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_NEW)) {
        /* Connect to the remote host */
        ret = mender_troubleshoot_port_forwarding_connect_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_STOP)) {
        /* Close connection to the remote host */
        ret = mender_troubleshoot_port_forwarding_close_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_FORWARD)) {
        /* Send data to the remote host */
        ret = mender_troubleshoot_port_forwarding_forward_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_ACK)) {
        /* Nothing to do */
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_ERROR)) {
        /* Nothing to do */
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
mender_troubleshoot_port_forwarding_forward(void *data, size_t length) {

    mender_troubleshoot_protomsg_t *protomsg = NULL;
    mender_err_t                    ret      = MENDER_OK;
    void                           *payload  = NULL;

    /* Check if a session is already opened */
    if (NULL == mender_troubleshoot_port_forwarding_sid) {
        mender_log_error("No port forwarding session opened");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Check if a connection is already opened */
    if (NULL == mender_troubleshoot_port_forwarding_connection_id) {
        mender_log_error("No port forwarding connection opened");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Send port forwarding forward message */
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
    protomsg->hdr->proto = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_PORT_FORWARD;
    if (NULL == (protomsg->hdr->typ = strdup(MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_FORWARD))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->hdr->sid = strdup(mender_troubleshoot_port_forwarding_sid))) {
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
    if (NULL == (protomsg->hdr->properties->connection_id = strdup(mender_troubleshoot_port_forwarding_connection_id))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
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
mender_troubleshoot_port_forwarding_close(void) {

    mender_err_t ret = MENDER_OK;

    /* Check if a session is already opened */
    if (NULL != mender_troubleshoot_port_forwarding_sid) {

        /* Check if a connection is already opened */
        if (NULL != mender_troubleshoot_port_forwarding_connection_id) {

            /* Invoke port forwarding close callback */
            if (NULL != mender_troubleshoot_port_forwarding_callbacks.close) {
                if (MENDER_OK != mender_troubleshoot_port_forwarding_callbacks.close(mender_troubleshoot_port_forwarding_handle)) {
                    mender_log_error("An error occured");
                }
            }

            /* Send stop message to the server */
            if (MENDER_OK != (ret = mender_troubleshoot_port_forwarding_send_stop())) {
                mender_log_error("Unable to send stop message to the server");
            }

            /* Release connection ID */
            free(mender_troubleshoot_port_forwarding_connection_id);
            mender_troubleshoot_port_forwarding_connection_id = NULL;
        }

        /* Release session ID */
        free(mender_troubleshoot_port_forwarding_sid);
        mender_troubleshoot_port_forwarding_sid = NULL;
    }

    return ret;
}

mender_err_t
mender_troubleshoot_port_forwarding_exit(void) {

    mender_err_t ret;

    /* Close connection if it is opened */
    if (MENDER_OK != (ret = mender_troubleshoot_port_forwarding_close())) {
        mender_log_error("Unable to close connection");
    }

    return ret;
}

static mender_err_t
mender_troubleshoot_port_forwarding_connect_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_troubleshoot_port_forwarding_error_t    error;
    mender_troubleshoot_port_forwarding_connect_t *connect = NULL;
    mender_err_t                                   ret     = MENDER_OK;

    /* Initialize error message */
    memset(&error, 0, sizeof(mender_troubleshoot_port_forwarding_error_t));

    /* Verify integrity of the message */
    if (NULL == protomsg->hdr) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->sid) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties->connection_id) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties->user_id) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->body) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->body->data) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Check is a session is already opened */
    if ((NULL != mender_troubleshoot_port_forwarding_sid) || (NULL != mender_troubleshoot_port_forwarding_connection_id)) {
        mender_log_warning("A port forwarding session is already opened");
        error.description = "A port forwarding session is already opened";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Start port forwarding session */
    mender_log_info("Starting a new port forwarding session");

    /* Save the session ID and connection ID */
    if (NULL == (mender_troubleshoot_port_forwarding_sid = strdup(protomsg->hdr->sid))) {
        mender_log_error("Unable to allocate memory");
        error.description = "Internal error";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (mender_troubleshoot_port_forwarding_connection_id = strdup(protomsg->hdr->properties->connection_id))) {
        mender_log_error("Unable to allocate memory");
        error.description = "Internal error";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Unpack and decode data */
    if (NULL == (connect = mender_troubleshoot_port_forwarding_connect_unpack(protomsg->body->data, protomsg->body->length))) {
        mender_log_error("Unable to decode connect request");
        error.description = "Unable to decode connect request";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Verify integrity of the connect request */
    if ((NULL == connect->remote_host) || (0 == connect->remote_port) || (NULL == connect->protocol)) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Connect to the remote host */
    if (NULL != mender_troubleshoot_port_forwarding_callbacks.connect) {
        if (MENDER_OK
            != (ret = mender_troubleshoot_port_forwarding_callbacks.connect(
                    connect->remote_host, connect->remote_port, connect->protocol, &mender_troubleshoot_port_forwarding_handle))) {
            mender_log_error("Unable to connect to '%s:%d' with protocol '%s'", connect->remote_host, connect->remote_port, connect->protocol);
            error.description = "Unable to connect to remote host";
            goto FAIL;
        }
    }

    /* Format ack */
    if (MENDER_OK != (ret = mender_troubleshoot_port_forwarding_format_ack(protomsg, response))) {
        mender_log_error("Unable to format response");
        error.description = "Unable to format response";
        goto FAIL;
    }

    /* Release memory */
    mender_troubleshoot_port_forwarding_connect_release(connect);

    return ret;

FAIL:

    /* Format error */
    if (MENDER_OK != mender_troubleshoot_port_forwarding_format_error(protomsg, &error, response)) {
        mender_log_error("Unable to format response");
    }

    /* Release memory */
    mender_troubleshoot_port_forwarding_connect_release(connect);

    /* Release connection ID */
    if (NULL != mender_troubleshoot_port_forwarding_connection_id) {
        free(mender_troubleshoot_port_forwarding_connection_id);
        mender_troubleshoot_port_forwarding_connection_id = NULL;
    }

    /* Release session ID */
    if (NULL != mender_troubleshoot_port_forwarding_sid) {
        free(mender_troubleshoot_port_forwarding_sid);
        mender_troubleshoot_port_forwarding_sid = NULL;
    }

    return ret;
}

static mender_err_t
mender_troubleshoot_port_forwarding_close_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_troubleshoot_port_forwarding_error_t error;
    mender_err_t                                ret = MENDER_OK;

    /* Initialize error message */
    memset(&error, 0, sizeof(mender_troubleshoot_port_forwarding_error_t));

    /* Check if a session is already opened */
    if (NULL == mender_troubleshoot_port_forwarding_sid) {
        mender_log_error("No port forwarding session opened");
        error.description = "No port forwarding session opened";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Check if a connection is already opened */
    if (NULL == mender_troubleshoot_port_forwarding_connection_id) {
        mender_log_error("No port forwarding connection opened");
        error.description = "No port forwarding connection opened";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Verify integrity of the message */
    if (NULL == protomsg->hdr) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->sid) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties->connection_id) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties->user_id) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Close connection to the remote host */
    if (NULL != mender_troubleshoot_port_forwarding_callbacks.close) {
        if (MENDER_OK != (ret = mender_troubleshoot_port_forwarding_callbacks.close(mender_troubleshoot_port_forwarding_handle))) {
            mender_log_error("Unable to close connection");
            error.description = "Unable to close connection";
            goto FAIL;
        }
    }

    /* Format ack */
    if (MENDER_OK != (ret = mender_troubleshoot_port_forwarding_format_ack(protomsg, response))) {
        mender_log_error("Unable to format response");
        error.description = "Unable to format response";
        goto FAIL;
    }

    /* Release connection ID */
    if (NULL != mender_troubleshoot_port_forwarding_connection_id) {
        free(mender_troubleshoot_port_forwarding_connection_id);
        mender_troubleshoot_port_forwarding_connection_id = NULL;
    }

    /* Release session ID */
    if (NULL != mender_troubleshoot_port_forwarding_sid) {
        free(mender_troubleshoot_port_forwarding_sid);
        mender_troubleshoot_port_forwarding_sid = NULL;
    }

    return ret;

FAIL:

    /* Format error */
    if (MENDER_OK != mender_troubleshoot_port_forwarding_format_error(protomsg, &error, response)) {
        mender_log_error("Unable to format response");
    }

    return ret;
}

static mender_err_t
mender_troubleshoot_port_forwarding_forward_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_troubleshoot_port_forwarding_error_t error;
    mender_err_t                                ret = MENDER_OK;

    /* Initialize error message */
    memset(&error, 0, sizeof(mender_troubleshoot_port_forwarding_error_t));

    /* Check if a session is already opened */
    if (NULL == mender_troubleshoot_port_forwarding_sid) {
        mender_log_error("No port forwarding session opened");
        error.description = "No port forwarding session opened";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Check if a connection is already opened */
    if (NULL == mender_troubleshoot_port_forwarding_connection_id) {
        mender_log_error("No port forwarding connection opened");
        error.description = "No port forwarding connection opened";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Verify integrity of the message */
    if (NULL == protomsg->hdr) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->sid) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties->connection_id) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->hdr->properties->user_id) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->body) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == protomsg->body->data) {
        mender_log_error("Invalid message received");
        error.description = "Invalid message received";
        ret               = MENDER_FAIL;
        goto FAIL;
    }

    /* Send data to the remote host */
    if (NULL != mender_troubleshoot_port_forwarding_callbacks.send) {
        if (MENDER_OK
            != (ret = mender_troubleshoot_port_forwarding_callbacks.send(
                    mender_troubleshoot_port_forwarding_handle, protomsg->body->data, protomsg->body->length))) {
            mender_log_error("Unable to send data to remote host");
            error.description = "Unable to send data to remote host";
            goto FAIL;
        }
    }

    /* Format ack */
    if (MENDER_OK != (ret = mender_troubleshoot_port_forwarding_format_ack(protomsg, response))) {
        mender_log_error("Unable to format response");
        error.description = "Unable to format response";
        goto FAIL;
    }

    return ret;

FAIL:

    /* Format error */
    if (MENDER_OK != mender_troubleshoot_port_forwarding_format_error(protomsg, &error, response)) {
        mender_log_error("Unable to format response");
    }

    return ret;
}

static mender_troubleshoot_port_forwarding_connect_t *
mender_troubleshoot_port_forwarding_connect_unpack(void *data, size_t length) {

    assert(NULL != data);
    msgpack_zone                                   zone;
    msgpack_object                                 object;
    mender_troubleshoot_port_forwarding_connect_t *connect = NULL;

    /* Unpack the message */
    if (MENDER_OK != mender_troubleshoot_msgpack_unpack_object(data, length, &zone, &object)) {
        mender_log_error("Unable to unpack the data");
        goto END;
    }

    /* Check object type */
    if ((MSGPACK_OBJECT_MAP != object.type) || (0 == object.via.map.size)) {
        mender_log_error("Invalid upload request object");
        goto END;
    }

    /* Decode connect */
    if (NULL == (connect = mender_troubleshoot_port_forwarding_connect_decode(&object))) {
        mender_log_error("Invalid connect object");
        goto END;
    }

END:

    /* Release memory */
    msgpack_zone_destroy(&zone);

    return connect;
}

static mender_troubleshoot_port_forwarding_connect_t *
mender_troubleshoot_port_forwarding_connect_decode(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_port_forwarding_connect_t *connect;

    /* Create connect */
    if (NULL == (connect = (mender_troubleshoot_port_forwarding_connect_t *)malloc(sizeof(mender_troubleshoot_port_forwarding_connect_t)))) {
        mender_log_error("Unable to allocate memory");
        return NULL;
    }
    memset(connect, 0, sizeof(mender_troubleshoot_port_forwarding_connect_t));

    /* Parse upload request */
    msgpack_object_kv *p = object->via.map.ptr;
    do {
        if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "remote_host", p->key.via.str.size)) && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (connect->remote_host = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(connect->remote_host, p->val.via.str.ptr, p->val.via.str.size);
            connect->remote_host[p->val.via.str.size] = '\0';
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "remote_port", p->key.via.str.size))
                   && (MSGPACK_OBJECT_POSITIVE_INTEGER == p->val.type)) {
            connect->remote_port = (uint16_t)p->val.via.u64;
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "protocol", p->key.via.str.size))
                   && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (connect->protocol = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(connect->protocol, p->val.via.str.ptr, p->val.via.str.size);
            connect->protocol[p->val.via.str.size] = '\0';
        }
        ++p;
    } while (p < object->via.map.ptr + object->via.map.size);

    return connect;

FAIL:

    /* Release memory */
    mender_troubleshoot_port_forwarding_connect_release(connect);

    return NULL;
}

static void
mender_troubleshoot_port_forwarding_connect_release(mender_troubleshoot_port_forwarding_connect_t *connect) {

    /* Release memory */
    if (NULL != connect) {
        if (NULL != connect->remote_host) {
            free(connect->remote_host);
        }
        if (NULL != connect->protocol) {
            free(connect->protocol);
        }
        free(connect);
    }
}

static mender_err_t
mender_troubleshoot_port_forwarding_format_ack(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Format port forwarding ack message */
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
    if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_FORWARD)) {
        if (NULL == ((*response)->hdr->typ = strdup(MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_ACK))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
    } else {
        if (NULL == ((*response)->hdr->typ = strdup(protomsg->hdr->typ))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
    }
    if (NULL != protomsg->hdr->sid) {
        if (NULL == ((*response)->hdr->sid = strdup(protomsg->hdr->sid))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
    }
    if (NULL != protomsg->hdr->properties) {
        if (NULL
            == ((*response)->hdr->properties
                = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset((*response)->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
        if (NULL != protomsg->hdr->properties->connection_id) {
            if (NULL == ((*response)->hdr->properties->connection_id = strdup(protomsg->hdr->properties->connection_id))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto FAIL;
            }
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
mender_troubleshoot_port_forwarding_format_error(mender_troubleshoot_protomsg_t              *protomsg,
                                                 mender_troubleshoot_port_forwarding_error_t *error,
                                                 mender_troubleshoot_protomsg_t             **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Format port forwarding error message */
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
    if (NULL == ((*response)->hdr->typ = strdup(MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_ERROR))) {
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
    if (NULL != protomsg->hdr->properties) {
        if (NULL
            == ((*response)->hdr->properties
                = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset((*response)->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
        if (NULL != protomsg->hdr->properties->user_id) {
            if (NULL == ((*response)->hdr->properties->user_id = strdup(protomsg->hdr->properties->user_id))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto FAIL;
            }
        }
    }
    if (NULL == ((*response)->body = (mender_troubleshoot_protomsg_body_t *)malloc(sizeof(mender_troubleshoot_protomsg_body_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->body, 0, sizeof(mender_troubleshoot_protomsg_body_t));

    /* Encode and pack error message */
    if (MENDER_OK != (ret = mender_troubleshoot_port_forwarding_error_message_pack(error, &((*response)->body->data), &((*response)->body->length)))) {
        mender_log_error("Unable to encode message");
        goto FAIL;
    }

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(*response);
    *response = NULL;

    return ret;
}

/**
 * @brief Encode and pack error message
 * @param error Error
 * @param data Packed data encoded
 * @param length Length of the data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t
mender_troubleshoot_port_forwarding_error_message_pack(mender_troubleshoot_port_forwarding_error_t *error, void **data, size_t *length) {

    assert(NULL != error);
    assert(NULL != data);
    assert(NULL != length);
    mender_err_t   ret = MENDER_OK;
    msgpack_object object;

    /* Encode error message */
    if (MENDER_OK != (ret = mender_troubleshoot_port_forwarding_error_message_encode(error, &object))) {
        mender_log_error("Invalid error message object");
        goto FAIL;
    }

    /* Pack error message */
    if (MENDER_OK != (ret = mender_troubleshoot_msgpack_pack_object(&object, data, length))) {
        mender_log_error("Unable to pack error message object");
        goto FAIL;
    }

    /* Release memory */
    mender_troubleshoot_msgpack_release_object(&object);

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_msgpack_release_object(&object);

    return ret;
}

static mender_err_t
mender_troubleshoot_port_forwarding_error_message_encode(mender_troubleshoot_port_forwarding_error_t *error, msgpack_object *object) {

    assert(NULL != error);
    assert(NULL != object);
    mender_err_t       ret = MENDER_OK;
    msgpack_object_kv *p;

    /* Create error */
    object->type = MSGPACK_OBJECT_MAP;
    if (0 == (object->via.map.size = ((NULL != error->description) ? 1 : 0) + ((NULL != error->type) ? 1 : 0))) {
        goto END;
    }
    if (NULL == (object->via.map.ptr = (msgpack_object_kv *)malloc(object->via.map.size * sizeof(struct msgpack_object_kv)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Parse error */
    p = object->via.map.ptr;
    if (NULL != error->description) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("err"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("err");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = (uint32_t)strlen(error->description);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy((void *)p->val.via.str.ptr, error->description, p->val.via.str.size);
        ++p;
    }
    if (NULL != error->type) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("msgtype"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("msgtype");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = (uint32_t)strlen(error->type);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy((void *)p->val.via.str.ptr, error->type, p->val.via.str.size);
        ++p;
    }

END:
FAIL:

    return ret;
}

static mender_err_t
mender_troubleshoot_port_forwarding_send_stop(void) {

    mender_troubleshoot_protomsg_t *protomsg = NULL;
    mender_err_t                    ret      = MENDER_OK;
    void                           *payload  = NULL;
    size_t                          length   = 0;

    /* Send port forwarding close message */
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
    protomsg->hdr->proto = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_PORT_FORWARD;
    if (NULL == (protomsg->hdr->typ = strdup(MENDER_TROUBLESHOOT_PORT_FORWARD_MESSAGE_TYPE_STOP))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL != mender_troubleshoot_port_forwarding_sid) {
        if (NULL == (protomsg->hdr->sid = strdup(mender_troubleshoot_port_forwarding_sid))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
    }
    if (NULL == (protomsg->hdr->properties = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->hdr->properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));
    if (NULL != mender_troubleshoot_port_forwarding_connection_id) {
        if (NULL == (protomsg->hdr->properties->connection_id = strdup(mender_troubleshoot_port_forwarding_connection_id))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
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

#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */
