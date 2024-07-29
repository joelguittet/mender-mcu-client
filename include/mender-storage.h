/**
 * @file      mender-storage.h
 * @brief     Mender storage interface
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

#ifndef __MENDER_STORAGE_H__
#define __MENDER_STORAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Initialize mender storage
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_init(void);

/**
 * @brief Set authentication keys
 * @param private_key Private key to store
 * @param private_key_length Private key length
 * @param public_key Public key to store
 * @param public_key_length Public key length
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_set_authentication_keys(unsigned char *private_key, size_t private_key_length, unsigned char *public_key, size_t public_key_length);

/**
 * @brief Get authentication keys
 * @param private_key Private key from storage, NULL if not found
 * @param private_key_length Private key length from storage, 0 if not found
 * @param public_key Public key from storage, NULL if not found
 * @param public_key_length Public key length from storage, 0 if not found
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_get_authentication_keys(unsigned char **private_key,
                                                    size_t         *private_key_length,
                                                    unsigned char **public_key,
                                                    size_t         *public_key_length);

/**
 * @brief Delete authentication keys
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_delete_authentication_keys(void);

/**
 * @brief Set deployment data
 * @param deployment_data Deployment data to store
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_set_deployment_data(char *deployment_data);

/**
 * @brief Get deployment data
 * @param deployment_data Deployment data from storage, NULL if not found
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_get_deployment_data(char **deployment_data);

/**
 * @brief Delete deployment data
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_delete_deployment_data(void);

#ifdef CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE
#ifdef CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE

/**
 * @brief Set device configuration
 * @param device_config Device configuration
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_set_device_config(char *device_config);

/**
 * @brief Get device configuration
 * @param device_config Device configuration
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_get_device_config(char **device_config);

/**
 * @brief Delete device configuration
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_delete_device_config(void);

#endif /* CONFIG_MENDER_CLIENT_CONFIGURE_STORAGE */
#endif /* CONFIG_MENDER_CLIENT_ADD_ON_CONFIGURE */

/**
 * @brief Release mender storage
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_storage_exit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_STORAGE_H__ */
