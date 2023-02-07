#include <zephyr/dfu/flash_img.h>

int
flash_img_init(struct flash_img_context *ctx) {
    return 0;
}

int
flash_img_buffered_write(struct flash_img_context *ctx, const uint8_t *data, size_t len, bool flush) {
    return 0;
}
