#include <zephyr/sys/ring_buffer.h>

void
ring_buf_init(struct ring_buf *buf, uint32_t size, uint8_t *data) {
}

bool
ring_buf_is_empty(struct ring_buf *buf) {
    return false;
}

void
ring_buf_reset(struct ring_buf *buf) {
}

uint32_t
ring_buf_size_get(struct ring_buf *buf) {
    return 0;
}

uint32_t
ring_buf_put(struct ring_buf *buf, const uint8_t *data, uint32_t size) {
    return 0;
}

uint32_t
ring_buf_get(struct ring_buf *buf, uint8_t *data, uint32_t size) {
    return 0;
}
