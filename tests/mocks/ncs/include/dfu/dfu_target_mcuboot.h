#ifndef __DFU_TARGET_MCUBOOT_H__
#define __DFU_TARGET_MCUBOOT_H__

#include <stddef.h>
#include <stdint.h>

int dfu_target_mcuboot_set_buf(uint8_t *buf, size_t len);

#endif /* __DFU_TARGET_MCUBOOT_H__ */
