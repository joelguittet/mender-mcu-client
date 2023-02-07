#ifndef __FLASH_IMG_H__
#define __FLASH_IMG_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct flash_img_context {
    void *dummy;
};

int flash_img_init(struct flash_img_context *ctx);
int flash_img_buffered_write(struct flash_img_context *ctx, const uint8_t *data, size_t len, bool flush);

#endif /* __FLASH_IMG_H__ */
