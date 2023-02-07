#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <stdbool.h>

struct device {
    const char *name;
};

bool device_is_ready(const struct device *dev);

#endif /* __DEVICE_H__ */
