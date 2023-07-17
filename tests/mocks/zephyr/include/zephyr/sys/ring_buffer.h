#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <stdbool.h>
#include <stdint.h>

struct ring_buf {
    void *dummy;
};

void     ring_buf_init(struct ring_buf *buf, uint32_t size, uint8_t *data);
bool     ring_buf_is_empty(struct ring_buf *buf);
void     ring_buf_reset(struct ring_buf *buf);
uint32_t ring_buf_size_get(struct ring_buf *buf);
uint32_t ring_buf_put(struct ring_buf *buf, const uint8_t *data, uint32_t size);
uint32_t ring_buf_get(struct ring_buf *buf, uint8_t *data, uint32_t size);

#endif /* __RING_BUFFER_H__ */
