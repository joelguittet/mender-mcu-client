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

#include "cJSON.h"

#include "mender-api.h"
#include "mender-http.h"
#include "mender-log.h"
#include "mender-tls.h"
#include "mender-untar.h"
#include "mender-utils.h"

/**
 * @brief Paths of the mender-server api
 */
#define MENDER_API_PATH_POST_AUTHENTICATION_REQUESTS "/api/devices/v1/authentication/auth_requests"
#define MENDER_API_PATH_PUT_DEVICE_ATTRIBUTES        "/api/devices/v1/inventory/device/attributes"
#define MENDER_API_PATH_GET_NEXT_DEPLOYMENT          "/api/devices/v1/deployments/device/deployments/next"
#define MENDER_API_PATH_PUT_DEPLOYMENT_STATUS        "/api/devices/v1/deployments/device/deployments/%s/status"

/**
 * @brief Mender API configuration
 */
static mender_api_config_t mender_api_config;

/**
 * @brief Mender API callbacks
 */
static mender_api_callbacks_t mender_api_callbacks;

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
mender_api_init(mender_api_config_t *config, mender_api_callbacks_t *callbacks) {

    assert(NULL != config);
    assert(NULL != config->mac_address);
    assert(NULL != config->artifact_name);
    assert(NULL != config->device_type);
    assert(NULL != config->host);
    assert(NULL != callbacks);
    assert(NULL != callbacks->ota_begin);
    assert(NULL != callbacks->ota_write);
    assert(NULL != callbacks->ota_abort);
    assert(NULL != callbacks->ota_end);
    mender_err_t ret;

    /* Save configuration */
    memcpy(&mender_api_config, config, sizeof(mender_api_config_t));

    /* Save callbacks */
    memcpy(&mender_api_callbacks, callbacks, sizeof(mender_api_callbacks_t));

    /* Initializations */
    mender_http_config_t mender_http_config = { .host = mender_api_config.host };
    if (MENDER_OK != (ret = mender_http_init(&mender_http_config))) {
        mender_log_error("Unable to initialize HTTP");
        return ret;
    }

    return MENDER_OK;
}

mender_err_t
mender_api_perform_authentication(char *private_key, char *public_key) {

    assert(NULL != private_key);
    assert(NULL != public_key);
    mender_err_t ret;
    char *       payload          = NULL;
    char *       response         = NULL;
    char *       signature        = NULL;
    size_t       signature_length = 0;
    int          status           = 0;

    /* Format payload */
    if (NULL != mender_api_config.tenant_token) {
        if (NULL
            == (payload = malloc(strlen("{ \"id_data\": \"{ \\\"mac\\\": \\\"\\\"}\", \"pubkey\": \"\", \"tenant_token\": \"\" }")
                                 + strlen(mender_api_config.mac_address) + strlen(public_key) + strlen(mender_api_config.tenant_token) + 1))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        sprintf(payload,
                "{ \"id_data\": \"{ \\\"mac\\\": \\\"%s\\\"}\", \"pubkey\": \"%s\", \"tenant_token\": \"%s\" }",
                mender_api_config.mac_address,
                public_key,
                mender_api_config.tenant_token);
    } else {
        if (NULL
            == (payload = malloc(strlen("{ \"id_data\": \"{ \\\"mac\\\": \\\"\\\"}\", \"pubkey\": \"\" }") + strlen(mender_api_config.mac_address)
                                 + strlen(public_key) + 1))) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        sprintf(payload, "{ \"id_data\": \"{ \\\"mac\\\": \\\"%s\\\"}\", \"pubkey\": \"%s\" }", mender_api_config.mac_address, public_key);
    }

    /* Sign payload */
    if (MENDER_OK != (ret = mender_tls_sign_payload(private_key, payload, &signature, &signature_length))) {
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

    return ret;
}

mender_err_t
mender_api_publish_inventory_data(mender_inventory_t *inventory, size_t inventory_length) {

    mender_err_t ret;
    char *       payload  = NULL;
    char *       response = NULL;
    int          status   = 0;

    /* Format payload */
    cJSON *array = cJSON_CreateArray();
    if (NULL == array) {
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
    cJSON_AddItemToArray(array, item);
    item = cJSON_CreateObject();
    if (NULL == item) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    cJSON_AddStringToObject(item, "name", "device_type");
    cJSON_AddStringToObject(item, "value", mender_api_config.device_type);
    cJSON_AddItemToArray(array, item);
    if (NULL != inventory) {
        for (size_t index = 0; index < inventory_length; index++) {
            if (NULL == (item = cJSON_CreateObject())) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                goto END;
            }
            cJSON_AddStringToObject(item, "name", inventory[index].name);
            cJSON_AddStringToObject(item, "value", inventory[index].value);
            cJSON_AddItemToArray(array, item);
        }
    }
    payload = cJSON_Print(array);

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
    if (NULL != array) {
        cJSON_Delete(array);
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
mender_api_download_artifact(char *uri) {

    assert(NULL != uri);
    mender_err_t ret;
    int          status = 0;

    /* Perform HTTP request */
    if (MENDER_OK != (ret = mender_http_perform(NULL, uri, MENDER_HTTP_GET, NULL, NULL, &mender_client_http_artifact_callback, NULL, &status))) {
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
            if ((NULL == data) || (0 == data_length)) {
                mender_log_error("Invalid data received");
                ret = MENDER_FAIL;
                break;
            }
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

    (void)params;
    mender_err_t           ret             = MENDER_OK;
    mender_untar_header_t *header          = NULL;
    void *                 output_data     = NULL;
    size_t                 output_length   = 0;
    static bool            ota_in_progress = false;

    /* un-tar context */
    static mender_untar_ctx_t *mender_untar_ctx = NULL;

    /* OTA handle, can be used to store temporary reference to write OTA data */
    static void *ota_handle = NULL;

    /* Treatment depending of the event */
    switch (event) {
        case MENDER_HTTP_EVENT_CONNECTED:
            /* Create new un-tar context */
            if (NULL == (mender_untar_ctx = mender_untar_init())) {
                mender_log_error("Unable to allocate memory");
                ret = MENDER_FAIL;
                break;
            }
            break;
        case MENDER_HTTP_EVENT_DATA_RECEIVED:
            if (NULL == mender_untar_ctx) {
                mender_log_error("Invalid un-tar context");
                ret = MENDER_FAIL;
                break;
            }
            if ((NULL == data) || (0 == data_length)) {
                mender_log_error("Invalid data received");
                ret = MENDER_FAIL;
                break;
            }

            /* Parse input data */
            if (mender_untar_parse(mender_untar_ctx, data, data_length, &header, &output_data, &output_length) < 0) {
                mender_log_error("Unable to parse data received");
                ret = MENDER_FAIL;
                break;
            }

            /* Check if header or output data are available */
            while ((NULL != header) || (NULL != output_data)) {
                if (NULL != header) {
                    /* Check file name */
                    if ((strcmp(header->name, "version")) && (strcmp(header->name, "manifest")) && (strcmp(header->name, "header-info"))
                        && (strcmp(header->name, "headers/0000/type-info")) && (strcmp(header->name, "headers/0000/meta-data"))) {
                        /* Binary file found, open the OTA handle */
                        if (MENDER_OK != (ret = mender_api_callbacks.ota_begin(header->name, header->size, &ota_handle))) {
                            mender_log_error("Unable to open OTA handle");
                        } else {
                            ota_in_progress = true;
                        }
                    } else {
                        ota_in_progress = false;
                    }
                    free(header);
                    header = NULL;
                }
                if (NULL != output_data) {
                    if (true == ota_in_progress) {
                        if (MENDER_OK != (ret = mender_api_callbacks.ota_write(ota_handle, output_data, output_length))) {
                            mender_log_error("Unable to write OTA data");
                        }
                    }
                    free(output_data);
                    output_data = NULL;
                }
                output_length = 0;

                /* Parse next data */
                if (mender_untar_parse(mender_untar_ctx, NULL, 0, &header, &output_data, &output_length) < 0) {
                    mender_log_error("Unable to parse data received");
                    ret = MENDER_FAIL;
                }
            }
            break;
        case MENDER_HTTP_EVENT_DISCONNECTED:
            /* Close OTA handle */
            if (MENDER_OK != (ret = mender_api_callbacks.ota_end(ota_handle))) {
                mender_log_error("Unable to close OTA handle");
            }
            ota_in_progress = false;
            /* Release un-tar context */
            mender_untar_release(mender_untar_ctx);
            mender_untar_ctx = NULL;
            break;
        case MENDER_HTTP_EVENT_ERROR:
            mender_log_error("An error occurred");
            ret = MENDER_FAIL;
            /* Abort current OTA */
            mender_api_callbacks.ota_abort(ota_handle);
            mender_api_callbacks.ota_end(ota_handle);
            ota_in_progress = false;
            /* Release un-tar context */
            mender_untar_release(mender_untar_ctx);
            mender_untar_ctx = NULL;
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
