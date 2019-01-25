/*$T Tlb.c GC 1.136 03/09/02 17:29:32 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Translation Lookaside Buffer (TLB) emulation. TLB converts virtual addresses to physical ones.
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
#include "globals.h"
#include "r4300i.h"
#include "hardware.h"
#include "interrupt.h"
#include "win32/windebug.h"
#include "debug_option.h"
#include "timer.h"
#include "memory.h"
#include "1964ini.h"
#include "emulator.h"
#include "plugins.h"
#include "compiler.h"

/* Local Definition and Variables */
#define MAXITLB 2
#define MAXDTLB 3

/* Debug Output */
#define DUMMYTLBINDEX	(-1)

int			ITLB_Index[MAXITLB];
int			DTLB_Index[MAXDTLB];
int			last_itlb_index;
int			last_dtlb_index;
static BOOL newtlb;

/* Define the TLB Lookup Table */
uint32		Direct_TLB_Lookup_Table[0x100000];
uint32		TLB_Refill_Exception_Vector = 0x80000080;
uint32		TLB_Error_Vector = 0x80000000;
BOOL		trigger_tlb_exception_faster = FALSE;
BOOL		ITLB_Error = FALSE;

tlb_struct	dummy_tlb;
void		Build_Whole_Direct_TLB_Lookup_Table(void);
void		Build_Direct_TLB_Lookup_Table(int index, BOOL tobuild);
void		Refresh_Direct_TLB_Lookup_Table(int index);
uint32		Direct_TLB_Lookup(uint32 address, int operation);

/* TLB.c Internal Functions */
uint32		TLBMapErrorAddress(uint32 address);
uint32		Trigger_TLB_Refill_Exception(uint32 address, int operation);
uint32		Trigger_TLB_Invalid_Exception(uint32 address, int operation);

/*
 =======================================================================================================================
    Translation Lookaside Buffer Probe. The Index register is loaded with the address of the TLB entry whose contents
    match the contents of the EntryHi register. If no TLB entry matches, the high-order bit of the Index register is
    set. The architecture does not specify the operation of memory references associated with the instruction
    immediately after a TLBP instruction, nor is the operation specified if more than one TLB entry matches.
 =======================================================================================================================
 */
void r4300i_COP0_tlbp(uint32 Instruction)
{
	/*~~~~~~~~~~*/
	uint32	index;
	/*~~~~~~~~~~*/

	/* uint32 g; */
	gHWS_COP0Reg[INDEX] |= 0x80000000;	/* initially set high-order bit */

	for(index = 0; index <= NTLBENTRIES; index++)
	{
		/*
		 * Not sure about here, if we should compare the all TLBHI_VPN2MASK, or need to
		 * mask with TLB.MyHiMask £
		 * I think should just mask with TLBHI_VPN2MASK, not with TLB.MyHiMask according
		 * to r4300i manual £
		 * But doing this, some game will give errors of TLBP lookup fail. £
		 * if( ( (gMS_TLB[index].EntryHi & TLBHI_VPN2MASK) == (gHWS_COP0Reg[ENTRYHI] &
		 * TLBHI_VPN2MASK) ) &&
		 */
		if
		(
			((gMS_TLB[index].EntryHi & gMS_TLB[index].MyHiMask) == (gHWS_COP0Reg[ENTRYHI] & gMS_TLB[index].MyHiMask))
		&&	(
				(gMS_TLB[index].EntryLo0 & TLBLO_G & gMS_TLB[index].EntryLo1)
			||	(gMS_TLB[index].EntryHi & TLBHI_PIDMASK) == (gHWS_COP0Reg[ENTRYHI] & TLBHI_PIDMASK)
			)
		)
		{
			gHWS_COP0Reg[INDEX] = index;
			TLB_DETAIL_TRACE
			(
				TRACE2
					(
						"TLBP - Load INDEX register:%d, VPN2 = 0x%08X [bit 31-13]",
						index,
						((uint32) gHWS_COP0Reg[ENTRYHI] & TLBHI_VPN2MASK)
					)
			) return;
		}
	}

	/*
	 * TLB_TRACE(TRACE1( "TLBP - no match, VPN2 = 0x%08X [bit 31-13]",
	 * ((uint32)gHWS_COP0Reg[ENTRYHI] & TLBHI_VPN2MASK)))
	 */
	TLB_TRACE(TRACE1("TLBP - no match, ENTRYHI = 0x%08X", ((uint32) gHWS_COP0Reg[ENTRYHI])))
}

/*
 =======================================================================================================================
    TLBR - Translation Lookaside Buffer Read. The G bit (which controls ASID matching) read from the TLB is written
    into both of the EntryLo0 and EntryLo1 registers. The EntryHi and EntryLo registers are loaded with the contents of
    the TLB entry pointed at by the contents of the TLB Index register. The operation is invalid (and the results are
    unspecified) if the contents of the TLB Index register are greater than the number of TLB entries in the processor.
 =======================================================================================================================
 */
void r4300i_COP0_tlbr(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	index = gHWS_COP0Reg[INDEX] & 0x7FFFFFFF;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(index > NTLBENTRIES)
	{
		TLB_TRACE(TRACE1("ERROR: tlbr received an invalid index=%08X", index));
	}

	index &= NTLBENTRIES;

	TLB_DETAIL_TRACE(TRACE1("TLBR - Read from TLB[%d]", index)) gHWS_COP0Reg[PAGEMASK] = (uint32)
		(gMS_TLB[index].PageMask);
	gHWS_COP0Reg[ENTRYHI] = (uint32) (gMS_TLB[index].EntryHi);
	gHWS_COP0Reg[ENTRYHI] &= (~(uint32) (gMS_TLB[index].PageMask));
	gHWS_COP0Reg[ENTRYLO1] = (uint32) (gMS_TLB[index].EntryLo1);
	gHWS_COP0Reg[ENTRYLO0] = (uint32) (gMS_TLB[index].EntryLo0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void tlb_write_entry(int index)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	tlb_struct	*theTLB;
	int			g = (gHWS_COP0Reg[ENTRYLO0] & gHWS_COP0Reg[ENTRYLO1]) & TLBLO_G;	/* The g bit */
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(index == DUMMYTLBINDEX)	/* please write to dummy tlb */
	{
		theTLB = &dummy_tlb;
	}
	else
		theTLB = &gMS_TLB[index];

	if(index > NTLBENTRIES)
	{
		DisplayError("ERROR: tlbwi called with invalid index");
		return;
	}

#ifdef DEBUG_TLB_DETAIL
	if(debugoptions.debug_tlb_detail && index != DUMMYTLBINDEX)
	{
		TRACE1("TLBWI/TLBWR - Load TLB[%d]", index);
		TRACE2("PAGEMASK = 0x%08X, ENTRYHI = 0x%08X", (uint32) gHWS_COP0Reg[PAGEMASK], (uint32) gHWS_COP0Reg[ENTRYHI]);
		TRACE2
		(
			"ENTRYLO1 = 0x%08X, ENTRYLO0 = 0x%08X",
			(uint32) gHWS_COP0Reg[ENTRYLO1],
			(uint32) gHWS_COP0Reg[ENTRYLO0]
		);
	}
#endif
	theTLB->valid = 0;			/* Not sure if we should use this valid bit any more */

	/*
	 * according to TLB comparasion, it is OK to have only EntryLo0 or EntryLo1 valid,
	 * not both of them £
	 * yes, still need in Direct TLB lookup
	 */
	if((gHWS_COP0Reg[ENTRYLO1] & TLBLO_V) || (gHWS_COP0Reg[ENTRYLO0] & TLBLO_V)) theTLB->valid = 1;

	theTLB->PageMask = gHWS_COP0Reg[PAGEMASK];
	theTLB->EntryLo1 = (gHWS_COP0Reg[ENTRYLO1] | g);
	theTLB->EntryLo0 = (gHWS_COP0Reg[ENTRYLO0] | g);
	theTLB->MyHiMask = ~(uint32) theTLB->PageMask & TLBHI_VPN2MASK;

	/*
	 * theTLB->EntryHi = gHWS_COP0Reg[ENTRYHI]; // This EntryHi should just copy from
	 * the ENTRYHI register £
	 * we will not store the g bit in the EntryHi, but keep them in EntryLo0 and
	 * EntryLo1 £
	 * theTLB->EntryHi = (gHWS_COP0Reg[ENTRYHI] & (~(uint32)gHWS_COP0Reg[PAGEMASK])) |
	 * (gHWS_COP0Reg[PAGEMASK]&TLBHI_PIDMASK);
	 */
	theTLB->EntryHi = gHWS_COP0Reg[ENTRYHI] & (~(uint32) gHWS_COP0Reg[PAGEMASK]);

	switch(theTLB->PageMask)
	{
	case 0x00000000:			/* 4k */theTLB->LoCompare = 0x00001000; break;
	case 0x00006000:			/* 16k */theTLB->LoCompare = 0x00004000; break;
	case 0x0001E000:			/* 64k */theTLB->LoCompare = 0x00010000; break;
	case 0x0007E000:			/* 256k */theTLB->LoCompare = 0x00040000; break;
	case 0x001FE000:			/* 1M */theTLB->LoCompare = 0x00100000; break;
	case 0x007FE000:			/* 4M */theTLB->LoCompare = 0x00400000; break;
	case 0x01FFE000:			/* 16M */theTLB->LoCompare = 0x01000000; break;
	default:			DisplayError("ERROR: tlbwi - invalid page size"); break;
	}

	newtlb = TRUE;
}

/*
 =======================================================================================================================
    TLBWI - Translation Lookaside Buffer Write Index. The G bit of the TLB is written with the logical AND of the G
    bits in the EntryLo0 and EntryLo1 registers. The TLB entry pointed at by the contents of the TLB Index register is
    loaded with the contents of the EntryHi and EntryLo registers. The operation is invalid (and the results are
    unspecified) if the contents of the TLB Index register are greater than the number of TLB entries in the processor.
 =======================================================================================================================
 */
void r4300i_COP0_tlbwi(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int index = gHWS_COP0Reg[INDEX] & NTLBENTRIES;
	int new_valid = ((gHWS_COP0Reg[ENTRYLO1] & TLBLO_V) || (gHWS_COP0Reg[ENTRYLO0] & TLBLO_V));
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	TLB_TRACE(TRACE1("TLBWI - Load TLB[%d]", index));

	if(gMS_TLB[index].valid == 1)
	{
		/*~~*/
		int i;
		/*~~*/

		for(i = 0; i < MAXDTLB; i++)
		{
			if(ITLB_Index[i] == index)
			{
				TLB_DETAIL_TRACE(TRACE1("TLB %d is unmapped while ITLB is still mapped to it", index));
				break;
			}
		}
	}

	Refresh_Direct_TLB_Lookup_Table(index);
}

/*
 =======================================================================================================================
    TLBWR - Translation Lookaside Buffer Write Random. The G bit of the TLB is written with the logical AND of the G
    bits in the EntryLo0 and EntryLo1 registers. The TLB entry pointed at by the contents of the TLB Random register is
    loaded with the contents of the EntryHi and EntryLo registers
 =======================================================================================================================
 */
void r4300i_COP0_tlbwr(uint32 Instruction)
{
	/*~~~~~~~~~*/
	int index, i;
	/*~~~~~~~~~*/

	/*
	 * gHWS_COP0Reg[RANDOM] = Get_COUNT_Register() %
	 * (0x40-(gHWS_COP0Reg[WIRED]&0x3f))+gHWS_COP0Reg[WIRED];
	 */
	gHWS_COP0Reg[RANDOM] = Get_COUNT_Register() % (0x20 - (gHWS_COP0Reg[WIRED] & 0x1f)) + gHWS_COP0Reg[WIRED];

	index = gHWS_COP0Reg[RANDOM] & NTLBENTRIES;

	/* hack, only use invalid entry */
	if(gMS_TLB[index].valid == 1)
	{
		for(i = gHWS_COP0Reg[WIRED]; i <= NTLBENTRIES; i++)
		{
			if(gMS_TLB[i].valid == 0)
			{
				index = i;
				break;
			}
		}
	}

	TLB_TRACE(TRACE1("TLBWR - Load TLB[%d]", index));
	Refresh_Direct_TLB_Lookup_Table(index);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 TranslateITLBAddress(uint32 address)
{
	return Direct_TLB_Lookup(address, TLB_INST);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 TranslateTLBAddressForLoad(uint32 address)
{
	return Direct_TLB_Lookup(address, TLB_LOAD);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 TranslateTLBAddressForStore(uint32 address)
{
	return Direct_TLB_Lookup(address, TLB_STORE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 RedoTLBLookupAfterException(uint32 address, int operation)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32		realAddress = 0x80000000;
	_int32		c;
	tlb_struct	*theTLB;
	uint32		EntryLo;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/* search the tlb entries */
	for(c = 0; c < MAXTLB; c++)
	{
		theTLB = &gMS_TLB[c];

		/*
		 * skip unused entries £
		 * if (theTLB->valid == 0) continue; £
		 * if ( ((theTLB->EntryLo0 | theTLB->EntryLo1)) == 0) continue; £
		 * compare upper bits £
		 * if ((address & theTLB->MyHiMask & 0x1FFFFFFF) == (theTLB->EntryHi &
		 * theTLB->MyHiMask & 0x1FFFFFFF))
		 */
		if((address & theTLB->MyHiMask) == (theTLB->EntryHi & theTLB->MyHiMask))
		{
			/* check the global bit */
			if
			(
				((0x01 & theTLB->EntryLo1 & theTLB->EntryLo0) == 1) /* g bit match */
			||	((theTLB->EntryHi & TLBHI_PIDMASK) == (gHWS_COP0Reg[ENTRYHI] & TLBHI_PIDMASK))
			)	/* ASID match */
			{
				/* select EntryLo depending on if we're in an even or odd page */
				if(address & theTLB->LoCompare)
					EntryLo = theTLB->EntryLo1;
				else
					EntryLo = theTLB->EntryLo0;

				if(EntryLo & 0x02)
				{
					/* calculate the real address from EntryLo */
					realAddress |= ((EntryLo << 6) & ((theTLB->MyHiMask) >> 1));
					realAddress |= (address & ((theTLB->PageMask | 0x00001FFF) >> 1));
					return realAddress;
				}
				else
				{
					continue;
				}
			}
		}
	}

	TLB_TRACE(TRACE1("Redo TLB check still fail, mapped to dummy memory section, addr=%08x", address)) if(operation == TLB_INST)
	{
		DisplayError
		(
			"TLBL Missing Exception for Instruction Access, Bad VPN2 = 0x%8X, Bad Virtual Address = 0x%08X",
			address >> 13,
			address
		);
	}

	return TLBMapErrorAddress(address);
}

/*
 =======================================================================================================================
    This is the main function to do mapped virtual address to physical address translation
 =======================================================================================================================
 */
uint32 TranslateTLBAddress(uint32 address, int operation)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	BOOL		invalidmatched;
	uint32		realAddress = 0x80000000;
	_int32		c;
	tlb_struct	*theTLB;
	uint32		EntryLo;
	int			maxtlb;
	int			*tlb_index_array;
	int			*lastused;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(operation == TLB_INST)
	{
		maxtlb = MAXITLB;
		tlb_index_array = ITLB_Index;
		lastused = &last_itlb_index;
	}
	else
	{
		maxtlb = MAXDTLB;
		tlb_index_array = DTLB_Index;
		lastused = &last_dtlb_index;
	}

	invalidmatched = FALSE;

	/* search the tlb entries */
	for(c = 0; c < MAXTLB + maxtlb; c++)
	/* for (c = 0; c < MAXTLB; c++) */
	{
		if(c < maxtlb) theTLB = &gMS_TLB[*(tlb_index_array + (maxtlb +*lastused - c) % maxtlb)];	/* Check micro TLB
																									 * first */
		else
			theTLB = &gMS_TLB[c - maxtlb];

		/*
		 * skip unused entries £
		 * if (theTLB->valid == 0) continue; £
		 * if ( ((theTLB->EntryLo0 | theTLB->EntryLo1) & TLBLO_V ) == 0) continue; // Skip
		 * this entry if both EntroLo0 and EntryLo1 are invalid £
		 * compare upper bits £
		 * This is a unreasonable hack, I should not use this 0x1FFFFFFF mask, but £
		 * I cannot find the bugs in dyna, without this mask, many games will not work
		 * well £
		 * games like Snowboard Kids 2 £
		 * if ((address & theTLB->MyHiMask &0x1FFFFFFF ) == (theTLB->EntryHi &
		 * theTLB->MyHiMask & 0x1FFFFFFF))
		 */
		if((address & theTLB->MyHiMask) == (theTLB->EntryHi & theTLB->MyHiMask))					/* match, this line
																									 * is correct */
		{
			/* check the global bit */
			if
			(
				((0x01 & theTLB->EntryLo1 & theTLB->EntryLo0) == 1) /* g bit match */
			||	((theTLB->EntryHi & TLBHI_PIDMASK) == (gHWS_COP0Reg[ENTRYHI] & TLBHI_PIDMASK))
			)						/* ASID match */
			{
				/* select EntryLo depending on if we're in an even or odd page */
				if(address & theTLB->LoCompare)
					EntryLo = theTLB->EntryLo1;
				else
					EntryLo = theTLB->EntryLo0;

				if(EntryLo & 0x02)	/* Is the entry valid ?? */
				{
					/* calculate the real address from EntryLo */
					realAddress |= ((EntryLo << 6) & ((theTLB->MyHiMask) >> 1));
					realAddress |= (address & ((theTLB->PageMask | 0x00001FFF) >> 1));
					Direct_TLB_Lookup_Table[address / 0x1000] = (realAddress & 0xFFFFF000);

					/* TLB_sDWORD_R[address/0x1000] = sDWord[realAddress>>16]+(realAddress&0x0000F000); */
					TLB_sDWORD_R[address / 0x1000] = TLB_sDWORD_R[realAddress / 0x1000];

					TLB_DETAIL_TRACE
					(
						TRACE2
							(
								"Direct TLB Map: VA=%08X, PA=%08X",
								(address & 0xFFFFF000),
								(realAddress & 0xFFFFF000)
							)
					) if(c >= maxtlb)
					{
						/* Set the microTLB */
						*lastused = (*lastused + 1) % maxtlb;
						*(tlb_index_array +*lastused) = c - maxtlb;
						TLB_DETAIL_TRACE
						(
							TRACE3
								(
									"%s remapped to TLB at %d, vaddr=%08x",
									operation == TLB_INST ? "ITLB" : "DTLB",
									c - maxtlb,
									address
								)
						)
					}
					else
					{
						*lastused = c;
					}

					return realAddress;
				}
				else
				{
					invalidmatched = TRUE;
					continue;
				}
			}
		}
	}

	/*
	 * hack for golden eye £
	 * if( strncmp(currentromoptions.Game_Name, "GOLDENEYE", 9) == 0 &&
	 * (address&0xFF000000) == 0x7F000000 ) { if( rominfo.TV_System == TV_SYSTEM_NTSC
	 * ) { uint32 i; for( i=0; i<(gAllocationLength - 0x34b30)/0x1000; i++) {
	 * Direct_TLB_Lookup_Table[0x7f000+i] = 0x90034b30+i*0x1000;
	 * TLB_sDWORD_R[0x7f000+i] = &gMS_ROM_Image[0x34b30+i*0x1000]; } realAddress
	 * (address - 0x7f000000) + 0x90034b30; } else { uint32 i; for( i=0;
	 * i<(gAllocationLength - 0x329f0)/0x1000; i++) {
	 * Direct_TLB_Lookup_Table[0x7f000+i] = 0x900329f0+i*0x1000;
	 * TLB_sDWORD_R[0x7f000+i] = &gMS_ROM_Image[0x329f0+i*0x1000]; } realAddress
	 * (address - 0x7f000000) + 0x900329f0; } TLB_DETAIL_TRACE(TRACE2("Hack, mapping
	 * for GoldenEye: %08X ==> %08X", address, realAddress)); return realAddress; }
	 */
	if(invalidmatched)
		return(Trigger_TLB_Invalid_Exception(address, operation));
	else
		/* Fire TLB refill exception here */
		return(Trigger_TLB_Refill_Exception(address, operation));
}

extern uint32	TLB_Error_Vector;

void __cdecl error(char *Message, ...)
{
	/*~~~~~~~~~~~~~*/
	char	Msg[400];
	va_list ap;
	/*~~~~~~~~~~~~~*/

	va_start(ap, Message);
	vsprintf(Msg, Message, ap);
	va_end(ap);

	MessageBox(NULL, Msg, "Error", MB_OK | MB_ICONINFORMATION);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */

uint32 Trigger_TLB_Refill_Exception(uint32 address, int operation)
{
	/*~~~~~~~~~*/
	uint32	addr;
	/*~~~~~~~~~*/

#ifdef DEBUG_TLB
	if(debugoptions.debug_tlb)
	{
		if(operation == TLB_LOAD)
		{
			TRACE2("0x%08X: TLBL Refill Exception, Bad Virtual Address = 0x%08X", gHWS_pc, address)
		}
		else if(operation == TLB_STORE)
		{
			TRACE2("0x%08X: TLBS Refill Exception, Bad Virtual Address = 0x%08X", gHWS_pc, address)
		}
		else
		{
			TRACE2("0x%08X: TLB Refill Exception for Instruction, VA=%08X", gHWS_pc, address)
		}
	}
#endif
	gHWS_COP0Reg[BADVADDR] = address;

	gHWS_COP0Reg[CONTEXT] &= 0xFF800000;	/* Mask off bottom 23 bits */
	gHWS_COP0Reg[CONTEXT] |= ((address >> 13) << 4);

	gHWS_COP0Reg[ENTRYHI] &= 0x00001FFF;	/* Mask off the top bit 13-31 */
	gHWS_COP0Reg[ENTRYHI] |= (address & 0xFFFFE000);

	if(operation == TLB_STORE)
		SET_EXCEPTION(TLBS_Miss) else
			SET_EXCEPTION(TLBL_Miss) newtlb = FALSE;

	/*
	 * Test the TLB refill service vector £
	 * Hack for Turok: Rage War and for NFL QBC: 2000, donno why I need this £
	 * For the TLB service vector here, it is tricky. £
	 * I should use 0x80000000 to serve TLB_Refill for 32bit TLB, and use 0x80000080
	 * to serve £
	 * 64bit TLB. I believe I should use 0x80000000 for most of games, £
	 * Somehow, Golden Eye only work if I use 0x80000000, while Turok 2 only work if I
	 * use £
	 * 0x80000080, do I need to check the status register to decide the vector? £
	 * if( TLB_Refill_Exception_Vector != 0x80000000 )
	 */
	{
		for(addr = 0x80000000; addr < 0x8000000C; addr += 4)
		{
			if(MEM_READ_UWORD(addr) != MEM_READ_UWORD(addr + 0x80))
			{
				if(TLB_Refill_Exception_Vector == 0x80000080)
				{
					TRACE0("");
					TRACE0("Warning, Set TLB exception vector to 0x80000000");
					TRACE0("");
					TLB_Refill_Exception_Vector = 0x80000000;
				}

				goto step2;
			}
		}

		if(TLB_Refill_Exception_Vector == 0x80000000)
		{
			TRACE0("");
			TRACE0("Warning, Set TLB exception vector to 0x80000080");
			TRACE0("");
			TLB_Refill_Exception_Vector = 0x80000080;
		}
	}

step2:
	if((gHWS_COP0Reg[STATUS] & 0xE0) != 0)
	{
		TRACE0("To trigger TLB refill exception, SX/UX/KX != 0");
		DisplayError("To trigger TLB refill exception, SX/UX/KX != 0");
	}

	if(operation == TLB_INST)
	{					/* here we have got a TLB error at compiling, need to invalidate the compiled block
						 * £
						 * and serve the TLB error */
		ITLB_Error = TRUE;
		TLB_Error_Vector = TLB_Refill_Exception_Vector;
		return address;
	}
	else
	{
		TLB_Error_Vector = TLB_Refill_Exception_Vector;
        HandleExceptions(TLB_Error_Vector);
		if(emustatus.cpucore == DYNACOMPILER)
		{
			if(newtlb)	/* if a new TLB entry is written in the TLB exception service routine */
			{
				/* Could make infinite loop, be careful here */
				return(TranslateTLBAddress(address, operation));
			}
			else
			{
				//return(address);
				/* Redo TLB lookup, of course, redo will still fail, need a little optimization here */
				return(RedoTLBLookupAfterException(address, operation));
			}
		}
		else
		{
			TRACE1("TLB Refill for load/store, vaddr=%08X", address);
			return(0x80800000); /* return an address to guarante an invalid address */
			/* so store opcode will not overwite any valid address */
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 Trigger_TLB_Invalid_Exception(uint32 address, int operation)
{
#ifdef DEBUG_TLB
	if(debugoptions.debug_tlb)
	{
		if(operation == TLB_LOAD)
		{
			TRACE2("0x%08X: TLBL Invalid Exception, Bad Virtual Address = 0x%08X", gHWS_pc, address)
		}
		else if(operation == TLB_STORE)
		{
			TRACE2("0x%08X: TLBS Invalid Exception, Bad Virtual Address = 0x%08X", gHWS_pc, address)
		}
		else
		{
			TRACE2("0x%08X: TLB Invalid Exception for Instruction, VA=%08X", gHWS_pc, address)
		}

		TRACE1("Bad Virtual Address = 0x%08X", address);
	}
#endif
	gHWS_COP0Reg[BADVADDR] = address;

	gHWS_COP0Reg[CONTEXT] &= 0xFF800000;	/* Mask off bottom 23 bits */
	gHWS_COP0Reg[CONTEXT] |= ((address >> 13) << 4);

	gHWS_COP0Reg[ENTRYHI] &= 0x00001FFF;	/* Mask off the top bit 13-31 */
	gHWS_COP0Reg[ENTRYHI] |= (address & 0xFFFFE000);

	if(operation == TLB_STORE)
		SET_EXCEPTION(TLBS_Miss) else
			SET_EXCEPTION(TLBL_Miss) newtlb = FALSE;

	if(operation == TLB_INST)
	{					/* here we have got a TLB error at compiling, need to invalidate the compiled block
						 * £
						 * and serve the TLB error */
		ITLB_Error = TRUE;
		TLB_Error_Vector = 0x80000180;

		/* TLB_Error_Vector = 0x80000000; */
		return address;
	}
	else
	{
		HandleExceptions(0x80000180);
		TLB_Error_Vector = 0x80000180;

		if(emustatus.cpucore == DYNACOMPILER)
		{
			if(newtlb)	/* if a new TLB entry is written in the TLB exception service routine */
			{
				/* Could make infinite loop, be careful here */
				return(TranslateTLBAddress(address, operation));
			}
			else
			{
				/* Redo TLB lookup, of course, redo will still fail, need a little optimize here */
				return(RedoTLBLookupAfterException(address, operation));
			}
		}
		else
		{
			TRACE1("TLB invalid for load/store, vaddr=%08X", address);
			return(0x80800000); /* return a address to garante a invalid address */
			/* so store opcode will not overwite any valid address */
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitTLBOther(void);

void InitTLB(void)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < MAXTLB; i++)
	{
		memset(&gMemoryState.TLB[i], 0, sizeof(tlb_struct));
	}

	last_itlb_index = 0;
	for(i = 0; i < MAXITLB; i++)
	{
		ITLB_Index[i] = 0;
	}

	last_dtlb_index = 0;
	for(i = 0; i < MAXDTLB; i++)
	{
		DTLB_Index[i] = 0;
	}

	/* Initialize the TLB Lookup Table */
	for(i = 0; i < 0x100000; i++)
	{
		Direct_TLB_Lookup_Table[i] = DUMMYDIRECTTLBVALUE; 
		/* This is a magic number, to represent invalid TLB address */
	}

	/* we probably need to review a little bit here later */
	for(i = 0; i < 0x100000; i++)
	{
		TLB_sDWORD_R[i] = sDWord[i >> 4] + 0x1000 * (i & 0xf);
	}

	InitTLBOther();
}

void InitTLBOther(void)
{
	/* some TLB hacks to speed up some games, but fails at others */
	trigger_tlb_exception_faster = FALSE;
	if(strncmp(currentromoptions.Game_Name, "GOLDENEYE", 9) == 0 || strstr(currentromoptions.Game_Name, "GOLD") != NULL)
	{
		/* Hack for golden eye, game still work without hack, but will be faster with hack */
		if(rominfo.TV_System == TV_SYSTEM_NTSC)
		{
			/*~~~~~~*/
			uint32	i;
			/*~~~~~~*/

			for(i = 0; i < (gAllocationLength - 0x34b30) / 0x1000 && i < 0xFCB; i++)
			{
				Direct_TLB_Lookup_Table[0x7f000 + i] = 0x90034b30 + i * 0x1000;
				TLB_sDWORD_R[0x7f000 + i] = &gMS_ROM_Image[0x34b30 + i * 0x1000];
			}
		}
		else
		{
			/*~~~~~~*/
			uint32	i;
			/*~~~~~~*/

			for(i = 0; i < (gAllocationLength - 0x329f0) / 0x1000; i++)
			{
				Direct_TLB_Lookup_Table[0x7f000 + i] = 0x900329f0 + i * 0x1000;
				TLB_sDWORD_R[0x7f000 + i] = &gMS_ROM_Image[0x329f0 + i * 0x1000];
			}
		}
		trigger_tlb_exception_faster = TRUE;
	}
	else if(strncmp(currentromoptions.Game_Name, "CONKER", 6) == 0)
	{
		trigger_tlb_exception_faster = TRUE;
	}
}

/*
 =======================================================================================================================
    This function will try best to map a TLB error virtual address to its real address
 =======================================================================================================================
 */
uint32 TLBMapErrorAddress(uint32 address)
{
	/*~~~~~~~~~~~~~~*/
	/* Check if the address is in any readable memory region */
	uint32	dummyword;
	/*~~~~~~~~~~~~~~*/

	__try
	{
		dummyword = LOAD_UWORD_PARAM(address);
		return((address & 0x1fffffff) | 0x80000000);
	}

	__except(NULL, EXCEPTION_EXECUTE_HANDLER)
	{
		__try
		{
			dummyword = LOAD_UWORD_PARAM_2(address);
			return(address & 0x1fffffff);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			return((address & 0x0000FFFF + 0x1FFF0000) | 0x80000000);	/* mapped to dummy segment */
		}
	}
}

/*
 =======================================================================================================================
    ERET - Return from Exception. ERET is the R4300 instruction for returning from an interrupt, exception, or error
    trap. Unlike a branch or jump instruction, ERET does not execute the next instruction.
 =======================================================================================================================
 */
void r4300i_COP0_eret(uint32 Instruction)
{
	if((gHWS_COP0Reg[STATUS] & 0x00000004))
	{
		CPUdelayPC = gHWS_COP0Reg[ERROREPC];	/* dynarec use: pc = COP0Reg[ERROREPC]; */

		/* clear the ERL bit to zero */
		gHWS_COP0Reg[STATUS] &= 0xFFFFFFFB;		/* 0xFFFFFFFB same as ~0x00000004; */
	}
	else
	{
		CPUdelayPC = gHWS_COP0Reg[EPC];			/* dynarec use: pc = COP0Reg[EPC]; */

		/* clear the EXL bit of the status register to zero */
		gHWS_COP0Reg[STATUS] &= 0xFFFFFFFD;		/* 0xFFFFFFFD same as ~0x00000002 */
	}

	CPUdelay = 2;	/* not used in dynarec */
	gHWS_LLbit = 0;
}

/*
 =======================================================================================================================
    TRAPS
 =======================================================================================================================
 */
void r4300i_teqi(uint32 Instruction)
{
	DisplayError("In Trap TEQI");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_tgei(uint32 Instruction)
{
	DisplayError("In Trap r4300i_tgei");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_tgeiu(uint32 Instruction)
{
	DisplayError("In Trap r4300i_tgeiu");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_tlti(uint32 Instruction)
{
	DisplayError("In Trap r4300i_tlti");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_tltiu(uint32 Instruction)
{
	DisplayError("In Trap r4300i_tltiu");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_tnei(uint32 Instruction)
{
	DisplayError("In Trap r4300i_tnei");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_teq(uint32 Instruction)
{
	if(gRS == gRT)
	{
#ifdef DEBUG_TRAP
		if(debugoptions.debug_trap) TRACE0("Trap r4300i_teq");
#endif
		SET_EXCEPTION(EXC_TRAP) HandleExceptions(0x80000180);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_tge(uint32 Instruction)
{
	/* DisplayError("In Trap r4300i_tge"); */
	if(gRS >= gRT)
	{
#ifdef DEBUG_TRAP
		if(debugoptions.debug_trap) TRACE0("Trap r4300i_tge");
#endif
		SET_EXCEPTION(EXC_TRAP) HandleExceptions(0x80000180);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_tgeu(uint32 Instruction)
{
	DisplayError("In Trap r4300i_tgeu");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_tlt(uint32 Instruction)
{
	DisplayError("In Trap r4300i_tlt");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_tltu(uint32 Instruction)
{
	DisplayError("In Trap r4300i_tltu");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_tne(uint32 Instruction)
{
	/* DisplayError("In Trap r4300i_tne"); */
	if(gRS != gRT)
	{
#ifdef DEBUG_TRAP
		if(debugoptions.debug_trap) TRACE0("Trap r4300i_tne");
#endif
		SET_EXCEPTION(EXC_TRAP) HandleExceptions(0x80000180);
	}
}

/*
 =======================================================================================================================
    Instruction cache size 16kb. Consideration for this cache opcode: Code cache=I cache=D What to do 0 Set Invalide
    Nothing Write back Note 1 1 Load Index Nothing 2 Store Index Nothing 3 Create Dirty Exclusive Nothing 4 Hit Invalid
    Nothing 5 fill Nothing 5 Write back Note 1 6 Write back Note 1 7 Nothing Note 1: Need to invalidate the compiled
    block
 =======================================================================================================================
 */
void r4300i_cache(uint32 Instruction)
{
	/*
	 * #ifdef CHECK_CACHE if( currentromoptions.Code_Check !=
	 * CODE_CHECK_PROTECT_MEMORY ) { uint32 op = RT_FT; int code = op/4; int cache
	 * op&1; int size = 0x10; // size for data cache STORE_TLB_FUN; QuerAddr &=
	 * 0xFFFFFFF0; if( cache == 0 ) //Instruction cache { size = 0x20; QuerAddr &=
	 * 0xFFFFFFE0; } //QuerAddr &= 0xFFFFC000; // align it to 16kb blocks, ?? if(
	 * QuerAddr >= 0x80000000 && QuerAddr < 0x80000000+current_rdram_size ) //if(
	 * QuerAddr >= 0x80004000 && QuerAddr < 0x80000000+current_rdram_size ) { if( code
	 * == 6 || ((code == 0 || code == 5) && cache == 1) ) { uint32 pc;
	 * DEBUG_CACHE_TRACE(TRACE3("Cache, code=%d, cache=%d, addr=%08x", op/4, (op&3),
	 * QuerAddr)); for(pc= QuerAddr; pc<QuerAddr+size; pc+=4) { if(
	 * (uint32*)(RDRAM_Copy+(pc&0x007FFFFF)) != *(uint32*)(gMS_RDRAM+(pc&0x007FFFFF))
	 * ) { if( sDYN_PC_LOOKUP[pc>>16] != gMemoryState.dummyAllZero )
	 * (uint32*)((uint8*)sDYN_PC_LOOKUP[pc>>16] + (uint16)pc) = 0; else break; } } } }
	 * } #endif
	 */
}

/*
 =======================================================================================================================
    This function will translate a virtual address to physical address by looking up in the TLB table
 =======================================================================================================================
 */
uint32 Direct_TLB_Lookup(uint32 address, int operation)
{
	if(currentromoptions.Use_TLB != USETLB_YES)
	{
		TLB_TRACE(TRACE1("Warning, Need to use TLB-Store, Vaddr=%08X", address)) return
			((address & 0x1FFFFFFF) | 0x80000000);
	}
	else
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint32	realAddress = Direct_TLB_Lookup_Table[address >> 12];
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		if(ISVALIDDIRECTTLBVALUE(realAddress))
		{
			return(realAddress + (address & 0x00000FFF));
		}
		else if(trigger_tlb_exception_faster)
		{
			/*
			 * if( realAddress == DUMMYDIRECTTLBVALUE ) £
			 * {
			 */
			return Trigger_TLB_Refill_Exception(address, operation);

			/*
			 * } £
			 * else £
			 * { £
			 * return Trigger_TLB_Invalid_Exception(address, operation); £
			 * }
			 */
		}
		else
			return TranslateTLBAddress(address, operation);
	}
}

enum { BUILDMAP, CLEARMAP, FORCECLEARMAP };

/*
 =======================================================================================================================
    This function is called when a new TLB entry is written. Be careful, this function should be called before the new
    TLB entry is written from register into selected TLB entry table, not after the TLB entry is written. The reason is
    because we need the old TLB entry and compare it to the new TLB entry content which is still in the registers.
    Input parameter "index" is the TLB entry which is to be written.
 =======================================================================================================================
 */
void Refresh_Direct_TLB_Lookup_Table(int index)
{
	/*
	 * This function need optimized otherwise it will be slow, I hope games are using
	 * TLB, but not £
	 * change TLB all the time. At least games should not change the TLB in used all
	 * the time while £
	 * leave the TLB entries not in used untouched forever. £
	 * But I know some games are just constantly used only a few TLB £
	 * TLB_TRACE(TRACE0("Refresh Direct TLB Lookup Table"))
	 */
	if(index > NTLBENTRIES)
	{
		DisplayError("ERROR: tlbwi called with invalid index"); /* This should never happen */
		return;
	}
	else
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		tlb_struct	*theTLB = &gMS_TLB[index];
		int			old_g = (gHWS_COP0Reg[ENTRYLO0] & gHWS_COP0Reg[ENTRYLO1]) & 0x01;
		int			old_valid = theTLB->valid;
		int			new_valid = ((gHWS_COP0Reg[ENTRYLO1] & TLBLO_V) || (gHWS_COP0Reg[ENTRYLO0] & TLBLO_V));
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		/* case1: If the new TLB is invalid and the old TLB is also invalid, do nothing */
		if(old_valid == 0 && new_valid == 0)
		{
			tlb_write_entry(index);
			return;
		}

		/*
		 * case2: Compare the new TLB and the old TLB content, if they are the same, do
		 * nothing
		 */
		tlb_write_entry(DUMMYTLBINDEX);
		if(memcmp(&dummy_tlb, theTLB, sizeof(tlb_struct)) == 0)
		{
			tlb_write_entry(index);
			return;
		}

		/*
		 * case3: If the old TLB is invalid, and the new TLB is valid, do something,
		 * without checking £
		 * other TLB entries
		 */
		if(old_valid == 0 /* And the new is valid */ )
		{
			tlb_write_entry(index); /* write the new entry */
			Build_Direct_TLB_Lookup_Table(index, BUILDMAP);			/* Clear the old map */

			return;
		}

		/*
		 * case4: If the old TLB is valid and new TLB is invalid, do something, need to be
		 * careful
		 */
		if(new_valid == 0)
		{
			Build_Direct_TLB_Lookup_Table(index, FORCECLEARMAP);	/* Clear the old map */
			tlb_write_entry(index);						/* write the new entry */
			return;
		}

		/*
		 * case5; If the old TLB is valid and new TLB is also valid, do something, need to
		 * be careful
		 */
		Build_Direct_TLB_Lookup_Table(index, CLEARMAP); /* Clear the old map */
		tlb_write_entry(index); /* write the new entry */
		Build_Direct_TLB_Lookup_Table(index, BUILDMAP); /* Clear the old map */

		return;

		/*
		 * For the case 4 and case 5, the easiest way is to write the new content and
		 * refresh the whole £
		 * TLB lookup table, this will be a little slower £
		 * If I can do better, I prefer to exam the conflict TLB entries, and modify the
		 * TLB Lookup £
		 * table only if needed £
		 * here is the easiest solution £
		 * 1. Write new TLB into TLB table (not Direct TLB Lookup Table) £
		 * tlb_write_entry(index); £
		 * 2. Refresh the whole Direct TLB Lookup Table £
		 * Build_Whole_Direct_TLB_Lookup_Table();
		 */
	}
}

/*
 =======================================================================================================================
    Need to be very careful in this function, this function is not good yet. Two parameter: index -> this TLB entry
    index to be built. tobuild = yes/no yes=to build no=to clear
 =======================================================================================================================
 */
void Build_Direct_TLB_Lookup_Table(int index, BOOL tobuild)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32		lowest, highest, middle, realAddress, i;
	uint32		loop_optimizer;
	tlb_struct	*theTLB = &gMS_TLB[index];
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(theTLB->valid == 0) return;

	if((theTLB->EntryLo0 & theTLB->EntryLo1 & 1) == 0)	/* g bit == 0 */
	{
		return; /* should check teh ASID field, but we cannot support ASID in direct TLB lookup */
	}

	/* Step 1: To calculate the mapped address range that this TLB entry is mapping */
	lowest = (theTLB->EntryHi & 0xFFFFFF00);	/* Don't support ASID field */
	middle = lowest + theTLB->LoCompare;
	highest = lowest + theTLB->LoCompare * 2;

	/* Step 2: Check the EntryLo1 */
	if(theTLB->EntryLo0 & 0x2)					/* is the TLB EntryLo is valid? */
	{
		realAddress = 0x80000000;

		/*
		 * realAddress |= ((EntryLo << 6) & ((theTLB->MyHiMask) >> 1)); £
		 * realAddress |= (address & ((theTLB->PageMask | 0x00001FFF) >> 1));
		 */
		realAddress |= ((theTLB->EntryLo0 << 6) & ((theTLB->MyHiMask) >> 1));
#ifdef DEBUG_COMMON
		if(debugoptions.debug_tlb_detail)
		{
			if(tobuild == BUILDMAP)
			{
				sprintf
				(
					tracemessage,
					"Direct TLB Map: entry=%d-0, VA=%08X-%08X, PA=%08X-%08X",
					index,
					lowest,
					lowest + theTLB->LoCompare - 1,
					realAddress,
					realAddress + theTLB->LoCompare - 1
				);
			}
			else
			{
				sprintf
				(
					tracemessage,
					"Direct TLB UnMap: entry=%d-0, VA=%08X-%08X, PA=%08X-%08X",
					index,
					lowest,
					lowest + theTLB->LoCompare - 1,
					realAddress,
					realAddress + theTLB->LoCompare - 1
				);
			}

			RefreshOpList(tracemessage);
		}
#endif
		loop_optimizer = middle / 0x1000;
		for(i = lowest / 0x1000; i < loop_optimizer; i++)
		{
			if(tobuild == BUILDMAP)
			{
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
				/*
				 * TLB_TRACE(TRACE2("Direct TLB Map: VA=%08X, PA=%08X", i*0x1000,
				 * realAddress+i*0x1000-lowest))
				 */
				uint32	real = realAddress + i * 0x1000 - lowest;
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

				Direct_TLB_Lookup_Table[i] = real | 0x80000000;

				/* TLB_sDWORD_R[i] = sDWord[real>>16]+(real&0x0000F000); */
				TLB_sDWORD_R[i] = TLB_sDWORD_R[real / 0x1000];
			}
			else
			{
				/*
				 * TLB_TRACE(TRACE2("Direct TLB UnMap: VA=%08X, OldPA=%08X", i*0x1000,
				 * realAddress+i*0x1000-lowest))
				 */
				Direct_TLB_Lookup_Table[i] = DUMMYDIRECTTLBVALUE;
				{
					TLB_sDWORD_R[i] = sDWord[i >> 4] + 0x1000 * (i & 0xf);
				}

				if(sDYN_PC_LOOKUP[i >> 4] != gMemoryState.dummyAllZero)
				{
					memset(((uint8 *) sDYN_PC_LOOKUP[i >> 4] + (uint16) (i * 0x1000)), 0, 0x1000);
				}
			}
		}
	}

	/* Step 3: Check the EntryLo0 */
	if(theTLB->EntryLo1 & 0x2)					/* is the TLB EntryLo is valid? */
	{
		realAddress = 0x80000000;
		realAddress |= ((theTLB->EntryLo1 << 6) & ((theTLB->MyHiMask) >> 1));
#ifdef DEBUG_COMMON
		if(debugoptions.debug_tlb_detail)
		{
			if(tobuild == BUILDMAP)
			{
				sprintf
				(
					tracemessage,
					"Direct TLB Map: entry=%d-1, VA=%08X-%08X, PA=%08X-%08X",
					index,
					middle,
					middle + theTLB->LoCompare - 1,
					realAddress,
					realAddress + theTLB->LoCompare - 1
				);
			}
			else
			{
				sprintf
				(
					tracemessage,
					"Direct TLB UnMap: entry=%d-1, VA=%08X-%08X, PA=%08X-%08X",
					index,
					middle,
					middle + theTLB->LoCompare - 1,
					realAddress,
					realAddress + theTLB->LoCompare - 1
				);
			}

			RefreshOpList(tracemessage);
		}
#endif
		loop_optimizer = highest / 0x1000;
		for(i = middle / 0x1000; i < loop_optimizer; i++)
		{
			if(tobuild == BUILDMAP)
			{
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
				/*
				 * TLB_TRACE(TRACE2("Direct TLB Map: VA=%08X, PA=%08X", i*0x1000,
				 * realAddress+i*0x1000-middle))
				 */
				uint32	real = realAddress + i * 0x1000 - middle;
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

				Direct_TLB_Lookup_Table[i] = real | 0x80000000;

				/* TLB_sDWORD_R[i] = sDWord[real>>16]+(real&0x0000F000); */
				TLB_sDWORD_R[i] = TLB_sDWORD_R[real / 0x1000];
			}
			else
			{
				/*
				 * TLB_TRACE(TRACE2("Direct TLB UnMap: VA=%08X, OldPA=%08X", i*0x1000,
				 * realAddress+i*0x1000-middle))
				 */
				Direct_TLB_Lookup_Table[i] = DUMMYDIRECTTLBVALUE;
				{
					TLB_sDWORD_R[i] = sDWord[i >> 4] + 0x1000 * (i & 0xf);
				}

				if(sDYN_PC_LOOKUP[i >> 4] != gMemoryState.dummyAllZero)
				{
					memset(((uint8 *) sDYN_PC_LOOKUP[i >> 4] + (uint16) (i * 0x1000)), 0, 0x1000);
				}
			}
		}
	}

	/*
	 * Remember, step 2 and step 3 is the most important part, if there are error,
	 * must be in here
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Build_Whole_Direct_TLB_Lookup_Table(void)
{
	/*~~*/
	int i;
	/*~~*/

	/*
	 * Clean the whole table first £
	 * memset(&Direct_TLB_Lookup_Table[0], 0xFF, sizeof(Direct_TLB_Lookup_Table));
	 */
	for(i = 0; i < 0x100000; i++)
	{
		Direct_TLB_Lookup_Table[i] = DUMMYDIRECTTLBVALUE; 
		/* This is a magic number, to represent invalid TLB address */
	}

	for(i = 0; i < 0x100000; i++)
	{
		TLB_sDWORD_R[i] = sDWord[i >> 4] + 0x1000 * (i & 0xf);
	}

	for(i = 0; i <= NTLBENTRIES; i++)
	{
		Build_Direct_TLB_Lookup_Table(i, BUILDMAP);
	}

	InitTLBOther();
}
