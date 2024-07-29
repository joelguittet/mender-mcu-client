/**
 * @file      mender-tls.h
 * @brief     Mender TLS interface
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

#ifndef __MENDER_TLS_H__
#define __MENDER_TLS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mender-utils.h"

/**
 * @brief Initialize mender TLS
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_tls_init(void);

/**
 * @brief Initialize mender TLS authentication keys
 * @param recommissioning Perform recommissioning (if supported by the platform)
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_tls_init_authentication_keys(bool recommissioning);

/**
 * @brief Get public key (PEM format suitable to be integrated in mender authentication request)
 * @param public_key Public key, NULL if an error occurred
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_tls_get_public_key_pem(char **public_key);

/**
 * @brief Sign payload
 * @param payload Payload to sign
 * @param signature Signature of the payload
 * @param signature_length Length of the signature buffer, updated to the length of the signature
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_tls_sign_payload(char *payload, char **signature, size_t *signature_length);

/**
 * @brief Release mender TLS
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_tls_exit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENDER_TLS_H__ */
