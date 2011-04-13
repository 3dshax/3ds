#include <stdio.h>
#include <string.h>

#include "types.h"
#include "fs.h"

#define BASE_OFFSET	0x2000

#define SAVE_OFFSET	0x3000

#define FST_BLOCK_OFFSET	0x6c
#define FST_OFFSET		0x58

u8 *sav = NULL;

void fs_set_sav(u8 *nsav) {
	sav = nsav;
}

u8 *fs_part_get_info(u8 *buf, u32 part_no) {
	return buf + 0x200 + (part_no * 0x130);
}

u8 *fs_part(u32 part_no, u8 *buf) {
	u32 pos = BASE_OFFSET;
	u8 *p = buf + 0x200;
	int num = 0;

	while(!strncmp((char*)p, "DIFI", 4)) {
		if (num == part_no) {
			return buf + pos + *(u32*)(p+0x9c);
		}

		pos += *(u32*)(p + 0xa4);

		p += 0x130;

		num++;
	}

	return NULL;
}

int fs_num_partition(u8 *buf) {
	u8 *p = buf + 0x200;
	int num = 0;

	while(!strncmp((char*)p, "DIFI", 4)) {
		num++;
		p += 0x130;
	}

	return num;
}

u32 fs_get_offset(u8 *buf) {
	return *(u32*)(buf + FST_OFFSET);
}

int fs_get_start(u8 *part) {
	u32 block_offset, fst_offset;
	u32 pos = BASE_OFFSET;
	u32 *info;
	u8 *p;
	int i;

	block_offset = *(u32*)(part + FST_BLOCK_OFFSET);
	fst_offset   = *(u32*)(part + FST_OFFSET);

	return (block_offset * 0x200) + fst_offset;
}

int fs_num_entries(u8 *part) {
	u32 *p = (u32*)(part + fs_get_start(part));

	return p[0];
}

fst_entry *fs_get_by_name(u8 *part, const char *name) {
	fst_entry *e;
	char name_buf[0x11];
	int i;

	if (part == NULL) {
		return NULL;
	}

	memset(name_buf, 0, 0x11);

	e = (fst_entry*)(part + fs_get_start(part) + sizeof(fst_entry));

	for(i = 0; i < fs_num_entries(part)-1; i++) {
		memcpy(name_buf, e->name, 0x10);

		if (strcmp(name_buf, name) == 0)
			return e;

		e++;
	}

	return NULL;
}
