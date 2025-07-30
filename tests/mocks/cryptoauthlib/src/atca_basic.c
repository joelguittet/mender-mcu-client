#include "atca_basic.h"

ATCA_STATUS atcab_info(uint8_t* revision) {
    return ATCA_SUCCESS;
}

ATCA_STATUS atcab_read_serial_number(uint8_t* serial_number) {
    return ATCA_SUCCESS;
}

ATCA_STATUS atcab_is_locked(uint8_t zone, bool* is_locked) {
    return ATCA_SUCCESS;
}

ATCA_STATUS atcab_get_pubkey(uint16_t key_id, uint8_t* public_key) {
    return ATCA_SUCCESS;
}

ATCA_STATUS atcab_hw_sha2_256(const uint8_t* data, size_t data_size, uint8_t* digest) {
    return ATCA_SUCCESS;
}

ATCA_STATUS atcab_sign(uint16_t key_id, const uint8_t* msg, uint8_t* signature) {
    return ATCA_SUCCESS;
}
