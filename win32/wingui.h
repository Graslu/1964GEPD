/*$T wingui.h GC 1.136 02/28/02 07:53:40 */


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
#ifndef _WINGUI_H__1964_
#define _WINGUI_H__1964_
#include "resource.h"

/* Functions in the wingui.c */
int APIENTRY		WinMain(HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow);
HWND				InitWin98UI(HANDLE hInstance, int nCmdShow);
LRESULT APIENTRY	MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY	About(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
void __cdecl		DisplayError(char *Message, ...);
void __cdecl		DisplayCriticalMessage(char *Message, ...);

BOOL				WinLoadRom(void);
BOOL				WinLoadRomStep2(char *szFileName);
void				Pause(void);
void				Resume(void);
void				Kill(void);
void				Play(BOOL WithFullScreen);
void				Stop(void);
void				OpenROM(void);
void				CloseROM(void);
void				ChangeDirectory(void);
void				SaveState(void);
void				LoadState(void);
void				SaveStateByNumber(WPARAM wparam);
void				LoadStateByNumber(WPARAM wparam);
void				SaveStateByDialog(int format);
void				LoadStateByDialog(int format);
void				EnableStateMenu(void);
void				DisableStateMenu(void);
void				SetWindowBorderless(void);
void				UnsetWindowBorderless(void);
void				PrepareBeforePlay(int IsFullScreen);
void				KillCPUThread(void);
void				SetOverclockFactor(int factor);
void				SetCounterFactor(int);
void				GEFiringRateHack(void);
void				PDTimingHack(void);
void				PDSpeedHack(void);
void				GEPDPause(BOOL pause);
void				SetCodeCheckMethod(int);
void				InitPluginData(void);
void				Set_1964_Directory(void);
void				CountryCodeToCountryName_and_TVSystem(int countrycode, char *countryname, int *tvsystem);
void				CaptureScreenToFile(void);
void				Set_Ready_Message(void);
void				DisableDebugMenu(void);
void				SetupDebuger(void);
void				SaveCmdLineParameter(char *cmdline);
BOOL				StartGameByCommandLine();
void				SetHighResolutionTimer(void);

void				StateSetNumber(int number);
void				Exit1964(void);
void				GetPluginDir(char *Directory);
extern int			LoadGNUDistConditions(char *ConditionsBuf);
LRESULT APIENTRY	ConditionsDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
LRESULT APIENTRY	DefaultOptionsDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
LRESULT APIENTRY	CheatAndHackDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
LRESULT APIENTRY	CriticalMessageDialog(HWND hDlg, unsigned message, WORD wParam, LONG lParam);

#define MAXFILENAME 256					/* maximum length of file pathname */

/* the legal stuff */
unsigned char	MainDisclaimer[320];
unsigned char	WarrantyPart1[700];
unsigned char	WarrantyPart2[700];
unsigned char	DistConditions[800];	/* GNU Redistribution Conditions */

struct EMU1964GUI
{
	char		*szBaseWindowTitle;
	HINSTANCE	hInst;
	HANDLE		hAccTable;				/* handle to accelerator table */
	HWND		hwnd1964main;			/* handle to main window */
	HWND		hwndRomList;			/* Handle to the rom list child window */
	HWND		hStatusBar;				/* Window Handle of the status bar */
	HWND		hToolBar;				/* Window Handle of the toolbar */
	HWND		hReBar;					/* Window Handle of the rebar */
	HWND		hClientWindow;			/* Window handle of the client child window */
	HWND		hCriticalMsgWnd;		/* handle to critical message window */
	HMENU		hMenu1964main;
	HMENU		hMenuRomListPopup;

	struct
	{
		/* Status Bar text fields */
		char	field_1[256];
		char	field_2[80];
		char	field_3[80];
		char	field_4[80];
		char	field_5[80];
	} staturbar_field;
	char	szWindowTitle[80];
};

extern struct EMU1964GUI	gui;

struct GUIOPTIONS
{
	BOOL	pause_at_inactive;
	BOOL	pause_at_menu;
	BOOL	ok_to_pause_at_menu;
	BOOL	use_default_save_directory;
	BOOL	use_default_state_save_directory;
	BOOL	use_default_plugin_directory;
	BOOL	use_last_rom_directory;
	BOOL	show_expert_user_menu;
	BOOL	show_recent_rom_directory_list;
	BOOL	show_recent_game_list;
	BOOL	display_detail_status;
	BOOL	display_profiler_status;
	BOOL	show_state_selector_menu;
	BOOL	show_critical_msg_window;
	BOOL	display_romlist;
	BOOL	display_statusbar;
	BOOL	highfreqtimer;
	BOOL	borderless_fullscreen;
	BOOL	hide_cursor_on_launch;
};

extern struct GUIOPTIONS	guioptions;

struct GUISTATUS
{
	int		clientwidth;				/* Client window width */
	int		clientheight;				/* Client window height */
	RECT	window_position;			/* 1964 main window location */
	BOOL	WindowIsMaximized;
	BOOL	window_is_moving;
	BOOL	window_is_maximized;
	BOOL	window_is_minimized;
	BOOL	block_menu;					/* Block all menu command while 1964 is busy */
	int		IsFullScreen;
	BOOL	IsBorderless;
};
extern struct GUISTATUS guistatus;

extern void				DockStatusBar(void);
extern void				InitStatusBarParts(void);
extern void				SetStatusBarText(int, char *);

#define MAX_RECENT_ROM_DIR		8
#define MAX_RECENT_GAME_LIST	8
char					recent_rom_directory_lists[MAX_RECENT_ROM_DIR][260];
char					recent_game_lists[MAX_RECENT_GAME_LIST][260];

struct DIRECTORIES
{
	char	main_directory[256];
	char	plugin_directory_to_use[256];
	char	save_directory_to_use[256];
	char	rom_directory_to_use[256];
	char	last_rom_directory[256];
};

extern struct DIRECTORIES	directories;

extern char					game_country_name[10];
extern int					game_country_tvsystem;

enum { LOAD_ALL_PLUGIN, LOAD_VIDEO_PLUGIN, LOAD_AUDIO_PLUGIN, LOAD_INPUT_PLUGIN, LOAD_RSP_PLUGIN };

void FreePlugins(void);
void LoadPlugins(int type);

enum { SAVE_STATE_1964_FORMAT, SAVE_STATE_PJ64_FORMAT };

typedef enum {
	CMDLINE_AUDIO_PLUGIN,
	CMDLINE_VIDEO_PLUGIN,
	CMDLINE_CONTROLLER_PLUGIN,
	CMDLINE_ROM_DIR,
	CMDLINE_GAME_FILENAME,
	CMDLINE_FULL_SCREEN_FLAG,
	CMDLINE_MAX_NUMBER
}CmdLineParameterType;

void GetCmdLineParameter(CmdLineParameterType arg, char *buf);

#endif
