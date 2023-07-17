#ifndef __ESP_TRANSPORT_WS_H__
#define __ESP_TRANSPORT_WS_H__

#include <esp_transport.h>

typedef enum ws_transport_opcodes {
    WS_TRANSPORT_OPCODES_CONT   = 0x00,
    WS_TRANSPORT_OPCODES_TEXT   = 0x01,
    WS_TRANSPORT_OPCODES_BINARY = 0x02,
    WS_TRANSPORT_OPCODES_CLOSE  = 0x08,
    WS_TRANSPORT_OPCODES_PING   = 0x09,
    WS_TRANSPORT_OPCODES_PONG   = 0x0a,
    WS_TRANSPORT_OPCODES_FIN    = 0x80,
    WS_TRANSPORT_OPCODES_NONE   = 0x100
} ws_transport_opcodes_t;

#endif /* __ESP_TRANSPORT_WS_H__ */
