#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"
#include "types.h"
#include "fs.h"

#define FST_BLOCK_OFFSET	0x6c
#define FST_OFFSET		0x58

typedef struct {
	u8 *sav;
	int sav_updated;

	u32 total_partentries;
	u32 savepart_offset;
	u32 datapart_offset;

	u8 activepart_table;
	u32 datapart_filebase;
	u32 activepart_tableoffset;
	u32 part_tablesize;

	u32 primary_tableoffset;
	u32 secondary_tableoffset;

	u8 activepart_tablehash[0x20];
} fs_savectx;

static fs_savectx savectx;

int fs_initsave_disa();
int fs_checkheaderhashes(int update);

int fs_initsave(u8 *nsav) {
	u32 magic;

	savectx.sav = nsav;
	savectx.sav_updated = 0;
	magic = *((u32*)&nsav[0x100]);

	if(magic == 0x41534944) {
		if(fs_initsave_disa())return 1;
	}
	else if(magic == 0x46464944) {
		printf("extdata DIFF is not yet supported.\n");
		return 1;
	}
	else {
		printf("unknown magic %x @ 0x100.\n", magic);
		return 1;
	}

	return fs_checkheaderhashes(0);
}

int fs_initsave_disa() {		
	disa_header *disa = (disa_header*)&savectx.sav[0x100];

	savectx.total_partentries = (u32)disa->total_partentries;
	savectx.savepart_offset = disa->save_partoffset;
	savectx.datapart_offset = disa->data_partoffset;
	savectx.activepart_table = (u8)disa->activepart_table;

	if(savectx.activepart_table)savectx.activepart_tableoffset = (u32)disa->secondarytable_offset;
	if(!savectx.activepart_table)savectx.activepart_tableoffset = (u32)disa->primarytable_offset;

	savectx.primary_tableoffset = (u32)disa->primarytable_offset;
	savectx.secondary_tableoffset = (u32)disa->secondarytable_offset;

	savectx.datapart_filebase = 0;
	if(savectx.datapart_offset)savectx.datapart_filebase = disa->activepart_table & ~0xFF;

	savectx.part_tablesize = (u32)disa->part_tablesize;
	memcpy(savectx.activepart_tablehash, disa->activepart_tablehash, 0x20);

	return 0;
}

int fs_checkheaderhashes(int update)
{
	int updated = 0;
	u8 *part;
	partition_table *table;
	u8 *lvl1_buf;
	u32 blksz;
	u8 calchash[0x20];

	table = (partition_table*)&savectx.sav[savectx.activepart_tableoffset];
	part = fs_part(savectx.sav, 0, 0);

	if(table == NULL || part == NULL) {
		printf("invalid partition.\n");
		return 0;
	}

	memset(calchash, 0, 0x20);

	blksz = 1 << table->ivfc.levels[0].blksize;
	lvl1_buf = (u8*)malloc(blksz);
	if(lvl1_buf==NULL) {
		printf("failed to allocate level1 buffer.\n");
		return 3;
	}
	memset(lvl1_buf, 0, blksz);
	memcpy(lvl1_buf, part, table->ivfc.levels[0].size);

	sha256(lvl1_buf, blksz, calchash);
	free(lvl1_buf);

	if(memcmp(table->ivfcpart_masterhash, calchash, 0x20)!=0) {
		if(update) {
			updated = 1;
			memcpy(table->ivfcpart_masterhash, calchash, 0x20);
		}
		else {
			printf("master hash over the IVFC partition is invalid.\n");
			return 2;
		}
	}

	sha256(&savectx.sav[savectx.activepart_tableoffset], savectx.part_tablesize, calchash);
	if(memcmp(savectx.activepart_tablehash, calchash, 0x20)!=0) {
		if(updated) {
			memcpy(savectx.activepart_tablehash, calchash, 0x20);
		}
		else {
			printf("active table hash from header is invalid.\n");
			return 2;
		}
	}

	if(updated)savectx.sav_updated = updated;

	return 0;
}

partition_table *fs_part_get_info(u8 *buf, u32 part_no) {
	return (partition_table*)(buf + savectx.activepart_tableoffset + (part_no * 0x130));
}

u8 *fs_part(u8 *buf, int fs, int datapart) {
	u64 pos = 0;
	u8 *p = buf + savectx.primary_tableoffset;
	partition_table *part = (partition_table*)p;
	int num = 0;

	if(!datapart)pos = savectx.savepart_offset;
	if(datapart)pos = savectx.datapart_offset;
	pos += part->dpfs.ivfcpart_offset;

	for(num=0; num<2; num++) {
		if(num==1)p = buf + savectx.secondary_tableoffset;
		if(datapart)p += 0x130;

		part = (partition_table*)p;
		if(part->difi.magic != 0x49464944) {
			printf("invalid DIFI magic.\n");
			return NULL;
		}

		if(num == (int)savectx.activepart_table) {
			//printf("datapart %x pos %llx ivfcpartsize %llx\n", datapart, pos, part->dpfs.ivfcpart_size);
			if(!fs)return buf + pos;
			return buf + pos + part->ivfc.levels[3].offset;
		}

		pos += part->dpfs.ivfcpart_size;
	}

	return NULL;
}

int fs_num_partition(u8 *buf) {
	return 1;
}

u8 *fs_getfilebase()
{
	u8 *part;

	if(savectx.datapart_offset==0) {
		part = fs_part(savectx.sav, 1, 0);
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

int fs_verifyhashtree(u8 *part, u8 *hashtree, ivfc_header *ivfc, u32 offset, u32 size, u32 level, int update)
{
	int updated = 0;
	int ret = 0;

	u32 blksz, levelsize, chunksize;
	u32 curpos = 0, hashpos = 0, cursize = 0;
	u32 curhashpos = 0, totalhashsize = 0x20;
	u32 hashtable_pos;

	u8 *hashtable;
	u8 *hashdata;
	u8 calchash[0x20];

	hashtable_pos = (u32)ivfc->levels[level-1].offset;
	hashtable = hashtree + hashtable_pos;

	blksz = 1 << ivfc->levels[level].blksize;
	levelsize = (u32)ivfc->levels[level].size;
	curpos = offset;
	chunksize = blksz;

	curpos >>= ivfc->levels[level].blksize;
	hashpos = curpos * 0x20;
	curpos <<= ivfc->levels[level].blksize;

	curhashpos = hashpos;

	hashdata = (u8*)malloc(blksz);
	if(hashdata == NULL) {
		printf("failed to alloc hashdata buffer with block size %x.\n", blksz);
		return 1;
	}

	while(cursize < size) {
		if(chunksize > levelsize - curpos)chunksize = levelsize - curpos;
		memset(hashdata, 0, blksz);
		memcpy(hashdata, &part[curpos], chunksize);

		sha256(hashdata, blksz, calchash);
		if(memcmp(hashtable + curhashpos, calchash, 0x20)!=0) {
			if(update) {
				updated = 1;
				memcpy(hashtable + curhashpos, calchash, 0x20);
			}
			else {
				printf("hash entry in lvl%u for lvl%u is invalid, curpos %x hashpos %x blksz %x\n", level-1, level, curpos, curhashpos, blksz);
				free(hashdata);
				return 2;
			}
		}

		cursize += blksz;
		curpos += blksz;
		curhashpos += 0x20;
		totalhashsize += 0x20;
	}

	free(hashdata);

	if(level>=2) {
		ret = fs_verifyhashtree(hashtable, hashtree, ivfc, hashtable_pos + hashpos, totalhashsize, level-1, updated);
		if(ret)return ret;
	}

	ret = 0;
	if(level==1)ret = fs_checkheaderhashes(updated);

	return ret;
}

int fs_verifyhashtree_fsdata(u32 offset, u32 size, int filedata, int update)
{
	int ret;
	u8 *hashtree;
	u8 *part;
	partition_table *table;

	if(savectx.datapart_offset==0 || !filedata) {
		part = fs_part(savectx.sav, 1, 0);
		offset += fs_get_offset(part);

		hashtree = fs_part(savectx.sav, 0, 0);
		table = fs_part_get_info(savectx.sav, 0);
	}
	else {
		part = savectx.sav + (savectx.datapart_offset + savectx.datapart_filebase);
		table = fs_part_get_info(savectx.sav, 1);

		hashtree = fs_part(savectx.sav, 0, 1);
	}

	ret = fs_verifyhashtree(part, hashtree, &table->ivfc, offset, size, 3, update);

	return ret;
}

