/**
 * @file      mender-configure.c
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

#include "mender-api.h"
#include "mender-client.h"
#include "mender-configure.h"
#include "mender-log.h"
#include "mender-scheduler.h"
#include "mender-storage.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE

/**
 * @brief Default configure refresh interval (seconds)
 */
#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL
#define CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL (28800)
#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL */

/**
 * @brief Mender configure instance
 */
const mender_addon_instance_t mender_configure_addon_instance
    = { .init = mender_configure_init, .activate = mender_configure_activate, .deactivate = mender_configure_deactivate, .exit = mender_configure_exit };

/**
 * @brief Mender configure configuration
 */
static mender_configure_config_t mender_configure_config;

/**
 * @brief Mender configure callbacks
 */
static mender_configure_callbacks_t mender_configure_callbacks;

/**
 * @brief Mender configure keystore
 */
static mender_keystore_t *mender_configure_keystore = NULL;
static void              *mender_configure_mutex    = NULL;

/**
 * @brief Mender configure artifact name
 */
static char *mender_configure_artifact_name = NULL;

/**
 * @brief Mender configure work handle
 */
static void *mender_configure_work_handle = NULL;

/**
 * @brief Mender configure work function
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_configure_work_function(void);

/**
 * @brief Callback function to be invoked to perform the treatment of the data from the artifact type "mender-configure"
 * @param id ID of the deployment
 * @param artifact name Artifact name
 * @param type Type from header-info payloads
 * @param meta_data Meta-data from header tarball
 * @param filename Artifact filename
 * @param size Artifact file size
 * @param data Artifact data
 * @param index Artifact data index
 * @param length Artifact data length
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_configure_download_artifact_callback(
    char *id, char *artifact_name, char *type, cJSON *meta_data, char *filename, size_t size, void *data, size_t index, size_t length);

mender_err_t
mender_configure_init(void *config, void *callbacks) {

    assert(NULL != config);
    char        *device_config      = NULL;
    cJSON       *json_device_config = NULL;
    char        *artifact_name;
    mender_err_t ret;

    /* Save configuration */
    if (0 != ((mender_configure_config_t *)config)->refresh_interval) {
        mender_configure_config.refresh_interval = ((mender_configure_config_t *)config)->refresh_interval;
    } else {
        mender_configure_config.refresh_interval = CONFIG_MENDER_CLIENT_CONFIGURE_REFRESH_INTERVAL;
    }

    /* Save callbacks */
    if (NULL != callbacks) {
        memcpy(&mender_configure_callbacks, callbacks, sizeof(mender_configure_callbacks_t));
    }

    /* Create configure mutex */
    if (MENDER_OK != (ret = mender_scheduler_mutex_create(&mender_configure_mutex))) {
        mender_log_error("Unable to create configure mutex");
        goto END;
    }

#ifdef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

    /* Initialize configuration from storage (if it is not empty) */
    if (MENDER_OK != (ret = mender_storage_get_device_config(&device_config))) {
        if (MENDER_NOT_FOUND != ret) {
            mender_log_error("Unable to get device configuration");
            goto END;
        }
    }

    /* Check if configuration is available */
    if (NULL != device_config) {

        /* Parse configuration */
        if (NULL == (json_device_config = cJSON_Parse(device_config))) {
            mender_log_error("Unable to set device configuration");
            ret = MENDER_FAIL;
            goto END;
        }

        /* Set device configuration */
        if (MENDER_OK != (ret = mender_utils_keystore_from_json(&mender_configure_keystore, cJSON_GetObjectItemCaseSensitive(json_device_config, "config")))) {
            mender_log_error("Unable to set device configuration");
            goto END;
        }

        /* Retrieve artifact name if it is available */
        if (NULL != (artifact_name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json_device_config, "artifact_name")))) {
            if (NULL == (mender_configure_artifact_name = strdup(artifact_name))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto END;
            }
        }
    }

    /* Register mender-configure artifact type */
    if (MENDER_OK
        != (ret
            = mender_client_register_artifact_type("mender-configure", &mender_configure_download_artifact_callback, true, mender_configure_artifact_name))) {
        mender_log_error("Unable to register 'mender-configure' artifact type");
        goto END;
    }

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

    /* Create mender configure work */
    mender_scheduler_work_params_t configure_work_params;
    configure_work_params.function = mender_configure_work_function;
    configure_work_params.period   = mender_configure_config.refresh_interval;
    configure_work_params.name     = "mender_configure";
    if (MENDER_OK != (ret = mender_scheduler_work_create(&configure_work_params, &mender_configure_work_handle))) {
        mender_log_error("Unable to create configure work");
        goto END;
    }

END:

    /* Release memory */
    if (NULL != device_config) {
        free(device_config);
    }
    if (NULL != json_device_config) {
        cJSON_Delete(json_device_config);
    }

    return ret;
}

mender_err_t
mender_configure_activate(void) {

    mender_err_t ret;

    /* Activate configure work */
    if (MENDER_OK != (ret = mender_scheduler_work_activate(mender_configure_work_handle))) {
        mender_log_error("Unable to activate configure work");
        return ret;
    }

    return ret;
}

mender_err_t
mender_configure_deactivate(void) {

    /* Deactivate mender configure work */
    mender_scheduler_work_deactivate(mender_configure_work_handle);

    return MENDER_OK;
}

mender_err_t
mender_configure_get(mender_keystore_t **configuration) {

    assert(NULL != configuration);
    mender_err_t ret;

    /* Take mutex used to protect access to the configuration key-store */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_configure_mutex, -1))) {
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
    mender_scheduler_mutex_give(mender_configure_mutex);

    return ret;
}

mender_err_t
mender_configure_set(mender_keystore_t *configuration) {

    cJSON       *json_device_config = NULL;
    cJSON       *json_config        = NULL;
    char        *device_config      = NULL;
    mender_err_t ret;

    /* Take mutex used to protect access to the configuration key-store */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_configure_mutex, -1))) {
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
    if (NULL == (json_device_config = cJSON_CreateObject())) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    if (MENDER_OK != (ret = mender_utils_keystore_to_json(mender_configure_keystore, &json_config))) {
        mender_log_error("Unable to format configuration");
        goto END;
    }
    if (NULL == json_config) {
        mender_log_error("Unable to format configuration");
        ret = MENDER_FAIL;
        goto END;
    }
    cJSON_AddItemToObject(json_device_config, "config", json_config);
    if (NULL == (device_config = cJSON_PrintUnformatted(json_device_config))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    if (MENDER_OK != (ret = mender_storage_set_device_config(device_config))) {
        mender_log_error("Unable to record configuration");
        goto END;
    }

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

END:

    /* Release memory */
    if (NULL != json_device_config) {
        cJSON_Delete(json_device_config);
    }
    if (NULL != device_config) {
        free(device_config);
    }

    /* Release mutex used to protect access to the configuration key-store */
    mender_scheduler_mutex_give(mender_configure_mutex);

    return ret;
}

mender_err_t
mender_configure_execute(void) {

    mender_err_t ret;

    /* Trigger execution of the work */
    if (MENDER_OK != (ret = mender_scheduler_work_execute(mender_configure_work_handle))) {
        mender_log_error("Unable to trigger configure work");
        return ret;
    }

    return MENDER_OK;
}

mender_err_t
mender_configure_exit(void) {

    mender_err_t ret;

    /* Delete mender configure work */
    mender_scheduler_work_delete(mender_configure_work_handle);
    mender_configure_work_handle = NULL;

    /* Take mutex used to protect access to the configure key-store */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_configure_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Release memory */
    mender_configure_config.refresh_interval = 0;
    mender_utils_keystore_delete(mender_configure_keystore);
    mender_configure_keystore = NULL;
    mender_scheduler_mutex_give(mender_configure_mutex);
    mender_scheduler_mutex_delete(mender_configure_mutex);
    mender_configure_mutex = NULL;
    if (NULL != mender_configure_artifact_name) {
        free(mender_configure_artifact_name);
        mender_configure_artifact_name = NULL;
    }

    return ret;
}

static mender_err_t
mender_configure_work_function(void) {

    mender_err_t ret;

    /* Take mutex used to protect access to the configuration key-store */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_configure_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Request access to the network */
    if (MENDER_OK != (ret = mender_client_network_connect())) {
        mender_log_error("Requesting access to the network failed");
        goto END;
    }

#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

    /* Download configuration */
    mender_keystore_t *configuration = NULL;
    if (MENDER_OK != (ret = mender_api_download_configuration_data(&configuration))) {
        mender_log_error("Unable to get configuration data");
        goto RELEASE;
    }

    /* Release previous configuration */
    if (MENDER_OK != (ret = mender_utils_keystore_delete(mender_configure_keystore))) {
        mender_log_error("Unable to delete device configuration");
        goto RELEASE;
    }

    /* Update device configuration */
    if (MENDER_OK != (ret = mender_utils_keystore_copy(&mender_configure_keystore, configuration))) {
        mender_log_error("Unable to update device configuration");
        goto RELEASE;
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

RELEASE:

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

    /* Release access to the network */
    mender_client_network_release();

END:

#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

    /* Release memeory */
    mender_utils_keystore_delete(configuration);

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

    /* Release mutex used to protect access to the configuration key-store */
    mender_scheduler_mutex_give(mender_configure_mutex);

    return ret;
}

#ifdef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

static mender_err_t
mender_configure_download_artifact_callback(
    char *id, char *artifact_name, char *type, cJSON *meta_data, char *filename, size_t size, void *data, size_t index, size_t length) {

    (void)id;
    (void)type;
    (void)filename;
    (void)size;
    (void)data;
    (void)index;
    (void)length;
    cJSON       *json_device_config = NULL;
    char        *device_config      = NULL;
    mender_err_t ret                = MENDER_OK;

    /* Check meta-data */
    if (NULL != meta_data) {

        /* Save the device configuration */
        if (NULL == (json_device_config = cJSON_CreateObject())) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        cJSON_AddStringToObject(json_device_config, "artifact_name", artifact_name);
        cJSON_AddItemToObject(json_device_config, "config", cJSON_Duplicate(meta_data, true));
        if (NULL == (device_config = cJSON_PrintUnformatted(json_device_config))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        if (MENDER_OK != (ret = mender_storage_set_device_config(device_config))) {
            mender_log_error("Unable to record configuration");
            goto END;
        }

    } else {

        /* Delete the configuration */
        if (MENDER_OK != (ret = mender_storage_delete_device_config())) {
            mender_log_error("Unable to delete configuration");
        }
    }

END:

    /* Release memory */
    if (NULL != json_device_config) {
        cJSON_Delete(json_device_config);
    }
    if (NULL != device_config) {
        free(device_config);
    }
    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */
