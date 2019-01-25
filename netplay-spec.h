/*
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
#ifndef _NETPLAY_SPEC_H_
#define _NETPLAY_SPEC_H_

#define NETPLAY_DLLS_SPEC_VER	1.00

/*
 =======================================================================================================================
 =======================================================================================================================
 */

typedef struct
{
	unsigned __int16	max_player;
	unsigned __int16	DLL_Version;
	char				DLL_Name[50];
	char				DLL_Auther_Name[50];
	char				DLL_Release_Date[50];
} Netplay_DLL_Info;

typedef enum {
	REMOTE_PLAYER_STATUS_UNKNOWN,
	IS_LOCAL_PLAYER, 
	REMOTE_PLAYER_NOT_AVAILABLE, 
	REMOTE_PLAYER_NORMAL, 
	REMOTE_PLAYER_DISCONECTED ,
} PlayerStatus ;

typedef enum{
	NETPLAY_OPTIONS,
	NETPLAY_USER_MANAGEMENT,
	NETPLAY_NETWORK_MANAGEMENT,
	NETPLAY_NETWORK_STATUS,
	NETPLAY_GAMEPLAY_STATUS,
	NETPLAY_OTHERS,
	NETPLAY_DISABLE,
	NETPLAY_ENABLE
} NetplayWhatToConfig ;

/*
 =======================================================================================================================
    Call this function to display the About dialog box
 =======================================================================================================================
 */
//void			dll_about(HWND hParent);

/*
 =======================================================================================================================
    Call this function to access netplay configuration dialog box and other management setting / statistics
 =======================================================================================================================
 */
//void			dll_config(HWND hParent, NetplayWhatToConfig what_to_config);

/*
 =======================================================================================================================
    Testing the netplay DLL, to see if it works
 =======================================================================================================================
 */
//void			dll_test(HWND hParent);

/*
 =======================================================================================================================
    Retrieve DLL version information
 =======================================================================================================================
 */
//void			get_dll_info(Netplay_DLL_Info *info);

/*
 =======================================================================================================================
    Initialize the Netplay DLL.
	Net play is using the current selected controller plugin to read gamepad at local machine
	To call this function, the emulator main program need to pass the function which Netplay DLL 
	can call to read gamepad keys, and the function must be in this format:
		getkeyfunc(int control, BUTTONS *keys);
	Please see Zilmar's Plugin Spec for more information about controller plugin and its functions
 =======================================================================================================================
 */
//BOOL			initialize_netplay(void (__cdecl *getkeysfunc) (int Control, BUTTONS *Keys));

/*
 =======================================================================================================================
    Close the Netplay DLL
 =======================================================================================================================
 */
//void			close_netplay(void);

/*
 =======================================================================================================================
    Emulator main program calls this function to retrive gamepad key status
	
	IMPORTANT:
		Emulator main program must call this function to read key status for all player, including
		the local player. Emulator should not call the controller plugin GetKeys() function any more
		for the local player. For local player, this function will call controller plugin's GetKeys() 
		function to read the kaypad for the local computer

		The local player is always the player #1

		The emulator main program need to maintain a correct count of frames, and passes the count
		as a parameter to call this function.

		Netplay DLL will use the frame count to do synchronization for all the player
 =======================================================================================================================
 */
//BOOL			get_keys(int Control, BUTTONS *Keys, unsigned __int32 frame_count);

/*
 =======================================================================================================================
    Return the number of players, including the local player which is Player #1
 =======================================================================================================================
 */
//int			get_number_of_players(void);

/*
 =======================================================================================================================
    Use this function to get the status of each player
 =======================================================================================================================
 */
//PlayerStatus get_player_status(int player);

/*
 =======================================================================================================================
    Call this function when finishing game play
 =======================================================================================================================
 */
//void			rom_closed(void);

/*
 =======================================================================================================================
    Call this function to start game play
 =======================================================================================================================
 */
//void			rom_open(void);

#endif /* _NETPLAY_SPEC_H_ */
