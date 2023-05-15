#ifndef __ESP_OTA_OPS_H__
#define __ESP_OTA_OPS_H__

#include <esp_partition.h>
#include <esp_flash_partitions.h>

#define OTA_SIZE_UNKNOWN           0xffffffff
#define OTA_WITH_SEQUENTIAL_WRITES 0xfffffffe

#define ESP_ERR_OTA_BASE                   0x1500
#define ESP_ERR_OTA_PARTITION_CONFLICT     (ESP_ERR_OTA_BASE + 0x01)
#define ESP_ERR_OTA_SELECT_INFO_INVALID    (ESP_ERR_OTA_BASE + 0x02)
#define ESP_ERR_OTA_VALIDATE_FAILED        (ESP_ERR_OTA_BASE + 0x03)
#define ESP_ERR_OTA_SMALL_SEC_VER          (ESP_ERR_OTA_BASE + 0x04)
#define ESP_ERR_OTA_ROLLBACK_FAILED        (ESP_ERR_OTA_BASE + 0x05)
#define ESP_ERR_OTA_ROLLBACK_INVALID_STATE (ESP_ERR_OTA_BASE + 0x06)

typedef void *esp_ota_handle_t;

esp_err_t              esp_ota_begin(const esp_partition_t *partition, size_t image_size, esp_ota_handle_t *out_handle);
esp_err_t              esp_ota_write(esp_ota_handle_t handle, const void *data, size_t size);
esp_err_t              esp_ota_end(esp_ota_handle_t handle);
esp_err_t              esp_ota_abort(esp_ota_handle_t handle);
esp_err_t              esp_ota_set_boot_partition(const esp_partition_t *partition);
const esp_partition_t *esp_ota_get_running_partition(void);
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *start_from);
esp_err_t              esp_ota_mark_app_valid_cancel_rollback(void);
esp_err_t              esp_ota_mark_app_invalid_rollback_and_reboot(void);
const esp_partition_t *esp_ota_get_last_invalid_partition(void);
esp_err_t              esp_ota_get_state_partition(const esp_partition_t *partition, esp_ota_img_states_t *ota_state);

#endif /* __ESP_OTA_OPS_H__ */
