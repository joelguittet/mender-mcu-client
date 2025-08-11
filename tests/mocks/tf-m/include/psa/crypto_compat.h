#ifndef __CRYPTO_COMPAT_H__
#define __CRYPTO_COMPAT_H__

#include <psa/crypto_types.h>
#include <psa/error.h>

typedef mbedtls_svc_key_id_t psa_key_handle_t;

psa_status_t psa_open_key(mbedtls_svc_key_id_t key, psa_key_handle_t *handle);
psa_status_t psa_close_key(psa_key_handle_t handle);

#endif /* __CRYPTO_COMPAT_H__ */
