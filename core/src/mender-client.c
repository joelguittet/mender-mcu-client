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
#include "mender-utils.h"

/**
 * @brief Default host
 */
#define MENDER_CLIENT_DEFAULT_HOST "https://hosted.mender.io"

/**
 * @brief Default authentication poll interval (seconds)
 */
#define MENDER_CLIENT_DEFAULT_AUTHENTICATION_POLL_INTERVAL (60)

/**
 * @brief Default inventory poll interval (seconds)
 */
#define MENDER_CLIENT_DEFAULT_INVENTORY_POLL_INTERVAL (28800)

/**
 * @brief Default update poll interval (seconds)
 */
#define MENDER_CLIENT_DEFAULT_UPDATE_POLL_INTERVAL (1800)

/**
 * @brief Default restart poll interval (seconds)
 */
#define MENDER_CLIENT_DEFAULT_RESTART_POLL_INTERVAL (60)

/**
 * @brief Mender client task size configuration (10% margin included)
 */
#ifndef MENDER_CLIENT_TASK_STACK_SIZE
#define MENDER_CLIENT_TASK_STACK_SIZE (14 * 1024)
#endif

/**
 * @brief Mender client task priority configuration
 */
#ifndef MENDER_CLIENT_TASK_PRIORITY
#define MENDER_CLIENT_TASK_PRIORITY (5)
#endif

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
 * @brief Mender client inventory
 */
static mender_inventory_t *mender_client_inventory        = NULL;
static size_t              mender_client_inventory_length = 0;
static void *              mender_client_inventory_mutex  = NULL;

/**
 * @brief Mender client task handle
 */
static void *mender_client_task_handle = NULL;

/**
 * @brief Mender client task
 * @param arg Argument
 */
static void mender_client_task(void *arg);

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
    if (0 != config->inventory_poll_interval) {
        mender_client_config.inventory_poll_interval = config->inventory_poll_interval;
    } else {
        mender_client_config.inventory_poll_interval = MENDER_CLIENT_DEFAULT_INVENTORY_POLL_INTERVAL;
    }
    if (0 != config->update_poll_interval) {
        mender_client_config.update_poll_interval = config->update_poll_interval;
    } else {
        mender_client_config.update_poll_interval = MENDER_CLIENT_DEFAULT_UPDATE_POLL_INTERVAL;
    }
    if (0 != config->restart_poll_interval) {
        mender_client_config.restart_poll_interval = config->restart_poll_interval;
    } else {
        mender_client_config.restart_poll_interval = MENDER_CLIENT_DEFAULT_RESTART_POLL_INTERVAL;
    }
    mender_client_config.recommissioning = config->recommissioning;

    /* Save callbacks */
    memcpy(&mender_client_callbacks, callbacks, sizeof(mender_client_callbacks_t));

    /* Initializations */
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
    mender_api_callbacks_t mender_api_callbacks = {
        .ota_begin = mender_client_callbacks.ota_begin,
        .ota_write = mender_client_callbacks.ota_write,
        .ota_abort = mender_client_callbacks.ota_abort,
        .ota_end   = mender_client_callbacks.ota_end,
    };
    if (MENDER_OK != (ret = mender_api_init(&mender_api_config, &mender_api_callbacks))) {
        mender_log_error("Unable to initialize API");
        return ret;
    }

    /* Create inventory mutex */
    if (NULL == (mender_client_inventory_mutex = mender_rtos_semaphore_create_mutex())) {
        mender_log_error("Unable to create inventory mutex");
        return MENDER_FAIL;
    }

    /* Create mender client task */
    mender_rtos_task_create(mender_client_task, "mender_client", MENDER_CLIENT_TASK_STACK_SIZE, NULL, MENDER_CLIENT_TASK_PRIORITY, &mender_client_task_handle);

    return MENDER_OK;
}

mender_err_t
mender_client_set_inventory(mender_inventory_t *inventory, size_t inventory_length) {

    mender_rtos_semaphore_take(mender_client_inventory_mutex, -1);

    /* Release previous inventory */
    if (NULL != mender_client_inventory) {
        for (size_t index = 0; index < mender_client_inventory_length; index++) {
            if (NULL != mender_client_inventory[index].name) {
                free(mender_client_inventory[index].name);
            }
            if (NULL != mender_client_inventory[index].value) {
                free(mender_client_inventory[index].value);
            }
        }
        free(mender_client_inventory);
        mender_client_inventory = NULL;
    }

    /* Copy the new inventory */
    if (NULL == (mender_client_inventory = (mender_inventory_t *)malloc(inventory_length * sizeof(mender_inventory_t)))) {
        mender_log_error("Unable to allocate memory");
        mender_rtos_semaphore_give(mender_client_inventory_mutex);
        return MENDER_FAIL;
    }
    memset(mender_client_inventory, 0, inventory_length * sizeof(mender_inventory_t));
    for (size_t index = 0; index < inventory_length; index++) {
        if (NULL == (mender_client_inventory[index].name = strdup(inventory[index].name))) {
            mender_log_error("Unable to allocate memory");
        }
        if (NULL == (mender_client_inventory[index].value = strdup(inventory[index].value))) {
            mender_log_error("Unable to allocate memory");
        }
    }
    mender_client_inventory_length = inventory_length;

    mender_rtos_semaphore_give(mender_client_inventory_mutex);

    return MENDER_OK;
}

mender_err_t
mender_client_exit(void) {

    /* Stop mender-client task */
    mender_rtos_task_delete(mender_client_task_handle);
    mender_client_task_handle = NULL;

    /* Release all modules */
    mender_api_exit();
    mender_tls_exit();
    mender_storage_exit();
    mender_log_exit();

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
    mender_client_config.inventory_poll_interval      = 0;
    mender_client_config.update_poll_interval         = 0;
    mender_client_config.restart_poll_interval        = 0;
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
    if (NULL != mender_client_inventory) {
        for (size_t index = 0; index < mender_client_inventory_length; index++) {
            if (NULL != mender_client_inventory[index].name) {
                free(mender_client_inventory[index].name);
            }
            if (NULL != mender_client_inventory[index].value) {
                free(mender_client_inventory[index].value);
            }
        }
        free(mender_client_inventory);
        mender_client_inventory = NULL;
    }
    mender_client_inventory_length = 0;
    mender_rtos_semaphore_delete(mender_client_inventory_mutex);

    return MENDER_OK;
}

static void
mender_client_task(void *arg) {

    (void)arg;

    /* Check if recommissioning is forced */
    if (true == mender_client_config.recommissioning) {

        /* Erase authentication keys */
        mender_log_info("Erasing authentication keys...");
        if (MENDER_OK != mender_storage_erase_authentication_keys()) {
            mender_log_error("Unable to erase authentication keys");
        }
    }

    /* Retrieve or generate authentication keys if not allready done */
    if (MENDER_OK
        != mender_storage_get_authentication_keys(
            &mender_client_private_key, &mender_client_private_key_length, &mender_client_public_key, &mender_client_public_key_length)) {

        /* Generate authentication keys */
        mender_log_info("Generating authentication keys...");
        if (MENDER_OK
            != mender_tls_generate_authentication_keys(
                &mender_client_private_key, &mender_client_private_key_length, &mender_client_public_key, &mender_client_public_key_length)) {
            mender_log_error("Unable to generate authentication keys");
            goto END;
        }

        /* Record keys */
        if (MENDER_OK
            != mender_storage_set_authentication_keys(
                mender_client_private_key, mender_client_private_key_length, mender_client_public_key, mender_client_public_key_length)) {
            mender_log_error("Unable to record authentication keys");
            goto END;
        }
    }

    /* Retrieve OTA ID if it is found (following an update) */
    mender_err_t ret;
    size_t       ota_id_length            = 0;
    size_t       ota_artifact_name_length = 0;
    if (MENDER_OK
        != (ret = mender_storage_get_ota_deployment(&mender_client_ota_id, &ota_id_length, &mender_client_ota_artifact_name, &ota_artifact_name_length))) {
        if (MENDER_NOT_FOUND != ret) {
            mender_log_error("Unable to get OTA ID");
            goto END;
        }
    }

    /* Loop until authentication is done or rebooting is required */
    unsigned long last_wake_time = 0;
    mender_rtos_delay_until_init(&last_wake_time);
    while (true) {

        /* Perform authentication with the mender server */
        if (MENDER_OK
            != mender_api_perform_authentication(
                mender_client_private_key, mender_client_private_key_length, mender_client_public_key, mender_client_public_key_length)) {

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

        } else {

            /* Invoke authentication success callback */
            if (NULL != mender_client_callbacks.authentication_success) {
                if (MENDER_OK != mender_client_callbacks.authentication_success()) {

                    /* Check if OTA is pending */
                    if ((NULL != mender_client_ota_id) && (NULL != mender_client_ota_artifact_name)) {
                        /* Authentication sucess callback inform the reboot should be done, probably something is broken and it prefers to rollback */
                        mender_log_error("Authentication success callback failed, rebooting");
                        goto REBOOT;
                    }

                } else {

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
                }
            }
            break;
        }

        /* Wait before trying again */
        mender_rtos_delay_until_s(&last_wake_time, mender_client_config.authentication_poll_interval);
    }

    /* Loop until rebooting is required */
    uint32_t inventory_timeout            = 0;
    uint32_t check_for_deployment_timeout = 0;
    mender_rtos_delay_until_init(&last_wake_time);
    while (true) {

        /* Check if it's time to send inventory */
        if (0 == inventory_timeout) {

            /* Publish inventory */
            mender_rtos_semaphore_take(mender_client_inventory_mutex, -1);
            if (MENDER_OK != mender_api_publish_inventory_data(mender_client_inventory, mender_client_inventory_length)) {
                mender_log_error("Unable to publish inventory data");
            }
            mender_rtos_semaphore_give(mender_client_inventory_mutex);

            /* Reload timeout */
            inventory_timeout = mender_client_config.inventory_poll_interval;
        }

        /* Check if it's time to check for deployment */
        if (0 == check_for_deployment_timeout) {

            /* Check for deployment */
            char *id            = NULL;
            char *artifact_name = NULL;
            char *uri           = NULL;
            if (MENDER_OK != mender_api_check_for_deployment(&id, &artifact_name, &uri)) {
                mender_log_error("Unable to check for deployment");
            } else if ((NULL != id) && (NULL != artifact_name) && (NULL != uri)) {
                /* Check if artifact is already installed (should not occur) */
                if (!strcmp(artifact_name, mender_client_config.artifact_name)) {
                    mender_log_error("Artifact is already installed");
                    mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
                } else {
                    /* Download deployment artifact */
                    mender_log_info("Downloading deployment artifact with id '%s', artifact name '%s' and uri '%s'", id, artifact_name, uri);
                    mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_DOWNLOADING);
                    if (MENDER_OK != mender_api_download_artifact(uri)) {
                        mender_log_error("Unable to download artifact");
                        mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
                    } else {
                        mender_log_info("Download done, installing artifact");
                        mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_INSTALLING);
                        /* Set boot partition */
                        if (MENDER_OK != mender_client_callbacks.ota_set_boot_partition()) {
                            mender_log_error("Unable to set boot partition");
                            mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
                        } else {
                            /* Save OTA ID to publish deployment status after rebooting */
                            if (MENDER_OK != mender_storage_set_ota_deployment(id, artifact_name)) {
                                mender_log_error("Unable to save OTA ID");
                                mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_FAILURE);
                            } else {
                                /* Now need to reboot to apply the update */
                                mender_client_publish_deployment_status(id, MENDER_DEPLOYMENT_STATUS_REBOOTING);
                                goto REBOOT;
                            }
                        }
                    }
                }
            } else {
                mender_log_info("No deployment available");
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

            /* Reload timeout */
            check_for_deployment_timeout = mender_client_config.update_poll_interval;
        }

        /* Management of timeouts */
        uint32_t delay = (check_for_deployment_timeout <= inventory_timeout) ? check_for_deployment_timeout : inventory_timeout;
        mender_rtos_delay_until_s(&last_wake_time, delay);
        check_for_deployment_timeout -= delay;
        inventory_timeout -= delay;
    }

REBOOT:

    /* Infinite loop, when reaching this point this means you are waiting to reboot */
    mender_rtos_delay_until_init(&last_wake_time);
    while (true) {

        /* Invoke restart callback */
        if (NULL != mender_client_callbacks.restart) {
            mender_client_callbacks.restart();
        }

        /* Wait before trying again */
        mender_rtos_delay_until_s(&last_wake_time, mender_client_config.restart_poll_interval);
    }

END:

    /* Delete myself */
    mender_rtos_task_delete(NULL);
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
