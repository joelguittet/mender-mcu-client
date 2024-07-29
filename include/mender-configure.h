/**
 * @file      mender-configure.h
 * @brief     Mender MCU Configure add-on implementation
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

#ifndef __MENDER_CONFIGURE_H__
#define __MENDER_CONFIGURE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-addon.h"
#include "mender-utils.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE

/**
 * @brief Mender configure instance
 */
extern const mender_addon_instance_t mender_configure_addon_instance;

/**
 * @brief Mender configure configuration
 */
typedef struct {
    int32_t refresh_interval; /**< Configure refresh interval, default is 28800 seconds, -1 permits to disable periodic execution */
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
 * @param callbacks Mender configure callbacks (optional)
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_init(void *config, void *callbacks);

/**
 * @brief Activate mender configure add-on
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_activate(void);

/**
 * @brief Deactivate mender configure add-on
 * @note This function stops synchronization with the server
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_deactivate(void);

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
 * @brief Function used to trigger execution of the configure work
 * @note Calling this function is optional when the periodic execution of the work is configured
 * @note It only permits to execute the work as soon as possible to synchronize configuration
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_configure_execute(void);

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
