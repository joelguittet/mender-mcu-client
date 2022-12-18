/**
 * @file      mender-api.h
 * @brief     Implementation of the Mender API
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

#ifndef __MENDER_API_H__
#define __MENDER_API_H__

#include "mender-common.h"

/**
 * @brief Mender API configuration
 */
typedef struct {
    char *mac_address;   /**< MAC address of the device */
    char *artifact_name; /**< Artifact name */
    char *device_type;   /**< Device type */
    char *host;          /**< URL of the mender server */
    char *tenant_token;  /**< Tenant token used to authenticate on the mender server (optional) */
} mender_api_config_t;

/**
 * @brief Mender API callbacks
 */
typedef struct {
    mender_err_t (*ota_begin)(char *, size_t, void **); /**< Invoked when the OTA starts */
    mender_err_t (*ota_write)(void *, void *, size_t);  /**< Invoked to write data received from the mender server */
    mender_err_t (*ota_abort)(void *);                  /**< Invoked to abort current OTA */
    mender_err_t (*ota_end)(void *);                    /**< Invoked to indicate the end of the artifact */
} mender_api_callbacks_t;

/**
 * @brief Initialization of the API
 * @param config Mender API configuration
 * @param callbacks Mender API callbacks
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_init(mender_api_config_t *config, mender_api_callbacks_t *callbacks);

/**
 * @brief Perform authentication of the device, retrieve token from mender-server used for the next requests
 * @param private_key Client private key used for authentication
 * @param public_key Client public key used for authentication
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_perform_authentication(char *private_key, char *public_key);

/**
 * @brief Publish inventory data of the device to the mender-server
 * @param inventory Inventory array, NULL if not defined
 * @param inventory_length Length of the inventory array, 0 if empty
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_publish_inventory_data(mender_inventory_t *inventory, size_t inventory_length);

/**
 * @brief Check for deployments for the device from the mender-server
 * @param id ID of the deployment, if one is pending
 * @param artifact_name Artifact name of the deployment, if one is pending
 * @param uri URI of the deployment, if one is pending
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_check_for_deployment(char **id, char **artifact_name, char **uri);

/**
 * @brief Publish deployment status of the device to the mender-server
 * @param id ID of the deployment received from mender_api_check_for_deployment function
 * @param deployment_status Deployment status
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_publish_deployment_status(char *id, mender_deployment_status_t deployment_status);

/**
 * @brief Download artifact from the mender-server
 * @param uri URI of the deployment received from mender_api_check_for_deployment function
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_download_artifact(char *uri);

/**
 * @brief Release mender API
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_exit(void);

#endif /* __MENDER_API_H__ */
