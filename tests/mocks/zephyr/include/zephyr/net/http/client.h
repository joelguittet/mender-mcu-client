#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <stdint.h>
#include <stddef.h>
#include <version.h>

enum http_method {
    HTTP_DELETE      = 0,  /**< DELETE */
    HTTP_GET         = 1,  /**< GET */
    HTTP_HEAD        = 2,  /**< HEAD */
    HTTP_POST        = 3,  /**< POST */
    HTTP_PUT         = 4,  /**< PUT */
    HTTP_CONNECT     = 5,  /**< CONNECT */
    HTTP_OPTIONS     = 6,  /**< OPTIONS */
    HTTP_TRACE       = 7,  /**< TRACE */
    HTTP_COPY        = 8,  /**< COPY */
    HTTP_LOCK        = 9,  /**< LOCK */
    HTTP_MKCOL       = 10, /**< MKCOL */
    HTTP_MOVE        = 11, /**< MOVE */
    HTTP_PROPFIND    = 12, /**< PROPFIND */
    HTTP_PROPPATCH   = 13, /**< PROPPATCH */
    HTTP_SEARCH      = 14, /**< SEARCH */
    HTTP_UNLOCK      = 15, /**< UNLOCK */
    HTTP_BIND        = 16, /**< BIND */
    HTTP_REBIND      = 17, /**< REBIND */
    HTTP_UNBIND      = 18, /**< UNBIND */
    HTTP_ACL         = 19, /**< ACL */
    HTTP_REPORT      = 20, /**< REPORT */
    HTTP_MKACTIVITY  = 21, /**< MKACTIVITY */
    HTTP_CHECKOUT    = 22, /**< CHECKOUT */
    HTTP_MERGE       = 23, /**< MERGE */
    HTTP_MSEARCH     = 24, /**< MSEARCH */
    HTTP_NOTIFY      = 25, /**< NOTIFY */
    HTTP_SUBSCRIBE   = 26, /**< SUBSCRIBE */
    HTTP_UNSUBSCRIBE = 27, /**< UNSUBSCRIBE */
    HTTP_PATCH       = 28, /**< PATCH */
    HTTP_PURGE       = 29, /**< PURGE */
    HTTP_MKCALENDAR  = 30, /**< MKCALENDAR */
    HTTP_LINK        = 31, /**< LINK */
    HTTP_UNLINK      = 32, /**< UNLINK */
};

enum http_final_call {
    HTTP_DATA_MORE  = 0,
    HTTP_DATA_FINAL = 1,
};

struct http_response {
    uint8_t *body_frag_start;
    size_t   body_frag_len;
    uint16_t http_status_code;
    uint8_t  body_found : 1;
};

struct http_client_internal_data {
    struct http_response response;
};

#if (ZEPHYR_VERSION_CODE > ZEPHYR_VERSION(4, 1, 0))
typedef int (*http_response_cb_t)(struct http_response *rsp, enum http_final_call final_data, void *user_data);
#else
typedef void (*http_response_cb_t)(struct http_response *rsp, enum http_final_call final_data, void *user_data);
#endif /* (ZEPHYR_VERSION_CODE > ZEPHYR_VERSION(4, 1, 0)) */

struct http_request {
    struct http_client_internal_data internal;
    enum http_method                 method;
    http_response_cb_t               response;
    uint8_t *                        recv_buf;
    size_t                           recv_buf_len;
    const char *                     url;
    const char *                     protocol;
    const char **                    header_fields;
    const char *                     host;
    const char *                     port;
    const char *                     payload;
    size_t                           payload_len;
};

typedef int (*http_header_cb_t)(int sock, struct http_request *req, void *user_data);

int http_client_req(int sock, struct http_request *req, int32_t timeout, void *user_data);

#endif /* __CLIENT_H__ */
