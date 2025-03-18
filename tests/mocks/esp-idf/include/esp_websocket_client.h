#ifndef __ESP_WEBSOCKET_CLIENT_H__
#define __ESP_WEBSOCKET_CLIENT_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_transport_ws.h>
#if __has_include("FreeRTOS.h")
#include <FreeRTOS.h>
#include "event_groups.h"
#include "semphr.h"
#else
#include <freertos/FreeRTOS.h>
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#endif /* __has_include("FreeRTOS.h") */

typedef struct esp_websocket_client *esp_websocket_client_handle_t;

typedef enum {
    WEBSOCKET_EVENT_ANY   = -1,
    WEBSOCKET_EVENT_ERROR = 0,
    WEBSOCKET_EVENT_CONNECTED,
    WEBSOCKET_EVENT_DISCONNECTED,
    WEBSOCKET_EVENT_DATA,
    WEBSOCKET_EVENT_CLOSED,
    WEBSOCKET_EVENT_BEFORE_CONNECT,
    WEBSOCKET_EVENT_BEGIN,
    WEBSOCKET_EVENT_FINISH,
    WEBSOCKET_EVENT_MAX
} esp_websocket_event_id_t;

typedef enum {
    WEBSOCKET_ERROR_TYPE_NONE = 0,
    WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT,
    WEBSOCKET_ERROR_TYPE_PONG_TIMEOUT,
    WEBSOCKET_ERROR_TYPE_HANDSHAKE
} esp_websocket_error_type_t;

typedef struct {
    esp_err_t                  esp_tls_last_esp_err;
    int                        esp_tls_stack_err;
    int                        esp_tls_cert_verify_flags;
    esp_websocket_error_type_t error_type;
    int                        esp_ws_handshake_status_code;
    int                        esp_transport_sock_errno;
} esp_websocket_error_codes_t;

typedef struct {
    const char *                  data_ptr;
    int                           data_len;
    bool                          fin;
    uint8_t                       op_code;
    esp_websocket_client_handle_t client;
    void *                        user_context;
    int                           payload_len;
    int                           payload_offset;
    esp_websocket_error_codes_t   error_handle;
} esp_websocket_event_data_t;

typedef enum {
    WEBSOCKET_TRANSPORT_UNKNOWN = 0x0,
    WEBSOCKET_TRANSPORT_OVER_TCP,
    WEBSOCKET_TRANSPORT_OVER_SSL,
} esp_websocket_transport_t;

typedef struct {
    const char *              uri;
    const char *              host;
    int                       port;
    const char *              username;
    const char *              password;
    const char *              path;
    bool                      disable_auto_reconnect;
    void *                    user_context;
    int                       task_prio;
    const char *              task_name;
    int                       task_stack;
    int                       buffer_size;
    const char *              cert_pem;
    size_t                    cert_len;
    const char *              client_cert;
    size_t                    client_cert_len;
    const char *              client_key;
    size_t                    client_key_len;
    esp_websocket_transport_t transport;
    const char *              subprotocol;
    const char *              user_agent;
    const char *              headers;
    int                       pingpong_timeout_sec;
    bool                      disable_pingpong_discon;
    bool                      use_global_ca_store;
    esp_err_t (*crt_bundle_attach)(void *conf);
    bool          skip_cert_common_name_check;
    bool          keep_alive_enable;
    int           keep_alive_idle;
    int           keep_alive_interval;
    int           keep_alive_count;
    int           reconnect_timeout_ms;
    int           network_timeout_ms;
    size_t        ping_interval_sec;
    struct ifreq *if_name;
} esp_websocket_client_config_t;

esp_websocket_client_handle_t esp_websocket_client_init(const esp_websocket_client_config_t *config);
esp_err_t                     esp_websocket_client_set_headers(esp_websocket_client_handle_t client, const char *headers);
esp_err_t                     esp_websocket_client_start(esp_websocket_client_handle_t client);
esp_err_t                     esp_websocket_client_destroy(esp_websocket_client_handle_t client);
int                           esp_websocket_client_send_bin(esp_websocket_client_handle_t client, const char *data, int len, TickType_t timeout);
int                           esp_websocket_client_send_with_opcode(
                              esp_websocket_client_handle_t client, ws_transport_opcodes_t opcode, const uint8_t *data, int len, TickType_t timeout);
esp_err_t esp_websocket_client_close(esp_websocket_client_handle_t client, TickType_t timeout);
esp_err_t esp_websocket_register_events(esp_websocket_client_handle_t client,
                                        esp_websocket_event_id_t      event,
                                        esp_event_handler_t           event_handler,
                                        void *                        event_handler_arg);

#endif /* __ESP_WEBSOCKET_CLIENT_H__ */
