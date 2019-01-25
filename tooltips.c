#include <windows.h>
#include <commctrl.h>
#include "win32/wingui.h"

HWND WINAPI CreateTT(HWND hwndOwner)
{
    INITCOMMONCONTROLSEX icex;
    HWND        hwndTT;
    TOOLINFO    ti;
    // Load the ToolTip class from the DLL.
    icex.dwSize = sizeof(icex);
    icex.dwICC  = ICC_BAR_CLASSES;

    if(!InitCommonControlsEx(&icex))
       return NULL;
	   
    // Create the ToolTip control.
    hwndTT = CreateWindow(TOOLTIPS_CLASS, TEXT("Hello"),
                          WS_POPUP,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          NULL, (HMENU)NULL, gui.hInst,
                          NULL);

    // Prepare TOOLINFO structure for use as tracking ToolTip.
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    ti.hwnd   = hwndOwner;
    ti.uId    = (UINT)gui.hwnd1964main;
    ti.hinst  = gui.hInst;
    ti.lpszText  = LPSTR_TEXTCALLBACK;
    ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0; 

    // Add the tool to the control, displaying an error if needed.
    if(!SendMessage(hwndTT,TTM_ADDTOOL,0,(LPARAM)&ti)){
        MessageBox(hwndOwner,"Couldn't create the ToolTip control.",
                   "Error",MB_OK);
        return NULL;
    }

    // Activate (display) the tracking ToolTip. Then, set a global
    // flag value to indicate that the ToolTip is active, so other
    // functions can check to see if it's visible.
    SendMessage(hwndTT,TTM_TRACKACTIVATE,(WPARAM)TRUE,(LPARAM)&ti);
//    g_bIsVisible = TRUE;

    return(hwndTT);    
}
