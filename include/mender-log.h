/**
 * @file      mender-log.h
 * @brief     Mender logging interface
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

#ifndef __MENDER_LOG_H__
#define __MENDER_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mender-common.h"

/**
 * @brief Mender log levels
 */
typedef enum {
    MENDER_LOG_DEBUG,   /**< DEBUG */
    MENDER_LOG_INFO,    /**< INFO */
    MENDER_LOG_WARNING, /**< WARNING */
    MENDER_LOG_ERROR,   /**< ERROR */
} mender_log_level_t;

/**
 * @brief Initialize mender log
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_log_init(void);

/**
 * @brief Print log
 * @param level Log level
 * @param filename Filename
 * @param function Function name
 * @param line Line
 * @param format Log format
 * @param ... Arguments
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_log_print(mender_log_level_t level, const char *filename, const char *function, int line, char *format, ...);

/**
 * @brief Print error log
 * @param ... Arguments
 * @return Error code
 */
#define mender_log_error(...) ({ mender_log_print(MENDER_LOG_ERROR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); })

/**
 * @brief Print warning log
 * @param ... Arguments
 * @return Error code
 */
#define mender_log_warning(...) ({ mender_log_print(MENDER_LOG_WARNING, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); })

/**
 * @brief Print info log
 * @param ... Arguments
 * @return Error code
 */
#define mender_log_info(...) ({ mender_log_print(MENDER_LOG_INFO, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); })

/**
 * @brief Print debug log
 * @param error_code Error code
 * @param ... Arguments
 * @return Error code
 */
#ifdef DEBUG
#define mender_log_debug(...) ({ mender_log_print(MENDER_LOG_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); })
#else
#define mender_log_debug(...)
#endif

/**
 * @brief Release mender log
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_log_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* __MENDER_LOG_H__ */
