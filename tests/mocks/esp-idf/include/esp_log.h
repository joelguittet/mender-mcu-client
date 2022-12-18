#ifndef __ESP_LOG_H__
#define __ESP_LOG_H__

typedef enum { ESP_LOG_NONE, ESP_LOG_ERROR, ESP_LOG_WARN, ESP_LOG_INFO, ESP_LOG_DEBUG, ESP_LOG_VERBOSE } esp_log_level_t;

#define ESP_LOGE(tag, format, ...) \
    (void)tag;                     \
    printf(format, ##__VA_ARGS__)

#define ESP_LOGW(tag, format, ...) \
    (void)tag;                     \
    printf(format, ##__VA_ARGS__)

#define ESP_LOGI(tag, format, ...) \
    (void)tag;                     \
    printf(format, ##__VA_ARGS__)

#define ESP_LOGD(tag, format, ...) \
    (void)tag;                     \
    printf(format, ##__VA_ARGS__)

#define ESP_LOGV(tag, format, ...) \
    (void)tag;                     \
    printf(format, ##__VA_ARGS__)

#endif /* __ESP_LOG_H__ */
