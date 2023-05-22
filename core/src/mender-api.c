/**
 * @file      mender-api.c
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

#include <cJSON.h>
#include "mender-api.h"
#include "mender-artifact.h"
#include "mender-http.h"
#include "mender-log.h"
#include "mender-tls.h"
#include "mender-utils.h"

/**
 * @brief Paths of the mender-server APIs
 */
#define MENDER_API_PATH_POST_AUTHENTICATION_REQUESTS "/api/devices/v1/authentication/auth_requests"
#define MENDER_API_PATH_GET_NEXT_DEPLOYMENT          "/api/devices/v1/deployments/device/deployments/next"
#define MENDER_API_PATH_PUT_DEPLOYMENT_STATUS        "/api/devices/v1/deployments/device/deployments/%s/status"
#define MENDER_API_PATH_GET_DEVICE_CONFIGURATION     "/api/devices/v1/deviceconfig/configuration"
#define MENDER_API_PATH_PUT_DEVICE_CONFIGURATION     "/api/devices/v1/deviceconfig/configuration"
#define MENDER_API_PATH_PUT_DEVICE_ATTRIBUTES        "/api/devices/v1/inventory/device/attributes"

/**
 * @brief Mender API configuration
 */
static mender_api_config_t mender_api_config;

/**
 * @brief Authentication token
 */
static char *mender_api_jwt = NULL;

/**
 * @brief HTTP callback used to handle text content
 * @param event HTTP client event
 * @param data Data received
 * @param data_length Data length
 * @param params Callback parameters
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_client_http_text_callback(mender_http_client_event_t event, void *data, size_t data_length, void *params);

/**
 * @brief HTTP callback used to handle artifact content
 * @param event HTTP client event
 * @param data Data received
 * @param data_length Data length
 * @param params Callback parameters
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_client_http_artifact_callback(mender_http_client_event_t event, void *data, size_t data_length, void *params);

/**
 * @brief Print response error
 * @param response HTTP response, NULL if not available
 * @param status HTTP status
 */
static void mender_api_print_response_error(char *response, int status);

mender_err_t
mender_api_init(mender_api_config_t *config) {

    assert(NULL != config);
    assert(NULL != config->mac_address);
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
mender_api_perform_authentication(unsigned char *private_key, size_t private_key_length, unsigned char *public_key, size_t public_key_length) {

    assert(NULL != private_key);
    assert(NULL != public_key);
    mender_err_t ret;
    char *       public_key_pem   = NULL;
    char *       payload          = NULL;
    char *       response         = NULL;
    char *       signature        = NULL;
    size_t       signature_length = 0;
    int          status           = 0;

    /* Convert public key to PEM format */
    size_t olen = 0;
    mender_tls_pem_write_buffer(public_key, public_key_length, NULL, 0, &olen);
    if (NULL == (public_key_pem = (char *)malloc(olen))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    if (MENDER_OK != (ret = mender_tls_pem_write_buffer(public_key, public_key_length, public_key_pem, olen, &olen))) {
        mender_log_error("Unable to convert public key");
        goto END;
    }

    /* Format payload */
    if (NULL != mender_api_config.tenant_token) {
        if (NULL
            == (payload = malloc(strlen("{ \"id_data\": \"{ \\\"mac\\\": \\\"\\\"}\", \"pubkey\": \"\", \"tenant_token\": \"\" }")
                                 + strlen(mender_api_config.mac_address) + strlen(public_key_pem) + strlen(mender_api_config.tenant_token) + 1))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        sprintf(payload,
                "{ \"id_data\": \"{ \\\"mac\\\": \\\"%s\\\"}\", \"pubkey\": \"%s\", \"tenant_token\": \"%s\" }",
                mender_api_config.mac_address,
                public_key_pem,
                mender_api_config.tenant_token);
    } else {
        if (NULL
            == (payload = malloc(strlen("{ \"id_data\": \"{ \\\"mac\\\": \\\"\\\"}\", \"pubkey\": \"\" }") + strlen(mender_api_config.mac_address)
                                 + strlen(public_key_pem) + 1))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        sprintf(payload, "{ \"id_data\": \"{ \\\"mac\\\": \\\"%s\\\"}\", \"pubkey\": \"%s\" }", mender_api_config.mac_address, public_key_pem);
    }

    /* Sign payload */
    if (MENDER_OK != (ret = mender_tls_sign_payload(private_key, private_key_length, payload, &signature, &signature_length))) {
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
                                      &mender_client_http_text_callback,
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
    if (NULL != public_key_pem) {
        free(public_key_pem);
    }

    return ret;
}

mender_err_t
mender_api_check_for_deployment(char **id, char **artifact_name, char **uri) {

    assert(NULL != id);
    assert(NULL != artifact_name);
    assert(NULL != uri);
    mender_err_t ret;
    char *       path     = NULL;
    char *       response = NULL;
    int          status   = 0;

    /* Compute path */
    if (NULL
        == (path = malloc(strlen("?artifact_name=&device_type=") + strlen(MENDER_API_PATH_GET_NEXT_DEPLOYMENT) + strlen(mender_api_config.artifact_name)
                          + strlen(mender_api_config.device_type) + 1))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    sprintf(path, "%s?artifact_name=%s&device_type=%s", MENDER_API_PATH_GET_NEXT_DEPLOYMENT, mender_api_config.artifact_name, mender_api_config.device_type);

    /* Perform HTTP request */
    if (MENDER_OK
        != (ret = mender_http_perform(mender_api_jwt, path, MENDER_HTTP_GET, NULL, NULL, &mender_client_http_text_callback, (void *)&response, &status))) {
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
    char *       value    = NULL;
    char *       payload  = NULL;
    char *       path     = NULL;
    char *       response = NULL;
    int          status   = 0;

    /* Deployment status to string */
    if (NULL == (value = mender_utils_deployment_status_to_string(deployment_status))) {
        mender_log_error("Invalid status");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Format payload */
    if (NULL == (payload = malloc(strlen("{ \"status\": \"\" }") + strlen(value) + 1))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    sprintf(payload, "{ \"status\": \"%s\" }", value);

    /* Compute path */
    if (NULL == (path = malloc(strlen(MENDER_API_PATH_PUT_DEPLOYMENT_STATUS) - strlen("%s") + strlen(id) + 1))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    sprintf(path, MENDER_API_PATH_PUT_DEPLOYMENT_STATUS, id);

    /* Perform HTTP request */
    if (MENDER_OK
        != (ret = mender_http_perform(mender_api_jwt, path, MENDER_HTTP_PUT, payload, NULL, &mender_client_http_text_callback, (void *)&response, &status))) {
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

    return ret;
}

mender_err_t
mender_api_download_artifact(char *uri, mender_err_t (*callback)(char *, cJSON *, char *, size_t, void *, size_t, size_t)) {

    assert(NULL != uri);
    assert(NULL != callback);
    mender_err_t ret;
    int          status = 0;

    /* Perform HTTP request */
    if (MENDER_OK != (ret = mender_http_perform(NULL, uri, MENDER_HTTP_GET, NULL, NULL, &mender_client_http_artifact_callback, callback, &status))) {
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

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

mender_err_t
mender_api_download_configuration_data(mender_keystore_t **configuration) {

    assert(NULL != configuration);
    mender_err_t ret;
    char *       response = NULL;
    int          status   = 0;

    /* Perform HTTP request */
    if (MENDER_OK
        != (ret = mender_http_perform(mender_api_jwt,
                                      MENDER_API_PATH_GET_DEVICE_CONFIGURATION,
                                      MENDER_HTTP_GET,
                                      NULL,
                                      NULL,
                                      &mender_client_http_text_callback,
                                      (void *)&response,
                                      &status))) {
        mender_log_error("Unable to perform HTTP request");
        goto END;
    }

    /* Treatment depending of the status */
    if (200 == status) {
        if (MENDER_OK != (ret = mender_utils_keystore_from_string(configuration, response))) {
            mender_log_error("Unable to set configuration");
            goto END;
        }
    } else {
        mender_api_print_response_error(response, status);
        ret = MENDER_FAIL;
    }

END:

    /* Release memory */
    if (NULL != response) {
        free(response);
    }

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */

mender_err_t
mender_api_publish_configuration_data(mender_keystore_t *configuration) {

    mender_err_t ret;
    char *       payload  = NULL;
    char *       response = NULL;
    int          status   = 0;

    /* Format payload */
    if (MENDER_OK != (ret = mender_utils_keystore_to_string(configuration, &payload))) {
        mender_log_error("Unable to format payload");
        goto END;
    }

    /* Perform HTTP request */
    if (MENDER_OK
        != (ret = mender_http_perform(mender_api_jwt,
                                      MENDER_API_PATH_PUT_DEVICE_CONFIGURATION,
                                      MENDER_HTTP_PUT,
                                      payload,
                                      NULL,
                                      &mender_client_http_text_callback,
                                      (void *)&response,
                                      &status))) {
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
    if (NULL != payload) {
        free(payload);
    }

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY

mender_err_t
mender_api_publish_inventory_data(mender_keystore_t *inventory) {

    mender_err_t ret;
    char *       payload  = NULL;
    char *       response = NULL;
    int          status   = 0;

    /* Format payload */
    cJSON *object = cJSON_CreateArray();
    if (NULL == object) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    cJSON *item = cJSON_CreateObject();
    if (NULL == item) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    cJSON_AddStringToObject(item, "name", "artifact_name");
    cJSON_AddStringToObject(item, "value", mender_api_config.artifact_name);
    cJSON_AddItemToArray(object, item);
    item = cJSON_CreateObject();
    if (NULL == item) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    cJSON_AddStringToObject(item, "name", "device_type");
    cJSON_AddStringToObject(item, "value", mender_api_config.device_type);
    cJSON_AddItemToArray(object, item);
    if (NULL != inventory) {
        size_t index = 0;
        while ((NULL != inventory[index].name) && (NULL != inventory[index].value)) {
            if (NULL == (item = cJSON_CreateObject())) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto END;
            }
            cJSON_AddStringToObject(item, "name", inventory[index].name);
            cJSON_AddStringToObject(item, "value", inventory[index].value);
            cJSON_AddItemToArray(object, item);
            index++;
        }
    }
    if (NULL == (payload = cJSON_PrintUnformatted(object))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Perform HTTP request */
    if (MENDER_OK
        != (ret = mender_http_perform(mender_api_jwt,
                                      MENDER_API_PATH_PUT_DEVICE_ATTRIBUTES,
                                      MENDER_HTTP_PUT,
                                      payload,
                                      NULL,
                                      &mender_client_http_text_callback,
                                      (void *)&response,
                                      &status))) {
        mender_log_error("Unable to perform HTTP request");
        goto END;
    }

    /* Treatment depending of the status */
    if (200 == status) {
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
    if (NULL != payload) {
        free(payload);
    }
    if (NULL != object) {
        cJSON_Delete(object);
    }

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY */

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

static mender_err_t
mender_client_http_text_callback(mender_http_client_event_t event, void *data, size_t data_length, void *params) {

    assert(NULL != params);
    char **      response = (char **)params;
    mender_err_t ret      = MENDER_OK;
    char *       tmp;

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

static mender_err_t
mender_client_http_artifact_callback(mender_http_client_event_t event, void *data, size_t data_length, void *params) {

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

static void
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
