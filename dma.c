/*
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Direct Memory Access service
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
#include "interrupt.h"
#include "r4300i.h"
#include "n64rcp.h"
#include "dma.h"
#include "debug_option.h"
#include "iPIF.h"
#include "timer.h"
#include "emulator.h"
#include "hardware.h"
#include "1964ini.h"
#include "memory.h"
#include "win32/Dll_Audio.h"
#include "gamesave.h"
#include "win32/windebug.h"
#include "compiler.h"
#include "fileio.h"

#ifdef SAVEOPCOUNTER
#define EXTRA_DMA_TIMING(val)	DMAIncreaseTimer(val);
#else
#define EXTRA_DMA_TIMING(val)
#endif
uint32			PIDMASourceAddress = 0;
uint32			PIDMATargetAddress = 0;
uint32			PIDMACurrentPosition = 0;
uint32			PIDMALength = 0;

BOOL			DMAInProgress = FALSE;
uint32			SPDMASourceAddress = 0;
uint32			SPDMATargetAddress = 0;
uint32			SPDMACurrentPosition = 0;
uint32			SPDMALength = 0;
uint32			DMA_SP_Transfer_Source_Begin_Address = 0;
uint32			DMA_SP_Transfer_Target_Begin_Address = 0;
uint32			SIDMASourceAddress = 0;
uint32			SIDMATargetAddress = 0;
uint32			saved_si_dram_addr_reg = 0;
int				DMA_SI_Transfer_Count = 0;
int				DMA_SP_Transfer_Count = 0;

enum DMATYPE	PIDMAInProgress = NO_DMA_IN_PROGRESS;
enum DMATYPE	SIDMAInProgress = NO_DMA_IN_PROGRESS;
enum DMATYPE	SPDMAInProgress = NO_DMA_IN_PROGRESS;

void	DMAIncreaseTimer(uint32 val);

/*
 =======================================================================================================================
    Initialize the DMA
 =======================================================================================================================
 */
void InitDMA(void)
{
	PIDMASourceAddress	= 0;
	PIDMATargetAddress = 0;
	PIDMACurrentPosition = 0;
	PIDMALength = 0;

	DMAInProgress = FALSE;
	PIDMAInProgress = NO_DMA_IN_PROGRESS;
	SIDMAInProgress = NO_DMA_IN_PROGRESS;
	SPDMAInProgress = NO_DMA_IN_PROGRESS;

	SPDMASourceAddress = 0;
	SPDMATargetAddress = 0;
	SPDMACurrentPosition = 0;
	SPDMALength = 0;
	DMA_SP_Transfer_Source_Begin_Address = 0;
	DMA_SP_Transfer_Target_Begin_Address = 0;
	DMA_SP_Transfer_Count = 0;

	SIDMASourceAddress = 0;
	SIDMATargetAddress = 0;
	DMA_SI_Transfer_Count = 0;
}

/*
 =======================================================================================================================
    Peripheral Interface DMA Read/Write (fast)
 =======================================================================================================================
 */
void FastPIMemoryCopy(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	register int				i;
	unsigned register __int32	target; /* = PIDMATargetMemory+PIDMATargetAddress; */
	unsigned register __int32	source; /* = PIDMASourceMemory+PIDMASourceAddress; */
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(PIDMAInProgress == DMA_PI_WRITE && currentromoptions.Code_Check == CODE_CHECK_PROTECT_MEMORY)
	{
		Check_And_Invalidate_Compiled_Blocks_By_DMA(PIDMATargetAddress, PIDMALength, "PIDMA");
	}
	
	target = (uint32) PMEM_READ_UWORD(PIDMATargetAddress);
	source = (uint32) PMEM_READ_UWORD(PIDMASourceAddress);
	
	if((target & 3) == 0 && (source & 3) == 0 && (PIDMALength & 3) == 0)		/* DWORD align */
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		int i = -(((__int32) PIDMALength) >> 2);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		__asm
		{
			pushad
			mov eax, i
			mov edx, target
			mov ebx, source
			align 16
_Label :
			test eax, eax
			jz _Label2
			mov ecx, dword ptr[ebx]
			mov dword ptr[edx], ecx
			add ebx, 4
			add edx, 4
			inc eax
			jmp _Label
_Label2 :
			popad
		}
	}

	else if((target & 1) == 0 && (source & 1) == 0 && (PIDMALength & 1) == 0)	/* WORD align */
	{
		for(i = -(((__int32) PIDMALength) >> 1); i < 0; i++)
		{
			*(uint16 *) (target ^ 2) = *(uint16 *) (source ^ 2);
			target += 2;
			source += 2;
		}
	}
	else	/* not align */
	{
		for(i = -(__int32) PIDMALength; i < 0; i++) 
		{
			*(uint8 *) (target++ ^ 0x3) = *(uint8 *) (source++ ^ 0x3);
		}
	}
}

/*
 =======================================================================================================================
    Peripheral Interface DMA Read into Cartridge Domain
 =======================================================================================================================
 */
void DMA_PI_MemCopy_From_DRAM_To_Cart(void)
{
	emustatus.PIDMACount++;

	PIDMASourceAddress = (PI_DRAM_ADDR_REG & 0x00FFFFFF) | 0x80000000;
	PIDMATargetAddress = (PI_CART_ADDR_REG & 0x1FFFFFFF) | 0x80000000;
	PIDMACurrentPosition = 0;;
	PIDMALength = (PI_RD_LEN_REG & 0x00FFFFFF) + 1;

	DEBUG_PI_DMA_MACRO(
		TRACE4( "%08X: PI Copy RDRAM to CART  %d bytes %08X to %08X", gHWS_pc, PIDMALength, PIDMASourceAddress, PIDMATargetAddress);

		if(PI_DRAM_ADDR_REG & 0x7)
		{
			DisplayError("Warning, PI DMA, address does not align as requirement. RDRAM ADDR = %08X", PI_DRAM_ADDR_REG);
		}

		if((PIDMALength & 0x1) || (PI_CART_ADDR_REG & 0x1))
		{
			DisplayError
				(
					"Warning, PI DMA, need half word swap. RDRAM ADDR = %08X, CART ADDR=%08X, Len=%X",
					PI_DRAM_ADDR_REG,
					PI_CART_ADDR_REG,
					PIDMALength
				);
		}
	);

	DEBUG_SRAM_TRACE(TRACE3( "SRAM/FLASHRAM or CART Write, %08X bytes %08X to %08X", PIDMALength, PIDMASourceAddress, PIDMATargetAddress));

	if((PI_CART_ADDR_REG & 0x1F000000) == MEMORY_START_C2A2)	/* Flashram PI DMA read */
	{
		/* Tell flashram about this DMA event */
		if(currentromoptions.Save_Type == FLASHRAM_SAVETYPE || gamesave.firstusedsavemedia == FLASHRAM_SAVETYPE)
		{
			/* No delay for Flashram DMA read */
			DMA_RDRAM_To_Flashram(PIDMASourceAddress, PIDMATargetAddress, PIDMALength);
		}
		else
		{
			//FileIO_ReadFLASHRAM();
			PIDMAInProgress = DMA_PI_READ;
			//FastPIMemoryCopy();
			FileIO_WriteFLASHRAM(PIDMATargetAddress, PIDMASourceAddress, PIDMALength);
		}
		PIDMAInProgress = NO_DMA_IN_PROGRESS;
		EXTRA_DMA_TIMING(PIDMALength);
		Trigger_PIInterrupt();
		return;
	}

	if(currentromoptions.DMA_Segmentation == USEDMASEG_YES && debug_opcode == 0	)
	{
		/* Setup DMA transfer in segments */
		PIDMAInProgress = DMA_PI_READ;
		DMAInProgress = TRUE;
		CPUNeedToDoOtherTask = TRUE;

		/* PI_STATUS_REG |= PI_STATUS_DMA_BUSY; // Set PI status register DMA busy */
		PI_STATUS_REG |= PI_STATUS_DMA_IO_BUSY;					/* Set PI status register DMA busy */
		Set_PIDMA_Timer_Event(PIDMALength);
	}
	else
	{
		PIDMAInProgress = DMA_PI_READ;
		__try
		{
			FastPIMemoryCopy();
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			TRACE3
			(
				"Bad PI DMA: Source=%08X, Target=%08X. Len=%08X",
				PIDMASourceAddress,
				PIDMATargetAddress,
				PIDMALength
			)
		}

		PIDMAInProgress = NO_DMA_IN_PROGRESS;
		EXTRA_DMA_TIMING(PIDMALength);
		Trigger_PIInterrupt();
	}
}

/*
 =======================================================================================================================
    Peripheral Interface DMA Write
 =======================================================================================================================
 */
void DMA_PI_MemCopy_From_Cart_To_DRAM(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	len = (PI_WR_LEN_REG & 0x00FFFFFF) + 1;
	uint32	pi_dram_addr_reg = (PI_DRAM_ADDR_REG & 0x00FFFFFF) | 0x80000000;
	uint32	pi_cart_addr_reg = (PI_CART_ADDR_REG & 0x1FFFFFFF) | 0x80000000;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	emustatus.PIDMACount++;

	DEBUG_PI_DMA_MACRO
	(
		TRACE4
			(
				"%08X: PI Copy CART to RDRAM %db from %08X to %08X",
				gHWS_pc,
				len,
				pi_cart_addr_reg | 0xA0000000,
				pi_dram_addr_reg
			)
	);

	if(len & 0x1)
	{
		TRACE4( "%08X: PI Copy CART to RDRAM %db from %08X to %08X", gHWS_pc, len, pi_cart_addr_reg|0xA0000000, pi_dram_addr_reg);
		TRACE0("Warning, PI DMA, odd length");

		len++;	/* This makes Doraemon3 (J) works, I hope this will not affect other game */
				/* because this should not happen in regular games */
	}

#ifdef DEBUG_COMMON
	if(pi_dram_addr_reg & 0x7)
	{
		TRACE1("Warning, PI DMA, address does not align as requirement. RDRAM ADDR = %08X", pi_dram_addr_reg);
	}

	if(pi_cart_addr_reg & 0x1)
	{
		TRACE0("Warning, PI DMA, odd CARD address");
	}

	if(debugoptions.debug_sram)
	{
		if((pi_cart_addr_reg & 0x1F000000) == MEMORY_START_C2A1)
		{
			TRACE0("Copy C2A1 (0x05000000) to RDRAM")
		}
		else if((pi_cart_addr_reg & 0x1F000000) == MEMORY_START_C1A1)
		{
			TRACE0("Copy C1A1 (0x06000000) to RDRAM")
		}
		else if((pi_cart_addr_reg & 0x1F000000) == MEMORY_START_C2A2)
		{
			TRACE3("DMA Flashram to RDRAM %d byte from %08X to %08X", len, PI_CART_ADDR_REG, PI_DRAM_ADDR_REG)
		}
		else if((pi_cart_addr_reg & 0x1FF00000) == MEMORY_START_C1A3)
		{
			TRACE0("Copy C1A3 (0x1FD00000) to RDRAM")
		}
	}
#endif

	PIDMASourceAddress = pi_cart_addr_reg;
	PIDMATargetAddress = pi_dram_addr_reg;

	if(((PIDMATargetAddress & 0x1FFFFFFF) + len) > current_rdram_size)
	{
		DisplayError("Bad PI DMA address, PI DMA skipped");
		Trigger_PIInterrupt();
		return;
	}

	PIDMACurrentPosition = 0;;
	PIDMALength = len;

	/*
	** Self-Modify code detection
	*/
	if
	(
		emustatus.cpucore == DYNACOMPILER
	&&	(
			currentromoptions.Code_Check == CODE_CHECK_MEMORY_QWORD_AND_DMA
		||	currentromoptions.Code_Check == CODE_CHECK_MEMORY_BLOCK_AND_DMA
		||	currentromoptions.Code_Check == CODE_CHECK_DMA_ONLY
		)
	)
	{
		/* Clear Dynacomplied code */
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			register uint32 addr = PIDMATargetAddress;
			register int	i = -(__int32) PIDMALength;;
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			/* Align to DWORD boundary */
			i += (addr % 4);
			addr += (addr % 4);

			for(; i < 0; i += 4, addr += 4)
			{
L1:
				if(sDYN_PC_LOOKUP[((uint16) (addr >> 16))] != gMemoryState.dummyAllZero)
				{
					/* Need to clear the Dyna marks in dynarommap, how to do it? */
					if(*(uint32 *) (RDRAM_Copy + (addr & 0x007FFFFF)) != DUMMYOPCODE)
					{
						*(uint32 *) ((uint8 *) sDYN_PC_LOOKUP[((uint16) (addr >> 16))] + (uint16) addr) = 0;
					}
				}
				else
				{
					/* exception only happens at 0x10000 boundary or at the beginning */
					if(addr % 0x10000 == 0)
					{
						if(i + 0x10000 < 0)
						{
							i += 0x10000;
							addr += 0x10000;
							goto L1;
						}
						else
							break;
					}
					else if(i + addr % 0x10000 < 0)
					{
						i += addr %
						0x10000;
						addr += (addr / 0x10000 + 1) *
						0x10000;
						goto L1;
					}
					else
						break;
				}
			}
		}
	}

	/* Check SRAM and/or FLASHRAM DMA Operation */
	if((pi_cart_addr_reg & 0x1F000000) == MEMORY_START_C2A2)	/* Flashram PI DMA read */
	{
		if(currentromoptions.Save_Type == FLASHRAM_SAVETYPE || gamesave.firstusedsavemedia == FLASHRAM_SAVETYPE)
		{
			/*
			 * No timing delay for flashram DMA write
			 * Tell flashram about this DMA event
			 */
			DMA_Flashram_To_RDRAM(PIDMATargetAddress, PIDMASourceAddress, PIDMALength);
		}
		else
		{
			FileIO_ReadFLASHRAM(PIDMASourceAddress, PIDMATargetAddress, PIDMALength);
			PIDMAInProgress = DMA_PI_WRITE;

/*			__try
			{
				FastPIMemoryCopy();
			}

			__except(NULL, EXCEPTION_EXECUTE_HANDLER)
			{
				TRACE3
				(
					"Bad PI DMA: Source=%08X, Target=%08X. Len=%08X",
					PIDMASourceAddress,
					PIDMATargetAddress,
					PIDMALength
				)
			}
*/		}

		PIDMAInProgress = NO_DMA_IN_PROGRESS;
		EXTRA_DMA_TIMING(PIDMALength);
		Trigger_PIInterrupt();
		return;
	}

	/*
	 * else if( ((PIDMASourceAddress&0x0FFFFFFF)+len) > gAllocationLength ) { len
	 * gAllocationLength - (PIDMASourceAddress&0x0FFFFFFF); PIDMALength = len;
	 * TRACE1("Warning, DMA length is too long, trimmed, len=%d", len); }
	 */
	if(currentromoptions.DMA_Segmentation == USEDMASEG_YES && debug_opcode == 0)
	{
		/* Setup DMA transfer in segments */
		PIDMAInProgress = DMA_PI_WRITE;
		DMAInProgress = TRUE;
		CPUNeedToDoOtherTask = TRUE;

		/* PI_STATUS_REG |= PI_STATUS_DMA_BUSY; */
		PI_STATUS_REG |= PI_STATUS_DMA_IO_BUSY;
		Set_PIDMA_Timer_Event(PIDMALength);
	}
	else
	{
		PIDMAInProgress = DMA_PI_WRITE;
		__try
		{
			FastPIMemoryCopy();
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			TRACE3
			(
				"Bad PI DMA: Source=%08X, Target=%08X. Len=%08X",
				PIDMASourceAddress,
				PIDMATargetAddress,
				PIDMALength
			)
		}

		PIDMAInProgress = NO_DMA_IN_PROGRESS;
		EXTRA_DMA_TIMING(PIDMALength);
		Trigger_PIInterrupt();
	}
}

/*
 =======================================================================================================================
    Signal Processor DMA Read
 =======================================================================================================================
 */
void DMA_MemCopy_DRAM_To_SP(int WasCalledByRSP)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	sp_mem_addr_reg = SP_MEM_ADDR_REG;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	DEBUG_SP_DMA_MACRO(TRACE3("SP DMA Read  %d bytes from %08X to %08X", SP_RD_LEN_REG + 1, SP_DRAM_ADDR_REG, SP_MEM_ADDR_REG));

#ifdef DEBUG_COMMON
	/* Check Half Word Alignment */
	if(SP_DRAM_ADDR_REG & 0x7)
	{
		/*
		 * DisplayError("Warning, SP DMA, address does not align as requirement. RDRAM
		 * ADDR = %08X", SPDMASourceAddress);
		 */
		TRACE2
		(
			"Warning, SP DMA, address does not align as requirement, RDRAM ADDR = %08X, SP_MEM_ADDR_REG=%08X",
			SP_DRAM_ADDR_REG,
			SP_MEM_ADDR_REG
		);
	}

	if(SP_MEM_ADDR_REG & 0x3)
	{
		/*
		 * DisplayError("Warning, SP DMA, needs half word swap. RDRAM ADDR = %08X,
		 * SP_MEM_ADDR_REG=%08X", SPDMASourceAddress, SPDMATargetAddress);
		 */
		TRACE2
		(
			"Warning, SP DMA, address does not align as requirement, RDRAM ADDR = %08X, SP_MEM_ADDR_REG=%08X",
			SP_DRAM_ADDR_REG,
			SP_MEM_ADDR_REG
		);
	}
#endif
	SPDMALength = (SP_RD_LEN_REG & 0x00000FFF) +
	1;	/* SP_RD_LEN_REG bit [0-11] is length to transfer */

	if((currentromoptions.DMA_Segmentation == USEDMASEG_YES) && (!WasCalledByRSP))
	{
		DEBUG_SP_DMA_TRACE0("SP DMA Starting");

		/* Setup DMA transfer in segments */
		SPDMAInProgress = DMA_SP_READ;
		DMAInProgress = TRUE;
		CPUNeedToDoOtherTask = TRUE;

		SPDMASourceAddress = (SP_DRAM_ADDR_REG & 0x00FFFFFF) |
		0x80000000;
		SPDMATargetAddress = SP_DMEM_START +
		(SP_MEM_ADDR_REG & 0x00001FFF) +
		0x80000000;

		/* SPDMATargetAddress = sp_mem_addr_reg; */
		SPDMACurrentPosition = 0;;

		SP_DMA_BUSY_REG = 1;
		SP_STATUS_REG |= SP_STATUS_DMA_BUSY;

		DMA_SP_Transfer_Source_Begin_Address = SPDMASourceAddress;
		DMA_SP_Transfer_Target_Begin_Address = SPDMATargetAddress;
		DMA_SP_Transfer_Count = (SP_RD_LEN_REG >> 12) & 0x000000FF; /* Bit [12-19] is for count */
		Set_SPDMA_Timer_Event(SPDMALength);
	}
	else
	{
		__try
		{
//			//ignore IMEM for speed (if not using low-level RSP)
//			if ((emuoptions.UsingRspPlugin == FALSE) && (sp_mem_addr_reg & 0x00001000))
//				;
//			else
			memcpy
			(
				&gMS_SP_MEM[(sp_mem_addr_reg & 0x1FFF) >> 2],
				&gMS_RDRAM[SP_DRAM_ADDR_REG & 0x00FFFFFF],
				SPDMALength
			);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			DisplayError("Bad SP DMA copy");
		}

		EXTRA_DMA_TIMING(SPDMALength);

		SP_DMA_BUSY_REG = 0;
		SP_STATUS_REG &= ~SP_STATUS_DMA_BUSY;						/* Clear the DMA Busy bit */

		/* Rice. 2001-08018 */
		SP_STATUS_REG |= SP_STATUS_HALT;

		/* Trigger_SPInterrupt(); */
		SPDMAInProgress = NO_DMA_IN_PROGRESS;

		DEBUG_SP_DMA_TRACE0("SP DMA Finished");
	}
}

/*
 =======================================================================================================================
    Signal Processor DMA Write
 =======================================================================================================================
 */
void DMA_MemCopy_SP_to_DRAM(int WasCalledByRSP)
{
	/*~~~~~~~~~~~~*/
	uint16	segment;
	/*~~~~~~~~~~~~*/

#ifdef DEBUG_SP_DMA
	if(debugoptions.debug_sp_dma)
	{
		TRACE3("SP DMA Write %d bytes from %08X to %08X", SP_WR_LEN_REG + 1, SP_MEM_ADDR_REG, SP_DRAM_ADDR_REG);
	}

	/* Check Half Word Alignment */
	if(SP_DRAM_ADDR_REG & 0x7)
	{
		/*
		 * DisplayError("Warning, SP DMA, address does not align as required. RDRAM
		 * ADDR = %08X", SP_DRAM_ADDR_REG);
		 */
		TRACE2
		(
			"Warning, SP DMA, address does not align as requirementRDRAM ADDR = %08X, SP_MEM_ADDR_REG=%08X",
			SP_DRAM_ADDR_REG,
			SP_MEM_ADDR_REG
		);
	}

	if(SP_MEM_ADDR_REG & 0x3)
	{
		/*
		 * DisplayError("Warning, SP DMA, need half word swap. RDRAM ADDR = %08X,
		 * SP_MEM_ADDR_REG=%08X", SP_DRAM_ADDR_REG, SP_MEM_ADDR_REG);
		 */
		TRACE2
		(
			"Warning, SP DMA, address does not align as requirementRDRAM ADDR = %08X, SP_MEM_ADDR_REG=%08X",
			SP_DRAM_ADDR_REG,
			SP_MEM_ADDR_REG
		);
	}
#endif
	SPDMALength = (SP_WR_LEN_REG & 0x00000FFF) + 1;	/* SP_RD_LEN_REG bit [0-11] is length to transfer */

	if((currentromoptions.DMA_Segmentation == USEDMASEG_YES) && (!WasCalledByRSP))
	{
		DEBUG_SP_DMA_TRACE0("SP DMA Starting");

		/* Setup DMA transfer in segments */
		DMAInProgress = TRUE;
		SPDMAInProgress = DMA_SP_WRITE;
		CPUNeedToDoOtherTask = TRUE;

		/* SPDMASourceAddress = SP_DMEM_START + (SP_MEM_ADDR_REG & 0x00001FFF); */
		SPDMASourceAddress = SP_MEM_ADDR_REG;
		SPDMATargetAddress = SP_DRAM_ADDR_REG & 0x00FFFFFF |
		0x80000000;
		SPDMACurrentPosition = 0;;

		SP_DMA_BUSY_REG = 1;
		SP_STATUS_REG |= SP_STATUS_DMA_BUSY;

		DMA_SP_Transfer_Source_Begin_Address = SPDMASourceAddress;
		DMA_SP_Transfer_Target_Begin_Address = SPDMATargetAddress;
		DMA_SP_Transfer_Count = (SP_RD_LEN_REG >> 12) & 0x000000FF; /* Bit [12-19] is for count */
		Set_SPDMA_Timer_Event(SPDMALength);
	}
	else
	{
		segment = (uint16) (SP_MEM_ADDR_REG >> 16);

		memcpy
		(
			&gMS_RDRAM[SP_DRAM_ADDR_REG & 0x00FFFFFF],
			&gMS_SP_MEM[(SP_MEM_ADDR_REG & 0x1FFF) >> 2],
			(SP_WR_LEN_REG) + 1
		);

		EXTRA_DMA_TIMING(SPDMALength);

		SP_DMA_BUSY_REG = 0;
		SP_STATUS_REG &= ~SP_STATUS_DMA_BUSY;						/* Clear the DMA Busy bit */

		/* Rice. 2001-08018 */
		SP_STATUS_REG |= SP_STATUS_HALT;

		/* Trigger_SPInterrupt(); */
		SPDMAInProgress = NO_DMA_IN_PROGRESS;

		DEBUG_SP_DMA_TRACE0("SP DMA Finished");
	}
}

/*
 =======================================================================================================================
    Serial Interface DMA Write
 =======================================================================================================================
 */
void Do_DMA_MemCopy_SI_To_DRAM(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	si_dram_addr_reg = SI_DRAM_ADDR_REG;
	uint32	PIF_RAM_PHYS_addr;
	uint32	RDRAM_addr;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/* Check Half Word Alignment */
	if(SI_DRAM_ADDR_REG & 0x7)
	{
		TRACE1("Warning, SI DMA is skipped, address does not align. RDRAM ADDR = %08X", SI_DRAM_ADDR_REG);

		/* Skip this DMA */
		SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		Trigger_SIInterrupt();

		return;
	}

	DEBUG_SI_DMA_TRACE0("Doing actual SI DMA Read");

	if(currentromoptions.Code_Check == CODE_CHECK_PROTECT_MEMORY)
	{
		Check_And_Invalidate_Compiled_Blocks_By_DMA(si_dram_addr_reg | 0x80000000, 64, "SIDMA");
	}

	iPifCheck();

	PIF_RAM_PHYS_addr = (uint32) (&gMS_PIF[PIF_RAM_PHYS]);	/* From */
	RDRAM_addr = (uint32) & gMS_RDRAM[0];					/* To */

	_asm
	{
		mov edi, RDRAM_addr
		add edi, si_dram_addr_reg
		mov ecx, PIF_RAM_PHYS_addr
		mov eax, dword ptr[ecx]
		mov ebx, dword ptr[ecx + 4]
		mov dword ptr[edi], eax
		mov dword ptr[edi + 4], ebx
		mov eax, dword ptr[ecx + 8]
		mov ebx, dword ptr[ecx + 12]
		mov dword ptr[edi + 8], eax
		mov dword ptr[edi + 12], ebx

		add ecx, 16
		add edi, 16
		mov eax, dword ptr[ecx]
		mov ebx, dword ptr[ecx + 4]
		mov dword ptr[edi], eax
		mov dword ptr[edi + 4], ebx
		mov eax, dword ptr[ecx + 8]
		mov ebx, dword ptr[ecx + 12]
		mov dword ptr[edi + 8], eax
		mov dword ptr[edi + 12], ebx

		add ecx, 16
		add edi, 16
		mov eax, dword ptr[ecx]
		mov ebx, dword ptr[ecx + 4]
		mov dword ptr[edi], eax
		mov dword ptr[edi + 4], ebx
		mov eax, dword ptr[ecx + 8]
		mov ebx, dword ptr[ecx + 12]
		mov dword ptr[edi + 8], eax
		mov dword ptr[edi + 12], ebx

		add ecx, 16
		add edi, 16
		mov eax, dword ptr[ecx]
		mov ebx, dword ptr[ecx + 4]
		mov dword ptr[edi], eax
		mov dword ptr[edi + 4], ebx
		mov eax, dword ptr[ecx + 8]
		mov ebx, dword ptr[ecx + 12]
		mov dword ptr[edi + 8], eax
		mov dword ptr[edi + 12], ebx
	}

	EXTRA_DMA_TIMING(64);
	SI_STATUS_REG |= SI_STATUS_INTERRUPT;
	Trigger_SIInterrupt();
}

/*
 =======================================================================================================================
    Serial Interface DMA Read (using segmented DMA transfers)
 =======================================================================================================================
 */
void DMA_MemCopy_SI_To_DRAM(void)
{
	if(currentromoptions.DMA_Segmentation == USEDMASEG_YES && debug_opcode == 0)
	{
		DEBUG_SI_DMA_TRACE0("SI DMA Read Start");

		DMAInProgress = TRUE;
		SIDMAInProgress = DMA_SI_WRITE;
		CPUNeedToDoOtherTask = TRUE;

		DMA_SI_Transfer_Count = 64;

		saved_si_dram_addr_reg = SI_DRAM_ADDR_REG;

		/* Set SI DMA Busy */
		SI_STATUS_REG |= SI_STATUS_DMA_BUSY;
		Set_SIDMA_Timer_Event(64);
	}
	else
	{
		Do_DMA_MemCopy_SI_To_DRAM();
	}
}

/*
 =======================================================================================================================
    Serial Interface DMA Read
 =======================================================================================================================
 */
void Do_DMA_MemCopy_DRAM_to_SI(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	si_dram_addr_reg = SI_DRAM_ADDR_REG;
	uint32	PIF_RAM_PHYS_addr;
	uint32	RDRAM_addr;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/* Check Half Word Alignment */
	if(SI_DRAM_ADDR_REG & 0x7)
	{
		TRACE1("Warning, SI DMA is skipped, address does not align. RDRAM ADDR = %08X", SI_DRAM_ADDR_REG);

		/* Skip this DMA */
		SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		Trigger_SIInterrupt();
		return;
	}

	DEBUG_SI_DMA_TRACE0("Doing actual SI DMA Write");

	PIF_RAM_PHYS_addr = (uint32) (&gMS_PIF[PIF_RAM_PHYS]);
	RDRAM_addr = (uint32) & gMS_RDRAM[0];
	_asm
	{
		mov edi, PIF_RAM_PHYS_addr	/* PifRamPos */
		mov ecx, RDRAM_addr
		add ecx, si_dram_addr_reg
		mov eax, dword ptr[ecx]
		mov ebx, dword ptr[ecx + 4]
		mov dword ptr[edi], eax
		mov dword ptr[edi + 4], ebx
		mov eax, dword ptr[ecx + 8]
		mov ebx, dword ptr[ecx + 12]
		mov dword ptr[edi + 8], eax
		mov dword ptr[edi + 12], ebx
		add ecx, 16
		add edi, 16

		mov eax, dword ptr[ecx]
		mov ebx, dword ptr[ecx + 4]
		mov dword ptr[edi], eax
		mov dword ptr[edi + 4], ebx
		mov eax, dword ptr[ecx + 8]
		mov ebx, dword ptr[ecx + 12]
		mov dword ptr[edi + 8], eax
		mov dword ptr[edi + 12], ebx
		add ecx, 16
		add edi, 16

		mov eax, dword ptr[ecx]
		mov ebx, dword ptr[ecx + 4]
		mov dword ptr[edi], eax
		mov dword ptr[edi + 4], ebx
		mov eax, dword ptr[ecx + 8]
		mov ebx, dword ptr[ecx + 12]
		mov dword ptr[edi + 8], eax
		mov dword ptr[edi + 12], ebx
		add ecx, 16
		add edi, 16

		mov eax, dword ptr[ecx]
		mov ebx, dword ptr[ecx + 4]
		mov dword ptr[edi], eax
		mov dword ptr[edi + 4], ebx
		mov eax, dword ptr[ecx + 8]
		mov ebx, dword ptr[ecx + 12]
		mov dword ptr[edi + 8], eax
		mov dword ptr[edi + 12], ebx
	}

	EXTRA_DMA_TIMING(64);

	SI_STATUS_REG |= SI_STATUS_INTERRUPT;
	Trigger_SIInterrupt();
}

/*
 =======================================================================================================================
    Serial Interface DMA Read (using segmented DMA transfers)
 =======================================================================================================================
 */
void DMA_MemCopy_DRAM_to_SI(void)
{
	if(currentromoptions.DMA_Segmentation == USEDMASEG_YES && debug_opcode == 0)
	{
		DEBUG_SI_DMA_TRACE0("SI DMA Write Start");

		/* Setup DMA transfer in segments */
		DMAInProgress = TRUE;
		SIDMAInProgress = DMA_SI_READ;
		CPUNeedToDoOtherTask = TRUE;

		DMA_SI_Transfer_Count = 64;
		saved_si_dram_addr_reg = SI_DRAM_ADDR_REG;

		/* Set SI DMA Busy */
		SI_STATUS_REG |= SI_STATUS_DMA_BUSY;
		Set_SIDMA_Timer_Event(64);
	}
	else
	{
		Do_DMA_MemCopy_DRAM_to_SI();
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DMA_AI(void)
{
	/* AI_STATUS_REG |= AI_STATUS_DMA_BUSY; */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Do_SI_IO_Task(void)
{
}

/*
 =======================================================================================================================
    This function will be called in main loop of emulator
 =======================================================================================================================
 */
void DoDMASegment(void)
{
	DMAInProgress = NO_DMA_IN_PROGRESS;
}

/*
 =======================================================================================================================
    Perform a Peripheral Interface DMA transfer
 =======================================================================================================================
 */
void DoPIDMASegment(void)
{
	__try
	{
		FastPIMemoryCopy();
	}

	__except(NULL, EXCEPTION_EXECUTE_HANDLER)
	{
		TRACE3("Bad PI DMA: Source=%08X, Target=%08X. Len=%08X", PIDMASourceAddress, PIDMATargetAddress, PIDMALength)
	}

	PI_STATUS_REG &= ~PI_STATUS_DMA_IO_BUSY;	/* Clear the PI DMA BUSY */
	Trigger_PIInterrupt();
	PIDMAInProgress = NO_DMA_IN_PROGRESS;

	DEBUG_PI_DMA_TRACE0("PI DMA Finished");

	EXTRA_DMA_TIMING(PIDMALength)
}

/*
 =======================================================================================================================
    Perform a Signal Processor DMA transfer
 =======================================================================================================================
 */
void DoSPDMASegment(void)
{
	/* PI DMA Transfer finished */
	__try
	{
		if(SPDMAInProgress == DMA_SP_WRITE)
		{
			
			memcpy
			(
				&gMS_RDRAM[SP_DRAM_ADDR_REG & 0x00FFFFFF],
				&gMS_SP_MEM[(SP_MEM_ADDR_REG & 0x1FFF) >> 2],
				(SP_WR_LEN_REG) + 1
			);
		}
		else
		{
//			//ignore IMEM for speed (if not using low-level RSP)
//			if ((emuoptions.UsingRspPlugin == FALSE) && (SP_MEM_ADDR_REG & 0x00001000))
//			{
//				//Do Nothing
//			}
//			else
						
			memcpy
			(
				&gMS_SP_MEM[(SP_MEM_ADDR_REG & 0x1FFF) >> 2],
				&gMS_RDRAM[SP_DRAM_ADDR_REG & 0x00FFFFFF],
				SPDMALength
			);
		}
	}

	__except(NULL, EXCEPTION_EXECUTE_HANDLER)
	{
		DisplayError("Bad SP DMA copy");
	}

	SP_DMA_BUSY_REG = 0;
	SP_STATUS_REG &= ~SP_STATUS_DMA_BUSY;	/* Clear the DMA Busy bit */

	/* Rice. 2001-08018 */
	SP_STATUS_REG |= SP_STATUS_HALT;

	/* Trigger_SPInterrupt(); */
	SPDMAInProgress = NO_DMA_IN_PROGRESS;

	DEBUG_SP_DMA_TRACE0("SP DMA Finished");
	EXTRA_DMA_TIMING(SPDMALength);
}

/*
 =======================================================================================================================
    Perform a Serial Interface DMA transfer
 =======================================================================================================================
 */
void DoSIDMASegment(void)
{
	SI_DRAM_ADDR_REG = saved_si_dram_addr_reg;
	if(SIDMAInProgress == DMA_SI_READ)
	{
		Do_DMA_MemCopy_DRAM_to_SI();
		DEBUG_SI_DMA_TRACE0("SI DMA Read Finished");
		SI_STATUS_REG |= SI_STATUS_RD_BUSY; /* Set SI is busy doing IO */
		Set_SI_IO_Timer_Event(SI_IO_DELAY); /* this value is important, cannot be too large */
	}
	else	/* SIDMAInProgress == DMA_SI_WRITE */
	{
		Do_DMA_MemCopy_SI_To_DRAM();
		DEBUG_SI_DMA_TRACE0("SI DMA Write Finished");
	}

	SI_STATUS_REG &= ~SI_STATUS_DMA_BUSY;	/* Clear the SI DMA Busy signal */
	SIDMAInProgress = NO_DMA_IN_PROGRESS;	/* Next step should set SI IO busy */
}

/*
 =======================================================================================================================
    Perform a segmented DMA transfer called from the dynarec
 =======================================================================================================================
 */
void DynDoDMASegment(void)
{
	if(DMAInProgress) DoDMASegment();
}

/*
 =======================================================================================================================
    Since DMA transfers take time, increase the COUNT timer for the time elapsed (estimated).
 =======================================================================================================================
 */
void DMAIncreaseTimer(uint32 val)
{
	Count_Down(val * VICounterFactors[CounterFactor] / 2);	/* assume each pclock will transfer 4 bytes */
}
