/**
 * @file      mender-troubleshoot.c
 * @brief     Mender MCU Troubleshoot add-on implementation
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

#include "mender-api.h"
#include "mender-troubleshoot.h"
#include "mender-log.h"

#ifndef CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS
#include "mender-rtos.h"
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS */

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

#include <msgpack.h>

/**
 * @brief Default troubleshoot healthcheck interval (seconds)
 */
#ifndef CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL
#define CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL (30)
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL */

/**
 * msgpack zone chunk initialization size
 */
#define MENDER_TROUBLESHOOT_ZONE_CHUNK_INIT_SIZE (2048)

/**
 * msgpack sbuffer initialization size
 */
#define MENDER_TROUBLESHOOT_SBUFFER_INIT_SIZE (256)

/**
 * Proto type
 */
typedef enum {
    MENDER_TROUBLESHOOT_PROTO_TYPE_INVALID       = 0x0000, /**< Invalid */
    MENDER_TROUBLESHOOT_PROTO_TYPE_SHELL         = 0x0001, /**< Shell */
    MENDER_TROUBLESHOOT_PROTO_TYPE_FILE_TRANSFER = 0x0002, /**< File transfer */
    MENDER_TROUBLESHOOT_PROTO_TYPE_PORT_FORWARD  = 0x0003, /**< Port forward */
    MENDER_TROUBLESHOOT_PROTO_TYPE_MENDER_CLIENT = 0x0004, /**< Mender client */
    MENDER_TROUBLESHOOT_PROTO_TYPE_CONTROL       = 0xFFFF  /**< Control */
} mender_troubleshoot_protohdr_type_t;

/**
 * Message type
 */
#define MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_PING   "ping"
#define MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_PONG   "pong"
#define MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_RESIZE "resize"
#define MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_SHELL  "shell"
#define MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_SPAWN  "new"
#define MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_STOP   "stop"

/**
 * Status type
 */
typedef enum {
    MENDER_TROUBLESHOOT_STATUS_TYPE_NORMAL  = 0x0001, /**< Normal message */
    MENDER_TROUBLESHOOT_STATUS_TYPE_ERROR   = 0x0002, /**< Error message */
    MENDER_TROUBLESHOOT_STATUS_TYPE_CONTROL = 0x0003  /**< Control message */
} mender_troubleshoot_properties_status_t;

/**
 * Proto message header properties
 */
typedef struct {
    uint16_t *                               terminal_width;  /**< Terminal width */
    uint16_t *                               terminal_height; /**< Terminal heigth */
    char *                                   user_id;         /**< User ID */
    uint32_t *                               timeout;         /**< Timeout */
    mender_troubleshoot_properties_status_t *status;          /**< Status */
} mender_troubleshoot_protohdr_properties_t;

/**
 * Proto message header
 */
typedef struct {
    mender_troubleshoot_protohdr_type_t        proto;      /**< Proto type */
    char *                                     typ;        /**< Message type */
    char *                                     sid;        /**< Session ID */
    mender_troubleshoot_protohdr_properties_t *properties; /**< Properties */
} mender_troubleshoot_protohdr_t;

/**
 * Proto message
 */
typedef struct {
    mender_troubleshoot_protohdr_t *protohdr; /**< Header */
    char *                          body;     /**< Body */
} mender_troubleshoot_protomsg_t;

/**
 * @brief Mender troubleshoot configuration
 */
static mender_troubleshoot_config_t mender_troubleshoot_config;

/**
 * @brief Mender troubleshoot callbacks
 */
static mender_troubleshoot_callbacks_t mender_troubleshoot_callbacks;

/**
 * @brief Mender troubleshoot work handle
 */
static void *mender_troubleshoot_healthcheck_work_handle = NULL;

#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS
/**
 * @brief Mender troubleshoot connection handle
 */
static void *mender_troubleshoot_handle = NULL;
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS */

/**
 * @brief Mender troubleshoot shell session ID
 */
static char *mender_troubleshoot_shell_sid = NULL;

/**
 * @brief Callback function to be invoked to perform the treatment of the data from the websocket
 * @param data Received data
 * @param length Received data length
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_data_received_callback(void *data, size_t length);

/**
 * @brief Function called to perform the treatment of the shell messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_shell_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to send shell ping protomsg
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_send_shell_ping_protomsg(void);

/**
 * @brief Function called to send shell stop protomsg
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_send_shell_stop_protomsg(void);

/**
 * @brief Unpack and decode Proto message
 * @param data Packed data to be decoded
 * @param length Length of the data to be decoded
 * @return Proto message if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_protomsg_t *mender_troubleshoot_unpack_protomsg(void *data, size_t length);

/**
 * @brief Decode Proto message object
 * @param object Proto message object
 * @return Proto message if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_protomsg_t *mender_troubleshoot_decode_protomsg(msgpack_object *object);

/**
 * @brief Decode Proto header object
 * @param object Proto header object
 * @return Proto header if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_protohdr_t *mender_troubleshoot_decode_protohdr(msgpack_object *object);

/**
 * @brief Decode Proto header properties object
 * @param object Proto header properties object
 * @return Proto header properties if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_protohdr_properties_t *mender_troubleshoot_decode_protohdr_properties(msgpack_object *object);

/**
 * @brief Decode body object
 * @param object Body object
 * @return Body if the function succeeds, NULL otherwise
 */
static char *mender_troubleshoot_decode_body(msgpack_object *object);

/**
 * @brief Encode and pack Proto message
 * @param protomsg Proto message
 * @param data Packed data encoded
 * @param length Length of the data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_pack_protomsg(mender_troubleshoot_protomsg_t *protomsg, void **data, size_t *length);

/**
 * @brief Encode Proto message
 * @param protomsg Proto message
 * @param p Object key-value
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_encode_protomsg(mender_troubleshoot_protomsg_t *protomsg, msgpack_object *object);

/**
 * @brief Encode Proto header
 * @param protohdr Proto header
 * @param p Object key-value
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_encode_protohdr(mender_troubleshoot_protohdr_t *protohdr, msgpack_object_kv *p);

/**
 * @brief Encode Proto header properties
 * @param properties Proto header properties
 * @param p Object key-value
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_encode_protohdr_properties(mender_troubleshoot_protohdr_properties_t *properties, msgpack_object_kv *p);

/**
 * @brief Encode body
 * @param body Body
 * @param p Object key-value
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_encode_body(char *body, msgpack_object_kv *p);

/**
 * @brief Release msgpack object
 * @param object msgpack object
 */
static void mender_troubleshoot_msgpack_object_release(msgpack_object *object);

/**
 * @brief Release Proto message
 * @param protomsg Proto message
 */
static void mender_troubleshoot_release_protomsg(mender_troubleshoot_protomsg_t *protomsg);

/**
 * @brief Release Proto header
 * @param protohdr Proto header
 */
static void mender_troubleshoot_release_protohdr(mender_troubleshoot_protohdr_t *protohdr);

/**
 * @brief Release Proto header properties
 * @param properties Proto header properties
 */
static void mender_troubleshoot_release_protohdr_properties(mender_troubleshoot_protohdr_properties_t *properties);

mender_err_t
mender_troubleshoot_init(mender_troubleshoot_config_t *config, mender_troubleshoot_callbacks_t *callbacks) {

    assert(NULL != config);
    mender_err_t ret;

    /* Save configuration */
    if (0 != config->healthcheck_interval) {
        mender_troubleshoot_config.healthcheck_interval = config->healthcheck_interval;
    } else {
        mender_troubleshoot_config.healthcheck_interval = CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL;
    }

    /* Save callbacks */
    memcpy(&mender_troubleshoot_callbacks, callbacks, sizeof(mender_troubleshoot_callbacks_t));

#ifndef CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS
    /* Create troubleshoot healthcheck work */
    mender_rtos_work_params_t healthcheck_work_params;
    healthcheck_work_params.function = mender_troubleshoot_healthcheck_routine;
    healthcheck_work_params.period   = mender_troubleshoot_config.healthcheck_interval;
    healthcheck_work_params.name     = "mender_troubleshoot_healthcheck";
    if (MENDER_OK != (ret = mender_rtos_work_create(&healthcheck_work_params, &mender_troubleshoot_healthcheck_work_handle))) {
        mender_log_error("Unable to create healthcheck work");
        return ret;
    }
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS */

    return ret;
}

#ifndef CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS
mender_err_t
mender_troubleshoot_activate(void) {

    mender_err_t ret;

    /* Activate troubleshoot healthcheck work */
    if (MENDER_OK != (ret = mender_rtos_work_activate(mender_troubleshoot_healthcheck_work_handle))) {
        mender_log_error("Unable to activate troubleshoot healthcheck work");
        return ret;
    }

    return ret;
}
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS */

mender_err_t
mender_troubleshoot_deactivate(void) {

    mender_err_t ret = MENDER_OK;

#ifndef CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS
    /* Deactivate troubleshoot healthcheck work */
    mender_rtos_work_deactivate(mender_troubleshoot_healthcheck_work_handle);
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS */

    /* Check if a session is already opened */
    if (NULL != mender_troubleshoot_shell_sid) {

        /* Invoke shell end callback */
        if (NULL != mender_troubleshoot_callbacks.shell_end) {
            if (MENDER_OK != (ret = mender_troubleshoot_callbacks.shell_end())) {
                mender_log_error("An error occured");
            }
        }
    }

    /* Check if connection is established */
    if (NULL != mender_troubleshoot_handle) {

        /* Check if a session is already opened */
        if (NULL != mender_troubleshoot_shell_sid) {

            /* Send stop message to the server */
            if (MENDER_OK != (ret = mender_troubleshoot_send_shell_stop_protomsg())) {
                mender_log_error("Unable to send stop message to the server");
            }
        }

        /* Disconnect the device of the server */
        if (MENDER_OK != (ret = mender_api_troubleshoot_disconnect(mender_troubleshoot_handle))) {
            mender_log_error("Unable to disconnect the device of the server");
        }
        mender_troubleshoot_handle = NULL;
    }

    /* Release session ID */
    if (NULL != mender_troubleshoot_shell_sid) {
        free(mender_troubleshoot_shell_sid);
        mender_troubleshoot_shell_sid = NULL;
    }

    return ret;
}

mender_err_t
mender_troubleshoot_shell_print(uint8_t *data, size_t length) {

    assert(NULL != data);
    mender_troubleshoot_protomsg_t *protomsg = NULL;
    mender_err_t                    ret      = MENDER_OK;
    void *                          payload  = NULL;

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
    if (NULL == (protomsg->protohdr = (mender_troubleshoot_protohdr_t *)malloc(sizeof(mender_troubleshoot_protohdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->protohdr, 0, sizeof(mender_troubleshoot_protohdr_t));
    protomsg->protohdr->proto = MENDER_TROUBLESHOOT_PROTO_TYPE_SHELL;
    if (NULL == (protomsg->protohdr->typ = strdup(MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_SHELL))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->protohdr->sid = strdup(mender_troubleshoot_shell_sid))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->protohdr->properties = (mender_troubleshoot_protohdr_properties_t *)malloc(sizeof(mender_troubleshoot_protohdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->protohdr->properties, 0, sizeof(mender_troubleshoot_protohdr_properties_t));
    if (NULL == (protomsg->protohdr->properties->status = (mender_troubleshoot_properties_status_t *)malloc(sizeof(mender_troubleshoot_properties_status_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    *(protomsg->protohdr->properties->status) = MENDER_TROUBLESHOOT_STATUS_TYPE_NORMAL;
    if (NULL == (protomsg->body = strndup((char *)data, length))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Encode and pack the message */
    if (MENDER_OK != (ret = mender_troubleshoot_pack_protomsg(protomsg, &payload, &length))) {
        mender_log_error("Unable to encode message");
        goto FAIL;
    }

    /* Send message */
    if (MENDER_OK != (ret = mender_api_troubleshoot_send(mender_troubleshoot_handle, payload, length))) {
        mender_log_error("Unable to send message");
        goto FAIL;
    }

FAIL:

    /* Release memory */
    mender_troubleshoot_release_protomsg(protomsg);
    if (NULL != payload) {
        free(payload);
    }

    return ret;
}

mender_err_t
mender_troubleshoot_exit(void) {

#ifndef CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS
    /* Delete troubleshoot healthcheck work */
    mender_rtos_work_delete(mender_troubleshoot_healthcheck_work_handle);
    mender_troubleshoot_healthcheck_work_handle = NULL;
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_DISABLE_RTOS */

    /* Release memory */
    if (NULL != mender_troubleshoot_shell_sid) {
        free(mender_troubleshoot_shell_sid);
        mender_troubleshoot_shell_sid = NULL;
    }
    mender_troubleshoot_config.healthcheck_interval = 0;

    return MENDER_OK;
}

mender_err_t
mender_troubleshoot_healthcheck_routine(void) {

    mender_err_t ret = MENDER_OK;

    /* Check if connection is established */
    if (NULL != mender_troubleshoot_handle) {

        /* Check if a session is already opened */
        if (NULL != mender_troubleshoot_shell_sid) {

            /* Send healthcheck ping message over websocket connection */
            if (MENDER_OK != (ret = mender_troubleshoot_send_shell_ping_protomsg())) {
                mender_log_error("Unable to send healthcheck message to the server");
                goto FAIL;
            }
        }

    } else {

        /* Connect the device to the server */
        if (MENDER_OK != (ret = mender_api_troubleshoot_connect(&mender_troubleshoot_data_received_callback, &mender_troubleshoot_handle))) {
            mender_log_error("Unable to connect the device to the server");
            goto END;
        }
    }

    goto END;

FAIL:

    /* Check if a session is already opened */
    if (NULL != mender_troubleshoot_shell_sid) {

        /* Invoke shell end callback */
        if (NULL != mender_troubleshoot_callbacks.shell_end) {
            if (MENDER_OK != (ret = mender_troubleshoot_callbacks.shell_end())) {
                mender_log_error("An error occured");
            }
        }
    }

    /* Check if connection is established */
    if (NULL != mender_troubleshoot_handle) {

        /* Disconnect the device of the server */
        if (MENDER_OK != (ret = mender_api_troubleshoot_disconnect(mender_troubleshoot_handle))) {
            mender_log_error("Unable to disconnect the device of the server");
        }
        mender_troubleshoot_handle = NULL;
    }

    /* Release session ID */
    if (NULL != mender_troubleshoot_shell_sid) {
        free(mender_troubleshoot_shell_sid);
        mender_troubleshoot_shell_sid = NULL;
    }

END:

    return ret;
}

static mender_err_t
mender_troubleshoot_data_received_callback(void *data, size_t length) {

    assert(NULL != data);
    mender_err_t                    ret = MENDER_OK;
    mender_troubleshoot_protomsg_t *protomsg;
    mender_troubleshoot_protomsg_t *response = NULL;
    void *                          payload  = NULL;

    /* Unpack and decode message */
    if (NULL == (protomsg = mender_troubleshoot_unpack_protomsg(data, length))) {
        mender_log_error("Unable to decode message");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Verify integrity of the message */
    if (NULL == protomsg->protohdr) {
        mender_log_error("Invalid message received");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Treatment of the message depending of the proto type */
    switch (protomsg->protohdr->proto) {
        case MENDER_TROUBLESHOOT_PROTO_TYPE_INVALID:
            mender_log_error("Invalid message received");
            ret = MENDER_FAIL;
            break;
        case MENDER_TROUBLESHOOT_PROTO_TYPE_SHELL:
            ret = mender_troubleshoot_shell_message_handler(protomsg, &response);
            break;
        case MENDER_TROUBLESHOOT_PROTO_TYPE_FILE_TRANSFER:
        case MENDER_TROUBLESHOOT_PROTO_TYPE_PORT_FORWARD:
        case MENDER_TROUBLESHOOT_PROTO_TYPE_MENDER_CLIENT:
        case MENDER_TROUBLESHOOT_PROTO_TYPE_CONTROL:
        default:
            mender_log_error("Unsupported message received with proto type 0x%04x", protomsg->protohdr->proto);
            ret = MENDER_FAIL;
            break;
    }

    /* Check if response is available */
    if (NULL != response) {

        /* Encode and pack the message */
        if (MENDER_OK != (ret = mender_troubleshoot_pack_protomsg(response, &payload, &length))) {
            mender_log_error("Unable to encode response");
            goto END;
        }

        /* Send response */
        if (MENDER_OK != (ret = mender_api_troubleshoot_send(mender_troubleshoot_handle, payload, length))) {
            mender_log_error("Unable to send response");
            goto END;
        }
    }

END:

    /* Release memory */
    mender_troubleshoot_release_protomsg(protomsg);
    mender_troubleshoot_release_protomsg(response);
    if (NULL != payload) {
        free(payload);
    }

    return ret;
}

static mender_err_t
mender_troubleshoot_shell_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->protohdr);
    mender_err_t ret = MENDER_OK;

    /* Verify integrity of the message */
    if ((NULL == protomsg->protohdr->typ) || (NULL == protomsg->protohdr->sid)) {
        mender_log_error("Invalid message received");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Treatment of the message depending of the message type */
    if (!strcmp(protomsg->protohdr->typ, MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_PING)) {

        /* Nothing to do */

    } else if (!strcmp(protomsg->protohdr->typ, MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_PONG)) {

        /* Nothing to do */

    } else if (!strcmp(protomsg->protohdr->typ, MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_RESIZE)) {

        /* Verify integrity of the message */
        if ((NULL == protomsg->protohdr->properties) || (NULL == protomsg->protohdr->properties->terminal_width)
            || (NULL == protomsg->protohdr->properties->terminal_height)) {
            mender_log_error("Invalid message received");
            ret = MENDER_FAIL;
            goto END;
        }

        /* Invoke shell resize callback */
        if (NULL != mender_troubleshoot_callbacks.shell_resize) {
            if (MENDER_OK
                != (ret = mender_troubleshoot_callbacks.shell_resize(*protomsg->protohdr->properties->terminal_width,
                                                                     *protomsg->protohdr->properties->terminal_height))) {
                mender_log_error("An error occured");
                goto FAIL;
            }
        }

    } else if (!strcmp(protomsg->protohdr->typ, MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_SHELL)) {

        /* Verify integrity of the message */
        if (NULL == protomsg->body) {
            mender_log_error("Invalid message received");
            ret = MENDER_FAIL;
            goto END;
        }

        /* Invoke shell data write callback */
        if (NULL != mender_troubleshoot_callbacks.shell_write) {
            if (MENDER_OK != (ret = mender_troubleshoot_callbacks.shell_write((uint8_t *)protomsg->body, strlen(protomsg->body)))) {
                mender_log_error("An error occured");
                goto FAIL;
            }
        }

    } else if (!strcmp(protomsg->protohdr->typ, MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_SPAWN)) {

        /* Check is a session is already opened */
        if (NULL != mender_troubleshoot_shell_sid) {
            mender_log_warning("A shell session is already opened");
            goto END;
        }

        /* Start shell session */
        mender_log_info("Starting a new shell session");

        /* Save the session ID */
        if (NULL == (mender_troubleshoot_shell_sid = strdup(protomsg->protohdr->sid))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }

        /* Acknowledge the message */
        if (NULL == (*response = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset(*response, 0, sizeof(mender_troubleshoot_protomsg_t));
        if (NULL == ((*response)->protohdr = (mender_troubleshoot_protohdr_t *)malloc(sizeof(mender_troubleshoot_protohdr_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset((*response)->protohdr, 0, sizeof(mender_troubleshoot_protohdr_t));
        (*response)->protohdr->proto = MENDER_TROUBLESHOOT_PROTO_TYPE_SHELL;
        if (NULL == ((*response)->protohdr->typ = strdup(MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_SPAWN))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        if (NULL == ((*response)->protohdr->sid = strdup(mender_troubleshoot_shell_sid))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        if (NULL
            == ((*response)->protohdr->properties = (mender_troubleshoot_protohdr_properties_t *)malloc(sizeof(mender_troubleshoot_protohdr_properties_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset((*response)->protohdr->properties, 0, sizeof(mender_troubleshoot_protohdr_properties_t));
        if (NULL
            == ((*response)->protohdr->properties->status
                = (mender_troubleshoot_properties_status_t *)malloc(sizeof(mender_troubleshoot_properties_status_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        *((*response)->protohdr->properties->status) = MENDER_TROUBLESHOOT_STATUS_TYPE_NORMAL;

        /* Invoke shell begin callback */
        if (NULL != mender_troubleshoot_callbacks.shell_begin) {
            if ((NULL != protomsg->protohdr->properties) && (NULL != protomsg->protohdr->properties->terminal_width)
                && (NULL != protomsg->protohdr->properties->terminal_height)) {
                ret = mender_troubleshoot_callbacks.shell_begin(*protomsg->protohdr->properties->terminal_width,
                                                                *protomsg->protohdr->properties->terminal_height);
            } else {
                ret = mender_troubleshoot_callbacks.shell_begin(0, 0);
            }
            if (MENDER_OK != ret) {
                mender_log_error("An error occured");
                goto FAIL;
            }
        }

    } else if (!strcmp(protomsg->protohdr->typ, MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_STOP)) {

        /* Check is a session is already opened */
        if (NULL == mender_troubleshoot_shell_sid) {
            mender_log_warning("No shell session opened");
            goto END;
        }

        /* Stop shell session */
        mender_log_info("Stopping current shell session");

        /* Invoke shell end callback */
        if (NULL != mender_troubleshoot_callbacks.shell_end) {
            if (MENDER_OK != (ret = mender_troubleshoot_callbacks.shell_end())) {
                mender_log_error("An error occured");
            }
        }

        /* Acknowledge the message */
        if (NULL == (*response = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset(*response, 0, sizeof(mender_troubleshoot_protomsg_t));
        if (NULL == ((*response)->protohdr = (mender_troubleshoot_protohdr_t *)malloc(sizeof(mender_troubleshoot_protohdr_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset((*response)->protohdr, 0, sizeof(mender_troubleshoot_protohdr_t));
        (*response)->protohdr->proto = MENDER_TROUBLESHOOT_PROTO_TYPE_SHELL;
        if (NULL == ((*response)->protohdr->typ = strdup(MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_STOP))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        if (NULL == ((*response)->protohdr->sid = strdup(mender_troubleshoot_shell_sid))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        if (NULL
            == ((*response)->protohdr->properties = (mender_troubleshoot_protohdr_properties_t *)malloc(sizeof(mender_troubleshoot_protohdr_properties_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memset((*response)->protohdr->properties, 0, sizeof(mender_troubleshoot_protohdr_properties_t));
        if (NULL
            == ((*response)->protohdr->properties->status
                = (mender_troubleshoot_properties_status_t *)malloc(sizeof(mender_troubleshoot_properties_status_t)))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        *((*response)->protohdr->properties->status) = MENDER_TROUBLESHOOT_STATUS_TYPE_NORMAL;

        /* Release session ID */
        if (NULL != mender_troubleshoot_shell_sid) {
            free(mender_troubleshoot_shell_sid);
            mender_troubleshoot_shell_sid = NULL;
        }
    } else {

        mender_log_error("Unsupported message received with message type '%s'", protomsg->protohdr->typ);
        ret = MENDER_FAIL;
        goto FAIL;
    }

END:

    return ret;

FAIL:

    /* Release memory */
    mender_troubleshoot_release_protomsg(*response);
    *response = NULL;

    return ret;
}

static mender_err_t
mender_troubleshoot_send_shell_ping_protomsg(void) {

    mender_troubleshoot_protomsg_t *protomsg = NULL;
    mender_err_t                    ret      = MENDER_OK;
    void *                          payload  = NULL;
    size_t                          length   = 0;

    /* Send shell ping message */
    if (NULL == (protomsg = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == (protomsg->protohdr = (mender_troubleshoot_protohdr_t *)malloc(sizeof(mender_troubleshoot_protohdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->protohdr, 0, sizeof(mender_troubleshoot_protohdr_t));
    protomsg->protohdr->proto = MENDER_TROUBLESHOOT_PROTO_TYPE_SHELL;
    if (NULL == (protomsg->protohdr->typ = strdup(MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_PING))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->protohdr->sid = strdup(mender_troubleshoot_shell_sid))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->protohdr->properties = (mender_troubleshoot_protohdr_properties_t *)malloc(sizeof(mender_troubleshoot_protohdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->protohdr->properties, 0, sizeof(mender_troubleshoot_protohdr_properties_t));
    if (NULL == (protomsg->protohdr->properties->timeout = (uint32_t *)malloc(sizeof(uint32_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    *protomsg->protohdr->properties->timeout = 2 * CONFIG_MENDER_CLIENT_TROUBLESHOOT_HEALTHCHECK_INTERVAL;
    if (NULL == (protomsg->protohdr->properties->status = (mender_troubleshoot_properties_status_t *)malloc(sizeof(mender_troubleshoot_properties_status_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    *protomsg->protohdr->properties->status = MENDER_TROUBLESHOOT_STATUS_TYPE_CONTROL;

    /* Encode and pack the message */
    if (MENDER_OK != (ret = mender_troubleshoot_pack_protomsg(protomsg, &payload, &length))) {
        mender_log_error("Unable to encode message");
        goto FAIL;
    }

    /* Send message */
    if (MENDER_OK != (ret = mender_api_troubleshoot_send(mender_troubleshoot_handle, payload, length))) {
        mender_log_error("Unable to send message");
        goto FAIL;
    }

FAIL:

    /* Release memory */
    mender_troubleshoot_release_protomsg(protomsg);
    if (NULL != payload) {
        free(payload);
    }

    return ret;
}

static mender_err_t
mender_troubleshoot_send_shell_stop_protomsg(void) {

    mender_troubleshoot_protomsg_t *protomsg = NULL;
    mender_err_t                    ret      = MENDER_OK;
    void *                          payload  = NULL;
    size_t                          length   = 0;

    /* Send shell stop message */
    if (NULL == (protomsg = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg, 0, sizeof(mender_troubleshoot_protomsg_t));
    if (NULL == (protomsg->protohdr = (mender_troubleshoot_protohdr_t *)malloc(sizeof(mender_troubleshoot_protohdr_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->protohdr, 0, sizeof(mender_troubleshoot_protohdr_t));
    protomsg->protohdr->proto = MENDER_TROUBLESHOOT_PROTO_TYPE_SHELL;
    if (NULL == (protomsg->protohdr->typ = strdup(MENDER_TROUBLESHOOT_MESSAGE_TYPE_SHELL_STOP))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->protohdr->sid = strdup(mender_troubleshoot_shell_sid))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL == (protomsg->protohdr->properties = (mender_troubleshoot_protohdr_properties_t *)malloc(sizeof(mender_troubleshoot_protohdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(protomsg->protohdr->properties, 0, sizeof(mender_troubleshoot_protohdr_properties_t));
    if (NULL == (protomsg->protohdr->properties->status = (mender_troubleshoot_properties_status_t *)malloc(sizeof(mender_troubleshoot_properties_status_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    *protomsg->protohdr->properties->status = MENDER_TROUBLESHOOT_STATUS_TYPE_ERROR;

    /* Encode and pack the message */
    if (MENDER_OK != (ret = mender_troubleshoot_pack_protomsg(protomsg, &payload, &length))) {
        mender_log_error("Unable to encode message");
        goto FAIL;
    }

    /* Send message */
    if (MENDER_OK != (ret = mender_api_troubleshoot_send(mender_troubleshoot_handle, payload, length))) {
        mender_log_error("Unable to send message");
        goto FAIL;
    }

FAIL:

    /* Release memory */
    mender_troubleshoot_release_protomsg(protomsg);
    if (NULL != payload) {
        free(payload);
    }

    return ret;
}

static mender_troubleshoot_protomsg_t *
mender_troubleshoot_unpack_protomsg(void *data, size_t length) {

    assert(NULL != data);
    mender_troubleshoot_protomsg_t *protomsg;
    msgpack_zone                    zone;
    msgpack_object                  object;

    /* Initialize msgpack zone */
    if (true != msgpack_zone_init(&zone, MENDER_TROUBLESHOOT_ZONE_CHUNK_INIT_SIZE)) {
        mender_log_error("Unable to initialize msgpack zone");
        return NULL;
    }

    /* Unpack the message */
    if (MSGPACK_UNPACK_SUCCESS != msgpack_unpack((const char *)data, length, NULL, &zone, &object)) {
        mender_log_error("Unable to unpack the message");
        goto FAIL;
    }

    /* Check object type */
    if ((MSGPACK_OBJECT_MAP != object.type) || (0 == object.via.map.size)) {
        mender_log_error("Invalid protomsg object");
        goto FAIL;
    }

    /* Decode protomsg */
    if (NULL == (protomsg = mender_troubleshoot_decode_protomsg(&object))) {
        mender_log_error("Invalid protomsg object");
        goto FAIL;
    }

    /* Release memory */
    msgpack_zone_destroy(&zone);

    return protomsg;

FAIL:

    /* Release memory */
    msgpack_zone_destroy(&zone);

    return NULL;
}

static mender_troubleshoot_protomsg_t *
mender_troubleshoot_decode_protomsg(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_protomsg_t *protomsg;

    /* Create protomsg */
    if (NULL == (protomsg = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        return NULL;
    }
    memset(protomsg, 0, sizeof(mender_troubleshoot_protomsg_t));

    /* Parse protomsg */
    msgpack_object_kv *p = object->via.map.ptr;
    do {
        if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "hdr", p->key.via.str.size)) && (MSGPACK_OBJECT_MAP == p->val.type)
            && (0 != p->val.via.map.size)) {
            if (NULL == (protomsg->protohdr = mender_troubleshoot_decode_protohdr(&p->val))) {
                mender_log_error("Invalid protomsg object");
                goto FAIL;
            }
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "body", p->key.via.str.size)) && (MSGPACK_OBJECT_BIN == p->val.type)
                   && (0 != p->val.via.bin.size)) {
            if (NULL == (protomsg->body = mender_troubleshoot_decode_body(&p->val))) {
                mender_log_error("Invalid protomsg object");
                goto FAIL;
            }
        }
        ++p;
    } while (p < object->via.map.ptr + object->via.map.size);

    return protomsg;

FAIL:

    /* Release memory */
    mender_troubleshoot_release_protomsg(protomsg);

    return NULL;
}

static mender_troubleshoot_protohdr_t *
mender_troubleshoot_decode_protohdr(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_protohdr_t *protohdr = NULL;

    /* Create protohdr */
    if (NULL == (protohdr = (mender_troubleshoot_protohdr_t *)malloc(sizeof(mender_troubleshoot_protohdr_t)))) {
        mender_log_error("Unable to allocate memory");
        return NULL;
    }
    memset(protohdr, 0, sizeof(mender_troubleshoot_protohdr_t));

    /* Parse protohdr */
    msgpack_object_kv *p = object->via.map.ptr;
    do {
        if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "proto", p->key.via.str.size))
            && (MSGPACK_OBJECT_POSITIVE_INTEGER == p->val.type)) {
            protohdr->proto = (mender_troubleshoot_protohdr_type_t)p->val.via.u64;
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "typ", p->key.via.str.size)) && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (protohdr->typ = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(protohdr->typ, p->val.via.str.ptr, p->val.via.str.size);
            protohdr->typ[p->val.via.str.size] = '\0';
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "sid", p->key.via.str.size)) && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (protohdr->sid = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(protohdr->sid, p->val.via.str.ptr, p->val.via.str.size);
            protohdr->sid[p->val.via.str.size] = '\0';
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "props", p->key.via.str.size)) && (MSGPACK_OBJECT_MAP == p->val.type)
                   && (0 != p->val.via.map.size)) {
            if (NULL == (protohdr->properties = mender_troubleshoot_decode_protohdr_properties(&p->val))) {
                mender_log_error("Invalid protohdr properties object");
                goto FAIL;
            }
        }
        ++p;
    } while (p < object->via.map.ptr + object->via.map.size);

    return protohdr;

FAIL:

    /* Release memory */
    mender_troubleshoot_release_protohdr(protohdr);

    return NULL;
}

static mender_troubleshoot_protohdr_properties_t *
mender_troubleshoot_decode_protohdr_properties(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_protohdr_properties_t *properties = NULL;

    /* Create protohdr properties */
    if (NULL == (properties = (mender_troubleshoot_protohdr_properties_t *)malloc(sizeof(mender_troubleshoot_protohdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        return NULL;
    }
    memset(properties, 0, sizeof(mender_troubleshoot_protohdr_properties_t));

    /* Parse protohdr properties */
    msgpack_object_kv *p = object->via.map.ptr;
    do {
        if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "terminal_width", p->key.via.str.size))
            && (MSGPACK_OBJECT_POSITIVE_INTEGER == p->val.type)) {
            if (NULL == (properties->terminal_width = (uint16_t *)malloc(sizeof(uint16_t)))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            *properties->terminal_width = (uint16_t)p->val.via.u64;
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "terminal_height", p->key.via.str.size))
                   && (MSGPACK_OBJECT_POSITIVE_INTEGER == p->val.type)) {
            if (NULL == (properties->terminal_height = (uint16_t *)malloc(sizeof(uint16_t)))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            *properties->terminal_height = (uint16_t)p->val.via.u64;
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "user_id", p->key.via.str.size))
                   && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (properties->user_id = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(properties->user_id, p->val.via.str.ptr, p->val.via.str.size);
            properties->user_id[p->val.via.str.size] = '\0';
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "timeout", p->key.via.str.size))
                   && (MSGPACK_OBJECT_POSITIVE_INTEGER == p->val.type)) {
            if (NULL == (properties->timeout = (uint32_t *)malloc(sizeof(uint32_t)))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            *properties->timeout = (uint32_t)p->val.via.u64;
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "status", p->key.via.str.size))
                   && (MSGPACK_OBJECT_POSITIVE_INTEGER == p->val.type)) {
            if (NULL == (properties->status = (mender_troubleshoot_properties_status_t *)malloc(sizeof(mender_troubleshoot_properties_status_t)))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            *properties->status = (mender_troubleshoot_properties_status_t)p->val.via.u64;
        }
        ++p;
    } while (p < object->via.map.ptr + object->via.map.size);

    return properties;

FAIL:

    /* Release memory */
    mender_troubleshoot_release_protohdr_properties(properties);

    return NULL;
}

static char *
mender_troubleshoot_decode_body(msgpack_object *object) {

    assert(NULL != object);
    char *body;

    /* Create body */
    if (NULL == (body = (char *)malloc(object->via.bin.size + 1))) {
        mender_log_error("Unable to allocate memory");
        goto FAIL;
    }
    memcpy(body, object->via.bin.ptr, object->via.bin.size);
    body[object->via.bin.size] = '\0';

    return body;

FAIL:

    return NULL;
}

static mender_err_t
mender_troubleshoot_pack_protomsg(mender_troubleshoot_protomsg_t *protomsg, void **data, size_t *length) {

    assert(NULL != protomsg);
    assert(NULL != data);
    assert(NULL != length);
    mender_err_t    ret = MENDER_OK;
    msgpack_sbuffer sbuffer;
    msgpack_packer  packer;
    msgpack_object  object;

    /* Initialize msgpack sbuffer */
    msgpack_sbuffer_init(&sbuffer);
    sbuffer.alloc = MENDER_TROUBLESHOOT_SBUFFER_INIT_SIZE;
    if (NULL == (sbuffer.data = (char *)malloc(sbuffer.alloc))) {
        mender_log_error("Unable  to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Initialize msgpack packer */
    msgpack_packer_init(&packer, &sbuffer, msgpack_sbuffer_write);

    /* Encode protomsg */
    if (MENDER_OK != (ret = mender_troubleshoot_encode_protomsg(protomsg, &object))) {
        mender_log_error("Invalid protomsg object");
        goto FAIL;
    }

    /* Pack the message */
    if (0 != msgpack_pack_object(&packer, object)) {
        mender_log_error("Unable to pack the message");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Return sbuffer data and size */
    *data   = sbuffer.data;
    *length = sbuffer.size;

    /* Release memory */
    mender_troubleshoot_msgpack_object_release(&object);

    return ret;

FAIL:

    /* Release memory */
    msgpack_sbuffer_destroy(&sbuffer);

    return ret;
}

static mender_err_t
mender_troubleshoot_encode_protomsg(mender_troubleshoot_protomsg_t *protomsg, msgpack_object *object) {

    assert(NULL != protomsg);
    assert(NULL != object);
    mender_err_t       ret = MENDER_OK;
    msgpack_object_kv *p;

    /* Create protomsg */
    object->type = MSGPACK_OBJECT_MAP;
    if (0 == (object->via.map.size = ((NULL != protomsg->protohdr) ? 1 : 0) + ((NULL != protomsg->body) ? 1 : 0))) {
        goto END;
    }
    if (NULL == (object->via.map.ptr = (msgpack_object_kv *)malloc(object->via.map.size * sizeof(struct msgpack_object_kv)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Parse protomsg */
    p = object->via.map.ptr;
    if (NULL != protomsg->protohdr) {
        if (MENDER_OK != (ret = mender_troubleshoot_encode_protohdr(protomsg->protohdr, p))) {
            mender_log_error("Unable to encode protohdr");
            goto END;
        }
        ++p;
    }
    if (NULL != protomsg->body) {
        if (MENDER_OK != (ret = mender_troubleshoot_encode_body(protomsg->body, p))) {
            mender_log_error("Unable to encode body");
            goto END;
        }
    }

END:

    return ret;
}

static mender_err_t
mender_troubleshoot_encode_protohdr(mender_troubleshoot_protohdr_t *protohdr, msgpack_object_kv *p) {

    assert(NULL != protohdr);
    assert(NULL != p);
    mender_err_t ret = MENDER_OK;

    /* Create protohdr */
    p->key.type = MSGPACK_OBJECT_STR;
    if (NULL == (p->key.via.str.ptr = strdup("hdr"))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    p->key.via.str.size = strlen("hdr");
    p->val.type         = MSGPACK_OBJECT_MAP;
    if (0 == (p->val.via.map.size = 1 + ((NULL != protohdr->typ) ? 1 : 0) + ((NULL != protohdr->sid) ? 1 : 0) + ((NULL != protohdr->properties) ? 1 : 0))) {
        goto END;
    }
    if (NULL == (p->val.via.map.ptr = (msgpack_object_kv *)malloc(p->val.via.map.size * sizeof(struct msgpack_object_kv)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Parse protohdr */
    p           = p->val.via.map.ptr;
    p->key.type = MSGPACK_OBJECT_STR;
    if (NULL == (p->key.via.str.ptr = strdup("proto"))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    p->key.via.str.size = strlen("proto");
    p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
    p->val.via.u64      = protohdr->proto;
    ++p;
    if (NULL != protohdr->typ) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("typ"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        p->key.via.str.size = strlen("typ");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = strlen(protohdr->typ);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        memcpy((void *)p->val.via.str.ptr, protohdr->typ, p->val.via.str.size);
        ++p;
    }
    if (NULL != protohdr->sid) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("sid"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        p->key.via.str.size = strlen("sid");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = strlen(protohdr->sid);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        memcpy((void *)p->val.via.str.ptr, protohdr->sid, p->val.via.str.size);
        ++p;
    }
    if (NULL != protohdr->properties) {
        if (MENDER_OK != (ret = mender_troubleshoot_encode_protohdr_properties(protohdr->properties, p))) {
            mender_log_error("Unable to encode protohdr properties");
            goto END;
        }
    }

END:

    return ret;
}

static mender_err_t
mender_troubleshoot_encode_protohdr_properties(mender_troubleshoot_protohdr_properties_t *properties, msgpack_object_kv *p) {

    assert(NULL != properties);
    assert(NULL != p);
    mender_err_t ret = MENDER_OK;

    /* Create properties */
    p->key.type = MSGPACK_OBJECT_STR;
    if (NULL == (p->key.via.str.ptr = strdup("props"))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    p->key.via.str.size = strlen("props");
    p->val.type         = MSGPACK_OBJECT_MAP;
    if (0
        == (p->val.via.map.size = ((NULL != properties->terminal_width) ? 1 : 0) + ((NULL != properties->terminal_height) ? 1 : 0)
                                  + ((NULL != properties->user_id) ? 1 : 0) + ((NULL != properties->timeout) ? 1 : 0)
                                  + ((NULL != properties->status) ? 1 : 0))) {
        goto END;
    }
    if (NULL == (p->val.via.map.ptr = (msgpack_object_kv *)malloc(p->val.via.map.size * sizeof(struct msgpack_object_kv)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Parse properties */
    p = p->val.via.map.ptr;
    if (NULL != properties->terminal_width) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("terminal_width"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        p->key.via.str.size = strlen("terminal_width");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *properties->terminal_width;
        ++p;
    }
    if (NULL != properties->terminal_height) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("terminal_height"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        p->key.via.str.size = strlen("terminal_height");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *properties->terminal_height;
        ++p;
    }
    if (NULL != properties->user_id) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("user_id"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        p->key.via.str.size = strlen("user_id");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = strlen(properties->user_id);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        memcpy((void *)p->val.via.str.ptr, properties->user_id, p->val.via.str.size);
        ++p;
    }
    if (NULL != properties->timeout) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("timeout"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        p->key.via.str.size = strlen("timeout");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *properties->timeout;
        ++p;
    }
    if (NULL != properties->status) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("status"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        p->key.via.str.size = strlen("status");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *properties->status;
    }

END:

    return ret;
}

static mender_err_t
mender_troubleshoot_encode_body(char *body, msgpack_object_kv *p) {

    assert(NULL != body);
    assert(NULL != p);
    mender_err_t ret = MENDER_OK;

    /* Create body */
    p->key.type = MSGPACK_OBJECT_STR;
    if (NULL == (p->key.via.str.ptr = strdup("body"))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    p->key.via.str.size = strlen("body");
    p->val.type         = MSGPACK_OBJECT_BIN;
    p->val.via.bin.size = strlen(body);
    if (NULL == (p->val.via.bin.ptr = (char *)malloc(p->val.via.bin.size * sizeof(uint8_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    memcpy((void *)p->val.via.bin.ptr, body, p->val.via.bin.size);

END:

    return ret;
}

static void
mender_troubleshoot_msgpack_object_release(msgpack_object *object) {

    /* Release memory */
    if (NULL != object) {
        switch (object->type) {
            case MSGPACK_OBJECT_NIL:
            case MSGPACK_OBJECT_BOOLEAN:
            case MSGPACK_OBJECT_POSITIVE_INTEGER:
            case MSGPACK_OBJECT_NEGATIVE_INTEGER:
            case MSGPACK_OBJECT_FLOAT32:
            case MSGPACK_OBJECT_FLOAT64:
                /* Nothing to do */
                break;
            case MSGPACK_OBJECT_STR:
                if (NULL != object->via.str.ptr) {
                    free((void *)object->via.str.ptr);
                }
                break;
            case MSGPACK_OBJECT_BIN:
                if (NULL != object->via.ext.ptr) {
                    free((void *)object->via.bin.ptr);
                }
                break;
            case MSGPACK_OBJECT_EXT:
                if (NULL != object->via.ext.ptr) {
                    free((void *)object->via.ext.ptr);
                }
                break;
            case MSGPACK_OBJECT_ARRAY:
                if (NULL != object->via.array.ptr) {
                    msgpack_object *p = object->via.array.ptr;
                    do {
                        mender_troubleshoot_msgpack_object_release(p);
                        ++p;
                    } while (p < object->via.array.ptr + object->via.array.size);
                    free(object->via.array.ptr);
                }
                break;
            case MSGPACK_OBJECT_MAP:
                if (NULL != object->via.map.ptr) {
                    msgpack_object_kv *p = object->via.map.ptr;
                    do {
                        mender_troubleshoot_msgpack_object_release(&(p->key));
                        mender_troubleshoot_msgpack_object_release(&(p->val));
                        ++p;
                    } while (p < object->via.map.ptr + object->via.map.size);
                    free(object->via.map.ptr);
                }
                break;
            default:
                /* Should not occur */
                break;
        }
    }
}

static void
mender_troubleshoot_release_protomsg(mender_troubleshoot_protomsg_t *protomsg) {

    /* Release memory */
    if (NULL != protomsg) {
        mender_troubleshoot_release_protohdr(protomsg->protohdr);
        if (NULL != protomsg->body) {
            free(protomsg->body);
        }
        free(protomsg);
    }
}

static void
mender_troubleshoot_release_protohdr(mender_troubleshoot_protohdr_t *protohdr) {

    /* Release memory */
    if (NULL != protohdr) {
        if (NULL != protohdr->typ) {
            free(protohdr->typ);
        }
        if (NULL != protohdr->sid) {
            free(protohdr->sid);
        }
        mender_troubleshoot_release_protohdr_properties(protohdr->properties);
        free(protohdr);
    }
}

static void
mender_troubleshoot_release_protohdr_properties(mender_troubleshoot_protohdr_properties_t *properties) {

    /* Release memory */
    if (NULL != properties) {
        if (NULL != properties->terminal_width) {
            free(properties->terminal_width);
        }
        if (NULL != properties->terminal_height) {
            free(properties->terminal_height);
        }
        if (NULL != properties->user_id) {
            free(properties->user_id);
        }
        if (NULL != properties->timeout) {
            free(properties->timeout);
        }
        if (NULL != properties->status) {
            free(properties->status);
        }
        free(properties);
    }
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */
