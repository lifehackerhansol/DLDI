/*
	DS-Xtreme (v2)
	Card IO routines

	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#include <nds/system.h>
#include <nds/ndstypes.h>
#include <libtwl/card/card.h>
#include <common/libtwl_ext.h>
#include "dsx.h"

static bool DSX_IsBusy(void)
{
	u32 ret;
	cardExt_RomReadData(DSX_CMD_WAIT_BUSY, DSX_CTRL_BASE | MCCNT1_LEN_4 | MCCNT1_LATENCY1(0x800), &ret, 1);
	return ret != 0;
}

static u32 currentZone = 0xFFFFFFFF;

static void DSX_SwitchZone(u32 sector)
{
	u32 newZone = (sector >> 8) / 1000;

	if (newZone != currentZone)
	{
		cardExt_RomReadData(DSX_CMD_READ_LBA(sector), DSX_CTRL_BASE | MCCNT1_LEN_512 | MCCNT1_LATENCY1(0xFFF), NULL, 0);
		while(DSX_IsBusy());
	}

	currentZone = newZone;
}

void DSX_NANDReadSector(u32 sector, void *buffer)
{
	// wait until ready
	while(DSX_IsBusy());

	//put us in the right zone
	//NOTE: reads which cross zones are invalid.
	DSX_SwitchZone(sector);

	// retrieve data
	cardExt_RomReadData(DSX_CMD_READ_LBA(sector), DSX_CTRL_BASE | MCCNT1_LEN_512 | MCCNT1_LATENCY1(0x8F8), buffer, 128);
}

void DSX_NANDWriteSectors(u32 sector, u32 num_sectors, const void *buffer)
{
	u32 ret = 0;

	// wait until ready
	while(DSX_IsBusy());

	for(u32 i = 0; i < num_sectors; i++)
	{
		// put us in the right zone
		DSX_SwitchZone(sector);

		// clear FPGA address
		// note according to original developers, only doing this command once has undefined behaviour. (why?)
		cardExt_RomReadData(DSX_CMD_RESET_FPGA_ADDRESS, DSX_CTRL_BASE | MCCNT1_LEN_0 | MCCNT1_LATENCY1(0), NULL, 0);
		cardExt_RomReadData(DSX_CMD_RESET_FPGA_ADDRESS, DSX_CTRL_BASE | MCCNT1_LEN_0 | MCCNT1_LATENCY1(0), NULL, 0);

		if((u32)buffer & 3) {
			u8 *u8_buffer = (u8*)buffer;
			for (u32 j=0; j < 512; j+=4) {
				u32 data = u8_buffer[j] | (u8_buffer[j + 1] << 8) | (u8_buffer[j + 2] << 16) | (u8_buffer[j + 3] << 24);
				cardExt_RomReadData(DSX_CMD_WRITE_LBA_TRANSFER_DATA(data), DSX_CTRL_BASE | MCCNT1_LEN_0 | MCCNT1_LATENCY1(0), NULL, 0);
			}
		}
		else
			// transfer to card buffer
			for(u32 j = 0; j < 128; j++)
				cardExt_RomReadData(DSX_CMD_WRITE_LBA_TRANSFER_DATA(((u32*)buffer)[j]), DSX_CTRL_BASE | MCCNT1_LEN_0 | MCCNT1_LATENCY1(0), NULL, 0);

		// commit the buffer to NAND
		cardExt_RomReadData(DSX_CMD_WRITE_LBA_COMMIT_DATA(sector), DSX_CTRL_BASE | MCCNT1_LEN_4 | MCCNT1_LATENCY1(0xFFF), &ret, 1);

		// let it finish
		while(DSX_IsBusy());

		sector++;
		buffer += 128;
	}

	// finalize
	cardExt_RomReadData(DSX_CMD_STOP_TRANSMISSION, DSX_CTRL_BASE | MCCNT1_LEN_4 | MCCNT1_LATENCY1(0xFFF), &ret, 1);

	// wait (again)
	while(DSX_IsBusy());

	/*
		Official code suggests we must wait at least 10ms here for the flush to complete.
		We will wait at least one full VBlank.
		This will wait a full 16.67ms + wherever we were when we started the first loop.
	*/
	// waits until VCOUNT 191, where we start tracking
	while(REG_VCOUNT != 191);
	// wait until we exit VCOUNT 191
	while(REG_VCOUNT == 191);
	// Start a full VBlank wait
	while(REG_VCOUNT != 191);
}
