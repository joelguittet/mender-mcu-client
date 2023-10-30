/**
 * @file      mender-log.c
 * @brief     Mender logging interface for Posix platform
 *
 * MIT License
 *
 * Copyright (c) 2022-2023 joelguittet and mender-mcu-client contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <time.h>
#include "mender-log.h"

mender_err_t
mender_log_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

mender_err_t
mender_log_print(uint8_t level, const char *filename, const char *function, int line, char *format, ...) {

    (void)function;
    struct timespec now;
    char            log[256] = { 0 };

    /* Get time */
    clock_gettime(CLOCK_REALTIME, &now);

    /* Format message */
    va_list args;
    va_start(args, format);
    vsnprintf(log, sizeof(log), format, args);
    va_end(args);

    /* Switch depending log level */
    switch (level) {
        case MENDER_LOG_LEVEL_ERR:
            printf("[%ld] <err> %s (%d): %s\n", now.tv_sec, filename, line, log);
            break;
        case MENDER_LOG_LEVEL_WRN:
            printf("[%ld] <war> %s (%d): %s\n", now.tv_sec, filename, line, log);
            break;
        case MENDER_LOG_LEVEL_INF:
            printf("[%ld] <inf> %s (%d): %s\n", now.tv_sec, filename, line, log);
            break;
        case MENDER_LOG_LEVEL_DBG:
            printf("[%ld] <dbg> %s (%d): %s\n", now.tv_sec, filename, line, log);
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
