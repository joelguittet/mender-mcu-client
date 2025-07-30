#include "psa/protected_storage.h"

psa_status_t psa_ps_set(psa_storage_uid_t uid,
                        size_t data_length,
                        const void *p_data,
                        psa_storage_create_flags_t create_flags)
{
    return PSA_SUCCESS;
}

psa_status_t psa_ps_get(psa_storage_uid_t uid,
                        size_t data_offset,
                        size_t data_size,
                        void *p_data,
                        size_t *p_data_length)
{
    return PSA_SUCCESS;
}

psa_status_t psa_ps_get_info(psa_storage_uid_t uid,
                             struct psa_storage_info_t *p_info)
{
    return PSA_SUCCESS;
}

psa_status_t psa_ps_remove(psa_storage_uid_t uid)
{
    return PSA_SUCCESS;
}

