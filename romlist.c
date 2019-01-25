/*$T romlist.c GC 1.136 03/09/02 17:32:23 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Routines for populating and sorting the romlist browser
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
#include "romlist.h"
#include "fileio.h"
#include "cheatcode.h"
#include "win32/wingui.h"
#include "win32/windebug.h"
#include "debug_option.h"
#include "memory.h"
#include "kaillera/kaillera.h"
#include "emulator.h"

/* Global variabls */
ROMLIST_ENTRY	*romlist[MAX_ROMLIST];
int				romlist_count = 0;
int				romlist_sort_method = ROMLIST_GAMENAME;
int				romlistNameToDisplay = ROMLIST_DISPLAY_FILENAME;
int				selected_rom_index;
static char		savedrompath[_MAX_PATH];
int				romListHeaderClickedColumn = 0;
void			RomListGetGoodRomNameToDisplay(char *buf, int index);

ColumnType romListColumns[] =
{
	{ROMLIST_COL_GAMENAME,	"Game Name",			0,	TRUE,	180,	0},	//The first column is always the filename, and always enabled
	{ROMLIST_COL_COUNTRY,	"Country",				1,	TRUE,	50,		0},
	{ROMLIST_COL_SIZE,		"Size",					2,	TRUE,	50,		0},
	{ROMLIST_COL_STATUS,	"Compatibility Status",	3,	TRUE,	360,	0},
	{ROMLIST_COL_GAMESAVE,	"Game Save",			4,	FALSE,	30,		0},
	{ROMLIST_COL_CICCHIP,	"CIC Chip",				5,	FALSE,	30,		0},
	{ROMLIST_COL_CRC1,		"CRC1",					6,	FALSE,	30,		0},
	{ROMLIST_COL_CRC2,		"CRC2",					7,	FALSE,	30,		0},
};

const int numberOfRomListColumns = sizeof(romListColumns)/sizeof(ColumnType);
ColumnID GetColumnIDByName(char *name);
ColumnID GetColumnIDByPos(int pos);
void InsertColumnsIntoTreeView(HWND hwndTV);
BOOL ColumnMoveUp(HTREEITEM selected);
BOOL ColumnMoveDown(HTREEITEM selected);


/*
 =======================================================================================================================
    Functions
 =======================================================================================================================
 */
BOOL RomListReadDirectory(const char *spath)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	MSG				msg;
	char			romfilename[_MAX_PATH];
	char			drive[_MAX_DRIVE], dir[_MAX_DIR];
	char			filename[_MAX_FNAME], ext[_MAX_EXT];
	char			searchpath[_MAX_PATH];
	char			path[_MAX_PATH];
	HANDLE			findfirst;
	WIN32_FIND_DATA libaa;
	long			filesize;
	INI_ENTRY		entry;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	entry.Code_Check = 0;
	entry.Comments[0] = '\0';
	entry.Alt_Title[0] = '\0';
	entry.countrycode = 0;
	entry.crc1 = 0;
	entry.crc2 = 0;
	entry.Emulator = 0;
	entry.Game_Name[0] = '\0';
	entry.Max_FPS = 0;
	entry.RDRAM_Size = 0;
	entry.Save_Type = 0;
	entry.Use_TLB = 0;
	entry.Eeprom_size = 0;
	entry.Counter_Factor = 0;
	entry.Use_Register_Caching = 0;
	entry.FPU_Hack = 0;
	entry.DMA_Segmentation = 0;
	entry.Link_4KB_Blocks = 0;
	entry.Assume_32bit = 0;
	entry.Use_HLE = 0;
	entry.Advanced_Block_Analysis = 0;

	strcpy(directories.last_rom_directory, spath);

	strcpy(savedrompath, spath);
	strcpy(path, spath);
	if(path[strlen(path) - 1] != '\\') strcat(path, "\\");

	strcpy(searchpath, path);
	strcat(searchpath, "*.*");

	findfirst = FindFirstFile(searchpath, &libaa);
	if(findfirst == INVALID_HANDLE_VALUE)
	{
		/* No file in the rom directory */
		return(FALSE);
	}

	SetStatusBarText(0, "Updating ROM browser...");
	do
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if(GetMessage(&msg, NULL, 0, 0))
			{
				if(!TranslateAccelerator(gui.hwnd1964main, gui.hAccTable, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

		strcpy(romfilename, path);
		strcat(romfilename, libaa.cFileName);
		_splitpath(romfilename, drive, dir, filename, ext);
		_strlwr(ext);	/* Convert file extension to lower case */

		/*
		 * DisplayError("Fullname=%s, drive=%s, dir=%s, filename=%s, ext=%s", romfilename,
		 * drive, dir, filename, ext);
		 */
		if
		(
			stricmp(ext, ".rom") == 0
		||	stricmp(ext, ".v64") == 0
		||	stricmp(ext, ".z64") == 0
		||	stricmp(ext, ".usa") == 0
		||	stricmp(ext, ".n64") == 0
		||	stricmp(ext, ".bin") == 0
		||	stricmp(ext, ".zip") == 0
		||	stricmp(ext, ".j64") == 0
		||	stricmp(ext, ".pal") == 0
		)
		{
			if(strcmp(ext, ".zip") == 0)
			{
				/* Open and read this zip file */
				if((filesize = ReadZippedRomHeader(romfilename, &entry)) == 0)
				{
					/* This is not a ROM zip file, skipped it */
					continue;
				}
			}
			else
			{
				/* Open and read this rom file */
				if((filesize = ReadRomHeader(romfilename, &entry)) == 0)
				{
					/* This is not a ROM file, skipped it */
					continue;
				}
			}

			/* Add the header information to our romlist */
			{
				/*~~~~~~~~~~~~~~~~~~*/
				int ini_entries_index;
				/*~~~~~~~~~~~~~~~~~~*/

				if((ini_entries_index = FindIniEntry2(&entry)) >= 0 || (ini_entries_index = AddIniEntry(&entry)) >= 0)
				{
					/* Add the romlist */
					strcat(filename, ext);
					RomListAddEntry(&entry, romfilename, filesize);
				}
				else
				{
					/*
					 * Cannot add to ini_entries list for some reason £
					 * Skipped
					 */
					continue;
				}
			}
		}
		else
		{
			continue;	/* Skip this file */
		}
	} while(FindNextFile(findfirst, &libaa));
	selected_rom_index = 0;
	NewRomList_Sort();
	return TRUE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ClearRomList(void)
{
	/*~~~~~~~~~~~~~~*/
	register int	i;
	/*~~~~~~~~~~~~~~*/

	for(i = 0; i < romlist_count; i++)
	{
		/* VirtualFree((void*)romlist[i], sizeof(ROMLIST_ENTRY), MEM_DECOMMIT); */
		VirtualFree((void *) romlist[i], 0, MEM_RELEASE);
		romlist[i] = NULL;
	}

	romlist_count = 0;
}

/*
 =======================================================================================================================
    Init the whole list, this function must be call before any other ROMLIST functions
 =======================================================================================================================
 */
void InitRomList(void)
{
	/*~~~~~~~~~~~~~~*/
	register int	i;
	/*~~~~~~~~~~~~~~*/

	for(i = 0; i < MAX_ROMLIST; i++) romlist[i] = NULL;
	romlist_count = 0;
}

/*
 =======================================================================================================================
    Return value is the index of the new entry that is added into the list
 =======================================================================================================================
 */
int RomListAddEntry(INI_ENTRY *newentry, char *romfilename, long filesize)
{
	/*~~~~~~*/
	int index;
	/*~~~~~~*/

	if(romlist_count == MAX_ROMLIST)
	{
		DisplayError("Your directory contains too many roms, I cannot display it");
		return -1;
	}

	if((index = FindIniEntry2(newentry)) >= 0 || (index = AddIniEntry(newentry)) >= 0)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		/*
		 * We can either locate the entry in the ini_entries list £
		 * or this is a new entry, never in the ini_entries list, but we have £
		 * successfully add it into the ini_entries list £
		 * Allocate memory for a new ROMLIST_ENTRY
		 */
		ROMLIST_ENTRY	*pnewentry = NULL;
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		pnewentry = (ROMLIST_ENTRY *) VirtualAlloc(NULL, sizeof(ROMLIST_ENTRY), MEM_COMMIT, PAGE_READWRITE);
		if(pnewentry == NULL)	/* fail to allocate memory */
			return -1;

		pnewentry->pinientry = ini_entries[index];
		strcpy(pnewentry->romfilename, romfilename);
		pnewentry->size = filesize;

		/* Insert the new entry sorted */
		if(romlist_count == 0)
		{
			selected_rom_index = romlist_count;
			romlist[romlist_count++] = pnewentry;
			return selected_rom_index;
		}
		else
		{
			/*~~*/
			int i;
			/*~~*/

			for(i = 0; i < romlist_count; i++)
			{
				if(stricmp(romlist[i]->pinientry->Game_Name, pnewentry->pinientry->Game_Name) >= 0) break;
			}

			selected_rom_index = i;
			if(selected_rom_index < romlist_count)
			{
				for(i = romlist_count; i > selected_rom_index; i--) romlist[i] = romlist[i - 1];
			}

			romlist[selected_rom_index] = pnewentry;
			romlist_count++;
			return selected_rom_index;
		}
	}
	else
	{
		return -1;
	}
}

extern void RefreshRecentGameMenus(char *filename);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RomListOpenRom(int index, BOOL RunThisRom)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	filename[_MAX_PATH];
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(index >= 0 && index < romlist_count)
	{
		selected_rom_index = index;
		strcpy(filename, romlist[index]->romfilename);

		if(WinLoadRomStep2(filename) == TRUE)
		{
			RefreshRecentGameMenus(filename);

			/* Read hack code for this rom */
			CodeList_ReadCode(rominfo.name);

			EnableRadioButtons(TRUE);
			EnableMenuItem(gui.hMenu1964main, ID_ROM_PAUSE, MF_GRAYED);
			EnableMenuItem(gui.hMenu1964main, ID_FILE_ROMINFO, MF_ENABLED);
			EnableButton(ID_BUTTON_ROM_PROPERTIES, TRUE);
			EnableMenuItem(gui.hMenu1964main, ID_FILE_CHEAT, MF_ENABLED);


			if( RunThisRom || Kaillera_Is_Running == TRUE)
			{
				Play(emuoptions.auto_full_screen); /* autoplay */
			}
		}
		else
		{
			EnableButton(ID_BUTTON_OPEN_ROM, TRUE);
			EnableMenuItem(gui.hMenu1964main, ID_OPENROM, MF_ENABLED);
			EnableButton(ID_BUTTON_SETUP_PLUGINS, TRUE);
			EnableMenuItem(gui.hMenu1964main, IDM_PLUGINS, MF_ENABLED);

		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RomListSelectRom(int index)
{
	if(index >= 0 && index < romlist_count)
	{
		selected_rom_index = index;
		ListView_SetSelectionMark(gui.hwndRomList, index);
	}
}

int  RomListGetSelectedIndex(void)
{
	return selected_rom_index;
}
/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RomListRomOptions(int index)
{
	if(index >= 0 && index < romlist_count)
	{
		int col, i, size;

		RomListSelectRom(index);
		DialogBox(gui.hInst, "ROM_OPTIONS", gui.hwnd1964main, (DLGPROC) RomListDialog);

		//Looking for the column id of the field "Compatibility Status"
		size = GetColumnIDByName("Compatibility Status");
		col = 0;
		for( i=0; i<size; i++)
		{
			if( romListColumns[i].enabled )
			{
				col++;
			}
		}
		ListView_SetItemText(gui.hwndRomList, index, col, romlist[index]->pinientry->Comments);

		RomListGetGoodRomNameToDisplay(generalmessage, index);
		ListView_SetItemText(gui.hwndRomList, index, 0, generalmessage);
	}
}

#define ROM_OPTION_SET_LISTBOX(CONTROLID, SIZE, DEFAULTVALUE, VALUE, NAMES) \
	SendDlgItemMessage \
	( \
		hDlg, \
		CONTROLID, \
		CB_RESETCONTENT, \
		0, \
		0 \
	); \
	for(i = 1; i < SIZE; i++) \
	{ \
		if(DEFAULTVALUE == i) \
		{ \
			strcpy(generalmessage, NAMES[i]); \
			strcat(generalmessage, " (default)"); \
			SendDlgItemMessage(hDlg, CONTROLID, CB_INSERTSTRING, i - 1, (LPARAM) (generalmessage)); \
		} \
		else \
			SendDlgItemMessage(hDlg, CONTROLID, CB_INSERTSTRING, i - 1, (LPARAM) (NAMES[i])); \
		if(i == VALUE) SendDlgItemMessage(hDlg, CONTROLID, CB_SETCURSEL, i - 1, 0); \
	} \
	if(VALUE == 0) SendDlgItemMessage(hDlg, CONTROLID, CB_SETCURSEL, DEFAULTVALUE - 1, 0);

/*
 =======================================================================================================================
 =======================================================================================================================
 */

LRESULT APIENTRY RomListDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	int		i;
	char	tempstr[_MAX_PATH];
	char	countryname[80];
	int		tvsystem;
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	if(!strcmp(romlist[selected_rom_index]->pinientry->Game_Name, "Perfect Dark") || !strcmp(romlist[selected_rom_index]->pinientry->Game_Name, "GoldenEye X") || strstr(romlist[selected_rom_index]->pinientry->Game_Name, "Perfect") != NULL)
		romlist[selected_rom_index]->pinientry->Eeprom_size = EEPROMSIZE_16KB;

	switch(message)
	{
	case WM_INITDIALOG:
		ROM_OPTION_SET_LISTBOX
		(
			IDC_ROMOPTION_RDRAMSIZE,
			3,
			defaultoptions.RDRAM_Size,
			romlist[selected_rom_index]->pinientry->RDRAM_Size,
			rdram_size_names
		) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_SAVETYPE,
				7,
				defaultoptions.Save_Type,
				romlist[selected_rom_index]->pinientry->Save_Type,
				save_type_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_CPUEMULATOR,
				3,
				defaultoptions.Emulator,
				romlist[selected_rom_index]->pinientry->Emulator,
				emulator_type_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_CODECHECK,
				9,
				defaultoptions.Code_Check,
				romlist[selected_rom_index]->pinientry->Code_Check,
				codecheck_type_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_MAXVISPEED,
				5,
				defaultoptions.Max_FPS,
				romlist[selected_rom_index]->pinientry->Max_FPS,
				maxfps_type_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_USETLB,
				3,
				defaultoptions.Use_TLB,
				romlist[selected_rom_index]->pinientry->Use_TLB,
				usetlb_type_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_EEPROMSIZE,
				4,
				defaultoptions.Eeprom_size,
				romlist[selected_rom_index]->pinientry->Eeprom_size,
				eepromsize_type_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_USEREGC,
				3,
				defaultoptions.Use_Register_Caching,
				romlist[selected_rom_index]->pinientry->Use_Register_Caching,
				register_caching_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_CF,
				9,
				defaultoptions.Counter_Factor,
				romlist[selected_rom_index]->pinientry->Counter_Factor,
				counter_factor_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_FPUHACK,
				3,
				defaultoptions.FPU_Hack,
				romlist[selected_rom_index]->pinientry->FPU_Hack,
				use_fpu_hack_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_DMASEG,
				3,
				defaultoptions.DMA_Segmentation,
				romlist[selected_rom_index]->pinientry->DMA_Segmentation,
				use_dma_segmentation
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_LINK4KB,
				3,
				defaultoptions.Link_4KB_Blocks,
				romlist[selected_rom_index]->pinientry->Link_4KB_Blocks,
				use_4kb_link_block_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_ANALYSIS,
				3,
				defaultoptions.Advanced_Block_Analysis,
				romlist[selected_rom_index]->pinientry->Advanced_Block_Analysis,
				use_block_analysis_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_ASSUME_32BIT,
				3,
				defaultoptions.Assume_32bit,
				romlist[selected_rom_index]->pinientry->Assume_32bit,
				assume_32bit_names
			) ROM_OPTION_SET_LISTBOX
			(
				IDC_ROMOPTION_HLE,
				3,
				defaultoptions.Use_HLE,
				romlist[selected_rom_index]->pinientry->Use_HLE,
				use_HLE_names
			)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			char	drive[_MAX_DIR], dir[_MAX_DIR];
			char	fname[_MAX_FNAME], ext[_MAX_EXT];
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			_splitpath(romlist[selected_rom_index]->romfilename, drive, dir, fname, ext);
			strcat(drive, dir);
			strcat(fname, ext);

			SetDlgItemText(hDlg, IDC_ROMOPTION_FILENAME, fname);
			SetDlgItemText(hDlg, IDC_ROMOPTION_FILELOCATION, drive);
		}

		SetDlgItemText(hDlg, IDC_ROMOPTION_GAMENAME, romlist[selected_rom_index]->pinientry->Game_Name);
		SetDlgItemText(hDlg, IDC_ROMOPTION_COMMENTS, romlist[selected_rom_index]->pinientry->Comments);
		SetDlgItemText(hDlg, IDC_ROMOPTION_ALTTITLE, romlist[selected_rom_index]->pinientry->Alt_Title);

		sprintf(tempstr, "%08X", romlist[selected_rom_index]->pinientry->crc1);
		SetDlgItemText(hDlg, IDC_ROMOPTION_CRC1, tempstr);

		sprintf(tempstr, "%08X", romlist[selected_rom_index]->pinientry->crc2);
		SetDlgItemText(hDlg, IDC_ROMOPTION_CRC2, tempstr);

		CountryCodeToCountryName_and_TVSystem
		(
			romlist[selected_rom_index]->pinientry->countrycode,
			countryname,
			&tvsystem
		);
		sprintf(tempstr, "%s (code=0x%02X)", countryname, romlist[selected_rom_index]->pinientry->countrycode);
		SetDlgItemText(hDlg, IDC_ROMOPTION_COUNTRYCODE, tempstr);

		return(TRUE);

	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			{
				/* Read option setting from dialog */
				romlist[selected_rom_index]->pinientry->RDRAM_Size = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_RDRAMSIZE,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Save_Type = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_SAVETYPE,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Emulator = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_CPUEMULATOR,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Code_Check = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_CODECHECK,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Max_FPS = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_MAXVISPEED,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Use_TLB = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_USETLB,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Eeprom_size = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_EEPROMSIZE,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Use_Register_Caching = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_USEREGC,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Counter_Factor = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_CF,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->FPU_Hack = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_FPUHACK,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->DMA_Segmentation = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_DMASEG,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Link_4KB_Blocks = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_LINK4KB,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Advanced_Block_Analysis = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_ANALYSIS,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Assume_32bit = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_ASSUME_32BIT,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				romlist[selected_rom_index]->pinientry->Use_HLE = SendDlgItemMessage
					(
						hDlg,
						IDC_ROMOPTION_HLE,
						CB_GETCURSEL,
						0,
						0
					) + 1;
				if
				(
					romlist[selected_rom_index]->pinientry->Code_Check != CODE_CHECK_PROTECT_MEMORY
				&&	romlist[selected_rom_index]->pinientry->Code_Check != CODE_CHECK_NONE
				&&	romlist[selected_rom_index]->pinientry->Link_4KB_Blocks == USE4KBLINKBLOCK_YES
				)
				{
					DisplayError("Link 4KB cannot be used when the self-mod code checking method is not PROTECT_MEMORY or No_Check, go to Game Options and set Link_4KB to No.");
					romlist[selected_rom_index]->pinientry->Link_4KB_Blocks = USE4KBLINKBLOCK_NO;
				}

				GetDlgItemText(hDlg, IDC_ROMOPTION_COMMENTS, romlist[selected_rom_index]->pinientry->Comments, 79);
				GetDlgItemText(hDlg, IDC_ROMOPTION_ALTTITLE, romlist[selected_rom_index]->pinientry->Alt_Title, 50);
				FileIO_Write1964Ini();

				EndDialog(hDlg, TRUE);
				return(TRUE);
			}

		case IDCANCEL:
			{
				EndDialog(hDlg, TRUE);
				return(TRUE);
			}
		}
	}

	return(FALSE);
}

/*
 =======================================================================================================================
    Create a rom list entry for the rom loaded in memory
 =======================================================================================================================
 */
void ReadRomHeaderInMemory(INI_ENTRY *ini_entry)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint8	buffer[0x100];
	/*~~~~~~~~~~~~~~~~~~*/

	memcpy(buffer, gMS_ROM_Image, 0x40);

	strncpy(ini_entry->Game_Name, buffer + 0x20, 0x14);
	SwapRomName(ini_entry->Game_Name);

	ini_entry->crc1 = *((uint32 *) (buffer + 0x10));
	ini_entry->crc2 = *((uint32 *) (buffer + 0x14));
	ini_entry->countrycode = buffer[0x3D];

	/* ini_entry->countrycode = buffer[0x3E]; */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RomListSelectLoadedRomEntry(void)
{
	/*~~~~~~~~~~~~~~*/
	int			i;
	INI_ENTRY	entry;
	/*~~~~~~~~~~~~~~*/

	ReadRomHeaderInMemory(&entry);

	for(i = 0; i < romlist_count; i++)
	{
		if( stricmp(romlist[i]->pinientry->Game_Name, entry.Game_Name) == 0 &&
			romlist[i]->pinientry->crc1 == entry.crc1 &&
			romlist[i]->pinientry->crc2 == entry.crc2 &&
			romlist[i]->pinientry->countrycode == entry.countrycode )
		{
			break;
		}
	}

	selected_rom_index = i;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
ROMLIST_ENTRY *RomListSelectedEntry(void)
{
	return romlist[selected_rom_index];
}

/*
 =======================================================================================================================
    DupString - allocates a copy of a string. £
    lpsz - address of the null-terminated string to copy.
 =======================================================================================================================
 */
LPSTR DupString(LPSTR lpsz)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int		cb = lstrlen(lpsz) + 1;
	LPSTR	lpszNew = LocalAlloc(LMEM_FIXED, cb);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(lpszNew != NULL) CopyMemory(lpszNew, lpsz, cb);
	return lpszNew;
}

#define C_COLUMNS	6

typedef struct	myitem_tag
{
	LPSTR	aCols[C_COLUMNS];
} MYITEM;

/*
 =======================================================================================================================
    InitListViewItems - adds items and subitems to a list view. £
    Returns TRUE if successful, or FALSE otherwise. £
    hwndLV - handle to the list view control. £
    pfData - text file containing list view items with columns £
    separated by semicolons.
 =======================================================================================================================
 */
BOOL WINAPI InitListViewItems(HWND hwndLV)
{
	/*~~~~~~~~~~~~~~~~~~~~*/
	int		index, i;
	char	size[20];
	char	countryname[40];
	char	textbuf[100];
	int		tvsystem;
	LVITEM	lvi;
	/*~~~~~~~~~~~~~~~~~~~~*/

	/* Initialize LVITEM members that are common to all items. */
	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;	/* app. maintains text */
	lvi.iImage = 0;						/* image list index */

	/* Read each line in the specified file. */
	for(index = 0; index < romlist_count; index++)
	{
		LVCOLUMN colinfo;
		/* Initialize item-specific LVITEM members. */
		lvi.iItem = index;
		lvi.iSubItem = 0;

		RomListGetGoodRomNameToDisplay(generalmessage, index);
		lvi.pszText = generalmessage;

		ListView_InsertItem(hwndLV, &lvi);

		CountryCodeToCountryName_and_TVSystem(romlist[index]->pinientry->countrycode, countryname, &tvsystem);

		colinfo.mask = LVCF_TEXT;
		colinfo.pszText = textbuf;
		colinfo.cchTextMax = 99;

		for( i=1; i< numberOfRomListColumns; i++)
		{
			__try {
			ListView_GetColumn(hwndLV, i, &colinfo);
			{
				if( stricmp(colinfo.pszText, romListColumns[1].text) == 0 )	//Country name
				{
					ListView_SetItemText(hwndLV, index, i, countryname);
				}
				else if( stricmp(colinfo.pszText, romListColumns[2].text) == 0 ) //Rom Size
				{
					sprintf(size, "%3dM", romlist[index]->size * 8 / 0x100000);
					ListView_SetItemText(hwndLV, index, i, size);
				}
				else if( stricmp(colinfo.pszText, romListColumns[3].text) == 0 ) //Comments
				{
					ListView_SetItemText(hwndLV, index, i, romlist[index]->pinientry->Comments);
				}
				else if( stricmp(colinfo.pszText, romListColumns[4].text) == 0 ) //Game Save Type
				{
					ListView_SetItemText(hwndLV, index, i, save_type_names[romlist[index]->pinientry->Save_Type]);
				}
				else if( stricmp(colinfo.pszText, romListColumns[5].text) == 0 ) //CIC_Chip
				{
				}
				else if( stricmp(colinfo.pszText, romListColumns[6].text) == 0 ) //CRC1
				{
					sprintf(size, "%08X", romlist[index]->pinientry->crc1);
					ListView_SetItemText(hwndLV, index, i, size);
				}
				else if( stricmp(colinfo.pszText, romListColumns[7].text) == 0 ) //CRC2
				{
					sprintf(size, "%08X", romlist[index]->pinientry->crc2);
					ListView_SetItemText(hwndLV, index, i, size);
				}
			}}__except(NULL, EXCEPTION_EXECUTE_HANDLER)
			{
			}

		}
	}
	if(romlist_count > 0) ListView_SetSelectionMark(hwndLV, 0);
	return TRUE;
}

/*
 =======================================================================================================================
    InitListViewColumns - adds columns to a list view control. £
    Returns TRUE if successful, or FALSE otherwise. £
    hwndLV - handle to the list view control.
 =======================================================================================================================
 */
BOOL WINAPI InitListViewColumns(HWND hwndLV, int windowwidth)
{
	/*~~~~~~~~~~~~*/
	int i;
	LVCOLUMNA	lvc;
	/*~~~~~~~~~~~~*/

	/* Initialize the LVCOLUMN structure. */
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;

	for( i=0; i<numberOfRomListColumns; i++ )
	{
		if( romListColumns[i].enabled )
		{
			if( romListColumns[i].colWidth > 500 || romListColumns[i].colWidth < 0 )
			{
				romListColumns[i].colWidth = 50;
			}
			lvc.cx = romListColumns[i].colWidth;
			lvc.pszText = romListColumns[i].text;
			lvc.iSubItem = i;
			ListView_InsertColumn(hwndLV, i, &lvc);
		}
	}
	return TRUE;
}

HWND WINAPI CreateRebar(HWND hwndOwner)
{
   REBARINFO     rbi;
   REBARBANDINFO rbBand;
   HWND   hwndRB;
   DWORD  dwBtnSize;
   INITCOMMONCONTROLSEX icex;
   
   icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
   icex.dwICC   = ICC_COOL_CLASSES|ICC_BAR_CLASSES;
   InitCommonControlsEx(&icex);
   hwndRB = CreateWindowEx(WS_EX_TOOLWINDOW,
                           REBARCLASSNAME,
                           NULL,
                           WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|
                           WS_CLIPCHILDREN|RBS_VARHEIGHT|
                           CCS_NODIVIDER,
                           0,0,0,0,
                           gui.hwnd1964main,
                           NULL,
                           gui.hInst,
                           NULL);
   if(!hwndRB)
      return NULL;
   // Initialize and send the REBARINFO structure.
   rbi.cbSize = sizeof(REBARINFO);  // Required when using this
                                    // structure.
   rbi.fMask  = 0;
   rbi.himl   = (HIMAGELIST)NULL;
   if(!SendMessage(hwndRB, RB_SETBARINFO, 0, (LPARAM)&rbi))
      return NULL;
   // Initialize structure members that both bands will share.
   rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
   rbBand.fMask  = RBBIM_COLORS | RBBIM_TEXT /*| RBBIM_BACKGROUND */| 
                   RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | 
                   RBBIM_SIZE;
   rbBand.fStyle = RBBS_CHILDEDGE;
 
   // Get the height of the toolbar.
   dwBtnSize = SendMessage(gui.hToolBar, TB_GETBUTTONSIZE, 0,0);

   // Set values unique to the band with the toolbar.
   rbBand.lpText     = "";
   rbBand.hwndChild  = gui.hToolBar;
   rbBand.cxMinChild = 0;
   rbBand.cyMinChild = HIWORD(dwBtnSize);
   rbBand.cx         = 250;
	rbBand.clrFore = GetSysColor(COLOR_BTNTEXT);
	rbBand.clrBack = GetSysColor(COLOR_BTNFACE);
   // Add the band that has the toolbar.
   SendMessage(hwndRB, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
   return (hwndRB);
}

void EnableRadioButtons(BOOL bEnable)
{
	EnableButton(ID_BUTTON_PLAY, bEnable);
	CheckButton(ID_BUTTON_PLAY, FALSE);
	EnableButton(ID_BUTTON_PAUSE, bEnable);
	CheckButton(ID_BUTTON_PAUSE, FALSE);
	EnableButton(ID_BUTTON_STOP, bEnable);
	CheckButton(ID_BUTTON_STOP, FALSE);

	//Enable or disable buttons and menus that open a web browser
	EnableButton(ID_BUTTON_HELP, bEnable^1);
	EnableButton(ID_BUTTON_OPEN_ROM, bEnable^1);
	if (bEnable == TRUE)
	{
		EnableMenuItem(gui.hMenu1964main, ID_CHECKWEB, MF_GRAYED);
		EnableMenuItem(gui.hMenu1964main, ID_OPENROM, MF_GRAYED);
		EnableMenuItem(gui.hMenu1964main, ID_ONLINE_HELP, MF_GRAYED);
	}
	else
	{
		EnableMenuItem(gui.hMenu1964main, ID_CHECKWEB, MF_ENABLED);
		EnableMenuItem(gui.hMenu1964main, ID_OPENROM, MF_ENABLED);
		EnableMenuItem(gui.hMenu1964main, ID_ONLINE_HELP, MF_ENABLED);
	}
}

BYTE ChangeButtonState(int nID)
{
	TBBUTTONINFO ButtonInfo;
	int temp;

	ButtonInfo.cbSize = sizeof(TBBUTTONINFO);
	ButtonInfo.dwMask = TBIF_STATE;

	SendMessage(gui.hToolBar, TB_GETBUTTONINFO, nID,
	(LPARAM)(LPTBBUTTONINFO) &ButtonInfo);

	ButtonInfo.fsState ^= TBSTATE_CHECKED;
	temp = ButtonInfo.fsState;

	SendMessage(gui.hToolBar, TB_SETBUTTONINFO, nID,
	(LPARAM)(LPTBBUTTONINFO) &ButtonInfo);

	return (temp);
}

void CheckButton(int nID, BOOL bCheck)
{
	TBBUTTONINFO ButtonInfo;

	ButtonInfo.cbSize = sizeof(TBBUTTONINFO);
	ButtonInfo.dwMask = TBIF_STATE;

	SendMessage(gui.hToolBar, TB_GETBUTTONINFO, nID,
	(LPARAM)(LPTBBUTTONINFO) &ButtonInfo);

	if (bCheck)
		ButtonInfo.fsState |= TBSTATE_CHECKED;
	else
		ButtonInfo.fsState &= ~TBSTATE_CHECKED;

	SendMessage(gui.hToolBar, TB_SETBUTTONINFO, nID,
	(LPARAM)(LPTBBUTTONINFO) &ButtonInfo);
}

void EnableButton(int nID, BOOL bEnable)
{
	TBBUTTONINFO ButtonInfo;

	ButtonInfo.cbSize = sizeof(TBBUTTONINFO);
	ButtonInfo.dwMask = TBIF_STATE;

	SendMessage(gui.hToolBar, TB_GETBUTTONINFO, nID,
	(LPARAM)(LPTBBUTTONINFO) &ButtonInfo);

	if (bEnable)
		ButtonInfo.fsState |= TBSTATE_ENABLED;
	else
	{
		ButtonInfo.fsState &= ~TBSTATE_CHECKED;
		ButtonInfo.fsState &= ~TBSTATE_ENABLED;
	}

	SendMessage(gui.hToolBar, TB_SETBUTTONINFO, nID,
	(LPARAM)(LPTBBUTTONINFO) &ButtonInfo);
}


void SetupToolBar()
{
		TBADDBITMAP tbab;
         TBBUTTON tbb[11];

      	gui.hToolBar = CreateWindowEx
		(	
			0, 
			TOOLBARCLASSNAME, 
			NULL,
		    WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS
			| WS_CLIPCHILDREN |
			WS_CLIPSIBLINGS | CCS_NODIVIDER,
			0, 
			0, 
			0, 
			0,
	        gui.hwnd1964main, 
			(HMENU)IDR_TOOLBAR1, 
			(HINSTANCE) gui.hInst, 
			NULL
		);

		 // Send the TB_BUTTONSTRUCTSIZE message, which is required for
         // backward compatibility.
         SendMessage(gui.hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

         tbab.hInst = gui.hInst;
         tbab.nID = IDR_TOOLBAR1;
         SendMessage(gui.hToolBar, TB_ADDBITMAP, 4, (LPARAM)&tbab);

         ZeroMemory(tbb, sizeof(tbb));
         
     	 tbb[0].iBitmap = 0;
         tbb[0].fsState = TBSTATE_ENABLED;
         tbb[0].fsStyle = TBSTYLE_BUTTON;
         tbb[0].idCommand = ID_BUTTON_OPEN_ROM;

         tbb[1].fsStyle = TBSTYLE_SEP;

		 tbb[2].iBitmap = 3;
         tbb[2].fsState = TBSTATE_ENABLED;
         tbb[2].fsStyle = TBSTYLE_CHECKGROUP;
         tbb[2].idCommand = ID_BUTTON_PLAY;

		 tbb[3].iBitmap = 4;
         tbb[3].fsState = TBSTATE_ENABLED;
         tbb[3].fsStyle = TBSTYLE_CHECKGROUP;
         tbb[3].idCommand = ID_BUTTON_PAUSE;

		 tbb[4].iBitmap = 5;
         tbb[4].fsState = TBSTATE_ENABLED;
         tbb[4].fsStyle = TBSTYLE_BUTTON; 
         tbb[4].idCommand = ID_BUTTON_STOP;

		 tbb[5].fsStyle = TBSTYLE_SEP;
		 
         tbb[6].iBitmap = 6;
         tbb[6].fsState = TBSTATE_ENABLED;
         tbb[6].fsStyle = TBSTYLE_BUTTON;
         tbb[6].idCommand = ID_BUTTON_SETUP_PLUGINS;

         tbb[7].iBitmap = 2;
         tbb[7].fsState = 0;
         tbb[7].fsStyle = TBSTYLE_BUTTON;
         tbb[7].idCommand = ID_BUTTON_ROM_PROPERTIES;
		 
         tbb[8].iBitmap = 7;
         tbb[8].fsState = TBSTATE_ENABLED;
         tbb[8].fsStyle = TBSTYLE_BUTTON;
         tbb[8].idCommand = ID_BUTTON_HELP;

		 tbb[9].fsStyle = TBSTYLE_SEP;

		 tbb[10].iBitmap = 1;
         tbb[10].fsState = TBSTATE_ENABLED;
         tbb[10].fsStyle = TBSTYLE_BUTTON;
         tbb[10].idCommand = ID_BUTTON_FULL_SCREEN;

		 SendMessage(gui.hToolBar, TB_ADDBUTTONS, 11, (LPARAM)&tbb);
		 gui.hReBar = CreateRebar(gui.hwnd1964main);
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
HWND NewRomList_CreateListViewControl(HWND hwndParent)
{
	/*~~~~~~~~~~~~~*/
	HWND	hwndLV;
	RECT	rcParent;

	/*~~~~~~~~~~~~~*/

	if(!guioptions.display_romlist) return NULL;

	/*
	 * Ensure that the common control DLL is loaded, and then create £
	 * the header control.
	 */
	InitCommonControls();
	GetClientRect(hwndParent, &rcParent);

	if(gui.hStatusBar != NULL)
	{
		/*~~~~~~~~~~~~~~~~*/
		RECT	rcStatusBar;
		/*~~~~~~~~~~~~~~~~*/

		GetWindowRect(gui.hStatusBar, &rcStatusBar);
		rcParent.bottom -= (rcStatusBar.bottom - rcStatusBar.top - 1);
	}

	if(gui.hToolBar != NULL)
	{
		/*~~~~~~~~~~~~~~~~*/
		RECT	rcToolBar;
		/*~~~~~~~~~~~~~~~~*/

		GetWindowRect(gui.hToolBar, &rcToolBar);
		rcParent.top += (rcToolBar.bottom - rcToolBar.top - 1);
		rcParent.bottom -= (rcToolBar.bottom - rcToolBar.top - 1);
	}

	/* Create the list view window. */
	hwndLV = CreateWindow
		(
			WC_LISTVIEW,
			"",
			WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
			0,
			rcParent.top,
			rcParent.right,
			rcParent.bottom,
			hwndParent,
			NULL,
			gui.hInst,
			NULL
		);

	if(hwndLV == NULL)
	{
		DisplayError("Error to create listview");
		return NULL;
	}

	InitListViewColumns(hwndLV, rcParent.right);
	InitListViewItems(hwndLV);

	ListView_SetExtendedListViewStyle(hwndLV, LVS_EX_FULLROWSELECT);	/* | LVS_EX_TRACKSELECT ); */

	/* ListView_SetHoverTime(hwndLV, 3000); */
	ShowWindow(hwndLV, SW_SHOW);
	UpdateWindow(hwndLV);
	if(romlist_count > 0) ListView_SetSelectionMark(hwndLV, 0);

	return hwndLV;	/* return the control's handle */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void NewRomList_ListViewHideHeader(HWND hwnd)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	DWORD	dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	SetWindowLong(hwnd, GWL_STYLE, dwStyle | LVS_NOCOLUMNHEADER);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void NewRomList_ListViewShowHeader(HWND hwnd)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	DWORD	dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	SetWindowLong(hwnd, GWL_STYLE, dwStyle &~LVS_NOCOLUMNHEADER);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void NewRomList_ListViewFreshRomList(void)
{
	ListView_DeleteAllItems(gui.hwndRomList);
	NewRomList_Sort();
	InitListViewItems(gui.hwndRomList);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void NewRomList_ListViewChangeWindowRect(void)
{
	RomListRememberColumnWidth();
	ListView_DeleteAllItems(gui.hwndRomList);
	DestroyWindow(gui.hwndRomList);
	gui.hwndRomList = NewRomList_CreateListViewControl(gui.hwnd1964main);
}

/*
 =======================================================================================================================
    Sort the rom list according to parameter romlist_sort_method
 =======================================================================================================================
 */
BOOL InternalNameIsValid(char *name)
{
	/*~~~~~~~~~~~~~~~~~~~~~*/
	BOOL	valid = FALSE;
	int		i;
	int		n = strlen(name);
	/*~~~~~~~~~~~~~~~~~~~~~*/

	if(n > 20) n = 20;
	for(i = 0; i < n; i++)
	{
		if((unsigned char) (name[i]) > 0x7F)
		{
			valid = FALSE;
			break;
		}
		else if(name[i] != ' ')
			valid = TRUE;
	}

	return valid;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void NewRomList_Sort()
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	struct ROMLIST_ENTRY_STRUCT *temp;
	int							i, j;
	BOOL						needswap;
	char						cname1[10], cname2[10];
	char						gamename1[200], gamename2[200];
	int							tv1, tv2;
	int							retval;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	TRACE1("Sort rom list: %d", romlist_count);

	for(i = 0; i < romlist_count - 1; i++)
	{
		RomListGetGoodRomNameToDisplay(gamename1, i);
		CountryCodeToCountryName_and_TVSystem(romlist[i]->pinientry->countrycode, cname1, &tv1);
		for(j = i + 1; j < romlist_count; j++)
		{
			RomListGetGoodRomNameToDisplay(gamename2, j);
			needswap = FALSE;
			switch(romlist_sort_method % 4)
			{
			case ROMLIST_GAMENAME:
				if(stricmp(gamename1, gamename2) >= 0) needswap = TRUE;
				break;
			case ROMLIST_COUNTRY:
				CountryCodeToCountryName_and_TVSystem(romlist[j]->pinientry->countrycode, cname2, &tv2);
				retval = stricmp(cname1, cname2);
				if( retval > 0)
				{
					needswap = TRUE;
				}
				else if( retval == 0 )
				{
					if(stricmp(gamename1, gamename2) >= 0)
					{
						needswap = TRUE;
					}
				}
				break;
			case ROMLIST_SIZE:
				if(romlist[i]->size > romlist[j]->size)
				{
					needswap = TRUE;
				}
				else if( romlist[i]->size == romlist[j]->size )
				{
					if(stricmp(gamename1, gamename2) >= 0)
					{
						needswap = TRUE;
					}
				}
				break;
			case ROMLIST_COMMENT:
				retval = stricmp(romlist[i]->pinientry->Comments, romlist[j]->pinientry->Comments);

				if( (retval > 0 && romlist_sort_method < 4 ) || (retval < 0 && romlist_sort_method > 4) )
				{
					needswap = TRUE;
				}
				else
				{
					if(stricmp(gamename1, gamename2) >= 0)
					{
						;//needswap = TRUE;
					}
				}
				break;
			}

			if(romlist_sort_method >= 4 && romlist_sort_method%4 != 3 )
			{
				needswap = 1 - needswap;
			}

			if(needswap)
			{
				temp = romlist[i];
				romlist[i] = romlist[j];
				romlist[j] = temp;
				RomListGetGoodRomNameToDisplay(gamename1, i);
				CountryCodeToCountryName_and_TVSystem(romlist[i]->pinientry->countrycode, cname1, &tv1);
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RomListSaveCurrentPos(void)
{
	RomListRememberColumnWidth();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RomListUseSavedPos(void)
{
	/*~~~~~~~~~*/
	RECT	rect;
	/*~~~~~~~~~*/

	/* int perpage = ListView_GetCountPerPage(); */
	ListView_GetItemRect(gui.hwndRomList, selected_rom_index - 1, &rect, LVIR_LABEL);
	ListView_Scroll(gui.hwndRomList, 0, rect.top);

	/*
	 * ListView_SetItemState(gui.hwndRomList, selected_rom_index,
	 * LVIS_FOCUSED|LVIS_SELECTED, LVIS_STATEIMAGEMASK );
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RomListRememberColumnWidth(void)
{
	/*~~*/
	int i;
	char	textbuf[100];
	LVCOLUMN colinfo;
	/*~~*/
	colinfo.mask = LVCF_TEXT;
	colinfo.pszText = textbuf;
	colinfo.cchTextMax = 99;
	
	for( i=1; i< numberOfRomListColumns; i++)
	{
		if( ListView_GetColumn(gui.hwndRomList, i, &colinfo) )
		{
			int col=GetColumnIDByName(colinfo.pszText);
			romListColumns[col].colWidth = ListView_GetColumnWidth(gui.hwndRomList, i);
			if( romListColumns[col].colWidth <= 0 )
			{
				romListColumns[col].colWidth = 50;
			}
		}
	}
}



/*
 =======================================================================================================================
 =======================================================================================================================
 */
ROMLIST_ENTRY *RomListGet_Selected_Entry(void)
{
	return romlist[selected_rom_index];
}

/*
 =======================================================================================================================
    Sort the rom list according to parameter romlist_sort_method
 =======================================================================================================================
 */
void ConvertInvalidInternalName(char *oldname, char *newname)
{
	/*~~~~~~~~~~~~~~~~~~~~*/
	int i;
	int n = strlen(oldname);
	/*~~~~~~~~~~~~~~~~~~~~*/

	if(n > 20) n = 20;
	for(i = 0; i < n; i++)
	{
		if((unsigned char) (oldname[i]) > 0x7F)
		{
			newname[i] = oldname[i] - 0x7F;
			if(newname[i] < 0x20) newname[i] += 0x20;
		}
		else
		{
			newname[i] = oldname[i];
		}
	}

	newname[i] = '\0';
}

long OnNotifyRomList(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(!emustatus.Emu_Is_Running)
	{
		int itemNo = ((LPNMLISTVIEW) lParam)->iItem;
		
		switch(((LPNMHDR) lParam)->code)
		{
		case NM_DBLCLK:
			EnableButton(ID_BUTTON_OPEN_ROM, FALSE);
			EnableMenuItem(gui.hMenu1964main, ID_OPENROM, MF_GRAYED);
			EnableButton(ID_BUTTON_SETUP_PLUGINS, FALSE);
			EnableMenuItem(gui.hMenu1964main, IDM_PLUGINS, MF_GRAYED);

			RomListSelectRom(((LPNMLISTVIEW) lParam)->iItem);
			RomListOpenRom(((LPNMLISTVIEW) lParam)->iItem, 1);
			break;
		case NM_RCLICK:
			if( itemNo >= 0 )
			{
				RECT pos;
				HMENU popupmainmenu, popupmenu;
				int x = ((LPNMLISTVIEW) lParam)->ptAction.x;
				int y = ((LPNMLISTVIEW) lParam)->ptAction.y;
				RomListSelectRom(((LPNMLISTVIEW) lParam)->iItem);
				EnableMenuItem(gui.hMenu1964main, ID_FILE_ROMINFO, MF_ENABLED);
				EnableButton(ID_BUTTON_ROM_PROPERTIES, TRUE);
				EnableMenuItem(gui.hMenu1964main, ID_FILE_CHEAT, MF_ENABLED);
				
				GetWindowRect(gui.hwndRomList, &pos);
				x+=pos.left;
				y+=pos.top;
				
				//popupmenu = GetSubMenu(gui.hMenu1964main, 2);
				popupmainmenu = LoadMenu(gui.hInst, "ROM_POPUP_MENU");
				popupmenu = GetSubMenu(popupmainmenu,0);
				TrackPopupMenuEx(popupmenu, TPM_VERTICAL, x, y, gui.hwnd1964main, NULL);
				DestroyMenu(popupmenu);
			}
			break;
		case NM_CLICK:
			RomListSelectRom(((LPNMLISTVIEW) lParam)->iItem);
			EnableMenuItem(gui.hMenu1964main, ID_FILE_ROMINFO, MF_ENABLED);
			EnableButton(ID_BUTTON_ROM_PROPERTIES, TRUE);
			EnableMenuItem(gui.hMenu1964main, ID_FILE_CHEAT, MF_ENABLED);
			break;
		case LVN_COLUMNCLICK:
			{
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
				LPNMLISTVIEW	p = (LPNMLISTVIEW) lParam;
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
				
				if(p->iItem == -1 && p->uOldState == 0)
				{
					if(romlist_sort_method != p->iSubItem)
					{
						romlist_sort_method = p->iSubItem;
					}
					else
					{
						romlist_sort_method += 4;
					}
					romlist_sort_method %= 8;
					NewRomList_Sort();
					NewRomList_ListViewFreshRomList();
				}
			}
			break;
		default:
			return(DefWindowProc(hWnd, message, wParam, lParam));
			break;
		}
	}
	
	return(0l);
}

long OnNotifyRomListHeader(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hWndHeader = ListView_GetHeader(gui.hwndRomList);
	switch(((LPNMHDR) lParam)->code)
	{
	case NM_RCLICK:
		{
			HMENU popupmainmenu, popupmenu;
			POINTS pts;
			POINT point;
			RECT rect;
			DWORD pos = GetMessagePos();
			pts = MAKEPOINTS(pos);
			point.x = pts.x;
			point.y = pts.y;
			ScreenToClient(hWndHeader, &point);
			popupmainmenu = LoadMenu(gui.hInst, "ROM_POPUP_MENU");
			
			for( romListHeaderClickedColumn=0; romListHeaderClickedColumn<numberOfRomListColumns; romListHeaderClickedColumn++)
			{
				Header_GetItemRect(hWndHeader, romListHeaderClickedColumn, &rect);
				if( PtInRect(&rect, point) )
				{
					break;
				}
			}
			
			if( romListHeaderClickedColumn == 0 )
			{
				popupmenu = GetSubMenu(popupmainmenu,1);
				switch( romlistNameToDisplay )
				{
				case ROMLIST_DISPLAY_INTERNAL_NAME:
					CheckMenuItem(popupmenu, ID_HEADERPOPUP_SHOW_INTERNAL_NAME, MF_CHECKED);
					break;
				case ROMLIST_DISPLAY_ALTER_NAME:
					CheckMenuItem(popupmenu, ID_HEADERPOPUP_SHOWALTERNATEROMNAME, MF_CHECKED);
					break;
				case ROMLIST_DISPLAY_FILENAME:
					CheckMenuItem(popupmenu, ID_HEADERPOPUP_SHOWROMFILENAME, MF_CHECKED);
					break;
				}
				
				if( romlist_sort_method == 0 )
				{
					CheckMenuItem(popupmenu, ID_HEADERPOPUP_1_SORT_ASCENDING, MF_CHECKED);
				}
				else if( romlist_sort_method == 4 )
				{
					CheckMenuItem(popupmenu, ID_HEADERPOPUP_1_SORT_DESCENDING, MF_CHECKED);
				}
				
			}
			else if( romListHeaderClickedColumn < numberOfRomListColumns )
			{
				char	textbuf[100];
				LVCOLUMN colinfo;
				colinfo.mask = LVCF_TEXT;
				colinfo.pszText = textbuf;
				colinfo.cchTextMax = 99;
				
				if( ListView_GetColumn(gui.hwndRomList, romListHeaderClickedColumn, &colinfo) )
				{
					romListHeaderClickedColumn=GetColumnIDByName(colinfo.pszText);
				}

				popupmenu = GetSubMenu(popupmainmenu,2);
				if( romListHeaderClickedColumn < 4 )
				{
					if( romlist_sort_method == romListHeaderClickedColumn )
					{
						CheckMenuItem(popupmenu, ID_HEADERPOPUP_2_SORT_ASCENDING, MF_CHECKED);
					}
					else if( romlist_sort_method == romListHeaderClickedColumn+4 )
					{
						CheckMenuItem(popupmenu, ID_HEADERPOPUP_2_SORT_DESCENDING, MF_CHECKED);
					}
				}
				else
				{
					EnableMenuItem(popupmenu, ID_HEADERPOPUP_2_SORT_ASCENDING, MF_GRAYED);
					EnableMenuItem(popupmenu, ID_HEADERPOPUP_2_SORT_DESCENDING, MF_GRAYED);
				}
			}
			
			TrackPopupMenuEx(popupmenu, TPM_VERTICAL, pts.x, pts.y, gui.hwnd1964main, NULL);
			DestroyMenu(popupmenu);
		}
	default:
		return(DefWindowProc(hWnd, message, wParam, lParam));
		break;
	}
	return(0l);
}

BOOL ColumnSelect_TreeView_GetCheckState(HWND hwndTreeView, HTREEITEM hItem)
{
    TVITEM tvItem;

    // Prepare to receive the desired information.
    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;

    // Request the information.
    TreeView_GetItem(hwndTreeView, &tvItem);

    // Return zero if it's not checked, or nonzero otherwise.
    return ((BOOL)(tvItem.state >> 12) -1);
}

LRESULT APIENTRY ColumnSelectDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	HWND hwndTV;
	int i;
	HTREEITEM selected, other;
	hwndTV = GetDlgItem(hDlg, IDC_COL_SEL_TREE);
	
	switch(message)
	{
	case WM_INITDIALOG:
		switch(romlistNameToDisplay)
		{
		case ROMLIST_DISPLAY_ALTER_NAME:
			CheckRadioButton(hDlg, IDC_ALT_NAME, IDC_ROM_FILENAME, IDC_ALT_NAME);
			break;
		case ROMLIST_DISPLAY_INTERNAL_NAME:
			CheckRadioButton(hDlg, IDC_ALT_NAME, IDC_ROM_FILENAME, IDC_INTERNAL_NAME);
			break;
		case ROMLIST_DISPLAY_FILENAME:
			CheckRadioButton(hDlg, IDC_ALT_NAME, IDC_ROM_FILENAME, IDC_ROM_FILENAME);
			break;
		}

		{
			//Make sure the checkboxes can be displayed correctly, TreeView has some timing issue.
			LONG val = GetWindowLong(hwndTV, GWL_STYLE);
			val &= ~TVS_CHECKBOXES;
			SetWindowLong(hwndTV, GWL_STYLE, val);
			val |= TVS_CHECKBOXES;
			SetWindowLong(hwndTV, GWL_STYLE, val);
		}

		InsertColumnsIntoTreeView(hwndTV);

		return(TRUE);
		break;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDC_COLSEL_UP:
			selected = TreeView_GetSelection(hwndTV);
			if( selected != NULL )
			{
				other = TreeView_GetPrevSibling(hwndTV, selected);
				if( other != NULL )
				{
					if( ColumnMoveUp(selected) )
					{
						TreeView_DeleteAllItems(hwndTV);
						InsertColumnsIntoTreeView(hwndTV);
					}
				}
			}
			break;
		case IDC_COLSEL_DOWN:
			selected = TreeView_GetSelection(hwndTV);
			if( selected != NULL )
			{
				other = TreeView_GetNextSibling(hwndTV, selected);
				if( other != NULL )
				{
					if( ColumnMoveDown(selected) )
					{
						TreeView_DeleteAllItems(hwndTV);
						InsertColumnsIntoTreeView(hwndTV);
					}
				}
			}
			break;
		case IDOK:
			//Check the rom filename selecting
			if( IsDlgButtonChecked(hDlg, IDC_ALT_NAME) == BST_CHECKED )
			{
				romlistNameToDisplay = ROMLIST_DISPLAY_ALTER_NAME;
			}
			else if( IsDlgButtonChecked(hDlg, IDC_INTERNAL_NAME) == BST_CHECKED )
			{
				romlistNameToDisplay = ROMLIST_DISPLAY_INTERNAL_NAME;
			}
			else
			{
				romlistNameToDisplay = ROMLIST_DISPLAY_FILENAME;
			}
			
			for( i=1; i<numberOfRomListColumns; i++ )
			{
				romListColumns[i].enabled = ColumnSelect_TreeView_GetCheckState(hwndTV, romListColumns[i].treeViewID);
			}
			EndDialog(hDlg, TRUE);
			NewRomList_ListViewChangeWindowRect();
			//SendMessage(gui.hwnd1964main, WM_COMMAND, ID_FILE_FRESHROMLIST, 0);
			return(TRUE);
		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return(TRUE);
		}
		return(TRUE);
		break;
	}
	return(FALSE);
}


ColumnID GetColumnIDByName(char *name)
{
	int i;
	for( i=0; i<numberOfRomListColumns; i++ )
	{
		if( stricmp(name, romListColumns[i].text) == 0 )
		{
			return i;
		}
	}

	return 0;
}

ColumnID GetColumnIDByPos(int pos)
{
	int i;
	if( pos >= 0 && pos < numberOfRomListColumns )
	{
		for( i=0; i<numberOfRomListColumns; i++ )
		{
			if( romListColumns[i].colPos = pos )
			{
				return i;
			}
		}

		return numberOfRomListColumns-1;
	}
	else
	{
		return numberOfRomListColumns-1;
	}
}

void InsertColumnsIntoTreeView(HWND hwndTV)
{
	TVINSERTSTRUCT insItem;
	int i;

	//Insert all items into the Tree View
	insItem.hParent = NULL;
	insItem.hInsertAfter = TVI_LAST;
	insItem.item.mask = TVIF_TEXT | TVIF_STATE;
	insItem.item.hItem = 0;
	insItem.item.cchTextMax = 10;
	insItem.item.cChildren = 0;
	insItem.item.iImage = 0;
	insItem.item.lParam = 0;
	insItem.item.stateMask = TVIS_STATEIMAGEMASK;
	
	for( i=1; i<numberOfRomListColumns; i++ )
	{
		insItem.item.state = INDEXTOSTATEIMAGEMASK((romListColumns[i].enabled ? 2 : 1));
		insItem.item.pszText = romListColumns[i].text;
		romListColumns[i].treeViewID = TreeView_InsertItem(hwndTV, &insItem);
	}
}

BOOL ColumnMoveUp(HTREEITEM selected)
{
	int i;
	int idx = -1;
	for( i=0; i<numberOfRomListColumns; i++ )
	{
		if(romListColumns[i].treeViewID == selected)
		{
			idx = i;
			break;
		}
	}

	if( idx == -1 || romListColumns[idx].colPos <= 1 )
	{
		return FALSE;
	}

	for( i=0; i<numberOfRomListColumns; i++ )
	{
		if(romListColumns[i].colPos == romListColumns[idx].colPos-1 )
		{
			romListColumns[i].colPos++;
			romListColumns[idx].colPos--;
			return TRUE;
		}
	}

	return FALSE;
}

BOOL ColumnMoveDown(HTREEITEM selected)
{
	int i;
	int idx = -1;
	for( i=0; i<numberOfRomListColumns; i++ )
	{
		if(romListColumns[i].treeViewID == selected)
		{
			idx = i;
			break;
		}
	}

	if( idx == -1 || romListColumns[idx].colPos == numberOfRomListColumns-1 )
	{
		return FALSE;
	}

	for( i=0; i<numberOfRomListColumns; i++ )
	{
		if(romListColumns[i].colPos == romListColumns[idx].colPos+1 )
		{
			romListColumns[i].colPos--;
			romListColumns[idx].colPos++;
			return TRUE;
		}
	}

	return FALSE;
}