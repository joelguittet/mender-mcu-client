/**
 * @file      mender-tls.c
 * @brief     Mender TLS interface for cryptoauthlib platform
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

#include <cryptoauthlib.h>
#include "mender-log.h"
#include "mender-tls.h"

/**
 * @brief Default private key ID
 */
#ifndef CONFIG_MENDER_TLS_PRIVATE_KEY_ID
#define CONFIG_MENDER_TLS_PRIVATE_KEY_ID (0)
#endif /* CONFIG_MENDER_TLS_PRIVATE_KEY_ID */

/**
 * @brief base64 rules to format public key and signature
 */
static const uint8_t mender_tls_atcab_b64rules[4] = { (uint8_t)'+', (uint8_t)'/', (uint8_t)'=', 0u };

/**
 * @brief Public key x509 header
 */
static const uint8_t mender_tls_public_key_x509_header[] = { 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06,
                                                             0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04 };

/**
 * @brief Public key of the device
 */
static unsigned char *mender_tls_public_key        = NULL;
static size_t         mender_tls_public_key_length = 0;

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

    uint8_t revision[4];
    uint8_t serial_number[ATCA_SERIAL_NUM_SIZE];
    bool    lock = false;

    /* 'atcab_init' is supposed to be called first in the user application to initialize the library with the wanted HAL settings */

    /* Check secure element information */
    if (ATCA_SUCCESS != atcab_info(revision)) {
        mender_log_error("Unable to get the secure element revision information");
        return MENDER_FAIL;
    }
    mender_log_info("Secure element revision information: '%02x%02x%02x%02x'", revision[0], revision[1], revision[2], revision[3]);
    if (0x02 == revision[3]) {
        mender_log_info("Secure element is an ATECC608A");
    } else if (0x03 == revision[3]) {
        mender_log_info("Secure element is an ATECC608B");
    }

    /* Check secure element serial number */
    if (ATCA_SUCCESS != atcab_read_serial_number(serial_number)) {
        mender_log_error("Unable to get the secure element serial number");
        return MENDER_FAIL;
    }
    mender_log_info("Secure element serial number is: '%02x%02x%02x%02x%02x%02x%02x%02x%02x'",
                    serial_number[0],
                    serial_number[1],
                    serial_number[2],
                    serial_number[3],
                    serial_number[4],
                    serial_number[5],
                    serial_number[6],
                    serial_number[7],
                    serial_number[8]);

    /* Ensure the data is locked (device has been provisioned) */
    if (ATCA_SUCCESS != atcab_is_locked(LOCK_ZONE_DATA, &lock)) {
        mender_log_error("Unable to check if the secure element is locked");
        return MENDER_FAIL;
    }
    if (true != lock) {
        mender_log_error("Secure element is not locked");
        return MENDER_FAIL;
    }

    return MENDER_OK;
}

mender_err_t
mender_tls_init_authentication_keys(bool recommissioning) {

    /* Release memory */
    if (NULL != mender_tls_public_key) {
        free(mender_tls_public_key);
        mender_tls_public_key = NULL;
    }
    mender_tls_public_key_length = 0;

    /* Check if recommissioning is forced */
    if (true == recommissioning) {
        mender_log_warning("Recommissioning not supported");
    }

    /* Retrieve public key */
    if (NULL == (mender_tls_public_key = (unsigned char *)malloc(ATCA_PUB_KEY_SIZE))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }
    if (ATCA_SUCCESS != atcab_get_pubkey(CONFIG_MENDER_TLS_PRIVATE_KEY_ID, (uint8_t *)mender_tls_public_key)) {
        mender_log_error("Unable to get public key");
        free(mender_tls_public_key);
        mender_tls_public_key = NULL;
        return MENDER_FAIL;
    }
    mender_tls_public_key_length = ATCA_PUB_KEY_SIZE;

    return MENDER_OK;
}

mender_err_t
mender_tls_get_public_key_pem(char **public_key) {

    assert(NULL != public_key);
    mender_err_t ret;

    /* Compute size of the public key */
    size_t olen = 0;
    mender_tls_pem_write_buffer(mender_tls_public_key, mender_tls_public_key_length, NULL, 0, &olen);
    if (0 == olen) {
        mender_log_error("Unable to compute public key size");
        return MENDER_FAIL;
    }
    if (NULL == (*public_key = (char *)malloc(olen))) {
        mender_log_error("Unable to allocate memory");
        return MENDER_FAIL;
    }

    /* Convert public key from DER to PEM format */
    if (MENDER_OK != (ret = mender_tls_pem_write_buffer(mender_tls_public_key, mender_tls_public_key_length, *public_key, olen, &olen))) {
        mender_log_error("Unable to convert public key");
        return ret;
    }

    return MENDER_OK;
}

mender_err_t
mender_tls_sign_payload(char *payload, char **signature, size_t *signature_length) {

    assert(NULL != payload);
    assert(NULL != signature);
    assert(NULL != signature_length);
    uint8_t  digest[ATCA_SHA256_DIGEST_SIZE];
    uint8_t  sign[ATCA_ECCP256_SIG_SIZE];
    uint8_t *r = &sign[0];
    uint8_t *s = &sign[32];
    uint8_t  asn1[72];
    size_t   asn1_len = 0;
    char    *tmp;

    /* Compute digest (sha256) of the payload */
    if (ATCA_SUCCESS != atcab_hw_sha2_256(payload, strlen(payload), digest)) {
        mender_log_error("Unable to compute digest of the payload");
        return MENDER_FAIL;
    }

    /* Compute signature of the digest value */
    if (ATCA_SUCCESS != atcab_sign(CONFIG_MENDER_TLS_PRIVATE_KEY_ID, digest, sign)) {
        mender_log_error("Unable to compute signature of the digest value");
        return MENDER_FAIL;
    }

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
    if (ATCA_SUCCESS != atcab_base64encode_(asn1, asn1_len, *signature, signature_length, mender_tls_atcab_b64rules)) {
        mender_log_error("Unable to convert signature to base64 format");
        free(*signature);
        *signature = NULL;
        return MENDER_FAIL;
    }
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
mender_tls_pem_write_buffer(const unsigned char *der_data, size_t der_len, char *buf, size_t buf_len, size_t *olen) {

#define PEM_BEGIN_PUBLIC_KEY "-----BEGIN PUBLIC KEY-----"
#define PEM_END_PUBLIC_KEY   "-----END PUBLIC KEY-----"

    mender_err_t   ret        = MENDER_OK;
    unsigned char *encode_buf = NULL;
    unsigned char *p          = (unsigned char *)buf;

    /* Compute length required to convert DER data */
    size_t use_len = 2 * ATCA_PUB_KEY_SIZE;

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
    uint8_t *tmp = buf + 2 * ATCA_PUB_KEY_SIZE - der_len - sizeof(mender_tls_public_key_x509_header);
    memcpy(tmp, mender_tls_public_key_x509_header, sizeof(mender_tls_public_key_x509_header));
    memcpy(tmp + sizeof(mender_tls_public_key_x509_header), der_data, der_len);
    if (ATCA_SUCCESS != atcab_base64encode_(tmp, der_len + sizeof(mender_tls_public_key_x509_header), encode_buf, &use_len, mender_tls_atcab_b64rules)) {
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
