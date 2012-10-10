#ifndef __FS_H__
#define __FS_H__

#include "types.h"

typedef struct {
	u32 magic;//"DISA"
	u32 magicnum;//0x40000
	u64 total_partentries;
	u64 secondarytable_offset;
	u64 primarytable_offset;
	u64 part_tablesize;

	u64 save_partentoffset;
	u64 save_partentsize;
	u64 data_partentoffset;
	u64 data_partentsize;

	u64 save_partoffset;
	u64 save_partsize;
	u64 data_partoffset;
	u64 data_partsize;

	u32 activepart_table;
	u8 activepart_tablehash[0x20];
	u8 reserved[0x74];
} disa_header;

typedef struct {
	u32 magic;//"DIFI"
	u32 magicnum;//0x10000

	u64 ivfc_offset;//Relative to this DIFI
	u64 ivfc_size;
	u64 dpfs_offset;
	u64 dpfs_size;
	u64 parthash_offset;
	u64 parthash_size;
	u32 flags;//When the low 8-bits are non-zero, this is a DATA partition
	u64 filebase_offset;//DATA partition only
} difi_header;

typedef struct {
	u64 offset;
	u64 size;
	u32 blksize;
	u32 reserved;
} ivfclevel_header;

typedef struct {
	u32 magic;//"IVFC"
	u32 magicnum;//0x20000
	u64 masterhash_size;//Size of the hash which hashes lvl1

	ivfclevel_header levels[4];
	u64 unknown;
} ivfc_header;

typedef struct {
	u32 magic;//"DPFS"
	u32 magicnum;//0x10000

	u64 table1_offset;
	u64 table1_size;
	u32 table1_blksize;
	u32 table1_reserved;

	u64 table2_offset;
	u64 table2_size;
	u32 table2_blksize;
	u32 table2_reserved;

	u64 ivfcpart_offset;
	u64 ivfcpart_size;
	u32 ivfcpart_blksize;
	u32 ivfcpart_reserved;
} dpfs_header;

typedef struct {
	difi_header difi;
	ivfc_header ivfc;
	dpfs_header dpfs;
	u8 ivfcpart_masterhash[0x20];
} partition_table;

typedef struct {
	u32 parent_dirid;
	u8 name[0x10];
	u32 id;
	u32 unk1;
	u32 block_no;
	u64 size;
	u32 unk4, unk5;
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
