#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "types.h"
#include "fs.h"

#define FST_BLOCK_OFFSET	0x6c
#define FST_OFFSET		0x58

typedef struct {
	u8 *sav;

	u32 total_partentries;
	u32 savepart_offset;
	u32 datapart_offset;

	u8 activepart_table;
	u32 datapart_filebase;
	u32 activepart_tableoffset;
	u32 part_tablesize;

	u8 activepart_tablehash[0x20];
} fs_savectx;

static fs_savectx savectx;

int fs_initsave_disa();
int fs_checkheaderpart_tablehash();

int fs_initsave(u8 *nsav) {
	u32 magic;

	savectx.sav = nsav;
	magic = *((u32*)&nsav[0x100]);

	if(magic == 0x41534944)
	{
		if(fs_initsave_disa())return 1;
	}
	else if(magic == 0x46464944)
	{
		printf("extdata DIFF is not yet supported.\n");
		return 1;
	}
	else
	{
		printf("unknown magic %x @ 0x100.\n", magic);
		return 1;
	}

	return fs_checkheaderpart_tablehash();
}

int fs_initsave_disa() {		
	disa_header *disa = (disa_header*)&savectx.sav[0x100];

	savectx.total_partentries = (u32)disa->total_partentries;
	savectx.savepart_offset = disa->save_partoffset;
	savectx.datapart_offset = disa->data_partoffset;
	savectx.activepart_table = (u8)disa->activepart_table;

	if(savectx.activepart_table)savectx.activepart_tableoffset = (u32)disa->secondarytable_offset;
	if(!savectx.activepart_table)savectx.activepart_tableoffset = (u32)disa->primarytable_offset;

	savectx.datapart_filebase = 0;
	if(savectx.datapart_offset)savectx.datapart_filebase = disa->activepart_table & ~0xFF;

	savectx.part_tablesize = (u32)disa->part_tablesize;
	memcpy(savectx.activepart_tablehash, disa->activepart_tablehash, 0x20);

	return 0;
}

int fs_checkheaderpart_tablehash()
{
	u8 calchash[0x20];

	sha256(&savectx.sav[savectx.activepart_tableoffset], savectx.part_tablesize, calchash);
	if(memcmp(savectx.activepart_tablehash, calchash, 0x20)!=0)
	{
		printf("active table hash from header is invalid.\n");
		return 2;
	}

	return 0;
}

u8 *fs_part_get_info(u8 *buf, u32 part_no) {
	return buf + savectx.activepart_tableoffset + (part_no * 0x130);
}

u8 *fs_part(u8 *buf) {
	u64 pos = 0;
	u8 *p = buf + savectx.activepart_tableoffset;// (part_no*0x130);
	partition_table *part = (partition_table*)p;
	u32 num = 0;

	pos = savectx.savepart_offset + part->dpfs.ivfcpart_offset;

	for(num=0; num<2; num++) {
		part = (partition_table*)p;
		if(part->difi.magic != 0x49464944)
		{
			printf("invalid DIFI magic.\n");
			return NULL;
		}

		if(num == (u32)savectx.activepart_table)
		{
			return buf + pos + part->ivfc.fs_offset;
		}

		pos += part->dpfs.ivfcpart_size;
	
		p += 0x130 * savectx.total_partentries;
	}

	return NULL;
}

int fs_num_partition(u8 *buf) {
	return 1;
}

u8 *fs_getfilebase()
{
	u8 *part;

	if(savectx.datapart_offset==0)
	{
		part = fs_part(savectx.sav);
		return part + fs_get_offset(part);
	}

	return savectx.sav + (savectx.datapart_offset + savectx.datapart_filebase);
}

u32 fs_get_offset(u8 *buf) {
	return *(u32*)(buf + FST_OFFSET);
}

u32 fs_get_start(u8 *part) {
	u32 block_offset, fst_offset;

	block_offset = *(u32*)(part + 0x78);
	fst_offset   = *(u32*)(part + 0x58);

	if(savectx.datapart_offset==0)return (block_offset * 0x200) + fst_offset;
	return block_offset;
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
