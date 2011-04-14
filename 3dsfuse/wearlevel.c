#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wearlevel.h"

int blockmap_get_size(u8 *buf) {
	int i, found = 0;
	u8 *p = buf;

	// find the first journal entry, in order to determine
	// the number of blockmap entries.
	for(i = 0; i < (0x1000 / 32); i++) {
		if(memcmp(p + 28, "\xe0\x6c\x0d\x08", 4) == 0) {
			found = 1;
			break;
		}

		p += 32;
	}

	return (found == 0) ? -1 : ((p - buf) / 10) - 1;
}

u8 *journal_get_start(u8 *buf) {
	int i, found =0;
	u8 *p = buf;

	// find the first journal entry
	for(i = 0; i < (0x1000 / 32); i++) {
		if(memcmp(p + 28, "\xe0\x6c\x0d\x08", 4) == 0) {
			found = 1;
			break;
		}

		p += 32;
	}

	return (found == 0) ? 0 : p;
}

int journal_get_size(u8 *buf) {
	int i, found = 0, journal_count = 0;
	u8 *p = buf;
	u8 *p2;

	// find the first journal entry
	for(i = 0; i < (0x1000 / 32); i++) {
		if(memcmp(p + 28, "\xe0\x6c\x0d\x08", 4) == 0) {
			found = 1;
			break;
		}

		p += 32;
	}

	if (found == 0)
		return -1;

	// now find the last
	p2 = p;

	while(memcmp(p2 + 28, "\xe0\x6c\x0d\x08", 4) == 0) {
		journal_count++;
		p2 += 32;
	}

	return journal_count;
}

int rearrange(u8 *buf, u8 *out, int size) {
	int i, blockmap_size, journal_size;

	blockmap_entry *blockmap;
	journal_entry *journal;
	journal_data *ent;
	mapping_entry *mapping;

	blockmap_size = blockmap_get_size(buf);

	if ((blockmap_size+1) * 0x1000 != size) {
		printf("ERROR: blockmap size mismatch (%08x vs. %08x)\n", size, (blockmap_size+1)*0x1000);
		exit(-1);
	}

	mapping = malloc((blockmap_size+1) * 2);
	blockmap = malloc(sizeof(blockmap_entry) * blockmap_size);
	memcpy(blockmap, buf, sizeof(blockmap_entry) * blockmap_size);

	for(i = 0; i < blockmap_size; i++) {
		mapping[i].phys_no = blockmap[i].phys_no & 0x7f;
		mapping[i].virt_no = i;

#ifdef DEBUG
		printf("%02x ", blockmap[i].phys_no);
		if (((i+1) % 8) == 0)
			printf("\n");
#endif
	}
	printf("\n");


	journal_size = journal_get_size(buf);

	// every journal entry contains a dupe of itself, and the 4 magic journal bytes
	journal = malloc(sizeof(journal_entry) * journal_size);
	memcpy(journal, journal_get_start(buf), journal_size * sizeof(journal_entry)); 

	for(i = 0; i < journal_size; i++) {
		ent = &journal[i].data;

#ifdef DEBUG
		printf(
			"journal entry #%04x -- virt:%02x virt_prev:%02x phys:%02x phys_prev:%02x phys_cnt:%02x virt_cnt:%02x\n", 
			i, ent->virt_no, ent->virt_prev_no, ent->phys_no, ent->phys_prev_no, ent->phys_realloc_cnt, ent->virt_realloc_cnt
		);
#endif

		mapping[ent->virt_no].phys_no = ent->phys_no & 0x7f;
		mapping[ent->virt_no].virt_no = ent->virt_no & 0x7f;
	}

	for(i = 0; i < blockmap_size; i++) {
#ifdef DEBUG
		printf("virt: %d phys: %d\n", i, mapping[i].phys_no);
#endif

		memcpy(out + (i * 0x1000), buf + (mapping[i].phys_no * 0x1000), 0x1000);
	}
	printf("\n");

	return 0;	
}
