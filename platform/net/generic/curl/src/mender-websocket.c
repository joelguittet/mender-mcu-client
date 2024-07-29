/**
 * @file      mender-websocket.c
 * @brief     Mender websocket interface for curl platform, requires libcurl >= 7.86.0
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

#include <curl/curl.h>
#include <pthread.h>
#include "mender-log.h"
#include "mender-utils.h"
#include "mender-websocket.h"

/**
 * @brief Default websocket thread stack size (kB)
 */
#ifndef CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE
#define CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE (64)
#endif /* CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE */

/**
 * @brief Default websocket thread priority
 */
#ifndef CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY
#define CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY (0)
#endif /* CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY */

/**
 * @brief WebSocket User-Agent
 */
#ifdef MENDER_CLIENT_VERSION
#define MENDER_WEBSOCKET_USER_AGENT "mender-mcu-client/" MENDER_CLIENT_VERSION " (mender-websocket) curl/" LIBCURL_VERSION
#else
#define MENDER_WEBSOCKET_USER_AGENT "mender-mcu-client (mender-websocket) curl/" LIBCURL_VERSION
#endif /* MENDER_CLIENT_VERSION */

/**
 * @brief Websocket handle
 */
typedef struct {
    CURL              *client;        /**< Websocket client handle */
    struct curl_slist *headers;       /**< Websocket client headers */
    pthread_t          thread_handle; /**< Websocket thread handle */
    bool               abort;         /**< Flag used to indicate connection should be terminated */
    mender_err_t (*callback)(mender_websocket_client_event_t,
                             void *,
                             size_t,
                             void *); /**< Callback function to be invoked to perform the treatment of the events and data from the websocket */
    void *params;                     /**< Callback function parameters */
} mender_websocket_handle_t;

/**
 * @brief Mender websocket configuration
 */
static mender_websocket_config_t mender_websocket_config;

/**
 * @brief Websocket PREREQ callback, used to inform the client is connected to the server
 * @param params User data
 * @param conn_primary_ip Primary IP of the remote server 
 * @param conn_local_ip Originating IP of the connection
 * @param conn_primary_port Primary port number on the remote server
 * @param conn_local_port Originating port number of the connection
 * @return CURL_PREREQFUNC_OK if the function succeeds, CURL_PREREQFUNC_ABORT otherwise
 */
static int mender_websocket_prereq_callback(void *params, char *conn_primary_ip, char *conn_local_ip, int conn_primary_port, int conn_local_port);

/**
 * @brief Websocket write callback, used to retrieve data from the server
 * @param data Data from the server
 * @param size Size of the data
 * @param nmemb Number of element
 * @param params User data
 * @return Real size of data if the function succeeds, -1 otherwise
 */
static size_t mender_websocket_write_callback(char *data, size_t size, size_t nmemb, void *params);

/**
 * @brief Thread used to perform connection and reception of data
 * @param arg Websocket handle
 * @return Not used
 */
static void *mender_websocket_thread(void *arg);

mender_err_t
mender_websocket_init(mender_websocket_config_t *config) {

    assert(NULL != config);
    assert(NULL != config->host);

    /* Save configuration */
    memcpy(&mender_websocket_config, config, sizeof(mender_websocket_config_t));

    /* Initialization of curl */
    curl_global_init(CURL_GLOBAL_DEFAULT);

    return MENDER_OK;
}

mender_err_t
mender_websocket_connect(
    char *jwt, char *path, mender_err_t (*callback)(mender_websocket_client_event_t, void *, size_t, void *), void *params, void **handle) {

    assert(NULL != path);
    assert(NULL != callback);
    assert(NULL != handle);
    CURLcode     err_curl;
    int          err_pthread;
    mender_err_t ret    = MENDER_OK;
    char        *url    = NULL;
    char        *bearer = NULL;

    /* Allocate a new handle */
    if (NULL == (*handle = malloc(sizeof(mender_websocket_handle_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }
    memset(*handle, 0, sizeof(mender_websocket_handle_t));

    /* Save callback and params */
    ((mender_websocket_handle_t *)*handle)->callback = callback;
    ((mender_websocket_handle_t *)*handle)->params   = params;

    /* Compute URL if required */
    if ((false == mender_utils_strbeginwith(path, "ws://")) && (false == mender_utils_strbeginwith(path, "wss://"))) {
        if ((true == mender_utils_strbeginwith(path, "http://")) || (true == mender_utils_strbeginwith(mender_websocket_config.host, "http://"))) {
            size_t str_length = strlen(mender_websocket_config.host) - strlen("http://") + strlen("ws://") + strlen(path) + 1;
            if (NULL == (url = (char *)malloc(str_length))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto FAIL;
            }
            snprintf(url, str_length, "ws://%s%s", mender_websocket_config.host + strlen("http://"), path);
        } else if ((true == mender_utils_strbeginwith(path, "https://")) || (true == mender_utils_strbeginwith(mender_websocket_config.host, "https://"))) {
            size_t str_length = strlen(mender_websocket_config.host) - strlen("https://") + strlen("wss://") + strlen(path) + 1;
            if (NULL == (url = (char *)malloc(str_length))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto FAIL;
            }
            snprintf(url, str_length, "wss://%s%s", mender_websocket_config.host + strlen("https://"), path);
        }
    }

    /* Initialization of the client */
    if (NULL == (((mender_websocket_handle_t *)*handle)->client = curl_easy_init())) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto FAIL;
    }

    /* Configuration of the client */
    if (CURLE_OK != (err_curl = curl_easy_setopt(((mender_websocket_handle_t *)*handle)->client, CURLOPT_URL, (NULL != url) ? url : path))) {
        mender_log_error("Unable to set websocket URL: %s", curl_easy_strerror(err_curl));
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (CURLE_OK != (err_curl = curl_easy_setopt(((mender_websocket_handle_t *)*handle)->client, CURLOPT_USERAGENT, MENDER_WEBSOCKET_USER_AGENT))) {
        mender_log_error("Unable to set HTTP User-Agent: %s", curl_easy_strerror(err_curl));
        ret = MENDER_FAIL;
        goto END;
    }
    if (CURLE_OK != (err_curl = curl_easy_setopt(((mender_websocket_handle_t *)*handle)->client, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2))) {
        mender_log_error("Unable to set TLSv1.2: %s", curl_easy_strerror(err_curl));
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (CURLE_OK != (err_curl = curl_easy_setopt(((mender_websocket_handle_t *)*handle)->client, CURLOPT_PREREQFUNCTION, &mender_websocket_prereq_callback))) {
        mender_log_error("Unable to set websocket PREREQ function: %s", curl_easy_strerror(err_curl));
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (CURLE_OK != (err_curl = curl_easy_setopt(((mender_websocket_handle_t *)*handle)->client, CURLOPT_PREREQDATA, *handle))) {
        mender_log_error("Unable to set websocket PREREQ data: %s", curl_easy_strerror(err_curl));
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (CURLE_OK != (err_curl = curl_easy_setopt(((mender_websocket_handle_t *)*handle)->client, CURLOPT_WRITEFUNCTION, &mender_websocket_write_callback))) {
        mender_log_error("Unable to set websocket write function: %s", curl_easy_strerror(err_curl));
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (CURLE_OK != (err_curl = curl_easy_setopt(((mender_websocket_handle_t *)*handle)->client, CURLOPT_WRITEDATA, *handle))) {
        mender_log_error("Unable to set websocket write data: %s", curl_easy_strerror(err_curl));
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (NULL != jwt) {
        size_t str_length = strlen("Authorization: Bearer ") + strlen(jwt) + 1;
        if (NULL == (bearer = (char *)malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto FAIL;
        }
        snprintf(bearer, str_length, "Authorization: Bearer %s", jwt);
        ((mender_websocket_handle_t *)*handle)->headers = curl_slist_append(((mender_websocket_handle_t *)*handle)->headers, bearer);
    }
    if (NULL != ((mender_websocket_handle_t *)*handle)->headers) {
        if (CURLE_OK
            != (err_curl
                = curl_easy_setopt(((mender_websocket_handle_t *)*handle)->client, CURLOPT_HTTPHEADER, ((mender_websocket_handle_t *)*handle)->headers))) {
            mender_log_error("Unable to set websocket HTTP headers: %s", curl_easy_strerror(err_curl));
            ret = MENDER_FAIL;
            goto FAIL;
        }
    }

    /* Create and start websocket thread */
    pthread_attr_t pthread_attr;
    if (0 != (err_pthread = pthread_attr_init(&pthread_attr))) {
        mender_log_error("Unable to initialize websocket thread attributes (ret=%d)", err_pthread);
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (0
        != (err_pthread = pthread_attr_setstacksize(
                &pthread_attr, ((CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE > 16) ? CONFIG_MENDER_WEBSOCKET_THREAD_STACK_SIZE : 16) * 1024))) {
        mender_log_error("Unable to set websocket thread stack size (ret=%d)", err_pthread);
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (0 != (err_pthread = pthread_create(&((mender_websocket_handle_t *)*handle)->thread_handle, &pthread_attr, mender_websocket_thread, *handle))) {
        mender_log_error("Unable to create websocket thread (ret=%d)", err_pthread);
        ret = MENDER_FAIL;
        goto FAIL;
    }
    if (0 != (err_pthread = pthread_setschedprio(((mender_websocket_handle_t *)*handle)->thread_handle, CONFIG_MENDER_WEBSOCKET_THREAD_PRIORITY))) {
        mender_log_error("Unable to set websocket thread priority (ret=%d)", err_pthread);
        ret = MENDER_FAIL;
        goto FAIL;
    }

    goto END;

FAIL:

    /* Release memory */
    if (NULL != *handle) {
        curl_easy_cleanup(((mender_websocket_handle_t *)*handle)->client);
        curl_slist_free_all(((mender_websocket_handle_t *)handle)->headers);
        free(*handle);
        *handle = NULL;
    }

END:

    /* Release memory */
    if (NULL != bearer) {
        free(bearer);
    }
    if (NULL != url) {
        free(url);
    }

    return ret;
}

mender_err_t
mender_websocket_send(void *handle, void *payload, size_t length) {

    assert(NULL != handle);
    assert(NULL != payload);
    CURLcode err;
    size_t   sent = 0;

    /* Send binary payload */
    if (CURLE_OK != (err = curl_ws_send(((mender_websocket_handle_t *)handle)->client, payload, length, &sent, 0, CURLWS_BINARY))) {
        mender_log_error("Unable to send data over websocket connection: %s", curl_easy_strerror(err));
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_websocket_disconnect(void *handle) {

    assert(NULL != handle);
    CURLcode err;
    size_t   sent = 0;

    /* Close websocket connection */
    ((mender_websocket_handle_t *)handle)->abort = true;
    if (CURLE_OK != (err = curl_ws_send(((mender_websocket_handle_t *)handle)->client, NULL, 0, &sent, 0, CURLWS_CLOSE))) {
        mender_log_error("Unable to send close payload: %s", curl_easy_strerror(err));
        return MENDER_FAIL;
    }

    /* Wait end of execution of the websocket thread */
    pthread_join(((mender_websocket_handle_t *)handle)->thread_handle, NULL);

    /* Release memory */
    curl_easy_cleanup(((mender_websocket_handle_t *)handle)->client);
    curl_slist_free_all(((mender_websocket_handle_t *)handle)->headers);
    free(handle);

    return MENDER_OK;
}

mender_err_t
mender_websocket_exit(void) {

    /* Cleaning */
    curl_global_cleanup();

    return MENDER_OK;
}

static int
mender_websocket_prereq_callback(void *params, char *conn_primary_ip, char *conn_local_ip, int conn_primary_port, int conn_local_port) {

    assert(NULL != params);
    mender_websocket_handle_t *handle = (mender_websocket_handle_t *)params;
    (void)conn_primary_ip;
    (void)conn_local_ip;
    (void)conn_primary_port;
    (void)conn_local_port;

    /* Invoke connected callback */
    if (MENDER_OK != handle->callback(MENDER_WEBSOCKET_EVENT_CONNECTED, NULL, 0, handle->params)) {
        mender_log_error("An error occurred");
        return CURL_PREREQFUNC_ABORT;
    }

    return CURL_PREREQFUNC_OK;
}

static size_t
mender_websocket_write_callback(char *data, size_t size, size_t nmemb, void *params) {

    assert(NULL != params);
    mender_websocket_handle_t *handle   = (mender_websocket_handle_t *)params;
    size_t                     realsize = size * nmemb;

    /* Transmit data received to the upper layer */
    if (realsize > 0) {
        if (MENDER_OK != handle->callback(MENDER_WEBSOCKET_EVENT_DATA_RECEIVED, (void *)data, realsize, handle->params)) {
            mender_log_error("An error occurred, stop reading data");
            return -1;
        }
    }

    return realsize;
}

__attribute__((noreturn)) static void *
mender_websocket_thread(void *arg) {

    assert(NULL != arg);
    mender_websocket_handle_t *handle = (mender_websocket_handle_t *)arg;
    CURLcode                   err;

    /* Perform reception of data from the websocket connection */
    while (false == handle->abort) {
        err = curl_easy_perform(handle->client);
        if (CURLE_OK != err) {
            if (CURLE_HTTP_RETURNED_ERROR == err) {
                mender_log_error("Connection has been closed");
                goto END;
            }
            mender_log_error("Unable to perform websocket request: %s", curl_easy_strerror(err));
        }
    }

END:

    /* Invoke disconnected callback */
    handle->callback(MENDER_WEBSOCKET_EVENT_DISCONNECTED, NULL, 0, handle->params);

    /* Terminate work queue thread */
    pthread_exit(NULL);
}
