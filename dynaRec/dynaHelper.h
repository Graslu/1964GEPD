/*$T dynaHelper.h GC 1.136 02/28/02 08:30:59 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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
 * authors: email: schibo@emulation64.com, rice1964@yahoo.com £
 * define __SPEED_HACK
 */
#include "../timer.h"
#include "../memory.h"
#include "../1964ini.h"
#include "../compiler.h"

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void HELP_Call(unsigned long function)
{
	FlushAllRegisters();
	MOV_ImmToReg(1, Reg_ECX, gHWS_pc);
	X86_CALL(function);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DoSpeedHack(void)
{
	if(Is_CPU_Doing_Other_Tasks()) return;

	//Count_Down_All
	countdown_counter = 0;
}

extern void TriggerFPUUnusableException(void);
void		COP1_instr(uint32 Instruction);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Do_COP1_with_exception(uint32 Instruction)
{
	if((gHWS_COP0Reg[STATUS] & STATUS_CU1) == 0)	/* CPU1 is not usable */
	{
		TriggerFPUUnusableException();
		COP1_instr(Instruction);
	}
	else
	{
		COP1_instr(Instruction);

		/* dyna_instruction[0x11] = dyna4300i_cop1; */
	}
}

extern uint32	*g_LookupPtr;
extern uint32	g_pc_is_rdram;

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void Set_Translate_PC(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/
	register uint32 translatepc;
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(NOT_IN_KO_K1_SEG(gHWS_pc))
	{
		translatepc = TranslateITLBAddress(gHWS_pc);
		if(ITLB_Error)
		{
			translatepc = gHWS_pc;
			g_LookupPtr = (uint32 *) (gMemoryState.dummyAllZero);
			return;
		}
	}
	else
		translatepc = gHWS_pc;

	if((translatepc & 0x1FFFFFFF) < current_rdram_size)
		g_pc_is_rdram = translatepc & 0x007FFFFF;
	else
		g_pc_is_rdram = 0;

	g_LookupPtr = (uint32 *) ((uint8 *) sDYN_PC_LOOKUP[translatepc >> 16] + (uint16) translatepc);
}

uint32	*test;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_Translate_PC_No_Check(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
	register uint32 pc = gHWS_pc;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
	{
		/*~~~~~~~~~~~~~~~~*/
		register uint32 tpc;
		/*~~~~~~~~~~~~~~~~*/

		if(NOT_IN_KO_K1_SEG(pc))
		{
			tpc = Direct_TLB_Lookup_Table[pc >> 12];
			if(ISNOTVALIDDIRECTTLBVALUE(tpc))
			{
				pc = TranslateTLBAddress(pc, TLB_INST);
				if(ITLB_Error)
				{
					g_LookupPtr = (uint32 *) (gMemoryState.dummyAllZero);
					return;
				}
			}
			else
				pc = tpc + (pc & 0x00000FFF);
			g_LookupPtr = (uint32 *) ((uint8 *) sDYN_PC_LOOKUP[pc >> 16] + (uint16) pc);
		}
		else
			g_LookupPtr = (uint32 *) ((uint8 *) sDYN_PC_LOOKUP[pc >> 16] + (uint16) pc);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_Translate_PC_No_TLB(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/
	register uint32 translatepc;
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/

	translatepc = gHWS_pc;

	if((translatepc & 0x1FFFFFFF) < current_rdram_size)
		g_pc_is_rdram = translatepc & 0x007FFFFF;
	else
		g_pc_is_rdram = 0;

	g_LookupPtr = (uint32 *) ((uint8 *) sDYN_PC_LOOKUP[translatepc >> 16] + (uint16) translatepc);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_Translate_PC_No_TLB_No_Check(void)
{
	g_LookupPtr = (uint32 *) ((uint8 *) sDYN_PC_LOOKUP[gHWS_pc >> 16] + (uint16) gHWS_pc);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void TLB_TRANSLATE_PC_INDIRECT(void)
{
	/* PushMap(); */
	if(currentromoptions.Use_TLB != USETLB_YES)
	{
		if((uint32) (*Dyna_Check_Codes) == (uint32) Dyna_Code_Check_None)
			X86_CALL((_u32) Set_Translate_PC_No_TLB_No_Check);
		else
			X86_CALL((_u32) Set_Translate_PC_No_TLB);
	}
	else
	{
		if((uint32) (*Dyna_Check_Codes) == (uint32) Dyna_Code_Check_None)
			X86_CALL((_u32) Set_Translate_PC_No_Check);
		else
			X86_CALL((_u32) Set_Translate_PC);
	}

	/* PopMap(); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void TLB_TRANSLATE_PC(uint32 pc)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/
	register uint32 translatepc;
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(NOT_IN_KO_K1_SEG(pc))
	{
		TLB_TRANSLATE_PC_INDIRECT();
	}
	else
	{
		translatepc = pc;
		if((translatepc & 0x1FFFFFFF) < current_rdram_size)
			g_pc_is_rdram = translatepc & 0x007FFFFF;
		else
			g_pc_is_rdram = 0;
		if((uint32) (*Dyna_Check_Codes) != (uint32) Dyna_Code_Check_None)
			MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &g_pc_is_rdram, g_pc_is_rdram);
		if(sDYN_PC_LOOKUP[translatepc >> 16] == gMemoryState.dummyAllZero) UnmappedMemoryExceptionHelper(translatepc);
		g_LookupPtr = (uint32 *) ((uint8 *) sDYN_PC_LOOKUP[translatepc >> 16] + (uint16) translatepc);
		MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &g_LookupPtr, (uint32) g_LookupPtr);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DisplayLinkPC(void)
{
	TRACE1("Linked to PC=%08X", gHWS_pc);

	/* DisplayError("Linked to PC=%08X", gHWS_pc); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DisplayJumpIndirectPC(void)
{
	TRACE1("Exit block/Indirect to PC=%08X", gHWS_pc);

	/* DisplayError("Linked to PC=%08X", gHWS_pc); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DisplayOutof4kbPC(void)
{
	TRACE1("Exit block/Outof4kb PC=%08X", gHWS_pc);

	/* DisplayError("Linked to PC=%08X", gHWS_pc); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DisplayPC(void)
{
	/*~~~~~~~~~~~~~~~~~~~*/
	static uint32	pc = 0;
	/*~~~~~~~~~~~~~~~~~~~*/

	if(pc != gHWS_pc) TRACE1("PC=%08X", gHWS_pc);
	pc = gHWS_pc;

	/* DisplayError("PC=%08X", gHWS_pc); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DisplayJumpPC(void)
{
	TRACE1("Jump to PC=%08X", gHWS_pc);

	/* DisplayError("Jump to PC=%08X", gHWS_pc); */
}
