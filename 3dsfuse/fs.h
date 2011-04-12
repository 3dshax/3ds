#ifndef __FS_H__
#define __FS_H__

#include "types.h"

typedef struct {
	u32 unk0;
	u8 name[0x10];
	u32 id;
	u32 unk1;
	u32 block_no;
	u32 size;
	u32 unk3, unk4, unk5;
} fst_entry;

int fs_num_entries(u8 *buf);
int fs_get_start(u8 *buf);
u32 fs_get_offset(u8 *buf);
u8 *fs_part_get_info(u8 *buf, u32 part_no);
u8 *fs_part(u32 part_no, u8 *buf);
fst_entry *fs_get_by_name(u8 *buf, const char *name);
int fs_num_partition(u8 *buf);

#endif
