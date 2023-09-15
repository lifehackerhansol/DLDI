/*
	DS-Xtreme (v2)
	Card IO routines

	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#include <nds/ndstypes.h>
#include "libtwl_card.h"
#include "iodsx.h"

static void ioDSX_ReadCardData(u64 command, u32 flags, void *buffer, u32 length)
{
	card_romSetCmd(command);
	card_romStartXfer(flags, false);
	if ((u32)buffer & 3)
		card_romCpuReadUnaligned((u8 *)buffer, length);
	else
		card_romCpuRead(buffer, length);
}

static bool ioDSX_IsBusy(void)
{
	u32 ret;
	ioDSX_ReadCardData(IODSX_CMD_WAIT_BUSY, IODSX_CTRL_BASE | MCCNT1_LEN_4 | MCCNT1_LATENCY1(0x800), &ret, 1);
	return ret != 0;
}

static u32 currentZone = 0xFFFFFFFF;

static void ioDSX_SwitchZone(u32 sector)
{
	ALIGN(4) u8 tmpBuffer[0x200];
	u32 newZone = (sector >> 8) / 1000;

	if (newZone != currentZone)
	{
		ioDSX_ReadCardData(IODSX_CMD_READ_LBA(sector), IODSX_CTRL_BASE | MCCNT1_LEN_512 | MCCNT1_LATENCY1(0xFFF), tmpBuffer, 128);
		while(ioDSX_IsBusy());
	}

	currentZone = newZone;
}

void ioDSX_NANDReadSector(u32 sector, void *buffer)
{
	// wait until ready
	while(ioDSX_IsBusy());

	//put us in the right zone
	//NOTE: reads which cross zones are invalid.
	ioDSX_SwitchZone(sector);

	// retrieve data
	ioDSX_ReadCardData(IODSX_CMD_READ_LBA(sector), IODSX_CTRL_BASE | MCCNT1_LEN_512 | MCCNT1_LATENCY1(0x8F8), buffer, 128);
}

extern void ioDSX_WaitMs(u32 requestTime);

void ioDSX_NANDWriteSectors(u32 sector, u32 num_sectors, const u32 *buffer)
{
	u32 ret = 0;

	// wait until ready
	while(ioDSX_IsBusy());

	for(u32 i = 0; i < num_sectors; i++)
	{
		// put us in the right zone
		ioDSX_SwitchZone(sector);

		// clear FPGA address
		// note according to original developers, only doing this command once has undefined behaviour. (why?)
		ioDSX_ReadCardData(IODSX_CMD_RESET_FPGA_ADDRESS, IODSX_CTRL_BASE | MCCNT1_LEN_0 | MCCNT1_LATENCY1(0), NULL, 0);
		ioDSX_ReadCardData(IODSX_CMD_RESET_FPGA_ADDRESS, IODSX_CTRL_BASE | MCCNT1_LEN_0 | MCCNT1_LATENCY1(0), NULL, 0);

		// transfer to card buffer
		for(u32 j = 0; j < 128; j++)
			ioDSX_ReadCardData(IODSX_CMD_WRITE_LBA_TRANSFER_DATA(buffer[j]), IODSX_CTRL_BASE | MCCNT1_LEN_0 | MCCNT1_LATENCY1(0), NULL, 0);

		// commit the buffer to NAND
		ioDSX_ReadCardData(IODSX_CMD_WRITE_LBA_COMMIT_DATA(sector), IODSX_CTRL_BASE | MCCNT1_LEN_4 | MCCNT1_LATENCY1(0xFFF), &ret, 1);

		// let it finish
		while(ioDSX_IsBusy());

		sector++;
		buffer += 128;
	}

	// finalize
	ioDSX_ReadCardData(IODSX_CMD_STOP_TRANSMISSION, IODSX_CTRL_BASE | MCCNT1_LEN_4 | MCCNT1_LATENCY1(0xFFF), &ret, 1);

	// wait (again)
	while(ioDSX_IsBusy());

	/*
		Now wait some more.

		If you're trying to optimize, and see this innocent looking, seemingly
		senseless wait, and decide to remove it, or even lower it, you'll regret it.

		Seriously.

		I'm not kidding around here.

		Just leave it alone.
	*/
	ioDSX_WaitMs(10);
}
