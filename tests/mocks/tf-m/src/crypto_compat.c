#include <psa/crypto_compat.h>

psa_status_t psa_open_key(mbedtls_svc_key_id_t key, psa_key_handle_t *handle) {
    return PSA_SUCCESS;
}

psa_status_t psa_close_key(psa_key_handle_t handle) {
    return PSA_SUCCESS;
}
