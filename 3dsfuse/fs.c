#include <stdio.h>
#include <string.h>

#include "types.h"
#include "fs.h"

#define BASE_OFFSET	0x2000

#define SAVE_OFFSET	0x3000

#define FST_BLOCK_OFFSET	0x6c
#define FST_OFFSET		0x58



u8 *fs_part_get_info(u8 *buf, u32 part_no) {
	return buf + 0x200 + (part_no * 0x130);
}

u8 *fs_part(u32 part_no, u8 *buf) {
	u32 pos = BASE_OFFSET;
	u8 *p = buf + 0x200;
	int num = 0;

	while(!strncmp((char*)p, "DIFI", 4)) {
		if (num == part_no) {
			printf("part %d is @ %08x\n", num, pos);
			return buf + pos;
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

int fs_get_start(u8 *part, u8 *part_info) {
	u32 block_offset, fst_offset;
	u32 *info = (u32*)part_info;

	block_offset = *(u32*)(part + info[0x9c / 4] + FST_BLOCK_OFFSET);
	fst_offset   = *(u32*)(part + info[0x9c / 4] + FST_OFFSET);

	printf("block_offset: %08x - fst_offset: %08x\n", block_offset, fst_offset);

	return info[0x9C / 4] + (block_offset * 0x200) + fst_offset;
}

int fs_num_entries(u8 *part, u8 *part_info) {
	u32 *info = (u32*)part_info;
	u32 *p = (u32*)(part + info[0x9c / 4] + fs_get_start(part, part_info));

	return p[0];
}

fst_entry *fs_get_by_name(u8 *buf, u8 *part_info, const char *name) {
	fst_entry *e;
	char name_buf[0x11];
	int i;

	printf("by_name: '%s'\n", name);

	if (buf == NULL)
		return NULL;

	memset(name_buf, 0, 0x11);

	e = (fst_entry*)(buf + fs_get_start(buf, part_info) + sizeof(fst_entry));

	for(i = 0; i < fs_num_entries(buf, part_info)-1; i++) {
		printf("name: '%s'\n", e->name);
		memcpy(name_buf, e->name, 0x10);

		if (strcmp(name_buf, name) == 0)
			return e;

		e++;
	}
	printf("lol\n");

	return NULL;
}
