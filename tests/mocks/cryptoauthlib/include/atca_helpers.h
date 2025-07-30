#ifndef __ATCA_HELPERS_H__
#define __ATCA_HELPERS_H__

#include "atca_status.h"

ATCA_STATUS atcab_base64encode_(const uint8_t* data, size_t data_size, char* encoded, size_t* encoded_size, const uint8_t * rules);

#endif /* __ATCA_HELPERS_H__ */
