#ifndef __MCUBOOT_H__
#define __MCUBOOT_H__

#include <stdbool.h>

#define BOOT_UPGRADE_TEST      0
#define BOOT_UPGRADE_PERMANENT 1

bool boot_is_img_confirmed(void);
int  boot_write_img_confirmed(void);
int  boot_request_upgrade(int permanent);

#endif /* __MCUBOOT_H__ */
