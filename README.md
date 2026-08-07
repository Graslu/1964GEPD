# 1964 GEPD Edition — Archipelago / Lua build

A fork of [1964 GEPD Edition](https://github.com/Graslu/1964GEPD) that adds Lua
scripting with a BizHawk-compatible API, so the
[Archipelago](https://archipelago.gg/) GoldenEye 007 randomizer can be played on
1964 instead of BizHawk.

The Archipelago GoldenEye world ships a connector written against BizHawk's Lua
API. This build embeds Lua 5.1 and implements enough of that API for the
connector to run unmodified, which means you get 1964's Mouse Injector support
(proper mouse aiming) in a randomizer seed.

ROM files are not included.

## What this fork adds

**Lua scripting**

- Lua 5.1.5, built as `lua51.dll` by `lua51.vcxproj`.
- A `Lua` menu with `Open Script...` and `Stop Script`.
- `lua_bizhawk.c` implements the BizHawk surface the connector uses:
  `mainmemory` and `memory` reads and writes across 8/16/32-bit, signed and
  unsigned, big and little endian; `console`; `client`; `joypad`; `savestate`;
  and `gui`.
- `joypad.set` injects buttons through the input plugin, so scripts can drive
  the controller.
- `gui.drawText` draws through a layered window owned by the main window rather
  than onto the plugin's output directly, which keeps the video plugin from
  overdrawing it and avoids flicker. This is what renders the "locked" labels on
  the mission select screen.
- LuaSocket is located automatically, and `socket.tcp4` is aliased to
  `socket.tcp` because the 32-bit LuaSocket build predates the `tcp4` name that
  the Archipelago connectors call.

**Fixes needed to build and run on a modern toolchain**

The upstream project targets Visual Studio .NET 2002. Building it with a current
compiler surfaced several latent bugs, fixed here:

- `win32/registry.c` — `INI_OptionReadString` returned a pointer to a stack
  local, so every configuration string read back as garbage and plugins silently
  failed to load.
- `win32/Wingui.c` — `OPENFILENAME` structures were not zeroed, so the ROM and
  save-state dialogs never opened.
- `romlist.c` — the rebar was sent `sizeof(REBARBANDINFO)`. That struct has grown
  since 2002 and comctl32 rejects a size it does not recognise, which left the
  toolbar unparented and drawing the window's black background.
- `dynaRec/regcache.h` — constant propagation is disabled. A modern MSVC
  miscompiles the folded store addresses, and the first block of the boot code
  stores to address 0, faulting as a TLB store miss before the game starts.
- `dynaRec/regcache.c` — `ConstMap[].FinalAddressUsedAt` was left uninitialised
  for registers 0 through 7.
- `1964ini.c` — `Project64.rdb` is written next to `1964.exe`. The bare filename
  resolved against the current directory, which the ROM dialog moves to wherever
  the ROM was picked from, so Jabo's per-game settings were written to a file the
  plugin never reads.
- `win32/registry.c` — `AutoHideCursorWhenActive` now defaults to off, so the
  mouse pointer stays visible over the window and the existing <kbd>Tab</kbd>
  show/hide toggle becomes usable. Re-enable it under
  Options → User Options if you prefer the old behaviour.

## Building

Requires **Visual Studio 2022 Build Tools with the C++ workload**. The project
builds as 32-bit; a 64-bit build is not possible because the recompiler is
x86 assembly.

```
msbuild 1964.vcxproj /p:Configuration=Release /p:Platform=Win32
```

This produces `build\Release\1964.exe` and `build\Release\lua51.dll`. Copy both
next to a normal 1964 GEPD install, which supplies `zlib.dll` and the `Plugin`
directory.

`1964.vcxproj` disables DEP, ASLR and the buffer security check. The recompiler
generates and executes its own code and mixes hand-written `__asm` with C, so
those hardening features and aggressive inlining break it.

Note that the source files are Windows-1252, not UTF-8. Comments contain byte
`0xA3` as a line-continuation marker left by the formatter the original authors
used. Configure your editor accordingly, or it will replace those bytes with
U+FFFD and produce large meaningless diffs.

## Playing an Archipelago seed

1. Install Archipelago and generate or join a GoldenEye seed as usual.
2. Load your patched ROM in 1964.
3. `Lua → Open Script...` and pick `goldeneye_ap_randomizer.lua` from your
   Archipelago `data\lua` directory. Keep `connector_bizhawk_generic.lua` in that
   same directory; it is loaded by path, not by module name.
4. Start the Archipelago BizHawk Client. The connector listens on localhost
   ports 43055-43060 and the client finds it there.

By default the emulator looks for the Lua directory in
`%PROGRAMDATA%\Archipelago\data\lua`. Set `ARCHIPELAGO_LUA_DIR` to override it.

## Steam Deck and Linux

There is no native Linux build of 1964, and upstream's "Steam Deck" download is
also the Windows binary. Add `1964.exe` to Steam as a non-Steam game and force a
Proton compatibility tool.

Two environment variables exist for that case, set through the shortcut's Launch
Options:

```
ARCHIPELAGO_LUA_DIR="Z:\home\deck\Archipelago\data\lua" ARCHIPELAGO_TEMP_DIR="Z:\tmp" %command%
```

`ARCHIPELAGO_LUA_DIR` is required under Proton because `%PROGRAMDATA%` resolves
inside the Wine prefix, where a Linux Archipelago install is not visible.
`ARCHIPELAGO_TEMP_DIR` is only needed for DeathLink: the connector signals a
death with a flag file in the temp directory, and `%TEMP%` also points inside the
prefix, where a Linux client would never look.

A 32-bit Windows LuaSocket is used from `ap_lua\x86` beside the executable if
your Archipelago install does not provide one, since a Linux install ships `.so`
files that a Windows process cannot load.

## Known limitations

- `savestate.load` from Lua is not implemented. The connector only uses it when
  its `ENABLE_STARTUP_SAVESTATE` option is enabled, which is off by default.
- `joypad.set` drives digital buttons only. There is no analog stick injection,
  so `joypad.setanalog` is absent rather than silently doing nothing.
- The `gui.*` overlay is composited over the emulator window and is intended for
  windowed or borderless use. Exclusive fullscreen may hide it.
- The Lua console is a separate console window.
- The Proton path has not been tested on Deck hardware; it was verified on
  Windows only.

## Copyright

1964 is Copyright (c) 1999-2002 Joel Middendorf, and is distributed under the
GNU General Public License v2. See `LICENSE`.

Unmodified 1964 0.8.5 source code:
https://sourceforge.net/projects/schibo/files/1964%200.8.5/1964-2002-0922.zip/1964-2002-0922.zip

GEPD Edition and the Mouse Injector integration are by Graslu and contributors:
https://github.com/Graslu/1964GEPD

Lua 5.1.5 is Copyright (c) 1994-2012 Lua.org, PUC-Rio, under the MIT license.
See `lua/LUA-LICENSE.txt`.
