/*$T opcodeDebugger.h GC 1.136 02/28/02 08:33:48 */


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
 * authors: email: schibo@emulation64.com, rice1964@yahoo.com
 */
#ifndef __OPCODE_DEBUGGER_H
#define __OPCODE_DEBUGGER_H

extern uint8			*TLB_sDWord[0x100000];
extern uint8			*sDWord[0x10000];
extern uint8			*sDWord2[0x10000];

extern unsigned _int8	**sDWord_ptr;
extern unsigned _int8	**sDWord_ptr2;
extern unsigned _int8	**TLB_sDWord_ptr;


#ifdef ENABLE_OPCODE_DEBUGGER
#define sDWORD_R		(sDWord_ptr)
#define sDWORD_R_2		(sDWord2_ptr)
#define TLB_sDWORD_R	(TLB_sDWord_ptr)
#define gHWS_GPR			p_gHardwareState->GPR
#define gHWS_COP0Reg		p_gHardwareState->COP0Reg
#define gHWS_fpr32			p_gHardwareState->fpr32
#define gHWS_COP1Reg		p_gHardwareState->COP1Reg
#define gHWS_RememberFprHi	p_gHardwareState->RememberFprHi
#define gHWS_COP1Con		p_gHardwareState->COP1Con
#define gHWS_COP0Con		p_gHardwareState->COP0Con
#define gHWS_LLbit			p_gHardwareState->LLbit
#define gHWS_pc				p_gHardwareState->pc
#define gHWS_code			p_gHardwareState->code
#define gMS_ramRegs0		p_gMemoryState->ramRegs0
#define gMS_ramRegs4		p_gMemoryState->ramRegs4
#define gMS_ramRegs8		p_gMemoryState->ramRegs8
#define gMS_SP_MEM			p_gMemoryState->SP_MEM
#define gMS_SP_REG_1		p_gMemoryState->SP_REG_1
#define gMS_SP_REG_2		p_gMemoryState->SP_REG_2
#define gMS_DPC				p_gMemoryState->DPC
#define gMS_DPS				p_gMemoryState->DPS
#define gMS_MI				p_gMemoryState->MI
#define gMS_VI				p_gMemoryState->VI
#define gMS_AI				p_gMemoryState->AI
#define gMS_PI				p_gMemoryState->PI
#define gMS_RI				p_gMemoryState->RI
#define gMS_SI				p_gMemoryState->SI
#define gMS_RDRAM			p_gMemoryState->RDRAM
#define gMS_C2A1			p_gMemoryState->C2A1
#define gMS_C1A1			p_gMemoryState->C1A1
#define gMS_C1A3			p_gMemoryState->C1A3
#define gMS_C2A2			p_gMemoryState->C2A2
#define gMS_ROM_Image		p_gMemoryState->ROM_Image
#define gMS_GIO_REG			p_gMemoryState->GIO_REG
#define gMS_PIF				p_gMemoryState->PIF
#define gMS_ExRDRAM			p_gMemoryState->ExRDRAM
#define gMS_dummyNoAccess	p_gMemoryState->dummyNoAccess
#define gMS_dummyReadWrite	p_gMemoryState->dummyReadWrite
#define gMS_dummyAllZero	p_gMemoryState->dummyAllZero
#define gMS_TLB				p_gMemoryState->TLB
#define OPCODE_DEBUGGER_EPILOGUE(x) \
	{ \
		int k; \
		for(k = 0; k <= 1; k++) \
		{ \
			if(k == 0 && debug_opcode != 0) COMPARE_SwitchToInterpretive(); \
			if(k == 1) COMPARE_SwitchToDynarec(); \
			x; \
			if(debug_opcode != 1) break; \
		} \
	}
#else
#define sDWORD_R			sDWord
#define sDWORD_R_2			sDWord2
#define TLB_sDWORD_R		TLB_sDWord
#define gHWS_GPR			gHardwareState.GPR
#define gHWS_COP0Reg		gHardwareState.COP0Reg
#define gHWS_fpr32			gHardwareState.fpr32
#define gHWS_COP1Reg		gHardwareState.COP1Reg
#define gHWS_RememberFprHi	gHardwareState.RememberFprHi
#define gHWS_COP1Con		gHardwareState.COP1Con
#define gHWS_COP0Con		gHardwareState.COP0Con
#define gHWS_LLbit			gHardwareState.LLbit
#define gHWS_pc				gHardwareState.pc
#define gHWS_code			gHardwareState.code
#define gMS_ramRegs0		gMemoryState.ramRegs0
#define gMS_ramRegs4		gMemoryState.ramRegs4
#define gMS_ramRegs8		gMemoryState.ramRegs8
#define gMS_SP_MEM			gMemoryState.SP_MEM
#define gMS_SP_REG_1		gMemoryState.SP_REG_1
#define gMS_SP_REG_2		gMemoryState.SP_REG_2
#define gMS_DPC				gMemoryState.DPC
#define gMS_DPS				gMemoryState.DPS
#define gMS_MI				gMemoryState.MI
#define gMS_VI				gMemoryState.VI
#define gMS_AI				gMemoryState.AI
#define gMS_PI				gMemoryState.PI
#define gMS_RI				gMemoryState.RI
#define gMS_SI				gMemoryState.SI
#define gMS_RDRAM			gMemoryState.RDRAM
#define gMS_C2A1			gMemoryState.C2A1
#define gMS_C1A1			gMemoryState.C1A1
#define gMS_C1A3			gMemoryState.C1A3
#define gMS_C2A2			gMemoryState.C2A2
#define gMS_ROM_Image		gMemoryState.ROM_Image
#define gMS_GIO_REG			gMemoryState.GIO_REG
#define gMS_PIF				gMemoryState.PIF
#define gMS_ExRDRAM			gMemoryState.ExRDRAM
#define gMS_dummyNoAccess	gMemoryState.dummyNoAccess
#define gMS_dummyReadWrite	gMemoryState.dummyReadWrite
#define gMS_dummyAllZero	gMemoryState.dummyAllZero
#define gMS_TLB				gMemoryState.TLB
#define OPCODE_DEBUGGER_EPILOGUE(x) x
#endif
void					COMPARE_SwitchToInterpretive(void);
void					COMPARE_SwitchToDynarec(void);

#ifdef ENABLE_OPCODE_DEBUGGER
extern uint8			*sDWORD_R__Debug[0x10000];
extern uint8			*sDWORD_R_2__Debug[0x10000];
#endif
extern uint8			**sDWord_ptr;
extern uint8			**sDWord2_ptr;
extern uint8			**TLB_sDWord_ptr;
extern HardwareState	gHardwareState_Interpreter_Compare;
extern HardwareState	gHardwareState_Flushed_Dynarec_Compare;
extern MemoryState		gMemoryState_Interpreter_Compare;

extern uint32			PcBeforeBranch;
extern uint32			BlockStartPC;
#endif
