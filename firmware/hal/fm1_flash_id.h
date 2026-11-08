/* SPDX-License-Identifier: GPL-3.0-only */
/* The flash chips the storage accepts (JEDEC id from 0x9F: fm1_flash.h fl_jedec_ram, read by project.c
 * persist_boot). Both are 1 MiB SPI NOR parts that answer the same plain commands this driver uses (9F, 05/35, 06,
 * 20, 02, 0B):
 *   0x856014  Puya P25Q80, the FM-1's chip until 2026
 *   0xB36014  UCUN UC25HQ80, in the FM-1s sold from autumn 2026 (the ones with the logo on the front)
 * Any other id leaves the firmware RAM-only (flash_ok 0): nothing is saved, and the update command is refused, so a
 * unit with a chip missing here cannot be updated over USB. That is what locked the new units in (Felucca #320,
 * Melodee b8f9f9f, MvaveFM1Unbricker): add a part here only when it is known to be a 1 MiB part with these commands. */
#pragma once
#define FL_JEDEC_OK(id) ((id) == 0x856014u || (id) == 0xB36014u)
