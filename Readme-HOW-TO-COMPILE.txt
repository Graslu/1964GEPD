Please visit http://1964emu.emulation64.com
or http://www.1964emu.com for the latest in 1964 news and information.

To compile this source code you will need Microsoft Visual C.NET.
Be sure to update your compiler with the latest service pack.

In the Configuration Manager are 4 build configurations:

Release (default)            = 1964 final public build. This is the build for end-users.
Release with .NET debugging  = Permits you to use the .NET debugger.
Release with Opcode Debugger = This is my debugger that compares interpretive opcodes
				to dyna ops and barks at you if there are any discrepancies.
Debug                        = 1964's N64 Debugger.

If you have source code related questions or If you
want to hire one of us for a job in this difficult job market,
you may leave us an email. If you have any other questions,
please use our messageboard.

Compiler settings
=================
Please note that 1964 uses the __fastcall calling convention by default, and
is built as a multithreaded application, needed for _beginthread(), _endthread().

The Release build is built using following additional Visual Studio .NET compiler settings:
- Maximize speed
- No .NET debugging tools

Preprocessor Definitions:
=========================
Release build has the following preprocessor definitions:
DYNAREC,WIN32,NDEBUG,_WINDOWS,ZLIB_DLL,WIN32_LEAN_AND_MEAN,ENABLE_64BIT_FPU

Debug build uses these preprocessor definitions:
DYNAREC,DYN_DEBUG,WIN32,_WINDOWS,VIDEO,ZIP_SUPPORT,HLE,GRAPHICS_TRACER,ZLIB_DLL,ENABLE_64BIT_FPU,DEBUG_COMMON,WINDEBUG_1964,_DEBUG

Release mode with opcode debugger uses the same as Release build, but adds:
ENABLE_OPCODE_DEBUGGER


1964 is copyright 1999-2002 by schibo and Rice

schibo - Joel Middendorf 
schibo@emulation64.com

Rice
rice1964@yahoo.com