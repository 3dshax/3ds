#ifndef _SAVE_H
#define _SAVE_H

#define SECTOR_MAGIC    0x080d6ce0

struct fs_entry {
	uint32_t node_cnt;
	char  filename[0x10];
	uint32_t node_id;
	uint32_t unk1; // magic?
	uint32_t block_nr;
	uint32_t size;
	uint32_t unk2;
	uint32_t unk3; // flags and/or date?
	uint32_t unk4;
} __attribute__((__packed__));

struct header_entry {
	uint8_t chksums[8];
	uint8_t phys_sec;
	uint8_t unk;
} __attribute__((__packed__));

struct sector_entry {
	uint8_t virt_sec;       // Mapped to sector
	uint8_t prev_virt_sec;  // Physical sector previously mapped to
	uint8_t phys_sec;       // Mapped from sector
	uint8_t prev_phys_sec;  // Virtual sector previously mapped to
 	uint8_t phys_realloc_cnt;       // Amount of times physical sector has been remapped
	uint8_t virt_realloc_cnt;       // Amount of times virtual sector has been remapped
	uint8_t chksums[8];
} __attribute__((__packed__));

struct long_sector_entry{
	struct sector_entry sector;
	struct sector_entry dupe;
	uint32_t magic;
}__attribute__((__packed__));

#endif /* _SAVE_H */
