/*$T r4300i.h GC 1.136 03/09/02 17:29:24 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Some interpretive load/store macros here
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
#ifndef _R4300I_H__1964_
#define _R4300I_H__1964_

#include "globals.h"
#include "debug_option.h"
#include "hardware.h"


#define CPU_CORE_CHECK_R0

/*
 * CPU Core Options £
 * Check and make sure register R0=0
 */
#ifdef DEBUG_COMMON

/* Check address alignment for store/load instructions */
#define ADDR_ALIGN_CHECKING

/*
 * If this option is enabled, CPU core will generate ADDRESS ERROR exception when
 * LOAD or STORE instructions access unmapped memory range. Otherwise CPU core
 * will not generate such exceptions
 */
#define ADDRESS_ERROR_EXCEPTION
#endif

/* TLB size constants */
#define NTLBENTRIES			31				/* entry 31 is reserved by rdb */

#define TLBHI_VPN2MASK		0xffffe000
#define TLBHI_VPN2SHIFT		13
#define TLBHI_PIDMASK		0xff
#define TLBHI_PIDSHIFT		0
#define TLBHI_NPID			255				/* 255 to fit in 8 bits */
#define TLBLO_PFNMASK		0x3fffffc0
#define TLBLO_PFNSHIFT		6
#define TLBLO_CACHMASK		0x38			/* cache coherency algorithm */
#define TLBLO_CACHSHIFT		3
#define TLBLO_UNCACHED		0x10			/* not cached */
#define TLBLO_NONCOHRNT		0x18			/* Cacheable non-coherent */
#define TLBLO_EXLWR			0x28			/* Exclusive write */
#define TLBLO_D				0x4				/* writeable */
#define TLBLO_V				0x2				/* valid bit */
#define TLBLO_G				0x1				/* global access bit */
#define TLBINX_PROBE		0x80000000
#define TLBINX_INXMASK		0x3f
#define TLBINX_INXSHIFT		0
#define TLBRAND_RANDMASK	0x3f
#define TLBRAND_RANDSHIFT	0
#define TLBWIRED_WIREDMASK	0x3f
#define TLBCTXT_BASEMASK	0xff800000
#define TLBCTXT_BASESHIFT	23
#define TLBCTXT_BASEBITS	9
#define TLBCTXT_VPNMASK		0x7ffff0
#define TLBCTXT_VPNSHIFT	4
#define TLBPGMASK_4K		0x0
#define TLBPGMASK_16K		0x6000
#define TLBPGMASK_64K		0x1e000

/* Exception vectors */
#define SIZE_EXCVEC 0x80					/* Size of an exc. vec */
#define UT_VEC		K0BASE					/* utlbmiss vector */
#define R_VEC		(K1BASE + 0x1fc00000)	/* reset vector */
#define XUT_VEC		(K0BASE + 0x80)			/* extended address tlbmiss */
#define ECC_VEC		(K0BASE + 0x100)		/* Ecc exception vector */
#define E_VEC		(K0BASE + 0x180)		/* Gen. exception vector */

/* Status register */
#define SR_CUMASK	0xf0000000				/* coproc usable bits */

#define SR_CU3		0x80000000				/* Coprocessor 3 usable */
#define SR_CU2		0x40000000				/* Coprocessor 2 usable */
#define SR_CU1		0x20000000				/* Coprocessor 1 usable */
#define SR_CU0		0x10000000				/* Coprocessor 0 usable */
#define SR_RP		0x08000000				/* Reduced power (quarter speed) */
#define SR_FR		0x04000000				/* MIPS III FP register mode */
#define SR_RE		0x02000000				/* Reverse endian */
#define SR_ITS		0x01000000				/* Instruction trace support */
#define SR_BEV		0x00400000				/* Use boot exception vectors */
#define SR_TS		0x00200000				/* TLB shutdown */
#define SR_SR		0x00100000				/* Soft reset occured */
#define SR_CH		0x00040000				/* Cache hit for last 'cache' op */
#define SR_CE		0x00020000				/* Create ECC */
#define SR_DE		0x00010000				/* ECC of parity does not cause error */
#define STATUS_CU1	SR_CU1

/*
 * Interrupt enable bits (NOTE: bits set to 1 enable the corresponding level
 * interrupt)
 */
#define SR_IMASK	0x0000ff00				/* Interrupt mask */
#define SR_IMASK8	0x00000000				/* mask level 8 */
#define SR_IMASK7	0x00008000				/* mask level 7 */
#define SR_IMASK6	0x0000c000				/* mask level 6 */
#define SR_IMASK5	0x0000e000				/* mask level 5 */
#define SR_IMASK4	0x0000f000				/* mask level 4 */
#define SR_IMASK3	0x0000f800				/* mask level 3 */
#define SR_IMASK2	0x0000fc00				/* mask level 2 */
#define SR_IMASK1	0x0000fe00				/* mask level 1 */
#define SR_IMASK0	0x0000ff00				/* mask level 0 */

#define SR_KX		0x00000080				/* extended-addr TLB vec in kernel */
#define SR_SX		0x00000040				/* xtended-addr TLB vec supervisor */
#define SR_UX		0x00000020				/* xtended-addr TLB vec in user mode */
#define SR_KSU_MASK 0x00000018				/* mode mask */
#define SR_KSU_USR	0x00000010				/* user mode */
#define SR_KSU_SUP	0x00000008				/* supervisor mode */
#define SR_KSU_KER	0x00000000				/* kernel mode */
#define SR_ERL		0x00000004				/* Error level, 1=>cache error */
#define SR_EXL		0x00000002				/* Exception level, 1=>exception */
#define SR_IE		0x00000001				/* interrupt enable, 1=>enable */

/* Cause Register */
#define CAUSE_BD		0x80000000			/* Branch delay slot */
#define CAUSE_CEMASK	0x30000000			/* coprocessor error */
#define CAUSE_CESHIFT	28

/* Interrupt pending bits */
#define CAUSE_IP8		0x00008000			/* External level 8 pending - COMPARE */
#define CAUSE_IP7		0x00004000			/* External level 7 pending - INT4 */
#define CAUSE_IP6		0x00002000			/* External level 6 pending - INT3 */
#define CAUSE_IP5		0x00001000			/* External level 5 pending - INT2 */
#define CAUSE_IP4		0x00000800			/* External level 4 pending - INT1 */
#define CAUSE_IP3		0x00000400			/* External level 3 pending - INT0 */
#define NOT_CAUSE_IP3	0xFFFFFBFF
#define CAUSE_SW2		0x00000200			/* Software level 2 pending */
#define CAUSE_SW1		0x00000100			/* Software level 1 pending */
#define CAUSE_BD		0x80000000
#define CAUSE_CE_MASK	0x30000000
#define CAUSE_CE0		0x00000000
#define CAUSE_CE1		0x10000000
#define CAUSE_CE2		0x20000000
#define CAUSE_CE3		0x30000000
#define CAUSE_IPMASK	0x0000FF00			/* Pending interrupt mask */
#define CAUSE_IPSHIFT	8
#define CAUSE_EXCMASK	0x0000007C			/* Cause code bits */
#define CAUSE_EXCSHIFT	2

#define COP1_CONDITION_BIT	0x00800000

/* little-endian memory operations */
#define QUER_ADDR					QuerAddr = (uint32) ((_int32) gBASE + (_int32) OFFSET_IMMEDIATE)
#define LOAD_SBYTE_PARAM(param)		*((_int8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF) ^ 3))
#define LOAD_UBYTE_PARAM(param)		*((uint8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF) ^ 3))
#define LOAD_SHALF_PARAM(param)		*(_int16 *) ((uint8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF) ^ 2))
#define LOAD_UHALF_PARAM(param)		*(uint16 *) ((uint8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF) ^ 2))
#define LOAD_SWORD_PARAM(param)		*(_int32 *) ((uint8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF)))
#define LOAD_UWORD_PARAM(param)		*(uint32 *) ((uint8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF)))
#define pLOAD_SBYTE_PARAM(param)	((_int8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF) ^ 3))
#define pLOAD_UBYTE_PARAM(param)	((uint8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF) ^ 3))
#define pLOAD_SHALF_PARAM(param)	(_int16 *) ((uint8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF) ^ 2))
#define pLOAD_UHALF_PARAM(param)	(uint16 *) ((uint8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF) ^ 2))
#define pLOAD_SWORD_PARAM(param)	(_int32 *) ((uint8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF)))
#define pLOAD_UWORD_PARAM(param)	(uint32 *) ((uint8 *) TLB_sDWORD_R[(((param) >> 12))] + (((param) & 0x00000FFF)))
#define LOAD_SBYTE_PARAM_2(param)	*((_int8 *) sDWORD_R_2[((uint16) ((param) >> 16))] + ((uint16) (param) ^ 3))
#define LOAD_UBYTE_PARAM_2(param)	*((uint8 *) sDWORD_R_2[((uint16) ((param) >> 16))] + ((uint16) (param) ^ 3))
#define LOAD_SHALF_PARAM_2(param) \
	*(_int16 *) ((uint8 *) sDWORD_R_2[((uint16) ((param) >> 16))] + ((uint16) (param) ^ 2))
#define LOAD_UHALF_PARAM_2(param) \
	*(uint16 *) ((uint8 *) sDWORD_R_2[((uint16) ((param) >> 16))] + ((uint16) (param) ^ 2))
#define LOAD_SWORD_PARAM_2(param)	*(_int32 *) ((uint8 *) sDWORD_R_2[((uint16) ((param) >> 16))] + ((uint16) (param)))
#define LOAD_UWORD_PARAM_2(param)	*(uint32 *) ((uint8 *) sDWORD_R_2[((uint16) ((param) >> 16))] + ((uint16) (param)))
#define LOAD_DOUBLE_PARAM_2(param)	*(uint64 *) ((uint8 *) sDWORD_R_2[((uint16) ((param) >> 16))] + ((uint16) (param)))
#define pLOAD_UBYTE_PARAM_2(param)	((uint8 *) sDWORD_R_2[((uint16) ((param) >> 16))] + ((uint16) (param) ^ 3))
#define BAD_TASK					0
#define GFX_TASK					1
#define SND_TASK					2

#define _GET_RSP_INSTRUCTION_		Instruction = *RSP_InstructionPointer;
#define _OPCODE_					((unsigned) (Instruction >> 26))
#define _FUNCTION_					((unsigned) (Instruction & 0x3F))
#define SA_FD						((unsigned) ((Instruction >> 6) & 0x1F))
#define RD_FS						((uint32) ((Instruction >> 11) & 0x1F))
#define RT_FT						((uint32) ((Instruction >> 16) & 0x1F))
#define RS_BASE_FMT					((uint32) ((Instruction >> 21) & 0x1F))
#define OFFSET_IMMEDIATE			((_int16) (Instruction & 0xFFFF))

/* define OFFSET_IMMEDIATE ((_int16)(Instruction)) */
#define BITWISE_IMMEDIATE	(uint16) (Instruction & 0xFFFF)
#define _ND_				((uint8) ((Instruction >> 17) & 0x1))
#define _TF_				((uint8) ((Instruction >> 16) & 0x1))

/*
 * The first 4 bits of instr_index = the 1st 4 bits of pc The next 26 bits of
 * instr_index = the last 26 bits of instruction The last 2 bits of instr_index 00
 * instr_index is a misnomer (should be called target, but easier to understand
 * this way)
 */
#define INSTR_INDEX ((gHWS_pc & 0xF0000000) | ((Instruction & 0x03FFFFFF) << 2))

extern uint32	CPUdelayPC;
extern uint32	CPUdelay;
extern uint32	FR_reg_offset;

#define cFD				gHWS_fpr32[SA_FD]
#define cFS				gHWS_fpr32[RD_FS]
#define cFT				gHWS_fpr32[RT_FT]
#define FPU_Reg(reg)	gHWS_fpr32[reg]
#define cCON31			gHWS_COP1Con[31]
#define cCONFS			gHWS_COP1Con[RD_FS]
#define gRS				gHWS_GPR[RS_BASE_FMT]
#define gBASE			gHWS_GPR[RS_BASE_FMT]
#define gRD				gHWS_GPR[RD_FS]
#define gRT				gHWS_GPR[RT_FT]
#define c0FD			gHWS_COP0Reg[SA_FD]
#define c0FS			gHWS_COP0Reg[RD_FS]
#define c0FT			gHWS_COP0Reg[RT_FT]

/* General Purpose Registers */
#define R0	0x00
#define AT	0x01
#define V0	0x02
#define V1	0x03
#define A0	0x04
#define A1	0x05
#define A2	0x06
#define A3	0x07
#define T0	0x08
#define T1	0x09
#define T2	0x0A
#define T3	0x0B
#define T4	0x0C
#define T5	0x0D
#define T6	0x0E
#define T7	0x0F
#define S0	0x10
#define S1	0x11
#define S2	0x12
#define S3	0x13
#define S4	0x14
#define S5	0x15
#define S6	0x16
#define S7	0x17
#define T8	0x18
#define T9	0x19
#define K0	0x1A
#define K1	0x1B
#define GP	0x1C
#define SP	0x1D
#define S8	0x1E
#define RA	0x1F

/* Coprocessor0 Registers */
#define INDEX		0x00
#define RANDOM		0x01
#define ENTRYLO0	0x02
#define ENTRYLO1	0x03
#define CONTEXT		0x04
#define PAGEMASK	0x05
#define WIRED		0x06
#define RESERVED0	0x07
#define BADVADDR	0x08
#define COUNT		0x09
#define ENTRYHI		0x0A
#define COMPARE		0x0B
#define STATUS		0x0C
#define CAUSE		0x0D
#define EPC			0x0E
#define PREVID		0x0F
#define CONFIG		0x10
#define LLADDR		0x11
#define WATCHLO		0x12
#define WATCHHI		0x13
#define XCONTEXT	0x14
#define RESERVED1	0x15
#define RESERVED2	0x16
#define RESERVED3	0x17
#define RESERVED4	0x18
#define RESERVED5	0x19
#define PERR		0x1A
#define CACHEERR	0x1B
#define TAGLO		0x1C
#define TAGHI		0x1D
#define ERROREPC	0x1E
#define RESERVED6	0x1F

#define DELAY_SET \
	{ \
		CPUdelayPC = gHWS_pc + 4 + (OFFSET_IMMEDIATE << 2); \
		CPUdelay = 1; \
	}
#define DELAY_SKIP \
	{ \
		gHWS_pc += 4; \
	}
#define INTERPRETIVE_LINK(X)	{ gHWS_GPR[X] = (_int32) (gHWS_pc + 8); }

#ifdef DEBUG_COMMON
#define CHECK_RS_EQUAL_RA(X, op)	{ if(X == 31) DisplayError("Opcode %s: rs=31 (rs=ra)", op); }
#else
#define CHECK_RS_EQUAL_RA(X, op)
#endif
#define sLOGIC(Sum, Operand1, OPERATOR, Operand2)	Sum = (_int64) ((_int32) Operand1 OPERATOR(_int32)Operand2)
#define uLOGIC(Sum, Operand1, OPERATOR, Operand2)	Sum = (_int64) (_int32) ((uint32) Operand1 OPERATOR (uint32)Operand2)
#define sDLOGIC(Sum, Operand1, OPERATOR, Operand2)	Sum = (_int64) Operand1 OPERATOR (_int64)Operand2
#define uDLOGIC(Sum, Operand1, OPERATOR, Operand2)	Sum = (uint64) Operand1 OPERATOR (uint64)Operand2
#define sLOGICAL(OPERATOR)							sLOGIC(gRD, gRS, OPERATOR, gRT)
#define uLOGICAL(OPERATOR)							uLOGIC(gRD, gRS, OPERATOR, gRT)
#define sDLOGICAL(OPERATOR)							sDLOGIC(gRD, gRS, OPERATOR, gRT)
#define uDLOGICAL(OPERATOR)							uDLOGIC(gRD, gRS, OPERATOR, gRT)
#define sLOGICAL_WITH_IMM(OPERATOR)					sLOGIC(gRT, gRS, OPERATOR, (_int16) OFFSET_IMMEDIATE)
#define uLOGICAL_WITH_IMM(OPERATOR)					uLOGIC(gRT, gRS, OPERATOR, (_int16) OFFSET_IMMEDIATE)
#define sDLOGICAL_WITH_IMM(OPERATOR)				sDLOGIC(gRT, gRS, OPERATOR, (_int16) OFFSET_IMMEDIATE)
#define uDLOGICAL_WITH_IMM(OPERATOR)				uDLOGIC(gRT, gRS, OPERATOR, (uint16) OFFSET_IMMEDIATE)
#define sLOGICAL_SHIFT(OPERATOR, ShiftAmount)		sLOGIC(gRD, gRT, OPERATOR, (ShiftAmount))
#define sDLOGICAL_SHIFT(OPERATOR, ShiftAmount)		sDLOGIC(gRD, gRT, OPERATOR, (ShiftAmount))
#define uLOGICAL_SHIFT(OPERATOR, ShiftAmount)		uLOGIC(gRD, gRT, OPERATOR, (ShiftAmount))
#define uDLOGICAL_SHIFT(OPERATOR, ShiftAmount)		uDLOGIC(gRD, gRT, OPERATOR, (ShiftAmount))

enum { TLB_LOAD, TLB_STORE, TLB_INST };

/* define DUMMYDIRECTTLBVALUE 0xFFFFFFFF */
#define DUMMYDIRECTTLBVALUE			0x00000000
#define VALIDDIRECTTLBVALUE			0x80000000
#define INVALIDDIRECTTLBVALUE		0x10000000
#define ISVALIDDIRECTTLBVALUE(val)	(val >= VALIDDIRECTTLBVALUE)

/*
 =======================================================================================================================
    define ISVALIDDIRECTTLBVALUE(val) (val&VALIDDIRECTTLBVALUE)
 =======================================================================================================================
 */
#define ISNOTVALIDDIRECTTLBVALUE(val)	(val < VALIDDIRECTTLBVALUE)
#define STORE_TLB_TRANSLATE_ADDR_IF_NEEDED(address) \
	{ \
		if(NOT_IN_KO_K1_SEG(address)) Direct_TLB_Lookup(address, TLB_STORE); \
	}
#define LOAD_TLB_TRANSLATE_ADDR_IF_NEEDED(address) \
	{ \
		if(NOT_IN_KO_K1_SEG(address)) Direct_TLB_Lookup(address, TLB_LOAD); \
	}

#define LOAD_TLB_FUN	uint32 QuerAddr; \
	QUER_ADDR; \
	LOAD_TLB_TRANSLATE_ADDR_IF_NEEDED(QuerAddr)
#define STORE_TLB_FUN	uint32 QuerAddr; \
	QUER_ADDR; \
	STORE_TLB_TRANSLATE_ADDR_IF_NEEDED(QuerAddr)

extern void		r4300i_Init(void);
extern void		r4300i_Reset(void);

extern uint32	Direct_TLB_Lookup_Table[0x100000];
extern _int32	Check_LW(unsigned __int32);
extern void		Check_SW(unsigned __int32, unsigned __int32);
extern void		UNUSED(unsigned __int32);
extern void		r4300i_cache(unsigned __int32);
extern void		r4300i_daddiu(unsigned __int32);
extern void		r4300i_daddiu_32bit(unsigned __int32);
extern void		r4300i_lb(unsigned __int32);
extern void		r4300i_lbu(unsigned __int32);
extern void		r4300i_ld(unsigned __int32);
extern void		r4300i_ldl(unsigned __int32);
extern void		r4300i_ldr(unsigned __int32);
extern void		r4300i_lh(unsigned __int32);
extern void		r4300i_lhu(unsigned __int32);
extern void		r4300i_ll(unsigned __int32);
extern void		r4300i_lld(unsigned __int32);
extern void		r4300i_lw(unsigned __int32);
extern void		r4300i_lwl(unsigned __int32);
extern void		r4300i_lwr(unsigned __int32);
extern void		r4300i_lwu(unsigned __int32);
extern void		r4300i_sb(unsigned __int32);
extern void		r4300i_sc(unsigned __int32);
extern void		r4300i_scd(unsigned __int32);
extern void		r4300i_sd(unsigned __int32);
extern void		r4300i_sdl(unsigned __int32);
extern void		r4300i_sdr(unsigned __int32);
extern void		r4300i_sh(unsigned __int32);
extern void		r4300i_sw(unsigned __int32);
extern void		r4300i_swl(unsigned __int32);
extern void		r4300i_swr(unsigned __int32);
extern void		r4300i_ldc1(unsigned __int32);
extern void		r4300i_lwc1(unsigned __int32);
extern void		r4300i_sdc1(unsigned __int32);
extern void		r4300i_swc1(unsigned __int32);
extern void		r4300i_lui(unsigned __int32);
extern void		r4300i_addiu(unsigned __int32);
extern void		r4300i_addi(unsigned __int32);
extern void		r4300i_andi(unsigned __int32);
extern void		r4300i_daddi(unsigned __int32);
extern void		r4300i_daddi_32bit(unsigned __int32);
extern void		r4300i_ori(unsigned __int32);
extern void		r4300i_slti(unsigned __int32);
extern void		r4300i_sltiu(unsigned __int32);
extern void		r4300i_xori(unsigned __int32);
extern void		r4300i_beq(unsigned __int32);
extern void		r4300i_beql(unsigned __int32);
extern void		r4300i_bne(unsigned __int32);
extern void		r4300i_bnel(unsigned __int32);
extern void		r4300i_jal(unsigned __int32);
extern void		r4300i_ldc2(unsigned __int32);
extern void		r4300i_swc2(unsigned __int32);
extern void		r4300i_bltz(unsigned __int32);
extern void		r4300i_bgez(unsigned __int32);
extern void		r4300i_bltzl(unsigned __int32);
extern void		r4300i_bgezl(unsigned __int32);
extern void		r4300i_tgei(unsigned __int32);
extern void		r4300i_tgeiu(unsigned __int32);
extern void		r4300i_tlti(unsigned __int32);
extern void		r4300i_tltiu(unsigned __int32);
extern void		r4300i_teqi(unsigned __int32);
extern void		r4300i_tnei(unsigned __int32);
extern void		r4300i_bltzal(unsigned __int32);
extern void		r4300i_bgezal(unsigned __int32);
extern void		r4300i_bltzall(unsigned __int32);
extern void		r4300i_bgezall(unsigned __int32);
extern void		r4300i_sll(unsigned __int32);
extern void		r4300i_srl(unsigned __int32);
extern void		r4300i_sra(unsigned __int32);
extern void		r4300i_sllv(unsigned __int32);
extern void		r4300i_srlv(unsigned __int32);
extern void		r4300i_srav(unsigned __int32);
extern void		r4300i_jr(unsigned __int32);
extern void		r4300i_jalr(unsigned __int32);
extern void		r4300i_syscall(unsigned __int32);
extern void		r4300i_Break(unsigned __int32);
extern void		r4300i_sync(unsigned __int32);
extern void		r4300i_mfhi(unsigned __int32);
extern void		r4300i_mthi(unsigned __int32);
extern void		r4300i_mflo(unsigned __int32);
extern void		r4300i_mtlo(unsigned __int32);
extern void		r4300i_dsllv(unsigned __int32);
extern void		r4300i_dsrlv(unsigned __int32);
extern void		r4300i_dsrav(unsigned __int32);
extern void		r4300i_mult(unsigned __int32);
extern void		r4300i_multu(unsigned __int32);
extern void		r4300i_div(unsigned __int32);
extern void		r4300i_divu(unsigned __int32);
extern void		r4300i_dmult(unsigned __int32);
extern void		r4300i_dmultu(unsigned __int32);
extern void		r4300i_ddiv(unsigned __int32);
extern void		r4300i_ddivu(unsigned __int32);
extern void		r4300i_add(unsigned __int32);
extern void		r4300i_addu(unsigned __int32);
extern void		r4300i_sub(unsigned __int32);
extern void		r4300i_subu(unsigned __int32);
extern void		r4300i_and(unsigned __int32);
extern void		r4300i_or(unsigned __int32);
extern void		r4300i_xor(unsigned __int32);
extern void		r4300i_nor(unsigned __int32);
extern void		r4300i_slt(unsigned __int32);
extern void		r4300i_sltu(unsigned __int32);
extern void		r4300i_dadd(unsigned __int32);
extern void		r4300i_dadd_32bit(unsigned __int32);
extern void		r4300i_daddu(unsigned __int32);
extern void		r4300i_daddu_32bit(unsigned __int32);
extern void		r4300i_dsub(unsigned __int32);
extern void		r4300i_dsubu(unsigned __int32);
extern void		r4300i_tge(unsigned __int32);
extern void		r4300i_tgeu(unsigned __int32);
extern void		r4300i_tlt(unsigned __int32);
extern void		r4300i_tltu(unsigned __int32);
extern void		r4300i_teq(unsigned __int32);
extern void		r4300i_tne(unsigned __int32);
extern void		r4300i_dsll(unsigned __int32);
extern void		r4300i_dsll_32bit(unsigned __int32);
extern void		r4300i_dsrl(unsigned __int32);
extern void		r4300i_dsrl_32bit(unsigned __int32);
extern void		r4300i_dsra(unsigned __int32);
extern void		r4300i_dsra_32bit(unsigned __int32);
extern void		r4300i_dsll32(unsigned __int32);
extern void		r4300i_dsrl32(unsigned __int32);
extern void		r4300i_dsra32(unsigned __int32);
extern void		r4300i_COP0_mfc0(unsigned __int32);
extern void		r4300i_COP0_mtc0(unsigned __int32);
extern void		r4300i_COP1_cfc1(unsigned __int32);
extern void		r4300i_COP1_ctc1(unsigned __int32);
extern void		r4300i_COP1_dmfc1(unsigned __int32);
extern void		r4300i_COP1_dmtc1(unsigned __int32);
extern void		r4300i_COP1_mfc1(unsigned __int32);
extern void		r4300i_COP1_mtc1(unsigned __int32);
extern void		r4300i_COP1_add_s(unsigned __int32);
extern void		r4300i_COP1_add_d(unsigned __int32);
extern void		r4300i_COP1_sub_s(unsigned __int32);
extern void		r4300i_COP1_sub_d(unsigned __int32);
extern void		r4300i_COP1_mul_s(unsigned __int32);
extern void		r4300i_COP1_mul_d(unsigned __int32);
extern void		r4300i_COP1_div_s(unsigned __int32);
extern void		r4300i_COP1_div_d(unsigned __int32);
extern void		r4300i_COP1_sqrt_s(unsigned __int32);
extern void		r4300i_COP1_sqrt_d(unsigned __int32);
extern void		r4300i_COP1_abs_s(unsigned __int32);
extern void		r4300i_COP1_abs_d(unsigned __int32);
extern void		r4300i_COP1_mov_s(unsigned __int32);
extern void		r4300i_COP1_mov_d(unsigned __int32);
extern void		r4300i_COP1_neg_s(unsigned __int32);
extern void		r4300i_COP1_neg_d(unsigned __int32);
extern void		r4300i_COP1_roundl_s(unsigned __int32);
extern void		r4300i_COP1_roundl_d(unsigned __int32);
extern void		r4300i_COP1_truncl_s(unsigned __int32);
extern void		r4300i_COP1_truncl_d(unsigned __int32);
extern void		r4300i_COP1_ceill_s(unsigned __int32);
extern void		r4300i_COP1_ceill_d(unsigned __int32);
extern void		r4300i_COP1_floorl_s(unsigned __int32);
extern void		r4300i_COP1_floorl_d(unsigned __int32);
extern void		r4300i_COP1_roundw_s(unsigned __int32);
extern void		r4300i_COP1_roundw_d(unsigned __int32);
extern void		r4300i_COP1_truncw_s(unsigned __int32);
extern void		r4300i_COP1_truncw_d(unsigned __int32);
extern void		r4300i_COP1_ceilw_s(unsigned __int32);
extern void		r4300i_COP1_ceilw_d(unsigned __int32);
extern void		r4300i_COP1_floorw_s(unsigned __int32);
extern void		r4300i_COP1_floorw_d(unsigned __int32);
extern void		r4300i_COP1_cvts_d(unsigned __int32);
extern void		r4300i_COP1_cvts_w(unsigned __int32);
extern void		r4300i_COP1_cvts_l(unsigned __int32);
extern void		r4300i_COP1_cvtd_s(unsigned __int32);
extern void		r4300i_COP1_cvtd_w(unsigned __int32);
extern void		r4300i_COP1_cvtd_l(unsigned __int32);
extern void		r4300i_COP1_cvtw_s(unsigned __int32);
extern void		r4300i_COP1_cvtw_d(unsigned __int32);
extern void		r4300i_COP1_cvtw_l(unsigned __int32);
extern void		r4300i_COP1_cvtl_s(unsigned __int32);
extern void		r4300i_COP1_cvtl_d(unsigned __int32);
extern void		r4300i_C_F_S(unsigned __int32);
extern void		r4300i_C_F_D(unsigned __int32);
extern void		r4300i_C_UN_S(unsigned __int32);
extern void		r4300i_C_UN_D(unsigned __int32);
extern void		r4300i_C_EQ_S(unsigned __int32);
extern void		r4300i_C_EQ_D(unsigned __int32);
extern void		r4300i_C_UEQ_S(unsigned __int32);
extern void		r4300i_C_UEQ_D(unsigned __int32);
extern void		r4300i_C_OLT_S(unsigned __int32);
extern void		r4300i_C_OLT_D(unsigned __int32);
extern void		r4300i_C_ULT_S(unsigned __int32);
extern void		r4300i_C_ULT_D(unsigned __int32);
extern void		r4300i_C_OLE_S(unsigned __int32);
extern void		r4300i_C_OLE_D(unsigned __int32);
extern void		r4300i_C_ULE_S(unsigned __int32);
extern void		r4300i_C_ULE_D(unsigned __int32);
extern void		r4300i_C_SF_S(unsigned __int32);
extern void		r4300i_C_SF_D(unsigned __int32);
extern void		r4300i_C_NGLE_S(unsigned __int32);
extern void		r4300i_C_NGLE_D(unsigned __int32);
extern void		r4300i_C_SEQ_S(unsigned __int32);
extern void		r4300i_C_SEQ_D(unsigned __int32);
extern void		r4300i_C_NGL_S(unsigned __int32);
extern void		r4300i_C_NGL_D(unsigned __int32);
extern void		r4300i_C_LT_S(unsigned __int32);
extern void		r4300i_C_LT_D(unsigned __int32);
extern void		r4300i_C_NGE_S(unsigned __int32);
extern void		r4300i_C_NGE_D(unsigned __int32);
extern void		r4300i_C_LE_S(unsigned __int32);
extern void		r4300i_C_LE_D(unsigned __int32);
extern void		r4300i_C_NGT_S(unsigned __int32);
extern void		r4300i_C_NGT_D(unsigned __int32);
extern void		r4300i_COP1_bc1f(unsigned __int32);
extern void		r4300i_COP1_bc1t(unsigned __int32);
extern void		r4300i_COP1_bc1fl(unsigned __int32);
extern void		r4300i_COP1_bc1tl(unsigned __int32);
extern void		r4300i_COP0_tlbr(unsigned __int32);
extern void		r4300i_COP0_tlbwi(unsigned __int32);
extern void		r4300i_COP0_tlbwr(unsigned __int32);
extern void		r4300i_COP0_tlbp(unsigned __int32);
extern void		r4300i_COP0_eret(unsigned __int32);
extern void		r4300i_j(unsigned __int32);
extern void		r4300i_blez(unsigned __int32);
extern void		r4300i_bgtz(unsigned __int32);
extern void		r4300i_blezl(unsigned __int32);
extern void		r4300i_bgtzl(unsigned __int32);
extern void		r4300i_break(unsigned __int32);
extern void		r4300i_sync(unsigned __int32);
extern void		r4300i_syscall(unsigned __int32);

extern void		InitTLB(void);
extern void		Build_Whole_Direct_TLB_Lookup_Table(void);
uint32			Direct_TLB_Lookup(uint32 address, int operation);
uint32			TranslateITLBAddress(uint32 address);
uint32			TranslateTLBAddress(uint32 address, int operation);
uint32			TranslateTLBAddressForLoad(uint32 address);
uint32			TranslateTLBAddressForStore(uint32 address);

#define PCLOCKMULT				1
#define PCLOCKMULTU				1
#define PCLOCKDIV				37
#define PCLOCKDIVU				37
#define PCLOCKDMULT				1
#define PCLOCKDMULTU			1
#define PCLOCKDDIV				70
#define PCLOCKDDIVU				70

#define R4300I_SPEEDHACK		if((uint16) OFFSET_IMMEDIATE == (uint16) 0xFFFF) \
	{ \
		if(NOT_IN_KO_K1_SEG(gHWS_pc)) \
		{ \
			uint32	temppc = TranslateITLBAddress(gHWS_pc); \
			if(LOAD_UWORD_PARAM(temppc + 4) == 0) \
			{ \
				CPUdelayPC = gHWS_pc; \
				CPUdelay = 1; \
				r4300i_do_speedhack(); \
			} \
		} \
		else if(LOAD_UWORD_PARAM(gHWS_pc + 4) == 0) \
		{ \
			CPUdelayPC = gHWS_pc; \
			CPUdelay = 1; \
			r4300i_do_speedhack(); \
		} \
	} \
	else
#define K0_TO_K1(Addr)			((uint32) (Addr) | 0xA0000000)	/* K0_SEG to K1_SEG */
#define K1_TO_K0(Addr)			((uint32) (Addr) & 0x9FFFFFFF)	/* K1_SEG to K0_SEG */
#define K0_TO_PHYS(Addr)		((uint32) (Addr) & 0x1FFFFFFF)	/* K0_SEG to Physical */
#define K1_TO_PHYS(Addr)		((uint32) (Addr) & 0x1FFFFFFF)	/* K1_SEG to Physical */
#define ADDR_TO_PHYS(Addr)		((uint32) (Addr) & 0x1FFFFFFF)	/* direct mapped to Physical */
#define PHYS_TO_K0(Addr)		((uint32) (Addr) | 0x80000000)	/* Physical to K0_SEG */
#define PHYS_TO_K1(Addr)		((uint32) (Addr) | 0xA0000000)	/* Physical to K1_SEG */
#define IN_K0_SEG(Addr)			((Addr & 0xE0000000) == 0x80000000)
#define IN_K1_SEG(Addr)			((Addr & 0xE0000000) == 0xA0000000)
#define IN_KO_K1_SEG(Addr)		(!((Addr ^ 0x80000000) & 0xC0000000))
#define NOT_IN_KO_K1_SEG(Addr)	((Addr ^ 0x80000000) & 0xC0000000)
#ifdef ADDRESS_ERROR_EXCEPTION
#define TRIGGER_ADDR_ERROR_EXCEPTION(exception, address) \
	gHWS_COP0Reg[CAUSE] |= exception; \
gHWS_COP0Reg[BADVADDR] = address; \
HandleExceptions(0x80000180);
#else
#define TRIGGER_ADDR_ERROR_EXCEPTION(exception, address)
#endif
#ifdef ADDR_ALIGN_CHECKING

/*
 =======================================================================================================================
    define CHECKING_ADDR_ALIGNMENT(addr, mask, opcode, exception) if( addr & mask )
    {Trigger_Address_Error_Exception(addr, opcode, exception);return;;}
 =======================================================================================================================
 */
#define CHECKING_ADDR_ALIGNMENT(addr, mask, opcode, exception) \
	if(addr & mask) \
	{ \
		Trigger_Address_Error_Exception(addr, opcode, exception); \
	}
#else
#define CHECKING_ADDR_ALIGNMENT(addr, mask, opcode, exception)
#endif
extern BOOL		FPU_Is_Enabled;
extern BOOL		ITLB_Error;
extern uint32	TLB_Error_Vector;

extern void (*CPU_instruction[64]) (uint32 Instruction);
extern void COP1_instr(uint32 Instruction);
extern void ClearCPUTasks(void);
void		COP1_NotAvailable_instr(uint32 Instruction);

#define DUMMYOPCODE 0xEEEEEEEE	/* this is an invalid opcode */
#endif
