/**
 * @file      mender-net.c
 * @brief     Mender network common file for Zephyr platform
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

#include <errno.h>
#include <version.h>
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

/**
 * @brief Perform connection with the server
 * @param host Host
 * @param addr Address information from zsock_getaddrinfo
 * @param sock Client socket
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_net_connect_internal(const char *host, struct zsock_addrinfo *addr, int *sock);

mender_err_t
mender_net_get_host_port_url(char *path, char *config_host, char **host, char **port, char **url) {

    assert(NULL != path);
    assert(NULL != host);
    assert(NULL != port);
    char *tmp;
    char *saveptr;

    /* Check if the path start with protocol */
    if ((false == mender_utils_strbeginwith(path, "http://")) && (false == mender_utils_strbeginwith(path, "https://"))
        && (false == mender_utils_strbeginwith(path, "ws://")) && (false == mender_utils_strbeginwith(path, "wss://"))) {

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
        if ((true == mender_utils_strbeginwith(path, "http://")) || (true == mender_utils_strbeginwith(path, "ws://"))) {
            *port = strdup("80");
        } else if ((true == mender_utils_strbeginwith(path, "https://")) || (true == mender_utils_strbeginwith(path, "wss://"))) {
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
    int                    result;
    mender_err_t           ret;
    struct zsock_addrinfo  hints;
    struct zsock_addrinfo *addr = NULL;
    struct zsock_addrinfo *tmp;
#if (ZEPHYR_VERSION_CODE >= ZEPHYR_VERSION(4, 3, 0))
    char tmp_str[ZSOCK_NI_MAXHOST];
#else
    char tmp_str[NI_MAXHOST];
#endif

    /* Set hints */
    memset(&hints, 0, sizeof(hints));
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
    hints.ai_family = AF_UNSPEC;
#elif defined(CONFIG_NET_IPV6)
    hints.ai_family = AF_INET6;
#elif defined(CONFIG_NET_IPV4)
    hints.ai_family = AF_INET;
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    /* Perform DNS resolution of the host */
    if (0 != (result = zsock_getaddrinfo(host, port, &hints, &addr))) {
        mender_log_error("Unable to resolve host name '%s:%s' (%d) (errno=%d)", host, port, result, errno);
        ret = MENDER_FAIL;
        goto END;
    }

    /* Connect to the host */
    ret = MENDER_FAIL;
    for (tmp = addr; (NULL != tmp) && (MENDER_OK != ret); tmp = tmp->ai_next) {
#if (ZEPHYR_VERSION_CODE >= ZEPHYR_VERSION(4, 3, 0))
        zsock_getnameinfo(tmp->ai_addr, tmp->ai_addrlen, tmp_str, sizeof(tmp_str), NULL, 0, ZSOCK_NI_NUMERICHOST);
#else
        zsock_getnameinfo(tmp->ai_addr, tmp->ai_addrlen, tmp_str, sizeof(tmp_str), NULL, 0, NI_NUMERICHOST);
#endif
        mender_log_debug("Trying to connect to the host '%s:%s' with IPv%d address '%s'...", host, port, (AF_INET6 == tmp->ai_family) ? 6 : 4, tmp_str);
        if (MENDER_OK != (ret = mender_net_connect_internal(host, tmp, sock))) {
            mender_log_warning("Unable to connect to the host '%s:%s' with IPv%d address '%s'", host, port, (AF_INET6 == tmp->ai_family) ? 6 : 4, tmp_str);
        } else {
            mender_log_info("Connected to the host '%s:%s' with IPv%d address '%s'", host, port, (AF_INET6 == tmp->ai_family) ? 6 : 4, tmp_str);
        }
    }
    if (MENDER_OK != ret) {
        mender_log_error("Unable to connect to the host '%s:%s'", host, port);
    }

END:

    /* Release memory */
    if (NULL != addr) {
        zsock_freeaddrinfo(addr);
    }

    return ret;
}

mender_err_t
mender_net_disconnect(int sock) {

    /* Close socket */
    zsock_close(sock);

    return MENDER_OK;
}

static mender_err_t
mender_net_connect_internal(const char *host, struct zsock_addrinfo *addr, int *sock) {

    assert(NULL != host);
    assert(NULL != sock);
    int          result;
    mender_err_t ret = MENDER_OK;

    /* Create socket */
#ifdef CONFIG_NET_SOCKETS_SOCKOPT_TLS
    if ((result = zsock_socket(addr->ai_family, SOCK_STREAM, IPPROTO_TLS_1_2)) < 0) {
#else
    if ((result = zsock_socket(addr->ai_family, SOCK_STREAM, IPPROTO_TCP)) < 0) {
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */
        mender_log_debug("Unable to create socket (%d) (errno=%d)", result, errno);
        ret = MENDER_FAIL;
        goto END;
    }
    *sock = result;

#ifdef CONFIG_NET_SOCKETS_SOCKOPT_TLS

    /* Set TLS_SEC_TAG_LIST option */
    sec_tag_t sec_tag[] = { CONFIG_MENDER_NET_CA_CERTIFICATE_TAG_PRIMARY,
#if (0 != CONFIG_MENDER_NET_CA_CERTIFICATE_TAG_SECONDARY)
                            CONFIG_MENDER_NET_CA_CERTIFICATE_TAG_SECONDARY
#endif /* (0 != CONFIG_MENDER_NET_CA_CERTIFICATE_TAG_SECONDARY) */
    };
    if ((result = zsock_setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag, sizeof(sec_tag))) < 0) {
        mender_log_debug("Unable to set TLS_SEC_TAG_LIST option (%d) (errno=%d)", result, errno);
        zsock_close(*sock);
        *sock = -1;
        ret   = MENDER_FAIL;
        goto END;
    }

    /* Set TLS_HOSTNAME option */
    if ((result = zsock_setsockopt(*sock, SOL_TLS, TLS_HOSTNAME, host, strlen(host))) < 0) {
        mender_log_debug("Unable to set TLS_HOSTNAME option (%d) (errno=%d)", result, errno);
        zsock_close(*sock);
        *sock = -1;
        ret   = MENDER_FAIL;
        goto END;
    }

    /* Set TLS_PEER_VERIFY option */
    int verify = CONFIG_MENDER_NET_TLS_PEER_VERIFY;
    if ((result = zsock_setsockopt(*sock, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(int))) < 0) {
        mender_log_debug("Unable to set TLS_PEER_VERIFY option (%d) (errno=%d)", result, errno);
        zsock_close(*sock);
        *sock = -1;
        ret   = MENDER_FAIL;
        goto END;
    }

#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */

    /* Connect to the host */
    if (0 != (result = zsock_connect(*sock, addr->ai_addr, addr->ai_addrlen))) {
        mender_log_debug("Unable to connect (%d) (errno=%d)", result, errno);
        zsock_close(*sock);
        *sock = -1;
        ret   = MENDER_FAIL;
        goto END;
    }

END:

    return ret;
}
