/*$T Kaillera.c GC 1.136 02/28/02 08:17:22 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */


#include "windows.h"
#include "Kaillera.h"

BOOL			Kaillera_Is_Running = FALSE;
int				Kaillera_Players = 0;
unsigned int	Kaillera_Counter = 0;

HINSTANCE		hKailleraDll = NULL;
BOOL			bKailleraDllLoaded = FALSE;

int (__stdcall *pDllKailleraGetVersion) (char *version) = NULL;
int (__stdcall *pDllKailleraInit) (void) = NULL;
int (__stdcall *pDllKailleraShutdown) (void) = NULL;
int (__stdcall *pDllKailleraSetInfos) (kailleraInfos * infos) = NULL;
int (__stdcall *pDllKailleraSelectServerDialog) (HWND parent) = NULL;
int (__stdcall *pDllKailleraModifyPlayValues) (void *values, int size) = NULL;
int (__stdcall *pDllKailleraChatSend) (char *text) = NULL;
int (__stdcall *pDllKailleraEndGame) (void) = NULL;

/*
 =======================================================================================================================
    Dynamic Loading of Kaillera 0.84 //
 =======================================================================================================================
 */
int LoadDllKaillera(void)
{
	UnloadDllKaillera();

	hKailleraDll = LoadLibrary("KailleraClient.dll");
	if(hKailleraDll == NULL) return 0;

	pDllKailleraGetVersion = (int(__stdcall *) (char *)) GetProcAddress(hKailleraDll, "_kailleraGetVersion@4");
	if(pDllKailleraGetVersion == NULL) return 0;

	pDllKailleraInit = (int(__stdcall *) (void)) GetProcAddress(hKailleraDll, "_kailleraInit@0");
	if(pDllKailleraInit == NULL) return 0;

	pDllKailleraShutdown = (int(__stdcall *) (void)) GetProcAddress(hKailleraDll, "_kailleraShutdown@0");
	if(pDllKailleraShutdown == NULL) return 0;

	pDllKailleraSetInfos = (int(__stdcall *) (kailleraInfos *)) GetProcAddress(hKailleraDll, "_kailleraSetInfos@4");
	if(pDllKailleraSetInfos == NULL) return 0;

	pDllKailleraSelectServerDialog = (int(__stdcall *) (HWND)) GetProcAddress
		(
			hKailleraDll,
			"_kailleraSelectServerDialog@4"
		);
	if(pDllKailleraSelectServerDialog == NULL) return 0;

	pDllKailleraModifyPlayValues = (int(__stdcall *) (void *values, int size)) GetProcAddress
		(
			hKailleraDll,
			"_kailleraModifyPlayValues@8"
		);
	if(pDllKailleraModifyPlayValues == NULL) return 0;

	pDllKailleraChatSend = (int(__stdcall *) (char *)) GetProcAddress(hKailleraDll, "_kailleraChatSend@4");
	if(pDllKailleraChatSend == NULL) return 0;

	pDllKailleraEndGame = (int(__stdcall *) (void)) GetProcAddress(hKailleraDll, "_kailleraEndGame@0");
	if(pDllKailleraEndGame == NULL) return 0;

	bKailleraDllLoaded = TRUE;
	return 1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int UnloadDllKaillera(void)
{
	//kailleraShutdown();
	bKailleraDllLoaded = FALSE;

	pDllKailleraGetVersion = NULL;
	pDllKailleraInit = NULL;
	pDllKailleraShutdown = NULL;
	pDllKailleraSetInfos = NULL;
	pDllKailleraSelectServerDialog = NULL;
	pDllKailleraModifyPlayValues = NULL;
	pDllKailleraChatSend = NULL;
	pDllKailleraEndGame = NULL;

	if(hKailleraDll != NULL) FreeLibrary(hKailleraDll);
	hKailleraDll = NULL;

	return 1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int IsKailleraDllLoaded(void)
{
	return bKailleraDllLoaded;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int kailleraGetVersion(char *version)
{
	if(bKailleraDllLoaded && pDllKailleraGetVersion != NULL) return pDllKailleraGetVersion(version);
	strcpy(version, "No Kaillera Dll");
	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int kailleraInit(void)
{
	if(bKailleraDllLoaded && pDllKailleraInit != NULL) return pDllKailleraInit();
	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int kailleraShutdown(void)
{
	if(bKailleraDllLoaded && pDllKailleraShutdown != NULL) return pDllKailleraShutdown();
	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int kailleraSetInfos(kailleraInfos *infos)
{
	if(bKailleraDllLoaded && pDllKailleraSetInfos != NULL) return pDllKailleraSetInfos(infos);
	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int kailleraSelectServerDialog(HWND parent)
{
	if(bKailleraDllLoaded && pDllKailleraSelectServerDialog != NULL) return pDllKailleraSelectServerDialog(parent);
	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int kailleraModifyPlayValues(void *values, int size)
{
	if(bKailleraDllLoaded && pDllKailleraModifyPlayValues != NULL) return pDllKailleraModifyPlayValues(values, size);
	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int kailleraChatSend(char *text)
{
	if(bKailleraDllLoaded && pDllKailleraChatSend != NULL) return pDllKailleraChatSend(text);
	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int kailleraEndGame(void)
{
	if(bKailleraDllLoaded && pDllKailleraEndGame != NULL) return pDllKailleraEndGame();
	return 0;
}
