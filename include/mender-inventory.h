/**
 * @file      mender-inventory.h
 * @brief     Mender MCU Inventory add-on implementation
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

#ifndef __MENDER_INVENTORY_H__
#define __MENDER_INVENTORY_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-addon.h"
#include "mender-utils.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY

/**
 * @brief Mender inventory instance
 */
extern const mender_addon_instance_t mender_inventory_addon_instance;

/**
 * @brief Mender inventory configuration
 */
typedef struct {
    int32_t refresh_interval; /**< Inventory refresh interval, default is 28800 seconds, -1 permits to disable periodic execution */
} mender_inventory_config_t;

/**
 * @brief Initialize mender inventory add-on
 * @param config Mender inventory configuration
 * @param callbacks Mender inventory callbacks (not used)
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_inventory_init(void *config, void *callbacks);

/**
 * @brief Activate mender inventory add-on
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_inventory_activate(void);

/**
 * @brief Deactivate mender inventory add-on
 * @note This function stops synchronization with the server
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_inventory_deactivate(void);

/**
 * @brief Set mender inventory
 * @param inventory Mender inventory key/value pairs table, must end with a NULL/NULL element, NULL if not defined
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_inventory_set(mender_keystore_t *inventory);

/**
 * @brief Function used to trigger execution of the inventory work
 * @note Calling this function is optional when the periodic execution of the work is configured
 * @note It only permits to execute the work as soon as possible to synchronize inventory
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_inventory_execute(void);

/**
 * @brief Release mender inventory add-on
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_inventory_exit(void);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_INVENTORY_H__ */
