#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "crypto.h"
#include "helper.h"

int find_key(u8 *buf, size_t len, u8 *out) {
	int i, j, count=0, found=0, rec_idx=0, rec_count=0;
	hash_entry **hash_list;
	u8 hash[16];

	u8 ff_hash[16]="\xde\x03\xfe\x65\xa6\x76\x5c\xaa\x8c\x91\x34\x3a\xcc\x62\xcf\xfc";

	hash_list = malloc(4 * ((len / 0x200)+1));

	for(i = 0; i < (len / 0x200); i++) {
		md5_buf(buf + (i*0x200), hash, 0x200);

		if(memcmp(hash, ff_hash, 16) == 0)
			continue;

		found = 0;

		for(j = 0; j < count; j++) {
			if (memcmp(hash_list[j]->hash, hash, 16) == 0) {
				hash_list[j]->count++;
				found = 1;
				break;
			}
		}

		// push new hashlist entry
		if(found == 0) {
			hash_list[count] = malloc(sizeof(hash_entry));
			memcpy(hash_list[count]->hash, hash, 16);
			hash_list[count]->count = 1;
			hash_list[count]->block_idx = i;
			count++;
		}
	}

	for(i = 0; i < count; i++) {
		if (hash_list[i]->count > rec_count) {
			rec_count = hash_list[i]->count;
			rec_idx = i;
		}
	}

	if (rec_count == 0)
		return -1;

#ifdef DEBUG
	printf("key hash: "); md5_print(hash_list[rec_idx]->hash); printf("\n");
#endif

	memcpy(out, buf + (hash_list[rec_idx]->block_idx * 0x200), 0x200);

	return 0;
}
