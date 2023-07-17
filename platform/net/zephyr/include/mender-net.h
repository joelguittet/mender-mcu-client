/**
 * @file      mender-net.h
 * @brief     Mender network common file interface for Zephyr platform
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
