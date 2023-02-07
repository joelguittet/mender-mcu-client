#ifndef __NVS_H__
#define __NVS_H__

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>

struct nvs_fs {
    off_t                offset;
    uint16_t             sector_size;
    uint16_t             sector_count;
    const struct device *flash_device;
};

int     nvs_mount(struct nvs_fs *fs);
ssize_t nvs_write(struct nvs_fs *fs, uint16_t id, const void *data, size_t len);
ssize_t nvs_read(struct nvs_fs *fs, uint16_t id, void *data, size_t len);
int     nvs_delete(struct nvs_fs *fs, uint16_t id);

#endif /* __NVS_H__ */
