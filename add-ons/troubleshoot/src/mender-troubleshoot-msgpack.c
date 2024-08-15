/**
 * @file      mender-troubleshoot-msgpack.c
 * @brief     Mender MCU Troubleshoot add-on msgpack implementation
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
#include "mender-troubleshoot-msgpack.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

/**
 * @brief msgpack sbuffer initialization size
 */
#define MENDER_TROUBLESHOOT_SBUFFER_INIT_SIZE (256)

/**
 * @brief msgpack zone chunk initialization size
 */
#define MENDER_TROUBLESHOOT_PROTOMSG_ZONE_CHUNK_INIT_SIZE (2048)

/**
 * @brief Pack object with support of fixint values
 * @note Created from msgpack_pack_object from msgpack-c library
 * @param pk Packer
 * @param d Object
 * @return See msgpack_pack_object definition
 */
int msgpack_pack_object_with_fixint(msgpack_packer *pk, msgpack_object d);

mender_err_t
mender_troubleshoot_msgpack_pack_object(msgpack_object *object, void **data, size_t *length) {

    assert(NULL != data);
    assert(NULL != length);
    mender_err_t    ret = MENDER_OK;
    msgpack_sbuffer sbuffer;
    msgpack_packer  packer;

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

    /* Pack the message */
    if (0 != msgpack_pack_object_with_fixint(&packer, *object)) {
        mender_log_error("Unable to pack the message");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Return sbuffer data and size */
    *data   = sbuffer.data;
    *length = sbuffer.size;

    return ret;

FAIL:

    /* Release memory */
    msgpack_sbuffer_destroy(&sbuffer);

    return ret;
}

int
msgpack_pack_object_with_fixint(msgpack_packer *pk, msgpack_object d) {
    switch ((int)d.type) {
        case MSGPACK_OBJECT_NIL:
            return msgpack_pack_nil(pk);

        case MSGPACK_OBJECT_BOOLEAN:
            if (d.via.boolean) {
                return msgpack_pack_true(pk);
            } else {
                return msgpack_pack_false(pk);
            }

        case MSGPACK_OBJECT_POSITIVE_INTEGER:
            return msgpack_pack_uint64(pk, d.via.u64);

        case MSGPACK_OBJECT_NEGATIVE_INTEGER:
            return msgpack_pack_int64(pk, d.via.i64);

        case MSGPACK_OBJECT_FLOAT32:
            return msgpack_pack_float(pk, (float)d.via.f64);

        case MSGPACK_OBJECT_FLOAT64:
            return msgpack_pack_double(pk, d.via.f64);

        case MSGPACK_OBJECT_STR: {
            int ret = msgpack_pack_str(pk, d.via.str.size);
            if (ret < 0) {
                return ret;
            }
            return msgpack_pack_str_body(pk, d.via.str.ptr, d.via.str.size);
        }

        case MSGPACK_OBJECT_BIN: {
            int ret = msgpack_pack_bin(pk, d.via.bin.size);
            if (ret < 0) {
                return ret;
            }
            return msgpack_pack_bin_body(pk, d.via.bin.ptr, d.via.bin.size);
        }

        case MSGPACK_OBJECT_EXT: {
            int ret = msgpack_pack_ext(pk, d.via.ext.size, d.via.ext.type);
            if (ret < 0) {
                return ret;
            }
            return msgpack_pack_ext_body(pk, d.via.ext.ptr, d.via.ext.size);
        }

        case MSGPACK_OBJECT_ARRAY: {
            int ret = msgpack_pack_array(pk, d.via.array.size);
            if (ret < 0) {
                return ret;
            } else {
                msgpack_object       *o    = d.via.array.ptr;
                msgpack_object *const oend = d.via.array.ptr + d.via.array.size;
                for (; o != oend; ++o) {
                    ret = msgpack_pack_object_with_fixint(pk, *o);
                    if (ret < 0) {
                        return ret;
                    }
                }

                return 0;
            }
        }

        case MSGPACK_OBJECT_MAP: {
            int ret = msgpack_pack_map(pk, d.via.map.size);
            if (ret < 0) {
                return ret;
            } else {
                msgpack_object_kv       *kv    = d.via.map.ptr;
                msgpack_object_kv *const kvend = d.via.map.ptr + d.via.map.size;
                for (; kv != kvend; ++kv) {
                    ret = msgpack_pack_object_with_fixint(pk, kv->key);
                    if (ret < 0) {
                        return ret;
                    }
                    ret = msgpack_pack_object_with_fixint(pk, kv->val);
                    if (ret < 0) {
                        return ret;
                    }
                }

                return 0;
            }
        }

        case MSGPACK_OBJECT_FIX_UINT8:
            return msgpack_pack_fix_uint8(pk, (uint8_t)d.via.u64);

        case MSGPACK_OBJECT_FIX_UINT16:
            return msgpack_pack_fix_uint16(pk, (uint16_t)d.via.u64);

        case MSGPACK_OBJECT_FIX_UINT32:
            return msgpack_pack_fix_uint32(pk, (uint32_t)d.via.u64);

        case MSGPACK_OBJECT_FIX_UINT64:
            return msgpack_pack_fix_uint64(pk, d.via.u64);

        case MSGPACK_OBJECT_FIX_INT8:
            return msgpack_pack_fix_int8(pk, (int8_t)d.via.i64);

        case MSGPACK_OBJECT_FIX_INT16:
            return msgpack_pack_fix_int16(pk, (int16_t)d.via.i64);

        case MSGPACK_OBJECT_FIX_INT32:
            return msgpack_pack_fix_int32(pk, (int32_t)d.via.i64);

        case MSGPACK_OBJECT_FIX_INT64:
            return msgpack_pack_fix_int64(pk, d.via.i64);

        default:
            return -1;
    }
}

mender_err_t
mender_troubleshoot_msgpack_unpack_object(void *data, size_t length, msgpack_zone *zone, msgpack_object *object) {

    assert(NULL != data);
    assert(NULL != object);
    msgpack_unpack_return res;
    mender_err_t          ret = MENDER_OK;

    /* Initialize msgpack zone */
    if (true != msgpack_zone_init(zone, MENDER_TROUBLESHOOT_PROTOMSG_ZONE_CHUNK_INIT_SIZE)) {
        mender_log_error("Unable to initialize msgpack zone");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Unpack the message */
    if (MSGPACK_UNPACK_SUCCESS != (res = msgpack_unpack((const char *)data, length, NULL, zone, object))) {
        mender_log_error("Unable to unpack object (res=%d)", res);
        ret = MENDER_FAIL;
        goto FAIL;
    }

FAIL:

    return ret;
}

void
mender_troubleshoot_msgpack_release_object(msgpack_object *object) {

    /* Release memory */
    if (NULL != object) {
        switch ((int)object->type) {
            case MSGPACK_OBJECT_NIL:
            case MSGPACK_OBJECT_BOOLEAN:
            case MSGPACK_OBJECT_POSITIVE_INTEGER:
            case MSGPACK_OBJECT_FIX_UINT8:
            case MSGPACK_OBJECT_FIX_UINT16:
            case MSGPACK_OBJECT_FIX_UINT32:
            case MSGPACK_OBJECT_FIX_UINT64:
            case MSGPACK_OBJECT_NEGATIVE_INTEGER:
            case MSGPACK_OBJECT_FIX_INT8:
            case MSGPACK_OBJECT_FIX_INT16:
            case MSGPACK_OBJECT_FIX_INT32:
            case MSGPACK_OBJECT_FIX_INT64:
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
                        mender_troubleshoot_msgpack_release_object(p);
                        ++p;
                    } while (p < object->via.array.ptr + object->via.array.size);
                    free(object->via.array.ptr);
                }
                break;
            case MSGPACK_OBJECT_MAP:
                if (NULL != object->via.map.ptr) {
                    msgpack_object_kv *p = object->via.map.ptr;
                    do {
                        mender_troubleshoot_msgpack_release_object(&(p->key));
                        mender_troubleshoot_msgpack_release_object(&(p->val));
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

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */
