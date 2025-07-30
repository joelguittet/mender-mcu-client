#ifndef __ATCA_BASIC_H__
#define __ATCA_BASIC_H__

#include "atca_status.h"

ATCA_STATUS atcab_info(uint8_t* revision);
ATCA_STATUS atcab_read_serial_number(uint8_t* serial_number);
ATCA_STATUS atcab_is_locked(uint8_t zone, bool* is_locked);
ATCA_STATUS atcab_get_pubkey(uint16_t key_id, uint8_t* public_key);
ATCA_STATUS atcab_hw_sha2_256(const uint8_t* data, size_t data_size, uint8_t* digest);
ATCA_STATUS atcab_sign(uint16_t key_id, const uint8_t* msg, uint8_t* signature);

#endif /* __ATCA_BASIC_H__ */
