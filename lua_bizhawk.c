/*
 * lua_bizhawk.c
 *
 * A BizHawk-compatible Lua scripting layer for 1964.
 *
 * The script runs on its own thread. At every VI interrupt the CPU thread
 * releases the script for one frame and blocks until it yields back via
 * emu.frameadvance(), which means script code never observes RDRAM while the
 * recompiler is mid-frame. This mirrors how BizHawk schedules Lua and is what
 * Archipelago's connector scripts expect.
 *
 * Memory note: 1964 stores both RDRAM and the ROM image with each 32-bit word
 * byte-reversed (see MEM_READ_UBYTE in memory.h and ByteSwap() in fileio.c).
 * BizHawk presents N64 domains in big-endian order, so every byte index is
 * translated with ^ 3.
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>

#include "r4300i.h"
#include "n64rcp.h"
#include "globals.h"
#include "emulator.h"
#include "1964ini.h"
#include "plugins.h"
#include "lua_bizhawk.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

/*
 * Declared here rather than via compiler.h: the rest of the emulator is built
 * with __fastcall, but this file is __cdecl, so the convention has to be stated
 * explicitly for the symbol to resolve.
 */
extern void __fastcall Check_And_Invalidate_Compiled_Blocks_By_DMA
			(
				uint32	startaddr,
				uint32	len,
				char	*op
			);

#define LUA_FRAME_TIMEOUT_MS	5000
#define SYSTEM_ID				"N64"

static lua_State	*g_L = NULL;
static HANDLE		g_hThread = NULL;
static HANDLE		g_hRunFrame = NULL;		/* emulator -> script: run one frame */
static HANDLE		g_hFrameDone = NULL;	/* script -> emulator: yielded back */
static volatile LONG g_running = 0;
static volatile LONG g_stopRequested = 0;
static volatile LONG g_scriptAlive = 0;
static unsigned int	g_frameCount = 0;
static char			g_scriptPath[MAX_PATH];
static int			g_frameEndTableRef = LUA_NOREF;

/*
 * joypad.set names only the buttons it cares about and leaves the rest under
 * the player's control, so forced presses and forced releases are tracked
 * separately rather than as one button word.
 */
typedef struct
{
	DWORD	press;
	DWORD	release;
} LuaInputOverride;

static LuaInputOverride	g_input[4];
static DWORD			g_lastKeys[4];

/*
 * Queued gui.* drawing.
 *
 * The overlay lives in a layered child window rather than being painted onto
 * the emulator window directly. A direct paint races the video plugin: its
 * present returns before the flip has actually happened, so roughly every
 * other frame the text is wiped as soon as it is drawn, which reads as flicker.
 * A child window is composited over the parent instead and survives presents.
 *
 * g_draw is owned by the script thread. It is copied to g_published once the
 * script's turn ends, and the UI thread paints from that copy under the lock.
 */
#define LUA_MAX_DRAW		64
#define LUA_FONT_CACHE		8
#define LUA_OVERLAY_CLASS	"1964LuaOverlay"
#define LUA_OVERLAY_KEY		RGB(255, 0, 255)

typedef struct
{
	int			x, y;
	int			size;
	COLORREF	color;
	char		text[128];
} LuaDrawText;

static LuaDrawText		g_draw[LUA_MAX_DRAW];
static int				g_drawCount;
static LuaDrawText		g_published[LUA_MAX_DRAW];
static int				g_publishedCount;
static CRITICAL_SECTION	g_overlayLock;
static BOOL				g_overlayLockReady;
static HWND				g_hOverlay;
static HWND				g_hRenderWnd;
static HWND				g_hStatusBar;

static struct
{
	int		size;
	HFONT	font;
} g_fonts[LUA_FONT_CACHE];
static int			g_exitTableRef = LUA_NOREF;
static BOOL			g_consoleOpen = FALSE;

/*
 =======================================================================================================================
    Console
 =======================================================================================================================
 */
static void LuaConsole_Open(void)
{
	if(g_consoleOpen) return;

	if(AllocConsole())
	{
		FILE *dummy;
		freopen_s(&dummy, "CONOUT$", "w", stdout);
		freopen_s(&dummy, "CONOUT$", "w", stderr);
		SetConsoleTitle("1964 - Lua Console");
	}

	g_consoleOpen = TRUE;
}

static void LuaConsole_Print(const char *text)
{
	LuaConsole_Open();
	fputs(text, stdout);
	fputc('\n', stdout);
	fflush(stdout);
}

/*
 =======================================================================================================================
    Memory domains

    All sizes are queried live because the expansion pak can be toggled and the
    ROM is not allocated until one is loaded.
 =======================================================================================================================
 */
typedef struct
{
	uint8	*base;
	uint32	size;
} LuaDomain;

static BOOL Domain_Lookup(const char *name, LuaDomain *out)
{
	out->base = NULL;
	out->size = 0;

	if(name == NULL) return FALSE;

	if(_stricmp(name, "RDRAM") == 0 || _stricmp(name, "System Bus") == 0)
	{
		out->base = gMemoryState.RDRAM;
		out->size = (gMemoryState.ExRDRAM != NULL) ? 0x800000 : 0x400000;
		return (out->base != NULL);
	}

	if(_stricmp(name, "ROM") == 0)
	{
		out->base = gMemoryState.ROM_Image;
		out->size = gAllocationLength;
		return (out->base != NULL);
	}

	return FALSE;
}

/* Translates a BizHawk big-endian byte index into 1964's word-swapped storage. */
#define SWIZZLE(offset) ((offset) ^ 3)

/*
 * The domain argument is optional throughout and defaults to RDRAM, which also
 * makes these usable as mainmemory.*, where RDRAM is implied.
 */
static BOOL Domain_Arg(lua_State *L, int index, LuaDomain *dom)
{
	return Domain_Lookup(luaL_optstring(L, index, "RDRAM"), dom);
}

/*
 * A write into RDRAM can land on code the recompiler has already translated,
 * so drop any affected blocks the same way a DMA would.
 */
static void Domain_Invalidate(const LuaDomain *dom, uint32 address, uint32 count)
{
	if(count > 0 && dom->base == gMemoryState.RDRAM
	&& currentromoptions.Code_Check == CODE_CHECK_PROTECT_MEMORY)
	{
		Check_And_Invalidate_Compiled_Blocks_By_DMA(address | 0x80000000, count, "Lua");
	}
}

static uint32 Domain_Read(const LuaDomain *dom, uint32 address, int width, BOOL bigEndian)
{
	uint32	value = 0;
	int		i;

	for(i = 0; i < width; i++)
	{
		uint32	offset = address + (uint32) (bigEndian ? i : (width - 1 - i));
		uint8	byte = (offset < dom->size) ? dom->base[SWIZZLE(offset)] : 0;

		value = (value << 8) | byte;
	}

	return value;
}

static void Domain_Write(const LuaDomain *dom, uint32 address, int width, BOOL bigEndian, uint32 value)
{
	int i;

	for(i = width - 1; i >= 0; i--)
	{
		uint32	offset = address + (uint32) (bigEndian ? i : (width - 1 - i));

		if(offset < dom->size) dom->base[SWIZZLE(offset)] = (uint8) (value & 0xFF);
		value >>= 8;
	}

	Domain_Invalidate(dom, address, (uint32) width);
}

/*
 * Values cross the boundary as Lua numbers rather than integers: lua_Integer is
 * only 32 bits signed here, which cannot represent a full unsigned word.
 */
static int MemRead(lua_State *L, int width, BOOL bigEndian, BOOL isSigned)
{
	LuaDomain	dom;
	uint32		address = (uint32) luaL_checkinteger(L, 1);
	uint32		value;

	if(!Domain_Arg(L, 2, &dom))
	{
		lua_pushnumber(L, 0);
		return 1;
	}

	value = Domain_Read(&dom, address, width, bigEndian);

	if(isSigned)
	{
		int shift = 32 - width * 8;

		lua_pushnumber(L, (lua_Number) (((int) (value << shift)) >> shift));
	}
	else
	{
		lua_pushnumber(L, (lua_Number) value);
	}

	return 1;
}

static int MemWrite(lua_State *L, int width, BOOL bigEndian)
{
	LuaDomain	dom;
	uint32		address = (uint32) luaL_checkinteger(L, 1);
	uint32		value = (uint32) (__int64) luaL_checknumber(L, 2);

	if(Domain_Arg(L, 3, &dom))
	{
		Domain_Write(&dom, address, width, bigEndian, value);
	}

	return 0;
}

/*
 =======================================================================================================================
    emu.*
 =======================================================================================================================
 */
static int l_emu_getsystemid(lua_State *L)
{
	lua_pushstring(L, Rom_Loaded ? SYSTEM_ID : "NULL");
	return 1;
}

static int l_emu_framecount(lua_State *L)
{
	lua_pushinteger(L, (lua_Integer) g_frameCount);
	return 1;
}

static void RunCallbackTable(lua_State *L, int tableRef);

static int l_emu_frameadvance(lua_State *L)
{
	/* Hand the frame back to the emulator, then block until the next VI. */
	SetEvent(g_hFrameDone);
	WaitForSingleObject(g_hRunFrame, INFINITE);

	if(g_stopRequested)
	{
		return luaL_error(L, "script stopped");
	}

	RunCallbackTable(L, g_frameEndTableRef);
	return 0;
}

static int l_emu_getdisplaytype(lua_State *L)
{
	lua_pushstring(L, (rominfo.TV_System == TV_SYSTEM_PAL) ? "PAL" : "NTSC");
	return 1;
}

static int l_emu_yield(lua_State *L)
{
	(void) L;
	return 0;
}

/*
 =======================================================================================================================
    gameinfo.*
 =======================================================================================================================
 */
static int l_gameinfo_getromhash(lua_State *L)
{
	char hash[32];

	/*
	 * Only self-consistency matters here: the Archipelago client uses this
	 * purely to notice that the loaded ROM changed.
	 */
	sprintf(hash, "%08X%08X", rominfo.crc1, rominfo.crc2);
	lua_pushstring(L, hash);
	return 1;
}

static int l_gameinfo_getromname(lua_State *L)
{
	char name[24];

	memcpy(name, rominfo.name, 20);
	name[20] = '\0';
	lua_pushstring(L, name);
	return 1;
}

/*
 =======================================================================================================================
    memory.*
 =======================================================================================================================
 */
static int l_memory_getmemorydomainsize(lua_State *L)
{
	LuaDomain	dom;
	const char	*name = luaL_checkstring(L, 1);

	if(!Domain_Lookup(name, &dom))
	{
		return luaL_error(L, "unknown memory domain: %s", name);
	}

	lua_pushinteger(L, (lua_Integer) dom.size);
	return 1;
}

static int l_memory_getmemorydomainlist(lua_State *L)
{
	lua_newtable(L);
	lua_pushstring(L, "RDRAM");
	lua_rawseti(L, -2, 1);
	lua_pushstring(L, "ROM");
	lua_rawseti(L, -2, 2);
	lua_pushstring(L, "System Bus");
	lua_rawseti(L, -2, 3);
	return 1;
}

static int l_memory_read_bytes_as_array(lua_State *L)
{
	LuaDomain	dom;
	uint32		address = (uint32) luaL_checkinteger(L, 1);
	uint32		size = (uint32) luaL_checkinteger(L, 2);
	const char	*name = luaL_optstring(L, 3, "RDRAM");
	uint32		i;

	if(!Domain_Lookup(name, &dom))
	{
		return luaL_error(L, "unknown memory domain: %s", name);
	}

	lua_createtable(L, (int) size, 0);

	for(i = 0; i < size; i++)
	{
		uint32	offset = address + i;
		uint8	value = 0;

		if(offset < dom.size)
		{
			value = dom.base[SWIZZLE(offset)];
		}

		lua_pushinteger(L, (lua_Integer) value);
		lua_rawseti(L, -2, (int) (i + 1));
	}

	return 1;
}

static int l_memory_write_bytes_as_array(lua_State *L)
{
	LuaDomain	dom;
	uint32		address = (uint32) luaL_checkinteger(L, 1);
	const char	*name;
	uint32		count;
	uint32		i;

	luaL_checktype(L, 2, LUA_TTABLE);
	name = luaL_optstring(L, 3, "RDRAM");

	if(!Domain_Lookup(name, &dom))
	{
		return luaL_error(L, "unknown memory domain: %s", name);
	}

	count = (uint32) lua_objlen(L, 2);

	for(i = 0; i < count; i++)
	{
		uint32	offset = address + i;
		uint8	value;

		lua_rawgeti(L, 2, (int) (i + 1));
		value = (uint8) lua_tointeger(L, -1);
		lua_pop(L, 1);

		if(offset < dom.size)
		{
			dom.base[SWIZZLE(offset)] = value;
		}
	}

	/*
	 * A write into RDRAM can land on code the recompiler has already
	 * translated, so drop any affected blocks the same way a DMA would.
	 */
	if(count > 0 && dom.base == gMemoryState.RDRAM
	&& currentromoptions.Code_Check == CODE_CHECK_PROTECT_MEMORY)
	{
		Check_And_Invalidate_Compiled_Blocks_By_DMA(address | 0x80000000, count, "Lua");
	}

	return 0;
}

static int l_memory_read_u8(lua_State *L)		{ return MemRead(L, 1, TRUE, FALSE); }
static int l_memory_read_s8(lua_State *L)		{ return MemRead(L, 1, TRUE, TRUE); }
static int l_memory_read_u16_be(lua_State *L)	{ return MemRead(L, 2, TRUE, FALSE); }
static int l_memory_read_s16_be(lua_State *L)	{ return MemRead(L, 2, TRUE, TRUE); }
static int l_memory_read_u16_le(lua_State *L)	{ return MemRead(L, 2, FALSE, FALSE); }
static int l_memory_read_u32_be(lua_State *L)	{ return MemRead(L, 4, TRUE, FALSE); }
static int l_memory_read_s32_be(lua_State *L)	{ return MemRead(L, 4, TRUE, TRUE); }
static int l_memory_read_u32_le(lua_State *L)	{ return MemRead(L, 4, FALSE, FALSE); }

static int l_memory_write_u8(lua_State *L)		{ return MemWrite(L, 1, TRUE); }
static int l_memory_write_u16_be(lua_State *L)	{ return MemWrite(L, 2, TRUE); }
static int l_memory_write_u16_le(lua_State *L)	{ return MemWrite(L, 2, FALSE); }
static int l_memory_write_u32_be(lua_State *L)	{ return MemWrite(L, 4, TRUE); }
static int l_memory_write_u32_le(lua_State *L)	{ return MemWrite(L, 4, FALSE); }

/*
 =======================================================================================================================
    gui.*
 =======================================================================================================================
 */
static int l_gui_addmessage(lua_State *L)
{
	const char *message = luaL_optstring(L, 1, "");

	LuaConsole_Print(message);
	return 0;
}

static int l_gui_noop(lua_State *L)
{
	(void) L;
	return 0;
}

/* The area the video plugin draws into, in client coordinates. */
static BOOL RenderArea(RECT *out)
{
	RECT rc;
	RECT sb;

	if(g_hRenderWnd == NULL || !GetClientRect(g_hRenderWnd, &rc)) return FALSE;

	if(g_hStatusBar != NULL && IsWindowVisible(g_hStatusBar) && GetWindowRect(g_hStatusBar, &sb))
	{
		rc.bottom -= (sb.bottom - sb.top);
	}

	if(rc.bottom < rc.top) rc.bottom = rc.top;

	*out = rc;
	return TRUE;
}

/* BizHawk passes colours as 0xAARRGGBB; GDI wants 0x00BBGGRR. */
static COLORREF ArgbToColorRef(unsigned int argb)
{
	return RGB((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF);
}

static int l_gui_cleargraphics(lua_State *L)
{
	(void) L;
	g_drawCount = 0;
	return 0;
}

/*
 * gui.drawText(x, y, message, forecolor, backcolor, fontsize). The backcolour
 * is a filled box behind the text in BizHawk; here the text is outlined in
 * black instead, which keeps it readable over the game without hiding it.
 */
static int l_gui_drawtext(lua_State *L)
{
	LuaDrawText *d;

	if(g_drawCount >= LUA_MAX_DRAW) return 0;

	d = &g_draw[g_drawCount];

	d->x = (int) luaL_checknumber(L, 1);
	d->y = (int) luaL_checknumber(L, 2);

	strncpy(d->text, luaL_optstring(L, 3, ""), sizeof(d->text) - 1);
	d->text[sizeof(d->text) - 1] = '\0';

	d->color = lua_isnoneornil(L, 4)
		? RGB(255, 255, 255)
		: ArgbToColorRef((unsigned int) (__int64) lua_tonumber(L, 4));

	d->size = (int) luaL_optnumber(L, 6, 14);

	if(d->size < 6) d->size = 6;
	if(d->size > 200) d->size = 200;

	g_drawCount++;
	return 0;
}

/* gui.text uses screen coordinates and a fixed small font in BizHawk. */
static int l_gui_text(lua_State *L)
{
	LuaDrawText *d;

	if(g_drawCount >= LUA_MAX_DRAW) return 0;

	d = &g_draw[g_drawCount];

	d->x = (int) luaL_checknumber(L, 1);
	d->y = (int) luaL_checknumber(L, 2);

	strncpy(d->text, luaL_optstring(L, 3, ""), sizeof(d->text) - 1);
	d->text[sizeof(d->text) - 1] = '\0';

	d->color = lua_isnoneornil(L, 4)
		? RGB(255, 255, 255)
		: ArgbToColorRef((unsigned int) (__int64) lua_tonumber(L, 4));

	d->size = 14;

	g_drawCount++;
	return 0;
}

static HFONT FontForSize(int size)
{
	int i;

	for(i = 0; i < LUA_FONT_CACHE; i++)
	{
		if(g_fonts[i].font != NULL && g_fonts[i].size == size) return g_fonts[i].font;
	}

	for(i = 0; i < LUA_FONT_CACHE; i++)
	{
		if(g_fonts[i].font == NULL)
		{
			g_fonts[i].size = size;

			/*
			 * Transparency is by colour key, so antialiased glyph edges would
			 * blend towards the key colour and leave a magenta fringe.
			 */
			g_fonts[i].font = CreateFont
				(
					-size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
					DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
					NONANTIALIASED_QUALITY, FF_DONTCARE, "Arial"
				);

			return g_fonts[i].font;
		}
	}

	/* Scripts realistically use one or two sizes; reuse rather than leak. */
	return g_fonts[0].font;
}

/*
 =======================================================================================================================
    event.*
 =======================================================================================================================
 */
static int RegisterCallback(lua_State *L, int tableRef)
{
	int count;

	luaL_checktype(L, 1, LUA_TFUNCTION);

	lua_rawgeti(L, LUA_REGISTRYINDEX, tableRef);
	count = (int) lua_objlen(L, -1);
	lua_pushvalue(L, 1);
	lua_rawseti(L, -2, count + 1);
	lua_pop(L, 1);

	lua_pushstring(L, luaL_optstring(L, 2, "callback"));
	return 1;
}

static int l_event_onframeend(lua_State *L)
{
	return RegisterCallback(L, g_frameEndTableRef);
}

static int l_event_onexit(lua_State *L)
{
	return RegisterCallback(L, g_exitTableRef);
}

static int l_event_unregister(lua_State *L)
{
	(void) L;
	return 0;
}

static void RunCallbackTable(lua_State *L, int tableRef)
{
	int count;
	int i;

	lua_rawgeti(L, LUA_REGISTRYINDEX, tableRef);
	count = (int) lua_objlen(L, -1);

	for(i = 1; i <= count; i++)
	{
		lua_rawgeti(L, -1, i);

		if(lua_pcall(L, 0, 0, 0) != 0)
		{
			LuaConsole_Print(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}

	lua_pop(L, 1);
}

/*
 =======================================================================================================================
    print
 =======================================================================================================================
 */
static int l_print(lua_State *L)
{
	int		n = lua_gettop(L);
	int		i;
	char	line[1024];

	line[0] = '\0';

	for(i = 1; i <= n; i++)
	{
		const char *piece;

		lua_getglobal(L, "tostring");
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
		piece = lua_tostring(L, -1);

		if(piece != NULL)
		{
			if(i > 1) strncat(line, "\t", sizeof(line) - strlen(line) - 1);
			strncat(line, piece, sizeof(line) - strlen(line) - 1);
		}

		lua_pop(L, 1);
	}

	LuaConsole_Print(line);
	return 0;
}

/*
 =======================================================================================================================
    Registration
 =======================================================================================================================
 */
static void RegisterTable(lua_State *L, const char *name, const luaL_Reg *functions)
{
	lua_newtable(L);

	while(functions->name != NULL)
	{
		lua_pushcfunction(L, functions->func);
		lua_setfield(L, -2, functions->name);
		functions++;
	}

	lua_setglobal(L, name);
}

static const luaL_Reg emu_functions[] =
{
	{ "getsystemid",	l_emu_getsystemid },
	{ "framecount",		l_emu_framecount },
	{ "frameadvance",	l_emu_frameadvance },
	{ "getdisplaytype",	l_emu_getdisplaytype },
	{ "yield",			l_emu_yield },
	{ NULL,				NULL }
};

static const luaL_Reg gameinfo_functions[] =
{
	{ "getromhash",		l_gameinfo_getromhash },
	{ "getromname",		l_gameinfo_getromname },
	{ NULL,				NULL }
};

static const luaL_Reg memory_functions[] =
{
	{ "read_bytes_as_array",	l_memory_read_bytes_as_array },
	{ "write_bytes_as_array",	l_memory_write_bytes_as_array },
	{ "getmemorydomainsize",	l_memory_getmemorydomainsize },
	{ "getmemorydomainlist",	l_memory_getmemorydomainlist },
	{ "readbyte",				l_memory_read_u8 },
	{ "writebyte",				l_memory_write_u8 },
	{ "read_u8",				l_memory_read_u8 },
	{ "write_u8",				l_memory_write_u8 },
	{ "read_s8",				l_memory_read_s8 },
	{ "write_s8",				l_memory_write_u8 },
	{ "read_u16_be",			l_memory_read_u16_be },
	{ "write_u16_be",			l_memory_write_u16_be },
	{ "read_s16_be",			l_memory_read_s16_be },
	{ "write_s16_be",			l_memory_write_u16_be },
	{ "read_u16_le",			l_memory_read_u16_le },
	{ "write_u16_le",			l_memory_write_u16_le },
	{ "read_u32_be",			l_memory_read_u32_be },
	{ "write_u32_be",			l_memory_write_u32_be },
	{ "read_s32_be",			l_memory_read_s32_be },
	{ "write_s32_be",			l_memory_write_u32_be },
	{ "read_u32_le",			l_memory_read_u32_le },
	{ "write_u32_le",			l_memory_write_u32_le },
	{ NULL,						NULL }
};

static const luaL_Reg gui_functions[] =
{
	{ "addmessage",		l_gui_addmessage },
	{ "text",			l_gui_text },
	{ "drawText",		l_gui_drawtext },
	{ "clearGraphics",	l_gui_cleargraphics },
	{ NULL,				NULL }
};

static const luaL_Reg console_functions[] =
{
	{ "log",			l_print },
	{ "write",			l_print },
	{ "clear",			l_gui_noop },
	{ NULL,				NULL }
};

/*
 * Scripts scale and position overlay text against these, so they have to match
 * the area gui.* actually paints into rather than the N64's internal
 * resolution. Falls back to the default framebuffer size before a window
 * exists.
 */
static int l_client_bufferwidth(lua_State *L)
{
	RECT rc;

	lua_pushinteger(L, RenderArea(&rc) ? (rc.right - rc.left) : 320);
	return 1;
}

static int l_client_bufferheight(lua_State *L)
{
	RECT rc;

	lua_pushinteger(L, RenderArea(&rc) ? (rc.bottom - rc.top) : 240);
	return 1;
}

static const luaL_Reg client_functions[] =
{
	{ "bufferwidth",	l_client_bufferwidth },
	{ "bufferheight",	l_client_bufferheight },
	{ NULL,				NULL }
};

/*
 * Bit positions are fixed by the Zilmar plugin ABI and match BUTTONS in
 * plugins.h. Names are BizHawk's, minus the "P<n> " prefix, which is parsed
 * separately. Alias rows exist only for matching and are skipped when
 * reporting state back.
 */
static const struct
{
	const char	*name;
	DWORD		mask;
	BOOL		alias;
} g_buttons[] =
{
	{ "A",			1u << 7,	FALSE },
	{ "B",			1u << 6,	FALSE },
	{ "Z",			1u << 5,	FALSE },
	{ "L",			1u << 13,	FALSE },
	{ "R",			1u << 12,	FALSE },
	{ "Start",		1u << 4,	FALSE },
	{ "DPad U",		1u << 3,	FALSE },
	{ "DPad D",		1u << 2,	FALSE },
	{ "DPad L",		1u << 1,	FALSE },
	{ "DPad R",		1u << 0,	FALSE },
	{ "C Up",		1u << 11,	FALSE },
	{ "C Down",		1u << 10,	FALSE },
	{ "C Left",		1u << 9,	FALSE },
	{ "C Right",	1u << 8,	FALSE },
	{ "DPad Up",	1u << 3,	TRUE },
	{ "DPad Down",	1u << 2,	TRUE },
	{ "DPad Left",	1u << 1,	TRUE },
	{ "DPad Right",	1u << 0,	TRUE },
	{ "C U",		1u << 11,	TRUE },
	{ "C D",		1u << 10,	TRUE },
	{ "C L",		1u << 9,	TRUE },
	{ "C R",		1u << 8,	TRUE },
};

/* Consumes a leading "P<n> " and returns the controller it names, or 1. */
static int ParsePlayerPrefix(const char **name)
{
	const char *s = *name;

	if((s[0] == 'P' || s[0] == 'p') && s[1] >= '1' && s[1] <= '4' && s[2] == ' ')
	{
		*name = s + 3;
		return s[1] - '0';
	}

	return 1;
}

static DWORD ButtonMask(const char *name)
{
	int i;

	for(i = 0; i < sizeof(g_buttons) / sizeof(g_buttons[0]); i++)
	{
		if(_stricmp(name, g_buttons[i].name) == 0) return g_buttons[i].mask;
	}

	return 0;
}

static int l_joypad_set(lua_State *L)
{
	int controller;

	luaL_checktype(L, 1, LUA_TTABLE);
	controller = (int) luaL_optinteger(L, 2, 0);

	lua_pushnil(L);

	while(lua_next(L, 1) != 0)
	{
		/*
		 * Only string keys are inspected: running lua_tostring over a numeric
		 * key would rewrite it in place and derail lua_next.
		 */
		if(lua_type(L, -2) == LUA_TSTRING)
		{
			const char	*name = lua_tostring(L, -2);
			int			player = ParsePlayerPrefix(&name);
			DWORD		mask = ButtonMask(name);

			if(controller >= 1 && controller <= 4) player = controller;

			if(mask != 0)
			{
				LuaInputOverride *o = &g_input[player - 1];

				/* A button named false is held off, not merely left alone. */
				if(lua_toboolean(L, -1))
				{
					o->press |= mask;
					o->release &= ~mask;
				}
				else
				{
					o->release |= mask;
					o->press &= ~mask;
				}
			}
		}

		lua_pop(L, 1);
	}

	return 0;
}

/* Reports the word last handed to the game, so overrides are included. */
static int l_joypad_get(lua_State *L)
{
	int		controller = (int) luaL_optinteger(L, 1, 1);
	DWORD	keys;
	int		i;

	if(controller < 1 || controller > 4) controller = 1;

	keys = g_lastKeys[controller - 1];
	lua_newtable(L);

	for(i = 0; i < sizeof(g_buttons) / sizeof(g_buttons[0]); i++)
	{
		if(g_buttons[i].alias) continue;

		lua_pushboolean(L, (keys & g_buttons[i].mask) != 0);
		lua_setfield(L, -2, g_buttons[i].name);
	}

	return 1;
}

static const luaL_Reg joypad_functions[] =
{
	{ "set",			l_joypad_set },
	{ "get",			l_joypad_get },
	{ "getimmediate",	l_joypad_get },
	{ NULL,				NULL }
};

static int l_savestate_unsupported(lua_State *L)
{
	LuaConsole_Print("savestate is not available from Lua in 1964; request ignored.");
	lua_pushboolean(L, 0);
	return 1;
}

static const luaL_Reg savestate_functions[] =
{
	{ "load",			l_savestate_unsupported },
	{ "save",			l_savestate_unsupported },
	{ "loadslot",		l_savestate_unsupported },
	{ "saveslot",		l_savestate_unsupported },
	{ NULL,				NULL }
};

static const luaL_Reg event_functions[] =
{
	{ "onframeend",		l_event_onframeend },
	{ "onframestart",	l_event_onframeend },
	{ "onexit",			l_event_onexit },
	{ "unregisterbyid",	l_event_unregister },
	{ "unregisterbyname", l_event_unregister },
	{ NULL,				NULL }
};

/*
 * Anything easier to express in Lua than in C. Notably this repoints the
 * Archipelago script directory onto the 32-bit luasocket build, since the
 * per-game connectors hardcode the x64 one and 1964 is a 32-bit process.
 */
static const char *g_prelude =
/*
 * ARCHIPELAGO_LUA_DIR overrides the install location. Under Proton the default
 * resolves inside the Wine prefix, where a Linux Archipelago install is not
 * visible, so the Steam Deck bundle points this at the real directory.
 */
"local ap = os.getenv('ARCHIPELAGO_LUA_DIR')\n"
"if ap == nil or ap == '' then\n"
"  local pd = os.getenv('PROGRAMDATA') or 'C:\\\\ProgramData'\n"
"  ap = pd .. '\\\\Archipelago\\\\data\\\\lua'\n"
"end\n"
"ARCHIPELAGO_LUA_DIR = ap\n"
"package.path = ap .. '\\\\?.lua;' .. package.path\n"
"package.cpath = ap .. '\\\\x86\\\\?.dll;' .. package.cpath\n"
/* Files shipped beside the executable are the last resort, not the preference. */
"local bundled = EMU_DIR and (EMU_DIR .. '\\\\ap_lua') or nil\n"
"if bundled then\n"
"  package.path = package.path .. ';' .. bundled .. '\\\\?.lua'\n"
"  package.cpath = package.cpath .. ';' .. bundled .. '\\\\x86\\\\?.dll'\n"
"end\n"
/*
 * Connectors exchange flag files through %TEMP% (DeathLink is one). Under
 * Proton that is a directory inside the Wine prefix, so a Linux Archipelago
 * client writing to /tmp would never be seen. Redirecting the lookup rather
 * than the process environment keeps the change to script code.
 */
"local tmpdir = os.getenv('ARCHIPELAGO_TEMP_DIR')\n"
"if tmpdir and tmpdir ~= '' then\n"
"  local getenv = os.getenv\n"
"  os.getenv = function(n)\n"
"    if n == 'TEMP' or n == 'TMP' then return tmpdir end\n"
"    return getenv(n)\n"
"  end\n"
"end\n"
/* The 32-bit luasocket build predates tcp4/udp4, but the connectors call tcp4. */
"local function patchsocket(m)\n"
"  if type(m) == 'table' then\n"
"    m.tcp4 = m.tcp4 or m.tcp\n"
"    m.udp4 = m.udp4 or m.udp\n"
"  end\n"
"  return m\n"
"end\n"
"local _loadlib = package.loadlib\n"
"package.loadlib = function(path, sym)\n"
"  local fixed = path\n"
"  fixed = fixed:gsub('\\\\x64\\\\', '\\\\x86\\\\')\n"
"  fixed = fixed:gsub('/x64/', '/x86/')\n"
"  fixed = fixed:gsub('socket%-windows%-5%-4', 'socket-windows-5-1')\n"
"  local loader, e1, e2 = _loadlib(fixed, sym)\n"
"  if type(loader) ~= 'function' then return loader, e1, e2 end\n"
"  return function(...) return patchsocket(loader(...)) end\n"
"end\n"
"package.preload['socket'] = function()\n"
"  local dirs = { ap }\n"
"  if bundled then dirs[#dirs + 1] = bundled end\n"
"  for _, d in ipairs(dirs) do\n"
"    local dll = d .. '\\\\x86\\\\socket-windows-5-1.dll'\n"
"    local probe = io.open(dll, 'rb')\n"
"    if probe then\n"
"      probe:close()\n"
"      return { socket = assert(package.loadlib(dll, 'luaopen_socket_core'))() }\n"
"    end\n"
"  end\n"
"  error('32-bit luasocket not found; looked in: ' .. table.concat(dirs, ', '))\n"
"end\n"
/* These tables are registered from C; extend them rather than replacing them. */
"client = client or {}\n"
"function client.getversion() return '2.10.0' end\n"
"function client.ispaused() return false end\n"
"function client.getconfig()\n"
"  local cores = {}\n"
"  local keys = {}\n"
"  local Keys = {}\n"
"  function Keys:GetEnumerator()\n"
"    local i = 0\n"
"    local e = {}\n"
"    function e:MoveNext() i = i + 1; return i <= #keys end\n"
"    return setmetatable(e, { __index = function(_, k)\n"
"      if k == 'Current' then return keys[i] end\n"
"    end })\n"
"  end\n"
"  return { PreferredCores = setmetatable({ Keys = Keys },\n"
"           { __index = function(_, k) return cores[k] end }) }\n"
"end\n";

static void LuaHook(lua_State *L, lua_Debug *ar)
{
	(void) ar;

	if(g_stopRequested)
	{
		luaL_error(L, "script stopped");
	}
}

/*
 =======================================================================================================================
    Script thread
 =======================================================================================================================
 */
static unsigned __stdcall ScriptThreadProc(void *param)
{
	lua_State	*L;
	char		message[MAX_PATH + 128];

	(void) param;

	L = luaL_newstate();

	if(L == NULL)
	{
		LuaConsole_Print("Failed to create Lua state.");
		InterlockedExchange(&g_scriptAlive, 0);
		SetEvent(g_hFrameDone);
		return 0;
	}

	luaL_openlibs(L);
	g_L = L;

	lua_newtable(L);
	g_frameEndTableRef = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_newtable(L);
	g_exitTableRef = luaL_ref(L, LUA_REGISTRYINDEX);

	RegisterTable(L, "emu", emu_functions);
	RegisterTable(L, "gameinfo", gameinfo_functions);
	RegisterTable(L, "memory", memory_functions);
	RegisterTable(L, "mainmemory", memory_functions);
	RegisterTable(L, "gui", gui_functions);
	RegisterTable(L, "event", event_functions);
	RegisterTable(L, "console", console_functions);
	RegisterTable(L, "client", client_functions);
	RegisterTable(L, "joypad", joypad_functions);
	RegisterTable(L, "savestate", savestate_functions);

	lua_pushcfunction(L, l_print);
	lua_setglobal(L, "print");

	/* The prelude falls back to files shipped alongside the executable. */
	{
		char	dir[MAX_PATH];
		char	*slash;

		GetModuleFileName(NULL, dir, sizeof(dir));
		slash = strrchr(dir, '\\');

		if(slash != NULL) *slash = '\0';

		lua_pushstring(L, dir);
		lua_setglobal(L, "EMU_DIR");
	}

	if(luaL_dostring(L, g_prelude) != 0)
	{
		sprintf(message, "Lua prelude error: %s", lua_tostring(L, -1));
		LuaConsole_Print(message);
	}

	lua_sethook(L, LuaHook, LUA_MASKCOUNT, 100000);

	sprintf(message, "Running %s", g_scriptPath);
	LuaConsole_Print(message);

	if(luaL_dofile(L, g_scriptPath) != 0)
	{
		sprintf(message, "Lua error: %s", lua_tostring(L, -1));
		LuaConsole_Print(message);
		lua_pop(L, 1);
	}

	if(!g_stopRequested)
	{
		RunCallbackTable(L, g_exitTableRef);
	}

	lua_close(L);
	g_L = NULL;

	InterlockedExchange(&g_scriptAlive, 0);
	InterlockedExchange(&g_running, 0);

	/* Never leave the CPU thread waiting on a script that has finished. */
	SetEvent(g_hFrameDone);
	return 0;
}

/*
 =======================================================================================================================
    Public interface
 =======================================================================================================================
 */
void LuaShim_Startup(void)
{
	if(g_hRunFrame == NULL)
	{
		g_hRunFrame = CreateEvent(NULL, FALSE, FALSE, NULL);
		g_hFrameDone = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
}

BOOL LuaShim_IsRunning(void)
{
	return g_running ? TRUE : FALSE;
}

BOOL LuaShim_LoadScript(const char *path)
{
	if(path == NULL) return FALSE;

	LuaShim_StopScript();
	LuaShim_Startup();
	LuaConsole_Open();

	strncpy(g_scriptPath, path, MAX_PATH - 1);
	g_scriptPath[MAX_PATH - 1] = '\0';

	memset(g_input, 0, sizeof(g_input));
	g_drawCount = 0;

	InterlockedExchange(&g_stopRequested, 0);
	InterlockedExchange(&g_scriptAlive, 1);
	InterlockedExchange(&g_running, 1);
	g_frameCount = 0;

	ResetEvent(g_hRunFrame);
	ResetEvent(g_hFrameDone);

	g_hThread = (HANDLE) _beginthreadex(NULL, 0, ScriptThreadProc, NULL, 0, NULL);

	if(g_hThread == NULL)
	{
		InterlockedExchange(&g_running, 0);
		InterlockedExchange(&g_scriptAlive, 0);
		LuaConsole_Print("Failed to start the Lua script thread.");
		return FALSE;
	}

	return TRUE;
}

void LuaShim_StopScript(void)
{
	if(g_hThread == NULL) return;

	InterlockedExchange(&g_stopRequested, 1);
	InterlockedExchange(&g_running, 0);

	/* Release the script if it is parked inside emu.frameadvance(). */
	SetEvent(g_hRunFrame);

	if(WaitForSingleObject(g_hThread, 3000) == WAIT_TIMEOUT)
	{
		LuaConsole_Print("Lua script did not stop in time; terminating it.");
		TerminateThread(g_hThread, 0);
	}

	CloseHandle(g_hThread);
	g_hThread = NULL;
	InterlockedExchange(&g_scriptAlive, 0);

	/* Nothing is driving the overlay any more, so take it off the screen. */
	g_drawCount = 0;

	if(g_hOverlay != NULL)
	{
		EnterCriticalSection(&g_overlayLock);
		g_publishedCount = 0;
		LeaveCriticalSection(&g_overlayLock);

		ShowWindowAsync(g_hOverlay, SW_HIDE);
	}
}

void LuaShim_OnRomClosed(void)
{
	LuaShim_StopScript();
}

void LuaShim_ApplyInput(int control, DWORD *value)
{
	if(value == NULL || control < 0 || control > 3) return;

	if(g_running)
	{
		*value |= g_input[control].press;
		*value &= ~g_input[control].release;
	}

	g_lastKeys[control] = *value;
}

/* Runs on the UI thread, from the overlay window's WM_PAINT. */
static void PaintDrawList(HDC dc)
{
	HFONT	previous = NULL;
	int		i;

	SetBkMode(dc, TRANSPARENT);

	EnterCriticalSection(&g_overlayLock);

	for(i = 0; i < g_publishedCount; i++)
	{
		const LuaDrawText	*d = &g_published[i];
		HFONT				font = FontForSize(d->size);
		int					len = (int) strlen(d->text);
		HFONT				old;

		if(font == NULL || len == 0) continue;

		old = (HFONT) SelectObject(dc, font);

		if(previous == NULL) previous = old;

		/* Outline first so the text stays legible over any background. */
		SetTextColor(dc, RGB(0, 0, 0));
		TextOut(dc, d->x - 1, d->y, d->text, len);
		TextOut(dc, d->x + 1, d->y, d->text, len);
		TextOut(dc, d->x, d->y - 1, d->text, len);
		TextOut(dc, d->x, d->y + 1, d->text, len);

		SetTextColor(dc, d->color);
		TextOut(dc, d->x, d->y, d->text, len);
	}

	LeaveCriticalSection(&g_overlayLock);

	if(previous != NULL) SelectObject(dc, previous);
}

static LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
	case WM_ERASEBKGND:
		return 1;	/* WM_PAINT repaints every pixel. */

	case WM_PAINT:
		{
			PAINTSTRUCT	ps;
			HDC			dc = BeginPaint(hwnd, &ps);
			RECT		rc;
			HBRUSH		key = CreateSolidBrush(LUA_OVERLAY_KEY);

			GetClientRect(hwnd, &rc);
			FillRect(dc, &rc, key);
			DeleteObject(key);

			PaintDrawList(dc);

			EndPaint(hwnd, &ps);
		}

		return 0;
	}

	return DefWindowProc(hwnd, msg, wp, lp);
}

void LuaShim_SetRenderTarget(HWND main, HWND statusBar)
{
	WNDCLASS wc;

	g_hRenderWnd = main;
	g_hStatusBar = statusBar;

	if(!g_overlayLockReady)
	{
		InitializeCriticalSection(&g_overlayLock);
		g_overlayLockReady = TRUE;
	}

	if(g_hOverlay != NULL || main == NULL) return;

	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = OverlayProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = LUA_OVERLAY_CLASS;
	RegisterClass(&wc);

	/*
	 * A separate owned popup rather than a child window. A child shares the
	 * parent's client area, so the plugin's present paints straight over it;
	 * a top-level window is composited by the window manager and cannot be
	 * touched by the plugin. Owned so it follows the emulator's z-order,
	 * layered for the colour key, transparent so clicks fall through, and
	 * created on this thread because it is the one pumping messages.
	 */
	g_hOverlay = CreateWindowEx
		(
			WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
			LUA_OVERLAY_CLASS, NULL, WS_POPUP,
			0, 0, 0, 0, main, NULL, GetModuleHandle(NULL), NULL
		);

	if(g_hOverlay != NULL)
	{
		SetLayeredWindowAttributes(g_hOverlay, LUA_OVERLAY_KEY, 0, LWA_COLORKEY);
	}
}

/*
 * Hands the script's finished draw list to the UI thread. Called from the CPU
 * thread, so every window call here has to be one that does not wait on the
 * owning thread.
 */
void LuaShim_DrawOverlay(void)
{
	static RECT	placed;

	RECT	rc;
	POINT	origin;
	BOOL	changed;
	int		count = g_running ? g_drawCount : 0;

	if(g_hOverlay == NULL || !g_overlayLockReady) return;

	EnterCriticalSection(&g_overlayLock);

	changed = (count != g_publishedCount)
		   || (count > 0 && memcmp(g_published, g_draw, count * sizeof(LuaDrawText)) != 0);

	if(changed)
	{
		if(count > 0) memcpy(g_published, g_draw, count * sizeof(LuaDrawText));
		g_publishedCount = count;
	}

	LeaveCriticalSection(&g_overlayLock);

	if(count == 0 || !IsWindowVisible(g_hRenderWnd) || IsIconic(g_hRenderWnd))
	{
		if(IsWindowVisible(g_hOverlay)) ShowWindowAsync(g_hOverlay, SW_HIDE);
		return;
	}

	if(!RenderArea(&rc)) return;

	/* The overlay is its own window now, so it has to be placed in screen space. */
	origin.x = rc.left;
	origin.y = rc.top;
	ClientToScreen(g_hRenderWnd, &origin);

	rc.right = origin.x + (rc.right - rc.left);
	rc.bottom = origin.y + (rc.bottom - rc.top);
	rc.left = origin.x;
	rc.top = origin.y;

	/* Follow the emulator window as it is moved or resized. */
	if(memcmp(&rc, &placed, sizeof(rc)) != 0)
	{
		placed = rc;
		SetWindowPos
			(
				g_hOverlay, HWND_TOP,
				rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW
			);
		changed = TRUE;
	}
	else if(!IsWindowVisible(g_hOverlay))
	{
		ShowWindowAsync(g_hOverlay, SW_SHOWNOACTIVATE);
		changed = TRUE;
	}

	if(changed) InvalidateRect(g_hOverlay, NULL, FALSE);
}

void LuaShim_OnFrameEnd(void)
{
	if(!g_running || !g_scriptAlive) return;

	g_frameCount++;

	/*
	 * An override from joypad.set lasts a single frame in BizHawk. The script
	 * thread is about to run and the CPU thread is blocked until it yields, so
	 * clearing here cannot race with the input path.
	 */
	memset(g_input, 0, sizeof(g_input));

	SetEvent(g_hRunFrame);

	if(WaitForSingleObject(g_hFrameDone, LUA_FRAME_TIMEOUT_MS) == WAIT_TIMEOUT)
	{
		LuaConsole_Print("Lua script stalled for 5 seconds; detaching it.");
		InterlockedExchange(&g_running, 0);
	}

	/* The script's turn is over, so its draw list is complete for this frame. */
	LuaShim_DrawOverlay();
}

void LuaShim_Shutdown(void)
{
	int i;

	LuaShim_StopScript();

	for(i = 0; i < LUA_FONT_CACHE; i++)
	{
		if(g_fonts[i].font != NULL)
		{
			DeleteObject(g_fonts[i].font);
			g_fonts[i].font = NULL;
		}
	}

	if(g_hRunFrame != NULL)
	{
		CloseHandle(g_hRunFrame);
		CloseHandle(g_hFrameDone);
		g_hRunFrame = NULL;
		g_hFrameDone = NULL;
	}
}
