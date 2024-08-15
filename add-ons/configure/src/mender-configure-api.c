/**
 * @file      mender-configure-api.c
 * @brief     Mender MCU configure add-on API implementation
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
#include "mender-log.h"
#include "mender-configure-api.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE

#include "mender-http.h"

/**
 * @brief Paths of the mender-server APIs
 */
#define MENDER_API_PATH_GET_DEVICE_CONFIGURATION "/api/devices/v1/deviceconfig/configuration"
#define MENDER_API_PATH_PUT_DEVICE_CONFIGURATION "/api/devices/v1/deviceconfig/configuration"

#ifndef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

mender_err_t
mender_configure_api_download_configuration_data(mender_keystore_t **configuration) {

    assert(NULL != configuration);
    mender_err_t ret;
    char        *response = NULL;
    int          status   = 0;

    /* Perform HTTP request */
    if (MENDER_OK
        != (ret = mender_http_perform(mender_api_get_authentication_token(),
                                      MENDER_API_PATH_GET_DEVICE_CONFIGURATION,
                                      MENDER_HTTP_GET,
                                      NULL,
                                      NULL,
                                      &mender_api_http_text_callback,
                                      (void *)&response,
                                      &status))) {
        mender_log_error("Unable to perform HTTP request");
        goto END;
    }

    /* Treatment depending of the status */
    if (200 == status) {
        cJSON *json_response = cJSON_Parse(response);
        if (NULL == json_response) {
            mender_log_error("Unable to set configuration");
            goto END;
        }
        if (MENDER_OK != (ret = mender_utils_keystore_from_json(configuration, json_response))) {
            mender_log_error("Unable to set configuration");
            cJSON_Delete(json_response);
            goto END;
        }
        cJSON_Delete(json_response);
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
mender_configure_api_publish_configuration_data(mender_keystore_t *configuration) {

    mender_err_t ret;
    cJSON       *json_configuration = NULL;
    char        *payload            = NULL;
    char        *response           = NULL;
    int          status             = 0;

    /* Format payload */
    if (MENDER_OK != (ret = mender_utils_keystore_to_json(configuration, &json_configuration))) {
        mender_log_error("Unable to format payload");
        goto END;
    }
    if (NULL == (payload = cJSON_PrintUnformatted(json_configuration))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Perform HTTP request */
    if (MENDER_OK
        != (ret = mender_http_perform(mender_api_get_authentication_token(),
                                      MENDER_API_PATH_PUT_DEVICE_CONFIGURATION,
                                      MENDER_HTTP_PUT,
                                      payload,
                                      NULL,
                                      &mender_api_http_text_callback,
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
    if (NULL != json_configuration) {
        cJSON_Delete(json_configuration);
    }

    return ret;
}

#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */
