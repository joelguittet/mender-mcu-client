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

#include "mender-common.h"

/**
 * @brief Initialize mender TLS
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_tls_init(void);

/**
 * @brief Generate authentication keys
 * @param private_key Private key generated
 * @param private_key_length Private key lenght
 * @param public_key Public key generated
 * @param public_key_length Public key lenght
 * @return ESP_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_tls_generate_authentication_keys(char **private_key, size_t *private_key_length, char **public_key, size_t *public_key_length);

/**
 * @brief Sign payload
 * @param private_key Private key
 * @param payload Payload to sign
 * @param signature Signature of the payload
 * @param signature_length Length of the signature buffer, updated to the length of the signature
 * @return ESP_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_tls_sign_payload(char *private_key, char *payload, char **signature, size_t *signature_length);

/**
 * @brief Release mender TLS
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
mender_err_t mender_tls_exit(void);

#endif /* __MENDER_TLS_H__ */
