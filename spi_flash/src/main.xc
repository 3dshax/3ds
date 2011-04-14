/*
 * main.xc
 *
 *  Created on: Apr 7, 2011
 *      Author: erant
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xclib.h>
#include "flash.h"
#include "io.h"

uint8_t array[4096];

#define ERASE

int main(void){
	uint32_t addr;
	io_init();
	flash_init();

#ifdef ERASE
	flash_erase_chip();
	return 0;
#endif

#ifdef READ
	for(int i = 0; i < flash_get_size() / sizeof(array); i++){
		flash_read(array, i * sizeof(array), sizeof(array));
		io_write(array, sizeof(array));
		if(!((i * sizeof(array)) % 0x4000))
			printf("@%06X\n", i * sizeof(array));
	}
#else

	flash_erase_chip();
	addr = 0;
	while(io_read(array, sizeof(array)) == sizeof(array)){
		flash_write(array, addr, sizeof(array));
		if(!(addr % 0x4000))
			printf("@%06X\n", addr);
		addr += sizeof(array);
	}
#endif

	io_close();
	return 0;
}
