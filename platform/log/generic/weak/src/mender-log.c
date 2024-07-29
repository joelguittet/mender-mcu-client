/**
 * @file      mender-log.c
 * @brief     Mender logging interface for weak platform
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

#include "mender-log.h"

__attribute__((weak)) mender_err_t
mender_log_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

__attribute__((weak)) mender_err_t
mender_log_print(uint8_t level, const char *filename, const char *function, int line, char *format, ...) {

    (void)level;
    (void)filename;
    (void)function;
    (void)line;
    (void)format;

    /* Nothing to do */
    return MENDER_OK;
}

__attribute__((weak)) mender_err_t
mender_log_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}
