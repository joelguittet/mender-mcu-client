/**
 * @file      mender-inventory.c
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

#include "mender-api.h"
#include "mender-inventory.h"
#include "mender-log.h"
#include "mender-scheduler.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY

/**
 * @brief Default inventory refresh interval (seconds)
 */
#ifndef CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL
#define CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL (28800)
#endif /* CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL */

/**
 * @brief Mender inventory configuration
 */
static mender_inventory_config_t mender_inventory_config;

/**
 * @brief Mender inventory
 */
static mender_keystore_t *mender_inventory_keystore = NULL;
static void *             mender_inventory_mutex    = NULL;

/**
 * @brief Mender inventory work handle
 */
static void *mender_inventory_work_handle = NULL;

/**
 * @brief Mender inventory work function
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_inventory_work_function(void);

mender_err_t
mender_inventory_init(mender_inventory_config_t *config) {

    assert(NULL != config);
    mender_err_t ret;

    /* Save configuration */
    if (0 != config->refresh_interval) {
        mender_inventory_config.refresh_interval = config->refresh_interval;
    } else {
        mender_inventory_config.refresh_interval = CONFIG_MENDER_CLIENT_INVENTORY_REFRESH_INTERVAL;
    }

    /* Create inventory mutex */
    if (MENDER_OK != (ret = mender_scheduler_mutex_create(&mender_inventory_mutex))) {
        mender_log_error("Unable to create inventory mutex");
        return ret;
    }

    /* Create mender inventory work */
    mender_scheduler_work_params_t inventory_work_params;
    inventory_work_params.function = mender_inventory_work_function;
    inventory_work_params.period   = mender_inventory_config.refresh_interval;
    inventory_work_params.name     = "mender_inventory";
    if (MENDER_OK != (ret = mender_scheduler_work_create(&inventory_work_params, &mender_inventory_work_handle))) {
        mender_log_error("Unable to create inventory work");
        return ret;
    }

    return ret;
}

mender_err_t
mender_inventory_activate(void) {

    mender_err_t ret;

    /* Activate inventory work */
    if (MENDER_OK != (ret = mender_scheduler_work_activate(mender_inventory_work_handle))) {
        mender_log_error("Unable to activate inventory work");
        return ret;
    }

    return ret;
}

mender_err_t
mender_inventory_set(mender_keystore_t *inventory) {

    mender_err_t ret;

    /* Take mutex used to protect access to the inventory key-store */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_inventory_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Release previous inventory */
    if (MENDER_OK != (ret = mender_utils_keystore_delete(mender_inventory_keystore))) {
        mender_log_error("Unable to delete inventory");
        goto END;
    }

    /* Copy the new inventory */
    if (MENDER_OK != (ret = mender_utils_keystore_copy(&mender_inventory_keystore, inventory))) {
        mender_log_error("Unable to copy inventory");
        goto END;
    }

END:

    /* Release mutex used to protect access to the inventory key-store */
    mender_scheduler_mutex_give(mender_inventory_mutex);

    return ret;
}

mender_err_t
mender_inventory_execute(void) {

    mender_err_t ret;

    /* Trigger execution of the work */
    if (MENDER_OK != (ret = mender_scheduler_work_execute(mender_inventory_work_handle))) {
        mender_log_error("Unable to trigger inventory work");
        return ret;
    }

    return MENDER_OK;
}

mender_err_t
mender_inventory_exit(void) {

    mender_err_t ret;

    /* Deactivate mender inventory work */
    mender_scheduler_work_deactivate(mender_inventory_work_handle);

    /* Delete mender inventory work */
    mender_scheduler_work_delete(mender_inventory_work_handle);
    mender_inventory_work_handle = NULL;

    /* Take mutex used to protect access to the inventory key-store */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_inventory_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Release memory */
    mender_inventory_config.refresh_interval = 0;
    mender_utils_keystore_delete(mender_inventory_keystore);
    mender_inventory_keystore = NULL;
    mender_scheduler_mutex_give(mender_inventory_mutex);
    mender_scheduler_mutex_delete(mender_inventory_mutex);
    mender_inventory_mutex = NULL;

    return ret;
}

static mender_err_t
mender_inventory_work_function(void) {

    mender_err_t ret;

    /* Take mutex used to protect access to the inventory key-store */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_inventory_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Publish inventory */
    if (MENDER_OK != (ret = mender_api_publish_inventory_data(mender_inventory_keystore))) {
        mender_log_error("Unable to publish inventory data");
    }

    /* Release mutex used to protect access to the inventory key-store */
    mender_scheduler_mutex_give(mender_inventory_mutex);

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */
