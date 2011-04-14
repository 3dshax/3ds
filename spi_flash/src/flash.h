/*
 * flash.h
 *
 *  Created on: Apr 7, 2011
 *      Author: erant
 */

#ifndef FLASH_H_
#define FLASH_H_

#include <stdint.h>

void flash_init(void);

#ifdef __XC__
void flash_read(uint8_t buf[], uint32_t addr, size_t sz);
void flash_write(uint8_t buf[], uint32_t addr, size_t sz);
void flash_erase_chip();
size_t flash_get_size();
#else
void flash_read(uint8_t* buf, uint32_t addr, size_t size);
void flash_write(uint8_t* buf, uint32_t addr, size_t size);
void flash_erase_chip();
#endif

#endif /* FLASH_H_ */
