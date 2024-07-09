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

#include "mender-addon.h"
#include "mender-utils.h"

/**
 * @brief Mender client configuration
 */
typedef struct {
    mender_keystore_t *identity;                     /**< Identity of the device */
    char              *artifact_name;                /**< Artifact name */
    char              *device_type;                  /**< Device type */
    char              *host;                         /**< URL of the mender server */
    char              *tenant_token;                 /**< Tenant token used to authenticate on the mender server (optional) */
    int32_t            authentication_poll_interval; /**< Authentication poll interval, default is 60 seconds, -1 permits to disable periodic execution */
    int32_t            update_poll_interval;         /**< Update poll interval, default is 1800 seconds, -1 permits to disable periodic execution */
    bool               recommissioning;              /**< Used to force creation of new authentication keys */
} mender_client_config_t;

/**
 * @brief Mender client callbacks
 */
typedef struct {
    mender_err_t (*network_connect)(void);                                 /**< Invoked when mender-client requests access to the network */
    mender_err_t (*network_release)(void);                                 /**< Invoked when mender-client releases access to the network */
    mender_err_t (*authentication_success)(void);                          /**< Invoked when authentication with the mender server succeeded */
    mender_err_t (*authentication_failure)(void);                          /**< Invoked when authentication with the mender server failed */
    mender_err_t (*deployment_status)(mender_deployment_status_t, char *); /**< Invoked on transition changes to inform of the new deployment status */
    mender_err_t (*restart)(void);                                         /**< Invoked to restart the device */
} mender_client_callbacks_t;

/**
 * @brief Return mender client version
 * @return Mender client version as string
 */
char *mender_client_version(void);

/**
 * @brief Initialize mender client
 * @param config Mender client configuration
 * @param callbacks Mender client callbacks
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_client_init(mender_client_config_t *config, mender_client_callbacks_t *callbacks);

/**
 * @brief Register artifact type
 * @param type Artifact type
 * @param callback Artifact type callback
 * @param needs_restart Flag to indicate if the artifact type requires the device to restart after downloading
 * @param artifact_name Artifact name (optional, NULL otherwise), set to validate module update after restarting
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_client_register_artifact_type(char *type,
                                                  mender_err_t (*callback)(char *, char *, char *, cJSON *, char *, size_t, void *, size_t, size_t),
                                                  bool  needs_restart,
                                                  char *artifact_name);

/**
 * @brief Register add-on
 * @param addon Add-on
 * @param config Add-on configuration
 * @param callbacks Add-on callbacks
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_client_register_addon(mender_addon_instance_t *addon, void *config, void *callbacks);

/**
 * @brief Activate mender client
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_client_activate(void);

/**
 * @brief Deactivate mender client
 * @note This function stops synchronization with the server
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_client_deactivate(void);

/**
 * @brief Function used to trigger execution of the authentication and update work
 * @note Calling this function is optional when the periodic execution of the work is configured
 * @note It only permits to execute the work as soon as possible to synchronize updates
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_client_execute(void);

/**
 * @brief Function to be called from add-ons to request network access
 * @return MENDER_OK if network is connected following the request, error code otherwise
 */
mender_err_t mender_client_network_connect(void);

/**
 * @brief Function to be called from add-ons to release network access
 * @return MENDER_OK if network is released following the request, error code otherwise
 */
mender_err_t mender_client_network_release(void);

/**
 * @brief Release mender client
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_client_exit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_CLIENT_H__ */
