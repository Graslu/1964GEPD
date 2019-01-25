/*$T Analyze.c GC 1.136 02/28/02 18:46:24 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 This file does pre-compile analysis.
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
#include "../r4300i.h"
#include "OpcodeDebugger.h"
#include "../Compiler.h"
#include "regcache.h"

typedef enum
{
	I		= 1,	/* Immediate type. Uses rs, rt */
	R,				/* Register type. */
	B,				/* Branch type. Terminator. */
	J,				/* Jump type. Terminator. */
	LUI,
	SH,				/* shift. ex: Rd=RT<<sa */
	EMPTY,
	TODO
}
Instruction_Format;

typedef enum { AGI_STALL = 1, NOT_PAIRED } Optimization_Bit_Flags;

/*
 =======================================================================================================================
 =======================================================================================================================
 */

int I_Type(unsigned __int32 Instruction)
{
	return I;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int J_Type(unsigned __int32 Instruction)
{
	return J;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int R_Type(unsigned __int32 Instruction)
{
	return R;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int B_Type(unsigned __int32 Instruction)
{
	return B;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int SH_Type(unsigned __int32 Instruction)
{
	return SH;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int LUI_Type(unsigned __int32 Instruction)
{
	return LUI;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int Empty(unsigned __int32 Instruction)
{
	return EMPTY;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int ToDo(unsigned __int32 Instruction)
{
	return TODO;
}

int Special(unsigned __int32 Instruction);
int CP0_(unsigned __int32 Instruction);

int (*Get_Instruction_Format[64]) (unsigned __int32 Instruction) =
{
	Special,
	B_Type,
	J_Type,
	J_Type,
	B_Type,
	B_Type,
	B_Type,
	B_Type,

	/* 1 *2 J JAL BEQ BNE BLEZ BGTZ */
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	LUI_Type,

	/*
	 * ADDI ADDIU SLTI SLTIU ANDI ORI XORI LUI £
	 * NOT NOT < but here because incomplete
	 */
	CP0_,
	B_Type,
	Empty,
	Empty,
	B_Type,
	B_Type,
	B_Type,
	B_Type,

	/* CP0 *4 BEQL BNEL BLEZL BGTZL */
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	Empty,
	Empty,
	Empty,
	Empty,

	/* DADDI DADDIU LDL LDR */
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	I_Type,

	/* LB LH LWL LW LBU LHU LWR LWU */
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	I_Type,
	ToDo,

	/* SB SH SWL SW SDL SDR SWR CACHE */
	I_Type,
	ToDo,
	Empty,
	Empty,
	I_Type,
	ToDo,
	Empty,
	I_Type,

	/* LL LWC1 LLD LDC1 (LDC2) LD */
	I_Type,
	ToDo,
	Empty,
	Empty,
	I_Type,
	ToDo,
	Empty,
	I_Type

	/* SC SWC1 SCD SDC1 (SDC2) SD */
};

int (*Get_SPECIAL_Instruction_Format[64]) (uint32 Instruction) =
{
	J_Type,
	J_Type,
	SH_Type,
	SH_Type,
	R_Type,
	Empty,
	R_Type,
	R_Type,

	/* SLL SRL SRA SLLV SRLV SRAV */
	J_Type,
	J_Type,
	Empty,
	Empty,
	ToDo,
	ToDo,
	ToDo,
	ToDo,

	/* JR JALR SYSCALL BREAK SYNC */
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	R_Type,
	ToDo,
	R_Type,
	R_Type,

	/* MFHI MTHI MFLO MTLO DSLLV DSRLV DSRAV */
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	ToDo,

	/* MULT MULTU DIV DIVU DMULT DMULTU DDIV DDIVU */
	R_Type,
	R_Type,
	R_Type,
	R_Type,
	R_Type,
	R_Type,
	R_Type,
	R_Type,

	/* ADD ADDU SUB SUBU AND OR XOR NOR */
	Empty,
	Empty,
	R_Type,
	R_Type,
	R_Type,
	R_Type,
	R_Type,
	R_Type,

	/* SLT SLTU DADD DADDU DSUB DSUBU */
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	Empty,
	ToDo,
	ToDo,

	/* TGE TGEU TLT TLTU TEQ TNE */
	SH_Type,
	Empty,
	SH_Type,
	SH_Type,
	SH_Type,
	Empty,
	SH_Type,
	SH_Type

	/* DSLL DSRL DSRA DSLL32 DSRL32 DSRA32 */
};

int (*Get_CP0_Instruction_Format[32]) (uint32 Instruction) =
{
	ToDo,
	Empty,
	Empty,
	Empty,
	ToDo,
	Empty,
	Empty,
	Empty,

	/* MFC0 MTC0 */
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	Empty,
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	Empty,
	ToDo,
	ToDo,

	/* 1 */
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	ToDo,
	Empty,
	Empty,
	Empty
};

/*
 =======================================================================================================================
 =======================================================================================================================
 */

int Special(unsigned __int32 Instruction)
{
	return Get_SPECIAL_Instruction_Format[_FUNCTION_](Instruction);

	/* return J; */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int CP0_(unsigned __int32 Instruction)
{
	return Get_CP0_Instruction_Format[RS_BASE_FMT](Instruction);
}

#define Get_SA_FD(Instruction)			((unsigned __int32) ((Instruction >> 6) & 0x1F))
#define Get_RD_FS(Instruction)			((unsigned __int32) ((Instruction >> 11) & 0x1F))
#define Get_RT_FT(Instruction)			((unsigned __int32) ((Instruction >> 16) & 0x1F))
#define Get_RS_BASE_FMT(Instruction)	((unsigned __int32) ((Instruction >> 21) & 0x1F))

int Instruction_Order[0x1000];	/* 4K should do it */

/*
 =======================================================================================================================
    This will likely become an obsolete function. The original intent of this function was to reorder instructions in a
    block to reduce register misses, AGI stalls, and to improve pairing of x86 instructions. The trouble with it is
    that it is a pre-compile step, and analyzes MIPS instructions only, so it is not very good. Some kind of
    post-compile analysis would be a better idea such as compiling into intermediate x86 asm source and then analyzing
    that before building the machine code.
 =======================================================================================================================
 */
void Reorder_Instructions_In_Block(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	unsigned _int32 Instruction[7];
	unsigned _int32 Instruction_Format[3];
	unsigned _int32 tempPC = gHWS_pc;
	uint32			rt[7];
	uint32			rs[7];
	uint32			rd[7];
	int				Unoptimized = 0;
	int				EndOfBlock = 0;
	int				k;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/* return; */
	if(ITLB_Error) return;

	memset(Instruction_Order, 0, sizeof(Instruction_Order));			/* Todo: memset only by previous block size!. */
	for(k = 0; !EndOfBlock; gHWS_pc += 4, k++)
	{
		/* level1: */
		Instruction[0] = DynaFetchInstruction2(gHWS_pc + (Instruction_Order[k] << 2));
		Instruction_Format[0] = Get_Instruction_Format[Instruction[0] >> 26](Instruction[0]);

		rd[0] = 100;
		rd[1] = 101;
		rd[2] = 102;

		switch(Instruction_Format[0])
		{
		case R:
		case I:
		case SH:
			if(Instruction_Format[0] == I)
			{
				rt[0] = Get_RT_FT(Instruction[0]);						/* Target */
				rs[0] = Get_RS_BASE_FMT(Instruction[0]);				/* Source */
			}
			else if(Instruction_Format[0] == SH)
			{
				rt[0] = Get_RD_FS(Instruction[0]);						/* Target */
				rs[0] = Get_RT_FT(Instruction[0]);						/* Source */
			}
			else if(Instruction_Format[0] == R)
			{
				rt[0] = Get_RD_FS(Instruction[0]);						/* Target */
				rs[0] = Get_RS_BASE_FMT(Instruction[0]);				/* Source */
				rd[0] = Get_RT_FT(Instruction[0]);						/* Operand2 */
			}

			/* level2: */
			Instruction[1] = DynaFetchInstruction2(gHWS_pc + 4 + (Instruction_Order[k + 1] << 2));
			Instruction_Format[1] = Get_Instruction_Format[Instruction[1] >> 26](Instruction[1]);
			switch(Instruction_Format[1])
			{
			case R:
			case I:
			case SH:
				if(Instruction_Format[1] == I)
				{
					rt[1] = Get_RT_FT(Instruction[1]);					/* Target */
					rs[1] = Get_RS_BASE_FMT(Instruction[1]);			/* Source */
				}
				else if(Instruction_Format[1] == SH)
				{
					rt[1] = Get_RD_FS(Instruction[1]);					/* Target */
					rs[1] = Get_RT_FT(Instruction[1]);					/* Source */
				}
				else if(Instruction_Format[1] == R)
				{
					rt[1] = Get_RD_FS(Instruction[1]);					/* Target */
					rs[1] = Get_RS_BASE_FMT(Instruction[1]);			/* Source */
					rd[1] = Get_RT_FT(Instruction[1]);					/* Operand2 */
				}

				if(rt[0] != rt[1]) Unoptimized |= NOT_PAIRED;

				if(Unoptimized)
				{
					/* level3: */
					Instruction[2] = DynaFetchInstruction2(gHWS_pc + 8 + (Instruction_Order[k + 2] << 2));
					Instruction_Format[2] = Get_Instruction_Format[Instruction[2] >> 26](Instruction[2]);
					switch(Instruction_Format[2])
					{
					case I:
					case SH:
					case R:
						if(Instruction_Format[2] == I)
						{
							rt[2] = Get_RT_FT(Instruction[2]);			/* Target */
							rs[2] = Get_RS_BASE_FMT(Instruction[2]);	/* Source */
						}
						else if(Instruction_Format[2] == SH)
						{
							rt[2] = Get_RD_FS(Instruction[2]);			/* Target */
							rs[2] = Get_RT_FT(Instruction[2]);			/* Source */
						}
						else if(Instruction_Format[2] == R)
						{
							rt[2] = Get_RD_FS(Instruction[2]);			/* Target */
							rs[2] = Get_RS_BASE_FMT(Instruction[2]);	/* Source */
							rd[2] = Get_RT_FT(Instruction[2]);			/* Operand2 */
						}

						/*
						 * Search for next instruction to use. Right now just use [2]. £
						 * Check for no conflict in previous instruction(s)
						 */
						{
							if
							(
								(rt[2] == rt[0])
							&&	(rd[2] != rd[1])
							&&	(rd[2] != rs[1])
							&&	(rd[2] != rt[1])
							&&	(rs[2] != rd[1])
							&&	(rs[2] != rt[1])
							&&	(rt[2] != rd[1])
							&&	(rt[2] != rs[1])
							&&	(rt[2] != rt[1])
							)
							{
								/* swap. */
								Instruction_Order[k + 2] = Instruction_Order[k + 1] - 1;
								Instruction_Order[k + 1] += 1;
							}
						}
						break;
					}
				}
				break;
			}
			break;
		case LUI:
			break;
		case TODO:
		case EMPTY:
		case J:
		case B:
			EndOfBlock = 1;
			break;
		}

		Unoptimized = 0;
	}

	gHWS_pc = tempPC;
}

extern uint32	FetchInstruction(void);

/*
 =======================================================================================================================
    This function will setup to use load-and-execute x86 instructions when it is appropriate to use them. It also calls
    Reorder_Instructions_In_Block().
 =======================================================================================================================
 */
extern void __cdecl error(char *Message, ...);
void AnalyzeBlock(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~*/
	uint32	Instruction;
	uint32	tempPC = gHWS_pc;
	uint32	EndOfBlock = 0;
	uint32	fmt = 0xffffffff;
	_int32	k;
	/*~~~~~~~~~~~~~~~~~~~~~*/

	for(k = 33; k >= 0; k--)
	{
		ConstMap[k].FinalAddressUsedAt = 0xffffffff;
	}

	if(ITLB_Error) return;

//	/*
//	 * Rice: This is still in development, but you can uncomment the next line to try
//	 * it out. It's cool. So far only a few games work with it (somewhat).
//	 Reorder_Instructions_In_Block();
//	 */
	while(!EndOfBlock)
	{
		/*~~~~~~~*/
		int opcode;
		/*~~~~~~~*/

_start:
		Instruction = DynaFetchInstruction2(gHWS_pc);
		opcode = _OPCODE_;
		if((opcode >= 4) && (opcode <= 7))						/* beq, bne, blez, bgtz */
		{
			/* it is assumed that more times than not, delay slots will be executed. */
			if(!EndOfBlock)
			{
				ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
				ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
				gHWS_pc += 4;
				EndOfBlock = 1;
				goto _start;
			}
		}
		else if((opcode >= 20) && (opcode <= 23))				/* beql, bnel, blezl, bgtzl */
		{
			/* it is assumed that more times than not, delay slots will be executed. */
			if(!EndOfBlock)
			{
				ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
				ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
				gHWS_pc += 4;
				EndOfBlock = 1;
				goto _start;
			}
		}
		else if((opcode == 2) || (opcode == 3))
		{
			/* delay slots always executed. */
			if(!EndOfBlock)
			{
				gHWS_pc += 4;
				EndOfBlock = 1;
				goto _start;
			}
		}
		else if(opcode == 15)
			ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
		else if(opcode >= 32 && opcode <= 49)					/* load/stores */
		{
			ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
			ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
		}
		else if(opcode >= 52 && opcode <= 63)					/* load/stores */
		{
			ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
			ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
		}
		else if(opcode >= 8 && opcode < 15)						/* imm type */
		{
			ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
			ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
		}
		else if(opcode == 17)									/* COP1 */
		{
			/* do nothing */
		}
		else
		{
			switch(opcode)
			{
			case 0:
				fmt = Instruction & 0x1f;
				if((fmt >= 40) && (fmt <= 47))					/* gates */
				{
					ConstMap[RD_FS].FinalAddressUsedAt = gHWS_pc;
					ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
					ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
				}
				else if((fmt == 0) || (fmt == 2) || (fmt == 3)) /* sll, srl, sra */
				{
					if(Instruction != 0)
					{
						ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
						ConstMap[RD_FS].FinalAddressUsedAt = gHWS_pc;
					}
				}
				else if((fmt == 4) || (fmt == 6) || (fmt == 7)) /* sllv, srlv, srav */
				{
					ConstMap[RD_FS].FinalAddressUsedAt = gHWS_pc;
					ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
					ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
				}
				else if((fmt == 25)||(fmt==26)||(fmt==29)||(fmt==31)) /* div, divu, dmultu, ddivu*/
				{
					ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
					ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
				}

				else if((fmt == 18)) /* mflo */
				{
					ConstMap[RD_FS].FinalAddressUsedAt = gHWS_pc;
					ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
				}
				
				else if ((fmt==10)||(fmt==11)||(fmt==5)||(fmt==1))
				{
					//unused. do nothing.
				}
				else if ((fmt==8)||(fmt==9)) //jal, jalr
				{
					if(EndOfBlock) break;
					ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
					gHWS_pc += 4;
					EndOfBlock = 1;
					goto _start;
					break;
				}
				else
				{
					//error("%d", fmt);
					goto _invalidate;
				}
				break;
			case 1: /* Regimm Instructions */
				/* it is assumed that more times than not, delay slots will be executed. */
				if(EndOfBlock) break;
				ConstMap[RS_BASE_FMT].FinalAddressUsedAt = gHWS_pc;
				ConstMap[RT_FT].FinalAddressUsedAt = gHWS_pc;
				gHWS_pc += 4;
				EndOfBlock = 1;
				goto _start;
				break;

				/* just invalidate our analysis. We don't do enough of it yet :) */

			case 16: //COP0 instruction
			{
				if (RS_BASE_FMT==16)
					if ((_FUNCTION_) == 24) //eret
						goto _invalidate;
			}
			break;
			default:
_invalidate:
				/* DisplayError("%d %d", opcode, fmt); */
				if(!EndOfBlock)
					for(k = 33; k >= 0; k--)
					{
						ConstMap[k].FinalAddressUsedAt = 0xFFFFFFFF;
					}

				EndOfBlock = 1;
				break;
			}
		}

		gHWS_pc += 4;
	}

	gHWS_pc = tempPC;
}
