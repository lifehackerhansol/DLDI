/*
	iPlayer
	Card IO routines

	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#include <nds/ndstypes.h>
#include "iply.h"
#include "libtwl_card.h"

static inline void IPLY_ReadCardData(u64 command, u32 flags, void *buffer, u32 length)
{
	card_romSetCmd(command);
	card_romStartXfer(flags, false);
	if ((u32)buffer & 3)
		card_romCpuReadUnaligned((u8 *)buffer, length);
	else
		card_romCpuRead(buffer, length);
}

static inline void IPLY_WriteCardData(u64 command, u32 flags, const void *buffer, u32 length)
{
	card_romSetCmd(command);
	card_romStartXfer(flags, false);
	if ((u32)buffer & 3)
		card_romCpuWriteUnaligned((u8 *)buffer, length);
	else
		card_romCpuWrite(buffer, length);
}

u32 IPLY_SendCommand(u64 command)
{
	card_romSetCmd(command);
	card_romStartXfer(IPLY_CTRL_READ_4B, false);
	card_romWaitDataReady();
	return card_romGetData();
}

void IPLY_SDReadSector(u32 sector, void *buffer)
{
	// Initiate an operation
	IPLY_SendCommand(IPLY_CMD_SD_BEGIN_OPERATION);

	// Initiate a sector read
	IPLY_SendCommand(IPLY_CMD_SD_READ_SECTOR(sector));

	// Wait for data ready
	while(IPLY_SendCommand(IPLY_CMD_CARD_STATE) & BIT(28));

	// retrieve data
	IPLY_ReadCardData(IPLY_CMD_SD_READ_DATA, IPLY_CTRL_READ_512B, buffer, 128);
}

void IPLY_SDWriteSector(u32 sector, const void *buffer)
{
	// Initiate an operation
	IPLY_SendCommand(IPLY_CMD_SD_BEGIN_OPERATION);

	// Write to buffer
	IPLY_WriteCardData(IPLY_CMD_SD_WRITE_BUFFER, IPLY_CTRL_WRITE_512B, buffer, 128);

	// Flush the buffer
	IPLY_SendCommand(IPLY_CMD_SD_FLUSH_BUFFER(sector));

	// Wait for completion
	while(IPLY_SendCommand(IPLY_CMD_CARD_STATE) & BIT(21));
}
