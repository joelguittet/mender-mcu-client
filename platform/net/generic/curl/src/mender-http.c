/**
 * @file      mender-http.c
 * @brief     Mender HTTP interface for curl platform, requires libcurl >= 7.80.0
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

#include <curl/curl.h>
#include "mender-http.h"
#include "mender-log.h"
#include "mender-utils.h"

/**
 * @brief HTTP User-Agent
 */
#ifdef MENDER_CLIENT_VERSION
#define MENDER_HTTP_USER_AGENT "mender-mcu-client/" MENDER_CLIENT_VERSION " (mender-http) curl/" LIBCURL_VERSION
#else
#define MENDER_HTTP_USER_AGENT "mender-mcu-client (mender-http) curl/" LIBCURL_VERSION
#endif /* MENDER_CLIENT_VERSION */

/**
 * @brief User data
 */
typedef struct {
    mender_err_t (*callback)(mender_http_client_event_t, void *, size_t, void *); /**< Callback invoked on HTTP events */
    void *params;                                                                 /**< Parameters passed to the callback, NULL if not used */
} mender_http_curl_user_data_t;

/**
 * @brief Mender HTTP configuration
 */
static mender_http_config_t mender_http_config;

/**
 * @brief HTTP PREREQ callback, used to inform the client is connected to the server
 * @param params User data
 * @param conn_primary_ip Primary IP of the remote server 
 * @param conn_local_ip Originating IP of the connection
 * @param conn_primary_port Primary port number on the remote server
 * @param conn_local_port Originating port number of the connection
 * @return CURL_PREREQFUNC_OK if the function succeeds, CURL_PREREQFUNC_ABORT otherwise
 */
static int mender_http_prereq_callback(void *params, char *conn_primary_ip, char *conn_local_ip, int conn_primary_port, int conn_local_port);

/**
 * @brief HTTP write callback, used to retrieve data from the server
 * @param data Data from the server
 * @param size Size of the data
 * @param nmemb Number of element
 * @param params User data
 * @return Real size of data if the function succeeds, -1 otherwise
 */
static size_t mender_http_write_callback(char *data, size_t size, size_t nmemb, void *params);

mender_err_t
mender_http_init(mender_http_config_t *config) {

    assert(NULL != config);
    assert(NULL != config->host);

    /* Save configuration */
    memcpy(&mender_http_config, config, sizeof(mender_http_config_t));

    /* Initialization of curl */
    curl_global_init(CURL_GLOBAL_DEFAULT);

    return MENDER_OK;
}

mender_err_t
mender_http_perform(char                *jwt,
                    char                *path,
                    mender_http_method_t method,
                    char                *payload,
                    char                *signature,
                    mender_err_t (*callback)(mender_http_client_event_t, void *, size_t, void *),
                    void *params,
                    int  *status) {

    assert(NULL != path);
    assert(NULL != callback);
    assert(NULL != status);
    CURLcode           err;
    mender_err_t       ret             = MENDER_OK;
    CURL              *curl            = NULL;
    char              *url             = NULL;
    char              *bearer          = NULL;
    char              *x_men_signature = NULL;
    struct curl_slist *headers         = NULL;

    /* Compute URL if required */
    if ((false == mender_utils_strbeginwith(path, "http://")) && (false == mender_utils_strbeginwith(path, "https://"))) {
        size_t str_length = strlen(mender_http_config.host) + strlen(path) + 1;
        if (NULL == (url = (char *)malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        snprintf(url, str_length, "%s%s", mender_http_config.host, path);
    }

    /* Initialization of the client */
    if (NULL == (curl = curl_easy_init())) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Configuration of the client */
    if (CURLE_OK != (err = curl_easy_setopt(curl, CURLOPT_URL, (NULL != url) ? url : path))) {
        mender_log_error("Unable to set HTTP URL: %s", curl_easy_strerror(err));
        ret = MENDER_FAIL;
        goto END;
    }
    if (CURLE_OK != (err = curl_easy_setopt(curl, CURLOPT_USERAGENT, MENDER_HTTP_USER_AGENT))) {
        mender_log_error("Unable to set HTTP User-Agent: %s", curl_easy_strerror(err));
        ret = MENDER_FAIL;
        goto END;
    }
    if (CURLE_OK != (err = curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2))) {
        mender_log_error("Unable to set TLSv1.2: %s", curl_easy_strerror(err));
        ret = MENDER_FAIL;
        goto END;
    }
    mender_http_curl_user_data_t user_data = { .callback = callback, .params = params };
    if (CURLE_OK != (err = curl_easy_setopt(curl, CURLOPT_PREREQFUNCTION, &mender_http_prereq_callback))) {
        mender_log_error("Unable to set HTTP PREREQ function: %s", curl_easy_strerror(err));
        ret = MENDER_FAIL;
        goto END;
    }
    if (CURLE_OK != (err = curl_easy_setopt(curl, CURLOPT_PREREQDATA, &user_data))) {
        mender_log_error("Unable to set HTTP PREREQ data: %s", curl_easy_strerror(err));
        ret = MENDER_FAIL;
        goto END;
    }
    if (CURLE_OK != (err = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &mender_http_write_callback))) {
        mender_log_error("Unable to set HTTP write function: %s", curl_easy_strerror(err));
        ret = MENDER_FAIL;
        goto END;
    }
    if (CURLE_OK != (err = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &user_data))) {
        mender_log_error("Unable to set HTTP write data: %s", curl_easy_strerror(err));
        ret = MENDER_FAIL;
        goto END;
    }
    if (NULL != jwt) {
        size_t str_length = strlen("Authorization: Bearer ") + strlen(jwt) + 1;
        if (NULL == (bearer = (char *)malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        snprintf(bearer, str_length, "Authorization: Bearer %s", jwt);
        headers = curl_slist_append(headers, bearer);
    }
    if (NULL != signature) {
        size_t str_length = strlen("X-MEN-Signature: ") + strlen(signature) + 1;
        if (NULL == (x_men_signature = (char *)malloc(str_length))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        snprintf(x_men_signature, str_length, "X-MEN-Signature: %s", signature);
        headers = curl_slist_append(headers, x_men_signature);
    }
    if (NULL != payload) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }
    if (NULL != headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    /* Write data if payload is defined */
    if (NULL != payload) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        if (MENDER_HTTP_PUT == method) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        }
    }

    /* Perform request */
    if (CURLE_OK != (err = curl_easy_perform(curl))) {
        mender_log_error("Unable to perform HTTP request: %s", curl_easy_strerror(err));
        callback(MENDER_HTTP_EVENT_ERROR, NULL, 0, params);
        ret = MENDER_FAIL;
        goto END;
    }

    /* Read HTTP status code */
    long response_code;
    if (CURLE_OK != (err = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code))) {
        mender_log_error("Unable to read HTTP response code: %s", curl_easy_strerror(err));
        ret = MENDER_FAIL;
        goto END;
    }
    *status = (int)response_code;
    if (MENDER_OK != (ret = callback(MENDER_HTTP_EVENT_DISCONNECTED, NULL, 0, params))) {
        mender_log_error("An error occurred");
        goto END;
    }

END:

    /* Release memory */
    if (NULL != curl) {
        curl_easy_cleanup(curl);
    }
    if (NULL != headers) {
        curl_slist_free_all(headers);
    }
    if (NULL != x_men_signature) {
        free(x_men_signature);
    }
    if (NULL != bearer) {
        free(bearer);
    }
    if (NULL != url) {
        free(url);
    }

    return ret;
}

mender_err_t
mender_http_exit(void) {

    /* Cleaning */
    curl_global_cleanup();

    return MENDER_OK;
}

static int
mender_http_prereq_callback(void *params, char *conn_primary_ip, char *conn_local_ip, int conn_primary_port, int conn_local_port) {

    assert(NULL != params);
    mender_http_curl_user_data_t *user_data = (mender_http_curl_user_data_t *)params;
    (void)conn_primary_ip;
    (void)conn_local_ip;
    (void)conn_primary_port;
    (void)conn_local_port;

    /* Invoke callback */
    if (MENDER_OK != user_data->callback(MENDER_HTTP_EVENT_CONNECTED, NULL, 0, user_data->params)) {
        mender_log_error("An error occurred");
        return CURL_PREREQFUNC_ABORT;
    }

    return CURL_PREREQFUNC_OK;
}

static size_t
mender_http_write_callback(char *data, size_t size, size_t nmemb, void *params) {

    assert(NULL != params);
    mender_http_curl_user_data_t *user_data = (mender_http_curl_user_data_t *)params;
    size_t                        realsize  = size * nmemb;

    /* Transmit data received to the upper layer */
    if (realsize > 0) {
        if (MENDER_OK != user_data->callback(MENDER_HTTP_EVENT_DATA_RECEIVED, (void *)data, realsize, user_data->params)) {
            mender_log_error("An error occurred, stop reading data");
            return -1;
        }
    }

    return realsize;
}
