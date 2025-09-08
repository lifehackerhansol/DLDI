/*
	SPDX-FileCopyrightText: 2006 Michael "Chishm" Chisholm
	SPDX-License-Identifier: BSD-3-Clause
*/

#include <io_cf_common.h>
#include <io_m3_common.h>
#include <m3cf.h>

bool M3CF_Startup(const CF_REGISTERS *usableCfRegs) {
	_M3_changeMode (M3_MODE_MEDIA);
	return _CF_startup (usableCfRegs);
}
