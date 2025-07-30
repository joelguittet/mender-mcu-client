#ifndef __CRYPTO_VALUES_H__
#define __CRYPTO_VALUES_H__

#include <psa/crypto_types.h>

#define MBEDTLS_SVC_KEY_ID_INIT ((psa_key_id_t) 0)

#define PSA_ECC_FAMILY_SECP_R1 ((psa_ecc_family_t) 0x12)

#define PSA_ALG_SHA_256 ((psa_algorithm_t) 0x02000009)
#define PSA_ALG_HASH_MASK ((psa_algorithm_t) 0x000000ff)
#define PSA_ALG_DETERMINISTIC_ECDSA_BASE ((psa_algorithm_t) 0x06000700)
#define PSA_ALG_DETERMINISTIC_ECDSA(hash_alg)                           \
    (PSA_ALG_DETERMINISTIC_ECDSA_BASE | ((hash_alg) & PSA_ALG_HASH_MASK))

#define PSA_KEY_TYPE_NONE ((psa_key_type_t) 0x0000)
#define PSA_KEY_TYPE_ECC_KEY_PAIR_BASE ((psa_key_type_t) 0x7100)
#define PSA_KEY_TYPE_ECC_KEY_PAIR(curve)         \
    (PSA_KEY_TYPE_ECC_KEY_PAIR_BASE | (curve))

#define PSA_KEY_LIFETIME_VOLATILE ((psa_key_lifetime_t) 0x00000000)
#define PSA_KEY_LIFETIME_PERSISTENT ((psa_key_lifetime_t) 0x00000001)

#define PSA_KEY_USAGE_SIGN_MESSAGE ((psa_key_usage_t) 0x00000400)

#define PSA_KEY_POLICY_INIT { 0, 0, 0 }

#endif /* __CRYPTO_VALUES_H__ */
