#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include "fuse_glue.h"
#include "fs.h"
#include "wearlevel.h"
#include "types.h"
#include "helper.h"

/** icky globals **/
static u8 *sav_buf;
static u8 *out_buf;
static u8 *xorpad_buf;
static u32 sav_size=0;
static u32 xorpad_size=0;

static struct fuse_operations sav_operations = {
	.getattr = sav_getattr,
	.readdir = sav_readdir,
	.open    = sav_open,
	.read    = sav_read,
	.write   = sav_write
};

u8 *path_to_part(const char *path) {
	if (strncmp(path, "/part_00/", 9) != 0) {
		return NULL; 
	}

	return fs_part(sav_buf, 1, 0, -1);
}

int fuse_sav_init(u8 *buf, u32 size, u8 *xorpad, u32 xorpad_sz, int argc, char *argv[]) {
	// lets keep this locally for the FUSE driver, these
	// images arent very huge anyway
	sav_buf = malloc(size);
	out_buf = malloc(size+0x2000);

	sav_size = size;
	xorpad_size = xorpad_sz;
	memcpy(sav_buf, buf, size);

	xorpad_buf = malloc(xorpad_size);
	memcpy(xorpad_buf, xorpad, xorpad_size);

	memset(out_buf, 0xff, size+0x2000);
	xor(sav_buf, sav_size, out_buf+0x2000, xorpad_buf, xorpad_size);

	if(fs_initsave(sav_buf))
		return 1;

	return fuse_main(argc, argv, &sav_operations);
}

int sav_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	char name_buf[0x11];
	fst_entry *entries;
	int j;
	u8 *part;

	// root?
	if (strcmp(path, "/") == 0) { 	
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		part = fs_part(sav_buf, 1, 0, -1);
		if(part) {
			if (strncmp((char*)part, "SAVE", 4) != 0) {
				printf("SAVE partition is invalid.\n");
				hexdump(part, 0x100);
			}
			else {
				filler(buf, "part_00", NULL, 0);
			}
		}

		filler(buf, "clean.sav", NULL, 0);
		filler(buf, "output.sav", NULL, 0);
		filler(buf, "xorpad.bin", NULL, 0);

		return 0;
	} 

	if (strncmp(path, "/part_00", 8) == 0) {
		part = fs_part(sav_buf, 1, 0, -1);
		if(part == NULL)return -ENOENT;

		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		if (strncmp((char*)part, "SAVE", 4) != 0) {
			printf("SAVE partition is invalid.\n");
			return 0;
		}

		printf("@@ part valid\n");
		
		// skip over root entry
		entries = (fst_entry*)(part + fs_get_start(part) + sizeof(fst_entry));
		printf("FST start: %x\n", (u32)entries - (u32)sav_buf);

		for(j = 0; j < fs_num_entries(part)-1; j++) {
			if(entries->name[0]==0x09) {
				entries++;
				continue;
			}

			printf("@@ name: '%s'\n", entries->name);
			memset(name_buf, 0, 0x11);
			memcpy(name_buf, entries->name, 0x10);
			filler(buf, name_buf, NULL, 0);
			entries++;
		}
	}

	return 0;
}

int sav_getattr(const char *path, struct stat *stbuf) {
	fst_entry *e;
	u8 *part;

	memset(stbuf, 0, sizeof(struct stat));

	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 1;

	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0445;
		stbuf->st_nlink = 2 + 1; // always 2 since we dont do subdirs yet
	} else if (strncmp(path, "/part_", 6) == 0 && strlen(path) == 8) {
		if(fs_part(sav_buf, 1, 0, -1) == NULL)return -ENOENT;
		stbuf->st_mode = S_IFDIR | 0445;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, "/clean.sav") == 0) {
		stbuf->st_size = sav_size;
	} else if (strcmp(path, "/xorpad.bin") == 0) {
		stbuf->st_size = xorpad_size;
	} else if (strcmp(path, "/output.sav") == 0) {
		stbuf->st_size = sav_size+0x2000;
	} else {
		part = fs_part(sav_buf, 1, 0, -1);
		if(part == NULL)return -ENOENT;

		if (strncmp((char*)part, "SAVE", 4) != 0) {
			printf("SAVE partition is invalid.\n");
			return -ENOENT;
		}

		e = fs_get_by_name(path_to_part(path), path + 9);

		if (e == NULL) {
			return -ENOENT;
		}

		stbuf->st_size = (size_t)getle64(e->size);
	}

	printf("@@ stat done\n");

	return 0;
}

int sav_open(const char *path, struct fuse_file_info *fi) {
	fst_entry *e;
	u8 *part = NULL;

	if (strcmp(path, "/clean.sav") == 0) {
		return 0;
	}

	if (strcmp(path, "/xorpad.bin") == 0) {
		return 0;
	}

	if (strcmp(path, "/output.sav") == 0) {
		if((fi->flags & 3) != O_RDONLY)
			return -EACCES;

		return 0;
	}

	part = path_to_part(path);

	if (part == NULL)
		return -ENOENT;

	if (strncmp((char*)part, "SAVE", 4) != 0) {
			printf("SAVE partition is invalid.\n");
			hexdump(part, 0x100);
			return -ENOENT;
	}

	e = fs_get_by_name(part, path + 9);

	if (e == NULL)
		return -ENOENT;

	return 0;
}

int sav_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {

	fst_entry *e;
	u8 *part;
	u16 crc=0;
	u32 saveoff, buf_offs=0, block_offs=0, jour_offs=0, nblocks=0, journalentrycount=0;
	int i, j;
	
	if (strcmp(path, "/clean.sav") == 0) {
		memcpy(buf, sav_buf + offset, size);
		return size;
	}

	if (strcmp(path, "/xorpad.bin") == 0) {
		memcpy(buf, xorpad_buf + offset, size);
		return size;
	}

	if (strcmp(path, "/output.sav") == 0) {
		// build flat blockmap and empty journal
		nblocks = ((sav_size+0x2000) >> 12);

		memset(out_buf, 0x00, 0x08); // clear first 8 bytes, unknown
		for(i=8; i<0x1000; i++)out_buf[i] = 0xff;
		xor(sav_buf, sav_size, out_buf+0x2000, xorpad_buf, xorpad_size);

		for(i=0; i<nblocks-2; i++) {
			buf_offs = 8 + (i*10);
			out_buf[buf_offs+0] = (i+2) | 0x80;
			out_buf[buf_offs+1] = 1;

			for(j=0; j<8; j++) {
				block_offs = (i * 0x1000) + (j * 0x200);
				crc = crc16(out_buf + 0x2000 + block_offs, 0x200, 0xFFFF);
				out_buf[buf_offs+2+j] = (crc >> 8) ^ (crc & 0xff);
			}
		}

		buf_offs = 8 + (nblocks-1)*10;

		crc = crc16(out_buf, buf_offs, 0xFFFF);
		printf("+++ BLOCKMAP CRC %04x\n", crc);
		out_buf[buf_offs+0] = crc & 0xff;
		out_buf[buf_offs+1] = (crc >> 8);

		/*journalentrycount = (0x1000 - ((nblocks-2) * sizeof(blockmap_entry))) / 32;
		i = 0;
		for(jour_offs = nblocks * 10; jour_offs < 0x1000; jour_offs += 0x20, i++, journalentrycount--) {
			journal_entry* journal = (journal_entry*)(out_buf + jour_offs);

			journal->data.virt_no = i;
			journal->data.virt_prev_no = i;
			journal->data.phys_no = i+2;
			journal->data.phys_prev_no = i+2;
			journal->data.phys_realloc_cnt = 1;
			journal->data.virt_realloc_cnt = 1;

			buf_offs = 8 + (i*10);
			for(j=0; j<8; j++)journal->data.checksum[j] = out_buf[buf_offs+2+j];

			memcpy(&journal->dupe_data, &journal->data, sizeof(journal_data));
		}*/

		// mirror blockmap+journal
		memcpy(out_buf+0x1000, out_buf, 0x1000);

		memcpy(buf, out_buf + offset, size);

		return size;
	}

	part = path_to_part(path);
	if (part == NULL)
		return -ENOENT;

	e = fs_get_by_name(part, path + 9);

	if (e == NULL)
		return -ENOENT;

	if (offset >= (size_t)getle64(e->size))
		return 0;

	if (offset+size > (size_t)getle64(e->size))
		size = (size_t)getle64(e->size) - offset;

	saveoff = (getle32(e->block_no) * 0x200) + offset;

	if(fs_verifyupdatehashtree_fsdata(saveoff, size, (u8*)buf, 1, 0))return -EIO;

	return size;
}

int sav_write(const char *path, const char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {

	fst_entry *e;
	u8 *part;
	u32 saveoff;
	
	if (strcmp(path, "/clean.sav") == 0) {
		memcpy(sav_buf + offset, buf, size);
		fs_setupdateflags();
		return size;
	}

	if (strcmp(path, "/xorpad.bin") == 0) {
		memcpy(xorpad_buf + offset, buf, size);
		return size;
	}


	if (strcmp(path, "/output.sav") == 0) {
		return -EINVAL;
	}

	part = path_to_part(path);
	if (part == NULL)
		return -ENOENT;

	e = fs_get_by_name(part, path + 9);

	if (e == NULL)
		return -ENOENT;

	if (offset >= (size_t)getle64(e->size))
		return 0;

	if (offset+size > (size_t)getle64(e->size))
		size = (size_t)getle64(e->size) - offset;

	saveoff = (getle32(e->block_no) * 0x200) + offset;

	if(fs_verifyupdatehashtree_fsdata(saveoff, size, NULL, 1, 0))return -EIO;//the hashtree must be already valid before writing any data.

	if(fs_verifyupdatehashtree_fsdata(saveoff, size, (u8*)buf, 1, 1))return -EIO;

	return size;
}

