/*$T dynaBranch.h GC 1.136 03/09/02 13:36:14 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Jump and branch opcodes
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
#ifndef __DYNABRANCH_H
#define __DYNABRANCH_H

#include "../debug_option.h"
#include "../1964ini.h"
#include "../compiler.h"

/*
 * These are defines for branches that set GPR[31] to the link value. £
 * This does not refer to our 4k link :)
 */
#define LINK_YES			1
#define LINK_NO				0

#define SPEEDHACK_MISSING	DisplayError("Missing SpeedHack");
#define NEAR_BRANCHES		1
#ifdef NEAR_BRANCHES
#define SAFE_NEAR_ONLY	1
#else
#define SAFE_NEAR_ONLY	0
#endif
extern uint32	*g_LookupPtr;
extern uint32	g_pc_is_rdram;

enum { JUMP_TYPE_INDIRECT, JUMP_TYPE_DIRECT, JUMP_TYPE_BREAKOUT };

#define _OPCODE_DEBUG_BRANCH_(x) \
	if(debug_opcode!=0) \
	{ \
		MOV_ImmToMemory(1, ModRM_disp32, (uint32) & gHardwareState_Interpreter_Compare.pc, reg->pc); \
		_SAFTY_CPU_(x) \
	}

#define SET_TARGET_1_LINK(pc, target) \
	if(currentromoptions.Link_4KB_Blocks == USE4KBLINKBLOCK_YES) \
	{ \
		if(IsTargetPcInTheSame4KB(pc, target)) \
		{ \
			current_block_entry->need_target_1 = TRUE; \
			current_block_entry->target_1_pc = (target); \
		} \
	}

#define SET_TARGET_2_LINK(pc, target) \
	if(currentromoptions.Link_4KB_Blocks == USE4KBLINKBLOCK_YES) \
	{ \
		if(IsTargetPcInTheSame4KB(pc, target)) \
		{ \
			current_block_entry->need_target_2 = TRUE; \
			current_block_entry->target_2_pc = (target); \
		} \
	}

#define SPEED_HACK	if(__I == -1) \
	{ \
		if(compilerstatus.pcptr[1] == 0) \
		{ \
			PushMap(); \
			X86_CALL((_u32) DoSpeedHack); \
			PopMap(); \
		} \
	}

#define J_SPEED_HACK	if((reg->pc == aValue) && compilerstatus.pcptr[1] == 0) \
	{ \
		/* MOV_ImmToMemory(1, ModRM_disp32, (_u32)&gHWS_COP0Reg[COUNT], max_vi_count); */ \
		PushMap(); \
		X86_CALL((_u32) DoSpeedHack); \
		PopMap(); \
	}

extern MapConstant	ConstMap[NUM_CONSTS];
extern x86regtyp	x86reg[8];
MapConstant			TempConstMap[NUM_CONSTS];
x86regtyp			Tempx86reg[8];

MapConstant			TempConstMap_Debug[NUM_CONSTS];
x86regtyp			Tempx86reg_Debug[8];

BOOL				CompilingSlot = FALSE;
void				Interrupts(uint32 JumpType, uint32 targetpc, uint32 DoLink, uint32 LinkVal);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Compile_Slot_And_Interrupts(_u32 pc, uint32 targetpc, uint32 DoLink, uint32 LinkVal)
{
	/*~~~*/
	_u8 op;
	/*~~~*/

	if(CompilingSlot)
	{
		TRACE1("Warning, Branch in branch delay slot PC=%08X", gHWS_pc);

		/*
		 * DisplayError("Branch in branch delay slot PC=%08X", gHWS_pc); £
		 * return;
		 */
	}

	CompilingSlot = TRUE;

	/* this needs to be here, not below. */
	memcpy(TempConstMap, ConstMap, sizeof(ConstMap));
	memcpy(Tempx86reg, x86reg, sizeof(x86reg));

	/* Add by Rice. 2001-08-11 */
	if(currentromoptions.Use_Register_Caching == USEREGC_NO)
	{
		FlushAllRegisters();
	}

	LOGGING_DYNA(LogDyna("** Compile Delay Slot\n", pc));
	{
		/*~~~~~~~~~~~~~~~~~~~~~~*/
		uint32	savedpc = gHWS_pc;
		/*~~~~~~~~~~~~~~~~~~~~~~*/

		gHWS_pc = pc;

		/* TRACE1("Branch Slot fetch code at PC=%08X", pc); */
		gHWS_code = DynaFetchInstruction2(gHWS_pc);
		gHWS_pc = savedpc;
	}

	op = (_u8) (gHWS_code >> 26);

	dyna_instruction[op](&gHardwareState);

	CompilingSlot = FALSE;
	Interrupts(JUMP_TYPE_DIRECT, targetpc, DoLink, LinkVal);

	memcpy(ConstMap, TempConstMap, sizeof(ConstMap));
	memcpy(x86reg, Tempx86reg, sizeof(x86reg));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Compile_Slot_Jump(_u32 pc)
{
	/*~~~*/
	_u8 op;
	/*~~~*/

	if(CompilingSlot)
	{
		DisplayError("Branch in branch delay slot PC=%08X", gHWS_pc);
		TRACE1("Branch in branch delay slot PC=%08X", gHWS_pc);

		/* return; */
	}

	CompilingSlot = TRUE;

	/* Add by Rice. 2001-08-11 */
	if(currentromoptions.Use_Register_Caching == USEREGC_NO) FlushAllRegisters();

	LOGGING_DYNA(LogDyna("** Compile Delay Slot\n", pc));
	{
		/*~~~~~~~~~~~~~~~~~~~~~~*/
		uint32	savedpc = gHWS_pc;
		/*~~~~~~~~~~~~~~~~~~~~~~*/

		gHWS_pc = pc;

		/* TRACE1("Branch Slot fetch code at PC=%08X", pc); */
		gHWS_code = DynaFetchInstruction2(gHWS_pc);
		gHWS_pc = savedpc;
	}

	op = (_u8) (gHWS_code >> 26);
	dyna_instruction[op](&gHardwareState);
	FlushAllRegisters();
	CompilingSlot = FALSE;
}

extern __int32	countdown_counter;
extern uint8	*Block;
uint32			BlockStartAddress;

void			CompareStates(uint32 Instruction);
void			UnwireMap(void);

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void Interrupts(uint32 JumpType, uint32 targetpc, uint32 DoLink, uint32 LinkVal)
{
	/*~~~~~~~~~~~~~~*/
	uint32	viCounter;
	/*~~~~~~~~~~~~~~*/

	if(JumpType == JUMP_TYPE_BREAKOUT)
	{
		viCounter = (compilerstatus.cp0Counter * VICounterFactors[CounterFactor]);
	}
	else	/* taken branches have a 3 cycle penalty */
	{
		viCounter = (compilerstatus.cp0Counter * VICounterFactors[CounterFactor]);
	}

	compilerstatus.KEEP_RECOMPILING = FALSE;	/* always. */

	FlushAllRegisters();

	/* BugFix: can't store the link val to memory until GPR[31] has been flushed! */
	if(DoLink)
	{
		MOV_ImmToMemory(1, ModRM_disp8_EBP, (31 - 16) << 3, (_s32) LinkVal);
		if(currentromoptions.Assume_32bit != ASSUME_32BIT_YES)
			MOV_ImmToMemory(1, ModRM_disp8_EBP, 4 + ((31 - 16) << 3), (_s32) (((__int32) LinkVal) >> 31));
	}

#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY23
	if((debug_opcode!=0) && debug_opcode_block == 1)
	{
		MOV_ImmToReg(1, Reg_ECX, 0);
		MOV_ImmToReg(1, Reg_EAX, (uint32) & CompareStates);
		CALL_Reg(Reg_EAX);
	}
#endif
	if(JumpType == JUMP_TYPE_BREAKOUT)
	{
		compilerstatus.KEEP_RECOMPILING = FALSE;
	}

	if(JumpType == JUMP_TYPE_INDIRECT)
	{
		if(emuoptions.OverclockFactor == 1)
			SUB_ImmToMemory((_u32) & countdown_counter, viCounter);
		else
			ADD_ImmToMemory((_u32) & countdown_counter, viCounter);
		TLB_TRANSLATE_PC_INDIRECT();
		RET();
	}
	else if(currentromoptions.Link_4KB_Blocks != USE4KBLINKBLOCK_YES)
	{
		if(emuoptions.OverclockFactor == 1)
			SUB_ImmToMemory((_u32) & countdown_counter, viCounter);
		else
			ADD_ImmToMemory((_u32) & countdown_counter, viCounter);
		MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &gHWS_pc, targetpc);
		TLB_TRANSLATE_PC(targetpc);
		RET();
	}
	else
	{
		if(JumpType == JUMP_TYPE_DIRECT)		/* may need to link target 1 */
		{
			SET_TARGET_1_LINK(compilerstatus.TempPC, targetpc);
			if(current_block_entry->need_target_1)
			{
				SUB_ImmToMemory((_u32) & countdown_counter, viCounter);
				CMP_MemoryWithImm(1, (uint32) & countdown_counter, 0);
				Jcc_auto(CC_LE, 95);			/* jmp true */

				JMP_Long((uint32) 0);			/* here we link to target 1's block ptr */
				current_block_entry->jmp_to_target_1_code_addr = compilerstatus.lCodePosition - 4;
				SetTarget(95);

				MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &gHWS_pc, targetpc);
				TLB_TRANSLATE_PC(targetpc);
				RET();
			}
			else
			{
				SUB_ImmToMemory((_u32) & countdown_counter, viCounter);
				MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &gHWS_pc, targetpc);
				TLB_TRANSLATE_PC(targetpc);
				RET();
			}
		}
		else	/* JumpType == JUMP_TYPE_BREAKOUT, may need to link target 2 */
		{
			SET_TARGET_2_LINK(compilerstatus.TempPC, targetpc);
			if(current_block_entry->need_target_2)
			{
				SUB_ImmToMemory((_u32) & countdown_counter, viCounter);
				CMP_MemoryWithImm(1, (uint32) & countdown_counter, 0);

				Jcc_auto(CC_LE, 31);	/* jmp true */
				JMP_Long((uint32) 0);	/* here we link to target 2's block ptr */
				current_block_entry->jmp_to_target_2_code_addr = compilerstatus.lCodePosition - 4;
				SetTarget(31);

				MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &gHWS_pc, targetpc);
				TLB_TRANSLATE_PC(targetpc);
				RET();
			}
			else
			{
				SUB_ImmToMemory((_u32) & countdown_counter, viCounter);
				MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &gHWS_pc, targetpc);
				TLB_TRANSLATE_PC(targetpc);
				RET();
			}
		}
	}

	if(CompilingSlot)
	{
		DisplayError("Ok, CompilingSlot is still TRUE when finishing compiling the block, PC=%08X", gHWS_pc);
	}
}

extern x86regtyp		xRD[1];
extern x86regtyp		xRS[1];
extern x86regtyp		xRT[1];

unsigned long			templCodePosition;
extern unsigned long	JumpTargets[100];
unsigned long			wPosition;
extern void				SetRdRsRt64bit(OP_PARAMS);
extern FlushedMap		FlushedRegistersMap[NUM_CONSTS];
extern void				SwitchToOpcodePass(void);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_bne(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc;
	int		IsNear = SAFE_NEAR_ONLY;			/* If short, range= -128 to +127 */
	int		Is32bit = 0;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS if(gMultiPass.WhichPass == COMPILE_OPCODES_ONLY) MessageBox(NULL, "Bad", 0, 0);
	SetRdRsRt64bit(PASS_PARAMS);

	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	_OPCODE_DEBUG_BRANCH_(r4300i_bne);

	aValue = (reg->pc + 4 + (__I << 2));

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);

	if(xRS->mips_reg == xRT->mips_reg)
	{
		/* false */
		Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		return;
	}
	else if((ConstMap[xRS->mips_reg].IsMapped == 1) && (ConstMap[xRT->mips_reg].IsMapped == 1))
	{
		if(ConstMap[xRS->mips_reg].value == ConstMap[xRT->mips_reg].value)
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		}
		else
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		}

		return;
	}

	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Is32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	MapRS;
	MapRT;

	CMP_Reg1WithReg2(1, xRS->x86reg, xRT->x86reg);
_Redo:
	templCodePosition = compilerstatus.lCodePosition;

	if(!Is32bit)
	{
		Jcc_auto(CC_NE, 90);					/* jmp true */
		CMP_Reg1WithReg2(1, xRS->HiWordLoc, xRT->HiWordLoc);
	}

	if(IsNear == 1) Jcc_Near_auto(CC_E, 91);	/* jmp false */
	else
		Jcc_auto(CC_E, 91);						/* jmp false */

	/* true */
	if(!Is32bit) SetTarget(90);

	SPEED_HACK reg->pc += 4;

	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	/* false */
	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	if((wPosition > 120) && (IsNear == 0))
	{
		compilerstatus.lCodePosition = templCodePosition;
		reg->pc -= 4;

		/* Rewrite the code as a near jump. Short jump won't cut it. */
		IsNear = 1;
		goto _Redo;
	}

	if(IsNear == 0)
	{
		SetTarget(91);
	}
	else
	{
		SetNearTarget(91);
	}

	/* end of compiled block */
	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_beq(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32				aValue;
	int					tempPC = reg->pc;
	int					Use32bit = 0;
	CHECK_OPCODE_PASS	SetRdRsRt64bit(PASS_PARAMS);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	_OPCODE_DEBUG_BRANCH_(r4300i_beq);

	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;

	aValue = (reg->pc + 4 + (__I << 2));

	if((CheckIs32Bit(__RT) && CheckIs32Bit(__RS)) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
		Use32bit = 1;
	}

	if(xRS->mips_reg == xRT->mips_reg)
	{
		/* true */
		SPEED_HACK reg->pc += 4;
		FlushAllRegisters();
		Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		return;
	}
	else if((ConstMap[xRS->mips_reg].IsMapped == 1) && (ConstMap[xRT->mips_reg].IsMapped == 1))
	{
		if(ConstMap[xRS->mips_reg].value == ConstMap[xRT->mips_reg].value)
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		}
		else
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		}

		return;
	}
	else if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		MapRT;
		CMP_RegWithImm(1, xRT->x86reg, ConstMap[xRS->mips_reg].value);
		Jcc_Near_auto(CC_NE, 91);		/* jmp false */

		if(!Use32bit)
		{
			CMP_RegWithImm(1, xRT->HiWordLoc, ((__int32) ConstMap[xRS->mips_reg].value >> 31));
			Jcc_Near_auto(CC_NE, 93);	/* jmp false */
		}
	}
	else if(ConstMap[xRT->mips_reg].IsMapped == 1)
	{
		MapRS;
		CMP_RegWithImm(1, xRS->x86reg, ConstMap[xRT->mips_reg].value);
		Jcc_Near_auto(CC_NE, 91);		/* jmp false */

		if(!Use32bit)
		{
			CMP_RegWithImm(1, xRS->HiWordLoc, ((__int32) ConstMap[xRT->mips_reg].value >> 31));
			Jcc_Near_auto(CC_NE, 93);	/* jmp false */
		}
	}
	else if(xRS->mips_reg != xRT->mips_reg)
	{
		MapRS;
		MapRT;
		CMP_Reg1WithReg2(1, xRS->x86reg, xRT->x86reg);
		Jcc_Near_auto(CC_NE, 91);		/* jmp false */

		if(!Use32bit)
		{
			CMP_Reg1WithReg2(1, xRS->HiWordLoc, xRT->HiWordLoc);
			Jcc_Near_auto(CC_NE, 93);	/* jmp false */
		}
	}

	/* true */
	SPEED_HACK
	reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	/* false */
	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	SetNearTarget(91);
	if(!Use32bit) SetNearTarget(93);
	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_beql(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc + 4;
	int		Use32bit = 0;
	int		NoTest = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
    SetRdRsRt64bit(PASS_PARAMS);

	_OPCODE_DEBUG_BRANCH_(r4300i_beql);

	aValue = (reg->pc + 4 + (__I << 2));

	if(xRS->mips_reg == xRT->mips_reg)
	{
		/* true */
		SPEED_HACK reg->pc += 4;
		FlushAllRegisters();
		Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		return;
	}
	else if((ConstMap[xRS->mips_reg].IsMapped == 1) && (ConstMap[xRT->mips_reg].IsMapped == 1))
	{
		if(ConstMap[xRS->mips_reg].value == ConstMap[xRT->mips_reg].value)
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		}
		else
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		}

		return;
	}

	if((CheckIs32Bit(__RT) && CheckIs32Bit(__RS)) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
		Use32bit = 1;
	}

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		MapRT;
		CMP_RegWithImm(1, xRT->x86reg, ConstMap[xRS->mips_reg].value);
		Jcc_Near_auto(CC_NE, 91);		/* jmp false */

		if(!Use32bit)
		{
			CMP_RegWithImm(1, xRT->HiWordLoc, ((__int32) ConstMap[xRS->mips_reg].value >> 31));
			Jcc_Near_auto(CC_NE, 93);	/* jmp false */
		}
	}
	else if(ConstMap[xRT->mips_reg].IsMapped == 1)
	{
		MapRS;
		CMP_RegWithImm(1, xRS->x86reg, ConstMap[xRT->mips_reg].value);
		Jcc_Near_auto(CC_NE, 91);		/* jmp false */

		if(!Use32bit)
		{
			CMP_RegWithImm(1, xRS->HiWordLoc, ((__int32) ConstMap[xRT->mips_reg].value >> 31));
			Jcc_Near_auto(CC_NE, 93);	/* jmp false */
		}
	}
	else if(xRS->mips_reg != xRT->mips_reg)
	{
		MapRS;
		MapRT;
		CMP_Reg1WithReg2(1, xRS->x86reg, xRT->x86reg);
		Jcc_Near_auto(CC_NE, 91);		/* jmp false */

		if(!Use32bit)
		{
			CMP_Reg1WithReg2(1, xRS->HiWordLoc, xRT->HiWordLoc);
			Jcc_Near_auto(CC_NE, 93);	/* jmp false */
		}
	}

	/* true */
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	/* false */
	wPosition = compilerstatus.lCodePosition - JumpTargets[91];

	/*
	 * if (wPosition > 120) £
	 * DisplayError("%08X: beq: Code %d bytes is too large for a short jump. Must be
	 * <= 127 bytes.", reg->pc-4, wPosition);
	 */
	if(!NoTest) SetNearTarget(91);
	if(!Use32bit) SetNearTarget(93);
	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_bnel(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc + 4;
	int		IsNear = 1;			/* If short, range= -128 to +127 */
	int		Is32bit = 0;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(gMultiPass.WhichPass == COMPILE_OPCODES_ONLY) MessageBox(NULL, "Bad", 0, 0);

	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bnel);

	aValue = (reg->pc + 4 + (__I << 2));

	if(xRS->mips_reg == xRT->mips_reg)
	{
		/* false */
		Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		return;
	}
	else if((ConstMap[xRS->mips_reg].IsMapped == 1) && (ConstMap[xRT->mips_reg].IsMapped == 1))
	{
		if(ConstMap[xRS->mips_reg].value == ConstMap[xRT->mips_reg].value)
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		}
		else
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		}

		return;
	}

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);
	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Is32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	MapRS;
	MapRT;

	CMP_Reg1WithReg2(1, xRS->x86reg, xRT->x86reg);
_Redo:
	templCodePosition = compilerstatus.lCodePosition;

	if(!Is32bit)
	{
		Jcc_auto(CC_NE, 90);	/* jmp true */
		CMP_Reg1WithReg2(1, xRS->HiWordLoc, xRT->HiWordLoc);
	}

	if(IsNear == 1) Jcc_Near_auto(CC_E, 91);	/* jmp false */
	else
		Jcc_auto(CC_E, 91);						/* jmp false */

	/* true */
	if(!Is32bit) SetTarget(90);

	SPEED_HACK reg->pc += 4;

	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	/* false */
	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	if((wPosition > 120) && (IsNear == 0))
	{
		compilerstatus.lCodePosition = templCodePosition;
		reg->pc -= 4;

		/* Rewrite the code as a near jump. Short jump won't cut it. */
		IsNear = 1;
		goto _Redo;
	}

	if(IsNear == 0)
	{
		SetTarget(91);
	}
	else
	{
		SetNearTarget(91);
	}

	/* end of compiled block */
	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */

void dyna4300i_blez(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc;
	int		IsNear = 1;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	int		Use32bit = 0;
	int		templCodePosition32;
	int		templCodePosition64;
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_blez);

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);

	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	aValue = (reg->pc + 4 + (__I << 2));

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		if((_int32) ConstMap[xRS->mips_reg].value <= 0)
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		}
		else
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		}

		return;
	}

	MapRS;

	if(!Use32bit)
	{
		CMP_RegWithShort(1, xRS->HiWordLoc, 0);
		templCodePosition64 = compilerstatus.lCodePosition;
_Redo64:
		__asm
		{
			pushad
		}

		if(IsNear == 1) Jcc_Near_auto(CC_G, 91);	/* jmp false */
		else
			Jcc_auto(CC_G, 91);						/* jmp false */
		Jcc_auto(CC_L, 90); /* jmp true */

		CMP_RegWithShort(1, xRS->x86reg, 0);
		if(IsNear == 1) Jcc_Near_auto(CC_A, 93);	/* jmp false */
		else
			Jcc_auto(CC_A, 93);						/* jmp false */

		/* true */
		SetTarget(90);
	}
	else
	{
		CMP_RegWithShort(1, xRS->x86reg, 0);
		templCodePosition32 = compilerstatus.lCodePosition;
_Redo32:
		__asm
		{
			pushad
		}

		if(IsNear == 1) Jcc_Near_auto(CC_G, 91);	/* jmp false */
		else
			Jcc_auto(CC_G, 91);						/* jmp false */
	}

	/* SPEED_HACK */
	reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	if((wPosition > 120) && (IsNear == 0))
	{
		reg->pc -= 4;

		/* Rewrite the code as a near jump. Short jump won't cut it. */
		IsNear = 1;
		if(!Use32bit)
		{
			compilerstatus.lCodePosition = templCodePosition64;

			__asm popad __asm jmp _Redo64
		}
		else
		{
			compilerstatus.lCodePosition = templCodePosition32;
			__asm popad __asm jmp _Redo32
		}
	}
	else
	{
		__asm popad

		/* false */
		if(IsNear == 0)
		{
			SetTarget(91);
			if(!Use32bit) SetTarget(93);
		}
		else
		{
			SetNearTarget(91);
			if(!Use32bit) SetNearTarget(93);
		}
	}

	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_blezl(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32				aValue;
	int					tempPC = reg->pc + 4;
	int					IsNear = SAFE_NEAR_ONLY;
	int					tempRSIs32bit = 0;
	int					tempRTIs32bit = 0;
	int					Use32bit = 0;
	int					templCodePosition32;
	int					templCodePosition64;
	CHECK_OPCODE_PASS	SetRdRsRt64bit(PASS_PARAMS);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	_OPCODE_DEBUG_BRANCH_(r4300i_blezl);
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);

	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	aValue = (reg->pc + 4 + (__I << 2));

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		if((_int32) ConstMap[xRS->mips_reg].value <= 0) /* <--bug fix here */
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		}
		else
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		}

		return;
	}

	MapRS;

	if(!Use32bit)
	{
		CMP_RegWithShort(1, xRS->HiWordLoc, 0);
_Redo64:
		templCodePosition64 = compilerstatus.lCodePosition;
		if(IsNear) Jcc_Near_auto(CC_G, 91);				/* jmp false */
		else
			Jcc_auto(CC_G, 91);						/* jmp false */
		Jcc_auto(CC_L, 90);							/* jmp true */

		CMP_RegWithShort(1, xRS->x86reg, 0);
		if(IsNear == 1) Jcc_Near_auto(CC_A, 93);	/* jmp false */
		else
			Jcc_auto(CC_A, 93);						/* jmp false */

		/* true */
		SetTarget(90);
	}
	else
	{
		CMP_RegWithShort(1, xRS->x86reg, 0);
_Redo32:
		templCodePosition32 = compilerstatus.lCodePosition;
		if(IsNear == 1) Jcc_Near_auto(CC_G, 91);	/* jmp false */
		else
			Jcc_auto(CC_G, 91);						/* jmp false */
	}

	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	if((wPosition > 120) && (IsNear == 0))
	{
		reg->pc -= 4;

		/* Rewrite the code as a near jump. Short jump won't cut it. */
		IsNear = 1;
		if(!Use32bit)
		{
			compilerstatus.lCodePosition = templCodePosition64;
			goto _Redo64;
		}
		else
		{
			compilerstatus.lCodePosition = templCodePosition32;
			goto _Redo32;
		}
	}

	/* false */
	if(IsNear == 0)
	{
		SetTarget(91);
		if(!Use32bit) SetTarget(93);
	}
	else
	{
		SetNearTarget(91);
		if(!Use32bit) SetNearTarget(93);
	}

	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_bgtz(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc;
	int		IsNear = SAFE_NEAR_ONLY;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	int		Use32bit = 0;
	int		templCodePosition32;
	int		templCodePosition64;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bgtz);

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);
	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	aValue = (reg->pc + 4 + (__I << 2));

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		if((_int32) ConstMap[xRS->mips_reg].value > 0)
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		}
		else
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		}

		return;
	}

	MapRS;

	if(!Use32bit)
	{
		CMP_RegWithShort(1, xRS->HiWordLoc, 0);
_Redo64:
		templCodePosition64 = compilerstatus.lCodePosition;

		if(IsNear == 1) Jcc_Near_auto(CC_L, 91);	/* jmp false */
		else
			Jcc_auto(CC_L, 91);						/* jmp false */
		Jcc_auto(CC_G, 90); /* jmp true */
		CMP_RegWithShort(1, xRS->x86reg, 0);
		if(IsNear == 1) Jcc_Near_auto(CC_BE, 93);	/* jmp false2 */
		else
			Jcc_auto(CC_BE, 93);					/* jmp false2 */
	}
	else
	{
		CMP_RegWithShort(1, xRS->x86reg, 0);
_Redo32:
		templCodePosition32 = compilerstatus.lCodePosition;
		if(IsNear == 1) Jcc_Near_auto(CC_LE, 91);	/* jmp false2 */
		else
			Jcc_auto(CC_LE, 91);					/* jmp false2 */
	}

	/*
	 * £
	 * delay Slot
	 */
	if(!Use32bit) SetTarget(90);

	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	if((wPosition > 120) && (IsNear == 0))
	{
		reg->pc -= 4;

		/* Rewrite the code as a near jump. Short jump won't cut it. */
		IsNear = 1;
		if(!Use32bit)
		{
			compilerstatus.lCodePosition = templCodePosition64;
			goto _Redo64;
		}
		else
		{
			compilerstatus.lCodePosition = templCodePosition32;
			goto _Redo32;
		}
	}

	/* false */
	if(IsNear == 0)
	{
		SetTarget(91);
		if(!Use32bit) SetTarget(93);
	}
	else
	{
		SetNearTarget(91);
		if(!Use32bit) SetNearTarget(93);
	}

	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_bgtzl(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc + 4;
	int		IsNear = SAFE_NEAR_ONLY;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	int		Use32bit = 0;
	int		templCodePosition32;
	int		templCodePosition64;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bgtzl);

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);
	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	/*
	 * BGTZ_MACRO £
	 */
	aValue = (reg->pc + 4 + (__I << 2));

	MapRS;

	if(!Use32bit)
	{
		CMP_RegWithShort(1, xRS->HiWordLoc, 0);
_Redo64:
		templCodePosition64 = compilerstatus.lCodePosition;

		if(IsNear == 1) Jcc_Near_auto(CC_L, 91);	/* jmp false */
		else
			Jcc_auto(CC_L, 91);						/* jmp false */
		Jcc_auto(CC_G, 90); /* jmp true */
		CMP_RegWithShort(1, xRS->x86reg, 0);
		if(IsNear == 1) Jcc_Near_auto(CC_BE, 93);	/* jmp false2 */
		else
			Jcc_auto(CC_BE, 93);					/* jmp false2 */
	}
	else
	{
		CMP_RegWithShort(1, xRS->x86reg, 0);
_Redo32:
		templCodePosition32 = compilerstatus.lCodePosition;
		if(IsNear == 1) Jcc_Near_auto(CC_LE, 91);	/* jmp false2 */
		else
			Jcc_auto(CC_LE, 91);					/* jmp false2 */
	}

	/*
	 * £
	 * delay Slot
	 */
	if(!Use32bit) SetTarget(90);

	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	if((wPosition > 120) && (IsNear == 0))
	{
		reg->pc -= 4;

		/* Rewrite the code as a near jump. Short jump won't cut it. */
		IsNear = 1;
		if(!Use32bit)
		{
			compilerstatus.lCodePosition = templCodePosition64;
			goto _Redo64;
		}
		else
		{
			compilerstatus.lCodePosition = templCodePosition32;
			goto _Redo32;
		}
	}

	/* false */
	if(IsNear == 0)
	{
		SetTarget(91);
		if(!Use32bit) SetTarget(93);
	}
	else
	{
		SetNearTarget(91);
		if(!Use32bit) SetNearTarget(93);
	}

	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_bltz(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc;
	int		IsNear = SAFE_NEAR_ONLY;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	int		Use32bit = 0;
	int		templCodePosition;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bltz);

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);
	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	aValue = (reg->pc + 4 + (__I << 2));

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		if((_int32) ConstMap[xRS->mips_reg].value < 0)
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		}
		else
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		}

		return;
	}

	MapRS;
	if(!Use32bit)
		CMP_RegWithShort(1, xRS->HiWordLoc, 0);
	else
		CMP_RegWithShort(1, xRS->x86reg, 0);
_Redo:
	templCodePosition = compilerstatus.lCodePosition;
	if(IsNear == 1) Jcc_Near_auto(CC_GE, 91);	/* jmp false */
	else
		Jcc_auto(CC_GE, 91);

	/* delay Slot */
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	if((wPosition > 120) && (IsNear == 0))
	{
		reg->pc -= 4;

		/* Rewrite the code as a near jump. Short jump won't cut it. */
		IsNear = 1;
		compilerstatus.lCodePosition = templCodePosition;
		goto _Redo;
	}

	if(IsNear == 1)
		SetNearTarget(91);
	else
		SetTarget(91);
	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_bltzl(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc + 4;
	int		IsNear = SAFE_NEAR_ONLY;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	int		Use32bit = 0;
	int		templCodePosition;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bltzl);

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);
	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	aValue = (reg->pc + 4 + (__I << 2));

	MapRS;
	if(!Use32bit)
		CMP_RegWithShort(1, xRS->HiWordLoc, 0);
	else
		CMP_RegWithShort(1, xRS->x86reg, 0);
_Redo:
	templCodePosition = compilerstatus.lCodePosition;
	if(IsNear == 1) Jcc_Near_auto(CC_GE, 91);	/* jmp false */
	else
		Jcc_auto(CC_GE, 91);

	/* delay Slot */
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	if((wPosition > 120) && (IsNear == 0))
	{
		reg->pc -= 4;

		/* Rewrite the code as a near jump. Short jump won't cut it. */
		IsNear = 1;
		compilerstatus.lCodePosition = templCodePosition;
		goto _Redo;
	}

	if(IsNear == 1)
		SetNearTarget(91);
	else
		SetTarget(91);

	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_bgez(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc;
	int		IsNear = SAFE_NEAR_ONLY;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	int		Use32bit = 0;
	int		templCodePosition32;
	int		templCodePosition64;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bgez);

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);
	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	aValue = (reg->pc + 4 + (__I << 2));

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		if((_int32) ConstMap[xRS->mips_reg].value >= 0)
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		}
		else
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		}

		return;
	}

	MapRS;
	if(!Use32bit)
	{
		CMP_RegWithShort(1, xRS->HiWordLoc, 0);
_Redo64:
		templCodePosition64 = compilerstatus.lCodePosition;

		if(IsNear == 1)
			Jcc_Near_auto(CC_L, 91);
		else
			Jcc_auto(CC_L, 91);
		Jcc_auto(CC_G, 90);

		CMP_RegWithShort(1, xRS->x86reg, 0);
		if(IsNear == 1)
			Jcc_Near_auto(CC_B, 93);
		else
			Jcc_auto(CC_B, 93);
	}
	else
	{
		CMP_RegWithShort(1, xRS->x86reg, 0);
_Redo32:
		templCodePosition32 = compilerstatus.lCodePosition;
		if(IsNear == 1) Jcc_Near_auto(CC_L, 91);	/* jmp false2 */
		else
			Jcc_auto(CC_L, 91);						/* jmp false2 */
	}

	/*
	 * BRANCH_MACRO(CC_L, CC_G, CC_B) £
	 * delay Slot
	 */
	if(!Use32bit) SetTarget(90);
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	if((wPosition > 120) && (IsNear == 0))
	{
		reg->pc -= 4;

		/* Rewrite the code as a near jump. Short jump won't cut it. */
		IsNear = 1;
		if(!Use32bit)
		{
			compilerstatus.lCodePosition = templCodePosition64;
			goto _Redo64;
		}
		else
		{
			compilerstatus.lCodePosition = templCodePosition32;
			goto _Redo32;
		}
	}

	if(IsNear == 0)
	{
		SetTarget(91);
		if(!Use32bit) SetTarget(93);
	}
	else
	{
		SetNearTarget(91);
		if(!Use32bit) SetNearTarget(93);
	}

	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_bgezl(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc + 4;
	int		IsNear = SAFE_NEAR_ONLY;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	int		Use32bit = 0;
	int		templCodePosition32;
	int		templCodePosition64;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bgezl);

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);
	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	aValue = (reg->pc + 4 + (__I << 2));

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		if((_int32) ConstMap[xRS->mips_reg].value >= 0)
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);
		}
		else
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
		}

		return;
	}

	MapRS;
	if(!Use32bit)
	{
		CMP_RegWithShort(1, xRS->HiWordLoc, 0);
_Redo64:
		templCodePosition64 = compilerstatus.lCodePosition;

		if(IsNear == 1)
			Jcc_Near_auto(CC_L, 91);
		else
			Jcc_auto(CC_L, 91);
		Jcc_auto(CC_G, 90);

		CMP_RegWithShort(1, xRS->x86reg, 0);
		if(IsNear == 1)
			Jcc_Near_auto(CC_B, 93);
		else
			Jcc_auto(CC_B, 93);
	}
	else
	{
		CMP_RegWithShort(1, xRS->x86reg, 0);
_Redo32:
		templCodePosition32 = compilerstatus.lCodePosition;
		if(IsNear == 1) Jcc_Near_auto(CC_L, 91);	/* jmp false2 */
		else
			Jcc_auto(CC_L, 91);						/* jmp false2 */
	}

	/*
	 * BRANCH_MACRO(CC_L, CC_G, CC_B) £
	 * delay Slot
	 */
	if(!Use32bit) SetTarget(90);
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	wPosition = compilerstatus.lCodePosition - JumpTargets[91];
	if((wPosition > 120) && (IsNear == 0))
	{
		reg->pc -= 4;

		/* Rewrite the code as a near jump. Short jump won't cut it. */
		IsNear = 1;
		if(!Use32bit)
		{
			compilerstatus.lCodePosition = templCodePosition64;
			goto _Redo64;
		}
		else
		{
			compilerstatus.lCodePosition = templCodePosition32;
			goto _Redo32;
		}
	}

	if(IsNear == 0)
	{
		SetTarget(91);
		if(!Use32bit) SetTarget(93);
	}
	else
	{
		SetNearTarget(91);
		if(!Use32bit) SetNearTarget(93);
	}

	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_bgezal(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	aValue;
	_int32	LinkVal = reg->pc + 8;
	int		tempPC = reg->pc;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	int		Use32bit = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bgezal);

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);

	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	aValue = (reg->pc + 4 + (__I << 2));

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		if((_int32) ConstMap[xRS->mips_reg].value >= 0)
		{
			/* true */
			SPEED_HACK reg->pc += 4;
			FlushAllRegisters();
			Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_YES, LinkVal);
		}
		else
		{
			/* false */
			Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_YES, LinkVal);
		}

		return;
	}

	MapRS;
	if(!Use32bit)
	{
		CMP_RegWithShort(1, xRS->HiWordLoc, 0);
		Jcc_Near_auto(CC_L, 91);
		Jcc_auto(CC_G, 90);

		CMP_RegWithShort(1, xRS->x86reg, 0);
		Jcc_Near_auto(CC_B, 93);
	}
	else
	{
		CMP_RegWithShort(1, xRS->x86reg, 0);
		Jcc_Near_auto(CC_L, 91);	/* jmp false */
	}

	/* delay Slot */
	if(!Use32bit) SetTarget(90);
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_YES, LinkVal);

	SetNearTarget(91);
	if(!Use32bit) SetNearTarget(93);
	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_YES, LinkVal);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_bgezall(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	aValue;
	_int32	LinkVal = reg->pc + 8;
	int		tempPC = reg->pc + 4;
	int		tempRSIs32bit = 0;
	int		tempRTIs32bit = 0;
	int		Use32bit = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bgezall);

	tempRSIs32bit = CheckIs32Bit(xRS->mips_reg);
	tempRTIs32bit = CheckIs32Bit(xRT->mips_reg);

	if((tempRSIs32bit && tempRTIs32bit) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	aValue = (reg->pc + 4 + (__I << 2));

	MapRS;
	if(!Use32bit)
	{
		CMP_RegWithShort(1, xRS->HiWordLoc, 0);
		Jcc_Near_auto(CC_L, 91);
		Jcc_auto(CC_G, 90);

		CMP_RegWithShort(1, xRS->x86reg, 0);
		Jcc_Near_auto(CC_B, 93);
	}
	else
	{
		CMP_RegWithShort(1, xRS->x86reg, 0);
		Jcc_Near_auto(CC_L, 91);	/* jmp false */
	}

	/* delay Slot */
	if(!Use32bit) SetTarget(90);
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_YES, LinkVal);

	SetNearTarget(91);
	if(!Use32bit) SetNearTarget(93);
	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_YES, LinkVal);
}

/*
 =======================================================================================================================
    Rice: do you know any games that use bltzal, bltzall? If so, i'll optimize these. £
    Otherwise, I will keep them the way they are now, so that I don't risk breaking them.
 =======================================================================================================================
 */
void dyna4300i_regimm_bltzal(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	aValue;
	int		tempPC = reg->pc;
	_int32	LinkVal = reg->pc + 8;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bltzal);

	aValue = (reg->pc + 4 + (__I << 2));

	MapRS;
	CMP_RegWithShort(1, xRS->HiWordLoc, 0);
	Jcc_Near_auto(CC_GE, 91);	/* jmp false */

	/* delay Slot */
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_YES, LinkVal);

	SetNearTarget(91);
	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_YES, LinkVal);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_bltzall(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	aValue;
	int		tempPC = reg->pc + 4;
	_int32	LinkVal = reg->pc + 8;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_OPCODE_DEBUG_BRANCH_(r4300i_bltzall);

	aValue = (reg->pc + 4 + (__I << 2));

	MapRS;
	CMP_RegWithShort(1, xRS->HiWordLoc, 0);
	Jcc_Near_auto(CC_GE, 91);	/* jmp false */

	/* delay Slot */
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_YES, LinkVal);

	SetNearTarget(91);
	Interrupts(JUMP_TYPE_BREAKOUT, tempPC + 4, LINK_YES, LinkVal);
}

/* For JAL finder */
extern void LogSomething(void);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void tempyness(void)
{
	__asm
	{
		pushad
	}

	LogSomething();
	__asm
	{
		popad
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_jal(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	aValue;
	_int32	LinkVal = reg->pc + 8;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	_OPCODE_DEBUG_BRANCH_(r4300i_jal) aValue = ((reg->pc & 0xf0000000) | (____T << 2));

	reg->pc += 4;
	compilerstatus.FlagJAL = TRUE;

	/*
	 * Jal finder £
	 * MOV_ImmToMemory(1, ModRM_disp32, (uint32)&reg->pc, reg->pc); £
	 * X86_CALL((uint32)&tempyness);
	 */
	Compile_Slot_Jump(reg->pc);
	MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &reg->pc, aValue);

	/* end of compiled block */
	compilerstatus.KEEP_RECOMPILING = FALSE;

	Interrupts(JUMP_TYPE_DIRECT, aValue, LINK_YES, LinkVal);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_j(OP_PARAMS)
{
	/*~~~~~~~~~~~*/
	_u32	aValue;
	/*~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	_OPCODE_DEBUG_BRANCH_(r4300i_j);

	aValue = ((reg->pc & 0xf0000000) | (____T << 2));

	J_SPEED_HACK reg->pc += 4;
	Compile_Slot_Jump(reg->pc);
	MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &reg->pc, aValue);

	/* end of compiled block */
	compilerstatus.KEEP_RECOMPILING = FALSE;
	FlushAllRegisters();
	Interrupts(JUMP_TYPE_DIRECT, aValue, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_jr(OP_PARAMS)
{
	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	_OPCODE_DEBUG_BRANCH_(r4300i_jr) FlushAllRegisters();
	LoadLowMipsCpuRegister(__RS, Reg_EAX);
	MOV_EAXToMemory(1, (unsigned long) &reg->pc);

	reg->pc += 4;
	Compile_Slot_Jump(reg->pc);

	/* end of compiled block */
	compilerstatus.KEEP_RECOMPILING = FALSE;
	FlushAllRegisters();
	Interrupts(JUMP_TYPE_INDIRECT, 0, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_bc1f(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc;
	/*~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	_OPCODE_DEBUG_BRANCH_(r4300i_COP1_bc1f);
	aValue = (reg->pc + 4 + (__I << 2));

	PUSH_RegIfMapped(Reg_EAX);
	MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (_u32) & reg->COP1Con[31]);
	AND_ImmToReg(1, Reg_EAX, COP1_CONDITION_BIT);
	TEST_Reg2WithReg1(1, Reg_EAX, Reg_EAX);
	POP_RegIfMapped(Reg_EAX);

	Jcc_Near_auto(CC_NZ, 91);	/* jmp false */

	/* delay Slot */
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	SetNearTarget(91);
	Interrupts(JUMP_TYPE_BREAKOUT, reg->pc, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_bc1fl(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc + 4;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	_OPCODE_DEBUG_BRANCH_(r4300i_COP1_bc1fl);
	aValue = (reg->pc + 4 + (__I << 2));

	PUSH_RegIfMapped(Reg_EAX);
	MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (unsigned long) (&reg->COP1Con[31]));
	AND_ImmToReg(1, Reg_EAX, COP1_CONDITION_BIT);
	TEST_Reg2WithReg1(1, Reg_EAX, Reg_EAX);
	POP_RegIfMapped(Reg_EAX);

	Jcc_Near_auto(CC_NZ, 91);	/* jmp false */

	/* delay Slot */
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	SetNearTarget(91);
	Interrupts(JUMP_TYPE_BREAKOUT, reg->pc + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_bc1t(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc;
	/*~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	_OPCODE_DEBUG_BRANCH_(r4300i_COP1_bc1t);
	aValue = (reg->pc + 4 + (__I << 2));

	PUSH_RegIfMapped(Reg_EAX);
	MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (_u32) & reg->COP1Con[31]);
	AND_ImmToReg(1, Reg_EAX, COP1_CONDITION_BIT);
	TEST_Reg2WithReg1(1, Reg_EAX, Reg_EAX);
	POP_RegIfMapped(Reg_EAX);

	Jcc_Near_auto(CC_Z, 91);	/* jmp false */

	/* delay Slot */
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	SetNearTarget(91);
	Interrupts(JUMP_TYPE_BREAKOUT, reg->pc, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_bc1tl(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	aValue;
	int		tempPC = reg->pc + 4;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	_OPCODE_DEBUG_BRANCH_(r4300i_COP1_bc1tl);
	aValue = (reg->pc + 4 + (__I << 2));

	PUSH_RegIfMapped(Reg_EAX);
	MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (unsigned long) (&reg->COP1Con[31]));
	AND_ImmToReg(1, Reg_EAX, COP1_CONDITION_BIT);
	TEST_Reg2WithReg1(1, Reg_EAX, Reg_EAX);
	POP_RegIfMapped(Reg_EAX);

	Jcc_Near_auto(CC_Z, 91);	/* jmp false */

	/* delay Slot */
	SPEED_HACK reg->pc += 4;
	Compile_Slot_And_Interrupts(reg->pc, aValue, LINK_NO, 0);

	SetNearTarget(91);
	Interrupts(JUMP_TYPE_BREAKOUT, reg->pc + 4, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_jalr(OP_PARAMS)
{
	/*~~~~~~~~~~~*/
	_u32	aValue;
	/*~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	_OPCODE_DEBUG_BRANCH_(r4300i_jalr)
#ifdef DEBUG_COMMON
	if(__RD == __RS)
	{
		DisplayError("In JALR, __RD == __RS");
	}
#endif
	aValue = ((reg->pc & 0xf0000000) | (____T << 2));

	FlushAllRegisters();
	if(__RD < 16)
	{
		MOV_ImmToMemory(1, ModRM_disp8_EBP, (-128 + ((__RD) << 3)), (reg->pc + 8));
		MOV_ImmToMemory(1, ModRM_disp8_EBP, (-124 + ((__RD) << 3)), ((_int32) (reg->pc + 8)) >> 31);
	}
	else
	{
		MOV_ImmToMemory(1, ModRM_disp8_EBP, ((__RD) - 16) << 3, (reg->pc + 8));
		MOV_ImmToMemory(1, ModRM_disp8_EBP, 4 + (((__RD) - 16) << 3), ((_int32) (reg->pc + 8)) >> 31);
	}

	/* FlushedRegistersMap[__RD].Is32bit = 1; */
	LoadLowMipsCpuRegister(__RS, Reg_EAX);
	MOV_EAXToMemory(1, (unsigned long) &reg->pc);

	reg->pc += 4;
	Compile_Slot_Jump(reg->pc);

	/* end of compiled block */
	compilerstatus.KEEP_RECOMPILING = FALSE;
	FlushAllRegisters();

	/* Already calculated the link above, so LINK_NO. */
	Interrupts(JUMP_TYPE_INDIRECT, 0, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_eret(OP_PARAMS)
{
	CHECK_OPCODE_PASS;
	if(emuoptions.OverclockFactor == 1)
		compilerstatus.cp0Counter += 1;
	_OPCODE_DEBUG_BRANCH_(r4300i_COP0_eret) /* questionable. */

	FlushAllRegisters();

//	PUSH_RegIfMapped(Reg_EAX);
	MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (_u32) & reg->COP0Reg[STATUS]);
	AND_ImmToReg(1, Reg_EAX, 0x0004);
	CMP_RegWithShort(1, Reg_EAX, 0);
	Jcc_auto(CC_E, 26);
	MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (_u32) & reg->COP0Reg[ERROREPC]);
	AND_ImmToMemory((unsigned long) &reg->COP0Reg[STATUS], 0xFFFFFFFB);
	JMP_Short_auto(27);
	SetTarget(26);
	MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (_u32) & reg->COP0Reg[EPC]);
	AND_ImmToMemory((unsigned long) &reg->COP0Reg[STATUS], 0xFFFFFFFD);
	SetTarget(27);

	MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &reg->LLbit, 0);
	MOV_RegToMemory(1, Reg_EAX, ModRM_disp32, (unsigned long) &reg->pc);
//	POP_RegIfMapped(Reg_EAX);

	/* end of compiled block */
	compilerstatus.KEEP_RECOMPILING = FALSE;

	Interrupts(JUMP_TYPE_INDIRECT, 0, LINK_NO, 0);
}
#endif
