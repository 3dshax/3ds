#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include "fuse_glue.h"
#include "fs.h"
#include "types.h"
#include "helper.h"

/** icky globals **/
static u8 *sav_buf;
static u8 *xorpad_buf;
static u32 sav_size=0;

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

int fuse_sav_init(u8 *buf, u32 size, u8 *xorpad, int argc, char *argv[]) {
	// lets keep this locally for the FUSE driver, these
	// images arent very huge anyway
	sav_buf = malloc(size);
	sav_size = size;
	memcpy(sav_buf, buf, size);

	xorpad_buf = malloc(0x200);
	memcpy(xorpad_buf, xorpad, 0x200);

	if(fs_initsave(sav_buf))return 1;

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
		filler(buf, "key.bin", NULL, 0);

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

		for(j = 0; j < fs_num_entries(part)-1; j++) {
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
	} else if (strcmp(path, "/key.bin") == 0) {
		stbuf->st_size = 512;
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

	if (strcmp(path, "/key.bin") == 0) {
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
	u32 saveoff;
	
	if (strcmp(path, "/clean.sav") == 0) {
		memcpy(buf, sav_buf + offset, size);
		return size;
	}

	if (strcmp(path, "/key.bin") == 0) {
		memcpy(buf, xorpad_buf + offset, size);
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

	if(fs_verifyupdatehashtree_fsdata(saveoff, size, 1, 0))return -EIO;

	memcpy(buf, fs_getfilebase() + saveoff, size);

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

	if (strcmp(path, "/key.bin") == 0)
		return -EINVAL;

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

	if(fs_verifyupdatehashtree_fsdata(saveoff, size, 1, 0))return -EIO;//the hashtree must be already valid before writing any data.

	memcpy(fs_getfilebase() + saveoff, buf, size);

	if(fs_verifyupdatehashtree_fsdata(saveoff, size, 1, 1))return -EIO;

	return size;
}

