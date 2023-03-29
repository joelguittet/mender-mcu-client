/**
 * @file      mender-http.c
 * @brief     Mender HTTP interface for Zephyr platform
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

#include <zephyr/net/http/client.h>
#include <zephyr/net/socket.h>
#ifdef CONFIG_NET_SOCKETS_SOCKOPT_TLS
#include <zephyr/net/tls_credentials.h>
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */
#include <errno.h>
#include "mender-http.h"
#include "mender-log.h"
#include "mender-rtos.h"

/**
 * @brief Receive buffer length
 */
#define MENDER_HTTP_RECV_BUF_LENGTH (512)

/**
 * @brief DNS resolve interval (seconds)
 */
#define MENDER_HTTP_DNS_RESOLVE_INTERVAL (10)

/**
 * @brief Connect interval (seconds)
 */
#define MENDER_HTTP_CONNECT_INTERVAL (10)

/**
 * @brief Request timeout (milliseconds)
 */
#define MENDER_HTTP_REQUEST_TIMEOUT (600000)

/**
 * @brief Request context
 */
typedef struct {
    mender_err_t (*callback)(mender_http_client_event_t, void *, size_t, void *);
    void *params;
    int   sock;
} mender_http_request_context;

/**
 * @brief Mender HTTP configuration
 */
static mender_http_config_t mender_http_config;

/**
 * @brief Perform HTTP connection with the server
 * @param host Host
 * @param port Port
 * @param sock Client socket
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_http_connect(const char *host, const char *port, int *sock);

/**
 * @brief HTTP response callback, invoked to handle data received
 * @param response HTTP response structure
 * @param final_call Indicate final call
 * @param user_data User data, used to retrieve request context data
 */
static void mender_http_response_cb(struct http_response *response, enum http_final_call final_call, void *user_data);

/**
 * @brief Returns host name, port and URL from path
 * @param path Path
 * @param host Host name
 * @param port Port as string
 * @param url URL
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_http_get_host_port_url(char *path, char **host, char **port, char **url);

/**
 * @brief Convert mender HTTP method to Zephyr HTTP client method
 * @param method Mender HTTP method
 * @return Zephyr HTTP client method if the function succeeds, -1 otherwise
 */
static enum http_method mender_http_method_to_zephyr_http_client_method(mender_http_method_t method);

mender_err_t
mender_http_init(mender_http_config_t *config) {

    assert(NULL != config);
    assert(NULL != config->host);

    /* Save configuration */
    memcpy(&mender_http_config, config, sizeof(mender_http_config_t));

    return MENDER_OK;
}

mender_err_t
mender_http_perform(char *               jwt,
                    char *               path,
                    mender_http_method_t method,
                    char *               payload,
                    char *               signature,
                    mender_err_t (*callback)(mender_http_client_event_t, void *, size_t, void *),
                    void *params,
                    int * status) {

    assert(NULL != path);
    assert(NULL != callback);
    assert(NULL != status);
    mender_err_t                ret;
    struct http_request         request;
    mender_http_request_context request_context;
    char *                      header_fields[4] = { NULL, NULL, NULL, NULL };
    size_t                      header_index     = 0;
    char *                      host             = NULL;
    char *                      port             = NULL;
    char *                      url              = NULL;

    /* Initialize request */
    memset(&request, 0, sizeof(struct http_request));

    /* Initialize request context */
    request_context.callback = callback;
    request_context.params   = params;
    request_context.sock     = -1;

    /* Retrieve host, port and url */
    if (MENDER_OK != (ret = mender_http_get_host_port_url(path, &host, &port, &url))) {
        mender_log_error("Unable to retrieve host/port/url");
        goto END;
    }

    /* Configuration of the client */
    request.method      = mender_http_method_to_zephyr_http_client_method(method);
    request.url         = url;
    request.host        = host;
    request.protocol    = "HTTP/1.1";
    request.payload     = payload;
    request.payload_len = (NULL != payload) ? strlen(payload) : 0;
    request.response    = mender_http_response_cb;
    if (NULL == (request.recv_buf = (uint8_t *)malloc(MENDER_HTTP_RECV_BUF_LENGTH))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    request.recv_buf_len = MENDER_HTTP_RECV_BUF_LENGTH;
    if (NULL != jwt) {
        if (NULL == (header_fields[header_index] = malloc(strlen("Authorization: Bearer ") + strlen(jwt) + strlen("\r\n") + 1))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        sprintf(header_fields[header_index], "Authorization: Bearer %s\r\n", jwt);
        header_index++;
    }
    if (NULL != signature) {
        if (NULL == (header_fields[header_index] = malloc(strlen("X-MEN-Signature: ") + strlen(signature) + strlen("\r\n") + 1))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        sprintf(header_fields[header_index], "X-MEN-Signature: %s\r\n", signature);
        header_index++;
    }
    if (NULL != payload) {
        if (NULL == (header_fields[header_index] = strdup("Content-Type: application/json\r\n"))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
    }
    request.header_fields = (0 != header_index) ? ((const char **)header_fields) : NULL;

    /* Open HTTP client connection */
    if (MENDER_OK != (ret = mender_http_connect(host, port, &request_context.sock))) {
        mender_log_error("Unable to open HTTP client connection");
        goto END;
    }
    if (MENDER_OK != (ret = callback(MENDER_HTTP_EVENT_CONNECTED, NULL, 0, params))) {
        mender_log_error("An error occurred");
        goto END;
    }

    /* Perform HTTP request */
    if (http_client_req(request_context.sock, &request, MENDER_HTTP_REQUEST_TIMEOUT, (void *)&request_context) < 0) {
        mender_log_error("Unable to write data");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Read HTTP status code */
    if (0 == request.internal.response.http_status_code) {
        mender_log_error("An error occurred, connection has been closed");
        callback(MENDER_HTTP_EVENT_ERROR, NULL, 0, params);
        ret = MENDER_FAIL;
        goto END;
    } else {
        *status = request.internal.response.http_status_code;
    }
    if (MENDER_OK != (ret = callback(MENDER_HTTP_EVENT_DISCONNECTED, NULL, 0, params))) {
        mender_log_error("An error occurred");
        goto END;
    }

END:

    /* Close socket */
    if (request_context.sock >= 0) {
        close(request_context.sock);
    }

    /* Release memory */
    if (NULL != host) {
        free(host);
    }
    if (NULL != port) {
        free(port);
    }
    if (NULL != url) {
        free(url);
    }
    if (NULL != request.recv_buf) {
        free(request.recv_buf);
    }
    for (size_t index = 0; index < sizeof(header_fields) / sizeof(header_fields[0]); index++) {
        if (NULL != header_fields[index]) {
            free(header_fields[index]);
        }
    }

    return ret;
}

mender_err_t
mender_http_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}

static mender_err_t
mender_http_connect(const char *host, const char *port, int *sock) {

    assert(NULL != host);
    assert(NULL != port);
    assert(NULL != sock);
    int              result;
    mender_err_t     ret = MENDER_OK;
    struct addrinfo  hints;
    struct addrinfo *addr             = NULL;
    int              resolve_attempts = 10;
    int              connect_attempts = 10;
    unsigned long    last_wake_time   = 0;

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
    if (MENDER_OK != (ret = mender_rtos_delay_until_init(&last_wake_time))) {
        mender_log_error("Unable to initialize delay");
        goto END;
    }
    while (resolve_attempts--) {
        if (0 == (result = getaddrinfo(host, port, &hints, &addr))) {
            break;
        }
        mender_log_debug("Unable to resolve host name '%s:%s', result = %d", host, port, result);
        /* Wait before trying again */
        mender_rtos_delay_until_s(&last_wake_time, MENDER_HTTP_DNS_RESOLVE_INTERVAL);
    }
    if (0 != result) {
        mender_log_error("Unable to resolve host name '%s:%s', result = %d", host, port, result);
        ret = MENDER_FAIL;
        goto END;
    }

    /* Create socket */
#ifdef CONFIG_NET_SOCKETS_SOCKOPT_TLS
    if ((result = socket(addr->ai_family, SOCK_STREAM, IPPROTO_TLS_1_2)) < 0) {
#else
    if ((result = socket(addr->ai_family, SOCK_STREAM, IPPROTO_TCP)) < 0) {
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */
        mender_log_error("Unable to create socket, result = %d", result);
        ret = MENDER_FAIL;
        goto END;
    }
    *sock = result;

#ifdef CONFIG_NET_SOCKETS_SOCKOPT_TLS

    /* Set TLS_SEC_TAG_LIST option */
    sec_tag_t sec_tag[] = {
        CONFIG_MENDER_HTTP_CA_CERTIFICATE_TAG,
    };
    if ((result = setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag, sizeof(sec_tag))) < 0) {
        mender_log_error("Unable to set TLS_SEC_TAG_LIST option, result = %d", result);
        close(*sock);
        *sock = -1;
        ret   = MENDER_FAIL;
        goto END;
    }

    /* Set SOL_TLS option */
    if ((result = setsockopt(*sock, SOL_TLS, TLS_HOSTNAME, host, strlen(host))) < 0) {
        mender_log_error("Unable to set TLS_HOSTNAME option, result = %d", result);
        close(*sock);
        *sock = -1;
        ret   = MENDER_FAIL;
        goto END;
    }

#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */

    /* Connect to the host */
    if (MENDER_OK != (ret = mender_rtos_delay_until_init(&last_wake_time))) {
        mender_log_error("Unable to initialize delay");
        goto END;
    }
    while (connect_attempts--) {
        if (0 == (result = connect(*sock, addr->ai_addr, addr->ai_addrlen))) {
            break;
        }
        mender_log_debug("Unable to connect to the host '%s:%s', result = %d", host, port, result);
        /* Wait before trying again */
        mender_rtos_delay_until_s(&last_wake_time, MENDER_HTTP_CONNECT_INTERVAL);
    }
    if (0 != result) {
        mender_log_error("Unable to connect to the host '%s:%s', result = %d", host, port, result);
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

static void
mender_http_response_cb(struct http_response *response, enum http_final_call final_call, void *user_data) {

    assert(NULL != response);
    (void)final_call;
    assert(NULL != user_data);

    /* Check if data is available */
    if ((true == response->body_found) && (NULL != response->body_frag_start) && (0 != response->body_frag_len)) {

        /* Transmit data received to the upper layer */
        if (MENDER_OK
            != ((mender_http_request_context *)user_data)
                   ->callback(MENDER_HTTP_EVENT_DATA_RECEIVED,
                              (void *)response->body_frag_start,
                              response->body_frag_len,
                              ((mender_http_request_context *)user_data)->params)) {
            mender_log_error("An error occurred, stop reading data");
            close(((mender_http_request_context *)user_data)->sock);
            ((mender_http_request_context *)user_data)->sock = -1;
        }
    }
}

static mender_err_t
mender_http_get_host_port_url(char *path, char **host, char **port, char **url) {

    assert(NULL != path);
    assert(NULL != host);
    assert(NULL != port);
    char *tmp;
    char *saveptr;

    /* Check if the path start with protocol */
    if ((strncmp(path, "http://", strlen("http://"))) && (strncmp(path, "https://", strlen("https://")))) {

        /* Path contain the URL only, retrieve host and port from configuration */
        assert(NULL != url);
        if (NULL == (*url = strdup(path))) {
            mender_log_error("Unable to allocate memory");
            return MENDER_FAIL;
        }
        return mender_http_get_host_port_url(mender_http_config.host, host, port, NULL);
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
        if (!strncmp(protocol, "https", strlen("https"))) {
            *port = strdup("443");
        } else {
            *port = strdup("80");
        }
    }
    if (NULL != url) {
        *url = strdup(path + strlen(protocol) + 2 + strlen(pch1));
    }

    /* Release memory */
    free(tmp);

    return MENDER_OK;
}

static enum http_method
mender_http_method_to_zephyr_http_client_method(mender_http_method_t method) {

    /* Convert method */
    if (MENDER_HTTP_GET == method) {
        return HTTP_GET;
    } else if (MENDER_HTTP_POST == method) {
        return HTTP_POST;
    } else if (MENDER_HTTP_PUT == method) {
        return HTTP_PUT;
    } else if (MENDER_HTTP_PATCH == method) {
        return HTTP_PATCH;
    }

    return -1;
}
