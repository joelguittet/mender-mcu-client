#include <esp_ota_ops.h>

esp_err_t
esp_ota_begin(const esp_partition_t *partition, size_t image_size, esp_ota_handle_t *out_handle) {
    return ESP_OK;
}

esp_err_t
esp_ota_write(esp_ota_handle_t handle, const void *data, size_t size) {
    return ESP_OK;
}

esp_err_t
esp_ota_end(esp_ota_handle_t handle) {
    return ESP_OK;
}

esp_err_t
esp_ota_abort(esp_ota_handle_t handle) {
    return ESP_OK;
}

esp_err_t
esp_ota_set_boot_partition(const esp_partition_t *partition) {
    return ESP_OK;
}

const esp_partition_t *
esp_ota_get_running_partition(void) {
    return NULL;
}

const esp_partition_t *
esp_ota_get_next_update_partition(const esp_partition_t *start_from) {
    return NULL;
}

esp_err_t
esp_ota_mark_app_valid_cancel_rollback(void) {
    return ESP_OK;
}

esp_err_t
esp_ota_mark_app_invalid_rollback_and_reboot(void) {
    return ESP_OK;
}

const esp_partition_t *
esp_ota_get_last_invalid_partition(void) {
    return NULL;
}

esp_err_t
esp_ota_get_state_partition(const esp_partition_t *partition, esp_ota_img_states_t *ota_state) {
    return ESP_OK;
}
