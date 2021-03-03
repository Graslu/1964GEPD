/*$T wingui2.c GC 1.136 03/09/02 17:35:12 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    User Interface Client Dialogue Windows and Message Boxes
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

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dsound.h>
#define WIN32_LEAN_AND_MEAN
#include "../debug_option.h"
#include "../interrupt.h"
#include "../n64rcp.h"
#include "wingui.h"
#include "dll_audio.h"
#include "dll_video.h"
#include "dll_input.h"
#include "dll_rsp.h"
#include "windebug.h"
#include "../1964ini.h"
#include "../timer.h"
#include "registry.h"
#include "../emulator.h"
#include "../romlist.h"

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void CountryCodeToCountryName_and_TVSystem(int countrycode, char *countryname, int *tvsystem)
{
	//Keep Country Name < 10 characters!
	switch(countrycode)
	{
	/* Demo */
	case 0:
		*tvsystem = TV_SYSTEM_NTSC;
		strcpy(countryname, "Demo");
		break;

	case '7':
		*tvsystem = TV_SYSTEM_NTSC;
		strcpy(countryname, "Beta");
		break;

	case 0x41:
		*tvsystem = TV_SYSTEM_NTSC;
		strcpy(countryname, "USA/Japan");
		break;

	/* Germany */
	case 0x44:
		*tvsystem = TV_SYSTEM_PAL;
		strcpy(countryname, "German");
		break;

	/* USA */
	case 0x45:
		*tvsystem = TV_SYSTEM_NTSC;
		strcpy(countryname, "USA");
		break;

	/* France */
	case 0x46:
		*tvsystem = TV_SYSTEM_PAL;
		strcpy(countryname, "France");
		break;

	/* Italy */
	case 'I':
		*tvsystem = TV_SYSTEM_PAL;
		strcpy(countryname, "Italy");
		break;

	/* Japan */
	case 0x4A:
		*tvsystem = TV_SYSTEM_NTSC;
		strcpy(countryname, "Japan");
		break;

	/* Europe - PAL */
	case 0x50:
		*tvsystem = TV_SYSTEM_PAL;
		strcpy(countryname, "Europe");
		break;

	case 'S':	/* Spain */
		*tvsystem = TV_SYSTEM_PAL;
		strcpy(countryname, "Spain");
		break;

	/* Australia */
	case 0x55:
		*tvsystem = TV_SYSTEM_PAL;
		strcpy(countryname, "Australia");
		break;

	case 0x58:
		*tvsystem = TV_SYSTEM_PAL;
		strcpy(countryname, "Europe");
		break;

	/* Australia */
	case 0x59:
		*tvsystem = TV_SYSTEM_PAL;
		strcpy(countryname, "Australia");
		break;

	case 0x20:
	case 0x21:
	case 0x38:
	case 0x70:
		*tvsystem = TV_SYSTEM_PAL;
		sprintf(countryname, "Europe", countrycode);
		break;

	/* ??? */
	default:
		*tvsystem = TV_SYSTEM_PAL;
		sprintf(countryname, "PAL", countrycode);
		break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitPluginData(void)
{
	Gfx_Info.hWnd = gui.hwnd1964main;
	Gfx_Info.hStatusBar = gui.hStatusBar;
	Gfx_Info.MemoryBswaped = TRUE;
	Gfx_Info.HEADER = (__int8 *) &HeaderDllPass[0];
	Gfx_Info.RDRAM = (__int8 *) &gMemoryState.RDRAM[0];
	Gfx_Info.DMEM = (__int8 *) &SP_DMEM;
	Gfx_Info.IMEM = (__int8 *) &SP_IMEM;
	Gfx_Info.MI_INTR_RG = &MI_INTR_REG_R;
	Gfx_Info.DPC_START_RG = &DPC_START_REG;
	Gfx_Info.DPC_END_RG = &DPC_END_REG;
	Gfx_Info.DPC_CURRENT_RG = &DPC_CURRENT_REG;
	Gfx_Info.DPC_STATUS_RG = &DPC_STATUS_REG;
	Gfx_Info.DPC_CLOCK_RG = &DPC_CLOCK_REG;
	Gfx_Info.DPC_BUFBUSY_RG = &DPC_BUFBUSY_REG;
	Gfx_Info.DPC_PIPEBUSY_RG = &DPC_PIPEBUSY_REG;
	Gfx_Info.DPC_TMEM_RG = &DPC_TMEM_REG;

	Gfx_Info.VI_STATUS_RG = &VI_STATUS_REG;
	Gfx_Info.VI_ORIGIN_RG = &VI_ORIGIN_REG;
	Gfx_Info.VI_WIDTH_RG = &VI_WIDTH_REG;
	Gfx_Info.VI_INTR_RG = &VI_INTR_REG;
	Gfx_Info.VI_V_CURRENT_LINE_RG = &VI_CURRENT_REG;
	Gfx_Info.VI_TIMING_RG = &VI_BURST_REG;
	Gfx_Info.VI_V_SYNC_RG = &VI_V_SYNC_REG;
	Gfx_Info.VI_H_SYNC_RG = &VI_H_SYNC_REG;
	Gfx_Info.VI_LEAP_RG = &VI_LEAP_REG;
	Gfx_Info.VI_H_START_RG = &VI_H_START_REG;
	Gfx_Info.VI_V_START_RG = &VI_V_START_REG;
	Gfx_Info.VI_V_BURST_RG = &VI_V_BURST_REG;
	Gfx_Info.VI_X_SCALE_RG = &VI_X_SCALE_REG;
	Gfx_Info.VI_Y_SCALE_RG = &VI_Y_SCALE_REG;
	Gfx_Info.CheckInterrupts = CheckInterrupts;

	Audio_Info.hwnd = gui.hwnd1964main;
	Audio_Info.hinst = hinstLibAudio;

	Audio_Info.MemoryBswaped = 1;	/* If this is set to TRUE, then the memory has been pre */

	/*
	 * bswap on a dword (32 bits) boundry £
	 * eg. the first 8 bytes are stored like this: £
	 * 4 3 2 1 8 7 6 5
	 */
	Audio_Info.HEADER = (__int8 *) &HeaderDllPass[0];
	Audio_Info.__RDRAM = (__int8 *) &gMemoryState.RDRAM[0];
	Audio_Info.__DMEM = (__int8 *) &SP_DMEM;
	Audio_Info.__IMEM = (__int8 *) &SP_IMEM;

	Audio_Info.__MI_INTR_REG = &MI_INTR_REG_R;

	Audio_Info.__AI_DRAM_ADDR_REG = &AI_DRAM_ADDR_REG;;
	Audio_Info.__AI_LEN_REG = &AI_LEN_REG;
	Audio_Info.__AI_CONTROL_REG = &AI_CONTROL_REG;
	Audio_Info.__AI_STATUS_REG = &AI_STATUS_REG;
	Audio_Info.__AI_DACRATE_REG = &AI_DACRATE_REG;
	Audio_Info.__AI_BITRATE_REG = &AI_BITRATE_REG;
	Audio_Info.CheckInterrupts = CheckInterrupts;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_1964_Directory(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR];
	char	fname[_MAX_FNAME], ext[_MAX_EXT];
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	GetModuleFileName(NULL, path_buffer, sizeof(path_buffer));
	_splitpath(path_buffer, drive, dir, fname, ext);

	/* Set the main 1964.exe directory */
	strcpy(directories.main_directory, drive);
	strcat(directories.main_directory, dir);
}

char	critical_msg_buffer[32 * 1024]; /* 32KB */

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void __cdecl DisplayCriticalMessage(char *Message, ...)
{
	if(guioptions.show_critical_msg_window)
	{
		/*~~~~~~~~~~~~~*/
		char	Msg[400];
		va_list ap;
		/*~~~~~~~~~~~~~*/

		va_start(ap, Message);
		vsprintf(Msg, Message, ap);
		va_end(ap);

		if(strlen(critical_msg_buffer) + strlen(Msg) + 2 < 32 * 1024)
		{
			strcat(critical_msg_buffer, Msg);
			strcat(critical_msg_buffer, "\t\n");
			SendDlgItemMessage
			(
				gui.hCriticalMsgWnd,
				IDC_CRITICAL_MESSAGE_TEXTBOX,
				WM_SETTEXT,
				0,
				(LPARAM) critical_msg_buffer
			);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void __cdecl DisplayError(char *Message, ...)
{
	/*~~~~~~~~~~~~~*/
#ifdef ENABLE_DISPLAY_ERROR
	char	Msg[400];
	va_list ap;
	/*~~~~~~~~~~~~~*/

#ifdef WINDEBUG_1964
	RefreshDebugger();
#endif
	va_start(ap, Message);
	vsprintf(Msg, Message, ap);
	va_end(ap);

	if(guistatus.IsFullScreen == 0)
	{
//		MessageBox(NULL, Msg, "Error", MB_OK | MB_ICONINFORMATION);
	}

	DisplayCriticalMessage(Msg);
#else
	/* Display this in the log window */
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL __cdecl DisplayError_AskIfContinue(char *Message, ...)
{
	/*~~~~~~~~~~~~~*/
	char	Msg[400];
	int		val;
	va_list ap;
	/*~~~~~~~~~~~~~*/

#ifdef WINDEBUG_1964
	RefreshDebugger();
#endif
	va_start(ap, Message);
	vsprintf(Msg, Message, ap);
	va_end(ap);
	strcat(Msg, "\nDo you want to continue emulation ?");

	val = MessageBox(NULL, Msg, "Error", MB_YESNO | MB_ICONINFORMATION | MB_SYSTEMMODAL);
	if(val == IDYES)
		return TRUE;
	else
		return FALSE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateCIC(void)
{
	/*~~~~~~~~~~~~~~~~*/
	/* Math CIC */
	__int64 CIC_CRC = 0;
	int		i;
	/*~~~~~~~~~~~~~~~~*/

	for(i = 0; i < 0xFC0; i++)
	{
		CIC_CRC = CIC_CRC + (uint8) gMemoryState.ROM_Image[0x40 + i];
	}

	switch(CIC_CRC)
	{
	/* CIC-NUS-6101 (starfox) */
	case 0x33a27:
	case 0x3421e:
		/* DisplayError("Using CIC-NUS-6101\n"); */
		TRACE0("Using CIC-NUS-6101 for starfox\n");
		rominfo.CIC = (uint64) 0x3f;
		rominfo.RDRam_Size_Hack = (uint32) 0x318;
		break;

	/* CIC-NUS-6102 (mario) */
	case 0x34044:
		/* DisplayError("Using CIC-NUS-6102\n"); */
		TRACE0("Using CIC-NUS-6102 for mario\n");
		rominfo.CIC = (uint64) 0x3f;
		rominfo.RDRam_Size_Hack = (uint32) 0x318;
		ROM_CheckSumMario();

		break;

	/* CIC-NUS-6103 (Banjo) */
	case 0x357d0:
		/* DisplayError("Using CIC-NUS-6103\n"); */
		TRACE0("Using CIC-NUS-6103 for Banjo\n");
		rominfo.CIC = (uint64) 0x78;
		rominfo.RDRam_Size_Hack = (uint32) 0x318;
		break;

	/* CIC-NUS-6105 (Zelda) */
	case 0x47a81:
		/* DisplayError("Using CIC-NUS-6105\n"); */
		TRACE0("Using CIC-NUS-6105 for Zelda\n");
		rominfo.CIC = 0x91;
		rominfo.RDRam_Size_Hack = (uint32) 0x3F0;
		ROM_CheckSumZelda();
		break;

	/* CIC-NUS-6106 (F-Zero X) */
	case 0x371cc:
		/* DisplayError("Using CIC-NUS-6106\n"); */
		TRACE0("Using CIC-NUS-6106 for F-Zero/Yoshi Story\n");
		rominfo.CIC = (uint64) 0x85;
		rominfo.RDRam_Size_Hack = (uint32) 0x318;
		break;

	/* Using F1 World Grand Prix */
	case 0x343c9:
		/*
		 * LogDirectToFile("Using f1 World Grand Prix\n"); £
		 * DisplayError("F1 World Grand Prix ... i never saw ths BootCode before");
		 */
		TRACE0("Using Boot Code for F1 World Grand Prix\n");
		rominfo.CIC = (uint64) 0x85;
		rominfo.RDRam_Size_Hack = (uint32) 0x3F0;
		break;

	default:
		/*
		 * DisplayError("unknown CIC %08x!!!", (uint32)CIC_CRC); £
		 * SystemFailure(FILEIO_EXIT); £
		 * Use Mario for unknown boot code
		 */
		TRACE0("Unknown boot code, using Mario boot code instead");
		rominfo.CIC = (uint64) 0x3f;
		rominfo.RDRam_Size_Hack = (uint32) 0x318;
		break;
	}

	TRACE1("Rom CIC=%02X", rominfo.CIC);

	rominfo.countrycode = HeaderDllPass[0x3D];

	CountryCodeToCountryName_and_TVSystem(rominfo.countrycode, game_country_name, &game_country_tvsystem);
	rominfo.TV_System = game_country_tvsystem;
	Init_VI_Counter(game_country_tvsystem);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT APIENTRY CriticalMessageDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		return(TRUE);
	case WM_COMMAND:
		if(wParam == IDOK)
		{
			guioptions.show_critical_msg_window = 0;
			EndDialog(hDlg, TRUE);
			gui.hCriticalMsgWnd = NULL;
			SetActiveWindow(gui.hwnd1964main);
			return(TRUE);
		}
		else if(wParam == ID_CLEAR_MESSAGE)
		{
			SendDlgItemMessage(hDlg, IDC_CRITICAL_MESSAGE_TEXTBOX, WM_SETTEXT, 0, (LPARAM) "");
			critical_msg_buffer[0] = '\0';	/* clear the critical message buffer */
		}
		break;
	}

	return(FALSE);
}

/*
 =======================================================================================================================
    type = 0 Load all plugins type = 1 Load video plugin type = 2 Load audio plugin type = 3 Load input plugin
 =======================================================================================================================
 */
LPDIRECTSOUND lpds;
void LoadPlugins(int type)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	AudioPath[_MAX_PATH];					/* _MAX_PATH = 260 */
	char	VideoPath[_MAX_PATH];
	char	InputPath[_MAX_PATH];
	char	StartPath[_MAX_PATH];
	static int		Audio = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	SetDefaultOptions();
	GetPluginDir(StartPath);

	if( type == LOAD_ALL_PLUGIN || type == LOAD_RSP_PLUGIN )
	{
		SetStatusBarText(0, "Loading RSP Plugin ...");

		/* Set RSP plugin path */
		strcpy(InputPath, StartPath);
		strcpy(gRegSettings.RSPPlugin, "rsp.dll");
		strcat(InputPath, gRegSettings.RSPPlugin);

		/* Load RSP plugin DLL */
		if(LoadRSPPlugin(InputPath) == FALSE)
		{
			CloseRSPPlugin();
			rsp_plugin_is_loaded = FALSE;
			emuoptions.UsingRspPlugin = FALSE;
			EnableMenuItem(gui.hMenu1964main, ID_RSP_CONFIG, MF_GRAYED);
		}
		else
		{
			rsp_plugin_is_loaded = TRUE;
		}

		SetStatusBarText(0, "Init RSP Plugin ...");
		InitializeRSP();
	}

	if(type == LOAD_ALL_PLUGIN || type == LOAD_VIDEO_PLUGIN)
	{
		strcpy(VideoPath, StartPath);

		SetStatusBarText(0, "Loading Video Plugin ...");

		/* Set Video plugin path */
		if(strcmp(gRegSettings.VideoPlugin, "") == 0)
		{
			strcpy(gRegSettings.VideoPlugin, "Jabo_Direct3D8.dll");
			strcat(VideoPath, gRegSettings.VideoPlugin);
		}
		else
		{
			strcat(VideoPath, gRegSettings.VideoPlugin);
		}

		/* Load Video plugin */
		if(LoadVideoPlugin(VideoPath) == FALSE)
		{
			DisplayError("Cannot load video plugin, check the file path or the plugin directory setting");
			strcpy(gRegSettings.VideoPlugin, "");
			WriteConfiguration();
			CloseVideoPlugin();
		}

		strcpy(generalmessage, gRegSettings.VideoPlugin);
		_strlwr(generalmessage);					/* convert to lower case */
		if(strstr(generalmessage, "gl") > 0)		/* Check if the plugin is opengl plugin */
		{
			guioptions.ok_to_pause_at_menu = FALSE; /* We should not pause game by menu if using opengl plugin */
		}
		else
		{
			guioptions.ok_to_pause_at_menu = TRUE;	/* if using D3D or other plugins, we can do it. */
		}
	}

	if(type == LOAD_ALL_PLUGIN || type == LOAD_INPUT_PLUGIN)
	{
		SetStatusBarText(0, "Loading Input Plugin ...");

		/* Set Input plugin path */
		strcpy(InputPath, StartPath);
		if(strcmp(gRegSettings.InputPlugin, "") == 0)
		{
			strcpy(gRegSettings.InputPlugin, "Mouse_Injector.dll");
			strcat(InputPath, gRegSettings.InputPlugin);
		}
		else
		{
			strcat(InputPath, gRegSettings.InputPlugin);
		}

		/* Load Input plugin DLL */
		if(LoadControllerPlugin(InputPath) == FALSE)
		{
			DisplayError("Cannot load controller plugin, check the file path or the plugin directory setting");
			strcpy(gRegSettings.InputPlugin, "");
			WriteConfiguration();
			CloseControllerPlugin();
		}

		/*
		 * Call the CONTROLLER_InitiateControllers function in the input DLL to initiate
		 * the controllers
		 */
		SetStatusBarText(0, "Init Input Plugin ...");
		CONTROLLER_InitiateControllers(gui.hwnd1964main, Controls);
		if(mouseinjectorpresent)
			CONTROLLER_HookRDRAM((DWORD *)TLB_sDWord_ptr, emuoptions.OverclockFactor);
	}

	if(type == LOAD_ALL_PLUGIN || type == LOAD_AUDIO_PLUGIN)
	{
		SetStatusBarText(0, "Loading Audio Plugin ...");

		/* Set path for the Audio plugin */
		strcpy(AudioPath, StartPath);
		if(strcmp(gRegSettings.AudioPlugin, "") == 0)
		{
			strcpy(gRegSettings.AudioPlugin, "AziAudio0.56WIP2.dll");
			strcat(AudioPath, gRegSettings.AudioPlugin);
		}
		else
		{
			strcat(AudioPath, gRegSettings.AudioPlugin);
		}

		Audio_Is_Initialized = 0;
		Audio = 0;
		if(LoadAudioPlugin(AudioPath) == TRUE)
		{
			Audio = 1;
		}

		if(_AUDIO_Initialize != NULL)
		{
			if(AUDIO_Initialize(Audio_Info) == TRUE)
			{
				Audio = 1;
				Audio_Is_Initialized = 1;
			}
			else
			{
				Audio = 0;
				Audio_Is_Initialized = 0;
			}
		}

		if (Audio_Is_Initialized)
		{
			HRESULT hr;

			__try {
				    if ( FAILED( hr = DirectSoundCreate( NULL, &lpds, NULL ) ) ) {
						Audio = 0;
					}
				    if ( lpds ) {
						IDirectSound_Release(lpds);
					}
				}
			__except(NULL, EXCEPTION_EXECUTE_HANDLER){
				Audio = 0;
				__try { if ( lpds ) IDirectSound_Release(lpds);}__except(NULL, EXCEPTION_EXECUTE_HANDLER){}
			}
		}

		if(Audio == 0)
		{
			DisplayError("Cannot load audio plugin, check the file path or the plugin directory setting");
			CloseAudioPlugin();
			strcpy(gRegSettings.AudioPlugin, "");
			strcpy(AudioPath, StartPath);
			strcpy(gRegSettings.AudioPlugin, "No Sound.dll");
			strcat(AudioPath, gRegSettings.AudioPlugin);
			WriteConfiguration();
			LoadAudioPlugin(AudioPath);
		}
	}

	if(type == LOAD_ALL_PLUGIN || type == LOAD_VIDEO_PLUGIN)
	{
		SetStatusBarText(0, "Init Video Plugin ...");
		InitPluginData();
		ShowWindow(gui.hwnd1964main, SW_HIDE);
		VIDEO_InitiateGFX(Gfx_Info);
		MoveWindow
		(
			gui.hwnd1964main,
			guistatus.window_position.left,
			guistatus.window_position.top,
			guistatus.clientwidth,
			guistatus.clientheight,
			TRUE
		);
		if(guistatus.WindowIsMaximized) ShowWindow(gui.hwnd1964main, SW_SHOWMAXIMIZED);
		ShowWindow(gui.hwnd1964main, SW_SHOW);

		/* VIDEO_RomOpen(); */
		NewRomList_ListViewChangeWindowRect();
		DockStatusBar();
	}

	Set_Ready_Message();
	if (Audio == 0)
		SetStatusBarText(1, "dsound fail");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FreePlugins(void)
{
	CloseControllerPlugin();
	CloseAudioPlugin();
	CloseVideoPlugin();
	CloseRSPPlugin();
}

void (__cdecl *GetDllInfo) (PLUGIN_INFO *) = NULL;
void (__cdecl *DLL_About) (HWND) = NULL;
char	temp_video_plugin[256];
char	temp_audio_plugin[256];
char	temp_input_plugin[256];

const char simplified_plugin_names[][2][100] =
{
	// video plugins
	{{"Highest: GLideN64 (2021)"},			{"GLideN64 rev.35554dab"}},
	{{"Highest: GLideN64 (2020)"},			{"GLideN64 rev.0a5216f8"}},
	{{"High: GLideN64 (2016)"},				{"GLideN64 Release V2 (Old)"}},
	{{"Medium: Glide64 (Final)"},			{"Glide64 'Final' Date: May  8 2012"}},
	{{"Low: Jabo (1.6.1)"},					{"Jabo's Direct3D8 1.6.1"}},
	{{"Lowest: Jabo (1.5.2)"},				{"Jabo's Direct3D6 1.5.2"}},
	{{"Developer: SoftGraphic"},			{"Shunyuan's SoftGraphic 1.5.0"}},

	// audio plugins
	{{"Default Audio"},						{"Azimer's HLE Audio v0.56 WIP 2"}},
	{{"Less Delay #1 (May Stutter)"},		{"Azimer's HLE Audio v0.60 WIP 2"}},
	{{"Less Delay #2 (May Stutter)"},		{"Azimer's HLE Audio v0.55.1 Alpha"}},

	// input plugins
	{{"Mouse Injector (Speedrun Build)"},	{" (Speedrun Build)"}},
	{{"Mouse Injector"},					{"Mouse Injector for GE/PD"}},
	{{"Controller (N-Rage 2.3c)"},			{"N-Rage Input Plugin V2 2.3c"}},
	{{"Controller (N-Rage 2.2)"},			{"N-Rage's Direct-Input8 V2 2.2 beta"}},
	{{"Controller (DirectInput)"},			{"Shunyuan's DirectInput 1.2.0"}},

	{{'\0'}, {'\0'}} // end of list
};

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT APIENTRY PluginsDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	PLUGIN_INFO		Plugin_Info;
	HINSTANCE		hinstLib = NULL;
	WIN32_FIND_DATA libaa;
	int				ComboItemNum;
	int				h = 0, i = 0, j = 0, bDONE = 0;
	HANDLE			FindFirst;
	char			PluginName[300];
	char			StartPath[_MAX_PATH];
	char			SearchPath[_MAX_PATH];
	int				index;
	BOOL			FoundRSPDll = FALSE;
	HWND			rsp_checkbox;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	GetPluginDir(StartPath);
	strcpy(SearchPath, StartPath);
	strcat(SearchPath, "*.dll");

	switch(message)
	{
	case WM_INITDIALOG:
		{
			/*~~~~~~~~~~~~~~~~*/
			int KeepLooping = 1;
			/*~~~~~~~~~~~~~~~~*/

			strcpy(temp_video_plugin, gRegSettings.VideoPlugin);
			strcpy(temp_audio_plugin, gRegSettings.AudioPlugin);
			strcpy(temp_input_plugin, gRegSettings.InputPlugin);

			FindFirst = FindFirstFile(SearchPath, &libaa);

			if(FindFirst == INVALID_HANDLE_VALUE)
			{
				return(TRUE);
			}

			/* Reset combo boxes content */
			SendDlgItemMessage(hDlg, IDC_COMBO_VIDEO, CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hDlg, IDC_COMBO_INPUT, CB_RESETCONTENT, 0, 0);
			SendDlgItemMessage(hDlg, IDC_COMBO_AUDIO, CB_RESETCONTENT, 0, 0);

			/* Populate combo boxes with Plugin Info */
			while(KeepLooping)
			{
				strcpy(PluginName, StartPath);
				strcat(PluginName, libaa.cFileName);

				hinstLib = LoadLibrary(PluginName);

				GetDllInfo = (void(__cdecl *) (PLUGIN_INFO *)) GetProcAddress(hinstLib, "GetDllInfo");

				__try
				{
					GetDllInfo(&Plugin_Info);
				}

				__except(NULL, EXCEPTION_EXECUTE_HANDLER)
				{
					goto skipdll;
				};

				if(guioptions.use_simplified_plugin_names) // replace plugin name with simplified names from list
				{
					for(index = 0; index < 256; index++)
					{
						if(simplified_plugin_names[index][0][0] == '\0') // reached end of list
							break;
						if(strstr(Plugin_Info.Name, simplified_plugin_names[index][1]) != NULL)
						{
							strcpy(Plugin_Info.Name, simplified_plugin_names[index][0]);
							break;
						}
					}
				}

				switch(Plugin_Info.Type)
				{
				case PLUGIN_TYPE_GFX:
					index = SendDlgItemMessage(hDlg, IDC_COMBO_VIDEO, CB_ADDSTRING, 0, (LPARAM) Plugin_Info.Name);
					if(_stricmp(libaa.cFileName, gRegSettings.VideoPlugin) == 0)
						SendDlgItemMessage(hDlg, IDC_COMBO_VIDEO, CB_SETCURSEL, (WPARAM) index, (LPARAM) 0);
					break;

				case PLUGIN_TYPE_CONTROLLER:
					index = SendDlgItemMessage(hDlg, IDC_COMBO_INPUT, CB_ADDSTRING, 0, (LPARAM) Plugin_Info.Name);
					if(_stricmp(libaa.cFileName, gRegSettings.InputPlugin) == 0)
						SendDlgItemMessage(hDlg, IDC_COMBO_INPUT, CB_SETCURSEL, (WPARAM) index, (LPARAM) 0);
					break;

				case PLUGIN_TYPE_AUDIO:
					index = SendDlgItemMessage(hDlg, IDC_COMBO_AUDIO, CB_ADDSTRING, 0, (LPARAM) Plugin_Info.Name);
					if(_stricmp(libaa.cFileName, gRegSettings.AudioPlugin) == 0)
						SendDlgItemMessage(hDlg, IDC_COMBO_AUDIO, CB_SETCURSEL, (WPARAM) index, (LPARAM) 0);
					break;
				case PLUGIN_TYPE_RSP:
					FoundRSPDll = TRUE;
					break;
				}

skipdll:
				FreeLibrary(hinstLib);
				hinstLib = NULL;
				KeepLooping = FindNextFile(FindFirst, &libaa);
				GetDllInfo = NULL;
				PluginName[0] = '\0';

				_CONTROLLER_Under_Selecting_DllAbout = _CONTROLLER_DllAbout;
				_CONTROLLER_Under_Selecting_DllTest = _CONTROLLER_DllTest;
				_VIDEO_Under_Selecting_About = _VIDEO_About;
				_VIDEO_Under_Selecting_Test = _VIDEO_Test;
				_AUDIO_Under_Selecting_About = _AUDIO_About;
				_AUDIO_Under_Selecting_Test = _AUDIO_Test;
			}
		}

		rsp_checkbox = GetDlgItem(hDlg, IDC_USE_RSP_PLUGIN);
		if( FoundRSPDll == TRUE )
		{
			EnableWindow(rsp_checkbox, guioptions.use_simplified_plugin_names == FALSE ? TRUE : FALSE);
			ShowWindow(rsp_checkbox, guioptions.use_simplified_plugin_names == FALSE ? TRUE : FALSE);
			ShowWindow(GetDlgItem(hDlg, IDC_SHOW_RSP_GROUP_BOX), guioptions.use_simplified_plugin_names == FALSE ? TRUE : FALSE);
			if( emuoptions.UsingRspPlugin == TRUE )
			{
				SendMessage(rsp_checkbox, BM_SETCHECK, (WPARAM)TRUE, (LPARAM)NULL);
			}
			else
			{
				SendMessage(rsp_checkbox, BM_SETCHECK, (WPARAM)FALSE, (LPARAM)NULL);
			}
		}
		else
		{
			SendMessage(rsp_checkbox, BM_SETCHECK, (WPARAM)FALSE, (LPARAM)NULL);
			EnableWindow(rsp_checkbox, FALSE);
		}
		SendDlgItemMessage(hDlg, IDC_USE_SIMPLIFIED_NAMES, BM_SETCHECK, guioptions.use_simplified_plugin_names ? BST_CHECKED : BST_UNCHECKED, (LPARAM)NULL);

		return(TRUE);

	case WM_COMMAND:
		{
			switch(wParam)
			{
			case IDOK:
				{
					/*~~~~~~~~~~~~~~~~~~~~~~~*/
					BOOL	is_changed = FALSE;
					/*~~~~~~~~~~~~~~~~~~~~~~~*/

					FreeLibrary(hinstLib);
					EndDialog(hDlg, TRUE);

					emuoptions.UsingRspPlugin = SendDlgItemMessage(hDlg, IDC_USE_RSP_PLUGIN, BM_GETCHECK, (WPARAM)NULL, (LPARAM)NULL);
					guioptions.use_simplified_plugin_names = SendDlgItemMessage(hDlg, IDC_USE_SIMPLIFIED_NAMES, BM_GETCHECK, (WPARAM)NULL, (LPARAM)NULL);

					if(strcmp(gRegSettings.VideoPlugin, temp_video_plugin) != 0)
					{
						strcpy(gRegSettings.VideoPlugin, temp_video_plugin);
						CloseVideoPlugin();
						LoadPlugins(LOAD_VIDEO_PLUGIN);
						NewRomList_ListViewChangeWindowRect();
						DockStatusBar();
						is_changed = TRUE;
					}

					if(strcmp(gRegSettings.AudioPlugin, temp_audio_plugin) != 0)
					{
						strcpy(gRegSettings.AudioPlugin, temp_audio_plugin);
						CloseAudioPlugin();
						LoadPlugins(LOAD_AUDIO_PLUGIN);
						is_changed = TRUE;
					}

					if(strcmp(gRegSettings.InputPlugin, temp_input_plugin) != 0)
					{
						strcpy(gRegSettings.InputPlugin, temp_input_plugin);
						CloseControllerPlugin();
						LoadPlugins(LOAD_INPUT_PLUGIN);
						CONTROLLER_InitiateControllers(gui.hwnd1964main, Controls); /* Input DLL Initialization */
						is_changed = TRUE;
					}

					if(is_changed) WriteConfiguration();
					return(TRUE);
				}

			case IDCANCEL:
				{
					EndDialog(hDlg, TRUE);
					return(TRUE);
				}

			case IDC_DI_ABOUT:
				CONTROLLER_Under_Selecting_DllAbout(hDlg);
				break;
			case IDC_DI_TEST:
				CONTROLLER_Under_Selecting_DllTest(hDlg);
				break;

			case IDC_VID_ABOUT:
				VIDEO_Under_Selecting_About(hDlg);
				break;
			case IDC_VID_TEST:
				VIDEO_Under_Selecting_Test(hDlg);
				break;

			case IDC_AUD_ABOUT:
				AUDIO_Under_Selecting_About(hDlg);
				break;
			case IDC_AUD_TEST:
				AUDIO_Under_Selecting_Test(hDlg);
				break;

			case IDC_USE_SIMPLIFIED_NAMES:
				MessageBox(hDlg, "Reopen plugin config dialog to update plugin names.", "Information", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
				break;
			}

		case CBN_SELCHANGE:
			switch(LOWORD(wParam))
			{
			case IDC_COMBO_VIDEO:
				/* Video */
				__try
				{
					FreeLibrary(hinstLib);
					ComboItemNum = SendDlgItemMessage(hDlg, IDC_COMBO_VIDEO, CB_GETCURSEL, 0, 0);

					bDONE = 0;
					FindFirst = FindFirstFile(SearchPath, &libaa);
					while(bDONE == 0)
					{
						strcpy(PluginName, StartPath);
						strcat(PluginName, libaa.cFileName);

						FreeLibrary(hinstLib);
						hinstLib = LoadLibrary(PluginName);

						GetDllInfo = (void(__cdecl *) (PLUGIN_INFO *)) GetProcAddress(hinstLib, "GetDllInfo");
						__try
						{
							GetDllInfo(&Plugin_Info);
						}

						__except(NULL, EXCEPTION_EXECUTE_HANDLER)
						{
							goto _skipPlugin3;
						}

						switch(Plugin_Info.Type)
						{
						case PLUGIN_TYPE_GFX:
							_VIDEO_Under_Selecting_Test = (void(__cdecl *) (HWND)) GetProcAddress(hinstLib, "DllTest");
							_VIDEO_Under_Selecting_About = (void(__cdecl *) (HWND)) GetProcAddress
								(
									hinstLib,
									"DllAbout"
								);

							h++;
							break;
						}

_skipPlugin3:
						if(h > ComboItemNum)
							bDONE = 1;
						else
							FindNextFile(FindFirst, &libaa);
						GetDllInfo = NULL;
						hinstLib = NULL;
					}

					bDONE = 0;
					strcpy(temp_video_plugin, libaa.cFileName);
				}

				__except(NULL, EXCEPTION_EXECUTE_HANDLER)
				{
					DisplayError("Video Plugin Error");

					/* No plugins of this type..do nothing */
				}
				break;
			case IDC_COMBO_AUDIO:
				__try
				{
					/* Audio */
					FreeLibrary(hinstLib);
					ComboItemNum = SendDlgItemMessage(hDlg, IDC_COMBO_AUDIO, CB_GETCURSEL, 0, 0);
					FindFirst = FindFirstFile(SearchPath, &libaa);
					bDONE = 0;

					while(bDONE == 0)
					{
						strcpy(PluginName, StartPath);
						strcat(PluginName, libaa.cFileName);

						FreeLibrary(hinstLib);
						hinstLib = LoadLibrary(PluginName);
						GetDllInfo = (void(__cdecl *) (PLUGIN_INFO *)) GetProcAddress(hinstLib, "GetDllInfo");
						__try
						{
							GetDllInfo(&Plugin_Info);
						}

						__except(NULL, EXCEPTION_EXECUTE_HANDLER)
						{
							goto _skipPlugin0;
						}

						switch(Plugin_Info.Type)
						{
						case PLUGIN_TYPE_AUDIO:
							_AUDIO_Under_Selecting_Test = (void(__cdecl *) (HWND)) GetProcAddress(hinstLib, "DllTest");
							_AUDIO_Under_Selecting_About = (void(__cdecl *) (HWND)) GetProcAddress
								(
									hinstLib,
									"DllAbout"
								);

							j++;
							break;
						}

_skipPlugin0:
						if(j > ComboItemNum)
							bDONE = 1;
						else
							FindNextFile(FindFirst, &libaa);
						GetDllInfo = NULL;
						hinstLib = NULL;
					}

					bDONE = 0;
					strcpy(temp_audio_plugin, libaa.cFileName);
				}

				__except(NULL, EXCEPTION_EXECUTE_HANDLER)
				{
					DisplayError("Audio Plugin Error");

					/* No plugins of this type..do nothing */
				}
				break;
			case IDC_COMBO_INPUT:
				/* Input */
				__try
				{
					FreeLibrary(hinstLib);
					ComboItemNum = SendDlgItemMessage(hDlg, IDC_COMBO_INPUT, CB_GETCURSEL, 0, 0);
					FindFirst = FindFirstFile(SearchPath, &libaa);
					bDONE = 0;

					while(bDONE == 0)
					{
						strcpy(PluginName, StartPath);
						strcat(PluginName, libaa.cFileName);
						FreeLibrary(hinstLib);
						hinstLib = LoadLibrary(PluginName);
						GetDllInfo = (void(__cdecl *) (PLUGIN_INFO *)) GetProcAddress(hinstLib, "GetDllInfo");

						__try
						{
							GetDllInfo(&Plugin_Info);
						}

						__except(NULL, EXCEPTION_EXECUTE_HANDLER)
						{
							goto _skipPlugin1;
						}

						switch(Plugin_Info.Type)
						{
						case PLUGIN_TYPE_CONTROLLER:
							_CONTROLLER_Under_Selecting_DllAbout = (void(__cdecl *) (HWND)) GetProcAddress
								(
									hinstLib,
									"DllAbout"
								);
							_CONTROLLER_Under_Selecting_DllTest = (void(__cdecl *) (HWND)) GetProcAddress
								(
									hinstLib,
									"DllTest"
								);
							i++;
							break;
						}

_skipPlugin1:
						if(i > ComboItemNum)
							bDONE = 1;
						else
							FindNextFile(FindFirst, &libaa);

						GetDllInfo = NULL;
						hinstLib = NULL;
					}

					strcpy(temp_input_plugin, libaa.cFileName);
				}

				__except(NULL, EXCEPTION_EXECUTE_HANDLER)
				{
					/* No plugins of this type..do nothing */
				}
				break;
			}
		}
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void GetPluginDir(char *Directory)
{
	/*
	 * strcpy(Directory,main_directory); £
	 * strcat(Directory,"Plugin\\");
	 */
	strcpy(Directory, directories.plugin_directory_to_use);
}

/*
 =======================================================================================================================
    Redistribution Conditions Window
 =======================================================================================================================
 */
LRESULT APIENTRY ConditionsDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	/*~~~~~~~~~~~~~~~~~~~~~~*/
	char	Conditions[11201];
	/*~~~~~~~~~~~~~~~~~~~~~~*/

	switch(message)
	{
	case WM_INITDIALOG:
		/* LoadString(gui.hInst, IDS_REDIST0, temp_buf, 4096); */
		LoadGNUDistConditions(Conditions);
		SetDlgItemText(hDlg, IDC_EDIT0, Conditions);
		return(TRUE);

	case WM_COMMAND:
		if(wParam == IDOK || wParam == IDCANCEL)
		{
			EndDialog(hDlg, TRUE);
			return(TRUE);
		}
		break;
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT APIENTRY About(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	switch(message)
	{
	case WM_INITDIALOG: return(TRUE);

	case WM_COMMAND:	if(wParam == IDOK || wParam == IDCANCEL){ EndDialog(hDlg, TRUE); return(TRUE); }break;
	}

	return(FALSE);
}


char cmdLineParameterBuf[250] = {0};
void SaveCmdLineParameter(char *cmdline)
{
	strcpy(cmdLineParameterBuf, cmdline);
	if( strlen(cmdLineParameterBuf) > 0 )
	{
		int i;
		int len = strlen(cmdLineParameterBuf);
		for( i=0; i<len; i++ )
		{
			if( isupper(cmdLineParameterBuf[i]) )
			{
				cmdLineParameterBuf[i] = tolower(cmdLineParameterBuf[i]);
			}
		}
	}
}

//To get a command line parameter if available, please pass a flag
// Flags:
//	"-v"	-> return video plugin name
//	"-a"	-> return audio plugin name
//  "-c"	-> return controller plugin name
//  "-g"	-> return game name to run
//	"-f"	-> return play-in-full-screen flag
//	"-r"	-> return rom path
char *CmdLineArgFlags[] =
{
	"-a",
	"-v",
	"-c",
	"-r",
	"-g",
	"-f"
};

void GetCmdLineParameter(CmdLineParameterType arg, char *buf)
{
	char *ptr1;
	char *ptr2 = buf;

	if( arg >= CMDLINE_MAX_NUMBER || strstr(cmdLineParameterBuf,CmdLineArgFlags[arg])==NULL )
	{
		buf[0] = 0;
		return;
	}
	
	if( arg == CMDLINE_FULL_SCREEN_FLAG )
	{
		strcpy(buf, "1");
		return;
	}

	ptr1 = strstr(cmdLineParameterBuf,CmdLineArgFlags[arg]);
	
	ptr1 += 2;	//Skip the flag
	while( *ptr1 != 0 && isspace(*ptr1) )
	{
		ptr1++;	//skip all spaces
	}

	while( !isspace(*ptr1) && *ptr1 != 0)
	{
		*ptr2++ = *ptr1++;
	};
	*ptr2 = 0;
}
