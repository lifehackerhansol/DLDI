/*
	EZ-Flash V
	Card IO routines

	Copyright (C) 2007 Michael Chisholm (Chishm)
	Copyright (C) 2007 SaTa
	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#include <nds/ndstypes.h>
#include "libtwl_card.h"

#include "ioez5.h"

static inline void ioEZ5_ReadCardData(u64 command, u32 flags, void *buffer, u32 length)
{
	card_romSetCmd(command);
	card_romStartXfer(flags, false);
	if ((u32)buffer & 3)
		card_romCpuReadUnaligned((u8 *)buffer, length);
	else
		card_romCpuRead(buffer, length);
}

u32 ioEZ5_SendCommand(const u64 command)
{
	u32 ret;
	ioEZ5_ReadCardData(command, IOEZ5_CTRL_READ_4B, &ret, 1);
	return ret;
}

bool ioEZ5_FlushResponse(void)
{
	u32 status = 0;
	u32 wait = 0x10000;
	do
	{
		status = ioEZ5_SendCommand(IOEZ5_CMD_WAIT_BUSY());
		wait--;
		if (!wait) return false;
	} while(status & 0xFF);
	ioEZ5_SendCommand(IOEZ5_CMD_READ_RESPONSE(2));
	return true;
}

/*
This code is taken from libgba, originally authored by Chishm.

Calculates the CRC16 for a sector of data. Calculates it 
as 4 separate lots, merged into one buffer. This is used
for 4 SD data lines, not for 1 data line alone.
*/
static void ioEZ5_CalculateBufferCRC16 (const u8* buff, int buffLength, u8* crc16buff) {
	u32 a, b, c, d;
	int count;
	u32 bitPattern = 0x80808080;	// r7
	u32 crcConst = 0x1021;	// r8
	u32 dataByte = 0;	// r2

	a = 0;	// r3
	b = 0;	// r4
	c = 0;	// r5
	d = 0;	// r6
	
	buffLength = buffLength * 8;
	
	
	do {
		if (bitPattern & 0x80) dataByte = *buff++;
		
		a = a << 1;
		if ( a & 0x10000) a ^= crcConst;
		if (dataByte & (bitPattern >> 24)) a ^= crcConst;
		
		b = b << 1;
		if (b & 0x10000) b ^= crcConst;
		if (dataByte & (bitPattern >> 25)) b ^= crcConst;
	
		c = c << 1;
		if (c & 0x10000) c ^= crcConst;
		if (dataByte & (bitPattern >> 26)) c ^= crcConst;
		
		d = d << 1;
		if (d & 0x10000) d ^= crcConst;
		if (dataByte & (bitPattern >> 27)) d ^= crcConst;
		
		bitPattern = (bitPattern >> 4) | (bitPattern << 28);
	} while (buffLength-=4);
	
	count = 16;	// r8
	
	do {
		bitPattern = bitPattern << 4;
		if (a & 0x8000) bitPattern |= 8;
		if (b & 0x8000) bitPattern |= 4;
		if (c & 0x8000) bitPattern |= 2;
		if (d & 0x8000) bitPattern |= 1;
	
		a = a << 1;
		b = b << 1;
		c = c << 1;
		d = d << 1;
		
		count--;
		
		if (!(count & 0x01)) {
			*crc16buff++ = (u8)(bitPattern & 0xff);
		}
	} while (count != 0);
	
	return;
}


bool ioEZ5_SDReadSector(u32 sector, void *buffer)
{
	ioEZ5_SendCommand(IOEZ5_CMD_SDIO_READ_SINGLE_BLOCK(sector));
	u32 status = 0;
	u32 wait = 0x10000;
	do
	{
		status = ioEZ5_SendCommand(IOEZ5_CMD_WAIT_BUSY());
		wait--;
		if (!wait) return false;
	} while(status & 0xFF);

	ioEZ5_ReadCardData(IOEZ5_CMD_SD_READ_DATA, IOEZ5_CTRL_READ_512B, buffer, 128);
	return true;
}

bool ioEZ5_SDWriteSector(u32 sector, const u8 *buffer)
{
	ALIGN(4) u8 buffer_crc[8];

	// calculate CRC for write buffer
	ioEZ5_CalculateBufferCRC16(buffer, 512, buffer_crc);

	// CMD24
	ioEZ5_SendCommand(IOEZ5_CMD_SDIO_WRITE_SINGLE_BLOCK(sector));
	if (!ioEZ5_FlushResponse()) return false;

	// add another wait just in case
	ioEZ5_SendCommand(IOEZ5_CMD_WAIT_BUSY());

	// actually start write
	ioEZ5_SendCommand(IOEZ5_CMD_SDMC_WRITE_DATA_START);

	// send write data
    for (u32 i=0; i < 512; i += 2) {
        card_romSetCmd(IOEZ5_CMD_SDMC_WRITE_DATA(buffer + i));
        card_romStartXfer(IOEZ5_CTRL_READ_0, false);
        while(card_romIsBusy());
    }
	// send CRC data
    for (u32 i=0; i < 8; i += 2) {
        card_romSetCmd(IOEZ5_CMD_SDMC_WRITE_DATA(buffer_crc + i));
        card_romStartXfer(IOEZ5_CTRL_READ_0, false);
        while(card_romIsBusy());
    }

	// Read write status
	while(ioEZ5_SendCommand(IOEZ5_CMD_WAIT_WRITE_BUSY()) & 0x1);

	// Read CRC status
	ioEZ5_SendCommand(IOEZ5_CMD_WAIT_WRITE_BUSY());
	while((ioEZ5_SendCommand(IOEZ5_CMD_WAIT_WRITE_BUSY()) & 0x1) != 0x1);

	// Wait until it's all nice and done
	while(ioEZ5_SendCommand(IOEZ5_CMD_WAIT_BUSY()));
	return true;
}
