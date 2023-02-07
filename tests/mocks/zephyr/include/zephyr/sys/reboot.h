#ifndef __REBOOT_H__
#define __REBOOT_H__

#define SYS_REBOOT_WARM 0
#define SYS_REBOOT_COLD 1

void sys_reboot(int type);

#endif /* __REBOOT_H__ */
