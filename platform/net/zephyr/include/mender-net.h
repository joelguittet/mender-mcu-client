/**
 * @file      mender-net.h
 * @brief     Mender network common file interface for Zephyr platform
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

#ifndef __MENDER_NET_H__
#define __MENDER_NET_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Returns host name, port and URL from path
 * @param path Path
 * @param config_host Host name from configuration
 * @param host Host name
 * @param port Port as string
 * @param url URL
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_net_get_host_port_url(char *path, char *config_host, char **host, char **port, char **url);

/**
 * @brief Perform connection with the server
 * @param host Host
 * @param port Port
 * @param sock Client socket
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_net_connect(const char *host, const char *port, int *sock);

/**
 * @brief Close connection with the server
 * @param sock Client socket
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_net_disconnect(int sock);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_NET_H__ */
