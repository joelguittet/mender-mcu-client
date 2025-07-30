#ifndef __BASE64_H__
#define __BASE64_H__

#include <stddef.h>

int mbedtls_base64_encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen);

#endif /* __BASE64_H__ */
