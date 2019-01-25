/*$T x86.h GC 1.136 02/28/02 08:36:56 */


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
#ifndef __X86_H__
#define __X86_H__

#define CODETABLE_SIZE		1024 * 1024 * 8
#define RECOMPCODE_SIZE		1024 * 1024 * 16
//#define RECOMPCODE_SIZE		65536*16 //tests the recompile buffer by smaller size

#define Reg_EAX				0x0
#define Reg_AX				Reg_EAX
#define Reg_AL				Reg_EAX
#define Reg_ECX				0x1
#define Reg_CX				Reg_ECX
#define Reg_CL				Reg_ECX
#define Reg_EDX				0x2
#define Reg_DX				Reg_EDX
#define Reg_DL				Reg_EDX
#define Reg_EBX				0x3
#define Reg_BX				Reg_EBX
#define Reg_BL				Reg_EBX
#define Reg_ESP				0x4
#define Reg_SP				Reg_ESP
#define Reg_AH				Reg_ESP
#define Reg_EBP				0x5
#define Reg_BP				Reg_EBP
#define Reg_CH				Reg_EBP
#define Reg_ESI				0x6
#define Reg_SI				Reg_ESI
#define Reg_DH				Reg_ESI
#define Reg_EDI				0x7
#define Reg_DI				Reg_EDI
#define Reg_BH				Reg_EDI

#define ModRM_EAX			0x0
#define ModRM_EBX			0x3
#define ModRM_ECX			0x1
#define ModRM_EDX			0x2
#define ModRM_SIB			0x4
#define ModRM_disp32		0x5
#define ModRM_EBP			0x5
#define ModRM_ESI			0x6
#define ModRM_EDI			0x7

#define ModRM_disp8_EAX		0x40
#define ModRM_disp8_EBX		0x43
#define ModRM_disp8_ECX		0x41
#define ModRM_disp8_EDX		0x42
#define ModRM_disp8_SIB		0x44
#define ModRM_disp8_EBP		0x45
#define ModRM_disp8_ESI		0x46
#define ModRM_disp8_EDI		0x47

#define ModRM_disp32_EAX	0x80
#define ModRM_disp32_EBX	0x83
#define ModRM_disp32_ECX	0x81
#define ModRM_disp32_EDX	0x82
#define ModRM_disp32_SIB	0x84
#define ModRM_disp32_EBP	0x85
#define ModRM_disp32_ESI	0x86
#define ModRM_disp32_EDI	0x87

/* Condition codes */
#define CC_O			0x0
#define CC_NO			0x1
#define CC_B			0x2
#define CC_NAE			CC_B
#define CC_NB			0x3
#define CC_NC			0x3
#define CC_AE			CC_NB
#define CC_E			0x4
#define CC_Z			CC_E
#define CC_NE			0x5
#define CC_NZ			CC_NE
#define CC_BE			0x6
#define CC_NA			CC_BE
#define CC_NBE			0x7
#define CC_A			CC_NBE
#define CC_S			0x8
#define CC_NS			0x9
#define CC_P			0xA
#define CC_PE			CC_P
#define CC_NP			0xB
#define CC_PO			CC_NP
#define CC_L			0xC
#define CC_NGE			CC_L
#define CC_NL			0xD
#define CC_GE			CC_NL
#define CC_LE			0xE
#define CC_NG			CC_LE
#define CC_NLE			0xF
#define CC_G			CC_NLE

#define FORMAT_SINGLE	0
#define FORMAT_QUAD		4

extern void				SetTranslator(unsigned char *Code, unsigned long Pos, unsigned long Size);
extern void				SetTarget(unsigned char bIndex);

/* extern void WC8(unsigned char bValue); */
extern void				WC16(unsigned int wValue);
extern void				WC32(unsigned long dwValue);
extern void				WC8(unsigned char bValue);

extern void				ADD_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);
extern void				ADD_ImmToReg(unsigned char OperandSize, unsigned char Reg, unsigned long Data);
extern void				ADD_ImmToMemory(unsigned long Address, unsigned long Data);
extern void				AND_Imm8sxToReg(unsigned char OperandSize, unsigned char Reg, unsigned char Data);
extern void				AND_Imm8sxToMemory(unsigned char Data, unsigned long Address);
extern void				ADD_Imm8sxToReg(unsigned char OperandSize, unsigned char Reg, unsigned char Data);
extern void				AND_ImmToReg(unsigned char OperandSize, unsigned char Reg, unsigned long Data);
extern void				AND_ImmToEAX(unsigned char OperandSize, unsigned long Data);
extern void				AND_ImmToMemory(unsigned long Address, unsigned long Data);
extern void				AND_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);
extern void				OR_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);

extern void				BSWAP(unsigned char Reg);
extern void				BREAKPOINT(void);
extern void				X86_CALL(unsigned long dwAddress);
extern void				CALL_Reg(unsigned char Reg);
extern void				CDQ(void);
extern void				CMP_Reg1WithReg2(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);
extern void				CMP_Reg2WithReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);
extern void				CMP_EAXWithImm(unsigned char OperandSize, unsigned long Data);
extern void				CMP_RegWithImm(unsigned char OperandSize, unsigned char Reg, unsigned long Data);
extern void				CMP_RegWithShort(unsigned char OperandSize, unsigned char Reg, unsigned char Data);
extern void				CMP_MemoryWithImm(unsigned char OperandSize, unsigned long dwAddress, unsigned long Data);
extern void				CMP_RegWithMemory(unsigned char Reg, unsigned long dwAddress);
extern void				CMP_sImm8WithReg(unsigned char OperandSize, unsigned char Reg, unsigned char Data);
extern void				DEC_Reg(unsigned char OperandSize, unsigned char Reg);
extern void				DIV_EAXWithReg(unsigned char OperandSize, unsigned char Reg);
extern void				FABS(void);
extern void				FADD_Memory(unsigned char OperandSize, unsigned long dwAddress);
extern void				FCOMP(unsigned char OperandSize, unsigned long dwAddress);
extern void				FNSTSW(void);
extern void				FDIV_Memory(unsigned char OperandSize, unsigned long dwAddress);
extern void				FILD_Memory(unsigned char OperandSize, unsigned long dwAddress);
extern void				FISTP_Memory(unsigned char OperandSize, unsigned long dwAddress);
extern void				FLD_Memory(unsigned char OperandSize, unsigned long dwAddress);
extern void				FLDCW_Memory(unsigned long dwAddress);
extern void				FMUL_Memory(unsigned char OperandSize, unsigned long dwAddress);
extern void				FNEG(void);
extern void				FRNDINT(void);
extern void				FSQRT(void);
extern void				FSTP_Memory(unsigned char OperandSize, unsigned long dwAddress);
extern void				FSUB_Memory(unsigned char OperandSize, unsigned long dwAddress);

extern void				IMUL_EAXWithReg(unsigned char OperandSize, unsigned char Reg);
extern void				IMUL_EAXWithMemory(unsigned char OperandSize, unsigned long Address);
extern void				IMUL_Reg2ToReg1(unsigned char Reg1, unsigned char Reg2);
extern void				IDIV_EAXWithReg(unsigned char OperandSize, unsigned char Reg);

extern void				INC_Reg(unsigned char OperandSize, unsigned char Reg);

extern void				Jcc(unsigned char ConditionCode, unsigned char Offset);
extern void				Jcc_auto(unsigned char ConditionCode, unsigned long Index);
extern void				Jcc_Near_auto(unsigned char ConditionCode, unsigned long Index);
extern void				JMP_Short(unsigned char Offset);
extern void				JMP_Short_auto(unsigned long Index);
extern void				JMP_FAR(unsigned long dwAddress);
extern void				JMP_Long(unsigned long dwAddress);
extern void				LEA(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address);
extern void				MOV_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);
extern void				MOV_MemoryToReg
						(
							unsigned char	OperandSize,
							unsigned char	Reg,
							unsigned char	ModRM,
							unsigned long	Address
						);
extern void				MOV_ModRMToReg(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM);
extern void				MOV_RegToModRM(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM);
extern void				MOV_MemoryToEAX(unsigned char OperandSize, unsigned long Address);
extern void				MOV_RegToMemory
						(
							unsigned char	OperandSize,
							unsigned char	Reg,
							unsigned char	ModRM,
							unsigned long	Address
						);
extern void				MOV_EAXToMemory(unsigned char OperandSize, unsigned long Address);
extern void				MOV_ImmToReg(unsigned char OperandSize, unsigned char Reg, unsigned long Data);
extern void				MOV_ImmToMemory
						(
							unsigned char	OperandSize,
							unsigned char	ModRM,
							unsigned long	Address,
							unsigned long	Data
						);
extern void				MOVSX_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);
extern void				MOVZX_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);
extern void				MOVSX_MemoryToReg
						(
							unsigned char	OperandSize,
							unsigned char	Reg,
							unsigned char	ModRM,
							unsigned long	Address
						);
extern void				MOVZX_MemoryToReg
						(
							unsigned char	OperandSize,
							unsigned char	Reg,
							unsigned char	ModRM,
							unsigned long	Address
						);
extern void				MUL_EAXWithReg(unsigned char OperandSize, unsigned char Reg);
extern void				MUL_EAXWithMemory(unsigned char OperandSize, unsigned long Address);
extern void				NOT_Reg(unsigned char OperandSize, unsigned char Reg);
extern void				NEG_Reg(unsigned char OperandSize, unsigned char Reg);
extern void				NOP(void);
extern void				OR_ImmToEAX(unsigned char OperandSize, unsigned long Data);
extern void				OR_ImmToReg(unsigned char OperandSize, unsigned char Reg, unsigned long Data);
extern void				OR_ShortToReg(unsigned char OperandSize, unsigned char Reg, unsigned char Data);
extern void				OR_RegToMemory
						(
							unsigned char	OperandSize,
							unsigned char	Reg,
							unsigned char	ModRM,
							unsigned long	Address
						);
extern void				POP_RegFromStack(unsigned char Reg);
extern void				PUSH_RegToStack(unsigned char Reg);
extern void				PUSH_WordToStack(unsigned __int32 wWord);
extern void				PUSHA(void);
extern void				PUSHAD(void);
extern void				POPA(void);
extern void				POPAD(void);
extern void				PushMap(void);
extern void				PopMap(void);
extern void				RET(void);
extern void				ROL_RegByImm(unsigned char OperandSize, unsigned char Reg, unsigned char Data);
extern void				SETcc_Reg(unsigned char ConditionCode, unsigned char Reg);
extern void				SAHF(void);
extern void				SAR_RegByImm(unsigned char OperandSize, unsigned char Reg, unsigned char Data);
extern void				SAR_RegByCL(unsigned char OperandSize, unsigned char Reg);
extern void				SHL_RegByImm(unsigned char OperandSize, unsigned char Reg, unsigned char Data);
extern void				SHL_RegBy1(unsigned char OperandSize, unsigned char Reg);
extern void				SHR_RegBy1(unsigned char OperandSize, unsigned char Reg);
extern void				SHL_RegByCL(unsigned char OperandSize, unsigned char Reg);
extern void				SAR_RegBy1(unsigned char OperandSize, unsigned char Reg);
extern void				SHR_RegByImm(unsigned char OperandSize, unsigned char Reg, unsigned char Data);
extern void				SHR_RegByCL(unsigned char OperandSize, unsigned char Reg);
extern void				SUB_Reg1OfReg2(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);
extern void				SUB_ImmToMemory(unsigned long Address, unsigned long Data);
extern void				TEST_ImmWithMemory(unsigned long dwAddress, unsigned long Value);
extern void				TEST_EAXWithImm(unsigned char OperandSize, unsigned long dwAddress);
extern void				TEST_EAXWithEAX(void);
extern void				TEST_Reg2WithReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);
extern void				TEST_RegWithImm(unsigned char OperandSize, unsigned char iReg, unsigned long dwAddress);
extern void				XCHG_Reg1WithReg2(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);
extern void				XOR_ImmToEAX(unsigned char OperandSize, unsigned long Data);
extern void				XOR_ImmToReg(unsigned char OperandSize, unsigned char Reg, unsigned long Data);
extern void				XOR_ShortToReg(unsigned char OperandSize, unsigned char Reg, unsigned char Data);
extern void				XOR_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2);

extern void				SetNearTarget(unsigned char bIndex);

/* Encoders */
extern void				Encode_Slash_R(unsigned char Reg, unsigned char ModRM, unsigned long Address);

/* load and execute instructions. Be careful using these with multipass! */
extern void				ADD_MemoryToReg
						(
							unsigned char	OperandSize,
							unsigned char	Reg,
							unsigned char	ModRM,
							unsigned long	Address
						);
extern void				AND_MemoryToReg
						(
							unsigned char	OperandSize,
							unsigned char	Reg,
							unsigned char	ModRM,
							unsigned long	Address
						);
extern void				SUB_MemoryToReg
						(
							unsigned char	OperandSize,
							unsigned char	Reg,
							unsigned char	ModRM,
							unsigned long	Address
						);
extern void				OR_MemoryToReg
						(
							unsigned char	OperandSize,
							unsigned char	Reg,
							unsigned char	ModRM,
							unsigned long	Address
						);
extern void				XOR_MemoryToReg
						(
							unsigned char	OperandSize,
							unsigned char	Reg,
							unsigned char	ModRM,
							unsigned long	Address
						);

/*
 =======================================================================================================================
    Schibo's new macros and functions //
 =======================================================================================================================
 */
#define CDQ_STORE(TO_MEM) \
	/*
	 * 0040A5FD 99 cdq £
	 * 0040A5FE BF 78 56 34 12 mov edi,12345678h £
	 * 0040A603 89 07 mov dword ptr [edi],eax £
	 * 0040A605 89 57 04 mov dword ptr [edi+4],edx
	 */ \
	WC16(0xBF99); \
	WC32(TO_MEM); \
	WC32(0x57890789); \
	WC8(0x04);
#endif
