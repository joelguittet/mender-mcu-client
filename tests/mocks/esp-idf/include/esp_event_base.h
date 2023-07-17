#ifndef __ESP_EVENT_BASE_H__
#define __ESP_EVENT_BASE_H__

#include <stdint.h>

typedef const char *esp_event_base_t;
typedef void *      esp_event_loop_handle_t;
typedef void (*esp_event_handler_t)(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

#endif /* __ESP_EVENT_BASE_H__ */
