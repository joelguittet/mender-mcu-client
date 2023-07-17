#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/net/http/client.h>

#define WEBSOCKET_FLAG_FINAL  0x00000001
#define WEBSOCKET_FLAG_TEXT   0x00000002
#define WEBSOCKET_FLAG_BINARY 0x00000004
#define WEBSOCKET_FLAG_CLOSE  0x00000008
#define WEBSOCKET_FLAG_PING   0x00000010
#define WEBSOCKET_FLAG_PONG   0x00000020

enum websocket_opcode {
    WEBSOCKET_OPCODE_CONTINUE    = 0x00,
    WEBSOCKET_OPCODE_DATA_TEXT   = 0x01,
    WEBSOCKET_OPCODE_DATA_BINARY = 0x02,
    WEBSOCKET_OPCODE_CLOSE       = 0x08,
    WEBSOCKET_OPCODE_PING        = 0x09,
    WEBSOCKET_OPCODE_PONG        = 0x0A,
};

typedef int (*websocket_connect_cb_t)(int ws_sock, struct http_request *req, void *user_data);

struct websocket_request {
    const char *                       host;
    const char *                       url;
    http_header_cb_t                   optional_headers_cb;
    const char **                      optional_headers;
    websocket_connect_cb_t             cb;
    const struct http_parser_settings *http_cb;
    uint8_t *                          tmp_buf;
    size_t                             tmp_buf_len;
};

void websocket_init(void);
int  websocket_connect(int http_sock, struct websocket_request *req, int32_t timeout, void *user_data);
int  websocket_send_msg(int ws_sock, const uint8_t *payload, size_t payload_len, enum websocket_opcode opcode, bool mask, bool final, int32_t timeout);
int  websocket_recv_msg(int ws_sock, uint8_t *buf, size_t buf_len, uint32_t *message_type, uint64_t *remaining, int32_t timeout);
int  websocket_disconnect(int ws_sock);

#endif /* __WEBSOCKET_H__ */
