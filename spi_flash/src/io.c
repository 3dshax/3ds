/*
 * io.c
 *
 *  Created on: Apr 7, 2011
 *      Author: erant
 */
#include <stdio.h>
#include <stdint.h>
#include <xccompat.h>
#include "io.h"

FILE* fp = NULL;

#define FILE_BUF_SIZE	8192
char vbuf[FILE_BUF_SIZE];

void io_init(void){
#ifdef READ
	fp = fopen("output", "wb");
#else
	fp = fopen("input", "rb");
#endif
	setvbuf(fp, vbuf, _IOFBF, FILE_BUF_SIZE);
}

void io_write(uint8_t* data, size_t sz){
	fwrite(data, 1, sz, fp);
}

int io_read(uint8_t* data, size_t sz){
	return fread(data, 1, sz, fp);
}

void io_close(void){
	fclose(fp);
}
