#ifndef __FS_H__
#define __FS_H__

#include "types.h"

typedef struct {
	u8 magic[4];//"DISA"
	u8 magicnum[4];//0x40000
	u8 total_partentries[8];
	u8 secondarytable_offset[8];
	u8 primarytable_offset[8];
	u8 part_tablesize[8];

	u8 save_partentoffset[8];
	u8 save_partentsize[8];
	u8 data_partentoffset[8];
	u8 data_partentsize[8];

	u8 save_partoffset[8];
	u8 save_partsize[8];
	u8 data_partoffset[8];
	u8 data_partsize[8];

	u8 activepart_table[4];
	u8 activepart_tablehash[0x20];
	u8 reserved[0x74];
} disa_header;

typedef struct {
	u8 magic[4];//"DIFI"
	u8 magicnum[4];//0x10000

	u8 ivfc_offset[8];//Relative to this DIFI
	u8 ivfc_size[8];
	u8 dpfs_offset[8];
	u8 dpfs_size[8];
	u8 parthash_offset[8];
	u8 parthash_size[8];
	u8 flags[4];//When the low 8-bits are non-zero, this is a DATA partition
	u8 filebase_offset[8];//DATA partition only
} difi_header;

typedef struct {
	u8 offset[8];
	u8 size[8];
	u8 blksize[4];
	u8 reserved[4];
} ivfclevel_header;

typedef struct {
	u8 magic[4];//"IVFC"
	u8 magicnum[4];//0x20000
	u8 masterhash_size[8];//Size of the hash which hashes lvl1

	ivfclevel_header levels[4];
	u8 unknown[8];
} ivfc_header;

typedef struct {
	u8 magic[4];//"DPFS"
	u8 magicnum[4];//0x10000

	u8 table1_offset[8];
	u8 table1_size[8];
	u8 table1_blksize[4];
	u8 table1_reserved[4];

	u8 table2_offset[8];
	u8 table2_size[8];
	u8 table2_blksize[4];
	u8 table2_reserved[4];

	u8 ivfcpart_offset[8];
	u8 ivfcpart_size[8];
	u8 ivfcpart_blksize[4];
	u8 ivfcpart_reserved[4];
} dpfs_header;

typedef struct {
	difi_header difi;
	ivfc_header ivfc;
	dpfs_header dpfs;
	u8 ivfcpart_masterhash[0x20];
} partition_table;

typedef struct {
	u8 parent_dirid[4];
	u8 name[0x10];
	u8 id[4];
	u8 unk1[4];
	u8 block_no[4];
	u8 size[8];
	u8 unk4[4];
	u8 unk5[4];
} fst_entry;

int fs_initsave(u8 *nsav);
void fs_setupdateflags();
u32 fs_get_start(u8 *buf);
u32 fs_get_offset(u8 *buf);

partition_table *fs_part_get_info(u8 *buf, u32 part_no);
u8 *fs_part(u8 *buf, int fs, int datapart);
u8 *fs_getfilebase();
int fs_verifyupdatehashtree_fsdata(u32 offset, u32 size, int filedata, int update);

fst_entry *fs_get_by_name(u8 *part, const char *name);
int fs_num_entries(u8 *buf);
int fs_num_partition(u8 *buf);

#endif
