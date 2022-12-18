#ifndef __ESP_FLASH_PARTITIONS_H__
#define __ESP_FLASH_PARTITIONS_H__

typedef enum {
    ESP_OTA_IMG_NEW            = 0x0U,
    ESP_OTA_IMG_PENDING_VERIFY = 0x1U,
    ESP_OTA_IMG_VALID          = 0x2U,
    ESP_OTA_IMG_INVALID        = 0x3U,
    ESP_OTA_IMG_ABORTED        = 0x4U,
    ESP_OTA_IMG_UNDEFINED      = 0xFFFFFFFFU,
} esp_ota_img_states_t;

#endif /* __ESP_FLASH_PARTITIONS_H__ */
