/**
 * @file      mender-troubleshoot-protomsg.h
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

#ifndef __MENDER_TROUBLESHOOT_PROTOMSG_H__
#define __MENDER_TROUBLESHOOT_PROTOMSG_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-troubleshoot-msgpack.h"
#include "mender-utils.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

/**
 * @brief Proto message header proto
 */
typedef enum {
    MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_INVALID       = 0x0000, /**< Invalid */
    MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_SHELL         = 0x0001, /**< Shell */
    MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_FILE_TRANSFER = 0x0002, /**< File transfer */
    MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_PORT_FORWARD  = 0x0003, /**< Port forward */
    MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_MENDER_CLIENT = 0x0004, /**< Mender client */
    MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROTO_CONTROL       = 0xFFFF  /**< Control */
} mender_troubleshoot_protomsg_hdr_proto_t;

/**
 * @brief Proto message header properties status type
 */
typedef enum {
    MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_NORMAL  = 0x0001, /**< Normal message */
    MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_ERROR   = 0x0002, /**< Error message */
    MENDER_TROUBLESHOOT_PROTOMSG_HDR_PROPERTIES_STATUS_TYPE_CONTROL = 0x0003  /**< Control message */
} mender_troubleshoot_protomsg_hdr_properties_status_t;

/**
 * @brief Proto message header properties
 */
typedef struct {
    uint16_t                                             *terminal_width;  /**< Terminal width */
    uint16_t                                             *terminal_height; /**< Terminal heigth */
    char                                                 *user_id;         /**< User ID */
    uint32_t                                             *timeout;         /**< Timeout */
    mender_troubleshoot_protomsg_hdr_properties_status_t *status;          /**< Status */
    size_t                                               *offset;          /**< Offset */
} mender_troubleshoot_protomsg_hdr_properties_t;

/**
 * @brief Proto message header
 */
typedef struct {
    mender_troubleshoot_protomsg_hdr_proto_t       proto;      /**< Proto type */
    char                                          *typ;        /**< Message type */
    char                                          *sid;        /**< Session ID */
    mender_troubleshoot_protomsg_hdr_properties_t *properties; /**< Properties */
} mender_troubleshoot_protomsg_hdr_t;

/**
 * @brief Proto message body
 */
typedef struct {
    void  *data;   /**< Data */
    size_t length; /**< Length */
} mender_troubleshoot_protomsg_body_t;

/**
 * @brief Proto message
 */
typedef struct {
    mender_troubleshoot_protomsg_hdr_t  *hdr;  /**< Header */
    mender_troubleshoot_protomsg_body_t *body; /**< Body */
} mender_troubleshoot_protomsg_t;

/**
 * @brief Encode and pack proto message
 * @param protomsg Proto message
 * @param data Packed data encoded
 * @param length Length of the data encoded
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_protomsg_pack(mender_troubleshoot_protomsg_t *protomsg, void **data, size_t *length);

/**
 * @brief Unpack and decode proto message
 * @param data Packed data to be decoded
 * @param length Length of the data to be decoded
 * @return Proto message if the function succeeds, NULL otherwise
 */
mender_troubleshoot_protomsg_t *mender_troubleshoot_protomsg_unpack(void *data, size_t length);

/**
 * @brief Release proto message
 * @param protomsg Proto message
 */
void mender_troubleshoot_protomsg_release(mender_troubleshoot_protomsg_t *protomsg);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_TROUBLESHOOT_PROTOMSG_H__ */
