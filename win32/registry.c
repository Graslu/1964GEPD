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

#define CFG_FILENAME		"\\1964.cfg"
#define KEY_WINDOW_X		"WindowXPos"
#define KEY_WINDOW_Y		"WindowYPos"
#define KEY_MAXIMIZED		"Maximized"
#define KEY_CLIENT_WIDTH	"ClientWidth"
#define KEY_ROM_PATH		"ROMPath"
#define KEY_THREAD_PRIORITY "ThreadPriority"
#define KEY_AUDIO_PLUGIN	"AudioPlugin"
#define KEY_INPUT_PLUGIN	"InputPlugin"
#define KEY_VIDEO_PLUGIN	"VideoPlugin"

BOOL	Test1964Registry(void);
void	InitAll1964Options(void);
static unsigned int INI_OptionReadUInt(const char *str);
static float INI_OptionReadFloat(const char *str);
static char *INI_OptionReadString(const char *str);
static int INI_FindString(const char *str);
static int INI_CheckString(const char *str);
static unsigned int INI_GetUInt(void);
static float INI_GetFloat(void);
static void INI_GetString(char *str, const int size);

/*
 =======================================================================================================================
    This function is called only when 1964 starts £
 =======================================================================================================================
 */
int ReadConfiguration(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	str[260];
	char	tempstr[_MAX_PATH];
	int		i;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(Test1964Registry() == FALSE)
	{
		InitAll1964Options();
		WriteConfiguration();
		return 1;
	}

	strcpy(default_plugin_directory, directories.main_directory);
	strcat(default_plugin_directory, "plugin\\");
	strcpy(default_save_directory, directories.main_directory);
	strcat(default_save_directory, "save\\");
	
	strcpy(gRegSettings.ROMPath, INI_OptionReadString("ROMPath"));

	GetCmdLineParameter(CMDLINE_AUDIO_PLUGIN, tempstr);		//Get command line audio plugin setting
	if( strlen(tempstr) == 0 )
	{
		strcpy(gRegSettings.AudioPlugin, INI_OptionReadString("AudioPlugin"));
	}
	else
	{
		strcpy(gRegSettings.AudioPlugin, tempstr);
	}

	GetCmdLineParameter(CMDLINE_VIDEO_PLUGIN, tempstr);		//Get command line video plugin setting
	if( strlen(tempstr) == 0 )
	{
		strcpy(gRegSettings.VideoPlugin, INI_OptionReadString("VideoPlugin"));
	}
	else
	{
		strcpy(gRegSettings.VideoPlugin, tempstr);
	}
	
	GetCmdLineParameter(CMDLINE_CONTROLLER_PLUGIN, tempstr);		//Get command line controller plugin setting
	if( strlen(tempstr) == 0 )
	{
		strcpy(gRegSettings.InputPlugin, INI_OptionReadString("InputPlugin"));
	}
	else
	{
		strcpy(gRegSettings.InputPlugin, tempstr);
	}
	
	
	strcpy(user_set_rom_directory, INI_OptionReadString("ROMDirectory"));
	if(strlen(user_set_rom_directory) == 0) 
	{
		strcpy(user_set_rom_directory, gRegSettings.ROMPath);
	}
	
	strcpy(directories.last_rom_directory, INI_OptionReadString("LastROMDirectory"));
	strcpy(user_set_save_directory, INI_OptionReadString("SaveDirectory"));

	strcpy(state_save_directory, INI_OptionReadString("StateSaveDirectory"));
	if(strlen(state_save_directory) == 0) strcpy(state_save_directory, default_state_save_directory);

	strcpy(user_set_plugin_directory, INI_OptionReadString("PluginDirectory"));
	if(strlen(user_set_plugin_directory) == 0) strcpy(user_set_plugin_directory, default_plugin_directory);

	emuoptions.auto_run_rom = 1; //0.8.5: This is no longer a user option
	emuoptions.auto_full_screen = INI_OptionReadUInt("AutoFullScreen");
	//emuoptions.auto_apply_cheat_code = INI_OptionReadUInt("AutoApplyCheat");
	emuoptions.UsingRspPlugin = INI_OptionReadUInt("UsingRspPlugin");

	guioptions.pause_at_inactive = INI_OptionReadUInt("PauseWhenInactive");
	guioptions.pause_at_menu = INI_OptionReadUInt("PauseAtMenu");
	guioptions.show_expert_user_menu = INI_OptionReadUInt("ExpertUserMode");
	guioptions.show_recent_rom_directory_list = INI_OptionReadUInt("RomDirectoryListMenu");
	guioptions.show_recent_game_list = INI_OptionReadUInt("GameListMenu");
	guioptions.display_detail_status = INI_OptionReadUInt("DisplayDetailStatus");
	guioptions.display_profiler_status = INI_OptionReadUInt("DisplayProfilerStatus");
	guioptions.show_state_selector_menu = INI_OptionReadUInt("StateSelectorMenu");
	guioptions.show_critical_msg_window = INI_OptionReadUInt("DisplayCriticalMessageWindow");
	guioptions.display_romlist = INI_OptionReadUInt("DisplayRomList");
	guioptions.display_statusbar = INI_OptionReadUInt("DisplayStatusBar");
	guioptions.highfreqtimer = INI_OptionReadUInt("HighFreqTimer");
	guioptions.borderless_fullscreen = INI_OptionReadUInt("BorderlessFullscreen");
	guioptions.auto_hide_cursor_when_active = INI_OptionReadUInt("AutoHideCursorWhenActive");
	romlist_sort_method = INI_OptionReadUInt("SortRomList");
	romlistNameToDisplay = INI_OptionReadUInt("RomNameToDisplay");

	emuoptions.dma_in_segments = INI_OptionReadUInt("DmaInSegments");
	emuoptions.OverclockFactor = INI_OptionReadUInt("OverclockFactor") > 0 && INI_OptionReadUInt("OverclockFactor") <= 18 ? INI_OptionReadUInt("OverclockFactor") : 9;
	emuoptions.GEFiringRateHack = INI_OptionReadUInt("GEFiringRateHack");
	emuoptions.PDSpeedHack = INI_OptionReadUInt("PDSpeedHack");
	emuoptions.PDSpeedHackBoost = INI_OptionReadUInt("PDSpeedHackBoost");
	emuoptions.SyncVI = TRUE;

	guioptions.use_default_save_directory = INI_OptionReadUInt("UseDefaultSaveDiectory");
	guioptions.use_default_state_save_directory = INI_OptionReadUInt("UseDefaultStateSaveDiectory");
	guioptions.use_default_plugin_directory = INI_OptionReadUInt("UseDefaultPluginDiectory");
	guioptions.use_last_rom_directory = INI_OptionReadUInt("UseLastRomDiectory");

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
	defaultoptions.Eeprom_size = INI_OptionReadUInt("DefaultEepromSize");
	if(defaultoptions.Eeprom_size == 0 || defaultoptions.Eeprom_size > 2) defaultoptions.Eeprom_size = 1;

	defaultoptions.RDRAM_Size = INI_OptionReadUInt("DefaultRdramSize");
	if(defaultoptions.RDRAM_Size == 0 || defaultoptions.RDRAM_Size > 2) defaultoptions.RDRAM_Size = 2;

	defaultoptions.Emulator = DYNACOMPILER;

	defaultoptions.Save_Type = INI_OptionReadUInt("DefaultSaveType");
	if(defaultoptions.Save_Type == DEFAULT_SAVETYPE) defaultoptions.Save_Type = ANYUSED_SAVETYPE;

	defaultoptions.Code_Check = INI_OptionReadUInt("DefaultCodeCheck");
	defaultoptions.Max_FPS = INI_OptionReadUInt("DefaultMaxFPS");
	defaultoptions.Use_TLB = INI_OptionReadUInt("DefaultUseTLB");
	defaultoptions.FPU_Hack = INI_OptionReadUInt("DefaultUseFPUHack");

	/*
	 * defaultoptions.Link_4KB_Blocks =INI_OptionReadUInt(
	 * "DefaultUseLinkBlocks");
	 */
	defaultoptions.Link_4KB_Blocks = USE4KBLINKBLOCK_YES;
	defaultoptions.DMA_Segmentation = emuoptions.dma_in_segments;
	defaultoptions.Use_Register_Caching = INI_OptionReadUInt("RegisterCaching");
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
	defaultoptions.RDRAM_Size = RDRAMSIZE_8MB;
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
	guistatus.clientwidth = INI_OptionReadUInt("ClientWindowWidth");
	if(guistatus.clientwidth < 320) guistatus.clientwidth = 640;
	guistatus.clientheight = INI_OptionReadUInt("ClientWindowHeight");
	if(guistatus.clientheight < 200) guistatus.clientheight = 480;
	guistatus.window_position.top = INI_OptionReadUInt("1964WindowTOP");
	if(guistatus.window_position.top < 0) guistatus.window_position.top = 100;
	guistatus.window_position.left = INI_OptionReadUInt("1964WindowLeft");
	if(guistatus.window_position.left < 0) guistatus.window_position.left = 100;
	guistatus.WindowIsMaximized = INI_OptionReadUInt("1964WindowIsMaximized");

#ifdef DEBUG_COMMON
	debugoptions.debug_trap = INI_OptionReadUInt("DebugTrap");
	debugoptions.debug_si_controller = INI_OptionReadUInt("DebugSIController");
	debugoptions.debug_sp_task = INI_OptionReadUInt("DebugSPTask");
	debugoptions.debug_si_task = INI_OptionReadUInt("DebugSITask");
	debugoptions.debug_sp_dma = INI_OptionReadUInt("DebugSPDMA");
	debugoptions.debug_si_dma = INI_OptionReadUInt("DebugSIDMA");
	debugoptions.debug_pi_dma = INI_OptionReadUInt("DebugPIDMA");
	debugoptions.debug_si_mempak = INI_OptionReadUInt("DebugMempak");
	debugoptions.debug_tlb = INI_OptionReadUInt("DebugTLB");
	debugoptions.debug_si_eeprom = INI_OptionReadUInt("DebugEEPROM");
	debugoptions.debug_sram = INI_OptionReadUInt("DebugSRAM");
#endif
	for(i = 0; i < MAX_RECENT_ROM_DIR; i++)
	{
		sprintf(str, "RecentRomDirectory%d", i);
		strcpy(recent_rom_directory_lists[i], INI_OptionReadString(str));
		if(strlen(recent_rom_directory_lists[i]) == 0) strcpy(recent_rom_directory_lists[i], "Empty Rom Folder Slot");
	}

	for(i = 0; i < MAX_RECENT_GAME_LIST; i++)
	{
		sprintf(str, "RecentGame%d", i);
		strcpy(recent_game_lists[i], INI_OptionReadString(str));
		if(strlen(recent_game_lists[i]) == 0) strcpy(recent_game_lists[i], "Empty Game Slot");
	}

	for(i = 0; i < numberOfRomListColumns; i++)
	{
		int width;
		sprintf(str, "RomListColumn%dWidth", i);
		width = INI_OptionReadUInt(str);
		if(width >= 0 && width <= 520)
		{
			romListColumns[i].colWidth = width;
		}

		sprintf(str, "RomListColumn%dEnabled", i);
		romListColumns[i].enabled = INI_OptionReadUInt(str);
		if( romListColumns[i].enabled != TRUE )
		{
			romListColumns[i].enabled = FALSE;
		}
	}

	return 0;
}

struct
{
	int textloc;
	int textsize;
	char text[0x10000000];
	char numtemp[0x1000];
} INI;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL Test1964Registry(void)
{
	FILE *fileptr;
	int temp, status = TRUE;
	char inifilepath[_MAX_PATH];
	strcpy(inifilepath, directories.main_directory);
	strcat(inifilepath, CFG_FILENAME);
	INI.textsize = 0;
	if((fileptr = fopen(inifilepath, "rb")) != NULL)
	{
		temp = fgetc(fileptr);
		while(temp != EOF)
		{
			INI.text[INI.textsize] = temp;
			INI.textsize++;
			temp = fgetc(fileptr);
		}
		INI.text[INI.textsize] = 0;
		fclose(fileptr);
	}
	else
		status = FALSE;
	INI.textloc = 0;
	return status;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WriteConfiguration(void)
{
#define INI_OptionWriteUInt(variable, value) fprintf(fileptr, "%s %d\r\n", variable, value)
#define INI_OptionWriteFloat(variable, value) fprintf(fileptr, "%s %f\r\n", variable, value)
#define INI_OptionWriteString(variable, value) fprintf(fileptr, "%s %s\r\n", variable, value)
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	str[260];
	int		i;
	FILE	*fileptr;
	char	inifilepath[_MAX_PATH];
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	strcpy(inifilepath, directories.main_directory);
	strcat(inifilepath, CFG_FILENAME);

	/* Save current configuration */
	if((fileptr = fopen(inifilepath, "wb")) != NULL)
	{
		INI_OptionWriteString(KEY_ROM_PATH, gRegSettings.ROMPath);

		INI_OptionWriteString(KEY_VIDEO_PLUGIN, gRegSettings.VideoPlugin);

		INI_OptionWriteString(KEY_INPUT_PLUGIN, gRegSettings.InputPlugin);

		INI_OptionWriteString(KEY_AUDIO_PLUGIN, gRegSettings.AudioPlugin);

		INI_OptionWriteString("ROMDirectory", user_set_rom_directory);

		INI_OptionWriteString("LastROMDirectory", directories.last_rom_directory);

		INI_OptionWriteString("SaveDirectory", user_set_save_directory);

		INI_OptionWriteString("StateSaveDirectory", state_save_directory);

		INI_OptionWriteString("PluginDirectory", user_set_plugin_directory);

		INI_OptionWriteUInt("AutoFullScreen", emuoptions.auto_full_screen);

		//INI_OptionWriteUInt("AutoApplyCheat", emuoptions.auto_apply_cheat_code);

		INI_OptionWriteUInt("UsingRspPlugin", emuoptions.UsingRspPlugin);

		INI_OptionWriteUInt("HighFreqTimer", guioptions.highfreqtimer);

		INI_OptionWriteUInt("BorderlessFullscreen", guioptions.borderless_fullscreen);

		INI_OptionWriteUInt("AutoHideCursorWhenActive", guioptions.auto_hide_cursor_when_active);

		INI_OptionWriteUInt("OverclockFactor", emuoptions.OverclockFactor);

		INI_OptionWriteUInt("GEFiringRateHack", emuoptions.GEFiringRateHack);

		INI_OptionWriteUInt("PDSpeedHack", emuoptions.PDSpeedHack);

		INI_OptionWriteUInt("PDSpeedHackBoost", emuoptions.PDSpeedHackBoost);

		INI_OptionWriteUInt("PauseWhenInactive", guioptions.pause_at_inactive);

		INI_OptionWriteUInt("PauseAtMenu", guioptions.pause_at_menu);

		INI_OptionWriteUInt("ExpertUserMode", guioptions.show_expert_user_menu);

		INI_OptionWriteUInt("RomDirectoryListMenu", guioptions.show_recent_rom_directory_list);

		INI_OptionWriteUInt("GameListMenu", guioptions.show_recent_game_list);

		INI_OptionWriteUInt("DisplayDetailStatus", guioptions.display_detail_status);

		INI_OptionWriteUInt("DisplayProfilerStatus", guioptions.display_profiler_status);

		INI_OptionWriteUInt("StateSelectorMenu", guioptions.show_state_selector_menu);

		INI_OptionWriteUInt("DisplayCriticalMessageWindow", guioptions.show_critical_msg_window);

		INI_OptionWriteUInt("DisplayRomList", guioptions.display_romlist);

		INI_OptionWriteUInt("DisplayStatusBar", guioptions.display_statusbar);

		INI_OptionWriteUInt("SortRomList", romlist_sort_method);

		INI_OptionWriteUInt("RomNameToDisplay", romlistNameToDisplay);

		INI_OptionWriteUInt("UseDefaultSaveDiectory", guioptions.use_default_save_directory);

		INI_OptionWriteUInt("UseDefaultStateSaveDiectory", guioptions.use_default_state_save_directory);

		INI_OptionWriteUInt("UseDefaultPluginDiectory", guioptions.use_default_plugin_directory);

		INI_OptionWriteUInt("UseLastRomDiectory", guioptions.use_last_rom_directory);

#ifdef DEBUG_COMMON
		INI_OptionWriteUInt("DmaInSegments", emuoptions.dma_in_segments);

		INI_OptionWriteUInt("DefaultEepromSize", defaultoptions.Eeprom_size);

		INI_OptionWriteUInt("DefaultRdramSize", defaultoptions.RDRAM_Size);

		INI_OptionWriteUInt("DefaultEmulator", defaultoptions.Emulator);

		INI_OptionWriteUInt("DefaultSaveType", defaultoptions.Save_Type);

		INI_OptionWriteUInt("DefaultCodeCheck", defaultoptions.Code_Check);

		INI_OptionWriteUInt("DefaultMaxFPS", defaultoptions.Max_FPS);

		INI_OptionWriteUInt("DefaultUseTLB", defaultoptions.Use_TLB);

		INI_OptionWriteUInt("CounterFactor", defaultoptions.Counter_Factor);

		INI_OptionWriteUInt("RegisterCaching", defaultoptions.Use_Register_Caching);

		INI_OptionWriteUInt("DefaultUseFPUHack", defaultoptions.FPU_Hack);
#endif
		INI_OptionWriteUInt("ClientWindowWidth", guistatus.clientwidth);

		INI_OptionWriteUInt("ClientWindowHeight", guistatus.clientheight);

		INI_OptionWriteUInt("1964WindowTOP", guistatus.window_position.top);

		INI_OptionWriteUInt("1964WindowLeft", guistatus.window_position.left);

		INI_OptionWriteUInt("1964WindowIsMaximized", guistatus.WindowIsMaximized);

#ifdef DEBUG_COMMON
		INI_OptionWriteUInt("DebugTrap", debugoptions.debug_trap);

		INI_OptionWriteUInt("DebugSIController", debugoptions.debug_si_controller);

		INI_OptionWriteUInt("DebugSPTask", debugoptions.debug_sp_task);

		INI_OptionWriteUInt("DebugSITask", debugoptions.debug_si_task);

		INI_OptionWriteUInt("DebugSPDMA", debugoptions.debug_sp_dma);

		INI_OptionWriteUInt("DebugSIDMA", debugoptions.debug_si_dma);

		INI_OptionWriteUInt("DebugPIDMA", debugoptions.debug_pi_dma);

		INI_OptionWriteUInt("DebugMempak", debugoptions.debug_si_mempak);

		INI_OptionWriteUInt("DebugTLB", debugoptions.debug_tlb);

		INI_OptionWriteUInt("DebugEEPROM", debugoptions.debug_si_eeprom);

		INI_OptionWriteUInt("DebugSRAM", debugoptions.debug_sram);
#endif
		for(i = 0; i < MAX_RECENT_ROM_DIR; i++)
		{
			sprintf(str, "RecentRomDirectory%d", i);
			INI_OptionWriteString(str, recent_rom_directory_lists[i]);
		}

		for(i = 0; i < MAX_RECENT_GAME_LIST; i++)
		{
			sprintf(str, "RecentGame%d", i);
			INI_OptionWriteString(str, recent_game_lists[i]);
		}

		for(i = 0; i < numberOfRomListColumns; i++)
		{
			sprintf(str, "RomListColumn%dWidth", i);
			INI_OptionWriteUInt(str, romListColumns[i].colWidth);

			sprintf(str, "RomListColumn%dEnabled", i);
			INI_OptionWriteUInt(str, romListColumns[i].enabled);
		}
		fclose(fileptr);
	}

#undef INI_OptionWriteInt
#undef INI_OptionWriteFloat
#undef INI_OptionWriteString
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
	strcat(default_plugin_directory, "plugin\\");
	strcpy(default_save_directory, directories.main_directory);
	strcat(default_save_directory, "save\\");

	strcpy(gRegSettings.ROMPath, "");
	//JoelTODO: config Plugins here.
	strcpy(gRegSettings.AudioPlugin, "AziAudio0.56WIP2.dll");
	strcpy(gRegSettings.VideoPlugin, "Jabo_Direct3D8.dll");
	strcpy(gRegSettings.InputPlugin, "Mouse_Injector.dll");
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
	emuoptions.OverclockFactor = 9;
	emuoptions.GEFiringRateHack = TRUE;
	emuoptions.PDSpeedHack = TRUE;
	emuoptions.PDSpeedHackBoost = FALSE;
	
	guioptions.pause_at_inactive = TRUE;
	guioptions.pause_at_menu = FALSE;

	guioptions.show_expert_user_menu = FALSE;
	guioptions.show_recent_rom_directory_list = TRUE;
	guioptions.show_recent_game_list = TRUE;
	guioptions.display_detail_status = FALSE;
	guioptions.display_profiler_status = FALSE;
	guioptions.show_state_selector_menu = FALSE;
	guioptions.show_critical_msg_window = FALSE;
	guioptions.display_romlist = TRUE;
	guioptions.display_statusbar = TRUE;
	guioptions.highfreqtimer = TRUE;
	guioptions.borderless_fullscreen = FALSE;
	guioptions.auto_hide_cursor_when_active = TRUE;
	romlistNameToDisplay = ROMLIST_DISPLAY_FILENAME;
	romlist_sort_method = ROMLIST_GAMENAME;

	guioptions.use_default_save_directory = TRUE;
	guioptions.use_default_state_save_directory = TRUE;
	guioptions.use_default_plugin_directory = TRUE;
	guioptions.use_last_rom_directory = TRUE;

	defaultoptions.Eeprom_size = EEPROMSIZE_4KB;
	defaultoptions.RDRAM_Size = RDRAMSIZE_8MB;
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

	guistatus.clientwidth = 864;
	guistatus.clientheight = 586;
	guistatus.window_position.top = 100;
	guistatus.window_position.left = 100;
	guistatus.WindowIsMaximized = FALSE;
	romListColumns[0].colWidth = 244;
	romListColumns[1].colWidth = 57;
	romListColumns[2].colWidth = 52;
	romListColumns[3].colWidth = 488;

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
//==========================================================================
// INI_OptionReadInt: read int from config file
//==========================================================================
static unsigned int INI_OptionReadUInt(const char *str)
{
	unsigned int temp = 0;
	if(INI_FindString(str))
		temp = INI_GetUInt();
	INI.textloc = 0;
	return temp;
}
//==========================================================================
// INI_OptionReadFloat: read float from config file
//==========================================================================
static float INI_OptionReadFloat(const char *str)
{
	float temp = 0;
	if(INI_FindString(str))
		temp = INI_GetFloat();
	INI.textloc = 0;
	return temp;
}
//==========================================================================
// INI_OptionReadString: read string from config file
//==========================================================================
static char *INI_OptionReadString(const char *str)
{
	int foundvar = 1;
	char temp[0x10000] = "";
	if(INI_FindString(str))
		INI_GetString(temp, sizeof(temp));
	else
		foundvar = 0;
	INI.textloc = 0;
	return foundvar ? (temp) : ("");
}
//==========================================================================
// INI_FindString: search for variable in config file
//==========================================================================
static int INI_FindString(const char *str)
{
	char variable[128];
	sprintf(variable, "%s ", str);
	while(INI.textloc < INI.textsize && !INI_CheckString(variable))
		INI.textloc++;
	INI.textloc += strlen(variable);
	if(INI.textloc < INI.textsize)
		return 1;
	return 0;
}
//==========================================================================
// INI_CheckString
//==========================================================================
static int INI_CheckString(const char *str)
{
	int count = 0;
	while(INI.textloc + count < INI.textsize && str[count] != 0 && str[count] == INI.text[INI.textloc + count])
		count++;
	if(str[count] == 0)
		return 1;
	return 0;
}
//==========================================================================
// INI_GetInt
//==========================================================================
static unsigned int INI_GetUInt(void)
{
	int count = 0;
	unsigned int temp = 0;
	while(INI.textloc < INI.textsize && (INI.text[INI.textloc] < 48 || INI.text[INI.textloc] > 57) && INI.text[INI.textloc] != '-')
		INI.textloc++;
	while(INI.textloc < INI.textsize && count < 255 && ((INI.text[INI.textloc] >= 48 && INI.text[INI.textloc] <= 57) || INI.text[INI.textloc] == '-'))
	{
		INI.numtemp[count] = INI.text[INI.textloc];
		INI.textloc++;
		count++;
	}
	INI.numtemp[count] = 0;
	sscanf(INI.numtemp, "%u", &temp);
	return temp;
}
//==========================================================================
// INI_GetFloat
//==========================================================================
static float INI_GetFloat(void)
{
	int count = 0;
	float temp = 0;
	while(INI.textloc < INI.textsize && (INI.text[INI.textloc] < 48 || INI.text[INI.textloc] > 57) && INI.text[INI.textloc] != '-' && INI.text[INI.textloc] != '.')
		INI.textloc++;
	while(INI.textloc < INI.textsize && count < 255 && ((INI.text[INI.textloc] >= 48 && INI.text[INI.textloc] <= 57) || INI.text[INI.textloc] == '-' || INI.text[INI.textloc] == '.'))
	{
		INI.numtemp[count] = INI.text[INI.textloc];
		INI.textloc++;
		count++;
	}
	INI.numtemp[count] = 0;
	sscanf(INI.numtemp, "%f", &temp);
	return temp;
}
//==========================================================================
// INI_GetString
//==========================================================================
static void INI_GetString(char *str, const int size)
{
	int count = 0;
	while(INI.textloc < INI.textsize && count < size - 1 && INI.text[INI.textloc] != 13 && INI.text[INI.textloc] != 10)
	{
		str[count] = INI.text[INI.textloc];
		INI.textloc++;
		count++;
	}
	str[count] = 0;
}