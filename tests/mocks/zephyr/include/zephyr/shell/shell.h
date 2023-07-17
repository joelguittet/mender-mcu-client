#ifndef __SHELL_H__
#define __SHELL_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum shell_transport_evt { SHELL_TRANSPORT_EVT_RX_RDY, SHELL_TRANSPORT_EVT_TX_RDY };

typedef void (*shell_transport_handler_t)(enum shell_transport_evt evt, void *context);

struct shell {
    void *dummy;
};

struct shell_transport;

struct shell_transport_api {
    int (*init)(const struct shell_transport *transport, const void *config, shell_transport_handler_t evt_handler, void *context);
    int (*uninit)(const struct shell_transport *transport);
    int (*enable)(const struct shell_transport *transport, bool blocking_tx);
    int (*write)(const struct shell_transport *transport, const void *data, size_t length, size_t *cnt);
    int (*read)(const struct shell_transport *transport, void *data, size_t length, size_t *cnt);
    void (*update)(const struct shell_transport *transport);
};

struct shell_transport {
    const struct shell_transport_api *api;
    void *                            ctx;
};

struct shell_backend_config_flags {
    uint32_t insert_mode : 1;
    uint32_t echo : 1;
    uint32_t obscure : 1;
    uint32_t mode_delete : 1;
    uint32_t use_colors : 1;
    uint32_t use_vt100 : 1;
};

#define SHELL_DEFAULT_BACKEND_CONFIG_FLAGS \
    {                                      \
        .insert_mode = 0,                  \
        .echo        = 1,                  \
        .obscure     = 0,                  \
        .mode_delete = 1,                  \
        .use_colors  = 1,                  \
        .use_vt100   = 1,                  \
    };

#define SHELL_DEFINE(_name, _prompt, _transport_iface, _log_queue_size, _log_timeout, _shell_flag) static const struct shell _name;

int shell_init(const struct shell *sh, const void *transport_config, struct shell_backend_config_flags cfg_flags, bool log_backend, uint32_t init_log_level);
int shell_start(const struct shell *sh);
int shell_stop(const struct shell *sh);

#endif /* __SHELL_H__ */
