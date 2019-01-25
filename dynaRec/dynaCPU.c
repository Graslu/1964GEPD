/*$T dynaCPU.c GC 1.136 03/09/02 16:56:23 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    This file converts integer MIPS® instructions to native x86 machine code by way of calling the functions seen in
    x86.c. Blocks of instructions are compiled at a time. (See compiler.c).
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
#include "../hardware.h"
#include "../r4300i.h"
#include "../n64rcp.h"
#include "regcache.h"
#include "../interrupt.h"
#include "x86.h"
#include "dynaCPU.h"
#include "dynaCPU_defines.h"
#include "dynaLog.h"
#include "../emulator.h"
#include "../compiler.h"

extern void		RefreshOpList(char *opcode);
extern uint32	r4300i_lbu_faster(uint32 QuerAddr);
extern uint32	r4300i_lhu_faster(uint32 QuerAddr);
extern void		r4300i_sb_faster(uint32 QuerAddr);
uint32			r4300i_sb_faster2(uint32 QuerAddr);
extern void		r4300i_sh_faster(uint32 QuerAddr);

#define INTERPRET_STORE_NEW(OPCODE) Interpret_Store_New((uint32) & OPCODE);
#define INTERPRET_LOADSTORE(OPCODE) \
	{ \
		int temp; \
		if(ConstMap[__RS].IsMapped) FlushOneConstant(__RS); \
		if(ConstMap[__RT].IsMapped) FlushOneConstant(__RT); \
		if((temp = CheckWhereIsMipsReg(__RS)) > -1) FlushRegister(temp); \
		if((temp = CheckWhereIsMipsReg(__RT)) > -1) FlushRegister(temp); \
		PushMap(); \
		MOV_ImmToReg(1, Reg_ECX, reg->code); \
		X86_CALL((uint32) & OPCODE); \
		PopMap(); \
	}

#define INTERPRET(OPCODE) \
	FlushAllRegisters(); \
	MOV_ImmToReg(1, Reg_ECX, gHWS_code); \
	X86_CALL((uint32) & OPCODE);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Interpret_Store_New(unsigned _int32 OPCODE)
{
	/*~~~~~~~~~~~~~*/
	int temp, temp2;
	int rt = __dotRT;
	int rs = __dotRS;
	/*~~~~~~~~~~~~~*/

	if(ConstMap[rs].IsMapped)
	{
		temp = ConstMap[rs].value;
		if(ConstMap[rt].IsMapped) FlushOneConstant(rt);
		if((temp2 = CheckWhereIsMipsReg(rt)) > -1) FlushRegister(temp2);
		PUSH_RegIfMapped(Reg_EAX);
		PUSH_RegIfMapped(Reg_ECX);

		MOV_ImmToMemory(1, ModRM_disp32, (_u32) & write_mem_rt, rt);
		MOV_ImmToReg(1, Reg_ECX, temp + (_s32) (_s16) __dotI);
		MOV_ImmToReg(1, Reg_EAX, (uint32) OPCODE);
		CALL_Reg(Reg_EAX);

		POP_RegIfMapped(Reg_ECX);
		POP_RegIfMapped(Reg_EAX);
	}
	else
	{
		if(ConstMap[rt].IsMapped) FlushOneConstant(rt);
		if((temp = CheckWhereIsMipsReg(rt)) > -1) FlushRegister(temp);
		PUSH_RegIfMapped(Reg_EAX);
		PUSH_RegIfMapped(Reg_ECX);

		if((temp = CheckWhereIsMipsReg(rs)) > -1)
			MOV_Reg2ToReg1(1, Reg_ECX, (unsigned char) temp);
		else
			LoadLowMipsCpuRegister(rs, Reg_ECX);	/* some things like this will need rethinking for 2-pass. */

		MOV_ImmToMemory(1, ModRM_disp32, (_u32) & write_mem_rt, rt);
		ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
		MOV_ImmToReg(1, Reg_EAX, (uint32) OPCODE);
		CALL_Reg(Reg_EAX);

		POP_RegIfMapped(Reg_ECX);
		POP_RegIfMapped(Reg_EAX);
	}
}

/* prototypes */
extern BOOL			CPUIsRunning;
extern int __cdecl	SetStatusBar(char *debug, ...);
extern void			HELP_debug(unsigned long pc);
extern _u32			ThisYear;
extern void			FlushConstants(void);
extern void			FlushRegister(int k);
extern void			MapConst(x86regtyp *xMAP, int value);
int					CheckWhereIsMipsReg(int mips_reg);

#include "dynaHelper.h" /* branches and jumps interpretive */
#include "dynabranch.h" /* branches and jumps */

FlushedMap			FlushedRegistersMap[NUM_CONSTS];

int					AlreadyRecompiled;

x86regtyp			xRD[1];
x86regtyp			xRS[1];
x86regtyp			xRT[1];
x86regtyp			xLO[1];
x86regtyp			xHI[1];

dyn_cpu_instr		now_do_dyna_instruction[64] =
{
	dyna4300i_special,
	dyna4300i_regimm,
	dyna4300i_j,
	dyna4300i_jal,
	dyna4300i_beq,
	dyna4300i_bne,
	dyna4300i_blez,
	dyna4300i_bgtz,
	dyna4300i_addi,
	dyna4300i_addiu,
	dyna4300i_slti,
	dyna4300i_sltiu,
	dyna4300i_andi,
	dyna4300i_ori,
	dyna4300i_xori,
	dyna4300i_lui,
	dyna4300i_cop0,
	dyna4300i_cop1,
	dyna4300i_cop2,
	dyna4300i_reserved,
	dyna4300i_beql,
	dyna4300i_bnel,
	dyna4300i_blezl,
	dyna4300i_bgtzl,
	dyna4300i_daddi,
	dyna4300i_daddiu,
	dyna4300i_ldl,
	dyna4300i_ldr,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_lb,
	dyna4300i_lh,
	dyna4300i_lwl,
	dyna4300i_lw,
	dyna4300i_lbu,
	dyna4300i_lhu,
	dyna4300i_lwr,
	dyna4300i_lwu,
	dyna4300i_sb,
	dyna4300i_sh,
	dyna4300i_swl,
	dyna4300i_sw,
	dyna4300i_sdl,
	dyna4300i_sdr,
	dyna4300i_swr,
	dyna4300i_cache,
	dyna4300i_ll,
	dyna4300i_lwc1,
	dyna4300i_lwc2,
	dyna4300i_reserved,
	dyna4300i_lld,
	dyna4300i_ldc1,
	dyna4300i_ldc2,
	dyna4300i_ld,
	dyna4300i_sc,
	dyna4300i_swc1,
	dyna4300i_swc2,
	dyna4300i_reserved,
	dyna4300i_scd,
	dyna4300i_sdc1,
	dyna4300i_sdc2,
	dyna4300i_sd
};

dyn_cpu_instr		dyna_special_instruction[64] =
{
	dyna4300i_special_shift,
	dyna4300i_reserved,
	dyna4300i_special_shift,
	dyna4300i_special_shift,
	dyna4300i_shift_var,
	dyna4300i_reserved,
	dyna4300i_shift_var,
	dyna4300i_shift_var,
	dyna4300i_special_jr,
	dyna4300i_special_jalr,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_special_syscall,
	dyna4300i_special_break,
	dyna4300i_reserved,
	dyna4300i_special_sync,
	dyna4300i_mf_mt,
	dyna4300i_mf_mt,
	dyna4300i_mf_mt,
	dyna4300i_mf_mt,
	dyna4300i_special_dsllv,
	dyna4300i_reserved,
	dyna4300i_special_dsrlv,
	dyna4300i_special_dsrav,
	dyna4300i_special_mul,
	dyna4300i_special_mul,
	dyna4300i_special_div,
	dyna4300i_special_divu,
	dyna4300i_special_dmult,
	dyna4300i_special_dmultu,
	dyna4300i_special_ddiv,
	dyna4300i_special_ddivu,
	dyna4300i_special_add,
	dyna4300i_special_addu,
	dyna4300i_special_sub,
	dyna4300i_special_subu,
	dyna4300i_special_and,
	dyna4300i_special_or,
	dyna4300i_special_xor,
	dyna4300i_special_nor,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_special_slt,
	dyna4300i_special_sltu,
	dyna4300i_special_dadd,
	dyna4300i_special_daddu,
	dyna4300i_special_dsub,
	dyna4300i_special_dsubu,
	dyna4300i_special_tge,
	dyna4300i_special_tgeu,
	dyna4300i_special_tlt,
	dyna4300i_special_tltu,
	dyna4300i_special_teq,
	dyna4300i_reserved,
	dyna4300i_special_tne,
	dyna4300i_reserved,
	dyna4300i_special_dsll,
	dyna4300i_reserved,
	dyna4300i_special_dsrl,
	dyna4300i_special_dsra,
	dyna4300i_special_dsll32,
	dyna4300i_reserved,
	dyna4300i_special_dsrl32,
	dyna4300i_special_dsra32
};

dyn_cpu_instr		dyna_regimm_instruction[32] =
{
	dyna4300i_regimm_bltz,
	dyna4300i_regimm_bgez,
	dyna4300i_regimm_bltzl,
	dyna4300i_regimm_bgezl,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_regimm_tgei,
	dyna4300i_regimm_tgeiu,
	dyna4300i_regimm_tlti,
	dyna4300i_regimm_tltiu,
	dyna4300i_regimm_teqi,
	dyna4300i_reserved,
	dyna4300i_regimm_tnei,
	dyna4300i_reserved,
	dyna4300i_regimm_bltzal,
	dyna4300i_regimm_bgezal,
	dyna4300i_regimm_bltzall,
	dyna4300i_regimm_bgezall,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved
};

dyn_cpu_instr		dyna_cop0_rs_instruction[32] =
{
	dyna4300i_cop0_rs_mf,
	dyna4300i_cop0_rs_dmf,
	dyna4300i_cop0_rs_cf,
	dyna4300i_reserved,
	dyna4300i_cop0_rs_mt,
	dyna4300i_cop0_rs_dmt,
	dyna4300i_cop0_rs_ct,
	dyna4300i_reserved,
	dyna4300i_cop0_rs_bc,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_cop0_tlb,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved
};

dyn_cpu_instr		dyna_cop0_rt_instruction[32] =
{
	dyna4300i_cop0_rt_bcf,
	dyna4300i_cop0_rt_bct,
	dyna4300i_cop0_rt_bcfl,
	dyna4300i_cop0_rt_bctl,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved
};

dyn_cpu_instr		dyna_tlb_instruction[64] =
{
	dyna4300i_invalid,
	dyna4300i_cop0_tlbr,
	dyna4300i_cop0_tlbwi,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_cop0_tlbwr,
	dyna4300i_invalid,
	dyna4300i_cop0_tlbp,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_reserved,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_cop0_eret,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid,
	dyna4300i_invalid
};

dyn_cpu_instr		dyna_cop2_rs_instruction[32] =
{
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented,
	dyna4300i_cop2_rs_not_implemented
};
extern void			COP0_instr(uint32 Instruction);
extern void (*dyna4300i_cop1_Instruction[]) (OP_PARAMS);

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void dyna4300i_cop0(OP_PARAMS)
{
	dyna_cop0_rs_instruction[__RS](PASS_PARAMS);

	/* if (/* eret */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1(OP_PARAMS)
{
	dyna4300i_cop1_Instruction[__RS](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL Init_Dynarec(void)
{
	compilerstatus.lCodePosition = 0;
	AlreadyRecompiled = FALSE;

	CompilingSlot = FALSE;
	compilerstatus.DynaBufferOverError = FALSE;

	memset(dyna_RecompCode, 0x00, RECOMPCODE_SIZE);
	memset(dyna_CodeTable, 0xFF, CODETABLE_SIZE);
	memset(dyna_RecompCode, 0x00, RECOMPCODE_SIZE);

	SetTranslator(dyna_RecompCode, 0, RECOMPCODE_SIZE);

	InitRegisterMap();
#ifdef LOG_DYNA
	InitLogDyna();
#endif
	memset(xRD, 0, sizeof(xRD));
	memset(xRS, 0, sizeof(xRS));
	memset(xRT, 0, sizeof(xRT));
	memset(xLO, 0, sizeof(xLO));
	memset(xHI, 0, sizeof(xHI));
	{
		/*~~*/
		int i;
		/*~~*/

		for(i = 0; i < 0x10000; i++)
		{
			if(dynarommap[i] != NULL)
			{
				memset(dynarommap[i], 0, 0x10000);
			}
		}
	}

	memset(RDRAM_Copy, 0xEE, 0x00800000);
	return(TRUE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Free_Dynarec(void)
{
#ifdef LOG_DYNA
	CloseLogDyna();
#endif
}

#ifdef DEBUG_COMMON
#include "../win32/windebug.h"
extern void HandleBreakpoint(uint32 Instruction);
#endif

/*
 =======================================================================================================================
    This function performs routine interrupt checking £
    Only interrupt will enter here, exceptions will be served otherwhere
 =======================================================================================================================
 */
void rc_Intr_Common(void)
{
	gHWS_COP0Reg[EPC] = gHWS_pc;
	gHWS_COP0Reg[STATUS] |= EXL;	/* set EXL = 1 */
	gHWS_pc = 0x80000180;
	gHWS_COP0Reg[CAUSE] &= NOT_BD;	/* clear BD */
}


void InitRdRsRt(OP_PARAMS)
{
	memset(xRD, 0, sizeof(xRD));
	memset(xRS, 0, sizeof(xRS));
	memset(xRT, 0, sizeof(xRT));
	xRD->mips_reg = __RD;
	xRT->mips_reg = __RT;
	xRS->mips_reg = __RS;

	if(xRS->mips_reg == 0)
		MapConst(xRS, 0);
	else if(xRT->mips_reg == 0)
		MapConst(xRT, 0);
	else if(xRD->mips_reg == 0)
		MapConst(xRD, 0);
}
/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SetRdRsRt32bit(OP_PARAMS)
{
	memset(xRD, 0, sizeof(xRD));
	memset(xRS, 0, sizeof(xRS));
	memset(xRT, 0, sizeof(xRT));
	xRD->mips_reg = __RD;
	xRT->mips_reg = __RT;
	xRS->mips_reg = __RS;
	xRD->Is32bit = 1;
	xRS->Is32bit = 1;
	xRT->Is32bit = 1;

	if(xRS->mips_reg == 0)
		MapConst(xRS, 0);
	else if(xRT->mips_reg == 0)
		MapConst(xRT, 0);
	else if(xRD->mips_reg == 0)
		MapConst(xRD, 0);
}

void Set32bit(OP_PARAMS)
{
	xRD->Is32bit = 1;
	xRS->Is32bit = 1;
	xRT->Is32bit = 1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SetRdRsRt64bit(OP_PARAMS)
{
	memset(xRD, 0, sizeof(xRD));
	memset(xRS, 0, sizeof(xRS));
	memset(xRT, 0, sizeof(xRT));
	xRD->mips_reg = __RD;
	xRT->mips_reg = __RT;
	xRS->mips_reg = __RS;

	if(xRS->mips_reg == 0)
		MapConst(xRS, 0);
	else if(xRT->mips_reg == 0)
		MapConst(xRT, 0);
	else if(xRD->mips_reg == 0)
		MapConst(xRD, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special(OP_PARAMS)
{
	dyna_special_instruction[__F](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm(OP_PARAMS)
{
	dyna_regimm_instruction[__RT](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop2(OP_PARAMS)
{
	dyna_cop2_rs_instruction[__RS](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_rs_bc(OP_PARAMS)
{
	dyna_cop0_rt_instruction[__RT](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_tlb(OP_PARAMS)
{
	dyna_tlb_instruction[__F](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_invalid(OP_PARAMS)
{
	DisplayError("invalid instruction");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_reserved(OP_PARAMS)
{
	INTERPRET(UNUSED);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lwl(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_lwl) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_lwl);
	return;
#endif SAFE_LOADSTORE
	if(__RT != 0) INTERPRET_LOADSTORE(r4300i_lwl);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lwr(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_lwr) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_lwr);
	return;
#endif
	if(__RT != 0) INTERPRET_LOADSTORE(r4300i_lwr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lwu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_lwu) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_lwu);
	return;
#endif
	if(__RT != 0) INTERPRET_LOADSTORE(r4300i_lwu);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_ldl(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_ldl) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_ldl);
	return;
#endif
	if(__RT != 0) INTERPRET_LOADSTORE(r4300i_ldl);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_ldr(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_ldr) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_ldr);
	return;
#endif
	if(__RT != 0) INTERPRET_LOADSTORE(r4300i_ldr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_swl(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_swl) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_swl);
	return;
#endif SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_swl);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_sdl(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_sdl) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_sdl);
	return;
#endif
	INTERPRET_LOADSTORE(r4300i_sdl);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_sdr(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_sdr) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_sdr);
	return;
#endif
	INTERPRET_LOADSTORE(r4300i_sdr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_swr(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_swr) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_swr);
	return;
#endif
	INTERPRET_LOADSTORE(r4300i_swr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_ll(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_ll) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_ll);
	return;
#endif
	INTERPRET_LOADSTORE(r4300i_ll)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lwc2(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
#ifdef SAFE_LOADSTORE
	TRACE1("Unhandled LWC2, pc=%08X", gHWS_pc);
	return;

	/* INTERPRET_LOADSTORE(r4300i_lwc2); return; */
#endif
	TRACE1("Unhandled LWC2, pc=%08X", gHWS_pc);

	/* INTERPRET_LOADSTORE(r4300i_lwc2); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lld(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_lld) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_lld);
	return;
#endif
	INTERPRET_LOADSTORE(r4300i_lld);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_ldc2(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
#ifdef SAFE_LOADSTORE
	TRACE1("Unhandled ldc2, pc=%08X", gHWS_pc);
	return;

	/* INTERPRET_LOADSTORE(r4300i_ldc2); return; */
#endif
	TRACE1("Unhandled ldc2, pc=%08X", gHWS_pc);

	/* INTERPRET_LOADSTORE(r4300i_ldc2); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_sc(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_sc) SetRdRsRt64bit(PASS_PARAMS);

	INTERPRET_LOADSTORE(r4300i_sc);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_swc2(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
#ifdef SAFE_LOADSTORE
	TRACE1("Unhandled swc2, pc=%08X", gHWS_pc);
	return;

	/* INTERPRET_LOADSTORE(r4300i_swc2); return; */
#endif
	TRACE1("Unhandled swc2, pc=%08X", gHWS_pc);

	/* INTERPRET_LOADSTORE(r4300i_swc2); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_scd(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_scd) SetRdRsRt64bit(PASS_PARAMS);

	INTERPRET_LOADSTORE(r4300i_scd);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_sdc2(OP_PARAMS)
{
	/* INTERPRET_LOADSTORE(r4300i_sdc2); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_daddi(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_daddi) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_DOUBLE_IMM
	INTERPRET(r4300i_daddi);
	return;
#endif SAFE_DOUBLE_IMM
	if((CheckIs32Bit(xRS->mips_reg)) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		INTERPRET(r4300i_daddi_32bit);
	}
	else
	{
		INTERPRET(r4300i_daddi);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_daddiu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_daddiu) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_DOUBLE_IMM
	INTERPRET(r4300i_daddiu);
	return;
#endif SAFE_DOUBLE_IMM
	if((CheckIs32Bit(xRS->mips_reg)) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		INTERPRET(r4300i_daddiu_32bit);
	}
	else
	{
		INTERPRET(r4300i_daddiu);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dadd(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_dadd) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_DOUBLE_MATH
	INTERPRET(r4300i_dadd);
	return;
#endif SAFE_DOUBLE_MATH
	if
	(
		(CheckIs32Bit(xRS->mips_reg) && CheckIs32Bit(xRT->mips_reg))
	||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
	)
	{
		INTERPRET(r4300i_dadd_32bit);
	}
	else
	{
		INTERPRET(r4300i_dadd);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_daddu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_daddu) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_DOUBLE_MATH
	INTERPRET(r4300i_daddu);
	return;
#endif SAFE_DOUBLE_MATH
	if
	(
		(CheckIs32Bit(xRS->mips_reg) && CheckIs32Bit(xRT->mips_reg))
	||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
	)
	{
		INTERPRET(r4300i_daddu_32bit);
	}
	else
	{
		INTERPRET(r4300i_daddu);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dsub(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_dsub) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_DOUBLE_MATH
	INTERPRET(r4300i_dsub);
	return;
#endif SAFE_DOUBLE_MATH
	if
	(
		(CheckIs32Bit(xRS->mips_reg) && CheckIs32Bit(xRT->mips_reg))
	||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
	)
	{
		dyna4300i_special_sub(PASS_PARAMS);
	}
	else
	{
		INTERPRET(r4300i_dsub);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dsubu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_dsubu) SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_DOUBLE_MATH
	INTERPRET(r4300i_dsubu);
	return;
#endif SAFE_DOUBLE_MATH
	if
	(
		(CheckIs32Bit(xRS->mips_reg) && CheckIs32Bit(xRT->mips_reg))
	||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
	)
	{
		dyna4300i_special_subu(PASS_PARAMS);
	}
	else
	{
		INTERPRET(r4300i_dsubu);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_tge(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_tge);
	SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_tge);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_tgeu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_tgeu) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_tgeu);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_tlt(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_tlt) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_tlt);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_tltu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_tltu) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_tltu);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_teq(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_teq) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_teq);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_tne(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_tne) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_tne);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_tgei(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_tgei) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_tgei);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_tgeiu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_tgeiu) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_tgeiu);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_tlti(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_tlti) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_tlti);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_tltiu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_tltiu) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_tltiu);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_teqi(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_teqi) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_teqi);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_regimm_tnei(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_tnei) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_tnei);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dsllv(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_dsllv)
#ifdef SAFE_DOUBLE_SHIFTS
	INTERPRET(r4300i_dsllv);
	return;
#endif SAFE_DOUBLE_SHIFTS
	SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_dsllv);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dsrlv(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_dsrlv)
#ifdef SAFE_DOUBLE_SHIFTS
	INTERPRET(r4300i_dsrlv);
	return;
#endif SAFE_DOUBLE_SHIFTS

	/* do not assume 32bit with right-shift logical. The signature can change. */
	SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_dsrlv);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dsrav(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_dsrav)
#ifdef SAFE_DOUBLE_SHIFTS
	INTERPRET(r4300i_dsrav);
	return;
#endif SAFE_DOUBLE_SHIFTS
	SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_dsrav);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dsll(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_dsll)
#ifdef SAFE_DOUBLE_SHIFTS
	INTERPRET(r4300i_dsll);
	return;
#endif SAFE_DOUBLE_SHIFTS

	/* Do not assume 32bit with left-shifts. The signature can change. */
	SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_dsll);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dsrl(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_dsrl)
#ifdef SAFE_DOUBLE_SHIFTS
	INTERPRET(r4300i_dsrl);
	return;
#endif SAFE_DOUBLE_SHIFTS

	/* do not assume 32bit with right-shift logical. The signature can change. */
	SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_dsrl);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dsra(OP_PARAMS)
{
	SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_DOUBLE_SHIFTS
	INTERPRET(r4300i_dsra);
	return;
#endif SAFE_DOUBLE_SHIFTS
	if((CheckIs32Bit(xRT->mips_reg)) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		compilerstatus.cp0Counter += 1;
		_SAFTY_CPU_(r4300i_dsra) compilerstatus.cp0Counter -= 1;	/* special_shift() adds 1 to counter. */
		dyna4300i_special_shift(PASS_PARAMS);
	}
	else
	{
		compilerstatus.cp0Counter += 1;
		_SAFTY_CPU_(r4300i_dsra) INTERPRET(r4300i_dsra);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dmult(OP_PARAMS)
{
	compilerstatus.cp0Counter += 2;

	_SAFTY_CPU_(r4300i_dmult)
#ifdef SAFE_DOUBLE_MATH
	INTERPRET(r4300i_dmult);
	return;
#endif SAFE_DOUBLE_MATH
	SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_dmult);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dmultu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 2;

	_SAFTY_CPU_(r4300i_dmultu)
#ifdef SAFE_DOUBLE_MATH
	INTERPRET(r4300i_dmultu);
	return;
#endif SAFE_DOUBLE_MATH
	SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_dmultu);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_ddiv(OP_PARAMS)
{
	SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_DOUBLE_MATH
	INTERPRET(r4300i_ddiv);
	return;
#endif SAFE_DOUBLE_MATH
	if
	(
		(CheckIs32Bit(xRS->mips_reg))
	&&	(CheckIs32Bit(xRT->mips_reg))
	||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
	)
	{
		compilerstatus.cp0Counter += 139;	/* no overlap. */
		_SAFTY_CPU_(r4300i_ddiv) compilerstatus.cp0Counter -= 75;	/* div() adds 75 to count. */
		dyna4300i_special_div(PASS_PARAMS);
	}
	else
	{
		compilerstatus.cp0Counter += 139;	/* no overlap. */
		_SAFTY_CPU_(r4300i_ddiv) INTERPRET(r4300i_ddiv);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_ddivu(OP_PARAMS)
{
	SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_DOUBLE_MATH
	INTERPRET(r4300i_ddivu);
	return;
#endif SAFE_DOUBLE_MATH
	if
	(
		(CheckIs32Bit(xRS->mips_reg))
	&&	(CheckIs32Bit(xRT->mips_reg))
	||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
	)
	{
		compilerstatus.cp0Counter += 139;
		_SAFTY_CPU_(r4300i_ddivu) compilerstatus.cp0Counter -= 75;	/* divu() adds 75 to count. */
		dyna4300i_special_divu(PASS_PARAMS);
	}
	else
	{
		compilerstatus.cp0Counter += 139;
		_SAFTY_CPU_(r4300i_ddivu) INTERPRET(r4300i_ddivu);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_rs_mf(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_COP0_mfc0) SetRdRsRt32bit(PASS_PARAMS);

	INTERPRET(r4300i_COP0_mfc0);
}

/*
 =======================================================================================================================
    Is this an op???????????????????????????????????
 =======================================================================================================================
 */
void dyna4300i_cop0_rs_dmf(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	rd = (unsigned long) &(_u32) reg->COP0Reg[__RD];
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/* SAFTY_CPU_(r4300i_COP0_dmfc0) */
	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);

	/* SAFTY_CPU_(r4300i_cop0_rs_dmf) */
	if(xRT->mips_reg == 0) return;

	xRT->NoNeedToLoadTheLo = 1;
	xRT->IsDirty = 1;
	MapRT;
	MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, rd);
}

/*
 =======================================================================================================================
    Is this an op???????????????????????????????????
 =======================================================================================================================
 */
void dyna4300i_cop0_rs_cf(OP_PARAMS)
{
	SetRdRsRt32bit(PASS_PARAMS);

	/* _SAFTY_CPU_(r4300i_cop0_rs_cf) */
	compilerstatus.cp0Counter += 1;
	if(xRT->mips_reg == 0) return;

	xRT->IsDirty = 1;
	xRT->NoNeedToLoadTheLo = 1;
	MapRT;
	MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (unsigned long) &reg->COP0Con[__FS]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_rs_mt(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_COP0_mtc0) SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_COP0_mtc0);
}

/*
 =======================================================================================================================
    Is this an op???????????????????????????????????
 =======================================================================================================================
 */
void dyna4300i_cop0_rs_dmt(OP_PARAMS)
{
	/* SAFTY_CPU_(r4300i_cop0_rs_dmt) */
	compilerstatus.cp0Counter += 1;
	DisplayError("%08X: Unhandled dmtc0", reg->pc);

	/* INTERPRET(r4300i_COP0_dmtc0); */
}

/*
 =======================================================================================================================
    Is this an op???????????????????????????????????
 =======================================================================================================================
 */
void dyna4300i_cop0_rs_ct(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_u32	fs = (unsigned long) &reg->COP0Con[__FS];
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);

	/* SAFTY_CPU_(r4300i_COP0_rs_ct) */
	if(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
	{
		DisplayError("Need to compile ctc0 for 32bit. Please use 64bit for now.");
	}

	MapRT;
	MOV_RegToMemory(1, xRT->x86reg, ModRM_disp32, fs);
	MOV_RegToMemory(1, xRT->HiWordLoc, ModRM_disp32, fs + 4);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_rt_bcf(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	DisplayError("%08X: Unhandled BCFC0", reg->pc);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_rt_bct(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	DisplayError("%08X: Unhandled BCTC0", reg->pc);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_rt_bcfl(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	DisplayError("%08X: Unhandled BCFL0", reg->pc);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_rt_bctl(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	DisplayError("%08X: Unhandled BCTL0", reg->pc);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_tlbr(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_COP0_tlbr) SetRdRsRt64bit(PASS_PARAMS);

	INTERPRET(r4300i_COP0_tlbr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_tlbwi(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_COP0_tlbwi) SetRdRsRt64bit(PASS_PARAMS);

	INTERPRET(r4300i_COP0_tlbwi);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_tlbwr(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_COP0_tlbwr) SetRdRsRt64bit(PASS_PARAMS);

	INTERPRET(r4300i_COP0_tlbwr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop0_tlbp(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_COP0_tlbp) SetRdRsRt64bit(PASS_PARAMS);

	INTERPRET(r4300i_COP0_tlbp);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop2_rs_not_implemented(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	TRACE1("%08X: There isnt a COP2\n", reg->pc);
}

/*
 =======================================================================================================================
    This function is called by add and addi
 =======================================================================================================================
 */
void DoConstAddi(_int32 Constant)
{
	if(ConstMap[xRS->mips_reg].IsMapped == 1)
		MapConst(xRT, ((_s32) ConstMap[xRS->mips_reg].value + (_s32) Constant));
	else if(xRT->mips_reg == xRS->mips_reg)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		int temp = CheckIs32Bit(xRT->mips_reg);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		if((temp) && (Constant == 0)) return;

		xRT->IsDirty = 1;
		MapRT;

		ADD_ImmToReg(1, xRT->x86reg, Constant);
	}
	else
	{
		ConstMap[xRT->mips_reg].IsMapped = 0;
		xRT->IsDirty = 1;
		xRT->NoNeedToLoadTheLo = 1;
		MapRT

		/* we checked if rs is a const above already. good. */
		MapRS_To(xRT, 1, MOV_MemoryToReg) ADD_ImmToReg(1, xRT->x86reg, Constant);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_addi(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	if(xRT->mips_reg == 0) return;

	_SAFTY_CPU_(r4300i_addi)
#ifdef SAFE_IMM
	INTERPRET(r4300i_addi);
	return;
#endif
	DoConstAddi((_s32) (_s16) __I);

	/* TODO: Check 32bit overflow */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_addiu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	if(xRT->mips_reg == 0) return;

	_SAFTY_CPU_(r4300i_addiu)
#ifdef SAFE_IMM
	INTERPRET(r4300i_addiu);
	return;
#endif
	DoConstAddi((_s32) (_s16) __I);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_slt(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);

	_SAFTY_CPU_(r4300i_slt) if(xRD->mips_reg == 0)
		return;
#ifdef SAFE_SLT
	INTERPRET(r4300i_slt);
	return;
#endif
	if(xRS->mips_reg == xRT->mips_reg)
	{
		MapConst(xRD, 0);
	}

	/* 32bit */
	else if
		(
			(CheckIs32Bit(xRS->mips_reg) && CheckIs32Bit(xRT->mips_reg))
		||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
		)
	{
		xRD->IsDirty = 1;
		xRD->Is32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
		if((xRD->mips_reg != xRS->mips_reg) && (xRD->mips_reg != xRT->mips_reg))
		{
			xRD->NoNeedToLoadTheLo = 1;
		}

		MapRD;
		MapRS;
		MapRT;

		CMP_Reg2WithReg1(1, xRS->x86reg, xRT->x86reg);
		Jcc_auto(CC_GE, 5);

		MOV_ImmToReg(1, xRD->x86reg, 1);
		JMP_Short_auto(6);

		SetTarget(5);

		XOR_Reg2ToReg1(1, xRD->x86reg, xRD->x86reg);

		SetTarget(6);
	}

	/* 64bit */
	else
	{
		xRD->IsDirty = 1;

		if((xRD->mips_reg != xRS->mips_reg) && (xRD->mips_reg != xRT->mips_reg))
		{
			xRD->Is32bit = 1;
			xRD->NoNeedToLoadTheLo = 1;
		}

		MapRD;
		MapRS;
		MapRT;

		CMP_Reg2WithReg1(1, xRS->HiWordLoc, xRT->HiWordLoc);

		Jcc_auto(CC_G, 2);
		Jcc_auto(CC_L, 1);

		CMP_Reg2WithReg1(1, xRS->x86reg, xRT->x86reg);
		Jcc_auto(CC_AE, 3);

		SetTarget(1);
		MOV_ImmToReg(1, xRD->x86reg, 1);
		JMP_Short_auto(4);

		SetTarget(2);
		SetTarget(3);

		XOR_Reg2ToReg1(1, xRD->x86reg, xRD->x86reg);

		SetTarget(4);

		if(xRD->Is32bit == 0)
		{
			xRD->Is32bit = 1;
			MapRD;	/* converts to 32bit */
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_sltu(OP_PARAMS)
{
	int Use32bit = 0;

	compilerstatus.cp0Counter += 1;

	InitRdRsRt(PASS_PARAMS);
	Use32bit = ( (CheckIs32Bit(xRS->mips_reg) && CheckIs32Bit(xRT->mips_reg)) | (currentromoptions.Assume_32bit == ASSUME_32BIT_YES));
	if (Use32bit)Set32bit(PASS_PARAMS);

	_SAFTY_CPU_(r4300i_sltu) if(xRD->mips_reg == 0)
		return;
#ifdef SAFE_SLT
	INTERPRET(r4300i_sltu);
	return;
#endif
	if(xRS->mips_reg == xRT->mips_reg)
	{
		MapConst(xRD, 0);
	}

	/* 32bit */
	else if	(Use32bit)
	{
		xRD->IsDirty = 1;
		if((xRD->mips_reg != xRS->mips_reg) && (xRD->mips_reg != xRT->mips_reg))
		{
			xRD->NoNeedToLoadTheLo = 1;
		}

		MapRD;
		MapRS;
		MapRT;

		CMP_Reg2WithReg1(1, xRS->x86reg, xRT->x86reg);
		Jcc_auto(CC_AE, 5);

		MOV_ImmToReg(1, xRD->x86reg, 1);
		JMP_Short_auto(6);

		SetTarget(5);

		XOR_Reg2ToReg1(1, xRD->x86reg, xRD->x86reg);

		SetTarget(6);
	}

	/* 64bit */
	else
	{
		if((xRD->mips_reg != xRS->mips_reg) && (xRD->mips_reg != xRT->mips_reg))
		{
			xRD->Is32bit = 1;
			xRD->NoNeedToLoadTheLo = 1;
		}

		xRD->IsDirty = 1;
		MapRD;
		MapRS;
		MapRT;

		CMP_Reg2WithReg1(1, xRS->HiWordLoc, xRT->HiWordLoc);

		Jcc_auto(CC_A, 2);
		Jcc_auto(CC_B, 1);

		CMP_Reg2WithReg1(1, xRS->x86reg, xRT->x86reg);
		Jcc_auto(CC_AE, 3);

		SetTarget(1);
		MOV_ImmToReg(1, xRD->x86reg, 1);
		JMP_Short_auto(4);

		SetTarget(2);
		SetTarget(3);

		XOR_Reg2ToReg1(1, xRD->x86reg, xRD->x86reg);

		SetTarget(4);

		if(xRD->Is32bit == 0)
		{
			xRD->Is32bit = 1;
			MapRD;
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_slti(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_int32	ConstInt = (_int32) __I;
	int		Is32bit = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;

	InitRdRsRt(PASS_PARAMS);
	Is32bit = ( CheckIs32Bit(xRS->mips_reg) | (currentromoptions.Assume_32bit == ASSUME_32BIT_YES));
	if (Is32bit)Set32bit(PASS_PARAMS);

	_SAFTY_CPU_(r4300i_slti) if(xRT->mips_reg == 0)
		return;
#ifdef SAFE_SLT
	INTERPRET(r4300i_slti);
	return;
#endif
	if(Is32bit)
	{
		xRT->Is32bit = 1;
		xRS->Is32bit = 1;
	}

	if(ConstMap[xRS->mips_reg].IsMapped)
	{
		if((_int32) ConstMap[xRS->mips_reg].value >= (_int32) ConstInt)
			MapConst(xRT, 0);
		else
			MapConst(xRT, 1);
	}
	else if(xRT->mips_reg == xRS->mips_reg)
	{
		xRT->IsDirty = 1;
		MapRT;

		if(!Is32bit)
		{
			CMP_RegWithImm(1, xRT->HiWordLoc, (uint32) ((_int32) ConstInt >> 31));
			Jcc_auto(CC_G, 2);
			Jcc_auto(CC_L, 1);
		}

		if(!Is32bit)
		{
			CMP_RegWithImm(1, xRT->x86reg, (_u32) ConstInt);
			Jcc_auto(CC_AE, 3);
		}
		else
		{
			CMP_RegWithImm(1, xRT->x86reg, (_u32) ConstInt);
			Jcc_auto(CC_GE, 3);
		}

		if(!Is32bit)
		{
			SetTarget(1);
		}

		MOV_ImmToReg(1, xRT->x86reg, 1);

		JMP_Short_auto(4);

		if(!Is32bit)
		{
			SetTarget(2);
		}

		SetTarget(3);

		XOR_Reg2ToReg1(1, xRT->x86reg, xRT->x86reg);

		SetTarget(4);
		if(!Is32bit)
		{
			XOR_Reg2ToReg1(1, xRT->HiWordLoc, xRT->HiWordLoc);
		}
	}
	else	/* rt != rs */
	{
		xRT->Is32bit = 1;
		xRT->NoNeedToLoadTheLo = 1;
		xRT->IsDirty = 1;
		MapRT;
		MapRS;

		XOR_Reg2ToReg1(1, xRT->x86reg, xRT->x86reg);
		if(!Is32bit)
		{
			CMP_RegWithImm(1, xRS->HiWordLoc, (uint32) ((_int32) ConstInt >> 31));
			Jcc_auto(CC_G, 2);
			Jcc_auto(CC_L, 1);
		}

		if(!Is32bit)
		{
			CMP_RegWithImm(1, xRS->x86reg, (_u32) ConstInt);
			Jcc_auto(CC_AE, 4);
		}
		else
		{
			CMP_RegWithImm(1, xRS->x86reg, (_u32) ConstInt);
			Jcc_auto(CC_GE, 4);
		}

		if(!Is32bit)
		{
			SetTarget(1);
		}

		INC_Reg(1, xRT->x86reg);

		if(!Is32bit)
		{
			SetTarget(2);
		}

		SetTarget(4);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_sltiu(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_s64	ConstInt = (_s64) (_s32) __I;
	int		Is32bit = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;

	InitRdRsRt(PASS_PARAMS);
	Is32bit = ( CheckIs32Bit(xRS->mips_reg) | (currentromoptions.Assume_32bit == ASSUME_32BIT_YES));
	if (Is32bit)Set32bit(PASS_PARAMS);

	_SAFTY_CPU_(r4300i_sltiu) if(xRT->mips_reg == 0)
		return;
#ifdef SAFE_SLT
	INTERPRET(r4300i_sltiu);
	return;
#endif

	if(ConstMap[xRS->mips_reg].IsMapped)
	{
		if((uint32) ConstMap[xRS->mips_reg].value >= (uint32) ConstInt)
			MapConst(xRT, 0);
		else
			MapConst(xRT, 1);
	}
	else if(xRT->mips_reg == xRS->mips_reg)
	{
		xRT->IsDirty = 1;
		MapRT;

		if(!Is32bit)
		{
			CMP_RegWithImm(1, xRT->HiWordLoc, (_u32) (ConstInt >> 32));
			Jcc_auto(CC_A, 2);
			Jcc_auto(CC_B, 1);
		}

		CMP_RegWithImm(1, xRT->x86reg, (_u32) ConstInt);
		Jcc_auto(CC_AE, 3);

		if(!Is32bit)
		{
			SetTarget(1);
		}

		MOV_ImmToReg(1, xRT->x86reg, 1);

		JMP_Short_auto(4);

		if(!Is32bit)
		{
			SetTarget(2);
		}

		SetTarget(3);

		XOR_Reg2ToReg1(1, xRT->x86reg, xRT->x86reg);

		SetTarget(4);
		if(!Is32bit)
		{
			XOR_Reg2ToReg1(1, xRT->HiWordLoc, xRT->HiWordLoc);
		}
	}
	else	/* rt != rs :) */
	{
		xRT->IsDirty = 1;
		xRT->Is32bit = 1;
		xRT->NoNeedToLoadTheLo = 1;
		MapRT;
		MapRS;

		XOR_Reg2ToReg1(1, xRT->x86reg, xRT->x86reg);
		if(!Is32bit)
		{
			CMP_RegWithImm(1, xRS->HiWordLoc, (_u32) (ConstInt >> 32));
			Jcc_auto(CC_A, 2);
			Jcc_auto(CC_B, 1);
		}

		CMP_RegWithImm(1, xRS->x86reg, (_u32) ConstInt);
		Jcc_auto(CC_AE, 4);

		if(!Is32bit)
		{
			SetTarget(1);
		}

		INC_Reg(1, xRT->x86reg);

		if(!Is32bit)
		{
			SetTarget(2);
		}

		SetTarget(4);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_andi(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_andi) 
		
	if(xRT->mips_reg == 0)
		return;
#ifdef SAFE_IMM
	INTERPRET(r4300i_andi);
	return;
#endif
	if(ConstMap[xRS->mips_reg].IsMapped)
		MapConst(xRT, (ConstMap[xRS->mips_reg].value & (_u32) (_u16) __I));
	else if(xRS->mips_reg == xRT->mips_reg)
	{
		xRT->IsDirty = 1;
		MapRT;

		AND_ImmToReg(1, xRT->x86reg, (_u32) (_u16) __I);
	}
	else
	{
		xRT->IsDirty = 1;
		xRT->NoNeedToLoadTheLo = 1;
		MapRT;

		/* we checked if rs is a const above already. good. */
		MapRS_To(xRT, 1, MOV_MemoryToReg);

		AND_ImmToReg(1, xRT->x86reg, (_u32) (_u16) __I);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_ori(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	int Use32bit = 0;
	/*~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_ori)
#ifdef SAFE_IMM
	INTERPRET(r4300i_ori);
	return;
#endif
	if(xRT->mips_reg == 0) return;

	if(CheckIs32Bit(xRS->mips_reg) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
		Use32bit = 1;
	}

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		MapConst(xRT, (ConstMap[xRS->mips_reg].value | (_u32) (_u16) __I));
	}
	else if(xRS->mips_reg == xRT->mips_reg)
	{
		Use32bit = CheckIs32Bit(xRT->mips_reg);
		if((((_u32) (_u16) __I) == 0) && (Use32bit)) return;

		xRT->IsDirty = 1;
		xRT->Is32bit = Use32bit;

		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		MapRT	OR_ImmToReg(1, xRT->x86reg, (_u32) (_u16) __I);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	}
	else
	{
		xRT->IsDirty = 1;
		xRT->NoNeedToLoadTheLo = 1;
		xRT->NoNeedToLoadTheHi = 1;
		MapRT;

		/* we checked if rs is a const above already. good. */
		MapRS_To(xRT, Use32bit, MOV_MemoryToReg);
		OR_ImmToReg(1, xRT->x86reg, (_u32) (_u16) __I);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_xori(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	int Use32bit = 0;
	/*~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_xori)
	/* 64bit */
	if(xRT->mips_reg == 0) return;
#ifdef SAFE_IMM
	INTERPRET(r4300i_xori);
	return;
#endif
	if(CheckIs32Bit(xRS->mips_reg) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		Use32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
		MapConst(xRT, (ConstMap[xRS->mips_reg].value ^ (uint32) (uint16) (__I)));
	else if(xRS->mips_reg == xRT->mips_reg)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint32	Constant = (uint32) (uint16) __I;
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		Use32bit = CheckIs32Bit(xRT->mips_reg);
		if((Constant == 0) && (Use32bit == 1)) return;

		xRT->IsDirty = 1;
		xRT->Is32bit = Use32bit;

		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		MapRT	XOR_ImmToReg(1, xRT->x86reg, (_u32) (_u16) __I);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	}
	else
	{
		/*
		 * if (xRS->mips_reg == 0){TODO: this optimization..doesn't seem to be used much
		 * tho. }
		 */
		xRT->IsDirty = 1;
		xRT->NoNeedToLoadTheLo = 1;
		xRT->NoNeedToLoadTheHi = 1;
		MapRT;

		/* we checked if rs is a const above already. good. */
		MapRS_To(xRT, Use32bit, MOV_MemoryToReg);

		if(((_u32) (_u16) __I) != 0) XOR_ImmToReg(1, xRT->x86reg, (_u32) (_u16) __I);
	}
}

extern x86regtyp	x86reg[];

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void dyna4300i_lui(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_lui) SetRdRsRt32bit(PASS_PARAMS);

#ifdef SAFE_IMM
	INTERPRET(r4300i_lui);
	return;
#endif SAFE_IMM
	MapConst(xRT, (_s32) ((__I) << 16));
}

extern _int32	r4300i_lh_faster(uint32 QuerAddr);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lh(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	_u32	QuerAddr;
	_u16	value;
	/*~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_lh);

	if(xRT->mips_reg == 0) return;

#ifdef SAFE_LOADSTORE
	/* INTERPRET_LOADSTORE(r4300i_lh); return; */
#endif
	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		__try
		{
			QuerAddr = (_u32) ((((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I)));
			if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

			value = LOAD_SHALF_PARAM(QuerAddr);
			if(xRT->mips_reg != 0)	/* mandatory */
			{
				if(ConstMap[xRT->mips_reg].IsMapped == 1)
				{
					ConstMap[xRT->mips_reg].IsMapped = 0;
				}

				xRT->IsDirty = 1;
				xRT->NoNeedToLoadTheLo = 1;
				MapRT;

				/* loads aligned, and loads 32bit. very cool. */
				switch(QuerAddr & 3)
				{
				case 0:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					SAR_RegByImm(1, xRT->x86reg, 16);
					break;
				case 2:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					SHL_RegByImm(1, xRT->x86reg, 16);
					SAR_RegByImm(1, xRT->x86reg, 16);
					break;
				default:
					MOVSX_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_SHALF_PARAM(QuerAddr));
				}
			}
			else
				goto _Default;
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			goto _Default;
		}
	}
	else
	{
_Default:
		if(ConstMap[xRS->mips_reg].IsMapped)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			int temp = ConstMap[xRS->mips_reg].value;
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			xRT->IsDirty = 1;
			xRT->NoNeedToLoadTheLo = 1;
			MapRT;
			if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
			if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);

			MOV_ImmToReg(1, Reg_EAX, (temp + (_s32) (_s16) __dotI) ^ 2);
			MOV_ImmToReg(1, Reg_ECX, ((uint32) (temp + (_s32) (_s16) __dotI)) >> SHIFTER2_READ);

			WC16(0x14FF);
			WC8(0x8D);
			WC32((uint32) & memory_read_functions);
			LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
			if(xRT->mips_reg != 0) MOVSX_MemoryToReg(1, xRT->x86reg, ModRM_EAX, 0);
			if(xRT->x86reg != Reg_ECX) POP_RegIfMapped(Reg_ECX);
			if(xRT->x86reg != Reg_EAX) POP_RegIfMapped(Reg_EAX);
		}
		else
		{
			/*~~~~~*/
			int temp;
			/*~~~~~*/

			xRT->IsDirty = 1;
			if(xRT->mips_reg != xRS->mips_reg) xRT->NoNeedToLoadTheLo = 1;
			MapRT;

			/* we checked if rs is a const above already. good. */
			if
			(
				((temp = CheckWhereIsMipsReg(xRS->mips_reg)) == -1)
			&&	(ConstMap[xRS->mips_reg].FinalAddressUsedAt <= gHWS_pc)
			)
			{
				FetchEBP_Params(xRS->mips_reg);
				if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
				if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);
				MOV_MemoryToReg(1, Reg_ECX, x86params.ModRM, x86params.Address);
			}
			else
			{
				MapRS;
				if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
				if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);
				MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);
			}

			ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
			MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
			SHR_RegByImm(1, Reg_ECX, SHIFTER2_READ);
			XOR_ImmToReg(1, Reg_EAX, 2);
			WC16(0x14FF);
			WC8(0x8D);
			WC32((uint32) & memory_read_functions);
			LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
			if(xRT->mips_reg != 0) MOVSX_MemoryToReg(1, xRT->x86reg, ModRM_EAX, 0);
			if(xRT->x86reg != Reg_ECX) POP_RegIfMapped(Reg_ECX);
			if(xRT->x86reg != Reg_EAX) POP_RegIfMapped(Reg_EAX);
		}
	}
}

extern char * ((**phys_read_fast) (_u32 addr));
extern char * ((**phys_write_fast) (_u32 addr));

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MemoryCase1(void)
{
	if((xRT->x86reg != Reg_EAX) && (xRT->HiWordLoc != Reg_EAX))
	{
		PUSH_RegIfMapped(Reg_EAX);

		MOV_Reg2ToReg1(1, xRT->x86reg, xRS->x86reg);
		ADD_ImmToReg(1, xRT->x86reg, (_s32) (_s16) __dotI);
		MOV_Reg2ToReg1(1, Reg_EAX, xRT->x86reg);
		SHR_RegByImm(1, xRT->x86reg, SHIFTER2_READ);
		WC16(0x14FF);
		WC8((uint8) (0x85 | (xRT->x86reg << 3)));
		WC32((uint32) & memory_read_functions);
	}
	else
	{
		if(xRS->x86reg != xRT->x86reg) PUSH_RegToStack(xRS->x86reg);

		ADD_ImmToReg(1, xRS->x86reg, (_s32) (_s16) __dotI);
		MOV_Reg2ToReg1(1, Reg_EAX, xRS->x86reg);
		SHR_RegByImm(1, xRS->x86reg, SHIFTER2_READ);
		WC16(0x14FF);
		WC8((uint8) (0x85 | (xRS->x86reg << 3)));
		WC32((uint32) & memory_read_functions);
	}

	LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MemoryCase2(void)
{
	if(xRT->x86reg != Reg_EAX)
	{
		/* we better have checked if rs is mapped above. */
		MapRS_To(xRT, 1, MOV_MemoryToReg);
		PUSH_RegIfMapped(Reg_EAX);
		ADD_ImmToReg(1, xRT->x86reg, (_s32) (_s16) __dotI);
		MOV_Reg2ToReg1(1, Reg_EAX, xRT->x86reg);
		SHR_RegByImm(1, xRT->x86reg, SHIFTER2_READ);
		WC16(0x14FF);
		WC8((uint8) (0x85 | (xRT->x86reg << 3)));
		WC32((uint32) & memory_read_functions);
	}
	else
	{
		MapRS;
		if(xRS->x86reg != xRT->x86reg) PUSH_RegToStack(xRS->x86reg);

		ADD_ImmToReg(1, xRS->x86reg, (_s32) (_s16) __dotI);
		MOV_Reg2ToReg1(1, Reg_EAX, xRS->x86reg);
		SHR_RegByImm(1, xRS->x86reg, SHIFTER2_READ);
		WC16(0x14FF);
		WC8((uint8) (0x85 | (xRS->x86reg << 3)));
		WC32((uint32) & memory_read_functions);
	}

	LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
}

extern x86regtyp	x86reg[8];

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lw(OP_PARAMS)
{
	/*~~~~~~~~~~*/
	_s32	value;
	/*~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_lw) SetRdRsRt32bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	goto _Default;
#endif
	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		/*~~~~~~~~~~~~~*/
		uint32	QuerAddr;
		/*~~~~~~~~~~~~~*/

		QuerAddr = (_u32) ((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I);

		if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

		__try
		{
			value = LOAD_SWORD_PARAM(QuerAddr);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			/* TRACE1("DYNA LW: Addr=%08X", QuerAddr); */
			goto _Default;
		}

		if(xRT->mips_reg != 0)	/* mandatory */
		{
			ConstMap[xRT->mips_reg].IsMapped = 0;
			xRT->IsDirty = 1;
			xRT->NoNeedToLoadTheLo = 1;
			MapRegister(xRT, 99, 99);

			MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_SWORD_PARAM(QuerAddr));
		}
	}
	else
	{
_Default:
		if(ConstMap[xRS->mips_reg].IsMapped)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			int		temp = ConstMap[xRS->mips_reg].value;
			uint32	vAddr = (uint32) ((_int32) temp + (_s32) (_s16) __dotI);
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			xRT->IsDirty = 1;
			xRT->NoNeedToLoadTheLo = 1;

			MapRT;

			if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);

			MOV_ImmToReg(1, Reg_EAX, vAddr);
			X86_CALL((uint32) memory_read_functions[vAddr >> SHIFTER2_READ]);

			LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
			if(__RT != 0) MOV_ModRMToReg(1, xRT->x86reg, ModRM_EAX);
			if(xRT->x86reg != Reg_EAX) POP_RegIfMapped(Reg_EAX);
		}
		else
		{
			xRT->IsDirty = 1;
			if(xRT->mips_reg != xRS->mips_reg) xRT->NoNeedToLoadTheLo = 1;
			MapRT;

			if(xRT->x86reg != Reg_EAX)
			{
				MemoryCase2();
				if(__RT != 0) MOV_ModRMToReg(1, xRT->x86reg, ModRM_EAX);

				if(xRT->x86reg != Reg_EAX)
					POP_RegIfMapped(Reg_EAX);
				else if(xRS->x86reg != xRT->x86reg)
					POP_RegFromStack(xRS->x86reg);
			}
			else
			{
				/* we checked if rs is a const above already. good. */
				if
				(
					(CheckWhereIsMipsReg(xRS->mips_reg) == -1)
				&&	(ConstMap[xRS->mips_reg].FinalAddressUsedAt <= gHWS_pc)
				)
				{
					FetchEBP_Params(xRS->mips_reg);
					if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
					if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);

					MOV_MemoryToReg(1, Reg_ECX, x86params.ModRM, x86params.Address);
				}
				else
				{
					MapRS;
					if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
					if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);

					MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);	/* mov rt, rs (lo) */
				}

				ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
				MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);

				SHR_RegByImm(1, Reg_ECX, SHIFTER2_READ);
				WC16(0x14FF);
				WC8(0x85 | (Reg_ECX << 3));
				WC32((uint32) & memory_read_functions);

				LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
				if(__RT != 0) MOV_ModRMToReg(1, xRT->x86reg, ModRM_EAX);
				if(xRT->x86reg != Reg_ECX) POP_RegIfMapped(Reg_ECX);
				if(xRT->x86reg != Reg_EAX) POP_RegIfMapped(Reg_EAX);
			}
		}
	}
}

extern uint32	HardwareStart;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_sw(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	_u32	QuerAddr;
	_s32	value;
	/*~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_sw);

#ifdef SAFE_LOADSTORE
	goto _Default;
#endif
	if((ConstMap[xRS->mips_reg].IsMapped == 1) && (currentromoptions.Code_Check != CODE_CHECK_PROTECT_MEMORY))
	{
		__try
		{
			QuerAddr = (_u32) ((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I);
			if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

			switch(QuerAddr)
			{
			case 0xA4300000:	/* MI_MODE_REG */
				MapRT;
				PUSH_RegIfMapped(Reg_EAX);
				PUSH_RegIfMapped(Reg_ECX);
				MOV_Reg2ToReg1(1, Reg_ECX, xRT->x86reg);
				X86_CALL((uint32) & WriteMI_ModeReg);
				POP_RegIfMapped(Reg_ECX);
				POP_RegIfMapped(Reg_EAX);

				return;
				break;

			case 0xA430000C:	/* MI_INTR_MASK_REG */
				MapRT;
				PUSH_RegIfMapped(Reg_EAX);
				PUSH_RegIfMapped(Reg_ECX);
				MOV_Reg2ToReg1(1, Reg_ECX, xRT->x86reg);
				X86_CALL((uint32) & Handle_MI);
				POP_RegIfMapped(Reg_ECX);
				POP_RegIfMapped(Reg_EAX);

				return;
				break;
			}

			value = LOAD_SWORD_PARAM(QuerAddr);
			if(xRT->mips_reg != 0)
			{
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
				int temp = (uint32) pLOAD_SWORD_PARAM(QuerAddr);
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

				MapRT;

				MOV_RegToMemory(1, xRT->x86reg, ModRM_disp32, temp);
			}
			else
			{
				PUSH_RegIfMapped(Reg_EAX);
				XOR_Reg2ToReg1(1, Reg_EAX, Reg_EAX);
				MOV_RegToMemory(1, Reg_EAX, ModRM_disp32, (_u32) pLOAD_SWORD_PARAM(QuerAddr));
				POP_RegIfMapped(Reg_EAX);
			}
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			goto _Default;
		}
	}
	else
	{
_Default:
		{
			/*~~~~~~~~~~~~~~~~~*/
			int		temp, temp2;
			uint32	vAddr;
			int		rt = __dotRT;
			int		rs = __dotRS;
			/*~~~~~~~~~~~~~~~~~*/

			if(ConstMap[rs].IsMapped)
			{
				temp = ConstMap[rs].value;
				vAddr = (uint32) ((_int32) temp + (_s32) (_s16) __dotI);

				if(ConstMap[rt].IsMapped) FlushOneConstant(rt);
				if((temp2 = CheckWhereIsMipsReg(rt)) > -1) FlushRegister(temp2);

				PUSH_RegIfMapped(Reg_EAX);
				PUSH_RegIfMapped(Reg_ECX);

				MOV_ImmToMemory(1, ModRM_disp32, (_u32) & write_mem_rt, rt);

				MOV_ImmToReg(1, Reg_ECX, vAddr);
				X86_CALL((uint32) memory_write_functions[vAddr >> SHIFTER2_WRITE]);

				LOGGING_DYNA(LogDyna("	CALL memory_write_functions[]\n"););

				if(xRT->mips_reg != 0)
					LoadLowMipsCpuRegister(xRT->mips_reg, Reg_ECX);
				else
					XOR_Reg2ToReg1(1, Reg_ECX, Reg_ECX);
				MOV_RegToModRM(1, Reg_ECX, ModRM_EAX);

				POP_RegIfMapped(Reg_ECX);
				POP_RegIfMapped(Reg_EAX);
			}
			else
			{
				if(ConstMap[rt].IsMapped) FlushOneConstant(rt);
				if((temp = CheckWhereIsMipsReg(rt)) > -1) FlushRegister(temp);

				PUSH_RegIfMapped(Reg_EAX);
				PUSH_RegIfMapped(Reg_ECX);
				if((temp = CheckWhereIsMipsReg(rs)) > -1)
					MOV_Reg2ToReg1(1, Reg_ECX, (unsigned char) temp);
				else
					LoadLowMipsCpuRegister(rs, Reg_ECX);	/* some things like this will need rethinking for 2-pass. */
				MOV_ImmToMemory(1, ModRM_disp32, (_u32) & write_mem_rt, rt);
				ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);

				MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
				SHR_RegByImm(1, Reg_EAX, SHIFTER2_WRITE);
				WC16(0x14FF);
				WC8(0x85);
				WC32((uint32) & memory_write_functions);
				LOGGING_DYNA(LogDyna("	CALL memory_write_functions[]\n"););

				if(xRT->mips_reg != 0)
					LoadLowMipsCpuRegister(xRT->mips_reg, Reg_ECX);
				else
					XOR_Reg2ToReg1(1, Reg_ECX, Reg_ECX);
				MOV_RegToModRM(1, Reg_ECX, ModRM_EAX);

				POP_RegIfMapped(Reg_ECX);
				POP_RegIfMapped(Reg_EAX);
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_ld(OP_PARAMS)
{
	/*~~~~~~~~~~*/
	_s32	value;
	/*~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_ld);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_ld);
	return;
#endif
	if(xRT->mips_reg == 0) return;

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		__try
		{
			/*~~~~~~~~~~~~~*/
			_u32	QuerAddr;
			/*~~~~~~~~~~~~~*/

			QuerAddr = (_u32) ((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I);

			if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

			value = LOAD_SWORD_PARAM(QuerAddr);
			value = LOAD_SWORD_PARAM((QuerAddr + 4));
			if(xRT->mips_reg != 0)	/* mandatory */
			{
				if(ConstMap[xRT->mips_reg].IsMapped == 1)
				{
					ConstMap[xRT->mips_reg].IsMapped = 0;
				}

				xRT->IsDirty = 1;
				xRT->NoNeedToLoadTheLo = 1;
				xRT->NoNeedToLoadTheHi = 1;
				MapRT;

				MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM((QuerAddr + 4)));
				MOV_MemoryToReg(1, xRT->HiWordLoc, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr));
			}
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			goto _Default;
		}
	}
	else
	{
_Default:

	{
			if(ConstMap[xRS->mips_reg].IsMapped)
			{
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
				int temp = ConstMap[xRS->mips_reg].value;
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

				xRT->IsDirty = 1;
				xRT->NoNeedToLoadTheLo = 1;
				xRT->NoNeedToLoadTheHi = 1;
				MapRT;
				if((xRT->x86reg != Reg_EAX) && (xRT->HiWordLoc != Reg_EAX)) PUSH_RegIfMapped(Reg_EAX);
				if((xRT->x86reg != Reg_ECX) && (xRT->HiWordLoc != Reg_ECX)) PUSH_RegIfMapped(Reg_ECX);

				MOV_ImmToReg(1, Reg_EAX, temp + (_s32) (_s16) __dotI);
				MOV_ImmToReg(1, Reg_ECX, (uint32) (temp + (_s32) (_s16) __dotI) >> SHIFTER2_READ);
				WC16(0x14FF);
				WC8(0x8D);
				WC32((uint32) & memory_read_functions);
				LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
				if(__RT != 0)
				{
					if(xRT->x86reg == Reg_EAX)
					{
						MOV_MemoryToReg(1, xRT->HiWordLoc, ModRM_EAX, 0);
						MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp8_EAX, 4);
					}
					else
					{
						MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp8_EAX, 4);
						MOV_MemoryToReg(1, xRT->HiWordLoc, ModRM_EAX, 0);
					}
				}

				if((xRT->x86reg != Reg_ECX) && (xRT->HiWordLoc != Reg_ECX)) POP_RegIfMapped(Reg_ECX);
				if((xRT->x86reg != Reg_EAX) && (xRT->HiWordLoc != Reg_EAX)) POP_RegIfMapped(Reg_EAX);
			}
			else
			{
				xRT->IsDirty = 1;
				if(xRT->mips_reg != xRS->mips_reg)
				{
					xRT->NoNeedToLoadTheLo = 1;
					xRT->NoNeedToLoadTheHi = 1;
				}

				MapRT;
				MapRS;

				if((xRS->x86reg != Reg_EAX) && (xRS->HiWordLoc != Reg_EAX))
				{
					if(xRT->mips_reg != 0)
					{
						if((xRT->x86reg == Reg_EAX) && (xRT->HiWordLoc != Reg_EAX))
						{
							MemoryCase1();
							MOV_MemoryToReg(1, xRT->HiWordLoc, ModRM_EAX, 0);
							MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp8_EAX, 4);
						}
						else
						{
							MemoryCase1();
							MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp8_EAX, 4);
							MOV_MemoryToReg(1, xRT->HiWordLoc, ModRM_EAX, 0);
						}
					}
					else if (xRT->x86reg == Reg_EAX)
						XOR_Reg2ToReg1(1, xRT->x86reg, xRT->x86reg);

					if((xRT->x86reg != Reg_EAX) && (xRT->HiWordLoc != Reg_EAX))
						POP_RegIfMapped(Reg_EAX);
					else if(xRS->x86reg != xRT->x86reg)
						POP_RegFromStack(xRS->x86reg);
				}
				else
				{
					{
						/* MapRS; */
						if((xRT->x86reg != Reg_EAX) && (xRT->HiWordLoc != Reg_EAX)) PUSH_RegIfMapped(Reg_EAX);
						if((xRT->x86reg != Reg_ECX) && (xRT->HiWordLoc != Reg_ECX)) PUSH_RegIfMapped(Reg_ECX);
						MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);	/* mov rt, rs (lo) */
					}

					ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
					MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
					SHR_RegByImm(1, Reg_ECX, SHIFTER2_READ);
					WC16(0x14FF);
					WC8(0x8D);
					WC32((uint32) & memory_read_functions);
					LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
					if(__RT != 0)
					{
						if(xRT->x86reg == Reg_EAX)
						{
							MOV_MemoryToReg(1, xRT->HiWordLoc, ModRM_EAX, 0);
							MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp8_EAX, 4);
						}
						else
						{
							MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp8_EAX, 4);
							MOV_MemoryToReg(1, xRT->HiWordLoc, ModRM_EAX, 0);
						}
					}

					if((xRT->x86reg != Reg_ECX) && (xRT->HiWordLoc != Reg_ECX)) POP_RegIfMapped(Reg_ECX);
					if((xRT->x86reg != Reg_EAX) && (xRT->HiWordLoc != Reg_EAX)) POP_RegIfMapped(Reg_EAX);
				}
			}
		}
	}
}

extern void r4300i_sd_faster(uint32 QuerAddr, uint32 rt_ft);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_sd(OP_PARAMS)
{
	/*~~~~~~~~~~*/
	int		temp;
	uint32	vaddr;
	/*~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_sd);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_sd);
	return;
#endif
	if(ConstMap[xRS->mips_reg].IsMapped)
	{
		vaddr = (uint32) ((_int32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I);
		if(ConstMap[xRT->mips_reg].IsMapped) FlushOneConstant(xRT->mips_reg);
		if((temp = CheckWhereIsMipsReg(xRT->mips_reg)) > -1) FlushRegister(temp);
		PUSH_RegIfMapped(Reg_EAX);
		PUSH_RegIfMapped(Reg_ECX);

		MOV_ImmToMemory(1, ModRM_disp32, (_u32) & write_mem_rt, xRT->mips_reg);
		MOV_ImmToReg(1, Reg_ECX, vaddr);
		MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
		SHR_RegByImm(1, Reg_EAX, SHIFTER2_WRITE);

		WC16(0x14FF);
		WC8(0x85);
		WC32((uint32) & memory_write_functions);
		LOGGING_DYNA(LogDyna("	CALL memory_write_functions[]\n"););

		MOV_MemoryToReg(1, Reg_ECX, ModRM_disp32, (uint32) & write_mem_rt);

		LoadLowMipsCpuRegister(xRT->mips_reg, Reg_ECX);
		MOV_RegToMemory(1, Reg_ECX, ModRM_disp8_EAX, 4);

		LoadHighMipsCpuRegister(xRT->mips_reg, Reg_ECX);
		MOV_RegToMemory(1, Reg_ECX, ModRM_EAX, 0);
		POP_RegIfMapped(Reg_ECX);
		POP_RegIfMapped(Reg_EAX);
	}
	else
	{
		if(ConstMap[xRT->mips_reg].IsMapped) FlushOneConstant(xRT->mips_reg);
		if((temp = CheckWhereIsMipsReg(xRT->mips_reg)) > -1) FlushRegister(temp);
		PUSH_RegIfMapped(Reg_EAX);
		PUSH_RegIfMapped(Reg_ECX);

		if((temp = CheckWhereIsMipsReg(xRS->mips_reg)) > -1)
			MOV_Reg2ToReg1(1, Reg_ECX, (unsigned char) temp);
		else
			LoadLowMipsCpuRegister(xRS->mips_reg, Reg_ECX);

		MOV_ImmToMemory(1, ModRM_disp32, (_u32) & write_mem_rt, xRT->mips_reg);
		ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
		MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
		SHR_RegByImm(1, Reg_EAX, SHIFTER2_WRITE);

		WC16(0x14FF);
		WC8(0x85);
		WC32((uint32) & memory_write_functions);
		LOGGING_DYNA(LogDyna("	CALL memory_write_functions[]\n"););

		LoadLowMipsCpuRegister(xRT->mips_reg, Reg_ECX);
		MOV_RegToMemory(1, Reg_ECX, ModRM_disp8_EAX, 4);

		LoadHighMipsCpuRegister(xRT->mips_reg, Reg_ECX);
		MOV_RegToMemory(1, Reg_ECX, ModRM_EAX, 0);
		POP_RegIfMapped(Reg_ECX);
		POP_RegIfMapped(Reg_EAX);
	}
}

extern _int32	r4300i_lb_faster(uint32 QuerAddr);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lb(OP_PARAMS)
{
	/*~~~~~~*/
	_s8 value;
	/*~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_lb);

	if(xRT->mips_reg == 0) return;

#ifdef SAFE_LOADSTORE
	goto _Default;
#endif
	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		__try
		{
			/*~~~~~~~~~~~~~*/
			_u32	QuerAddr;
			/*~~~~~~~~~~~~~*/

			/* TLB range */
			QuerAddr = (_u32) (((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I));
			if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

			value = LOAD_SBYTE_PARAM(QuerAddr);
			if(xRT->mips_reg != 0)	/* mandatory */
			{
				if(ConstMap[xRT->mips_reg].IsMapped == 1)
				{
					ConstMap[xRT->mips_reg].IsMapped = 0;
				}

				xRT->IsDirty = 1;
				xRT->NoNeedToLoadTheLo = 1;
				MapRT;

				/* loads aligned, and loads 32bit. very cool. */
				switch(QuerAddr & 3)
				{
				case 0:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					SAR_RegByImm(1, xRT->x86reg, 24);
					break;
				case 1:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					SHL_RegByImm(1, xRT->x86reg, 8);
					SAR_RegByImm(1, xRT->x86reg, 24);
					break;
				case 2:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					SHL_RegByImm(1, xRT->x86reg, 16);
					SAR_RegByImm(1, xRT->x86reg, 24);
					break;
				case 3:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					SHL_RegByImm(1, xRT->x86reg, 24);
					SAR_RegByImm(1, xRT->x86reg, 24);
					break;
				default:
					MOVSX_MemoryToReg(0, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_SBYTE_PARAM(QuerAddr));
				}
			}
			else
				goto _Default;
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			goto _Default;
		}
	}
	else
	{
_Default:
		if(ConstMap[xRS->mips_reg].IsMapped)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			int temp = ConstMap[xRS->mips_reg].value;
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			xRT->IsDirty = 1;
			xRT->NoNeedToLoadTheLo = 1;
			MapRT;
			if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
			if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);

			MOV_ImmToReg(1, Reg_EAX, (temp + (_s32) (_s16) __dotI) ^ 3);
			MOV_ImmToReg(1, Reg_ECX, ((uint32) (temp + (_s32) (_s16) __dotI)) >> SHIFTER2_READ);

			WC16(0x14FF);
			WC8(0x8D);
			WC32((uint32) & memory_read_functions);
			LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
			if(__RT != 0) MOVSX_MemoryToReg(0, xRT->x86reg, ModRM_EAX, 0);
			if(xRT->x86reg != Reg_ECX) POP_RegIfMapped(Reg_ECX);
			if(xRT->x86reg != Reg_EAX) POP_RegIfMapped(Reg_EAX);
		}
		else
		{
			xRT->IsDirty = 1;
			if(xRT->mips_reg != xRS->mips_reg) xRT->NoNeedToLoadTheLo = 1;
			MapRT;
			MapRS;

			if(xRS->x86reg != Reg_EAX)
			{
				MemoryCase1();
				if(__RT != 0)
				{
					if(xRT->x86reg != Reg_ECX)
					{
						PUSH_RegIfMapped(Reg_ECX);
						MOV_Reg2ToReg1(1, Reg_ECX, Reg_EAX);
						AND_ImmToReg(1, Reg_EAX, 0xfffffffc);
						MOV_MemoryToReg(1, xRT->x86reg, ModRM_EAX, 0);
						SHL_RegByImm(1, Reg_ECX, 3);
						SHL_RegByCL(1, xRT->x86reg);
						SAR_RegByImm(1, xRT->x86reg, 24);
						POP_RegIfMapped(Reg_ECX);
					}
					else
					{
						XOR_ImmToReg(1, Reg_EAX, 3);
						MOVSX_MemoryToReg(0, xRT->x86reg, ModRM_EAX, 0);
					}
				}
				else
					XOR_Reg2ToReg1(1, xRT->x86reg, xRT->x86reg);

				if(xRT->x86reg != Reg_EAX)
					POP_RegIfMapped(Reg_EAX);
				else if(xRS->x86reg != xRT->x86reg)
					POP_RegFromStack(xRS->x86reg);
			}
			else
			{
				{
					/* MapRS; */
					if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
					if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);
					MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);
				}

				ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
				MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
				SHR_RegByImm(1, Reg_ECX, SHIFTER2_READ);

				WC16(0x14FF);
				WC8(0x8D);
				WC32((uint32) & memory_read_functions);
				LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
				if(__RT != 0)
				{					/* loads aligned, and loads 32bit. very cool. */
					if(xRT->x86reg != Reg_ECX)
					{
						MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);
						AND_ImmToReg(1, Reg_EAX, 0xfffffffc);
						MOV_MemoryToReg(1, xRT->x86reg, ModRM_EAX, 0);
						SHL_RegByImm(1, Reg_ECX, 3);
						SHL_RegByCL(1, xRT->x86reg);
						SAR_RegByImm(1, xRT->x86reg, 24);
					}
					else
					{
						XOR_ImmToReg(1, Reg_EAX, 3);
						MOVSX_MemoryToReg(0, xRT->x86reg, ModRM_EAX, 0);
					}
				}
				else if(xRT->x86reg == Reg_EAX)
					XOR_Reg2ToReg1(1, xRT->x86reg, xRT->x86reg);

				if(xRT->x86reg != Reg_ECX) POP_RegIfMapped(Reg_ECX);
				if(xRT->x86reg != Reg_EAX) POP_RegIfMapped(Reg_EAX);
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lbu(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	_u32	QuerAddr;
	_u8		value;
	/*~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_lbu);

	if(xRT->mips_reg == 0) return;

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_lbu); return;
#endif
	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		__try
		{
			QuerAddr = (_u32) ((((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I)));
			if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

			value = LOAD_UBYTE_PARAM(QuerAddr);
			if(xRT->mips_reg != 0)	/* mandatory */
			{
				if(ConstMap[xRT->mips_reg].IsMapped == 1)
				{
					ConstMap[xRT->mips_reg].IsMapped = 0;
				}

				xRT->IsDirty = 1;
				xRT->NoNeedToLoadTheLo = 1;
				MapRT;

				/* loads aligned, and loads 32bit. very cool. */
				switch(QuerAddr & 3)
				{
				case 0:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					ROL_RegByImm(1, xRT->x86reg, 8);
					AND_ImmToReg(1, xRT->x86reg, 0x000000ff);
					break;
				case 1:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					SHR_RegByImm(1, xRT->x86reg, 16);
					AND_ImmToReg(1, xRT->x86reg, 0x000000ff);
					break;
				case 2:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					WC8(0x66);
					SHR_RegByImm(1, xRT->x86reg, 8);
					AND_ImmToReg(1, xRT->x86reg, 0x000000ff);
					break;
				case 3:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					AND_ImmToReg(1, xRT->x86reg, 0x000000ff);
					break;

				default:
					MOVZX_MemoryToReg(0, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_SBYTE_PARAM(QuerAddr));
				}
			}
			else
				goto _Default;
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			goto _Default;
		}
	}
	else
	{
_Default:
		if(ConstMap[xRS->mips_reg].IsMapped)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			int temp = ConstMap[xRS->mips_reg].value;
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			xRT->IsDirty = 1;
			xRT->NoNeedToLoadTheLo = 1;
			MapRT;

			if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
			if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);

			MOV_ImmToReg(1, Reg_EAX, (temp + (_s32) (_s16) __dotI) ^ 3);
			MOV_ImmToReg(1, Reg_ECX, ((uint32) (temp + (_s32) (_s16) __dotI)) >> SHIFTER2_READ);

			WC16(0x14FF);
			WC8(0x8D);
			WC32((uint32) & memory_read_functions);
			LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
			if(__RT != 0) MOVZX_MemoryToReg(0, xRT->x86reg, ModRM_EAX, 0);
			if(xRT->x86reg != Reg_ECX) POP_RegIfMapped(Reg_ECX);
			if(xRT->x86reg != Reg_EAX) POP_RegIfMapped(Reg_EAX);
		}
		else
		{
			xRT->IsDirty = 1;
			if(xRT->mips_reg != xRS->mips_reg) xRT->NoNeedToLoadTheLo = 1;
			MapRT;
			MapRS;

			if(xRS->x86reg != Reg_EAX)
			{
				MemoryCase1();
				if(__RT != 0)
				{
					if(xRT->x86reg != Reg_ECX)
					{
						PUSH_RegIfMapped(Reg_ECX);
						MOV_Reg2ToReg1(1, Reg_ECX, Reg_EAX);
						AND_ImmToReg(1, Reg_EAX, 0xfffffffc);
						MOV_MemoryToReg(1, xRT->x86reg, ModRM_EAX, 0);
						SHL_RegByImm(1, Reg_ECX, 3);
						SHL_RegByCL(1, xRT->x86reg);
						SHR_RegByImm(1, xRT->x86reg, 24);
						POP_RegIfMapped(Reg_ECX);
					}
					else
					{
						XOR_ImmToReg(1, Reg_EAX, 3);
						MOVZX_MemoryToReg(0, xRT->x86reg, ModRM_EAX, 0);
					}
				}
				else if (xRT->x86reg == Reg_EAX)
					XOR_Reg2ToReg1(1, xRT->x86reg, xRT->x86reg);

				if(xRT->x86reg != Reg_EAX)
					POP_RegIfMapped(Reg_EAX);
				else if(xRS->x86reg != xRT->x86reg)
					POP_RegFromStack(xRS->x86reg);
			}
			else
			{
				{
					/* MapRS; */
					if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
					if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);
					MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);
				}

				ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
				MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
				SHR_RegByImm(1, Reg_ECX, SHIFTER2_READ);

				WC16(0x14FF);
				WC8(0x8D);
				WC32((uint32) & memory_read_functions);
				LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
				if(__RT != 0)
				{					/* loads aligned, and loads 32bit. very cool. */
					if(xRT->x86reg != Reg_ECX)
					{
						MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);
						AND_ImmToReg(1, Reg_EAX, 0xfffffffc);
						MOV_MemoryToReg(1, xRT->x86reg, ModRM_EAX, 0);
						SHL_RegByImm(1, Reg_ECX, 3);
						SHL_RegByCL(1, xRT->x86reg);
						SHR_RegByImm(1, xRT->x86reg, 24);
					}
					else
					{
						XOR_ImmToReg(1, Reg_EAX, 3);
						MOVZX_MemoryToReg(0, xRT->x86reg, ModRM_EAX, 0);
					}
				}

				if(xRT->x86reg != Reg_ECX) POP_RegIfMapped(Reg_ECX);
				if(xRT->x86reg != Reg_EAX) POP_RegIfMapped(Reg_EAX);
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lhu(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	_u32	QuerAddr;
	_u16	value;
	/*~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_lhu);

	if(xRT->mips_reg == 0) return;

#ifdef SAFE_LOADSTORE
	/* INTERPRET_LOADSTORE(r4300i_lhu); return; */
#endif
	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		__try
		{
			QuerAddr = (_u32) ((((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I)));
			if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

			value = LOAD_UHALF_PARAM(QuerAddr);
			if(xRT->mips_reg != 0)	/* mandatory */
			{
				if(ConstMap[xRT->mips_reg].IsMapped == 1)
				{
					ConstMap[xRT->mips_reg].IsMapped = 0;
				}

				xRT->IsDirty = 1;
				xRT->NoNeedToLoadTheLo = 1;
				MapRT;

				/* loads aligned, and loads 32bit. very cool. */
				switch(QuerAddr & 3)
				{
				case 0:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					SHR_RegByImm(1, xRT->x86reg, 16);
					break;
				case 2:
					MOV_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr & 0xfffffffc));
					AND_ImmToReg(1, xRT->x86reg, 0x0000ffff);
					break;
				default:
					/* hmm..this is never touched, but looks wrong. */
					MOVZX_MemoryToReg(1, xRT->x86reg, ModRM_disp32, (_u32) pLOAD_UHALF_PARAM(QuerAddr));
					break;
				}
			}
			else
				goto _Default;
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			goto _Default;
		}
	}
	else
	{
_Default:
		{
			if(ConstMap[xRS->mips_reg].IsMapped)
			{
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
				int temp = ConstMap[xRS->mips_reg].value;
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

				xRT->IsDirty = 1;
				xRT->NoNeedToLoadTheLo = 1;
				MapRT;
				if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
				if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);

				MOV_ImmToReg(1, Reg_EAX, temp + (_s32) (_s16) __dotI);
				MOV_ImmToReg(1, Reg_ECX, ((uint32) (temp + (_s32) (_s16) __dotI)) >> SHIFTER2_READ);
				XOR_ImmToReg(1, Reg_EAX, 2);
				WC16(0x14FF);
				WC8(0x8D);
				WC32((uint32) & memory_read_functions);
				LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
				if(__RT != 0) MOVZX_MemoryToReg(1, xRT->x86reg, ModRM_EAX, 0);
				if(xRT->x86reg != Reg_ECX) POP_RegIfMapped(Reg_ECX);
				if(xRT->x86reg != Reg_EAX) POP_RegIfMapped(Reg_EAX);
			}
			else
			{
				/*~~~~~*/
				int temp;
				/*~~~~~*/

				xRT->IsDirty = 1;
				if(xRT->mips_reg != xRS->mips_reg) xRT->NoNeedToLoadTheLo = 1;
				MapRT;

				/* we checked if rs is a const above already. good. */
				if
				(
					((temp = CheckWhereIsMipsReg(xRS->mips_reg)) == -1)
				&&	(ConstMap[xRS->mips_reg].FinalAddressUsedAt <= gHWS_pc)
				)
				{
					FetchEBP_Params(xRS->mips_reg);
					if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
					if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);
					MOV_MemoryToReg(1, Reg_ECX, x86params.ModRM, x86params.Address);
				}
				else
				{
					MapRS;
					if(xRT->x86reg != Reg_EAX) PUSH_RegIfMapped(Reg_EAX);
					if(xRT->x86reg != Reg_ECX) PUSH_RegIfMapped(Reg_ECX);
					MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);
				}

				ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
				MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
				SHR_RegByImm(1, Reg_ECX, SHIFTER2_READ);
				XOR_ImmToReg(1, Reg_EAX, 2);
				WC16(0x14FF);
				WC8(0x8D);
				WC32((uint32) & memory_read_functions);
				LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););
				if(__RT != 0) MOVZX_MemoryToReg(1, xRT->x86reg, ModRM_EAX, 0);
				if(xRT->x86reg != Reg_ECX) POP_RegIfMapped(Reg_ECX);
				if(xRT->x86reg != Reg_EAX) POP_RegIfMapped(Reg_EAX);
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_sb(OP_PARAMS)
{
	/*~~~~~~~~~~*/
	int		temp;
	uint32	vaddr;
	/*~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_sb);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_sb);
	return;
#endif
	if(ConstMap[xRS->mips_reg].IsMapped)
	{
		vaddr = (uint32) ((_int32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I);
		if(ConstMap[xRT->mips_reg].IsMapped) FlushOneConstant(xRT->mips_reg);
		if((temp = CheckWhereIsMipsReg(xRT->mips_reg)) > -1) FlushRegister(temp);
		PUSH_RegIfMapped(Reg_EAX);
		PUSH_RegIfMapped(Reg_ECX);

		MOV_ImmToMemory(1, ModRM_disp32, (_u32) & write_mem_rt, xRT->mips_reg);
		MOV_ImmToReg(1, Reg_ECX, vaddr ^ 3);
		MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
		SHR_RegByImm(1, Reg_EAX, SHIFTER2_WRITE);

		WC16(0x14FF);
		WC8(0x85);
		WC32((uint32) & memory_write_functions);
		LOGGING_DYNA(LogDyna("	CALL memory_write_functions[]\n"););

		if(xRT->mips_reg != 0)
			LoadLowMipsCpuRegister(xRT->mips_reg, Reg_ECX);
		else
			XOR_Reg2ToReg1(1, Reg_ECX, Reg_ECX);

		MOV_RegToMemory(0, Reg_ECX, ModRM_EAX, 0);
		POP_RegIfMapped(Reg_ECX);
		POP_RegIfMapped(Reg_EAX);
	}
	else
	{
		if(ConstMap[xRT->mips_reg].IsMapped) FlushOneConstant(xRT->mips_reg);
		if((temp = CheckWhereIsMipsReg(xRT->mips_reg)) > -1) FlushRegister(temp);
		PUSH_RegIfMapped(Reg_EAX);
		PUSH_RegIfMapped(Reg_ECX);

		if((temp = CheckWhereIsMipsReg(xRS->mips_reg)) > -1)
			MOV_Reg2ToReg1(1, Reg_ECX, (unsigned char) temp);
		else
			LoadLowMipsCpuRegister(xRS->mips_reg, Reg_ECX); /* some things like this will need rethinking for 2-pass. */

		MOV_ImmToMemory(1, ModRM_disp32, (_u32) & write_mem_rt, xRT->mips_reg);
		ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
		MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
		SHR_RegByImm(1, Reg_EAX, SHIFTER2_WRITE);
		XOR_ImmToReg(1, Reg_ECX, 3);

		WC16(0x14FF);
		WC8(0x85);
		WC32((uint32) & memory_write_functions);
		LOGGING_DYNA(LogDyna("	CALL memory_write_functions[]\n"););

		if(xRT->mips_reg != 0)
			LoadLowMipsCpuRegister(xRT->mips_reg, Reg_ECX);
		else
			XOR_Reg2ToReg1(1, Reg_ECX, Reg_ECX);

		MOV_RegToMemory(0, Reg_ECX, ModRM_EAX, 0);
		POP_RegIfMapped(Reg_ECX);
		POP_RegIfMapped(Reg_EAX);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_sh(OP_PARAMS)
{
	/*~~~~~~~~~~*/
	int		temp;
	uint32	vaddr;
	/*~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_sh) SetRdRsRt32bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE
	INTERPRET_LOADSTORE(r4300i_sh);
	return;
#endif SAFE_LOADSTORE
	if(ConstMap[xRS->mips_reg].IsMapped)
	{
		vaddr = (uint32) ((_int32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I);
		vaddr ^= 2;
		if(ConstMap[xRT->mips_reg].IsMapped) FlushOneConstant(xRT->mips_reg);
		if((temp = CheckWhereIsMipsReg(xRT->mips_reg)) > -1) FlushRegister(temp);
		PUSH_RegIfMapped(Reg_EAX);
		PUSH_RegIfMapped(Reg_ECX);

		MOV_ImmToMemory(1, ModRM_disp32, (_u32) & write_mem_rt, xRT->mips_reg);
		MOV_ImmToReg(1, Reg_ECX, vaddr);
		MOV_ImmToReg(1, Reg_EAX, (vaddr >> SHIFTER2_WRITE));

		WC16(0x14FF);
		WC8(0x85);
		WC32((uint32) & memory_write_functions);
		LOGGING_DYNA(LogDyna("	CALL memory_write_functions[]\n"););

		if(xRT->mips_reg != 0)
			LoadLowMipsCpuRegister(xRT->mips_reg, Reg_ECX);
		else
			XOR_Reg2ToReg1(1, Reg_ECX, Reg_ECX);
		WC8(0x66);
		MOV_RegToMemory(1, Reg_ECX, ModRM_EAX, 0);

		POP_RegIfMapped(Reg_ECX);
		POP_RegIfMapped(Reg_EAX);
	}
	else
	{
		if(ConstMap[xRT->mips_reg].IsMapped) FlushOneConstant(xRT->mips_reg);
		if((temp = CheckWhereIsMipsReg(xRT->mips_reg)) > -1) FlushRegister(temp);
		PUSH_RegIfMapped(Reg_EAX);
		PUSH_RegIfMapped(Reg_ECX);

		if((temp = CheckWhereIsMipsReg(xRS->mips_reg)) > -1)
			MOV_Reg2ToReg1(1, Reg_ECX, (unsigned char) temp);
		else
			LoadLowMipsCpuRegister(xRS->mips_reg, Reg_ECX); /* some things like this will need rethinking for 2-pass. */

		MOV_ImmToMemory(1, ModRM_disp32, (_u32) & write_mem_rt, xRT->mips_reg);
		ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
		MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
		SHR_RegByImm(1, Reg_EAX, SHIFTER2_WRITE);
		XOR_ImmToReg(1, Reg_ECX, 2);

		WC16(0x14FF);
		WC8(0x85);
		WC32((uint32) & memory_write_functions);
		LOGGING_DYNA(LogDyna("	CALL memory_write_functions[]\n"););

		if(xRT->mips_reg != 0)
			LoadLowMipsCpuRegister(xRT->mips_reg, Reg_ECX);
		else
			XOR_Reg2ToReg1(1, Reg_ECX, Reg_ECX);

		WC8(0x66);
		MOV_RegToMemory(1, Reg_ECX, ModRM_EAX, 0);
		POP_RegIfMapped(Reg_ECX);
		POP_RegIfMapped(Reg_EAX);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cache(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_cache)
#ifdef CHECK_CACHE
	INTERPRET(r4300i_cache);
#endif
}

#define _SLL_	0
#define _SRL_	2
#define _SRA_	3
#define _DSLL_	56
#define _DSRL_	58
#define _DSRA_	59

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_shift(uint32 Instruction)
{
	/* Interpret the shift. (For debugging purposes.) */
	switch(Instruction & 0x3F)
	{
	case 0:
		r4300i_sll(Instruction);
		break;
	case 2:
		r4300i_srl(Instruction);
		break;
	case 3:
	case _DSRA_:
		r4300i_sra(Instruction);
		break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void LogSomething(void)
{
	/*~~~~~~~~~~~~~~*/
	static int	k = 0;
	/*~~~~~~~~~~~~~~*/

	if(k == 0)
	{
		InitLogDyna();
		k = 1;
	}

	LogDyna("%08X\n", gHWS_pc);
}

/*
 =======================================================================================================================
    This function has 3 advantages: - Reduces register pressure. - Doesn't need the mov instruction between 2
    registers. - Allows for better instruction pairing What it does: When RD != RT, we can use RT as RD (and free the
    old RD) when we know the following: - RT is not used in the remainder of the block - RT is not dirty Notes: - With
    this optimization, there can be no errors in AnalyzeBlock(). - You can only use this function when you've already
    confirmed that RT is not a constant, due to this function's use of MapRT_To() - Use this function in place of
    MapRT_To(xRD, 1, MOV_MemoryToReg)
 =======================================================================================================================
 */
void Map_RD_NE_RT(void)
{
	/*~~~~~~~~~~~~~~~*/
	int tempRT, tempRD;
	/*~~~~~~~~~~~~~~~*/

	if
	(
		((tempRT = CheckWhereIsMipsReg(xRT->mips_reg)) > -1)
	&&	(currentromoptions.Advanced_Block_Analysis == USEBLOCKANALYSIS_YES)
	&&	(x86reg[tempRT].IsDirty == 0)
	&&	(ConstMap[xRT->mips_reg].FinalAddressUsedAt <= gHWS_pc)
	)
	{
		/* Free RT's hi. */
		if(x86reg[tempRT].HiWordLoc != tempRT)
		{
			x86reg[x86reg[tempRT].HiWordLoc].IsDirty = 0;
			x86reg[x86reg[tempRT].HiWordLoc].mips_reg = -1;
			x86reg[tempRT].Is32bit = 1;
			x86reg[tempRT].HiWordLoc = tempRT;
		}

		if(ConstMap[xRD->mips_reg].IsMapped) ConstMap[xRD->mips_reg].IsMapped = 0;
		if((tempRD = CheckWhereIsMipsReg(xRD->mips_reg)) > -1)
		{
			x86reg[tempRD].IsDirty = 0;
			FlushRegister(tempRD);
		}

		/* Maps RT */
		x86reg[tempRT].x86reg = tempRT;
		x86reg[tempRT].mips_reg = xRD->mips_reg;
		x86reg[tempRT].HiWordLoc = tempRT;
		x86reg[tempRT].IsDirty = 1;
		xRD->x86reg = tempRT;
	}
	else
	{
		xRD->IsDirty = 1;
		xRD->NoNeedToLoadTheLo = 1;
		MapRD;

		/* Make sure you know that RT is not a constant before using this function */
		MapRT_To(xRD, 1, MOV_MemoryToReg);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_shift(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_shift)
#ifdef SAFE_SHIFTS
	INTERPRET(r4300i_shift);
	return;
#endif
	if((xRD->mips_reg == 0))	/* RD is r0
								 * £
								 * */
		return;

	if(ConstMap[xRT->mips_reg].IsMapped == 1)
	/*
	 * RT is a constant (includes r0) £
	 */
	{
		/*~~~~~~~~~~~~~~~~~~~~*/
		uint8	sa = (_u8) __SA;
		/*~~~~~~~~~~~~~~~~~~~~*/

		switch(reg->code & 0x3F)
		{
		case _SLL_:
			/* case _DSLL_: (never use dsll as 32bit. the hiword changes) */
			MapConst(xRD, ((_u32) ConstMap[xRT->mips_reg].value << (_u32) (_u8) sa));
			break;
		case _SRL_:
		case _DSRL_:
			MapConst(xRD, ((_u32) ConstMap[xRT->mips_reg].value >> (_u32) (_u8) sa));
			break;
		case _DSRA_:
		case _SRA_:
			MapConst(xRD, ((_s32) ConstMap[xRT->mips_reg].value >> (_s32) (_u32) (_u8) sa));
			break;
		}
	}
	else
	{
		/*~~~~~~~~~~~~~~~~~~~~*/
		uint8	sa = (_u8) __SA;
		/*~~~~~~~~~~~~~~~~~~~~*/

		if(xRD->mips_reg == xRT->mips_reg)
		{
			sa = (_u8) __SA;

			xRD->IsDirty = 1;
			MapRD;
			if(sa != 0)
			{
				switch(reg->code & 0x3F)
				{
				case 0:
					/* case _DSLL_: (never use dsll as 32bit. the hiword changes) */
					SHL_RegByImm(1, (_u8) xRD->x86reg, (_u8) sa);
					break;
				case 2:
				case _DSRL_:
					SHR_RegByImm(1, (_u8) xRD->x86reg, (_u8) sa);
					break;
				case 3:
					SAR_RegByImm(1, (_u8) xRD->x86reg, (_u8) sa);
					break;
				case _DSRA_:
					SAR_RegByImm(1, (_u8) xRD->x86reg, (_u8) sa);
					break;
				}
			}
		}
		else
		{
			/* we checked if rt is a const above already. good. */
			Map_RD_NE_RT();
			{
				/*~~~~~~~*/
				uint8	sa;
				/*~~~~~~~*/

				sa = (_u8) __SA;

				switch(reg->code & 0x3F)
				{
				case 0:
					/* case _DSLL_: (never use dsll as 32bit. the hiword changes) */
					SHL_RegByImm(1, (_u8) xRD->x86reg, (_u8) sa);
					break;
				case 2:
				case _DSRL_:
					SHR_RegByImm(1, (_u8) xRD->x86reg, (_u8) sa);
					break;
				case 3:
					/*
					 * Conker!! £
					 * LogSomething();
					 */
					SAR_RegByImm(1, (_u8) xRD->x86reg, (_u8) sa);
					break;
				case _DSRA_:
					SAR_RegByImm(1, (_u8) xRD->x86reg, (_u8) sa);
					break;
				}
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_shift_var(uint32 Instruction)
{
	/* Interpret the shift. (For debugging purposes.) */
	switch(Instruction & 0x3f)
	{
	case 4: r4300i_sllv(Instruction); break;
	case 6: r4300i_srlv(Instruction); break;
	case 7: r4300i_srav(Instruction); break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_shift_var(OP_PARAMS)
{
	/*~~*/
	int k;
	/*~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_shift_var)
#ifdef SAFE_SHIFTS
	INTERPRET(r4300i_shift_var);
	return;
#endif
	if(xRD->mips_reg == 0) return;

	/* Protect ECX */
	if(x86reg[Reg_ECX].mips_reg > -1)
		FlushRegister(Reg_ECX);
	else if(x86reg[1].mips_reg == -2)
	{
		for(k = 0; k < 8; k++)
		{
			if(ItIsARegisterNotToUse(k));

			/* Don't use these registers for mapping */
			else if(x86reg[k].HiWordLoc == Reg_ECX)
			{
				FlushRegister(k);
				k = 9;
			}
		}
	}

	x86reg[1].mips_reg = 100;				/* protect ecx. */
	x86reg[1].HiWordLoc = 1;
	x86reg[1].Is32bit = 1;
	x86reg[1].IsDirty = 0;

	xRD->IsDirty = 1;
	if(xRD->mips_reg == xRT->mips_reg)
	{
		MapRD;

		if(xRS->mips_reg == xRT->mips_reg)	/* rd=rs=rt */
		{
			MOV_Reg2ToReg1(1, Reg_ECX, xRD->x86reg);
			switch(reg->code & 0x3f)
			{
			case 4: SHL_RegByCL(1, xRD->x86reg); break;
			case 6: SHR_RegByCL(1, xRD->x86reg); break;
			case 7: SAR_RegByCL(1, xRD->x86reg); break;
			}
		}
		else	/* rd=rt, rs!=rt */
		{
			MapRS;

			MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);
			switch(reg->code & 0x3f)
			{
			case 4: SHL_RegByCL(1, xRD->x86reg); break;
			case 6: SHR_RegByCL(1, xRD->x86reg); break;
			case 7: SAR_RegByCL(1, xRD->x86reg); break;
			}
		}
	}
	else		/* rd != rt */
	{
		if(xRD->mips_reg == xRS->mips_reg)
		{
			MapRD;
			MapRT;

			MOV_Reg2ToReg1(1, Reg_ECX, xRD->x86reg);
			MOV_Reg2ToReg1(1, xRD->x86reg, xRT->x86reg);
			switch(reg->code & 0x3f)
			{
			case 4: SHL_RegByCL(1, xRD->x86reg); break;
			case 6: SHR_RegByCL(1, xRD->x86reg); break;
			case 7: SAR_RegByCL(1, xRD->x86reg); break;
			}
		}
		else	/* rd != rs */
		{
			xRD->NoNeedToLoadTheLo = 1;
			MapRD;
			if(xRS->mips_reg == xRT->mips_reg)
			{
				if(xRT->mips_reg == 0)	/* rd = (rt=0) >> whatever */
					XOR_Reg2ToReg1(1, xRD->x86reg, xRD->x86reg);
				else
				{
					MapRT;
					MOV_Reg2ToReg1(1, Reg_ECX, xRT->x86reg);
					MOV_Reg2ToReg1(1, xRD->x86reg, xRT->x86reg);
					switch(reg->code & 0x3f)
					{
					case 4: SHL_RegByCL(1, xRD->x86reg); break;
					case 6: SHR_RegByCL(1, xRD->x86reg); break;
					case 7: SAR_RegByCL(1, xRD->x86reg); break;
					}
				}
			}
			else						/* rs != rt */
			{
				if(xRS->mips_reg != 0)
				{
					MapRS;

					/*
					 * if rt == 0..what here ? £
					 * rs!=0, rt!=0 , rs!=rt
					 */
					MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);

					if(ConstMap[xRT->mips_reg].IsMapped)
					{
						MOV_ImmToReg(1, xRD->x86reg, ConstMap[xRT->mips_reg].value);
					}
					else
						/* we checked if rt is a const above already. good. */
						Map_RD_NE_RT(); /* MapRT_To(xRD, 1, MOV_MemoryToReg); */

					switch(reg->code & 0x3f)
					{
					case 4: SHL_RegByCL(1, xRD->x86reg); break;
					case 6: SHR_RegByCL(1, xRD->x86reg); break;
					case 7: SAR_RegByCL(1, xRD->x86reg); break;
					}
				}
				else					/* rs=0, rt!=0 */
				{
					MapRT;
					MOV_Reg2ToReg1(1, xRD->x86reg, xRT->x86reg);
				}
			}
		}
	}

	/* Unprotect ECX */
	x86reg[1].mips_reg = -1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void prepare_run_exception(uint32 exception_code)
{
	gHWS_COP0Reg[CAUSE] &= NOT_EXCCODE;
	gHWS_COP0Reg[CAUSE] |= exception_code;
	gHWS_COP0Reg[EPC] = gHWS_pc;
	gHWS_COP0Reg[STATUS] |= EXL;	/* set EXL = 1 */
	gHWS_COP0Reg[CAUSE] &= NOT_BD;	/* clear BD */

	if(exception_code == EXC_CPU)
	{
		gHWS_COP0Reg[CAUSE] &= 0xCFFFFFFF;
		gHWS_COP0Reg[CAUSE] |= CAUSE_CE1;
	}

	TRACE0("Exception in Dyna");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna_set_exception(OP_PARAMS, uint32 exception_code, uint32 vector)
{
	FlushAllRegisters();
	MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &reg->pc, reg->pc);
	MOV_ImmToReg(1, Reg_ECX, exception_code);
	X86_CALL((uint32) prepare_run_exception);
	MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &reg->pc, vector);
	X86_CALL((uint32) Set_Translate_PC_No_TLB);
	RET();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_syscall(OP_PARAMS)
{
	if(debug_opcode) DisplayError("TODO: OpcodeDebugger: syscall");

	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);

	MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &reg->pc, reg->pc);
	INTERPRET(r4300i_syscall);
	MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &reg->pc, reg->pc);

	/* end of compiled block */
	compilerstatus.KEEP_RECOMPILING = FALSE;
	FlushAllRegisters();
	Interrupts(JUMP_TYPE_INDIRECT, reg->pc, LINK_NO, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_break(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_CPU_(r4300i_break);
	SetRdRsRt64bit(PASS_PARAMS);

	MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &reg->pc, reg->pc);
	INTERPRET(r4300i_break);
	MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &reg->pc, reg->pc);

	/* end of compiled block */
	compilerstatus.KEEP_RECOMPILING = FALSE;
	FlushAllRegisters();
	Interrupts(JUMP_TYPE_INDIRECT, reg->pc, LINK_NO, 0);


	/*
	 * dyna_set_exception(reg, EXC_BREAK, 0x80000180); £
	 * compilerstatus.KEEP_RECOMPILING = FALSE; £
	 * MOV_ImmToMemory(1, ModRM_disp32, (unsigned long)&reg->pc, reg->pc);
	 * INTERPRET(r4300i_break); //MOV_ImmToMemory(1, (unsigned long)&reg->pc,
	 * reg->pc+4); //MOV_ImmToMemory(1, ModRM_disp32, (unsigned long)&reg->pc,
	 * reg->pc); // end of compiled block compilerstatus.KEEP_RECOMPILING = FALSE;
	 * FlushAllRegisters(); Interrupts(JUMP_TYPE_INDIRECT, reg->pc, LINK_NO, 0);
	 */
}

extern void TriggerFPUUnusableException(void);
void (*Dyna_Code_Check[]) (uint32 pc);
void (*Dyna_Check_Codes) (uint32 pc);
void	COMPARE_SwitchToInterpretive(void);
void	COMPARE_SwitchToDynarec(void);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_with_exception(OP_PARAMS)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	MapConstant LocalTempConstMap[NUM_CONSTS];
	x86regtyp	LocalTempx86reg[8];
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifdef ENABLE_OPCODE_DEBUGGER
	SetRdRsRt32bit(PASS_PARAMS);
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY24
	if(debug_opcode)
	{
		PUSHAD();
		MOV_ImmToReg(1, Reg_EAX, (_u32) & COMPARE_SwitchToInterpretive);
		CALL_Reg(Reg_EAX);

		MOV_ImmToReg(1, Reg_ECX, reg->code);
		if(currentromoptions.FPU_Hack == USEFPUHACK_YES)
		{
			MOV_ImmToReg(1, Reg_EAX, (_u32) COP1_NotAvailable_instr);
		}
		else
		{
			MOV_ImmToReg(1, Reg_EAX, (_u32) COP1_instr);
		}

		CALL_Reg(Reg_EAX);	/* disable this line to test integrity of dyna portion */

		MOV_ImmToReg(1, Reg_EAX, (_u32) & COMPARE_SwitchToDynarec);
		CALL_Reg(Reg_EAX);
		POPAD();
	}
#endif
#endif
	if( currentromoptions.FPU_Hack == USEFPUHACK_YES)
	{
		/*~~~~~~~~~~~~~~~*/
		int temp = reg->pc;
		/*~~~~~~~~~~~~~~~*/

		TEST_ImmWithMemory((uint32) & gHWS_COP0Reg[STATUS], SR_CU1);
		Jcc_Near_auto(CC_NE, 8);

		/*
		memcpy(LocalTempConstMap, ConstMap, sizeof(ConstMap));
		memcpy(LocalTempx86reg, x86reg, sizeof(x86reg));
		dyna_set_exception(reg, EXC_CPU, 0x80000180);
		*/

		MOV_ImmToMemory(1, ModRM_disp32, (uint32)&reg->pc, temp);
		memcpy(LocalTempConstMap, ConstMap, sizeof(ConstMap)); 
		memcpy(LocalTempx86reg,	x86reg, sizeof(x86reg)); 
		FlushAllRegisters();

		X86_CALL((uint32)TriggerFPUUnusableException); 
		MOV_ImmToMemory(1, ModRM_disp32, (uint32)&reg->pc, temp); RET();

		SetNearTarget(8);
		memcpy(ConstMap, LocalTempConstMap, sizeof(ConstMap));
		memcpy(x86reg, LocalTempx86reg, sizeof(x86reg));
	}

	if(debug_opcode)
	{
		debug_opcode = 0;	/* this stops the opcode is interpreted again within dyna4300i_cop1_Instruction */
		dyna4300i_cop1_Instruction[__RS](PASS_PARAMS);
		debug_opcode = 1;
	}
	else
		dyna4300i_cop1_Instruction[__RS](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_sync(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_mf_mt(uint32 Instruction)
{
	switch(Instruction & 0x1f)
	{
	case 16:	r4300i_mfhi(Instruction); break;
	case 18:	r4300i_mflo(Instruction); break;
	case 17:	r4300i_mthi(Instruction); break;
	case 19:	r4300i_mtlo(Instruction); break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_mf_mt(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	int Use32bit = 0;
	/*~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;

#ifdef SAFE_LOADSTORE
	SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET(r4300i_mf_mt);
	return;
#endif
	if
	(
		(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
	&&	(((reg->code & 0x1f) == 19) || ((reg->code & 0x1f) == 17))
	)
	{
		Use32bit = 1;
		SetRdRsRt32bit(PASS_PARAMS);
	}
	else
		SetRdRsRt64bit(PASS_PARAMS);

	_SAFTY_CPU_(r4300i_mf_mt)
	/*
	 * note: RD (or RS) is a misnomer. It really corresponds to LO (or HI), £
	 * but it works in our Mapping macros £
	 * MFHI(16) | MTHI(17) | MFLO(18) | MTLO(19)
	 */
	switch(reg->code & 0x1f)
	{
	case 16:	if(xRD->mips_reg == 0) return; else xRS->mips_reg = __HI; break;	/* mfhi */
	case 18:	if(xRD->mips_reg == 0) return; else xRS->mips_reg = __LO; break;	/* mflo */
	case 17:	xRD->mips_reg = __HI; break;	/* mthi */
	case 19:	xRD->mips_reg = __LO; break;	/* mtlo */
	}

	xRD->IsDirty = 1;
	xRD->NoNeedToLoadTheLo = 1;
	xRD->NoNeedToLoadTheHi = 1;

	MapRD;

	if(ConstMap[xRS->mips_reg].IsMapped)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		__int32 temp = ConstMap[xRS->mips_reg].value;
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		MOV_ImmToReg(1, xRD->x86reg, temp);
		if(!Use32bit) MOV_ImmToReg(1, xRD->HiWordLoc, (temp >> 31));
	}
	else
	{
		/* we checked if rs is a const above already. good. */
		MapRS_To(xRD, Use32bit, MOV_MemoryToReg);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_mul(uint32 Instruction)
{
	switch(Instruction & 0x3f)
	{
	case 24:	r4300i_mult(Instruction); break;
	case 25:	r4300i_multu(Instruction); break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_mul(OP_PARAMS)
{
	/*~~~~~~~~~~~~*/
	int High = __HI;
	int Low = __LO;
	/*~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 2;
	_SAFTY_CPU_(r4300i_mul)
#ifdef SAFE_MATH
	INTERPRET(r4300i_mul);
	return;
#endif
	SetRdRsRt32bit(PASS_PARAMS);

	FreeMipsRegister(Low);
	FreeMipsRegister(High);

	xRD->IsDirty = 1;
	xRD->mips_reg = __LO;
	xRD->Is32bit = 0;
	xRD->NoNeedToLoadTheLo = 1;
	xRD->NoNeedToLoadTheHi = 1;

	MapRD;
	MapRS;
	MapRT;

	if((xRD->x86reg != Reg_EAX) && (xRD->HiWordLoc != Reg_EAX)) PUSH_RegIfMapped(Reg_EAX);
	if((xRD->x86reg != Reg_EDX) && (xRD->HiWordLoc != Reg_EDX)) PUSH_RegIfMapped(Reg_EDX);

	if(xRT->x86reg == Reg_EAX)
	{
		switch(__F)
		{
		case 24:	/* case 32: */IMUL_EAXWithReg(1, xRS->x86reg); break;
		case 25:	/* case 33: */MUL_EAXWithReg(1, xRS->x86reg); break;
		}
	}
	else
	{
		MOV_Reg2ToReg1(1, Reg_EAX, xRS->x86reg);
		switch(__F)
		{
		case 24:	/* case 32: */IMUL_EAXWithReg(1, xRT->x86reg); break;
		case 25:	/* case 33: */MUL_EAXWithReg(1, xRT->x86reg); break;
		}
	}

	if((xRD->HiWordLoc == Reg_EAX) || (xRD->x86reg == Reg_EDX))
	{
		High = __LO;
		Low = __HI;
		MOV_Reg2ToReg1(1, xRD->HiWordLoc, Reg_EAX);
		MOV_Reg2ToReg1(1, xRD->x86reg, Reg_EDX);
	}
	else
	{
		MOV_Reg2ToReg1(1, xRD->x86reg, Reg_EAX);
		MOV_Reg2ToReg1(1, xRD->HiWordLoc, Reg_EDX);
	}

	if((xRD->x86reg != Reg_EDX) && (xRD->HiWordLoc != Reg_EDX)) POP_RegIfMapped(Reg_EDX);
	if((xRD->x86reg != Reg_EAX) && (xRD->HiWordLoc != Reg_EAX)) POP_RegIfMapped(Reg_EAX);

	/* split up the Lo and Hi into 2 registers. */
	x86reg[xRD->HiWordLoc].IsDirty = 1;
	x86reg[xRD->HiWordLoc].Is32bit = 1;
	x86reg[xRD->HiWordLoc].mips_reg = High;
	x86reg[xRD->HiWordLoc].HiWordLoc = xRD->HiWordLoc;

	x86reg[xRD->x86reg].IsDirty = 1;
	x86reg[xRD->x86reg].Is32bit = 1;
	x86reg[xRD->x86reg].mips_reg = Low;
	x86reg[xRD->x86reg].HiWordLoc = xRD->x86reg;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_div(OP_PARAMS)
{
	/*~~~~~~~~*/
	int temp, k;
	/*~~~~~~~~*/

	compilerstatus.cp0Counter += 75;
	_SAFTY_CPU_(r4300i_div)
#ifdef SAFE_MATH
	INTERPRET(r4300i_div);
	return;
#endif
	SetRdRsRt64bit(PASS_PARAMS);

	FreeMipsRegister(__LO);
	FreeMipsRegister(__HI);

	if(ConstMap[__RS].IsMapped) FlushOneConstant(__RS);
	if(ConstMap[__RT].IsMapped) FlushOneConstant(__RT);
	if((temp = CheckWhereIsMipsReg(__RS)) > -1) FlushRegister(temp);
	if((temp = CheckWhereIsMipsReg(__RT)) > -1) FlushRegister(temp);
	if(x86reg[0].mips_reg > -1)
		FlushRegister(0);
	else if(x86reg[0].mips_reg == -2)
	{
		for(k = 0; k < 8; k++)
		{
			if(ItIsARegisterNotToUse(k));

			/* Don't use these registers for mapping */
			else if(x86reg[k].HiWordLoc == 0)
			{
				FlushRegister(k);
				k = 9;
			}
		}
	}

	if(x86reg[2].mips_reg > -1)
		FlushRegister(2);
	else if(x86reg[2].mips_reg == -2)
	{
		for(k = 0; k < 8; k++)
		{
			if(ItIsARegisterNotToUse(k));

			/* Don't use these registers for mapping */
			else if(x86reg[k].HiWordLoc == 2)
			{
				FlushRegister(k);
				k = 9;
			}
		}
	}

	PUSH_RegIfMapped(Reg_ECX);
	LoadLowMipsCpuRegister(__RT, Reg_ECX);
	TEST_Reg2WithReg1(1, Reg_ECX, Reg_ECX);

	Jcc_auto(CC_E, 12);
	LoadLowMipsCpuRegister(__RS, Reg_EAX);
	MOV_Reg2ToReg1(1, Reg_EDX, Reg_EAX);
	SAR_RegByImm(1, Reg_EDX, 0x1F);

	IDIV_EAXWithReg(1, Reg_ECX);

	x86reg[0].IsDirty = 1;
	x86reg[0].Is32bit = 1;
	x86reg[0].mips_reg = __LO;
	x86reg[0].BirthDate = ThisYear;
	x86reg[0].HiWordLoc = 0;

	x86reg[2].IsDirty = 1;
	x86reg[2].Is32bit = 1;
	x86reg[2].mips_reg = __HI;
	x86reg[2].BirthDate = ThisYear;
	x86reg[2].HiWordLoc = 2;

	SetTarget(12);
	POP_RegIfMapped(Reg_ECX);

	SAVE_OP_COUNTER_INCREASE(PCLOCKDIV * 2 * VICounterFactors[CounterFactor]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_divu(OP_PARAMS)
{
	/*~~~~~~~~*/
	int temp, k;
	/*~~~~~~~~*/

	compilerstatus.cp0Counter += 75;
	_SAFTY_CPU_(r4300i_divu)
#ifdef SAFE_MATH
	INTERPRET(r4300i_divu);
	return;
#endif
	SetRdRsRt64bit(PASS_PARAMS);

	/*
	 * FlushAllRegisters(); £
	 * Cheesy alternative to FlushAllRegisters() but more efficient.
	 */
	FreeMipsRegister(__LO);
	FreeMipsRegister(__HI);
	if(ConstMap[__RS].IsMapped) FlushOneConstant(__RS);
	if(ConstMap[__RT].IsMapped) FlushOneConstant(__RT);
	if((temp = CheckWhereIsMipsReg(__RS)) > -1) FlushRegister(temp);
	if((temp = CheckWhereIsMipsReg(__RT)) > -1) FlushRegister(temp);
	if(x86reg[0].mips_reg > -1)
		FlushRegister(0);
	else if(x86reg[0].mips_reg == -2)
	{
		for(k = 0; k < 8; k++)
		{
			if(ItIsARegisterNotToUse(k));

			/* Don't use these registers for mapping */
			else if(x86reg[k].HiWordLoc == 0)
			{
				FlushRegister(k);
				k = 9;
			}
		}
	}

	if(x86reg[2].mips_reg > -1)
		FlushRegister(2);
	else if(x86reg[2].mips_reg == -2)
	{
		for(k = 0; k < 8; k++)
		{
			if(ItIsARegisterNotToUse(k));

			/* Don't use these registers for mapping */
			else if(x86reg[k].HiWordLoc == 2)
			{
				FlushRegister(k);
				k = 9;
			}
		}
	}

	PUSH_RegIfMapped(Reg_ECX);
	LoadLowMipsCpuRegister(__RT, Reg_ECX);
	TEST_Reg2WithReg1(1, Reg_ECX, Reg_ECX);

	Jcc_auto(CC_E, 0);
	LoadLowMipsCpuRegister(__RS, Reg_EAX);
	XOR_Reg2ToReg1(1, Reg_EDX, Reg_EDX);
	DIV_EAXWithReg(1, Reg_ECX);

	x86reg[0].IsDirty = 1;
	x86reg[0].Is32bit = 1;
	x86reg[0].mips_reg = __LO;
	x86reg[0].BirthDate = ThisYear;
	x86reg[0].HiWordLoc = 0;

	x86reg[2].IsDirty = 1;
	x86reg[2].Is32bit = 1;
	x86reg[2].mips_reg = __HI;
	x86reg[2].BirthDate = ThisYear;
	x86reg[2].HiWordLoc = 2;

	SetTarget(0);
	POP_RegIfMapped(Reg_ECX);

	SAVE_OP_COUNTER_INCREASE(PCLOCKDIV * 2 * VICounterFactors[CounterFactor]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MipsAdd(subtraction)
{
	if((ConstMap[xRT->mips_reg].IsMapped == 1) && (ConstMap[xRS->mips_reg].IsMapped == 1))
	{
		if(!subtraction)
			MapConst(xRD, (ConstMap[xRS->mips_reg].value + ConstMap[xRT->mips_reg].value));
		else
			MapConst(xRD, (ConstMap[xRS->mips_reg].value - ConstMap[xRT->mips_reg].value));
	}
	else
	{
		if(xRD->mips_reg == xRT->mips_reg)
		{
			if(xRS->mips_reg == xRT->mips_reg)	/* rd=rs=rt */
			{
				if(ConstMap[xRT->mips_reg].IsMapped == 1)
				{
					if(subtraction)
						(_s32) ConstMap[xRT->mips_reg].value -= (_s32) ConstMap[xRT->mips_reg].value;
					else
						(_s32) ConstMap[xRT->mips_reg].value += (_s32) ConstMap[xRT->mips_reg].value;
				}
				else
				{
					if(subtraction)
					{
						FreeMipsRegister(xRD->mips_reg);
						MapConst(xRD, 0);
					}
					else
					{
						xRD->IsDirty = 1;
						MapRD;
						SHL_RegBy1(1, xRD->x86reg);
					}
				}
			}
			else						/* rd=rt, rs!=rt */
			{
				if(xRS->mips_reg == 0)
					if(subtraction)
					{
						xRD->IsDirty = 1;
						MapRD;
						if(subtraction)
						{
							NEGATE(xRD)
						}

						return;
					}
					else
						return;

				xRD->IsDirty = 1;
				MapRD;
				if(subtraction)
				{
					NEGATE(xRD)
				}

				if(ConstMap[xRS->mips_reg].IsMapped)
				{
					ADD_ImmToReg(1, xRD->x86reg, ConstMap[xRS->mips_reg].value);
				}
				else
				{
					/* we checked if rs is a const above, good. */
					MapRS_To(xRD, 1, ADD_MemoryToReg);
				}
			}
		}
		else							/* rd != rt */
		{
			if(xRD->mips_reg == xRS->mips_reg)
			{
				if(xRT->mips_reg == 0) return;
				if(ConstMap[xRT->mips_reg].IsMapped)
				{
					xRD->IsDirty = 1;
					MapRD;

					if(!subtraction)	/* add */
						ADD_ImmToReg(1, xRD->x86reg, ConstMap[xRT->mips_reg].value);
					else
					{
						ADD_ImmToReg(1, xRD->x86reg, -ConstMap[xRT->mips_reg].value);
					}
				}
				else
				{
					if(ConstMap[xRS->mips_reg].IsMapped)
					{
						/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
						int temp = ConstMap[xRS->mips_reg].value;
						/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

						xRD->NoNeedToLoadTheLo = 1;
						xRD->IsDirty = 1;
						MapRD;
						MapRT;
						if(subtraction) temp = -temp;
						LEA(1, xRD->x86reg, (uint8) (ModRM_disp32_EAX + xRT->x86reg), temp);
						if(subtraction) NEGATE(xRD);
						return;
					}

					xRD->IsDirty = 1;
					MapRD;

					if(!subtraction)	/* add */
					{
						MapRT_To(xRD, 1, ADD_MemoryToReg);
					}
					else
					{
						MapRT_To(xRD, 1, SUB_MemoryToReg);
					}
				}
			}
			else						/* rd != rs */
			{
				xRD->IsDirty = 1;
				xRD->NoNeedToLoadTheLo = 1;
				MapRD;
				if(xRS->mips_reg == xRT->mips_reg)
				{
					if(xRT->mips_reg == 0)	/* rd = 0+0 */
					{
						XOR_Reg2ToReg1(1, xRD->x86reg, xRD->x86reg);	/* xor rd, rd */
					}
					else if(subtraction)		/* rd = rs-rs */
					{
						XOR_Reg2ToReg1(1, xRD->x86reg, xRD->x86reg);
					}
					else						/* rd = rs+rs */
					{
						/* we checked RT is mapped above (when rs==rt). good. */
						Map_RD_NE_RT();			/* MapRT_To(xRD, 1, MOV_MemoryToReg) */
						SHL_RegBy1(1, xRD->x86reg);
					}
				}
				else							/* rs != rt */
				{
					if(xRS->mips_reg != 0)
					{
						if(ConstMap[xRS->mips_reg].IsMapped)
						{
							MOV_ImmToReg(1, xRD->x86reg, ConstMap[xRS->mips_reg].value);
						}
						else
						{
							MapRS_To(xRD, 1, MOV_MemoryToReg)
						}

						if(xRT->mips_reg == 0) return;

						if(ConstMap[xRT->mips_reg].IsMapped)
						{
							/* rs!=0, rt!=0 , rs!=rt */
							if(!subtraction)	/* add */
								ADD_ImmToReg(1, xRD->x86reg, ConstMap[xRT->mips_reg].value);
							else
							{
								ADD_ImmToReg(1, xRD->x86reg, -ConstMap[xRT->mips_reg].value);
							}
						}
						else
						{
							if(!subtraction)	/* add */
							{
								MapRT_To(xRD, 1, ADD_MemoryToReg);
							}
							else
							{
								MapRT_To(xRD, 1, SUB_MemoryToReg);
							}
						}
					}
					else						/* rs=0, rt!=0 */
					{
						if(ConstMap[xRT->mips_reg].IsMapped)
						{
							if(!subtraction)
								MapConst(xRD, ConstMap[xRT->mips_reg].value);
							else
								MapConst(xRD, -ConstMap[xRT->mips_reg].value);
							return;
						}

						Map_RD_NE_RT();			/* MapRT_To(xRD, 1, MOV_MemoryToReg) */
						if(subtraction)
						{
							NEGATE(xRD)
						}
					}
				}
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_add(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_add) if(xRD->mips_reg == 0)
		return;
#ifdef SAFE_MATH
	INTERPRET(r4300i_add);
	return;
#endif
	MipsAdd(0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_addu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_addu) if(xRD->mips_reg == 0)
		return;
#ifdef SAFE_MATH
	INTERPRET(r4300i_addu);
	return;
#endif
	MipsAdd(0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_sub(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_sub) if(xRD->mips_reg == 0)
		return;
#ifdef SAFE_MATH
	INTERPRET(r4300i_sub);
	return;
#endif
	MipsAdd(1);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_subu(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt32bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_subu) if(xRD->mips_reg == 0)
		return;
#ifdef SAFE_MATH
	INTERPRET(r4300i_subu);
	return;
#endif
	MipsAdd(1);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_and(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	int Use32bit = 0;
	/*~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;

	InitRdRsRt(PASS_PARAMS);
	Use32bit = ( (CheckIs32Bit(xRS->mips_reg) && CheckIs32Bit(xRT->mips_reg)) | (currentromoptions.Assume_32bit == ASSUME_32BIT_YES));
	if (Use32bit)Set32bit(PASS_PARAMS);

	if(xRD->mips_reg == 0) return;

	_SAFTY_CPU_(r4300i_and)
#ifdef SAFE_GATES
	INTERPRET(r4300i_and);
	return;
#endif

	if((ConstMap[xRS->mips_reg].IsMapped == 1) && (ConstMap[xRT->mips_reg].IsMapped == 1))
		MapConst(xRD, (ConstMap[xRS->mips_reg].value & ConstMap[xRT->mips_reg].value));
	else if(xRD->mips_reg == xRT->mips_reg)
	{
		if(xRS->mips_reg == xRT->mips_reg)
		{
		}
		else if(xRD->mips_reg != xRS->mips_reg)
		{
			xRD->IsDirty = 1;
			MapRD;
			MapRS;

			AND_Reg2ToReg1(1, xRD->x86reg, xRS->x86reg);
			if(!Use32bit) AND_Reg2ToReg1(1, xRD->HiWordLoc, xRS->HiWordLoc);
		}
	}
	else if(xRD->mips_reg == xRS->mips_reg)
	{
		xRD->IsDirty = 1;
		MapRD;

		if(ConstMap[xRT->mips_reg].IsMapped)
		{
			AND_ImmToReg(1, xRD->x86reg, ConstMap[xRT->mips_reg].value);
			if(!Use32bit) AND_ImmToReg(1, xRD->HiWordLoc, ((_int32) ConstMap[xRT->mips_reg].value) >> 31);
		}
		else
			/* we checked if rt is a const above already. good. */
			MapRT_To(xRD, Use32bit, AND_MemoryToReg);
	}
	else
	{
		if(ConstMap[xRS->mips_reg].IsMapped)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			__int32 temp = (__int32) ConstMap[xRS->mips_reg].value;
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			xRD->NoNeedToLoadTheHi = 1;
			xRD->NoNeedToLoadTheLo = 1;
			xRD->IsDirty = 1;
			MapRD;
			MapRT;

			MOV_ImmToReg(1, xRD->x86reg, temp);
			if(!Use32bit) MOV_ImmToReg(1, xRD->HiWordLoc, (temp >> 31));
			AND_Reg2ToReg1(1, xRD->x86reg, xRT->x86reg);
			if(!Use32bit) AND_Reg2ToReg1(1, xRD->HiWordLoc, xRT->HiWordLoc);
		}
		else	/* rd != rt, rd != rs */
		{
			xRD->NoNeedToLoadTheHi = 1;
			xRD->NoNeedToLoadTheLo = 1;
			xRD->IsDirty = 1;
			MapRD;

			/* we checked if rs is a const above already. good. */
			MapRS_To(xRD, Use32bit, MOV_MemoryToReg);

			if(ConstMap[xRT->mips_reg].IsMapped)
			{
				AND_ImmToReg(1, xRD->x86reg, ConstMap[xRT->mips_reg].value);
				if(!Use32bit) AND_ImmToReg(1, xRD->HiWordLoc, ((_int32) ConstMap[xRT->mips_reg].value) >> 31);
			}
			else
				/* we checked if rt is a const above already. good. */
				MapRT_To(xRD, Use32bit, AND_MemoryToReg);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_or(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	int Use32bit = 0;
	/*~~~~~~~~~~~~~*/

	InitRdRsRt(PASS_PARAMS);
	Use32bit = ( (CheckIs32Bit(xRS->mips_reg) && CheckIs32Bit(xRT->mips_reg)) | (currentromoptions.Assume_32bit == ASSUME_32BIT_YES));
	if (Use32bit)Set32bit(PASS_PARAMS);

	compilerstatus.cp0Counter += 1;
	if(xRD->mips_reg == 0) return;

	_SAFTY_CPU_(r4300i_or)
#ifdef SAFE_GATES
	INTERPRET(r4300i_or);
	return;
#endif
#define _Or 0x0B

	if((ConstMap[xRS->mips_reg].IsMapped == 1) && (ConstMap[xRT->mips_reg].IsMapped == 1))
	{
		MapConst(xRD, (ConstMap[xRS->mips_reg].value | ConstMap[xRT->mips_reg].value));
		return;
	}

	if(xRD->mips_reg == xRT->mips_reg)
	{
		if(xRS->mips_reg == xRT->mips_reg)
		{
		}
		else
		{
			xRD->IsDirty = 1;
			MapRD;
			MapRS;

			OR_Reg2ToReg1(1, xRD->x86reg, xRS->x86reg);
			if(!Use32bit) OR_Reg2ToReg1(1, xRD->HiWordLoc, xRS->HiWordLoc);
		}
	}
	else
	{
		if(xRD->mips_reg == xRS->mips_reg)
		{
			xRD->IsDirty = 1;
			MapRD;
			if(ConstMap[xRT->mips_reg].IsMapped)
			{
				OR_ImmToReg(1, xRD->x86reg, ConstMap[xRT->mips_reg].value);
				if(!Use32bit) OR_ImmToReg(1, xRD->HiWordLoc, (ConstMap[xRT->mips_reg].value) >> 31);
			}
			else
				MapRT_To(xRD, Use32bit, OR_MemoryToReg);
		}
		else
		{
			if(xRS->mips_reg != xRT->mips_reg)	/* rs!=rt, rd!=rt, rd!=rs */
			{
				if((xRT->mips_reg == 0))
				{
					if(ConstMap[xRS->mips_reg].IsMapped == 1)
					{
						MapConst(xRD, ConstMap[xRS->mips_reg].value);
					}

					/* we checked if rs is a const above already. good. */
					else
					{
						/* Conker!! */
						xRD->NoNeedToLoadTheLo = 1;
						xRD->NoNeedToLoadTheHi = 1;
						xRD->IsDirty = 1;
						MapRD;

						MapRS_To(xRD, Use32bit, MOV_MemoryToReg);
					}
				}
				else
				{
					xRD->NoNeedToLoadTheLo = 1;
					xRD->NoNeedToLoadTheHi = 1;
					xRD->IsDirty = 1;
					MapRD;

					if(ConstMap[xRS->mips_reg].IsMapped)
					{
						MOV_ImmToReg(1, xRD->x86reg, ConstMap[xRS->mips_reg].value);
						if(!Use32bit) MOV_ImmToReg(1, xRD->HiWordLoc, (ConstMap[xRS->mips_reg].value) >> 31);
					}

					/* we checked if rs is a const above already. good. */
					else
						MapRS_To(xRD, Use32bit, MOV_MemoryToReg);

					if(ConstMap[xRT->mips_reg].IsMapped)
					{
						OR_ImmToReg(1, xRD->x86reg, ConstMap[xRT->mips_reg].value);
						if(!Use32bit) OR_ImmToReg(1, xRD->HiWordLoc, (_int32) ConstMap[xRT->mips_reg].value >> 31);
					}
					else
					{
						/* we checked if rt is a const above already. good. */
						MapRT_To(xRD, Use32bit, OR_MemoryToReg);
					}
				}
			}
			else	/* rs = rt */
			{
				if(ConstMap[xRS->mips_reg].IsMapped == 1)
				{
					if(ConstMap[xRD->mips_reg].IsMapped)
					{
						MapConst(xRD, ConstMap[xRS->mips_reg].value);
					}
					else
					{
						xRD->NoNeedToLoadTheLo = 1;
						xRD->NoNeedToLoadTheHi = 1;
						xRD->IsDirty = 1;
						MapRD;
						MOV_ImmToReg(1, xRD->x86reg, ConstMap[xRS->mips_reg].value);
						if(!Use32bit) MOV_ImmToReg(1, xRD->HiWordLoc, (ConstMap[xRS->mips_reg].value) >> 31);
					}
				}
				else
				{
					xRD->NoNeedToLoadTheLo = 1;
					xRD->NoNeedToLoadTheHi = 1;
					xRD->IsDirty = 1;
					MapRD;

					/* we checked if rs is a const above already. good. */
					MapRS_To(xRD, Use32bit, MOV_MemoryToReg);
				}
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_xor(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	int Use32bit = 0;
	/*~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_xor)
#ifdef SAFE_GATES
	INTERPRET(r4300i_xor);
	return;
#endif
	if(xRD->mips_reg == 0) return;

	if
	(
		(CheckIs32Bit(xRS->mips_reg))
	&&	(CheckIs32Bit(xRT->mips_reg))
	||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
	)
	{
		Use32bit = 1;
		xRD->Is32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	if(ConstMap[xRS->mips_reg].IsMapped && ConstMap[xRT->mips_reg].IsMapped)
	{
		MapConst(xRD, ConstMap[xRS->mips_reg].value ^ ConstMap[xRT->mips_reg].value);
	}
	else if(xRS->mips_reg == xRT->mips_reg)
	{
		MapConst(xRD, 0);
	}

	/*
	 * RS is const £
	 */
	else if(ConstMap[xRS->mips_reg].IsMapped)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		__int32 rsConst = (__int32) ConstMap[xRS->mips_reg].value;
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		if(ConstMap[xRT->mips_reg].IsMapped)
		{
			MapConst(xRD, (rsConst ^ ConstMap[xRT->mips_reg].value));
		}
		else if(xRD->mips_reg == xRT->mips_reg)
		{
			xRD->IsDirty = 1;
			MapRD;

			XOR_ImmToReg(1, xRD->x86reg, rsConst);
			if(!Use32bit) XOR_ImmToReg(1, xRD->x86reg, (rsConst >> 31));
		}
		else if(xRD->mips_reg == xRS->mips_reg)
		{
			xRD->IsDirty = 1;
			MapRD;

			/* we checked if rt s a const (when rs is a const) above. good. */
			MapRT_To(xRD, Use32bit, XOR_MemoryToReg);
		}
		else
		{
			xRD->NoNeedToLoadTheHi = 1;
			xRD->NoNeedToLoadTheLo = 1;
			xRD->IsDirty = 1;

			MapRD;

			/* we checked if rt s a const (when rs is a const) above. good. */
			MapRT_To(xRD, Use32bit, MOV_MemoryToReg);

			XOR_ImmToReg(1, xRD->x86reg, rsConst);
			if(!Use32bit) XOR_ImmToReg(1, xRD->HiWordLoc, (rsConst >> 31));
		}
	}

	/*
	 * RT is const £
	 */
	else if(ConstMap[xRT->mips_reg].IsMapped)
	{
		if(xRD->mips_reg == xRT->mips_reg)
		{
			xRD->IsDirty = 1;
			MapRD;
			MapRS;

			XOR_Reg2ToReg1(1, xRD->x86reg, xRS->x86reg);
			if(!Use32bit) XOR_Reg2ToReg1(1, xRD->HiWordLoc, xRS->HiWordLoc);
		}
		else if(xRD->mips_reg == xRS->mips_reg)
		{
			xRD->IsDirty = 1;
			MapRD;

			XOR_ImmToReg(1, xRD->x86reg, ConstMap[xRT->mips_reg].value);
			if(!Use32bit) XOR_ImmToReg(1, xRD->HiWordLoc, (_int32) ConstMap[xRT->mips_reg].value >> 31);
		}
		else
		{
			xRD->NoNeedToLoadTheHi = 1;
			xRD->NoNeedToLoadTheLo = 1;
			xRD->IsDirty = 1;

			MapRD;
			MapRS_To(xRD, Use32bit, MOV_MemoryToReg);

			XOR_ImmToReg(1, xRD->x86reg, ConstMap[xRT->mips_reg].value);
			if(!Use32bit) XOR_ImmToReg(1, xRD->HiWordLoc, (_int32) ConstMap[xRT->mips_reg].value >> 31);
		}
	}

	/*
	 * Neither operands are consts £
	 */
	else if(xRD->mips_reg == xRT->mips_reg)
	{
		xRD->IsDirty = 1;
		MapRD;

		/* we checked if rs is a const above already. good. */
		MapRS_To(xRD, Use32bit, XOR_MemoryToReg);
	}
	else if(xRD->mips_reg == xRS->mips_reg)
	{
		xRD->IsDirty = 1;
		MapRD;

		/* we checked if rt is a const above already. good. */
		MapRT_To(xRD, Use32bit, XOR_MemoryToReg);
	}
	else	/* rd!=rt, rd!=rs, */
	{
		xRD->NoNeedToLoadTheHi = 1;
		xRD->NoNeedToLoadTheLo = 1;
		xRD->IsDirty = 1;

		MapRD;

		/* we checked if rs is a const above already. good. */
		MapRS_To(xRD, Use32bit, MOV_MemoryToReg);

		/* we checked if rt is a const above already. good. */
		MapRT_To(xRD, Use32bit, XOR_MemoryToReg);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_nor(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	int Use32bit = 0;
	/*~~~~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);
	if(xRD->mips_reg == 0) return;

	_SAFTY_CPU_(r4300i_nor)
#ifdef SAFE_GATES
	INTERPRET(r4300i_nor);
	return;
#endif
	if
	(
		(CheckIs32Bit(xRS->mips_reg))
	&&	(CheckIs32Bit(xRT->mips_reg))
	||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
	)
	{
		Use32bit = 1;
		xRD->Is32bit = 1;
		xRS->Is32bit = 1;
		xRT->Is32bit = 1;
	}

	if(xRD->mips_reg != 0)
	{
		if(xRD->mips_reg == xRT->mips_reg)
		{
			if(xRS->mips_reg == xRT->mips_reg)
			{
				xRD->IsDirty = 1;
				MapRD;

				XOR_ImmToReg(1, xRD->x86reg, 0xffffffff);
				if(!Use32bit) XOR_ImmToReg(1, xRD->HiWordLoc, 0xffffffff);
			}
			else
			{
				xRD->IsDirty = 1;
				MapRD;
				MapRS;

				OR_Reg2ToReg1(1, xRD->x86reg, xRS->x86reg);
				if(!Use32bit) OR_Reg2ToReg1(1, xRD->HiWordLoc, xRS->HiWordLoc);
				XOR_ImmToReg(1, xRD->x86reg, 0xffffffff);
				if(!Use32bit) XOR_ImmToReg(1, xRD->HiWordLoc, 0xffffffff);
			}
		}
		else
		{
			if(xRD->mips_reg == xRS->mips_reg)
			{
				xRD->IsDirty = 1;
				MapRD;
				MapRT;

				OR_Reg2ToReg1(1, xRD->x86reg, xRT->x86reg);
				if(!Use32bit) OR_Reg2ToReg1(1, xRD->HiWordLoc, xRT->HiWordLoc);
				XOR_ImmToReg(1, xRD->x86reg, 0xffffffff);
				if(!Use32bit) XOR_ImmToReg(1, xRD->HiWordLoc, 0xffffffff);
			}
			else	/* rd!=rt, rd!=rs */
			{
				xRD->NoNeedToLoadTheHi = 1;
				xRD->NoNeedToLoadTheLo = 1;
				xRD->IsDirty = 1;
				MapRD;
				MapRS;

				if(xRS->mips_reg != xRT->mips_reg)	/* rd!=rt, rd!=rs, rs!=rt */
				{
					MapRT;

					MOV_Reg2ToReg1(1, xRD->x86reg, xRS->x86reg);
					OR_Reg2ToReg1(1, xRD->x86reg, xRT->x86reg);
					XOR_ImmToReg(1, xRD->x86reg, 0xffffffff);
					if(!Use32bit)
					{
						MOV_Reg2ToReg1(1, xRD->HiWordLoc, xRS->HiWordLoc);
						OR_Reg2ToReg1(1, xRD->HiWordLoc, xRT->HiWordLoc);
						XOR_ImmToReg(1, xRD->HiWordLoc, 0xffffffff);
					}
				}
				else	/* rd!=rt, rd!=rs, rs=rt */
				{
					MOV_Reg2ToReg1(1, xRD->x86reg, xRS->x86reg);						/* mov rd,rs (lo) */
					if(!Use32bit) MOV_Reg2ToReg1(1, xRD->HiWordLoc, xRS->HiWordLoc);	/* mov rd,rs (hi) */
					XOR_ImmToReg(1, xRD->x86reg, 0xffffffff);
					if(!Use32bit) XOR_ImmToReg(1, xRD->HiWordLoc, 0xffffffff);
				}
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dsll32(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	int Use32bit = 0;
	/*~~~~~~~~~~~~~*/

#ifdef SAFE_DOUBLE_SHIFTS
	INTERPRET(r4300i_dsll32);
	return;
#endif
	compilerstatus.cp0Counter += 1;

	/* Do not assume 32bit for double shift left logical. */
	SetRdRsRt64bit(PASS_PARAMS);
	_SAFTY_CPU_(r4300i_dsll32) 
		
	if(xRD->mips_reg == 0)
		return;

	if(xRT->mips_reg == xRD->mips_reg)
	{
		xRD->NoNeedToLoadTheHi = 1;
		if(!Use32bit) xRD->IsDirty = 1;
		MapRD;
		if(!Use32bit) MOV_Reg2ToReg1(1, xRD->HiWordLoc, xRD->x86reg);
		XOR_Reg2ToReg1(1, xRD->x86reg, xRD->x86reg);
		if(!Use32bit) SHL_RegByImm(1, xRD->HiWordLoc, (_s8) __SA);
	}
	else
	{
		xRD->IsDirty = 1;
		xRD->NoNeedToLoadTheLo = 1;
		if(!Use32bit) xRD->NoNeedToLoadTheHi = 1;
		MapRD;

		xRT->Is32bit = 1;
		MapRT;

		if(!Use32bit) MOV_Reg2ToReg1(1, xRD->HiWordLoc, xRT->x86reg);
		XOR_Reg2ToReg1(1, xRD->x86reg, xRD->x86reg);
		if(!Use32bit) SHL_RegByImm(1, xRD->HiWordLoc, (_s8) __SA);
	}
}

extern void StoreMipsCpuRegister(unsigned long iMipsReg, unsigned char iIntelReg1, unsigned char iIntelReg2);

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void dyna4300i_special_dsrl32(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);

	_SAFTY_CPU_(r4300i_dsrl32)
#ifdef SAFE_DOUBLE_SHIFTS
	INTERPRET(r4300i_dsrl32);
	return;
#endif
	if(xRD->mips_reg == 0) return;

	if((CheckIs32Bit(xRT->mips_reg)) || (currentromoptions.Assume_32bit == ASSUME_32BIT_YES))
	{
		xRD->Is32bit = 1;
		MapConst(xRD, 0);
	}
	else
	{
		INTERPRET(r4300i_dsrl32);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_special_dsra32(OP_PARAMS)
{
	/*~~~~~~~~~~~~~*/
	int Use32bit = 0;
	/*~~~~~~~~~~~~~*/

#ifdef SAFE_DOUBLE_SHIFTS
	INTERPRET(r4300i_dsra32);
	return;
#endif
	compilerstatus.cp0Counter += 1;
	SetRdRsRt64bit(PASS_PARAMS);

	_SAFTY_CPU_(r4300i_dsra32) if(xRD->mips_reg == 0)
		return;

	if(xRD->mips_reg == xRT->mips_reg)
	{
		/* Ths can be done more efficiently since we only need to load the HiWord. */

		if(!Use32bit)
		{
			xRD->NoNeedToLoadTheLo = 1;
			xRD->IsDirty = 1;
			MapRD;
			MOV_Reg2ToReg1(1, xRD->x86reg, xRD->HiWordLoc);
			SAR_RegByImm(1, xRD->x86reg, (unsigned char) __SA);
			SAR_RegByImm(1, xRD->HiWordLoc, 31);
		}
		else
		{
			xRD->IsDirty = 1;
			MapRD;
			SAR_RegByImm(1, xRD->x86reg, 31);
		}

		xRD->IsDirty = 1;
		xRD->Is32bit = 1;
		MapRD;
	}
	else
	{
		if (!Use32bit) 
			xRD->NoNeedToLoadTheLo = 1;
		xRD->Is32bit = 1;
		xRD->IsDirty = 1;
		MapRD;
		MapRT;

		if(!Use32bit)
		{
			MOV_Reg2ToReg1(1, xRD->x86reg, xRT->HiWordLoc);
			SAR_RegByImm(1, xRD->x86reg, (unsigned char) __SA);
		}
		else
		{
			MOV_Reg2ToReg1(1, xRD->x86reg, xRT->x86reg);
			SAR_RegByImm(1, xRD->x86reg, 31);
		}
	}
}
