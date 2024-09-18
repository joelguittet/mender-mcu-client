/**
 * @file      mender-troubleshoot-port-forwarding.h
 * @brief     Mender MCU Troubleshoot add-on port forwarding messages handler implementation
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

#ifndef __MENDER_TROUBLESHOOT_PORT_FORWARDING_H__
#define __MENDER_TROUBLESHOOT_PORT_FORWARDING_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-troubleshoot-protomsg.h"
#include <time.h>

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT
#ifdef CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING

/**
 * @brief Mender troubleshoot port forwarding callbacks
 */
typedef struct {
    mender_err_t (*connect)(char *, uint16_t, char *, void **); /**< Open connection to remote host */
    mender_err_t (*send)(void *, void *, size_t);               /**< Send data to remote host */
    mender_err_t (*close)(void *);                              /**< Close connection to remote host */
} mender_troubleshoot_port_forwarding_callbacks_t;

/**
 * @brief Initialize mender troubleshoot add-on port forwarding handler
 * @param callbacks Mender troubleshoot port forwarding callbacks
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_port_forwarding_init(mender_troubleshoot_port_forwarding_callbacks_t *callbacks);

/**
 * @brief Function called to perform the treatment of the port forwarding messages
 * @param protomsg Received proto message
 * @param response Response to be sent back to the server, NULL if no response to send
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
mender_err_t mender_troubleshoot_port_forwarding_message_handler(mender_troubleshoot_protomsg_t *protomsg, mender_troubleshoot_protomsg_t **response);

/**
 * @brief Send port forwarding data to the server
 * @param data Data to send to the server
 * @param length Length of the data to send to the server
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
mender_err_t mender_troubleshoot_port_forwarding_forward(void *data, size_t length);

/**
 * @brief Close mender troubleshoot add-on port forwarding connection
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
mender_err_t mender_troubleshoot_port_forwarding_close(void);

/**
 * @brief Release mender troubleshoot add-on port forwarding handler
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_troubleshoot_port_forwarding_exit(void);

#endif /* CONFIG_MENDER_CLIENT_TROUBLESHOOT_PORT_FORWARDING */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_TROUBLESHOOT_PORT_FORWARDING_H__ */
