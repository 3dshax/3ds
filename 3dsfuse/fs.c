#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "utils.h"
#include "helper.h"
#include "fs.h"

#define FST_BLOCK_OFFSET	0x6c
#define FST_OFFSET		0x58

typedef struct {
	u8 *sav;
	int sav_updated;
	int savetype;

	u32 total_partentries;
	u32 savepart_offset;
	u32 datapart_offset;

	u32 activepart_table;
	u32 save_activepart;
	u32 data_activepart;

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
	savectx.savetype = 0;

	magic = getle32(&nsav[0x100]);

	if(magic == 0x41534944) {
		savectx.savetype = 0;
		if(fs_initsave_disa())return 1;
	}
	else if(magic == 0x46464944) {
		savectx.savetype = 1;
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

	savectx.total_partentries = (u32)getle64(disa->total_partentries);
	savectx.savepart_offset = (u32)getle64(disa->save_partoffset);
	savectx.datapart_offset = (u32)getle64(disa->data_partoffset);

	savectx.activepart_table = 0;
	if(getle32(disa->activepart_table) & 0xff)savectx.activepart_table = 1;
	savectx.save_activepart = savectx.activepart_table;
	savectx.data_activepart = savectx.activepart_table;

	savectx.primary_tableoffset = (u32)getle64(disa->primarytable_offset);
	savectx.secondary_tableoffset = (u32)getle64(disa->secondarytable_offset);

	if(savectx.activepart_table)savectx.activepart_tableoffset = savectx.secondary_tableoffset;
	if(!savectx.activepart_table)savectx.activepart_tableoffset = savectx.primary_tableoffset;

	savectx.datapart_filebase = 0;
	if(savectx.datapart_offset)savectx.datapart_filebase = getle32(disa->activepart_table) & ~0xFF;

	savectx.part_tablesize = (u32)getle64(disa->part_tablesize);
	memcpy(savectx.activepart_tablehash, disa->activepart_tablehash, 0x20);

	return 0;
}

void fs_setupdateflags()
{
	savectx.sav_updated = 1;
}

void fs_updateheaderhash()
{
	disa_header *disa = (disa_header*)&savectx.sav[0x100];

	if(savectx.savetype == 0) {
		memcpy(disa->activepart_tablehash, savectx.activepart_tablehash, 0x20);
	}
}

int fs_calcivfchash(partition_table *table, int datapart, int *update)
{
	int partno = 1;
	u8 *part, *part2;
	u8 *lvl1_buf;
	u32 blksz;
	u8 calchash[0x20];
	u8 calchash2[0x20];

	if(savectx.activepart_table)partno = 0;

	memset(calchash, 0, 0x20);
	memset(calchash2, 0, 0x20);

	part = fs_part(savectx.sav, 0, datapart, -1);
	part2 = fs_part(savectx.sav, 0, datapart, partno);
	if(part == NULL || part2 == NULL) {
		printf("invalid partition.\n");
		return 1;
	}

	blksz = 1 << getle32(table->ivfc.levels[0].blksize);
	lvl1_buf = (u8*)malloc(blksz);
	if(lvl1_buf==NULL) {
		printf("failed to allocate level1 buffer.\n");
		return 3;
	}

	memset(lvl1_buf, 0, blksz);
	memcpy(lvl1_buf, part, (u32)getle64(table->ivfc.levels[0].size));
	sha256(lvl1_buf, blksz, calchash);

	memset(lvl1_buf, 0, blksz);
	memcpy(lvl1_buf, part2, (u32)getle64(table->ivfc.levels[0].size));
	sha256(lvl1_buf, blksz, calchash2);
	free(lvl1_buf);

	if(*update == 0) {
		if(memcmp(table->ivfcpart_masterhash, calchash, 0x20)!=0) {
			if(memcmp(table->ivfcpart_masterhash, calchash2, 0x20)==0) {
				if(datapart==0)savectx.save_activepart = (u32)partno;
				if(datapart)savectx.data_activepart = (u32)partno;

				printf("using part %d instead, for datapart %d.\n", partno, datapart);
				return 0;
			}

			printf("master hash over the IVFC partition is invalid, datapart %d.\n", datapart);
			return 2;
		}
	}
	else {
		if(memcmp(table->ivfcpart_masterhash, calchash, 0x20)!=0) {
			memcpy(table->ivfcpart_masterhash, calchash, 0x20);
		}
		else {
			*update = 0;
		}
	}

	return 0;
}

int fs_checkheaderhashes(int update)
{
	partition_table *savetable = NULL, *datatable = NULL;
	int saveupdated = 0, dataupdated = 0;
	int updated;
	int ret;
	u8 calchash[0x20];

	savetable = (partition_table*)&savectx.sav[savectx.activepart_tableoffset];
	if(savectx.datapart_offset)datatable = (partition_table*)&savectx.sav[savectx.activepart_tableoffset + 0x130];

	saveupdated = update;
	dataupdated = update;	

	ret = fs_calcivfchash(savetable, 0, &saveupdated);
	if(ret)return ret;

	if(datatable) {
		ret = fs_calcivfchash(datatable, 1, &dataupdated);
		if(ret)return ret;
	}

	updated = saveupdated | dataupdated;

	sha256(&savectx.sav[savectx.activepart_tableoffset], savectx.part_tablesize, calchash);
	if(memcmp(savectx.activepart_tablehash, calchash, 0x20)!=0) {
		if(updated) {
			memcpy(savectx.activepart_tablehash, calchash, 0x20);
			fs_updateheaderhash();
		}
		else {
			printf("active table hash from header is invalid.\n");
			return 2;
		}
	}

	if(updated) {
		savectx.sav_updated = 1;
	}

	return 0;
}

partition_table *fs_part_get_info(u8 *buf, u32 part_no) {
	return (partition_table*)(buf + savectx.activepart_tableoffset + (part_no * 0x130));
}

u8 *fs_part(u8 *buf, int fs, int datapart, int part_tableno) {
	u32 pos = 0;
	u32 fs_off = 0, fs_sz = 0;
	u8 *p = buf + savectx.primary_tableoffset;
	partition_table *part = (partition_table*)p;
	int num = 0;

	if(!datapart)pos = savectx.savepart_offset;
	if(datapart)pos = savectx.datapart_offset;
	pos += (u32)getle64(part->dpfs.ivfcpart.offset);

	if(part_tableno==-1)
	{
		if(datapart==0)part_tableno = (int)savectx.save_activepart;
		if(datapart)part_tableno = (int)savectx.data_activepart;
	}

	for(num=0; num<2; num++) {
		if(num==1)p = buf + savectx.secondary_tableoffset;
		if(datapart)p += 0x130;

		part = (partition_table*)p;
		if(getle32(part->difi.magic) != 0x49464944) {
			printf("invalid DIFI magic\n");
			return NULL;
		}

		fs_off = (u32)getle64(part->ivfc.levels[3].offset);
		fs_sz = (u32)getle64(part->ivfc.levels[3].size);

		if(num == part_tableno) {
			//printf("datapart %x pos %x ivfcpartsize %x fs off %x fs sz %x\n", datapart, pos, (u32)getle64(part->dpfs.ivfcpart.size), fs_off, fs_sz);
			if(!fs)return buf + pos;
			return buf + pos + fs_off;
		}

		pos += (u32)getle64(part->dpfs.ivfcpart.size);
	}

	return NULL;
}

int fs_dpfs_getpartno(int datapart, u32 part_offset)
{
	u32 part_base = 0;
	u32 maskstart, blksz, blockpos;
	u32 tableno;
	u32 mask = 0;
	u8 *part;
	partition_table *part_table = (partition_table*)&savectx.sav[savectx.activepart_tableoffset];

	tableno = savectx.activepart_table;
	part_base = savectx.savepart_offset;
	if(datapart)part_base = savectx.datapart_offset;

	maskstart = (u32)getle64(part_table->dpfs.tables[tableno].offset) + (u32)getle64(part_table->dpfs.tables[tableno].size);
	blksz = getle32(part_table->dpfs.ivfcpart.blksize);

	part = savectx.sav + part_base + maskstart;

	blockpos = part_offset >> blksz;
	if(blockpos > 30) {
		printf("fs_dpfs_getpartflag: part_offset>=0x1f000 not supported currently, part_offset %x blockpos %x.\n", part_offset, blockpos);
		return -1;
	}

	if(tableno==0)mask = getle32(part+4);
	if(tableno)mask = getle32(part);
	if(mask & (1 << ( 30 - blockpos)))return 1;
	return 0;
}

u8 *fs_getfilebase()
{
	u8 *part;

	if(savectx.datapart_offset==0) {
		part = fs_part(savectx.sav, 1, 0, -1);
		return part + fs_get_offset(part);
	}

	return fs_part(savectx.sav, 1, 1, -1);
}

u32 fs_get_offset(u8 *buf) {
	return getle32(buf + FST_OFFSET);
}

u32 fs_get_start(u8 *part) {
	u32 block_offset, fst_offset;

	block_offset = getle32(part + 0x78);
	fst_offset   = getle32(part + 0x58);

	if(savectx.datapart_offset==0)return (block_offset * 0x200) + fst_offset;
	return block_offset;
}

int fs_num_entries(u8 *part) {
	return getle32(part + fs_get_start(part));
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

int fs_verifyhashtree(u8 *part, u8 *part2, int datapart, u8 *hashtree, ivfc_header *ivfc, u32 offset, u32 size, u8 *databuf, u32 level, int update)
{
	int i;
	int updated = 0;
	int ret = 0;
	int partno;

	u32 blksz, levelsize, chunksize;
	u32 aligned_imgpos = 0, hashpos = 0, datapos = 0;
	u32 databuf_chunksize = 0;
	u32 curhashpos = 0, totalhashsize = 0x20;
	u32 hashtable_pos;

	u8 *hashtable;
	u8 *hashdata;
	u8 *cur_part;
	u8 calchash[0x20];

	hashtable_pos = (u32)getle64(ivfc->levels[level-1].offset);
	hashtable = hashtree + hashtable_pos;

	blksz = 1 << getle32(ivfc->levels[level].blksize);
	levelsize = (u32)getle64(ivfc->levels[level].size);
	aligned_imgpos = offset;
	chunksize = blksz;

	aligned_imgpos >>= getle32(ivfc->levels[level].blksize);
	hashpos = aligned_imgpos * 0x20;
	aligned_imgpos <<= getle32(ivfc->levels[level].blksize);

	curhashpos = hashpos;

	hashdata = (u8*)malloc(blksz);
	if(hashdata == NULL) {
		printf("failed to alloc hashdata buffer with block size %x.\n", blksz);
		return 1;
	}

	databuf_chunksize = blksz - (offset & (blksz-1));

	while(datapos < size) {
		if(chunksize > levelsize - aligned_imgpos)chunksize = levelsize - aligned_imgpos;
		if(databuf_chunksize > size - datapos)databuf_chunksize = size - datapos;

		partno = 0;
		if(datapart!=-1) {
			partno = fs_dpfs_getpartno(datapart, aligned_imgpos);
		}
		if(partno==-1)return 2;

		memset(hashdata, 0, blksz);

		cur_part = part;
		if(part2 && partno==1) {
			cur_part = part2;
		}

		if(update && databuf)memcpy(&cur_part[offset], &databuf[datapos], databuf_chunksize);

		memcpy(hashdata, &cur_part[aligned_imgpos], chunksize);
		sha256(hashdata, blksz, calchash);

		if(memcmp(hashtable + curhashpos, calchash, 0x20)!=0) {
			if(update) {
				updated = 1;
				memcpy(hashtable + curhashpos, calchash, 0x20);
			}
			else {
				printf("hash entry in lvl%u for lvl%u is invalid for part%d, aligned_imgpos %x hashpos %x blksz %x\n", level, level+1, partno, aligned_imgpos, curhashpos, blksz);
				printf("calc hash: \n");
				for(i=0; i<0x20; i++)printf("%02x", calchash[i]);
				printf("\nhash-table hash: \n");
				for(i=0; i<0x20; i++)printf("%02x", hashtable[curhashpos + i]);
				printf("\n");
				printf("part off %x\n", (u32)part - (u32)savectx.sav);
				if(part2)printf("part2 off %x\n", (u32)part2 - (u32)savectx.sav);
				free(hashdata);
				return 2;
			}
		}
		else {
			//printf("hash entry in lvl%u for lvl%u is valid for part%d, aligned_imgpos %x hashpos %x blksz %x\n", level, level+1, partno, aligned_imgpos, curhashpos, blksz);

			if(!update && databuf)memcpy(&databuf[datapos], &cur_part[offset], databuf_chunksize);
		}

		datapos += databuf_chunksize;
		offset += databuf_chunksize;
		databuf_chunksize = blksz;
		aligned_imgpos += blksz;
		curhashpos += 0x20;
		totalhashsize += 0x20;
	}

	free(hashdata);

	if(level>=2) {
		ret = fs_verifyhashtree(hashtable, NULL, -1, hashtree, ivfc, hashtable_pos + hashpos, totalhashsize, NULL, level-1, updated);
		if(ret)return ret;
	}

	ret = 0;
	if(level==1)ret = fs_checkheaderhashes(updated);

	return ret;
}

int fs_verifyupdatehashtree_fsdata(u32 offset, u32 size, u8 *databuf, int filedata, int update)
{
	int ret, datapart;
	u8 *hashtree;
	u8 *part, *part2;
	partition_table *table;

	if(savectx.datapart_offset==0 || !filedata) {
		datapart = 0;
		part = fs_part(savectx.sav, 1, 0, 0);
		part2 = fs_part(savectx.sav, 1, 0, 1);
		offset += fs_get_offset(part);

		hashtree = fs_part(savectx.sav, 0, 0, -1);
		table = fs_part_get_info(savectx.sav, 0);
	}
	else {
		datapart = 1;
		part = fs_part(savectx.sav, 1, 1, 0);
		part2 = fs_part(savectx.sav, 1, 1, 1);
		table = fs_part_get_info(savectx.sav, 1);

		hashtree = fs_part(savectx.sav, 0, 1, -1);
	}

	ret = fs_verifyhashtree(part, part2, datapart, hashtree, &table->ivfc, offset, size, databuf, 3, update);

	return ret;
}

