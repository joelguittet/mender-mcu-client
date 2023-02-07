#include <zephyr/dfu/mcuboot.h>

bool
boot_is_img_confirmed(void) {
    return true;
}

int
boot_write_img_confirmed(void) {
    return 0;
}

int
boot_request_upgrade(int permanent) {
    return 0;
}
