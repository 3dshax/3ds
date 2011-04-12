#ifndef __WEARLEVEL_H__
#define __WEARLEVEL_H__

#include "types.h"

typedef struct {
	u8 phys_no;
	u8 virt_no;
} mapping_entry;

typedef struct {
	u8 checksum[8];
	u8 phys_no;
	u8 unk;
} blockmap_entry;

typedef struct {
	u8 virt_no;
	u8 virt_prev_no;
	u8 phys_no;
	u8 phys_prev_no;
	u8 phys_realloc_cnt;
	u8 virt_realloc_cnt;
	u8 checksum[8];
} journal_data;

typedef struct {
	journal_data data;
	journal_data dupe_data;
	u8 magic[4];
} journal_entry;

int blockmap_get_size(u8 *buf);
u8 *journal_get_start(u8 *buf);
int journal_get_size(u8 *buf);
int rearrange(u8 *buf, u8 *out, int size);

#endif
