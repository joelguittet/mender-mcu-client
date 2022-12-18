#include "esp_http_client.h"

esp_http_client_handle_t
esp_http_client_init(const esp_http_client_config_t *config) {
    return NULL;
}

esp_err_t
esp_http_client_set_header(esp_http_client_handle_t client, const char *key, const char *value) {
    return ESP_OK;
}

esp_err_t
esp_http_client_set_method(esp_http_client_handle_t client, esp_http_client_method_t method) {
    return ESP_OK;
}

esp_err_t
esp_http_client_open(esp_http_client_handle_t client, int write_len) {
    return ESP_OK;
}

int
esp_http_client_write(esp_http_client_handle_t client, const char *buffer, int len) {
    return 0;
}

int
esp_http_client_fetch_headers(esp_http_client_handle_t client) {
    return 0;
}

int
esp_http_client_read(esp_http_client_handle_t client, char *buffer, int len) {
    return 0;
}

int
esp_http_client_get_status_code(esp_http_client_handle_t client) {
    return 0;
}

esp_err_t
esp_http_client_cleanup(esp_http_client_handle_t client) {
    return ESP_OK;
}

bool
esp_http_client_is_complete_data_received(esp_http_client_handle_t client) {
    return true;
}
