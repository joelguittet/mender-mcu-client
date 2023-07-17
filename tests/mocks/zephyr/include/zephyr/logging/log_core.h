#ifndef __LOG_CORE_H__
#define __LOG_CORE_H__

#define LOG_LEVEL_NONE 0U
#define LOG_LEVEL_ERR  1U
#define LOG_LEVEL_WRN  2U
#define LOG_LEVEL_INF  3U
#define LOG_LEVEL_DBG  4U

#ifndef CONFIG_LOG
#define CONFIG_LOG_DEFAULT_LEVEL 0U
#define CONFIG_LOG_MAX_LEVEL     0U
#endif

#endif /* __LOG_CORE_H__ */
