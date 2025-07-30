#ifndef __PSA_STORAGE_COMMON_H__
#define __PSA_STORAGE_COMMON_H__

#include <stddef.h>
#include <stdint.h>

typedef uint32_t psa_storage_create_flags_t;

typedef uint64_t psa_storage_uid_t;

#define PSA_STORAGE_FLAG_NONE        0u
#define PSA_STORAGE_FLAG_WRITE_ONCE (1u << 0)
#define PSA_STORAGE_FLAG_NO_CONFIDENTIALITY (1u << 1)
#define PSA_STORAGE_FLAG_NO_REPLAY_PROTECTION (1u << 2)

struct psa_storage_info_t {
    size_t capacity;
    size_t size;
    psa_storage_create_flags_t flags;
};

#define PSA_STORAGE_SUPPORT_SET_EXTENDED (1u << 0)

#define PSA_ERROR_INVALID_SIGNATURE     ((psa_status_t)-149)
#define PSA_ERROR_DATA_CORRUPT          ((psa_status_t)-152)

#endif /* __PSA_STORAGE_COMMON_H__ */
