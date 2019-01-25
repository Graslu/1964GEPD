/*$T Dll_Audio.c GC 1.136 03/09/02 17:41:09 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Audio plugin interface functions
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
#include "../n64rcp.h"
#include "../emulator.h"
#include "registry.h"
#include "DLL_Audio.h"

AUDIO_INFO	Audio_Info;

HINSTANCE	hinstLibAudio = NULL;
BOOL	CoreDoingAIUpdate = TRUE;

void (__cdecl *_AUDIO_RomClosed) (void) = NULL;
void (__cdecl *_AUDIO_DllClose) () = NULL;
void (__cdecl *_AUDIO_DllConfig) (HWND) = NULL;
void (__cdecl *_AUDIO_About) (HWND) = NULL;
void (__cdecl *_AUDIO_Test) (HWND) = NULL;
void (__cdecl *_AUDIO_GetDllInfo) (PLUGIN_INFO *) = NULL;
BOOL (__cdecl *_AUDIO_Initialize) (AUDIO_INFO) = NULL;
void (__cdecl *_AUDIO_End) () = NULL;
void (__cdecl *_AUDIO_PlaySnd) (unsigned __int8 *, unsigned __int32 *) = NULL;
_int32 (__cdecl *_AUDIO_TimeLeft) (unsigned char *) = NULL;
void (__cdecl *_AUDIO_ProcessAList) (void) = NULL;
void (__cdecl *_AUDIO_AiDacrateChanged) (int) = NULL;
void (__cdecl *_AUDIO_AiLenChanged) (void) = NULL;
DWORD (__cdecl *_AUDIO_AiReadLength) (void) = NULL;
void (__cdecl *_AUDIO_AiUpdate) (BOOL) = NULL;

/* Used when selecting plugins */
void (__cdecl *_AUDIO_Under_Selecting_About) (HWND) = NULL;
void (__cdecl *_AUDIO_Under_Selecting_Test) (HWND) = NULL;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL LoadAudioPlugin(char *libname)
{
	if(hinstLibAudio != NULL)
	{
		FreeLibrary(hinstLibAudio);
	}

	/* Load the Audio DLL */
	hinstLibAudio = LoadLibrary(libname);

	if(hinstLibAudio != NULL)	/* Check if load DLL successfully */
	{
		/* Get the function address AUDIO_GetDllInfo in the audio DLL file */
		_AUDIO_GetDllInfo = (void(__cdecl *) (PLUGIN_INFO *)) GetProcAddress(hinstLibAudio, "GetDllInfo");

		if(_AUDIO_GetDllInfo != NULL)
		{
			/*~~~~~~~~~~~~~~~~~~~~*/
			PLUGIN_INFO Plugin_Info;
			/*~~~~~~~~~~~~~~~~~~~~*/

			ZeroMemory(&Plugin_Info, sizeof(Plugin_Info));

			AUDIO_GetDllInfo(&Plugin_Info);

			if(Plugin_Info.Type == PLUGIN_TYPE_AUDIO)
			{
				/* if(Plugin_Info.Version == 1) */
				{
					_AUDIO_AiDacrateChanged = (void(__cdecl *) (int)) GetProcAddress(hinstLibAudio, "AiDacrateChanged");
					_AUDIO_AiLenChanged = (void(__cdecl *) (void)) GetProcAddress(hinstLibAudio, "AiLenChanged");
					_AUDIO_AiReadLength = (DWORD(__cdecl *) (void)) GetProcAddress(hinstLibAudio, "AiReadLength");
					_AUDIO_AiUpdate = (void(__cdecl *) (BOOL)) GetProcAddress(hinstLibAudio, "AiUpdate");
					_AUDIO_DllClose = (void(__cdecl *) (void)) GetProcAddress(hinstLibAudio, "CloseDLL");

					_AUDIO_About = (void(__cdecl *) (HWND)) GetProcAddress(hinstLibAudio, "DllAbout");
					_AUDIO_DllConfig = (void(__cdecl *) (HWND)) GetProcAddress(hinstLibAudio, "DllConfig");
					_AUDIO_Test = (void(__cdecl *) (HWND)) GetProcAddress(hinstLibAudio, "DllTest");
					_AUDIO_Initialize = (BOOL(__cdecl *) (AUDIO_INFO)) GetProcAddress(hinstLibAudio, "InitiateAudio");
					_AUDIO_ProcessAList = (void(__cdecl *) (void)) GetProcAddress(hinstLibAudio, "ProcessAList");
					_AUDIO_RomClosed = (void(__cdecl *) (void)) GetProcAddress(hinstLibAudio, "RomClosed");

					if( strstr(Plugin_Info.Name, "Jabo") != NULL || strstr(Plugin_Info.Name, "jabo") != NULL )
					{
						CoreDoingAIUpdate = FALSE;
						if( emuoptions.UsingRspPlugin == FALSE )
						{
							DisplayError("Warning, Jabo DirectSound Plugin is selected and loaded, but RSP Plugin is not "\
										"selected, Jabo DirectSound Plugin does not produce sound without RSP plugin. You can activate "\
										"RSP Plugin in plugin setting, or you can change to other audio plugins");
						}
					}
					else
					{
						CoreDoingAIUpdate = TRUE;
					}
					return(TRUE);
				}
			}
		}
	}

	return FALSE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AUDIO_GetDllInfo(PLUGIN_INFO *Plugin_Info)
{
	if(_AUDIO_GetDllInfo != NULL) __try
	{
		_AUDIO_GetDllInfo(Plugin_Info);
	}

	__except(NULL, EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL AUDIO_Initialize(AUDIO_INFO Audio_Info)
{
	if(_AUDIO_Initialize != NULL) __try
	{
		_AUDIO_Initialize(Audio_Info);
	}

	__except(NULL, EXCEPTION_EXECUTE_HANDLER)
	{
		/* Some people won't have a soud card. No error. */
	}

	return(1);	/* for now.. */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AUDIO_ProcessAList(void)
{
	/* try/except is handled from the call to this function */
	if(_AUDIO_ProcessAList != NULL)
	{
		__try
		{
			_AUDIO_ProcessAList();
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			/* Some people won't have a soud card. No error. */
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AUDIO_RomClosed(void)
{
	if(_AUDIO_RomClosed != NULL)
	{
		__try
		{
			_AUDIO_RomClosed();
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
void AUDIO_DllClose(void)
{
	if(_AUDIO_DllClose != NULL)
	{
		__try
		{
			_AUDIO_DllClose();
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
void CloseAudioPlugin(void)
{
	/* AUDIO_End(); */
	AUDIO_RomClosed();
	AUDIO_DllClose();

	if(hinstLibAudio)
	{
		FreeLibrary(hinstLibAudio);
		hinstLibAudio = NULL;
	}

	_AUDIO_DllClose = NULL;
	_AUDIO_DllConfig = NULL;
	_AUDIO_About = NULL;
	_AUDIO_Test = NULL;
	_AUDIO_GetDllInfo = NULL;
	_AUDIO_Initialize = NULL;
	_AUDIO_End = NULL;
	_AUDIO_PlaySnd = NULL;
	_AUDIO_TimeLeft = NULL;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AUDIO_DllConfig(HWND hParent)
{
	if(_AUDIO_DllConfig != NULL)
	{
		_AUDIO_DllConfig(hParent);
	}
	else
	{
		DisplayError("%s cannot be configured.", "Audio Plugin");
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AUDIO_About(HWND hParent)
{
	if(_AUDIO_About != NULL)
	{
		_AUDIO_About(hParent);
	}
	else
	{
		/*
		 * DisplayError("%s: About information is not available for this plug-in.", "Audio
		 * Plugin");
		 */
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AUDIO_Test(HWND hParent)
{
	if(_AUDIO_Test != NULL)
	{
		_AUDIO_Test(hParent);
	}
	else
	{
		/* DisplayError("%s: Test box is not available for this plug-in.", "Audio Plugin"); */
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AUDIO_AiDacrateChanged(int SystemType)
{
	if(_AUDIO_AiDacrateChanged != NULL)
	{
		_AUDIO_AiDacrateChanged(SystemType);
	}
	else
	{
		/* DisplayError("%s: Test box is not available for this plug-in.", "Audio Plugin"); */
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AUDIO_AiLenChanged(void)
{
	if(_AUDIO_AiLenChanged != NULL)
	{
		_AUDIO_AiLenChanged();
	}
	else
	{
		/* DisplayError("%s: Test box is not available for this plug-in.", "Audio Plugin"); */
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
DWORD AUDIO_AiReadLength(void)
{
	if(_AUDIO_AiReadLength != NULL)
	{
		return _AUDIO_AiReadLength();
	}
	else
	{
		/*
		 * DisplayError("%s: Test box is not available for this plug-in.", "Audio
		 * Plugin"); £
		 * return AI_LEN_REG;
		 */
		return 0;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AUDIO_AiUpdate(BOOL update)
{
	if(_AUDIO_AiUpdate != NULL)
	{
		_AUDIO_AiUpdate(update);
	}
	else
	{
		/* DisplayError("%s: Test box is not available for this plug-in.", "Audio Plugin"); */
	}
}

/*
 =======================================================================================================================
    Used when selecting plugins
 =======================================================================================================================
 */
void AUDIO_Under_Selecting_About(HWND hParent)
{
	if(_AUDIO_Under_Selecting_About != NULL)
	{
		_AUDIO_Under_Selecting_About(hParent);
	}
	else
	{
		/*
		 * DisplayError("%s: About information is not available for this plug-in.", "Audio
		 * Plugin");
		 */
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AUDIO_Under_Selecting_Test(HWND hParent)
{
	if(_AUDIO_Under_Selecting_Test != NULL)
	{
		_AUDIO_Under_Selecting_Test(hParent);
	}
	else
	{
		/* DisplayError("%s: Test box is not available for this plug-in.", "Audio Plugin"); */
	}
}
