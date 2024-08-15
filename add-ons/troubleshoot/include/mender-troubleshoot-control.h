/**
 * @file      mender-troubleshoot-control.h
 * @brief     Mender MCU Troubleshoot add-on control messages handler implementation
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

#ifndef __MENDER_TROUBLESHOOT_CONTROL_H__
#define __MENDER_TROUBLESHOOT_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-troubleshoot-protomsg.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

/**
 * @brief Initialize mender troubleshoot add-on control handler
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_control_init(void);

/**
 * @brief Function called to perform the treatment of the control messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
mender_err_t mender_troubleshoot_control_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Close mender troubleshoot add-on control connection
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_control_close(void);

/**
 * @brief Release mender troubleshoot add-on control handler
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_control_exit(void);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_TROUBLESHOOT_CONTROL_H__ */
