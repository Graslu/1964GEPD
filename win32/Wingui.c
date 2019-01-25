/*$T wingui.c GC 1.136 03/09/02 17:33:42 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    The Windows User Interface
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
#include <commdlg.h>
#include <direct.h>
#include <shlobj.h>
#include "wingui.h"
#include <shellapi.h>
#include "../globals.h"
#include "../debug_option.h"
#include "../hardware.h"
#include "../fileio.h"
#include "../emulator.h"
#include "../interrupt.h"
#include "../memory.h"
#include "../iPIF.h"
#include "../gamesave.h"
#include "DLL_Video.h"
#include "DLL_Audio.h"
#include "DLL_Input.h"
#include "DLL_RSP.h"
#include "registry.h"
#include "../r4300i.h"
#include "../timer.h"
#include "../romlist.h"
#include "../cheatcode.h"
#include "../kaillera/kaillera.h"
#include "../netplay.h"

#ifdef WINDEBUG_1964
#include "windebug.h"
#endif
struct EMU1964GUI	gui;
struct GUIOPTIONS	guioptions;
struct DIRECTORIES	directories;
struct GUISTATUS	guistatus;

int					ActiveApp;

unsigned int		cfmenulist[8] =
{
	ID_CF_CF1,
	ID_CF_CF2,
	ID_CF_CF3,
	ID_CF_CF4,
	ID_CF_CF5,
	ID_CF_CF6,
	ID_CF_CF7,
	ID_CF_CF8
};

unsigned int		codecheckmenulist[8] =
{
	ID_CPU_DYNACODECHECKING_NOCHECK,
	ID_CPU_DYNACODECHECKING_DMA,
	ID_CPU_DYNACODECHECKING_DWORD,
	ID_CPU_DYNACODECHECKING_QWORD,
	ID_CPU_DYNACODECHECKING_QWORDANDDMA,
	ID_CPU_DYNACODECHECKING_BLOCK,
	ID_CPU_DYNACODECHECKING_BLOCKANDDMA,
	ID_CPU_DYNACODECHECKING_PROTECTMEMORY
};

char				recent_rom_directory_lists[MAX_RECENT_ROM_DIR][260];
char				recent_game_lists[MAX_RECENT_GAME_LIST][260];

char				game_country_name[10];
int					game_country_tvsystem = 0;

int					Audio_Is_Initialized = 0;
int					timer;
int					StateFileNumber = 1;
int					togglecursor = FALSE;
int					firstlaunch1964 = 1;

extern int			selected_rom_index;
extern BOOL			Is_Reading_Rom_File;
extern BOOL			To_Stop_Reading_Rom_File;
extern BOOL			opcode_debugger_memory_is_allocated;
extern HINSTANCE	hinstControllerPlugin;
BOOL				NeedFreshromListAfterStop = TRUE;

#ifdef DEBUG_COMMON
void					ToggleDebugOptions(WPARAM wParam);
#endif
LRESULT APIENTRY		OptionsDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam);

void					SelectVISpeed(WPARAM wParam);
void					SetupAdvancedMenus(void);
void					RegenerateAdvancedUserMenus(void);
void					DeleteAdvancedUserMenus(void);
void					RegenerateStateSelectorMenus(void);
void					DeleteStateSelectorMenus(void);
void					RegerateRecentGameMenus(void);
void					DeleteRecentGameMenus(void);
void					RegerateRecentRomDirectoryMenus(void);
void					DeleteRecentRomDirectoryMenus(void);
void					RefreshRecentGameMenus(char *newgamefilename);
void					RefreshRecentRomDirectoryMenus(char *newromdirectory);
void					ChangeToRecentDirectory(int id);
void					OpenRecentGame(int id);
void					UpdateCIC(void);
void					init_debug_options(void);
extern LRESULT APIENTRY PluginsDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
long					OnNotifyStatusBar(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
long					OnPopupMenuCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
long					OnOpcodeDebuggerCommands(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void					OnFreshRomList();
void					DisableNetplayMemu();

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	if(Rom_Loaded)
	{
		if(emustatus.Emu_Is_Running)
		{
			if(!emustatus.Emu_Is_Paused)
			{
				vips = (float)(viCountePerSecond);

				if(vips >= 100.0)
				{
					sprintf(generalmessage, "%3d VI/s", (int) vips);
				}
				else
				{
					sprintf(generalmessage, "%3.1f VI/s", vips);
				}

				viCountePerSecond = 0;
				QueryPerformanceCounter(&LastSecondTime);

				if( guistatus.IsFullScreen == FALSE )
				{
					SetStatusBarText(1, generalmessage);

					if(guioptions.display_profiler_status)
					{
						format_profiler_result_msg(generalmessage);
						reset_profiler();
						SetStatusBarText(0, generalmessage);
					}
					else if(guioptions.display_detail_status)
					{
						sprintf
							(
							generalmessage,
							"PC=%08x, DList=%d, AList=%d, PI=%d, Cont=%d",
							gHWS_pc,
							emustatus.DListCount,
							emustatus.AListCount,
							emustatus.PIDMACount,
							emustatus.ControllerReadCount
							);
						SetStatusBarText(0, generalmessage);
					}
				}

				/* Apply the hack codes */
				if(emuoptions.auto_apply_cheat_code)
				{
#ifndef CHEATCODE_LOCK_MEMORY
					CodeList_ApplyAllCode(INGAME);
#endif
				}
				if(emuoptions.PDSpeedHack && emuoptions.OverclockFactor != 1 && (!strcmp(currentromoptions.Game_Name, "Perfect Dark") || !strcmp(currentromoptions.Game_Name, "GoldenEye X") || strstr(currentromoptions.Game_Name, "Perfect")))
					PDSpeedHack();
				if(emustatus.gepd_pause)
				{
					emustatus.gepd_pause--;
					if(!emustatus.gepd_pause)
						GEPDPause(FALSE);
				}
			}
		}
	}
	if(mouseinjectorpresent)
		CONTROLLER_HookRDRAM((DWORD *)TLB_sDWord_ptr, emuoptions.OverclockFactor);
}

LRESULT APIENTRY PropertyPagesProc(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		return(TRUE);
	
	case WM_NOTIFY:
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
            {
			case PSN_APPLY:
				EndDialog(lpnm->hwndFrom, TRUE);
				break;

            case PSN_RESET :
                //Handle a Cancel button click, if necessary
				EndDialog(lpnm->hwndFrom, TRUE);
				break;
			}
		}
	}
	return(FALSE);
}


//Test: Creating property pages for all options
void CreateOptionsDialog(void)
{
	/*~~~~~~~~~~~~~~~~~~~*/
	PROPSHEETPAGE	psp[4];
	PROPSHEETHEADER psh;
	/*~~~~~~~~~~~~~~~~~~~*/

	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_USETITLE;
	psp[0].hInstance = gui.hInst;
	psp[0].pszTemplate = "CHEAT_HACK";
	psp[0].pszIcon = NULL;
	psp[0].pfnDlgProc = (DLGPROC) PropertyPagesProc;
	psp[0].pszTitle = "Cheat Codes";
	psp[0].lParam = 0;

	psp[1].dwSize = sizeof(PROPSHEETPAGE);
	psp[1].dwFlags = PSP_USETITLE;
	psp[1].hInstance = gui.hInst;
	psp[1].pszTemplate = "OPTIONS";
	psp[1].pszIcon = NULL;
	psp[1].pfnDlgProc = (DLGPROC) PropertyPagesProc;
	psp[1].pszTitle = "User Options";
	psp[1].lParam = 0;

	psp[2].dwSize = sizeof(PROPSHEETPAGE);
	psp[2].dwFlags = PSP_USETITLE;
	psp[2].hInstance = gui.hInst;
	psp[2].pszTemplate = "PLUGINS";
	psp[2].pszIcon = NULL;
	psp[2].pfnDlgProc = (DLGPROC) PropertyPagesProc;
	psp[2].pszTitle = "Change Plugins";
	psp[2].lParam = 0;

	psp[3].dwSize = sizeof(PROPSHEETPAGE);
	psp[3].dwFlags = PSP_USETITLE;
	psp[3].hInstance = gui.hInst;
	psp[3].pszTemplate = "ROM_OPTIONS";
	psp[3].pszIcon = NULL;
	psp[3].pfnDlgProc = (DLGPROC) PropertyPagesProc;
	psp[3].pszTitle = "ROM Properties";
	psp[3].lParam = 0;

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = NULL;
	psh.hInstance = gui.hInst;
	psh.pszIcon = NULL;
	psh.pszCaption = (LPSTR) "1964 Options";
	psh.nStartPage = 0;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.ppsp = (LPCPROPSHEETPAGE) & psp;

	{
	HWND hCOP2Vecswnd;
	
	hCOP2Vecswnd = (HWND) PropertySheet(&psh);
	}
}


extern HWND WINAPI CreateTT(HWND hwndOwner);
/*
 =======================================================================================================================
 =======================================================================================================================
 */
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	/*~~~~*/
	MSG msg;
	/*~~~~*/

	if(hPrevInstance) return FALSE;
	SaveCmdLineParameter(lpszCmdLine);

	gui.szBaseWindowTitle = "1964 0.8.5";
	gui.hwnd1964main = NULL;		/* handle to main window */
	gui.hwndRomList = NULL;			/* Handle to the rom list child window */
	gui.hStatusBar = NULL;			/* Window Handle of the status bar */
	gui.hToolBar = NULL;			/* Window Handle of the toolbar */
	gui.hClientWindow = NULL;		/* Window handle of the client child window */
	gui.hCriticalMsgWnd = NULL;		/* handle to critical message window */
	gui.hMenu1964main = NULL;
	gui.hMenuRomListPopup = NULL;

	Rom_Loaded = FALSE;
	guistatus.block_menu = TRUE;	/* block all menu commands during startip */
	emustatus.cpucore = DYNACOMPILER;
	emustatus.Emu_Is_Resetting = FALSE;
	guistatus.IsFullScreen = FALSE;

#ifdef DEBUG_COMMON
	init_debug_options();
#endif
	p_gHardwareState = (HardwareState *) &gHardwareState;
	p_gMemoryState = (MemoryState *) &gMemoryState;

	gui.hInst = hInstance;
	LoadString(hInstance, IDS_MAINDISCLAIMER, MainDisclaimer, sizeof(MainDisclaimer));

	Set_1964_Directory();
	LoadDllKaillera();
	kailleraInit();
	firstlaunch1964 = ReadConfiguration();	/* System registry settings */

	gui.hwnd1964main = InitWin98UI(hInstance, nCmdShow);
	if(gui.hwnd1964main == NULL)
	{
		DisplayError("Could not get a windows handle.");
		exit(1);
	}

	SetupAdvancedMenus();
	SetupDebuger();
	if(guioptions.highfreqtimer)
		SetHighResolutionTimer();
	/*	
		Loading Netplay DLL, this must be done after loading the controller DLL because
		netplay DLL will be using the controller DLL
	*/
	if( load_netplay_dll() == FALSE )
	{
		DisableNetplayMemu();
	}

#ifndef ENABLE_OPCODE_DEBUGGER
	DeleteMenu(gui.hMenu1964main, ID_OPCODEDEBUGGER, MF_BYCOMMAND);
	DeleteMenu(gui.hMenu1964main, ID_OPCODEDEBUGGER_BLOCK_ONLY, MF_BYCOMMAND);
	DeleteMenu(gui.hMenu1964main, ID_DIRTYONLY, MF_BYCOMMAND);
#else
	if(debug_opcode!=0)
	{
		CheckMenuItem(gui.hMenu1964main, ID_OPCODEDEBUGGER, MF_CHECKED);
	}
	else
	{
		CheckMenuItem(gui.hMenu1964main, ID_OPCODEDEBUGGER, MF_UNCHECKED);
	}
#endif

	gui.hStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_OVERLAPPED, gui.staturbar_field.field_1, gui.hwnd1964main, 0x1122);
	{
		/*~~~~~~~~~~~~*/
		RECT	rc, src;
		/*~~~~~~~~~~~~*/

		GetWindowRect(gui.hwnd1964main, &rc);
		GetWindowRect(gui.hStatusBar, &src);
		DockStatusBar();
	}
	SetupToolBar();

	InitVirtualMemory();
	InitPluginData();

	SetStatusBarText(0, "Load Rom Setting from 1964.ini");
	FileIO_Load1964Ini();

	SetWindowText(gui.hwnd1964main, gui.szBaseWindowTitle);
	emustatus.cpucore = defaultoptions.Emulator;

	SetStatusBarText(2, "CF=1");
	SetStatusBarText(3, "8MB");
	SetStatusBarText(4, "D");

	gui.hwndRomList = NewRomList_CreateListViewControl(gui.hwnd1964main);	/* this must be before the video plugin init */
	SetStatusBarText(0, "Loading plugins");
	LoadPlugins(LOAD_ALL_PLUGIN);
	netplay_initialize_netplay(hinstControllerPlugin, Controls);

	EnableRadioButtons(FALSE);

	ShowWindow(gui.hwnd1964main, SW_SHOW);
	UpdateWindow(gui.hwnd1964main);
	
	if(guistatus.WindowIsMaximized)
	{
		ShowWindow(gui.hwnd1964main, SW_SHOWMAXIMIZED);
	}
	
	SetStatusBarText(0, "Initialize emulator and r4300 core");
	r4300i_Init();
	timer = SetTimer(gui.hwnd1964main, 1, 1000, TimerProc);

/*	if( emuoptions.UsingRspPlugin )
	{
		EnableMenuItem(gui.hMenu1964main, ID_RSP_CONFIG, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(gui.hMenu1964main, ID_RSP_CONFIG, MF_GRAYED);
	}
*/	
	if( StartGameByCommandLine() )
	{
	}
	else
	{
		NeedFreshromListAfterStop = FALSE;
		NewRomList_ListViewChangeWindowRect();
		DockStatusBar();

		SetStatusBarText(0, "Looking for ROM file in the ROM directory and Generate List");
		RomListReadDirectory(directories.rom_directory_to_use);
		NewRomList_ListViewFreshRomList();

		Set_Ready_Message();
	}
	guistatus.block_menu = FALSE;	/* allow menu commands */

	if(guioptions.show_critical_msg_window)
	{
		if(gui.hCriticalMsgWnd == NULL)
		{
			gui.hCriticalMsgWnd = CreateDialog(gui.hInst, "CRITICAL_MESSAGE", NULL, (DLGPROC) CriticalMessageDialog);
			SetActiveWindow(gui.hwnd1964main);
		}
	}

	SetFocus(gui.hwnd1964main);
	//CreateOptionsDialog();
	if(firstlaunch1964) // open the change ROM directory dialog window on first launch
		ChangeDirectory();


_HOPPITY:
	if(GetMessage(&msg, NULL, 0, 0))
	{
		if(!TranslateAccelerator(gui.hwnd1964main, gui.hAccTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	else
	{
		return msg.wParam;
	}

	goto _HOPPITY;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
HWND InitWin98UI(HANDLE hInstance, int nCmdShow)
{
	/*~~~~~~~~~~~*/
	WNDCLASS	wc;
	/*~~~~~~~~~~~*/

	wc.style = CS_SAVEBITS; /* | CS_DBLCLKS; */
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = (HINSTANCE) hInstance;
	wc.hIcon = LoadIcon((HINSTANCE) hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);	/* hBrush; */
	wc.lpszMenuName = "WINGUI_MENU";

	wc.lpszClassName = "WinGui";

	RegisterClass(&wc);

	gui.hwnd1964main = CreateWindow
		(
			"WinGui",
			gui.szBaseWindowTitle,
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,	/* | WS_VSCROLL, */
			guistatus.window_position.left,
			guistatus.window_position.top,
			guistatus.clientwidth,
			guistatus.clientheight,
			NULL,
			NULL,
			(HINSTANCE) hInstance,
			NULL
		);


	ShowWindow(gui.hToolBar, SW_SHOW);

	if(gui.hwnd1964main == NULL)
	{
		MessageBox(NULL, "CreateWindow() failed: Cannot create a window.", "Error", MB_OK);
		return(NULL);
	}

	gui.hAccTable = LoadAccelerators(gui.hInst, (LPCTSTR) WINGUI_ACC);
	gui.hMenu1964main = GetMenu(gui.hwnd1964main);

	if(!IsKailleraDllLoaded()) EnableMenuItem(gui.hMenu1964main, ID_KAILLERA_MODE, MF_GRAYED);
	SetOverclockFactor(emuoptions.OverclockFactor);
	SetCounterFactor(defaultoptions.Counter_Factor);

	return gui.hwnd1964main;
}

void ProcessMenuCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int PlayButtonState; //For pause/Play	
	
	if(guistatus.block_menu)
			return; /* ok, all menu commands are blocked */

		switch(LOWORD(wParam))
		{
		case ID_ROM_STOP:
		case ID_BUTTON_STOP:
			CloseROM();
			break;
		case ID_ROM_PAUSE:
			ChangeButtonState(ID_BUTTON_PAUSE);
			PlayButtonState = ChangeButtonState(ID_BUTTON_PLAY);
			if((PlayButtonState &0x01) != TBSTATE_CHECKED)
			{
				PauseEmulator();
				HideCursor(FALSE);
			}
			else
			{
				ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
				HideCursor(TRUE);
			}
			break;
		case ID_BUTTON_PLAY:
			ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
			break;
		case ID_BUTTON_PAUSE:
			PauseEmulator();
			break;
		case ID_CPU_KILL:
			KillCPUThread();
			break;
		case ID_KAILLERA_MODE:
			KailleraPlay();
			break;
		case ID_OPENROM:
		case ID_BUTTON_OPEN_ROM:

			EnableButton(ID_BUTTON_OPEN_ROM, FALSE);
			EnableMenuItem(gui.hMenu1964main, ID_OPENROM, MF_GRAYED);
			EnableButton(ID_BUTTON_SETUP_PLUGINS, FALSE);
			EnableMenuItem(gui.hMenu1964main, IDM_PLUGINS, MF_GRAYED);

			if (!guistatus.IsFullScreen)
			OpenROM();
			break;
		case ID_CLOSEROM:
			CloseROM();
			break;
		case ID_FILE_ROMINFO:
		case ID_BUTTON_ROM_PROPERTIES:
			if (!guistatus.IsFullScreen)
			RomListRomOptions(selected_rom_index);
			break;
		case ID_FILE_FRESHROMLIST:
			if (!guistatus.IsFullScreen)
			OnFreshRomList();
			break;
		case ID_DEFAULTOPTIONS:
			if (!guistatus.IsFullScreen)
			DialogBox(gui.hInst, "DEFAULT_OPTIONS", hWnd, (DLGPROC) DefaultOptionsDialog);
			break;
		case ID_PREFERENCE_OPTIONS:
			if (!guistatus.IsFullScreen)
			DialogBox(gui.hInst, "Options", hWnd, (DLGPROC) OptionsDialog);
			break;
		case ID_CHANGEDIRECTORY:
			if (!guistatus.IsFullScreen)
			ChangeDirectory();
			break;
		case ID_FILE_ROMDIRECTORY1:
			ChangeToRecentDirectory(0);
			break;
		case ID_FILE_ROMDIRECTORY2:
			if (!guistatus.IsFullScreen)
			ChangeToRecentDirectory(1);
			break;
		case ID_FILE_ROMDIRECTORY3:
			ChangeToRecentDirectory(2);
			break;
		case ID_FILE_ROMDIRECTORY4:
			ChangeToRecentDirectory(3);
			break;
		case ID_FILE_ROMDIRECTORY5:
			ChangeToRecentDirectory(4);
			break;
		case ID_FILE_ROMDIRECTORY6:
			ChangeToRecentDirectory(5);
			break;
		case ID_FILE_ROMDIRECTORY7:
			ChangeToRecentDirectory(6);
			break;
		case ID_FILE_ROMDIRECTORY8:
			ChangeToRecentDirectory(7);
			break;
		case ID_FILE_RECENTGAMES_GAME1:
			OpenRecentGame(0);
			break;
		case ID_FILE_RECENTGAMES_GAME2:
			OpenRecentGame(1);
			break;
		case ID_FILE_RECENTGAMES_GAME3:
			OpenRecentGame(2);
			break;
		case ID_FILE_RECENTGAMES_GAME4:
			OpenRecentGame(3);
			break;
		case ID_FILE_RECENTGAMES_GAME5:
			OpenRecentGame(4);
			break;
		case ID_FILE_RECENTGAMES_GAME6:
			OpenRecentGame(5);
			break;
		case ID_FILE_RECENTGAMES_GAME7:
			OpenRecentGame(6);
			break;
		case ID_FILE_RECENTGAMES_GAME8:
			OpenRecentGame(7);
			break;
		case ID_FILE_CHEAT:
			if (!guistatus.IsFullScreen)
			if(emustatus.Emu_Is_Running)
			{
				PauseEmulator();
				//SuspendThread(CPUThreadHandle);
				HideCursor(FALSE);
				DialogBox(gui.hInst, "CHEAT_HACK", hWnd, (DLGPROC) CheatAndHackDialog);
				//ResumeThread(CPUThreadHandle);
				ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
			}
			else
			{
				CodeList_ReadCode(romlist[selected_rom_index]->pinientry->Game_Name);
				DialogBox(gui.hInst, "CHEAT_HACK", hWnd, (DLGPROC) CheatAndHackDialog);
			}
			break;
		case ID_ABOUT:
			if (!guistatus.IsFullScreen)
			DialogBox(gui.hInst, "ABOUTBOX", hWnd, (DLGPROC) About);
			break;
		case ID_CHEATS_APPLY:
			CodeList_ApplyAllCode(GSBUTTON);
			break;
		case ID_CPU_AUDIOSYNC:
			if(emuoptions.SyncVI)
			{
				CheckMenuItem(gui.hMenu1964main, ID_CPU_AUDIOSYNC, MF_UNCHECKED);
				emuoptions.SyncVI = FALSE;
			}
			else
			{
				CheckMenuItem(gui.hMenu1964main, ID_CPU_AUDIOSYNC, MF_CHECKED);
				emuoptions.SyncVI = TRUE;
			}
			break;

		case ID_VIDEO_CONFIG:
			if (!guistatus.IsFullScreen)
			{
				if(emustatus.Emu_Is_Running)
				{
					/* SuspendThread(CPUThreadHandle); */
					HideCursor(FALSE);
					VIDEO_DllConfig(hWnd);

					/* ResumeThread(CPUThreadHandle); */
				}
				else
				{
					HideCursor(FALSE);
					VIDEO_DllConfig(hWnd);
					NewRomList_ListViewChangeWindowRect();
				}

				DockStatusBar();
			}
			break;
		case ID_AUD_CONFIG:
			if (!guistatus.IsFullScreen)
			if(emustatus.Emu_Is_Running)
			{
				SuspendThread(CPUThreadHandle);
				HideCursor(FALSE);
				AUDIO_DllConfig(hWnd);
				ResumeThread(CPUThreadHandle);
			}
			else
			{
				HideCursor(FALSE);
				AUDIO_DllConfig(hWnd);
			}
			break;
		case ID_DI_CONFIG:
			if (!guistatus.IsFullScreen)
			if(emustatus.Emu_Is_Running)
			{
				/* SuspendThread(CPUThreadHandle); */
				HideCursor(FALSE);
				CONTROLLER_DllConfig(hWnd);
				/* ResumeThread(CPUThreadHandle); */
			}
			else
			{
				HideCursor(FALSE);
				CONTROLLER_DllConfig(hWnd);
			}
			break;
		case ID_RSP_CONFIG:
			if (!guistatus.IsFullScreen)
			RSPDllConfig(hWnd);
			break;
		case ID_INTERPRETER:
			CheckMenuItem(gui.hMenu1964main, ID_INTERPRETER, MF_CHECKED);
			CheckMenuItem(gui.hMenu1964main, ID_STATICCOMPILER, MF_UNCHECKED);
			CheckMenuItem(gui.hMenu1964main, ID_DYNAMICCOMPILER, MF_UNCHECKED);
			EmulatorSetCore(INTERPRETER);
			break;
		case ID_STATICCOMPILER:
			CheckMenuItem(gui.hMenu1964main, ID_INTERPRETER, MF_UNCHECKED);
			CheckMenuItem(gui.hMenu1964main, ID_STATICCOMPILER, MF_CHECKED);
			CheckMenuItem(gui.hMenu1964main, ID_DYNAMICCOMPILER, MF_UNCHECKED);
			EmulatorSetCore(1);
			break;
		case ID_DYNAMICCOMPILER:
			CheckMenuItem(gui.hMenu1964main, ID_INTERPRETER, MF_UNCHECKED);
			CheckMenuItem(gui.hMenu1964main, ID_STATICCOMPILER, MF_UNCHECKED);
			CheckMenuItem(gui.hMenu1964main, ID_DYNAMICCOMPILER, MF_CHECKED);
			EmulatorSetCore(DYNACOMPILER);
			break;
		case ID_CF_CF1:
			SetCounterFactor(1);
			break;
		case ID_CF_CF2:
			SetCounterFactor(2);
			break;
		case ID_CF_CF3:
			SetCounterFactor(3);
			break;
		case ID_CF_CF4:
			SetCounterFactor(4);
			break;
		case ID_CF_CF5:
			SetCounterFactor(5);
			break;
		case ID_CF_CF6:
			SetCounterFactor(6);
			break;
		case ID_CF_CF7:
			SetCounterFactor(7);
			break;
		case ID_CF_CF8:
			SetCounterFactor(8);
			break;
		case ID_CPU_DYNACODECHECKING_NOCHECK:
			SetCodeCheckMethod(1);
			break;
		case ID_CPU_DYNACODECHECKING_DMA:
			SetCodeCheckMethod(2);
			break;
		case ID_CPU_DYNACODECHECKING_DWORD:
			SetCodeCheckMethod(3);
			break;
		case ID_CPU_DYNACODECHECKING_QWORD:
			SetCodeCheckMethod(4);
			break;
		case ID_CPU_DYNACODECHECKING_QWORDANDDMA:
			SetCodeCheckMethod(5);
			break;
		case ID_CPU_DYNACODECHECKING_BLOCK:
			SetCodeCheckMethod(6);
			break;
		case ID_CPU_DYNACODECHECKING_BLOCKANDDMA:
			SetCodeCheckMethod(7);
			break;
		case ID_CPU_DYNACODECHECKING_PROTECTMEMORY:
			SetCodeCheckMethod(8);
			break;
		case ID_BUTTON_FULL_SCREEN:
		case IDM_FULLSCREEN:
			if(emustatus.Emu_Is_Running)
			{
				if(emustatus.Emu_Is_Paused && !guistatus.IsFullScreen)
					ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
				VIDEO_ChangeWindow(guistatus.IsFullScreen);
			}
			break;
		case ID_PLUGINS_SCREENSHOTS:
			CaptureScreenToFile();
			break;
		case ID_TOGGLECURSOR:
			if(emustatus.Emu_Is_Running)
				HideCursor(showcursor);
			break;
		case IDM_PLUGINS:
		case ID_BUTTON_SETUP_PLUGINS:
			if (!guistatus.IsFullScreen)
			DialogBox(gui.hInst, "PLUGINS", hWnd, (DLGPROC) PluginsDialog);
			break;
		case ID_CHECKWEB:
		case ID_ONLINE_HELP:
		case ID_BUTTON_HELP:
			if (!guistatus.IsFullScreen)
			MessageBox(NULL, "If you are having issues, please read the file BUNDLE_README.txt located in the 1964 directory.", "Help", MB_OK);
			break;
		case ID_CONFIGURE_VIDEO:
			VIDEO_DllConfig(hWnd);
			break;
		case ID_HELP_FINDER:
			DisplayError("Help contents");
			break;
		case ID_OVERCLOCKSTOCK:
			SetOverclockFactor(1);
			SetCounterFactor(currentromoptions.Counter_Factor);
			break;
		case ID_OVERCLOCK6:
		case ID_OVERCLOCK9:
		case ID_OVERCLOCK12:
		case ID_OVERCLOCK15:
		case ID_OVERCLOCK18:
			SetOverclockFactor((LOWORD(wParam) - ID_OVERCLOCK6 + 2) * 3);
			SetCounterFactor(COUTERFACTOR_1);
			break;
		case ID_PDSPEEDHACK:
			if(emuoptions.PDSpeedHack)
			{
				CheckMenuItem(gui.hMenu1964main, ID_PDSPEEDHACK, MF_UNCHECKED);
				emuoptions.PDSpeedHack = FALSE;
			}
			else
			{
				CheckMenuItem(gui.hMenu1964main, ID_PDSPEEDHACK, MF_CHECKED);
				emuoptions.PDSpeedHack = TRUE;
			}
			emuoptions.PDSpeedHackBoost = FALSE;
			CheckMenuItem(gui.hMenu1964main, ID_PDSPEEDHACKBOOST, MF_UNCHECKED);
			EnableMenuItem(gui.hMenu1964main, ID_PDSPEEDHACKBOOST, emuoptions.PDSpeedHack && emuoptions.OverclockFactor != 1 ? MF_ENABLED : MF_GRAYED);
			break;
		case ID_PDSPEEDHACKBOOST:
			if(emuoptions.PDSpeedHackBoost)
			{
				CheckMenuItem(gui.hMenu1964main, ID_PDSPEEDHACKBOOST, MF_UNCHECKED);
				emuoptions.PDSpeedHackBoost = FALSE;
			}
			else
			{
				CheckMenuItem(gui.hMenu1964main, ID_PDSPEEDHACKBOOST, MF_CHECKED);
				emuoptions.PDSpeedHackBoost = TRUE;
				MessageBox(gui.hwnd1964main, "The PD Speed-Hack can be unstable when injected too quickly.\nIf you experience clock skips or instant deaths with PD, turn off\nBoost Speed-Hack Freq.", "Information", MB_ICONINFORMATION | MB_OK);
			}
			break;
		case ID_ABOUT_WARRANTY:
			if (!guistatus.IsFullScreen)
			{
				LoadString(gui.hInst, IDS_WARRANTY_SEC11, WarrantyPart1, 700);
				LoadString(gui.hInst, IDS_WARRANTY_SEC12, WarrantyPart2, 700);
				MessageBox(gui.hwnd1964main, WarrantyPart1, "NO WARRANTY", MB_OK);
				MessageBox(gui.hwnd1964main, WarrantyPart2, "NO WARRANTY", MB_OK);
			}
			break;

		case ID_REDISTRIBUTE:
			if (!guistatus.IsFullScreen)
			DialogBox(gui.hInst, "REDISTRIB_DIALOG", hWnd, (DLGPROC) ConditionsDialog);
			break;

		case ID_OPCODEDEBUGGER:
		case ID_OPCODEDEBUGGER_BLOCK_ONLY:
		case ID_DIRTYONLY:
			OnOpcodeDebuggerCommands(hWnd, message, wParam, lParam);
			break;
		case ID_SAVE_STATE_1:
		case ID_SAVE_STATE_2:
		case ID_SAVE_STATE_3:
		case ID_SAVE_STATE_4:
		case ID_SAVE_STATE_5:
		case ID_SAVE_STATE_6:
		case ID_SAVE_STATE_7:
		case ID_SAVE_STATE_8:
		case ID_SAVE_STATE_9:
		case ID_SAVE_STATE_0:
			SaveStateByNumber(wParam);
			break;
		case ID_LOAD_STATE_1:
		case ID_LOAD_STATE_2:
		case ID_LOAD_STATE_3:
		case ID_LOAD_STATE_4:
		case ID_LOAD_STATE_5:
		case ID_LOAD_STATE_6:
		case ID_LOAD_STATE_7:
		case ID_LOAD_STATE_8:
		case ID_LOAD_STATE_9:
		case ID_LOAD_STATE_0:
			LoadStateByNumber(wParam);
			break;
		case ID_SAVESTATE:
			if (!guistatus.IsFullScreen)
			SaveStateByDialog(SAVE_STATE_1964_FORMAT);
			break;
		case ID_LOADSTATE:
			if (!guistatus.IsFullScreen)
			LoadStateByDialog(SAVE_STATE_1964_FORMAT);
			break;
		case ID_CPU_IMPORTPJ64STATE:
			if (!guistatus.IsFullScreen)
			LoadStateByDialog(SAVE_STATE_PJ64_FORMAT);
			break;
		case ID_CPU_EXPORTPJ64STATE:
			if (!guistatus.IsFullScreen)
			SaveStateByDialog(SAVE_STATE_PJ64_FORMAT);
			break;
		case ID_NETPLAY_CONFIG:
			netplay_dll_config(hWnd, NETPLAY_OPTIONS);
			break;
		case ID_NETPLAY_ADDAREMOTEPLAYER:
			netplay_dll_config(hWnd, NETPLAY_USER_MANAGEMENT);
			break;
		case ID_NETPLAY_DROPAREMOTEPLAYER:
			netplay_dll_config(hWnd, NETPLAY_USER_MANAGEMENT);
			break;
		case ID_NETPLAY_ENABLENETPLAY:
			netplay_dll_config(hWnd, NETPLAY_NETWORK_MANAGEMENT);
			break;
		case ID_POPUP_LOADPLAY:
		case ID_POPUP_LOADPLAYINFULLSCREEN:
		case ID_POPUP_LOADPLAYINWINDOWMODE:
		case ID_POPUP_ROM_SETTING:
		case ID_POPUP_CHEATCODE:
		case ID_HEADERPOPUP_SHOW_INTERNAL_NAME:
		case ID_HEADERPOPUP_SHOWALTERNATEROMNAME:
		case ID_HEADERPOPUP_SHOWROMFILENAME:
		case ID_HEADERPOPUP_1_SORT_ASCENDING:
		case ID_HEADERPOPUP_1_SORT_DESCENDING:
		case ID_HEADERPOPUP_2_SORT_ASCENDING:
		case ID_HEADERPOPUP_2_SORT_DESCENDING:
		case ID_HEADERPOPUP_1_SELECTING:
		case ID_HEADERPOPUP_2_SELECTING:
			OnPopupMenuCommand(hWnd, message, wParam, lParam);
			break;
		case ID_EXIT:
			KillTimer(hWnd, timer);
			Exit1964();
			break;
		default:
#ifdef DEBUG_COMMON
			ProcessDebugMenuCommand(wParam);
#endif
			break;
		}
}

void ProcessToolTips(LPARAM lParam)
{
    LPTOOLTIPTEXT lpttt; 

    lpttt = (LPTOOLTIPTEXT) lParam; 
    lpttt->hinst = gui.hInst; 

    // Specify the resource identifier of the descriptive 
    // text for the given button. 
    switch (lpttt->hdr.idFrom) 
	{ 
		case ID_BUTTON_OPEN_ROM:
			lpttt->lpszText = MAKEINTRESOURCE(ID_BUTTON_OPEN_ROM); 
			break;

		case ID_BUTTON_PLAY:
			lpttt->lpszText = MAKEINTRESOURCE(ID_BUTTON_PLAY);
			break; 

		case ID_BUTTON_PAUSE:
			lpttt->lpszText = MAKEINTRESOURCE(ID_BUTTON_PAUSE);
			break; 

		case ID_BUTTON_STOP:
			lpttt->lpszText = MAKEINTRESOURCE(ID_BUTTON_STOP);
			break;

		case ID_BUTTON_SETUP_PLUGINS:
			lpttt->lpszText = MAKEINTRESOURCE(ID_BUTTON_SETUP_PLUGINS);
			break;

		case ID_BUTTON_ROM_PROPERTIES:
			lpttt->lpszText = MAKEINTRESOURCE(ID_BUTTON_ROM_PROPERTIES); 
            break;

		case ID_BUTTON_HELP:
			lpttt->lpszText = MAKEINTRESOURCE(ID_BUTTON_HELP); 
            break; 

		case ID_BUTTON_FULL_SCREEN:
			lpttt->lpszText = MAKEINTRESOURCE(ID_BUTTON_FULL_SCREEN); 
            break;
    }
}

void OnWindowSize(WPARAM wParam)
{
	RECT rcStatusBar;
	RECT rcRomList;
	RECT rcToolBar;

	if(gui.hToolBar != NULL)
	{
		RECT rcMainWnd;

		GetClientRect(gui.hToolBar, &rcToolBar);
		GetWindowRect(gui.hwnd1964main, &rcMainWnd);
		SendMessage(gui.hReBar, WM_SIZE, 0, 0); //This will resize it, but there's bad flicker.
	}

	GetWindowRect(gui.hStatusBar, &rcStatusBar);

	if (gui.hwndRomList != NULL)
	{
		GetClientRect(gui.hwnd1964main, &rcRomList);
		if(gui.hToolBar != NULL)
		{
			rcRomList.top += (rcToolBar.bottom - rcToolBar.top - 1);
			rcRomList.bottom -= (rcToolBar.bottom - rcToolBar.top - 1);
		}
		rcRomList.bottom -= (rcStatusBar.bottom - rcStatusBar.top);
		SetWindowPos(gui.hwndRomList, HWND_BOTTOM, 0, rcRomList.top, rcRomList.right, rcRomList.bottom, 0);
		UpdateWindow(gui.hwndRomList);
	}

	DockStatusBar();

	if(wParam == SIZE_MAXIMIZED && !guistatus.window_is_maximized)
	{
		guistatus.window_is_maximized = TRUE;
		DockStatusBar();
	}
    else if(wParam == SIZE_MINIMIZED && !guistatus.window_is_minimized)
	{
		guistatus.window_is_minimized = TRUE;
	}
	else if(guistatus.window_is_maximized || guistatus.window_is_minimized)
	{
		DockStatusBar();
	}
}

void ProcessKeyboardInput(UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL				ctrlkey;
	BOOL				shiftkey;
	/* Disable Alt Key for menus in full screen */
     BYTE keymap[256];

    if ((wParam == VK_CONTROL) && (guistatus.IsFullScreen))
    {
		GetKeyboardState (keymap);
        keymap [(lParam & 0x1000000) ? VK_RCONTROL:VK_LCONTROL] &= ~0x80;
        SetKeyboardState (keymap);
    }
    else if (wParam == VK_MENU)
    {
		GetKeyboardState (keymap);
        keymap [(lParam & 0x1000000) ? VK_RMENU:VK_LMENU] &= ~0x80;
        SetKeyboardState (keymap);
    }

    if (message == WM_SYSKEYUP && wParam == VK_MENU) {
        /* ignore ALT key up event to stop it activating the menus */
		return;
    }

	ctrlkey = GetKeyState(VK_CONTROL) & 0xFF000000;
	shiftkey = GetKeyState(VK_LSHIFT) & 0xFF000000;
	switch(wParam)
	{
	case VK_F5:
		SaveState();
	break;
	
	case VK_F7:
		LoadState();
	break;
	
	case VK_F4:
	case VK_ESCAPE:
		if (guistatus.IsFullScreen)
		if(emustatus.Emu_Is_Running)
		{
			if(PauseEmulator())
			{
				VIDEO_ChangeWindow(guistatus.IsFullScreen);
				ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
			}
		}
		if (wParam == VK_F4) 
			CloseROM();
	break;

	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
		if(shiftkey && !ctrlkey)
		{
			StateSetNumber(wParam - 0x30);
			MessageBeep(MB_ICONASTERISK);
		}
	break;
	
	default:
		CONTROLLER_WM_KeyUp(wParam, lParam);
	break;
	}
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
long FAR PASCAL MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	static PAINTSTRUCT	ps;
	static int			ok = 0;
	static BOOL			gamepausebyinactive = FALSE;	/* static for this looks like a bad idea. */
	static int			MenuCausedPause = FALSE;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	switch(message)
	{
	case WM_ACTIVATE:
		if(guioptions.pause_at_inactive)
		{
			BOOL	minimize = (BOOL) HIWORD(wParam);

			switch(LOWORD(wParam))
			{
			case WA_ACTIVE:
			case WA_CLICKACTIVE:
				if( emustatus.Emu_Is_Running && 
				    emustatus.Emu_Is_Paused  &&
				    gamepausebyinactive )
				{
					Resume();
					gamepausebyinactive = FALSE;
				}
			break;
			
			case WA_INACTIVE:
				if( minimize && 
				    emustatus.Emu_Is_Running &&
				    emustatus.Emu_Is_Paused == FALSE )
				{
					Pause();
					gamepausebyinactive = TRUE;
				}
			break;
			}
		}
	break;

	case WM_PAINT:
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	break;

	case WM_ACTIVATEAPP:
		ActiveApp = wParam;
	break;

	case WM_SETFOCUS:
		ActiveApp = wParam;
	break;

	case WM_MOVE:
		if(emustatus.Emu_Is_Running)
		{
			/*~~~~~~~~~~~*/
			int xPos, yPos;
			/*~~~~~~~~~~~*/

			xPos = (int) (short) LOWORD(lParam);		/* horizontal position */
			yPos = (int) (short) HIWORD(lParam);		/* vertical position */
			VIDEO_MoveScreen(xPos, yPos);
		}
	break;
	
	case WM_EXITSIZEMOVE:
		guistatus.window_is_moving = FALSE;
	break;
		
	case WM_SIZE:
		OnWindowSize(wParam);
	break;

	case WM_MOVING:
		guistatus.window_is_moving = TRUE;
	break;

	case WM_KEYDOWN:
		CONTROLLER_WM_KeyDown(wParam, lParam);
	break;
	
	case WM_KEYUP:
    case WM_SYSKEYUP:
		ProcessKeyboardInput(message, wParam, lParam);
	break;

	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) 
		{ 
		    case TTN_GETDISPINFO: 
                ProcessToolTips(lParam);
				break;
		}
		if(((LPNMHDR) lParam)->hwndFrom == gui.hwndRomList)
		{
			OnNotifyRomList(hWnd, message, wParam, lParam);
		}
		else if(((LPNMHDR) lParam)->hwndFrom == gui.hStatusBar )
		{
			OnNotifyStatusBar(hWnd, message, wParam, lParam);
		}
		else if(((LPNMHDR) lParam)->hwndFrom == ListView_GetHeader(gui.hwndRomList) )
		{
			OnNotifyRomListHeader(hWnd, message, wParam, lParam);
		}
		else
		{
			return(DefWindowProc(hWnd, message, wParam, lParam));
		}
	break;

	case WM_COMMAND:
		ProcessMenuCommand(hWnd, message, wParam, lParam);
	break;

	case WM_ENTERMENULOOP:
		/* To pause game when user enter the menu bar */
		MenuCausedPause = emustatus.Emu_Is_Paused;
		if(guioptions.ok_to_pause_at_menu)
			if(emustatus.Emu_Is_Running && !emustatus.Emu_Is_Paused )
			{
				PauseEmulator();

			}
		emustatus.Emu_Is_Paused = MenuCausedPause;
	break;

	case WM_EXITMENULOOP:
		/* To resume game when user leaves the menu bar */
		if(guioptions.ok_to_pause_at_menu)
			if(emustatus.Emu_Is_Running && !MenuCausedPause )
			{
				ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
			}
	break;

	case WM_CLOSE:
		KillTimer(hWnd, timer);
		Exit1964();
	break;

	case WM_POWERBROADCAST:
		switch( wParam )
		{
#ifndef PBT_APMQUERYSUSPEND
#define PBT_APMQUERYSUSPEND 0x0000
#endif
		case PBT_APMQUERYSUSPEND:
			// At this point, the app should save any data for open
			// network connections, files, etc., and prepare to go into
			// a suspended mode.
			return TRUE;
			
#ifndef PBT_APMRESUMESUSPEND
#define PBT_APMRESUMESUSPEND 0x0007
#endif
		case PBT_APMRESUMESUSPEND:
			// At this point, the app should recover any data, network
			// connections, files, etc., and resume running from when
			// the app was suspended.
			return TRUE;
		}
	break;

	case WM_SYSCOMMAND:
		switch (wParam) 
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			if(emustatus.Emu_Is_Running)
				return 1;	//Disable screen saver
		case SC_MOVE:
		case SC_SIZE:
		case SC_MAXIMIZE:
		case SC_KEYMENU:
			if (guistatus.IsFullScreen)
				return 1;
		default:
			return(DefWindowProc(hWnd, message, wParam, lParam));
		}
	break;
	}
	
	return(DefWindowProc(hWnd, message, wParam, lParam));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Pause(void)
{
//	if(emustatus.Emu_Is_Running)
//	{
//		if(!emustatus.Emu_Is_Paused)
//		{
			PauseEmulator();
//		}
//		else
//		{
//			ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
//			sprintf(generalmessage, "%s - Running", gui.szWindowTitle);
//			SetWindowText(gui.hwnd1964main, generalmessage);
//		}
//	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Resume(void)
{
	if(emustatus.Emu_Is_Running && emustatus.Emu_Is_Paused)
	{
		ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
	}
}

void AfterStop(void);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void KillCPUThread(void)
{
	if(emustatus.Emu_Is_Running)
	{
		SuspendThread(CPUThreadHandle);
		TerminateThread(CPUThreadHandle, 1);
		CloseHandle(CPUThreadHandle);

		if(currentromoptions.Code_Check == CODE_CHECK_PROTECT_MEMORY) UnprotectAllBlocks();

		AUDIO_RomClosed();
		CONTROLLER_RomClosed();
		VIDEO_RomClosed();
		netplay_rom_closed();

		AfterStop();
	}

	//To finish off the window placement and settings
	Rom_Loaded = TRUE;
	CloseROM(); 
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Kill(void)
{
	StopEmulator();
	CPUThreadHandle = NULL;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void Play(BOOL WithFullScreen)
{
	/*~~~~~*/
	int core;
	/*~~~~~*/

	if(Rom_Loaded)
	{
		if(emustatus.Emu_Is_Running) 
		{
			Stop();
		}

		PrepareBeforePlay(guistatus.IsFullScreen);

		core = currentromoptions.Emulator;
		if(core == DYNACOMPILER)
		{					/* Dynarec */
			CheckMenuItem(gui.hMenu1964main, ID_INTERPRETER, MF_UNCHECKED);
			CheckMenuItem(gui.hMenu1964main, ID_DYNAMICCOMPILER, MF_CHECKED);
			emustatus.cpucore = DYNACOMPILER;
		}
		else
		{					/* Interpreter */
			CheckMenuItem(gui.hMenu1964main, ID_INTERPRETER, MF_CHECKED);
			CheckMenuItem(gui.hMenu1964main, ID_DYNAMICCOMPILER, MF_UNCHECKED);
			emustatus.cpucore = INTERPRETER;
		}

		if ((GFX_PluginRECT.UseThis == TRUE) && (emustatus.Emu_Is_Resetting == FALSE))
		{
			RECT Rect;

			GetWindowRect(gui.hwnd1964main, &Rect);
			SetWindowPos
			(
			gui.hwnd1964main,
			NULL,
			Rect.left,
			Rect.top,
			GFX_PluginRECT.rect.right - GFX_PluginRECT.rect.left,
			GFX_PluginRECT.rect.bottom - GFX_PluginRECT.rect.top,
			SWP_NOZORDER | SWP_SHOWWINDOW
			);
		}

		if (emustatus.Emu_Is_Resetting == 0)
			DockStatusBar();

		r4300i_Reset();
		RunEmulator(emustatus.cpucore);

		//EnableMenuItem(gui.hMenu1964main, ID_OPENROM, MF_GRAYED);
		EnableMenuItem(gui.hMenu1964main, IDM_PLUGINS, MF_GRAYED);
		EnableButton(ID_BUTTON_SETUP_PLUGINS, FALSE);

		if(Kaillera_Is_Running == FALSE)
		{
			EnableMenuItem(gui.hMenu1964main, ID_ROM_PAUSE, MF_ENABLED);
			EnableRadioButtons(TRUE);
			CheckButton(ID_BUTTON_PLAY, TRUE);
			EnableMenuItem(gui.hMenu1964main, ID_ROM_STOP, MF_ENABLED);
			EnableStateMenu();
		}
		else
		{
			EnableMenuItem(gui.hMenu1964main, ID_FILE_CHEAT, MF_GRAYED);
			EnableMenuItem(gui.hMenu1964main, ID_CLOSEROM, MF_GRAYED);
			EnableRadioButtons(FALSE);
			EnableMenuItem(gui.hMenu1964main, ID_ROM_PAUSE, MF_GRAYED);

			/* EnableMenuItem(gui.hMenu1964main, ID_ROM_STOP, MF_GRAYED); */
		}

		if(GfxPluginVersion == 0x0103)
		{
			EnableMenuItem(gui.hMenu1964main, ID_PLUGINS_SCREENSHOTS, MF_ENABLED);
		}
		else
		{
			EnableMenuItem(gui.hMenu1964main, ID_PLUGINS_SCREENSHOTS, MF_GRAYED);
		}

		sprintf(generalmessage, "%s - Running", gui.szWindowTitle);
		SetWindowText(gui.hwnd1964main, generalmessage);
		//if(emuoptions.auto_full_screen)
		if(WithFullScreen && (emustatus.Emu_Is_Resetting == 0))
		{
			if(guistatus.IsFullScreen == 0)
			{
				VIDEO_ChangeWindow(guistatus.IsFullScreen);
			}
		}
	}
	else
		DisplayError("Please load a ROM first.");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Stop()
{
	if(emustatus.Emu_Is_Running)
	{
		if(Kaillera_Is_Running == TRUE)
		{
			Kaillera_Is_Running = FALSE;
			kailleraEndGame();
		}

		StopEmulator();

		if (emustatus.Emu_Is_Resetting == 0)
		{
			if(guistatus.IsFullScreen)
			{
				VIDEO_ChangeWindow(guistatus.IsFullScreen);
			}
			HideCursor(FALSE);
			AfterStop();
		}
	}
	else if(Is_Reading_Rom_File)
	{
		To_Stop_Reading_Rom_File = TRUE;
	}
}

/*
 =======================================================================================================================
    Kaillera Stuff £
 =======================================================================================================================
 */
int WINAPI kailleraGameCallback(char *game, int player, int numplayers)
{
	/*~~*/
	int i;
	/*~~*/

	Kaillera_Is_Running = TRUE;
	Kaillera_Players = numplayers;
	Kaillera_Counter = 0;

	for(i = 0; i < romlist_count; i++)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		char			szRom[50];
		ROMLIST_ENTRY	*entry = romlist[i];
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		sprintf
		(
			szRom,
			"%s (%X-%X:%c)",
			entry->pinientry->Game_Name,
			entry->pinientry->crc1,
			entry->pinientry->crc2,
			entry->pinientry->countrycode
		);

		if(strcmp(szRom, game) == 0)
		{
			RomListOpenRom(i, 1);
		}
	}

	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WINAPI kailleraChatReceivedCallback(char *nick, char *text)
{
	/* Do what you want with this :) */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WINAPI kailleraClientDroppedCallback(char *nick, int playernb)
{
	/* Do what you want with this :) */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WINAPI kailleraMoreInfosCallback(char *gamename)
{
	/* Do what you want with this :) */
}

char	szKailleraNamedRoms[50 * MAX_ROMLIST];
/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RomListGetGoodRomNameToDisplay(char *buf, int index)
{
	switch( romlistNameToDisplay )
	{
	case ROMLIST_DISPLAY_ALTER_NAME:
		if(strlen(romlist[index]->pinientry->Alt_Title) > 1)
		{
			strcpy(buf, romlist[index]->pinientry->Alt_Title);
			break;
		}
	case ROMLIST_DISPLAY_INTERNAL_NAME:
		if(InternalNameIsValid(romlist[index]->pinientry->Game_Name))
		{
			strcpy(buf, romlist[index]->pinientry->Game_Name);
			break;
		}		
	case ROMLIST_DISPLAY_FILENAME:
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			char	drive[_MAX_DIR], dir[_MAX_DIR];
			char	fname[_MAX_DIR], ext[_MAX_EXT];
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			
			_splitpath(romlist[index]->romfilename, drive, dir, fname, ext);
			strcat(fname, ext);
			strcpy(buf, fname);
		}
		break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void KailleraPlay(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int				i;
	kailleraInfos	kInfos;
	char			*pszKailleraNamedRoms = szKailleraNamedRoms;
	int				saved_romlist_sort_method;
	int				saved_romlistNameToDisplay;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/* build roms list :) */
	
	saved_romlistNameToDisplay = romlistNameToDisplay;
	romlistNameToDisplay = ROMLIST_DISPLAY_INTERNAL_NAME;
	*pszKailleraNamedRoms = '\0';
	
	saved_romlist_sort_method = romlist_sort_method;
	romlist_sort_method = ROMLIST_GAMENAME;
	NewRomList_Sort();
	romlist_sort_method = saved_romlist_sort_method;


	for(i = 0; i < romlist_count; i++)
	{
		char			szRom[50];
		char			szAlt[50];
		ROMLIST_ENTRY	*entry = romlist[i];

		RomListGetGoodRomNameToDisplay(szAlt, i);
		if (szAlt[0] >= 'a' && szAlt[0] <= 'z') 
			szAlt[0] -= 32;
		szAlt[20] = '\0';

		sprintf
		(
			szRom,
			"%s (%X-%X:%c)",
			szAlt,
			entry->pinientry->crc1,
			entry->pinientry->crc2,
			entry->pinientry->countrycode
		);

		strncpy(pszKailleraNamedRoms, szRom, strlen(szRom) + 1);
		pszKailleraNamedRoms += strlen(szRom) + 1;

	}

	romlistNameToDisplay = saved_romlistNameToDisplay;
	*(++pszKailleraNamedRoms) = '\0';

	kInfos.appName = CURRENT1964VERSION;
	kInfos.gameList = szKailleraNamedRoms;
	kInfos.gameCallback = kailleraGameCallback;
	kInfos.chatReceivedCallback = kailleraChatReceivedCallback;
	kInfos.clientDroppedCallback = kailleraClientDroppedCallback;
	kInfos.moreInfosCallback = kailleraMoreInfosCallback;

	// Lock some menu items 
	EnableMenuItem(gui.hMenu1964main, ID_KAILLERA_MODE, MF_GRAYED);

	kailleraInit();
	kailleraSetInfos(&kInfos);
	kailleraSelectServerDialog(gui.hwnd1964main);

	// Stop emulator if running
	Stop();

	// Unlock menu items
	EnableMenuItem(gui.hMenu1964main, ID_KAILLERA_MODE, (IsKailleraDllLoaded())? MF_ENABLED:MF_GRAYED);

}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CloseROM(void)
{
	WINDOWPLACEMENT placement;

	GetWindowPlacement(gui.hwnd1964main, &placement);
	
	if(emustatus.Emu_Is_Running)
	{
		Stop();

		if (placement.showCmd == SW_SHOWMAXIMIZED)
		{
			guistatus.WindowIsMaximized = TRUE;
			ShowWindow(gui.hwnd1964main, SW_SHOWNORMAL);
			ShowWindow(gui.hwnd1964main, SW_SHOWMAXIMIZED);
		}

		Close_iPIF();
		FreeVirtualRomMemory();
		r4300i_Init();

		Rom_Loaded = FALSE;

		EnableMenuItem(gui.hMenu1964main, ID_OPENROM, MF_ENABLED);
		EnableButton(ID_BUTTON_OPEN_ROM, TRUE);
		EnableMenuItem(gui.hMenu1964main, IDM_PLUGINS, MF_ENABLED);
		EnableButton(ID_BUTTON_SETUP_PLUGINS, TRUE);
		EnableRadioButtons(FALSE);

		EnableMenuItem(gui.hMenu1964main, ID_ROM_PAUSE, MF_GRAYED);

		/* EnableMenuItem(gui.hMenu1964main, ID_ROM_STOP, MF_GRAYED); */
		EnableMenuItem(gui.hMenu1964main, ID_CLOSEROM, MF_GRAYED);
		EnableMenuItem(gui.hMenu1964main, ID_FILE_ROMINFO, MF_GRAYED);
		EnableButton(ID_BUTTON_ROM_PROPERTIES, FALSE);
		EnableMenuItem(gui.hMenu1964main, ID_FILE_CHEAT, MF_GRAYED);

		SetWindowText(gui.hwnd1964main, gui.szBaseWindowTitle);
	}
	HideCursor(FALSE);
	/*
	 * else
	 * DisplayError("Please load a ROM first.");
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void OpenROM(void)
{
	if(Rom_Loaded) 
	{
		CloseROM();
	}

	if(WinLoadRom() == TRUE)	/* If the user opened a rom, */
	{
		EnableRadioButtons(TRUE);
		EnableMenuItem(gui.hMenu1964main, ID_CLOSEROM, MF_ENABLED);
		EnableMenuItem(gui.hMenu1964main, ID_FILE_ROMINFO, MF_ENABLED);
		EnableButton(ID_BUTTON_ROM_PROPERTIES, TRUE);
		EnableMenuItem(gui.hMenu1964main, ID_FILE_CHEAT, MF_ENABLED);

		Play(emuoptions.auto_full_screen); /* autoplay */
	}
	else
	{
		EnableButton(ID_BUTTON_OPEN_ROM, TRUE);
		EnableMenuItem(gui.hMenu1964main, ID_OPENROM, MF_ENABLED);
		EnableButton(ID_BUTTON_SETUP_PLUGINS, TRUE);
		EnableMenuItem(gui.hMenu1964main, IDM_PLUGINS, MF_ENABLED);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL WinLoadRom(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	OPENFILENAME	ofn;
	char			szFileName[MAXFILENAME];
	char			szFileTitle[MAXFILENAME];
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	memset(&szFileName, 0, sizeof(szFileName));
	memset(&szFileTitle, 0, sizeof(szFileTitle));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = gui.hwnd1964main;
	ofn.lpstrFilter = "N64 ROMs (*.ZIP, *.V64, *.BIN, *.ROM, *.Z64, *.N64, *.USA, *.PAL, *.J64)\0*.ZIP;*.V64;*.BIN;*.ROM;*.Z64;*.N64;*.USA;*.PAL;*.J64\0All Files (*.*)\0*.*\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAXFILENAME;
	ofn.lpstrInitialDir = directories.rom_directory_to_use;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = MAXFILENAME;
	ofn.lpstrTitle = "Open ROM";
	ofn.lpstrDefExt = "TXT";
	ofn.Flags = OFN_HIDEREADONLY;

	if(!GetOpenFileName((LPOPENFILENAME) & ofn))
	{
		return FALSE;
	}

	_getcwd(directories.rom_directory_to_use, MAX_PATH);
	if ( strcmp(directories.last_rom_directory, directories.rom_directory_to_use) != 0)
	{ 
		NeedFreshromListAfterStop = TRUE;
	}
	strcpy(directories.last_rom_directory, directories.rom_directory_to_use);

	WriteConfiguration();

	if(WinLoadRomStep2(szFileName))
	{
		/*~~~~~~~~~~~~~~~~~*/
		INI_ENTRY	*pentry;
		long		filesize;
		/*~~~~~~~~~~~~~~~~~*/

		/* Check and create romlist entry for this new loaded rom */
		pentry = GetNewIniEntry();
		ReadRomHeaderInMemory(pentry);
		filesize = ReadRomHeader(szFileName, pentry);
		RomListAddEntry(pentry, szFileName, filesize);
		DeleteIniEntryByEntry(pentry);

		/* Read hack code for this rom */
		CodeList_ReadCode(rominfo.name);
		RefreshRecentGameMenus(szFileName);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*
=======================================================================================================================
=======================================================================================================================
*/
BOOL WinLoadRomStep2(char *szFileName)
{
	lstrcpy(gui.szWindowTitle, gui.szBaseWindowTitle);
	lstrcat(gui.szWindowTitle, " - ");
	
	if(ReadRomData(szFileName) == FALSE) 
	{
		Rom_Loaded = FALSE;
		return FALSE;
	}
	
	lstrcat(gui.szWindowTitle, rominfo.name);
	
	memcpy(&HeaderDllPass[0], &gMemoryState.ROM_Image[0], 0x40);
	EnableMenuItem(gui.hMenu1964main, IDM_PLUGINS, MF_GRAYED);
	EnableButton(ID_BUTTON_SETUP_PLUGINS, FALSE);
	EnableMenuItem(gui.hMenu1964main, ID_CLOSEROM, MF_ENABLED);
	SetWindowText(gui.hwnd1964main, gui.szWindowTitle);
	
	Rom_Loaded = TRUE;
	gHWS_pc = 0xA4000040;	/* We do it in r4300i_inithardware */
	
	UpdateCIC();
	sprintf(generalmessage, "%s - Loaded", gui.szWindowTitle);
	SetWindowText(gui.hwnd1964main, generalmessage);
	Set_Ready_Message();
	
	return TRUE;
}

BOOL StartGameByCommandLine()
{
	char szFileName[300], temp[300];
	GetCmdLineParameter(CMDLINE_GAME_FILENAME, szFileName);
	if( strlen(szFileName) == 0 )
	{
		return FALSE;
	}
	strcpy(temp, directories.last_rom_directory);
	strcat(temp, "\\");
	strcat(temp, szFileName);
	strcpy(szFileName, temp);

	if(WinLoadRomStep2(szFileName))
	{
		/*~~~~~~~~~~~~~~~~~*/
		INI_ENTRY	*pentry;
		long		filesize;
		char		tempstr[20];
		/*~~~~~~~~~~~~~~~~~*/
		
		/* Check and create romlist entry for this new loaded rom */
		pentry = GetNewIniEntry();
		ReadRomHeaderInMemory(pentry);
		filesize = ReadRomHeader(szFileName, pentry);
		RomListAddEntry(pentry, szFileName, filesize);
		DeleteIniEntryByEntry(pentry);
		
		/* Read hack code for this rom */
		CodeList_ReadCode(rominfo.name);
		RefreshRecentGameMenus(szFileName);

		EnableRadioButtons(TRUE);
		EnableMenuItem(gui.hMenu1964main, ID_CLOSEROM, MF_ENABLED);
		EnableMenuItem(gui.hMenu1964main, ID_FILE_ROMINFO, MF_ENABLED);
		EnableButton(ID_BUTTON_ROM_PROPERTIES, TRUE);
		EnableMenuItem(gui.hMenu1964main, ID_FILE_CHEAT, MF_ENABLED);
		
		GetCmdLineParameter(CMDLINE_FULL_SCREEN_FLAG, tempstr);
		if( strlen(tempstr) > 0 )
		{
			Play(TRUE);
		}
		else
		{
			Play(emuoptions.auto_full_screen); /* autoplay */
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
	
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SetHighResolutionTimer(void)
{
	LONG (NTAPI *_NtQueryTimerResolution) (unsigned long *minimumResolution, unsigned long *maximumResolution, unsigned long *currentResolution) = NULL;
	LONG (NTAPI *_NtSetTimerResolution) (unsigned long desiredResolution, BOOL setResolution, unsigned long *currentResolution) = NULL;
	unsigned long minResolution = 0U, maxResolution = 0U, curResolution = 0U;
	HINSTANCE hinstNTDLL = NULL;
	hinstNTDLL = LoadLibrary("ntdll.dll");
	if(hinstNTDLL != NULL)
	{
		_NtQueryTimerResolution = (LONG(NTAPI *) (unsigned long *minimumResolution, unsigned long *maximumResolution, unsigned long *currentResolution)) GetProcAddress(hinstNTDLL, "NtQueryTimerResolution");
		_NtSetTimerResolution = (LONG(NTAPI *) (unsigned long desiredResolution, BOOL setResolution, unsigned long *currentResolution)) GetProcAddress(hinstNTDLL, "NtSetTimerResolution");
		if(_NtSetTimerResolution != NULL && _NtQueryTimerResolution != NULL)
		{
			_NtQueryTimerResolution(&minResolution, &maxResolution, &curResolution);
			_NtSetTimerResolution(maxResolution, TRUE, &curResolution);
		}
		else
			DisplayError("Error: Could not increase Kernel Timing resolution");
		FreeLibrary(hinstNTDLL);
	}
	else
		DisplayError("Could not load ntdll.dll");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void OpenRecentGame(int id)
{
	if(emustatus.Emu_Is_Running) return;

	if(id >= 0 && id < 8)
	{
		if(WinLoadRomStep2(recent_game_lists[id]))
		{
			/*~~~~~~~~~~~~~~~~~*/
			INI_ENTRY	*pentry;
			long		filesize;
			/*~~~~~~~~~~~~~~~~~*/

			/* Check and create romlist entry for this new loaded rom */
			pentry = GetNewIniEntry();
			ReadRomHeaderInMemory(pentry);
			filesize = ReadRomHeader(recent_game_lists[id], pentry);
			RomListAddEntry(pentry, recent_game_lists[id], filesize);
			DeleteIniEntryByEntry(pentry);

			/* Read hack code for this rom */
			CodeList_ReadCode(rominfo.name);

			EnableRadioButtons(TRUE);
			EnableMenuItem(gui.hMenu1964main, ID_CLOSEROM, MF_ENABLED);
			EnableMenuItem(gui.hMenu1964main, ID_FILE_ROMINFO, MF_ENABLED);
			EnableButton(ID_BUTTON_ROM_PROPERTIES, TRUE);
			EnableMenuItem(gui.hMenu1964main, ID_FILE_CHEAT, MF_ENABLED);

			strcpy(generalmessage, recent_game_lists[id]);
			RefreshRecentGameMenus(generalmessage);

			Play(emuoptions.auto_full_screen); /* autoplay */
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SaveState(void)
{
	if(Rom_Loaded && !emustatus.gepd_pause)
	{
		if(emustatus.Emu_Is_Running)
		{
			if(PauseEmulator())
			{
				sprintf(generalmessage, "%s - Saving State %d", gui.szWindowTitle, StateFileNumber);
				SetStatusBarText(0, generalmessage);

				FileIO_gzSaveState();
				ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
			}
		}
		else
		{
			sprintf(generalmessage, "%s - Saving State %d", gui.szWindowTitle, StateFileNumber);
			SetStatusBarText(0, generalmessage);

			FileIO_gzSaveState();
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void LoadState(void)
{
	/*~~~~~~~~~~~~~~~~~~~~*/
	int was_running = FALSE;
	/*~~~~~~~~~~~~~~~~~~~~*/

	if(Rom_Loaded && !emustatus.gepd_pause)
	{
		if(emustatus.Emu_Is_Running)
		{
			if(PauseEmulator())
			{
				sprintf(generalmessage, "%s - Loading State %d", gui.szWindowTitle, StateFileNumber);
				SetStatusBarText(0, generalmessage);
				FileIO_gzLoadState();
				Init_Count_Down_Counters();
				ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);
			}
		}
		else
		{
			sprintf(generalmessage, "%s - Loading State %d", gui.szWindowTitle, StateFileNumber);
			SetStatusBarText(0, generalmessage);
			FileIO_gzLoadState();
			Init_Count_Down_Counters();
		}
		GEPDPause(TRUE);
	}
}

unsigned int	statesavemenulist[10] =
{
	ID_SAVE_STATE_0,
	ID_SAVE_STATE_1,
	ID_SAVE_STATE_2,
	ID_SAVE_STATE_3,
	ID_SAVE_STATE_4,
	ID_SAVE_STATE_5,
	ID_SAVE_STATE_6,
	ID_SAVE_STATE_7,
	ID_SAVE_STATE_8,
	ID_SAVE_STATE_9
};
unsigned int	stateloadmenulist[10] =
{
	ID_LOAD_STATE_0,
	ID_LOAD_STATE_1,
	ID_LOAD_STATE_2,
	ID_LOAD_STATE_3,
	ID_LOAD_STATE_4,
	ID_LOAD_STATE_5,
	ID_LOAD_STATE_6,
	ID_LOAD_STATE_7,
	ID_LOAD_STATE_8,
	ID_LOAD_STATE_9
};

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void StateSetNumber(int number)
{
	CheckMenuItem(gui.hMenu1964main, statesavemenulist[StateFileNumber], MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, stateloadmenulist[StateFileNumber], MF_UNCHECKED);
	StateFileNumber = number;
	CheckMenuItem(gui.hMenu1964main, statesavemenulist[StateFileNumber], MF_CHECKED);
	CheckMenuItem(gui.hMenu1964main, stateloadmenulist[StateFileNumber], MF_CHECKED);
	sprintf(generalmessage, "%s - Selected State Slot %d", gui.szWindowTitle, number);
	SetStatusBarText(0, generalmessage);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void EnableStateMenu(void)
{
	/*~~*/
	int i;
	/*~~*/

	EnableMenuItem(gui.hMenu1964main, ID_SAVESTATE, MF_ENABLED);
	EnableMenuItem(gui.hMenu1964main, ID_LOADSTATE, MF_ENABLED);
	EnableMenuItem(gui.hMenu1964main, ID_CPU_IMPORTPJ64STATE, MF_ENABLED);
	EnableMenuItem(gui.hMenu1964main, ID_CPU_EXPORTPJ64STATE, MF_ENABLED);

	for(i = 0; i < 10; i++)
	{
		EnableMenuItem(gui.hMenu1964main, statesavemenulist[i], MF_ENABLED);
		EnableMenuItem(gui.hMenu1964main, stateloadmenulist[i], MF_ENABLED);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DisableStateMenu(void)
{
	/*~~*/
	int i;
	/*~~*/

	EnableMenuItem(gui.hMenu1964main, ID_SAVESTATE, MF_GRAYED);
	EnableMenuItem(gui.hMenu1964main, ID_LOADSTATE, MF_GRAYED);
	EnableMenuItem(gui.hMenu1964main, ID_CPU_IMPORTPJ64STATE, MF_GRAYED);
	EnableMenuItem(gui.hMenu1964main, ID_CPU_EXPORTPJ64STATE, MF_GRAYED);

	for(i = 0; i < 10; i++)
	{
		EnableMenuItem(gui.hMenu1964main, statesavemenulist[i], MF_GRAYED);
		EnableMenuItem(gui.hMenu1964main, stateloadmenulist[i], MF_GRAYED);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SaveStateByNumber(WPARAM wparam)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < 10; i++)
	{
		CheckMenuItem(gui.hMenu1964main, statesavemenulist[i], MF_UNCHECKED);
		CheckMenuItem(gui.hMenu1964main, stateloadmenulist[i], MF_UNCHECKED);
		if(statesavemenulist[i] == wparam)
		{
			StateFileNumber = i;
			CheckMenuItem(gui.hMenu1964main, statesavemenulist[i], MF_CHECKED);
			CheckMenuItem(gui.hMenu1964main, stateloadmenulist[i], MF_CHECKED);
		}
	}

	SaveState();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void LoadStateByNumber(WPARAM wparam)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < 10; i++)
	{
		CheckMenuItem(gui.hMenu1964main, statesavemenulist[i], MF_UNCHECKED);
		CheckMenuItem(gui.hMenu1964main, stateloadmenulist[i], MF_UNCHECKED);
		if(stateloadmenulist[i] == wparam)
		{
			StateFileNumber = i;
			CheckMenuItem(gui.hMenu1964main, statesavemenulist[i], MF_CHECKED);
			CheckMenuItem(gui.hMenu1964main, stateloadmenulist[i], MF_CHECKED);
		}
	}

	LoadState();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SaveStateByDialog(int format)
{
	if(!Rom_Loaded) return;
	if(!PauseEmulator())
		return;
	else
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		OPENFILENAME	ofn;
		char			szFileName[MAXFILENAME];
		char			szFileTitle[MAXFILENAME];
		char			szPath[_MAX_PATH];
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		memset(&szFileName, 0, sizeof(szFileName));
		memset(&szFileTitle, 0, sizeof(szFileTitle));
		memset(szPath, 0, _MAX_PATH);

		strcpy(szPath, directories.save_directory_to_use);

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = gui.hwnd1964main;
		if(format == SAVE_STATE_1964_FORMAT)
			ofn.lpstrFilter = "1964 State File (*.sav?)\0*.SAV?\0All Files (*.*)\0*.*\0";
		else
			ofn.lpstrFilter = "Project 64 State File (*.pj?)\0*.PJ;*.PJ?\0All Files (*.*)\0*.*\0";
		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter = 0;
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = szFileName;
		ofn.nMaxFile = MAXFILENAME;
		ofn.lpstrInitialDir = szPath;
		ofn.lpstrFileTitle = szFileTitle;
		ofn.nMaxFileTitle = MAXFILENAME;
		ofn.lpstrTitle = "Save State";
		ofn.lpstrDefExt = "";
		ofn.Flags = OFN_ENABLESIZING | OFN_HIDEREADONLY;

		if(GetSaveFileName((LPOPENFILENAME) & ofn))
		{
			if(format == SAVE_STATE_1964_FORMAT)
				FileIO_gzSaveStateFile(szFileName);
			else
				FileIO_ExportPJ64State(szFileName);
		}

		if(emustatus.Emu_Is_Running) ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void LoadStateByDialog(int format)
{
	if(!Rom_Loaded) return;
	if(!PauseEmulator())
		return;
	else
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		OPENFILENAME	ofn;
		char			szFileName[MAXFILENAME];
		char			szFileTitle[MAXFILENAME];
		char			szPath[_MAX_PATH];
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		memset(&szFileName, 0, sizeof(szFileName));
		memset(&szFileTitle, 0, sizeof(szFileTitle));
		memset(szPath, 0, _MAX_PATH);

		strcpy(szPath, directories.save_directory_to_use);

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = gui.hwnd1964main;
		if(format == SAVE_STATE_1964_FORMAT)
			ofn.lpstrFilter = "1964 State File (*.sav?)\0*.SAV?\0All Files (*.*)\0*.*\0";
		else
			ofn.lpstrFilter = "Project 64 State File (*.pj?)\0*.PJ;*.PJ?\0All Files (*.*)\0*.*\0";
		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter = 0;
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = szFileName;
		ofn.nMaxFile = MAXFILENAME;
		ofn.lpstrInitialDir = szPath;
		ofn.lpstrFileTitle = szFileTitle;
		ofn.nMaxFileTitle = MAXFILENAME;
		ofn.lpstrTitle = "Load State";
		ofn.lpstrDefExt = "";
		ofn.Flags = OFN_ENABLESIZING | OFN_HIDEREADONLY;

		if(GetOpenFileName((LPOPENFILENAME) & ofn))
		{
			if(format == SAVE_STATE_1964_FORMAT)
				FileIO_gzLoadStateFile(szFileName);
			else
				FileIO_ImportPJ64State(szFileName);
		}

		if(emustatus.Emu_Is_Running) ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL SelectDirectory(char *title, char buffer[MAX_PATH])
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	BROWSEINFO		bi;
	char			pszBuffer[MAX_PATH];
	LPITEMIDLIST	pidl;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	bi.hwndOwner = gui.hwnd1964main;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = pszBuffer;
	bi.lpszTitle = title;
	bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.lParam = 0;

	if((pidl = SHBrowseForFolder(&bi)) != NULL)
	{
		if(SHGetPathFromIDList(pidl, buffer))
		{
			return TRUE;
		}
	}

	return FALSE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ChangeDirectory(void)
{
	/*~~~~~~~~~~~~~~~~~~~*/
	char	path[MAX_PATH];
	/*~~~~~~~~~~~~~~~~~~~*/

	if(emustatus.Emu_Is_Running) return;

	if(SelectDirectory("Select a ROM folder", path))
	{
		strcpy(directories.rom_directory_to_use, path);
		strcpy(directories.last_rom_directory, path);
		WriteConfiguration();
		RefreshRecentRomDirectoryMenus(path);

		ClearRomList();
		SetStatusBarText(0, "Looking for ROM file(s) in the ROM folder and Generating List");
		RomListReadDirectory(directories.rom_directory_to_use);
		NewRomList_ListViewFreshRomList();
		Set_Ready_Message();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ChangeToRecentDirectory(int id)
{
	if(emustatus.Emu_Is_Running) return;

	if(id >= 0 && id < MAX_RECENT_ROM_DIR)
	{
		strcpy(generalmessage, recent_rom_directory_lists[id]);
		strcpy(directories.rom_directory_to_use, generalmessage);
		strcpy(directories.last_rom_directory, generalmessage);
		WriteConfiguration();
		RefreshRecentRomDirectoryMenus(generalmessage);

		ClearRomList();
		SetStatusBarText(0, "Looking for ROM file in the ROM folder and Generate List");
		RomListReadDirectory(directories.rom_directory_to_use);
		NewRomList_ListViewFreshRomList();
		Set_Ready_Message();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT APIENTRY DefaultOptionsDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	/*~~*/
	int i;
	/*~~*/

	switch(message)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage
		(
			hDlg,
			IDC_DEFAULTOPTIONS_EXPANSIONPAK,
			BM_SETCHECK,
			defaultoptions.RDRAM_Size == RDRAMSIZE_8MB ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_DEFAULTOPTIONS_USETLB,
			BM_SETCHECK,
			defaultoptions.Use_TLB == USETLB_YES ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_DEFAULTOPTIONS_DMASEGMENTATION,
			BM_SETCHECK,
			emuoptions.dma_in_segments == USEDMASEG_YES ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_DEFAULTOPTIONS_REGC,
			BM_SETCHECK,
			defaultoptions.Use_Register_Caching == USEREGC_YES ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_DEFAULTOPTIONS_FPUHACK,
			BM_SETCHECK,
			defaultoptions.FPU_Hack == USEFPUHACK_YES ? BST_CHECKED : BST_UNCHECKED,
			0
		);

		SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_SAVETYPE, CB_RESETCONTENT, 0, 0);
		for(i = 1; i < 7; i++)
		{
			SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_SAVETYPE, CB_INSERTSTRING, i - 1, (LPARAM) save_type_names[i]);
			if(i == defaultoptions.Save_Type)
				SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_SAVETYPE, CB_SETCURSEL, i - 1, 0);
		}

		SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_CODECHECK, CB_RESETCONTENT, 0, 0);
		for(i = 1; i < 9; i++)
		{
			SendDlgItemMessage
			(
				hDlg,
				IDC_DEFAULTOPTIONS_CODECHECK,
				CB_INSERTSTRING,
				i - 1,
				(LPARAM) codecheck_type_names[i]
			);
			if(i == defaultoptions.Code_Check)
				SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_CODECHECK, CB_SETCURSEL, i - 1, 0);
		}

		SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_MAXVISPEED, CB_RESETCONTENT, 0, 0);
		for(i = 1; i < 5; i++)
		{
			SendDlgItemMessage
			(
				hDlg,
				IDC_DEFAULTOPTIONS_MAXVISPEED,
				CB_INSERTSTRING,
				i - 1,
				(LPARAM) maxfps_type_names[i]
			);
			if(i == defaultoptions.Max_FPS)
				SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_MAXVISPEED, CB_SETCURSEL, i - 1, 0);
		}

		SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_EEPROMSIZE, CB_RESETCONTENT, 0, 0);
		for(i = 1; i < 4; i++)
		{
			SendDlgItemMessage
			(
				hDlg,
				IDC_DEFAULTOPTIONS_EEPROMSIZE,
				CB_INSERTSTRING,
				i - 1,
				(LPARAM) eepromsize_type_names[i]
			);
			if(i == defaultoptions.Eeprom_size)
				SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_EEPROMSIZE, CB_SETCURSEL, i - 1, 0);
		}

		return(TRUE);

	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			{
				/* Read option setting from dialog */
				defaultoptions.RDRAM_Size =
					(SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_EXPANSIONPAK, BM_GETCHECK, 0, 0) == BST_CHECKED) +
					1;
				if(!emustatus.Emu_Is_Running)
					SetStatusBarText(3, defaultoptions.RDRAM_Size == RDRAMSIZE_4MB ? "4MB" : "8MB");
				defaultoptions.Save_Type = SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_SAVETYPE, CB_GETCURSEL, 0, 0) + 1;
				defaultoptions.Code_Check = SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_CODECHECK, CB_GETCURSEL, 0, 0) + 1;
				defaultoptions.Max_FPS = SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_MAXVISPEED, CB_GETCURSEL, 0, 0) + 1;
				defaultoptions.Use_TLB = 2 - (SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_USETLB, BM_GETCHECK, 0, 0) == BST_CHECKED);
				defaultoptions.Eeprom_size = SendDlgItemMessage
						(
							hDlg,
							IDC_DEFAULTOPTIONS_EEPROMSIZE,
							CB_GETCURSEL,
							0,
							0
						) +
					1;
				defaultoptions.Use_Register_Caching = (SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_REGC, BM_GETCHECK, 0, 0) == BST_CHECKED);
				defaultoptions.Use_Register_Caching = (defaultoptions.Use_Register_Caching ? USEREGC_YES : USEREGC_NO);
				defaultoptions.FPU_Hack = (SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_FPUHACK, BM_GETCHECK, 0, 0) == BST_CHECKED);
				defaultoptions.FPU_Hack = (defaultoptions.FPU_Hack ? USEFPUHACK_YES : USEFPUHACK_NO);
				emuoptions.dma_in_segments = (SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_DMASEGMENTATION, BM_GETCHECK, 0, 0) == BST_CHECKED);
				emuoptions.dma_in_segments = (emuoptions.dma_in_segments == 1 ? USEDMASEG_YES : USEDMASEG_NO);
				defaultoptions.DMA_Segmentation = emuoptions.dma_in_segments;

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
 =======================================================================================================================
 */
LRESULT APIENTRY OptionsDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	/*~~~~~~~~~~~~~~~~~~~*/
	char	path[MAX_PATH];
	/*~~~~~~~~~~~~~~~~~~~*/

	switch(message)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage
		(
			hDlg,
			IDC_OPTION_AUTOFULLSCREEN,
			BM_SETCHECK,
			emuoptions.auto_full_screen ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_DEFAULTOPTIOS_PAUSEONMENU,
			BM_SETCHECK,
			guioptions.pause_at_menu ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_DEFAULTOPTIONS_PAUSEWHENINACTIVE,
			BM_SETCHECK,
			guioptions.pause_at_inactive ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_EXPERT_MODE,
			BM_SETCHECK,
			guioptions.show_expert_user_menu ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_ENABLE_DIRECTORY_LIST,
			BM_SETCHECK,
			guioptions.show_recent_rom_directory_list ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_ENABLE_GAME_LIST,
			BM_SETCHECK,
			guioptions.show_recent_game_list ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_ENABLE_DETAIL_STATUS,
			BM_SETCHECK,
			guioptions.display_detail_status ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_ENABLE_PROFILER,
			BM_SETCHECK,
			guioptions.display_profiler_status ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_DISPLAY_STATUSBAR,
			BM_SETCHECK,
			guioptions.display_statusbar ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_HIGHFREQTIMER,
			BM_SETCHECK,
			guioptions.highfreqtimer ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_ENABLE_STATE_MENU,
			BM_SETCHECK,
			guioptions.show_state_selector_menu ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_OPTION_ERROR_WINDOW,
			BM_SETCHECK,
			guioptions.show_critical_msg_window ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_OPTION_ROMBROWSER,
			BM_SETCHECK,
			guioptions.display_romlist ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_OPTIONS_USE_DEFAULT_SAVE_DIRECTORY,
			BM_SETCHECK,
			guioptions.use_default_save_directory ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_DEFAULTOPTIONS_USE1964PLUGINDIRECTORY,
			BM_SETCHECK,
			guioptions.use_default_plugin_directory ? BST_CHECKED : BST_UNCHECKED,
			0
		);
		SendDlgItemMessage
		(
			hDlg,
			IDC_DEFAULTOPTIONS_USELASTROMDIRECTORY,
			BM_SETCHECK,
			guioptions.use_last_rom_directory ? BST_CHECKED : BST_UNCHECKED,
			0
		);

		SetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_STATESAVEDIRECTORY, state_save_directory);
		SetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_ROMDIRECTORY, user_set_rom_directory);
		SetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_SAVEDIRECTORY, user_set_save_directory);
		SetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_PLUGINDIRECTORY, user_set_plugin_directory);

		return(TRUE);

	case WM_COMMAND:
		switch(wParam)
		{
		case IDC_DEFAULTOPTIONS_BUTTON_SAVEDIR:
			if(SelectDirectory("Selecting Game Save Directory", path))
			{
				strcat(path, "\\");
				SetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_SAVEDIRECTORY, path);
			}
			break;
		case IDC_DEFAULTOPTIONS_BUTTON_PLUGINDIR:
			if(SelectDirectory("Selecting Plugin Directory", path))
			{
				strcat(path, "\\");
				SetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_PLUGINDIRECTORY, path);
			}
			break;
		case IDC_DEFAULTOPTIONS_BUTTON_ROMDIR:
			if(SelectDirectory("Selecting Default ROM Directory", path))
			{
				strcat(path, "\\");
				SetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_ROMDIRECTORY, path);
			}
			break;

		case IDOK:
			{
				/* Read option setting from dialog */
				emuoptions.auto_full_screen = (SendDlgItemMessage(hDlg, IDC_OPTION_AUTOFULLSCREEN, BM_GETCHECK, 0, 0) == BST_CHECKED);
				emuoptions.auto_apply_cheat_code = (SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_AUTOCHEAT, BM_GETCHECK, 0, 0) == BST_CHECKED);
				guioptions.pause_at_menu = (SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIOS_PAUSEONMENU, BM_GETCHECK, 0, 0) == BST_CHECKED);
				guioptions.pause_at_inactive = (SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_PAUSEWHENINACTIVE, BM_GETCHECK, 0, 0) == BST_CHECKED);

				if
				(
					guioptions.show_expert_user_menu !=
						(SendDlgItemMessage(hDlg, IDC_EXPERT_MODE, BM_GETCHECK, 0, 0) == BST_CHECKED)
				)
				{
					guioptions.show_expert_user_menu = 1 - guioptions.show_expert_user_menu;
					if(guioptions.show_expert_user_menu)
						RegenerateAdvancedUserMenus();
					else
					{
						DeleteAdvancedUserMenus();
					}
				}

				if
				(
					guioptions.show_recent_rom_directory_list !=
						(SendDlgItemMessage(hDlg, IDC_ENABLE_DIRECTORY_LIST, BM_GETCHECK, 0, 0) == BST_CHECKED)
				)
				{
					guioptions.show_recent_rom_directory_list = 1 - guioptions.show_recent_rom_directory_list;
					if(guioptions.show_recent_rom_directory_list)
						RegerateRecentRomDirectoryMenus();
					else
					{
						DeleteRecentRomDirectoryMenus();
					}
				}

				if
				(
					guioptions.show_recent_game_list !=
						(SendDlgItemMessage(hDlg, IDC_ENABLE_GAME_LIST, BM_GETCHECK, 0, 0) == BST_CHECKED)
				)
				{
					guioptions.show_recent_game_list = 1 - guioptions.show_recent_game_list;
					if(guioptions.show_recent_game_list)
						RegerateRecentGameMenus();
					else
					{
						DeleteRecentGameMenus();
					}
				}

				if
				(
					guioptions.show_state_selector_menu !=
						(SendDlgItemMessage(hDlg, IDC_ENABLE_STATE_MENU, BM_GETCHECK, 0, 0) == BST_CHECKED)
				)
				{
					guioptions.show_state_selector_menu = 1 - guioptions.show_state_selector_menu;
					if(guioptions.show_state_selector_menu)
						RegenerateStateSelectorMenus();
					else
					{
						DeleteStateSelectorMenus();
					}
				}

				if
				(
					guioptions.show_critical_msg_window !=
						(SendDlgItemMessage(hDlg, IDC_OPTION_ERROR_WINDOW, BM_GETCHECK, 0, 0) == BST_CHECKED)
				)
				{
					guioptions.show_critical_msg_window = 1 - guioptions.show_critical_msg_window;
					if(guioptions.show_critical_msg_window)
					{
						if(gui.hCriticalMsgWnd == NULL)
						{
							gui.hCriticalMsgWnd = CreateDialog
								(
									gui.hInst,
									"CRITICAL_MESSAGE",
									NULL,
									(DLGPROC) CriticalMessageDialog
								);
							SetActiveWindow(gui.hwnd1964main);
						}
					}
					else
					{
						if(gui.hCriticalMsgWnd != NULL)
						{
							DestroyWindow(gui.hCriticalMsgWnd);
							gui.hCriticalMsgWnd = NULL;
						}
					}
				}

				if(	guioptions.display_romlist !=(SendDlgItemMessage(hDlg, IDC_OPTION_ROMBROWSER, BM_GETCHECK, 0, 0) == BST_CHECKED))
				{
					guioptions.display_romlist = 1 - guioptions.display_romlist;
					EndDialog(hDlg, TRUE);
					SendMessage(gui.hwnd1964main, WM_COMMAND, ID_FILE_FRESHROMLIST, 0);
					return(TRUE);
				}

				guioptions.display_detail_status = (SendDlgItemMessage(hDlg, IDC_ENABLE_DETAIL_STATUS, BM_GETCHECK, 0, 0) == BST_CHECKED);
				guioptions.display_profiler_status = (SendDlgItemMessage(hDlg, IDC_ENABLE_PROFILER, BM_GETCHECK, 0, 0) == BST_CHECKED);
				if(SendDlgItemMessage(hDlg, IDC_DISPLAY_STATUSBAR, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					ShowWindow(gui.hToolBar, SW_SHOW);
					ShowWindow(gui.hStatusBar, SW_SHOW);
				}
				else
					ShowWindow(gui.hStatusBar, SW_HIDE);
				if(guioptions.highfreqtimer != SendDlgItemMessage(hDlg, IDC_HIGHFREQTIMER, BM_GETCHECK, 0, 0))
					MessageBox(gui.hwnd1964main, "Please restart 1964 to apply high frequency settings.", "User Options", MB_OK);
				guioptions.highfreqtimer = (SendDlgItemMessage(hDlg, IDC_HIGHFREQTIMER, BM_GETCHECK, 0, 0) == BST_CHECKED);
				guioptions.display_statusbar = (SendDlgItemMessage(hDlg, IDC_DISPLAY_STATUSBAR, BM_GETCHECK, 0, 0) == BST_CHECKED);
				guioptions.use_default_save_directory = (SendDlgItemMessage(hDlg, IDC_OPTIONS_USE_DEFAULT_SAVE_DIRECTORY, BM_GETCHECK, 0, 0) == BST_CHECKED);
				guioptions.use_default_plugin_directory =
					(
						SendDlgItemMessage
						(
							hDlg,
							IDC_DEFAULTOPTIONS_USE1964PLUGINDIRECTORY,
							BM_GETCHECK,
							0,
							0
						) == BST_CHECKED
					);
				guioptions.use_last_rom_directory = (SendDlgItemMessage(hDlg, IDC_DEFAULTOPTIONS_USELASTROMDIRECTORY, BM_GETCHECK, 0, 0) == BST_CHECKED);

				GetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_ROMDIRECTORY, user_set_rom_directory, _MAX_PATH);
				GetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_SAVEDIRECTORY, user_set_save_directory, _MAX_PATH);
				GetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_STATESAVEDIRECTORY, state_save_directory, _MAX_PATH);
				GetDlgItemText(hDlg, IDC_DEFAULTOPTIONS_PLUGINDIRECTORY, user_set_plugin_directory, _MAX_PATH);

				/* Set the save directory to use */
				if(guioptions.use_default_save_directory)
					strcpy(directories.save_directory_to_use, default_save_directory);
				else
					strcpy(directories.save_directory_to_use, user_set_save_directory);

				/* Set the ROM directory to use */
				if(guioptions.use_last_rom_directory)
					strcpy(directories.rom_directory_to_use, directories.last_rom_directory);
				else
					strcpy(directories.rom_directory_to_use, user_set_rom_directory);

				/* Set the plugin directory to use */
				if(guioptions.use_default_plugin_directory)
					strcpy(directories.plugin_directory_to_use, default_plugin_directory);
				else
					strcpy(directories.plugin_directory_to_use, user_set_plugin_directory);

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
 =======================================================================================================================
 */
void SetOverclockFactor(int factor)
{
	int ocmenuid = ID_OVERCLOCK6;
	int cfmenuid = ID_CF_CF1;
	CheckMenuItem(gui.hMenu1964main, ID_OVERCLOCKSTOCK, factor == 1 ? MF_CHECKED : MF_UNCHECKED);
	while(ocmenuid <= ID_OVERCLOCK18)
	{
		if((LOWORD(ocmenuid) - ID_OVERCLOCK6 + 2) * 3 != factor)
			CheckMenuItem(gui.hMenu1964main, ocmenuid, MF_UNCHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ocmenuid, MF_CHECKED);
		ocmenuid++;
	}
	while(cfmenuid <= ID_CF_CF8)
	{
		if(factor != 1)
			EnableMenuItem(gui.hMenu1964main, cfmenuid, MF_GRAYED);
		else
			EnableMenuItem(gui.hMenu1964main, cfmenuid, MF_ENABLED);
		cfmenuid++;
	}
	if(!emustatus.gepd_pause)
		GEPDPause(TRUE);
	else
		emustatus.gepd_pause = 2;
	emuoptions.OverclockFactor = factor;
	EnableMenuItem(gui.hMenu1964main, ID_PDSPEEDHACK, factor != 1 ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(gui.hMenu1964main, ID_PDSPEEDHACKBOOST, emuoptions.PDSpeedHack && factor != 1 ? MF_ENABLED : MF_GRAYED);
	CheckMenuItem(gui.hMenu1964main, ID_PDSPEEDHACK, emuoptions.PDSpeedHack && factor != 1 ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, ID_PDSPEEDHACKBOOST, emuoptions.PDSpeedHack && emuoptions.PDSpeedHackBoost && factor != 1 ? MF_CHECKED : MF_UNCHECKED);
	if(mouseinjectorpresent)
		CONTROLLER_HookRDRAM((DWORD *)TLB_sDWord_ptr, factor);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SetCounterFactor(int factor)
{
	if(emuoptions.OverclockFactor != 1 || factor < COUTERFACTOR_1 || factor > COUTERFACTOR_8)
		factor = COUTERFACTOR_1;
	if(CounterFactor != factor)
	{
		CheckMenuItem(gui.hMenu1964main, cfmenulist[CounterFactor - 1], MF_UNCHECKED);
		CounterFactor = factor;
		if(emustatus.Emu_Is_Running)
		{
			Init_Count_Down_Counters();
			if(PauseEmulator())
				ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);	/* Need to init emu */
		}

		CheckMenuItem(gui.hMenu1964main, cfmenulist[CounterFactor - 1], MF_CHECKED);
		sprintf(generalmessage, "CF=%d", factor);
		SetStatusBarText(2, generalmessage);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */

#define PD_tickrate 0x80099FC0 // game loop's tickrate (used to detect when PD is running at lower framerate - PD was designed to halve game tickrate when detected bottlenecks)
#define PD_timer 0x8008DBD0 // we intentionally screw with this timer address to force PD to run at unlocked speed
#define PD_mpspeed 0x800ACB91 // used to check if the match is set to slow motion or normal speed
#define PD_masterclock 0x8038CECC // location of master clock code (TLB'd to 7F)
#define PD_newcodearea 0x803C78E0 // free area to write new timing code for 60fps mode
#define PD_newcodearealastcode 0x803C7964 // used to check if memory is safe to write

static const unsigned int codearray[34] = {0x3C028006, 0x8C42EE10, 0x240E0007, 0x51C2000E, 0x3C02800A, 0x3C02800B, 0x8042CB97, 0x304E0080, 0x15C00017, 0x304E0040, 0x11C00012, 0x3C02800A, 0x8C42A424, 0x14400012, 0x00000000, 0x10000010, 0x00129040, 0x00000000, 0x804221D3, 0x30420040, 0x10400007, 0x00000000, 0x3C02800A, 0x8C42A424, 0x14400007, 0x00000000, 0x10000005, 0x00129040, 0x3C02800A, 0x8C42A424, 0x54400001, 0x00129040, 0x0BC5B3B5, 0x3631EBC2}; // hijack timing code to allow combat boost at 60fps

void PDTimingHack(void)
{
	int codeindex;
	if(LOAD_UWORD_PARAM(PD_masterclock) == 0x3652FAF0 && LOAD_UWORD_PARAM(PD_newcodearealastcode) == 0x73666572) // inject new timing code
	{
		for(codeindex = 0; codeindex < 34; codeindex++)
			LOAD_UWORD_PARAM(PD_newcodearea + codeindex * 4) = codearray[codeindex];
		LOAD_UWORD_PARAM(PD_masterclock) = 0x0BC69E38;
		LOAD_UWORD_PARAM(PD_masterclock + 4) = 0x3652FAF0;
	}
}

void PDSpeedHack(void)
{
	int bruteforce;
	if(LOAD_SWORD_PARAM(PD_tickrate) == 2 && (LOAD_UWORD_PARAM(PD_mpspeed) & 0x80) != 0x80) // if PD is running at 30fps and mp slow motion setting is not set to smart
	{
		for(bruteforce = 0; bruteforce < (emuoptions.PDSpeedHackBoost ? 48 : 24); bruteforce++)
			LOAD_UWORD_PARAM(PD_timer) = 0x00000000; // trick the system into thinking there is plenty of room left to run at full speed
		Sleep(emuoptions.PDSpeedHackBoost ? 8 : 4);
		LOAD_UWORD_PARAM(PD_timer) = 0x00010000; // set clock back to default (reset fps lock to 60)
	}
}

#undef PD_tickrate
#undef PD_timer
#undef PD_mpspeed
#undef PD_masterclock
#undef PD_newcodearea
#undef PD_newcodearealastcode

/*
 =======================================================================================================================
 =======================================================================================================================
 */

#define GE_pause 0x80048370 // pause flag (1 = GE is paused)
#define PD_pause 0x80084014 // menu flag (1 = PD is paused)

static BOOL alreadypaused = FALSE;

void GEPDPause(BOOL pause)
{
	int game;
	if(!strcmp(currentromoptions.Game_Name, "GOLDENEYE") || strstr(currentromoptions.Game_Name, "GOLD"))
		game = 0;
	else if(!strcmp(currentromoptions.Game_Name, "Perfect Dark") || !strcmp(currentromoptions.Game_Name, "GoldenEye X") || strstr(currentromoptions.Game_Name, "Perfect"))
		game = 1;
	else return;
	if(pause)
	{
		emustatus.gepd_pause = 2;
		if(!LOAD_UWORD_PARAM(!game ? GE_pause : PD_pause))
			LOAD_UWORD_PARAM(!game ? GE_pause : PD_pause) = 1;
		else
			alreadypaused = TRUE, emustatus.gepd_pause = 1;
	}
	else
	{
		if(!alreadypaused)
			LOAD_UWORD_PARAM(!game ? GE_pause : PD_pause) = 0;
		alreadypaused = FALSE;
	}
}

#undef GE_pause
#undef PD_pause

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SetCodeCheckMethod(int method)
{
	if(emustatus.Emu_Is_Running)
	{
		CheckMenuItem(gui.hMenu1964main, codecheckmenulist[emustatus.CodeCheckMethod - 1], MF_UNCHECKED);
		if(PauseEmulator())
		{
			Dyna_Check_Codes = Dyna_Code_Check[method - 1];
			emustatus.CodeCheckMethod = method;
			ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);	/* Need to init emu */
			CheckMenuItem(gui.hMenu1964main, codecheckmenulist[method - 1], MF_CHECKED);
		}
	}
	else
	{
		CheckMenuItem(gui.hMenu1964main, codecheckmenulist[emustatus.CodeCheckMethod - 1], MF_UNCHECKED);
		emustatus.CodeCheckMethod = method;
		defaultoptions.Code_Check = method;
		CheckMenuItem(gui.hMenu1964main, codecheckmenulist[method - 1], MF_CHECKED);
	}
}

WINDOWPLACEMENT window_placement_save_romlist;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RememberWindowSize(void)
{
	/* Try to remember the main window position and size before playing a game */
	window_placement_save_romlist.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(gui.hwnd1964main, &window_placement_save_romlist);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ResetWindowSizeAsRemembered(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	RECT	*prect = &(window_placement_save_romlist.rcNormalPosition);
	RECT    Rect;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	GetWindowRect(gui.hwnd1964main, &Rect);
	SetWindowPos
	(
		gui.hwnd1964main,
		NULL,
		Rect.left,
		Rect.top,
		prect->right - prect->left ,
		prect->bottom - prect->top ,
		SWP_NOZORDER | SWP_SHOWWINDOW
	);
}

extern char critical_msg_buffer[32 * 1024]; /* 32KB */

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void PrepareBeforePlay(int IsFullScreen)
{
	if (IsFullScreen == 0)	RememberWindowSize();
	critical_msg_buffer[0] = '\0';					/* clear the critical message buffer */

	if (IsFullScreen == 0)
	{
	/* Hide romlist */
	RomListSaveCurrentPos();
	ListView_SetExtendedListViewStyle(gui.hwndRomList, LVS_EX_FULLROWSELECT);
	NewRomList_ListViewHideHeader(gui.hwndRomList);
	ShowWindow(gui.hwndRomList, SW_HIDE);
	EnableWindow(gui.hwndRomList, FALSE);
	EnableMenuItem(gui.hMenu1964main, ID_KAILLERA_MODE, MF_GRAYED);
	}

	/* Setting options */
	RomListSelectLoadedRomEntry();
	GenerateCurrentRomOptions();
	if(!strcmp(currentromoptions.Game_Name, "GOLDENEYE") || strstr(currentromoptions.Game_Name, "GOLD"))
		emuoptions.UsingRspPlugin = TRUE;
	else if(!strcmp(currentromoptions.Game_Name, "Perfect Dark") || !strcmp(currentromoptions.Game_Name, "GoldenEye X") || strstr(currentromoptions.Game_Name, "Perfect"))
		emuoptions.UsingRspPlugin = FALSE;
	init_whole_mem_func_array();					/* Needed here. The tlb function pointers change. */
	ResetRdramSize(currentromoptions.RDRAM_Size);
	if(strcpy(current_cheatcode_rom_internal_name, currentromoptions.Game_Name) != 0)
		CodeList_ReadCode(currentromoptions.Game_Name);

	SetCounterFactor(currentromoptions.Counter_Factor);
	emustatus.CodeCheckMethod = currentromoptions.Code_Check;

	/*
	 * Using the Check_QWORD to boot, will switch to ROM specified
	 * emustatus.CodeCheckMethod £
	 * at first FPU exception. I don't know why use NoCheck method will not boot £
	 * Game like SuperMario should not need to do DynaCodeCheck but how the ROM does
	 * not boot £
	 * with DynaCodeCheck, need debug
	 */
	if(emustatus.CodeCheckMethod == CODE_CHECK_NONE || emustatus.CodeCheckMethod == CODE_CHECK_DMA_ONLY)
	{
		Dyna_Check_Codes = Dyna_Code_Check_None_Boot;
		TRACE0("Set code check method = Dyna_Code_Check_None_Boot / Check_DMA_only");
	}
	else
	{
		Dyna_Check_Codes = Dyna_Code_Check[emustatus.CodeCheckMethod - 1];
	}

	emustatus.cpucore = currentromoptions.Emulator;
	SendMessage
	(
		gui.hwnd1964main,
		WM_COMMAND,
		emustatus.cpucore == DYNACOMPILER ? ID_DYNAMICCOMPILER : ID_INTERPRETER,
		0
	);

	/* About FPU usage exceptions */
	if(currentromoptions.FPU_Hack == USEFPUHACK_YES)
	{
		EnableFPUUnusableException();
	}
	else
	{
		DisableFPUUnusableException();
	}

	Flashram_Init();
	Init_iPIF();

	emustatus.DListCount = 0;
	emustatus.AListCount = 0;
	emustatus.PIDMACount = 0;
	emustatus.ControllerReadCount = 0;

	if(!QueryPerformanceFrequency(&Freq))
	{
		currentromoptions.Max_FPS = MAXFPS_NONE;	/* ok, this computer does not support */
		/* accurate timer, don't use speed limiter */
	}
	else
	{
		if(rominfo.TV_System == 0)					/* PAL */
		{
			vips_speed_limits[MAXFPS_AUTO_SYNC] = vips_speed_limits[MAXFPS_PAL_50];
		}
		else	/* NTSC */
		{
			vips_speed_limits[MAXFPS_AUTO_SYNC] = vips_speed_limits[MAXFPS_NTSC_60];
		}
	}
	if (IsFullScreen == 0)
	{
		CheckMenuItem(gui.hMenu1964main, codecheckmenulist[emustatus.CodeCheckMethod - 1], MF_UNCHECKED);
		CheckMenuItem(gui.hMenu1964main, codecheckmenulist[emustatus.CodeCheckMethod - 1], MF_CHECKED);
		sprintf(generalmessage, "CF=%d", emuoptions.OverclockFactor == 1 ? currentromoptions.Counter_Factor : 1);
		SetStatusBarText(2, generalmessage);
		SetStatusBarText(4, emustatus.cpucore == DYNACOMPILER ? "D" : "I");
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AfterStop(void)
{
#ifdef CHEATCODE_LOCK_MEMORY
	CloseCheatCodeEngineMemoryLock();
#endif

	Close_Save();
	Close_iPIF();

	ResetWindowSizeAsRemembered();

	emustatus.Emu_Is_Running = FALSE;
	EnableMenuItem(gui.hMenu1964main, ID_OPENROM, MF_ENABLED);
	EnableButton(ID_BUTTON_OPEN_ROM, TRUE);
	EnableMenuItem(gui.hMenu1964main, IDM_PLUGINS, MF_ENABLED);
	EnableButton(ID_BUTTON_SETUP_PLUGINS, TRUE);
	EnableMenuItem(gui.hMenu1964main, ID_CLOSEROM, MF_ENABLED);
	EnableMenuItem(gui.hMenu1964main, ID_ROM_PAUSE, MF_GRAYED);
	EnableMenuItem(gui.hMenu1964main, ID_KAILLERA_MODE, (IsKailleraDllLoaded())? MF_ENABLED:MF_GRAYED);
	EnableRadioButtons(FALSE);

	/* EnableMenuItem(gui.hMenu1964main, ID_ROM_STOP, MF_GRAYED); */
	EnableMenuItem(gui.hMenu1964main, ID_PLUGINS_SCREENSHOTS, MF_GRAYED);
	DisableStateMenu();

	ShowWindow(gui.hwndRomList, SW_SHOW);

	EnableWindow(gui.hwndRomList, TRUE);
	ListView_SetExtendedListViewStyle(gui.hwndRomList, LVS_EX_FULLROWSELECT);	/* | LVS_EX_TRACKSELECT ); */
	NewRomList_ListViewShowHeader(gui.hwndRomList);

	/* refresh the rom list, just to prevent user has changed resolution */
	NewRomList_ListViewChangeWindowRect();
	DockStatusBar();
	RomListUseSavedPos();

	/* Reset some of the default options */
	defaultoptions.Emulator = DYNACOMPILER;
	emustatus.cpucore = defaultoptions.Emulator;
	SendMessage
	(
		gui.hwnd1964main,
		WM_COMMAND,
		emustatus.cpucore == DYNACOMPILER ? ID_DYNAMICCOMPILER : ID_INTERPRETER,
		0
	);
	SetStatusBarText(4, emustatus.cpucore == DYNACOMPILER ? "D" : "I");
	SetCounterFactor(defaultoptions.Counter_Factor);
	SetCodeCheckMethod(defaultoptions.Code_Check);

	/* Flash the status bar */
	if(guioptions.display_statusbar)
	{
		ShowWindow(gui.hStatusBar, SW_HIDE);
		ShowWindow(gui.hStatusBar, SW_SHOW);
	}
	SetStatusBarText(3, defaultoptions.RDRAM_Size == RDRAMSIZE_4MB ? "4MB" : "8MB");

	sprintf(generalmessage, "%s - Stopped", gui.szWindowTitle);
	SetWindowText(gui.hwnd1964main, generalmessage);
	
	Set_Ready_Message();
	SetStatusBarText(1, " 0 VI/s");

	if( NeedFreshromListAfterStop == TRUE )
	{
		NeedFreshromListAfterStop = FALSE;
		OnFreshRomList();
	}
}

/*
 =======================================================================================================================
    Move the status bar to the bottom of the main window.
 =======================================================================================================================
 */
void DockStatusBar(void)
{
	/*~~~~~~~~~~~~~~~~~~~~*/
	RECT	rc, rcstatusbar;
	RECT	rcToolBar;
	/*~~~~~~~~~~~~~~~~~~~~*/

	if(gui.hStatusBar == NULL) return;
	GetClientRect(gui.hwnd1964main, &rc);
	GetWindowRect(gui.hStatusBar, &rcstatusbar);
	GetWindowRect(gui.hToolBar, &rcToolBar);
	MoveWindow
	(
		gui.hStatusBar,
		0,
		rc.bottom - (rcstatusbar.bottom - rcstatusbar.top + 1),
		rcstatusbar.right - rcstatusbar.left + 1,
		rcstatusbar.bottom - rcstatusbar.top + 1,
		TRUE
	);
	MoveWindow
	(
		gui.hToolBar,
		0,
		0,
		rcToolBar.right - rcToolBar.left + 1,
		rcToolBar.bottom - rcToolBar.top + 1,
		TRUE
	);

//	ShowWindow(gui.hStatusBar, SW_HIDE);
	ShowWindow(gui.hToolBar, SW_SHOW);
	ShowWindow(gui.hStatusBar, SW_SHOW);

	InitStatusBarParts();
	ShowWindow(gui.hStatusBar, guistatus.IsFullScreen || !guioptions.display_statusbar ? SW_HIDE : SW_SHOW);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitStatusBarParts(void)
{
	/*~~~~~~~*/
	RECT	rc;
	/*~~~~~~~*/

	if(gui.hStatusBar == NULL)
		return;
	else
	{
		/*~~~~~~~~~*/
		int sizes[6];
		/*~~~~~~~~~*/

		GetWindowRect(gui.hStatusBar, &rc);

		/*
		 * sizes[5] = rc.right-rc.left-25; £
		 * sizes[4] = sizes[5]-40;
		 */
		sizes[4] = rc.right - rc.left - 25;
		sizes[3] = sizes[4] - 15;
		sizes[2] = sizes[3] - 30;
		sizes[1] = sizes[2] - 40;
		sizes[0] = sizes[1] - 60;

		SendMessage(gui.hStatusBar, SB_SETPARTS, 5, (LPARAM) sizes);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SetStatusBarText(int fieldno, char *text)
{
	SendMessage(gui.hStatusBar, SB_SETTEXT, fieldno, (LPARAM) text);
}

HMENU			file_submenu;
HMENU			CPU_submenu;
HMENU			recent_rom_directory_submenu;
HMENU			recent_game_submenu;
HMENU			switch_compiler_submenu;
MENUITEMINFO	switch_compiler_submenu_info;
HMENU			dyna_code_checking_submenu;
HMENU			counter_hack_submenu;
HMENU			state_save_submenu;
HMENU			state_load_submenu;
UINT			recent_rom_directory_submenu_pos;
UINT			recent_game_submenu_pos;
UINT			switch_compiler_submenu_pos;
UINT			dyna_code_checking_submenu_pos;
UINT			counter_hack_submenu_pos;
UINT			state_save_submenu_pos;
UINT			state_load_submenu_pos;

MENUITEMINFO	advanced_options_menuitem;
MENUITEMINFO	seperator_menuitem;
UINT			advanced_options_menuitem_pos;
UINT			seperator_menuitem_pos;

UINT			recent_game_menu_ids[MAX_RECENT_GAME_LIST] =
{
	ID_FILE_RECENTGAMES_GAME1,
	ID_FILE_RECENTGAMES_GAME2,
	ID_FILE_RECENTGAMES_GAME3,
	ID_FILE_RECENTGAMES_GAME4,
	ID_FILE_RECENTGAMES_GAME5,
	ID_FILE_RECENTGAMES_GAME6,
	ID_FILE_RECENTGAMES_GAME7,
	ID_FILE_RECENTGAMES_GAME8
};
UINT			recent_rom_directory_menu_ids[MAX_RECENT_ROM_DIR] =
{
	ID_FILE_ROMDIRECTORY1,
	ID_FILE_ROMDIRECTORY2,
	ID_FILE_ROMDIRECTORY3,
	ID_FILE_ROMDIRECTORY4,
	ID_FILE_ROMDIRECTORY5,
	ID_FILE_ROMDIRECTORY6,
	ID_FILE_ROMDIRECTORY7,
	ID_FILE_ROMDIRECTORY8
};

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void ModifyMenuText(UINT menuid, char *newtext)
{
	ModifyMenu(gui.hMenu1964main, menuid, MF_BYCOMMAND, menuid, newtext);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RegerateRecentGameMenus(void)
{
	if(guioptions.show_recent_rom_directory_list)
	{
		InsertMenu
		(
			file_submenu,
			recent_game_submenu_pos,
			MF_BYPOSITION | MF_POPUP,
			(UINT) recent_game_submenu,
			"Recent Games"
		);
	}
	else
	{
		InsertMenu
		(
			file_submenu,
			recent_rom_directory_submenu_pos,
			MF_BYPOSITION | MF_POPUP,
			(UINT) recent_game_submenu,
			"Recent Games"
		);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DeleteRecentGameMenus(void)
{
	/*~~~~~~~~~~~~~~~*/
	int		k, i, j, n;
	char	str[100];
	/*~~~~~~~~~~~~~~~*/

	i = GetMenuItemCount(gui.hMenu1964main);
	for(k = 0; k < i; k++)
	{
		GetMenuString(gui.hMenu1964main, k, str, 80, MF_BYPOSITION);
		if(strcmp(str, "&File") == 0)
		{
			file_submenu = GetSubMenu(gui.hMenu1964main, k);
			j = GetMenuItemCount(file_submenu);
			for(n = j - 1; n >= 0; n--) /* I have to delete the menu in reverse order */
			{
				GetMenuString(file_submenu, n, str, 80, MF_BYPOSITION);
				if(strcmp(str, "Recent Games") == 0)
				{
					recent_game_submenu = GetSubMenu(file_submenu, n);
					recent_game_submenu_pos = n;
					RemoveMenu(file_submenu, n, MF_BYPOSITION);
				}
			}
		}
	}
}

/*
 =======================================================================================================================
    char recent_game_lists[8][260];
 =======================================================================================================================
 */
void RefreshRecentGameMenus(char *newgamefilename)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < 8; i++)
	{
		if(strcmp(recent_game_lists[i], newgamefilename) == 0) break;
	}

	if(i != 0)
	{
		if(i == 8) i = 7;	/* if not found */

		/* need to move the most recent file to the 1st position */
		for(; i > 0; i--)
		{
			strcpy(recent_game_lists[i], recent_game_lists[i - 1]);
			ModifyMenuText(recent_game_menu_ids[i], recent_game_lists[i]);
		}

		strcpy(recent_game_lists[0], newgamefilename);
		ModifyMenuText(recent_game_menu_ids[0], newgamefilename);
	}

	return;
}

/*
 =======================================================================================================================
    char recent_rom_directory_lists[MAX_RECENT_ROM_DIR][260];
 =======================================================================================================================
 */
void RefreshRecentRomDirectoryMenus(char *newromdirectory)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < MAX_RECENT_ROM_DIR; i++)
	{
		if(strcmp(recent_rom_directory_lists[i], newromdirectory) == 0) break;
	}

	if(i != 0)
	{
		if(i == MAX_RECENT_ROM_DIR) i = MAX_RECENT_ROM_DIR - 1; /* if not found */

		/* need to move the most recent file to the 1st position */
		for(; i > 0; i--)
		{
			strcpy(recent_rom_directory_lists[i], recent_rom_directory_lists[i - 1]);
			ModifyMenuText(recent_rom_directory_menu_ids[i], recent_rom_directory_lists[i]);
		}

		strcpy(recent_rom_directory_lists[0], newromdirectory);
		ModifyMenuText(recent_rom_directory_menu_ids[0], newromdirectory);
	}

	return;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RegerateRecentRomDirectoryMenus(void)
{
	InsertMenu
	(
		file_submenu,
		recent_rom_directory_submenu_pos,
		MF_BYPOSITION | MF_POPUP,
		(UINT) recent_rom_directory_submenu,
		"Recent ROM Folders"
	);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DeleteRecentRomDirectoryMenus(void)
{
	/*~~~~~~~~~~~~~~~*/
	int		k, i, j, n;
	char	str[100];
	/*~~~~~~~~~~~~~~~*/

	i = GetMenuItemCount(gui.hMenu1964main);
	for(k = 0; k < i; k++)
	{
		GetMenuString(gui.hMenu1964main, k, str, 80, MF_BYPOSITION);
		if(strcmp(str, "&File") == 0)
		{
			file_submenu = GetSubMenu(gui.hMenu1964main, k);
			j = GetMenuItemCount(file_submenu);
			for(n = j - 1; n >= 0; n--) /* I have to delete the menu in reverse order */
			{
				GetMenuString(file_submenu, n, str, 80, MF_BYPOSITION);
				if(strcmp(str, "Recent ROM Folders") == 0)
				{
					recent_rom_directory_submenu = GetSubMenu(file_submenu, n);
					recent_rom_directory_submenu_pos = n;
					RemoveMenu(file_submenu, n, MF_BYPOSITION);
				}
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RegenerateStateSelectorMenus(void)
{
//	if(guioptions.show_expert_user_menu)
	{
		InsertMenu
		(
			CPU_submenu,
			state_save_submenu_pos,
			MF_BYPOSITION | MF_POPUP,
			(UINT) state_save_submenu,
			"Save State\tF5"
		);
		InsertMenu
		(
			CPU_submenu,
			state_save_submenu_pos,
			MF_BYPOSITION | MF_POPUP,
			(UINT) state_load_submenu,
			"Load State\tF7"
		);
	}
//	else
//	{
//		AppendMenu(CPU_submenu, MF_POPUP, (UINT) state_save_submenu, "Save State\tF5");
//		AppendMenu(CPU_submenu, MF_POPUP, (UINT) state_load_submenu, "Load State\tF7");
//	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DeleteStateSelectorMenus(void)
{
	/*~~~~~~~~~~~~~~~*/
	int		k, i, j, n;
	char	str[100];
	/*~~~~~~~~~~~~~~~*/

	i = GetMenuItemCount(gui.hMenu1964main);
	for(k = 0; k < i; k++)
	{
		GetMenuString(gui.hMenu1964main, k, str, 80, MF_BYPOSITION);
		if(strcmp(str, "CPU") == 0)
		{
			CPU_submenu = GetSubMenu(gui.hMenu1964main, k);
			j = GetMenuItemCount(CPU_submenu);
			for(n = j - 1; n >= 0; n--) /* I have to delete the menu in reverse order */
			{
				GetMenuString(CPU_submenu, n, str, 80, MF_BYPOSITION);

				/* Delete all cpu core switch menus */
				if(strcmp(str, "Load State\tF7") == 0)
				{
					state_load_submenu = GetSubMenu(CPU_submenu, n);
					state_load_submenu_pos = n;
					RemoveMenu(CPU_submenu, n, MF_BYPOSITION);
				}
				else if(strcmp(str, "Save State\tF5") == 0)
				{
					state_save_submenu = GetSubMenu(CPU_submenu, n);
					state_save_submenu_pos = n;
					RemoveMenu(CPU_submenu, n, MF_BYPOSITION);
				}
			}
		}
	}
}

int Separator = 0;
/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RegenerateAdvancedUserMenus(void)
{
	AppendMenu(CPU_submenu, MF_POPUP, (UINT) switch_compiler_submenu, "Switch Compiler");
	AppendMenu(CPU_submenu, MF_POPUP, (UINT) dyna_code_checking_submenu, "Self-Modifying Code Detection Method");
	AppendMenu(CPU_submenu, MF_POPUP, (UINT) counter_hack_submenu, "Counter Factor");
#ifdef DEBUG_COMMON
	InsertMenu(file_submenu, ID_PREFERENCE_OPTIONS, MF_UNCHECKED, ID_DEFAULTOPTIONS, "Advanced Options ...");
#endif
	InsertMenu(CPU_submenu, ID_SAVESTATE, MF_GRAYED, ID_CPU_IMPORTPJ64STATE, "Import Project64 Save State...");
	InsertMenu(CPU_submenu, ID_SAVESTATE, MF_GRAYED, ID_CPU_EXPORTPJ64STATE, "Export Project64 Save State...");
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DeleteAdvancedUserMenus(void)
{
	/*~~~~~~~~~~~~~~~*/
	int		k, i, j, n;
	char	str[100];
	/*~~~~~~~~~~~~~~~*/

	i = GetMenuItemCount(gui.hMenu1964main);
	for(k = 0; k < i; k++)
	{
		GetMenuString(gui.hMenu1964main, k, str, 80, MF_BYPOSITION);
		if(strcmp(str, "CPU") == 0)
		{
			CPU_submenu = GetSubMenu(gui.hMenu1964main, k);
			j = GetMenuItemCount(CPU_submenu);
			for(n = j - 1; n >= 0; n--) /* I have to delete the menu in reverse order */
			{
				GetMenuString(CPU_submenu, n, str, 80, MF_BYPOSITION);

				/* Delete all cpu core switch menus */
				if(strcmp(str, "Switch Compiler") == 0)
				{
					switch_compiler_submenu = GetSubMenu(CPU_submenu, n);
					GetMenuItemInfo(CPU_submenu, n, MF_BYPOSITION, &switch_compiler_submenu_info);
					switch_compiler_submenu_pos = n;
					RemoveMenu(CPU_submenu, n, MF_BYPOSITION);
				}

				/* Delete all code check method switch menus */
				else if(strcmp(str, "Self-Modifying Code Detection Method") == 0)
				{
					dyna_code_checking_submenu = GetSubMenu(CPU_submenu, n);
					dyna_code_checking_submenu_pos = n;
					RemoveMenu(CPU_submenu, n, MF_BYPOSITION);
				}

				/* Delete all Counter Hack menus */
				else if(strcmp(str, "Counter Factor") == 0)
				{
					counter_hack_submenu = GetSubMenu(CPU_submenu, n);
					counter_hack_submenu_pos = n;
					RemoveMenu(CPU_submenu, n, MF_BYPOSITION);
				}

				/*
				 * else if( strnicmp(str+2,"port Project64",14 )==0 ) £
				 * { £
				 * RemoveMenu(CPU_submenu, n, MF_BYPOSITION); £
				 * }
				 */
			}
		}
	}

	GetMenuItemInfo(gui.hMenu1964main, ID_DEFAULTOPTIONS, MF_BYCOMMAND, &advanced_options_menuitem);
	RemoveMenu(gui.hMenu1964main, ID_DEFAULTOPTIONS, MF_BYCOMMAND);
	RemoveMenu(gui.hMenu1964main, ID_CPU_IMPORTPJ64STATE, MF_BYCOMMAND);
	RemoveMenu(gui.hMenu1964main, ID_CPU_EXPORTPJ64STATE, MF_BYCOMMAND);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SetupAdvancedMenus(void)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < MAX_RECENT_ROM_DIR; i++)
	{
		ModifyMenuText(recent_rom_directory_menu_ids[i], recent_rom_directory_lists[i]);
	}

	for(i = 0; i < MAX_RECENT_GAME_LIST; i++)
	{
		ModifyMenuText(recent_game_menu_ids[i], recent_game_lists[i]);
	}

	if(guioptions.show_expert_user_menu == FALSE) DeleteAdvancedUserMenus();
#ifndef DEBUG_COMMON
	else
	{	/* hide the default option menu from RELEASE MODE */
		RemoveMenu(gui.hMenu1964main, ID_DEFAULTOPTIONS, MF_BYCOMMAND);
	}
#endif
	if(guioptions.show_recent_rom_directory_list == FALSE) DeleteRecentRomDirectoryMenus();
	if(guioptions.show_recent_game_list == FALSE) DeleteRecentGameMenus();
	if(guioptions.show_state_selector_menu == FALSE) DeleteStateSelectorMenus();
	if(!emuoptions.SyncVI) CheckMenuItem(gui.hMenu1964main, ID_CPU_AUDIOSYNC, MF_UNCHECKED);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CaptureScreenToFile(void)
{
	if(emustatus.Emu_Is_Running)
	{
		if(GfxPluginVersion != 0x0103)
			DisplayError("Current video plugin does not support screen capture");
		else
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
			char	directory[_MAX_PATH];
			/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

			strcpy(directory, directories.main_directory);
			strcat(directory, "screens\\");
			VIDEO_CaptureScreen(directory);
		}
	}
}

static BOOL exiting_1964 = FALSE;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Exit1964(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~*/
	WINDOWPLACEMENT placement;
	/*~~~~~~~~~~~~~~~~~~~~~~*/

	if(exiting_1964) exit(0);

	exiting_1964 = TRUE;

	SetStatusBarText(0, "Exiting 1964");

	if(emustatus.Emu_Is_Running) Stop();

	placement.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(gui.hwnd1964main, &placement);
	guistatus.clientwidth = placement.rcNormalPosition.right - placement.rcNormalPosition.left;
	guistatus.clientheight = placement.rcNormalPosition.bottom - placement.rcNormalPosition.top;
	guistatus.window_position.left = placement.rcNormalPosition.left;
	guistatus.window_position.top = placement.rcNormalPosition.top;
	guistatus.WindowIsMaximized = (placement.showCmd == SW_SHOWMAXIMIZED);

	RomListRememberColumnWidth();

	Close_iPIF();			/* save mempak and eeprom */

	FreeVirtualMemory();

	FileIO_Write1964Ini();	/* Save 1964.ini */
	WriteConfiguration();
	DeleteAllIniEntries();	/* Release all ini entries */
	ClearRomList();			/* Clean the Rom List */

	unload_netplay_dll();
	FreePlugins();

	UnloadDllKaillera();

	/*
	 * Here is the fix for the problem that 1964 crash when exiting if using opengl
	 * plugins. £
	 * I don't know why 1964 crash, looks like crash is not happen in 1964, but dll
	 * related. £
	 * just doing exit(0) will not crash,(maybe we have left some resource not
	 * released, donno)
	 */
	exit(0);

	/* PostQuitMessage(0); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_Ready_Message(void)
{
	if(Rom_Loaded)
	{
		if(strlen(currentromoptions.Alt_Title) < 1 || strcmp(rominfo.name, currentromoptions.Game_Name) != 0)
			sprintf(generalmessage, "Ready - %s - [%s]", directories.rom_directory_to_use, rominfo.name);
		else
		{
			sprintf(generalmessage, "Ready - %s - [%s]", directories.rom_directory_to_use, currentromoptions.Alt_Title);
		}
	}
	else
	{
		sprintf(generalmessage, "Ready - %s", directories.rom_directory_to_use);
	}

	SetStatusBarText(0, generalmessage);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DisableDebugMenu(void)
{
	/*~~~~~~~~~~~~*/
	/* Disable the DEBUG menu */
	int		i, k;
	char	str[80];
	/*~~~~~~~~~~~~*/

	i = GetMenuItemCount(gui.hMenu1964main);
	for(k = 0; k < i; k++)
	{
		GetMenuString(gui.hMenu1964main, k, str, 80, MF_BYPOSITION);
		if(strcmp(str, "Debug") == 0)
		{
			DeleteMenu(gui.hMenu1964main, k, MF_BYPOSITION);
		}
	}
}

/*
 *	Delete the netplay menu
 */
void DisableNetplayMemu(void)
{
	int		i, k;
	char	str[80];
	/*~~~~~~~~~~~~*/

	i = GetMenuItemCount(gui.hMenu1964main);
	for(k = 0; k < i; k++)
	{
		GetMenuString(gui.hMenu1964main, k, str, 80, MF_BYPOSITION);
		if(strcmp(str, "Net Play") == 0)
		{
			DeleteMenu(gui.hMenu1964main, k, MF_BYPOSITION);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SetupDebuger(void)
{
#ifdef DEBUG_COMMON
	CheckMenuItem(gui.hMenu1964main, ID_DEBUG_CONTROLLER, debugoptions.debug_si_controller ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, ID_DEBUGSPTASK, debugoptions.debug_sp_task ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, ID_DEBUGSITASK, debugoptions.debug_si_task ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, ID_DEBUGSPDMA, debugoptions.debug_sp_dma ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, ID_DEBUGSIDMA, debugoptions.debug_si_dma ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, ID_DEBUGPIDMA, debugoptions.debug_pi_dma ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, ID_DEBUGMEMPAK, debugoptions.debug_si_mempak ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, ID_DEBUGTLB, debugoptions.debug_tlb ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, ID_DEBUGEEPROM, debugoptions.debug_si_eeprom ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(gui.hMenu1964main, ID_DEBUG_SRAM, debugoptions.debug_sram ? MF_CHECKED : MF_UNCHECKED);

	DebuggerBreakPointActive = FALSE;
	OpCount = 0;
	NextClearCode = 250;
	BreakAddress = -1;
	DebuggerActive = FALSE;
	OpenDebugger();
#else
	DisableDebugMenu();
#endif
}

long OnNotifyStatusBar(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if( ((LPNMHDR) lParam)->code == NM_DBLCLK )
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		int fieldno = ((LPNMLISTVIEW) lParam)->iItem;
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		
		switch(fieldno)
		{
		case 2:						/* Counter Factor */
			/* Reset Counter Factor to default value */
			SendMessage(gui.hwnd1964main, WM_COMMAND, ID_CF_CF1, 0);
			break;
		case 4:						/* CPU core */
			/* Switch CPU core */
			if(emustatus.Emu_Is_Running)
			{
				SendMessage
					(
					gui.hwnd1964main,
					WM_COMMAND,
					emustatus.cpucore == DYNACOMPILER ? ID_INTERPRETER : ID_DYNAMICCOMPILER,
					0
					);
			}
			else
			{
				SendMessage
					(
					gui.hwnd1964main,
					WM_COMMAND,
					defaultoptions.Emulator == DYNACOMPILER ? ID_INTERPRETER : ID_DYNAMICCOMPILER,
					0
					);
			}
			break;
		}
	}

	return 0l;
}

long OnPopupMenuCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case ID_POPUP_LOADPLAY:
		if( Rom_Loaded )
		{
			Play(emuoptions.auto_full_screen);
		}
		else
		{
			RomListOpenRom(RomListGetSelectedIndex(), TRUE);
		}
		break;
	case ID_POPUP_LOADPLAYINFULLSCREEN:
		RomListOpenRom(RomListGetSelectedIndex(), FALSE);
		Play(TRUE);
		break;
	case ID_POPUP_LOADPLAYINWINDOWMODE:
		RomListOpenRom(RomListGetSelectedIndex(), FALSE);
		Play(FALSE);
		break;
	case ID_POPUP_ROM_SETTING:
		RomListRomOptions(selected_rom_index);
		break;
	case ID_POPUP_CHEATCODE:
		if(emustatus.Emu_Is_Running)
		{
			PauseEmulator();
			//SuspendThread(CPUThreadHandle);
			DialogBox(gui.hInst, "CHEAT_HACK", hWnd, (DLGPROC) CheatAndHackDialog);
			//ResumeThread(CPUThreadHandle);
			ResumeEmulator(DO_NOTHING_AFTER_PAUSE);
		}
		else
		{
			CodeList_ReadCode(romlist[selected_rom_index]->pinientry->Game_Name);
			DialogBox(gui.hInst, "CHEAT_HACK", hWnd, (DLGPROC) CheatAndHackDialog);
		}
		break;
	case ID_HEADERPOPUP_SHOW_INTERNAL_NAME:
		romlistNameToDisplay = ROMLIST_DISPLAY_INTERNAL_NAME;
		SendMessage(gui.hwnd1964main, WM_COMMAND, ID_FILE_FRESHROMLIST, 0);
		break;
	case ID_HEADERPOPUP_SHOWALTERNATEROMNAME:
		romlistNameToDisplay = ROMLIST_DISPLAY_ALTER_NAME;
		SendMessage(gui.hwnd1964main, WM_COMMAND, ID_FILE_FRESHROMLIST, 0);
		break;
	case ID_HEADERPOPUP_SHOWROMFILENAME:
		romlistNameToDisplay = ROMLIST_DISPLAY_FILENAME;
		SendMessage(gui.hwnd1964main, WM_COMMAND, ID_FILE_FRESHROMLIST, 0);
		break;
	case ID_HEADERPOPUP_1_SORT_ASCENDING:
		romlist_sort_method = 0;
		NewRomList_Sort();
		NewRomList_ListViewFreshRomList();
		break;
	case ID_HEADERPOPUP_1_SORT_DESCENDING:
		romlist_sort_method = 4;
		NewRomList_Sort();
		NewRomList_ListViewFreshRomList();
		break;
	case ID_HEADERPOPUP_2_SORT_ASCENDING:
		romlist_sort_method = romListHeaderClickedColumn;
		NewRomList_Sort();
		NewRomList_ListViewFreshRomList();
		break;
	case ID_HEADERPOPUP_2_SORT_DESCENDING:
		romlist_sort_method = romListHeaderClickedColumn+4;
		NewRomList_Sort();
		NewRomList_ListViewFreshRomList();
		break;
	case ID_HEADERPOPUP_1_SELECTING:
	case ID_HEADERPOPUP_2_SELECTING:
		DialogBox(gui.hInst, "COL_SELECT", hWnd, (DLGPROC) ColumnSelectDialog);
		break;
	}

	return 0l;

}

long OnOpcodeDebuggerCommands(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
		case ID_OPCODEDEBUGGER:
            debug_opcode = 1 - debug_opcode;
			if(debug_opcode!=0)
				CheckMenuItem(gui.hMenu1964main, ID_OPCODEDEBUGGER, MF_CHECKED);
			else
				CheckMenuItem(gui.hMenu1964main, ID_OPCODEDEBUGGER, MF_UNCHECKED);

			if(opcode_debugger_memory_is_allocated == FALSE)
			{
				/* now we allocate the allocate the debugger */
				InitVirtualMemory1(&gMemoryState_Interpreter_Compare);
				InitMemoryLookupTables();
				gMemoryState_Interpreter_Compare.ROM_Image = gMemoryState.ROM_Image;

				/*
				 * TODO: here we need to copy the memorystate and hardware state to £
				 * interpreter_compare. Actually, it is probably better to just use the same
				 * method £
				 * you use when switching from interpreter to dyna, because a few £
				 * other things need to be initialized, like rdram size pointers for £
				 * interpreter_compare.
				 */
				opcode_debugger_memory_is_allocated = TRUE;
				TRACE0("Allocate memory for opcode debugger");
			}

			if(emustatus.Emu_Is_Running)
			{
				if(PauseEmulator())
				{
					Debugger_Copy_Memory(&gMemoryState_Interpreter_Compare, &gMemoryState);
					memcpy(&gHardwareState_Interpreter_Compare, &gHardwareState, sizeof(HardwareState));
					ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);	/* Need to init emu */
				}
			}
			break;
		case ID_OPCODEDEBUGGER_BLOCK_ONLY:
			debug_opcode_block = 1 - debug_opcode_block;
			if(debug_opcode_block)
				CheckMenuItem(gui.hMenu1964main, ID_OPCODEDEBUGGER_BLOCK_ONLY, MF_CHECKED);
			else
				CheckMenuItem(gui.hMenu1964main, ID_OPCODEDEBUGGER_BLOCK_ONLY, MF_UNCHECKED);
			if(emustatus.Emu_Is_Running)
			{
				if(debug_opcode != 1 && debug_opcode_block)
				{
                    debug_opcode = 1;
					CheckMenuItem(gui.hMenu1964main, ID_OPCODEDEBUGGER, MF_CHECKED);
				}

				if(PauseEmulator())
				{
					Debugger_Copy_Memory(&gMemoryState_Interpreter_Compare, &gMemoryState);
					memcpy(&gHardwareState_Interpreter_Compare, &gHardwareState, sizeof(HardwareState));
					ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);	/* Need to init emu */
				}
			}
			break;
	}

	return 0l;
}

void OnFreshRomList()
{
	if( !emustatus.Emu_Is_Running )
	{
		NewRomList_ListViewChangeWindowRect();
		DockStatusBar();
		ClearRomList();
		NewRomList_ListViewFreshRomList();
		SetStatusBarText(0, "Looking for ROM file in the ROM directory and Generating List");
		RomListReadDirectory(directories.rom_directory_to_use);
		NewRomList_ListViewFreshRomList();
		Set_Ready_Message();
	}
}

