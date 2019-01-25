/*$T registry.c GC 1.136 03/09/02 17:30:37 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    System registry keys and fields. Search your registry for 1964emu
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
#include <windows.h>
#include "registry.h"
#include "../1964ini.h"
#include "../debug_option.h"
#include "wingui.h"
#include "../emulator.h"
#include "windebug.h"
#include "../romlist.h"

#define MAIN_1964_KEY		"Software\\1964emu_085\\GUI"
#define KEY_WINDOW_X		"WindowXPos"
#define KEY_WINDOW_Y		"WindowYPos"
#define KEY_MAXIMIZED		"Maximized"
#define KEY_CLIENT_WIDTH	"ClientWidth"
#define KEY_ROM_PATH		"ROMPath"
#define KEY_THREAD_PRIORITY "ThreadPriority"
#define KEY_AUDIO_PLUGIN	"AudioPlugin"
#define KEY_INPUT_PLUGIN	"InputPlugin"
#define KEY_VIDEO_PLUGIN	"VideoPlugin"

char	*ReadRegistryStrVal(char *MainKey, char *Field);
uint32	ReadRegistryDwordVal(char *MainKey, char *Field);
BOOL	Test1964Registry(void);
void	InitAll1964Options(void);

/*
 =======================================================================================================================
    This function is called only when 1964 starts £
 =======================================================================================================================
 */
void ReadConfiguration(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	Directory[_MAX_PATH], str[260];
	char	tempstr[_MAX_PATH];
	int		i;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(Test1964Registry() == FALSE)
	{
		InitAll1964Options();
		WriteConfiguration();
		return;
	}

	strcpy(Directory, directories.main_directory);

	strcpy(default_plugin_directory, Directory);
	strcat(default_plugin_directory, "Plugin\\");

	strcpy(default_save_directory, Directory);
	strcat(default_save_directory, "Save\\");
	
	strcpy(gRegSettings.ROMPath, ReadRegistryStrVal(MAIN_1964_KEY, "ROMPath"));

	GetCmdLineParameter(CMDLINE_AUDIO_PLUGIN, tempstr);		//Get command line audio plugin setting
	if( strlen(tempstr) == 0 )
	{
		strcpy(gRegSettings.AudioPlugin, ReadRegistryStrVal(MAIN_1964_KEY, "AudioPlugin"));
	}
	else
	{
		strcpy(gRegSettings.AudioPlugin, tempstr);
	}

	GetCmdLineParameter(CMDLINE_VIDEO_PLUGIN, tempstr);		//Get command line video plugin setting
	if( strlen(tempstr) == 0 )
	{
		strcpy(gRegSettings.VideoPlugin, ReadRegistryStrVal(MAIN_1964_KEY, "VideoPlugin"));
	}
	else
	{
		strcpy(gRegSettings.VideoPlugin, tempstr);
	}
	
	GetCmdLineParameter(CMDLINE_CONTROLLER_PLUGIN, tempstr);		//Get command line controller plugin setting
	if( strlen(tempstr) == 0 )
	{
		strcpy(gRegSettings.InputPlugin, ReadRegistryStrVal(MAIN_1964_KEY, "InputPlugin"));
	}
	else
	{
		strcpy(gRegSettings.InputPlugin, tempstr);
	}
	
	
	strcpy(user_set_rom_directory, ReadRegistryStrVal(MAIN_1964_KEY, "ROMDirectory"));
	if(strlen(user_set_rom_directory) == 0) 
	{
		strcpy(user_set_rom_directory, gRegSettings.ROMPath);
	}
	
	strcpy(directories.last_rom_directory, ReadRegistryStrVal(MAIN_1964_KEY, "LastROMDirectory"));
	strcpy(user_set_save_directory, ReadRegistryStrVal(MAIN_1964_KEY, "SaveDirectory"));

	strcpy(state_save_directory, ReadRegistryStrVal(MAIN_1964_KEY, "StateSaveDirectory"));
	if(strlen(state_save_directory) == 0) strcpy(state_save_directory, default_state_save_directory);

	strcpy(user_set_plugin_directory, ReadRegistryStrVal(MAIN_1964_KEY, "PluginDirectory"));
	if(strlen(user_set_plugin_directory) == 0) strcpy(user_set_plugin_directory, default_plugin_directory);

	emuoptions.auto_run_rom = 1; //0.8.5: This is no longer a user option
	emuoptions.auto_full_screen = ReadRegistryDwordVal(MAIN_1964_KEY, "AutoFullScreen");
	//emuoptions.auto_apply_cheat_code = ReadRegistryDwordVal(MAIN_1964_KEY, "AutoApplyCheat");
	emuoptions.UsingRspPlugin = ReadRegistryDwordVal(MAIN_1964_KEY, "UsingRspPlugin");

	guioptions.pause_at_inactive = ReadRegistryDwordVal(MAIN_1964_KEY, "PauseWhenInactive");
	guioptions.pause_at_menu = ReadRegistryDwordVal(MAIN_1964_KEY, "PauseAtMenu");
	guioptions.show_expert_user_menu = ReadRegistryDwordVal(MAIN_1964_KEY, "ExpertUserMode");
	guioptions.show_recent_rom_directory_list = ReadRegistryDwordVal(MAIN_1964_KEY, "RomDirectoryListMenu");
	guioptions.show_recent_game_list = ReadRegistryDwordVal(MAIN_1964_KEY, "GameListMenu");
	guioptions.display_detail_status = ReadRegistryDwordVal(MAIN_1964_KEY, "DisplayDetailStatus");
	guioptions.display_profiler_status = ReadRegistryDwordVal(MAIN_1964_KEY, "DisplayProfilerStatus");
	guioptions.show_state_selector_menu = ReadRegistryDwordVal(MAIN_1964_KEY, "StateSelectorMenu");
	guioptions.show_critical_msg_window = ReadRegistryDwordVal(MAIN_1964_KEY, "DisplayCriticalMessageWindow");
	guioptions.display_romlist = ReadRegistryDwordVal(MAIN_1964_KEY, "DisplayRomList");
	romlist_sort_method = ReadRegistryDwordVal(MAIN_1964_KEY, "SortRomList");
	romlistNameToDisplay = ReadRegistryDwordVal(MAIN_1964_KEY, "RomNameToDisplay");

	emuoptions.dma_in_segments = ReadRegistryDwordVal(MAIN_1964_KEY, "DmaInSegments");
	emuoptions.SyncVI = ReadRegistryDwordVal(MAIN_1964_KEY, "emuoptions.SyncVI");

	guioptions.use_default_save_directory = ReadRegistryDwordVal(MAIN_1964_KEY, "UseDefaultSaveDiectory");
	guioptions.use_default_state_save_directory = ReadRegistryDwordVal(MAIN_1964_KEY, "UseDefaultStateSaveDiectory");
	guioptions.use_default_plugin_directory = ReadRegistryDwordVal(MAIN_1964_KEY, "UseDefaultPluginDiectory");
	guioptions.use_last_rom_directory = ReadRegistryDwordVal(MAIN_1964_KEY, "UseLastRomDiectory");

	/* Set the save directory to use */
	if(guioptions.use_default_save_directory)
		strcpy(directories.save_directory_to_use, default_save_directory);
	else
		strcpy(directories.save_directory_to_use, user_set_save_directory);

	/* Set the ROM directory to use */
	GetCmdLineParameter(CMDLINE_ROM_DIR, tempstr);		//Get command line rom path setting
	if( strlen(tempstr) > 0 )
	{
		strcpy(directories.rom_directory_to_use, tempstr);
	}
	else
	{
		if(guioptions.use_last_rom_directory)
		{
			strcpy(directories.rom_directory_to_use, directories.last_rom_directory);
		}
		else
		{
			strcpy(directories.rom_directory_to_use, user_set_rom_directory);
		}
	}
	
	/* Set the plugin directory to use */
	if(guioptions.use_default_plugin_directory)
		strcpy(directories.plugin_directory_to_use, default_plugin_directory);
	else
		strcpy(directories.plugin_directory_to_use, user_set_plugin_directory);

#ifdef DEBUG_COMMON
	defaultoptions.Eeprom_size = ReadRegistryDwordVal(MAIN_1964_KEY, "DefaultEepromSize");
	if(defaultoptions.Eeprom_size == 0 || defaultoptions.Eeprom_size > 2) defaultoptions.Eeprom_size = 1;

	defaultoptions.RDRAM_Size = ReadRegistryDwordVal(MAIN_1964_KEY, "DefaultRdramSize");
	if(defaultoptions.RDRAM_Size == 0 || defaultoptions.RDRAM_Size > 2) defaultoptions.RDRAM_Size = 2;

	defaultoptions.Emulator = DYNACOMPILER;

	defaultoptions.Save_Type = ReadRegistryDwordVal(MAIN_1964_KEY, "DefaultSaveType");
	if(defaultoptions.Save_Type == DEFAULT_SAVETYPE) defaultoptions.Save_Type = ANYUSED_SAVETYPE;

	defaultoptions.Code_Check = ReadRegistryDwordVal(MAIN_1964_KEY, "DefaultCodeCheck");
	defaultoptions.Max_FPS = ReadRegistryDwordVal(MAIN_1964_KEY, "DefaultMaxFPS");
	defaultoptions.Use_TLB = ReadRegistryDwordVal(MAIN_1964_KEY, "DefaultUseTLB");
	defaultoptions.FPU_Hack = ReadRegistryDwordVal(MAIN_1964_KEY, "DefaultUseFPUHack");

	/*
	 * defaultoptions.Link_4KB_Blocks =ReadRegistryDwordVal(MAIN_1964_KEY,
	 * "DefaultUseLinkBlocks");
	 */
	defaultoptions.Link_4KB_Blocks = USE4KBLINKBLOCK_YES;
	defaultoptions.DMA_Segmentation = emuoptions.dma_in_segments;
	defaultoptions.Use_Register_Caching = ReadRegistryDwordVal(MAIN_1964_KEY, "RegisterCaching");
	if(defaultoptions.Use_Register_Caching == USEREGC_DEFAULT || defaultoptions.Use_Register_Caching > USEREGC_NO)
		defaultoptions.Use_Register_Caching = USEREGC_YES;

	defaultoptions.Counter_Factor = COUTERFACTOR_2;
#else
	/*
	 * Rice: should we be doing this here like this? We init default options in your
	 * function. £
	 * I think it is better to call the function if we need this here.
	 */
	defaultoptions.Eeprom_size = EEPROMSIZE_4KB;
	defaultoptions.RDRAM_Size = RDRAMSIZE_4MB;
	defaultoptions.Emulator = DYNACOMPILER;
	defaultoptions.Save_Type = ANYUSED_SAVETYPE;
	defaultoptions.Code_Check = CODE_CHECK_MEMORY_QWORD;
	defaultoptions.Max_FPS = MAXFPS_AUTO_SYNC;
	defaultoptions.Use_TLB = USETLB_YES;
	defaultoptions.FPU_Hack = USEFPUHACK_YES;
	defaultoptions.DMA_Segmentation = USEDMASEG_YES;
	defaultoptions.Use_Register_Caching = USEREGC_YES;
	defaultoptions.Counter_Factor = COUTERFACTOR_2;
	defaultoptions.Link_4KB_Blocks = USE4KBLINKBLOCK_YES;
	defaultoptions.Advanced_Block_Analysis = USEBLOCKANALYSIS_YES;
	defaultoptions.Assume_32bit = ASSUME_32BIT_NO;
	defaultoptions.Use_HLE = USEHLE_NO;
#endif
	guistatus.clientwidth = ReadRegistryDwordVal(MAIN_1964_KEY, "ClientWindowWidth");
	if(guistatus.clientwidth < 320) guistatus.clientwidth = 640;
	guistatus.clientheight = ReadRegistryDwordVal(MAIN_1964_KEY, "ClientWindowHeight");
	if(guistatus.clientheight < 200) guistatus.clientheight = 480;
	guistatus.window_position.top = ReadRegistryDwordVal(MAIN_1964_KEY, "1964WindowTOP");
	if(guistatus.window_position.top < 0) guistatus.window_position.top = 100;
	guistatus.window_position.left = ReadRegistryDwordVal(MAIN_1964_KEY, "1964WindowLeft");
	if(guistatus.window_position.left < 0) guistatus.window_position.left = 100;
	guistatus.WindowIsMaximized = ReadRegistryDwordVal(MAIN_1964_KEY, "1964WindowIsMaximized");

#ifdef DEBUG_COMMON
	debugoptions.debug_trap = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugTrap");
	debugoptions.debug_si_controller = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugSIController");
	debugoptions.debug_sp_task = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugSPTask");
	debugoptions.debug_si_task = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugSITask");
	debugoptions.debug_sp_dma = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugSPDMA");
	debugoptions.debug_si_dma = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugSIDMA");
	debugoptions.debug_pi_dma = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugPIDMA");
	debugoptions.debug_si_mempak = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugMempak");
	debugoptions.debug_tlb = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugTLB");
	debugoptions.debug_si_eeprom = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugEEPROM");
	debugoptions.debug_sram = ReadRegistryDwordVal(MAIN_1964_KEY, "DebugSRAM");
#endif
	for(i = 0; i < MAX_RECENT_ROM_DIR; i++)
	{
		sprintf(str, "RecentRomDirectory%d", i);
		strcpy(recent_rom_directory_lists[i], ReadRegistryStrVal(MAIN_1964_KEY, str));
		if(strlen(recent_rom_directory_lists[i]) == 0) strcpy(recent_rom_directory_lists[i], "Empty Rom Folder Slot");
	}

	for(i = 0; i < MAX_RECENT_GAME_LIST; i++)
	{
		sprintf(str, "RecentGame%d", i);
		strcpy(recent_game_lists[i], ReadRegistryStrVal(MAIN_1964_KEY, str));
		if(strlen(recent_game_lists[i]) == 0) strcpy(recent_game_lists[i], "Empty Game Slot");
	}

	for(i = 0; i < numberOfRomListColumns; i++)
	{
		int width;
		sprintf(str, "RomListColumn%dWidth", i);
		width = ReadRegistryDwordVal(MAIN_1964_KEY, str);
		if(width >= 0 && width <= 500)
		{
			romListColumns[i].colWidth = width;
		}

		sprintf(str, "RomListColumn%dEnabled", i);
		romListColumns[i].enabled = ReadRegistryDwordVal(MAIN_1964_KEY, str);
		if( romListColumns[i].enabled != TRUE )
		{
			romListColumns[i].enabled = FALSE;
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL Test1964Registry(void)
{
	/*~~~~~~~~~~~~~~~~~*/
	HKEY	hKey1, hKey2;
	DWORD	rc;
	/*~~~~~~~~~~~~~~~~~*/

	if(RegConnectRegistry(NULL, HKEY_CURRENT_USER, &hKey1) == ERROR_SUCCESS)
	{
		/*~~~~~~~~~~~~~~~~~~*/
		char	szBuffer[260];
		/*~~~~~~~~~~~~~~~~~~*/

		strcpy(szBuffer, MAIN_1964_KEY);

		rc = RegOpenKey(hKey1, MAIN_1964_KEY, &hKey2);
		RegCloseKey(hKey1);

		if(rc == ERROR_SUCCESS)
			return TRUE;
		else
			return FALSE;
	}

	DisplayError("Error to read Windows registry database");
	return FALSE;
}

char	szData[MAX_PATH];

/*
 =======================================================================================================================
 =======================================================================================================================
 */
char *ReadRegistryStrVal(char *MainKey, char *Field)
{
	/*~~~~~~~~~~~~~~~~~~~*/
	HKEY	hKey1, hKey2;
	DWORD	rc;
	DWORD	cbData, dwType;
	/*~~~~~~~~~~~~~~~~~~~*/

	if(RegConnectRegistry(NULL, HKEY_CURRENT_USER, &hKey1) == ERROR_SUCCESS)
	{
		/*~~~~~~~~~~~~~~~~~~*/
		char	szBuffer[260];
		/*~~~~~~~~~~~~~~~~~~*/

		strcpy(szBuffer, MainKey);

		rc = RegOpenKey(hKey1, szBuffer, &hKey2);
		if(rc == ERROR_SUCCESS)
		{
			cbData = sizeof(szData);
			rc = RegQueryValueEx(hKey2, Field, NULL, &dwType, (LPBYTE) szData, &cbData);

			RegCloseKey(hKey2);
		}

		RegCloseKey(hKey1);
	}

	if(rc == ERROR_SUCCESS && cbData != 0)
	{
		return(szData);
	}
	else
	{
		return("");
	}
}

uint32	DwordData;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 ReadRegistryDwordVal(char *MainKey, char *Field)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	HKEY	hKey1, hKey2;
	DWORD	rc;
	DWORD	cbData;
	DWORD	dwType = REG_DWORD;
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	if(RegConnectRegistry(NULL, HKEY_CURRENT_USER, &hKey1) == ERROR_SUCCESS)
	{
		/*~~~~~~~~~~~~~~~~~~*/
		char	szBuffer[260];
		/*~~~~~~~~~~~~~~~~~~*/

		strcpy(szBuffer, MainKey);

		rc = RegOpenKey(hKey1, szBuffer, &hKey2);
		if(rc == ERROR_SUCCESS)
		{
			cbData = sizeof(DwordData);
			rc = RegQueryValueEx(hKey2, Field, NULL, &dwType, (LPBYTE) & DwordData, &cbData);

			RegCloseKey(hKey2);
		}

		RegCloseKey(hKey1);
	}

	if(rc == ERROR_SUCCESS && cbData != 0)
	{
		return(DwordData);
	}
	else
	{
		return(0);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WriteConfiguration(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	HKEY	hKey1, hKey2;
	DWORD	rc;
	DWORD	cbData;
	char	szBuffer[260], str[260];
	int		i;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/* Save current configuration */
	if(RegConnectRegistry(NULL, HKEY_CURRENT_USER, &hKey1) != ERROR_SUCCESS)
	{
		DisplayError("Error to write registry");
		return;
	}

	strcpy(szBuffer, MAIN_1964_KEY);
	rc = RegOpenKey(hKey1, szBuffer, &hKey2);
	if(rc != ERROR_SUCCESS)
	{
		rc = RegCreateKey(hKey1, szBuffer, &hKey2);
		if(rc != ERROR_SUCCESS)
		{
			DisplayError("Error to create MAIN_1964_KEY in the registry");
			return;
		}
	}

	strcpy(szData, gRegSettings.ROMPath);
	cbData = strlen(szData) + 1;
	RegSetValueEx(hKey2, KEY_ROM_PATH, 0, REG_SZ, (LPBYTE) szData, cbData);

	strcpy(szData, gRegSettings.VideoPlugin);
	cbData = strlen(szData) + 1;
	RegSetValueEx(hKey2, KEY_VIDEO_PLUGIN, 0, REG_SZ, (LPBYTE) szData, cbData);

	strcpy(szData, gRegSettings.InputPlugin);
	cbData = strlen(szData) + 1;
	RegSetValueEx(hKey2, KEY_INPUT_PLUGIN, 0, REG_SZ, (LPBYTE) szData, cbData);

	strcpy(szData, gRegSettings.AudioPlugin);
	cbData = strlen(szData) + 1;
	RegSetValueEx(hKey2, KEY_AUDIO_PLUGIN, 0, REG_SZ, (LPBYTE) szData, cbData);

	strcpy(szData, user_set_rom_directory);
	cbData = strlen(szData) + 1;
	RegSetValueEx(hKey2, "ROMDirectory", 0, REG_SZ, (LPBYTE) szData, cbData);

	strcpy(szData, directories.last_rom_directory);
	cbData = strlen(szData) + 1;
	RegSetValueEx(hKey2, "LastROMDirectory", 0, REG_SZ, (LPBYTE) szData, cbData);

	strcpy(szData, user_set_save_directory);
	cbData = strlen(szData) + 1;
	RegSetValueEx(hKey2, "SaveDirectory", 0, REG_SZ, (LPBYTE) szData, cbData);

	strcpy(szData, state_save_directory);
	cbData = strlen(szData) + 1;
	RegSetValueEx(hKey2, "StateSaveDirectory", 0, REG_SZ, (LPBYTE) szData, cbData);

	strcpy(szData, user_set_plugin_directory);
	cbData = strlen(szData) + 1;
	RegSetValueEx(hKey2, "PluginDirectory", 0, REG_SZ, (LPBYTE) szData, cbData);

	cbData = sizeof(DwordData);

	DwordData = emuoptions.SyncVI;
	RegSetValueEx(hKey2, "emuoptions.SyncVI", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = emuoptions.auto_full_screen;
	RegSetValueEx(hKey2, "AutoFullScreen", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = emuoptions.auto_apply_cheat_code;
	RegSetValueEx(hKey2, "AutoApplyCheat", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = emuoptions.UsingRspPlugin;
	RegSetValueEx(hKey2, "UsingRspPlugin", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.pause_at_inactive;
	RegSetValueEx(hKey2, "PauseWhenInactive", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.pause_at_menu;
	RegSetValueEx(hKey2, "PauseAtMenu", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.show_expert_user_menu;
	RegSetValueEx(hKey2, "ExpertUserMode", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.show_recent_rom_directory_list;
	RegSetValueEx(hKey2, "RomDirectoryListMenu", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.show_recent_game_list;
	RegSetValueEx(hKey2, "GameListMenu", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.display_detail_status;
	RegSetValueEx(hKey2, "DisplayDetailStatus", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.display_profiler_status;
	RegSetValueEx(hKey2, "DisplayProfilerStatus", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.show_state_selector_menu;
	RegSetValueEx(hKey2, "StateSelectorMenu", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.show_critical_msg_window;
	RegSetValueEx(hKey2, "DisplayCriticalMessageWindow", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.display_romlist;
	RegSetValueEx(hKey2, "DisplayRomList", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = romlist_sort_method;
	RegSetValueEx(hKey2, "SortRomList", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = romlistNameToDisplay;
	RegSetValueEx(hKey2, "RomNameToDisplay", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.use_default_save_directory;
	RegSetValueEx(hKey2, "UseDefaultSaveDiectory", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.use_default_state_save_directory;
	RegSetValueEx(hKey2, "UseDefaultStateSaveDiectory", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.use_default_plugin_directory;
	RegSetValueEx(hKey2, "UseDefaultPluginDiectory", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guioptions.use_last_rom_directory;
	RegSetValueEx(hKey2, "UseLastRomDiectory", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

#ifdef DEBUG_COMMON
	DwordData = emuoptions.dma_in_segments;
	RegSetValueEx(hKey2, "DmaInSegments", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = defaultoptions.Eeprom_size;
	RegSetValueEx(hKey2, "DefaultEepromSize", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = defaultoptions.RDRAM_Size;
	RegSetValueEx(hKey2, "DefaultRdramSize", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = defaultoptions.Emulator;
	RegSetValueEx(hKey2, "DefaultEmulator", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = defaultoptions.Save_Type;
	RegSetValueEx(hKey2, "DefaultSaveType", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = defaultoptions.Code_Check;
	RegSetValueEx(hKey2, "DefaultCodeCheck", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = defaultoptions.Max_FPS;
	RegSetValueEx(hKey2, "DefaultMaxFPS", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = defaultoptions.Use_TLB;
	RegSetValueEx(hKey2, "DefaultUseTLB", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = defaultoptions.Counter_Factor;
	RegSetValueEx(hKey2, "CounterFactor", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = defaultoptions.Use_Register_Caching;
	RegSetValueEx(hKey2, "RegisterCaching", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = defaultoptions.FPU_Hack;
	RegSetValueEx(hKey2, "DefaultUseFPUHack", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);
#endif
	DwordData = guistatus.clientwidth;
	RegSetValueEx(hKey2, "ClientWindowWidth", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guistatus.clientheight;
	RegSetValueEx(hKey2, "ClientWindowHeight", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guistatus.window_position.top;
	RegSetValueEx(hKey2, "1964WindowTOP", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guistatus.window_position.left;
	RegSetValueEx(hKey2, "1964WindowLeft", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = guistatus.WindowIsMaximized;
	RegSetValueEx(hKey2, "1964WindowIsMaximized", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

#ifdef DEBUG_COMMON
	DwordData = debugoptions.debug_trap;
	RegSetValueEx(hKey2, "DebugTrap", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = debugoptions.debug_si_controller;
	RegSetValueEx(hKey2, "DebugSIController", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = debugoptions.debug_sp_task;
	RegSetValueEx(hKey2, "DebugSPTask", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = debugoptions.debug_si_task;
	RegSetValueEx(hKey2, "DebugSITask", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = debugoptions.debug_sp_dma;
	RegSetValueEx(hKey2, "DebugSPDMA", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = debugoptions.debug_si_dma;
	RegSetValueEx(hKey2, "DebugSIDMA", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = debugoptions.debug_pi_dma;
	RegSetValueEx(hKey2, "DebugPIDMA", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = debugoptions.debug_si_mempak;
	RegSetValueEx(hKey2, "DebugMempak", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = debugoptions.debug_tlb;
	RegSetValueEx(hKey2, "DebugTLB", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = debugoptions.debug_si_eeprom;
	RegSetValueEx(hKey2, "DebugEEPROM", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

	DwordData = debugoptions.debug_sram;
	RegSetValueEx(hKey2, "DebugSRAM", 0, REG_DWORD, (LPBYTE) & DwordData, cbData);
#endif
	for(i = 0; i < MAX_RECENT_ROM_DIR; i++)
	{
		strcpy(szData, recent_rom_directory_lists[i]);
		sprintf(str, "RecentRomDirectory%d", i);
		cbData = strlen(szData) + 1;
		RegSetValueEx(hKey2, str, 0, REG_SZ, (LPBYTE) szData, cbData);
	}

	for(i = 0; i < MAX_RECENT_GAME_LIST; i++)
	{
		strcpy(szData, recent_game_lists[i]);
		sprintf(str, "RecentGame%d", i);
		cbData = strlen(szData) + 1;
		RegSetValueEx(hKey2, str, 0, REG_SZ, (LPBYTE) szData, cbData);
	}

	for(i = 0; i < numberOfRomListColumns; i++)
	{
		sprintf(str, "RomListColumn%dWidth", i);
		cbData = sizeof(DwordData);
		DwordData = romListColumns[i].colWidth;
		RegSetValueEx(hKey2, str, 0, REG_DWORD, (LPBYTE) & DwordData, cbData);

		sprintf(str, "RomListColumn%dEnabled", i);
		cbData = sizeof(DwordData);
		DwordData = romListColumns[i].enabled;
		RegSetValueEx(hKey2, str, 0, REG_DWORD, (LPBYTE) & DwordData, cbData);
	}

	RegCloseKey(hKey2);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitAll1964Options(void)
{
	/*~~*/
	int i;
	/*~~*/

	strcpy(default_plugin_directory, directories.main_directory);
	strcat(default_plugin_directory, "Plugin\\");

	strcpy(default_save_directory, directories.main_directory);
	strcat(default_save_directory, "Save\\");

	strcpy(gRegSettings.ROMPath, "");
	//JoelTODO: config Plugins here.
	strcpy(gRegSettings.AudioPlugin, "AudioHLE.dll");
	strcpy(gRegSettings.VideoPlugin, "DaedalusD3D8.dll");
	strcpy(gRegSettings.InputPlugin, "Basic Keyboard Plugin.dll");
	strcpy(user_set_rom_directory, "");
	strcpy(directories.last_rom_directory, "");
	strcpy(user_set_save_directory, default_save_directory);
	strcpy(state_save_directory, default_save_directory);
	strcpy(user_set_plugin_directory, default_plugin_directory);
	strcpy(directories.save_directory_to_use, default_save_directory);
	strcpy(directories.rom_directory_to_use, directories.last_rom_directory);
	strcpy(directories.plugin_directory_to_use, default_plugin_directory);

	emuoptions.auto_run_rom = TRUE;
	emuoptions.auto_full_screen = FALSE;
	emuoptions.auto_apply_cheat_code = FALSE;
	emuoptions.UsingRspPlugin = FALSE;
	emuoptions.dma_in_segments = TRUE;
	emuoptions.SyncVI = TRUE;
	
	guioptions.pause_at_inactive = TRUE;
	guioptions.pause_at_menu = FALSE;

	guioptions.show_expert_user_menu = FALSE;
	guioptions.show_recent_rom_directory_list = TRUE;
	guioptions.show_recent_game_list = TRUE;
	guioptions.display_detail_status = TRUE;
	guioptions.display_profiler_status = TRUE;
	guioptions.show_state_selector_menu = FALSE;
	guioptions.show_critical_msg_window = FALSE;
	guioptions.display_romlist = TRUE;
	romlist_sort_method = ROMLIST_GAMENAME;

	guioptions.use_default_save_directory = TRUE;
	guioptions.use_default_state_save_directory = TRUE;
	guioptions.use_default_plugin_directory = TRUE;
	guioptions.use_last_rom_directory = TRUE;

	defaultoptions.Eeprom_size = EEPROMSIZE_4KB;
	defaultoptions.RDRAM_Size = RDRAMSIZE_4MB;
	defaultoptions.Emulator = DYNACOMPILER;
	defaultoptions.Save_Type = ANYUSED_SAVETYPE;

	defaultoptions.Code_Check = CODE_CHECK_MEMORY_QWORD;
	defaultoptions.Max_FPS = MAXFPS_AUTO_SYNC;
	defaultoptions.Use_TLB = USETLB_YES;
	defaultoptions.FPU_Hack = USEFPUHACK_DEFAULT;
	defaultoptions.DMA_Segmentation = USEDMASEG_YES;
	defaultoptions.Link_4KB_Blocks = USE4KBLINKBLOCK_YES;
	defaultoptions.Advanced_Block_Analysis = USEBLOCKANALYSIS_YES;
	defaultoptions.Assume_32bit = ASSUME_32BIT_NO;
	defaultoptions.Use_HLE = USEHLE_NO;
	defaultoptions.Counter_Factor = COUTERFACTOR_2;
	defaultoptions.Use_Register_Caching = USEREGC_YES;

	guistatus.clientwidth = 640;
	guistatus.clientheight = 580;
	guistatus.window_position.top = 100;
	guistatus.window_position.left = 100;
	guistatus.WindowIsMaximized = FALSE;

#ifdef DEBUG_COMMON
	debugoptions.debug_trap = 1;
	debugoptions.debug_si_controller = 1;
	debugoptions.debug_sp_task = 0;
	debugoptions.debug_si_task = 0;
	debugoptions.debug_sp_dma = 0;
	debugoptions.debug_si_dma = 0;
	debugoptions.debug_pi_dma = 1;
	debugoptions.debug_si_mempak = 1;
	debugoptions.debug_tlb = 1;
	debugoptions.debug_si_eeprom = 1;
	debugoptions.debug_sram = 1;
#endif
	for(i = 0; i < MAX_RECENT_ROM_DIR; i++)
	{
		strcpy(recent_rom_directory_lists[i], "Empty Rom Folder Slot");
	}

	for(i = 0; i < MAX_RECENT_GAME_LIST; i++)
	{
		strcpy(recent_game_lists[i], "Empty Game Slot");
	}
}
