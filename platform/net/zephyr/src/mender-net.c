/**
 * @file      mender-net.c
 * @brief     Mender network common file for Zephyr platform
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

#include <errno.h>
#include <zephyr/net/socket.h>
#ifdef CONFIG_NET_SOCKETS_SOCKOPT_TLS
#include <zephyr/net/tls_credentials.h>
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */
#include "mender-log.h"
#include "mender-net.h"
#include "mender-utils.h"

/**
 * @brief Default TLS_PEER_VERIFY option
 */
#ifndef CONFIG_MENDER_NET_TLS_PEER_VERIFY
#define CONFIG_MENDER_NET_TLS_PEER_VERIFY (2)
#endif /* CONFIG_MENDER_NET_TLS_PEER_VERIFY */

mender_err_t
mender_net_get_host_port_url(char *path, char *config_host, char **host, char **port, char **url) {

    assert(NULL != path);
    assert(NULL != host);
    assert(NULL != port);
    char *tmp;
    char *saveptr;

    /* Check if the path start with protocol */
    if ((false == mender_utils_strbeginwith(path, "http://")) && (false == mender_utils_strbeginwith(path, "https://"))) {

        /* Path contain the URL only, retrieve host and port from configuration */
        assert(NULL != url);
        if (NULL == (*url = strdup(path))) {
            mender_log_error("Unable to allocate memory");
            return MENDER_FAIL;
        }
        return mender_net_get_host_port_url(config_host, NULL, host, port, NULL);
    }

    /* Create a working copy of the path */
    if (NULL == (tmp = strdup(path))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Retrieve protocol and host */
    char *protocol = strtok_r(tmp, "/", &saveptr);
    char *pch1     = strtok_r(NULL, "/", &saveptr);

    /* Check if the host contains port */
    char *pch2 = strchr(pch1, ':');
    if (NULL != pch2) {
        /* Port is specified */
        if (NULL == (*host = malloc(pch2 - pch1 + 1))) {
            mender_log_error("Unable to allocate memory");
            free(tmp);
            return MENDER_FAIL;
        }
        strncpy(*host, pch1, pch2 - pch1);
        *port = strdup(pch2 + 1);
    } else {
        /* Port is not specified */
        *host = strdup(pch1);
        if (true == mender_utils_strbeginwith(path, "http://")) {
            *port = strdup("80");
        } else if (true == mender_utils_strbeginwith(path, "https://")) {
            *port = strdup("443");
        }
    }
    if (NULL != url) {
        *url = strdup(path + strlen(protocol) + 2 + strlen(pch1));
    }

    /* Release memory */
    free(tmp);

    return MENDER_OK;
}

mender_err_t
mender_net_connect(const char *host, const char *port, int *sock) {

    assert(NULL != host);
    assert(NULL != port);
    assert(NULL != sock);
    int              result;
    mender_err_t     ret = MENDER_OK;
    struct addrinfo  hints;
    struct addrinfo *addr = NULL;

    /* Set hints */
    memset(&hints, 0, sizeof(hints));
    if (IS_ENABLED(CONFIG_NET_IPV6)) {
        hints.ai_family = AF_INET6;
    } else if (IS_ENABLED(CONFIG_NET_IPV4)) {
        hints.ai_family = AF_INET;
    }
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    /* Perform DNS resolution of the host */
    if (0 != (result = getaddrinfo(host, port, &hints, &addr))) {
        mender_log_error("Unable to resolve host name '%s:%s', result = %d, errno = %d", host, port, result, errno);
        ret = MENDER_FAIL;
        goto END;
    }

    /* Create socket */
#ifdef CONFIG_NET_SOCKETS_SOCKOPT_TLS
    if ((result = socket(addr->ai_family, SOCK_STREAM, IPPROTO_TLS_1_2)) < 0) {
#else
    if ((result = socket(addr->ai_family, SOCK_STREAM, IPPROTO_TCP)) < 0) {
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */
        mender_log_error("Unable to create socket, result = %d, errno= %d", result, errno);
        ret = MENDER_FAIL;
        goto END;
    }
    *sock = result;

#ifdef CONFIG_NET_SOCKETS_SOCKOPT_TLS

    /* Set TLS_SEC_TAG_LIST option */
    sec_tag_t sec_tag[] = {
        CONFIG_MENDER_NET_CA_CERTIFICATE_TAG,
    };
    if ((result = setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag, sizeof(sec_tag))) < 0) {
        mender_log_error("Unable to set TLS_SEC_TAG_LIST option, result = %d, errno = %d", result, errno);
        close(*sock);
        *sock = -1;
        ret   = MENDER_FAIL;
        goto END;
    }

    /* Set TLS_HOSTNAME option */
    if ((result = setsockopt(*sock, SOL_TLS, TLS_HOSTNAME, host, strlen(host))) < 0) {
        mender_log_error("Unable to set TLS_HOSTNAME option, result = %d, errno = %d", result, errno);
        close(*sock);
        *sock = -1;
        ret   = MENDER_FAIL;
        goto END;
    }

    /* Set TLS_PEER_VERIFY option */
    int verify = CONFIG_MENDER_NET_TLS_PEER_VERIFY;
    if ((result = setsockopt(*sock, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(int))) < 0) {
        mender_log_error("Unable to set TLS_PEER_VERIFY option, result = %d, errno = %d", result, errno);
        close(*sock);
        *sock = -1;
        ret   = MENDER_FAIL;
        goto END;
    }

#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */

    /* Connect to the host */
    if (0 != (result = connect(*sock, addr->ai_addr, addr->ai_addrlen))) {
        mender_log_error("Unable to connect to the host '%s:%s', result = %d, errno = %d", host, port, result, errno);
        close(*sock);
        *sock = -1;
        ret   = MENDER_FAIL;
        goto END;
    }

END:

    /* Release memory */
    if (NULL != addr) {
        freeaddrinfo(addr);
    }

    return ret;
}

mender_err_t
mender_net_disconnect(int sock) {

    /* Close socket */
    close(sock);

    return MENDER_OK;
}
