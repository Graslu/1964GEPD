/*$T 1964ini.c GC 1.136 03/09/02 17:36:01 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Source for the managing the 1964.ini file.
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

#include <stdio.h>
#include "romlist.h"
#include "win32/registry.h"

uint32		ConvertHexCharToInt(char c);
uint32		ConvertHexStringToInt(const char *str, int nchars);
void		chopm(char *str);

/* Global variables */
char		*rdram_size_names[] = { "Default", "No", "Yes" };
char		*save_type_names[] = { "Default", "EEPROM", "MEMPAK", "SRAM", "FLASHRAM", "First Use", "All Used Types" };
char		*emulator_type_names[] = { "Default", "Dyna-Complier", "Interpreter" };
char		*codecheck_type_names[] =
{
	"Default",
	"No Check",
	"Check DMA only",
	"Check DWORD",
	"Check QWORD",
	"Check QWORD & DMA",
	"Check Block",
	"Check Block & DMA",
	"Protect Memory"
};
char		*maxfps_type_names[] = { "Default", "No Limit", "NTSC 60 fps", "PAL 50 fps", "Auto Sync" };
char		*usetlb_type_names[] = { "Default", "Yes", "No" };
char		*eepromsize_type_names[] = { "Default", "No EEPROM", "4Kb EEPROM", "16Kb EEPROM" };
char		*counter_factor_names[] = { "Default", "CF=1", "CF=2", "CF=3", "CF=4", "CF=5", "CF=6", "CF=7", "CF=8", };
char		*register_caching_names[] = { "Default", "Yes", "No" };
char		*use_fpu_hack_names[] = { "Default", "Yes", "No" };
char		*use_dma_segmentation[] = { "Default", "Yes", "No" };
char		*use_4kb_link_block_names[] = { "Default", "Yes", "No" };
char		*use_block_analysis_names[] = { "Default", "Yes", "No" };
char		*assume_32bit_names[] = { "Default", "Yes", "No" };
char		*use_HLE_names[] = { "Default", "Yes", "No" };

char		default_rom_directory[_MAX_PATH];
char		default_save_directory[_MAX_PATH];
char		default_state_save_directory[_MAX_PATH];
char		default_plugin_directory[_MAX_PATH];
char		user_set_rom_directory[_MAX_PATH];
char		user_set_save_directory[_MAX_PATH];
char		state_save_directory[_MAX_PATH];
char		user_set_plugin_directory[_MAX_PATH];

float		vips_speed_limits[] = { 500.0f, 500.0f, 60.0f, 50.0f, 60.0f };
int			ini_entry_count = 0;

INI_ENTRY	*ini_entries[MAX_INI_ENTRIES] = {NULL};	/* The array of ini entries. Array is sorted by Game Name */
INI_ENTRY	defaultoptions;							/* 1964 default options */
INI_ENTRY	currentromoptions;						/* option setting for the current ROM, options are generates */

/*
 =======================================================================================================================
    from rom options and 1964 default options if the options £
    are set as DEFAULT in ROM setting
 =======================================================================================================================
 */
void SetDefaultOptions(void)
{
	defaultoptions.Code_Check = CODE_CHECK_PROTECT_MEMORY;
	defaultoptions.Comments[0] = '\0';
	defaultoptions.Alt_Title[0] = '\0';
	defaultoptions.countrycode = 0;
	defaultoptions.crc1 = 0;
	defaultoptions.crc2 = 0;
	defaultoptions.Emulator = DYNACOMPILER;
	defaultoptions.Game_Name[0] = '\0';
	defaultoptions.Max_FPS = MAXFPS_AUTO_SYNC;
	defaultoptions.RDRAM_Size = RDRAMSIZE_8MB;
	defaultoptions.Save_Type = ANYUSED_SAVETYPE;
	defaultoptions.Use_TLB = USETLB_YES;
	defaultoptions.Eeprom_size = EEPROMSIZE_4KB;
	defaultoptions.Use_Register_Caching = USEREGC_YES;
	defaultoptions.Counter_Factor = COUTERFACTOR_1;
	defaultoptions.FPU_Hack = USEFPUHACK_YES;
	defaultoptions.DMA_Segmentation = USEDMASEG_YES;
	defaultoptions.Link_4KB_Blocks = USE4KBLINKBLOCK_YES;
	defaultoptions.Advanced_Block_Analysis = USEBLOCKANALYSIS_YES;
	defaultoptions.Assume_32bit = ASSUME_32BIT_NO;
	defaultoptions.Use_HLE = USEHLE_NO;
}

/*
 =======================================================================================================================
    This function should be called everytime before playing a game, will £
    recalculate the values in the currentromoptions for emulator and CPU £
    core to use
 =======================================================================================================================
 */
void GenerateCurrentRomOptions(void)
{
	__try{

	CopyIniEntry(&currentromoptions, RomListSelectedEntry()->pinientry);

	if(RomListSelectedEntry()->pinientry->Code_Check == 0) currentromoptions.Code_Check = defaultoptions.Code_Check;

	if(RomListSelectedEntry()->pinientry->Eeprom_size == 0)
		currentromoptions.Eeprom_size = defaultoptions.Eeprom_size;

	if(RomListSelectedEntry()->pinientry->Emulator == 0) currentromoptions.Emulator = defaultoptions.Emulator;
	if(RomListSelectedEntry()->pinientry->Max_FPS == 0) currentromoptions.Max_FPS = defaultoptions.Max_FPS;
	if(RomListSelectedEntry()->pinientry->RDRAM_Size == 0) currentromoptions.RDRAM_Size = defaultoptions.RDRAM_Size;
	if(RomListSelectedEntry()->pinientry->Save_Type == 0) currentromoptions.Save_Type = defaultoptions.Save_Type;
	if(RomListSelectedEntry()->pinientry->Use_TLB == 0) currentromoptions.Use_TLB = defaultoptions.Use_TLB;

	if(RomListSelectedEntry()->pinientry->Counter_Factor == 0)
		currentromoptions.Counter_Factor = defaultoptions.Counter_Factor;

	if(RomListSelectedEntry()->pinientry->Use_Register_Caching == 0)
		currentromoptions.Use_Register_Caching = defaultoptions.Use_Register_Caching;

	if(RomListSelectedEntry()->pinientry->FPU_Hack == 0) currentromoptions.FPU_Hack = defaultoptions.FPU_Hack;

	if(RomListSelectedEntry()->pinientry->DMA_Segmentation == 0)
		currentromoptions.DMA_Segmentation = defaultoptions.DMA_Segmentation;

	if(RomListSelectedEntry()->pinientry->Link_4KB_Blocks == 0)
		currentromoptions.Link_4KB_Blocks = defaultoptions.Link_4KB_Blocks;

	if(RomListSelectedEntry()->pinientry->Advanced_Block_Analysis == 0)
		currentromoptions.Advanced_Block_Analysis = defaultoptions.Advanced_Block_Analysis;

	if(RomListSelectedEntry()->pinientry->Assume_32bit == 0)
		currentromoptions.Assume_32bit = defaultoptions.Assume_32bit;

	if(RomListSelectedEntry()->pinientry->Use_HLE == 0) currentromoptions.Use_HLE = defaultoptions.Use_HLE;

	if(RomListSelectedEntry()->pinientry->countrycode == 0x45) // if USA ROM
	{
		if(!strcmp(rominfo.name, "GOLDENEYE") || strstr(rominfo.name, "GOLD") != NULL)
			WriteProject64RDB(RomListSelectedEntry()->pinientry->crc1, RomListSelectedEntry()->pinientry->crc2, RomListSelectedEntry()->pinientry->countrycode);

		if(!strcmp(rominfo.name, "Perfect Dark") || !strcmp(rominfo.name, "GoldenEye X") || strstr(rominfo.name, "Perfect") != NULL)
		{
			currentromoptions.Eeprom_size = EEPROMSIZE_16KB;
			WriteProject64RDB(RomListSelectedEntry()->pinientry->crc1, RomListSelectedEntry()->pinientry->crc2, RomListSelectedEntry()->pinientry->countrycode);
		}
	}

	if
	(
		RomListSelectedEntry()->pinientry->Code_Check != CODE_CHECK_PROTECT_MEMORY
	&&	RomListSelectedEntry()->pinientry->Code_Check != CODE_CHECK_NONE
	&&	RomListSelectedEntry()->pinientry->Link_4KB_Blocks == USE4KBLINKBLOCK_YES
	)
	{
		DisplayError("Link 4KB cannot be used when self-mod code checking method is not PROTECT_MEMORY or No_Check, setting the Link_4KB to No");
		RomListSelectedEntry()->pinientry->Link_4KB_Blocks = USE4KBLINKBLOCK_NO;
	}
	}
	__except(NULL,EXCEPTION_EXECUTE_HANDLER)
	{
		DisplayError("Error Generating Rom Options. Possibly Corrupt ROM.");
	}
}

/*
 =======================================================================================================================
    Scans Project64.rdb for GOLDENEYE\Perfect Dark\GoldenEye X settings
    and if not found write new settings
 =======================================================================================================================
 */
void WriteProject64RDB(const uint32 crc1, const uint32 crc2, const uint8 countrycode)
{
	/*~~~~~~~~~~~~~~*/
	FILE *fp;
	int found = 0;
	char line[512], search_string[32];
	/*~~~~~~~~~~~~~~*/

	if(!strstr(gRegSettings.VideoPlugin, "Jabo")) /* if jabo isn't current video plugin, don't bother updating rdb file */
		return;
	sprintf(search_string, "[%08X-%08X-C:%02X]", crc1, crc2, countrycode);
	if((fp = fopen("Project64.rdb", "a+")) != NULL)
	{
		while(!found && fgets(line, 512, fp) != NULL) /* read every line */
			if(strstr(line, search_string)) /* setting exists exit function */
				found = 1;
		if(!found) /* write jabo settings at eof */
			fprintf(fp, "\n\n[%08X-%08X-C:%02X]\nClear Frame=2\nCulling=1\nSelf Texture=0\nPrimary Frame Buffer=0\nEmulate Clear=0\n", crc1, crc2, countrycode);
		fclose(fp);
	}
}
/*
 =======================================================================================================================
    Initialize all pointers int the array
 =======================================================================================================================
 */
void InitIniEntries(void)
{
	/*~~~~~~~~~~~~~~*/
	register int	i;
	/*~~~~~~~~~~~~~~*/

	for(i = 0; i < MAX_INI_ENTRIES; i++) ini_entries[i] = NULL;
}

/*
 =======================================================================================================================
    Allocate memory for a new entry, assign the default values and £
    return the pointer
 =======================================================================================================================
 */
INI_ENTRY *GetNewIniEntry(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
	INI_ENTRY	*newentry = NULL;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

	newentry = (INI_ENTRY *) VirtualAlloc(NULL, sizeof(INI_ENTRY), MEM_COMMIT, PAGE_READWRITE);
	newentry->Code_Check = 0;
	newentry->Comments[0] = '\0';
	newentry->Alt_Title[0] = '\0';
	newentry->countrycode = 0;
	newentry->crc1 = 0;
	newentry->crc2 = 0;
	newentry->Emulator = 0;
	newentry->Game_Name[0] = '\0';
	newentry->Max_FPS = 0;
	newentry->RDRAM_Size = 0;
	newentry->Save_Type = 0;
	newentry->Use_TLB = 0;
	newentry->Eeprom_size = 0;
	newentry->Counter_Factor = 1;
	newentry->Use_Register_Caching = 1;
	newentry->FPU_Hack = 0;
	newentry->DMA_Segmentation = 0;
	newentry->Link_4KB_Blocks = 0;
	newentry->Advanced_Block_Analysis = 0;
	newentry->Assume_32bit = 0;
	newentry->Use_HLE = 0;
	return(newentry);
}

/*
 =======================================================================================================================
    Add new entry into the list £
    Return value: £
    1) New entry, return the index of new entry £
    2) Overwrite old entry, return the index of old entry £
    3) Cannot insert new entry to list, return -1
 =======================================================================================================================
 */
int AddIniEntry(const INI_ENTRY *pnewentry)
{
	/*~~~~~~~~~~~~~~~~~~*/
	int i;
	int compname, compcrc;
	/*~~~~~~~~~~~~~~~~~~*/

	i = 0;
	if(ini_entry_count < MAX_INI_ENTRIES)
	{
		/* Step 1, locate the insertion point */
		while(i < ini_entry_count)
		{
			if(strlen(ini_entries[i]->Game_Name) == 0 && strlen(pnewentry->Game_Name) == 0)
			{
				compname = 0;
			}
			else
			{
				compname = stricmp(ini_entries[i]->Game_Name, pnewentry->Game_Name);
			}

			if(pnewentry->crc1 == ini_entries[i]->crc1 && pnewentry->crc2 == ini_entries[i]->crc2)
			{
				compcrc = 0;
			}
			else if
				(
					pnewentry->crc1 < ini_entries[i]->crc1
				||	(pnewentry->crc1 < ini_entries[i]->crc1 && pnewentry->crc2 <= ini_entries[i]->crc2)
				)
			{
				compcrc = -1;
			}
			else
			{
				compcrc = 1;
			}

			/*
			 * DisplayError("i=%d, newcrc=%16X, crc=%16X, Compare Name=%d",i,
			 * newcrc,crc,compname);
			 */
			if
			(
				compname > 0
			||	(compname == 0 && compcrc > 0)
			||	(compname == 0 && compcrc == 0 && ini_entries[i]->countrycode >= pnewentry->countrycode)
			) break;
			else
				i++;
		}

		if
		(
			i < ini_entry_count
		&&	compname == 0
		&&	compcrc == 0
		&&	ini_entries[i]->countrycode == pnewentry->countrycode
		)
		{
			/* Duplicate entry, just copy the new entry to the old entry */
			CopyIniEntry(ini_entries[i], pnewentry);
		}
		else
		{
			/*~~~~~~~~~~~*/
			INI_ENTRY	*p;
			/*~~~~~~~~~~~*/

			p = GetNewIniEntry();
			if(p == NULL)
			{
				return -1;	/* Cannot insert any new entry */
			}

			/* A new entry */
			if(i < ini_entry_count)
			{
				/*~~*/
				/* Insert in the middle of array */
				int j;
				/*~~*/

				for(j = ini_entry_count; j > i; j--) ini_entries[j] = ini_entries[j - 1];
			}

			CopyIniEntry(p, pnewentry);
			ini_entries[i] = p;
			ini_entry_count++;
		}

		return i;
	}
	else
	{
		return -1;			/* Cannot insert new entry */
	}
}

/*
 =======================================================================================================================
    Delete the entry at index
 =======================================================================================================================
 */
void DeleteIniEntry(const int index)
{
	/*~~~~~~~~~~~~~~*/
	register int	i;
	/*~~~~~~~~~~~~~~*/

	if(index < 0 || index >= ini_entry_count) return;

	/* VirtualFree((void*)ini_entries[index], sizeof(INI_ENTRY), MEM_DECOMMIT); */
	VirtualFree((void *) ini_entries[index], 0, MEM_RELEASE);

	/* Move all rest entries forward */
	for(i = index; i < ini_entry_count - 1; i++) ini_entries[i] = ini_entries[i + 1];

	ini_entries[ini_entry_count] = NULL;
	ini_entry_count--;
}

/*
 =======================================================================================================================
    Delete one entry and free the memory
 =======================================================================================================================
 */
void DeleteIniEntryByEntry(INI_ENTRY *pentry)
{
	/* VirtualFree((void*)pentry, sizeof(INI_ENTRY), MEM_DECOMMIT); */
	VirtualFree((void *) pentry, 0, MEM_RELEASE);
}

/*
 =======================================================================================================================
    Find the index of the ini entry in the ini_entries array
 =======================================================================================================================
 */
int FindIniEntry(const char *gamename, const uint32 crc1, const uint32 crc2, const uint8 countrycode)
{
	/*~~~~~~~~~~~~~~~~~~*/
	register int	i = 0;
	/*~~~~~~~~~~~~~~~~~~*/

	while
	(
		i < ini_entry_count
	&&	(
			strcmp(ini_entries[i]->Game_Name, gamename) != 0
		||	ini_entries[i]->crc1 != crc1
		||	ini_entries[i]->crc2 != crc2
		||	ini_entries[i]->countrycode != countrycode
		)
	)
	{
		i++;
	}

	if
	(
		i < ini_entry_count
	&&	strcmp(ini_entries[i]->Game_Name, gamename) == 0
	&&	ini_entries[i]->crc1 == crc1
	&&	ini_entries[i]->crc2 == crc2
	&&	ini_entries[i]->countrycode == countrycode
	)
	{
		return i;
	}
	else
		return -1;
}

/*
 =======================================================================================================================
    Find the index of the ini entry in the ini_entries array
 =======================================================================================================================
 */
int FindIniEntry2(const INI_ENTRY *p)
{
	return(FindIniEntry(p->Game_Name, p->crc1, p->crc2, p->countrycode));
}

/*
 =======================================================================================================================
    Read value from the the current INI_ENTRY from 1964.ini as default options
 =======================================================================================================================
 */
BOOL Read1964DefaultOptionsEntry(FILE *pstream)
{
	if(ReadIniEntry(pstream, &defaultoptions))
	{
		defaultoptions.crc1 = 0;
		defaultoptions.crc2 = 0;
		defaultoptions.countrycode = 0;
		strcpy(defaultoptions.Comments, "");
		strcpy(defaultoptions.Game_Name, "1964 Default Options");
		return TRUE;
	}

	return FALSE;
}

/*
 =======================================================================================================================
    Read a INI_ENTRY from 1964.ini
 =======================================================================================================================
 */
BOOL ReadIniEntry(FILE *pstream, INI_ENTRY *pnewentry)
{
	/*~~~~~~~~~~~~~~*/
	char	line[256];
	BOOL	goodline;
	/*~~~~~~~~~~~~~~*/

	/* Here set all attributes to default value */
	pnewentry->Code_Check = 0;
	pnewentry->Emulator = 0;
	pnewentry->Max_FPS = 0;
	pnewentry->RDRAM_Size = 0;
	pnewentry->Save_Type = 0;
	pnewentry->Use_TLB = 0;
	pnewentry->Eeprom_size = 0;
	pnewentry->Counter_Factor = 0;
	pnewentry->Use_Register_Caching = 0;
	pnewentry->FPU_Hack = 0;
	pnewentry->DMA_Segmentation = 0;
	pnewentry->Link_4KB_Blocks = 0;
	pnewentry->Advanced_Block_Analysis = 0;
	pnewentry->Assume_32bit = 0;
	pnewentry->Use_HLE = 0;
	pnewentry->Alt_Title[0] = '\0';

	/* Skip all empty lines and comments lines */
	do
	{
		fgets(line, 255, pstream);
		chopm(line);
	} while((strlen(line) < 2 || strncmp(line, "//", 2) == 0) && feof(pstream) == 0);

	if(feof(pstream) != 0) return FALSE;	/* Read entry failed, reach to the end of file */

	/* Now start to read the entry record */
	goodline = TRUE;
	do
	{
		if(strncmp(line, "[", 1) == 0 && strncmp(line + 23, "]", 1) == 0)
		{
			pnewentry->crc1 = ConvertHexStringToInt(line + 1, 8);
			pnewentry->crc2 = ConvertHexStringToInt(line + 10, 8);
			pnewentry->countrycode = (uint8) ConvertHexStringToInt(line + 21, 2);

			/*
			 * sscanf(line,"[%8X-%8X-C:%2X]",pnewentry->crc1, pnewentry->crc2,
			 * pnewentry->countrycode);
			 */
		}
		else if(strncmp(line, "Game Name=", 10) == 0)
		{
			strcpy(pnewentry->Game_Name, line + 10);
		}
		else if(strncmp(line, "Comments=", 9) == 0)
		{
			strcpy(pnewentry->Comments, line + 9);
		}
		else if(strncmp(line, "Alternate Title=", 16) == 0)
		{
			strcpy(pnewentry->Alt_Title, line + 16);
		}
		else if(strncmp(line, "RDRAM Size=", 11) == 0)
		{
			pnewentry->RDRAM_Size = atoi(line + 11);
		}
		else if(strncmp(line, "Save Type=", 10) == 0)
		{
			pnewentry->Save_Type = atoi(line + 10);
		}
		else if(strncmp(line, "Emulator=", 9) == 0)
		{
			pnewentry->Emulator = atoi(line + 9);
		}
		else if(strncmp(line, "Check Self-modifying Code=", 26) == 0)
		{
			pnewentry->Code_Check = atoi(line + 26);
			if(pnewentry->Code_Check > CODE_CHECK_PROTECT_MEMORY) pnewentry->Code_Check = CODE_CHECK_MEMORY_QWORD;
		}
		else if(strncmp(line, "Max FPS=", 8) == 0)
		{
			pnewentry->Max_FPS = atoi(line + 8);
		}
		else if(strncmp(line, "TLB=", 4) == 0)
		{
			pnewentry->Use_TLB = atoi(line + 4);
		}
		else if(strncmp(line, "EEPROM Size=", 12) == 0)
		{
			pnewentry->Eeprom_size = atoi(line + 12);
		}
		else if(strncmp(line, "Use Register Caching=", 21) == 0)
		{
			pnewentry->Use_Register_Caching = atoi(line + 21);
			if(pnewentry->Use_Register_Caching > 2) pnewentry->Use_Register_Caching = 0;
		}
		else if(strncmp(line, "Counter Factor=", 15) == 0)
		{
			pnewentry->Counter_Factor = atoi(line + 15);
			if(pnewentry->Counter_Factor > 8) pnewentry->Counter_Factor = 0;
		}
		else if(strncmp(line, "FPU Hack=", 9) == 0)
		{
			pnewentry->FPU_Hack = atoi(line + 9);
			if(pnewentry->FPU_Hack > 2) pnewentry->FPU_Hack = 0;
		}
		else if(strncmp(line, "DMA=", 4) == 0)
		{
			pnewentry->DMA_Segmentation = atoi(line + 4);
			if(pnewentry->DMA_Segmentation > 2) pnewentry->DMA_Segmentation = 0;
		}
		else if(strncmp(line, "Link 4KB Blocks=", 16) == 0)
		{
			pnewentry->Link_4KB_Blocks = atoi(line + 16);
			if(pnewentry->Link_4KB_Blocks > 2) pnewentry->Link_4KB_Blocks = 0;
		}
		else if(strncmp(line, "Advanced Block Analysis=", 24) == 0)
		{
			pnewentry->Advanced_Block_Analysis = atoi(line + 24);
			if(pnewentry->Advanced_Block_Analysis > 2) pnewentry->Advanced_Block_Analysis = 0;
		}
		else if(strncmp(line, "Assume 32bit=", 13) == 0)
		{
			pnewentry->Assume_32bit = atoi(line + 13);
			if(pnewentry->Assume_32bit > 2) pnewentry->Assume_32bit = 0;
		}
		else if(strncmp(line, "HLE=", 4) == 0)
		{
			pnewentry->Use_HLE = atoi(line + 4);
			if(pnewentry->Use_HLE > 2) pnewentry->Use_HLE = 0;
		}
		else
			goodline = FALSE;

		if(goodline)
		{
			fgets(line, 256, pstream);
			chopm(line);
		}
	} while(goodline && feof(pstream) == 0);

	return TRUE;
}

/*
 =======================================================================================================================
    Write the default options to file
 =======================================================================================================================
 */
BOOL Write1964DefaultOptionsEntry(FILE *pstream)
{
	/*~~~~~~~~*/
	BOOL	ret;
	/*~~~~~~~~*/

	ret = WriteIniEntry(pstream, &defaultoptions);
	if(ret)
		return TRUE;
	else
		return FALSE;
}

/*
 =======================================================================================================================
    Write an INI_ENTRY to file
 =======================================================================================================================
 */
BOOL WriteIniEntry(FILE *pstream, const INI_ENTRY *p)
{
	if(p == NULL) return FALSE;

	fprintf(pstream, "[%08X-%08X-C:%02X]\n", p->crc1, p->crc2, p->countrycode);
	fprintf(pstream, "Game Name=%s\n", p->Game_Name);
	fprintf(pstream, "Comments=%s\n", p->Comments);

	if(strlen(p->Alt_Title) > 0) fprintf(pstream, "Alternate Title=%s\n", p->Alt_Title);
	if(p->Code_Check != 0 && p->Code_Check != defaultoptions.Code_Check)
		fprintf(pstream, "Check Self-modifying Code=%d\n", p->Code_Check);
	if(p->Emulator != 0 && p->Emulator != defaultoptions.Emulator) fprintf(pstream, "Emulator=%d\n", p->Emulator);
	if(p->RDRAM_Size != 0 && p->RDRAM_Size != defaultoptions.RDRAM_Size)
		fprintf(pstream, "RDRAM Size=%d\n", p->RDRAM_Size);
	if(p->Max_FPS != 0 && p->Max_FPS != defaultoptions.Max_FPS) fprintf(pstream, "Max FPS=%d\n", p->Max_FPS);
	if(p->Save_Type != 0 && p->Save_Type != defaultoptions.Save_Type)
		fprintf(pstream, "Save Type=%d\n", p->Save_Type);
	if(p->Use_TLB != 0 && p->Use_TLB != defaultoptions.Use_TLB) 
	{
		fprintf(pstream, "TLB=%d\n", p->Use_TLB);
	}
	if(p->Eeprom_size != 0 && p->Eeprom_size != defaultoptions.Eeprom_size)
		fprintf(pstream, "EEPROM Size=%d\n", p->Eeprom_size);
	if(p->Counter_Factor != 0 && p->Counter_Factor != defaultoptions.Counter_Factor)
		fprintf(pstream, "Counter Factor=%d\n", p->Counter_Factor);
	if(p->Use_Register_Caching != 0 && p->Use_Register_Caching != defaultoptions.Use_Register_Caching)
		fprintf(pstream, "Use Register Caching=%d\n", p->Use_Register_Caching);
	if(p->FPU_Hack != 0 && p->FPU_Hack != defaultoptions.FPU_Hack) fprintf(pstream, "FPU Hack=%d\n", p->FPU_Hack);
	if(p->DMA_Segmentation != 0 && p->DMA_Segmentation != defaultoptions.DMA_Segmentation)
		fprintf(pstream, "DMA=%d\n", p->DMA_Segmentation);
	if(p->Link_4KB_Blocks != 0 && p->Link_4KB_Blocks != defaultoptions.Link_4KB_Blocks)
		fprintf(pstream, "Link 4KB Blocks=%d\n", p->Link_4KB_Blocks);
	if(p->Advanced_Block_Analysis != 0 && p->Advanced_Block_Analysis != defaultoptions.Advanced_Block_Analysis)
		fprintf(pstream, "Advanced Block Analysis=%d\n", p->Advanced_Block_Analysis);
	if(p->Assume_32bit != 0 && p->Assume_32bit != defaultoptions.Assume_32bit)
		fprintf(pstream, "Assume 32bit=%d\n", p->Assume_32bit);
	if(p->Use_HLE != 0 && p->Use_HLE != defaultoptions.Use_HLE) fprintf(pstream, "HLE=%d\n", p->Use_HLE);
	fprintf(pstream, "\n\n");

	return TRUE;
}

/*
 =======================================================================================================================
    This function is called to read 1964.ini file and generate the ini_entry £
    array
 =======================================================================================================================
 */
BOOL ReadAllIniEntries(FILE *pstream)
{
	/*~~~~~~~~~~~~~~~~~~*/
	INI_ENTRY	tempentry;
	/*~~~~~~~~~~~~~~~~~~*/

	while(feof(pstream) == 0)
	{
		if(ReadIniEntry(pstream, &tempentry))
		{
			AddIniEntry(&tempentry);
		}
	}

	return TRUE;
}

/*
 =======================================================================================================================
    This function is called to write/create 1964.ini file
 =======================================================================================================================
 */
BOOL WriteAllIniEntries(FILE *pstream)
{
	/*~~~~~~~~~~~~~~*/
	register int	i;
	/*~~~~~~~~~~~~~~*/

	for(i = 0; i < ini_entry_count; i++)
	{
		WriteIniEntry(pstream, ini_entries[i]);
	}

	return TRUE;
}

/*
 =======================================================================================================================
    This function will destroy the entries in the ini_entries array. Memory for £
    each entry is dynamatic allocated, need to free the memorys before program £
    exit.
 =======================================================================================================================
 */
void DeleteAllIniEntries(void)
{
	/*~~~~~~~~~~~~~~*/
	register int	i;
	/*~~~~~~~~~~~~~~*/

	for(i = 0; i < ini_entry_count; i++)
	{
		/* VirtualFree((void*)ini_entries[i], sizeof(INI_ENTRY), MEM_DECOMMIT); */
		VirtualFree((void *) ini_entries[i], 0, MEM_RELEASE);
		ini_entries[i] = NULL;
	}

	ini_entry_count = 0;
}

/*
 =======================================================================================================================
    Copy content of INI_ENTRY from src to dest.
 =======================================================================================================================
 */
void CopyIniEntry(INI_ENTRY *dest, const INI_ENTRY *src)
{
	strcpy(dest->Game_Name, src->Game_Name);
	strcpy(dest->Comments, src->Comments);
	strcpy(dest->Alt_Title, src->Alt_Title);
	dest->crc1 = src->crc1;
	dest->crc2 = src->crc2;
	dest->countrycode = src->countrycode;
	dest->RDRAM_Size = src->RDRAM_Size;
	dest->Emulator = src->Emulator;
	dest->Code_Check = src->Code_Check;
	dest->Save_Type = src->Save_Type;
	dest->Max_FPS = src->Max_FPS;
	dest->Use_TLB = src->Use_TLB;
	dest->Eeprom_size = src->Eeprom_size;
	dest->Counter_Factor = src->Counter_Factor;
	dest->Use_Register_Caching = src->Use_Register_Caching;
	dest->FPU_Hack = src->FPU_Hack;
	dest->DMA_Segmentation = src->DMA_Segmentation;
	dest->Link_4KB_Blocks = src->Link_4KB_Blocks;
	dest->Advanced_Block_Analysis = src->Advanced_Block_Analysis;
	dest->Assume_32bit = src->Assume_32bit;
	dest->Use_HLE = src->Use_HLE;
}

/*
 =======================================================================================================================
    I should not need to write such a stupid function to convert String to Int £
    However, the sscanf() function does not work for me to input hex number from £
    input string. I spent some time to debug it, no use, so I wrote £
    this function to do the converting myself. £
    Someone could help me to elimiate this funciton
 =======================================================================================================================
 */
uint32 ConvertHexCharToInt(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	else if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else
		return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 ConvertHexStringToInt(const char *str, int nchars)
{
	/*~~~~~~~~~~~~~~~*/
	int		i;
	uint32	result = 0;
	/*~~~~~~~~~~~~~~~*/

	for(i = 0; i < nchars; i++) result = result * 16 + ConvertHexCharToInt(str[i]);

	return result;
}

/*
 =======================================================================================================================
    Delete the ctrl-m, ctrl-r characters at the end of string
 =======================================================================================================================
 */
void chopm(char *str)
{
	/*~~*/
	int i;
	/*~~*/

	i = strlen(str);
	while(str[i] < ' ') /* is a ctrl character */
	{
		str[i] = '\0';
		i--;
	}
}
