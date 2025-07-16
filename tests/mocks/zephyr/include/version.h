#ifndef __VERSION_H__
#define __VERSION_H__

#define KERNEL_VERSION_STRING "x.y.z"
#define ZEPHYR_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#define ZEPHYR_VERSION_CODE ZEPHYR_VERSION(9, 9, 9)

#endif /* __VERSION_H__ */
