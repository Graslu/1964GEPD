/*
 * lua_bizhawk.h
 *
 * A BizHawk-compatible Lua scripting layer for 1964, sufficient to run
 * Archipelago's connector_bizhawk_generic.lua and per-game connector scripts.
 */

#ifndef _LUA_BIZHAWK_H__1964_
#define _LUA_BIZHAWK_H__1964_

#include <windows.h>

/*
 * 1964 is built with __fastcall as the default convention, but the Lua core and
 * this shim are built with __cdecl so that Lua's C function pointers and the
 * luasocket DLL entry point stay compatible. These declarations are explicit so
 * that callers compiled with /Gr still get it right.
 */

/* Called once at emulator startup / shutdown. */
void	__cdecl LuaShim_Startup(void);
void	__cdecl LuaShim_Shutdown(void);

/* Loads and starts a script. Any previously running script is stopped first. */
BOOL	__cdecl LuaShim_LoadScript(const char *path);

/* Stops the running script and runs its event.onexit handlers. */
void	__cdecl LuaShim_StopScript(void);

BOOL	__cdecl LuaShim_IsRunning(void);

/*
 * Called from the CPU thread at the VI interrupt. Releases the script thread
 * for exactly one frame and blocks until it yields back, so the script sees a
 * coherent view of RDRAM.
 */
void	__cdecl LuaShim_OnFrameEnd(void);

/* Called when a ROM is unloaded so a running script stops touching dead memory. */
void	__cdecl LuaShim_OnRomClosed(void);

/*
 * Called from the input path with the button word the plugin produced, so a
 * script's joypad.set() can force buttons on or off for the coming frame.
 */
void	__cdecl LuaShim_ApplyInput(int control, DWORD *value);

/*
 * Identifies the window the video plugin renders into. gui.* draws over it and
 * client.bufferwidth/height report its size.
 */
void	__cdecl LuaShim_SetRenderTarget(HWND main, HWND statusBar);

/*
 * Publishes whatever the script queued with gui.* to the overlay window. Called
 * once the script's turn ends; the painting itself happens on the UI thread.
 */
void	__cdecl LuaShim_DrawOverlay(void);

#endif /* _LUA_BIZHAWK_H__1964_ */
