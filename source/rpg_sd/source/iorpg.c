/*
	Acekard RPG
	Card IO routines

	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#include <nds/ndstypes.h>
#include <nds/card.h>

#include "libtwl_card.h"

#include "iorpg.h"

static u32 isSDHC = 0;
static u32 SDSectorCount = 0;

extern void ioRPG_Delay(u32 count);

static inline void ioRPG_ReadCardData(u64 command, u32 flags, void *buffer, u32 length)
{
	card_romSetCmd(command);
	card_romStartXfer(flags, false);
	if ((u32)buffer & 3)
		card_romCpuReadUnaligned((u8 *)buffer, length);
	else
		card_romCpuRead(buffer, length);
}

// SDIO commands send every bit in a byte in order from MSB to LSB
// This function doesn't handle unaligned reads
static void ioRPG_SDSendSDIOCommand(u64 command, u32 *buffer, u32 length)
{
	u32 flags;
	if(length == 0)
		flags = IORPG_CTRL_POLL | MCCNT1_LATENCY1(80);
	else
		// actually the original driver used 4KB reads.
		// but with SDIO, it only really reads up to 136 bytes,
		// so 4KB is unnecessary.
		flags = IORPG_CTRL_READ_512B | MCCNT1_LATENCY1(40);

	// param SDIO, get response
	card_romSetCmd(command);
	card_romStartXfer(flags, false);

	// don't poll if we aren't reading
	if(length == 0) {
		card_romWaitBusy();
		return;
	}

	u32 i = 0;
	do
	{
		// Read data if available
		if (card_romIsDataReady())
		{
			*buffer++ = card_romGetData();
			i++;
		}
		// Exit loop when we reach the number of words needed
		if (i >= (length)) {
			break;
		}
	} while (card_romIsBusy());

	// Here we cut the transfer short. We don't actually read all 512 bytes as seen
	// in MCCNT1.
	// Cutting the transfer length in the middle of transfer causes undefined
	// behaviour, so wait until then
	card_romWaitDataReady();
	// add a delay just in case
	ioRPG_Delay(33);
	// Cut transmission
	ioRPG_ReadCardData(command, IORPG_CTRL_POLL, NULL, 0);
}

// This function gets the full R1 response, and truncates it to bits
// 8 - 39 in a single u32.
// QUIRK: RPG doesn't send the MSB. We must assume the MSB is 0, and shift the response >> 1.
static u32 ioRPG_SDSendR1Command(u8 cmd, u32 argument) {
	u32 buffer = 0;
	u8 *u8_buffer = (u8*)&buffer;
	ALIGN(4) u8 response[48];
	ioRPG_SDSendSDIOCommand(IORPG_CMD_SDIO(cmd, IORPG_SDIO_READ_RESPONSE, argument),  (u32 *)response, 48 >> 2);

	u32 bit_count = 38; // Get bits 8-39, and consider the quirk
	for(u32 i = 0; i < 4; ++i) {
		// Since every byte is a bit, pack it into a single byte
		// to represent the true value
		// BIT(7) is the needed bit from the return values
		for(u32 j = 0; j < 8; ++j)
		{
			u8 bit = (response[bit_count--] & BIT(7)) >> 7;
			u8_buffer[i] |= bit << j;
		}
	}

	return buffer;
}

// This function gets the full R2 response, and truncates it to bits
// 8 - 135. (CID/CSD with no CRC7.)
// QUIRK: RPG doesn't send the MSB. We must assume the MSB is 0, and shift the response >> 1.
static void ioRPG_SDSendR2Command(u8 cmd, u32 argument, u8 * buffer) {
	ALIGN(4) u8 response[136];
	ioRPG_SDSendSDIOCommand(IORPG_CMD_SDIO(cmd, IORPG_SDIO_READ_RESPONSE, argument), (u32 *)response, 136 >> 2);

	u32 bit_count = 134; // Get bits 8-135, and consider the quirk
	for(u32 i = 0; i < 16; ++i) {
		// Since every byte is a bit, pack it into a single byte
		// to represent the true value
		// BIT(7) is the needed bit from the return values
		for(u32 j = 0; j < 8; ++j)
		{
			u8 bit = (response[bit_count--] & BIT(7)) >> 7;
			buffer[i] |= bit << j;
		}
	}
}

static bool ioRPG_WaitBusy(void)
{
	bool timeout = true;
	u64 command = IORPG_CMD_CARD_WAIT_BUSY;

	u32 i;
	u32 data = 0;
	while(timeout)
	{
		i = 0;
		card_romSetCmd(command);
		card_romStartXfer(IORPG_CTRL_READ_16KB, false);
		do
		{
			if(card_romIsDataReady())
			{
				data = card_romGetData();
				i++;
				if (data == 0x00000FC2) {
					timeout = false;
					break;
				}
				ioRPG_Delay(16); // 1us
			}
		} while(card_romIsBusy());

		// If we were able to get the chip ID before we read all 16KiB, 
		// we need to cut transmission short
		// See comments in ioRPG_SDSendSDIOCommand for reference
		if (i != 0x1000)
		{
			card_romWaitDataReady();
			ioRPG_Delay(33);
			ioRPG_ReadCardData(command, IORPG_CTRL_POLL, NULL, 0);
		}
	}

	return !timeout;
}

// RPG uses MCCNT1 to transfer data instead of MCD1
// So we write two words for each transfer
static void ioRPG_SDWriteData(const u32 *buffer, u32 length)
{
	for (u32 i=0; i < length; i+=2)
	{
		ioRPG_ReadCardData(IORPG_CMD_SD_WRITE_DATA(buffer + i), IORPG_CTRL_POLL, NULL, 0);
	}
}

// TODO: what are each of the states?
static void ioRPG_SDWaitForState(uint8_t state) {
	u32 data = 0;
	u32 mask = 0x0F;
#if defined(AK2) || defined(R4IDSN)
	mask = 0xF0;
	state <<= 4;
#endif

	do {
		data = ioRPG_SendCommand(IORPG_CMD_SD_READ_STATE) & mask;
	} while(data != state);
}

u32 ioRPG_SendCommand(u64 command)
{
	card_romSetCmd(command);
	card_romStartXfer(IORPG_CTRL_READ_4B | MCCNT1_LATENCY1(4), false);
	card_romWaitDataReady();
	return card_romGetData();
}

extern u32 calculateSDSectorCount(u32 * csd);

// SDIO initialization
bool ioRPG_Initialize(void)
{
	int isSD20 = 0;
	u32 responseR1 = 0;
	u32 responseR2[4] = {};

	// CMD0
	ioRPG_SDSendSDIOCommand(IORPG_CMD_SDIO(0, IORPG_SDIO_NORESPONSE, 0), NULL, 0);

	// CMD8
	responseR1 = ioRPG_SDSendR1Command(8, 0x1AA);
	isSD20 = responseR1 == 0x1AA ? 1 : 0;

	for(u32 i = 0; i < 10000; i++)
	{
		// CMD55
		ioRPG_SDSendR1Command(55, 0);

		// ACMD41
		u32 argument = 0x00FF8000;
		if(isSD20)
			argument |= BIT(30);
		responseR1 = ioRPG_SDSendR1Command(41, argument);
		if(responseR1 & BIT(31))
		{
			isSDHC = responseR1 & BIT(30) ? 1 : 0;
			break;
		}
		ioRPG_Delay(1666);
	}

	// CMD2
	ioRPG_SDSendR2Command(2, 0, (u8 *)responseR2);

	// CMD3
	responseR1 = ioRPG_SDSendR1Command(3, 0);
	u32 sdio_rca = responseR1 >> 16;

	// CMD9
	ioRPG_SDSendR2Command(9, (sdio_rca << 16), (u8 *)responseR2);
	SDSectorCount = calculateSDSectorCount(responseR2);

	// CMD7
	ioRPG_SDSendR1Command(7, (sdio_rca << 16));

	// ACMD6
	ioRPG_SDSendR1Command(55, (sdio_rca << 16));
	ioRPG_SDSendR1Command(6, 2);

	// CMD13
	responseR1 = ioRPG_SDSendR1Command(13, (sdio_rca << 16));
	if((responseR1 >> 9) != 4)
		return false;

	if (isSDHC) {
		card_romSetCmd(IORPG_CMD_SET_SD_MODE_SDHC);
		card_romStartXfer(IORPG_CTRL_POLL, false);
		card_romWaitBusy();
	}

	return true;
}

void ioRPG_SDReadSingleSector(u32 sector, void* buffer)
{
	u32 address = isSDHC ? sector : sector << 9;
	// CMD17
	ioRPG_SDSendSDIOCommand(IORPG_CMD_SDIO(17, IORPG_SDIO_READ_SINGLE_BLOCK, address), NULL, 0);
	ioRPG_WaitBusy();

	ioRPG_ReadCardData(IORPG_CMD_CARD_READ_DATA, (IORPG_CTRL_READ_512B | MCCNT1_LATENCY1(4)), buffer, 128);
	ioRPG_Delay(80); // wait for sd crc auto complete
}

void ioRPG_SDReadMultiSector(u32 sector, u32 num_sectors, void* buffer)
{
	u32 address = isSDHC ? sector : sector << 9;
	// CMD18
	ioRPG_SDSendSDIOCommand(IORPG_CMD_SDIO(18, IORPG_SDIO_READ_MULTI_BLOCK, address), NULL, 0);
	ioRPG_WaitBusy();

	while(num_sectors)
	{
		ioRPG_ReadCardData(IORPG_CMD_CARD_READ_DATA, (IORPG_CTRL_READ_512B | MCCNT1_LATENCY1(4)), buffer, 128);
		ioRPG_SDWaitForState(0x7);
		buffer = (u8 *)buffer + 0x200;
		num_sectors--;

		// If we reached the last sector, we need to switch to single-sector reads to finish off
		if((++sector) == SDSectorCount) break;
	};

	// CMD12
	ioRPG_SDSendR1Command(12, 0);

	// Finish off remaining sectors that may be left
	if(num_sectors)
	{
		while(num_sectors)
		{
			ioRPG_SDReadSingleSector(sector, buffer);
			sector++;
			buffer = (u8 *)buffer + 0x200;
			num_sectors--;
		}
	}
}

void ioRPG_SDWriteSingleSector(u32 sector, const void* buffer)
{
	u32 address = isSDHC ? sector : sector << 9;
	// CMD24
	ioRPG_SDSendSDIOCommand(IORPG_CMD_SDIO(24, IORPG_SDIO_WRITE_SINGLE_BLOCK, address), NULL, 0);

	ioRPG_SDWriteData(buffer, 128);
	ioRPG_SDWaitForState(0);
}

void ioRPG_SDWriteMultiSector(u32 sector, u32 num_sectors, const void* buffer)
{
	u32 address = isSDHC ? sector : sector << 9;

	do
	{
		// CMD25
		ioRPG_SDSendSDIOCommand(IORPG_CMD_SDIO(25, IORPG_SDIO_WRITE_MULTI_BLOCK, address), NULL, 0);
		ioRPG_SDWriteData(buffer, 128);
		ioRPG_SDWaitForState(0xE);
		buffer = (u8 *)buffer + 0x200;
		num_sectors--;
	} while(num_sectors);

	// CMD12
	ioRPG_SDSendR1Command(12, 0);
	ioRPG_SDWaitForState(0);
}
