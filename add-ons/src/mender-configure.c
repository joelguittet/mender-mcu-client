/**
 * @file      mender-configure.c
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

#include "mender-api.h"
#include "mender-configure.h"
#include "mender-log.h"
#include "mender-rtos.h"
#include "mender-storage.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE

/**
 * @brief Default configure refresh interval (seconds)
 */
#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL
#define CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL (28800)
#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL */

/**
 * @brief Mender configure configuration
 */
static mender_configure_config_t mender_configure_config;

/**
 * @brief Mender configure callbacks
 */
static mender_configure_callbacks_t mender_configure_callbacks;

/**
 * @brief Mender configure
 */
static mender_keystore_t *mender_configure_keystore = NULL;
static void *             mender_configure_mutex    = NULL;

/**
 * @brief Mender configure work handle
 */
static void *mender_configure_work_handle = NULL;

/**
 * @brief Mender configure work function
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_configure_work_function(void);

mender_err_t
mender_configure_init(mender_configure_config_t *config, mender_configure_callbacks_t *callbacks) {

    assert(NULL != config);
    mender_err_t ret;

    /* Save configuration */
    if (0 != config->refresh_interval) {
        mender_configure_config.refresh_interval = config->refresh_interval;
    } else {
        mender_configure_config.refresh_interval = CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL;
    }

    /* Save callbacks */
    memcpy(&mender_configure_callbacks, callbacks, sizeof(mender_configure_callbacks_t));

    /* Create configure mutex */
    if (MENDER_OK != (ret = mender_rtos_mutex_create(&mender_configure_mutex))) {
        mender_log_error("Unable to create configure mutex");
        return ret;
    }

#ifdef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

    /* Initialize configuration from storage (if it is not empty) */
    char *device_config = NULL;
    if (MENDER_OK != (ret = mender_storage_get_device_config(&device_config))) {
        if (MENDER_NOT_FOUND != ret) {
            mender_log_error("Unable to get device configuration");
            return ret;
        }
    }

    /* Set device configuration */
    if (NULL != device_config) {
        if (MENDER_OK != (ret = mender_utils_keystore_from_string(&mender_configure_keystore, device_config))) {
            mender_log_error("Unable to set device configuration");
            free(device_config);
            return ret;
        }
        free(device_config);
    }

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

    /* Create mender configure work */
    mender_rtos_work_params_t configure_work_params;
    configure_work_params.function = mender_configure_work_function;
    configure_work_params.period   = mender_configure_config.refresh_interval;
    configure_work_params.name     = "mender_configure";
    if (MENDER_OK != (ret = mender_rtos_work_create(&configure_work_params, &mender_configure_work_handle))) {
        mender_log_error("Unable to create configure work");
        return ret;
    }

    return ret;
}

mender_err_t
mender_configure_activate(void) {

    mender_err_t ret;

    /* Activate configure work */
    if (MENDER_OK != (ret = mender_rtos_work_activate(mender_configure_work_handle))) {
        mender_log_error("Unable to activate configure work");
        return ret;
    }

    return ret;
}

mender_err_t
mender_configure_get(mender_keystore_t **configuration) {

    assert(NULL != configuration);
    mender_err_t ret;

    /* Take mutex used to protect access to the configuration key-store */
    if (MENDER_OK != (ret = mender_rtos_mutex_take(mender_configure_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Copy the configuration */
    if (MENDER_OK != (ret = mender_utils_keystore_copy(configuration, mender_configure_keystore))) {
        mender_log_error("Unable to copy configuration");
        goto END;
    }

END:

    /* Release mutex used to protect access to the configuration key-store */
    mender_rtos_mutex_give(mender_configure_mutex);

    return ret;
}

mender_err_t
mender_configure_set(mender_keystore_t *configuration) {

    mender_err_t ret;

    /* Take mutex used to protect access to the configuration key-store */
    if (MENDER_OK != (ret = mender_rtos_mutex_take(mender_configure_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Release previous configuration */
    if (MENDER_OK != (ret = mender_utils_keystore_delete(mender_configure_keystore))) {
        mender_log_error("Unable to delete device configuration");
        goto END;
    }

    /* Copy the new configuration */
    if (MENDER_OK != (ret = mender_utils_keystore_copy(&mender_configure_keystore, configuration))) {
        mender_log_error("Unable to copy configuration");
        goto END;
    }

#ifdef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

    /* Save the device configuration */
    char *device_config = NULL;
    if (MENDER_OK != (ret = mender_utils_keystore_to_string(mender_configure_keystore, &device_config))) {
        mender_log_error("Unable to format configuration");
        goto END;
    }
    if (NULL != device_config) {
        if (MENDER_OK != (ret = mender_storage_set_device_config(device_config))) {
            mender_log_error("Unable to record configuration");
        }
        free(device_config);
    }

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

END:

    /* Release mutex used to protect access to the configuration key-store */
    mender_rtos_mutex_give(mender_configure_mutex);

    return ret;
}

mender_err_t
mender_configure_exit(void) {

    mender_err_t ret;

    /* Deactivate mender configure work */
    mender_rtos_work_deactivate(mender_configure_work_handle);

    /* Delete mender configure work */
    mender_rtos_work_delete(mender_configure_work_handle);
    mender_configure_work_handle = NULL;

    /* Take mutex used to protect access to the configure key-store */
    if (MENDER_OK != (ret = mender_rtos_mutex_take(mender_configure_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Release memory */
    mender_configure_config.refresh_interval = 0;
    mender_utils_keystore_delete(mender_configure_keystore);
    mender_configure_keystore = NULL;
    mender_rtos_mutex_give(mender_configure_mutex);
    mender_rtos_mutex_delete(mender_configure_mutex);

    return ret;
}

static mender_err_t
mender_configure_work_function(void) {

    mender_err_t ret;

    /* Take mutex used to protect access to the configuration key-store */
    if (MENDER_OK != (ret = mender_rtos_mutex_take(mender_configure_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

    /* Download configuration */
    mender_keystore_t *configuration = NULL;
    if (MENDER_OK != (ret = mender_api_download_configuration_data(&configuration))) {
        mender_log_error("Unable to get configuration data");
        goto END;
    }

    /* Release previous configuration */
    if (MENDER_OK != (ret = mender_utils_keystore_delete(mender_configure_keystore))) {
        mender_log_error("Unable to delete device configuration");
        goto END;
    }

    /* Update device configuration */
    if (MENDER_OK != (ret = mender_utils_keystore_copy(&mender_configure_keystore, configuration))) {
        mender_log_error("Unable to update device configuration");
        goto END;
    }

    /* Invoke the update callback */
    if (NULL != mender_configure_callbacks.config_updated) {
        mender_configure_callbacks.config_updated(mender_configure_keystore);
    }

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

    /* Publish configuration */
    if (MENDER_OK != (ret = mender_api_publish_configuration_data(mender_configure_keystore))) {
        mender_log_error("Unable to publish configuration data");
    }

#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

END:

    /* Release memeory */
    mender_utils_keystore_delete(configuration);

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

    /* Release mutex used to protect access to the configuration key-store */
    mender_rtos_mutex_give(mender_configure_mutex);

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */
