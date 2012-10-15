#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wearlevel.h"
#include "helper.h"


int rearrange(u8* savebuffer, u8* out, int savesize) {
	u16 calcedcrc;
	u16 havecrc;
	u32 blkmapsize;
	u32 offs;
	u16 shortcrc;
	u8 bytecrc;
	u32 blknum;
	int blkmapentrycount;
	int journalentrycount;
	u32 i, j;
	blockmap blkmap;
	
	if (savesize == 0x80000)
		blkmapentrycount = 0x80;
	else
		blkmapentrycount = 0x20;
		
	blkmapsize = blkmapentrycount * sizeof(blockmap_entry) - 2;
	journalentrycount = (0x1000 - blkmapentrycount * sizeof(blockmap_entry)) / 32;
		
	calcedcrc = crc16(savebuffer, blkmapsize, CRC16_DEFAULT_SEED);
	havecrc = getle16(savebuffer + blkmapsize);
#ifdef DEBUG
	printf("Blockmap entrycount: 0x%X\n", blkmapentrycount);
	printf("Calculated CRC: 0x%04X\n", calcedcrc);
	printf("Have CRC:       0x%04X\n", havecrc);
#endif
	if (calcedcrc != havecrc)
		return -1;
		
	memcpy(blkmap.header, savebuffer, blkmapsize);
	
	
	
	for(offs = blkmapentrycount * 10; offs < 0x1000; offs += 0x20, journalentrycount--) {
		journal_entry* journal = (journal_entry*)(savebuffer + offs);
		
		// TODO compare journal with dupe
		
		if (journal->data.phys_no == 0)
			continue;
			
		if (journal->data.virt_no >= (blkmapentrycount-1))
			break;
#ifdef DEBUG			
		printf("Applying journal:\n");
		hexdump(journal, sizeof(journal_entry));
#endif
		
		blkmap.entries[journal->data.virt_no].phys_no = 0x80 | (journal->data.phys_no & 0x7F);
		blkmap.entries[journal->data.virt_no].unk = journal->data.phys_realloc_cnt;
		for(i=0; i<8; i++)
			blkmap.entries[journal->data.virt_no].checksum[i] = journal->data.checksum[i];
			
		if ( (journal->data.phys_no & 0x7F) != (journal->data.phys_prev_no & 0x7F) ) {
#ifdef DEBUG
		printf("Applying prev-journal\n");
#endif		
			blkmap.entries[journal->data.virt_prev_no].phys_no = journal->data.phys_prev_no & 0x7F;
			blkmap.entries[journal->data.virt_prev_no].unk = journal->data.virt_realloc_cnt;
			
			for(i=0; i<8; i++)
				blkmap.entries[journal->data.virt_prev_no].checksum[i] = 0;
		}
	}
	
	for(i=0; i<(blkmapentrycount-2); i++) {
		blknum = blkmap.entries[i].phys_no;
		
		if (0 == (blknum & 0x80))
		{
			memset(out + i*0x1000, 0xFF, 0x1000);
			
			continue;
		}
		
		
		blknum &= 0x7F;
		offs = blknum * 0x1000;
		
		if (blknum > (blkmapentrycount-1))
		{
			printf("Invalid physical block for logical block %d (physical 0x%02x).\n", i, blknum);
			return -1;
		}
		
		for(j=0; j<8; j++) {
			shortcrc = crc16(savebuffer + offs + j*0x200, 0x200, CRC16_DEFAULT_SEED);
			bytecrc = shortcrc ^ (shortcrc>>8);
			
			if (blkmap.entries[i].checksum[j] != bytecrc) {
				printf("Invalid CRC for logical block %d (physical 0x%02x).\n", i, blknum);
				return -2;
			}
		}
		
		memcpy(out + i*0x1000, savebuffer + offs, 0x1000);
	}
	
	return 0;
}
