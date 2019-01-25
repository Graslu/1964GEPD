/*$T R4300i.c GC 1.136 03/09/02 17:29:19 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Interpretive integer opcode functions of the R4300i processor.
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
#include "debug_option.h"
#include "r4300i.h"
#include "n64rcp.h"
#include "memory.h"
#include "1964ini.h"
#include "gamesave.h"
#include "timer.h"
#include "interrupt.h"
#include "win32/DLL_Video.h"
#include "win32/DLL_Audio.h"
#include "win32/windebug.h"
#include "win32/wingui.h"
#include "dma.h"

void	SPECIAL_instr(uint32 Instruction);
void	REGIMM_instr(uint32 Instruction);
void	COP0_instr(uint32 Instruction);
void	COP1_instr(uint32 Instruction);
void	TLB_instr(uint32 Instruction);
void	COP1_BC_instr(uint32 Instruction);
void	COP1_S_instr(uint32 Instruction);
void	COP1_D_instr(uint32 Instruction);
void	COP1_W_instr(uint32 Instruction);
void	COP1_L_instr(uint32 Instruction);

void (*CPU_instruction[64]) (uint32 Instruction) =
{
	SPECIAL_instr,	//0
	REGIMM_instr,	//1
	r4300i_j,		//2
	r4300i_jal,		//3
	r4300i_beq,		//4
	r4300i_bne,		//5
	r4300i_blez,	//6
	r4300i_bgtz,	//7
	r4300i_addi,	//8
	r4300i_addiu,	//9
	r4300i_slti,	//10
	r4300i_sltiu,	//11
	r4300i_andi,	//12
	r4300i_ori,		//13
	r4300i_xori,	//14
	r4300i_lui,		//15
	COP0_instr,		//16
	COP1_instr,		//17
	UNUSED,			//18
	UNUSED,			//19
	r4300i_beql,	//20
	r4300i_bnel,	//21
	r4300i_blezl,	//22
	r4300i_bgtzl,	//23
	r4300i_daddi,	//24
	r4300i_daddiu,	//25
	r4300i_ldl,		//26
	r4300i_ldr,		//27
	UNUSED,			//28
	UNUSED,			//29
	UNUSED,			//30
	UNUSED,			//31
	r4300i_lb,		//32
	r4300i_lh,		//33
	r4300i_lwl,		//34
	r4300i_lw,		//35
	r4300i_lbu,		//36
	r4300i_lhu,		//37
	r4300i_lwr,		//38
	r4300i_lwu,		//39
	r4300i_sb,		//40
	r4300i_sh,		//41
	r4300i_swl,		//42
	r4300i_sw,		//43
	r4300i_sdl,		//44
	r4300i_sdr,		//45
	r4300i_swr,		//46
	r4300i_cache,	//47
	r4300i_ll,		//48
	r4300i_lwc1,	//49
	UNUSED,			//50
	UNUSED,			//51
	r4300i_lld,		//52
	r4300i_ldc1,	//53
	UNUSED,			//54
	r4300i_ld,		//55
	r4300i_sc,		//56
	r4300i_swc1,	//57
	UNUSED,			//58
	UNUSED,			//59
	r4300i_scd,		//60
	r4300i_sdc1,	//61
	UNUSED,			//62
	r4300i_sd		//63
};

void (*SPECIAL_Instruction[64]) (uint32 Instruction) =
{
	r4300i_sll,		//0
	UNUSED,			//1
	r4300i_srl,		//2
	r4300i_sra,		//3
	r4300i_sllv,	//4
	UNUSED,			//5
	r4300i_srlv,	//6
	r4300i_srav,	//7
	r4300i_jr,		//8
	r4300i_jalr,	//9
	UNUSED,			//10
	UNUSED,			//11
	r4300i_syscall,	//12
	r4300i_break,	//13
	UNUSED,			//14
	r4300i_sync,	//15
	r4300i_mfhi,	//16
	r4300i_mthi,	//17
	r4300i_mflo,	//18
	r4300i_mtlo,	//19
	r4300i_dsllv,	//20
	UNUSED,			//21
	r4300i_dsrlv,	//22
	r4300i_dsrav,	//23
	r4300i_mult,	//24
	r4300i_multu,	//25
	r4300i_div,		//26
	r4300i_divu,	//27
	r4300i_dmult,	//28
	r4300i_dmultu,	//29
	r4300i_ddiv,	//30
	r4300i_ddivu,	//31
	r4300i_add,		//32
	r4300i_addu,	//33
	r4300i_sub,		//34
	r4300i_subu,	//35
	r4300i_and,		//36
	r4300i_or,		//37
	r4300i_xor,		//38
	r4300i_nor,		//39
	UNUSED,			//40
	UNUSED,			//41
	r4300i_slt,		//42
	r4300i_sltu,	//43
	r4300i_dadd,	//44
	r4300i_daddu,	//45
	r4300i_dsub,	//46
	r4300i_dsubu,	//47
	r4300i_tge,		//48
	r4300i_tgeu,	//49
	r4300i_tlt,		//50
	r4300i_tltu,	//51
	r4300i_teq,		//52
	UNUSED,			//53
	r4300i_tne,		//54
	UNUSED,			//55
	r4300i_dsll,	//56
	UNUSED,			//57
	r4300i_dsrl,	//58
	r4300i_dsra,	//59
	r4300i_dsll32,	//60
	UNUSED,			//61
	r4300i_dsrl32,	//62
	r4300i_dsra32	//63
};

void (*REGIMM_Instruction[32]) (uint32 Instruction) =
{
	r4300i_bltz,
	r4300i_bgez,
	r4300i_bltzl,
	r4300i_bgezl,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	r4300i_tgei,
	r4300i_tgeiu,
	r4300i_tlti,
	r4300i_tltiu,
	r4300i_teqi,
	UNUSED,
	r4300i_tnei,
	UNUSED,
	r4300i_bltzal,
	r4300i_bgezal,
	r4300i_bltzall,
	r4300i_bgezall,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED
};

void (*COP0_Instruction[32]) (uint32 Instruction) =
{
	r4300i_COP0_mfc0,
	UNUSED,
	UNUSED,
	UNUSED,
	r4300i_COP0_mtc0,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	TLB_instr,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED
};

void (*TLB_Instruction[64]) (uint32 Instruction) =
{
	UNUSED,				//0
	r4300i_COP0_tlbr,	//1
	r4300i_COP0_tlbwi,	//2
	UNUSED,				//3
	UNUSED,				//4
	UNUSED,				//5
	r4300i_COP0_tlbwr,	//6
	UNUSED,				//7
	r4300i_COP0_tlbp,	//8
	UNUSED,				//9
	UNUSED,				//10
	UNUSED,				//11
	UNUSED,				//12
	UNUSED,				//13
	UNUSED,				//14
	UNUSED,				//15
	UNUSED,				//16
	UNUSED,				//17
	UNUSED,				//18
	UNUSED,				//19
	UNUSED,				//20
	UNUSED,				//21
	UNUSED,				//22
	UNUSED,				//23
	r4300i_COP0_eret,	//24
	UNUSED,				//25
	UNUSED,				//26
	UNUSED,				//27
	UNUSED,				//28
	UNUSED,				//29
	UNUSED,				//30
	UNUSED,				//31
	UNUSED,				//32
	UNUSED,				//33
	UNUSED,				//34
	UNUSED,				//35
	UNUSED,				//36
	UNUSED,				//37
	UNUSED,				//38
	UNUSED,				//39
	UNUSED,				//40
	UNUSED,				//41
	UNUSED,				//42
	UNUSED,				//43
	UNUSED,				//44
	UNUSED,				//45
	UNUSED,				//46
	UNUSED,				//47
	UNUSED,				//48
	UNUSED,				//49
	UNUSED,				//50
	UNUSED,				//51
	UNUSED,				//52
	UNUSED,				//53
	UNUSED,				//54
	UNUSED,				//55
	UNUSED,				//56
	UNUSED,				//57
	UNUSED,				//58
	UNUSED,				//59
	UNUSED,				//60
	UNUSED,				//61
	UNUSED,				//62
	UNUSED 				//63
};

void (*COP1_Instruction[32]) (uint32 Instruction) =
{
	r4300i_COP1_mfc1,
	r4300i_COP1_dmfc1,
	r4300i_COP1_cfc1,
	UNUSED,
	r4300i_COP1_mtc1,
	r4300i_COP1_dmtc1,
	r4300i_COP1_ctc1,
	UNUSED,
	COP1_BC_instr,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	COP1_S_instr,
	COP1_D_instr,
	UNUSED,
	UNUSED,
	COP1_W_instr,
	COP1_L_instr,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED
};

void (*COP1_BC_Instruction[4]) (uint32 Instruction) =
{
	r4300i_COP1_bc1f,
	r4300i_COP1_bc1t,
	r4300i_COP1_bc1fl,
	r4300i_COP1_bc1tl
};

void (*COP1_S_Instruction[64]) (uint32 Instruction) =
{
	r4300i_COP1_add_s,
	r4300i_COP1_sub_s,
	r4300i_COP1_mul_s,
	r4300i_COP1_div_s,
	r4300i_COP1_sqrt_s,
	r4300i_COP1_abs_s,
	r4300i_COP1_mov_s,
	r4300i_COP1_neg_s,
	r4300i_COP1_roundl_s,
	r4300i_COP1_truncl_s,
	r4300i_COP1_ceill_s,
	r4300i_COP1_floorl_s,
	r4300i_COP1_roundw_s,
	r4300i_COP1_truncw_s,
	r4300i_COP1_ceilw_s,
	r4300i_COP1_floorw_s,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	r4300i_COP1_cvtd_s,
	UNUSED,
	UNUSED,
	r4300i_COP1_cvtw_s,
	r4300i_COP1_cvtl_s,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	r4300i_C_F_S,
	r4300i_C_UN_S,
	r4300i_C_EQ_S,
	r4300i_C_UEQ_S,
	r4300i_C_OLT_S,
	r4300i_C_ULT_S,
	r4300i_C_OLE_S,
	r4300i_C_ULE_S,
	r4300i_C_SF_S,
	r4300i_C_NGLE_S,
	r4300i_C_SEQ_S,
	r4300i_C_NGL_S,
	r4300i_C_LT_S,
	r4300i_C_NGE_S,
	r4300i_C_LE_S,
	r4300i_C_NGT_S
};

void (*COP1_D_Instruction[64]) (uint32 Instruction) =
{
	r4300i_COP1_add_d,
	r4300i_COP1_sub_d,
	r4300i_COP1_mul_d,
	r4300i_COP1_div_d,
	r4300i_COP1_sqrt_d,
	r4300i_COP1_abs_d,
	r4300i_COP1_mov_d,
	r4300i_COP1_neg_d,
	r4300i_COP1_roundl_d,
	r4300i_COP1_truncl_d,
	r4300i_COP1_ceill_d,
	r4300i_COP1_floorl_d,
	r4300i_COP1_roundw_d,
	r4300i_COP1_truncw_d,
	r4300i_COP1_ceilw_d,
	r4300i_COP1_floorw_d,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	r4300i_COP1_cvts_d,
	UNUSED,
	UNUSED,
	UNUSED,
	r4300i_COP1_cvtw_d,
	r4300i_COP1_cvtl_d,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	r4300i_C_F_D,
	r4300i_C_UN_D,
	r4300i_C_EQ_D,
	r4300i_C_UEQ_D,
	r4300i_C_OLT_D,
	r4300i_C_ULT_D,
	r4300i_C_OLE_D,
	r4300i_C_ULE_D,
	r4300i_C_SF_D,
	r4300i_C_NGLE_D,
	r4300i_C_SEQ_D,
	r4300i_C_NGL_D,
	r4300i_C_LT_D,
	r4300i_C_NGE_D,
	r4300i_C_LE_D,
	r4300i_C_NGT_D
};

void (*COP1_W_Instruction[64]) (uint32 Instruction) =
{
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	r4300i_COP1_cvts_w,
	r4300i_COP1_cvtd_w,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED
};

void (*COP1_L_Instruction[64]) (uint32 Instruction) =
{
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	r4300i_COP1_cvts_l,
	r4300i_COP1_cvtd_l,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED,
	UNUSED
};

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SPECIAL_instr(uint32 Instruction)
{
	SPECIAL_Instruction[_FUNCTION_](Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void REGIMM_instr(uint32 Instruction)
{
	REGIMM_Instruction[RT_FT](Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COP0_instr(uint32 Instruction)
{
	COP0_Instruction[RS_BASE_FMT](Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COP1_instr(uint32 Instruction)
{
	COP1_Instruction[RS_BASE_FMT](Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void TLB_instr(uint32 Instruction)
{
	TLB_Instruction[_FUNCTION_](Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COP1_BC_instr(uint32 Instruction)
{
	COP1_BC_Instruction[(_ND_ << 1) | _TF_](Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COP1_S_instr(uint32 Instruction)
{
	COP1_S_Instruction[_FUNCTION_](Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COP1_D_instr(uint32 Instruction)
{
	COP1_D_Instruction[_FUNCTION_](Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COP1_W_instr(uint32 Instruction)
{
	COP1_W_Instruction[_FUNCTION_](Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COP1_L_instr(uint32 Instruction)
{
	COP1_L_Instruction[_FUNCTION_](Instruction);
}


/*
 * r4300i.c internal macro definition £
 */
#ifdef DEBUG_TLB
#define DISPLAY_ADDRESS_ERROR(addr, opcodestr)	Display_Address_Error(addr, opcodestr);
#else
#define DISPLAY_ADDRESS_ERROR(addr, opcodestr)
#endif

/*
 * r4300i.c internal functions £
 */
extern void		DebugIO(uint32, char *, uint32);
__inline void	Display_Address_Error(uint32 addr, char *opcode);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_break(uint32 Instruction)
{
	/* Trigger the BREAK Exception */
	TRACE2("BREAK, PC=%08X, code=%08X", gHWS_pc, Instruction >> 6);
	
	 SET_EXCEPTION(EXC_BREAK); 
	 HandleExceptions(0x80000180);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sync(uint32 Instruction)
{
	/*
	 * ifdef DEBUG_COMMON £
	 * DisplayError("Intruction SYNC, not implemented yet"); £
	 * #endif
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_syscall(uint32 Instruction)
{
	/* Cause a SYSCALL exception */
	TRACE1("SYSCALL, PC=%08X", gHWS_pc);

	/* DisplayError("SYSCALL, PC=%08X", gHWS_pc); */
	SET_EXCEPTION(EXC_SYSCALL) 
	HandleExceptions(0x80000180);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UNUSED(uint32 Instruction)
{
	/*
	 * Trigger the RI Exception £
	 * SET_EXCEPTION(EXC_II) £
	 * HandleExceptions(0x80000180); £
	 * DisplayError("%08X: Illegal opcode request.", gHWS_pc);
	 */
	TRACE1("Invalid Instruction, PC=%08X", gHWS_pc);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_add(uint32 Instruction)
{
	gRD = (_int64) ((_int32) gRS + (_int32) gRT);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_addu(uint32 Instruction)
{
	gRD = (_int64) ((_int32) gRS + (_int32) gRT);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_addi(uint32 Instruction)
{
	sLOGICAL_WITH_IMM(+);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_addiu(uint32 Instruction)
{
	sLOGICAL_WITH_IMM(+);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dadd(uint32 Instruction)
{
	sDLOGICAL(+);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_daddu(uint32 Instruction)
{
	sDLOGICAL(+);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_daddi(uint32 Instruction)
{
	sDLOGICAL_WITH_IMM(+);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_daddiu(uint32 Instruction)
{
	sDLOGICAL_WITH_IMM(+);
}

/*
 =======================================================================================================================
    32bit operands.
 =======================================================================================================================
 */
void r4300i_dadd_32bit(uint32 Instruction)
{
	gHWS_GPR[RD_FS] = (_int64) (_int32) gHWS_GPR[RS_BASE_FMT] + (_int64) (_int32) gHWS_GPR[RT_FT];
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_daddu_32bit(uint32 Instruction)
{
	gHWS_GPR[RD_FS] = (_int64) (_int32) gHWS_GPR[RS_BASE_FMT] + (_int64) (_int32) gHWS_GPR[RT_FT];
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_daddi_32bit(uint32 Instruction)
{
	gHWS_GPR[RT_FT] = (_int64) (_int32) gHWS_GPR[RS_BASE_FMT] + (_int64) (_int32) OFFSET_IMMEDIATE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_daddiu_32bit(uint32 Instruction)
{
	gHWS_GPR[RT_FT] = (_int64) (_int32) gHWS_GPR[RS_BASE_FMT] + (_int64) (_int32) OFFSET_IMMEDIATE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sub(uint32 Instruction)
{
	sLOGICAL(-);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_subu(uint32 Instruction)
{
	sLOGICAL(-);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dsub(uint32 Instruction)
{
	sDLOGICAL(-);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dsubu(uint32 Instruction)
{
	sDLOGICAL(-);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_and(uint32 Instruction)
{
	uDLOGICAL(&);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_andi(uint32 Instruction)
{
	uDLOGICAL_WITH_IMM(&);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_or(uint32 Instruction)
{
	uDLOGICAL( | );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_ori(uint32 Instruction)
{
	uDLOGICAL_WITH_IMM( | );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_xor(uint32 Instruction)
{
	uDLOGICAL( ^ );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_xori(uint32 Instruction)
{
	uDLOGICAL_WITH_IMM( ^ );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_nor(uint32 Instruction)
{
	gRD = ~((uint64) gRS | (uint64) gRT);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
_int32 r4300i_lb_faster(uint32 QuerAddr)
{
	return MEM_READ_SBYTE(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lb(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	QUER_ADDR;
	gHWS_GPR[rt_ft] = r4300i_lb_faster(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 r4300i_lbu_faster(uint32 QuerAddr)
{
	return MEM_READ_UBYTE(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lbu(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	QUER_ADDR;
	gHWS_GPR[rt_ft] = r4300i_lbu_faster(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
_int32 r4300i_lh_faster(uint32 QuerAddr)
{
	return MEM_READ_SHALFWORD(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lh(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	QUER_ADDR;
	CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x1, "LH", EXC_RADE) gHWS_GPR[rt_ft] = r4300i_lh_faster(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 r4300i_lhu_faster(uint32 QuerAddr)
{
	return MEM_READ_UHALFWORD(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lhu(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	QUER_ADDR;
	CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x1, "LHU", EXC_RADE) gHWS_GPR[rt_ft] = r4300i_lhu_faster(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lwu(uint32 Instruction)
{
	/*~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	/*~~~~~~~~~~~~~*/

	QUER_ADDR;
	CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x3, "lwu", EXC_RADE) * (uint64 *) &gRT = (uint64) (MEM_READ_UWORD(QuerAddr));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_ll(uint32 Instruction)
{
	/*~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	/*~~~~~~~~~~~~~*/

	QUER_ADDR;
	CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x3, "LL", EXC_RADE) 
	gRT = MEM_READ_SWORD(QuerAddr);
	gHWS_LLbit = 1; /* Unconditionally ?? */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_ld_faster(uint32 QuerAddr, uint32 rt_ft)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	*pmem = PMEM_READ_UWORD(QuerAddr);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	gHWS_GPR[rt_ft] = pmem[1];
	*((uint32 *) &gHWS_GPR[rt_ft] + 1) = pmem[0];
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_ld(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	QUER_ADDR;
	r4300i_ld_faster(QuerAddr, rt_ft);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lld(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	rt_ft = RT_FT;
	uint32	*pmem;
	uint32	QuerAddr;
	/*~~~~~~~~~~~~~~~~~~*/

	QUER_ADDR;
	pmem = PMEM_READ_UWORD(QuerAddr);
	gHWS_GPR[rt_ft] = pmem[1];
	*((uint32 *) &gHWS_GPR[rt_ft] + 1) = pmem[0];
	gHWS_LLbit = 1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sb_faster(uint32 QuerAddr)
{
	*(PMEM_WRITE_UBYTE(QuerAddr)) = (uint8) gHWS_GPR[write_mem_rt];
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sb(uint32 Instruction)
{
	/*~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	/*~~~~~~~~~~~~~*/

	write_mem_rt = RT_FT;

	QUER_ADDR;
	r4300i_sb_faster(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sh_faster(uint32 QuerAddr)
{
	*(PMEM_WRITE_UHALFWORD(QuerAddr)) = (uint16) gHWS_GPR[write_mem_rt];
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sh(uint32 Instruction)
{
	/*~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	/*~~~~~~~~~~~~~*/

	write_mem_rt = RT_FT;

	QUER_ADDR;
	r4300i_sh_faster(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sd_faster(uint32 QuerAddr, uint32 rt_ft)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/* We can have problems here when doing protecting memory */
	uint32	*pmem = PMEM_WRITE_UWORD(QuerAddr);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	pmem[1] = (uint32) gHWS_GPR[rt_ft];
	pmem[0] = *((uint32 *) &gHWS_GPR[rt_ft] + 1);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sd(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	QUER_ADDR;
	r4300i_sd_faster(QuerAddr, rt_ft);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sc(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	if(gHWS_LLbit)
	{
		/*~~~~~~~~~~~~~*/
		uint32	QuerAddr;
		/*~~~~~~~~~~~~~*/

		QUER_ADDR;
		CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x3, "SC", EXC_WADE) * (PMEM_WRITE_UWORD(QuerAddr)) =
			(uint32) gHWS_GPR[rt_ft];
	}
	(uint64) gHWS_GPR[rt_ft] = (uint64) gHWS_LLbit;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_scd(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	if(gHWS_LLbit)
	{
		/*~~~~~~~~~~~~~*/
		uint32	QuerAddr;
		uint32	*pmem;
		/*~~~~~~~~~~~~~*/

		QUER_ADDR;
		CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x7, "SCD", EXC_WADE) pmem = PMEM_WRITE_UWORD(QuerAddr);
		pmem[1] = (uint32) gHWS_GPR[rt_ft];
		pmem[0] = *((uint32 *) &gHWS_GPR[rt_ft] + 1);
	}
	(uint64) gHWS_GPR[rt_ft] = (uint64) gHWS_LLbit;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_mfhi(uint32 Instruction)
{
	gRD = gHWS_GPR[__HI];
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_mthi(uint32 Instruction)
{
	gHWS_GPR[__HI] = gRS;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_mflo(uint32 Instruction)
{
	gRD = gHWS_GPR[__LO];
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_mtlo(uint32 Instruction)
{
	gHWS_GPR[__LO] = gRS;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sll(uint32 Instruction)
{
	if(RD_FS == 0 && RT_FT == 0 && gRT == 0) return;	/* NOP */

	/* uLOGICAL_SHIFT(<<, SA_FD); */
	gRD = (__int64) (__int32) ((uint32) gRT << SA_FD);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sllv(uint32 Instruction)
{
	uLOGICAL_SHIFT( << , (gRS & 0x1F));

	/* gRD = (__int64)(__int32)((uint32)gRT << (uint32)(gRS&0x1F)); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dsll(uint32 Instruction)
{
	(uint64) gRD = (uint64) gRT << SA_FD;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dsllv(uint32 Instruction)
{
	(uint64) gRD = (uint64) gRT << (((uint32) gRS) & 0x3F);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dsll32(uint32 Instruction)
{
	(uint64) gRD = (uint64) gRT << (32 + SA_FD);

	/*
	 * (uint32*)((uint8*)&gRD+4) = (uint32)gRT << SA_FD; £
	 * (uint32*)((uint8*)&gRD ) = 0;
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_srl(uint32 Instruction)
{
	/*
	 * TRACE0("SRL"); £
	 * TRACE2("original gRT = %08X%08X", (uint32)(gRT>>32), (uint32)gRT);
	 */
	gRD = (__int64) (__int32) ((uint32) gRT >> SA_FD);

	/*
	 * uLOGICAL_SHIFT(>>, SA_FD); £
	 * TRACE3("SA=%d, Result gRD = %08X%08X", SA_FD, (uint32)(gRD>>32), (uint32)gRD);
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_srlv(uint32 Instruction)
{
	/*
	 * TRACE0("SRLV"); £
	 * TRACE2("original gRT = %08X%08X", (uint32)(gRT>>32), (uint32)gRT);
	 */
	gRD = (__int64) (__int32) ((uint32) gRT >> (((uint32) gRS) & 0x1F));

	/*
	 * uLOGICAL_SHIFT(>>, (((uint32)gRS)&0x1F)); £
	 * TRACE3("gRS=%d, Result gRD = %08X%08X", (((uint32)gRS)&0x1F),
	 * (uint32)(gRD>>32), (uint32)gRD);
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dsrl(uint32 Instruction)
{
	/*
	 * TRACE0("DSRL"); £
	 * TRACE2("original gRT = %08X%08X", (uint32)(gRT>>32), (uint32)gRT);
	 */
	(uint64) gRD = (uint64) gRT >> SA_FD;

	/* TRACE3("SA=%d, Result gRD = %08X%08X", SA_FD, (uint32)(gRD>>32), (uint32)gRD); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dsrlv(uint32 Instruction)
{
	(uint64) gRD = (uint64) gRT >> (((uint32) gRS) & 0x3F);

	/*
	 * TRACE3("SA=%d, Result gRD = %08X%08X", SA_FD, (uint32)(gRD>>32), (uint32)gRD); £
	 * TRACE0("DSRLV");
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dsrl32(uint32 Instruction)
{
	(uint64) gRD = (uint64) gRT >> (SA_FD + 32);

	/*
	 * (uint32)gRD = *(uint32*)((uint8*)&gRT+4); £
	 * (uint32*)((uint8*)&gRD) = *(uint32*)((uint8*)&gRD) >> SA_FD; £
	 * (uint32*)((uint8*)&gRD+4) = 0; £
	 * TRACE0("DSRL32");
	 */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sra(uint32 Instruction)
{
	gRD = (__int64) ((__int32) gRT >> SA_FD);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_srav(uint32 Instruction)
{
	gRD = (__int64) ((__int32) gRT >> (((uint32) gRS) & 0x1F));
}

/*
 =======================================================================================================================
    Questionable: dsra & dsrav are probably not getting sign-extended £
 =======================================================================================================================
 */
void r4300i_dsrav(uint32 Instruction)
{
	gRD = gRT >> (((uint32) gRS) & 0x3F);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dsra(uint32 Instruction)
{
	gRD = gRT >> SA_FD;
	TRACE0("r4300i_dsra");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dsra32(uint32 Instruction)
{
	gRD = gRT >> (SA_FD + 32);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_slt(uint32 Instruction)
{
	if(gRS < gRT)
		gRD = 1;
	else
		gRD = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sltu(uint32 Instruction)
{
	if((uint64) gRS < (uint64) gRT)
		gRD = 1;
	else
		gRD = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_slti(uint32 Instruction)
{
	if(gRS < (_int64) (_int32) (_int16) (uint16) OFFSET_IMMEDIATE)
		gRT = 1;
	else
		gRT = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sltiu(uint32 Instruction)
{
	if((uint64) gRS < (uint64) (_int64) (_int32) (_int16) (uint16) OFFSET_IMMEDIATE)
		gRT = 1;
	else
		gRT = 0;
}

BOOL	FPU_Is_Enabled = FALSE;
void	Init_Count_Down_Counters(void);

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void r4300i_COP0_mfc0(uint32 Instruction)
{
	switch(RD_FS)
	{
	case INDEX:		/* The INDEX Register */
		TLB_EXTRA_TRACE(TRACE1("Read INDEX: val=%08X", c0FS));
		break;
	case RANDOM:
		gHWS_COP0Reg[RANDOM] = Get_COUNT_Register() % (0x20 - (gHWS_COP0Reg[WIRED] & 0x1f)) + gHWS_COP0Reg[WIRED];
		TRACE2("RANDOM register is read = %08X, PC=%08X", c0FS, gHWS_pc);
		break;
	case ENTRYLO0:
		TLB_EXTRA_TRACE(TRACE1("Read ENTRYLO0: val=%08X", c0FS));
		break;
	case ENTRYLO1:
		TLB_EXTRA_TRACE(TRACE1("Read ENTRYLO1: val=%08X", c0FS));
		break;
	case PAGEMASK:
		TLB_EXTRA_TRACE(TRACE1("Read PAGEMASK: val=%08X", c0FS));
		break;
	case COUNT:
		gHWS_COP0Reg[COUNT] = Get_COUNT_Register();
		DEBUG_CPU_COUNTER_TRACE(TRACE3("COUNT register is read = %8X, COMPARE=%8X , PC=0x%08X", c0FS, gHWS_COP0Reg[COMPARE], gHWS_pc););
		break;
	case ENTRYHI:	/* The EntryHi register */
		TLB_EXTRA_TRACE(TRACE1("Read ENTRYHI: val=%08X", c0FS));
		break;
	case EPC:
		/* TLB_EXTRA_TRACE(TRACE1("Read EPC: val=%08X", c0FS)); */
		break;
	}

	gRT = (_int64) (_int32) c0FS;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP0_mtc0(uint32 Instruction)
{
	switch(RD_FS)
	{
	case INDEX:		/* The INDEX Register */
		/*
		 * gHWS_COP0Reg[INDEX] = (uint32)gRT & 0x3F; £
		 * gHWS_COP0Reg[INDEX] = gHWS_COP0Reg[INDEX] & 0x80000000 | ((uint32)gRT & 0x3F);
		 */
		gHWS_COP0Reg[INDEX] = (uint32) gRT & 0x8000003F;
		TLB_EXTRA_TRACE(TRACE2("Write INDEX: val=%08X%08X", (uint32) (gRT >> 32), (uint32) gRT));
		return;
	case RANDOM:	/* The RANDOM register, read only */
		return;
	case CAUSE:		/* The CAUSE register */
		if(((uint32) gRT & 0x300) != (gHWS_COP0Reg[CAUSE] & 0x300)) /* check software interrupt */
		{
			gHWS_COP0Reg[CAUSE] = ((uint32) gRT & 0x300) |
			(gHWS_COP0Reg[CAUSE] &~(0x300));
			if(((uint32) gRT & 0x300))	/* to generate a software interrupt */
			{
				gHWS_COP0Reg[CAUSE] = ((uint32) gRT & 0x300) |
				(gHWS_COP0Reg[CAUSE] &~(0x300));

				/* CPU will check interrupts at the next cycle */
				if((gHWS_COP0Reg[CAUSE] & gHWS_COP0Reg[STATUS] & 0x0000FF00))
				{
					TRACE1("Software Interrupt at PC = %8X", gHWS_pc);
					SET_EXCEPTION(EXC_INT);
					HandleInterrupts(0x80000180);
				}
				else
				{
					TRACE1("To trigger software Interrupt at PC = %8X, but interrupt is disabled", gHWS_pc);
				}
			}
			else
			{
				TRACE1("Software Interrupt is disabled at PC = %8X", gHWS_pc);
			}
		}
		else
		{
			gHWS_COP0Reg[CAUSE] = ((uint32) gRT & 0x300) |
			(gHWS_COP0Reg[CAUSE] &~(0x300));
		}

		return;
	case ENTRYLO0:
		TLB_EXTRA_TRACE(TRACE2("Write ENTRYLO0: val=%08X%08X", (uint32) (gRT >> 32), (uint32) gRT));
		c0FS = (uint32) gRT & 0x3FFFFFFF;
		break;
	case ENTRYLO1:
		TLB_EXTRA_TRACE(TRACE2("Write ENTRYLO1: val=%08X%08X", (uint32) (gRT >> 32), (uint32) gRT));
		c0FS = (uint32) gRT & 0x3FFFFFFF;
		return;
	case PAGEMASK:						/* The PAGEMASK register */
		TLB_EXTRA_TRACE(TRACE2("Write PAGEMASK: val=%08X%08X", (uint32) (gRT >> 32), (uint32) gRT));
		gHWS_COP0Reg[PAGEMASK] = (uint32) gRT & 0x01ffe000;
		return;
	case WIRED:		/* The WIRED register */
		/* gHWS_COP0Reg[WIRED] = (uint32)gRT & 0x3f; */
		gHWS_COP0Reg[WIRED] = (uint32) gRT & 0x1f;
		TRACE1("Write to WIRED = %08X", gHWS_COP0Reg[WIRED]);
		gHWS_COP0Reg[RANDOM] = 0x1f;
		return;
	case BADVADDR:	/* BADVADDR register, Read only registers */
		return;
	case COUNT:
		DEBUG_COMPARE_INTERRUPT_TRACE(TRACE2("COUNT register is written = %8X, COMPARE=%8X ", (uint32) gRT, gHWS_COP0Reg[COMPARE]););
		gHWS_COP0Reg[COUNT] = (uint32) gRT; /* COUNT register is writable */

		/* This is risky */
		TRACE1("Set COUNT register = %08X", gHWS_COP0Reg[COUNT]);
		Init_Count_Down_Counters();
		break;
	case ENTRYHI:	/* The EntryHi register */
		TLB_EXTRA_TRACE(TRACE2("Write ENTRYHI: val=%08X%08X", (uint32) (gRT >> 32), (uint32) gRT));
		gHWS_COP0Reg[ENTRYHI] = (uint32) gRT & 0xFFFFE0FF;
		return;
	case COMPARE:	/* The Compare register */
		gHWS_COP0Reg[CAUSE] &= 0xffff7fff;	/* Clear current COMPARE interrupt */
		DEBUG_COMPARE_INTERRUPT_TRACE(TRACE2("COMPARE register is rewritten = %8X, COUNT=%8X ", (uint32) gRT, gHWS_COP0Reg[COUNT]););
		gHWS_COP0Reg[COMPARE] = (uint32) gRT;
		Set_COMPARE_Timer_Event();
		return;
	case EPC:		/* The EntryHi register */
		/*
		 * TLB_EXTRA_TRACE(TRACE2("Write EPC: val=%08X%08X", (uint32)(gRT>>32),
		 * (uint32)gRT)); £
		 * DisplayError("Write EPC: val=%08X%08X", (uint32)(gRT>>32), (uint32)gRT);
		 */
		gHWS_COP0Reg[EPC] = (uint32) gRT;
		return;
	case STATUS:	/* The status register */
		/*
		 * TRACE2( "Write STATUS register: PC = %08X, new value=%08X", gHWS_pc,
		 * (uint32)gRT); £
		 * Has FR bit changed ?
		 */
		if((gHWS_COP0Reg[STATUS] & 0x04000000) != (gRT & 0x04000000))
		{
			FR_reg_offset = (gRT & 0x04000000) ? 32 : 1;
		}

		/* Test the exception bit */
		if((gRT & EXL) == 0 && ((gHWS_COP0Reg[STATUS] & EXL) == 1))
		{
			TRACE3("EXL Bit is cleared at PC = %8X, COMPARE=%8X , PC=0x%08X", gRT, gHWS_COP0Reg[COMPARE], gHWS_pc);

			/* CPU will check interrupts at the next cycle */
			if((gHWS_COP0Reg[CAUSE] & gHWS_COP0Reg[STATUS] & 0x0000FF00))
			{
				CPUNeedToCheckInterrupt = TRUE;
				CPUNeedToDoOtherTask = TRUE;
				Set_Check_Interrupt_Timer_Event();
			}
		}

		/* Test the IE bit */
		if(((gRT & IE) == 1) && ((gHWS_COP0Reg[STATUS] & IE) == 0)) /* If enable interrupt */
		{
			/* CPU will check interrupts at the next cycle */
			///TRACE3("Interrupt is enabled at PC = %8X, COMPARE=%8X , PC=0x%08X", gRT, gHWS_COP0Reg[COMPARE], gHWS_pc);
			if((gHWS_COP0Reg[CAUSE] & gHWS_COP0Reg[STATUS] & 0x0000FF00))
			{
				CPUNeedToCheckInterrupt = TRUE;
				CPUNeedToDoOtherTask = TRUE;
				Set_Check_Interrupt_Timer_Event();
			}
		}

		/* Check FPU usage bit */
		if(currentromoptions.FPU_Hack == USEFPUHACK_YES && (gRT & SR_CU1) != (gHWS_COP0Reg[STATUS] & SR_CU1))
		{
			if(gRT & SR_CU1)
			{
				DisableFPUUnusableException();
			}
			else
			{
				EnableFPUUnusableException();
			}
		}

		if((gHWS_COP0Reg[STATUS] & 0xE0) != ((uint32) gRT & 0xE0))
		{
			TRACE2("Changing CPU to %s, STATUS=%08X", ((uint32) gRT & 0xE0) ? "64bit" : "32bit", ((uint32) gRT & 0xE0));
		}

		gHWS_COP0Reg[STATUS] = (uint32) gRT;
		return;
	case PREVID:	/* PRID register, Read only registers */
		return;		/* This makes BomberMan Hero not to freeze after [START] */
	}

	c0FS = (uint32) gRT;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lui(uint32 Instruction)
{
	gRT = (__int64) (__int32) (OFFSET_IMMEDIATE << (uint32) 16);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_do_speedhack(void)
{
	/* Before SPEEDHACK, let CPU to finish all other tasks, let DMA, SP Task and so on */
	if(Is_CPU_Doing_Other_Tasks()) return;

	//Count_Down_All
	countdown_counter = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bgez(uint32 Instruction)
{
	if(gRS >= 0)
	{
		R4300I_SPEEDHACK DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bgezal(uint32 Instruction)
{
	CHECK_RS_EQUAL_RA(RS_BASE_FMT, "BGEZAL") INTERPRETIVE_LINK(RA) if(gRS >= 0)
	{
		R4300I_SPEEDHACK DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bgezall(uint32 Instruction)
{
	CHECK_RS_EQUAL_RA(RS_BASE_FMT, "bgezall") INTERPRETIVE_LINK(RA) if(gRS >= 0)
	{
		R4300I_SPEEDHACK DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
		DELAY_SKIP
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bgezl(uint32 Instruction)
{
	if(gRS >= 0)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
		DELAY_SKIP
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bltz(uint32 Instruction)
{
	if(gRS < 0)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bltzal(uint32 Instruction)
{
	CHECK_RS_EQUAL_RA(RS_BASE_FMT, "bltzal")
	INTERPRETIVE_LINK(RA)
	if(gRS < 0)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bltzall(uint32 Instruction)
{
	CHECK_RS_EQUAL_RA(RS_BASE_FMT, "bltzall")
	INTERPRETIVE_LINK(RA)
	if(gRS < 0)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
		DELAY_SKIP
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bltzl(uint32 Instruction)
{
	if(gRS < 0)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
		DELAY_SKIP
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bgtz(uint32 Instruction)
{
	if(gRS > 0)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bgtzl(uint32 Instruction)
{
	if(gRS > 0)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
		DELAY_SKIP
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_blez(uint32 Instruction)
{
	if(gRS <= 0)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_blezl(uint32 Instruction)
{
	if(gRS <= 0)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
		DELAY_SKIP
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bne(uint32 Instruction)
{
	if((uint64) gRS != (uint64) gRT)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else if(debug_opcode!=0)
		CPUdelay = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_bnel(uint32 Instruction)
{
	if((uint64) gRS != (uint64) gRT)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
		DELAY_SKIP
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_beql(uint32 Instruction)
{
	if((uint64) gRS == (uint64) gRT)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
		DELAY_SKIP
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_beq(uint32 Instruction)
{
	if((uint64) gRS == (uint64) gRT)
	{
		R4300I_SPEEDHACK
		DELAY_SET
	}
	else
	{
		if(debug_opcode!=0) CPUdelay = 0;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_div(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	__int32 RSReg = (__int32) gRS;
	__int32 RTReg = (__int32) gRT;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(RTReg != 0)
	{
		gHWS_GPR[__LO] = (__int64) (__int32) (RSReg / RTReg);
		gHWS_GPR[__HI] = (__int64) (__int32) (RSReg % RTReg);
	}

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(PCLOCKDIV);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_divu(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	RSReg = (uint32) gRS;
	uint32	RTReg = (uint32) gRT;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(RTReg != 0)
	{
		gHWS_GPR[__LO] = (__int64) (__int32) (RSReg / RTReg);
		gHWS_GPR[__HI] = (__int64) (__int32) (RSReg % RTReg);
	}

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(PCLOCKDIVU);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_ddiv(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	__int64 RSReg = (__int64) gRS;
	__int64 RTReg = (__int64) gRT;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(RTReg != 0)	/* Divide by zero */
	{
		gHWS_GPR[__LO] = RSReg /
		RTReg;
		gHWS_GPR[__HI] = RSReg %
		RTReg;
	}

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(PCLOCKDDIV);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_ddivu(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	RSReg = (uint64) gRS;
	uint64	RTReg = (uint64) gRT;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(RTReg != 0)
	{
		gHWS_GPR[__LO] = RSReg /
		RTReg;
		gHWS_GPR[__HI] = RSReg %
		RTReg;
	}

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(PCLOCKDDIVU);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_mult(uint32 Instruction)
{
	/*~~~~~~~~~~~*/
	_int64	result;
	/*~~~~~~~~~~~*/

	result = (_int64) (_int32) gRS *
	(_int64) (_int32) gRT;
	gHWS_GPR[__LO] = (__int64) (__int32) result;
	gHWS_GPR[__HI] = (__int64) (__int32) (((uint64) result) >> 32);

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(PCLOCKMULT);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_multu(uint32 Instruction)
{
	/*~~~~~~~~~~~*/
	uint64	result;
	/*~~~~~~~~~~~*/

	result = (uint64) (uint32) gRS *
	(uint64) (uint32) gRT;
	gHWS_GPR[__LO] = (__int64) (__int32) result;
	gHWS_GPR[__HI] = (__int64) (__int32) (((uint64) result) >> 32);

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(PCLOCKMULTU);
}

#define NEWER_DMULT
#ifdef NEWER_DMULT
#define MULT_X86(src1, src2) \
	__asm mov eax, src1 \
	__asm mov edx, src2 \
	__asm mul edx

#define ADD_CARRY_X86(dest1, dest2) \
	__asm add dword ptr[dest1 + 4], eax \
	__asm adc dword ptr[dest2 + 4], edx

#define STORE_X86(dest) \
	__asm mov dword ptr[dest], eax \
	__asm mov dword ptr[dest + 4], edx

void r4300i_dmultu (uint32 Instruction)
{
	__int64 glo, ghi;
	_int32	rs = RS_BASE_FMT;
	_int32	rt = RT_FT;
	uint32	a;
	uint32	b = (uint32) gHWS_GPR[rs];
	uint32	c;
	uint32	d = (uint32) gHWS_GPR[rt];

	a = *((uint32 *) &gHWS_GPR[rs] + 1);
	c = *((uint32 *) &gHWS_GPR[rt] + 1);

	__asm
	{
		/* mov eax, b mov edx, d mul edx mov dword ptr [glo],eax mov dword ptr [glo+4],edx */
	}
	MULT_X86(b, d)
	STORE_X86(glo)
	MULT_X86(a, c)
	STORE_X86(ghi)
	MULT_X86(b, c)
	ADD_CARRY_X86(glo, ghi)
	MULT_X86(a, d)
	ADD_CARRY_X86(glo, ghi)
	gHWS_GPR[__LO] = glo;
	gHWS_GPR[__HI] = ghi;

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(PCLOCKDMULTU);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dmult(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~*/
	int		sgn = 0;
	uint32	a, b, c, d;
	__int64 v1, v2;
	__int64 glo, ghi;
	/*~~~~~~~~~~~~~~~*/

	v1 = gRS;
	v2 = gRT;

	if(v1 < 0)
	{
		v1 = -v1;
		sgn = 1;
	}

	if(v2 < 0)
	{
		v2 = -v2;
		sgn = !sgn;
	}

	a = (uint32) (v1 >> 32);
	b = (uint32) v1;
	c = (uint32) (v2 >> 32);
	d = (uint32) v2;

	__asm
	{
	}

	MULT_X86(b, d);
	STORE_X86(glo);
	MULT_X86(a, c);
	STORE_X86(ghi);
	MULT_X86(b, c);
	ADD_CARRY_X86(glo, ghi);
	MULT_X86(a, d);
	ADD_CARRY_X86(glo, ghi);
	if(sgn) /* neg */
	{
		glo = ~glo;
		ghi = ~ghi;
		glo += 1;
		if(glo == 0) ghi += 1;
	}

	gHWS_GPR[__LO] = glo;
	gHWS_GPR[__HI] = ghi;

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(PCLOCKDMULT);
}

#else

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dmultu(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	hh, hl, lh, ll, b;
	uint64	t1, t2;
	/*~~~~~~~~~~~~~~~~~~~~~~*/

	t1 = gRS;
	t2 = gRT;
	hh = ((__int64) (t1 >> 32) & 0x0ffffffff) *
	((__int64) (t2 >> 32) & 0x0ffffffff);
	hl = (__int64) (t1 & 0x0ffffffff) *
	((__int64) (t2 >> 32) & 0x0ffffffff);
	lh = ((__int64) (t1 >> 32) & 0x0ffffffff) *
	(__int64) (t2 & 0x0ffffffff);
	ll = ((__int64) (t1 & 0x0ffffffff) * (__int64) (t2 & 0x0ffffffff));

	gHWS_GPR[__LO] = ((hl + lh) << 32) +
	ll;

	b = (((hl + lh) + (ll >> 32)) & 0x0100000000) >>
	32;

	gHWS_GPR[__HI] = (unsigned __int64) hh +
	((signed __int64) (unsigned __int32) (hl >> 32) + (signed __int64) (unsigned __int32) (lh >> 32) + b);

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(PCLOCKDMULTU);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_dmult(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	unsigned __int64	hh, hl, lh, ll, b;
	__int64				t1, t2;
	int					sgn = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	t1 = gRS;
	t2 = gRT;
	if(t1 < 0)
	{
		sgn ^= 1;
		t1 = -t1;
	}

	if(t2 < 0)
	{
		sgn ^= 1;
		t2 = -t2;
	}

	hh = ((__int64) (t1 >> 32) & 0x0ffffffff) *
	((__int64) (t2 >> 32) & 0x0ffffffff);
	hl = (__int64) (t1 & 0x0ffffffff) *
	((__int64) (t2 >> 32) & 0x0ffffffff);
	lh = ((__int64) (t1 >> 32) & 0x0ffffffff) *
	(__int64) (t2 & 0x0ffffffff);
	ll = ((__int64) (t1 & 0x0ffffffff) * (__int64) (t2 & 0x0ffffffff));

	gHWS_GPR[__LO] = ((hl + lh) << 32) +
	ll;

	b = (((hl + lh) + (ll >> 32)) & 0x0100000000) >>
	32;

	gHWS_GPR[__HI] = (unsigned __int64) hh +
	((signed __int64) (unsigned __int32) (hl >> 32) + (signed __int64) (unsigned __int32) (lh >> 32) + b);

	b = (gHWS_GPR[__LO] >= 0) ? 1 : 0;

	if(sgn != 0)
	{
		gHWS_GPR[__LO] = -gHWS_GPR[__LO];
		gHWS_GPR[__HI] = -gHWS_GPR[__HI] +
		b;
	}

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(PCLOCKDMULT);
}
#endif

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_jal(uint32 Instruction)
{
	INTERPRETIVE_LINK(RA) CPUdelay = 1;
	CPUdelayPC = INSTR_INDEX;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_jalr(uint32 Instruction)
{
#ifdef DEBUG_COMMON
	if(RD_FS == RS_BASE_FMT) DisplayError("In JALR, RD==RS");
#endif
	INTERPRETIVE_LINK(RD_FS) CPUdelay = 1;
#ifdef DEBUG_COMMON
	if(gRS & 0x3)
	{
		DisplayError("Warning, JALR, the target address is not aligned");
	}
#endif
	CPUdelayPC = (uint32) gRS;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_jr(uint32 Instruction)
{
	CPUdelay = 1;

#ifdef DEBUG_COMMON
	if(gRS & 0x3)
	{
		DisplayError("Warning, JR, the target address is not aligned");
	}
#endif
	CPUdelayPC = (uint32) gRS;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_j(uint32 Instruction)
{
	CPUdelay = 1;
	CPUdelayPC = INSTR_INDEX;
	if(gHWS_pc == CPUdelayPC)
	{
		if(NOT_IN_KO_K1_SEG(gHWS_pc))
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			uint32	temppc = TranslateITLBAddress(gHWS_pc);
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			if(MEM_READ_UWORD(temppc + 4) == 0)
			{
				r4300i_do_speedhack();
			}
		}
		else if(MEM_READ_UWORD(gHWS_pc + 4) == 0)
		{
			r4300i_do_speedhack();
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lw(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	QUER_ADDR;
	CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x3, "LW", EXC_RADE) gHWS_GPR[rt_ft] = MEM_READ_SWORD(QuerAddr);

#ifdef CPU_CORE_CHECK_R0
	if(rt_ft == 0 && gHWS_GPR[rt_ft] != 0)
	{
		TRACE0("R0 != 0 after LW");
	}
#endif
}

/*
 =======================================================================================================================
    uint32* write_mem_rdram_k0seg(uint32 addr, uint32 rt);
 =======================================================================================================================
 */
void r4300i_sw_faster(uint32 QuerAddr)
{
	*(PMEM_WRITE_UWORD(QuerAddr)) = (uint32) gHWS_GPR[write_mem_rt];
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sw(uint32 Instruction)
{
	/*~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	/*~~~~~~~~~~~~~*/

	write_mem_rt = RT_FT;
	QUER_ADDR;
	r4300i_sw_faster(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lwl(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32			LoadWord1 = 0;
	uint32			rt_ft = RT_FT;
	uint32			vAddr;
	LOAD_TLB_FUN	vAddr = QuerAddr & 0xfffffffc;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	LoadWord1 = MEM_READ_UWORD(vAddr);

	switch(QuerAddr & 3)
	{
	case 0:
		gHWS_GPR[rt_ft] = (_int64) (_int32) LoadWord1;
		break;
	case 1:
		gHWS_GPR[rt_ft] = (_int64) (_int32) ((((uint32) gHWS_GPR[rt_ft]) & 0x000000ff) | (LoadWord1 << 8));
		break;
	case 2:
		gHWS_GPR[rt_ft] = (_int64) (_int32) ((((uint32) gHWS_GPR[rt_ft]) & 0x0000ffff) | (LoadWord1 << 16));
		break;
	case 3:
		gHWS_GPR[rt_ft] = (_int64) (_int32) ((((uint32) gHWS_GPR[rt_ft]) & 0x00ffffff) | (LoadWord1 << 24));
		break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lwr(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32			LoadWord1 = 0;
	uint32			rt_ft = RT_FT;
	uint32			vAddr;
	LOAD_TLB_FUN	vAddr = QuerAddr & 0xfffffffc;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	LoadWord1 = MEM_READ_UWORD(vAddr);

	switch(QuerAddr & 3)
	{
	case 3:
		gHWS_GPR[rt_ft] = (_int64) (_int32) LoadWord1;
		break;
	case 2:
		gHWS_GPR[rt_ft] = (_int64) (_int32) ((((uint32) gHWS_GPR[rt_ft]) & 0xff000000) | (LoadWord1 >> 8));
		break;
	case 1:
		gHWS_GPR[rt_ft] = (_int64) (_int32) ((((uint32) gHWS_GPR[rt_ft]) & 0xffff0000) | (LoadWord1 >> 16));
		break;
	case 0:
		gHWS_GPR[rt_ft] = (_int64) (_int32) ((((uint32) gHWS_GPR[rt_ft]) & 0xffffff00) | (LoadWord1 >> 24));
		break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_swl(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32			LoadWord1 = 0;
	uint32			rt_ft = RT_FT;
	uint32			vAddr;
	STORE_TLB_FUN	vAddr = QuerAddr & 0xfffffffc;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	LoadWord1 = MEM_READ_UWORD(vAddr);
	switch(QuerAddr & 3)
	{
	case 0: LoadWord1 = (uint32) gHWS_GPR[rt_ft]; break;
	case 1: LoadWord1 = (uint32) (LoadWord1 & 0xff000000) | ((uint32) gHWS_GPR[rt_ft] >> 8); break;
	case 2: LoadWord1 = (uint32) (LoadWord1 & 0xffff0000) | ((uint32) gHWS_GPR[rt_ft] >> 16); break;
	case 3: LoadWord1 = (uint32) (LoadWord1 & 0xffffff00) | ((uint32) gHWS_GPR[rt_ft] >> 24); break;
	}
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		/*
		 * a little stupid, do I have to do such things? or can I take chances by just
		 * using £
		 * (PMEM_WRITE_UWORD(vAddr, rt_ft)) = LoadWord1; £
		 * first of all, I hope SWL/SWR/SDL/SDR opcode are not used so much, so speed is
		 * not a problem here
		 */
		uint32	temp = (uint32) gHWS_GPR[rt_ft];
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		*(uint32 *) &gHWS_GPR[rt_ft] = LoadWord1;
		*(PMEM_WRITE_UWORD(vAddr)) = LoadWord1;
		*(uint32 *) &gHWS_GPR[rt_ft] = temp;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_swr(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32			LoadWord1 = 0;
	uint32			vAddr;
	uint32			rt_ft = RT_FT;
	STORE_TLB_FUN	vAddr = QuerAddr & 0xfffffffc;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	LoadWord1 = MEM_READ_UWORD(vAddr);
	switch(QuerAddr & 3)
	{
	case 3: LoadWord1 = (uint32) gHWS_GPR[rt_ft]; break;
	case 2: LoadWord1 = (uint32) ((LoadWord1 & 0x000000FF) | ((uint32) gHWS_GPR[rt_ft] << 8)); break;
	case 1: LoadWord1 = (uint32) ((LoadWord1 & 0x0000FFFF) | ((uint32) gHWS_GPR[rt_ft] << 16)); break;
	case 0: LoadWord1 = (uint32) ((LoadWord1 & 0x00FFFFFF) | ((uint32) gHWS_GPR[rt_ft] << 24)); break;
	}
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		/*
		 * a little stupid, do I have to do such things? or can I take chances by just
		 * using £
		 * (PMEM_WRITE_UWORD(vAddr, rt_ft)) = LoadWord1; £
		 * first of all, I hope SWL/SWR/SDL/SDR opcode are not used so much, so speed is
		 * not a problem here
		 */
		uint32	temp = (uint32) gHWS_GPR[rt_ft];
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		*(uint32 *) &gHWS_GPR[rt_ft] = LoadWord1;
		*(PMEM_WRITE_UWORD(vAddr)) = LoadWord1;
		*(uint32 *) &gHWS_GPR[rt_ft] = temp;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_ldl(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64			tempdword;
	uint32			rt_ft = RT_FT;
	LOAD_TLB_FUN
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	tempdword = (uint64) MEM_READ_UWORD((QuerAddr & 0xFFFFFFF8));
	tempdword = (tempdword << 32) |
	MEM_READ_UWORD(((QuerAddr & 0xFFFFFFF8) + 4));

	switch(QuerAddr % 8)
	{
	case 0: gHWS_GPR[rt_ft] = tempdword; break;
	case 1: gHWS_GPR[rt_ft] = ((gHWS_GPR[rt_ft] & 0x00000000000000FF) | (tempdword << 8)); break;
	case 2: gHWS_GPR[rt_ft] = ((gHWS_GPR[rt_ft] & 0x000000000000FFFF) | (tempdword << 16)); break;
	case 3: gHWS_GPR[rt_ft] = ((gHWS_GPR[rt_ft] & 0x0000000000FFFFFF) | (tempdword << 24)); break;
	case 4: gHWS_GPR[rt_ft] = ((gHWS_GPR[rt_ft] & 0x00000000FFFFFFFF) | (tempdword << 32)); break;
	case 5: gHWS_GPR[rt_ft] = ((gHWS_GPR[rt_ft] & 0x000000FFFFFFFFFF) | (tempdword << 40)); break;
	case 6: gHWS_GPR[rt_ft] = ((gHWS_GPR[rt_ft] & 0x0000FFFFFFFFFFFF) | (tempdword << 48)); break;
	case 7: gHWS_GPR[rt_ft] = ((gHWS_GPR[rt_ft] & 0x00FFFFFFFFFFFFFF) | (tempdword << 56)); break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_ldr(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64			tempdword;
	uint32			rt_ft = RT_FT;
	LOAD_TLB_FUN
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	tempdword = (uint64) MEM_READ_UWORD((QuerAddr & 0xFFFFFFF8));
	tempdword = (tempdword << 32) |
	MEM_READ_UWORD(((QuerAddr & 0xFFFFFFF8) + 4));

	switch(QuerAddr % 8)
	{
	case 0: gHWS_GPR[rt_ft] = (gHWS_GPR[rt_ft] & 0xFFFFFFFFFFFFFF00) | (tempdword >> 56); break;
	case 1: gHWS_GPR[rt_ft] = (gHWS_GPR[rt_ft] & 0xFFFFFFFFFFFF0000) | (tempdword >> 48); break;
	case 2: gHWS_GPR[rt_ft] = (gHWS_GPR[rt_ft] & 0xFFFFFFFFFF000000) | (tempdword >> 40); break;
	case 3: gHWS_GPR[rt_ft] = (gHWS_GPR[rt_ft] & 0xFFFFFFFF00000000) | (tempdword >> 32); break;
	case 4: gHWS_GPR[rt_ft] = (gHWS_GPR[rt_ft] & 0xFFFFFF0000000000) | (tempdword >> 24); break;
	case 5: gHWS_GPR[rt_ft] = (gHWS_GPR[rt_ft] & 0xFFFF000000000000) | (tempdword >> 16); break;
	case 6: gHWS_GPR[rt_ft] = (gHWS_GPR[rt_ft] & 0xFF00000000000000) | (tempdword >> 8); break;
	case 7: gHWS_GPR[rt_ft] = tempdword; break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sdl(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32			rt_ft = RT_FT;
	uint64			tempdword;
	uint64			grt = (uint64) gHWS_GPR[rt_ft];
	STORE_TLB_FUN	tempdword = (uint64) MEM_READ_UWORD((QuerAddr & 0xFFFFFFF8));
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	tempdword = (tempdword << 32) |
	MEM_READ_UWORD(((QuerAddr & 0xFFFFFFF8) + 4));
	switch(QuerAddr % 8)
	{
	case 0: tempdword = grt; break;
	case 1: tempdword = (tempdword & 0xFF00000000000000) | (grt >> 8); break;
	case 2: tempdword = (tempdword & 0xFFFF000000000000) | (grt >> 16); break;
	case 3: tempdword = (tempdword & 0xFFFFFF0000000000) | (grt >> 24); break;
	case 4: tempdword = (tempdword & 0xFFFFFFFF00000000) | (grt >> 32); break;
	case 5: tempdword = (tempdword & 0xFFFFFFFFFF000000) | (grt >> 40); break;
	case 6: tempdword = (tempdword & 0xFFFFFFFFFFFF0000) | (grt >> 48); break;
	case 7: tempdword = (tempdword & 0xFFFFFFFFFFFFFF00) | (grt >> 56); break;
	}
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint32	temp = (uint32) gHWS_GPR[rt_ft];
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		*(uint32 *) &gHWS_GPR[rt_ft] = (uint32) (tempdword >> 32);
		*(PMEM_WRITE_UWORD((QuerAddr & 0xFFFFFFF8))) = (uint32) (tempdword >> 32);
		*(uint32 *) &gHWS_GPR[rt_ft] = (uint32) tempdword;
		*(PMEM_WRITE_UWORD(((QuerAddr & 0xFFFFFFF8) + 4))) = (uint32) tempdword;
		*(uint32 *) &gHWS_GPR[rt_ft] = temp;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sdr(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64			tempdword;
	uint32			rt_ft = RT_FT;
	uint64			grt = (uint64) gHWS_GPR[rt_ft];
	STORE_TLB_FUN	tempdword = (uint64) MEM_READ_UWORD((QuerAddr & 0xFFFFFFF8));
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	tempdword = (tempdword << 32) |
	MEM_READ_UWORD(((QuerAddr & 0xFFFFFFF8) + 4));
	switch(QuerAddr % 8)
	{
	case 0: tempdword = (tempdword & 0x00FFFFFFFFFFFFFF) | (grt << 56); break;
	case 1: tempdword = (tempdword & 0x0000FFFFFFFFFFFF) | (grt << 48); break;
	case 2: tempdword = (tempdword & 0x000000FFFFFFFFFF) | (grt << 40); break;
	case 3: tempdword = (tempdword & 0x00000000FFFFFFFF) | (grt << 32); break;
	case 4: tempdword = (tempdword & 0x0000000000FFFFFF) | (grt << 24); break;
	case 5: tempdword = (tempdword & 0x000000000000FFFF) | (grt << 16); break;
	case 6: tempdword = (tempdword & 0x00000000000000FF) | (grt << 8); break;
	case 7: tempdword = grt; break;
	}
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint32	temp = (uint32) gHWS_GPR[rt_ft];
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		*(uint32 *) &gHWS_GPR[rt_ft] = (uint32) (tempdword >> 32);
		*(PMEM_WRITE_UWORD((QuerAddr & 0xFFFFFFF8))) = (uint32) (tempdword >> 32);
		*(uint32 *) &gHWS_GPR[rt_ft] = (uint32) tempdword;
		*(PMEM_WRITE_UWORD(((QuerAddr & 0xFFFFFFF8) + 4))) = (uint32) tempdword;
		*(uint32 *) &gHWS_GPR[rt_ft] = temp;
	}
}

void	r4300i_InitHardware(HardwareState *gHardwareState);
void	r4300i_ResetMemory(MemoryState *gMemoryState);

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void r4300i_Init(void)
{
	r4300i_InitHardware(&gHardwareState);
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY2
	if(debug_opcode!=0)
	{
		r4300i_InitHardware(&gHardwareState_Interpreter_Compare);
		r4300i_InitHardware(&gHardwareState_Flushed_Dynarec_Compare);
	}
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_Reset(void)
{
	r4300i_ResetMemory(&gMemoryState);

	if(debug_opcode!=0)
	{
		r4300i_ResetMemory(&gMemoryState_Interpreter_Compare);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_InitHardware(HardwareState *gHWState)
{
	/* set all registers to 0 */
	memset(gHWState->COP0Con, 0, sizeof(gHWState->COP0Con));	/* not sure if we even need the COP0Con array */
	memset(gHWState->COP0Reg, 0, sizeof(gHWState->COP0Reg));
	memset(gHWState->COP1Con, 0, sizeof(gHWState->COP1Con));
	memset(gHWState->fpr32, 0, sizeof(gHWState->fpr32));
	memset(gHWState->RememberFprHi, 0, sizeof(gHWState->RememberFprHi));

	gHWState->GPR[__HI] = 0;
	gHWState->GPR[__LO] = 0;
	gHWState->LLbit = 0;

	gHWState->GPR[0x00] = 0;
	gHWState->GPR[0x01] = 0;
	gHWState->GPR[0x02] = 0xffffffffd1731be9;
	gHWState->GPR[0x03] = 0xffffffffd1731be9;
	gHWState->GPR[0x04] = 0x01be9;
	gHWState->GPR[0x05] = 0xfffffffff45231e5;
	gHWState->GPR[0x06] = 0xffffffffa4001f0c;
	gHWState->GPR[0x07] = 0xffffffffa4001f08;
	gHWState->GPR[0x08] = 0x070;
	gHWState->GPR[0x09] = 0;
	gHWState->GPR[0x0a] = 0x040;
	gHWState->GPR[0x0b] = 0xffffffffa4000040;
	gHWState->GPR[0x0c] = 0xffffffffd1330bc3;
	gHWState->GPR[0x0d] = 0xffffffffd1330bc3;
	gHWState->GPR[0x0e] = 0x025613a26;
	gHWState->GPR[0x0f] = 0x02ea04317;
	gHWState->GPR[0x10] = 0;
	gHWState->GPR[0x11] = 0;
	gHWState->GPR[0x12] = 0;
	gHWState->GPR[0x13] = 0;
	gHWState->GPR[0x14] = rominfo.TV_System;
	gHWState->GPR[0x15] = 0;
	gHWState->GPR[0x16] = rominfo.CIC;
	gHWState->GPR[0x17] = 0x06;
	gHWState->GPR[0x18] = 0;
	gHWState->GPR[0x19] = 0xffffffffd73f2993;
	gHWState->GPR[0x1a] = 0;
	gHWState->GPR[0x1b] = 0;
	gHWState->GPR[0x1c] = 0;
	gHWState->GPR[0x1d] = 0xffffffffa4001ff0;
	gHWState->GPR[0x1e] = 0;
	gHWState->GPR[0x1f] = 0xffffffffa4001554;

	gHWState->COP0Reg[STATUS] = 0x70400004;
	gHWState->COP0Reg[RANDOM] = 0x0000001f;

	/* gHWState->COP0Reg[RANDOM] = 0x0000002f; */
	gHWState->COP0Reg[CONFIG] = 0x0006e463;						/* zils boot doc ... */
	gHWState->COP0Reg[PREVID] = 0x00000b00;
	gHWState->COP1Con[0] = 0x00000511;

	/* Copied from PJ64, try to solve the Zelda's RSP LLE error msg problem */
	switch (rominfo.countrycode) {
	case 0x44: //Germany
	case 0x46: //french
	case 0x49: //Italian
	case 0x50: //Europe
	case 0x53: //Spanish
	case 0x55: //Australia
	case 0x58: // ????
	case 0x59: // X (PAL)
		switch (rominfo.CIC) {
		case 0x3f:	// 2 or 1
			gHWState->GPR[5]=0xFFFFFFFFC0F1D859;
			gHWState->GPR[14]=0x000000002DE108EA;
			gHWState->GPR[24]=0x0000000000000000;
			break;
		case 0x78:	// 3
			gHWState->GPR[5]=0xFFFFFFFFD4646273;
			gHWState->GPR[14]=0x000000001AF99984;
			gHWState->GPR[24]=0x0000000000000000;
			break;
		case 0x91:	//5
			*(&SP_IMEM+1) = 0xBDA807FC;
			gHWState->GPR[5]=0xFFFFFFFFDECAAAD1;
			gHWState->GPR[14]=0x000000000CF85C13;
			gHWState->GPR[24]=0x0000000000000002;
			break;
		case 0x85:	//6
			gHWState->GPR[5]=0xFFFFFFFFB04DC903;
			gHWState->GPR[14]=0x000000001AF99984;
			gHWState->GPR[24]=0x0000000000000002;
			break;
		}
		
		gHWState->GPR[20]=0x0000000000000000;
		gHWState->GPR[23]=0x0000000000000006;
		gHWState->GPR[31]=0xFFFFFFFFA4001554;
		break;
		case 0x37: // 7 (Beta)
		case 0x41: // ????
		case 0x45: //USA
		case 0x4A: //Japan
		default:
			switch (rominfo.CIC) {
			case 0x3f:	// 2 or 1
				gHWState->GPR[5]=0xFFFFFFFFC95973D5;
				gHWState->GPR[14]=0x000000002449A366;
				break;
			case 0x78:	// 3
				gHWState->GPR[5]=0xFFFFFFFF95315A28;
				gHWState->GPR[14]=0x000000005BACA1DF;
				break;
			case 0x91:	//5
				*(&SP_IMEM+1) = 0x8DA807FC;
				gHWState->GPR[5]=0x000000005493FB9A;
				gHWState->GPR[14]=0xFFFFFFFFC2C20384;
			case 0x85:	//6
				gHWState->GPR[5]=0xFFFFFFFFE067221F;
				gHWState->GPR[14]=0x000000005CD2B70F;
				break;
			}
			gHWState->GPR[20]=0x0000000000000001;
			gHWState->GPR[23]=0x0000000000000000;
			gHWState->GPR[24]=0x0000000000000003;
			gHWState->GPR[31]=0xFFFFFFFFA4001550;
	}
	
	switch (rominfo.CIC) {
	case 1: 
		gHWState->GPR[22]=0x000000000000003F; 
		break;
	case 0x3f:	// 2 or 1 
		gHWState->GPR[1]=0x0000000000000001;
		gHWState->GPR[2]=0x000000000EBDA536;
		gHWState->GPR[3]=0x000000000EBDA536;
		gHWState->GPR[4]=0x000000000000A536;
		gHWState->GPR[12]=0xFFFFFFFFED10D0B3;
		gHWState->GPR[13]=0x000000001402A4CC;
		gHWState->GPR[15]=0x000000003103E121;
		gHWState->GPR[22]=0x000000000000003F; 
		gHWState->GPR[25]=0xFFFFFFFF9DEBB54F;
		break;
	case 0x78:	// 3
		gHWState->GPR[1]=0x0000000000000001;
		gHWState->GPR[2]=0x0000000049A5EE96;
		gHWState->GPR[3]=0x0000000049A5EE96;
		gHWState->GPR[4]=0x000000000000EE96;
		gHWState->GPR[12]=0xFFFFFFFFCE9DFBF7;
		gHWState->GPR[13]=0xFFFFFFFFCE9DFBF7;
		gHWState->GPR[15]=0x0000000018B63D28;
		gHWState->GPR[22]=0x0000000000000078; 
		gHWState->GPR[25]=0xFFFFFFFF825B21C9;
		break;
	case 0x91:	//5
		*(&SP_IMEM+0) = 0x3C0DBFC0;
		*(&SP_IMEM+2) = 0x25AD07C0;
		*(&SP_IMEM+3) = 0x31080080;
		*(&SP_IMEM+4) = 0x5500FFFC;
		*(&SP_IMEM+5) = 0x3C0DBFC0;
		*(&SP_IMEM+6) = 0x8DA80024;
		*(&SP_IMEM+7) = 0x3C0BB000;
		gHWState->GPR[1]=0x0000000000000000;
		gHWState->GPR[2]=0xFFFFFFFFF58B0FBF;
		gHWState->GPR[3]=0xFFFFFFFFF58B0FBF;
		gHWState->GPR[4]=0x0000000000000FBF;
		gHWState->GPR[12]=0xFFFFFFFF9651F81E;
		gHWState->GPR[13]=0x000000002D42AAC5;
		gHWState->GPR[15]=0x0000000056584D60;
		gHWState->GPR[22]=0x0000000000000091; 
		gHWState->GPR[25]=0xFFFFFFFFCDCE565F;
		break;
	case 0x85:	//6
		gHWState->GPR[1]=0x0000000000000000;
		gHWState->GPR[2]=0xFFFFFFFFA95930A4;
		gHWState->GPR[3]=0xFFFFFFFFA95930A4;
		gHWState->GPR[4]=0x00000000000030A4;
		gHWState->GPR[12]=0xFFFFFFFFBCB59510;
		gHWState->GPR[13]=0xFFFFFFFFBCB59510;
		gHWState->GPR[15]=0x000000007A3C07F4;
		gHWState->GPR[22]=0x0000000000000085; 
		gHWState->GPR[25]=0x00000000465E3F72;
		break;
	}	
	/* End of copied from PJ64, try to solve the Zelda's RSP LLE error msg problem */

	CPUdelayPC = 0;
	CPUdelay = 0;
	gHWState->pc = 0xA4000040;

	gMemoryState.MI[1] = 0x01010101;	/* MI_VERSION_REG (odd place for this) */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_BootHelper(HardwareState *gHWState, MemoryState *gMemoryState)
{
	/*~~~~~~~~~~~~~*/
	uint32	bootaddr;
	/*~~~~~~~~~~~~~*/

	bootaddr = *(uint32 *) (gMemoryState->ROM_Image + 8) & 0x007FFFFF;
	memcpy(gMemoryState->RDRAM + bootaddr, gMemoryState->ROM_Image + 0x1000, 0x400000 - bootaddr);
	gHWState->pc = 0x80000000 +
	bootaddr;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_ResetMemory(MemoryState *gMemoryState)
{
	memset(gMemoryState->RDRAM, 0, MEMORY_SIZE_RDRAM);
	if(current_rdram_size == MEMORY_SIZE_WITH_EXPANSION) memset(gMemoryState->ExRDRAM, 0, MEMORY_SIZE_EXRDRAM);

	/* memset(gMemoryState->RDREG, 0, MEMORY_SIZE_RDREG ); */
	memset(gMemoryState->ramRegs0, 0, MEMORY_SIZE_RAMREGS0);
	memset(gMemoryState->ramRegs4, 0, MEMORY_SIZE_RAMREGS4);
	memset(gMemoryState->ramRegs8, 0, MEMORY_SIZE_RAMREGS8);
	memset(gMemoryState->SP_MEM, 0, MEMORY_SIZE_SPMEM);
	memset(gMemoryState->SP_REG_1, 0, MEMORY_SIZE_SPREG_1);
	memset(gMemoryState->SP_REG_2, 0, MEMORY_SIZE_SPREG_2);
	gMemoryState->SP_REG_1[0x4] = SP_STATUS_HALT;	/* SP_STATUS_REG */
	memset(gMemoryState->DPC, 0, MEMORY_SIZE_DPC);
	memset(gMemoryState->DPS, 0, MEMORY_SIZE_DPS);
	memset(gMemoryState->MI, 0, MEMORY_SIZE_MI);
	gMemoryState->MI[1] = 0x01010101;				/* MI_VERSION_REG */
	memset(gMemoryState->VI, 0, MEMORY_SIZE_VI);
	memset(gMemoryState->AI, 0, MEMORY_SIZE_AI);
	memset(gMemoryState->PI, 0, MEMORY_SIZE_PI);
	memset(gMemoryState->RI, 0, MEMORY_SIZE_RI);
	gMemoryState->RI[1] = 1;						/* RI_CONFIG_REG */
	memset(gMemoryState->SI, 0, MEMORY_SIZE_SI);
	memset(gMemoryState->C2A1, 0, MEMORY_SIZE_C2A1);
	memset(gMemoryState->C1A1, 0, MEMORY_SIZE_C1A1);
	memset(gMemoryState->C2A2, 0, MEMORY_SIZE_C2A2);
	memset(gMemoryState->C1A3, 0, MEMORY_SIZE_C1A3);
	memset(gMemoryState->GIO_REG, 0, MEMORY_SIZE_GIO_REG);
	memset(gMemoryState->PIF, 0, MEMORY_SIZE_PIF);

	r4300i_Init();

	CPUdelayPC = 0;
	CPUdelay = 0;
	InitTLB();
	InitDMA();
	ClearCPUTasks();
	RCP_Reset();

	/* Copy boot code to SP_DMEM */
	memcpy((uint8 *) &gMemoryState->SP_MEM[0], gMemoryState->ROM_Image, 0x1000);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
_int32 Check_LW(uint32 QuerAddr)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	static uint32	Saved_AI_STATUS_REG = 0;
	_int32			tempGPR;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if((QuerAddr & 0xFF000000) == 0x84000000) QuerAddr |= 0xA0000000;

	if (currentromoptions.Save_Type == FLASHRAM_SAVETYPE)
		CHECK_FLASHRAM_LW(QuerAddr);

	switch(QuerAddr)
	{
	/* AI_LEN_REG */
	case 0xA4500004:
		AI_LEN_REG = AUDIO_AiReadLength();
		tempGPR = AI_LEN_REG;
		break;

#ifdef ENABLE_OPCODE_DEBUGGER
	/* AI_STATUS_REG */
	case 0xA450000C:
		AI_STATUS_REG = gMemoryState.AI[3];
		tempGPR = gMemoryState.AI[3];
		break;
#endif

	/* SP_SEMAPHORE_REG */
	case 0xA404001C:
		tempGPR = SP_SEMAPHORE_REG;
		SP_SEMAPHORE_REG = 1;
		break;

	/* VI_CURRENT_REG */
	case 0xA4400010:
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY3
		if(debug_opcode != 1 || p_gMemoryState != &gMemoryState)
		{
			Count_Down(VI_COUNTER_INC_PER_LINE);
		}

#else
		Count_Down(VI_COUNTER_INC_PER_LINE);
#endif
		VI_CURRENT_REG = (Get_VIcounter() / VI_COUNTER_INC_PER_LINE + VI_INTR_REG) % (max_vi_lines + 1);
		tempGPR = VI_CURRENT_REG & 0xFFFFFFFE + vi_field_number;

		/* TRACE1("VI CURRENT_REG is read, val=%08X", VI_CURRENT_REG); */
		break;

	/* SI_STATUS_REG */
	case 0xA4800018:
		if(MI_INTR_REG_R & MI_INTR_SI)	/* This is necessary, but not very necessary */
			SI_STATUS_REG |= SI_STATUS_INTERRUPT;
		else
			SI_STATUS_REG &= ~SI_STATUS_INTERRUPT;
		tempGPR = SI_STATUS_REG;
		break;

	default:
		__try
		{
			tempGPR = LOAD_SWORD_PARAM_2(QuerAddr);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			tempGPR = 0;
			DISPLAY_ADDRESS_ERROR(QuerAddr, "LW");
		}
		break;
	}

#ifdef DEBUG_IO
	DebugIO(QuerAddr, "Read", tempGPR);
#endif

	return(tempGPR);
}

uint32	max_vi_lines;
uint32	max_vi_count;
uint32	vi_count_per_line;

uint32	RTVal;
uint32	SW_QuerAddr;

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void Check_SW(uint32 QuerAddr, uint32 rt_ft)
{
	RTVal = (uint32) gHWS_GPR[rt_ft];

	if((QuerAddr & 0xFF000000) == 0x84000000) QuerAddr |= 0xA0000000;

	if (currentromoptions.Save_Type == FLASHRAM_SAVETYPE)
	if(QuerAddr >= 0xA8000000 && QuerAddr < 0xA8020000)
	{
		if(QuerAddr == FLASHRAM_COMMAND_REG_ADDR)
		{
			Flashram_Command(RTVal);
			return;
		}
		else
		{
			SW_Flashram(QuerAddr, RTVal);
			return;
		}
	}

#ifdef DEBUG_IO
	DebugIO(QuerAddr, "Write", RTVal);
#endif
	switch(QuerAddr)
	{
	/* MI_MODE_REG_ADDR */case 0xA4300000:
		WriteMI_ModeReg(RTVal);
		break;
	/* MI_INTR_MASK_REG */case 0xA430000C:
		Handle_MI(RTVal);
		break;
	/* VI_STATUS_REG */case 0xA4400000:
		if(VI_STATUS_REG != RTVal)
		{
			VI_STATUS_REG = RTVal;
			VIDEO_ViStatusChanged();
		}
		break;

	/* VI_CURRENT_REG */case 0xA4400010:
		/* Clear VI interrupt */
		Clear_MIInterrupt(NOT_MI_INTR_VI);
		break;
	/* VI_V_SYNC_REG */case 0xA4400018:
		VI_V_SYNC_REG = RTVal;
		Set_VI_Counter_By_VSYNC();
		break;
	/* VI_ORIGIN_REG */case 0xA4400004:
		if(VI_ORIGIN_REG != RTVal)
		{
			VI_ORIGIN_REG = RTVal;
			VIDEO_UpdateScreen();
		}
		break;

	/* SP_STATUS_REG */case 0xA4040010:
		Handle_SP(RTVal);
		break;
	/* SI_STATUS_REG */case 0xA4800018:
		Clear_MIInterrupt(NOT_MI_INTR_SI);
		SI_STATUS_REG &= ~SI_STATUS_INTERRUPT;			/* Clear the interrupt bit */
		break;

	/* break; */

	/* AI_STATUS_REG */case 0xA450000C:
		Clear_MIInterrupt(NOT_MI_INTR_AI);
		break;
	/* AI_DACRATE_REG */case 0xA4500010:
		AI_DACRATE_REG = RTVal;
		if(rominfo.TV_System == TV_SYSTEM_NTSC)
			AUDIO_AiDacrateChanged(0);
		else
			AUDIO_AiDacrateChanged(1);
		break;

	/* PI_STATUS_REG */case 0xA4600010:
		if(RTVal & PI_STATUS_CLR_INTR)
		{
			Clear_MIInterrupt(NOT_MI_INTR_PI);
		}

		if(RTVal & PI_STATUS_RESET)
		{
			/*
			 * - When PIC is reset, if PIC happens to be busy, an interrupt will be generated
			 * as PIC returns to idle. Otherwise, no interrupt will be generated and PIC
			 * remains idle.
			 */
			if(PI_STATUS_REG & PI_STATUS_DMA_IO_BUSY)	/* is PI busy */
			{
				/* Reset the PIC */
				PI_STATUS_REG = 0;

				/* Reset finished, set PI Interrupt */
				Trigger_PIInterrupt();
			}
			else
			{
				/* Reset the PIC */
				PI_STATUS_REG = 0;
			}
		}
		break;			/* Does not actually write into the PI_STATUS_REG */

	/* break; */

	/* DPC_STATUS_REG */case 0xA410000C:
		Handle_DPC(RTVal);
		break;
	/* DPC_END_REG */case 0xA4100004:
		Trigger_DPInterrupt();
		break;

	case 0xA4100010:	/* DPC_CLOCK_REG */
	case 0xA4100014:	/* DPC_BUFBUSY_REG */
	case 0xA4100018:	/* DPC_PIPEBUSY_REG */
	case 0xA410001C:	/* DPC_TMEM_REG */
	case 0xA4300004:	/* MI_VERSION_REG or MI_NOOP_REG */
	case 0xA4300008:	/* MI_INTR_REG */
		break;			/* read only registers */

	/* SP_SEMAPHORE_REG */case 0xA404001C:
		SP_SEMAPHORE_REG = 0;
		break;
	/* PI_WR_LEN_REG */case 0xA460000C:
		PI_WR_LEN_REG = RTVal;
		DMA_PI_MemCopy_From_Cart_To_DRAM();
		break;
	/* PI_RD_LEN_REG */case 0xA4600008:
		PI_RD_LEN_REG = RTVal;
		DMA_PI_MemCopy_From_DRAM_To_Cart();
		break;
	/* SP_WR_LEN_REG */case 0xA404000C:
		SP_WR_LEN_REG = RTVal;
		DMA_MemCopy_SP_to_DRAM(0);
		break;
	/* SP_RD_LEN_REG */case 0xA4040008:
		SP_RD_LEN_REG = RTVal;
		DMA_MemCopy_DRAM_To_SP(0);
		break;
	/* SI_PIF_ADDR_WR64B_REG */case 0xA4800010:
		SI_PIF_ADDR_WR64B_REG = RTVal;
		DMA_MemCopy_DRAM_to_SI();
		break;
	/* SI_PIF_ADDR_RD64B_REG */case 0xA4800004:
		SI_PIF_ADDR_RD64B_REG = RTVal;
		DMA_MemCopy_SI_To_DRAM();
		break;
	/* VI_WIDTH_REG */case 0xA4400008:
		if(VI_WIDTH_REG != RTVal)
		{
			VI_WIDTH_REG = RTVal;
			VIDEO_ViWidthChanged();
		}
		break;
	/* AI_LEN_REG */case 0xA4500004:
		DMA_AI();
		AI_LEN_REG = RTVal;
		DEBUG_AUDIO_MACRO(TRACE3("%08X: Play %d bytes of audio at %08X", gHWS_pc, AI_LEN_REG, AI_DRAM_ADDR_REG));
		DO_PROFILIER_AUDIO;
		AUDIO_AiLenChanged();
		if( CoreDoingAIUpdate )
		{
			AUDIO_AiUpdate(FALSE);
		}
		
		DO_PROFILIER_R4300I;
		break;

	default:
		__try
		{
			LOAD_UWORD_PARAM_2(QuerAddr) = RTVal;
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			DISPLAY_ADDRESS_ERROR(QuerAddr, "SW");
		}
		break;
	}
}

/*
 =======================================================================================================================
    This function will display error message and set exception when address error happens when SW
 =======================================================================================================================
 */
void SW_AddrError(uint32 QuerAddr, uint32 val)
{
	DISPLAY_ADDRESS_ERROR(QuerAddr, "SW");
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
__inline void Display_Address_Error(uint32 addr, char *opcode)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef DEBUG_COMMON
	uint32	Instruction = FetchInstruction();
	uint32	virtualaddress = (uint32) ((_int32) gBASE + (_int32) OFFSET_IMMEDIATE);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	TRACE4("%08X: %s to access VA=%08X, PA=%08X, Out of range. ", gHWS_pc, opcode, virtualaddress, addr);
#endif
}
