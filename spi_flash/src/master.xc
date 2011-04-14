///////////////////////////////////////////////////////////////////////////////
//
// SPI Master (mode 3)
// Version 1.0
// 25 Nov 2009
//
// master.xc
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

#include <xs1.h>
#include <xclib.h>
#include <stdio.h>
#include "master.h"
#include <platform.h>

//out buffered port:8 spi_mosi = SPI_MOSI;
//out buffered port:8 spi_sclk = SPI_CLK;
//in buffered port:8 spi_miso = SPI_MISO;

in buffered port:8 spi_miso = PORT_TGT_MISO;
out buffered port:8 spi_sclk = PORT_TGT_CLK;
out buffered port:1 spi_ss = PORT_TGT_SS;
out buffered port:8 spi_mosi = PORT_TGT_MOSI;

//#define spi_miso	PORT_TGT_MISO
//#define spi_mosi	PORT_TGT_MOSI
//#define spi_sclk	PORT_TGT_CLK
//#define spi_ss 		PORT_TGT_CS

// need two clock blocks:
// (1) continuous clock
// (2) software initiated clock ticks aligned to (1)
clock blk1 = XS1_CLKBLK_5;
clock blk2 = XS1_CLKBLK_4;

void spi_select()
{
  spi_ss <: 0;
}

void spi_deselect()
{
  spi_ss <: 1;
}


void spi_init()
{
	// configure ports and clock blocks
	set_port_use_on(spi_sclk);
    set_port_use_on(spi_mosi);
	set_port_use_on(spi_miso);
	set_port_use_on(spi_ss);

	spi_deselect();

	configure_clock_rate(blk1, 100, 8);
	configure_out_port(spi_sclk, blk1, 0);
	configure_clock_src(blk2, spi_sclk);
	configure_out_port(spi_mosi, blk2, 0);
	configure_in_port(spi_miso, blk2);
	clearbuf(spi_mosi);
	clearbuf(spi_sclk);
	start_clock(blk1);
	start_clock(blk2);
	spi_sclk <: 0xFF;
}

void spi_shutdown()
{
	// need clock ticks in order to stop clock blocks
	spi_sclk <: 0xAA;
	spi_sclk <: 0xAA;
	stop_clock(blk2);
	stop_clock(blk1);
}

unsigned char spi_in_byte()
{
	// MSb-first bit order - SPI standard
	unsigned x;
	clearbuf(spi_miso);
	spi_sclk <: 0xAA;
	spi_sclk <: 0xAA;
	sync(spi_sclk);
	spi_miso :> x;\
	return bitrev(x) >> 24;
}

unsigned short spi_in_short()
{
	// big endian byte order
	unsigned short data = 0;
	data |= (spi_in_byte() << 8);
	data |= spi_in_byte();
	return data;
}

unsigned int spi_in_word()
{
	// big endian byte order
	unsigned int data = 0;
	data |= (spi_in_byte() << 24);
	data |= (spi_in_byte() << 16);
	data |= (spi_in_byte() << 8);
	data |= spi_in_byte();
	return data;
}

void spi_out_byte(unsigned char data)
{
	// MSb-first bit order - SPI standard
	unsigned x = bitrev(data) >> 24;
	spi_mosi <: x;
	spi_sclk <: 0xAA;
	spi_sclk <: 0xAA;
	sync(spi_sclk);
	spi_miso :> void;
}

void spi_out_short(unsigned short data)
{
	// big endian byte order
	spi_out_byte((data >> 8) & 0xFF);
	spi_out_byte(data & 0xFF);
}

void spi_out_word(unsigned int data)
{
	// big endian byte order
	spi_out_byte((data >> 24) & 0xFF);
	spi_out_byte((data >> 16) & 0xFF);
	spi_out_byte((data >> 8) & 0xFF);
	spi_out_byte(data & 0xFF);
}
