/*$T Dll_Input.c GC 1.136 03/09/02 17:41:27 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Input plugin interface functions
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
#include "../globals.h"
#include "registry.h"
#include "../plugins.h"
#include "DLL_Input.h"

CONTROL		Controls[4];

/* Input plugin */
HINSTANCE	hinstControllerPlugin = NULL;
int			mouseinjectorpresent = 0;

void (__cdecl *_CONTROLLER_CloseDLL) (void) = NULL;
void (__cdecl *_CONTROLLER_ControllerCommand) (int Control, BYTE *Command) = NULL;
void (__cdecl *_CONTROLLER_DllAbout) (HWND) = NULL;
void (__cdecl *_CONTROLLER_DllConfig) (HWND) = NULL;
void (__cdecl *_CONTROLLER_DllTest) (HWND) = NULL;
void (__cdecl *_CONTROLLER_GetDllInfo) (PLUGIN_INFO *) = NULL;
void (__cdecl *_CONTROLLER_GetKeys) (int Control, BUTTONS *Keys) = NULL;
void (__cdecl *_CONTROLLER_InitiateControllers) (HWND hMainWindow, CONTROL Controls[4]) = NULL;
void (__cdecl *_CONTROLLER_ReadController) (int Control, BYTE *Command) = NULL;
void (__cdecl *_CONTROLLER_RomClosed) (void) = NULL;
void (__cdecl *_CONTROLLER_RomOpen) (void) = NULL;
void (__cdecl *_CONTROLLER_WM_KeyDown) (WPARAM wParam, LPARAM lParam) = NULL;
void (__cdecl *_CONTROLLER_WM_KeyUp) (WPARAM wParam, LPARAM lParam) = NULL;
void (__cdecl *_CONTROLLER_HookRDRAM) (DWORD *Mem, int OCFactor) = NULL;
void (__cdecl *_CONTROLLER_HookROM) (DWORD *Rom) = NULL;
void (__cdecl *_CONTROLLER_Under_Selecting_DllAbout) (HWND) = NULL;
void (__cdecl *_CONTROLLER_Under_Selecting_DllTest) (HWND) = NULL;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL LoadControllerPlugin(char *libname)
{
	mouseinjectorpresent = 0;
	if(hinstControllerPlugin != NULL)
	{
		CloseControllerPlugin();
	}

	/* Load the input plugin DLL */
	hinstControllerPlugin = LoadLibrary(libname);

	if(hinstControllerPlugin != NULL)	/* Check if DLL is loaded successfully */
	{
		/* Get function address _CONTROLLER_GetDllInfo */
		_CONTROLLER_GetDllInfo = (void(__cdecl *) (PLUGIN_INFO *)) GetProcAddress(hinstControllerPlugin, "GetDllInfo");

		if(_CONTROLLER_GetDllInfo)		/* Is there the function _CONTROLLER_GetDllInfo in the Input plugin DLL */
		{
			/*~~~~~~~~~~~~~~~~~~~~*/
			PLUGIN_INFO Plugin_Info;
			/*~~~~~~~~~~~~~~~~~~~~*/

			ZeroMemory(&Plugin_Info, sizeof(Plugin_Info));

			_CONTROLLER_GetDllInfo(&Plugin_Info);

			if(Plugin_Info.Type == PLUGIN_TYPE_CONTROLLER)
			{
				if(Plugin_Info.Version == CONTROLLER_VERSION || Plugin_Info.Version == 0xFBAD)
				{
					_CONTROLLER_CloseDLL = (void(__cdecl *) (void)) GetProcAddress(hinstControllerPlugin, "CloseDLL");
					_CONTROLLER_ControllerCommand = (void(__cdecl *) (int Control, BYTE *Command)) GetProcAddress
						(
							hinstControllerPlugin,
							"ControllerCommand"
						);
					_CONTROLLER_DllAbout = (void(__cdecl *) (HWND)) GetProcAddress(hinstControllerPlugin, "DllAbout");
					_CONTROLLER_DllConfig = (void(__cdecl *) (HWND)) GetProcAddress(hinstControllerPlugin, "DllConfig");
					_CONTROLLER_DllTest = (void(__cdecl *) (HWND)) GetProcAddress(hinstControllerPlugin, "DllTest");
					_CONTROLLER_GetDllInfo = (void(__cdecl *) (PLUGIN_INFO *)) GetProcAddress
						(
							hinstControllerPlugin,
							"GetDllInfo"
						);
					_CONTROLLER_GetKeys = (void(__cdecl *) (int Control, BUTTONS *Keys)) GetProcAddress
						(
							hinstControllerPlugin,
							"GetKeys"
						);
					_CONTROLLER_InitiateControllers =
						(void(__cdecl *) (HWND hMainWindow, CONTROL Controls[4])) GetProcAddress
							(
								hinstControllerPlugin,
								"InitiateControllers"
							);
					_CONTROLLER_ReadController = (void(__cdecl *) (int Control, BYTE *Command)) GetProcAddress
						(
							hinstControllerPlugin,
							"ReadController"
						);
					_CONTROLLER_RomClosed = (void(__cdecl *) (void)) GetProcAddress(hinstControllerPlugin, "RomClosed");
					_CONTROLLER_RomOpen = (void(__cdecl *) (void)) GetProcAddress(hinstControllerPlugin, "RomOpen");
					_CONTROLLER_WM_KeyDown = (void(__cdecl *) (WPARAM wParam, LPARAM lParam)) GetProcAddress
						(
							hinstControllerPlugin,
							"WM_KeyDown"
						);
					_CONTROLLER_WM_KeyUp = (void(__cdecl *) (WPARAM wParam, LPARAM lParam)) GetProcAddress
						(
							hinstControllerPlugin,
							"WM_KeyUp"
						);
					if(Plugin_Info.Version == 0xFBAD)
					{
						_CONTROLLER_HookRDRAM = (void(__cdecl *) (DWORD *Mem, int OCFactor)) GetProcAddress
							(
								hinstControllerPlugin,
								"HookRDRAM"
							);
						_CONTROLLER_HookROM = (void(__cdecl *) (DWORD *Rom)) GetProcAddress
							(
								hinstControllerPlugin,
								"HookROM"
							);
						mouseinjectorpresent = 1;
					}
					return(TRUE);
				}
			}
		}
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CloseControllerPlugin(void)
{
	if(_CONTROLLER_RomClosed)
	{
		_CONTROLLER_RomClosed();
	}

	if(_CONTROLLER_CloseDLL)
	{
		_CONTROLLER_CloseDLL();
	}

	/* if(_CONTROLLER_RomClosed) { _CONTROLLER_RomClosed(); } */
	if(hinstControllerPlugin)
	{
		FreeLibrary(hinstControllerPlugin);
	}

	hinstControllerPlugin = NULL;

	_CONTROLLER_CloseDLL = NULL;
	_CONTROLLER_ControllerCommand = NULL;
	_CONTROLLER_DllAbout = NULL;
	_CONTROLLER_DllConfig = NULL;
	_CONTROLLER_DllTest = NULL;
	_CONTROLLER_GetDllInfo = NULL;
	_CONTROLLER_GetKeys = NULL;
	_CONTROLLER_InitiateControllers = NULL;
	_CONTROLLER_ReadController = NULL;
	_CONTROLLER_RomClosed = NULL;
	_CONTROLLER_RomOpen = NULL;
	_CONTROLLER_WM_KeyDown = NULL;
	_CONTROLLER_WM_KeyUp = NULL;
	_CONTROLLER_HookRDRAM = NULL;
	_CONTROLLER_HookROM = NULL;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_CloseDLL(void)
{
	if(_CONTROLLER_CloseDLL != NULL)
	{
		__try
		{
			_CONTROLLER_CloseDLL();
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_ControllerCommand(int _Control, BYTE *_Command)
{
	if(_CONTROLLER_ControllerCommand != NULL)
	{
		__try
		{
			_CONTROLLER_ControllerCommand(_Control, _Command);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_DllAbout(HWND _hWnd)
{
	if(_CONTROLLER_DllAbout != NULL)
	{
		__try
		{
			_CONTROLLER_DllAbout(_hWnd);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_DllConfig(HWND _hWnd)
{
	if(_CONTROLLER_DllConfig != NULL)
	{
		__try
		{
			_CONTROLLER_DllConfig(_hWnd);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_DllTest(HWND _hWnd)
{
	if(_CONTROLLER_DllTest != NULL)
	{
		__try
		{
			_CONTROLLER_DllTest(_hWnd);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_GetDllInfo(PLUGIN_INFO *_plugin)
{
	if(_CONTROLLER_GetDllInfo != NULL)
	{
		__try
		{
			_CONTROLLER_GetDllInfo(_plugin);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_GetKeys(int _Control, BUTTONS *_Keys)
{
	if(_CONTROLLER_GetKeys != NULL)
	{
		__try
		{
			_CONTROLLER_GetKeys(_Control, _Keys);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_InitiateControllers(HWND _hMainWindow, CONTROL _Controls[4])
{
	if(_CONTROLLER_InitiateControllers != NULL)
	{
		__try
		{
			_CONTROLLER_InitiateControllers(_hMainWindow, _Controls);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

	/*
	 * Add mempak support in 1964 by configure the control 4 as mempak £
	 * no matter if the control plugin support it or not
	 */
	if(_Controls[0].Plugin == PLUGIN_NONE) _Controls[0].Plugin = PLUGIN_MEMPAK;

	/*
	 * Controls[1].Present = FALSE; £
	 * _Controls[2].Present = FALSE; £
	 * _Controls[3].Present = FALSE;
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_ReadController(int _Control, BYTE *_Command)
{
	if(_CONTROLLER_ReadController != NULL)
	{
		__try
		{
			_CONTROLLER_ReadController(_Control, _Command);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_RomClosed(void)
{
	if(_CONTROLLER_RomClosed != NULL)
	{
		__try
		{
			_CONTROLLER_RomClosed();
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_RomOpen(void)
{
	if(_CONTROLLER_RomOpen != NULL)
	{
		__try
		{
			_CONTROLLER_RomOpen();
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_WM_KeyDown(WPARAM _wParam, LPARAM _lParam)
{
	if(_CONTROLLER_WM_KeyDown != NULL)
	{
		__try
		{
			_CONTROLLER_WM_KeyDown(_wParam, _lParam);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_WM_KeyUp(WPARAM _wParam, LPARAM _lParam)
{
	if(_CONTROLLER_WM_KeyUp != NULL)
	{
		__try
		{
			_CONTROLLER_WM_KeyUp(_wParam, _lParam);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_HookRDRAM(DWORD *Mem, int OCFactor)
{
	if(_CONTROLLER_HookRDRAM != NULL)
	{
		__try
		{
			_CONTROLLER_HookRDRAM(Mem, OCFactor);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_HookROM(DWORD *Rom)
{
	if(_CONTROLLER_HookROM != NULL)
	{
		__try
		{
			_CONTROLLER_HookROM(Rom);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
    Use when selecting plugin
 =======================================================================================================================
 */
void CONTROLLER_Under_Selecting_DllAbout(HWND _hWnd)
{
	if(_CONTROLLER_Under_Selecting_DllAbout != NULL)
	{
		__try
		{
			_CONTROLLER_Under_Selecting_DllAbout(_hWnd);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CONTROLLER_Under_Selecting_DllTest(HWND _hWnd)
{
	if(_CONTROLLER_Under_Selecting_DllTest != NULL)
	{
		__try
		{
			_CONTROLLER_Under_Selecting_DllTest(_hWnd);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}
