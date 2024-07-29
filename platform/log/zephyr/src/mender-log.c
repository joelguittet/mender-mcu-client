/**
 * @file      mender-log.c
 * @brief     Mender logging interface for Zephyr platform
 *
 * Copyright joelguittet and mender-mcu-client contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mender, CONFIG_MENDER_LOG_LEVEL);

#include "mender-log.h"

mender_err_t
mender_log_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

mender_err_t
mender_log_print(uint8_t level, const char *filename, const char *function, int line, char *format, ...) {

    (void)function;
    char log[256] = { 0 };

    /* Format message */
    va_list args;
    va_start(args, format);
    vsnprintf(log, sizeof(log), format, args);
    va_end(args);

    /* Switch depending log level */
    switch (level) {
        case MENDER_LOG_LEVEL_ERR:
            LOG_ERR("%s (%d): %s", filename, line, log);
            break;
        case MENDER_LOG_LEVEL_WRN:
            LOG_WRN("%s (%d): %s", filename, line, log);
            break;
        case MENDER_LOG_LEVEL_INF:
            LOG_INF("%s (%d): %s", filename, line, log);
            break;
        case MENDER_LOG_LEVEL_DBG:
            LOG_DBG("%s (%d): %s", filename, line, log);
            break;
        default:
            break;
    }

    return MENDER_OK;
}

mender_err_t
mender_log_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}
