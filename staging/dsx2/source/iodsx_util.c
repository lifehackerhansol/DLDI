/*
    DS-Xtreme
    Card IO routine utility functions

    Copyright (C) 2006-2007 Michael Chisholm (Chishm)
    Copyright (C) 2007 DS-Xtreme

    SPDX-License-Identifier: BSD-3-Clause
*/

#define	REG_VCOUNT		(*(vu16*)0x4000006)

#include <nds/ndstypes.h>

void ioDSX_WaitMs(u32 requestTime)
{
	u32 lastLine = REG_VCOUNT;
	u32 newLine;
	u32 elapsedTime = 0; // in ms
	u32 elapsedLines = 0; // in lines


	while(elapsedTime < requestTime)
	{
		int diffLine;
		newLine = REG_VCOUNT;

		diffLine = newLine - lastLine;
		if (diffLine < 0)
			diffLine = 263+diffLine;

		elapsedLines += diffLine;

		//does this correctly optimize?
		elapsedTime = elapsedLines/16; // 16 lines = 1ms

		lastLine = newLine;
	}
}
