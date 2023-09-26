/**
 * @file      mender-client.h
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

#ifndef __MENDER_CLIENT_H__
#define __MENDER_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Mender client configuration
 */
typedef struct {
    char *   mac_address;                  /**< MAC address of the device */
    char *   artifact_name;                /**< Artifact name */
    char *   device_type;                  /**< Device type */
    char *   host;                         /**< URL of the mender server */
    char *   tenant_token;                 /**< Tenant token used to authenticate on the mender server (optional) */
    uint32_t authentication_poll_interval; /**< Authentication poll interval, default is 60 seconds */
    uint32_t update_poll_interval;         /**< Update poll interval, default is 1800 seconds */
    bool     recommissioning;              /**< Used to force creation of new authentication keys */
} mender_client_config_t;

/**
 * @brief Mender client callbacks
 */
typedef struct {
    mender_err_t (*authentication_success)(void);                          /**< Invoked when authentication with the mender server succeeded */
    mender_err_t (*authentication_failure)(void);                          /**< Invoked when authentication with the mender server failed */
    mender_err_t (*deployment_available)(char *, char *, char *);          /**< Invoked when a new deployment is available */
    mender_err_t (*deployment_status)(mender_deployment_status_t, char *); /**< Invoked on transition changes to inform of the new deployment status */
    mender_err_t (*ota_begin)(char *, size_t, void **);                    /**< Invoked when the OTA starts */
    mender_err_t (*ota_write)(void *, void *, size_t, size_t);             /**< Invoked to write data received from the mender server */
    mender_err_t (*ota_abort)(void *);                                     /**< Invoked to abort current OTA */
    mender_err_t (*ota_end)(void *);                                       /**< Invoked to indicate the end of the artifact */
    mender_err_t (*ota_set_boot_partition)(void *);                        /**< Invoked to set the new boot parition to be used on the next restart */
    mender_err_t (*restart)(void);                                         /**< Invoked to restart the devide */
} mender_client_callbacks_t;

/**
 * @brief Initialize mender client
 * @param config Mender client configuration
 * @param callbacks Mender client callbacks
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_client_init(mender_client_config_t *config, mender_client_callbacks_t *callbacks);

/**
 * @brief Release mender client
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_client_exit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_CLIENT_H__ */
