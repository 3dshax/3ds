///////////////////////////////////////////////////////////////////////////////
//
// SPI Master (mode 3)
// Version 1.0
// 25 Nov 2009
//
// master.h
//
// Select lines are intentionally not part of API
// They are simple port outputs
// They depend on how many slaves there are and how they're connected
//
// Copyright (C) 2009, XMOS Ltd
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// * Neither the name of the XMOS Ltd nor the names of its contributors may
//   be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY XMOS LTD ''AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#ifndef _master_h_
#define _master_h_

void spi_init();
void spi_shutdown();

// SPI master output
// big endian byte order
void spi_select();
void spi_deselect();
void spi_out_word(unsigned int data);
void spi_out_short(unsigned short data);
void spi_out_byte(unsigned char data);

// SPI master input
// big endian byte order
unsigned int spi_in_word();
unsigned short spi_in_short();
unsigned char spi_in_byte();

// SPI ports
#ifndef SPI_MISO
#define SPI_MISO XS1_PORT_1A
#endif
#ifndef SPI_CLK
#define SPI_CLK  XS1_PORT_1C
#endif
#ifndef SPI_MOSI
#define SPI_MOSI XS1_PORT_1D
#endif

// SPI clock frequency is fref/(2*SPI_CLOCK_DIV)
// where fref defaults to 100MHz
//#ifndef SPI_CLOCK_DIV
#define SPI_CLOCK_DIV 4
//#endif

#endif
