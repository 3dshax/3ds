#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include "types.h"

typedef struct {
	u8 hash[16];
	u32 block_idx;
	u32 count;
} hash_entry;

int find_key(u8 *buf, size_t len, u8 *out);

#endif
