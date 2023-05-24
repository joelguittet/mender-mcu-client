/**
 * @file      mender-configure.h
 * @brief     Mender MCU Configure add-on implementation
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

#ifndef __MENDER_CONFIGURE_H__
#define __MENDER_CONFIGURE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE

/**
 * @brief Mender configure configuration
 */
typedef struct {
    uint32_t refresh_interval; /**< Configure refresh interval, default is 28800 seconds */
} mender_configure_config_t;

/**
 * @brief Mender configure callbacks
 */
typedef struct {
#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE
    mender_err_t (*config_updated)(mender_keystore_t *); /**< Invoked when configuration is updated */
#endif                                                   /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */
} mender_configure_callbacks_t;

/**
 * @brief Initialize mender configure add-on
 * @param config Mender configure configuration
 * @param callbacks Mender configure callbacks
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_init(mender_configure_config_t *config, mender_configure_callbacks_t *callbacks);

/**
 * @brief Activate mender configure add-on
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_activate(void);

/**
 * @brief Get mender configuration
 * @param configuration Mender configuration key/value pairs table, ends with a NULL/NULL element, NULL if not defined
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_get(mender_keystore_t **configuration);

/**
 * @brief Set mender configuration
 * @param configuration Mender configuration key/value pairs table, must end with a NULL/NULL element, NULL if not defined
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_set(mender_keystore_t *configuration);

/**
 * @brief Release mender configure add-on
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_exit(void);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_CONFIGURE_H__ */
