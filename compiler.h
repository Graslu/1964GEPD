/*$T compiler.h GC 1.136 03/09/02 15:47:18 */


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
#ifndef _COMPILER_H__1964_
#define _COMPILER_H__1964_

#include <windows.h>
#include "debug_option.h"

struct BLOCK_ENTRY_STRUCT
{
	BOOL						HasBeenCompiled;
	uint32						block_ptr;
	uint32						block_pc;
	BOOL						need_target_1;
	uint32						jmp_to_target_1_code_addr;	/* need to write target 1 block addr into this memory */
	uint32						target_1_pc;
	BOOL						need_target_2;
	uint32						jmp_to_target_2_code_addr;	/* need to write target 2 block addr into this memory */
	uint32						target_2_pc;
	struct BLOCK_ENTRY_STRUCT	*next;
};

struct CompilerStatus
{
	uint32			TempPC;
	BOOL			DynaBufferOverError;
	uint32			cp0Counter;
	uint32			KEEP_RECOMPILING;
	uint32			BlockStart;
	int				Is_Compiling;
	unsigned long	lCodePosition;
	uint32			*pcptr;
	uint32			realpc_fetched;
	uint32			FlagJAL;
	uint32			InstructionCount;
};

typedef struct BLOCK_ENTRY_STRUCT	BLOCK_ENTRY;
extern struct CompilerStatus		compilerstatus;
extern BLOCK_ENTRY					*current_block_entry;
extern uint8						*sDYN_PC_LOOKUP[0x10000];
extern uint8						*dyna_CodeTable;
extern uint8						*dyna_RecompCode;
uint32								Dyna_Compile_Block(void);
uint32								Dyna_Compile_4KB_Block(void);

BOOL							  IsTargetPcInTheSame4KB(uint32 pc, uint32 target);
void							  InvalidateOneBlock(uint32 pc);
void							  Invalidate4KBlock(uint32 addr, char *opcodename, int type, uint64 newvalue);
void							  Check_And_Invalidate_Compiled_Blocks_By_DMA
								(
									uint32	startaddr,
									uint32	len,
									char	*operation
								);
void							  Dyna_Code_Check_None(void);
void							  Set_Translate_PC(void);
uint32								DynaFetchInstruction2(uint32 pc);

/* dynarec globals */
extern uint8						*Block;
#endif
