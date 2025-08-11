#ifndef __ERROR_H__
#define __ERROR_H__

#include <stddef.h>

void mbedtls_strerror(int errnum, char *buffer, size_t buflen);

#endif /* __ERROR_H__ */
