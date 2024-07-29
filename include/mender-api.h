/**
 * @file      mender-api.h
 * @brief     Implementation of the Mender API
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

#ifndef __MENDER_API_H__
#define __MENDER_API_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Mender API configuration
 */
typedef struct {
    mender_keystore_t *identity;      /**< Identity of the device */
    char              *artifact_name; /**< Artifact name */
    char              *device_type;   /**< Device type */
    char              *host;          /**< URL of the mender server */
    char              *tenant_token;  /**< Tenant token used to authenticate on the mender server (optional) */
} mender_api_config_t;

/**
 * @brief Initialization of the API
 * @param config Mender API configuration
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_init(mender_api_config_t *config);

/**
 * @brief Perform authentication of the device, retrieve token from mender-server used for the next requests
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_perform_authentication(void);

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

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT

/**
 * @brief Connect the device and make it available to the server
 * @param callback Callback function to be invoked to perform the treatment of the data from the websocket
 * @param handle Connection handle
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_troubleshoot_connect(mender_err_t (*callback)(void *, size_t), void **handle);

/**
 * @brief Send binary data to the server
 * @param handle Connection handle
 * @param payload Payload to send
 * @param length Length of the payload
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_troubleshoot_send(void *handle, void *payload, size_t length);

/**
 * @brief Disconnect the device
 * @param handle Connection handle
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_troubleshoot_disconnect(void *handle);

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_TROUBLESHOOT */

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
