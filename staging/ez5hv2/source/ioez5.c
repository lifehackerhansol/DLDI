/*
	EZ-Flash V
	Card IO routines

	Copyright (C) 2007 Michael Chisholm (Chishm)
	Copyright (C) 2007 SaTa
	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#include <nds/ndstypes.h>
#include "card.h"

#include "ccitt.h"
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

void ioEZ5_GetCCITTForWriteBuffer(u8 *ccittResults, u8 *buffer)
{
	ALIGN(4) u8 crcBuffer[128];
	u8 w1,w2,w3,w4 ;
	u16 b1,b2,b3,b4 ;

	for(int i=0;i<512;i+=4)
	{
		crcBuffer[i>>2] =   (buffer[i + 0] & 0x80  << 0) 
					| ((buffer[i + 1] & 0x80) >> 2)
					| ((buffer[i + 2] & 0x80) >> 4) 
					| ((buffer[i + 3] & 0x80) >> 6) 
					| ((buffer[i + 0] & 0x8)  << 3) 
					| ((buffer[i + 1] & 0x8)  << 1)
					| ((buffer[i + 2] & 0x8)  >> 1) 
					| ((buffer[i + 3] & 0x8)  >> 3) ;
	}
	b1 = ccitt(crcBuffer,128);
	for(int i=0;i<512;i+=4)
	{
		crcBuffer[i>>2] =  ((buffer[i + 0] & 0x40) << 1) 
					| ((buffer[i + 1] & 0x40) >> 1)
					| ((buffer[i + 2] & 0x40) >> 3) 
					| ((buffer[i + 3] & 0x40) >> 5) 
					| ((buffer[i + 0] & 0x4)  << 4) 
					| ((buffer[i + 1] & 0x4)  << 2)
					| ((buffer[i + 2] & 0x4)  << 0) 
					| ((buffer[i + 3] & 0x4)  >> 2) ;
	}
	b2 = ccitt(crcBuffer,128);
	for(int i=0;i<512;i+=4) {
		crcBuffer[i>>2] =  ((buffer[i + 0] & 0x20) << 2) 
					| ((buffer[i + 1] & 0x20) << 0)
					| ((buffer[i + 2] & 0x20) >> 2) 
					| ((buffer[i + 3] & 0x20) >> 4) 
					| ((buffer[i + 0] & 0x2)  << 5) 
					| ((buffer[i + 1] & 0x2)  << 3)
					| ((buffer[i + 2] & 0x2)  << 1) 
					| ((buffer[i + 3] & 0x2)  >> 1) ;
	}
	b3 = ccitt(crcBuffer,128);
	for(int i=0;i<512;i+=4)
	{
		crcBuffer[i>>2] =  ((buffer[i + 0] & 0x10) << 3) 
					| ((buffer[i + 1] & 0x10) << 1)
					| ((buffer[i + 2] & 0x10) >> 1) 
					| ((buffer[i + 3] & 0x10) >> 3) 
					| ((buffer[i + 0] & 0x1)  << 6) 
					| ((buffer[i + 1] & 0x1)  << 4)
					| ((buffer[i + 2] & 0x1)  << 2) 
					| ((buffer[i + 3] & 0x1)  << 0) ;
	}
	b4 = ccitt(crcBuffer,128);
	w1=b1>>8 ;
	w2=b2>>8 ;
	w3=b3>>8 ;
	w4=b4>>8 ;
	ccittResults[0] =    ((w1 & 0x80))
						|((w2 & 0x80) >> 1)
						|((w3 & 0x80) >> 2)
						|((w4 & 0x80) >> 3)
						|((w1 & 0x40) >> 3)
						|((w2 & 0x40) >> 4)
						|((w3 & 0x40) >> 5)
						|((w4 & 0x40) >> 6) ;
	ccittResults[1] =    ((w1 & 0x20) << 2)
						|((w2 & 0x20) << 1)
						|((w3 & 0x20))
						|((w4 & 0x20) >> 1)
						|((w1 & 0x10) >> 1)
						|((w2 & 0x10) >> 2)
						|((w3 & 0x10) >> 3)
						|((w4 & 0x10) >> 4) ;
	ccittResults[2] =    ((w1 & 0x8)  << 4)
						|((w2 & 0x8)  << 3)
						|((w3 & 0x8)  << 2)
						|((w4 & 0x8)  << 1)
						|((w1 & 0x4)  << 1)
						|((w2 & 0x4))
						|((w3 & 0x4)  >> 1)
						|((w4 & 0x4)  >> 2) ;
 	ccittResults[3] =    ((w1 & 0x2)  << 6)
						|((w2 & 0x2)  << 5)
						|((w3 & 0x2)  << 4)
						|((w4 & 0x2)  << 3)
						|((w1 & 0x1)  << 3)
						|((w2 & 0x1)  << 2)
						|((w3 & 0x1)  << 1)
						|((w4 & 0x1)) ;
	w1=b1 ;
	w2=b2 ;
	w3=b3 ;
	w4=b4 ;	
	ccittResults[4] =    ((w1 & 0x80))
						|((w2 & 0x80) >> 1)
						|((w3 & 0x80) >> 2)
						|((w4 & 0x80) >> 3)
						|((w1 & 0x40) >> 3)
						|((w2 & 0x40) >> 4)
						|((w3 & 0x40) >> 5)
						|((w4 & 0x40) >> 6) ;
	ccittResults[5] =    ((w1 & 0x20) << 2)
						|((w2 & 0x20) << 1)
						|((w3 & 0x20))
						|((w4 & 0x20) >> 1)
						|((w1 & 0x10) >> 1)
						|((w2 & 0x10) >> 2)
						|((w3 & 0x10) >> 3)
						|((w4 & 0x10) >> 4) ;
	ccittResults[6] =    ((w1 & 0x8)  << 4)
						|((w2 & 0x8)  << 3)
						|((w3 & 0x8)  << 2)
						|((w4 & 0x8)  << 1)
						|((w1 & 0x4)  << 1)
						|((w2 & 0x4))
						|((w3 & 0x4)  >> 1)
						|((w4 & 0x4)  >> 2) ;
 	ccittResults[7] =    ((w1 & 0x2)  << 6)
						|((w2 & 0x2)  << 5)
						|((w3 & 0x2)  << 4)
						|((w4 & 0x2)  << 3)
						|((w1 & 0x1)  << 3)
						|((w2 & 0x1)  << 2)
						|((w3 & 0x1)  << 1)
						|((w4 & 0x1)) ;
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

bool ioEZ5_SDWriteSector(u32 sector, const void *buffer)
{
	ALIGN(4) u8 ccittResults[8];

	// calculate CRC for write buffer
	ioEZ5_GetCCITTForWriteBuffer(ccittResults, (u8*)buffer);

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
        card_romSetCmd(IOEZ5_CMD_SDMC_WRITE_DATA(ccittResults + i));
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
