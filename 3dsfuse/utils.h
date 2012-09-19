#ifndef _UTILS_H
#define _UTILS_H

#include <openssl/sha.h>
#include <stdint.h>

void sha256(void* buffer, size_t size, uint8_t* result);

#endif /* _UTILS_H */
