/**
 * @file      mender-client.c
 * @brief     Mender MCU client implementation
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
#include "mender-flash.h"
#include "mender-log.h"
#include "mender-scheduler.h"
#include "mender-storage.h"
#include "mender-tls.h"

/**
 * @brief Default host
 */
#ifndef CONFIG_MENDER_SERVER_HOST
#define CONFIG_MENDER_SERVER_HOST "https://hosted.mender.io"
#endif /* CONFIG_MENDER_SERVER_HOST */

/**
 * @brief Default tenant token
 */
#ifndef CONFIG_MENDER_SERVER_TENANT_TOKEN
#define CONFIG_MENDER_SERVER_TENANT_TOKEN NULL
#endif /* CONFIG_MENDER_SERVER_TENANT_TOKEN */

/**
 * @brief Default authentication poll interval (seconds)
 */
#ifndef CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL
#define CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL (600)
#endif /* CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL */

/**
 * @brief Default update poll interval (seconds)
 */
#ifndef CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL
#define CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL (1800)
#endif /* CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL */

/**
 * @brief Mender client configuration
 */
static mender_client_config_t mender_client_config;

/**
 * @brief Mender client callbacks
 */
static mender_client_callbacks_t mender_client_callbacks;

/**
 * @brief Mender client states
 */
typedef enum {
    MENDER_CLIENT_STATE_INITIALIZATION, /**< Perform initialization */
    MENDER_CLIENT_STATE_AUTHENTICATION, /**< Perform authentication with the server */
    MENDER_CLIENT_STATE_AUTHENTICATED,  /**< Perform updates */
} mender_client_state_t;

/**
 * @brief Mender client state
 */
static mender_client_state_t mender_client_state = MENDER_CLIENT_STATE_INITIALIZATION;

/**
 * @brief Counter and mutex for the management of network connect/release callbacks
 */
static uint8_t mender_client_network_count = 0;
static void   *mender_client_network_mutex = NULL;

/**
 * @brief Deployment data (ID, artifact name and payload types), used to report deployment status after rebooting
 */
static cJSON *mender_client_deployment_data = NULL;

/**
 * @brief Mender client artifact type
 */
typedef struct {
    char *type; /**< Artifact type */
    mender_err_t (*callback)(
        char *, char *, char *, cJSON *, char *, size_t, void *, size_t, size_t); /**< Callback to be invoked to handle the artifact type */
    bool  needs_restart;                                                          /**< Indicate the artifact type needs a restart to be applied on the system */
    char *artifact_name; /**< Artifact name (optional, NULL otherwise), set to validate module update after restarting */
} mender_client_artifact_type_t;

/**
 * @brief Mender client artifact types list and mutex
 */
static mender_client_artifact_type_t **mender_client_artifact_types_list  = NULL;
static size_t                          mender_client_artifact_types_count = 0;
static void                           *mender_client_artifact_types_mutex = NULL;

/**
 * @brief Mender client add-ons list and mutex
 */
static mender_addon_instance_t **mender_client_addons_list  = NULL;
static size_t                    mender_client_addons_count = 0;
static void                     *mender_client_addons_mutex = NULL;

/**
 * @brief Mender client work handle
 */
static void *mender_client_work_handle = NULL;

/**
 * @brief Flash handle used to store temporary reference to write rootfs-image data
 */
static void *mender_client_flash_handle = NULL;

/**
 * @brief Flag to indicate if the deployment needs to set pending image status
 */
static bool mender_client_deployment_needs_set_pending_image = false;

/**
 * @brief Flag to indicate if the deployment needs restart
 */
static bool mender_client_deployment_needs_restart = false;

/**
 * @brief Mender client work function
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_client_work_function(void);

/**
 * @brief Mender client initialization work function
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_client_initialization_work_function(void);

/**
 * @brief Mender client authentication work function
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_client_authentication_work_function(void);

/**
 * @brief Mender client update work function
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_client_update_work_function(void);

/**
 * @brief Callback function to be invoked to perform the treatment of the data from the artifact
 * @param id ID of the deployment
 * @param artifact name Artifact name
 * @param type Type from header-info payloads
 * @param meta_data Meta-data from header tarball
 * @param filename Artifact filename
 * @param size Artifact file size
 * @param data Artifact data
 * @param index Artifact data index
 * @param length Artifact data length
 * @return MENDER_OK if the function succeeds, error code if an error occurred
 */
static mender_err_t mender_client_download_artifact_callback(
    char *type, cJSON *meta_data, char *filename, size_t size, void *data, size_t index, size_t length);

/**
 * @brief Callback function to be invoked to perform the treatment of the data from the artifact type "rootfs-image"
 * @param id ID of the deployment
 * @param artifact name Artifact name
 * @param type Type from header-info payloads
 * @param meta_data Meta-data from header tarball
 * @param filename Artifact filename
 * @param size Artifact file size
 * @param data Artifact data
 * @param index Artifact data index
 * @param length Artifact data length
 * @return MENDER_OK if the function succeeds, error code if an error occurred
 */
static mender_err_t mender_client_download_artifact_flash_callback(
    char *id, char *artifact_name, char *type, cJSON *meta_data, char *filename, size_t size, void *data, size_t index, size_t length);

/**
 * @brief Publish deployment status of the device to the mender-server and invoke deployment status callback
 * @param id ID of the deployment
 * @param deployment_status Deployment status
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_client_publish_deployment_status(char *id, mender_deployment_status_t deployment_status);

char *
mender_client_version(void) {

    /* Return version as string */
    return MENDER_CLIENT_VERSION;
}

mender_err_t
mender_client_init(mender_client_config_t *config, mender_client_callbacks_t *callbacks) {

    assert(NULL != config);
    assert(NULL != config->identity);
    assert(NULL != config->artifact_name);
    assert(NULL != config->device_type);
    assert(NULL != callbacks);
    assert(NULL != callbacks->restart);
    mender_err_t ret;

    /* Save configuration */
    if (MENDER_OK != (ret = mender_utils_keystore_copy(&mender_client_config.identity, config->identity))) {
        mender_log_error("Unable to copy identity");
        goto END;
    }
    mender_client_config.artifact_name = config->artifact_name;
    mender_client_config.device_type   = config->device_type;
    if ((NULL != config->host) && (strlen(config->host) > 0)) {
        mender_client_config.host = config->host;
    } else {
        mender_client_config.host = CONFIG_MENDER_SERVER_HOST;
    }
    if ((NULL == mender_client_config.host) || (0 == strlen(mender_client_config.host))) {
        mender_log_error("Invalid server host configuration, can't be null or empty");
        ret = MENDER_FAIL;
        goto END;
    }
    if ('/' == mender_client_config.host[strlen(mender_client_config.host) - 1]) {
        mender_log_error("Invalid server host configuration, trailing '/' is not allowed");
        ret = MENDER_FAIL;
        goto END;
    }
    if ((NULL != config->tenant_token) && (strlen(config->tenant_token) > 0)) {
        mender_client_config.tenant_token = config->tenant_token;
    } else {
        mender_client_config.tenant_token = CONFIG_MENDER_SERVER_TENANT_TOKEN;
    }
    if ((NULL != mender_client_config.tenant_token) && (0 == strlen(mender_client_config.tenant_token))) {
        mender_client_config.tenant_token = NULL;
    }
    if (0 != config->authentication_poll_interval) {
        mender_client_config.authentication_poll_interval = config->authentication_poll_interval;
    } else {
        mender_client_config.authentication_poll_interval = CONFIG_MENDER_CLIENT_AUTHENTICATION_POLL_INTERVAL;
    }
    if (0 != config->update_poll_interval) {
        mender_client_config.update_poll_interval = config->update_poll_interval;
    } else {
        mender_client_config.update_poll_interval = CONFIG_MENDER_CLIENT_UPDATE_POLL_INTERVAL;
    }
    mender_client_config.recommissioning = config->recommissioning;

    /* Save callbacks */
    memcpy(&mender_client_callbacks, callbacks, sizeof(mender_client_callbacks_t));

    /* Initializations */
    if (MENDER_OK != (ret = mender_scheduler_init())) {
        mender_log_error("Unable to initialize scheduler");
        goto END;
    }
    if (MENDER_OK != (ret = mender_log_init())) {
        mender_log_error("Unable to initialize log");
        goto END;
    }
    if (MENDER_OK != (ret = mender_storage_init())) {
        mender_log_error("Unable to initialize storage");
        goto END;
    }
    if (MENDER_OK != (ret = mender_tls_init())) {
        mender_log_error("Unable to initialize TLS");
        goto END;
    }
    mender_api_config_t mender_api_config = {
        .identity      = mender_client_config.identity,
        .artifact_name = mender_client_config.artifact_name,
        .device_type   = mender_client_config.device_type,
        .host          = mender_client_config.host,
        .tenant_token  = mender_client_config.tenant_token,
    };
    if (MENDER_OK != (ret = mender_api_init(&mender_api_config))) {
        mender_log_error("Unable to initialize API");
        goto END;
    }

    /* Create network management mutex */
    if (MENDER_OK != (ret = mender_scheduler_mutex_create(&mender_client_network_mutex))) {
        mender_log_error("Unable to create network management mutex");
        return ret;
    }

    /* Create artifact types management mutex */
    if (MENDER_OK != (ret = mender_scheduler_mutex_create(&mender_client_artifact_types_mutex))) {
        mender_log_error("Unable to create artifact types management mutex");
        return ret;
    }

    /* Create add-ons management mutex */
    if (MENDER_OK != (ret = mender_scheduler_mutex_create(&mender_client_addons_mutex))) {
        mender_log_error("Unable to create add-ons management mutex");
        return ret;
    }

    /* Register rootfs-image artifact type */
    if (MENDER_OK
        != (ret = mender_client_register_artifact_type("rootfs-image", &mender_client_download_artifact_flash_callback, true, config->artifact_name))) {
        mender_log_error("Unable to register 'rootfs-image' artifact type");
        goto END;
    }

    /* Create mender client work */
    mender_scheduler_work_params_t update_work_params;
    update_work_params.function = mender_client_work_function;
    update_work_params.period   = mender_client_config.authentication_poll_interval;
    update_work_params.name     = "mender_client_update";
    if (MENDER_OK != (ret = mender_scheduler_work_create(&update_work_params, &mender_client_work_handle))) {
        mender_log_error("Unable to create update work");
        goto END;
    }

END:

    return ret;
}

mender_err_t
mender_client_register_artifact_type(char *type,
                                     mender_err_t (*callback)(char *, char *, char *, cJSON *, char *, size_t, void *, size_t, size_t),
                                     bool  needs_restart,
                                     char *artifact_name) {

    assert(NULL != type);
    mender_client_artifact_type_t  *artifact_type;
    mender_client_artifact_type_t **tmp;
    mender_err_t                    ret;

    /* Take mutex used to protect access to the artifact types management list */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_client_artifact_types_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Create mender artifact type */
    if (NULL == (artifact_type = (mender_client_artifact_type_t *)malloc(sizeof(mender_client_artifact_type_t)))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    artifact_type->type          = type;
    artifact_type->callback      = callback;
    artifact_type->needs_restart = needs_restart;
    artifact_type->artifact_name = artifact_name;

    /* Add mender artifact type to the list */
    if (NULL
        == (tmp = (mender_client_artifact_type_t **)realloc(mender_client_artifact_types_list,
                                                            (mender_client_artifact_types_count + 1) * sizeof(mender_client_artifact_type_t *)))) {
        mender_log_error("Unable to allocate memory");
        free(artifact_type);
        ret = MENDER_FAIL;
        goto END;
    }
    mender_client_artifact_types_list                                     = tmp;
    mender_client_artifact_types_list[mender_client_artifact_types_count] = artifact_type;
    mender_client_artifact_types_count++;

END:

    /* Release mutex used to protect access to the artifact types management list */
    mender_scheduler_mutex_give(mender_client_artifact_types_mutex);

    return ret;
}

mender_err_t
mender_client_register_addon(mender_addon_instance_t *addon, void *config, void *callbacks) {

    assert(NULL != addon);
    mender_addon_instance_t **tmp;
    mender_err_t              ret;

    /* Take mutex used to protect access to the add-ons management list */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_client_addons_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Initialization of the add-on */
    if (NULL != addon->init) {
        if (MENDER_OK != (ret = addon->init(config, callbacks))) {
            mender_log_error("Unable to initialize add-on");
            goto END;
        }
    }

    /* Activate add-on if authentication is already done */
    if (MENDER_CLIENT_STATE_AUTHENTICATED == mender_client_state) {
        if (NULL != addon->activate) {
            if (MENDER_OK != (ret = addon->activate())) {
                mender_log_error("Unable to activate add-on");
                if (NULL != addon->exit) {
                    addon->exit();
                }
                goto END;
            }
        }
    }

    /* Add add-on to the list */
    if (NULL == (tmp = (mender_addon_instance_t **)realloc(mender_client_addons_list, (mender_client_addons_count + 1) * sizeof(mender_addon_instance_t *)))) {
        mender_log_error("Unable to allocate memory");
        if (NULL != addon->exit) {
            addon->exit();
        }
        ret = MENDER_FAIL;
        goto END;
    }
    mender_client_addons_list                             = tmp;
    mender_client_addons_list[mender_client_addons_count] = addon;
    mender_client_addons_count++;

END:

    /* Release mutex used to protect access to the add-ons management list */
    mender_scheduler_mutex_give(mender_client_addons_mutex);

    return ret;
}

mender_err_t
mender_client_activate(void) {

    mender_err_t ret;

    /* Activate update work */
    if (MENDER_OK != (ret = mender_scheduler_work_activate(mender_client_work_handle))) {
        mender_log_error("Unable to activate update work");
        goto END;
    }

END:

    return ret;
}

mender_err_t
mender_client_deactivate(void) {

    mender_err_t ret;

    /* Take mutex used to protect access to the add-ons management list */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_client_addons_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Deactivate add-ons */
    if (NULL != mender_client_addons_list) {
        for (size_t index = 0; index < mender_client_addons_count; index++) {
            if (NULL != mender_client_addons_list[index]->deactivate) {
                mender_client_addons_list[index]->deactivate();
            }
        }
    }

    /* Release mutex used to protect access to the add-ons management list */
    mender_scheduler_mutex_give(mender_client_addons_mutex);

    /* Deactivate mender client work */
    mender_scheduler_work_deactivate(mender_client_work_handle);

    return ret;
}

mender_err_t
mender_client_execute(void) {

    mender_err_t ret;

    /* Trigger execution of the work */
    if (MENDER_OK != (ret = mender_scheduler_work_execute(mender_client_work_handle))) {
        mender_log_error("Unable to trigger update work");
        goto END;
    }

END:

    return ret;
}

mender_err_t
mender_client_network_connect(void) {

    mender_err_t ret;

    /* Take mutex used to protect access to the network management counter */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_client_network_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Check the network management counter value */
    if (0 == mender_client_network_count) {

        /* Request network access */
        if (NULL != mender_client_callbacks.network_connect) {
            if (MENDER_OK != (ret = mender_client_callbacks.network_connect())) {
                mender_log_error("Unable to connect network");
                goto END;
            }
        }
    }

    /* Increment network management counter */
    mender_client_network_count++;

END:

    /* Release mutex used to protect access to the network management counter */
    mender_scheduler_mutex_give(mender_client_network_mutex);

    return ret;
}

mender_err_t
mender_client_network_release(void) {

    mender_err_t ret;

    /* Take mutex used to protect access to the network management counter */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_client_network_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Decrement network management counter */
    mender_client_network_count--;

    /* Check the network management counter value */
    if (0 == mender_client_network_count) {

        /* Release network access */
        if (NULL != mender_client_callbacks.network_release) {
            if (MENDER_OK != (ret = mender_client_callbacks.network_release())) {
                mender_log_error("Unable to release network");
                goto END;
            }
        }
    }

END:

    /* Release mutex used to protect access to the network management counter */
    mender_scheduler_mutex_give(mender_client_network_mutex);

    return ret;
}

mender_err_t
mender_client_exit(void) {

    mender_err_t ret;

    /* Take mutex used to protect access to the add-ons management list */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_client_addons_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Release add-ons */
    if (NULL != mender_client_addons_list) {
        for (size_t index = 0; index < mender_client_addons_count; index++) {
            if (NULL != mender_client_addons_list[index]->exit) {
                mender_client_addons_list[index]->exit();
            }
        }
    }

    /* Release mutex used to protect access to the add-ons management list */
    mender_scheduler_mutex_give(mender_client_addons_mutex);

    /* Delete mender client work */
    mender_scheduler_work_delete(mender_client_work_handle);
    mender_client_work_handle = NULL;

    /* Release all modules */
    mender_api_exit();
    mender_tls_exit();
    mender_storage_exit();
    mender_log_exit();
    mender_scheduler_exit();

    /* Release memory */
    mender_utils_keystore_delete(mender_client_config.identity);
    mender_client_config.identity                     = NULL;
    mender_client_config.artifact_name                = NULL;
    mender_client_config.device_type                  = NULL;
    mender_client_config.host                         = NULL;
    mender_client_config.tenant_token                 = NULL;
    mender_client_config.authentication_poll_interval = 0;
    mender_client_config.update_poll_interval         = 0;
    mender_client_network_count                       = 0;
    mender_scheduler_mutex_give(mender_client_network_mutex);
    mender_scheduler_mutex_delete(mender_client_network_mutex);
    mender_client_network_mutex = NULL;
    if (NULL != mender_client_deployment_data) {
        cJSON_Delete(mender_client_deployment_data);
        mender_client_deployment_data = NULL;
    }
    if (NULL != mender_client_artifact_types_list) {
        for (size_t artifact_type_index = 0; artifact_type_index < mender_client_artifact_types_count; artifact_type_index++) {
            free(mender_client_artifact_types_list[artifact_type_index]);
        }
        free(mender_client_artifact_types_list);
        mender_client_artifact_types_list = NULL;
    }
    mender_client_artifact_types_count = 0;
    mender_scheduler_mutex_give(mender_client_artifact_types_mutex);
    mender_scheduler_mutex_delete(mender_client_artifact_types_mutex);
    mender_client_artifact_types_mutex = NULL;
    if (NULL != mender_client_addons_list) {
        free(mender_client_addons_list);
        mender_client_addons_list = NULL;
    }
    mender_client_addons_count = 0;
    mender_scheduler_mutex_give(mender_client_addons_mutex);
    mender_scheduler_mutex_delete(mender_client_addons_mutex);
    mender_client_addons_mutex = NULL;

    return ret;
}

static mender_err_t
mender_client_work_function(void) {

    mender_err_t ret = MENDER_OK;

    /* Work depending of the client state */
    if (MENDER_CLIENT_STATE_INITIALIZATION == mender_client_state) {
        /* Perform initialization of the client */
        if (MENDER_DONE != (ret = mender_client_initialization_work_function())) {
            goto END;
        }
        /* Update client state */
        mender_client_state = MENDER_CLIENT_STATE_AUTHENTICATION;
    }
    /* Request access to the network */
    if (MENDER_OK != (ret = mender_client_network_connect())) {
        goto END;
    }
    /* Intentional pass-through */
    if (MENDER_CLIENT_STATE_AUTHENTICATION == mender_client_state) {
        /* Perform authentication with the server */
        if (MENDER_DONE != (ret = mender_client_authentication_work_function())) {
            goto RELEASE;
        }
        /* Update work period */
        if (MENDER_OK != (ret = mender_scheduler_work_set_period(mender_client_work_handle, mender_client_config.update_poll_interval))) {
            mender_log_error("Unable to set work period");
            goto RELEASE;
        }
        /* Update client state */
        mender_client_state = MENDER_CLIENT_STATE_AUTHENTICATED;
    }
    /* Intentional pass-through */
    if (MENDER_CLIENT_STATE_AUTHENTICATED == mender_client_state) {
        /* Perform updates */
        ret = mender_client_update_work_function();
    }

RELEASE:

    /* Release access to the network */
    mender_client_network_release();

END:

    return ret;
}

static mender_err_t
mender_client_initialization_work_function(void) {

    char        *deployment_data = NULL;
    mender_err_t ret;

    /* Retrieve or generate authentication keys */
    if (MENDER_OK != (ret = mender_tls_init_authentication_keys(mender_client_config.recommissioning))) {
        mender_log_error("Unable to retrieve or generate authentication keys");
        goto END;
    }

    /* Retrieve deployment data if it is found (following an update) */
    if (MENDER_OK != (ret = mender_storage_get_deployment_data(&deployment_data))) {
        if (MENDER_NOT_FOUND != ret) {
            mender_log_error("Unable to get deployment data");
            goto REBOOT;
        }
    }
    if (NULL != deployment_data) {
        if (NULL == (mender_client_deployment_data = cJSON_Parse(deployment_data))) {
            mender_log_error("Unable to parse deployment data");
            free(deployment_data);
            ret = MENDER_FAIL;
            goto REBOOT;
        }
        free(deployment_data);
    }

    return MENDER_DONE;

END:

    return ret;

REBOOT:

    /* Delete pending deployment */
    mender_storage_delete_deployment_data();

    /* Invoke restart callback, application is responsible to shutdown properly and restart the system */
    if (NULL != mender_client_callbacks.restart) {
        mender_client_callbacks.restart();
    }

    return ret;
}

static mender_err_t
mender_client_authentication_work_function(void) {

    mender_err_t ret;

    /* Perform authentication with the mender server */
    if (MENDER_OK != (ret = mender_api_perform_authentication())) {

        /* Invoke authentication error callback */
        if (NULL != mender_client_callbacks.authentication_failure) {
            if (MENDER_OK != mender_client_callbacks.authentication_failure()) {

                /* Check if deployment is pending */
                if (NULL != mender_client_deployment_data) {
                    /* Authentication error callback inform the reboot should be done, probably something is broken and it prefers to rollback */
                    mender_log_error("Authentication error callback failed, rebooting");
                    goto REBOOT;
                }
            }
        }

        return ret;
    }

    /* Invoke authentication success callback */
    if (NULL != mender_client_callbacks.authentication_success) {
        if (MENDER_OK != mender_client_callbacks.authentication_success()) {

            /* Check if deployment is pending */
            if (NULL != mender_client_deployment_data) {
                /* Authentication success callback inform the reboot should be done, probably something is broken and it prefers to rollback */
                mender_log_error("Authentication success callback failed, rebooting");
                goto REBOOT;
            }
        }
    }

    /* Check if deployment is pending */
    if (NULL != mender_client_deployment_data) {

        /* Retrieve deployment data */
        cJSON *json_id = NULL;
        if (NULL == (json_id = cJSON_GetObjectItemCaseSensitive(mender_client_deployment_data, "id"))) {
            mender_log_error("Unable to get ID from the deployment data");
            goto RELEASE;
        }
        char *id;
        if (NULL == (id = cJSON_GetStringValue(json_id))) {
            mender_log_error("Unable to get ID from the deployment data");
            goto RELEASE;
        }
        cJSON *json_artifact_name = NULL;
        if (NULL == (json_artifact_name = cJSON_GetObjectItemCaseSensitive(mender_client_deployment_data, "artifact_name"))) {
            mender_log_error("Unable to get artifact name from the deployment data");
            goto RELEASE;
        }
        char *artifact_name;
        if (NULL == (artifact_name = cJSON_GetStringValue(json_artifact_name))) {
            mender_log_error("Unable to get artifact name from the deployment data");
            goto RELEASE;
        }
        cJSON *json_types = NULL;
        if (NULL == (json_types = cJSON_GetObjectItemCaseSensitive(mender_client_deployment_data, "types"))) {
            mender_log_error("Unable to get types from the deployment data");
            goto RELEASE;
        }

        /* Take mutex used to protect access to the artifact types management list */
        if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_client_artifact_types_mutex, -1))) {
            mender_log_error("Unable to take mutex");
            goto RELEASE;
        }

        /* Check if artifact running is the pending one */
        bool   success   = true;
        cJSON *json_type = NULL;
        cJSON_ArrayForEach(json_type, json_types) {
            if (NULL != mender_client_artifact_types_list) {
                for (size_t artifact_type_index = 0; artifact_type_index < mender_client_artifact_types_count; artifact_type_index++) {
                    if (!strcmp(mender_client_artifact_types_list[artifact_type_index]->type, cJSON_GetStringValue(json_type))) {
                        if (NULL != mender_client_artifact_types_list[artifact_type_index]->artifact_name) {
                            if (strcmp(mender_client_artifact_types_list[artifact_type_index]->artifact_name, artifact_name)) {
                                /* Deployment status failure */
                                success = false;
                            }
                        }
                    }
                }
            }
        }

        /* Release mutex used to protect access to the artifact types management list */
        mender_scheduler_mutex_give(mender_client_artifact_types_mutex);

        /* Publish deployment status */
        if (true == success) {
            mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_SUCCESS);
        } else {
            mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
        }

        /* Delete pending deployment */
        mender_storage_delete_deployment_data();
    }

RELEASE:

    /* Release memory */
    if (NULL != mender_client_deployment_data) {
        cJSON_Delete(mender_client_deployment_data);
        mender_client_deployment_data = NULL;
    }

    /* Take mutex used to protect access to the add-ons management list */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_client_addons_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Activate add-ons */
    if (NULL != mender_client_addons_list) {
        for (size_t index = 0; index < mender_client_addons_count; index++) {
            if (NULL != mender_client_addons_list[index]->activate) {
                mender_client_addons_list[index]->activate();
            }
        }
    }

    /* Release mutex used to protect access to the add-ons management list */
    mender_scheduler_mutex_give(mender_client_addons_mutex);

    return MENDER_DONE;

REBOOT:

    /* Invoke restart callback, application is responsible to shutdown properly and restart the system */
    if (NULL != mender_client_callbacks.restart) {
        mender_client_callbacks.restart();
    }

    return ret;
}

static mender_err_t
mender_client_update_work_function(void) {

    mender_err_t ret;

    /* Check for deployment */
    char *id              = NULL;
    char *artifact_name   = NULL;
    char *uri             = NULL;
    char *deployment_data = NULL;
    mender_log_info("Checking for deployment...");
    if (MENDER_OK != (ret = mender_api_check_for_deployment(&id, &artifact_name, &uri))) {
        mender_log_error("Unable to check for deployment");
        goto END;
    }

    /* Check if deployment is available */
    if ((NULL == id) || (NULL == artifact_name) || (NULL == uri)) {
        mender_log_info("No deployment available");
        goto END;
    }

    /* Reset flags */
    mender_client_deployment_needs_set_pending_image = false;
    mender_client_deployment_needs_restart           = false;

    /* Create deployment data */
    if (NULL == (mender_client_deployment_data = cJSON_CreateObject())) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    cJSON_AddStringToObject(mender_client_deployment_data, "id", id);
    cJSON_AddStringToObject(mender_client_deployment_data, "artifact_name", artifact_name);
    cJSON_AddArrayToObject(mender_client_deployment_data, "types");

    /* Download deployment artifact */
    mender_log_info("Downloading deployment artifact with id '%s', artifact name '%s' and uri '%s'", id, artifact_name, uri);
    mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_DOWNLOADING);
    if (MENDER_OK != (ret = mender_api_download_artifact(uri, mender_client_download_artifact_callback))) {
        mender_log_error("Unable to download artifact");
        mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
        if (true == mender_client_deployment_needs_set_pending_image) {
            mender_flash_abort_deployment(mender_client_flash_handle);
        }
        goto END;
    }

    /* Set boot partition */
    mender_log_info("Download done, installing artifact");
    mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_INSTALLING);
    if (true == mender_client_deployment_needs_set_pending_image) {
        if (MENDER_OK != (ret = mender_flash_set_pending_image(mender_client_flash_handle))) {
            mender_log_error("Unable to set boot partition");
            mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
            goto END;
        }
    }

    /* Check if the system must restart following downloading the deployment */
    if (true == mender_client_deployment_needs_restart) {
        /* Save deployment data to publish deployment status after rebooting */
        if (NULL == (deployment_data = cJSON_PrintUnformatted(mender_client_deployment_data))) {
            mender_log_error("Unable to save deployment data");
            mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
            ret = MENDER_FAIL;
            goto END;
        }
        if (MENDER_OK != (ret = mender_storage_set_deployment_data(deployment_data))) {
            mender_log_error("Unable to save deployment data");
            mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
            goto END;
        }
        mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_REBOOTING);
    } else {
        /* Publish deployment status success */
        mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_SUCCESS);
        goto END;
    }

    /* Release memory */
    if (NULL != id) {
        free(id);
    }
    if (NULL != artifact_name) {
        free(artifact_name);
    }
    if (NULL != uri) {
        free(uri);
    }
    if (NULL != deployment_data) {
        free(deployment_data);
    }
    if (NULL != mender_client_deployment_data) {
        cJSON_Delete(mender_client_deployment_data);
        mender_client_deployment_data = NULL;
    }

    /* Check if the system must restart following downloading the deployment */
    if (true == mender_client_deployment_needs_restart) {
        /* Invoke restart callback, application is responsible to shutdown properly and restart the system */
        if (NULL != mender_client_callbacks.restart) {
            mender_client_callbacks.restart();
        }
    }

    return MENDER_DONE;

END:

    /* Release memory */
    if (NULL != id) {
        free(id);
    }
    if (NULL != artifact_name) {
        free(artifact_name);
    }
    if (NULL != uri) {
        free(uri);
    }
    if (NULL != deployment_data) {
        free(deployment_data);
    }
    if (NULL != mender_client_deployment_data) {
        cJSON_Delete(mender_client_deployment_data);
        mender_client_deployment_data = NULL;
    }

    return ret;
}

static mender_err_t
mender_client_download_artifact_callback(char *type, cJSON *meta_data, char *filename, size_t size, void *data, size_t index, size_t length) {

    assert(NULL != type);
    cJSON       *json_types;
    mender_err_t ret;

    /* Take mutex used to protect access to the artifact types management list */
    if (MENDER_OK != (ret = mender_scheduler_mutex_take(mender_client_artifact_types_mutex, -1))) {
        mender_log_error("Unable to take mutex");
        return ret;
    }

    /* Treatment depending of the type */
    if (NULL != mender_client_artifact_types_list) {
        for (size_t artifact_type_index = 0; artifact_type_index < mender_client_artifact_types_count; artifact_type_index++) {

            /* Check artifact type */
            if (!strcmp(type, mender_client_artifact_types_list[artifact_type_index]->type)) {

                /* Retrieve ID and artifact name */
                cJSON *json_id = NULL;
                if (NULL == (json_id = cJSON_GetObjectItemCaseSensitive(mender_client_deployment_data, "id"))) {
                    mender_log_error("Unable to get ID from the deployment data");
                    goto END;
                }
                char *id;
                if (NULL == (id = cJSON_GetStringValue(json_id))) {
                    mender_log_error("Unable to get ID from the deployment data");
                    goto END;
                }
                cJSON *json_artifact_name = NULL;
                if (NULL == (json_artifact_name = cJSON_GetObjectItemCaseSensitive(mender_client_deployment_data, "artifact_name"))) {
                    mender_log_error("Unable to get artifact name from the deployment data");
                    goto END;
                }
                char *artifact_name;
                if (NULL == (artifact_name = cJSON_GetStringValue(json_artifact_name))) {
                    mender_log_error("Unable to get artifact name from the deployment data");
                    goto END;
                }

                /* Invoke artifact type callback */
                if (MENDER_OK
                    != (ret = mender_client_artifact_types_list[artifact_type_index]->callback(
                            id, artifact_name, type, meta_data, filename, size, data, index, length))) {
                    mender_log_error("An error occurred while processing data of the artifact '%s'", type);
                    goto END;
                }

                /* Treatments related to the artifact type (once) */
                if (0 == index) {

                    /* Add type to the deployment data */
                    if (NULL == (json_types = cJSON_GetObjectItemCaseSensitive(mender_client_deployment_data, "types"))) {
                        mender_log_error("Unable to add type to the deployment data");
                        ret = MENDER_FAIL;
                        goto END;
                    }
                    bool   found     = false;
                    cJSON *json_type = NULL;
                    cJSON_ArrayForEach(json_type, json_types) {
                        if (!strcmp(type, cJSON_GetStringValue(json_type))) {
                            found = true;
                        }
                    }
                    if (false == found) {
                        cJSON_AddItemToArray(json_types, cJSON_CreateString(type));
                    }

                    /* Set flags */
                    if (true == mender_client_artifact_types_list[artifact_type_index]->needs_restart) {
                        mender_client_deployment_needs_restart = true;
                    }
                }

                goto END;
            }
        }
    }

    /* Content is not supported by the mender-mcu-client */
    mender_log_error("Unable to handle artifact type '%s'", type);
    ret = MENDER_FAIL;

END:

    /* Release mutex used to protect access to the artifact types management list */
    mender_scheduler_mutex_give(mender_client_artifact_types_mutex);

    return ret;
}

static mender_err_t
mender_client_download_artifact_flash_callback(
    char *id, char *artifact_name, char *type, cJSON *meta_data, char *filename, size_t size, void *data, size_t index, size_t length) {

    (void)id;
    (void)artifact_name;
    (void)type;
    (void)meta_data;
    mender_err_t ret = MENDER_OK;

    /* Check if the filename is provided */
    if (NULL != filename) {

        /* Check if the flash handle must be opened */
        if (0 == index) {

            /* Open the flash handle */
            if (MENDER_OK != (ret = mender_flash_open(filename, size, &mender_client_flash_handle))) {
                mender_log_error("Unable to open flash handle");
                goto END;
            }
        }

        /* Write data */
        if (MENDER_OK != (ret = mender_flash_write(mender_client_flash_handle, data, index, length))) {
            mender_log_error("Unable to write data to flash");
            goto END;
        }

        /* Check if the flash handle must be closed */
        if (index + length >= size) {

            /* Close the flash handle */
            if (MENDER_OK != (ret = mender_flash_close(mender_client_flash_handle))) {
                mender_log_error("Unable to close flash handle");
                goto END;
            }
        }
    }

    /* Set flags */
    mender_client_deployment_needs_set_pending_image = true;

END:

    return ret;
}

static mender_err_t
mender_client_publish_deployment_status(char *id, mender_deployment_status_t deployment_status) {

    assert(NULL != id);
    mender_err_t ret;

    /* Publish status to the mender server */
    ret = mender_api_publish_deployment_status(id, deployment_status);

    /* Invoke deployment status callback if defined */
    if (NULL != mender_client_callbacks.deployment_status) {
        mender_client_callbacks.deployment_status(deployment_status, mender_utils_deployment_status_to_string(deployment_status));
    }

    return ret;
}
