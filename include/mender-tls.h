/**
 * @file      mender-tls.h
 * @brief     Mender TLS interface
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
