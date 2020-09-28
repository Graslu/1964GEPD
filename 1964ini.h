/*$T 1964ini.h GC 1.136 02/28/02 07:55:22 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */


/*
 * 1964 Copyright (C) 1999-2002 Joel Middendorf, <schibo@emulation64.com> This
 * program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. To contact the
 * authors: email: schibo@emulation64.com, rice1964@yahoo.com
 */
#ifndef _1964INI_H__1964_
#define _1964INI_H__1964_

#include "globals.h"
#include <stdio.h>
#include <stdlib.h>

enum GAMESAVETYPE
{
	DEFAULT_SAVETYPE,
	EEPROM_SAVETYPE,
	MEMPAK_SAVETYPE,
	SRAM_SAVETYPE,
	FLASHRAM_SAVETYPE,
	FIRSTUSE_SAVETYPE,
	ANYUSED_SAVETYPE
};

enum EMULATORTYPE { DEFAULT_EMULATORTYPE, DYNACOMPILER, INTERPRETER };

enum EEPROMSIZE { EEPROMSIZE_DEFAULT, EEPROMSIZE_NONE, EEPROMSIZE_4KB, EEPROMSIZE_16KB };

enum CODECHECKTYPE
{
	CODE_CHECK_DEFAULT,
	CODE_CHECK_NONE,
	CODE_CHECK_DMA_ONLY,
	CODE_CHECK_MEMORY_DWORD,
	CODE_CHECK_MEMORY_QWORD,
	CODE_CHECK_MEMORY_QWORD_AND_DMA,
	CODE_CHECK_MEMORY_BLOCK,
	CODE_CHECK_MEMORY_BLOCK_AND_DMA,
	CODE_CHECK_PROTECT_MEMORY
};

enum USETLBTYPE { USETLB_DEFAULT, USETLB_YES, USETLB_NO };

enum MAXFPSTYPE { MAXFPS_DEFAULT, MAXFPS_NONE, MAXFPS_NTSC_60, MAXFPS_PAL_50, MAXFPS_AUTO_SYNC };

enum RDRAMSIZETYPE { RDRAMSIZE_DEFAULT, RDRAMSIZE_4MB, RDRAMSIZE_8MB };

enum USEREGISTERCACHING { USEREGC_DEFAULT, USEREGC_YES, USEREGC_NO };

enum COUNTERFACTOR
{
	COUTERFACTOR_DEFAULT			= 0,
	COUTERFACTOR_1,
	COUTERFACTOR_2,
	COUTERFACTOR_3,
	COUTERFACTOR_4,
	COUTERFACTOR_5,
	COUTERFACTOR_6,
	COUTERFACTOR_7,
	COUTERFACTOR_8
};

enum USEFPUHACK { USEFPUHACK_DEFAULT, USEFPUHACK_YES, USEFPUHACK_NO };

enum USEDMASEGMENTATION { USEDMASEG_DEFAULT, USEDMASEG_YES, USEDMASEG_NO };

enum USE4KBLINKBLOCK { USE4KBLINKBLOCK_DEFAULT, USE4KBLINKBLOCK_YES, USE4KBLINKBLOCK_NO };

enum USEADVANCEDBLOCKANALYSIS { USEBLOCKANALYSIS_DEFAULT, USEBLOCKANALYSIS_YES, BLOCKANALYSIS_NO };

enum ASSUME32BIT { ASSUME_32BIT_DEFAULT, ASSUME_32BIT_YES, ASSUME_32BIT_NO };

enum USEHLE { USEHLE_DEFAULT, USEHLE_YES, USEHLE_NO };

struct INI_ENTRY_STRUCT
{
	uint32				crc1;
	uint32				crc2;
	enum EMULATORTYPE	Emulator;
	enum GAMESAVETYPE	Save_Type;
	enum CODECHECKTYPE	Code_Check;
	char				Game_Name[40];
	char				Comments[80];
	char				Alt_Title[51];
	uint8				countrycode;
	uint8				RDRAM_Size;
	uint8				Max_FPS;
	uint8				Use_TLB;
	uint8				Eeprom_size;
	uint8				Counter_Factor;
	uint8				Use_Register_Caching;
	uint8				FPU_Hack;
	uint8				DMA_Segmentation;
	uint8				Link_4KB_Blocks;
	uint8				Advanced_Block_Analysis;
	uint8				Assume_32bit;
	uint8				Use_HLE;
};
typedef struct INI_ENTRY_STRUCT INI_ENTRY;

/* Support update to 50000 entries, should be enough for all the N64 Games + hacks */
#define MAX_INI_ENTRIES 50000

/* Globals definition */
char				*rdram_size_names[];
char				*save_type_names[];
char				*emulator_type_names[];
char				*codecheck_type_names[];
char				*maxfps_type_names[];
char				*usetlb_type_names[];
char				*eepromsize_type_names[];
char				*counter_factor_names[];
char				*register_caching_names[];
char				*use_fpu_hack_names[];
char				*use_dma_segmentation[];
char				*use_4kb_link_block_names[];
char				*use_block_analysis_names[];
char				*use_HLE_names[];
char				*assume_32bit_names[];

float				vips_speed_limits[];

extern INI_ENTRY	currentromoptions;
extern INI_ENTRY	*ini_entries[MAX_INI_ENTRIES];	/* Only allocate memory for entry pointers */

/* entries will be dynamically allocated */
extern int			ini_entry_count;

/* Function definition */
void				InitIniEntries(void);
INI_ENTRY			*GetNewIniEntry(void);
int					AddIniEntry(const INI_ENTRY *);
void				DeleteIniEntry(const int index);
void				DeleteAllIniEntries(void);
int					FindIniEntry(const char *gamename, const uint32 crc1, const uint32 crc2, const uint8 country);
int					FindIniEntry2(const INI_ENTRY *);
int					ReadIniEntry(FILE *, INI_ENTRY *);
int					WriteIniEntry(FILE *, const INI_ENTRY *);
int					ReadAllIniEntries(FILE *);
int					WriteAllIniEntries(FILE *);
void				CopyIniEntry(INI_ENTRY *, const INI_ENTRY *);
void				DeleteIniEntryByEntry(INI_ENTRY *pentry);
void				SetDefaultOptions(void);
void				GenerateCurrentRomOptions(void);
void				WriteProject64RDB(const uint32 crc1, const uint32 crc2, const uint8 countrycode);
int					Write1964DefaultOptionsEntry(FILE *pstream);
int					Read1964DefaultOptionsEntry(FILE *pstream);
void				chopm(char *str);
uint32				ConvertHexStringToInt(const char *str, int nchars);

/* 1964 default options */
extern INI_ENTRY	defaultoptions;
extern int			romlist_sort_method;

extern char			default_rom_directory[_MAX_PATH];
extern char			default_save_directory[_MAX_PATH];
extern char			default_state_save_directory[_MAX_PATH];
extern char			default_plugin_directory[_MAX_PATH];
extern char			user_set_rom_directory[_MAX_PATH];
extern char			user_set_save_directory[_MAX_PATH];
extern char			state_save_directory[_MAX_PATH];
extern char			user_set_plugin_directory[_MAX_PATH];
#endif
