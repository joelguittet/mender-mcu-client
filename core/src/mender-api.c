/**
 * @file      mender-api.c
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

#include "mender-api.h"
#include "mender-artifact.h"
#include "mender-http.h"
#include "mender-log.h"
#include "mender-tls.h"

/**
 * @brief Paths of the mender-server APIs
 */
#define MENDER_API_PATH_POST_AUTHENTICATION_REQUESTS "/api/devices/v1/authentication/auth_requests"
#define MENDER_API_PATH_GET_NEXT_DEPLOYMENT          "/api/devices/v1/deployments/device/deployments/next"
#define MENDER_API_PATH_PUT_DEPLOYMENT_STATUS        "/api/devices/v1/deployments/device/deployments/%s/status"

/**
 * @brief Mender API configuration
 */
static mender_api_config_t mender_api_config;

/**
 * @brief Authentication token
 */
static char *mender_api_jwt = NULL;

mender_err_t
mender_api_init(mender_api_config_t *config) {

    assert(NULL != config);
    assert(NULL != config->identity);
    assert(NULL != config->artifact_name);
    assert(NULL != config->device_type);
    assert(NULL != config->host);
    mender_err_t ret;

    /* Save configuration */
    memcpy(&mender_api_config, config, sizeof(mender_api_config_t));

    /* Initializations */
    mender_http_config_t mender_http_config = { .host = mender_api_config.host };
    if (MENDER_OK != (ret = mender_http_init(&mender_http_config))) {
        mender_log_error("Unable to initialize HTTP");
        return ret;
    }

    return ret;
}

mender_err_t
mender_api_perform_authentication(void) {

    mender_err_t ret;
    char        *public_key_pem   = NULL;
    cJSON       *json_identity    = NULL;
    char        *identity         = NULL;
    cJSON       *json_payload     = NULL;
    char        *payload          = NULL;
    char        *response         = NULL;
    char        *signature        = NULL;
    size_t       signature_length = 0;
    int          status           = 0;

    /* Get public key in PEM format */
    if (MENDER_OK != (ret = mender_tls_get_public_key_pem(&public_key_pem))) {
        mender_log_error("Unable to get public key");
        goto END;
    }

    /* Format identity */
    if (MENDER_OK != (ret = mender_utils_keystore_to_json(mender_api_config.identity, &json_identity))) {
        mender_log_error("Unable to format identity");
        goto END;
    }
    if (NULL == (identity = cJSON_PrintUnformatted(json_identity))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Format payload */
    if (NULL == (json_payload = cJSON_CreateObject())) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    cJSON_AddStringToObject(json_payload, "id_data", identity);
    cJSON_AddStringToObject(json_payload, "pubkey", public_key_pem);
    if (NULL != mender_api_config.tenant_token) {
        cJSON_AddStringToObject(json_payload, "tenant_token", mender_api_config.tenant_token);
    }
    if (NULL == (payload = cJSON_PrintUnformatted(json_payload))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Sign payload */
    if (MENDER_OK != (ret = mender_tls_sign_payload(payload, &signature, &signature_length))) {
        mender_log_error("Unable to sign payload");
        goto END;
    }

    /* Perform HTTP request */
    if (MENDER_OK
        != (ret = mender_http_perform(NULL,
                                      MENDER_API_PATH_POST_AUTHENTICATION_REQUESTS,
                                      MENDER_HTTP_POST,
                                      payload,
                                      signature,
                                      &mender_api_http_text_callback,
                                      (void *)&response,
                                      &status))) {
        mender_log_error("Unable to perform HTTP request");
        goto END;
    }

    /* Treatment depending of the status */
    if (200 == status) {
        if (NULL == response) {
            mender_log_error("Response is empty");
            ret = MENDER_FAIL;
            goto END;
        }
        if (NULL != mender_api_jwt) {
            free(mender_api_jwt);
        }
        if (NULL == (mender_api_jwt = strdup(response))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        ret = MENDER_OK;
    } else {
        mender_api_print_response_error(response, status);
        ret = MENDER_FAIL;
    }

END:

    /* Release memory */
    if (NULL != response) {
        free(response);
    }
    if (NULL != signature) {
        free(signature);
    }
    if (NULL != payload) {
        free(payload);
    }
    if (NULL != json_payload) {
        cJSON_Delete(json_payload);
    }
    if (NULL != identity) {
        free(identity);
    }
    if (NULL != json_identity) {
        cJSON_Delete(json_identity);
    }
    if (NULL != public_key_pem) {
        free(public_key_pem);
    }

    return ret;
}

char *
mender_api_get_authentication_token(void) {

    /* Return authentication token */
    return mender_api_jwt;
}

mender_err_t
mender_api_check_for_deployment(char **id, char **artifact_name, char **uri) {

    assert(NULL != id);
    assert(NULL != artifact_name);
    assert(NULL != uri);
    mender_err_t ret;
    char        *path     = NULL;
    char        *response = NULL;
    int          status   = 0;

    /* Compute path */
    size_t str_length = strlen("?artifact_name=&device_type=") + strlen(MENDER_API_PATH_GET_NEXT_DEPLOYMENT) + strlen(mender_api_config.artifact_name)
                        + strlen(mender_api_config.device_type) + 1;
    if (NULL == (path = (char *)malloc(str_length))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    snprintf(path,
             str_length,
             "%s?artifact_name=%s&device_type=%s",
             MENDER_API_PATH_GET_NEXT_DEPLOYMENT,
             mender_api_config.artifact_name,
             mender_api_config.device_type);

    /* Perform HTTP request */
    if (MENDER_OK
        != (ret = mender_http_perform(mender_api_jwt, path, MENDER_HTTP_GET, NULL, NULL, &mender_api_http_text_callback, (void *)&response, &status))) {
        mender_log_error("Unable to perform HTTP request");
        goto END;
    }

    /* Treatment depending of the status */
    if (200 == status) {
        cJSON *json_response = cJSON_Parse(response);
        if (NULL != json_response) {
            cJSON *json_id = cJSON_GetObjectItem(json_response, "id");
            if (NULL != json_id) {
                if (NULL == (*id = strdup(cJSON_GetStringValue(json_id)))) {
                    ret = MENDER_FAIL;
                    goto END;
                }
            }
            cJSON *json_artifact = cJSON_GetObjectItem(json_response, "artifact");
            if (NULL != json_artifact) {
                cJSON *json_artifact_name = cJSON_GetObjectItem(json_artifact, "artifact_name");
                if (NULL != json_artifact_name) {
                    if (NULL == (*artifact_name = strdup(cJSON_GetStringValue(json_artifact_name)))) {
                        ret = MENDER_FAIL;
                        goto END;
                    }
                }
                cJSON *json_source = cJSON_GetObjectItem(json_artifact, "source");
                if (NULL != json_source) {
                    cJSON *json_uri = cJSON_GetObjectItem(json_source, "uri");
                    if (NULL != json_uri) {
                        if (NULL == (*uri = strdup(cJSON_GetStringValue(json_uri)))) {
                            ret = MENDER_FAIL;
                            goto END;
                        }
                        ret = MENDER_OK;
                    } else {
                        mender_log_error("Invalid response");
                        ret = MENDER_FAIL;
                    }
                } else {
                    mender_log_error("Invalid response");
                    ret = MENDER_FAIL;
                }
            } else {
                mender_log_error("Invalid response");
                ret = MENDER_FAIL;
            }
            cJSON_Delete(json_response);
        } else {
            mender_log_error("Invalid response");
            ret = MENDER_FAIL;
        }
    } else if (204 == status) {
        /* No response expected */
        ret = MENDER_OK;
    } else {
        mender_api_print_response_error(response, status);
        ret = MENDER_FAIL;
    }

END:

    /* Release memory */
    if (NULL != response) {
        free(response);
    }
    if (NULL != path) {
        free(path);
    }

    return ret;
}

mender_err_t
mender_api_publish_deployment_status(char *id, mender_deployment_status_t deployment_status) {

    assert(NULL != id);
    mender_err_t ret;
    char        *value        = NULL;
    cJSON       *json_payload = NULL;
    char        *payload      = NULL;
    char        *path         = NULL;
    char        *response     = NULL;
    int          status       = 0;

    /* Deployment status to string */
    if (NULL == (value = mender_utils_deployment_status_to_string(deployment_status))) {
        mender_log_error("Invalid status");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Format payload */
    if (NULL == (json_payload = cJSON_CreateObject())) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    cJSON_AddStringToObject(json_payload, "status", value);
    if (NULL == (payload = cJSON_PrintUnformatted(json_payload))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Compute path */
    size_t str_length = strlen(MENDER_API_PATH_PUT_DEPLOYMENT_STATUS) - strlen("%s") + strlen(id) + 1;
    if (NULL == (path = (char *)malloc(str_length))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    snprintf(path, str_length, MENDER_API_PATH_PUT_DEPLOYMENT_STATUS, id);

    /* Perform HTTP request */
    if (MENDER_OK
        != (ret = mender_http_perform(mender_api_jwt, path, MENDER_HTTP_PUT, payload, NULL, &mender_api_http_text_callback, (void *)&response, &status))) {
        mender_log_error("Unable to perform HTTP request");
        goto END;
    }

    /* Treatment depending of the status */
    if (204 == status) {
        /* No response expected */
        ret = MENDER_OK;
    } else {
        mender_api_print_response_error(response, status);
        ret = MENDER_FAIL;
    }

END:

    /* Release memory */
    if (NULL != response) {
        free(response);
    }
    if (NULL != path) {
        free(path);
    }
    if (NULL != payload) {
        free(payload);
    }
    if (NULL != json_payload) {
        cJSON_Delete(json_payload);
    }

    return ret;
}

mender_err_t
mender_api_download_artifact(char *uri, mender_err_t (*callback)(char *, cJSON *, char *, size_t, void *, size_t, size_t)) {

    assert(NULL != uri);
    assert(NULL != callback);
    mender_err_t ret;
    int          status = 0;

    /* Perform HTTP request */
    if (MENDER_OK != (ret = mender_http_perform(NULL, uri, MENDER_HTTP_GET, NULL, NULL, &mender_api_http_artifact_callback, callback, &status))) {
        mender_log_error("Unable to perform HTTP request");
        goto END;
    }

    /* Treatment depending of the status */
    if (200 == status) {
        /* Nothing to do */
        ret = MENDER_OK;
    } else {
        mender_api_print_response_error(NULL, status);
        ret = MENDER_FAIL;
    }

END:

    return ret;
}

mender_err_t
mender_api_http_text_callback(mender_http_client_event_t event, void *data, size_t data_length, void *params) {

    assert(NULL != params);
    char       **response = (char **)params;
    mender_err_t ret      = MENDER_OK;
    char        *tmp;

    /* Treatment depending of the event */
    switch (event) {
        case MENDER_HTTP_EVENT_CONNECTED:
            /* Nothing to do */
            break;
        case MENDER_HTTP_EVENT_DATA_RECEIVED:
            /* Check input data */
            if ((NULL == data) || (0 == data_length)) {
                mender_log_error("Invalid data received");
                ret = MENDER_FAIL;
                break;
            }
            /* Concatenate data to the response */
            size_t response_length = (NULL != *response) ? strlen(*response) : 0;
            if (NULL == (tmp = realloc(*response, response_length + data_length + 1))) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                break;
            }
            *response = tmp;
            memcpy((*response) + response_length, data, data_length);
            *((*response) + response_length + data_length) = '\0';
            break;
        case MENDER_HTTP_EVENT_DISCONNECTED:
            /* Nothing to do */
            break;
        case MENDER_HTTP_EVENT_ERROR:
            /* Downloading the response fails */
            mender_log_error("An error occurred");
            ret = MENDER_FAIL;
            break;
        default:
            /* Should no occur */
            ret = MENDER_FAIL;
            break;
    }

    return ret;
}

mender_err_t
mender_api_http_artifact_callback(mender_http_client_event_t event, void *data, size_t data_length, void *params) {

    assert(NULL != params);
    mender_err_t ret = MENDER_OK;

    /* Artifact context */
    static mender_artifact_ctx_t *mender_artifact_ctx = NULL;

    /* Treatment depending of the event */
    switch (event) {
        case MENDER_HTTP_EVENT_CONNECTED:
            /* Create new artifact context */
            if (NULL == (mender_artifact_ctx = mender_artifact_create_ctx())) {
                mender_log_error("Unable to create artifact context");
                ret = MENDER_FAIL;
                break;
            }
            break;
        case MENDER_HTTP_EVENT_DATA_RECEIVED:
            /* Check input data */
            if ((NULL == data) || (0 == data_length)) {
                mender_log_error("Invalid data received");
                ret = MENDER_FAIL;
                break;
            }
            /* Check artifact context */
            if (NULL == mender_artifact_ctx) {
                mender_log_error("Invalid artifact context");
                ret = MENDER_FAIL;
                break;
            }
            /* Parse input data */
            if (MENDER_OK != (ret = mender_artifact_process_data(mender_artifact_ctx, data, data_length, params))) {
                mender_log_error("Unable to process data");
                break;
            }
            break;
        case MENDER_HTTP_EVENT_DISCONNECTED:
            /* Release artifact context */
            mender_artifact_release_ctx(mender_artifact_ctx);
            mender_artifact_ctx = NULL;
            break;
        case MENDER_HTTP_EVENT_ERROR:
            /* Downloading the artifact fails */
            mender_log_error("An error occurred");
            ret = MENDER_FAIL;
            /* Release artifact context */
            mender_artifact_release_ctx(mender_artifact_ctx);
            mender_artifact_ctx = NULL;
            break;
        default:
            /* Should not occur */
            ret = MENDER_FAIL;
            break;
    }

    return ret;
}

void
mender_api_print_response_error(char *response, int status) {

    char *desc;

    /* Treatment depending of the status */
    if (NULL != (desc = mender_utils_http_status_to_string(status))) {
        if (NULL != response) {
            cJSON *json_response = cJSON_Parse(response);
            if (NULL != json_response) {
                cJSON *json_error = cJSON_GetObjectItemCaseSensitive(json_response, "error");
                if (NULL != json_error) {
                    mender_log_error("[%d] %s: %s", status, desc, cJSON_GetStringValue(json_error));
                } else {
                    mender_log_error("[%d] %s: unknown error", status, desc);
                }
                cJSON_Delete(json_response);
            } else {
                mender_log_error("[%d] %s: unknown error", status, desc);
            }
        } else {
            mender_log_error("[%d] %s: unknown error", status, desc);
        }
    } else {
        mender_log_error("Unknown error occurred, status=%d", status);
    }
}

mender_err_t
mender_api_exit(void) {

    /* Release all modules */
    mender_http_exit();

    /* Release memory */
    if (NULL != mender_api_jwt) {
        free(mender_api_jwt);
        mender_api_jwt = NULL;
    }

    return MENDER_OK;
}
