/*$T regcache.c GC 1.136 03/09/02 16:35:59 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Register Caching. These are functions for mapping and unmapping MIPS® registers to x86 registers.
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
 * authors: email: schibo@emulation64.com, rice1964@yahoo.com £
 * define SAFE_32_TO_64 1 £
 * #define SAFE_64_TO_32 1 £
 * #define SAFE_FLUSHING 1
 */
#include "../hardware.h"
#include "regcache.h"
#include "../memory.h"
#include "../1964ini.h"
#include "../compiler.h"
#include "x86.h"

#define IT_IS_HIGHWORD	- 2
#define IT_IS_UNUSED	- 1
#define NUM_X86REGS		8
#define DEBUG_REGCACHE
#define R0_COMPENSATION

void				FlushRegister(int k);
void				WriteBackDirty(_int8 k);
void				FlushOneConstant(int k);
void				MapOneConstantToRegister(x86regtyp *Conditions, int The2ndOneNotToFlush, int The3rdOneNotToFlush);
extern FlushedMap	FlushedRegistersMap[NUM_CONSTS];
x86regtyp			x86reg[8];
extern int			Instruction_Order[0x1000];
extern char			*r4300i_RegNames[];

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitRegisterMap(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int		k;
	const	Num_x86regs = NUM_X86REGS;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	ThisYear = 2001;
	for(k = 0; k < Num_x86regs; k++)
	{
		x86reg[k].mips_reg = -1;
		x86reg[k].HiWordLoc = -1;
		x86reg[k].BirthDate = ThisYear;
		x86reg[k].Is32bit = 0;		/* If 1, this register uses 32bit */

		x86reg[k].IsDirty = 0;		/* If 1, this register will eventually need */

		/*
		 * to be unmapped (written back to memory-- £
		 * "flushed")
		 */
		ConstMap[k].IsMapped = 0;
		ConstMap[k].value = 0;
		FlushedRegistersMap[k].Is32bit = 0;
	}

	for(k = NUM_X86REGS; k < NUM_CONSTS; k++)
	{
		ConstMap[k].IsMapped = 0;
		ConstMap[k].value = 0;
		FlushedRegistersMap[k].Is32bit = 0;
		ConstMap[k].FinalAddressUsedAt = 0xffffffff;
	}

#ifndef NO_CONSTS
	ConstMap[0].IsMapped = 1;
#endif
	FlushedRegistersMap[0].Is32bit = 1;

	gMultiPass.UseOnePassOnly = 1;	/* this will be a user option. (zero = multipass) */
#ifdef _DEBUG
	gMultiPass.UseOnePassOnly = 1;	/* not sure how we'll debug multipass yet. We don't do single-pass */

	/* properly in debug yet. */
#endif
	gMultiPass.WriteCode = 1;
	gMultiPass.JumpToPhysAddr = 0;
	gMultiPass.WhichPass = COMPILE_MAP_ONLY;

	memset(Instruction_Order, 0, sizeof(Instruction_Order));
}

/*
 =======================================================================================================================
    Check if a register is mapped as a 32bit register
 =======================================================================================================================
 */
int CheckIs32Bit(int mips_reg)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_s32	k;
	const	Num_x86regs = NUM_X86REGS;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(ConstMap[mips_reg].IsMapped) return(1);
	for(k = 0; k < Num_x86regs; k++)
	{
		if(ItIsARegisterNotToUse(k));
		else if(x86reg[k].mips_reg == mips_reg) /* if mapped */
		{
			return(x86reg[k].Is32bit);
		}
	}

	return(0);
}

/*
 =======================================================================================================================
    Check if a register is in need of be written back to memory (It is dirty)
 =======================================================================================================================
 */
int CheckIsDirty(int mips_reg)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_s32	k;
	const	Num_x86regs = NUM_X86REGS;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	for(k = 0; k < Num_x86regs; k++)
	{
		if(ItIsARegisterNotToUse(k));
		else if(x86reg[k].mips_reg == mips_reg) /* if mapped */
		{
			return(x86reg[k].IsDirty);
		}
	}

	return(0);
}

/*
 =======================================================================================================================
    If a MIPS® reg is mapped to an x86 reg, tell us where it is.
 =======================================================================================================================
 */
int CheckWhereIsMipsReg(int mips_reg)
{
	if(x86reg[Reg_EDI].mips_reg == mips_reg) return(Reg_EDI);
	if(x86reg[Reg_ESI].mips_reg == mips_reg) return(Reg_ESI);
	if(x86reg[Reg_EBX].mips_reg == mips_reg) return(Reg_EBX);
	if(x86reg[Reg_EDX].mips_reg == mips_reg) return(Reg_EDX);
	if(x86reg[Reg_ECX].mips_reg == mips_reg) return(Reg_ECX);
	if(x86reg[Reg_EAX].mips_reg == mips_reg) return(Reg_EAX);

	return(-1);
}

/*
 =======================================================================================================================
    This function gets the params needed to pass to the x86.c functions £
    when loading/storing MIPS® GPR registers from/to the array in memory. £
    EBP points to the middle of the GPR array..pretty clever, eh ;)
 =======================================================================================================================
 */
void FetchEBP_Params(int mips_reg)
{
	if(mips_reg < 16)
	{
		/* negative 8bit displacement */
		x86params.ModRM = ModRM_disp8_EBP;
		x86params.Address = -128 + (mips_reg << 3);
	}
	else if(mips_reg < 32)
	{
		/* positive 8bit displacement */
		x86params.ModRM = ModRM_disp8_EBP;
		x86params.Address = ((mips_reg - 16) << 3);
	}
	else
	{
		/* positive 32bit displacement. Only GPR_HI/GPR_LO use this. */
		x86params.ModRM = ModRM_disp32;
		x86params.Address = (_u32) & gHWS_GPR[0] + ((mips_reg) << 3);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Calculate_ModRM_Byte_Via_Slash_Digit(int SlashDigit)
{
	x86params.ModRM |= (SlashDigit << 3);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void x86reg_Delete(int k)
{
	x86reg[k].mips_reg = IT_IS_UNUSED;
	x86reg[k].Is32bit = 0;
	x86reg[k].IsDirty = 0;
	x86reg[k].HiWordLoc = -1;
	x86reg[k].BirthDate = ThisYear;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ConvertRegTo32bit(int k)
{
#ifdef SAFE_64_TO_32
	FlushRegister(k);
	goto _Do32bit;
#endif
	if(x86reg[k].HiWordLoc == -1) DisplayError("MapRegister() bug: HiWordLoc should not be -1");

	x86reg_Delete(x86reg[k].HiWordLoc);

	x86reg[k].HiWordLoc = k;
	x86reg[k].Is32bit = 1;
	x86reg[k].BirthDate = ThisYear;
}

void	WireMap(void);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SwitchToOpcodePass(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	const	Num_x86regs = NUM_X86REGS;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/* gMultiPass.VirtAddrAfterMap = gHWS_pc; */
	gMultiPass.lCodePositionAfterMap = compilerstatus.lCodePosition;
	gMultiPass.PhysAddrAfterMap = (uint32) (&dyna_RecompCode[compilerstatus.lCodePosition]);
	gMultiPass.JumpToPhysAddr = 1;
	gMultiPass.pc = compilerstatus.TempPC - 4;

	gMultiPass.WhichPass = COMPILE_OPCODES_ONLY;
	gMultiPass.WriteCode = 1;
	WireMap();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ConvertRegTo64bit(int k, x86regtyp *Conditions, int keep2, int keep3)
{
	/*~~*/
	int i;
	/*~~*/

	/* check if the HiWord is mapped to the same register where we are at now...k. */
	if((x86reg[k].HiWordLoc == k))
	{
		/*~~~~~~~~~~~~~~~*/
		int HiMapped = 0;
		int couldntmap = 0;
		/*~~~~~~~~~~~~~~~*/

#ifdef SAFE_32_TO_64
		FlushRegister(k);
		goto _Do64bit;
#endif

		/* find a spot for the HiWord */
		while(HiMapped == 0)
		{
			if(couldntmap++ >= 100) DisplayError("I can't find a place to map.");

			for(i = 0; i < NUM_X86REGS; i++)
			{
				if(ItIsARegisterNotToUse(i));
				else if((x86reg[i].mips_reg == IT_IS_UNUSED))
				{
					HiMapped = 1;

					x86reg[k].HiWordLoc = i;
					x86reg[k].Is32bit = 0;

					x86reg[i].mips_reg = IT_IS_HIGHWORD;
					x86reg[i].Is32bit = 0;
					x86reg[i].IsDirty = 0;
					x86reg[i].HiWordLoc = -1;

					if((x86reg[k].IsDirty == 0))
					{
						if
						(
							(FlushedRegistersMap[x86reg[k].mips_reg].Is32bit == 1)
						||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
						)
						{
							MOV_Reg2ToReg1(1, x86reg[k].HiWordLoc, (uint8) k);
							SAR_RegByImm(1, x86reg[k].HiWordLoc, 31);
						}
						else
						{
							LoadGPR_HI(k);
						}
					}
					else if(Conditions->NoNeedToLoadTheHi == 0)
					{
						MOV_Reg2ToReg1(1, (_u8) i, (_u8) k);
						SAR_RegByImm(1, (_u8) i, 31);
					}

					FlushedRegistersMap[x86reg[k].mips_reg].Is32bit = 0;
					i = 99;
				}
			}

			if(HiMapped == 0)
			{
				FlushOneButNotThese3(x86reg[k].mips_reg, keep2, keep3);
			}
		}
	}
}

/*
 =======================================================================================================================
    Map a register as 32bit.
 =======================================================================================================================
 */
void Map32bit(int k, x86regtyp *Conditions, int keep2, int keep3)
{
	/*~~~~~~~~~~~~~~~*/
	int MapSuccess = 0;
	/*~~~~~~~~~~~~~~~*/

	while(MapSuccess == 0)
	{
		for(k = NUM_X86REGS - 1; k >= 0; k--)
		{
			if(ItIsARegisterNotToUse(k));
			else if(x86reg[k].mips_reg == IT_IS_UNUSED)
			{
				/* map it */
				x86reg[k].mips_reg = Conditions->mips_reg;
				x86reg[k].Is32bit = 1;
				x86reg[k].HiWordLoc = k;

				if(x86reg[k].HiWordLoc == Reg_ESP) DisplayError("2: x86reg[%d]: Write to esp: cannot do that!", k);
				if(Conditions->IsDirty == 1) x86reg[k].IsDirty = 1;

				Conditions->x86reg = k;
				Conditions->HiWordLoc = k;	/* ok.. */

				if((Conditions->NoNeedToLoadTheLo == 0))
				{
					LoadGPR_LO(k);
				}

				if(x86reg[k].HiWordLoc == -1) DisplayError("Do32bit: Hiword = -1 Not expected.");

				x86reg[k].BirthDate = ThisYear;
				MapSuccess = 1;

				ConstMap[Conditions->mips_reg].FinalAddressUsedAt = gHWS_pc;
				return;
			}
		}

		if(MapSuccess == 0)
		{
			FlushOneButNotThese3(99, keep2, keep3);
		}
	}
}

/*
 =======================================================================================================================
    Map a register as 64bit.
 =======================================================================================================================
 */
void Map64bit(int k, x86regtyp *Conditions, int The2ndOneNotToFlush, int The3rdOneNotToFlush)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int TheFirstAvailableReg = 0;
	int NumRegsAvailable = 0;
	/* make sure we have 2 registers available for mapping */
	int MapSuccess = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

	while(MapSuccess == 0)
	{
		TheFirstAvailableReg = 0;
		NumRegsAvailable = 0;
		for(k = NUM_X86REGS - 1; k >= 0; k--)
		{
			if(x86reg[k].HiWordLoc == Reg_ESP) DisplayError("4: x86reg[%d]: Write to esp: cannot do that!", k);

			if(ItIsARegisterNotToUse(k));
			else if(x86reg[k].mips_reg == IT_IS_UNUSED)
			{
				NumRegsAvailable += 1;
				if(NumRegsAvailable == 1) TheFirstAvailableReg = k; /* This will represent the hiword */

				if(NumRegsAvailable == 2)
				{
					/* map lo */
					x86reg[k].mips_reg = Conditions->mips_reg;
					x86reg[k].Is32bit = 0;

					if(Conditions->IsDirty == 1) x86reg[k].IsDirty = 1;

					Conditions->x86reg = k;

					if((Conditions->NoNeedToLoadTheLo == 0)) LoadGPR_LO(k);

					/* map hi */
					x86reg[k].HiWordLoc = TheFirstAvailableReg;
					Conditions->HiWordLoc = TheFirstAvailableReg;

					if(x86reg[k].HiWordLoc == Reg_ESP)
						DisplayError("3: x86reg[%d]: Write to esp: cannot do that!", k);

					x86reg[TheFirstAvailableReg].mips_reg = IT_IS_HIGHWORD;

					if((Conditions->NoNeedToLoadTheHi == 0))
					{
						if
						(
							(FlushedRegistersMap[x86reg[k].mips_reg].Is32bit == 1)
						||	(currentromoptions.Assume_32bit == ASSUME_32BIT_YES)
						)
						{
							MOV_Reg2ToReg1(1, x86reg[k].HiWordLoc, (uint8) k);
							SAR_RegByImm(1, x86reg[k].HiWordLoc, 31);
						}
						else
						{
							LoadGPR_HI(k);
						}
					}

					FlushedRegistersMap[x86reg[k].mips_reg].Is32bit = 0;

					if(x86reg[k].HiWordLoc == -1) DisplayError("Do64bit: Hiword = -1 Not expected.");

					x86reg[k].BirthDate = ThisYear;
					x86reg[TheFirstAvailableReg].BirthDate = ThisYear;
					MapSuccess = 1;
					ConstMap[Conditions->mips_reg].FinalAddressUsedAt = gHWS_pc;
					return;
				}
			}
		}

		if(MapSuccess == 0)
		{
			FlushOneButNotThese3(99, The2ndOneNotToFlush, The3rdOneNotToFlush);
		}
	}
}

x86regtyp	Targetx86reg[8];
MapConstant TargetConstMap[NUM_CONSTS];

/*
 =======================================================================================================================
    These functions will prepare the map for a jump/branch
 =======================================================================================================================
 */
void WireMap(void)
{
	memcpy(Targetx86reg, x86reg, sizeof(x86reg));
	memcpy(TargetConstMap, ConstMap, sizeof(ConstMap));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UnwireMap(void)
{
	/*~~*/
	int k;
	/*~~*/

	ConstMap[0].IsMapped = 1;

	for(k = 0; k < 8; k++)
	{
		if((Targetx86reg[k].mips_reg != x86reg[k].mips_reg) || (Targetx86reg[k].HiWordLoc != x86reg[k].HiWordLoc))
		{
			/* TODO: optimize this confusing crap later. */
			FlushAllRegisters();
			gMultiPass.PhysAddrAfterMap = compilerstatus.BlockStart;	/* Jump to start of block until we optimize
																		 * this. */
			return;
		}
	}
}

/*
 =======================================================================================================================
    Convert register from 64bit to 32bit..or vice-versa.
 =======================================================================================================================
 */
void ConvertReg(int k, x86regtyp *Conditions, int The2ndOneNotToFlush, int The3rdOneNotToFlush)
{
	/* check if the register wants 32bit */
	if(Conditions->Is32bit == 1)
	{
		/*
		 * check if the HiWord is mapped to some other x86 register than where we are
		 * now...k.
		 */
		if((x86reg[k].HiWordLoc != k) && (Conditions->IsDirty == 1))
		{
			CHECK_OPCODE_PASS;
			ConvertRegTo32bit(k);
		}
		else if(x86reg[k].HiWordLoc != k)
			Conditions->Is32bit = 0;	/* means do not convert. */
	}
	else
	{
		CHECK_OPCODE_PASS;
		ConvertRegTo64bit(k, Conditions, The2ndOneNotToFlush, The3rdOneNotToFlush);
	}

	/* set and return the map info */
	Conditions->x86reg = k;
	Conditions->HiWordLoc = x86reg[k].HiWordLoc;

	if(Conditions->IsDirty == 1) x86reg[k].IsDirty = 1;
	x86reg[k].Is32bit = Conditions->Is32bit;
	x86reg[k].BirthDate = ThisYear;
	x86reg[x86reg[k].HiWordLoc].BirthDate = ThisYear;

	if(x86reg[k].HiWordLoc == -1) DisplayError("Set&return map info: HiWord is -1!!!");
}

/*
 =======================================================================================================================
    Map a MIPS® register to an x86 register.
 =======================================================================================================================
 */
void MapRegister(x86regtyp *Conditions, int keep2, int keep3)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int		k;
	const	Num_x86regs = NUM_X86REGS;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	ThisYear++;

	gMultiPass.WriteCode = ((gMultiPass.WhichPass != COMPILE_OPCODES_ONLY) ? 1 : 0) | gMultiPass.UseOnePassOnly;

	/* we still haven't optimized consts fully, so do this: */
	if(Conditions->NoNeedToLoadTheLo == 0)
	{
		CHECK_OPCODE_PASS;
		MapOneConstantToRegister(Conditions, keep2, keep3);
	}

	if(Conditions->mips_reg > 0) ConstMap[Conditions->mips_reg].IsMapped = 0;

	if((k = CheckWhereIsMipsReg(Conditions->mips_reg)) > -1)
		ConvertReg(k, Conditions, keep2, keep3);
	else if(Conditions->Is32bit == 1)
		Map32bit(k, Conditions, keep2, keep3);
	else
		Map64bit(k, Conditions, keep2, keep3);

	if((k == Reg_ESP)) DisplayError("Writing Lo to esp..bad!");
	if((x86reg[k].HiWordLoc == Reg_ESP)) DisplayError("Writing Hi to esp..bad!");

	x86reg[k].BirthDate = ThisYear;

	gMultiPass.WriteCode =
		(
			(gMultiPass.WhichPass == COMPILE_OPCODES_ONLY)
		||	(gMultiPass.WhichPass == COMPILE_ALL) ? 1 : 0) |
		gMultiPass.UseOnePassOnly;
}

/*
 =======================================================================================================================
    If the MIPS® register is mapped, this will free it without writing it back to memory.
 =======================================================================================================================
 */
void FreeMipsRegister(int mips_reg)
{
	/*~~~~~*/
	int temp;
	/*~~~~~*/

	ConstMap[mips_reg].IsMapped = 0;
	if((temp = CheckWhereIsMipsReg(mips_reg)) > -1)
	{
		x86reg[temp].IsDirty = 0;
		FlushRegister(temp);
	}
}

/*
 =======================================================================================================================
    Map a constant to our constant array. Constants are known values that don't require a register. Examples: r0 is
    always 0, the LUI instruction always generates constants. 1964 does a lot of constant detection.
 =======================================================================================================================
 */
void MapConst(x86regtyp *xMAP, int value)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifndef NO_CONSTS
	int where = CheckWhereIsMipsReg(xMAP->mips_reg);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	ConstMap[xMAP->mips_reg].IsMapped = 1;
	ConstMap[xMAP->mips_reg].value = value;
	if(where > -1)
	{
		x86reg[where].IsDirty = 0;
		FlushRegister(where);
	}

	ConstMap[0].value = 0;
#else
	if(xMAP->mips_reg != 0)
	{
		xMAP->IsDirty = 1;	/* bug fix */
		xMAP->Is32bit = 1;
		xMAP->NoNeedToLoadTheLo = 1;
		MapRegister(xMAP, 99, 99);
		MOV_ImmToReg(1, xMAP->x86reg, value);
	}
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MapOneConstantToRegister(x86regtyp *Conditions, int The2ndOneNotToFlush, int The3rdOneNotToFlush)
{
	/*~~~~~~~~~~~~~~~*/
	x86regtyp	xRJ[1];
	/*~~~~~~~~~~~~~~~*/

	xRJ->Is32bit = Conditions->Is32bit;

	if(ConstMap[Conditions->mips_reg].IsMapped == 1)
	{
		ConstMap[Conditions->mips_reg].IsMapped = 0;
		if(Conditions->mips_reg != 0)
		{
			xRJ->IsDirty = 1;
			Conditions->IsDirty = 1;
		}

		xRJ->mips_reg = Conditions->mips_reg;
		xRJ->NoNeedToLoadTheLo = 1;
		xRJ->NoNeedToLoadTheHi = 1;
		Conditions->NoNeedToLoadTheLo = 1;
		Conditions->NoNeedToLoadTheHi = 1;
#ifndef NO_CONSTS
		ConstMap[0].IsMapped = 1;
#endif
		MapRegister(xRJ, The2ndOneNotToFlush, The3rdOneNotToFlush);
#ifndef NO_CONSTS
		ConstMap[0].IsMapped = 1;
#endif
		ConstMap[0].value = 0;

		if(ConstMap[Conditions->mips_reg].value == 0)
		{
			XOR_Reg2ToReg1(1, xRJ->x86reg, xRJ->x86reg);
			if(x86reg[xRJ->x86reg].Is32bit == 0) XOR_Reg2ToReg1(1, xRJ->HiWordLoc, xRJ->HiWordLoc);
		}
		else if((_u32) ConstMap[Conditions->mips_reg].value < 32)
		{
			MOV_ImmToReg(1, xRJ->x86reg, (_u8) ConstMap[Conditions->mips_reg].value);

			if(x86reg[xRJ->x86reg].Is32bit == 0) XOR_Reg2ToReg1(1, xRJ->HiWordLoc, xRJ->HiWordLoc);
		}
		else
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			int k = (_s32) ConstMap[Conditions->mips_reg].value;
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			MOV_ImmToReg(1, xRJ->x86reg, ConstMap[Conditions->mips_reg].value);
			if(x86reg[xRJ->x86reg].Is32bit == 0)
			{
				_asm
				{
					sar k, 31
				}

				XOR_Reg2ToReg1(1, xRJ->HiWordLoc, xRJ->HiWordLoc);
				if(k != 0) DEC_Reg(1, xRJ->HiWordLoc);
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FlushOneConstant(int k)
{
	/*~~~~~~*/
	int where;
	/*~~~~~~*/

	CHECK_OPCODE_PASS;

	if(k == 0) return;
	if(ConstMap[k].IsMapped == 1)
	{
		FlushedRegistersMap[k].Is32bit = 1;
		if(k == 0) return;
		ConstMap[k].IsMapped = 0;
		where = CheckWhereIsMipsReg(k);
		if(where > -1)
		{
#ifdef DEBUG_REGCACHE
			DisplayError("Odd");
#endif
			x86reg[where].IsDirty = 0;
			FlushRegister(where);
		}
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			_u32	off = (_u32) & gHWS_GPR[0] + (k << 3);
			_s32	i = (_s32) ConstMap[k].value;
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			StoreImm_LO(k);
			_asm
			{
				sar i, 31
			}

			StoreImm_HI(k);
#ifndef NO_CONSTS
			ConstMap[0].IsMapped = 1;
#endif
		}
	}
}

/*
 =======================================================================================================================
    Store our constants to the mips gpr array in memory.
 =======================================================================================================================
 */
void FlushConstants(void)
{
	/*~~*/
	int k;
	/*~~*/

	for(k = 1; k < NUM_CONSTS; k++)
	{
		FlushOneConstant(k);
	}

#ifndef NO_CONSTS
	ConstMap[0].IsMapped = 1;
#endif
	ConstMap[0].value = 0;
}

/*
 =======================================================================================================================
    Store the dirty register to the mips gpr array in memory.
 =======================================================================================================================
 */
void WriteBackDirty(_int8 k)
{
	/* 32 bit register */
	if((x86reg[k].HiWordLoc == k))
	{
		if(x86reg[k].Is32bit != 1) DisplayError("Bug");

		StoreGPR_LO(k);

		if(currentromoptions.Assume_32bit == ASSUME_32BIT_NO)
		{
			SAR_RegByImm(1, k, 31);
			StoreGPR_HI(k);
		}

		FlushedRegistersMap[x86reg[k].mips_reg].Is32bit = 1;
	}
	else
	/* 64 bit register */
	{
		if(x86reg[k].Is32bit == 1) DisplayError("Bug");

		StoreGPR_LO(k);

		if
		(
			(x86reg[k].mips_reg == _t1) /* why 9 ? Rice: can ya help me debug it ? (in zelda2, MarioGolf, and many
										 * others that use the same OS) */
		||	(currentromoptions.Assume_32bit == ASSUME_32BIT_NO)
		)
		{
			StoreGPR_HI(k);
		}

		FlushedRegistersMap[x86reg[k].mips_reg].Is32bit = 0;
	}
}

/*
 =======================================================================================================================
    Unmaps a mips register from our x86reg[] register map. If the mips reg is dirty, it will be stored back to memory.
 =======================================================================================================================
 */
void FlushRegister(int k)
{
#ifdef DEBUG_REGCACHE
	/* paranoid error check */
	if(x86reg[k].HiWordLoc == -1) DisplayError("FlushRegister: The HiWord was not set!");
#endif
#ifdef R0_COMPENSATION
	/*
	 * Until we don't map all r0's, we'll need this check £
	 * if (ConstMap[0].IsMapped == 0) DisplayError("How did Const[0] get
	 * unmapped???");
	 */
	if(x86reg[k].mips_reg == 0) x86reg[k].IsDirty = 0;
#endif
	if(x86reg[k].IsDirty == 1)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		CHECK_OPCODE_PASS	WriteBackDirty((_s8) k);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	}

	x86reg_Delete(x86reg[k].HiWordLoc);
	x86reg_Delete(k);
}

/*
 =======================================================================================================================
    This function is just a combination of FlushRegister() and WriteBackDirty(). £
    It was written so that FlushAllRegisters() can interleave the register flushing £
    so that instructions will become paired for the processor.
 =======================================================================================================================
 */
void WriteBackDirtyLoOrHi(_int8 k, int Lo)
{
	/* 32 bit register */
#ifdef R0_COMPENSATION
	/*
	 * Until we don't map all r0's, we'll need this check £
	 * if (ConstMap[0].IsMapped == 0) DisplayError("How did Const[0] get
	 * unmapped???");
	 */
	if(x86reg[k].mips_reg == 0) x86reg[k].IsDirty = 0;
#endif
	if(x86reg[k].IsDirty == 1)
	{
		if((x86reg[k].HiWordLoc == k))
		{
			if(x86reg[k].Is32bit != 1) DisplayError("Bug");

			if(Lo == 0)
			{
				StoreGPR_LO(k);
			}
			else if(Lo == 1)
			{
				if(currentromoptions.Assume_32bit == ASSUME_32BIT_NO) SAR_RegByImm(1, k, 31);
			}
			else if(currentromoptions.Assume_32bit == ASSUME_32BIT_NO)
			{
				if(Lo == 2) StoreGPR_HI(k);
			}

			FlushedRegistersMap[x86reg[k].mips_reg].Is32bit = 1;
		}
		else
		/* 64 bit register */
		{
			if(x86reg[k].Is32bit == 1) DisplayError("Bug");

			if(!Lo)
			{
				StoreGPR_LO(k);
			}
			else if(currentromoptions.Assume_32bit == ASSUME_32BIT_NO)
			{
				if(Lo == 2) StoreGPR_HI(k);
			}

			FlushedRegistersMap[x86reg[k].mips_reg].Is32bit = 0;
		}
	}
}

/*
 =======================================================================================================================
    Function: FlushAllRegisters() Purpose: This function is used when you want to "flush" all the General Purpose
    Registers (GPR) that are mapped so that no registers are mapped. An unmapped array entry = -1. input: output:
 =======================================================================================================================
 */
void FlushAllRegisters(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	_int8	k;
	int		Lo = 0;
	int		iterations = 3;
	const	Num_x86regs = NUM_X86REGS;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS;
	FlushConstants();

	ThisYear = 2001;

	/* TODO: __LO, __HI flush 64bit. */
	if(currentromoptions.Assume_32bit == ASSUME_32BIT_NO) iterations = 3;
	for(Lo = 0; Lo < iterations; Lo++)
	{
		for(k = 0; k < Num_x86regs; k++)
		{
			if(ItIsARegisterNotToUse(k));
			else if(x86reg[k].mips_reg > -1)
				WriteBackDirtyLoOrHi(k, Lo);
		}
	}

	for(k = 0; k < Num_x86regs; k++)
	{
		if(ItIsARegisterNotToUse(k));
		else if(x86reg[k].mips_reg > -1)
		{
			x86reg_Delete(x86reg[k].HiWordLoc);
			x86reg_Delete(k);
		}
	}

	for(k = 1; k < NUM_CONSTS; k++) FlushedRegistersMap[k].Is32bit = 0;
}

int PopFlag[8];

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void PushMap(void)	/* Alternative to PUSHAD. */

/* This is ok to use in the regcache, but don't use it in the opcode debugger code! £Use PUSHAD() in opcode debugger code. */
{
#ifdef SAFE_PUSHPOP
	PUSHAD();
	return;
#endif
	PUSH_RegIfMapped(Reg_EAX);
	PUSH_RegIfMapped(Reg_ECX);
	PUSH_RegIfMapped(Reg_EDX);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void PUSH_RegIfMapped(int k)
{
	/*~~*/
	int i;
	/*~~*/

	PopFlag[k] = 1;

	if(x86reg[k].mips_reg != -1)
	{
		if(currentromoptions.Advanced_Block_Analysis == USEBLOCKANALYSIS_YES)
		{
			if(x86reg[k].mips_reg == IT_IS_HIGHWORD)
			{
				for(i = 0; i < 8; i++)
				{
					if(ItIsARegisterNotToUse(i));
					else if((x86reg[i].mips_reg > -1) && (x86reg[i].HiWordLoc == k))
					{
						if(ConstMap[x86reg[i].mips_reg].FinalAddressUsedAt >= gHWS_pc)
							PUSH_RegToStack((uint8) k);
						else if(x86reg[i].IsDirty == 1)
							PUSH_RegToStack((uint8) k);
						else
						{
							PopFlag[k] = 0;
							FlushRegister(i);
						}

						i = 9;	/* exit loop */
					}
				}
			}
			else if(ConstMap[x86reg[k].mips_reg].FinalAddressUsedAt >= gHWS_pc)
				PUSH_RegToStack((uint8) k);
			else if(x86reg[k].IsDirty == 1)
				PUSH_RegToStack((uint8) k);
			else
			{
				PopFlag[k] = 0;
				FlushRegister(k);
			}
		}
		else
			PUSH_RegToStack((uint8) k);
	}
	else
		PopFlag[k] = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void POP_RegIfMapped(int k)
{
	if(PopFlag[k]) POP_RegFromStack((uint8) k);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void PopMap(void)	/* Alternative to POPAD */
{
#ifdef SAFE_PUSHPOP
	POPAD();
	return;
#endif
	POP_RegIfMapped(Reg_EDX);
	POP_RegIfMapped(Reg_ECX);
	POP_RegIfMapped(Reg_EAX);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void FlushOneButNotThese3(int The1stOneNotToFlush, int The2ndOneNotToFlush, int The3rdOneNotToFlush)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	const _u8	Num_x86regs = NUM_X86REGS;
	_u32		paranoid = 0;
	_u32		init = 0;
	_u32		EarliestYear;
	_int32		k;
	_u32		MarkedForDeletion;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHECK_OPCODE_PASS

	/* Flush by least recently used */
	for(k = Num_x86regs - 1; k >= 0; k--)
	{
		if(ItIsARegisterNotToUse(k));
		else if
			(
				(x86reg[k].mips_reg > -1)
			&&	(x86reg[k].mips_reg != The1stOneNotToFlush)
			&&	(x86reg[k].mips_reg != The2ndOneNotToFlush)
			&&	(x86reg[k].mips_reg != The3rdOneNotToFlush)
			&&	(x86reg[k].mips_reg != 100)
			)				/* 100 is a Protected register */
		{
#ifdef DEBUG_REGCACHE
			if(x86reg[k].HiWordLoc == -1) DisplayError("FlushOne: The HiWord was not set!");
#endif
			if(ConstMap[x86reg[k].mips_reg].FinalAddressUsedAt <= gHWS_pc)
			{
				if(currentromoptions.Advanced_Block_Analysis == USEBLOCKANALYSIS_YES)
				{
					MarkedForDeletion = k;
					EarliestYear = x86reg[k].BirthDate;
					k = -1; /* exits */
				}
				else
					goto _next;
			}
			else
			{
_next:
				if(init == 0)
				{
					init = 1;
					MarkedForDeletion = k;
					EarliestYear = x86reg[k].BirthDate;
				}
				else if(x86reg[k].mips_reg == 0)
				{
					MarkedForDeletion = k;
					EarliestYear = x86reg[k].BirthDate;
					k = -1; /* exits */
				}
				else if(x86reg[k].BirthDate <= EarliestYear)
				{
					/*
					 * If they have same birth year, a nondirty has £
					 * priority over a dirty for flushing
					 */
					if(x86reg[k].BirthDate == EarliestYear)
					{
						if((x86reg[MarkedForDeletion].IsDirty == 1) && (x86reg[k].IsDirty == 0))
							MarkedForDeletion = k;
					}
					else
					{
						MarkedForDeletion = k;
						EarliestYear = x86reg[k].BirthDate;
					}
				}
			}

			paranoid = 1;
		}
	}

	if(ConstMap[x86reg[MarkedForDeletion].mips_reg].IsMapped == 1)
	{
#ifdef DEBUG_REGCACHE
		if(x86reg[MarkedForDeletion].mips_reg != 0)
			DisplayError("This should not happen. Mips = %d", x86reg[MarkedForDeletion].mips_reg);
#endif
		FlushOneConstant(x86reg[MarkedForDeletion].mips_reg);	/* Unmaps corr. reg also */
	}

	/* until we don't map all r0's, we'll need this */
#ifdef R0_COMPENSATION
	if(x86reg[MarkedForDeletion].mips_reg == 0) x86reg[MarkedForDeletion].IsDirty = 0;
	FlushRegister(MarkedForDeletion);
#endif
	if(paranoid == 0) DisplayError("Could not flush a register!!");
}