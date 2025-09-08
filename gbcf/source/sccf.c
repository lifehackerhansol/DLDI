/*
	SPDX-FileCopyrightText: 2006 Michael "Chishm" Chisholm
	SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdbool.h>
#include <io_cf_common.h>
#include <sccf.h>


bool SCCF_Startup(const CF_REGISTERS *usableCfRegs) {
	SCCF_ChangeMode (SC_MODE_MEDIA);
	return _CF_startup (usableCfRegs);
}
