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
u8 *sav_buf;
u8 *key_buf;
u32 sav_size=0;

static struct fuse_operations sav_operations = {
	.getattr = sav_getattr,
	.readdir = sav_readdir,
	.open    = sav_open,
	.read    = sav_read
};

u32 path_to_idx(const char *path) {
	int i;
	char name_buf[10];

	// determine partition
	for(i = 0; i < fs_num_partition(sav_buf); i++) {
		sprintf(name_buf, "/part_%02x/", i);
		if (strncmp(name_buf, path, 9) != 0)
			return i;
	}

	return 0;
}

u8 *path_to_part(const char *path) {
	int i;
	char name_buf[16];

	// determine partition
	for(i = 0; i < fs_num_partition(sav_buf); i++) {
		sprintf(name_buf, "/part_%02x/", i);
		if (strncmp(name_buf, path, 9) == 0) {
			return fs_part(i, sav_buf); 
		}
	}

	return NULL;
}

int fuse_sav_init(u8 *buf, u32 size, u8 *key, int argc, char *argv[]) {
	// lets keep this locally for the FUSE driver, these
	// images arent very huge anyway
	sav_buf = malloc(size);
	sav_size = size;
	memcpy(sav_buf, buf, size);

	key_buf = malloc(0x200);
	memcpy(key_buf, key, 0x200);

	return fuse_main(argc, argv, &sav_operations);
}

int sav_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	char name_buf[0x11];
	fst_entry *entries;
	int i, j;
	u8 *part;

	// root?
	if (strcmp(path, "/") == 0) { 	
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		for(i = 0; i < fs_num_partition(sav_buf); i++) {
			sprintf(name_buf, "part_%02x", i);
			filler(buf, name_buf, NULL, 0); 
		}

		filler(buf, "clean.sav", NULL, 0);
		filler(buf, "key.bin", NULL, 0);

		return 0;
	} 

	for(i = 0; i < fs_num_partition(sav_buf); i++) {
		sprintf(name_buf, "/part_%02x", i);
		if (strncmp(name_buf, path, 8) != 0)
			continue;

		part = fs_part(i, sav_buf);

		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		if (strncmp((char*)part, "SAVE", 4) != 0) {
			printf("skipping invalid partition %d\n", i);
			hexdump(part, 0x100);
			continue;
		}

		printf("@@ part %d valid\n", i);
		
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

	memset(stbuf, 0, sizeof(struct stat));

	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 1;

	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0444;
		stbuf->st_nlink = 2 + fs_num_partition(sav_buf); // always 2 since we dont do subdirs yet
	} else if (strncmp(path, "/part_", 6) == 0 && strlen(path) == 8) {
		stbuf->st_mode = S_IFDIR | 0444;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, "/clean.sav") == 0) {
		stbuf->st_size = sav_size;
	} else if (strcmp(path, "/key.bin") == 0) {
		stbuf->st_size = 512;
	} else {
		e = fs_get_by_name(path_to_part(path), path + 9);

		if (e == NULL) {
			return -ENOENT;
		}

		stbuf->st_size = e->size;
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
		return 0;
	}

	part = path_to_part(path);

	if (part == NULL)
		return -ENOENT;

	e = fs_get_by_name(part, path + 9);

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
	u8 *part;
	
	if (strcmp(path, "/clean.sav") == 0) {
		memcpy(buf, sav_buf + offset, size);
		return size;
	}

	if (strcmp(path, "/key.bin") == 0) {
		memcpy(buf, key_buf + offset, size);
		return size;
	}

	part = path_to_part(path);
	e = fs_get_by_name(part, path + 9);

	if (e == NULL)
		return -ENOENT;

	if (offset >= e->size)
		return 0;

	if (offset+size > e->size)
		size = e->size - offset;

	memcpy(buf, sav_buf + 0x3000 + fs_get_offset(path_to_part(path)) + (e->block_no * 0x200) + offset, size);

	return size;
}
