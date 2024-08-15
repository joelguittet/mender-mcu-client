/**
 * @file      mender-inventory-api.c
 * @brief     Mender MCU inventory add-on API implementation
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
#include "mender-inventory-api.h"

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_INVENTORY

#include "mender-http.h"

/**
 * @brief Paths of the mender-server APIs
 */
#define MENDER_API_PATH_PUT_DEVICE_ATTRIBUTES "/api/devices/v1/inventory/device/attributes"

mender_err_t
mender_inventory_api_publish_inventory_data(char *artifact_name, char *device_type, mender_keystore_t *inventory) {

    mender_err_t ret;
    cJSON       *item;
    char        *payload  = NULL;
    char        *response = NULL;
    int          status   = 0;

    /* Format payload */
    cJSON *object = cJSON_CreateArray();
    if (NULL == object) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }
    if (NULL != artifact_name) {
        if (NULL == (item = cJSON_CreateObject())) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        cJSON_AddStringToObject(item, "name", "artifact_name");
        cJSON_AddStringToObject(item, "value", artifact_name);
        cJSON_AddItemToArray(object, item);
        item = cJSON_CreateObject();
        if (NULL == item) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        cJSON_AddStringToObject(item, "name", "rootfs-image.version");
        cJSON_AddStringToObject(item, "value", artifact_name);
        cJSON_AddItemToArray(object, item);
    }
    if (NULL != device_type) {
        if (NULL == (item = cJSON_CreateObject())) {
            mender_log_error("Unable to allocate memory");
            ret = MENDER_FAIL;
            goto END;
        }
        cJSON_AddStringToObject(item, "name", "device_type");
        cJSON_AddStringToObject(item, "value", device_type);
        cJSON_AddItemToArray(object, item);
    }
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
        != (ret = mender_http_perform(mender_api_get_authentication_token(),
                                      MENDER_API_PATH_PUT_DEVICE_ATTRIBUTES,
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
