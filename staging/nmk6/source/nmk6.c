/*
	Acekard RPG
	Card IO routines

	Copyright (C) 2024 lifehackerhansol

	SPDX-License-Identifier: Zlib
*/

#include <nds/ndstypes.h>
#include <nds/card.h>

#include "libtwl_card.h"

static inline void NMK6_spiControlSetFlags(u16 flags)
{
    REG_AUXSPICNT = CARD_ENABLE | CARD_SPI_ENABLE | CARD_SPI_HOLD | flags;
}

static inline void NMK6_spiControlDisable(u16 flags)
{
    REG_AUXSPICNT = 0;
}

static void NMK6_spiDataWriteSingle(u8 data)
{
    NMK6_spiControlSetFlags(0);
    REG_AUXSPIDATA = data;
    while(REG_AUXSPICNT & CARD_SPI_BUSY);
    NMK6_spiControlDisable();
}

static void NMK6_spiDataWriteLoop(u8 * data, int size)
{
    if (size != 0) {
        for (int i=0; i < size; i++) {
            NMK6_spiControlSetFlags(0);
            REG_AUXSPIDATA = data;
            while(REG_AUXSPICNT & CARD_SPI_BUSY);
            NMK6_spiControlDisable();
        }
    }
}

static void NMK6_spiEnable(void)
{
    const u8 command[4] = { 0xfe, 0xfd, 0xfb, 0xf7 };
    NMK6_spiDataWriteLoop(command, 4);
}

static u32 NMK6_cardSendCommand(const u64 command, u32 flags)
{
    card_romSetCmd(command);
    card_romStartXfer(flags, false);
	card_romWaitDataReady();
    return card_romGetData();
}

static void NMK6_sdioSendCommand(u8 sdio, u32 parameter)
{
    u8 command[6] = {
        (sdio | 0x40),
        (u8)((parameter >> 24) & 0xFF),
        (u8)((parameter >> 16) & 0xFF),
        (u8)((parameter >> 8) & 0xFF),
        (u8)(parameter & 0xFF),
        0
    }
    command[5] = (crc7(command, 5) << 1) | 1;
    NMK6_cardSendCommand(NMK6_CMD_WRITE(0xFF), NMK6_CTRL_READ_4B);
    NMK6_cardSendCommand(NMK6_CMD_WRITE(0xFF), NMK6_CTRL_READ_4B);
    for(int i=0; i < 6; i++)
        NMK6_cardSendCommand(NMK6_CMD_WRITE(command[i]), NMK6_CTRL_READ_4B);
}

static void NMK6_sdioInitialize(void)
{
    for(int i=0; i < 50; i++)
    {
        NMK6_cardSendCommand(NMK6_CMD_WRITE(0xFF), NMK6_CTRL_READ_4B);
    }

    // CMD0
    NMK6_sdioSendCommand(0, 0);

    for (int i=0; i < 1024; i++)
    {
        
    }
}
