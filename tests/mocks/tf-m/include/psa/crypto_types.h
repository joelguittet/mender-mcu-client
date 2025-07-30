#ifndef __CRYPTO_TYPES_H__
#define __CRYPTO_TYPES_H__

#include <stdint.h>
#include <mbedtls/private_access.h>
#include <psa/crypto_platform.h>

typedef uint8_t psa_ecc_family_t;
typedef uint32_t psa_key_lifetime_t;
typedef uint16_t psa_key_type_t;
typedef uint32_t psa_key_id_t;
typedef uint32_t psa_algorithm_t;
typedef uint32_t psa_key_usage_t;

typedef struct psa_key_attributes_s psa_key_attributes_t;

#if !defined(MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER)
typedef psa_key_id_t mbedtls_svc_key_id_t;
#else /* MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER */
typedef struct {
    psa_key_id_t MBEDTLS_PRIVATE(key_id);
    mbedtls_key_owner_id_t MBEDTLS_PRIVATE(owner);
} mbedtls_svc_key_id_t;
#endif /* !MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER */

#endif /* __CRYPTO_TYPES_H__ */
