/**
 * @file      mender-troubleshoot-protomsg.c
 * @brief     Mender MCU Troubleshoot add-on protomsg implementation
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

#include <assert.h>
#include "mender-log.h"
#include "mender-troubleshoot-msgpack.h"
#include "mender-troubleshoot-protomsg.h"
#include <stdlib.h>

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

/**
 * @brief Encode proto message
 * @param protomsg Proto message
 * @param p Object key-value
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_protomsg_encode(mender_troubleshoot_protomsg_t *protomsg, msgpack_object *object);

/**
 * @brief Encode proto message header
 * @param hdr Proto message header
 * @param p Object key-value
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_protomsg_hdr_encode(mender_troubleshoot_protomsg_hdr_t *hdr, msgpack_object_kv *p);

/**
 * @brief Encode proto message header properties
 * @param properties Proto message header properties
 * @param p Object key-value
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_protomsg_hdr_properties_encode(mender_troubleshoot_protomsg_hdr_properties_t *properties, msgpack_object_kv *p);

/**
 * @brief Encode proto message body
 * @param body Proto message body
 * @param p Object key-value
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_troubleshoot_protomsg_body_encode(mender_troubleshoot_protomsg_body_t *body, msgpack_object_kv *p);

/**
 * @brief Decode proto message object
 * @param object Proto message object
 * @return Proto message if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_protomsg_t *mender_troubleshoot_protomsg_decode(msgpack_object *object);

/**
 * @brief Decode proto message header object
 * @param object Proto message header object
 * @return Proto header if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_protomsg_hdr_t *mender_troubleshoot_protomsg_hdr_decode(msgpack_object *object);

/**
 * @brief Decode proto message header properties object
 * @param object Proto message header properties object
 * @return Proto header properties if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_protomsg_hdr_properties_t *mender_troubleshoot_protomsg_hdr_properties_decode(msgpack_object *object);

/**
 * @brief Decode proto message body object
 * @param object Proto message body object
 * @return Body if the function succeeds, NULL otherwise
 */
static mender_troubleshoot_protomsg_body_t *mender_troubleshoot_protomsg_body_decode(msgpack_object *object);

/**
 * @brief Release proto message header
 * @param hdr Proto message header
 */
static void mender_troubleshoot_protomsg_hdr_release(mender_troubleshoot_protomsg_hdr_t *hdr);

/**
 * @brief Release proto message header properties
 * @param properties Proto message header properties
 */
static void mender_troubleshoot_protomsg_hdr_properties_release(mender_troubleshoot_protomsg_hdr_properties_t *properties);

/**
 * @brief Release proto message body
 * @param body Proto message body
 */
static void mender_troubleshoot_protomsg_body_release(mender_troubleshoot_protomsg_body_t *body);

mender_err_t
mender_troubleshoot_protomsg_pack(mender_troubleshoot_protomsg_t *protomsg, void **data, size_t *length) {

    assert(NULL != protomsg);
    assert(NULL != data);
    assert(NULL != length);
    mender_err_t   ret = MENDER_OK;
    msgpack_object object;

    /* Encode protomsg */
    if (MENDER_OK != (ret = mender_troubleshoot_protomsg_encode(protomsg, &object))) {
        mender_log_error("Invalid protomsg object");
        goto FAIL;
    }

    /* Pack protomsg */
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
mender_troubleshoot_protomsg_encode(mender_troubleshoot_protomsg_t *protomsg, msgpack_object *object) {

    assert(NULL != protomsg);
    assert(NULL != object);
    mender_err_t       ret = MENDER_OK;
    msgpack_object_kv *p;

    /* Create protomsg */
    object->type = MSGPACK_OBJECT_MAP;
    if (0 == (object->via.map.size = ((NULL != protomsg->hdr) ? 1 : 0) + ((NULL != protomsg->body) ? 1 : 0))) {
        goto END;
    }
    if (NULL == (object->via.map.ptr = (msgpack_object_kv *)malloc(object->via.map.size * sizeof(struct msgpack_object_kv)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Parse protomsg */
    p = object->via.map.ptr;
    if (NULL != protomsg->hdr) {
        if (MENDER_OK != (ret = mender_troubleshoot_protomsg_hdr_encode(protomsg->hdr, p))) {
            mender_log_error("Unable to encode header");
            goto FAIL;
        }
        ++p;
    }
    if (NULL != protomsg->body) {
        if (MENDER_OK != (ret = mender_troubleshoot_protomsg_body_encode(protomsg->body, p))) {
            mender_log_error("Unable to encode body");
            goto FAIL;
        }
    }

END:
FAIL:

    return ret;
}

static mender_err_t
mender_troubleshoot_protomsg_hdr_encode(mender_troubleshoot_protomsg_hdr_t *hdr, msgpack_object_kv *p) {

    assert(NULL != hdr);
    assert(NULL != p);
    mender_err_t ret = MENDER_OK;

    /* Create header */
    p->key.type = MSGPACK_OBJECT_STR;
    if (NULL == (p->key.via.str.ptr = strdup("hdr"))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    p->key.via.str.size = (uint32_t)strlen("hdr");
    p->val.type         = MSGPACK_OBJECT_MAP;
    if (0 == (p->val.via.map.size = 1 + ((NULL != hdr->typ) ? 1 : 0) + ((NULL != hdr->sid) ? 1 : 0) + ((NULL != hdr->properties) ? 1 : 0))) {
        goto END;
    }
    if (NULL == (p->val.via.map.ptr = (msgpack_object_kv *)malloc(p->val.via.map.size * sizeof(struct msgpack_object_kv)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Parse header */
    p           = p->val.via.map.ptr;
    p->key.type = MSGPACK_OBJECT_STR;
    if (NULL == (p->key.via.str.ptr = strdup("proto"))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    p->key.via.str.size = (uint32_t)strlen("proto");
    p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
    p->val.via.u64      = hdr->proto;
    ++p;
    if (NULL != hdr->typ) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("typ"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("typ");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = (uint32_t)strlen(hdr->typ);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy((void *)p->val.via.str.ptr, hdr->typ, p->val.via.str.size);
        ++p;
    }
    if (NULL != hdr->sid) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("sid"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("sid");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = (uint32_t)strlen(hdr->sid);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy((void *)p->val.via.str.ptr, hdr->sid, p->val.via.str.size);
        ++p;
    }
    if (NULL != hdr->properties) {
        if (MENDER_OK != (ret = mender_troubleshoot_protomsg_hdr_properties_encode(hdr->properties, p))) {
            mender_log_error("Unable to encode header properties");
            goto FAIL;
        }
    }

END:
FAIL:

    return ret;
}

static mender_err_t
mender_troubleshoot_protomsg_hdr_properties_encode(mender_troubleshoot_protomsg_hdr_properties_t *properties, msgpack_object_kv *p) {

    assert(NULL != properties);
    assert(NULL != p);
    mender_err_t ret = MENDER_OK;

    /* Create properties */
    p->key.type = MSGPACK_OBJECT_STR;
    if (NULL == (p->key.via.str.ptr = strdup("props"))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    p->key.via.str.size = (uint32_t)strlen("props");
    p->val.type         = MSGPACK_OBJECT_MAP;
    if (0
        == (p->val.via.map.size = ((NULL != properties->terminal_width) ? 1 : 0) + ((NULL != properties->terminal_height) ? 1 : 0)
                                  + ((NULL != properties->user_id) ? 1 : 0) + ((NULL != properties->timeout) ? 1 : 0) + ((NULL != properties->status) ? 1 : 0)
                                  + ((NULL != properties->offset) ? 1 : 0))) {
        goto END;
    }
    if (NULL == (p->val.via.map.ptr = (msgpack_object_kv *)malloc(p->val.via.map.size * sizeof(struct msgpack_object_kv)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Parse properties */
    p = p->val.via.map.ptr;
    if (NULL != properties->terminal_width) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("terminal_width"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("terminal_width");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *properties->terminal_width;
        ++p;
    }
    if (NULL != properties->terminal_height) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("terminal_height"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("terminal_height");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *properties->terminal_height;
        ++p;
    }
    if (NULL != properties->user_id) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("user_id"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("user_id");
        p->val.type         = MSGPACK_OBJECT_STR;
        p->val.via.str.size = (uint32_t)strlen(properties->user_id);
        if (NULL == (p->val.via.str.ptr = (char *)malloc(p->val.via.str.size))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        memcpy((void *)p->val.via.str.ptr, properties->user_id, p->val.via.str.size);
        ++p;
    }
    if (NULL != properties->timeout) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("timeout"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("timeout");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *properties->timeout;
        ++p;
    }
    if (NULL != properties->status) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("status"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("status");
        p->val.type         = MSGPACK_OBJECT_POSITIVE_INTEGER;
        p->val.via.u64      = *properties->status;
        ++p;
    }
    if (NULL != properties->offset) {
        p->key.type = MSGPACK_OBJECT_STR;
        if (NULL == (p->key.via.str.ptr = strdup("offset"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        p->key.via.str.size = (uint32_t)strlen("offset");
        p->val.type         = MSGPACK_OBJECT_FIX_INT64; /* Mandatory so that the mender-server has the correct type */
        p->val.via.i64      = *properties->offset;
    }

END:
FAIL:

    return ret;
}

static mender_err_t
mender_troubleshoot_protomsg_body_encode(mender_troubleshoot_protomsg_body_t *body, msgpack_object_kv *p) {

    assert(NULL != body);
    assert(NULL != body->data);
    assert(NULL != p);
    mender_err_t ret = MENDER_OK;

    /* Create body */
    p->key.type = MSGPACK_OBJECT_STR;
    if (NULL == (p->key.via.str.ptr = strdup("body"))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    p->key.via.str.size = (uint32_t)strlen("body");
    p->val.type         = MSGPACK_OBJECT_BIN;
    p->val.via.bin.size = (uint32_t)body->length;
    if (NULL == (p->val.via.bin.ptr = (char *)malloc(p->val.via.bin.size * sizeof(uint8_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memcpy((void *)p->val.via.bin.ptr, body->data, p->val.via.bin.size);

FAIL:

    return ret;
}

mender_troubleshoot_protomsg_t *
mender_troubleshoot_protomsg_unpack(void *data, size_t length) {

    assert(NULL != data);
    msgpack_zone                    zone;
    msgpack_object                  object;
    mender_troubleshoot_protomsg_t *protomsg = NULL;

    /* Unpack the message */
    if (MENDER_OK != mender_troubleshoot_msgpack_unpack_object(data, length, &zone, &object)) {
        mender_log_error("Unable to unpack the message");
        goto FAIL;
    }

    /* Check object type */
    if ((MSGPACK_OBJECT_MAP != object.type) || (0 == object.via.map.size)) {
        mender_log_error("Invalid protomsg object");
        goto FAIL;
    }

    /* Decode protomsg */
    if (NULL == (protomsg = mender_troubleshoot_protomsg_decode(&object))) {
        mender_log_error("Invalid protomsg object");
        goto FAIL;
    }

FAIL:

    /* Release memory */
    msgpack_zone_destroy(&zone);

    return protomsg;
}

static mender_troubleshoot_protomsg_t *
mender_troubleshoot_protomsg_decode(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_protomsg_t *protomsg;

    /* Create protomsg */
    if (NULL == (protomsg = (mender_troubleshoot_protomsg_t *)malloc(sizeof(mender_troubleshoot_protomsg_t)))) {
        mender_log_error("Unable to allocate memory");
        goto FAIL;
    }
    memset(protomsg, 0, sizeof(mender_troubleshoot_protomsg_t));

    /* Parse protomsg */
    msgpack_object_kv *p = object->via.map.ptr;
    do {
        if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "hdr", p->key.via.str.size)) && (MSGPACK_OBJECT_MAP == p->val.type)
            && (0 != p->val.via.map.size)) {
            if (NULL == (protomsg->hdr = mender_troubleshoot_protomsg_hdr_decode(&p->val))) {
                mender_log_error("Invalid protomsg object");
                goto FAIL;
            }
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "body", p->key.via.str.size)) && (MSGPACK_OBJECT_BIN == p->val.type)
                   && (0 != p->val.via.bin.size)) {
            if (NULL == (protomsg->body = mender_troubleshoot_protomsg_body_decode(&p->val))) {
                mender_log_error("Invalid protomsg object");
                goto FAIL;
            }
        }
        ++p;
    } while (p < object->via.map.ptr + object->via.map.size);

    return protomsg;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_release(protomsg);

    return NULL;
}

static mender_troubleshoot_protomsg_hdr_t *
mender_troubleshoot_protomsg_hdr_decode(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_protomsg_hdr_t *hdr;

    /* Create header */
    if (NULL == (hdr = (mender_troubleshoot_protomsg_hdr_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_t)))) {
        mender_log_error("Unable to allocate memory");
        goto FAIL;
    }
    memset(hdr, 0, sizeof(mender_troubleshoot_protomsg_hdr_t));

    /* Parse header */
    msgpack_object_kv *p = object->via.map.ptr;
    do {
        if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "proto", p->key.via.str.size))
            && (MSGPACK_OBJECT_POSITIVE_INTEGER == p->val.type)) {
            hdr->proto = (mender_troubleshoot_protomsg_hdr_proto_t)p->val.via.u64;
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "typ", p->key.via.str.size)) && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (hdr->typ = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(hdr->typ, p->val.via.str.ptr, p->val.via.str.size);
            hdr->typ[p->val.via.str.size] = '\0';
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "sid", p->key.via.str.size)) && (MSGPACK_OBJECT_STR == p->val.type)) {
            if (NULL == (hdr->sid = (char *)malloc(p->val.via.str.size + 1))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            memcpy(hdr->sid, p->val.via.str.ptr, p->val.via.str.size);
            hdr->sid[p->val.via.str.size] = '\0';
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "props", p->key.via.str.size)) && (MSGPACK_OBJECT_MAP == p->val.type)
                   && (0 != p->val.via.map.size)) {
            if (NULL == (hdr->properties = mender_troubleshoot_protomsg_hdr_properties_decode(&p->val))) {
                mender_log_error("Invalid header properties object");
                goto FAIL;
            }
        }
        ++p;
    } while (p < object->via.map.ptr + object->via.map.size);

    return hdr;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_hdr_release(hdr);

    return NULL;
}

static mender_troubleshoot_protomsg_hdr_properties_t *
mender_troubleshoot_protomsg_hdr_properties_decode(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_protomsg_hdr_properties_t *properties = NULL;

    /* Create header properties */
    if (NULL == (properties = (mender_troubleshoot_protomsg_hdr_properties_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_t)))) {
        mender_log_error("Unable to allocate memory");
        goto FAIL;
    }
    memset(properties, 0, sizeof(mender_troubleshoot_protomsg_hdr_properties_t));

    /* Parse header properties */
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
            if (NULL
                == (properties->status
                    = (mender_troubleshoot_protomsg_hdr_properties_status_t *)malloc(sizeof(mender_troubleshoot_protomsg_hdr_properties_status_t)))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            *properties->status = (mender_troubleshoot_protomsg_hdr_properties_status_t)p->val.via.u64;
        } else if ((MSGPACK_OBJECT_STR == p->key.type) && (!strncmp(p->key.via.str.ptr, "offset", p->key.via.str.size))
                   && (MSGPACK_OBJECT_POSITIVE_INTEGER == p->val.type)) {
            if (NULL == (properties->offset = (size_t *)malloc(sizeof(size_t)))) {
                mender_log_error("Unable to allocate memory");
                goto FAIL;
            }
            *properties->offset = (size_t)p->val.via.u64;
        }
        ++p;
    } while (p < object->via.map.ptr + object->via.map.size);

    return properties;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_hdr_properties_release(properties);

    return NULL;
}

static mender_troubleshoot_protomsg_body_t *
mender_troubleshoot_protomsg_body_decode(msgpack_object *object) {

    assert(NULL != object);
    mender_troubleshoot_protomsg_body_t *body;

    /* Create body */
    if (NULL == (body = (mender_troubleshoot_protomsg_body_t *)malloc(sizeof(mender_troubleshoot_protomsg_body_t)))) {
        mender_log_error("Unable to allocate memory");
        goto FAIL;
    }
    memset(body, 0, sizeof(mender_troubleshoot_protomsg_body_t));
    if (NULL == (body->data = malloc(object->via.bin.size))) {
        mender_log_error("Unable to allocate memory");
        goto FAIL;
    }
    memcpy(body->data, object->via.bin.ptr, object->via.bin.size);
    body->length = object->via.bin.size;

    return body;

FAIL:

    /* Release memory */
    mender_troubleshoot_protomsg_body_release(body);

    return NULL;
}

void
mender_troubleshoot_protomsg_release(mender_troubleshoot_protomsg_t *protomsg) {

    /* Release memory */
    if (NULL != protomsg) {
        mender_troubleshoot_protomsg_hdr_release(protomsg->hdr);
        mender_troubleshoot_protomsg_body_release(protomsg->body);
        free(protomsg);
    }
}

static void
mender_troubleshoot_protomsg_hdr_release(mender_troubleshoot_protomsg_hdr_t *hdr) {

    /* Release memory */
    if (NULL != hdr) {
        if (NULL != hdr->typ) {
            free(hdr->typ);
        }
        if (NULL != hdr->sid) {
            free(hdr->sid);
        }
        mender_troubleshoot_protomsg_hdr_properties_release(hdr->properties);
        free(hdr);
    }
}

static void
mender_troubleshoot_protomsg_hdr_properties_release(mender_troubleshoot_protomsg_hdr_properties_t *properties) {

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
        if (NULL != properties->offset) {
            free(properties->offset);
        }
        free(properties);
    }
}

static void
mender_troubleshoot_protomsg_body_release(mender_troubleshoot_protomsg_body_t *body) {

    /* Release memory */
    if (NULL != body) {
        if (NULL != body->data) {
            free(body->data);
        }
        free(body);
    }
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */
