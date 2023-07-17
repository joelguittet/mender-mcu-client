#include <esp_websocket_client.h>

esp_websocket_client_handle_t
esp_websocket_client_init(const esp_websocket_client_config_t *config) {
    return NULL;
}

esp_err_t
esp_websocket_client_set_headers(esp_websocket_client_handle_t client, const char *headers) {
    return ESP_OK;
}

esp_err_t
esp_websocket_client_start(esp_websocket_client_handle_t client) {
    return ESP_OK;
}

esp_err_t
esp_websocket_client_destroy(esp_websocket_client_handle_t client) {
    return ESP_OK;
}

int
esp_websocket_client_send_bin(esp_websocket_client_handle_t client, const char *data, int len, TickType_t timeout) {
    return 0;
}

int
esp_websocket_client_send_with_opcode(esp_websocket_client_handle_t client, ws_transport_opcodes_t opcode, const uint8_t *data, int len, TickType_t timeout) {
    return 0;
}

esp_err_t
esp_websocket_client_close(esp_websocket_client_handle_t client, TickType_t timeout) {
    return ESP_OK;
}

esp_err_t
esp_websocket_register_events(esp_websocket_client_handle_t client,
                              esp_websocket_event_id_t      event,
                              esp_event_handler_t           event_handler,
                              void *                        event_handler_arg) {
    return ESP_OK;
}
