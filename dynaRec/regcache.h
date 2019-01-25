/*$T regcache.h GC 1.136 03/09/02 16:50:59 */


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
#ifndef _REGCACHE_H__1964_
#define _REGCACHE_H__1964_

/* The following are macros that help us debug the dynarec */

#ifdef _DEBUG
#define CRASHABLE 1 //so i can go to MSVC disassembler at crash. Disable for release. */
#endif

//#define NO_CONSTS 1
//#define SAFE_SLT 1
//#define SAFE_GATES 1
//#define SAFE_MATH 1
//#define SAFE_IMM 1
//#define SAFE_LOADSTORE 1
//#define SAFE_LOADSTORE_FPU 1
//#define SAFE_SHIFTS 1
//#define SAFE_PUSHPOP 1
//#define SAFE_DOUBLE_SHIFTS 1
//#define SAFE_DOUBLE_MATH 1
//#define SAFE_DOUBLE_IMM 1

/*
 * This is probably ugly as sin, but a very easy way to debug £
 * the integrity of our opcode debugger. Let's keep this PLEASE. £
 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY1 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY2 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY3 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY4 1 */
#define TEST_OPCODE_DEBUGGER_INTEGRITY5 1

/* /*$ define TEST_OPCODE_DEBUGGER_INTEGRITY6 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY7 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY8 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY9 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY10 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY11 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY12 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY13 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY14 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY15 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY16 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY17 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY18 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY19 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY20 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY21 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY22 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY23 1 */

/*$ #define TEST_OPCODE_DEBUGGER_INTEGRITY24 1 */
#define NUM_CONSTS	34

extern unsigned char	*RecompCode;

/*
 =======================================================================================================================
    Don't use these registers for mapping. 100 is a protected reg.
 =======================================================================================================================
 */
#define ItIsARegisterNotToUse(k)	((k == Reg_EBP) || (k == Reg_ESP) || (x86reg[k].mips_reg == 100))
#define LoadGPR_LO(k) \
	{ \
		if(x86reg[k].mips_reg == 0) \
			XOR_Reg2ToReg1(1, (_u8) k, (_u8) k); \
		else \
		{ \
			FetchEBP_Params(x86reg[k].mips_reg); \
			MOV_MemoryToReg(1, (uint8) k, x86params.ModRM, x86params.Address); \
		} \
	}

#define StoreGPR_LO(k) \
	{ \
		if(x86reg[k].mips_reg == 0); \
		else \
		{ \
			FetchEBP_Params(x86reg[k].mips_reg); \
			MOV_RegToMemory(1, k, x86params.ModRM, x86params.Address); \
		} \
	}

#define LoadGPR_HI(k) \
	if(currentromoptions.Assume_32bit == ASSUME_32BIT_NO) \
	{ \
		if(x86reg[k].mips_reg == 0) \
			XOR_Reg2ToReg1(1, (_u8) x86reg[k].HiWordLoc, (_u8) x86reg[k].HiWordLoc); \
		else \
		{ \
			FetchEBP_Params(x86reg[k].mips_reg); \
			MOV_MemoryToReg(1, (uint8) x86reg[k].HiWordLoc, x86params.ModRM, 4 + x86params.Address); \
		} \
	} \
	else \
	{ \
		MOV_Reg2ToReg1(1, x86reg[k].HiWordLoc, (uint8) k); \
		SAR_RegByImm(1, x86reg[k].HiWordLoc, 31); \
	}

#define StoreGPR_HI(k) \
	{ \
		if(x86reg[k].mips_reg == 0); \
		else \
		{ \
			FetchEBP_Params(x86reg[k].mips_reg); \
			MOV_RegToMemory(1, (uint8) x86reg[k].HiWordLoc, x86params.ModRM, 4 + x86params.Address); \
		} \
	}

#define StoreImm_LO(k) \
	{ \
		if(k < 16) \
		{ \
			MOV_ImmToMemory(1, ModRM_disp8_EBP, (-128 + (k << 3)), i); \
		} \
		else if(k < 32) \
		{ \
			MOV_ImmToMemory(1, ModRM_disp8_EBP, ((k - 16) << 3), i); \
		} \
		else \
			MOV_ImmToMemory(1, ModRM_disp32_EBP, ((k - 16) << 3), i); \
	}

#define StoreImm_HI(k) \
	if(currentromoptions.Assume_32bit == ASSUME_32BIT_NO) \
	{ \
		if(k < 16) \
		{ \
			MOV_ImmToMemory(1, ModRM_disp8_EBP, (-124 + (k << 3)), i); \
		} \
		else if(k < 32) \
		{ \
			MOV_ImmToMemory(1, ModRM_disp8_EBP, 4 + ((k - 16) << 3), i); \
		} \
		else \
		{ \
			MOV_ImmToMemory(1, ModRM_disp32_EBP, 4 + ((k - 16) << 3), i); \
		} \
	}

#define LoadFPR_LO(k) \
	{ \
		if(k < 16) \
		{ \
			FLD(0, ModRM_disp8_EBP, (-64 + (k << 2))); \
		} \
		else if(k < 32) \
		{ \
			FLD(0, ModRM_disp8_EBP, ((k - 16) << 2)); \
		} \
	}

#define StoreFPR_LO(k) \
	{ \
		if(k < 16) \
		{ \
			FSTP(0, ModRM_disp8_EBP, (-64 + (k << 2))); \
		} \
		else if(k < 32) \
		{ \
			FSTP(0, ModRM_disp8_EBP, ((k - 16) << 2)); \
		}\
	}

typedef struct	x86regtyp
{
	uint32	BirthDate;
	_int8	HiWordLoc;
	_int8	Is32bit;
	_int8	IsDirty;
	_int8	mips_reg;
	_int8	NoNeedToLoadTheLo;
	_int8	NoNeedToLoadTheHi;
	_int8	x86reg;
} x86regtyp;

/*
 -----------------------------------------------------------------------------------------------------------------------
    Keeps status of constants
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct	MapConstant
{
	_int32	value;
	_int32	IsMapped;
	uint32	FinalAddressUsedAt; /* Analysis: at what PC the last Mips reg (rd,rs, or rt) is */
} MapConstant;

typedef struct	x86paramstyp
{
	unsigned __int32	Address;
	unsigned char		ModRM;
} x86paramstyp;
x86paramstyp	x86params;

/*
 -----------------------------------------------------------------------------------------------------------------------
    Keeps status of registers stored to memory
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct	FlushedMap
{
	uint32	Is32bit;
} FlushedMap;

uint32		ThisYear;
MapConstant ConstMap[NUM_CONSTS];

/*
 * Multi-Pass definitions £
 */
#define CHECK_OPCODE_PASS	if(gMultiPass.WhichPass == COMPILE_MAP_ONLY) \
	{ \
		SwitchToOpcodePass(); \
		return; \
	} \
	{ \
		gMultiPass.WhichPass = COMPILE_ALL; \
		gMultiPass.WriteCode = 1; \
	}

enum PASSTYPE { COMPILE_MAP_ONLY, COMPILE_OPCODES_ONLY, COMPILE_ALL };

typedef struct MultiPass
{
	int UseOnePassOnly;		/* Make this a rom option. default option will be yes (1). */
	int WhichPass;			/* Mapping pass or opcode pass */
	int WriteCode;			/* Whether or not we write code on this pass */
	int PhysAddrAfterMap;	/* Phyical Start address of 1st instruction after the block's first mappings. */
	int lCodePositionAfterMap;
	int VirtAddrAfterMap;	/* Virtual "" */
	int JumpToPhysAddr;
	int pc;
} MultiPass;
MultiPass	gMultiPass;

/*
 * Mnemonics £
 */
#define __OPCODE	((_u8) (reg->code >> 26))
#define __RS		(((_u8) (reg->code >> 21)) & 0x1f)
#define __RT		(((_u8) (reg->code >> 16)) & 0x1f)
#define __RD		(((_u8) (reg->code >> 11)) & 0x1f)
#define __SA		(((_u8) (reg->code >> 6)) & 0x1f)
#define __F			((_u8) (reg->code) & 0x3f)

/* define __I ( (_s32)(_s16)reg->code ) */
#define __I			((_s32) (_s16) (reg->code & 0xFFFF))
#define __O			(reg->pc + 4 + (__I << 2))
#define ____T		(reg->code & 0x3ffffff)
#define __T			((reg->pc & 0xf0000000) | (____T << 2))
#define __FS		__RD
#define __FT		__RT
#define __FD		__SA

#define __dotRS		(((_u8) (gHWS_code >> 21)) & 0x1f)
#define __dotRT		(((_u8) (gHWS_code >> 16)) & 0x1f)
#define __dotRD		(((_u8) (gHWS_code >> 11)) & 0x1f)
#define __dotSA		(((_u8) (gHWS_code >> 6)) & 0x1f)
#define __dotI		((_s32) (_s16) gHWS_code)
#define __dotO		(gHWS_pc + 4 + (__dotI << 2))
#define ____dotT	(gHWS_code & 0x3ffffff)
#define __dotT		((gHWS_pc & 0xf0000000) | (____dotT << 2))
#define __dotF		((_u8) (gHWS_code) & 0x3f)
#define __ND		((uint8) ((reg->code >> 17) & 0x1))
#define __TF		((uint8) ((reg->code >> 16) & 0x1))

/*
 * Function Declarations £
 */
extern int	CheckIs32Bit(int mips_reg);
extern void FlushAllRegisters(void);
extern void FlushOneButNotThese3(int The1stOneNotToFlush, int The2ndOneNotToFlush, int The3rdOneNotToFlush);
extern void MapRegister(x86regtyp *Conditions, int The2ndOneNotToFlush, int The3rdOneNotToFlush);
extern int	CheckRegStatus(x86regtyp *Query);
extern void InitRegisterMap(void);
extern void FlushRegister(int k);
extern void WriteBackDirty(_int8 k);
extern void FlushOneConstant(int k);
extern void MapOneConstantToRegister(x86regtyp *Conditions, int The2ndOneNotToFlush, int The3rdOneNotToFlush);
extern int	CheckWhereIsMipsReg(int mips_reg);
void		PUSH_RegIfMapped(int k);
void		POP_RegIfMapped(int k);
void		FetchEBP_Params(int mips_reg);
void		FreeMipsRegister(int mips_reg);
void		Calculate_ModRM_Byte_Via_Slash_Digit(int SlashDigit);

#define MapRD	MapRegister(xRD, xRS->mips_reg, xRT->mips_reg);
#define MapRS	MapRegister(xRS, xRD->mips_reg, xRT->mips_reg);
#define MapRT	MapRegister(xRT, xRD->mips_reg, xRS->mips_reg);

/*
 =======================================================================================================================
    Before you can use these MapXX_To() macros, it is critical £
    that you first test if XX as a constant and handle that £
    const condition first. £
    These macros take advantage of the Advanced Analysis by utilizing load-and-execute £
    instructions when feasible.
 =======================================================================================================================
 */
#define MapRS_To(xDst, Use32bit, LoadAndExecute_Function) \
	{ \
		if(((CheckWhereIsMipsReg(xRS->mips_reg)) == -1) && (ConstMap[xRS->mips_reg].FinalAddressUsedAt <= gHWS_pc)) \
		{ \
			FetchEBP_Params(xRS->mips_reg); \
 \
			/* memory to register */ \
			LoadAndExecute_Function(1, xDst->x86reg, x86params.ModRM, x86params.Address); \
			if(!Use32bit) LoadAndExecute_Function(1, xDst->HiWordLoc, x86params.ModRM, 4 + x86params.Address); \
		} \
		else \
		{ \
			MapRS \
 \
			/* register to register */ \
			LoadAndExecute_Function(1, xDst->x86reg, (uint8) (0xC0 | xRS->x86reg), 0); \
			if(!Use32bit) LoadAndExecute_Function(1, xDst->HiWordLoc, (uint8) (0xC0 | xRS->HiWordLoc), 0); \
		} \
	}

#define MapRT_To(xDst, Use32bit, LoadAndExecute_Function) \
	{ \
		if(((CheckWhereIsMipsReg(xRT->mips_reg)) == -1) && (ConstMap[xRT->mips_reg].FinalAddressUsedAt <= gHWS_pc)) \
		{ \
			FetchEBP_Params(xRT->mips_reg); \
 \
			/* memory to register */ \
			LoadAndExecute_Function(1, xDst->x86reg, x86params.ModRM, x86params.Address); \
			if(!Use32bit) LoadAndExecute_Function(1, xDst->HiWordLoc, x86params.ModRM, 4 + x86params.Address); \
		} \
		else \
		{ \
			MapRT \
 \
			/* register to register */ \
			LoadAndExecute_Function(1, xDst->x86reg, (uint8) (0xC0 | xRT->x86reg), 0); \
			if(!Use32bit) LoadAndExecute_Function(1, xDst->HiWordLoc, (uint8) (0xC0 | xRT->HiWordLoc), 0); \
		} \
	}

#define NEGATE(REG)			{ XOR_ShortToReg(1, (_u8) REG->x86reg, 0xFF); INC_Reg(1, (_u8) REG->x86reg); }

#define SetLoHiRsRt32bit	memset(xLO, 0, sizeof(xLO)); \
	memset(xHI, 0, sizeof(xHI)); \
	memset(xRS, 0, sizeof(xRS)); \
	memset(xRT, 0, sizeof(xRT)); \
	xLO->mips_reg = __LO; \
	xRT->mips_reg = __RT; \
	xRS->mips_reg = __RS; \
	xHI->mips_reg = __HI; \
	xLO->Is32bit = 1; \
	xRT->Is32bit = 1; \
	xRS->Is32bit = 1; \
	xHI->Is32bit = 1;

#define SetRsRt64bit		memset(xRD, 0, sizeof(xRD)); \
	memset(xRS, 0, sizeof(xRS)); \
	memset(xRT, 0, sizeof(xRT)); \
	xRD->mips_reg = __RD; \
	xRT->mips_reg = __RT; \
	xRS->mips_reg = __RS;
#endif /* REGCACHE_H__1964_ */
