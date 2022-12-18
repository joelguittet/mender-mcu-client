#include "nvs.h"

esp_err_t
nvs_open(const char *name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
    return ESP_OK;
}

esp_err_t
nvs_set_str(nvs_handle_t handle, const char *key, const char *value) {
    return ESP_OK;
}

esp_err_t
nvs_get_str(nvs_handle_t handle, const char *key, char *out_value, size_t *length) {
    return ESP_OK;
}

esp_err_t
nvs_erase_key(nvs_handle_t handle, const char *key) {
    return ESP_OK;
}

esp_err_t
nvs_commit(nvs_handle_t handle) {
    return ESP_OK;
}

void
nvs_close(nvs_handle_t handle) {
}
