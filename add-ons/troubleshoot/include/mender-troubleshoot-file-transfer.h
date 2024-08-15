/**
 * @file      mender-troubleshoot-file-transfer.h
 * @brief     Mender MCU Troubleshoot add-on file transfer messages handler implementation
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

#ifndef __MENDER_TROUBLESHOOT_FILE_TRANSFER_H__
#define __MENDER_TROUBLESHOOT_FILE_TRANSFER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-troubleshoot-protomsg.h"
#include <time.h>

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER

/**
 * @brief Mender troubleshoot file transfer callbacks
 */
typedef struct {
    mender_err_t (*stat)(char *, size_t **, uint32_t **, uint32_t **, uint32_t **, time_t **); /**< Invoked to get statistics of a file */
    mender_err_t (*open)(char *, char *, void **);                                             /**< Invoked to open a file */
    mender_err_t (*read)(void *, void *, size_t *);                                            /**< Invoked to read data from the file */
    mender_err_t (*write)(void *, void *, size_t);                                             /**< Invoked to write data to the file */
    mender_err_t (*close)(void *);                                                             /**< Invoked to close the file */
} mender_troubleshoot_file_transfer_callbacks_t;

/**
 * @brief Initialize mender troubleshoot add-on file transfer handler
 * @param callbacks Mender troubleshoot file transfer callbacks
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_file_transfer_init(mender_troubleshoot_file_transfer_callbacks_t *callbacks);

/**
 * @brief Function called to perform the treatment of the file transfer messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
mender_err_t mender_troubleshoot_file_transfer_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Release mender troubleshoot add-on file transfer handler
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_file_transfer_exit(void);

#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_FILE_TRANSFER */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_TROUBLESHOOT_FILE_TRANSFER_H__ */
