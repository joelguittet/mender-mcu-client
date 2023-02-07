#ifndef __FLASH_H__
#define __FLASH_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <zephyr/device.h>

struct flash_pages_info {
    size_t   size;
    uint32_t index;
};

int flash_get_page_info_by_offs(const struct device *dev, off_t offset, struct flash_pages_info *info);

#endif /* __FLASH_H__ */
