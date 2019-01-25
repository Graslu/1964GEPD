/*$T OpcodeDebugger.c GC 1.136 03/09/02 17:49:31 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    The opcode debugger is cool. It wil run interpretive and dynarec opcodes "side-by-side" and the states of each
    virtual machines can be compared either after each instruction, or at the end of each block. It is a great way to
    detect bugs, but it was a bitch to write ;) To use it, build the "Win32 - Release Opcode Debugger" MS VisualC
    configuration. The end-user build is just "Win32 Release". Be sure to not use "Assume 32bit" when running the
    debugger.
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
#include <stdio.h>
#include <malloc.h>
#include "../hardware.h"
#include "../r4300i.h"
#include "../n64rcp.h"
#include "../1964ini.h"
#include "regcache.h"
#include "../interrupt.h"
#include "x86.h"
#include "dynaCPU.h"
#include "dynaLog.h"
#include "../emulator.h"
#include "../timer.h"
#include "opcodeDebugger.h"

extern void				RefreshOpList(char *opcode);
extern BOOL				CompilingSlot;
extern dyn_cpu_instr	now_do_dyna_instruction[64];
char					*r4300i_RegNames[32];
char					*r4300i_COP0_RegNames[32];
char					*r4300i_COP1_RegNames[32];

extern char				*DebugPrintInstr(uint32 Instruction);
extern char				op_str[0xff];

extern x86regtyp		xRD[1];
extern x86regtyp		xRS[1];
extern x86regtyp		xRT[1];
extern x86regtyp		xLO[1];
extern x86regtyp		xHI[1];
extern x86regtyp		x86reg[8];
extern MapConstant		TempConstMap_Debug[NUM_CONSTS];
extern x86regtyp		Tempx86reg_Debug[8];

HardwareState			gHardwareState_Interpreter_Compare;
MemoryState				gMemoryState_Interpreter_Compare;
HardwareState			gHardwareState_Flushed_Dynarec_Compare;

#ifdef ENABLE_OPCODE_DEBUGGER
uint8					*sDWORD_R__Debug[0x10000];
uint8					*sDWORD_R_2__Debug[0x10000];
#endif

uint8					**TLB_sDWord_ptr;
uint8					**sDWord_ptr;
uint8					**sDWord2_ptr;
uint32					PcBeforeBranch;
uint32					BlockStartPC;

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void COMPARE_SwitchToInterpretive(void)
{
	/* Setup for interpreter */
#ifdef ENABLE_OPCODE_DEBUGGER
	sDWord_ptr = (uint8 **) &sDWORD_R__Debug;
	sDWord2_ptr = (uint8 **) &sDWORD_R_2__Debug;
#endif

	/* TLB_sDWord_ptr = (uint8**)&TLB_sDWord__Debug; */
	TLB_sDWord_ptr = (uint8 **) &TLB_sDWord;

	p_gMemoryState = &gMemoryState_Interpreter_Compare;
	p_gHardwareState = &gHardwareState_Interpreter_Compare;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COMPARE_SwitchToDynarec(void)
{
	/* Setup for dyna */
	sDWord_ptr = (uint8 **) &sDWord;
	sDWord2_ptr = (uint8 **) &sDWord2;
	TLB_sDWord_ptr = (uint8 **) &TLB_sDWord;

	p_gMemoryState = &gMemoryState;
	p_gHardwareState = &gHardwareState;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CompareStates1(uint32 Instruction)
{
	memcpy(&gHardwareState_Flushed_Dynarec_Compare, &gHardwareState, sizeof(gHardwareState));
}

char	op_Str[0xffff];

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CompareStates(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int reg, errcount = 0;
	int OutputDisplayed = 0;
	int GPR_array_size = sizeof(gHardwareState.GPR);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	op_Str[0] = '\0';

	gHardwareState_Interpreter_Compare.COP0Reg[COUNT] = gHardwareState.COP0Reg[COUNT];

	if(debug_opcode_block == 1)
	{
		if(memcmp(&gHardwareState, &gHardwareState_Interpreter_Compare, GPR_array_size - 8) != 0)
		{
			errcount = 1;
			memcpy(&gHardwareState_Flushed_Dynarec_Compare, &gHardwareState, GPR_array_size - 8);
		}
	}
	else
	{
		if
		(
			memcmp
				(
					&gHardwareState_Flushed_Dynarec_Compare,
					&gHardwareState_Interpreter_Compare,
					GPR_array_size - 8
				) != 0
		)
		{
			errcount = 1;
		}
	}

	/*
	 * -8 to ignore gHardwareState.code..so code must be last item in struct! £
	 * memcmp should be faster, but not sure yet.
	 */
	if(errcount > 0)
	{
		/*
		 * This part needs no optimization because it's only executed when memcmp != 0 £
		 * Which GPR is it ?
		 */
		for(reg = sizeof(gHardwareState.GPR) / 8 - 1; reg >= 0; reg--)
		{
			if(gHardwareState_Flushed_Dynarec_Compare.GPR[reg] != gHardwareState_Interpreter_Compare.GPR[reg])
			{
				if(reg >= 31) break;	/* We won't use this method to compare GPR[32] and GPR[33] (which are mult and
										 * div's LO and HI) */
				sprintf
				(
					generalmessage,
					"%s\n%08X: Dynarec and Interpretive GPR do not match.\n \
					InterGPR[%s(%d)] = %016I64X\n \
					DynaGPR[%s(%d)] = %016I64X\n\n",
					DebugPrintInstr(Instruction),
					gHardwareState.pc,
					r4300i_RegNames[reg],
					reg,
					gHardwareState_Interpreter_Compare.GPR[reg],
					r4300i_RegNames[reg],
					reg,
					gHardwareState_Flushed_Dynarec_Compare.GPR[reg]
				);
				strcat(op_Str, generalmessage);

				/* TRACE0(generalmessage); */
				OutputDisplayed = 1;
			}
		}
	}

	if
	(
		memcmp
			(
				&gHardwareState + GPR_array_size,
				&gHardwareState_Interpreter_Compare + GPR_array_size,
				sizeof(gHardwareState) - (GPR_array_size + 8)
			) != 0
	)
	{
		errcount = 1;

		/* Is it COP0Reg ? */
		for(reg = sizeof(gHardwareState.COP0Reg) / 4 - 1; reg >= 0; reg--)
		{
			if(gHardwareState.COP0Reg[reg] != gHardwareState_Interpreter_Compare.COP0Reg[reg])
			{
				sprintf
				(
					generalmessage,
					"%s\n%08X: Dynarec and Interpretive COP0Reg do not match.\n \
            InterCOP0Reg[%s] = %08X\n \
            DynaCOP0Reg[%s] = %08X\n\n",
					DebugPrintInstr(Instruction),
					gHardwareState.pc,
					r4300i_COP0_RegNames[reg],
					gHardwareState_Interpreter_Compare.COP0Reg[reg],
					r4300i_COP0_RegNames[reg],
					gHardwareState.COP0Reg[reg]
				);
				strcat(op_Str, generalmessage);

				/* TRACE0(generalmessage); */
				OutputDisplayed = 1;
			}
		}

		/* Is it COP0Con ? */
		for(reg = sizeof(gHardwareState.COP0Con) / 4 - 1; reg >= 0; reg--)
		{
			if(gHardwareState.COP0Con[reg] != gHardwareState_Interpreter_Compare.COP0Con[reg])
			{
				sprintf
				(
					generalmessage,
					"%s\n%08X: Dynarec and Interpretive COP0Con do not match.\n \
			InterCOP0Con[%d] = %08X\n \
			DynaCOP0Con[%d] = %08X\n\n",
					DebugPrintInstr(Instruction),
					gHardwareState.pc,
					reg,
					gHardwareState_Interpreter_Compare.COP0Con[reg],
					reg,
					gHardwareState.COP0Con[reg]
				);
				strcat(op_Str, generalmessage);

				/* TRACE0(generalmessage); */
				OutputDisplayed = 1;
			}
		}

		/* Is it COP1Con ? */
		for(reg = sizeof(gHardwareState.COP1Con) / 4 - 1; reg >= 0; reg--)
		{
			if(gHardwareState.COP1Con[reg] != gHardwareState_Interpreter_Compare.COP1Con[reg])
			{
				sprintf
				(
					generalmessage,
					"%s\n%08X: Dynarec and Interpretive COP1Con do not match.\n \
			InterCOP1Con[%d] = %08X\n \
			DynaCOP1Con[%d] = %08X\n\n",
					DebugPrintInstr(Instruction),
					gHardwareState.pc,
					reg,
					gHardwareState_Interpreter_Compare.COP1Con[reg],
					reg,
					gHardwareState.COP1Con[reg]
				);
				strcat(op_Str, generalmessage);

				/* TRACE0(generalmessage); */
				OutputDisplayed = 1;
			}
		}

		/* Is it FPR32 ? */
		if(FR_reg_offset == 1)			/* 64bit FPU is off */
		{
			for(reg = 0; reg < 32; reg++)
			{
				if(gHardwareState.fpr32[reg] != gHardwareState_Interpreter_Compare.fpr32[reg])
				{
					sprintf
					(
						generalmessage,
						"%s\n%08X: Dynarec and Interpretive 32bit FPU reg do not match.\n \
					Inter_fpr32[%d] = %08X\n \
					Dyna_fpr32[%d] = %08X\n\n",
						DebugPrintInstr(Instruction),
						gHardwareState.pc,
						reg,
						gHardwareState_Interpreter_Compare.fpr32[reg],
						reg,
						gHardwareState.fpr32[reg]
					);
					strcat(op_Str, generalmessage);

					/* TRACE0(generalmessage); */
					OutputDisplayed = 1;
				}
			}
		}
		else	/* 64bit FPU */
		{
			for(reg = 0; reg < 32; reg++)
			{
				if
				(
					gHardwareState.fpr32[reg] != gHardwareState_Interpreter_Compare.fpr32[reg]
				||	gHardwareState.fpr32[reg + 32] != gHardwareState_Interpreter_Compare.fpr32[reg + 32]
				)
				{
					sprintf
					(
						generalmessage,
						"%s\n%08X: Dynarec and Interpretive 64bit FPU reg do not match.\n \
					Inter_fpr32[%d] = %08X%08X\n \
					Dyna_fpr32[%d] = %08X%08X\n\n",
						DebugPrintInstr(Instruction),
						gHardwareState.pc,
						reg,
						gHardwareState_Interpreter_Compare.fpr32[reg + 32],
						gHardwareState_Interpreter_Compare.fpr32[reg],
						reg,
						gHardwareState.fpr32[reg + 32],
						gHardwareState.fpr32[reg]
					);
					strcat(op_Str, generalmessage);

					/* TRACE0(generalmessage); */
					OutputDisplayed = 1;
				}
			}
		}

		/* Is it LLbit ? */
		if(gHardwareState.LLbit != gHardwareState_Interpreter_Compare.LLbit)
		{
			DebugPrintInstr(Instruction);

			sprintf
			(
				generalmessage,
				"%s\n%08X: Dynarec and Interpretive LLbit do not match.\n \
			InterLLbit = %08X\n \
			DynaLLbit = %08X\n\n",
				DebugPrintInstr(Instruction),
				gHardwareState.pc,
				gHardwareState_Interpreter_Compare.LLbit,
				reg,
				gHardwareState.LLbit
			);
			strcat(op_Str, generalmessage);

			/* TRACE0(generalmessage); */
			OutputDisplayed = 1;
		}

		/* Is it RememberFprHi ? */
		for(reg = sizeof(gHardwareState.RememberFprHi) / 4 - 1; reg >= 0; reg--)
		{
			if(gHardwareState.RememberFprHi[reg] != gHardwareState_Interpreter_Compare.RememberFprHi[reg])
			{
				DebugPrintInstr(Instruction);

				sprintf
				(
					generalmessage,
					"%s\n%08X: Dynarec and Interpretive RememberFprHi do not match.\n \
			Inter_RememberFprHi[%d] = %08X\n \
			Dyna_RememberFprHi[%d] = %08X\n\n",
					DebugPrintInstr(Instruction),
					gHardwareState.pc,
					reg,
					gHardwareState_Interpreter_Compare.RememberFprHi[reg],
					reg,
					gHardwareState.RememberFprHi[reg]
				);
				strcat(op_Str, generalmessage);

				/* TRACE0(generalmessage); */
				OutputDisplayed = 1;
			}
		}

		/* ignore for now. TODO: FixMe! */
		if(!OutputDisplayed);
		else if(errcount > 0)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			int val = MessageBox(NULL, op_Str, "Error", MB_YESNOCANCEL | MB_ICONINFORMATION | MB_SYSTEMMODAL);
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			sprintf(generalmessage, "\nPC before jump = %08X, Block starts at: %08X", PcBeforeBranch, BlockStartPC);
			strcat(op_Str, generalmessage);

			strcat(op_Str, "\nYes-Copy Dyna to Interpreter,  No->Copy Interpreter to Dyna, Cancel->Do nothing");

			if(!OutputDisplayed) DisplayError("Missed a message output.");

			memset(op_Str, 0, sizeof(op_Str));

			if(val == IDYES)
			{
				Debugger_Copy_Memory(&gMemoryState_Interpreter_Compare, &gMemoryState);

				/*
				 * memcpy(&gMemoryState_Interpreter_Compare, &gMemoryState,
				 * sizeof(gHardwareState));
				 */
				memcpy
				(
					&gHardwareState_Interpreter_Compare,
					&gHardwareState_Flushed_Dynarec_Compare,
					sizeof(gHardwareState)
				);
			}
			else if(val == IDNO)
			{
				memcpy(&gHardwareState, &gHardwareState_Interpreter_Compare, sizeof(gHardwareState));
				Debugger_Copy_Memory(&gMemoryState, &gMemoryState_Interpreter_Compare);

				/*
				 * memcpy(&gMemoryState, &gMemoryState_Interpreter_Compare,
				 * sizeof(gHardwareState));
				 */
				memset(op_Str, 0, sizeof(op_Str));
				return;
			}
		}
	}
}

extern void (*CPU_instruction[64]) (uint32 Instruction);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COMPARE_Run_Interpreter_Opcode(uint32 code)
{
	/*
	 * static BOOL passed = FALSE; £
	 * if( gHWS_pc >= 0x800ED79C && gHWS_pc <= 0x800ED7C4 ) £
	 * { £
	 * passed=TRUE; £
	 * }
	 */
	CPU_instruction[(code >> 26)](code);

	/*
	 * if( passed ) £
	 * { £
	 * if( *(uint32*)&gMS_RDRAM[0x39060] != 0x1B ) £
	 * DisplayError("Memory is not 0x1B any more, pc=%08X", gHWS_pc); £
	 * }
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COMPARE_Run(uint32 Inter_Opcode_Address, uint32 code)
{
	if(debug_opcode != 1) return;

#ifdef TEST_OPCODE_DEBUGGER_INTEGRITY21
	return;
#endif
	PUSHAD();			/* bugfix: don't use PushMap in the Opcode Debugger because */

	/*
	 * PushMap can change the x86reg map! Then when we restore the £
	 * map for the compare, it bombs.
	 */
	MOV_ImmToReg(1, Reg_EAX, (_u32) & COMPARE_SwitchToInterpretive);
	CALL_Reg(Reg_EAX);

	MOV_ImmToReg(1, Reg_ECX, code);
	MOV_ImmToReg(1, Reg_EAX, (_u32) COMPARE_Run_Interpreter_Opcode);
	CALL_Reg(Reg_EAX);	/* disable this line to test integrity of dyna portion */

	MOV_ImmToReg(1, Reg_EAX, (_u32) & COMPARE_SwitchToDynarec);
	CALL_Reg(Reg_EAX);

	POPAD();
}

_int32	RegisterRecall[8];

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void WriteBackDirtyToDynaCompare(_int8 k)
{
	/* 32 bit register */
	if((x86reg[k].HiWordLoc == k))
	{
		if(x86reg[k].Is32bit != 1) DisplayError("Bug");

		MOV_RegToMemory
		(
			1,
			k,
			ModRM_disp32,
			(_u32) & gHardwareState_Flushed_Dynarec_Compare.GPR[0] + (x86reg[k].mips_reg << 3)
		);
		SAR_RegByImm(1, k, 31);
		MOV_RegToMemory
		(
			1,
			x86reg[k].HiWordLoc,
			ModRM_disp32,
			4 + (_u32) & gHardwareState_Flushed_Dynarec_Compare.GPR[0] + (x86reg[k].mips_reg << 3)
		);
	}
	else
	/* 64 bit register */
	{
		if(x86reg[k].Is32bit == 1) DisplayError("Bug");

		MOV_RegToMemory
		(
			1,
			k,
			ModRM_disp32,
			(_u32) & gHardwareState_Flushed_Dynarec_Compare.GPR[0] + (x86reg[k].mips_reg << 3)
		);
		MOV_RegToMemory
		(
			1,
			x86reg[k].HiWordLoc,
			ModRM_disp32,
			4 + (_u32) & gHardwareState_Flushed_Dynarec_Compare.GPR[0] + (x86reg[k].mips_reg << 3)
		);
	}
}

void	x86reg_Delete(int k);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FlushRegisterToDynaCompare(int k)
{
#ifdef DEBUG_REGCACHE
	/* paranoid error check */
	if(x86reg[k].HiWordLoc == -1) DisplayError("FlushRegister: The HiWord was not set!");
#endif
#ifdef R0_COMPENSATION
	/*
	 * Until we don't map all r0's, we'll need this check £
	 * if (ConstMap[0].IsMapped == 0) DisplayError("How did Const[0] get
	 * unmapped???");
	 */
	if(x86reg[k].mips_reg == 0) x86reg[k].IsDirty = 0;
#endif
	if(x86reg[k].IsDirty == 1) WriteBackDirtyToDynaCompare((_s8) k);

	x86reg_Delete(x86reg[k].HiWordLoc);
	x86reg_Delete(k);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FlushRegisterToDynaCompare_NoUnmap(int k)
{
#ifdef DEBUG_REGCACHE
	/* paranoid error check */
	if(x86reg[k].HiWordLoc == -1) DisplayError("FlushRegister: The HiWord was not set!");
#endif
#ifdef R0_COMPENSATION
	/*
	 * Until we don't map all r0's, we'll need this check £
	 * if (ConstMap[0].IsMapped == 0) DisplayError("How did Const[0] get
	 * unmapped???");
	 */
	if(x86reg[k].mips_reg == 0) x86reg[k].IsDirty = 0;
#endif
	if(x86reg[k].IsDirty == 1) WriteBackDirtyToDynaCompare((_s8) k);

	/*
	 * x86reg_Delete(x86reg[k].HiWordLoc); £
	 * x86reg_Delete(k);
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FlushConstantsToDynaCompare(void)
{
	/*~~*/
	int k;
	/*~~*/

	for(k = 1; k < NUM_CONSTS; k++)
	{
		if(ConstMap[k].IsMapped)
		{
			MOV_ImmToMemory
			(
				1,
				ModRM_disp32,
				(_u32) & gHardwareState_Flushed_Dynarec_Compare.GPR[0] + (k << 3),
				ConstMap[k].value
			);
			MOV_ImmToMemory
			(
				1,
				ModRM_disp32,
				4 + (_u32) & gHardwareState_Flushed_Dynarec_Compare.GPR[0] + (k << 3),
				((_int32) ConstMap[k].value >> 31)
			);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FlushAllRegistersToDynaCompare(void)
{
	/*~~~~~~~~~~~~~~~~~~~~*/
	_int8	k;
	const	Num_x86regs = 8;
	/*~~~~~~~~~~~~~~~~~~~~*/

	FlushConstantsToDynaCompare();	/* schibo: need to work on this for opcode debugger when using constants!! */

	for(k = 0; k < Num_x86regs; k++)
	{
		if(ItIsARegisterNotToUse(k));
		else if(x86reg[k].mips_reg > -1)
			FlushRegisterToDynaCompare(k);
	}
}

#ifdef _DEBUG
extern void UpdateGPR(void);
extern void UpdateCOP0(void);
extern void UpdateFPR(void);
extern void UpdateMisc(void);
extern void UpdateVIReg(void);
#endif
_int32		bad_mips_reg = 0;
uint32		bad_const_hi;
uint32		bad_const_lo;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COMPARE_ConstErrorMessage(uint32 pc)
{
	DisplayError
	(
		"Consts do not match.PC=%08X, Bad FPU Reg=%d, Constant=%08X%08X",
		pc,
		bad_mips_reg,
		bad_const_hi,
		bad_const_lo
	);

	/* TODO: call patcher. Put patcher in its own function. */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COMPARE_DirtyFPU_ErrorMessage(uint32 pc)
{
	DisplayError("FPU reg do not match. PC=%08X, Bad FPU Reg=%d", pc, bad_mips_reg);

	/* TODO: call patcher. Put patcher in its own function. */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COMPARE_SomeErrorMessage(uint32 pc, uint32 code)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	Instruction = code;
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

#ifdef DEBUG_COMMON
	/*
	 * FlushAllRegisters(); £
	 * UpdateGPR(); £
	 * UpdateCOP0(); £
	 * UpdateFPR(); £
	 * UpdateMisc(); £
	 * UpdateVIReg();
	 */
#endif

	/*
	 * This COP1Con[31] check needs to be one of the first because I set bad_mips_reg
	 * to a negative value.
	 */
	if(gHardwareState_Interpreter_Compare.LLbit != gHardwareState.LLbit)
	{
		sprintf
		(
			op_Str,
			"%s\n%08X: COP1Con[31] mismatch.\n \
        COP1Con[31] = %08X\n \
        COP1Con[31] = %08X\n\n",
			DebugPrintInstr(Instruction),
			pc,
			bad_mips_reg,
			gHardwareState_Interpreter_Compare.LLbit,
			bad_mips_reg,
			gHardwareState.LLbit
		);
	}

	/*
	 * This COP1Con[31] check needs to be one of the first because I set bad_mips_reg
	 * to a negative value.
	 */
	else if(gHardwareState_Interpreter_Compare.COP1Con[31] != gHardwareState.COP1Con[31])
	{
		sprintf
		(
			op_Str,
			"%s\n%08X: COP1Con[31] mismatch.\n \
        COP1Con[31] = %08X\n \
        COP1Con[31] = %08X\n\n",
			DebugPrintInstr(Instruction),
			pc,
			bad_mips_reg,
			gHardwareState_Interpreter_Compare.COP1Con[31],
			bad_mips_reg,
			gHardwareState.COP1Con[31]
		);
	}
	else if
		(
			gHardwareState_Interpreter_Compare.GPR[bad_mips_reg] != gHardwareState_Flushed_Dynarec_Compare.GPR[
				bad_mips_reg]
		)
	{
		sprintf
		(
			op_Str,
			"%s\n%08X: Dirty GPR mismatch.\n \
        InterGPR[%s/%d] = %016I64X\n \
        DynaGPR[%s/%d] = %016I64X\n\n",
			DebugPrintInstr(Instruction),
			pc,
			r4300i_RegNames[bad_mips_reg],
			bad_mips_reg,
			gHardwareState_Interpreter_Compare.GPR[bad_mips_reg],
			r4300i_RegNames[bad_mips_reg],
			bad_mips_reg,
			gHardwareState_Flushed_Dynarec_Compare.GPR[bad_mips_reg]
		);
	}
	else if(gHardwareState_Interpreter_Compare.fpr32[bad_mips_reg] != gHardwareState.fpr32[bad_mips_reg])
	{
		sprintf
		(
			op_Str,
			"%s\n%08X: Dirty FPR mismatch.\n \
        InterFPR[%d] = %016I64X\n \
        DynaFPR[%d] = %016I64X\n\n",
			DebugPrintInstr(Instruction),
			pc,
			bad_mips_reg,
			gHardwareState_Interpreter_Compare.fpr32[bad_mips_reg],
			bad_mips_reg,
			gHardwareState.fpr32[bad_mips_reg]
		);
	}
	else if(gHardwareState_Interpreter_Compare.COP0Reg[bad_mips_reg] != gHardwareState.COP0Reg[bad_mips_reg])
	{
		sprintf
		(
			op_Str,
			"%08X: COP0 mismatch.\n \
        InterCOP0[%s/%d] = %08X\n \
        DynaCOP0[%s/%d] = %08X\n\n",
			pc,
			r4300i_COP0_RegNames[bad_mips_reg],
			bad_mips_reg,
			gHardwareState_Interpreter_Compare.COP0Reg[bad_mips_reg],
			r4300i_COP0_RegNames[bad_mips_reg],
			bad_mips_reg,
			gHardwareState.COP0Reg[bad_mips_reg]
		);
	}
	else
	{
		DisplayError("Some mismatch is found, bad register should be %d", bad_mips_reg);
		return;
	}
{
	/*~~~~*/
	int val;
	/*~~~~*/

	sprintf(generalmessage, "\nPC before jump = %08X, Block starts at: %08X", PcBeforeBranch, BlockStartPC);
	strcat(op_Str, generalmessage);
	TRACE0(op_Str);
	strcat(op_Str, "\nYes-Copy Dyna to Interpreter,  No->Do nothing");
	val = MessageBox(NULL, op_Str, "woops", MB_YESNO | MB_ICONINFORMATION);

	memset(op_Str, 0, sizeof(op_Str));

	if(val == IDYES)
	{
		Debugger_Copy_Memory(&gMemoryState_Interpreter_Compare, &gMemoryState);

		/*
		 * memcpy(&gMemoryState_Interpreter_Compare, &gMemoryState,
		 * sizeof(gHardwareState)); £
		 * we don't know which chip it was, so for now let's just do this:
		 */
		memcpy
		(
			&gHardwareState_Interpreter_Compare.GPR[bad_mips_reg],
			&gHardwareState_Flushed_Dynarec_Compare.GPR[bad_mips_reg],
			4
		);
		memcpy(&gHardwareState_Interpreter_Compare.GPR[bad_mips_reg] + 4, &gHardwareState.GPR[bad_mips_reg] + 4, 4);
		memcpy(&gHardwareState_Interpreter_Compare.fpr32[bad_mips_reg], &gHardwareState.fpr32[bad_mips_reg], 4);
		memcpy(&gHardwareState_Interpreter_Compare.fpr32[bad_mips_reg] + 4, &gHardwareState.fpr32[bad_mips_reg] + 4, 4);
	}
}
	/* DisplayError("%08X: Dirty reg does not match", pc); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COMPARE_DebugDirtyConst(uint32 mips_reg)
{
	CMP_MemoryWithImm(1, (_u32) & gHardwareState_Interpreter_Compare.GPR[mips_reg], ConstMap[mips_reg].value);
	Jcc_auto(CC_NE, 20);

	CMP_MemoryWithImm
	(
		1,
		(_u32) & gHardwareState_Interpreter_Compare.GPR[mips_reg] + 4,
		((_s32) ConstMap[mips_reg].value >> 31)
	);
	Jcc_auto(CC_NE, 21);

	/* Passed */
	JMP_Short_auto(22);

	/* Failed */
	SetTarget(20);
	SetTarget(21);
	MOV_ImmToMemory(1, ModRM_disp32, (uint32) & bad_mips_reg, (uint32) mips_reg);
	MOV_ImmToMemory(1, ModRM_disp32, (uint32) & bad_const_hi, (uint32) ((_s32) ConstMap[mips_reg].value >> 31));
	MOV_ImmToMemory(1, ModRM_disp32, (uint32) & bad_const_lo, (uint32) ConstMap[mips_reg].value);

	PUSHAD();
	MOV_ImmToReg(1, Reg_ECX, gHardwareState.pc);
	MOV_ImmToReg(1, Reg_EAX, (uint32) & COMPARE_ConstErrorMessage);
	CALL_Reg(Reg_EAX);
	POPAD();

	SetTarget(22);
}

/*
 =======================================================================================================================
    Debug the Dirty only
 =======================================================================================================================
 */
void COMPARE_DebugDirty(x86regtyp *xCMP)
{
	if((xCMP->mips_reg) > 31) return;	/* We won't use this method to compare GPR[32] and GPR[33] (which are mult and
										 * div's LO and HI) */

	/* Compare the lo */
	CMP_RegWithMemory(xCMP->x86reg, (_u32) & gHardwareState_Interpreter_Compare.GPR[xCMP->mips_reg]);
	Jcc_auto(CC_NE, 41);

	/* Compare the hi */
	if(xCMP->Is32bit)
	{
		PUSH_RegToStack(xCMP->x86reg);
		SAR_RegByImm(1, xCMP->x86reg, 31);
		CMP_RegWithMemory(xCMP->x86reg, (_u32) & gHardwareState_Interpreter_Compare.GPR[xCMP->mips_reg] + 4);
		POP_RegFromStack(xCMP->x86reg);
		Jcc_auto(CC_NE, 43);
	}
	else
	{
		CMP_RegWithMemory(xCMP->HiWordLoc, (_u32) & gHardwareState_Interpreter_Compare.GPR[xCMP->mips_reg] + 4);
		Jcc_auto(CC_NE, 43);
	}

	/* Passed */
	JMP_Short_auto(42);

	/* Failed */
	SetTarget(41);
	SetTarget(43);
	MOV_ImmToMemory(1, ModRM_disp32, (uint32) & bad_mips_reg, (uint32) xCMP->mips_reg);

	FlushRegisterToDynaCompare_NoUnmap(xCMP->x86reg);

	PUSHAD();
	MOV_ImmToReg(1, Reg_ECX, gHardwareState.pc);
	MOV_ImmToReg(1, Reg_EDX, gHardwareState.code);
	MOV_ImmToReg(1, Reg_EAX, (uint32) & COMPARE_SomeErrorMessage);
	CALL_Reg(Reg_EAX);
	POPAD();

	SetTarget(42);
}

/*
 =======================================================================================================================
    Runtime test
 =======================================================================================================================
 */
int COMPARE_DebugDirty_TheRest(uint32 fd, uint32 fs, uint32 ft)
{
	if(gHardwareState_Interpreter_Compare.fpr32[ft] != gHardwareState.fpr32[ft])
		return(ft);
	else if(gHardwareState_Interpreter_Compare.fpr32[ft + FR_reg_offset] != gHardwareState.fpr32[ft + FR_reg_offset])
		return(ft);
	else if(gHardwareState_Interpreter_Compare.fpr32[fs] != gHardwareState.fpr32[fs])
		return(fs);
	else if(gHardwareState_Interpreter_Compare.fpr32[fs + FR_reg_offset] != gHardwareState.fpr32[fs + FR_reg_offset])
		return(fs);
	else if(gHardwareState_Interpreter_Compare.fpr32[fd] != gHardwareState.fpr32[fd])
		return(fd);
	else if(gHardwareState_Interpreter_Compare.fpr32[fd + FR_reg_offset] != gHardwareState.fpr32[fd + FR_reg_offset])
		return(fd);
	else if(gHardwareState_Interpreter_Compare.COP1Con[31] != gHardwareState.COP1Con[31])
		return(-1);
	else if(gHardwareState_Interpreter_Compare.LLbit != gHardwareState.LLbit)
		return(-2);

	/* patches for CP0 discrepancy */
	else if(gHardwareState_Interpreter_Compare.COP0Reg[COUNT] != gHardwareState.COP0Reg[COUNT])
		memcpy(&gHardwareState_Interpreter_Compare.COP0Reg[COUNT], &gHardwareState.COP0Reg[COUNT], 4);
	else if(gHardwareState_Interpreter_Compare.COP0Reg[STATUS] != gHardwareState.COP0Reg[STATUS])
		memcpy(&gHardwareState_Interpreter_Compare.COP0Reg[STATUS], &gHardwareState.COP0Reg[STATUS], 4);

	return(0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COMPARE_DebugDirtyFPU(OP_PARAMS)
{
	PUSHAD();

	/* set parameters to pass */
	PUSH_WordToStack(__FT);
	MOV_ImmToReg(1, Reg_ECX, (uint32) __FS);
	MOV_ImmToReg(1, Reg_EDX, (uint32) __FD);

	/* now call the function */
	MOV_ImmToReg(1, Reg_EAX, (uint32) & COMPARE_DebugDirty_TheRest);
	CALL_Reg(Reg_EAX);

	/* get return value in eax */
	MOV_RegToMemory(1, Reg_EAX, ModRM_disp32, (uint32) & bad_mips_reg);
	TEST_Reg2WithReg1(1, Reg_EAX, Reg_EAX);
	POPAD();
	Jcc_auto(CC_NZ, 41);	/* jmp false */

	/* Passed */
	JMP_Short_auto(42);

	/* Failed */
	SetTarget(41);

	PUSHAD();
	MOV_ImmToReg(1, Reg_ECX, gHardwareState.pc);
	MOV_ImmToReg(1, Reg_EDX, gHardwareState.code);
	MOV_ImmToReg(1, Reg_EAX, (uint32) & COMPARE_DirtyFPU_ErrorMessage);
	CALL_Reg(Reg_EAX);
	POPAD();

	SetTarget(42);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void OpcodeDebugger(OP_PARAMS)
{
	/*~~*/
	int k;
	/*~~*/

#ifdef TEST_OPCODE_DEBUGGER_INTEGRITY22
	now_do_dyna_instruction[__OPCODE](PASS_PARAMS);
	return;
#endif
	if(debug_opcode != 1)
	{
		now_do_dyna_instruction[__OPCODE](PASS_PARAMS);
		return;
	}

	if(!CompilingSlot)
		MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &gHardwareState_Interpreter_Compare.pc, gHWS_pc);

	if(debug_opcode_block == 1 && debug_dirty_only == 0)
	{
		now_do_dyna_instruction[__OPCODE](PASS_PARAMS);
		return;
	}

	PUSH_RegToStack(Reg_EBP);
	MOV_MemoryToReg(1, Reg_EBP, ModRM_disp32, (uint32) & gHardwareState.COP0Reg[COUNT]);
	MOV_RegToMemory(1, Reg_EBP, ModRM_disp32, (uint32) & gHardwareState_Interpreter_Compare.COP0Reg[COUNT]);
	POP_RegFromStack(Reg_EBP);

	now_do_dyna_instruction[__OPCODE](PASS_PARAMS);

	if(currentromoptions.Use_Register_Caching == USEREGC_NO) FlushAllRegisters();

	if(debug_dirty_only && currentromoptions.Use_Register_Caching == USEREGC_YES)
	{
		if(ConstMap[xRD->mips_reg].IsMapped == 1) COMPARE_DebugDirtyConst(xRD->mips_reg);
		if(ConstMap[xRT->mips_reg].IsMapped == 1) COMPARE_DebugDirtyConst(xRT->mips_reg);
		if(ConstMap[xRS->mips_reg].IsMapped == 1) COMPARE_DebugDirtyConst(xRS->mips_reg);

		if(xRD->IsDirty)
			COMPARE_DebugDirty(xRD);
		else if(xRT->IsDirty)
			COMPARE_DebugDirty(xRT);
		else if(xRS->IsDirty)
			COMPARE_DebugDirty(xRS);
		else
			COMPARE_DebugDirtyFPU(PASS_PARAMS);
	}
	else
	{
		PUSHAD();
		MOV_ImmToReg(1, Reg_EAX, (uint32) & CompareStates1);
		CALL_Reg(Reg_EAX);

		memcpy(TempConstMap_Debug, ConstMap, sizeof(ConstMap));
		memcpy(Tempx86reg_Debug, x86reg, sizeof(x86reg));
		POPAD();

		for(k = 0; k < 8; k++)
			if(k == Reg_ESP);
			else
				MOV_RegToMemory(1, (unsigned char) k, ModRM_disp32, (uint32) & RegisterRecall[k]);

		FlushAllRegistersToDynaCompare();

		for(k = 0; k < 8; k++)
			if(k == Reg_ESP);
			else
				MOV_MemoryToReg(1, (unsigned char) k, ModRM_disp32, (uint32) & RegisterRecall[k]);

		memcpy(ConstMap, TempConstMap_Debug, sizeof(ConstMap));
		memcpy(x86reg, Tempx86reg_Debug, sizeof(x86reg));

		PUSHAD();
		MOV_ImmToReg(1, Reg_ECX, reg->code);
		MOV_ImmToReg(1, Reg_EAX, (uint32) & CompareStates);
		CALL_Reg(Reg_EAX);
		POPAD();
	}
}

dyn_cpu_instr	dyna_instruction[64] =
{
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
	OpcodeDebugger,
};
