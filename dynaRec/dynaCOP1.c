/*$T dynaCOP1.c GC 1.136 03/09/02 16:56:16 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    This file converts floating-point MIPS® instructions to native x86 machine code by way of calling the functions seen
    in x86.c. Blocks of instructions are compiled at a time. (See compiler.c).
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
#include "../1964ini.h"
#include "../memory.h"
#include "regcache.h"
#include "dynaCOP1.h"
#include "dynaCPU.h"
#include "x86.h"
#include "dynalog.h"
#include "../compiler.h"

#define INTERPRET_64BIT(x) \
	INTERPRET_NOFLUSH(x); \
	return;

extern void COMPARE_Run(uint32 Inter_Opcode_Address, uint32 code);

#define _SAFTY_COP1_(x) \
	if(debug_opcode!=0) \
	{ \
		SetRdRsRt64bit(PASS_PARAMS); \
		COMPARE_Run((uint32) & x, reg->code); \
	}\

/*
 =======================================================================================================================
    define _SAFTY_COP1_(x) INTERPRET(x); return;
 =======================================================================================================================
 */
#define INTERPRET_NOFLUSH(OPCODE) \
	PushMap(); \
	MOV_ImmToReg(1, Reg_ECX, reg->code); \
	X86_CALL((uint32) & OPCODE); \
	PopMap();

#define INTERPRET_FLUSH1(OPCODE, reg1) \
	{ \
		int temp; \
		if(ConstMap[reg1].IsMapped) FlushOneConstant(reg1); \
		if((temp = CheckWhereIsMipsReg(reg1)) > -1) FlushRegister(temp); \
		PushMap(); \
		MOV_ImmToReg(1, Reg_ECX, reg->code); \
		X86_CALL((uint32) & OPCODE); \
		PopMap(); \
		return; \
	}

#define INTERPRET_LOADSTORE_NEW_FPU(OPCODE) Interpret_LoadStore_NewFPU((uint32) & OPCODE, xRD, xRS, xRT);

#define INTERPRET(OPCODE) \
	FlushAllRegisters(); \
	MOV_ImmToReg(1, Reg_ECX, reg->code); \
	X86_CALL((uint32) & OPCODE);

extern void			dyna4300i_reserved(OP_PARAMS);
extern void			HELP_cop1_bc1f(_u32 pc);
extern void			HELP_cop1_bc1t(_u32 pc);
extern void			HELP_cop1_bc1fl(_u32 pc);
extern void			HELP_cop1_bc1tl(_u32 pc);
extern void			HELP_Call(unsigned long function);
extern void			StoreMipsCpuRegister(unsigned long iMipsReg, unsigned char iIntelReg1, unsigned char iIntelReg2);
extern void			LoadMipsCpuRegister(unsigned long iMipsReg, unsigned char iIntelReg1, unsigned char iIntelReg2);

extern MapConstant	ConstMap[NUM_CONSTS];
extern x86regtyp	xRD[1];
extern x86regtyp	xRS[1];
extern x86regtyp	xRT[1];

void (*dyna4300i_cop1_D_instruction[64]) (OP_PARAMS);
void	dyna4300i_cop1_mfc1(OP_PARAMS);
void	dyna4300i_cop1_dmfc1(OP_PARAMS);
void	dyna4300i_cop1_cfc1(OP_PARAMS);
void	dyna4300i_cop1_mtc1(OP_PARAMS);
void	dyna4300i_cop1_dmtc1(OP_PARAMS);
void	dyna4300i_cop1_ctc1(OP_PARAMS);
void	dyna4300i_cop1_BC_instr(OP_PARAMS);
void	dyna4300i_cop1_S_instr(OP_PARAMS);
void	dyna4300i_cop1_D_instr(OP_PARAMS);
void	dyna4300i_cop1_W_instr(OP_PARAMS);
void	dyna4300i_cop1_L_instr(OP_PARAMS);
void	dyna4300i_cop1_bc1f(OP_PARAMS);
void	dyna4300i_cop1_bc1t(OP_PARAMS);
void	dyna4300i_cop1_bc1fl(OP_PARAMS);
void	dyna4300i_cop1_bc1tl(OP_PARAMS);
void	dyna4300i_cop1_add_s(OP_PARAMS);
void	dyna4300i_cop1_sub_s(OP_PARAMS);
void	dyna4300i_cop1_mul_s(OP_PARAMS);
void	dyna4300i_cop1_div_s(OP_PARAMS);
void	dyna4300i_cop1_sqrt_s(OP_PARAMS);
void	dyna4300i_cop1_abs_s(OP_PARAMS);
void	dyna4300i_cop1_mov_s(OP_PARAMS);
void	dyna4300i_cop1_neg_s(OP_PARAMS);
void	dyna4300i_cop1_roundl_s(OP_PARAMS);
void	dyna4300i_cop1_truncl_s(OP_PARAMS);
void	dyna4300i_cop1_ceill_s(OP_PARAMS);
void	dyna4300i_cop1_floorl_s(OP_PARAMS);
void	dyna4300i_cop1_roundw_s(OP_PARAMS);
void	dyna4300i_cop1_truncw_s(OP_PARAMS);
void	dyna4300i_cop1_ceilw_s(OP_PARAMS);
void	dyna4300i_cop1_floorw_s(OP_PARAMS);
void	dyna4300i_cop1_cvtd_s(OP_PARAMS);
void	dyna4300i_cop1_cvtw_s(OP_PARAMS);
void	dyna4300i_cop1_cvtl_s(OP_PARAMS);
void	dyna4300i_c_f_s(OP_PARAMS);
void	dyna4300i_c_un_s(OP_PARAMS);
void	dyna4300i_c_eq_s(OP_PARAMS);
void	dyna4300i_c_ueq_s(OP_PARAMS);
void	dyna4300i_c_olt_s(OP_PARAMS);
void	dyna4300i_c_ult_s(OP_PARAMS);
void	dyna4300i_c_ole_s(OP_PARAMS);
void	dyna4300i_c_ule_s(OP_PARAMS);
void	dyna4300i_c_sf_s(OP_PARAMS);
void	dyna4300i_c_ngle_s(OP_PARAMS);
void	dyna4300i_c_seq_s(OP_PARAMS);
void	dyna4300i_c_ngl_s(OP_PARAMS);
void	dyna4300i_c_lt_s(OP_PARAMS);
void	dyna4300i_c_nge_s(OP_PARAMS);
void	dyna4300i_c_le_s(OP_PARAMS);
void	dyna4300i_c_ngt_s(OP_PARAMS);
void	dyna4300i_cop1_add_d(OP_PARAMS);
void	dyna4300i_cop1_sub_d(OP_PARAMS);
void	dyna4300i_cop1_mul_d(OP_PARAMS);
void	dyna4300i_cop1_div_d(OP_PARAMS);
void	dyna4300i_cop1_sqrt_d(OP_PARAMS);
void	dyna4300i_cop1_abs_d(OP_PARAMS);
void	dyna4300i_cop1_mov_d(OP_PARAMS);
void	dyna4300i_cop1_neg_d(OP_PARAMS);
void	dyna4300i_cop1_roundl_d(OP_PARAMS);
void	dyna4300i_cop1_truncl_d(OP_PARAMS);
void	dyna4300i_cop1_ceill_d(OP_PARAMS);
void	dyna4300i_cop1_floorl_d(OP_PARAMS);
void	dyna4300i_cop1_roundw_d(OP_PARAMS);
void	dyna4300i_cop1_truncw_d(OP_PARAMS);
void	dyna4300i_cop1_ceilw_d(OP_PARAMS);
void	dyna4300i_cop1_floorw_d(OP_PARAMS);
void	dyna4300i_cop1_cvts_d(OP_PARAMS);
void	dyna4300i_cop1_cvtw_d(OP_PARAMS);
void	dyna4300i_cop1_cvtl_d(OP_PARAMS);
void	dyna4300i_c_f_d(OP_PARAMS);
void	dyna4300i_c_un_d(OP_PARAMS);
void	dyna4300i_c_eq_d(OP_PARAMS);
void	dyna4300i_c_ueq_d(OP_PARAMS);
void	dyna4300i_c_olt_d(OP_PARAMS);
void	dyna4300i_c_ult_d(OP_PARAMS);
void	dyna4300i_c_ole_d(OP_PARAMS);
void	dyna4300i_c_ule_d(OP_PARAMS);
void	dyna4300i_c_sf_d(OP_PARAMS);
void	dyna4300i_c_ngle_d(OP_PARAMS);
void	dyna4300i_c_seq_d(OP_PARAMS);
void	dyna4300i_c_ngl_d(OP_PARAMS);
void	dyna4300i_c_lt_d(OP_PARAMS);
void	dyna4300i_c_nge_d(OP_PARAMS);
void	dyna4300i_c_le_d(OP_PARAMS);
void	dyna4300i_c_ngt_d(OP_PARAMS);
void	dyna4300i_cop1_cvts_w(OP_PARAMS);
void	dyna4300i_cop1_cvtd_w(OP_PARAMS);
void	dyna4300i_cop1_cvts_l(OP_PARAMS);
void	dyna4300i_cop1_cvtd_l(OP_PARAMS);
void	dyna4300i_c_s(OP_PARAMS);
void	dyna4300i_c_d(OP_PARAMS);

/* \ Function Tables \ */
void (*dyna4300i_cop1_Instruction[32]) (OP_PARAMS) =
{
	dyna4300i_cop1_mfc1,
	dyna4300i_cop1_dmfc1,
	dyna4300i_cop1_cfc1,
	dyna4300i_reserved,
	dyna4300i_cop1_mtc1,
	dyna4300i_cop1_dmtc1,
	dyna4300i_cop1_ctc1,
	dyna4300i_reserved,
	dyna4300i_cop1_BC_instr,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_cop1_S_instr,
	dyna4300i_cop1_D_instr,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_cop1_W_instr,
	dyna4300i_cop1_L_instr,
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

void (*dyna4300i_cop1_BC_instruction[]) (OP_PARAMS) =
{
	dyna4300i_cop1_bc1f,
	dyna4300i_cop1_bc1t,
	dyna4300i_cop1_bc1fl,
	dyna4300i_cop1_bc1tl
};

void (*dyna4300i_cop1_S_instruction[]) (OP_PARAMS) =
{
	dyna4300i_cop1_add_s,
	dyna4300i_cop1_sub_s,
	dyna4300i_cop1_mul_s,
	dyna4300i_cop1_div_s,
	dyna4300i_cop1_sqrt_s,
	dyna4300i_cop1_abs_s,
	dyna4300i_cop1_mov_s,
	dyna4300i_cop1_neg_s,
	dyna4300i_cop1_roundl_s,
	dyna4300i_cop1_truncl_s,
	dyna4300i_cop1_ceill_s,
	dyna4300i_cop1_floorl_s,
	dyna4300i_cop1_roundw_s,
	dyna4300i_cop1_truncw_s,
	dyna4300i_cop1_ceilw_s,
	dyna4300i_cop1_floorw_s,
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
	dyna4300i_cop1_cvtd_s,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_cop1_cvtw_s,
	dyna4300i_cop1_cvtl_s,
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
	dyna4300i_c_f_s,
	dyna4300i_c_un_s,
	dyna4300i_c_eq_s,
	dyna4300i_c_ueq_s,
	dyna4300i_c_olt_s,
	dyna4300i_c_ult_s,
	dyna4300i_c_ole_s,
	dyna4300i_c_ule_s,
	dyna4300i_c_sf_s,
	dyna4300i_c_ngle_s,
	dyna4300i_c_seq_s,
	dyna4300i_c_ngl_s,
	dyna4300i_c_lt_s,
	dyna4300i_c_nge_s,
	dyna4300i_c_le_s,
	dyna4300i_c_ngt_s

	/*
	 * dyna4300i_c_s, dyna4300i_c_s, dyna4300i_c_s, dyna4300i_c_s, dyna4300i_c_s,
	 * dyna4300i_c_s, dyna4300i_c_s, dyna4300i_c_s, dyna4300i_c_s, dyna4300i_c_s,
	 * dyna4300i_c_s, dyna4300i_c_s, dyna4300i_c_s, dyna4300i_c_s, dyna4300i_c_s,
	 * dyna4300i_c_s
	 */
};

void (*dyna4300i_cop1_D_instruction[64]) (OP_PARAMS) =
{
	dyna4300i_cop1_add_d,
	dyna4300i_cop1_sub_d,
	dyna4300i_cop1_mul_d,
	dyna4300i_cop1_div_d,
	dyna4300i_cop1_sqrt_d,
	dyna4300i_cop1_abs_d,
	dyna4300i_cop1_mov_d,
	dyna4300i_cop1_neg_d,
	dyna4300i_cop1_roundl_d,
	dyna4300i_cop1_truncl_d,
	dyna4300i_cop1_ceill_d,
	dyna4300i_cop1_floorl_d,
	dyna4300i_cop1_roundw_d,
	dyna4300i_cop1_truncw_d,
	dyna4300i_cop1_ceilw_d,
	dyna4300i_cop1_floorw_d,
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
	dyna4300i_cop1_cvts_d,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_cop1_cvtw_d,
	dyna4300i_cop1_cvtl_d,
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

	/*
	 * dyna4300i_c_f_d, dyna4300i_c_un_d, dyna4300i_c_eq_d, dyna4300i_c_ueq_d,
	 * dyna4300i_c_olt_d, dyna4300i_c_ult_d, dyna4300i_c_ole_d, dyna4300i_c_ule_d,
	 * dyna4300i_c_sf_d, dyna4300i_c_ngle_d, dyna4300i_c_seq_d, dyna4300i_c_ngl_d,
	 * dyna4300i_c_lt_d, dyna4300i_c_nge_d, dyna4300i_c_le_d, dyna4300i_c_ngt_d
	 */
	dyna4300i_c_d,
	dyna4300i_c_d,
	dyna4300i_c_eq_d,
	dyna4300i_c_d,
	dyna4300i_c_d,
	dyna4300i_c_d,
	dyna4300i_c_d,
	dyna4300i_c_d,
	dyna4300i_c_sf_d,
	dyna4300i_c_ngle_d,
	dyna4300i_c_d,
	dyna4300i_c_d,
	dyna4300i_c_lt_d,
	dyna4300i_c_nge_d,
	dyna4300i_c_le_d,
	dyna4300i_c_ngt_d
};

void (*dyna4300i_cop1_W_instruction[64]) (OP_PARAMS) =
{
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
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_cop1_cvts_w,
	dyna4300i_cop1_cvtd_w,
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
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved
};
void (*dyna4300i_cop1_L_instruction[]) (OP_PARAMS) =
{
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
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_cop1_cvts_l,
	dyna4300i_cop1_cvtd_l,
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
	dyna4300i_reserved,
	dyna4300i_reserved,
	dyna4300i_reserved
};

/*
 =======================================================================================================================
    \ CoProcessor1 (COP1) \ £
    void (*dyna4300i_cop1_Instruction[])(OP_PARAMS)= //
 =======================================================================================================================
 */
void dyna4300i_cop1_mfc1(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_mfc1) SetRdRsRt32bit(PASS_PARAMS);

	if(xRT->mips_reg != 0)	/* mandatory */
	{
		ConstMap[xRT->mips_reg].IsMapped = 0;
		xRT->IsDirty = 1;
		xRT->NoNeedToLoadTheLo = 1;
		MapRT;
		MOV_MemoryToReg(1, (_u8) xRT->x86reg, ModRM_disp32, (unsigned long) &reg->fpr32[__FS]);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_dmfc1(OP_PARAMS)
{
	_SAFTY_COP1_(r4300i_COP1_dmfc1) SetRdRsRt64bit(PASS_PARAMS);

	INTERPRET_FLUSH1(r4300i_COP1_dmfc1, __RT);

	if(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
		MessageBox(0, "Need to compile dmfc1 for 32bit? Please use 64bit for now.", "", 0);
	if(xRT->mips_reg != 0)	/* mandatory */
	{
		ConstMap[xRT->mips_reg].IsMapped = 0;
		xRT->IsDirty = 1;
		xRT->NoNeedToLoadTheLo = 1;
		xRT->NoNeedToLoadTheHi = 1;
		MapRT;
		MOV_MemoryToReg(1, (_u8) xRT->x86reg, ModRM_disp32, (unsigned long) &reg->fpr32[__FS]);
		MOV_MemoryToReg(1, (_u8) xRT->HiWordLoc, ModRM_disp32, 4 + (unsigned long) &reg->fpr32[__FS]);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_cfc1(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cfc1) SetRdRsRt32bit(PASS_PARAMS);

	if(xRT->mips_reg != 0)					/* mandatory */
	{
		if(((__FS == 0) || (__FS == 31)))	/* This operation is only defined if */
		{
			ConstMap[xRT->mips_reg].IsMapped = 0;
			xRT->IsDirty = 1;
			MapRT;
			MOV_MemoryToReg(1, (_u8) xRT->x86reg, ModRM_disp32, (unsigned long) &reg->COP1Con[__FS]);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_ctc1(OP_PARAMS)
{
	/* SetRdRsRt32bit(PASS_PARAMS); */
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_ctc1) INTERPRET(r4300i_COP1_ctc1);
	return; /* because I have added rounding control */

	/*
	 * if (__RT == 31) { MapRT; MOV_RegToMemory(1, (_u8)xRT->x86reg, ModRM_disp32,
	 * (unsigned long)&reg->COP1Con[__FS]); }
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_mtc1(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_mtc1) SetRdRsRt32bit(PASS_PARAMS);

	if(ConstMap[xRT->mips_reg].IsMapped == 0)
	{
		MapRT;
		MOV_RegToMemory(1, (_u8) xRT->x86reg, ModRM_disp32, (unsigned long) &reg->fpr32[__FS]);
	}
	else
		MOV_ImmToMemory(1, ModRM_disp32, (unsigned long) &reg->fpr32[__FS], ConstMap[xRT->mips_reg].value);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_dmtc1(OP_PARAMS)
{
	_SAFTY_COP1_(r4300i_COP1_dmtc1) INTERPRET(r4300i_COP1_dmtc1);
}

extern void COP1_instr(uint32 Instruction);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_BC_instr(OP_PARAMS)
{
	dyna4300i_cop1_BC_instruction[(__ND << 1) | __TF](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_S_instr(OP_PARAMS)
{
	dyna4300i_cop1_S_instruction[__F](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_D_instr(OP_PARAMS)
{
	dyna4300i_cop1_D_instruction[__F](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_W_instr(OP_PARAMS)
{
	dyna4300i_cop1_W_instruction[__F](PASS_PARAMS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_L_instr(OP_PARAMS)
{
	dyna4300i_cop1_L_instruction[__F](PASS_PARAMS);
}

/*
 =======================================================================================================================
    void (*dyna4300i_cop1_S_instruction[])(OP_PARAMS)= //
 =======================================================================================================================
 */
void dyna4300i_cop1_add_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_add_s) 

	FLD_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FS]);
	FADD_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FT]);
	FSTP_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_sub_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_sub_s) 
	
	FLD_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FS]);
	FSUB_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FT]);
	FSTP_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_mul_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 4;
	_SAFTY_COP1_(r4300i_COP1_mul_s)
	
	FLD_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FS]);
	FMUL_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FT]);
	FSTP_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_div_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_div_s) FLD_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FS]);
	FDIV_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FT]);
	FSTP_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_sqrt_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_sqrt_s)
	
	FLD_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FS]);
	FSQRT();
	FSTP_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_abs_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_abs_s)
	
	FLD_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FS]);
	FABS();
	FSTP_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_mov_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	
	if (__FD == __FS) return;
	_SAFTY_COP1_(r4300i_COP1_mov_s)
		
	PUSH_RegIfMapped(Reg_EAX);
	MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (unsigned long) &reg->fpr32[__FS]);
	MOV_RegToMemory(1, Reg_EAX, ModRM_disp32, (unsigned long) &reg->fpr32[__FD]);
	POP_RegIfMapped(Reg_EAX);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_neg_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_neg_s)
		
	FLD_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FS]);
	FNEG();
	FSTP_Memory(FORMAT_SINGLE, (unsigned long) &reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_roundl_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_roundl_s)
		
	INTERPRET_NOFLUSH(r4300i_COP1_roundl_s);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_truncl_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_truncl_s)
		
	INTERPRET_NOFLUSH(r4300i_COP1_truncl_s);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_ceill_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_ceill_s)
		
	INTERPRET_NOFLUSH(r4300i_COP1_ceill_s);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_floorl_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_floorl_s)
		
	INTERPRET_NOFLUSH(r4300i_COP1_floorl_s);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_roundw_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_roundw_s)

	INTERPRET_NOFLUSH(r4300i_COP1_roundw_s);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_truncw_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_truncw_s)

	INTERPRET_NOFLUSH(r4300i_COP1_truncw_s);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_ceilw_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_ceilw_s)

	INTERPRET_NOFLUSH(r4300i_COP1_ceilw_s);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_floorw_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_floorw_s)

	INTERPRET_NOFLUSH(r4300i_COP1_floorw_s);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_cvtd_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cvtd_s) 
		
	INTERPRET_64BIT(r4300i_COP1_cvtd_s);

	FLD_Memory(FORMAT_SINGLE, (_u32) & reg->fpr32[__FS]);
	FSTP_Memory(FORMAT_QUAD, (_u32) & reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_cvtw_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cvtw_s)

	INTERPRET_NOFLUSH(r4300i_COP1_cvtw_s);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_cvtl_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cvtl_s)

	INTERPRET_NOFLUSH(r4300i_COP1_cvtl_s);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_f_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_F_S)

	INTERPRET_NOFLUSH(r4300i_C_F_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_un_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_UN_S)

	INTERPRET_NOFLUSH(r4300i_C_UN_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ueq_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_UEQ_S)

	INTERPRET_NOFLUSH(r4300i_C_UEQ_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_olt_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_OLT_S)

	INTERPRET_NOFLUSH(r4300i_C_OLT_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ult_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_ULT_S)

	INTERPRET_NOFLUSH(r4300i_C_ULT_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ole_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_OLE_S)

	INTERPRET_NOFLUSH(r4300i_C_OLE_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ule_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_ULE_S)

	INTERPRET_NOFLUSH(r4300i_C_ULE_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_sf_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_SF_S)

	INTERPRET_NOFLUSH(r4300i_C_SF_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ngle_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_NGLE_S)

	INTERPRET_NOFLUSH(r4300i_C_NGLE_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_seq_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_SEQ_S)

	INTERPRET_NOFLUSH(r4300i_C_SEQ_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ngl_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_NGL_S)

	INTERPRET_NOFLUSH(r4300i_C_NGL_S);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void _do_c_(HardwareState *reg, _u8 format, _u8 testval)
{
	PUSH_RegIfMapped(Reg_EAX);
	FLD_Memory(format, (unsigned long) &reg->fpr32[__FS]);
	FCOMP(format, (unsigned long) &reg->fpr32[__FT]);

	// * 00431CA5 DF E0 fnstsw ax £
	// * 00431CA7 F6 C4 01 test ah,1
	
	FNSTSW();
	WC16(0xC4F6);
	WC8(testval);
	LOGGING_DYNA(LogDyna("	TEST AH, 0x%x\n", testval););

	Jcc_auto(CC_E, 13);

	MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (_u32) & reg->COP1Con[31]);
	OR_ImmToReg(1, Reg_EAX, COP1_CONDITION_BIT);

	JMP_Short_auto(14);

	SetTarget(13);

	MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (_u32) & reg->COP1Con[31]);
	AND_ImmToReg(1, Reg_EAX, ~COP1_CONDITION_BIT);

	SetTarget(14);
	MOV_RegToMemory(1, Reg_EAX, ModRM_disp32, (_u32) & reg->COP1Con[31]);
	POP_RegIfMapped(Reg_EAX);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_lt_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_LT_S)

	_do_c_(reg, FORMAT_SINGLE, 0x01);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_nge_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_NGE_S)

	_do_c_(reg, FORMAT_SINGLE, 0x01);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_eq_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_EQ_S)

	_do_c_(reg, FORMAT_SINGLE, 0x40);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_le_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_LE_S)

	_do_c_(reg, FORMAT_SINGLE, 0x41);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ngt_s(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_NGT_S) 

	_do_c_(reg, FORMAT_SINGLE, 0x41);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_lt_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_LT_D) 
		
	INTERPRET_64BIT(r4300i_C_LT_D);
	_do_c_(reg, FORMAT_QUAD, 0x01);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_nge_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_NGE_D)

	INTERPRET_64BIT(r4300i_C_NGE_D);
	_do_c_(reg, FORMAT_QUAD, 0x01);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_eq_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_EQ_D)
	
	INTERPRET_64BIT(r4300i_C_EQ_D);
	_do_c_(reg, FORMAT_QUAD, 0x40);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_le_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_LE_D)
	
	INTERPRET_64BIT(r4300i_C_LE_D);
	_do_c_(reg, FORMAT_QUAD, 0x41);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ngt_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_NGT_D)
	
	INTERPRET_64BIT(r4300i_C_NGT_D);
	_do_c_(reg, FORMAT_QUAD, 0x41);
}

#define fdLOGIC(Sum, Operand1, OPERATOR16, Operand2) \
	WC16(0x05DD); \
	WC32((_u32) & Operand1); \
	WC16(OPERATOR16); \
	WC32((_u32) & Operand2); \
	WC16(0x1DDD); \
	WC32((_u32) & Sum);

#define fdoubleLOGICAL(OPERATOR16)	fdLOGIC(reg->fpr32[__FD], reg->fpr32[__FS], OPERATOR16, reg->fpr32[__FT])

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void dyna4300i_cop1_add_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_add_d)

	INTERPRET_64BIT(r4300i_COP1_add_d);
	fdoubleLOGICAL(0x05DC)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_sub_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_sub_d)
	
	INTERPRET_64BIT(r4300i_COP1_sub_d);
	fdoubleLOGICAL(0x25DC)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_mul_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 4;
	_SAFTY_COP1_(r4300i_COP1_mul_d)

	INTERPRET_64BIT(r4300i_COP1_mul_d);
	fdoubleLOGICAL(0x0DDC)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_div_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_div_d)

	INTERPRET_64BIT(r4300i_COP1_div_d);
	fdoubleLOGICAL(0x35DC)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_sqrt_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_sqrt_d)

	INTERPRET_NOFLUSH(r4300i_COP1_sqrt_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_abs_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_abs_d)

	INTERPRET_NOFLUSH(r4300i_COP1_abs_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_mov_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_mov_d)

	INTERPRET_NOFLUSH(r4300i_COP1_mov_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_neg_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_neg_d)

	INTERPRET_NOFLUSH(r4300i_COP1_neg_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_roundl_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_roundl_d)

	INTERPRET_NOFLUSH(r4300i_COP1_roundl_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_truncl_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_truncl_d)

	INTERPRET_NOFLUSH(r4300i_COP1_truncl_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_ceill_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_ceill_d)

	INTERPRET_NOFLUSH(r4300i_COP1_ceill_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_floorl_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_floorl_d)

	INTERPRET_NOFLUSH(r4300i_COP1_floorl_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_roundw_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_roundw_d)

	INTERPRET_NOFLUSH(r4300i_COP1_roundw_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_truncw_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_truncw_d)

	INTERPRET_NOFLUSH(r4300i_COP1_truncw_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_ceilw_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_ceilw_d)

	INTERPRET_NOFLUSH(r4300i_COP1_ceilw_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_floorw_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_floorw_d)

	INTERPRET_NOFLUSH(r4300i_COP1_floorw_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_cvts_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cvts_d);

	/*
	 * 00417A4A DD 05 C8 78 4A 00 fld qword ptr [_fpr32+8 (004a78c8)] £
	 * 00417A50 D9 1D C0 78 4A 00 fstp dword ptr [_fpr32 (004a78c0)]
	 */
	SetRdRsRt64bit(PASS_PARAMS);
	INTERPRET_64BIT(r4300i_COP1_cvts_d);

	WC16(0x05DD);
	WC32((_u32) & reg->fpr32[__FS]);
	WC16(0x1DD9);
	WC32((_u32) & reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_cvtw_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cvtw_d)

	INTERPRET_NOFLUSH(r4300i_COP1_cvtw_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_cvtl_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cvtl_d)

	INTERPRET_NOFLUSH(r4300i_COP1_cvtl_d);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_f_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_F_D)

	INTERPRET_NOFLUSH(r4300i_C_F_D);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_un_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_UN_D)

	INTERPRET_NOFLUSH(r4300i_C_UN_D);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ueq_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_UEQ_D)

	INTERPRET_NOFLUSH(r4300i_C_UEQ_D);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_olt_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_OLT_D)

	INTERPRET_NOFLUSH(r4300i_C_OLT_D);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ult_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_ULT_D)

	INTERPRET_NOFLUSH(r4300i_C_ULT_D);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ole_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_OLE_D)

	INTERPRET_NOFLUSH(r4300i_C_OLE_D);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ule_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_ULE_D)

	INTERPRET_NOFLUSH(r4300i_C_ULE_D);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_sf_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_SF_D)

	INTERPRET_NOFLUSH(r4300i_C_SF_D);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ngle_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_NGLE_D)

	INTERPRET_NOFLUSH(r4300i_C_NGLE_D);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_seq_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_SEQ_D)

	INTERPRET_NOFLUSH(r4300i_C_SEQ_D);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_ngl_d(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_C_NGL_D)

	INTERPRET_NOFLUSH(r4300i_C_NGL_D);
}

/*
 =======================================================================================================================
    void (*dyna4300i_cop1_W_instruction[])(OP_PARAMS)= //
 =======================================================================================================================
 */
void dyna4300i_cop1_cvts_w(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cvts_w)

	FILD_Memory(FORMAT_SINGLE, (_u32) & reg->fpr32[__FS]);
	FSTP_Memory(FORMAT_SINGLE, (_u32) & reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_cvtd_w(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cvtd_w)
	
	INTERPRET_64BIT(r4300i_COP1_cvtd_w);
	FILD_Memory(FORMAT_SINGLE, (_u32) & reg->fpr32[__FS]);
	FSTP_Memory(FORMAT_QUAD, (_u32) & reg->fpr32[__FD]);
}

/*
 =======================================================================================================================
    void (*dyna4300i_cop1_L_instruction[])(OP_PARAMS)= //
 =======================================================================================================================
 */
void dyna4300i_cop1_cvts_l(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cvts_l)

	INTERPRET_NOFLUSH(r4300i_COP1_cvts_l);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_cop1_cvtd_l(OP_PARAMS)
{
	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_COP1_cvtd_l)

	INTERPRET_NOFLUSH(r4300i_COP1_cvtd_l);
}

/* Load From / Store To CPR[Reg] // */
extern void HELP_Cop1(unsigned long pc);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_lwc1(OP_PARAMS)
{
	/*~~~~~~~~~~*/
	_s32	value;
	/*~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_lwc1);
	SetRdRsRt32bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE_FPU
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
			QuerAddr = (_u32) ((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I);

			if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

			value = LOAD_UWORD_PARAM(QuerAddr);

			PUSH_RegIfMapped(Reg_EAX);
			MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr));
			MOV_RegToMemory(1, Reg_EAX, ModRM_disp32, (_u32) & gHWS_fpr32[__FT]);
			POP_RegIfMapped(Reg_EAX);
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

			PUSH_RegIfMapped(Reg_EAX);
			PUSH_RegIfMapped(Reg_ECX);

			MOV_ImmToReg(1, Reg_EAX, temp + (_s32) (_s16) __dotI);
			MOV_ImmToReg(1, Reg_ECX, ((uint32) (temp + (_s32) (_s16) __dotI)) >> SHIFTER2_READ);
			WC16(0x14FF);
			WC8(0x8D);
			WC32((uint32) & memory_read_functions);
			LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););

			MOV_MemoryToReg(1, Reg_EAX, ModRM_EAX, 0);
			MOV_RegToMemory(1, Reg_EAX, ModRM_disp32, (uint32) & gHWS_fpr32[__RT]);

			POP_RegIfMapped(Reg_ECX);
			POP_RegIfMapped(Reg_EAX);
		}
		else
		{
			MapRS;
			PUSH_RegIfMapped(Reg_EAX);
			PUSH_RegIfMapped(Reg_ECX);

			MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);
			ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
			MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
			SHR_RegByImm(1, Reg_ECX, SHIFTER2_READ);
			WC16(0x14FF);
			WC8(0x8D);
			WC32((uint32) & memory_read_functions);
			LOGGING_DYNA(LogDyna("	CALL memory_read_functions[]\n"););

			MOV_MemoryToReg(1, Reg_EAX, ModRM_EAX, 0);
			MOV_RegToMemory(1, Reg_EAX, ModRM_disp32, (uint32) & gHWS_fpr32[__RT]);

			POP_RegIfMapped(Reg_ECX);
			POP_RegIfMapped(Reg_EAX);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_ldc1(OP_PARAMS)
{
	/*~~~~~~~~~~*/
	_s32	value;
	/*~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_ldc1);
	SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE_FPU
	goto _Default;
#endif
	INTERPRET_FLUSH1(r4300i_ldc1, __RS);

	if((__RT & 0x1))	/* Ignore this instruction if the RT_FT is odd */
		goto _Default;

	if(ConstMap[xRS->mips_reg].IsMapped == 1)
	{
		__try
		{
			/*~~~~~~~~~~~~~*/
			_u32	QuerAddr;
			/*~~~~~~~~~~~~~*/

			/* TLB range */
			QuerAddr = (_u32) ((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I);

			if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

			value = LOAD_UWORD_PARAM(QuerAddr);
			value = LOAD_UWORD_PARAM((QuerAddr + 4));

			PUSH_RegIfMapped(Reg_EDI);
			MOV_MemoryToReg(1, Reg_EDI, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr));
			MOV_RegToMemory(1, Reg_EDI, ModRM_disp32, (_u32) & gHWS_fpr32[__FT] + 4);
			MOV_MemoryToReg(1, Reg_EDI, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM((QuerAddr + 4)));
			MOV_RegToMemory(1, Reg_EDI, ModRM_disp32, (_u32) & gHWS_fpr32[__FT]);
			POP_RegIfMapped(Reg_EDI);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			goto _Default;
		}
	}
	else
		_Default : INTERPRET_FLUSH1(r4300i_ldc1, __RS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_swc1(OP_PARAMS)
{
	/*~~~~~~~~~~*/
	_s32	value;
	/*~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_swc1);
	SetRdRsRt32bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE_FPU
	goto _Default;
#endif
	if(ConstMap[xRS->mips_reg].IsMapped == 1 && currentromoptions.Code_Check != CODE_CHECK_PROTECT_MEMORY)
	{
		__try
		{
			/*~~~~~~~~~~~~~*/
			_u32	QuerAddr;
			/*~~~~~~~~~~~~~*/

			/* TLB range */
			QuerAddr = (_u32) ((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I);

			if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

			value = LOAD_UWORD_PARAM(QuerAddr);
			PUSH_RegIfMapped(Reg_EAX);
			MOV_MemoryToReg(1, Reg_EAX, ModRM_disp32, (_u32) & gHWS_fpr32[__FT]);
			MOV_RegToMemory(1, Reg_EAX, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM(QuerAddr));
			POP_RegIfMapped(Reg_EAX);
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

			PUSH_RegIfMapped(Reg_EAX);
			PUSH_RegIfMapped(Reg_ECX);

			MOV_ImmToReg(1, Reg_ECX, temp + (_s32) (_s16) __dotI);
			MOV_ImmToReg(1, Reg_EAX, ((uint32) (temp + (_s32) (_s16) __dotI)) >> SHIFTER2_WRITE);

			WC16(0x14FF);
			WC8(0x85);
			WC32((uint32) & memory_write_functions);
			LOGGING_DYNA(LogDyna("	CALL memory_write_functions[]\n"););

			MOV_MemoryToReg(1, Reg_ECX, ModRM_disp32, (uint32) & gHWS_fpr32[__RT]);
			MOV_RegToMemory(1, Reg_ECX, ModRM_EAX, 0);

			POP_RegIfMapped(Reg_ECX);
			POP_RegIfMapped(Reg_EAX);
		}
		else
		{
			MapRS;
			PUSH_RegIfMapped(Reg_EAX);
			PUSH_RegIfMapped(Reg_ECX);

			MOV_Reg2ToReg1(1, Reg_ECX, xRS->x86reg);
			ADD_ImmToReg(1, Reg_ECX, (_s32) (_s16) __dotI);
			MOV_Reg2ToReg1(1, Reg_EAX, Reg_ECX);
			SHR_RegByImm(1, Reg_EAX, SHIFTER2_WRITE);
			WC16(0x14FF);
			WC8(0x85);
			WC32((uint32) & memory_write_functions);
			LOGGING_DYNA(LogDyna("	CALL memory_write_functions[]\n"););
			MOV_MemoryToReg(1, Reg_ECX, ModRM_disp32, (uint32) & gHWS_fpr32[__RT]);
			MOV_RegToMemory(1, Reg_ECX, ModRM_EAX, 0);

			POP_RegIfMapped(Reg_ECX);
			POP_RegIfMapped(Reg_EAX);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_sdc1(OP_PARAMS)
{
	/*~~~~~~~~~~*/
	_s32	value;
	/*~~~~~~~~~~*/

	compilerstatus.cp0Counter += 1;
	_SAFTY_COP1_(r4300i_sdc1);
	SetRdRsRt64bit(PASS_PARAMS);

#ifdef SAFE_LOADSTORE_FPU
	goto _Default;
#endif
	INTERPRET_FLUSH1(r4300i_sdc1, __RS);

	if((__RT & 0x1))	/* Ignore this instruction if the RT_FT is odd */
		goto _Default;

	if(ConstMap[xRS->mips_reg].IsMapped == 1 && currentromoptions.Code_Check != CODE_CHECK_PROTECT_MEMORY)
	{
		__try
		{
			/*~~~~~~~~~~~~~*/
			uint32	QuerAddr;
			/*~~~~~~~~~~~~~*/

			/* TLB range */
			QuerAddr = (_u32) ((_s32) ConstMap[xRS->mips_reg].value + (_s32) (_s16) __I);

			if(NOT_IN_KO_K1_SEG(QuerAddr)) goto _Default;

			value = LOAD_UWORD_PARAM(QuerAddr);
			value = LOAD_UWORD_PARAM((QuerAddr + 4));
			PUSH_RegIfMapped(Reg_EDI);
			MOV_MemoryToReg(1, Reg_EDI, ModRM_disp32, (_u32) & reg->fpr32[__FT]);
			MOV_RegToMemory(1, Reg_EDI, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM((QuerAddr + 4)));
			MOV_MemoryToReg(1, Reg_EDI, ModRM_disp32, (_u32) & reg->fpr32[__FT] + 4);
			MOV_RegToMemory(1, Reg_EDI, ModRM_disp32, (_u32) pLOAD_UWORD_PARAM((QuerAddr)));
			POP_RegIfMapped(Reg_EDI);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			goto _Default;
		}
	}
	else
		_Default : INTERPRET_FLUSH1(r4300i_sdc1, __RS);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void dyna4300i_c_d(OP_PARAMS)
{
	/*
	 * SAFTY_COP1_(r4300i_c_d); £
	 * INTERPRET_NOFLUSH(r4300i_c_d);
	 */
}
