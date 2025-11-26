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

#include "mender-http.h"
#include "mender-utils.h"

/**
 * @brief Mender API configuration
 */
typedef struct {
    mender_keystore_t *identity;      /**< Identity of the device */
    char              *artifact_name; /**< Artifact name */
    char const        *device_type;   /**< Device type */
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
 * @brief Retrieve authentification token from the mender-server
 * @return Authentication token from the mender-server if the function succeeds, NULL otherwise
 */
char *mender_api_get_authentication_token(void);

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

/**
 * @brief HTTP callback used to handle text content
 * @param event HTTP client event
 * @param data Data received
 * @param data_length Data length
 * @param params Callback parameters
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_http_text_callback(mender_http_client_event_t event, void *data, size_t data_length, void *params);

/**
 * @brief HTTP callback used to handle artifact content
 * @param event HTTP client event
 * @param data Data received
 * @param data_length Data length
 * @param params Callback parameters
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_http_artifact_callback(mender_http_client_event_t event, void *data, size_t data_length, void *params);

/**
 * @brief Print response error
 * @param response HTTP response, NULL if not available
 * @param status HTTP status
 */
void mender_api_print_response_error(char *response, int status);

/**
 * @brief Release mender API
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_api_exit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_API_H__ */
