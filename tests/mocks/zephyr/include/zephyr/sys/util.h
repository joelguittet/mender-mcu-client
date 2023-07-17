#ifndef __UTIL_H__
#define __UTIL_H__

#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util_macro.h>

#define CONTAINER_OF(ptr, type, field) ((type *)(((char *)(ptr)) - offsetof(type, field)))

#endif /* __UTIL_H__ */
