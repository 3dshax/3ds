/*
 * io.h
 *
 *  Created on: Apr 7, 2011
 *      Author: erant
 */

#ifndef IO_H_
#define IO_H_

#include <stdio.h>
#include <stdint.h>

#define WRITE

void io_init(void);
void io_write(uint8_t data[], size_t sz);
int io_read(uint8_t data[], size_t sz);
void io_close(void);

#endif /* IO_H_ */
