/**
 * @file      mender-client.c
 * @brief     Mender MCU client implementation
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
#include "mender-client.h"
#include "mender-log.h"
#include "mender-rtos.h"
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
 * @brief OTA ID and artifact name, used to report OTA status after rebooting
 */
static char *mender_client_ota_id            = NULL;
static char *mender_client_ota_artifact_name = NULL;

/**
 * @brief Mender client work handle
 */
static void *mender_client_work_handle = NULL;

/**
 * @brief OTA handle used to store temporary reference to write OTA data
 */
static void *mender_client_ota_handle = NULL;

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
 * @param type Type from header-info payloads
 * @param meta_data Meta-data from header tarball
 * @param filename Artifact filename
 * @param size Artifact file size
 * @param data Artifact data
 * @param index Artifact data index
 * @param length Artifact data length
 * @return MENDER_OK if the function succeeds, error code if an error occured
 */
static mender_err_t mender_client_download_artifact_callback(
    char *type, cJSON *meta_data, char *filename, size_t size, void *data, size_t index, size_t length);

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
    assert(NULL != config->mac_address);
    assert(NULL != config->artifact_name);
    assert(NULL != config->device_type);
    assert(NULL != callbacks);
    assert(NULL != callbacks->ota_begin);
    assert(NULL != callbacks->ota_write);
    assert(NULL != callbacks->ota_abort);
    assert(NULL != callbacks->ota_end);
    assert(NULL != callbacks->ota_set_boot_partition);
    assert(NULL != callbacks->restart);
    mender_err_t ret;

    /* Save configuration */
    mender_client_config.mac_address   = config->mac_address;
    mender_client_config.artifact_name = config->artifact_name;
    mender_client_config.device_type   = config->device_type;
    if ((NULL != config->host) && (strlen(config->host) > 0)) {
        mender_client_config.host = config->host;
    } else {
        mender_client_config.host = CONFIG_MENDER_SERVER_HOST;
    }
    if ((NULL != config->tenant_token) && (strlen(config->tenant_token) > 0)) {
        mender_client_config.tenant_token = config->tenant_token;
    } else {
        mender_client_config.tenant_token = CONFIG_MENDER_SERVER_TENANT_TOKEN;
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
    if (MENDER_OK != (ret = mender_rtos_init())) {
        mender_log_error("Unable to initialize rtos");
        return ret;
    }
    if (MENDER_OK != (ret = mender_log_init())) {
        mender_log_error("Unable to initialize log");
        return ret;
    }
    if (MENDER_OK != (ret = mender_storage_init())) {
        mender_log_error("Unable to initialize storage");
        return ret;
    }
    if (MENDER_OK != (ret = mender_tls_init())) {
        mender_log_error("Unable to initialize TLS");
        return ret;
    }
    mender_api_config_t mender_api_config = {
        .mac_address   = mender_client_config.mac_address,
        .artifact_name = mender_client_config.artifact_name,
        .device_type   = mender_client_config.device_type,
        .host          = mender_client_config.host,
        .tenant_token  = mender_client_config.tenant_token,
    };
    if (MENDER_OK != (ret = mender_api_init(&mender_api_config))) {
        mender_log_error("Unable to initialize API");
        return ret;
    }

    /* Create mender client work */
    mender_rtos_work_params_t update_work_params;
    update_work_params.function = mender_client_work_function;
    update_work_params.period   = mender_client_config.authentication_poll_interval;
    update_work_params.name     = "mender_client_update";
    if (MENDER_OK != (ret = mender_rtos_work_create(&update_work_params, &mender_client_work_handle))) {
        mender_log_error("Unable to create update work");
        return ret;
    }

    /* Activate update work */
    if (MENDER_OK != (ret = mender_rtos_work_activate(mender_client_work_handle))) {
        mender_log_error("Unable to activate update work");
        return ret;
    }

    return ret;
}

mender_err_t
mender_client_execute(void) {

    mender_err_t ret;

    /* Trigger execution of the work */
    if (MENDER_OK != (ret = mender_rtos_work_execute(mender_client_work_handle))) {
        mender_log_error("Unable to trigger update work");
        return ret;
    }

    return MENDER_OK;
}

mender_err_t
mender_client_exit(void) {

    /* Deactivate mender client work */
    mender_rtos_work_deactivate(mender_client_work_handle);

    /* Delete mender client work */
    mender_rtos_work_delete(mender_client_work_handle);
    mender_client_work_handle = NULL;

    /* Release all modules */
    mender_api_exit();
    mender_tls_exit();
    mender_storage_exit();
    mender_log_exit();
    mender_rtos_exit();

    /* Release memory */
    mender_client_config.mac_address                  = NULL;
    mender_client_config.artifact_name                = NULL;
    mender_client_config.device_type                  = NULL;
    mender_client_config.host                         = NULL;
    mender_client_config.tenant_token                 = NULL;
    mender_client_config.authentication_poll_interval = 0;
    mender_client_config.update_poll_interval         = 0;
    if (NULL != mender_client_ota_id) {
        free(mender_client_ota_id);
        mender_client_ota_id = NULL;
    }
    if (NULL != mender_client_ota_artifact_name) {
        free(mender_client_ota_artifact_name);
        mender_client_ota_artifact_name = NULL;
    }

    return MENDER_OK;
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
    /* Intentional pass-through */
    if (MENDER_CLIENT_STATE_AUTHENTICATION == mender_client_state) {
        /* Perform authentication with the server */
        if (MENDER_DONE != (ret = mender_client_authentication_work_function())) {
            goto END;
        }
        /* Update work period */
        if (MENDER_OK != (ret = mender_rtos_work_set_period(mender_client_work_handle, mender_client_config.update_poll_interval))) {
            mender_log_error("Unable to set work period");
            goto END;
        }
        /* Update client state */
        mender_client_state = MENDER_CLIENT_STATE_AUTHENTICATED;
    }
    /* Intentional pass-through */
    if (MENDER_CLIENT_STATE_AUTHENTICATED == mender_client_state) {
        /* Perform updates */
        ret = mender_client_update_work_function();
    }

END:

    return ret;
}

static mender_err_t
mender_client_initialization_work_function(void) {

    mender_err_t ret;

    /* Retrieve or generate authentication keys */
    if (MENDER_OK != (ret = mender_tls_init_authentication_keys(mender_client_config.recommissioning))) {
        mender_log_error("Unable to retrieve or generate authentication keys");
        return ret;
    }

    /* Retrieve OTA ID if it is found (following an update) */
    if (MENDER_OK != (ret = mender_storage_get_ota_deployment(&mender_client_ota_id, &mender_client_ota_artifact_name))) {
        if (MENDER_NOT_FOUND != ret) {
            mender_log_error("Unable to get OTA ID");
            return ret;
        }
    }

    return MENDER_DONE;
}

static mender_err_t
mender_client_authentication_work_function(void) {

    mender_err_t ret;

    /* Perform authentication with the mender server */
    if (MENDER_OK != (ret = mender_api_perform_authentication())) {

        /* Invoke authentication error callback */
        if (NULL != mender_client_callbacks.authentication_failure) {
            if (MENDER_OK != mender_client_callbacks.authentication_failure()) {

                /* Check if OTA is pending */
                if ((NULL != mender_client_ota_id) && (NULL != mender_client_ota_artifact_name)) {
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

            /* Check if OTA is pending */
            if ((NULL != mender_client_ota_id) && (NULL != mender_client_ota_artifact_name)) {
                /* Authentication success callback inform the reboot should be done, probably something is broken and it prefers to rollback */
                mender_log_error("Authentication success callback failed, rebooting");
                goto REBOOT;
            }
        }
    }

    /* Check if OTA is pending */
    if ((NULL != mender_client_ota_id) && (NULL != mender_client_ota_artifact_name)) {

        /* Check if artifact running is the pending one or if the configuration deployed is the expected one, fails if rollback occurred */
        if ((!strcmp(mender_client_ota_artifact_name, mender_client_config.artifact_name))
            || ((mender_utils_strbeginwith(mender_client_ota_artifact_name, "configuration"))
                && (!strcmp(mender_client_ota_artifact_name + strlen("configuration") + 1, mender_client_ota_id)))) {

            /* Publish deployment status success */
            mender_client_publish_deployment_status(mender_client_ota_id, MENDER_DEPLOYMENT_STATUS_SUCCESS);

        } else {

            /* Publish deployment status failure */
            mender_client_publish_deployment_status(mender_client_ota_id, MENDER_DEPLOYMENT_STATUS_FAILURE);
        }

        /* Delete pending OTA */
        mender_storage_delete_ota_deployment();
    }

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
    char *id            = NULL;
    char *artifact_name = NULL;
    char *uri           = NULL;
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

    /* Check if artifact is already installed (should not occur) */
    if (!strcmp(artifact_name, mender_client_config.artifact_name)) {
        mender_log_error("Artifact is already installed");
        mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_ALREADY_INSTALLED);
        goto END;
    }

    /* Download deployment artifact */
    mender_log_info("Downloading deployment artifact with id '%s', artifact name '%s' and uri '%s'", id, artifact_name, uri);
    mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_DOWNLOADING);
    if (MENDER_OK != (ret = mender_api_download_artifact(uri, mender_client_download_artifact_callback))) {
        mender_log_error("Unable to download artifact");
        mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
        mender_client_callbacks.ota_abort(mender_client_ota_handle);
        goto END;
    }

    /* Set boot partition */
    mender_log_info("Download done, installing artifact");
    mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_INSTALLING);
    if (MENDER_OK != (ret = mender_client_callbacks.ota_set_boot_partition(mender_client_ota_handle))) {
        mender_log_error("Unable to set boot partition");
        mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
        goto END;
    }

    /* Save OTA ID to publish deployment status after rebooting */
    if (MENDER_OK != (ret = mender_storage_set_ota_deployment(id, artifact_name))) {
        mender_log_error("Unable to save OTA ID");
        mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
        goto END;
    }

    /* Now need to reboot to apply the update */
    mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_REBOOTING);

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

    /* Invoke restart callback, application is responsible to shutdown properly and restart the system */
    if (NULL != mender_client_callbacks.restart) {
        mender_client_callbacks.restart();
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

    return ret;
}

static mender_err_t
mender_client_download_artifact_callback(char *type, cJSON *meta_data, char *filename, size_t size, void *data, size_t index, size_t length) {

    assert(NULL != type);
    (void)meta_data;
    mender_err_t ret = MENDER_OK;

    /* Treatment depending of the type */
    if (!strcmp(type, "rootfs-image")) {

        /* Check if the filename is provided */
        if (NULL != filename) {

            /* Check if the OTA handle must be opened */
            if (0 == index) {

                /* Open the OTA handle */
                if (MENDER_OK != (ret = mender_client_callbacks.ota_begin(filename, size, &mender_client_ota_handle))) {
                    mender_log_error("Unable to open OTA handle");
                    goto END;
                }
            }

            /* Write data */
            if (MENDER_OK != (ret = mender_client_callbacks.ota_write(mender_client_ota_handle, data, index, length))) {
                mender_log_error("Unable to write OTA data");
                goto END;
            }

            /* Check if the OTA handle must be closed */
            if (index + length >= size) {

                /* Close the OTA handle */
                if (MENDER_OK != (ret = mender_client_callbacks.ota_end(mender_client_ota_handle))) {
                    mender_log_error("Unable to close OTA handle");
                    goto END;
                }
            }
        }

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
#ifdef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

    } else if (!strcmp(type, "mender-configure")) {

        /* Check meta-data */
        if (NULL != meta_data) {

            /* Record the configuration */
            char *device_config = cJSON_PrintUnformatted(meta_data);
            if (NULL != device_config) {
                if (MENDER_OK != (ret = mender_storage_set_device_config(device_config))) {
                    mender_log_error("Unable to record configuration");
                }
                free(device_config);
            } else {
                mender_log_error("Unable to format configuration");
                ret = MENDER_FAIL;
            }

        } else {

            /* Delete the configuration */
            if (MENDER_OK != (ret = mender_storage_delete_device_config())) {
                mender_log_error("Unable to delete configuration");
            }
        }

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

    } else {

        /* Content is not supported by the mender-mcu-client */
        mender_log_error("Unable to handle artifact type '%s'", type);
        ret = MENDER_FAIL;
    }

END:

    return ret;
}

static mender_err_t
mender_client_publish_deployment_status(char *id, mender_deployment_status_t deployment_status) {

    assert(NULL != id);
    mender_err_t ret;

    /* Publish status to the mender server */
    ret = mender_api_publish_deployment_status(id, deployment_status);

    /* Invoke deployment status callback is defined */
    if (NULL != mender_client_callbacks.deployment_status) {
        mender_client_callbacks.deployment_status(deployment_status, mender_utils_deployment_status_to_string(deployment_status));
    }

    return ret;
}
