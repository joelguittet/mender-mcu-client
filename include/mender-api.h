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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <cJSON.h>
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
 * @brief Initialization of the API
 * @param config Mender API configuration
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_init(mender_api_config_t *config);

/**
 * @brief Perform authentication of the device, retrieve token from mender-server used for the next requests
 * @param private_key Client private key used for authentication
 * @param private_key_length Private key length
 * @param public_key Client public key used for authentication
 * @param public_key_length Public key length
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_perform_authentication(unsigned char *private_key, size_t private_key_length, unsigned char *public_key, size_t public_key_length);

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
 * @param callback Callback function to be invoked to perform the treatment of the data from the artifact
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_download_artifact(char *uri, mender_err_t (*callback)(char *, cJSON *, char *, size_t, void *, size_t, size_t));

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

/**
 * @brief Download configure data of the device from the mender-server
 * @param configuration Mender configuration key/value pairs table, ends with a NULL/NULL element, NULL if not defined
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_download_configuration_data(mender_keystore_t **configuration);

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

/**
 * @brief Publish configure data of the device to the mender-server
 * @param configuration Mender configuration key/value pairs table, must end with a NULL/NULL element, NULL if not defined
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_publish_configuration_data(mender_keystore_t *configuration);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY

/**
 * @brief Publish inventory data of the device to the mender-server
 * @param inventory Mender inventory key/value pairs table, must end with a NULL/NULL element, NULL if not defined
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_publish_inventory_data(mender_keystore_t *inventory);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */

/**
 * @brief Release mender API
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_exit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_API_H__ */
