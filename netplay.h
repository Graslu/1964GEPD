#ifndef __NETPLAY_1964_H_
#define __NETPLAY_1964_H_

//#include "../1964netplay/1964netplay.h"
#include "globals.h"
#include "plugins.h"
#include "netplay-spec.h"

extern BOOL	NetplayDllLoaded;
extern BOOL NetplayInitialized;

BOOL load_netplay_dll(void);
void unload_netplay_dll(void);

void			netplay_dll_about(HWND hParent);
void			netplay_dll_config(HWND hParent, NetplayWhatToConfig what_to_config);
void			netplay_dll_test(HWND hParent);
void			netplay_get_dll_info(Netplay_DLL_Info *info);
//BOOL			netplay_initialize_netplay(void (__cdecl *getkeysfunc) (int Control, BUTTONS *Keys));
BOOL			netplay_initialize_netplay(HINSTANCE controller_plugin, CONTROL * local_players /*CONTROL [4]*/);

void			netplay_close_netplay(void);
BOOL			netplay_get_keys(int Control, BUTTONS *Keys, unsigned __int32 frame_count);
int				netplay_get_number_of_players(void);
PlayerStatus	netplay_get_player_status(int player);
void			netplay_rom_closed(void);
void			netplay_rom_open(void);


#endif
