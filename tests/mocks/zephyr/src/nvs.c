#include <zephyr/fs/nvs.h>

int
nvs_mount(struct nvs_fs *fs) {
    return 0;
}

ssize_t
nvs_write(struct nvs_fs *fs, uint16_t id, const void *data, size_t len) {
    return 0;
}

ssize_t
nvs_read(struct nvs_fs *fs, uint16_t id, void *data, size_t len) {
    return 0;
}

int
nvs_delete(struct nvs_fs *fs, uint16_t id) {
    return 0;
}
