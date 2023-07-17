#include <zephyr/shell/shell.h>

int
shell_init(const struct shell *sh, const void *transport_config, struct shell_backend_config_flags cfg_flags, bool log_backend, uint32_t init_log_level) {
    return 0;
}

int
shell_start(const struct shell *sh) {
    return 0;
}

int
shell_stop(const struct shell *sh) {
    return 0;
}
