#ifndef __CRYPTO_STRUCT_H__
#define __CRYPTO_STRUCT_H__

#include <mbedtls/private_access.h>
#include <psa/crypto_values.h>
#include <psa/crypto_se_driver.h>

typedef uint16_t psa_key_bits_t;

struct psa_key_policy_s {
    psa_key_usage_t MBEDTLS_PRIVATE(usage);
    psa_algorithm_t MBEDTLS_PRIVATE(alg);
    psa_algorithm_t MBEDTLS_PRIVATE(alg2);
};
typedef struct psa_key_policy_s psa_key_policy_t;

#define PSA_KEY_POLICY_INIT { 0, 0, 0 }

#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
#define PSA_KEY_ATTRIBUTES_MAYBE_SLOT_NUMBER 0, 0,
#else
#define PSA_KEY_ATTRIBUTES_MAYBE_SLOT_NUMBER
#endif

#define PSA_KEY_ATTRIBUTES_INIT { PSA_KEY_ATTRIBUTES_MAYBE_SLOT_NUMBER \
                                      PSA_KEY_TYPE_NONE, 0,            \
                                      PSA_KEY_LIFETIME_VOLATILE,       \
                                      PSA_KEY_POLICY_INIT,             \
                                      MBEDTLS_SVC_KEY_ID_INIT }

#if !defined(MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER)
typedef psa_key_id_t mbedtls_svc_key_id_t;
#else /* MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER */
typedef struct {
    psa_key_id_t MBEDTLS_PRIVATE(key_id);
    mbedtls_key_owner_id_t MBEDTLS_PRIVATE(owner);
} mbedtls_svc_key_id_t;
#endif /* !MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER */

struct psa_key_attributes_s {
#if defined(MBEDTLS_PSA_CRYPTO_SE_C)
    psa_key_slot_number_t MBEDTLS_PRIVATE(slot_number);
    int MBEDTLS_PRIVATE(has_slot_number);
#endif /* MBEDTLS_PSA_CRYPTO_SE_C */
    psa_key_type_t MBEDTLS_PRIVATE(type);
    psa_key_bits_t MBEDTLS_PRIVATE(bits);
    psa_key_lifetime_t MBEDTLS_PRIVATE(lifetime);
    psa_key_policy_t MBEDTLS_PRIVATE(policy);
    mbedtls_svc_key_id_t MBEDTLS_PRIVATE(id);
};

#endif /* __CRYPTO_STRUCT_H__ */
