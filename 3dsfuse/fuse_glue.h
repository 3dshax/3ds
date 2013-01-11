#ifndef __FUSE_GLUE_H__
#define __FUSE_GLUE_H__

#include "types.h"

int fuse_sav_init(u8 *buf, u32 size, u8 *xorpad, u32 xorpad_sz, int argc, char *argv[]);
int sav_getattr(const char *path, struct stat *stbuf);
int sav_open(const char *path, struct fuse_file_info *fi);
int sav_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int sav_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int sav_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

#endif
