#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "save.h"
#include <sys/stat.h>

#define XORPAD_SIZE	512
#define SECTOR_SIZE	0x1000
#define FS_OFFSET	0x3000

#define DEBUG

int do_wearlevel = 0;
int do_decrypt = 0;
int do_analysis = 0;
int do_filesys = 0;

size_t file_size(FILE* fp){
	long tmp = ftell(fp);
	fseek(fp, 0, SEEK_END);
	long ret = ftell(fp);
	fseek(fp, tmp, SEEK_SET);
	return ret;
}

void decrypt(void* buf, size_t size, void* xorpad){
	for(int i = SECTOR_SIZE; i < size; i++)
		((uint8_t*)buf)[i] ^= ((uint8_t*)xorpad)[i % XORPAD_SIZE];		
}

int wearlevel(void* buffer, size_t size, void* dest){
	uint8_t* buf = (uint8_t*)buffer;
	
	struct sector_entry* entries;
	struct long_sector_entry *sectors = (struct long_sector_entry*)buffer;

	// Find the start
	while(sectors->magic != SECTOR_MAGIC)
		sectors++;
	int num_entries = ((((void*)sectors) - buffer) / sizeof(struct header_entry));
	entries = calloc(num_entries, sizeof(*entries));
	num_entries--;

	// Sanity check the entries with the size
	if(((num_entries + 1) * SECTOR_SIZE) != size){
		fprintf(stderr, "Filesize does not match flash size.\n");
		return -1;
	}

	// Populate the base state
	struct header_entry *header = (struct header_entry*)buf;
	for(int i = 0; i < num_entries; i++){
		entries[i].phys_sec = header[i].phys_sec & 0x7F;
		entries[i].virt_sec = i;
		memcpy(entries[i].chksums, header[i].chksums, sizeof(entries[0].chksums));
	}

	// Apply the journal
	while(sectors->magic == SECTOR_MAGIC){
		int sec = sectors->sector.virt_sec;
		entries[sec] = sectors->sector;
		sectors++;
	}

#ifdef DEBUG
	for(int i = 0; i < num_entries; i++){
		printf("virt: %d phys: %d, cnt: %d\n", 
				entries[i].virt_sec,
				entries[i].phys_sec,
				entries[i].virt_realloc_cnt);
	}
#endif /* DEBUG */

	for(int i = 0; i < num_entries; i++)
		memcpy(&((uint8_t*)dest)[SECTOR_SIZE * i], &buf[SECTOR_SIZE * entries[i].phys_sec], SECTOR_SIZE);

	return 0;
}

void* find_next_fs(void* buf, size_t sz){
	char* magic = (char*) buf;
	sz = sz / SECTOR_SIZE;
	for(int i = 0; i < sz; i++){
		if(!strncmp(&magic[i * SECTOR_SIZE], "SAVE", 4))
			break;
	}

	if(i >= sz)
		return NULL;
	return &magic[i * SECTOR_SIZE];
}

#define	FST_OFF_OFFSET	(0x6C)
#define FST_BASE_OFFSET (0x58)

#define FS_BLOCK_SIZE	0x200

void parse_fs(void* buf, char* target_dir){
	uint8_t* buffer = (uint8_t*)buf;
	struct fs_entry* entries;
	int nr_entries = 0;

	if(strncmp((char*)buffer, "SAVE", 4)){
		fprintf(stderr, "Failed to find filesystem.\n");
		return;
	}
	
	uint16_t fst_base = *((uint16_t*)&buffer[FST_BASE_OFFSET]);
	uint32_t fst_offset = (*((uint32_t*)&buffer[FST_OFF_OFFSET])) * FS_BLOCK_SIZE;
	entries = (struct fs_entry*)(&buffer[fst_offset + fst_base]);
	nr_entries = entries->node_cnt - 1;
	
	// Skip the root node
	entries++;

#ifdef DEBUG
	for(int i = 0; i < nr_entries; i++){
		fprintf(stderr, "File: %s, size: %u bytes, block_nr: %u\n",
			entries[i].filename,
			(unsigned int)entries[i].size,
			entries[i].block_nr);
	}
#endif /* DEBUG */
	if(mkdir(target_dir, 0766) < 0){
		fprintf(stderr, "Couldn't create dir %s\n", target_dir);
		return;
	}

	char filename[256];
	for(int i = 0; i < nr_entries; i++){
		snprintf(filename, sizeof(filename), "%s/%s", target_dir, entries[i].filename);
		FILE* fp = fopen(filename, "wb");
		if(!fp){
			fprintf(stderr, "Failed to open %s\n", filename);
			continue;
		}
		fwrite(&buffer[fst_base + (entries[i].block_nr * FS_BLOCK_SIZE)],
			1,
			entries[i].size,
			fp);
		fclose(fp);
	}	
}

void analyze(void* buf, size_t size){

}

int main(int argc, char** argv){
	FILE* fp;
	uint8_t* buf = NULL;
	size_t size;
	uint8_t xorpad[XORPAD_SIZE];
	uint8_t* wearlevel_buf = NULL;

	if(argc < 5){
		fprintf(stderr, "Syntax: %s <operation> <savegame> <xorpad> <outfile>\n", argv[0]);
		goto exit;
	}

	if(!strcmp("decrypt", argv[1])){
		do_decrypt = 1;
	} else if(!strcmp("wearlevel", argv[1])){
		do_decrypt = 1;
		do_wearlevel = 1;
	} else if(!strcmp("analyze", argv[1])){
		do_analysis = 1;
	} else if(!strcmp("filesystem", argv[1])){
		do_decrypt = 1;
		do_wearlevel = 1;
		do_filesys = 1;
	}
	
	// Read in the savefile
	fp = fopen(argv[2], "rb");
	if(fp == NULL){
		fprintf(stderr, "Failed to open %s\n", argv[2]);
		goto exit;
	}

	size = file_size(fp);
	buf = malloc(size);
	fread(buf, 1, size, fp);
	fclose(fp);

	fp = fopen(argv[3], "rb");
	if(fp == NULL){
		fprintf(stderr, "Failed to open %s\n", argv[3]);
		goto exit;
	}

	if(fread(xorpad, 1, XORPAD_SIZE, fp) != XORPAD_SIZE){
		fprintf(stderr, "Failed to read in the xor pad\n");
		goto exit;
	}	
	fclose(fp);

	if(do_analysis){
		analyze(buf, size);
	}

	if(do_decrypt){
		void* write_pointer = buf;
		decrypt(buf, size, xorpad);
		if(do_wearlevel){
			wearlevel_buf = calloc(1, size - XORPAD_SIZE);
			if(wearlevel(buf, size, wearlevel_buf) < 0)
				goto exit;
			write_pointer = wearlevel_buf;
			size -= SECTOR_SIZE;
		}
		
		if(do_wearlevel && do_filesys){
			parse_fs(&wearlevel_buf[FS_OFFSET], argv[4]);
		} else {
			fp = fopen(argv[4], "wb");
			if(fp == NULL){
				fprintf(stderr, "Couldn't open %s for writing.\n", argv[4]);
				goto exit;
			}

			fwrite(write_pointer, 1, size, fp);
			fclose(fp);
		}
	}

exit:
	free(buf);
	free(wearlevel_buf);
	
	return 0;
}
