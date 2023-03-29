/**
 * @file      mender-tls.c
 * @brief     Mender TLS interface for mbedTLS platform
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

#include "mbedtls/base64.h"
#include "mbedtls/bignum.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#ifdef MBEDTLS_ERROR_C
#include "mbedtls/error.h"
#endif /* MBEDTLS_ERROR_C */
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "mbedtls/x509.h"
#include "mender-log.h"
#include "mender-tls.h"

/**
 * @brief Default keys length
 */
#ifndef MENDER_TLS_PRIVATE_KEY_LENGTH
#define MENDER_TLS_PRIVATE_KEY_LENGTH (2048)
#endif /* MENDER_TLS_PRIVATE_KEY_LENGTH */
#ifndef MENDER_TLS_PUBLIC_KEY_LENGTH
#define MENDER_TLS_PUBLIC_KEY_LENGTH (768)
#endif /* MENDER_TLS_PUBLIC_KEY_LENGTH */

/**
 * @brief Default signature length (base64 encoded)
 */
#ifndef MENDER_TLS_SIGNATURE_LENGTH
#define MENDER_TLS_SIGNATURE_LENGTH (512)
#endif /* MENDER_TLS_SIGNATURE_LENGTH */

mender_err_t
mender_tls_init(void) {

    /* Nothing to do */
    return MENDER_OK;
}

mender_err_t
mender_tls_generate_authentication_keys(unsigned char **private_key, size_t *private_key_length, unsigned char **public_key, size_t *public_key_length) {

    assert(NULL != private_key);
    assert(NULL != private_key_length);
    assert(NULL != public_key);
    assert(NULL != public_key_length);
    int                       ret;
    mbedtls_pk_context *      pk_context = NULL;
    mbedtls_ctr_drbg_context *ctr_drbg   = NULL;
    mbedtls_entropy_context * entropy    = NULL;
    unsigned char *           tmp;

#ifdef MBEDTLS_ERROR_C
    char err[128];
#endif /* MBEDTLS_ERROR_C */

    /* Initialize mbedtls */
    if (NULL == (pk_context = malloc(sizeof(mbedtls_pk_context)))) {
        mender_log_error("Unable to allocate memory");
        ret = -1;
        goto END;
    }
    mbedtls_pk_init(pk_context);
    if (NULL == (ctr_drbg = malloc(sizeof(mbedtls_ctr_drbg_context)))) {
        mender_log_error("Unable to allocate memory");
        ret = -1;
        goto END;
    }
    mbedtls_ctr_drbg_init(ctr_drbg);
    if (NULL == (entropy = malloc(sizeof(mbedtls_entropy_context)))) {
        mender_log_error("Unable to allocate memory");
        ret = -1;
        goto END;
    }
    mbedtls_entropy_init(entropy);

    /* Setup CRT DRBG */
    if (0 != (ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy, (const unsigned char *)"mender", strlen("mender")))) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to initialize ctr drbg (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to initialize ctr drbg (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        goto END;
    }

    /* PK setup */
    if (0 != (ret = mbedtls_pk_setup(pk_context, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)))) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to setup pk (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to setup pk (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        goto END;
    }

    /* Generate key pair */
    if (0 != (ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(*pk_context), mbedtls_ctr_drbg_random, ctr_drbg, 3072, 65537))) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to setup pk (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to setup pk (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        goto END;
    }

    /* Export private key */
    if (NULL == (*private_key = malloc(MENDER_TLS_PRIVATE_KEY_LENGTH))) {
        mender_log_error("Unable to allocate memory");
        ret = -1;
        goto END;
    }
    if ((ret = mbedtls_pk_write_key_der(pk_context, *private_key, MENDER_TLS_PRIVATE_KEY_LENGTH)) < 0) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to write private key to PEM format (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to write private key to PEM format (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        free(*private_key);
        *private_key = NULL;
        goto END;
    }
    *private_key_length = (size_t)ret;
    memcpy(*private_key, *private_key + MENDER_TLS_PRIVATE_KEY_LENGTH - *private_key_length, *private_key_length);
    if (NULL == (tmp = realloc(*private_key, *private_key_length))) {
        mender_log_error("Unable to allocate memory");
        free(*private_key);
        *private_key = NULL;
        ret          = -1;
        goto END;
    }
    *private_key = tmp;

    /* Export public key */
    if (NULL == (*public_key = malloc(MENDER_TLS_PUBLIC_KEY_LENGTH))) {
        mender_log_error("Unable to allocate memory");
        free(*private_key);
        *private_key = NULL;
        ret          = -1;
        goto END;
    }
    if ((ret = mbedtls_pk_write_pubkey_der(pk_context, *public_key, MENDER_TLS_PUBLIC_KEY_LENGTH)) < 0) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to write public key to PEM format (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to write public key to PEM format (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        free(*private_key);
        *private_key = NULL;
        free(*public_key);
        *public_key = NULL;
        goto END;
    }
    *public_key_length = (size_t)ret;
    memcpy(*public_key, *public_key + MENDER_TLS_PUBLIC_KEY_LENGTH - *public_key_length, *public_key_length);
    if (NULL == (tmp = realloc(*public_key, *public_key_length))) {
        mender_log_error("Unable to allocate memory");
        free(*private_key);
        *private_key = NULL;
        free(*public_key);
        *public_key = NULL;
        ret         = -1;
        goto END;
    }
    *public_key = tmp;
    ret         = 0;

END:

    /* Release mbedtls */
    if (NULL != entropy) {
        mbedtls_entropy_free(entropy);
        free(entropy);
    }
    if (NULL != ctr_drbg) {
        mbedtls_ctr_drbg_free(ctr_drbg);
        free(ctr_drbg);
    }
    if (NULL != pk_context) {
        mbedtls_pk_free(pk_context);
        free(pk_context);
    }

    return (0 != ret) ? MENDER_FAIL : MENDER_OK;
}

mender_err_t
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
        ret = MENDER_FAIL;
        goto END;
    }

    /* Compute length required to format PEM */
    size_t add_len = strlen(PEM_BEGIN_PUBLIC_KEY) + 2 + strlen(PEM_END_PUBLIC_KEY) + 2 * ((use_len / 64) + 1);

    /* Check buffer length */
    if (use_len + add_len > buf_len) {
        *olen = use_len + add_len;
        ret   = MENDER_FAIL;
        goto END;
    }

    /* Allocate memory to store PEM data */
    if (NULL == (encode_buf = malloc(use_len))) {
        ret = MENDER_FAIL;
        goto END;
    }

    /* Convert DER data */
    if (0 != mbedtls_base64_encode(encode_buf, use_len, &use_len, der_data, der_len)) {
        ret = MENDER_FAIL;
        goto END;
    }

    /* Copy header */
    memcpy(p, PEM_BEGIN_PUBLIC_KEY, strlen(PEM_BEGIN_PUBLIC_KEY));
    p += strlen(PEM_BEGIN_PUBLIC_KEY);
    *p++ = '\\';
    *p++ = 'n';

    /* Copy PEM data */
    unsigned char *c = encode_buf;
    while (use_len) {
        size_t len = (use_len > 64) ? 64 : use_len;
        memcpy(p, c, len);
        use_len -= len;
        p += len;
        c += len;
        *p++ = '\\';
        *p++ = 'n';
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

mender_err_t
mender_tls_sign_payload(unsigned char *private_key, size_t private_key_length, char *payload, char **signature, size_t *signature_length) {

    assert(NULL != private_key);
    assert(NULL != payload);
    assert(NULL != signature);
    assert(NULL != signature_length);
    int                       ret;
    mbedtls_pk_context *      pk_context = NULL;
    mbedtls_ctr_drbg_context *ctr_drbg   = NULL;
    mbedtls_entropy_context * entropy    = NULL;
    unsigned char *           sig        = NULL;
    size_t                    sig_length;
    char *                    tmp;
#ifdef MBEDTLS_ERROR_C
    char err[128];
#endif /* MBEDTLS_ERROR_C */

    /* Initialize mbedtls */
    if (NULL == (pk_context = malloc(sizeof(mbedtls_pk_context)))) {
        mender_log_error("Unable to allocate memory");
        ret = -1;
        goto END;
    }
    mbedtls_pk_init(pk_context);
    if (NULL == (ctr_drbg = malloc(sizeof(mbedtls_ctr_drbg_context)))) {
        mender_log_error("Unable to allocate memory");
        ret = -1;
        goto END;
    }
    mbedtls_ctr_drbg_init(ctr_drbg);
    if (NULL == (entropy = malloc(sizeof(mbedtls_entropy_context)))) {
        mender_log_error("Unable to allocate memory");
        ret = -1;
        goto END;
    }
    mbedtls_entropy_init(entropy);

    /* Setup CRT DRBG */
    if (0 != (ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy, (const unsigned char *)"mender", strlen("mender")))) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to initialize ctr drbg (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to initialize ctr drbg (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        goto END;
    }

    /* Parse private key (IMPORTANT NOTE: length must include the ending \0 character) */
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    if (0 != (ret = mbedtls_pk_parse_key(pk_context, private_key, private_key_length, NULL, 0, mbedtls_ctr_drbg_random, ctr_drbg))) {
#else
    if (0 != (ret = mbedtls_pk_parse_key(pk_context, private_key, private_key_length, NULL, 0))) {
#endif /* MBEDTLS_VERSION_NUMBER >= 0x03000000 */
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to parse private key (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to parse private key (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        goto END;
    }

    /* Generate digest */
    uint8_t digest[32];
    if (0 != (ret = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), (unsigned char *)payload, strlen(payload), digest))) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to generate digest (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to generate digest (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        goto END;
    }

    /* Compute signature */
    if (NULL == (sig = malloc(MENDER_TLS_SIGNATURE_LENGTH + 1))) {
        mender_log_error("Unable to allocate memory");
        ret = -1;
        goto END;
    }
    sig_length = MENDER_TLS_SIGNATURE_LENGTH + 1;
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    if (0 != (ret = mbedtls_pk_sign(pk_context, MBEDTLS_MD_SHA256, digest, sizeof(digest), sig, sig_length, &sig_length, mbedtls_ctr_drbg_random, ctr_drbg))) {
#else
    if (0 != (ret = mbedtls_pk_sign(pk_context, MBEDTLS_MD_SHA256, digest, sizeof(digest), sig, &sig_length, mbedtls_ctr_drbg_random, ctr_drbg))) {
#endif /* MBEDTLS_VERSION_NUMBER >= 0x03000000 */
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to compute signature (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to compute signature (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        goto END;
    }

    /* Encode signature to base64 */
    if (NULL == (*signature = malloc(MENDER_TLS_SIGNATURE_LENGTH + 1))) {
        mender_log_error("Unable to allocate memory");
        ret = -1;
        goto END;
    }
    *signature_length = MENDER_TLS_SIGNATURE_LENGTH + 1;
    if (0 != (ret = mbedtls_base64_encode((unsigned char *)*signature, *signature_length, signature_length, sig, sig_length))) {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror(ret, err, sizeof(err));
        mender_log_error("Unable to encode signature (-0x%04x: %s)", -ret, err);
#else
        mender_log_error("Unable to encode signature (-0x%04x)", -ret);
#endif /* MBEDTLS_ERROR_C */
        free(*signature);
        *signature = NULL;
        goto END;
    }
    *signature_length = strlen(*signature);
    if (NULL == (tmp = realloc(*signature, *signature_length + 1))) {
        mender_log_error("Unable to allocate memory");
        free(*signature);
        *signature = NULL;
        ret        = -1;
        goto END;
    }
    *signature = tmp;

END:

    /* Release mbedtls */
    if (NULL != entropy) {
        mbedtls_entropy_free(entropy);
        free(entropy);
    }
    if (NULL != ctr_drbg) {
        mbedtls_ctr_drbg_free(ctr_drbg);
        free(ctr_drbg);
    }
    if (NULL != pk_context) {
        mbedtls_pk_free(pk_context);
        free(pk_context);
    }

    /* Release memory */
    if (NULL != sig) {
        free(sig);
    }

    return (0 != ret) ? MENDER_FAIL : MENDER_OK;
}

mender_err_t
mender_tls_exit(void) {

    /* Nothing to do */
    return MENDER_OK;
}