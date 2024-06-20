/**
 * @file      mender-inventory.h
 * @brief     Mender MCU Inventory add-on implementation
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
