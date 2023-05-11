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

#include <cJSON.h>
#include "mender-api.h"
#include "mender-client.h"
#include "mender-log.h"
#include "mender-rtos.h"
#include "mender-storage.h"
#include "mender-tls.h"
#include "mender-utils.h"

/**
 * @brief Default host
 */
#define MENDER_CLIENT_DEFAULT_HOST "https://hosted.mender.io"

/**
 * @brief Default authentication poll interval (seconds)
 */
#define MENDER_CLIENT_DEFAULT_AUTHENTICATION_POLL_INTERVAL (600)

/**
 * @brief Default update poll interval (seconds)
 */
#define MENDER_CLIENT_DEFAULT_UPDATE_POLL_INTERVAL (1800)

/**
 * @brief Mender client configuration
 */
static mender_client_config_t mender_client_config;

/**
 * @brief Mender client callbacks
 */
static mender_client_callbacks_t mender_client_callbacks;

/**
 * @brief Authentication keys
 */
static unsigned char *mender_client_private_key        = NULL;
static size_t         mender_client_private_key_length = 0;
static unsigned char *mender_client_public_key         = NULL;
static size_t         mender_client_public_key_length  = 0;

/**
 * @brief OTA ID and artifact name, used to report OTA status after rebooting
 */
static char *mender_client_ota_id            = NULL;
static char *mender_client_ota_artifact_name = NULL;

/**
 * @brief Mender client work handles
 */
static void *mender_client_initialization_work_handle = NULL;
static void *mender_client_authentication_work_handle = NULL;
static void *mender_client_update_work_handle         = NULL;

/**
 * @brief OTA handle used to store temporary reference to write OTA data
 */
static void *mender_client_ota_handle = NULL;

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
    if (NULL == (mender_client_config.mac_address = strdup(config->mac_address))) {
        mender_log_error("Unable to save MAC address");
        return MENDER_FAIL;
    }
    if (NULL == (mender_client_config.artifact_name = strdup(config->artifact_name))) {
        mender_log_error("Unable to save artifact name");
        return MENDER_FAIL;
    }
    if (NULL == (mender_client_config.device_type = strdup(config->device_type))) {
        mender_log_error("Unable to save device type");
        return MENDER_FAIL;
    }
    if (NULL != config->host) {
        if (NULL == (mender_client_config.host = strdup(config->host))) {
            mender_log_error("Unable to save host");
            return MENDER_FAIL;
        }
    } else {
        if (NULL == (mender_client_config.host = strdup(MENDER_CLIENT_DEFAULT_HOST))) {
            mender_log_error("Unable to save host");
            return MENDER_FAIL;
        }
    }
    if ((NULL != config->tenant_token) && (strlen(config->tenant_token) > 0)) {
        if (NULL == (mender_client_config.tenant_token = strdup(config->tenant_token))) {
            mender_log_error("Unable to save tenant token");
            return MENDER_FAIL;
        }
    } else {
        mender_client_config.tenant_token = NULL;
    }
    if (0 != config->authentication_poll_interval) {
        mender_client_config.authentication_poll_interval = config->authentication_poll_interval;
    } else {
        mender_client_config.authentication_poll_interval = MENDER_CLIENT_DEFAULT_AUTHENTICATION_POLL_INTERVAL;
    }
    if (0 != config->update_poll_interval) {
        mender_client_config.update_poll_interval = config->update_poll_interval;
    } else {
        mender_client_config.update_poll_interval = MENDER_CLIENT_DEFAULT_UPDATE_POLL_INTERVAL;
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

    /* Create mender client works */
    mender_rtos_work_params_t initialization_work_params;
    initialization_work_params.function = mender_client_initialization_work_function;
    initialization_work_params.period   = mender_client_config.authentication_poll_interval;
    initialization_work_params.name     = "mender_client_initialization";
    if (MENDER_OK != (ret = mender_rtos_work_create(&initialization_work_params, &mender_client_initialization_work_handle))) {
        mender_log_error("Unable to create initialization work");
        return ret;
    }
    mender_rtos_work_params_t authentication_work_params;
    authentication_work_params.function = mender_client_authentication_work_function;
    authentication_work_params.period   = mender_client_config.authentication_poll_interval;
    authentication_work_params.name     = "mender_client_authentication";
    if (MENDER_OK != (ret = mender_rtos_work_create(&authentication_work_params, &mender_client_authentication_work_handle))) {
        mender_log_error("Unable to create authentication work");
        return ret;
    }
    mender_rtos_work_params_t update_work_params;
    update_work_params.function = mender_client_update_work_function;
    update_work_params.period   = mender_client_config.update_poll_interval;
    update_work_params.name     = "mender_client_update";
    if (MENDER_OK != (ret = mender_rtos_work_create(&update_work_params, &mender_client_update_work_handle))) {
        mender_log_error("Unable to create update work");
        return ret;
    }

    /* Activate initialization work */
    if (MENDER_OK != (ret = mender_rtos_work_activate(mender_client_initialization_work_handle))) {
        mender_log_error("Unable to activate initialization work");
        return ret;
    }

    return MENDER_OK;
}

mender_err_t
mender_client_exit(void) {

    /* Deactivate mender client works */
    mender_rtos_work_deactivate(mender_client_initialization_work_handle);
    mender_rtos_work_deactivate(mender_client_authentication_work_handle);
    mender_rtos_work_deactivate(mender_client_update_work_handle);

    /* Delete mender client works */
    mender_rtos_work_delete(mender_client_initialization_work_handle);
    mender_client_initialization_work_handle = NULL;
    mender_rtos_work_delete(mender_client_authentication_work_handle);
    mender_client_authentication_work_handle = NULL;
    mender_rtos_work_delete(mender_client_update_work_handle);
    mender_client_update_work_handle = NULL;

    /* Release all modules */
    mender_api_exit();
    mender_tls_exit();
    mender_storage_exit();
    mender_log_exit();
    mender_rtos_exit();

    /* Release memory */
    if (NULL != mender_client_config.mac_address) {
        free(mender_client_config.mac_address);
        mender_client_config.mac_address = NULL;
    }
    if (NULL != mender_client_config.artifact_name) {
        free(mender_client_config.artifact_name);
        mender_client_config.artifact_name = NULL;
    }
    if (NULL != mender_client_config.device_type) {
        free(mender_client_config.device_type);
        mender_client_config.device_type = NULL;
    }
    if (NULL != mender_client_config.host) {
        free(mender_client_config.host);
        mender_client_config.host = NULL;
    }
    if (NULL != mender_client_config.tenant_token) {
        free(mender_client_config.tenant_token);
        mender_client_config.tenant_token = NULL;
    }
    mender_client_config.authentication_poll_interval = 0;
    mender_client_config.update_poll_interval         = 0;
    if (NULL != mender_client_private_key) {
        free(mender_client_private_key);
        mender_client_private_key = NULL;
    }
    mender_client_private_key_length = 0;
    if (NULL != mender_client_public_key) {
        free(mender_client_public_key);
        mender_client_public_key = NULL;
    }
    mender_client_public_key_length = 0;
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
mender_client_initialization_work_function(void) {

    mender_err_t ret;

    /* Check if recommissioning is forced */
    if (true == mender_client_config.recommissioning) {

        /* Erase authentication keys */
        mender_log_info("Erasing authentication keys...");
        if (MENDER_OK != mender_storage_erase_authentication_keys()) {
            mender_log_warning("Unable to erase authentication keys");
        }
    }

    /* Retrieve or generate authentication keys if not allready done */
    if (MENDER_OK
        != mender_storage_get_authentication_keys(
            &mender_client_private_key, &mender_client_private_key_length, &mender_client_public_key, &mender_client_public_key_length)) {

        /* Generate authentication keys */
        mender_log_info("Generating authentication keys...");
        if (MENDER_OK
            != (ret = mender_tls_generate_authentication_keys(
                    &mender_client_private_key, &mender_client_private_key_length, &mender_client_public_key, &mender_client_public_key_length))) {
            mender_log_error("Unable to generate authentication keys");
            return ret;
        }

        /* Record keys */
        if (MENDER_OK
            != (ret = mender_storage_set_authentication_keys(
                    mender_client_private_key, mender_client_private_key_length, mender_client_public_key, mender_client_public_key_length))) {
            mender_log_error("Unable to record authentication keys");
            return ret;
        }
    }

    /* Retrieve OTA ID if it is found (following an update) */
    size_t ota_id_length            = 0;
    size_t ota_artifact_name_length = 0;
    if (MENDER_OK
        != (ret = mender_storage_get_ota_deployment(&mender_client_ota_id, &ota_id_length, &mender_client_ota_artifact_name, &ota_artifact_name_length))) {
        if (MENDER_NOT_FOUND != ret) {
            mender_log_error("Unable to get OTA ID");
            return ret;
        }
    }

    /* Activate authentication work */
    if (MENDER_OK != (ret = mender_rtos_work_activate(mender_client_authentication_work_handle))) {
        mender_log_error("Unable to activate authentication work");
        return ret;
    }

    return MENDER_DONE;
}

static mender_err_t
mender_client_authentication_work_function(void) {

    mender_err_t ret;

    /* Perform authentication with the mender server */
    if (MENDER_OK
        != (ret = mender_api_perform_authentication(
                mender_client_private_key, mender_client_private_key_length, mender_client_public_key, mender_client_public_key_length))) {

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

        /* Check if artifact running is the pending one, fails if rollback occurred */
        if ((NULL != mender_client_ota_artifact_name) && (strcmp(mender_client_ota_artifact_name, mender_client_config.artifact_name))) {

            /* Publish deployment status failure */
            mender_client_publish_deployment_status(mender_client_ota_id, MENDER_DEPLOYMENT_STATUS_FAILURE);

        } else {

            /* Publish deployment status success */
            mender_client_publish_deployment_status(mender_client_ota_id, MENDER_DEPLOYMENT_STATUS_SUCCESS);
        }

        /* Clear pending OTA */
        mender_storage_clear_ota_deployment();
    }

    /* Activate update work */
    if (MENDER_OK != (ret = mender_rtos_work_activate(mender_client_update_work_handle))) {
        mender_log_error("Unable to activate update work");
        return ret;
    }

    return MENDER_DONE;

REBOOT:

    /* Invoke restart callback, application is responsible to shutdown properly and restart the system */
    if (NULL != mender_client_callbacks.restart) {
        mender_client_callbacks.restart();
    }

    return MENDER_DONE;
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
