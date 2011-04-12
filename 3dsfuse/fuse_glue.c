#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include "fuse_glue.h"
#include "fs.h"
#include "types.h"

/** icky globals **/
u8 *sav_buf;
u32 sav_size=0;

static struct fuse_operations sav_operations = {
	.getattr = sav_getattr,
	.readdir = sav_readdir,
	.open    = sav_open,
	.read    = sav_read
};

int fuse_sav_init(u8 *buf, u32 size, int argc, char *argv[]) {
	// lets keep this locally for the FUSE driver, these
	// images arent very huge anyway
	sav_buf = malloc(size);
	memcpy(sav_buf, buf, size);

	return fuse_main(argc, argv, &sav_operations);
}

int sav_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	char name_buf[0x11];
	fst_entry *entries;
	int i;

	memset(name_buf, 0, 0x11);

	// TODO: Learn more about dir entries and handle subdirectory support
	if (strcmp(path, "/") != 0) 
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	// skip over root entry
	entries = (fst_entry*)(sav_buf + fs_get_start(sav_buf) + sizeof(fst_entry));

	for(i = 0; i < fs_num_entries(sav_buf)-1; i++) {
		memcpy(name_buf, entries[i].name, 0x10);
		filler(buf, name_buf, NULL, 0);
	}

	return 0;
}

int sav_getattr(const char *path, struct stat *stbuf) {
	fst_entry *e;

	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0444;
		stbuf->st_nlink = 2; // always 2 since we dont do subdirs yet
	} else {
		e = fs_get_by_name(sav_buf, path + 1);

		if (e == NULL)
			return -ENOENT;

		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = e->size;
	}

	return 0;
}

int sav_open(const char *path, struct fuse_file_info *fi) {
	fst_entry *e;

	e = fs_get_by_name(sav_buf, path + 1);

	if (e == NULL)
		return -ENOENT;

	// this driver is readonly for now
	// we are lacking *lots* of information to do a proper rebuild of the eeprom image
	if((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}


int sav_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {

	fst_entry *e;

	e = fs_get_by_name(sav_buf, path + 1);

	if (e == NULL)
		return -ENOENT;

	if (offset >= e->size)
		return 0;

	if (offset+size > e->size)
		size = e->size - offset;

	memcpy(buf, sav_buf + 0x3000 + fs_get_offset(sav_buf) + (e->block_no * 0x200) + offset, size);

	return size;
}

