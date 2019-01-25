#include "windows.h"
#include "netplay.h"

void (__cdecl *_netplay_dll_about)(HWND hParent) = NULL;
void (__cdecl *_netplay_dll_config)(HWND hParent, NetplayWhatToConfig what_to_config) = NULL;
void (__cdecl *_netplay_dll_test)(HWND hParent) = NULL;
void (__cdecl *_netplay_get_dll_info)(Netplay_DLL_Info *info) = NULL;
BOOL (__cdecl *_netplay_initialize_netplay)(HINSTANCE controller_plugin, CONTROL *) = NULL;
void (__cdecl *_netplay_close_netplay)(void);
BOOL (__cdecl *_netplay_get_keys)(int Control, BUTTONS *Keys, unsigned __int32 frame_count) = NULL;
int (__cdecl *_netplay_get_number_of_players)(void) = NULL;
PlayerStatus (__cdecl *_netplay_get_player_status)(int player) = NULL;
void (__cdecl *_netplay_rom_closed)(void) = NULL;
void (__cdecl *_netplay_rom_open)(void) = NULL;

HINSTANCE	NetplayDllHandle = NULL;
BOOL		NetplayDllLoaded = FALSE;
BOOL		NetplayInitialized = FALSE;

BOOL load_netplay_dll(void)
{
	unload_netplay_dll();

	NetplayDllHandle = LoadLibrary("plugin/1964netplay.dll");
	if( NetplayDllHandle == NULL )
		return FALSE;

	_netplay_dll_about				= (void(__cdecl *) (HWND)) GetProcAddress(NetplayDllHandle, "dll_about");
	_netplay_dll_config				= (void(__cdecl *) (HWND, NetplayWhatToConfig)) GetProcAddress(NetplayDllHandle, "dll_config");
	_netplay_dll_test				= (void(__cdecl *) (HWND)) GetProcAddress(NetplayDllHandle, "dll_test");
	
	_netplay_get_dll_info			= (void(__cdecl *) (Netplay_DLL_Info *)) GetProcAddress(NetplayDllHandle, "get_dll_info");
	_netplay_initialize_netplay		= (BOOL(__cdecl *) (HINSTANCE controller_plugin, CONTROL *)) GetProcAddress(NetplayDllHandle, "initialize_netplay");

	_netplay_close_netplay			= (void(__cdecl *) (void)) GetProcAddress(NetplayDllHandle, "close_netplay");

	_netplay_rom_closed				= (int(__cdecl *) (void)) GetProcAddress(NetplayDllHandle, "rom_closed");
	_netplay_rom_open				= (void(__cdecl *) (void)) GetProcAddress(NetplayDllHandle, "rom_open");

	_netplay_get_keys				= (BOOL(__cdecl *) (int , BUTTONS *, unsigned __int32)) GetProcAddress(NetplayDllHandle, "get_keys");
	_netplay_get_number_of_players	= (int(__cdecl *) (void)) GetProcAddress(NetplayDllHandle, "get_number_of_players");
	_netplay_get_player_status		= (PlayerStatus(__cdecl *) (int)) GetProcAddress(NetplayDllHandle, "get_player_status");

	if( _netplay_dll_config == NULL ||
		_netplay_dll_about == NULL ||
		_netplay_get_dll_info == NULL ||
		_netplay_initialize_netplay == NULL ||
		_netplay_close_netplay == NULL ||
		_netplay_rom_closed == NULL ||
		_netplay_rom_open == NULL ||
		_netplay_get_keys == NULL ||
		_netplay_get_number_of_players == NULL ||
		_netplay_get_player_status == NULL )
	{
		return FALSE;
	}

	NetplayDllLoaded = TRUE;
	return TRUE;
}

void unload_netplay_dll(void)
{
	if( NetplayDllLoaded && NetplayDllHandle != NULL )
	{
		FreeLibrary(NetplayDllHandle);
		NetplayDllHandle = NULL;
		NetplayDllLoaded = FALSE;
		NetplayInitialized = FALSE;
	}
}


void netplay_dll_about(HWND hParent)
{
	if( _netplay_dll_about )
		_netplay_dll_about(hParent);
}

void netplay_dll_config(HWND hParent, NetplayWhatToConfig what_to_config)
{
	if( _netplay_dll_config )
		_netplay_dll_config(hParent, what_to_config);
}

void netplay_dll_test(HWND hParent)
{
	if( _netplay_dll_test )
		_netplay_dll_test(hParent);
}

void netplay_get_dll_info(Netplay_DLL_Info *info)
{
	if( _netplay_get_dll_info )
		_netplay_get_dll_info(info);
}

//BOOL netplay_initialize_netplay(void (__cdecl *getkeysfunc) (int Control, BUTTONS *Keys))
BOOL netplay_initialize_netplay(HINSTANCE controller_plugin, CONTROL * local_players /*CONTROL [4]*/)
{
	if( _netplay_initialize_netplay )
	{
		NetplayInitialized = _netplay_initialize_netplay(controller_plugin, local_players);
		return NetplayInitialized;
	}
	else
		return FALSE;
}

void netplay_close_netplay(void)
{
	if( _netplay_close_netplay )
		_netplay_close_netplay();
}

BOOL netplay_get_keys(int Control, BUTTONS *Keys, unsigned __int32 frame_count)
{
	if( _netplay_get_keys )
		return _netplay_get_keys(Control, Keys, frame_count);
	else
		return FALSE;
}

int	netplay_get_number_of_players(void)
{
	if( _netplay_get_number_of_players )
		return 	_netplay_get_number_of_players();
	else
		return 0;
}

PlayerStatus netplay_get_player_status(int player)
{
	if( _netplay_get_player_status )
		return _netplay_get_player_status(player);
	else
		return REMOTE_PLAYER_STATUS_UNKNOWN;
}

void netplay_rom_closed(void)
{
	if( _netplay_rom_closed )
		_netplay_rom_closed();
}

void netplay_rom_open(void)
{
	int i;
	if( _netplay_rom_open )
	{
		for( i=0; i<4; i++ )	//Reset the 4 controllers' status
		{
			PlayerStatus retval = netplay_get_player_status(i);
			if( retval == IS_LOCAL_PLAYER || retval == REMOTE_PLAYER_NORMAL )
			{
				Controls[i].Present = TRUE;
				Controls[i].RawData = FALSE;
			}
			else
			{
				Controls[i].Present = FALSE;
			}
		}

		_netplay_rom_open();
	}
}

