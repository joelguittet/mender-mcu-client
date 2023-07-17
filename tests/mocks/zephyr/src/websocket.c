#include <zephyr/net/websocket.h>

void
websocket_init(void) {
}

int
websocket_connect(int http_sock, struct websocket_request *req, int32_t timeout, void *user_data) {
    return 0;
}

int
websocket_send_msg(int ws_sock, const uint8_t *payload, size_t payload_len, enum websocket_opcode opcode, bool mask, bool final, int32_t timeout) {
    return 0;
}

int
websocket_recv_msg(int ws_sock, uint8_t *buf, size_t buf_len, uint32_t *message_type, uint64_t *remaining, int32_t timeout) {
    return 0;
}

int
websocket_disconnect(int ws_sock) {
    return 0;
}
