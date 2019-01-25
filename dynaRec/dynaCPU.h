/*$T dynaCPU.h GC 1.136 03/09/02 16:12:25 */


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
#ifndef __1964_DYNACPU_H
#define __1964_DYNACPU_H

void	COMPARE_Run(uint32 Inter_Opcode_Address, uint32 code);
#define _SAFTY_CPU_(x) \
	if(debug_opcode!=0) COMPARE_Run((uint32) & x, reg->code);

extern void dyna4300i_reserved(OP_PARAMS);
extern void dyna4300i_reserved1(OP_PARAMS);
extern void dyna4300i_invalid(OP_PARAMS);
extern void dyna4300i_special(OP_PARAMS);
extern void dyna4300i_regimm(OP_PARAMS);
extern void dyna4300i_j(OP_PARAMS);
extern void dyna4300i_jal(OP_PARAMS);
extern void dyna4300i_beq(OP_PARAMS);
extern void dyna4300i_bne(OP_PARAMS);
extern void dyna4300i_blez(OP_PARAMS);
extern void dyna4300i_bgtz(OP_PARAMS);
extern void dyna4300i_addi(OP_PARAMS);
extern void dyna4300i_addiu(OP_PARAMS);
extern void dyna4300i_slti(OP_PARAMS);
extern void dyna4300i_sltiu(OP_PARAMS);
extern void dyna4300i_andi(OP_PARAMS);
extern void dyna4300i_ori(OP_PARAMS);
extern void dyna4300i_xori(OP_PARAMS);
extern void dyna4300i_lui(OP_PARAMS);
extern void dyna4300i_cop0(OP_PARAMS);
extern void dyna4300i_cop1(OP_PARAMS);
extern void dyna4300i_cop1_with_exception(OP_PARAMS);
extern void dyna4300i_cop2(OP_PARAMS);
extern void dyna4300i_beql(OP_PARAMS);
extern void dyna4300i_bnel(OP_PARAMS);
extern void dyna4300i_blezl(OP_PARAMS);
extern void dyna4300i_bgtzl(OP_PARAMS);
extern void dyna4300i_daddi(OP_PARAMS);
extern void dyna4300i_daddiu(OP_PARAMS);
extern void dyna4300i_ldl(OP_PARAMS);
extern void dyna4300i_ldr(OP_PARAMS);
extern void dyna4300i_lb(OP_PARAMS);
extern void dyna4300i_lh(OP_PARAMS);
extern void dyna4300i_lwl(OP_PARAMS);
extern void dyna4300i_lw(OP_PARAMS);
extern void dyna4300i_lbu(OP_PARAMS);
extern void dyna4300i_lhu(OP_PARAMS);
extern void dyna4300i_lwr(OP_PARAMS);
extern void dyna4300i_lwu(OP_PARAMS);
extern void dyna4300i_sb(OP_PARAMS);
extern void dyna4300i_sh(OP_PARAMS);
extern void dyna4300i_swl(OP_PARAMS);
extern void dyna4300i_sw(OP_PARAMS);
extern void dyna4300i_sdl(OP_PARAMS);
extern void dyna4300i_sdr(OP_PARAMS);
extern void dyna4300i_swr(OP_PARAMS);
extern void dyna4300i_cache(OP_PARAMS);
extern void dyna4300i_ll(OP_PARAMS);
extern void dyna4300i_lwc1(OP_PARAMS);
extern void dyna4300i_lwc2(OP_PARAMS);
extern void dyna4300i_lld(OP_PARAMS);
extern void dyna4300i_ldc1(OP_PARAMS);
extern void dyna4300i_ldc2(OP_PARAMS);
extern void dyna4300i_ld(OP_PARAMS);
extern void dyna4300i_sc(OP_PARAMS);
extern void dyna4300i_swc1(OP_PARAMS);
extern void dyna4300i_swc2(OP_PARAMS);
extern void dyna4300i_scd(OP_PARAMS);
extern void dyna4300i_sdc1(OP_PARAMS);
extern void dyna4300i_sdc2(OP_PARAMS);
extern void dyna4300i_sd(OP_PARAMS);
extern void dyna4300i_special_shift(OP_PARAMS); /* sll, srl, sra */
extern void dyna4300i_shift_var(OP_PARAMS);		/* sllv, srlv, srav */
extern void dyna4300i_special_jr(OP_PARAMS);
extern void dyna4300i_special_jalr(OP_PARAMS);
extern void dyna4300i_special_syscall(OP_PARAMS);
extern void dyna4300i_special_break(OP_PARAMS);
extern void dyna4300i_special_sync(OP_PARAMS);
extern void dyna4300i_mf_mt(OP_PARAMS);
extern void dyna4300i_special_dsllv(OP_PARAMS);
extern void dyna4300i_special_dsrlv(OP_PARAMS);
extern void dyna4300i_special_dsrav(OP_PARAMS);
extern void dyna4300i_special_mul(OP_PARAMS);
extern void dyna4300i_special_div(OP_PARAMS);
extern void dyna4300i_special_divides(OP_PARAMS);
extern void dyna4300i_special_divu(OP_PARAMS);
extern void dyna4300i_special_dmult(OP_PARAMS);
extern void dyna4300i_special_dmultu(OP_PARAMS);
extern void dyna4300i_special_ddiv(OP_PARAMS);
extern void dyna4300i_special_ddivu(OP_PARAMS);
extern void dyna4300i_special_add(OP_PARAMS);
extern void dyna4300i_special_addu(OP_PARAMS);
extern void dyna4300i_special_sub(OP_PARAMS);
extern void dyna4300i_special_subu(OP_PARAMS);
extern void dyna4300i_special_and(OP_PARAMS);
extern void dyna4300i_special_or(OP_PARAMS);
extern void dyna4300i_special_xor(OP_PARAMS);
extern void dyna4300i_special_nor(OP_PARAMS);
extern void dyna4300i_special_slt(OP_PARAMS);
extern void dyna4300i_special_sltu(OP_PARAMS);
extern void dyna4300i_special_dadd(OP_PARAMS);
extern void dyna4300i_special_daddu(OP_PARAMS);
extern void dyna4300i_special_dsub(OP_PARAMS);
extern void dyna4300i_special_dsubu(OP_PARAMS);
extern void dyna4300i_special_tge(OP_PARAMS);
extern void dyna4300i_special_tgeu(OP_PARAMS);
extern void dyna4300i_special_tlt(OP_PARAMS);
extern void dyna4300i_special_tltu(OP_PARAMS);
extern void dyna4300i_special_teq(OP_PARAMS);
extern void dyna4300i_special_tne(OP_PARAMS);
extern void dyna4300i_special_dsll(OP_PARAMS);
extern void dyna4300i_special_dsrl(OP_PARAMS);
extern void dyna4300i_special_dsra(OP_PARAMS);
extern void dyna4300i_special_dsll32(OP_PARAMS);
extern void dyna4300i_special_dsrl32(OP_PARAMS);
extern void dyna4300i_special_dsra32(OP_PARAMS);
extern void dyna4300i_regimm_bltz(OP_PARAMS);
extern void dyna4300i_regimm_bgez(OP_PARAMS);
extern void dyna4300i_regimm_bltzl(OP_PARAMS);
extern void dyna4300i_regimm_bgezl(OP_PARAMS);
extern void dyna4300i_regimm_tgei(OP_PARAMS);
extern void dyna4300i_regimm_tgeiu(OP_PARAMS);
extern void dyna4300i_regimm_tlti(OP_PARAMS);
extern void dyna4300i_regimm_tltiu(OP_PARAMS);
extern void dyna4300i_regimm_teqi(OP_PARAMS);
extern void dyna4300i_regimm_tnei(OP_PARAMS);
extern void dyna4300i_regimm_bltzal(OP_PARAMS);
extern void dyna4300i_regimm_bgezal(OP_PARAMS);
extern void dyna4300i_regimm_bltzall(OP_PARAMS);
extern void dyna4300i_regimm_bgezall(OP_PARAMS);
extern void dyna4300i_cop0_rs_mf(OP_PARAMS);
extern void dyna4300i_cop0_rs_dmf(OP_PARAMS);
extern void dyna4300i_cop0_rs_cf(OP_PARAMS);
extern void dyna4300i_cop0_rs_mt(OP_PARAMS);
extern void dyna4300i_cop0_rs_dmt(OP_PARAMS);
extern void dyna4300i_cop0_rs_ct(OP_PARAMS);
extern void dyna4300i_cop0_rs_bc(OP_PARAMS);
extern void dyna4300i_cop0_tlb(OP_PARAMS);
extern void dyna4300i_cop0_rt_bcf(OP_PARAMS);
extern void dyna4300i_cop0_rt_bct(OP_PARAMS);
extern void dyna4300i_cop0_rt_bcfl(OP_PARAMS);
extern void dyna4300i_cop0_rt_bctl(OP_PARAMS);
extern void dyna4300i_cop0_tlbr(OP_PARAMS);
extern void dyna4300i_cop0_tlbwi(OP_PARAMS);
extern void dyna4300i_cop0_tlbwr(OP_PARAMS);
extern void dyna4300i_cop0_tlbp(OP_PARAMS);
extern void dyna4300i_cop0_eret(OP_PARAMS);
extern void dyna4300i_cop2_rs_not_implemented(OP_PARAMS);

typedef void (*dyn_cpu_instr) (OP_PARAMS);

extern dyn_cpu_instr	asm_instruction[64];
extern dyn_cpu_instr	asm_special_instruction[64];
extern dyn_cpu_instr	asm_regimm_instruction[32];
extern dyn_cpu_instr	asm_cop0_rs_instruction[32];
extern dyn_cpu_instr	asm_cop0_rt_instruction[32];
extern dyn_cpu_instr	asm_cop0_instruction[64];
extern dyn_cpu_instr	asm_cop2_rs_instruction[32];
extern dyn_cpu_instr	dyna_instruction[64];
extern dyn_cpu_instr	now_do_dyna_instruction[64];
extern dyn_cpu_instr	dyna_special_instruction[64];
extern dyn_cpu_instr	dyna_regimm_instruction[32];
extern dyn_cpu_instr	dyna_cop0_rs_instruction[32];
extern dyn_cpu_instr	dyna_cop0_rt_instruction[32];
extern dyn_cpu_instr	dyna_tlb_instruction[64];
extern dyn_cpu_instr	dyna_cop2_rs_instruction[32];

extern void				SetRdRsRt32bit(HardwareState *reg);
extern void				SetRdRsRt64bit(HardwareState *reg);
#endif
