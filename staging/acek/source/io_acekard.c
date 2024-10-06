/*---------------------------------------------------------------------------------


Copyright (C) 2007 Acekard, www.acekard.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.


---------------------------------------------------------------------------------*/









#include <nds.h>
#include <string.h>
#include "io_ak_sd.h"


typedef enum _tagAK_SLOT1_ROM_MODE
{
	SLOT1_ROM_MODE_FLASH = 0,
	SLOT1_ROM_MODE_SD = 1
} AK_SLOT1_ROM_MODE;


static void AKP_set_direct_sd_mode()
{
	u8 cmd[] = { 0, 0, 0, 0, 0, 0, 0, 0xd3 };

	cardWriteCommand( cmd );
	CARD_CR2 = CARD_ACTIVATE | CARD_nRESET | 0x00000000 | 0x00406000 | CARD_27;
	while( CARD_CR2 & CARD_BUSY ) {}

	memset( (void* )cmd, 0xff, 8 );// = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	cardWriteCommand( cmd );
	CARD_CR2 = CARD_ACTIVATE | CARD_nRESET | 0x00000000 | 0x00406000 | CARD_27;
	while( CARD_CR2 & CARD_BUSY ) {}
}

static void AKP_set_normal_sd_mode()
{
	u8 cmd[] = { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F };

	cardWriteCommand( cmd );
	CARD_CR2 = CARD_ACTIVATE | CARD_nRESET | 0x00000000 | 0x00406000 | CARD_27;
	while( CARD_CR2 & CARD_BUSY ) {}
}

static void AKP_set_rom_offset( u32 offset, bool flashMode )
{
	if( flashMode )
		offset -= 7;
	u8 b1 = (offset & 0xFF000000) >> 24;
	u8 b2 = (offset & 0x00FF0000) >> 16;
	u8 b3 = (offset & 0x0000FF00) >> 8;
	u8 b4 = (offset & 0x000000FF);
	u8 cmd[] = { 0, 0, 0, b4, b3, b2, b1, 0xD0 };

	cardWriteCommand( cmd );
	CARD_CR2 = CARD_ACTIVATE | CARD_nRESET | 0x00406000 | 0x00000000 | CARD_27;

	while( CARD_CR2 & CARD_BUSY ) {}
}

static void AKP_set_slot1_rom_mode( AK_SLOT1_ROM_MODE mode )
{
	if( SLOT1_ROM_MODE_SD == mode )
	{
		// switch from Flash mode to SD mode
		u8 cmd[] = { 0, 0, 0, 0, 0, 0, 0x80, 0xD1 };
		cardWriteCommand( cmd );
	}
	else if( SLOT1_ROM_MODE_FLASH == mode )
	{
		// switch from SD mode to Flash mode
		u8 cmd[] = { 0, 0, 0, 0, 0, 0, 0x00, 0xD1 };
		cardWriteCommand( cmd );
	}
	
	CARD_CR2 = CARD_ACTIVATE | CARD_nRESET | 0x00406000 | 0x00000000 | CARD_27;
	while( CARD_CR2 & CARD_BUSY ) {}
}


//---------------------------------------------------------------
// Functions needed for the external interface

bool _AK_startUp (void) {
	AKP_set_slot1_rom_mode( SLOT1_ROM_MODE_SD );
	AKP_set_rom_offset( 0, false );
	return true;
}

bool _AK_isInserted (void) {
	return true;
}

bool _AK_readSectors (u32 sector, u32 numSectors, void* buffer) {
	u32 i = 0;
	for( i = 0; i < numSectors; ++i )
	{
		u32 address = (sector + i) * 512;
		u8 b1 = (address & 0xFF000000) >> 24;
		u8 b2 = (address & 0x00FF0000) >> 16;
		u8 b3 = (address & 0x0000FF00) >> 8;
		u8 b4 = (address & 0x000000FF);

		u8 * pbuffer = ((u8 *)buffer) + i * 512;
		u8 cmd[] = { 0x00, 0x00, 0x00, b4, b3, b2, b1, 0xB7 };
		cardWriteCommand( cmd );

		//////////////////////////////////////////////
		CARD_CR2 = CARD_ACTIVATE | CARD_nRESET | 0x01000000 | NDSHeader.cardControl13;
		u32 p = 0;
		for( p = 0; p < 512; p += 4 )
		{
			while (!(CARD_CR2 & CARD_DATA_READY)) {}
			u32 data = CARD_DATA_RD;
			if( NULL != pbuffer )
				*(u32 *)(&pbuffer[p]) = data;
		}
	}
	return true;
}

bool _AK_writeSectors (u32 sector, u32 numSectors, const void* buffer) {
	u32 i = 0;
	for( i = 0; i < numSectors; ++i )
	{
		bool flagSucc = false;
		const u8 * pbuffer = ((const u8 *)buffer) + (i * 512);
		u16 buff_crc = swiCRC16( 0x0000, (void *)pbuffer, 512 );
		u32 address = (sector + i) * 512;

		u32 retryCount = 0;
		do
		{
			AKP_set_direct_sd_mode();
			dsd_write_sd_sector( address, pbuffer);
			AKP_set_normal_sd_mode();
			// check sd busy
			while( dsd_is_sd_busy() ) {}

			u16 data_crc = 0;
			u8 data[512];
			memset(data, 0, 512);
			_AK_readSectors( sector + i, 1, (void *)data );
			data_crc = swiCRC16( 0x0000, data, 512 );
			if( data_crc == buff_crc )
			{
				flagSucc = true;
				break;
			}
			++retryCount;
		} while( !flagSucc && retryCount < 16 );

		if( !flagSucc )
		{
			return false;
		}
	}
	return true;
}

bool _AK_clearStatus (void) {
	return true;
}

bool _AK_shutdown (void) {
	return true;
}
