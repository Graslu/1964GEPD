/*$T FPU.c GC 1.136 03/09/02 17:28:59 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Interpretive functions for Coprocessor1 (FPU)
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
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "r4300i.h"
#include "hardware.h"
#include "memory.h"
#include "win32/windebug.h"
#include "timer.h"
#include "interrupt.h"

/* Rounding control of FPU */
#define FPCSR_RM_MASK	0x00000003	/* rounding mode mask */
#define FPCSR_RM_RN		0x00000000	/* round to nearest */
#define FPCSR_RM_RZ		0x00000001	/* round to zero */
#define FPCSR_RM_RP		0x00000002	/* round to positive infinity */
#define FPCSR_RM_RM		0x00000003	/* round to negative infinity */
#define RM_METHOD		(cCON31 & FPCSR_RM_MASK)
#define SAVE_RM

#ifdef SAVE_RM
#define SET_ROUNDING	_control87(RM_METHOD, 0x00000300);
#else
#define SET_ROUNDING
#endif
#define COP1_CONDITION_BIT		0x00800000
#define NOT_COP1_CONDITION_BIT	0xFF7FFFFF

#ifdef DEBUG_COMMON
#define CHECK_ODD_FPR_REG
#endif
#ifdef CHECK_ODD_FPR_REG
#define CHK_ODD_FPR_1_REG(reg1) \
	if((gHWS_COP0Reg[STATUS] & SR_FR) == 0 && ((reg1) & 1)) \
	{ \
		if(debugoptions.debug_64bit_fpu) TRACE1("Using odd FPU reg %d", reg1); \
		return; \
	}
#define CHK_ODD_FPR_2_REG(reg1, reg2) \
	if((gHWS_COP0Reg[STATUS] & SR_FR) == 0 && (((reg1) & 1) || ((reg2) & 1))) \
	{ \
		if(debugoptions.debug_64bit_fpu) TRACE0("Using odd FPU reg"); \
		return; \
	}
#define CHK_ODD_FPR_3_REG(reg1, reg2, reg3) \
	if \
	( \
		(gHWS_COP0Reg[STATUS] & SR_FR) == 0 \
	&&	(((reg1) & 1) || ((reg2) & 1) || ((reg3) & 1)) \
	) \
	{ \
		if(debugoptions.debug_64bit_fpu) TRACE0("Odd FPU reg"); \
		return; \
	}
#else
#define CHK_ODD_FPR_1_REG(reg1)
#define CHK_ODD_FPR_2_REG(reg1, reg2)
#define CHK_ODD_FPR_3_REG(reg1, reg2, reg3)
#endif
#define CHK_64BITMODE(Name)

extern	r4300i_do_speedhack(void);
#ifdef DEBUG_COMMON
void	DebugPrintPC(uint32 thePC);
#endif

/*
 =======================================================================================================================
    Write a 64bit MIPS® floating point register to memory
 =======================================================================================================================
 */
__forceinline void write_64bit_fpu_reg(int regno, uint32 *val)
{
	gHWS_fpr32[regno] = val[0];
	gHWS_fpr32[regno + FR_reg_offset] = val[1];
}

/*
 =======================================================================================================================
    Read a 64bit MIPS® floating point register from memory
 =======================================================================================================================
 */
__forceinline uint64 read_64bit_fpu_reg(int regno)
{
	/*~~~~~~~~~~~~*/
	uint64	tempval;
	/*~~~~~~~~~~~~*/

	*(((uint32 *) &tempval) + 1) = gHWS_fpr32[regno + FR_reg_offset];
	*(uint32 *) &tempval = gHWS_fpr32[regno];
	return tempval;
}

#define ENABLE_CHECK_FPU_USABILITY
#ifdef ENABLE_CHECK_FPU_USABILITY
#define CheckFPU_Usablity(addr) \
	if((gHWS_COP0Reg[STATUS] & 0x20000000) == 0) TriggerFPUUnusableException();
#else ENABLE_CHECK_FPU_USABILITY
#define CheckFPU_Usablity(addr)
#endif ENABLE_CHECK_FPU_USABILITY
uint32	FR_reg_offset = 1;

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void r4300i_COP1_add_s(uint32 Instruction)
{
	CHK_ODD_FPR_3_REG(RD_FS, SA_FD, RT_FT);
	SET_ROUNDING;
	(*((float *) &cFD)) = (*((float *) &cFS)) + (*((float *) &cFT));
	SAVE_OP_COUNTER_INCREASE_INTERPRETER(2);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_sub_s(uint32 Instruction)
{
	CHK_ODD_FPR_3_REG(RD_FS, SA_FD, RT_FT);
	SET_ROUNDING;
	(*((float *) &cFD)) = (*((float *) &cFS)) - (*((float *) &cFT));
	SAVE_OP_COUNTER_INCREASE_INTERPRETER(2);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_mul_s(uint32 Instruction)
{
	CHK_ODD_FPR_3_REG(RD_FS, SA_FD, RT_FT);
	SET_ROUNDING;
	(*((float *) &cFD)) = (*((float *) &cFS)) * (*((float *) &cFT));
	SAVE_OP_COUNTER_INCREASE_INTERPRETER(4);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_div_s(uint32 Instruction)
{
	CHK_ODD_FPR_3_REG(RD_FS, SA_FD, RT_FT);
	SET_ROUNDING;
	(*((float *) &cFD)) = (*((float *) &cFS)) / (*((float *) &cFT));
	SAVE_OP_COUNTER_INCREASE_INTERPRETER(12);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_add_d(uint32 Instruction)
{
	CHK_ODD_FPR_3_REG(RD_FS, SA_FD, RT_FT);
	SET_ROUNDING;
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint64	u1 = read_64bit_fpu_reg(RD_FS);
		uint64	u2 = read_64bit_fpu_reg(RT_FT);
		double	val3 = (*(double *) &u1) + (*(double *) &u2);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		write_64bit_fpu_reg(SA_FD, (uint32 *) &val3);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_sub_d(uint32 Instruction)
{
	CHK_ODD_FPR_3_REG(RD_FS, SA_FD, RT_FT);
	SET_ROUNDING;
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint64	u1 = read_64bit_fpu_reg(RD_FS);
		uint64	u2 = read_64bit_fpu_reg(RT_FT);
		double	val3 = (*(double *) &u1) - (*(double *) &u2);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		write_64bit_fpu_reg(SA_FD, (uint32 *) (&val3));
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_mul_d(uint32 Instruction)
{
	CHK_ODD_FPR_3_REG(RD_FS, SA_FD, RT_FT);
	SET_ROUNDING;
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint64	u1 = read_64bit_fpu_reg(RD_FS);
		uint64	u2 = read_64bit_fpu_reg(RT_FT);
		double	val3 = (*(double *) &u1) * (*(double *) &u2);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		write_64bit_fpu_reg(SA_FD, (uint32 *) (&val3));
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_div_d(uint32 Instruction)
{
	CHK_ODD_FPR_3_REG(RD_FS, SA_FD, RT_FT);
	SET_ROUNDING;
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint64	u1 = read_64bit_fpu_reg(RD_FS);
		uint64	u2 = read_64bit_fpu_reg(RT_FT);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		if(*(double *) &u2 != 0)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			double	val3 = (*(double *) &u1) / (*(double *) &u2);
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			write_64bit_fpu_reg(SA_FD, ((uint32 *) (&val3)));
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_abs_s(uint32 Instruction)
{
	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	SET_ROUNDING;

	*((float *) &cFD) = (float) fabs((double) *((float *) &cFS));
	SAVE_OP_COUNTER_INCREASE_INTERPRETER(27);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_sqrt_s(uint32 Instruction)
{
	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	SET_ROUNDING;
	*((float *) &cFD) = (float) sqrt((double) *((float *) &cFS));
	SAVE_OP_COUNTER_INCREASE_INTERPRETER(1);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_neg_s(uint32 Instruction)
{
	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	SET_ROUNDING;
	*((float *) &cFD) = -(*((float *) &cFS));
	SAVE_OP_COUNTER_INCREASE_INTERPRETER(1);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_abs_d(uint32 Instruction)
{
	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	SET_ROUNDING;
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint64	u1 = read_64bit_fpu_reg(RD_FS);
		double	val3 = fabs(*(double *) &u1);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		write_64bit_fpu_reg(SA_FD, (uint32 *) (&val3));
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_sqrt_d(uint32 Instruction)
{
	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	SET_ROUNDING;
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint64	u1 = read_64bit_fpu_reg(RD_FS);
		double	val3 = sqrt(*(double *) &u1);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		write_64bit_fpu_reg(SA_FD, (uint32 *) (&val3));
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_neg_d(uint32 Instruction)
{
	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	SET_ROUNDING;
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint64	u1 = read_64bit_fpu_reg(RD_FS);
		double	val1 = -*((double *) &u1);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		write_64bit_fpu_reg(SA_FD, (uint32 *) (&val1));
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_bc1f(uint32 Instruction)
{
	CHK_64BITMODE("bc1f") if((((uint32) cCON31 & 0x00800000)) == 0)
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
void r4300i_COP1_bc1t(uint32 Instruction)
{
	CHK_64BITMODE("bc1t") if((((uint32) cCON31 & 0x00800000)) != 0)
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
void r4300i_COP1_bc1fl(uint32 Instruction)
{
	CHK_64BITMODE("bc1fl") if((((uint32) cCON31 & 0x00800000)) == 0)
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
void r4300i_COP1_bc1tl(uint32 Instruction)
{
	CHK_64BITMODE("bc1tl") if((((uint32) cCON31 & 0x00800000)) != 0)
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
void r4300i_C_cond_fmt_s(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	float	fcFS32, fcFT32;
	BOOL	less, equal, unordered, cond, cond0, cond1, cond2, cond3;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, RT_FT);

	cond0 = (Instruction) & 0x1;
	cond1 = (Instruction >> 1) & 0x1;
	cond2 = (Instruction >> 2) & 0x1;
	cond3 = (Instruction >> 3) & 0x1;
	fcFS32 = *((float *) &cFS);
	fcFT32 = *((float *) &cFT);

	if(_isnan(fcFS32) || _isnan(fcFT32))
	{
		less = FALSE;
		equal = FALSE;
		unordered = TRUE;

		if(cond3)
		{
			/* Fire invalid operation exception */
			return;
		}
	}
	else
	{
		less = (fcFS32 < fcFT32);
		equal = (fcFS32 == fcFT32);
		unordered = FALSE;
	}

	cond = ((cond0 && unordered) || (cond1 && equal) || (cond2 && less));

	if(cond)
		cCON31 |= COP1_CONDITION_BIT;
	else
		cCON31 &= ~COP1_CONDITION_BIT;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_cond_fmt_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	double	fcFS32, fcFT32;
	BOOL	less, equal, unordered, cond, cond0, cond1, cond2, cond3;
	uint64	val1, val2;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, RT_FT);

	cond0 = (Instruction) & 0x1;
	cond1 = (Instruction >> 1) & 0x1;
	cond2 = (Instruction >> 2) & 0x1;
	cond3 = (Instruction >> 3) & 0x1;
	val1 = read_64bit_fpu_reg(RD_FS);
	val2 = read_64bit_fpu_reg(RT_FT);
	fcFS32 = *((double *) &val1);
	fcFT32 = *((double *) &val2);

	if(_isnan(fcFS32) || _isnan(fcFT32))
	{
		less = FALSE;
		equal = FALSE;
		unordered = TRUE;

		if(cond3)
		{
			/* Fire invalid operation exception */
			return;
		}
	}
	else
	{
		less = (fcFS32 < fcFT32);
		equal = (fcFS32 == fcFT32);
		unordered = FALSE;
	}

	cond = ((cond0 && unordered) || (cond1 && equal) || (cond2 && less));

	if(cond)
		cCON31 |= COP1_CONDITION_BIT;
	else
		cCON31 &= ~COP1_CONDITION_BIT;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_EQ_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_UEQ_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_EQ_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_UEQ_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_LT_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_NGE_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_LT_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_NGE_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_LE_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_NGT_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_LE_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_NGT_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
    we won't need to confirm mode for control registers...
 =======================================================================================================================
 */
void r4300i_COP1_cfc1(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	rt_ft = RT_FT;
	uint32	rd_fs = RD_FS;
	/*~~~~~~~~~~~~~~~~~~*/

	if(rd_fs == 0 || rd_fs == 31)
	{
		gHWS_GPR[rt_ft] = (__int64) (__int32) cCONFS;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_ctc1(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	rt_ft = RT_FT;
	uint32	rd_fs = RD_FS;
	/*~~~~~~~~~~~~~~~~~~*/

	if((rd_fs == 31) && (cCON31 != (uint32) gHWS_GPR[rt_ft]))					/* Only Control Register 31 is writeable */
	{
		/* Check if the automatic round setting changes */
		if(((uint32) gHWS_GPR[rt_ft] ^ cCON31) & 0x00000003)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			uint32	newsetting = ((uint32) gHWS_GPR[rt_ft] & 0x00000003) << 8;	/* Set 80x87 round setting bits */
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			_control87(newsetting, 0x00000300);

			/*
			 * TRACE1("Change FPU Rounding Setting %08X",
			 * newsetting);//((uint32)gHWS_GPR[rt_ft]&0x00000003));
			 */
		}

		/*
		 * Check if exceptions are enabled £
		 * Need to set 80x87 control register to auto round and precision control
		 */
		cCON31 = (uint32) gHWS_GPR[rt_ft];
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_cvtd_s(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	float	f1 = *((float *) &cFS);
	double	val2 = (double) (f1);;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, (uint32 *) &val2);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_cvtd_w(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	__int32 i1 = *((__int32 *) &cFS);
	double	val2 = (double) i1;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, (uint32 *) &val2);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_cvtd_l(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	double	val2 = (double) (*((_int64 *) &val));
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, (uint32 *) &val2);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_cvts_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	double	d1 = *((double *) &val);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	*((float *) &cFD) = (float) d1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_cvts_l(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	*((float *) &cFD) = (float) (*((_int64 *) &val));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_cvtw_d(uint32 Instruction)
{
#ifdef SAVE_RM
	switch(RM_METHOD)
	{
	case FPCSR_RM_RN:	/* 0x00000000 round to nearest */
		r4300i_COP1_roundw_d(Instruction);
		break;
	case FPCSR_RM_RZ:	/* 0x00000001 round to zero */
		r4300i_COP1_truncw_d(Instruction);
		break;
	case FPCSR_RM_RP:	/* 0x00000002 round to positive infinity */
		r4300i_COP1_ceilw_d(Instruction);
		break;
	case FPCSR_RM_RM:	/* 0x00000003 round to negative infinity */
	default:
		r4300i_COP1_floorw_d(Instruction);
		break;
	}

#else
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	double	d1 = *((double *) &val);
	__int32 i1 = (__int32) d1;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/* ((__int32 *)&cFD) = (__int32)(*((double *)&val)); */
	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	*((__int32 *) &cFD) = (__int32) d1;
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_cvtl_s(uint32 Instruction)
{
#ifdef SAVE_RM
	switch(RM_METHOD)
	{
	case FPCSR_RM_RN:	/* 0x00000000 round to nearest */
		r4300i_COP1_roundl_s(Instruction);
		break;
	case FPCSR_RM_RZ:	/* 0x00000001 round to zero */
		r4300i_COP1_truncl_s(Instruction);
		break;
	case FPCSR_RM_RP:	/* 0x00000002 round to positive infinity */
		r4300i_COP1_ceill_s(Instruction);
		break;
	case FPCSR_RM_RM:	/* 0x00000003 round to negative infinity */
	default:
		r4300i_COP1_floorl_s(Instruction);
		break;
	}

#else
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	float	val = *((float *) &cFS);
	__int64 val2 = (__int64) val;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, *(uint64 *) &val2);
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_cvtl_d(uint32 Instruction)
{
#ifdef SAVE_RM
	switch(RM_METHOD)
	{
	case FPCSR_RM_RN:	/* 0x00000000 round to nearest */
		r4300i_COP1_roundl_d(Instruction);
		break;
	case FPCSR_RM_RZ:	/* 0x00000001 round to zero */
		r4300i_COP1_truncl_d(Instruction);
		break;
	case FPCSR_RM_RP:	/* 0x00000002 round to positive infinity */
		r4300i_COP1_ceill_d(Instruction);
		break;
	case FPCSR_RM_RM:	/* 0x00000003 round to negative infinity */
	default:
		r4300i_COP1_floorl_d(Instruction);
		break;
	}

#else
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	__int64 val2 = (__int64) val;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, *(uint64 *) &val2);
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_cvts_w(uint32 Instruction)
{
	CHK_64BITMODE("cvts_w") * ((float *) &cFD) = (float) (*((_int32 *) &cFS));
	SAVE_OP_COUNTER_INCREASE_INTERPRETER(3);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_cvtw_s(uint32 Instruction)
{
#ifdef SAVE_RM
	switch(RM_METHOD)
	{
	case FPCSR_RM_RN:	/* 0x00000000 round to nearest */
		r4300i_COP1_roundw_s(Instruction);
		break;
	case FPCSR_RM_RZ:	/* 0x00000001 round to zero */
		r4300i_COP1_truncw_s(Instruction);
		break;
	case FPCSR_RM_RP:	/* 0x00000002 round to positive infinity */
		r4300i_COP1_ceilw_s(Instruction);
		break;
	case FPCSR_RM_RM:	/* 0x00000003 round to negative infinity */
	default:
		r4300i_COP1_floorw_s(Instruction);
		break;
	}

#else
	CHK_64BITMODE("cvtw_s") * ((__int32 *) &cFD) = (__int32) (*((float *) &cFS));
	SAVE_OP_COUNTER_INCREASE_INTERPRETER(2);
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_mtc1(uint32 Instruction)
{
	CHK_64BITMODE("mtc1") * (uint32 *) &cFS = (uint32) gRT;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_mfc1(uint32 Instruction)
{
	CHK_64BITMODE("mfc1")

	(*(_int64 *) &gRT) = (__int64) (*((_int32 *) &cFS));

	SAVE_OP_COUNTER_INCREASE_INTERPRETER(2);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_dmtc1(uint32 Instruction)
{
	write_64bit_fpu_reg(RD_FS, (uint32 *) &gRT);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_dmfc1(uint32 Instruction)
{
	CHK_64BITMODE("dmfc1") 
	
	gRT = read_64bit_fpu_reg(RD_FS);
	SAVE_OP_COUNTER_INCREASE_INTERPRETER(2);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_mov_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	read_64 = read_64bit_fpu_reg(RD_FS);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	write_64bit_fpu_reg(SA_FD, (uint32 *) &read_64);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_COP1_mov_s(uint32 Instruction)
{
	*(uint32 *) &cFD = *(uint32 *) &cFS;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_lwc1(uint32 Instruction)
{
	LOAD_TLB_FUN
	CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x3, "LWC1", EXC_RADE) * (uint32 *) 
	&cFT = MEM_READ_UWORD(QuerAddr);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_swc1(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	STORE_TLB_FUN CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x3, "SWC1", EXC_WADE)
	/*
	 * I have some problems here. I should pass the rt_ft value to the
	 * memory_write_functions, £
	 * and the memory_write_functions will read the real value from GPR[rf_ft]. well
	 * here we £
	 * are dealing with FPR, not GPR, so I just pass rt_ft and try to believe that
	 * SWC1 will never £
	 * be used to access an io register or something like flashram command/status
	 * registers £
	 * I pass rt_ft because this will never be used anyway.
	 */
	* (PMEM_WRITE_UWORD(QuerAddr)) = (uint32) gHWS_fpr32[rt_ft];
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_ldc1(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_1_REG(RT_FT);

	QuerAddr = (uint32) ((_int32) gBASE + (_int32) OFFSET_IMMEDIATE);
	LOAD_TLB_TRANSLATE_ADDR_IF_NEEDED(QuerAddr);
	CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x7, "ldc1", EXC_RADE)
	{
		/*~~~~~~~~~~~*/
		uint32	reg[2];
		/*~~~~~~~~~~~*/

		reg[0] = MEM_READ_UWORD(QuerAddr + 4);
		reg[1] = MEM_READ_UWORD(QuerAddr);
		write_64bit_fpu_reg(RT_FT, (uint32 *) &reg[0]);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_sdc1(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~*/
	uint32	QuerAddr;
	uint32	rt_ft = RT_FT;
	/*~~~~~~~~~~~~~~~~~~*/

	CHK_64BITMODE("sdc1");

	CHK_ODD_FPR_1_REG(RT_FT);

	QuerAddr = (uint32) ((_int32) gBASE + (_int32) OFFSET_IMMEDIATE);
	STORE_TLB_TRANSLATE_ADDR_IF_NEEDED(QuerAddr);
	CHECKING_ADDR_ALIGNMENT(QuerAddr, 0x7, "sdc1", EXC_WADE)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint32	reg[2];
		uint32	temp = (uint32) gHWS_GPR[rt_ft];
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		*(uint64 *) &reg[0] = read_64bit_fpu_reg(rt_ft);
		*(uint32 *) &gHWS_GPR[rt_ft] = reg[0];
		*(PMEM_WRITE_UWORD((QuerAddr + 4))) = reg[0];
		*(uint32 *) &gHWS_GPR[rt_ft] = reg[1];
		*(PMEM_WRITE_UWORD(QuerAddr)) = reg[1];
		*(uint32 *) &gHWS_GPR[rt_ft] = temp;
	}
}

/*
 =======================================================================================================================
    Format: TRUNC.W.S fd, fs Purpose: To convert an FP value to 32-bit fixed-point, rounding toward zero.
 =======================================================================================================================
 */
void r4300i_COP1_truncw_s(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	float	tempf = *((float *) &cFS);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_64BITMODE("truncw_s") CHK_ODD_FPR_2_REG(RD_FS, SA_FD);

	if(tempf >= 0)
		*((__int32 *) &cFD) = (__int32) (tempf);
	else
		*((__int32 *) &cFD) = -((__int32) (-tempf));
}

/*
 =======================================================================================================================
    Format: TRUNC.W.D fd, fs Purpose: To convert an FP value to 32-bit fixed-point, rounding toward zero.
 =======================================================================================================================
 */
void r4300i_COP1_truncw_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	double	tempd = *((double *) &val);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(tempd >= 0)
		*((__int32 *) &cFD) = (__int32) (tempd);
	else
		*((__int32 *) &cFD) = -((__int32) (-tempd));;
}

/*
 =======================================================================================================================
    Format: TRUNC.L.S fd, fs Purpose: To convert an FP float value to 64-bit fixed-point, rounding toward zero.
 =======================================================================================================================
 */
void r4300i_COP1_truncl_s(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	float	tempf = *((float *) &cFS);
	__int64 templ;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);

	if(tempf >= 0)
		templ = (__int64) (tempf);
	else
		templ = -((__int64) (-tempf));

	write_64bit_fpu_reg(SA_FD, (uint32 *) &templ);
}

/*
 =======================================================================================================================
    Format: TRUNC.L.D fd, fs Purpose: To convert an FP double float value to 64-bit fixed-point, rounding toward zero.
 =======================================================================================================================
 */
void r4300i_COP1_truncl_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	double	tempf = *(double *) &val;
	__int64 templ;;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);

	if(tempf >= 0)
	{
		templ = (__int64) tempf;
	}
	else
	{
		templ = -(__int64) (-tempf);
	}

	write_64bit_fpu_reg(SA_FD, (uint32 *) &templ);
}

/*
 =======================================================================================================================
    Format: FLOOR.L.S fd, fs Purpose: To convert an FP float value to 64-bit fixed-point, rounding down.
 =======================================================================================================================
 */
void r4300i_COP1_floorl_s(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	float	tempf = *((float *) &cFS);
	__int64 templ = (__int64) floor((double) tempf);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, (uint32 *) &templ);
}

/*
 =======================================================================================================================
    Format: FLOOR.L.D fd, fs Purpose: To convert an FP double float value to 64-bit fixed-point, rounding down.
 =======================================================================================================================
 */
void r4300i_COP1_floorl_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	__int64 templ = (__int64) floor(*((double *) &val));
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, (uint32 *) &templ);
}

/*
 =======================================================================================================================
    Format: FLOOR.W.S fd, fs Purpose: To convert an FP float value to 32-bit fixed-point, rounding down.
 =======================================================================================================================
 */
void r4300i_COP1_floorw_s(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	float	tempf = *((float *) &cFS);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	*((__int32 *) &cFD) = (__int32) floor((double) tempf);
}

/*
 =======================================================================================================================
    Format: FLOOR.W.D fd, fs Purpose: To convert an FP doule float value to 32-bit fixed-point, rounding down.
 =======================================================================================================================
 */
void r4300i_COP1_floorw_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	*((__int32 *) &cFD) = (__int32) floor(*((double *) &val));
}

/*
 =======================================================================================================================
    Format: ROUND.L.S fd, fs Purpose: To convert an FP float value to 64-bit fixed-point, rounding to nearest.
 =======================================================================================================================
 */
void r4300i_COP1_roundl_s(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	__int64 templ;
	float	cfs = *(float *) &cFS;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(cfs > 0)
		templ = (__int64) (cfs + 0.5);
	else
		templ = -(__int64) (-cfs + 0.5);

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, (uint32 *) &templ);
}

/*
 =======================================================================================================================
    Format: ROUND.L.D fd, fs Purpose: To convert an FP double float value to 64-bit fixed-point, rounding to nearest.
 =======================================================================================================================
 */
void r4300i_COP1_roundl_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	__int64 templ;
	double	cfs = *((double *) &val);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(cfs > 0)
		templ = (__int64) (cfs + 0.5);
	else
		templ = -(__int64) (-cfs + 0.5);

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, (uint32 *) &templ);
}

/*
 =======================================================================================================================
    Format: ROUND.W.S fd, fs Purpose: To convert an FP float value to 32-bit fixed-point, rounding to nearest.
 =======================================================================================================================
 */
void r4300i_COP1_roundw_s(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	float	cfs = *((float *) &cFS);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	if(cfs > 0)
		*((__int32 *) &cFD) = (__int32) (cfs + 0.5);
	else
		*((__int32 *) &cFD) = -(__int32) (-cfs + 0.5);
}

/*
 =======================================================================================================================
    Format: ROUND.W.D fd, fs Purpose: To convert an FP double float value to 32-bit fixed-point, rounding to nearest.
 =======================================================================================================================
 */
void r4300i_COP1_roundw_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	double	cfs = *((double *) &val);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	if(cfs > 0)
		*((__int32 *) &cFD) = (__int32) (cfs + 0.5);
	else
		*((__int32 *) &cFD) = -(__int32) (-cfs + 0.5);
}

/*
 =======================================================================================================================
    Format: CEIL.W.S fd, fs Purpose: To convert an FP single float value to 32-bit fixed-point, rounding up.
 =======================================================================================================================
 */
void r4300i_COP1_ceilw_s(uint32 Instruction)
{
	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	*((__int32 *) &cFD) = (__int32) ceil(((double) (*((float *) &cFS))));
}

/*
 =======================================================================================================================
    Format: CEIL.W.D fd, fs Purpose: To convert an FP double float value to 32-bit fixed-point, rounding up.
 =======================================================================================================================
 */
void r4300i_COP1_ceilw_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	*((__int32 *) &cFD) = (__int32) ceil(*((double *) &val));
}

/*
 =======================================================================================================================
    Format: CEIL.L.S fd, fs Purpose: To convert an FP single float value to 64-bit fixed-point, rounding up.
 =======================================================================================================================
 */
void r4300i_COP1_ceill_s(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	__int64 val2 = (__int64) ceil(((double) (*((float *) &cFS))));
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, (uint32 *) &val2);
}

/*
 =======================================================================================================================
    Format: CEIL.L.D fd, fs Purpose: To convert an FP double float value to 64-bit fixed-point, rounding up.
 =======================================================================================================================
 */
void r4300i_COP1_ceill_d(uint32 Instruction)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint64	val = read_64bit_fpu_reg(RD_FS);
	__int64 val2 = (__int64) ceil(*((double *) &val));
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	CHK_ODD_FPR_2_REG(RD_FS, SA_FD);
	write_64bit_fpu_reg(SA_FD, (uint32 *) &val2);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_UN_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_UN_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_OLT_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_OLT_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_ULT_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_ULT_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_OLE_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_OLE_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_ULE_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_ULE_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_SF_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_SF_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_NGLE_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_NGLE_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_SEQ_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_SEQ_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_NGL_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_NGL_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_F_S(uint32 Instruction)
{
	r4300i_C_cond_fmt_s(Instruction);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void r4300i_C_F_D(uint32 Instruction)
{
	r4300i_C_cond_fmt_d(Instruction);
}

extern void COP1_instr(uint32);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void COP1_NotAvailable_instr(uint32 Instruction)
{
	if((gHWS_COP0Reg[STATUS] & SR_CU1))
	{
		COP1_instr(Instruction);
	}
	else
	{
		TRACE0("Trigger FPU Exception from Interpreter");
		TriggerFPUUnusableException();
	}
}
