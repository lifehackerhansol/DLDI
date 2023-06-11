/*
	SuperCard DSTWO
	Card IO routines

	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#include <nds/ndstypes.h>
#include "card.h"
#include "iods2.h"

static inline void ioDS2_ReadCardData(u64 command, u32 flags, void *buffer, u32 length)
{
	card_romSetCmd(command);
	card_romStartXfer(flags, false);
	if ((u32)buffer & 3)
		card_romCpuReadUnaligned((u8 *)buffer, length);
	else
		card_romCpuRead(buffer, length);
}

static inline void ioDS2_WriteCardData(u64 command, u32 flags, const void *buffer, u32 length)
{
	card_romSetCmd(command);
	card_romStartXfer(flags, false);
	if ((u32)buffer & 3)
		card_romCpuWriteUnaligned((u8 *)buffer, length);
	else
		card_romCpuWrite(buffer, length);
}

// useful tool for simple commands
// 4 byte requests only
static inline u32 ioDS2_SendCommand(u64 command, u32 flags)
{
	u32 ret;
	ioDS2_ReadCardData(command, flags, &ret, 1);
	return ret;
}

static inline void ioDS2_WaitBusy(void)
{
	while ((ioDS2_SendCommand(IODS2_CMD_CARD_BUSY, (IODS2_CTRL_CARD_PARAM | MCCNT1_LEN_4)) & 0x100) != 0);
}

void ioDS2_Initialize(void)
{
	u32 ret;
	do
	{
		ioDS2_SendCommand(IODS2_CMD_CARD_INIT, IODS2_CTRL_CARD_PARAM);
		ret = ioDS2_SendCommand(IODS2_CMD_SD_INIT, IODS2_CTRL_READ_4B);
	} while (ret != 0xA5A55A5A);
}

void ioDS2_SDReadSingleSector(u32 sector, void *buffer)
{
	ioDS2_SendCommand(IODS2_CMD_SD_READ_SINGLE_BLOCK(sector), IODS2_CTRL_READ_4B);
	ioDS2_WaitBusy();

	// send the same command, but this time do 512B polling
	ioDS2_ReadCardData(IODS2_CMD_SD_READ_SINGLE_BLOCK(sector), IODS2_CTRL_READ_512B, buffer, 128);
}

void ioDS2_SDReadMultiSector(u32 sector, void *buffer, u32 num_sectors)
{
	ioDS2_SendCommand(IODS2_CMD_SD_READ_MULTI_BLOCK(sector, num_sectors), IODS2_CTRL_READ_4B);
	ioDS2_WaitBusy();

	for (int i = 0; i < num_sectors; i++)
	{
		// send the same command, but this time do 512B polling
		ioDS2_ReadCardData(IODS2_CMD_SD_READ_MULTI_BLOCK(sector, num_sectors), IODS2_CTRL_READ_512B, buffer, 128);
		buffer = (u8 *)buffer + 0x200;

		// wait
		ioDS2_WaitBusy();
	}
}

void ioDS2_SDWriteSingleSector(u32 sector, const void *buffer)
{
	ioDS2_WriteCardData(IODS2_CMD_SD_WRITE_SINGLE_BLOCK(sector), IODS2_CTRL_WRITE_512B, buffer, 128);

	// wait
	ioDS2_WaitBusy();
}

void ioDS2_SDWriteMultiSector(u32 sector, const void *buffer, u32 num_sectors)
{
	for (int i = 0; i < num_sectors; i++)
	{
		ioDS2_WriteCardData(IODS2_CMD_SD_WRITE_MULTI_BLOCK(sector, num_sectors), IODS2_CTRL_WRITE_512B, buffer, 128);
		buffer = (u8 *)buffer + 0x200;

		// wait
		ioDS2_WaitBusy();
	}
}
