#ifndef __LOG_H__
#define __LOG_H__

#include <zephyr/logging/log_core.h>

#define LOG_MODULE_REGISTER(...)

#define LOG_ERR(...) printf(__VA_ARGS__)
#define LOG_WRN(...) printf(__VA_ARGS__)
#define LOG_DBG(...) printf(__VA_ARGS__)
#define LOG_INF(...) printf(__VA_ARGS__)

#endif /* __LOG_H__ */
