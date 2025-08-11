/**
 * @file      mender-tls.c
 * @brief     Mender TLS interface for PSA Crypto API platform
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

#include <mbedtls/base64.h>
#ifdef MBEDTLS_ERROR_C
#include <mbedtls/error.h>
#endif /* MBEDTLS_ERROR_C */
#include <psa/crypto.h>
#include "mender-log.h"
#include "mender-tls.h"

/**
 * @brief Default PSA signature key ID
 */
#ifndef CONFIG_MENDER_TLS_PSA_CRYPTO_SIGNATURE_KEY_ID
#define CONFIG_MENDER_TLS_PSA_CRYPTO_SIGNATURE_KEY_ID (1)
#endif /* CONFIG_MENDER_TLS_PSA_CRYPTO_SIGNATURE_KEY_ID */

/**
 * @brief Keys buffer length
 */
#define MENDER_TLS_PUBLIC_KEY_LENGTH (64 + 1)

/**
 * @brief Public key x509 header
 */
static const uint8_t mender_tls_public_key_x509_header[] = { 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01,
                                                             0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00 };

/**
 * @brief Public key of the device
 */
static unsigned char *mender_tls_public_key        = NULL;
static size_t         mender_tls_public_key_length = 0;

/**
 * @brief Generate authentication keys
 * @param public_key Public key generated
 * @param public_key_length Public key length
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_tls_generate_authentication_keys(unsigned char **public_key, size_t *public_key_length);

/**
 * @brief Get authentication keys
 * @param public_key Public key from storage, NULL if not found
 * @param public_key_length Public key length from storage, 0 if not found
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_tls_get_authentication_keys(unsigned char **public_key, size_t *public_key_length);

/**
 * @brief Delete authentication keys
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_tls_delete_authentication_keys(void);

/**
 * @brief Write a buffer of PEM information from a DER encoded buffer
 * @note This function is derived from mbedtls_pem_write_buffer with const header and footer
 * @param der_data The DER data to encode
 * @param der_len The length of the DER data
 * @param buf The buffer to write to
 * @param buf_len The length of the output buffer
 * @param olen The address at which to store the total length written or required output buffer length is not enough
 * @return MENDER_OK if the function succeeds, error code otherwise
 */
static mender_err_t mender_tls_pem_write_buffer(const unsigned char *der_data, size_t der_len, char *buf, size_t buf_len, size_t *olen);

mender_err_t
mender_tls_init(void) {

    psa_status_t status;

    /* Initialize PSA Crypto */
    if (PSA_SUCCESS != (status = psa_crypto_init())) {
        mender_log_error("Unable to initialize PSA Crypto (status=%d)", status);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_tls_init_authentication_keys(bool recommissioning) {

    mender_err_t ret;

    /* Release memory */
    if (NULL != mender_tls_public_key) {
        free(mender_tls_public_key);
        mender_tls_public_key = NULL;
    }
    mender_tls_public_key_length = 0;

    /* Check if recommissioning is forced */
    if (true == recommissioning) {

        /* Erase authentication keys */
        mender_log_info("Delete authentication keys...");
        if (MENDER_OK != mender_tls_delete_authentication_keys()) {
            mender_log_warning("Unable to delete authentication keys");
        }
    }

    /* Retrieve or generate private and public keys if not allready done */
    if (MENDER_OK != (ret = mender_tls_get_authentication_keys(&mender_tls_public_key, &mender_tls_public_key_length))) {

        /* Generate authentication keys */
        mender_log_info("Generating authentication keys...");
        if (MENDER_OK != (ret = mender_tls_generate_authentication_keys(&mender_tls_public_key, &mender_tls_public_key_length))) {
            mender_log_error("Unable to generate authentication keys");
            return ret;
        }
    }

    return ret;
}

mender_err_t
mender_tls_get_public_key_pem(char **public_key) {

    assert(NULL != public_key);
    unsigned char *der_data;
    size_t         der_len;
    mender_err_t   ret;

    /* Prepare DER data */
    if (NULL == (der_data = (unsigned char *)malloc(sizeof(mender_tls_public_key_x509_header) + mender_tls_public_key_length))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    memcpy(der_data, mender_tls_public_key_x509_header, sizeof(mender_tls_public_key_x509_header));
    memcpy(der_data + sizeof(mender_tls_public_key_x509_header), mender_tls_public_key, mender_tls_public_key_length);
    der_len = sizeof(mender_tls_public_key_x509_header) + mender_tls_public_key_length;

    /* Compute size of the public key */
    size_t olen = 0;
    mender_tls_pem_write_buffer(der_data, der_len, NULL, 0, &olen);
    if (0 == olen) {
        mender_log_error("Unable to compute public key size");
        free(der_data);
        return MENDER_FAIL;
    }
    if (NULL == (*public_key = (char *)malloc(olen))) {
        mender_log_error("Unable to allocate memory");
        free(der_data);
        return MENDER_FAIL;
    }

    /* Convert public key from DER to PEM format */
    if (MENDER_OK != (ret = mender_tls_pem_write_buffer(der_data, der_len, *public_key, olen, &olen))) {
        mender_log_error("Unable to convert public key");
        free(der_data);
        return ret;
    }

    /* Release memory */
    free(der_data);

    return MENDER_OK;
}

mender_err_t
mender_tls_sign_payload(char *payload, char **signature, size_t *signature_length) {

    assert(NULL != payload);
    assert(NULL != signature);
    assert(NULL != signature_length);
    psa_status_t     status;
    psa_key_handle_t key_handle;
    uint8_t          sig[PSA_ECDSA_SIGNATURE_SIZE(256)];
    size_t           sig_len;
    uint8_t         *r = &sig[0];
    uint8_t         *s = &sig[PSA_ECDSA_SIGNATURE_SIZE(256) / 2];
    uint8_t          asn1[PSA_ECDSA_SIGNATURE_SIZE(256) + 8];
    size_t           asn1_len = 0;
    int              ret;
    char            *tmp;
#ifdef MBEDTLS_ERROR_C
    char err[128];
#endif /* MBEDTLS_ERROR_C */

    /* Open key */
    if (PSA_SUCCESS != (status = psa_open_key(CONFIG_MENDER_TLS_PSA_CRYPTO_SIGNATURE_KEY_ID, &key_handle))) {
        mender_log_error("Unable to open key (status=%d)", status);
        return MENDER_FAIL;
    }

    /* Compute signature of the payload */
    if (PSA_SUCCESS
        != (status = psa_sign_message(key_handle, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), payload, strlen(payload), sig, sizeof(sig), &sig_len))) {
        mender_log_error("Unable to compute signature of the hash (status=%d)", status);
        psa_close_key(key_handle);
        return MENDER_FAIL;
    }

    /* Close key */
    psa_close_key(key_handle);

    /* Convert signature to ASN.1 format */
    asn1[asn1_len] = 0x30;
    asn1_len++;
    asn1[asn1_len] = 4 + ((0x00 != (r[0] & 0x80)) ? 1 : 0) + 32 + ((0x00 != (s[0] & 0x80)) ? 1 : 0) + 32;
    asn1_len++;
    asn1[asn1_len] = 0x02;
    asn1_len++;
    asn1[asn1_len] = ((0x00 != (r[0] & 0x80)) ? 1 : 0) + 32;
    asn1_len++;
    if (0x00 != (r[0] & 0x80)) {
        asn1[asn1_len] = 0x00;
        asn1_len++;
    }
    memcpy(&asn1[asn1_len], r, 32);
    asn1_len += 32;
    asn1[asn1_len] = 0x02;
    asn1_len++;
    asn1[asn1_len] = ((0x00 != (s[0] & 0x80)) ? 1 : 0) + 32;
    asn1_len++;
    if (0x00 != (s[0] & 0x80)) {
        asn1[asn1_len] = 0x00;
        asn1_len++;
    }
    memcpy(&asn1[asn1_len], s, 32);
    asn1_len += 32;

    /* Encode signature to base64 */
    if (NULL == (*signature = malloc(2 * asn1_len + 1))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    memset(*signature, 0, 2 * asn1_len + 1);
    *signature_length = 2 * asn1_len;
    if (0 != (ret = mbedtls_base64_encode((unsigned char *)*signature, *signature_length, signature_length, asn1, asn1_len))) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to encode signature (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to encode signature (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        free(*signature);
        *signature = NULL;
        return MENDER_FAIL;
    }
    *signature_length = strlen(*signature);
    if (NULL == (tmp = realloc(*signature, *signature_length + 1))) {
        mender_log_error("Unable to allocate memory");
        free(*signature);
        *signature = NULL;
        return MENDER_FAIL;
    }
    *signature = tmp;

    return MENDER_OK;
}

mender_err_t
mender_tls_exit(void) {

    /* Release memory */
    if (NULL != mender_tls_public_key) {
        free(mender_tls_public_key);
        mender_tls_public_key = NULL;
    }
    mender_tls_public_key_length = 0;

    return MENDER_OK;
}

static mender_err_t
mender_tls_generate_authentication_keys(unsigned char **public_key, size_t *public_key_length) {

    assert(NULL != public_key);
    assert(NULL != public_key_length);
    psa_status_t         status;
    psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_handle_t     key_handle;

    /* Setup the key's attributes before the creation request */
    psa_set_key_id(&key_attributes, CONFIG_MENDER_TLS_PSA_CRYPTO_SIGNATURE_KEY_ID);
    psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_PERSISTENT);
    psa_set_key_algorithm(&key_attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&key_attributes, 256);

    /* Generate the private key, creating the persistent key on success */
    if (PSA_SUCCESS != (status = psa_generate_key(&key_attributes, &key_handle))) {
        mender_log_error("Unable to generate key (status=%d)", status);
        return MENDER_FAIL;
    }

    /* Close key */
    psa_close_key(key_handle);

    /* Return authentification keys */
    return mender_tls_get_authentication_keys(public_key, public_key_length);
}

static mender_err_t
mender_tls_get_authentication_keys(unsigned char **public_key, size_t *public_key_length) {

    assert(NULL != public_key);
    assert(NULL != public_key_length);
    psa_status_t     status;
    psa_key_handle_t key_handle;

    /* Open key */
    if (PSA_SUCCESS != (status = psa_open_key(CONFIG_MENDER_TLS_PSA_CRYPTO_SIGNATURE_KEY_ID, &key_handle))) {
        mender_log_error("Unable to open key (status=%d)", status);
        return MENDER_FAIL;
    }

    /* Export the persistent key's public key part */
    if (NULL == (*public_key = (unsigned char *)malloc(MENDER_TLS_PUBLIC_KEY_LENGTH))) {
        mender_log_error("Unable to allocate memory");
        psa_close_key(key_handle);
        return MENDER_FAIL;
    }
    if (PSA_SUCCESS != (status = psa_export_public_key(key_handle, *public_key, MENDER_TLS_PUBLIC_KEY_LENGTH, public_key_length))) {
        mender_log_error("Unable to export public key (status=%d)", status);
        psa_close_key(key_handle);
        return MENDER_FAIL;
    }

    /* Close key */
    psa_close_key(key_handle);

    return MENDER_OK;
}

static mender_err_t
mender_tls_delete_authentication_keys(void) {

    psa_status_t status;

    /* Destroy the authentification keys */
    if (PSA_SUCCESS != (status = psa_destroy_key(CONFIG_MENDER_TLS_PSA_CRYPTO_SIGNATURE_KEY_ID))) {
        mender_log_error("Unable to delete authentification keys (status=%d)", status);
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

static mender_err_t
mender_tls_pem_write_buffer(const unsigned char *der_data, size_t der_len, char *buf, size_t buf_len, size_t *olen) {

#define PEM_BEGIN_PUBLIC_KEY "-----BEGIN PUBLIC KEY-----"
#define PEM_END_PUBLIC_KEY   "-----END PUBLIC KEY-----"

    mender_err_t   ret        = MENDER_OK;
    unsigned char *encode_buf = NULL;
    unsigned char *p          = (unsigned char *)buf;

    /* Compute length required to convert DER data */
    size_t use_len = 0;
    mbedtls_base64_encode(NULL, 0, &use_len, der_data, der_len);
    if (0 == use_len) {
        mender_log_error("Unable to compute length");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Compute length required to format PEM */
    size_t add_len = strlen(PEM_BEGIN_PUBLIC_KEY) + 1 + strlen(PEM_END_PUBLIC_KEY) + ((use_len / 64) + 1);

    /* Check buffer length */
    if (use_len + add_len > buf_len) {
        *olen = use_len + add_len;
        ret   = MENDER_FAIL;
        goto END;
    }

    /* Check buffer */
    if (NULL == p) {
        ret = MENDER_FAIL;
        goto END;
    }

    /* Allocate memory to store PEM data */
    if (NULL == (encode_buf = (unsigned char *)malloc(use_len))) {
        mender_log_error("Unable to allocate memory");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Convert DER data */
    if (0 != mbedtls_base64_encode(encode_buf, use_len, &use_len, der_data, der_len)) {
        mender_log_error("Unable to convert data to base64 format");
        ret = MENDER_FAIL;
        goto END;
    }

    /* Copy header */
    memcpy(p, PEM_BEGIN_PUBLIC_KEY, strlen(PEM_BEGIN_PUBLIC_KEY));
    p += strlen(PEM_BEGIN_PUBLIC_KEY);
    *p++ = '\n';

    /* Copy PEM data */
    unsigned char *c = encode_buf;
    while (use_len) {
        size_t len = (use_len > 64) ? 64 : use_len;
        memcpy(p, c, len);
        use_len -= len;
        p += len;
        c += len;
        *p++ = '\n';
    }

    /* Copy footer */
    memcpy(p, PEM_END_PUBLIC_KEY, strlen(PEM_END_PUBLIC_KEY));
    p += strlen(PEM_END_PUBLIC_KEY);
    *p++ = '\0';

    /* Compute output length */
    *olen = p - (unsigned char *)buf;

    /* Clean any remaining data previously written to the buffer */
    memset(buf + *olen, 0, buf_len - *olen);

END:

    /* Release memory */
    if (NULL != encode_buf) {
        free(encode_buf);
    }

    return ret;
}
