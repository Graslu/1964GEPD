/*$T x86.c GC 1.136 02/28/02 08:36:03 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 In this file, the calls to x86 assembly functions are converted to machine code.
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
#include "../globals.h"
#include "../debug_option.h"
#include "../compiler.h"
#include "x86.h"
#include "dynaLog.h"
#include "regcache.h"

extern void __cdecl MBox(char *debug, ...);
extern void __cdecl DisplayError(char *Message, ...);

unsigned long		JumpTargets[100];
unsigned char		*RecompCode;
unsigned long		lCodeSize;

char				*RegNames[] = { "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI" };

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void SetTranslator(unsigned char *Code, unsigned long Pos, unsigned long Size)
{
	RecompCode = Code;
	compilerstatus.lCodePosition = Pos;
	lCodeSize = Size;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SetTarget(unsigned char bIndex)
{
	/*~~~~~~~~~~~~~~*/
	char	bPosition;
	/*~~~~~~~~~~~~~~*/

	LOGGING_DYNA(LogDyna("j_point %i:\n", bIndex);) bPosition = (char)
		((compilerstatus.lCodePosition - JumpTargets[bIndex]) & 0xFF);
	RecompCode[JumpTargets[bIndex] - 1] = bPosition;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DynaBufferOverrun(void)
{
	compilerstatus.DynaBufferOverError = TRUE;
	compilerstatus.lCodePosition  = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WC8(unsigned char bValue)
{
	if(compilerstatus.DynaBufferOverError) return;

	if(gMultiPass.WriteCode)
	{
		if((compilerstatus.lCodePosition + 10) > lCodeSize) 
		{
			DynaBufferOverrun();
		}
		else
		{
			RecompCode[compilerstatus.lCodePosition] = bValue;
			compilerstatus.lCodePosition++;
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WC16(unsigned int wValue)
{
	if(compilerstatus.DynaBufferOverError) return;

	if(gMultiPass.WriteCode)
	{
		if((compilerstatus.lCodePosition + 10) > lCodeSize) 
		{
			DynaBufferOverrun();
		}
		else
		{
			(*((unsigned _int16 *) (&RecompCode[compilerstatus.lCodePosition])) = (unsigned _int16) (wValue));
			compilerstatus.lCodePosition += 2;
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WC32(unsigned long dwValue)
{
	if(compilerstatus.DynaBufferOverError) return;

	if(gMultiPass.WriteCode)
	{
		if((compilerstatus.lCodePosition + 10) > lCodeSize) 
		{
			DynaBufferOverrun();
		}
		else
		{
			(*((unsigned _int32 *) (&RecompCode[compilerstatus.lCodePosition])) = (unsigned _int32) (dwValue));
			compilerstatus.lCodePosition += 4;
		}
	}
}

/*
 =======================================================================================================================
    Now some X86 Opcodes ...
 =======================================================================================================================
 */

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ADD_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	WC8((unsigned char) (0x02 | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));
	LOGGING_DYNA(LogDyna("	ADD %s, %s\n", RegNames[Reg1], RegNames[Reg2]););
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ADD_MemoryToReg(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8((unsigned char) (0x02 | OperandSize));
	Encode_Slash_R(Reg, ModRM, Address);

	/* LOGGING_DYNA(LogDyna(" ADD %s, %s\n", RegNames[Reg1], RegNames[Reg2]);); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AND_MemoryToReg(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8((unsigned char) (0x22 | OperandSize));
	Encode_Slash_R(Reg, ModRM, Address);

	/* LOGGING_DYNA(LogDyna(" ADD %s, %s\n", RegNames[Reg1], RegNames[Reg2]);); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SUB_MemoryToReg(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8((unsigned char) (0x2A | OperandSize));
	Encode_Slash_R(Reg, ModRM, Address);

	/* LOGGING_DYNA(LogDyna(" ADD %s, %s\n", RegNames[Reg1], RegNames[Reg2]);); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void OR_MemoryToReg(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8((unsigned char) (0x0A | OperandSize));
	Encode_Slash_R(Reg, ModRM, Address);

	/* LOGGING_DYNA(LogDyna(" ADD %s, %s\n", RegNames[Reg1], RegNames[Reg2]);); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void XOR_MemoryToReg(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8((unsigned char) (0x32 | OperandSize));
	Encode_Slash_R(Reg, ModRM, Address);

	/* LOGGING_DYNA(LogDyna(" ADD %s, %s\n", RegNames[Reg1], RegNames[Reg2]);); */
}

/*
 =======================================================================================================================
    Danger: if Operand size == 0!. Fix! (Fortunately we never use 0 yet)
 =======================================================================================================================
 */
void ADD_ImmToReg(unsigned char OperandSize, unsigned char Reg, unsigned long Data)
{
	if(Data == 0) return;
	if(OperandSize == 1)
	{
		if(Data == 0xffffffff)
		{
			DEC_Reg(1, Reg);
			return;
		}

		if(Data == 1)
		{
			INC_Reg(1, Reg);
			return;
		}

		if((Data < 128) || (Data >= (0xffffff80)))	/* [-128 to 127] */
		{
			ADD_Imm8sxToReg(1, Reg, (unsigned char) Data);
			return;
		}
	}

	if(Reg == Reg_EAX)
	{
		WC8((unsigned char) (0x04 | OperandSize));	/* ADD_ImmToEAX */
	}
	else
	{
		WC8((unsigned char) (0x80 | OperandSize));
		WC8((unsigned char) (0xC0 | Reg));
	}

	if(OperandSize == 0)
		WC8((unsigned char) Data);
	else
		WC32(Data);

	LOGGING_DYNA(LogDyna("	ADD %s, 0x%08X\n", RegNames[Reg], Data););
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ADD_ImmToMemory(unsigned long Address, unsigned long Data)
{
	WC8((unsigned char) 0x81);
	WC8((unsigned char) 0x05);
	WC32(Address);
	WC32((unsigned long) (Data));

	LOGGING_DYNA(LogDyna(" ADD [0x%08X], 0x%08x\n", Address, Data););
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AND_ImmToEAX(unsigned char OperandSize, unsigned long Data)
{
	WC8((unsigned char) (0x24 | OperandSize));
	if(OperandSize == 0)
		WC8((unsigned char) Data);
	else
		WC32(Data);

	LOGGING_DYNA(LogDyna("	AND EAX, 0x%08X\n", Data);)
}

/*
 =======================================================================================================================
    Be careful with operandsize = 0!. there is no esl, edl
 =======================================================================================================================
 */
void AND_ImmToReg(unsigned char OperandSize, unsigned char Reg, unsigned long Data)
{
	if(Data == 0xffffffff) return;
	if(OperandSize == 1)
	{
		if(Data == 0)
		{
			XOR_Reg2ToReg1(1, Reg, Reg);
			return;
		}

		if((Data < 128) || (Data >= (0xffffff80)))	/* [-128 to 127] */
		{
			AND_Imm8sxToReg(OperandSize, Reg, (unsigned char) Data);
			return;
		}
	}

	if(Reg == Reg_EAX)
	{
		AND_ImmToEAX(OperandSize, Data);
		return;
	}

	WC8((unsigned char) (0x80 | OperandSize));
	WC8((unsigned char) (0xE0 | Reg));
	if(OperandSize != 0)
		WC32(Data);
	else
		WC8((unsigned char) (Data & 0xFF));

	LOGGING_DYNA(LogDyna("	AND %s, 0x%08X\n", RegNames[Reg], Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AND_ImmToMemory(unsigned long Address, unsigned long Data)
{
	if(Data == 0xffffffff) return;
	WC8((unsigned char) 0x81);
	WC8((unsigned char) 0x25);
	WC32(Address);
	WC32(Data);

	LOGGING_DYNA(LogDyna("	AND [0x%08X], 0x%08x\n", Address, Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void BSWAP(unsigned char Reg)
{
	WC8((unsigned char) 0x0F);
	WC8((unsigned char) (0xC8 | Reg));

	LOGGING_DYNA(LogDyna("	BSWAP %s\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void X86_CALL(unsigned long dwAddress)
{
	/*~~~~~~~~~~~~~~~~~~~~~~*/
	unsigned __int32	wTemp;
	/*~~~~~~~~~~~~~~~~~~~~~~*/

	wTemp = dwAddress - (unsigned __int32) (char *) &RecompCode[compilerstatus.lCodePosition] - 5;
	WC8(0xe8);
	WC32(wTemp);

	LOGGING_DYNA(LogDyna("	CALL 0x%08X\n", dwAddress);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CALL_Reg(unsigned char Reg)
{
	WC8((unsigned char) 0xFF);
	WC8((unsigned char) (0xD0 | Reg));

	LOGGING_DYNA(LogDyna("	CALL %s\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CDQ(void)
{
	WC8((unsigned char) 0x99);

	LOGGING_DYNA(LogDyna("	CDQ\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMP_Reg1WithReg2(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	WC8((unsigned char) (0x38 | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));

	LOGGING_DYNA(LogDyna("	CMP %s, %s\n", RegNames[Reg1], RegNames[Reg2]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMP_Reg2WithReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	WC8((unsigned char) (0x3A | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));

	LOGGING_DYNA(LogDyna("	CMP %s, %s\n", RegNames[Reg2], RegNames[Reg1]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMP_EAXWithImm(unsigned char OperandSize, unsigned long Data)
{
	WC8((unsigned char) (0x3C | OperandSize));
	WC32(Data);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMP_RegWithImm(unsigned char OperandSize, unsigned char Reg, unsigned long Data)
{
	if(OperandSize == 1)
	{
		if(Data == 0)
		{
			if(Reg == Reg_EAX)
			{
				TEST_EAXWithEAX();
				return;
			}

			TEST_Reg2WithReg1(1, Reg, Reg);
			return;
		}

		if((Data < 128) || (Data >= (0xffffff80)))	/* [-128 to 127] */
		{
			CMP_sImm8WithReg(1, Reg, (unsigned char) Data);
			return;
		}

		if(Reg == Reg_EAX)
		{
			CMP_EAXWithImm(1, Data);
			return;
		}
	}

	WC8((unsigned char) (0x80 | OperandSize));
	WC8((unsigned char) (0xF8 | Reg));
	WC32(Data);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMP_RegWithShort(unsigned char OperandSize, unsigned char Reg, unsigned char Data)
{
	if(OperandSize == 1)
	{
		if(Data == 0)
		{
			if(Reg == Reg_EAX)
			{
				TEST_EAXWithEAX();
				return;
			}

			TEST_Reg2WithReg1(1, Reg, Reg);
			return;
		}
	}

	WC8((unsigned char) (0x82 | OperandSize));	/* assumed for now..verify */
	WC8((unsigned char) (0xF8 | Reg));
	WC8(Data);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMP_MemoryWithImm(unsigned char OperandSize, unsigned long dwAddress, unsigned long Data)
{
	WC8((unsigned char) (0x80 | OperandSize));
	WC8((unsigned char) 0x3D);
	WC32(dwAddress);
	WC32(Data);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMP_RegWithMemory(unsigned char Reg, unsigned long dwAddress)
{
	WC8(0x3B);
	WC8((unsigned char) (0x05 | (Reg << 3)));
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FABS(void)
{
	WC8(0xd9);
	WC8(0xe1);

	LOGGING_DYNA(LogDyna("	FABS\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FADD_Memory(unsigned char OperandSize, unsigned long dwAddress)
{
	WC8((unsigned char) (0xD8 + OperandSize));
	WC8(0x05);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FCOMP(unsigned char OperandSize, unsigned long dwAddress)
{
	WC8((unsigned char) (0xD8 + OperandSize));
	WC8(0x1D);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FNSTSW(void)
{
	/* WC8(0x9B); why this ? */
	WC8(0xDF);
	WC8(0xE0);

	LOGGING_DYNA(LogDyna("	FNSTSW\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FDIV_Memory(unsigned char OperandSize, unsigned long dwAddress)
{
	WC8((unsigned char) (0xD8 + OperandSize));
	WC8(0x35);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FLD_Memory(unsigned char OperandSize, unsigned long dwAddress)
{
	WC8((unsigned char) (0xD9 + OperandSize));
	WC8(0x05);
	WC32(dwAddress);
}


void FLD(unsigned char OperandSize, unsigned char ModRM, unsigned long dwAddress)
{
	if (ModRM == ModRM_disp8_EBP)
	{
		WC8((unsigned char) (0xD9+OperandSize));
		WC8((unsigned char) (0x45));
		WC8((unsigned char) dwAddress);
	}
	else
	{
		WC8((unsigned char) (0xD9 + OperandSize));
		WC8(0x05);
		WC32(dwAddress);
	}
}

void FSTP(unsigned char OperandSize, unsigned char ModRM, unsigned long dwAddress)
{
	if (ModRM == ModRM_disp8_EBP)
	{
		WC8((unsigned char) (0xD9+OperandSize));
		WC8((unsigned char) (0x55+8));
		WC8((unsigned char) dwAddress);
	}
	else
	{
		WC8((unsigned char) (0xd9 + OperandSize));
		WC8(0x1d);
		WC32(dwAddress);
	}
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FLDCW_Memory(unsigned long dwAddress)
{
	WC8(0xd9);
	WC8(0x2d);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FISTP_Memory(unsigned char OperandSize, unsigned long dwAddress)
{
	WC8(0xDB);
	WC8(0x1D);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FILD_Memory(unsigned char OperandSize, unsigned long dwAddress)
{
	WC8((unsigned char) (0xDB + OperandSize));
	WC8(0x05);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FMUL_Memory(unsigned char OperandSize, unsigned long dwAddress)
{
	WC8((unsigned char) (0xD8 + OperandSize));
	WC8(0x0d);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FNEG(void)
{
	WC8(0xd9);
	WC8(0xe0);

	LOGGING_DYNA(LogDyna("	FNEG\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FRNDINT(void)
{
	WC8(0xD9);
	WC8(0xFC);

	LOGGING_DYNA(LogDyna("	FRNDINT\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FSQRT(void)
{
	WC8(0xd9);
	WC8(0xfa);

	LOGGING_DYNA(LogDyna("	FSQRT\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FSTP_Memory(unsigned char OperandSize, unsigned long dwAddress)
{
	WC8((unsigned char) (0xd9 + OperandSize));
	WC8(0x1d);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FSUB_Memory(unsigned char OperandSize, unsigned long dwAddress)
{
	WC8((unsigned char) (0xD8 + OperandSize));
	WC8(0x25);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void IMUL_EAXWithReg(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xF6 | OperandSize));
	WC8((unsigned char) (0xE8 | Reg));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void IMUL_EAXWithMemory(unsigned char OperandSize, unsigned long Address)
{
	WC8((unsigned char) (0xF6 | OperandSize));
	WC8((unsigned char) (0x2D));
	WC32(Address);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void IMUL_Reg2ToReg1(unsigned char Reg1, unsigned char Reg2)
{
	WC16(0xAF0F);
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));

	LOGGING_DYNA(LogDyna("	IMUL %s, %s\n", RegNames[Reg2], RegNames[Reg1]);)
}

/*
 =======================================================================================================================
    Incomplete..assumed: operandsize = 1
 =======================================================================================================================
 */
void DEC_Reg(unsigned char OperandSize, unsigned char Reg)
{
	if(OperandSize == 1)
	{
		WC8((unsigned char) (0x48 | Reg));
	}
	else
	{
		DisplayError("DEC: Incomplete");
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void INC_Reg(unsigned char OperandSize, unsigned char Reg)
{
	if(OperandSize == 1)
	{
		WC8((unsigned char) (0x40 | Reg));
	}
	else
	{
		WC8(0xfe);
		WC8((unsigned char) (0xc0 | Reg));
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DIV_EAXWithReg(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xF6 | OperandSize));
	WC8((unsigned char) (0xF0 | Reg));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void IDIV_EAXWithReg(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xF6 | OperandSize));
	WC8((unsigned char) (0xF8 | Reg));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Jcc(unsigned char ConditionCode, unsigned char Offset)
{
	WC8((unsigned char) (0x70 | ConditionCode));
	WC8((unsigned char) Offset);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Jcc_auto(unsigned char ConditionCode, unsigned long Index)
{
	LOGGING_DYNA(LogDyna("	Jcc () to %i\n", Index);) 
	
	WC8((unsigned char) (0x70 | ConditionCode));
	WC8((unsigned char) 0x00);
	JumpTargets[Index] = compilerstatus.lCodePosition;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
__inline void Jcc_Near_auto(unsigned char ConditionCode, unsigned long Index)
{
	LOGGING_DYNA(LogDyna("	Jcc () to %i\n", Index);) WC8((unsigned char) (0x0F));
	WC8((unsigned char) (0x80 | ConditionCode));
	WC32(0x00000000);
	JumpTargets[Index] = compilerstatus.lCodePosition;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SetNearTarget(unsigned char bIndex)
{
	/*~~~~~~~~~~*/
	int wPosition;
	/*~~~~~~~~~~*/

	LOGGING_DYNA(LogDyna("j_point %i:\n", bIndex););

	wPosition = ((compilerstatus.lCodePosition - JumpTargets[bIndex]));
	*(unsigned _int32 *) &RecompCode[JumpTargets[bIndex] - 4] = wPosition;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void JMP_Short(unsigned char Offset)
{
	WC8((unsigned char) 0xEB);
	WC8((unsigned char) Offset);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void JMP_Short_auto(unsigned long Index)
{
	LOGGING_DYNA(LogDyna("	JMP_SHORT () to %i\n", Index);) WC8((unsigned char) 0xEB);
	WC8((unsigned char) 0x00);
	JumpTargets[Index] = compilerstatus.lCodePosition;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void JMP_Long(unsigned long dwAddress)
{
	LOGGING_DYNA(LogDyna("	JMP_Long () to %08x\n", dwAddress);) 
	WC8(0xE9);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void JMP_FAR(unsigned long dwAddress)
{
	LOGGING_DYNA(LogDyna("	JMP () to %08x\n", dwAddress);) 
	WC8(0xFF);
	WC8(0x25);
	WC32(dwAddress);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void LEA(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8(0x8D);
	Encode_Slash_R(Reg, ModRM, Address);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOV_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	if(Reg1 == Reg2) return;
	WC8((unsigned char) (0x8A | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));

	LOGGING_DYNA(LogDyna("	MOV %s, %s\n", RegNames[Reg1], RegNames[Reg2]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOV_MemoryToReg(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	if((Reg == Reg_EAX) && (ModRM == ModRM_disp32))
	{
		MOV_MemoryToEAX(OperandSize, Address);
		return;
	}

	WC8((unsigned char) (0x8A | OperandSize));
	Encode_Slash_R(Reg, ModRM, Address);

	LOGGING_DYNA(LogDyna("	MOV %s, [0x%08X]\n", RegNames[Reg], Address);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOV_ModRMToReg(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM)
{
	if(Reg == Reg_ESP || Reg == Reg_EBP)
		DisplayError("Fix me in function MOV_ModRMToReg(), does not support ESP and EBP");
	WC8((unsigned char) (0x8A | OperandSize));
	WC8((unsigned char) ((Reg << 3) | ModRM));
	LOGGING_DYNA(LogDyna("	MOV %s, [%s]\n", RegNames[Reg], RegNames[ModRM]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOV_RegToModRM(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM)
{
	if(Reg == Reg_ESP || Reg == Reg_EBP)
		DisplayError("Fix me in function MOV_ModRMToReg(), does not support ESP and EBP");
	WC8((unsigned char) (0x88 | OperandSize));
	WC8((unsigned char) ((Reg << 3) | ModRM));
	LOGGING_DYNA(LogDyna("	MOV [%s], %s\n", RegNames[ModRM], RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOVSX_MemoryToReg(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8(0x0F);
	WC8((unsigned char) (0xBE | OperandSize));
	WC8((unsigned char) (ModRM | (Reg << 3)));
	if(Address != 0) WC32(Address);

	LOGGING_DYNA(LogDyna("	MOVSX %s, [0x%08X]\n", RegNames[Reg], Address);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOVZX_MemoryToReg(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8(0x0F);
	WC8((unsigned char) (0xB6 | OperandSize));
	WC8((unsigned char) (ModRM | (Reg << 3)));
	if(Address != 0) WC32(Address);

	LOGGING_DYNA(LogDyna("	MOVSX %s, [0x%08X]\n", RegNames[Reg], Address);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOV_MemoryToEAX(unsigned char OperandSize, unsigned long Address)
{
	WC8((unsigned char) (0xA0 | OperandSize));
	WC32(Address);
}

/*
 =======================================================================================================================
    1964 uses ebp as a ponter to the middle of the Mips GPR array in memory. £
    Address can also represent displacement.
 =======================================================================================================================
 */
void MOV_RegToMemory(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	if((Reg == Reg_EAX) && (ModRM == ModRM_disp32))
	{
		MOV_EAXToMemory(OperandSize, Address);
		return;
	}

	WC8((unsigned char) (0x88 | OperandSize));
	Encode_Slash_R(Reg, ModRM, Address);

	LOGGING_DYNA(LogDyna("	MOV [0x%08X],%s\n", Address, RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOV_RegToMemory2(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8((unsigned char) (0x88 | OperandSize));
	WC8((unsigned char) (0x80 | ModRM | (Reg << 3)));
	WC32(Address);

	LOGGING_DYNA(LogDyna("	MOV [0x%08X],%s\n", Address, RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOV_MemoryToReg2(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8((unsigned char) (0x8a | OperandSize));
	WC8((unsigned char) (0x80 | ModRM | (Reg << 3)));
	WC32(Address);

	LOGGING_DYNA(LogDyna("	MOV %s, [0x%08X]\n", RegNames[Reg], Address);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOV_EAXToMemory(unsigned char OperandSize, unsigned long Address)
{
	WC8((unsigned char) (0xA2 | OperandSize));
	WC32(Address);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOV_ImmToReg(unsigned char OperandSize, unsigned char Reg, unsigned long Data)
{
	if(OperandSize == 1)
	{
		if(Data == 0)
		{
			XOR_Reg2ToReg1(1, Reg, Reg);
			return;
		}

		if(Data == 1)
		{
			XOR_Reg2ToReg1(1, Reg, Reg);
			INC_Reg(1, Reg);
			return;
		}

		if ((Data < 128) || (Data >= 0xffffff80)) /* [-128 to 127] */
		{
			XOR_Reg2ToReg1(1, Reg, Reg);
			OR_ShortToReg(1, Reg, (unsigned char) Data);
			return;
		}
	}

	WC8((unsigned char) (0xB0 | (OperandSize << 3) | Reg));
	if(OperandSize == 0)
	{
		WC8((unsigned char) Data);
	}
	else
	{
		WC32(Data);
	}

	LOGGING_DYNA(LogDyna("	MOV %s, 0x%08X\n", RegNames[Reg], Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOV_ImmToMemory(unsigned char OperandSize, unsigned char ModRM, unsigned long Address, unsigned long Data)
{
	WC8((unsigned char) (0xC6 | OperandSize));
	if((ModRM >= ModRM_disp8_EAX) && (ModRM <= ModRM_disp8_EDI))
	{
		if((Address == 0) && (ModRM != ModRM_disp8_EBP) && (ModRM != ModRM_disp8_SIB)) ModRM -= 0x40;
		WC8((unsigned char) (ModRM));
		if((Address == 0) && (ModRM == ModRM_EBP))
			WC8(0x28);
		else
			WC8((unsigned char) Address);
	}
	else
	{
		WC8((unsigned char) (ModRM));
		if(Address != 0) WC32(Address);
	}

	if(OperandSize == 1)
		WC32(Data);
	else
		WC8((unsigned __int8) (Data));

	LOGGING_DYNA(LogDyna("	MOV [0x%08X], 0x%08x\n", Address, Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOVSX_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	WC8((unsigned char) 0x0F);
	WC8((unsigned char) (0xBE | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));

	LOGGING_DYNA(LogDyna("	MOVSX %s, %s\n", RegNames[Reg1], RegNames[Reg2]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MOVZX_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	WC8((unsigned char) 0x0F);
	WC8((unsigned char) (0xB6 | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));
	LOGGING_DYNA(LogDyna("	MOVZX %s, %s\n", RegNames[Reg1], RegNames[Reg2]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MUL_EAXWithReg(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xF6 | OperandSize));
	WC8((unsigned char) (0xE0 | Reg));
	LOGGING_DYNA(LogDyna("	MUL EAX, %s\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MUL_EAXWithMemory(unsigned char OperandSize, unsigned long Address)
{
	WC8((unsigned char) (0xF6 | OperandSize));
	WC8((unsigned char) (0x25));
	WC32(Address);
	LOGGING_DYNA(LogDyna("	MUL EAX, [0x%08x]\n", Address);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void NEG_Reg(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xF6 | OperandSize));
	WC8((unsigned char) (0xD8 | Reg));
	LOGGING_DYNA(LogDyna("	NEG %s\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void NOT_Reg(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xF6 | OperandSize));
	WC8((unsigned char) (0xD0 | Reg));
	LOGGING_DYNA(LogDyna("	NOT %s\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void NOP(void)
{
	WC8((unsigned char) 0x90);

	LOGGING_DYNA(LogDyna("	NOP\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void OR_ImmToEAX(unsigned char OperandSize, unsigned long Data)
{
	WC8((unsigned char) (0x0C | OperandSize));
	if(OperandSize == 0)
		WC8((unsigned char) Data);
	else
		WC32(Data);

	LOGGING_DYNA(LogDyna("	OR EAX, 0x%08X\n", Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void OR_ImmToReg(unsigned char OperandSize, unsigned char Reg, unsigned long Data)
{
	if(Data == 0) return;

	if(OperandSize == 1)
	{
		if((Data < 128) || (Data >= (0xffffff80)))	/* [-128 to 127] */
		{
			OR_ShortToReg(1, Reg, (unsigned char) Data);
			return;
		}
	}

	if(Reg == Reg_EAX)
	{
		OR_ImmToEAX(OperandSize, Data);
		return;
	}

	WC8((unsigned char) (0x80 | OperandSize));
	WC8((unsigned char) (0xC8 | Reg));
	if(OperandSize != 0)
		WC32(Data);
	else
		WC8((unsigned char) (Data & 0xFF));

	LOGGING_DYNA(LogDyna("	OR %s, 0x%08X\n", RegNames[Reg], Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void OR_RegToMemory(unsigned char OperandSize, unsigned char Reg, unsigned char ModRM, unsigned long Address)
{
	WC8((unsigned char) (0x0A | OperandSize));
	Encode_Slash_R(Reg, ModRM, Address);

	LOGGING_DYNA(LogDyna("	MOV [0x%08X],%s\n", Address, RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void POP_RegFromStack(unsigned char Reg)
{
	WC8((unsigned char) (0x58 | Reg));

	LOGGING_DYNA(LogDyna("	POP %s\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void POPA(void)
{
	WC8((unsigned char) 0x66);
	WC8((unsigned char) 0x61);

	LOGGING_DYNA(LogDyna("	POPA\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void POPAD(void)
{
	WC8((unsigned char) 0x61);

	LOGGING_DYNA(LogDyna("	POPAD\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void PUSH_RegToStack(unsigned char Reg)
{
	WC8((unsigned char) (0x50 | Reg));

	LOGGING_DYNA(LogDyna("	PUSH %s\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void PUSH_WordToStack(unsigned __int32 wWord)
{
	WC8(0x68);
	WC32(wWord);

	LOGGING_DYNA(LogDyna("	PUSH 0x%08X\n", wWord);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void PUSHA(void)
{
	WC8((unsigned char) 0x66);
	WC8((unsigned char) 0x60);

	LOGGING_DYNA(LogDyna("	PUSHAA\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void PUSHAD(void)
{
	WC8((unsigned char) 0x60);

	LOGGING_DYNA(LogDyna("	PUSHADA\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RET(void)
{
	WC8((unsigned char) 0xC3);

	LOGGING_DYNA(LogDyna("	RET\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SAHF(void)
{
	WC8(0x9e);

	LOGGING_DYNA(LogDyna("	SAHF\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SETcc_Reg(unsigned char ConditionCode, unsigned char Reg)
{
	WC8((unsigned char) 0x0F);
	WC8((unsigned char) (0x90 | ConditionCode));
	WC8((unsigned char) (0xC0 | Reg));
	LOGGING_DYNA(LogDyna("	SET cc %s\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SAR_RegByCL(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xD2 | OperandSize));
	WC8((unsigned char) (0xF8 | Reg));
	LOGGING_DYNA(LogDyna("	SAR %s,CL\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SHL_RegByImm(unsigned char OperandSize, unsigned char Reg, unsigned char Data)
{
	if(Data == 0) return;
	if(Data == 1)
	{
		ADD_Reg2ToReg1(1, Reg, Reg);
		return;
	}

	if(Data == 2)
	{
		/* lea reg, [reg*4] */
		WC8(0x8D);
		WC8((uint8) (0x04 | (Reg << 3)));
		WC8((uint8) (0x85 | (Reg << 3)));
		WC32(0x00);
		return;
	}

	if(Data == 3)
	{
		/* lea reg, [reg*8] */
		WC8(0x8D);
		WC8((uint8) (0x04 | (Reg << 3)));
		WC8((uint8) (0xC5 | (Reg << 3)));
		WC32(0x00);
		return;
	}

	WC8((unsigned char) (0xC0 | OperandSize));
	WC8((unsigned char) (0xE0 | Reg));
	WC8((unsigned char) Data);
	LOGGING_DYNA(LogDyna("	SHL %s,%d\n", RegNames[Reg], Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SHR_RegByImm(unsigned char OperandSize, unsigned char Reg, unsigned char Data)
{
	if(Data == 0) return;
	if(Data == 1)
	{
		SHR_RegBy1(OperandSize, Reg);
		return;
	}

	WC8((unsigned char) (0xC0 | OperandSize));
	WC8((unsigned char) (0xE8 | Reg));
	WC8((unsigned char) Data);
	LOGGING_DYNA(LogDyna("	SHR %s,%d\n", RegNames[Reg], Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SAR_RegByImm(unsigned char OperandSize, unsigned char Reg, unsigned char Data)
{
	if(Data == 0) return;
	if(Data == 1)
	{
		SAR_RegBy1(OperandSize, Reg);
		return;
	}

	WC8((unsigned char) (0xC0 | OperandSize));
	WC8((unsigned char) (0xF8 | Reg));
	WC8((unsigned char) Data);
	LOGGING_DYNA(LogDyna("	SAR %s,%d\n", RegNames[Reg], Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SHL_RegBy1(unsigned char OperandSize, unsigned char Reg)
{
	ADD_Reg2ToReg1(1, Reg, Reg);

	LOGGING_DYNA(LogDyna("	SHL %s,1\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SHR_RegBy1(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xD0 | OperandSize));
	WC8((unsigned char) (0xE8 | Reg));
	LOGGING_DYNA(LogDyna("	SHR %s,1\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SAR_RegBy1(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xD0 | OperandSize));
	WC8((unsigned char) (0xF8 | Reg));
	LOGGING_DYNA(LogDyna("	SAR %s,1\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SHL_RegByCL(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xD2 | OperandSize));
	WC8((unsigned char) (0xE0 | Reg));
	LOGGING_DYNA(LogDyna("	SHL %s,CL\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SHR_RegByCL(unsigned char OperandSize, unsigned char Reg)
{
	WC8((unsigned char) (0xD2 | OperandSize));
	WC8((unsigned char) (0xE8 | Reg));
	LOGGING_DYNA(LogDyna("	SHR %s,CL\n", RegNames[Reg]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SUB_Reg1OfReg2(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	WC8((unsigned char) (0x28 | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));

	LOGGING_DYNA(LogDyna("	SUB %s, %s\n", RegNames[Reg1], RegNames[Reg2]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DEC_Memory(unsigned char OperandSize, unsigned long Address)
{
	WC8((unsigned char) (0xFE | OperandSize));
	WC8(0x0D);	/* Disp32 */
	WC32(Address);
	LOGGING_DYNA(LogDyna("	DEC [0x%x]\n", Address);)
}

/*
 =======================================================================================================================
    Assumed, Operand size = 1
 =======================================================================================================================
 */
void SUB_ImmToMemory(unsigned long Address, unsigned long Data)
{
	ADD_ImmToMemory(Address, (uint32) (-(_int32) Data));

	LOGGING_DYNA(LogDyna(" SUB [0x%08X], 0x%08x\n", Address, Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
__inline void TEST_ImmWithMemory(unsigned long dwAddress, unsigned long Value)
{
	WC8(0xf7);
	WC8(0x05);
	WC32(dwAddress);
	WC32(Value);
	LOGGING_DYNA(LogDyna("	TEST [0x%08x], 0x%08x\n", dwAddress, Value);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void TEST_EAXWithImm(unsigned char OperandSize, unsigned long dwAddress)
{
	WC8((unsigned char) (0xa8 | OperandSize));
	WC32(dwAddress);
	LOGGING_DYNA(LogDyna("	TEST EAX, 0x%08x\n", dwAddress);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void TEST_EAXWithEAX(void)
{
	WC8(0x85);
	WC8(0xc0);

	LOGGING_DYNA(LogDyna("	TEST EAX, EAX\n");)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void TEST_Reg2WithReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	if((Reg2 == Reg1) && (Reg1 == Reg_EAX))
	{
		TEST_EAXWithEAX();
		return;
	}

	WC8((unsigned char) (0x84 | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));
	LOGGING_DYNA(LogDyna("	TEST %s, %s\n", RegNames[Reg1], RegNames[Reg2]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void TEST_RegWithImm(unsigned char OperandSize, unsigned char iReg, unsigned long dwAddress)
{
	WC8((unsigned char) (0xf6 | OperandSize));
	WC8((unsigned char) (0xc0 | iReg));
	WC32(dwAddress);
	LOGGING_DYNA(LogDyna("	TEST %s, 0x%08x\n", RegNames[iReg], dwAddress);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void XCHG_Reg1WithReg2(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	if(Reg1 == Reg2) return;	/* Note: if you are padding, do it manually with WC16() */
	WC8((unsigned char) (0x86 | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));

	LOGGING_DYNA(LogDyna("	XCHG %s, %s\n", RegNames[Reg1], RegNames[Reg2]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void XOR_ImmToEAX(unsigned char OperandSize, unsigned long Data)
{
	WC8((unsigned char) (0x34 | OperandSize));

	if(OperandSize == 0)
		WC8((unsigned char) Data);
	else
		WC32(Data);

	LOGGING_DYNA(LogDyna("	OR EAX, 0x%08X\n", Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void XOR_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	WC8((unsigned char) (0x32 | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));

	LOGGING_DYNA(LogDyna("	XOR %s, %s\n", RegNames[Reg1], RegNames[Reg2]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void XOR_ImmToReg(unsigned char OperandSize, unsigned char Reg, unsigned long Data)
{
	if(Data == 0) return;

	if((Data < 128) || (Data >= (0xffffff80)))	/* [-128 to 127] */
	{
		XOR_ShortToReg(OperandSize, Reg, (unsigned char) Data);
		return;
	}

	if(Reg == Reg_EAX)
	{
		XOR_ImmToEAX(OperandSize, Data);
		return;
	}

	/* Normal execution */
	WC8((unsigned char) (0x80 | OperandSize));
	WC8((unsigned char) (0xF0 | Reg));
	if(OperandSize == 0)
	{
		WC8((unsigned char) Data);
	}
	else
	{
		WC32(Data);
	}

	LOGGING_DYNA(LogDyna("	XOR %s, 0x%08X\n", RegNames[Reg], Data);)
}

/*
 =======================================================================================================================
    When OperandSize is 0, this doesn't sign extend.
 =======================================================================================================================
 */
void OR_ShortToReg(unsigned char OperandSize, unsigned char Reg, unsigned char Data)
{
	WC8((unsigned char) (0x80 | (OperandSize * 3)));
	WC8((unsigned char) (0xC8 | Reg));
	WC8((unsigned char) Data);

	LOGGING_DYNA(LogDyna("\tOR %s, 0x%02X\n", RegNames[Reg], Data);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void XOR_ShortToReg(unsigned char OperandSize, unsigned char Reg, unsigned char Data)
{
	WC8((unsigned char) (0x80 | (OperandSize * 3)));
	WC8((unsigned char) (0xF0 | Reg));
	WC8((unsigned char) Data);

	LOGGING_DYNA(LogDyna("	XOR %s, 0x%02X\n", RegNames[Reg], Data););
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AND_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	if(Reg2 == Reg1) return;	/* Note: for testing, use TEST. */
	WC8((unsigned char) (0x22 | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));

	LOGGING_DYNA(LogDyna(" AND %s, %s\n", RegNames[Reg1], RegNames[Reg2]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void OR_Reg2ToReg1(unsigned char OperandSize, unsigned char Reg1, unsigned char Reg2)
{
	if(Reg2 == Reg1) return;
	WC8((unsigned char) (0x0A | OperandSize));
	WC8((unsigned char) (0xC0 | (Reg1 << 3) | Reg2));

	LOGGING_DYNA(LogDyna(" AND %s, %s\n", RegNames[Reg1], RegNames[Reg2]);)
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ADD_Imm8sxToReg(unsigned char OperandSize, unsigned char Reg, unsigned char Data)
{
	WC8((unsigned char) (0x83));
	WC8((unsigned char) (0xC0 | Reg));
	WC8((unsigned char) Data);

	LOGGING_DYNA(LogDyna("	ADD %s, 0x%02X\n", RegNames[Reg], Data););
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ROL_RegByImm(unsigned char OperandSize, unsigned char Reg, unsigned char Data)
{
	WC8((unsigned char) (0xC0 | OperandSize));
	x86params.ModRM = 0xC0 | Reg;
	Calculate_ModRM_Byte_Via_Slash_Digit(0);
	WC8((unsigned char) (x86params.ModRM));

	/* WC8((unsigned char)Address); */
	WC8((unsigned char) Data);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AND_Imm8sxToMemory(unsigned char Data, unsigned long Address)
{
	WC8((unsigned char) (0x83));
	Calculate_ModRM_Byte_Via_Slash_Digit(4);
	WC8((unsigned char) (x86params.ModRM));
	WC8((unsigned char) Address);
	WC8((unsigned char) Data);

	/* LOGGING_DYNA(LogDyna(" ADD %s, 0x%02X\n", RegNames[Reg], Data);); */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void AND_Imm8sxToReg(unsigned char OperandSize, unsigned char Reg, unsigned char Data)
{
	WC8((unsigned char) (0x80 | (OperandSize * 3)));
	WC8((unsigned char) (0xE0 | Reg));
	WC8((unsigned char) Data);

	LOGGING_DYNA(LogDyna("	AND %s, 0x%02X\n", RegNames[Reg], Data););
}


/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CMP_sImm8WithReg(unsigned char OperandSize, unsigned char Reg, unsigned char Data)
{
	WC8((unsigned char) (0x83));
	WC8((unsigned char) (0xF8 | Reg));
	WC8((unsigned char) Data);
	LOGGING_DYNA(LogDyna("	CMP %s, 0x%02X\n", RegNames[Reg], Data););
}

/*
 =======================================================================================================================
    Encoders £
    1964 uses ebp as a ponter to the middle of the Mips GPR array in memory.
 =======================================================================================================================
 */
void Encode_Slash_R(unsigned char Reg, unsigned char ModRM, unsigned long Address)	/* so-named for the /r mnemonic in
																					 * Intel x86 docs */
{
	if((ModRM >= ModRM_disp8_EAX) && (ModRM <= ModRM_disp8_EDI))
	{
		WC8((unsigned char) (ModRM | (Reg << 3)));
		WC8((unsigned char) Address);
	}
	else if((ModRM >= ModRM_disp32_EAX) && (ModRM <= ModRM_disp32_EDI))
	{
		if(Address == 0)
		{
			ModRM -= ModRM_disp32_EAX;
			WC8((unsigned char) (ModRM | (Reg << 3)));
		}
		else
		{
			WC8((unsigned char) (ModRM | (Reg << 3)));
			WC32(Address);
		}
	}
	else
	{
		WC8((unsigned char) (ModRM | (Reg << 3)));
		if(Address != 0) WC32(Address);
	}
}
