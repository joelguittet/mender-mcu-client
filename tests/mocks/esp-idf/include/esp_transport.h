#ifndef __ESP_TRANSPORT_H__
#define __ESP_TRANSPORT_H__

typedef struct esp_transport_keepalive {
    bool keep_alive_enable;
    int  keep_alive_idle;
    int  keep_alive_interval;
    int  keep_alive_count;
} esp_transport_keep_alive_t;

typedef struct esp_transport_list_t *esp_transport_list_handle_t;
typedef struct esp_transport_item_t *esp_transport_handle_t;

#endif /* __ESP_TRANSPORT_H__ */
