#ifndef __DFU_TARGET_H__
#define __DFU_TARGET_H__

#include <stdbool.h>
#include <stddef.h>

enum dfu_target_image_type {
	/** Not a DFU image */
	DFU_TARGET_IMAGE_TYPE_NONE = 0,
	/** Application image in MCUBoot format */
	DFU_TARGET_IMAGE_TYPE_MCUBOOT = 1,
	/** Modem delta-update image */
	DFU_TARGET_IMAGE_TYPE_MODEM_DELTA = 2,
	/** Full update image for modem */
	DFU_TARGET_IMAGE_TYPE_FULL_MODEM = 4,
	/** SMP external MCU */
	DFU_TARGET_IMAGE_TYPE_SMP = 8,
	/** SUIT Envelope */
	DFU_TARGET_IMAGE_TYPE_SUIT = 16,
	/** Any application image type */
	DFU_TARGET_IMAGE_TYPE_ANY_APPLICATION = DFU_TARGET_IMAGE_TYPE_MCUBOOT,
	/** Any modem image */
	DFU_TARGET_IMAGE_TYPE_ANY_MODEM =
		(DFU_TARGET_IMAGE_TYPE_MODEM_DELTA | DFU_TARGET_IMAGE_TYPE_FULL_MODEM),
	/** Any DFU image type */
	DFU_TARGET_IMAGE_TYPE_ANY =
		(DFU_TARGET_IMAGE_TYPE_MCUBOOT | DFU_TARGET_IMAGE_TYPE_MODEM_DELTA |
		 DFU_TARGET_IMAGE_TYPE_FULL_MODEM),
};

enum dfu_target_evt_id {
	DFU_TARGET_EVT_ERASE_PENDING,
	DFU_TARGET_EVT_TIMEOUT,
	DFU_TARGET_EVT_ERASE_DONE
};

typedef void (*dfu_target_callback_t)(enum dfu_target_evt_id evt_id);

int dfu_target_init(int img_type, int img_num, size_t file_size, dfu_target_callback_t cb);
int dfu_target_write(const void *const buf, size_t len);
int dfu_target_done(bool successful);
int dfu_target_reset(void);
int dfu_target_schedule_update(int img_num);

#endif /* __DFU_TARGET_H__ */
