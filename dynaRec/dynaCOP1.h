/*$T dynaCOP1.h GC 1.136 03/09/02 16:04:44 */


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
#ifndef _DYNACOP1_H_
#define _DYNACOP1_H_

extern void rs4300i_cop1_mfc1(_u32 Instruction);
extern void rs4300i_cop1_dmfc1(_u32 Instruction);
extern void rs4300i_cop1_cfc1(_u32 Instruction);
extern void rs4300i_cop1_mtc1(_u32 Instruction);
extern void rs4300i_cop1_dmtc1(_u32 Instruction);
extern void rs4300i_cop1_ctc1(_u32 Instruction);
extern void cop1_BC_instr(_u32 Instruction);
extern void cop1_S_instr(_u32 Instruction);
extern void cop1_D_instr(_u32 Instruction);
extern void cop1_W_instr(_u32 Instruction);
extern void cop1_L_instr(_u32 Instruction);
extern void rs4300i_cop1_bc1f(_u32 Instruction);
extern void rs4300i_cop1_bc1t(_u32 Instruction);
extern void rs4300i_cop1_bc1fl(_u32 Instruction);
extern void rs4300i_cop1_bc1tl(_u32 Instruction);
extern void rs4300i_cop1_add_s(_u32 Instruction);
extern void rs4300i_cop1_sub_s(_u32 Instruction);
extern void rs4300i_cop1_mul_s(_u32 Instruction);
extern void rs4300i_cop1_div_s(_u32 Instruction);
extern void rs4300i_cop1_sqrt_s(_u32 Instruction);
extern void rs4300i_cop1_abs_s(_u32 Instruction);
extern void rs4300i_cop1_mov_s(_u32 Instruction);
extern void rs4300i_cop1_neg_s(_u32 Instruction);
extern void rs4300i_cop1_roundl_s(_u32 Instruction);
extern void rs4300i_cop1_truncl_s(_u32 Instruction);
extern void rs4300i_cop1_ceill_s(_u32 Instruction);
extern void rs4300i_cop1_floorl_s(_u32 Instruction);
extern void rs4300i_cop1_roundw_s(_u32 Instruction);
extern void rs4300i_cop1_truncw_s(_u32 Instruction);
extern void rs4300i_cop1_ceilw_s(_u32 Instruction);
extern void rs4300i_cop1_floorw_s(_u32 Instruction);
extern void rs4300i_cop1_cvtd_s(_u32 Instruction);
extern void rs4300i_cop1_cvtw_s(_u32 Instruction);
extern void rs4300i_cop1_cvtl_s(_u32 Instruction);
extern void rs4300i_c_s(_u32 Instruction);	/* cop1 compare single */
extern void rs4300i_c_f_s(_u32 Instruction);
extern void rs4300i_c_un_s(_u32 Instruction);
extern void rs4300i_c_eq_s(_u32 Instruction);
extern void rs4300i_c_ueq_s(_u32 Instruction);
extern void rs4300i_c_olt_s(_u32 Instruction);
extern void rs4300i_c_ult_s(_u32 Instruction);
extern void rs4300i_c_ole_s(_u32 Instruction);
extern void rs4300i_c_ule_s(_u32 Instruction);
extern void rs4300i_c_sf_s(_u32 Instruction);
extern void rs4300i_c_ngle_s(_u32 Instruction);
extern void rs4300i_c_seq_s(_u32 Instruction);
extern void rs4300i_c_ngl_s(_u32 Instruction);
extern void rs4300i_c_lt_s(_u32 Instruction);
extern void rs4300i_c_nge_s(_u32 Instruction);
extern void rs4300i_c_le_s(_u32 Instruction);
extern void rs4300i_c_ngt_s(_u32 Instruction);
extern void rs4300i_cop1_add_d(_u32 Instruction);
extern void rs4300i_cop1_sub_d(_u32 Instruction);
extern void rs4300i_cop1_mul_d(_u32 Instruction);
extern void rs4300i_cop1_div_d(_u32 Instruction);
extern void rs4300i_cop1_sqrt_d(_u32 Instruction);
extern void rs4300i_cop1_abs_d(_u32 Instruction);
extern void rs4300i_cop1_mov_d(_u32 Instruction);
extern void rs4300i_cop1_neg_d(_u32 Instruction);
extern void rs4300i_cop1_roundl_d(_u32 Instruction);
extern void rs4300i_cop1_truncl_d(_u32 Instruction);
extern void rs4300i_cop1_ceill_d(_u32 Instruction);
extern void rs4300i_cop1_floorl_d(_u32 Instruction);
extern void rs4300i_cop1_roundw_d(_u32 Instruction);
extern void rs4300i_cop1_truncw_d(_u32 Instruction);
extern void rs4300i_cop1_ceilw_d(_u32 Instruction);
extern void rs4300i_cop1_floorw_d(_u32 Instruction);
extern void rs4300i_cop1_cvts_d(_u32 Instruction);
extern void rs4300i_cop1_cvtw_d(_u32 Instruction);
extern void rs4300i_cop1_cvtl_d(_u32 Instruction);
extern void rs4300i_c_d(_u32 Instruction);	/* cop1 compare double */
extern void rs4300i_c_f_d(_u32 Instruction);
extern void rs4300i_c_un_d(_u32 Instruction);
extern void rs4300i_c_eq_d(_u32 Instruction);
extern void rs4300i_c_ueq_d(_u32 Instruction);
extern void rs4300i_c_olt_d(_u32 Instruction);
extern void rs4300i_c_ult_d(_u32 Instruction);
extern void rs4300i_c_ole_d(_u32 Instruction);
extern void rs4300i_c_ule_d(_u32 Instruction);
extern void rs4300i_c_sf_d(_u32 Instruction);
extern void rs4300i_c_ngle_d(_u32 Instruction);
extern void rs4300i_c_seq_d(_u32 Instruction);
extern void rs4300i_c_ngl_d(_u32 Instruction);
extern void rs4300i_c_lt_d(_u32 Instruction);
extern void rs4300i_c_nge_d(_u32 Instruction);
extern void rs4300i_c_le_d(_u32 Instruction);
extern void rs4300i_c_ngt_d(_u32 Instruction);
extern void rs4300i_cop1_cvts_w(_u32 Instruction);
extern void rs4300i_cop1_cvtd_w(_u32 Instruction);
extern void rs4300i_cop1_cvts_l(_u32 Instruction);
extern void rs4300i_cop1_cvtd_l(_u32 Instruction);
extern void rs4300i_lwc1(_u32 Instruction, _u32 pc);
extern void rs4300i_ldc1(_u32 Instruction, _u32 pc);
extern void rs4300i_swc1(_u32 Instruction, _u32 pc);
extern void rs4300i_sdc1(_u32 Instruction, _u32 pc);
#endif
