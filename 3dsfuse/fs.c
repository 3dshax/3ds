#include <stdio.h>
#include <string.h>

#include "types.h"
#include "fs.h"

#define SAVE_OFFSET	0x3000
#define FST_BLOCK_OFFSET	0x6c
#define FST_OFFSET		0x58

u32 fs_get_offset(u8 *buf) {
	return *(u32*)(buf + SAVE_OFFSET + FST_OFFSET);
}

int fs_get_start(u8 *buf) {
	u32 block_offset, fst_offset;

	block_offset = *(u32*)(buf + SAVE_OFFSET + FST_BLOCK_OFFSET);
	fst_offset   = *(u32*)(buf + SAVE_OFFSET + FST_OFFSET);

	return SAVE_OFFSET + (block_offset * 0x200) + fst_offset;
}

int fs_num_entries(u8 *buf) {
	u32 *p = (u32*)(buf + fs_get_start(buf));

	return p[0];
}

fst_entry *fs_get_by_name(u8 *buf, const char *name) {
	fst_entry *e;
	char name_buf[0x11];
	int i;

	memset(name_buf, 0, 0x11);

	e = (fst_entry*)(buf + fs_get_start(buf) + sizeof(fst_entry));

	for(i = 0; i < fs_num_entries(buf)-1; i++) {
		memcpy(name_buf, e->name, 0x10);

		if (strcmp(name_buf, name) == 0)
			return e;

		e++;
	}

	return NULL;
}
