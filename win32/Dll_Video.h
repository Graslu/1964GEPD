/*$T Dll_Video.h GC 1.136 02/28/02 09:00:18 */


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
#ifndef _VIDEO_H__1964_
#define _VIDEO_H__1964_

#include "../plugins.h"

extern BOOL (__cdecl *_VIDEO_InitiateGFX) (GFX_INFO);
extern void (__cdecl *_VIDEO_ProcessDList) (void);
extern void (__cdecl *_VIDEO_RomOpen) (void);
extern void (__cdecl *_VIDEO_RomClosed) (void);
extern void (__cdecl *_VIDEO_DllClose) (void);
extern void (__cdecl *_VIDEO_DllConfig) (HWND);
extern void (__cdecl *_VIDEO_GetDllInfo) (PLUGIN_INFO *);
extern void (__cdecl *_VIDEO_UpdateScreen) (void);
extern void (__cdecl *_VIDEO_Test) (HWND);
extern void (__cdecl *_VIDEO_About) (HWND);
extern void (__cdecl *_VIDEO_MoveScreen) (int, int);
extern void (__cdecl *_VIDEO_ChangeWindow) (int);
extern void (__cdecl *_VIDEO_ExtraChangeResolution) (HWND, long, HWND);
extern void (__cdecl *_VIDEO_DrawScreen) (void);
extern void (__cdecl *_VIDEO_ViStatusChanged) (void);
extern void (__cdecl *_VIDEO_ViWidthChanged) (void);

/* changes for spec 1.3 */
extern void (__cdecl *_VIDEO_ChangeWindow_1_3) (void);
extern void (__cdecl *_VIDEO_CaptureScreen) (char *Directory);
extern void (__cdecl *_VIDEO_ProcessRDPList) (void);
extern void (__cdecl *_VIDEO_ShowCFB) (void);

extern void VIDEO_GetDllInfo(PLUGIN_INFO *);
extern void VIDEO_DllClose(void);
extern BOOL VIDEO_InitiateGFX(GFX_INFO);
extern void VIDEO_ProcessDList(void);
extern void VIDEO_RomOpen(void);
extern void VIDEO_RomClosed(void);
extern BOOL LoadVideoPlugin(char *libname);
extern void CloseVideoPlugin(void);
extern void VIDEO_DllConfig(HWND);
extern void VIDEO_About(HWND);
extern void VIDEO_Test(HWND);
extern void VIDEO_MoveScreen(int, int);
extern void VIDEO_ExtraChangeResolution(HWND, long, HWND);
extern void VIDEO_ChangeWindow(int);
extern void VIDEO_DrawScreen(void);
extern void VIDEO_ViStatusChanged(void);
extern void VIDEO_UpdateScreen(void);
extern void VIDEO_ViWidthChanged(void);

/* changes for spec 1.3 */
extern void VIDEO_ChangeWindow_1_3(void);
extern void VIDEO_CaptureScreen(char *Directory);
extern void VIDEO_ProcessRDPList(void);
extern void VIDEO_ShowCFB(void);

/* Used when video plugin is being selected */
extern void VIDEO_Under_Selecting_About(HWND);
extern void VIDEO_Under_Selecting_Test(HWND);
extern void (__cdecl *_VIDEO_Under_Selecting_Test) (HWND);
extern void (__cdecl *_VIDEO_Under_Selecting_About) (HWND);

extern HINSTANCE	hinstLibVideo;
extern uint16		GfxPluginVersion;


typedef struct g_GFX_PluginRECT
{
	RECT rect;
	BOOL UseThis;
} g_GFX_PluginRECT;

extern g_GFX_PluginRECT GFX_PluginRECT;
#endif
