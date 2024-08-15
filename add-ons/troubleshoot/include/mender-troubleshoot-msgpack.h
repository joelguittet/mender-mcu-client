/**
 * @file      mender-troubleshoot-msgpack.h
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

#ifndef __MENDER_TROUBLESHOOT_MSGPACK_H__
#define __MENDER_TROUBLESHOOT_MSGPACK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

#include <msgpack.h>

/**
 * @brief msgpack object additional types
 */
#define MSGPACK_OBJECT_FIX_UINT8  0x80
#define MSGPACK_OBJECT_FIX_UINT16 0x81
#define MSGPACK_OBJECT_FIX_UINT32 0x82
#define MSGPACK_OBJECT_FIX_UINT64 0x83
#define MSGPACK_OBJECT_FIX_INT8   0x84
#define MSGPACK_OBJECT_FIX_INT16  0x85
#define MSGPACK_OBJECT_FIX_INT32  0x86
#define MSGPACK_OBJECT_FIX_INT64  0x87

/**
 * @brief Pack msgpack object
 * @param object msgpack object
 * @param data Data
 * @param length Length of the data
 * @param MENDER_OK if the function succeeds, error code if an error occured
 */
mender_err_t mender_troubleshoot_msgpack_pack_object(msgpack_object *object, void **data, size_t *length);

/**
 * @brief Unpack msgpack object
 * @param data Data
 * @param length Length of the data
 * @param zone msgpack zone
 * @param object msgpack object
 * @param MENDER_OK if the function succeeds, error code if an error occured
 */
mender_err_t mender_troubleshoot_msgpack_unpack_object(void *data, size_t length, msgpack_zone *zone, msgpack_object *object);

/**
 * @brief Release msgpack object
 * @param object msgpack object
 */
void mender_troubleshoot_msgpack_release_object(msgpack_object *object);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_TROUBLESHOOT_MSGPACK_H__ */
