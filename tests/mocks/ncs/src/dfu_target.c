#include <dfu/dfu_target.h>

int dfu_target_init(int img_type, int img_num, size_t file_size, dfu_target_callback_t cb) {
    return 0;
}

int dfu_target_write(const void *const buf, size_t len) {
    return 0;
}

int dfu_target_done(bool successful) {
    return 0;
}

int dfu_target_reset(void) {
}

int dfu_target_schedule_update(int img_num) {
}

