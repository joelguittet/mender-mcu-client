/**
 * @file      mender-troubleshoot-control.c
 * @brief     Mender MCU Troubleshoot add-on control message handler implementation
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
#include "mender-troubleshoot-control.h"
#include "mender-troubleshoot-msgpack.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

/**
 * @brief Version
 */
#define MENDER_TROUBLESHOOT_CONTROL_VERSION (1)

/**
 * @brief Message type
 */
#define MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_PING   "ping"
#define MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_PONG   "pong"
#define MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_OPEN   "open"
#define MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_ACCEPT "accept"
#define MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_CLOSE  "close"
#define MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_ERROR  "error"

/**
 * @brief Function called to perform the treatment of the control ping messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_control_ping_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function called to perform the treatment of the control open messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_control_open_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function used to format control pong message
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_control_format_pong(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Function used to format control accept message
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_troubleshoot_control_format_accept(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Encode and pack accept data
 * @param data Accept data encoded
 * @param length Length of the accept data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_control_accept_pack(void **data, size_t *length);

/**
 * @brief Encode Accept data
 * @param p Object key-value
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_control_encode_accept_data(msgpack_object *object);

mender_err_t
mender_troubleshoot_control_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

mender_err_t
mender_troubleshoot_control_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

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
    if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_PING)) {
        /* Format pong */
        ret = mender_troubleshoot_control_ping_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_PONG)) {
        /* Nothing to do */
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_OPEN)) {
        /* Open session */
        ret = mender_troubleshoot_control_open_message_handler(protomsg, response);
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_ACCEPT)) {
        /* Nothing to do */
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_CLOSE)) {
        /* Nothing to do */
    } else if (!strcmp(protomsg->hdr->typ, MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_ERROR)) {
        /* Nothing to do */
    } else {
        /* Not supported */
        mender_log_error("Unsupported control message received with message type '%s'", protomsg->hdr->typ);
        ret = MENDER_FAIL;
        goto FAIL;
    }

FAIL:

    return ret;
}

mender_err_t
mender_troubleshoot_control_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}

static mender_err_t
mender_troubleshoot_control_ping_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_err_t ret = MENDER_OK;

    /* Format pong */
    if (MENDER_OK != (ret = mender_troubleshoot_control_format_pong(protomsg, response))) {
        mender_log_error("Unable to format pong message");
        goto END;
    }

END:

    return ret;
}

static mender_err_t
mender_troubleshoot_control_open_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    mender_err_t ret = MENDER_OK;

    /* Format accept */
    if (MENDER_OK != (ret = mender_troubleshoot_control_format_accept(protomsg, response))) {
        mender_log_error("Unable to format accept message");
        goto END;
    }

END:

    return ret;
}

static mender_err_t
mender_troubleshoot_control_format_pong(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Format control pong message */
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
    if (NULL == ((*response)->hdr->typ = strdup(MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_PONG))) {
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
mender_troubleshoot_control_format_accept(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response) {

    assert(NULL != protomsg);
    assert(NULL != protomsg->hdr);
    mender_err_t ret = MENDER_OK;

    /* Format control accept message */
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
    if (NULL == ((*response)->hdr->typ = strdup(MENDER_TROUBLESHOOT_CONTROL_MESSAGE_TYPE_ACCEPT))) {
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
    if (NULL == ((*response)->body = (mender_troubleshoot_protomsg_body_t *)malloc(sizeof(mender_troubleshoot_protomsg_body_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset((*response)->body, 0, sizeof(mender_troubleshoot_protomsg_body_t));

    /* Encode and pack accept data message */
    if (MENDER_OK != (ret = mender_troubleshoot_control_accept_pack(&((*response)->body->data), &((*response)->body->length)))) {
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

static mender_err_t
mender_troubleshoot_control_accept_pack(void **data, size_t *length) {

    assert(NULL != data);
    assert(NULL != length);
    mender_err_t   ret = MENDER_OK;
    msgpack_object object;

    /* Encode accept */
    if (MENDER_OK != (ret = mender_troubleshoot_control_encode_accept_data(&object))) {
        mender_log_error("Invalid accept object");
        goto FAIL;
    }

    /* Pack accept */
    if (MENDER_OK != (ret = mender_troubleshoot_msgpack_pack_object(&object, data, length))) {
        mender_log_error("Unable to pack protomsg object");
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
mender_troubleshoot_control_encode_accept_data(msgpack_object *object) {

    assert(NULL != object);
    mender_err_t       ret         = MENDER_OK;
    size_t             proto_index = 0;
    msgpack_object_kv *p;

    /* Create accept */
    object->type         = MSGPACK_OBJECT_MAP;
    object->via.map.size = 2;
    if (NULL == (object->via.map.ptr = (msgpack_object_kv *)malloc(object->via.map.size * sizeof(struct msgpack_object_kv)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Parse accept */
    p           = object->via.map.ptr;
    p->key.type = MSGPACK_OBJECT_STR;
    if (NULL == (p->key.via.str.ptr = strdup("version"))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    p->key.via.str.size = (uint32_t)strlen("version");
    p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
    p->val.via.u64      = MENDER_TROUBLESHOOT_CONTROL_VERSION;
    ++p;
    p->key.type = MSGPACK_OBJECT_STR;
    if (NULL == (p->key.via.str.ptr = strdup("protocols"))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    p->key.via.str.size = (uint32_t)strlen("protocols");
    p->val.type         = MSGPACK_OBJECT_ARRAY;
    p->val.via.array.size =
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER
        1 +
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER */
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING
        1 +
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING */
#if CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL
        1 +
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */
        1;
    if (NULL == (p->val.via.array.ptr = (struct msgpack_object *)malloc(p->val.via.array.size * sizeof(struct msgpack_object)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER
    p->val.via.array.ptr[proto_index].type    = MSGPACK_OBJECT_POSITIVE_INTEGER;
    p->val.via.array.ptr[proto_index].via.u64 = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_FILE_TRANSFER;
    proto_index++;
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER */
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING
    p->val.via.array.ptr[proto_index].type    = MSGPACK_OBJECT_POSITIVE_INTEGER;
    p->val.via.array.ptr[proto_index].via.u64 = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_PORT_FORWARD;
    proto_index++;
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING */
#if CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL
    p->val.via.array.ptr[proto_index].type    = MSGPACK_OBJECT_POSITIVE_INTEGER;
    p->val.via.array.ptr[proto_index].via.u64 = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_SHELL;
    proto_index++;
#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_SHELL */
    p->val.via.array.ptr[proto_index].type    = MSGPACK_OBJECT_POSITIVE_INTEGER;
    p->val.via.array.ptr[proto_index].via.u64 = MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_MENDER_CLIENT;

END:

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */
