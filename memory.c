/*$T memory.c GC 1.136 03/09/02 17:38:05 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Initializes memory lookup tables and allocates the ultra memory regions
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
#include <windowsx.h>
#include <memory.h>
#include <process.h>
#include <malloc.h>
#include "globals.h"
#include "r4300i.h"
#include "memory.h"
#include "n64rcp.h"
#include "interrupt.h"
#include "iPIF.h"
#include "1964ini.h"
#include "Dynarec/x86.h"
#include "hardware.h"
#include "win32/windebug.h"
#include "win32/wingui.h"
#include "gamesave.h"
#include "emulator.h"
#include "compiler.h"

void __cdecl	LogDyna(char *debug, ...);

uint32			rdram_sizes[3] = { MEMORY_SIZE_NO_EXPANSION, MEMORY_SIZE_NO_EXPANSION, MEMORY_SIZE_WITH_EXPANSION };

uint32			current_rdram_size = MEMORY_SIZE_NO_EXPANSION;
BOOL			rdram_is_at_0x20000000 = FALSE;

#include "globals.h"

uint8			*dynarommap[0x10000];
uint8			*sDWord[0x10000];
uint8			*sDWord2[0x10000];
uint8			*TLB_sDWord[0x100000];

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Init_R_AND_W(uint8 **sDWORD_R, uint8 *MemoryRange, uint32 startAddress, uint32 size)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint8	*pTmp = (uint8 *) MemoryRange;
	uint32	curSegment = ((startAddress) >> 16);
	uint32	endSegment = ((startAddress + size - 1) >> 16);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	while(curSegment <= endSegment)
	{
		sDWORD_R[curSegment | 0x8000] = pTmp;
		sDWORD_R[curSegment | 0xA000] = pTmp;
		pTmp += 0x10000;
		curSegment++;
	}
}

/*
 =======================================================================================================================
    Init DynaMemory Lookup Table £
    Function is called from fileio.c £
    right after a new ROM image is loaded £
 =======================================================================================================================
 */
void DynInit_R_AND_W(uint8 *MemoryRange, uint32 startAddress, uint32 endAddress)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint8	*pTmp = (uint8 *) MemoryRange;
	uint32	curSegment = ((startAddress & 0x1FFFFFFF) >> 16);	/* ????????? */
	uint32	endSegment = ((endAddress & 0x1FFFFFFF) >> 16);		/* ????????? */
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	while(curSegment <= endSegment)
	{
		sDYN_PC_LOOKUP[curSegment | 0x8000] = pTmp;
		sDYN_PC_LOOKUP[curSegment | 0xA000] = pTmp;
		pTmp += 0x10000;
		curSegment++;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitMemoryLookupTables1(uint8 **LUT, uint8 **LUT_2, MemoryState *gMemoryState)
{
	Init_R_AND_W(LUT, (uint8 *) gMemoryState->RDRAM, MEMORY_START_RDRAM, MEMORY_SIZE_RDRAM);
	Init_R_AND_W(LUT, (uint8 *) gMemoryState->ramRegs0, MEMORY_START_RAMREGS0, MEMORY_SIZE_RAMREGS0);
	Init_R_AND_W(LUT, (uint8 *) gMemoryState->ramRegs8, MEMORY_START_RAMREGS8, MEMORY_SIZE_RAMREGS8);
	Init_R_AND_W(LUT, (uint8 *) gMemoryState->SP_MEM, MEMORY_START_SPMEM, MEMORY_SIZE_SPMEM);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->SP_REG_1, MEMORY_START_SPREG_1, MEMORY_SIZE_SPREG_1);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->SP_REG_2, MEMORY_START_SPREG_2, MEMORY_SIZE_SPREG_2);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->DPC, MEMORY_START_DPC, MEMORY_SIZE_DPC);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->DPS, MEMORY_START_DPS, MEMORY_SIZE_DPS);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->MI, MEMORY_START_MI, MEMORY_SIZE_MI);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->VI, MEMORY_START_VI, MEMORY_SIZE_VI);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->AI, MEMORY_START_AI, MEMORY_SIZE_AI);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->PI, MEMORY_START_PI, MEMORY_SIZE_PI);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->RI, MEMORY_START_RI, MEMORY_SIZE_RI);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->SI, MEMORY_START_SI, MEMORY_SIZE_SI);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->C2A1, MEMORY_START_C2A1, MEMORY_SIZE_C2A1);
	Init_R_AND_W(LUT, (uint8 *) gMemoryState->C1A1, MEMORY_START_C1A1, MEMORY_SIZE_C1A1);
	Init_R_AND_W(LUT_2, (uint8 *) gMemoryState->C2A2, MEMORY_START_C2A2, MEMORY_SIZE_C2A2);

	Init_R_AND_W(LUT, (uint8 *) gMemoryState->ROM_Image, MEMORY_START_ROM_IMAGE, gAllocationLength);
	Init_R_AND_W(LUT, (uint8 *) gMemoryState->GIO_REG, MEMORY_START_GIO, MEMORY_SIZE_GIO_REG);
	Init_R_AND_W(LUT, (uint8 *) gMemoryState->PIF, MEMORY_START_PIF, MEMORY_SIZE_PIF);
	Init_R_AND_W(LUT, (uint8 *) gMemoryState->C1A3, MEMORY_START_C1A3, MEMORY_SIZE_C1A3);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitMemoryLookupTables(void)
{
	/*~~*/
	int i;
	/*~~*/

	sDWord_ptr = (uint8 **) &sDWord;
	sDWord2_ptr = (uint8 **) &sDWord2;

	for(i = 0; i < 0x10000; i++)
	{
		sDWord[i] = gMemoryState.dummyNoAccess;
		sDWord2[i] = gMemoryState.dummyNoAccess;
		sDYN_PC_LOOKUP[i] = gMemoryState.dummyAllZero;
#ifdef ENABLE_OPCODE_DEBUGGER
		sDWORD_R__Debug[i] = gMemoryState_Interpreter_Compare.dummyNoAccess;
		sDWORD_R_2__Debug[i] = gMemoryState_Interpreter_Compare.dummyNoAccess;
#endif
	}

	InitMemoryLookupTables1(sDWord, sDWord2, &gMemoryState);

#ifdef ENABLE_OPCODE_DEBUGGER
	InitMemoryLookupTables1(sDWORD_R__Debug, sDWORD_R_2__Debug, &gMemoryState_Interpreter_Compare);
#endif
	TLB_sDWord_ptr = (uint8 **) &TLB_sDWord;

	for(i = 0; i < 0x100000; i++)
	{
		TLB_sDWord[i] = sDWord[i >> 4] + 0x1000 * (i & 0xf);
	}

	init_whole_mem_func_array();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitVirtualDynaMemory(void)
{
	if(dyna_CodeTable != NULL) VirtualFree(dyna_CodeTable, 0, MEM_RELEASE);
	if(dyna_RecompCode != NULL) VirtualFree(dyna_RecompCode, 0, MEM_RELEASE);
	if(RDRAM_Copy != NULL) VirtualFree(RDRAM_Copy, 0, MEM_RELEASE);
	(double *) dyna_CodeTable = (double *) VirtualAlloc(NULL, CODETABLE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	(double *) dyna_RecompCode = (double *) VirtualAlloc(NULL, RECOMPCODE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	RDRAM_Copy = (uint8 *) VirtualAlloc(NULL, MEMORY_SIZE_WITH_EXPANSION, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if(dyna_CodeTable == NULL)
	{
		DisplayError("Cant alloc Mem for dyna_CodeTable");
		exit(1);
	}

	if(dyna_RecompCode == NULL)
	{
		DisplayError("Cant alloc Mem for dyna_RecompCode");
		exit(1);
	}

	if(RDRAM_Copy == NULL)
	{
		DisplayError("Cant alloc RDRAM_Copy");
		exit(1);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FreeVirtualDynaMemory(void)
{
	if(dyna_CodeTable != NULL) VirtualFree(dyna_CodeTable, 0, MEM_RELEASE);
	if(dyna_RecompCode != NULL) VirtualFree(dyna_RecompCode, 0, MEM_RELEASE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitVirtualMemory1(MemoryState *gMemoryState)
{
	if(gMemoryState->dummyReadWrite == NULL) gMemoryState->dummyReadWrite = (uint8 *) malloc(MEMORY_SIZE_DUMMY * 15);

	gMemoryState->SP_MEM = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY);
	gMemoryState->dummyAllZero = (uint8 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 2);
	gMemoryState->SP_REG_1 = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 3);
	gMemoryState->SP_REG_2 = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 4);
	gMemoryState->DPC = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 5);
	gMemoryState->DPS = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 6);
	gMemoryState->MI = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 7);
	gMemoryState->VI = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 8);
	gMemoryState->AI = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 9);
	gMemoryState->PI = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 10);
	gMemoryState->RI = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 11);
	gMemoryState->SI = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 12);
	gMemoryState->PIF = (uint8 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 13);
	gMemoryState->GIO_REG = (uint32 *) (gMemoryState->dummyReadWrite + MEMORY_SIZE_DUMMY * 14);

	/*
	 * We have to allocate the RDRAM and ExRDRAM atlocation 0x20000000, so then we can
	 * do faster £
	 * read/write_mem_rdram
	 */
	if(gMemoryState->RDRAM == NULL)
	{
		(double *) gMemoryState->RDRAM = (double *) VirtualAlloc
			(
				(double *) 0x20000000,
				MEMORY_SIZE_RDRAM + MEMORY_SIZE_EXRDRAM,
				MEM_RESERVE,
				PAGE_NOACCESS
			);
	}

	/* ifndef TEST_OPCODE_DEBUGGER_INTEGRITY */
	rdram_is_at_0x20000000 = TRUE;

	if((uint32) gMemoryState->RDRAM != 0x20000000 || (debug_opcode == 1))
	{
		/*
		 * DisplayError("Cannot allocate 8MB RDRAM at fixed address. You may have some
		 * problems with memory settings in Windows. 1964 will still work without any
		 * problems, but speed will be affected a little bit.");
		 */
		VirtualFree(gMemoryState->RDRAM, 0, MEM_RELEASE);
		(double *) gMemoryState->RDRAM = (double *) VirtualAlloc
			(
				NULL,
				MEMORY_SIZE_RDRAM + MEMORY_SIZE_EXRDRAM,
				MEM_RESERVE,
				PAGE_NOACCESS
			);
		rdram_is_at_0x20000000 = FALSE;
	}
	else
	/* endif */
	{
		rdram_is_at_0x20000000 = TRUE;
	}

	gMemoryState->RDRAM = (uint8 *) VirtualAlloc
		(
			(void *) (gMemoryState->RDRAM),
			MEMORY_SIZE_RDRAM,
			MEM_COMMIT,
			PAGE_READWRITE
		);

	/* These registers are a gray area.. */
	if(gMemoryState->ramRegs0 == NULL)
	{
		gMemoryState->ramRegs0 = (uint32 *) VirtualAlloc(NULL, MEMORY_SIZE_DUMMY, MEM_RESERVE, PAGE_NOACCESS);
		gMemoryState->ramRegs0 = (uint32 *) VirtualAlloc
			(
				(void *) (gMemoryState->ramRegs0),
				MEMORY_SIZE_RAMREGS0,
				MEM_COMMIT,
				PAGE_READWRITE
			);
	}

	if(gMemoryState->ramRegs4 == NULL)
	{
		gMemoryState->ramRegs4 = (uint32 *) VirtualAlloc
			(
				(void *) (((uint8 *) gMemoryState->ramRegs0) + 0x4000),
				MEMORY_SIZE_RAMREGS4,
				MEM_COMMIT,
				PAGE_READWRITE
			);
	}

	if(gMemoryState->ramRegs8 == NULL)
	{
		gMemoryState->ramRegs8 = (uint32 *) VirtualAlloc(NULL, MEMORY_SIZE_RAMREGS8, MEM_RESERVE, PAGE_NOACCESS);
		gMemoryState->ramRegs8 = (uint32 *) VirtualAlloc
			(
				(void *) (gMemoryState->ramRegs8),
				MEMORY_SIZE_RAMREGS8,
				MEM_COMMIT,
				PAGE_READWRITE
			);
	}

	if(gMemoryState->C1A1 == NULL) gMemoryState->C1A1 = (uint32 *) gamesave.SRam;
	if(gMemoryState->C1A3 == NULL) gMemoryState->C1A3 = (uint32 *) gamesave.SRam;
	if(gMemoryState->C2A1 == NULL) gMemoryState->C2A1 = (uint32 *) gamesave.SRam;

	if(gMemoryState->dummyNoAccess == NULL)
	{
		gMemoryState->dummyNoAccess = (uint8 *) VirtualAlloc(NULL, MEMORY_SIZE_DUMMY, MEM_RESERVE, PAGE_NOACCESS);
		gMemoryState->dummyNoAccess = (uint8 *) VirtualAlloc
			(
				(void *) (gMemoryState->dummyNoAccess),
				MEMORY_SIZE_DUMMY,
				MEM_COMMIT,
				PAGE_NOACCESS
			);
	}

	if(gMemoryState->C2A2 == NULL)
	{
		gMemoryState->C2A2 = (uint32 *) VirtualAlloc(NULL, MEMORY_SIZE_C2A2, MEM_RESERVE, PAGE_NOACCESS);
		gMemoryState->C2A2 = (uint32 *) VirtualAlloc
			(
				(void *) (gMemoryState->C2A2),
				MEMORY_SIZE_C2A2,
				MEM_COMMIT,
				PAGE_READWRITE
			);

		/* gMemoryState->C2A2 = (uint32*)gamesave.FlashRAM; //well, cannot do this */
	}

	memset(gMemoryState->dummyAllZero, 0, MEMORY_SIZE_DUMMY);
}

extern unsigned _int32	sync_valloc, sync_valloc2;
BOOL					opcode_debugger_memory_is_allocated = FALSE;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitVirtualMemory(void)
{
	InitVirtualMemory1(&gMemoryState);

#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY16
	if(debug_opcode!=0)
	{
		InitVirtualMemory1(&gMemoryState_Interpreter_Compare);
		opcode_debugger_memory_is_allocated = TRUE;
		TRACE0("Allocate memory for opcode debugger");
	}
#endif
	InitVirtualDynaMemory();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FreeVirtualMemory1(MemoryState *gMemoryState)
{
	if(gMemoryState->dummyReadWrite != NULL) free((uint8 *) gMemoryState->dummyReadWrite);

	VirtualFree(gMemoryState->RDRAM, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->ExRDRAM, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->ramRegs0, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->ramRegs4, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->ramRegs8, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->SP_MEM, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->C2A1, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->C1A1, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->C2A2, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->ROM_Image, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->C1A3, 0, MEM_RELEASE);
	VirtualFree(gMemoryState->dummyNoAccess, 0, MEM_RELEASE);

	gMemoryState->RDRAM = NULL;
	gMemoryState->ExRDRAM = NULL;
	gMemoryState->C2A1 = NULL;
	gMemoryState->C1A1 = NULL;
	gMemoryState->C2A2 = NULL;
	gMemoryState->GIO_REG = NULL;
	gMemoryState->C1A3 = NULL;
	gMemoryState->PIF = NULL;
	gMemoryState->dummyNoAccess = NULL;
	gMemoryState->dummyReadWrite = NULL;
	gMemoryState->dummyAllZero = NULL;
	gMemoryState->ramRegs0 = NULL;
	gMemoryState->ramRegs4 = NULL;
	gMemoryState->ramRegs8 = NULL;
	gMemoryState->SP_MEM = NULL;
	gMemoryState->SP_REG_1 = NULL;
	gMemoryState->SP_REG_2 = NULL;
	gMemoryState->DPC = NULL;
	gMemoryState->DPS = NULL;
	gMemoryState->MI = NULL;
	gMemoryState->VI = NULL;
	gMemoryState->AI = NULL;
	gMemoryState->PI = NULL;
	gMemoryState->RI = NULL;
	gMemoryState->SI = NULL;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FreeVirtualMemory(void)
{
	FreeVirtualMemory1(&gMemoryState);
	FreeVirtualMemory1(&gMemoryState_Interpreter_Compare);

	FreeVirtualDynaMemory();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitVirtualRomMemory1(MemoryState *gMemoryState, uint32 memsize)
{
	FreeVirtualRomMemory();

	memsize = ((memsize + 0x1fffff) / 0x200000) * 0x200000; /* bring it up to a even 2MB. */

	gMemoryState->ROM_Image = (uint8 *) VirtualAlloc(NULL, memsize, MEM_RESERVE, PAGE_NOACCESS);
	gMemoryState->ROM_Image = (uint8 *) VirtualAlloc
		(
			(void *) (gMemoryState->ROM_Image),
			memsize,
			MEM_COMMIT,
			PAGE_READWRITE
		);
	TRACE1("Allocated memory for ROM image = %08X", (uint32) gMemoryState->ROM_Image);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitVirtualRomMemory(uint32 memsize)
{
	InitVirtualRomMemory1(&gMemoryState, memsize);

#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY17
	if(debug_opcode!=0)
	{
		gMemoryState_Interpreter_Compare.ROM_Image = gMemoryState.ROM_Image;
	}
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FreeVirtualRomMemory(void)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < 0x10000; i++)
	{
		if(dynarommap[i] != NULL)
		{
			/* VirtualFree(dynarommap[i], 0x10000, MEM_DECOMMIT); */
			VirtualFree(dynarommap[i], 0, MEM_RELEASE);
			dynarommap[i] = NULL;
		}
	}

	if(gMemoryState.ROM_Image != NULL)
	{
		/* if ( !(VirtualFree((void*)gMemoryState.ROM_Image, 64*1024*1024,MEM_DECOMMIT)) ) */
		if(!(VirtualFree((void *) gMemoryState.ROM_Image, 0, MEM_RELEASE)))
		{
			DisplayError("Failed to release virtual memory for ROM Image, error code is %ld", GetLastError());
		}
		else
		{
			gMemoryState.ROM_Image = NULL;
		}
	}

	for(i = 0x1000; i <= 0x1FFF; i++)
	{
		sDWord[i + 0x8000] = gMemoryState.dummyNoAccess;
		sDWord[i + 0xA000] = gMemoryState.dummyNoAccess;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL UnmappedMemoryExceptionHelper(uint32 addr)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	realpc = addr;
	uint32	offset;
	int		index;
	/*~~~~~~~~~~~~~~~~~~*/

	/*
	 * if( NOT_IN_KO_K1_SEG(realpc) ) £
	 * realpc = TranslateITLBAddress(realpc);
	 */
	offset = realpc & 0x1FFF0000;
	index = realpc / 0x10000;

	if(dynarommap[index] != NULL) VirtualFree(dynarommap[index], 0, MEM_RELEASE);

	dynarommap[index] = VirtualAlloc(NULL, 0x10000, MEM_COMMIT, PAGE_READWRITE);
	if(dynarommap[index] == NULL)
	{
		DisplayError("Unable to allocate memory to support dyna rom mapping, PC=%8X", gHWS_pc);
		return FALSE;
	}
	else
	{
		memset(dynarommap[index], 0, 0x10000);

		/* Mapped the memory */
		if(IN_KO_K1_SEG(addr))
			DynInit_R_AND_W(dynarommap[index], offset, offset + 0x0000FFFF);
		else
			sDYN_PC_LOOKUP[index] = dynarommap[index];

		/*
		 * TRACE3("Dyna memory mapped %08X, PC=%08X, Address=%08X", realpc, gHWS_pc,
		 * (uint32)dynarommap[index]);
		 */
		return TRUE;
	}
}

/*
 =======================================================================================================================
    Set RDRAM size
 =======================================================================================================================
 */
void ResetRdramSize(int setsize)
{
	/*~~~~~~~*/
	int retval;
	/*~~~~~~~*/

	if(rdram_sizes[setsize] != current_rdram_size)
	{
		if(setsize == RDRAMSIZE_4MB)	/* Need to turn off the expansion pack */
		{
			/*~~*/
			int i;
			/*~~*/

			for(i = 0x40; i < 0x80; i++)
			{
				sDWord[i | 0x8000] = gMemoryState.dummyNoAccess;
				sDWord[i | 0xA000] = gMemoryState.dummyNoAccess;
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY18
#ifdef ENABLE_OPCODE_DEBUGGER
				if(debug_opcode!=0)
				{
					sDWORD_R__Debug[i | 0x8000] = gMemoryState_Interpreter_Compare.dummyNoAccess;
					sDWORD_R__Debug[i | 0xA000] = gMemoryState_Interpreter_Compare.dummyNoAccess;
				}
#endif
#endif
			}

			for(i = 0x400; i < 0x800; i++)
			{
				TLB_sDWord[i] = sDWord[i >> 4] + 0x1000 * (i & 0xf);
			}

			disable_exrdram_func_array();
			retval = VirtualFree(gMemoryState.ExRDRAM, MEMORY_SIZE_EXRDRAM, MEM_DECOMMIT);
			if(retval == 0)
			{
				DisplayError("Error to release the ExRDRAM");
			}
			else
			{
				gMemoryState.ExRDRAM = NULL;
			}
		}
		else	/* Need to turn on the expansion pack */
		{
			if(gMemoryState.ExRDRAM != NULL)
			{
				retval = VirtualFree(gMemoryState.ExRDRAM, MEMORY_SIZE_EXRDRAM, MEM_DECOMMIT);
			}

			gMemoryState.ExRDRAM = (uint8 *) VirtualAlloc
				(
					(((uint8 *) gMemoryState.RDRAM) + 0x400000),
					MEMORY_SIZE_EXRDRAM,
					MEM_COMMIT,
					PAGE_READWRITE
				);

			if((uint32) gMemoryState.ExRDRAM != (uint32) gMemoryState.RDRAM + 0x400000)
			{
				DisplayError
				(
					"Fix me in ResetRdramSize()!, RDRAM and ExRDRAM is not in contiguous memory address: RDRAM=%08X, ExRDRAM=%08X",
					(uint32) gMemoryState.RDRAM,
					(uint32) gMemoryState.ExRDRAM
				);
			}

			if(debug_opcode!=0)
			{
				gMemoryState_Interpreter_Compare.ExRDRAM = (uint8 *) VirtualAlloc
					(
						(((uint8 *) gMemoryState_Interpreter_Compare.RDRAM) + 0x400000),
						MEMORY_SIZE_EXRDRAM,
						MEM_COMMIT,
						PAGE_READWRITE
					);
				if
				(
					(uint32) gMemoryState_Interpreter_Compare.ExRDRAM !=
							(uint32) gMemoryState_Interpreter_Compare.RDRAM +
						0x400000
				)
				{
					DisplayError
					(
						"Fix me in ResetRdramSize()!, RDRAM and ExRDRAM is not in contiguous memory address: RDRAM=%08X, ExRDRAM=%08X",
						(uint32) gMemoryState_Interpreter_Compare.RDRAM,
						(uint32) gMemoryState_Interpreter_Compare.ExRDRAM
					);
				}
			}

			Init_R_AND_W(sDWord, (uint8 *) gMemoryState.ExRDRAM, MEMORY_START_EXRDRAM, MEMORY_SIZE_EXRDRAM);
#ifdef ENABLE_OPCODE_DEBUGGER
			if(debug_opcode!=0)
			{
				Init_R_AND_W
				(
					sDWORD_R__Debug,
					(uint8 *) gMemoryState_Interpreter_Compare.ExRDRAM,
					MEMORY_START_EXRDRAM,
					MEMORY_SIZE_EXRDRAM
				);
			}
#endif
			enable_exrdram_func_array();
		}

		current_rdram_size = rdram_sizes[setsize];
	}
	else if(setsize == RDRAMSIZE_8MB)	/* Need to use 8MB and we are on 8MB now */
	{
		/*~~*/
		int i;
		/*~~*/

		Init_R_AND_W(sDWord, (uint8 *) gMemoryState.ExRDRAM, MEMORY_START_EXRDRAM, MEMORY_SIZE_EXRDRAM);
#ifdef ENABLE_OPCODE_DEBUGGER
		if(debug_opcode!=0)
		{
			Init_R_AND_W
			(
				sDWORD_R__Debug,
				(uint8 *) gMemoryState_Interpreter_Compare.ExRDRAM,
				MEMORY_START_EXRDRAM,
				MEMORY_SIZE_EXRDRAM
			);
		}
#endif
		for(i = 0x400; i < 0x800; i++)
		{
			TLB_sDWord[0x80000 + i] = sDWord[(0x80000 + i) >> 4] + 0x1000 * (i & 0xf);
			TLB_sDWord[0xA0000 + i] = sDWord[(0xA0000 + i) >> 4] + 0x1000 * (i & 0xf);
		}

		enable_exrdram_func_array();
	}

	SetStatusBarText(3, setsize == RDRAMSIZE_4MB ? "4MB" : "8MB");
}

/*
 =======================================================================================================================
    These two CheckSum Checking routines are borrowed from Deadalus
 =======================================================================================================================
 */
void ROM_CheckSumMario(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~*/
	DWORD	*rom;
	DWORD	addr1;
	DWORD	a1;
	DWORD	t7;
	DWORD	v1 = 0;
	DWORD	t0 = 0;
	DWORD	v0 = 0xF8CA4DDC;	/* (MARIO_BOOT_CIC * 0x5d588b65) + 1; */
	DWORD	a3 = 0xF8CA4DDC;
	DWORD	t2 = 0xF8CA4DDC;
	DWORD	t3 = 0xF8CA4DDC;
	DWORD	s0 = 0xF8CA4DDC;
	DWORD	a2 = 0xF8CA4DDC;
	DWORD	t4 = 0xF8CA4DDC;
	DWORD	t8, t6, a0;
	/*~~~~~~~~~~~~~~~~~~~~~~*/

	rom = (DWORD *) gMemoryState.ROM_Image;

	TRACE0("Checking CRC for this ROM");

	for(addr1 = 0; addr1 < 0x00100000; addr1 += 4)
	{
		v0 = rom[(addr1 + 0x1000) >> 2];
		v1 = a3 + v0;
		a1 = v1;
		if(v1 < a3) t2++;

		v1 = v0 & 0x001f;
		t7 = 0x20 - v1;
		t8 = (v0 >> (t7 & 0x1f));
		t6 = (v0 << (v1 & 0x1f));
		a0 = t6 | t8;

		a3 = a1;
		t3 ^= v0;
		s0 += a0;
		if(a2 < v0)
			a2 ^= a3 ^ v0;
		else
			a2 ^= a0;

		t0 += 4;
		t7 = v0 ^ s0;
		t4 += t7;
	}

	TRACE0("Finish CRC Checking");

	a3 ^= t2 ^ t3;				/* CRC1 */
	s0 ^= a2 ^ t4;				/* CRC2 */

	if(a3 != rom[0x10 >> 2] || s0 != rom[0x14 >> 2])
	{
		/* DisplayError("Warning, CRC values don't match, fixed"); */
		TRACE0("Warning, CRC values don't match, fixed");

		rom[0x10 >> 2] = a3;
		rom[0x14 >> 2] = s0;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ROM_CheckSumZelda(void)
{
	/*~~~~~~~~~~~~~~~~~~~~*/
	DWORD	*rom;
	DWORD	addr1;
	DWORD	addr2;
	DWORD	t5 = 0x00000020;
	DWORD	a3 = 0xDF26F436;
	DWORD	t2 = 0xDF26F436;
	DWORD	t3 = 0xDF26F436;
	DWORD	s0 = 0xDF26F436;
	DWORD	a2 = 0xDF26F436;
	DWORD	t4 = 0xDF26F436;
	DWORD	v0, v1, a1, a0;
	/*~~~~~~~~~~~~~~~~~~~~*/

	rom = (DWORD *) gMemoryState.ROM_Image;

	addr2 = 0;

	TRACE0("Checking CRC for this ROM");

	for(addr1 = 0; addr1 < 0x00100000; addr1 += 4)
	{
		v0 = rom[(addr1 + 0x1000) >> 2];
		v1 = a3 + v0;
		a1 = v1;

		if(v1 < a3) t2++;

		v1 = v0 & 0x1f;
		a0 = (v0 >> (t5 - v1)) | (v0 << v1);
		a3 = a1;
		t3 = t3 ^ v0;
		s0 += a0;
		if(a2 < v0)
			a2 ^= a3 ^ v0;
		else
			a2 ^= a0;

		t4 += rom[(addr2 + 0x750) >> 2] ^ v0;
		addr2 = (addr2 + 4) & 0xFF;
	}

	TRACE0("Finish CRC Checking");

	a3 ^= t2 ^ t3;
	s0 ^= a2 ^ t4;

	if(a3 != rom[0x10 >> 2] || s0 != rom[0x14 >> 2])
	{
		/* DisplayError("Warning, CRC values don't match, fixed"); */
		TRACE0("Warning, CRC values don't match, fixed");

		rom[0x10 >> 2] = a3;
		rom[0x14 >> 2] = s0;
	}

	TRACE2("Generating CRC [M%d / %d]", 0x00100000, 0x00100000);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Debugger_Copy_Memory(MemoryState *target, MemoryState *source)
{
	memcpy(target->RDRAM, source->RDRAM, current_rdram_size);
	memcpy(target->ramRegs0, source->ramRegs0, MEMORY_SIZE_RAMREGS0);
	memcpy(target->ramRegs4, source->ramRegs4, MEMORY_SIZE_RAMREGS4);
	memcpy(target->ramRegs8, source->ramRegs8, MEMORY_SIZE_RAMREGS8);
	memcpy(target->SP_MEM, source->SP_MEM, MEMORY_SIZE_SPMEM);
	memcpy(target->SP_REG_1, source->SP_REG_1, MEMORY_SIZE_SPREG_1);
	memcpy(target->SP_REG_2, source->SP_REG_2, MEMORY_SIZE_SPREG_2);
	memcpy(target->DPC, source->DPC, MEMORY_SIZE_DPC);
	memcpy(target->DPS, source->DPS, MEMORY_SIZE_DPS);
	memcpy(target->MI, source->MI, MEMORY_SIZE_MI);
	memcpy(target->VI, source->VI, MEMORY_SIZE_VI);
	memcpy(target->AI, source->AI, MEMORY_SIZE_AI);
	memcpy(target->PI, source->PI, MEMORY_SIZE_PI);
	memcpy(target->RI, source->RI, MEMORY_SIZE_RI);
	memcpy(target->SI, source->SI, MEMORY_SIZE_SI);
	memcpy(target->GIO_REG, source->GIO_REG, MEMORY_SIZE_GIO_REG);
	memcpy(target->PIF, source->PIF, MEMORY_SIZE_PIF);
	memcpy(target->TLB, source->TLB, sizeof(tlb_struct) * MAXTLB);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL ProtectBlock(uint32 pc)
{
	if(IN_KO_K1_SEG(pc))
		pc = pc & 0xDFFFFFFF;
	else
		return FALSE;

	if(pc < 0x80000000 + current_rdram_size)
	{
		protect_memory_set_func_array(pc);
	}

	return TRUE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL UnprotectBlock(uint32 pc)
{
	if(IN_KO_K1_SEG(pc))
		pc = pc & 0xDFFFFFFF;
	else
		return FALSE;

	if(pc >= 0x80000000 && pc < 0x80000000 + current_rdram_size)
	{
		unprotect_memory_set_func_array(pc);
	}

	return TRUE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UnprotectAllBlocks(void)
{
	/*~~~~~~*/
	uint32	i;
	/*~~~~~~*/

	for(i = 0; i < current_rdram_size / 0x1000; i++)
	{
		UnprotectBlock(0x80000000 + i * 0x1000);
	}

	PROTECT_MEMORY_TRACE(TRACE0("Unprotect All Blocks"));
}
