/*$T DbgPrint.h GC 1.136 03/09/02 17:28:42 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Debug print macros
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
#ifndef _DBGPRINT_H__1964_
#define _DBGPRINT_H__1964_

#include "dynarec/dynalog.h"

extern char op_str[0xFF];

char		*DebugPrintInstruction(uint32 instruction);
char		*DebugPrintInstructionWithOutRefresh(uint32 Instruction);
char		*Get_Interrupt_Name(void);
void		DebugPrintPC(uint32 thePC);

/*
 =======================================================================================================================
    argument printing macros
 =======================================================================================================================
 */
#define DBGPRINT_RT_IMM(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X],%04Xh", \
		gHWS_pc, \
		_op_name, \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT], \
		OFFSET_IMMEDIATE \
	);

#define DBGPRINT_RT_FS_COP0(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X],%s", \
		gHWS_pc, \
		_op_name, \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT], \
		r4300i_COP0_RegNames[RD_FS] \
	);

#define DBGPRINT_RT_FS_COP1(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X],%s", \
		gHWS_pc, \
		_op_name, \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT], \
		r4300i_COP1_RegNames[RD_FS] \
	);

#define DBGPRINT_BASE_RT_OFFSET(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%2s[%08X],%04Xh(%s[%08X])", \
		gHWS_pc, \
		_op_name, \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT], \
		OFFSET_IMMEDIATE, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT] \
	);

#define DBGPRINT_BASE_RT64BIT_OFFSET(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%2s[%8X], %04Xh(%s[%08X])", \
		gHWS_pc, \
		_op_name, \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT], \
		OFFSET_IMMEDIATE, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT] \
	);

#define DBGPRINT_RS_RT_IMM(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%2s[%08X],%s[%08X],%04Xh", \
		gHWS_pc, \
		_op_name, \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT], \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT], \
		(signed) (OFFSET_IMMEDIATE) \
	);

#define DBGPRINT_RS_RT_IMMH(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%2s[%08X],%s[%08X],%04Xh", \
		gHWS_pc, \
		_op_name, \
		r4300i_RegNames[RT_FT], \
		((uint32) gHWS_GPR[RT_FT]), \
		r4300i_RegNames[RS_BASE_FMT], \
		((uint32) gHWS_GPR[RS_BASE_FMT]), \
		(uint16) OFFSET_IMMEDIATE \
	);

#define DBGPRINT_RS_OFF(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X],%04Xh", \
		gHWS_pc, \
		_op_name, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT], \
		OFFSET_IMMEDIATE \
	);

#define DBGPRINT_RS_IMM(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X],%04Xh", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT], \
		OFFSET_IMMEDIATE \
	);

#define DBGPRINT_RS_RD(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X],%s[%08X]", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT], \
		r4300i_RegNames[RD_FS], \
		(uint32) gHWS_GPR[RD_FS] \
	);

#define DBGPRINT_RS_RT_RD(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%2s[%08X],%s[%08X],%s[%08X]", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_RegNames[RD_FS], \
		(uint32) gHWS_GPR[RD_FS], \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT], \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT] \
	);

#define DBGPRINT_RT_RD_SA(_op_name) \
	if((RT_FT | SA_FD | RD_FS) == 0) \
		sprintf(op_str, "%08X: NOP", gHWS_pc); \
	else \
	{ \
		sprintf \
		( \
			op_str, \
			"%08X: %s%2s,%s,%04Xh", \
			gHWS_pc, \
			(uint32) _op_name, \
			(uint32) r4300i_RegNames[RD_FS], \
			(uint32) r4300i_RegNames[RT_FT], \
			SA_FD \
		); \
	}

#define DBGPRINT_RS_RT(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X],%s[%08X]", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT], \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT] \
	);

#define DBGPRINT_RD(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X]", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_RegNames[RD_FS], \
		(uint32) gHWS_GPR[RD_FS] \
	);

#define DBGPRINT_RS(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X]", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT] \
	);

#define DBGPRINT_RS_RT_OFF(_op_name)	DBGPRINT_RS(_op_name)
#define DBGPRINT_RS_RT_OFF_BRANCH(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X],%s[%08X],%04Xh", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT], \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT], \
		((OFFSET_IMMEDIATE * 4) + gHWS_pc + 4) \
	);

#define DBGPRINT_RS_OFF_BRANCH(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X],%04Xh", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT], \
		((OFFSET_IMMEDIATE * 4) + gHWS_pc + 4) \
	);

#define DBGPRINT_FPR_OFF_BRANCH(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%04Xh", \
		gHWS_pc, \
		(uint32) _op_name, \
		((OFFSET_IMMEDIATE * 4) + gHWS_pc + 4) \
	);

#define DBGPRINT_INSTR_INDEX(_op_name)	sprintf(op_str, "%08X: %s%08X", gHWS_pc, (uint32) _op_name, instr_index);

#define DBGPRINT_RT_RD_RS(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%2s[%08X],%s[%08X],%s[%08X]", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_RegNames[RD_FS], \
		(uint32) gHWS_GPR[RD_FS], \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT], \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT] \
	);

#define DBGPRINT_OPCODE(_op_name)	sprintf(op_str, "%08X: %s", gHWS_pc, (uint32) _op_name);

#define DBGPRINT_FD_FS(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s,%s", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_COP1_RegNames[SA_FD], \
		r4300i_COP1_RegNames[RD_FS] \
	);

#define DBGPRINT_FD_FS_FT(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s[%08X],%s[%08X],%s[%08X]", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_RegNames[SA_FD], \
		(uint32) gHWS_GPR[SA_FD], \
		r4300i_RegNames[RD_FS], \
		(uint32) gHWS_GPR[RD_FS], \
		r4300i_RegNames[RT_FT], \
		(uint32) gHWS_GPR[RT_FT] \
	);

#define DBGPRINT_BASE_FPR_OFFSET(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%2s[%08X],%04Xh(%s[%08X])", \
		gHWS_pc, \
		_op_name, \
		r4300i_COP1_RegNames[RT_FT], \
		(uint32) gHWS_fpr32[RT_FT], \
		OFFSET_IMMEDIATE, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT] \
	);

#define DBGPRINT_BASE_FPR64BIT_OFFSET(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%2s,%04Xh(%s[%08X])", \
		gHWS_pc, \
		_op_name, \
		r4300i_COP1_RegNames[RT_FT], \
		OFFSET_IMMEDIATE, \
		r4300i_RegNames[RS_BASE_FMT], \
		(uint32) gHWS_GPR[RS_BASE_FMT] \
	);

#define DBGPRINT_FPR_FT_FS(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s,%s", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_COP1_RegNames[RD_FS], \
		r4300i_COP1_RegNames[RT_FT] \
	);

#define DBGPRINT_FPR64BIT_FT_FS(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s,%s", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_COP1_RegNames[RD_FS], \
		r4300i_COP1_RegNames[RT_FT] \
	);

#define DBGPRINT_FPR64BIT_FS_FD(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s,%s", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_COP1_RegNames[RD_FS], \
		r4300i_COP1_RegNames[SA_FD] \
	);

#define DBGPRINT_FPU_FD_FS_FT(_op_name) \
	sprintf \
	( \
		op_str, \
		"%08X: %s%s,%s,%s", \
		gHWS_pc, \
		(uint32) _op_name, \
		r4300i_COP1_RegNames[SA_FD], \
		r4300i_COP1_RegNames[RD_FS], \
		r4300i_COP1_RegNames[RT_FT] \
	);
#endif
