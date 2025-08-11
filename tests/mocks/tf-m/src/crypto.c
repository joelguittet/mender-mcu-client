#include <psa/crypto.h>

psa_status_t psa_crypto_init(void) {
    return PSA_SUCCESS;
}

void psa_set_key_id(psa_key_attributes_t *attributes, mbedtls_svc_key_id_t key) {
}

void psa_set_key_lifetime(psa_key_attributes_t *attributes, psa_key_lifetime_t lifetime) {
}

void psa_set_key_bits(psa_key_attributes_t *attributes, size_t bits) {
}

void psa_set_key_usage_flags(psa_key_attributes_t *attributes, psa_key_usage_t usage_flags) {
}

void psa_set_key_algorithm(psa_key_attributes_t *attributes, psa_algorithm_t alg) {
}

void psa_set_key_type(psa_key_attributes_t *attributes, psa_key_type_t type) {
}

psa_status_t psa_generate_key(const psa_key_attributes_t *attributes, mbedtls_svc_key_id_t *key) {
    return PSA_SUCCESS;
}

psa_status_t psa_export_public_key(mbedtls_svc_key_id_t key,
                                   uint8_t *data,
                                   size_t data_size,
                                   size_t *data_length) {
    return PSA_SUCCESS;
}

psa_status_t psa_sign_message(mbedtls_svc_key_id_t key,
                              psa_algorithm_t alg,
                              const uint8_t *input,
                              size_t input_length,
                              uint8_t *signature,
                              size_t signature_size,
                              size_t *signature_length) {
    return PSA_SUCCESS;
}

psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key) {
    return PSA_SUCCESS;
}
