/*
 * flash.c
 *
 *  Created on: Apr 7, 2011
 *      Author: erant
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "master.h"

#define FLASH_READ_ID		0x9F
#define FLASH_ERASE_CHIP	0x60
#define FLASH_ERASE_SECTOR	0x10
#define	FLASH_READ			0x03
#define FLASH_WRITE_ENABLE	0x06
#define FLASH_WRITE_DISABLE	0x04
#define FLASH_READ_STATUS	0x05
#define FLASH_PROG_PAGE		0x02

#define PAGE_SIZE		32
#define	NUMBER_PAGES	4096

size_t flash_size = 0;
uint32_t flash_id = 0;

void flash_init(){
	spi_init();
	spi_select();
	spi_out_byte(FLASH_READ_ID);

	flash_id = spi_in_byte() << 16;
	flash_id |= spi_in_byte() << 8;
	flash_id |= spi_in_byte();

	flash_size = 1 << (flash_id & 0xFF);
	printf("JEDEC ID: %06X, size: %u bytes\n", (unsigned int)flash_id, flash_size);
	spi_deselect();
}

void flash_protect(){
	spi_select();
	spi_out_byte(FLASH_WRITE_DISABLE);
	spi_deselect();
}

uint8_t flash_read_status(){
	uint8_t status;
	spi_select();
	spi_out_byte(FLASH_READ_STATUS);
	status = spi_in_byte();
	spi_deselect();
	return status;
}

void flash_unprotect(){
	spi_select();
	spi_out_byte(FLASH_WRITE_ENABLE);
	spi_deselect();
}



void flash_wait_complete(){
	while(flash_read_status() & 0x1);
}

void flash_send_address(uint32_t addr){
	addr = ((flash_size - 1) & addr) | (-1 - (flash_size - 1));
	spi_out_byte((addr >> 16) & 0xFF);
	spi_out_byte((addr >> 8) & 0xFF);
	spi_out_byte((addr >> 0) & 0xFF);
}

void flash_erase_sector(int sector){
	flash_unprotect(sector);

	spi_select();
	spi_out_byte(FLASH_ERASE_SECTOR);
	flash_send_address((sector * PAGE_SIZE) + 1);
	spi_deselect();

	flash_wait_complete();
	flash_protect(sector);
}

void flash_erase_chip(){
	flash_unprotect();

	spi_select();
	spi_out_byte(FLASH_ERASE_CHIP);
	spi_deselect();

	flash_wait_complete();
	flash_protect();
}

void flash_read(uint8_t* buf, uint32_t addr, size_t size){
	spi_select();
	spi_out_byte(FLASH_READ);
	flash_send_address(addr);

	int i;
	for(i = 0; i < size; i++){
		buf[i] = spi_in_byte();
	}
	spi_deselect();
}

size_t flash_get_size(){
	return flash_size;
}

void flash_write_page(uint8_t* buf, int page){
	flash_unprotect();
	spi_select();
	spi_out_byte(FLASH_PROG_PAGE);
	flash_send_address(page * PAGE_SIZE);
	int i;
	for(i = 0; i < PAGE_SIZE; i++){
		spi_out_byte(buf[i]);
	}
	spi_deselect();
	flash_wait_complete();
}

void flash_write(uint8_t* buf, uint32_t addr, size_t size){
	int i = 0;

	for(; i < size / PAGE_SIZE; i++){
		flash_write_page(buf + (i * PAGE_SIZE), i + (addr / PAGE_SIZE));
	}
}
