/*$T memory.h GC 1.136 02/28/02 08:04:48 */


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
#ifndef _MEMORY_H__1964_
#define _MEMORY_H__1964_

#include "globals.h"
#include "hardware.h"

uint8			*dynarommap[0x10000];

#define SP_START_ADDR	0x04000000
#define SP_END			0x04080007
#define SP_SIZE			(SP_END - SP_START_ADDR + 1)

void			Init_R_AND_W(uint8 **sDWORD_R, uint8 *MemoryRange, uint32 startAddress, uint32 endAddress);
void			DynInit_R_AND_W(uint8 *MemoryRange, uint32 startAddress, uint32 endAddress);
void			InitMemoryLookupTables(void);
void			InitVirtualMemory(void);
void			InitVirtualMemory1(MemoryState *gMemoryState);
void			FreeVirtualMemory(void);
void			InitVirtualRomMemory(uint32 filesize);
void			FreeVirtualRomMemory(void);
void			ResetRdramSize(int setsize);
BOOL			UnmappedMemoryExceptionHelper(uint32 addr);
BOOL			ProtectBlock(uint32 pc);
BOOL			UnprotectBlock(uint32 pc);
void			UnprotectAllBlocks(void);
void			ROM_CheckSumZelda(void);
void			ROM_CheckSumMario(void);

extern uint32	current_rdram_size;

/* Memory segment size definition */
#define MEMORY_SIZE_RDRAM				0x400000
#define MEMORY_SIZE_EXRDRAM				0x400000
#define MEMORY_SIZE_NO_EXPANSION		0x400000
#define MEMORY_SIZE_WITH_EXPANSION		0x800000
#define MEMORY_SIZE_NO_EXPANSION_MASK	0x3FFFFF
#define MEMORY_SIZE_WITH_EXPANSION_MASK 0x7FFFFF
#define MEMORY_SIZE_RAMREGS0			0x30
#define MEMORY_SIZE_RAMREGS4			0x30
#define MEMORY_SIZE_RAMREGS8			0x30
#define MEMORY_SIZE_SPMEM				0x2000
#define MEMORY_SIZE_SPREG_1				0x24
#define MEMORY_SIZE_SPREG_2				0x8
#define MEMORY_SIZE_DPC					0x20
#define MEMORY_SIZE_DPS					0x10
#define MEMORY_SIZE_MI					0x10
#define MEMORY_SIZE_VI					0x50
#define MEMORY_SIZE_AI					0x18
#define MEMORY_SIZE_PI					0x4C
#define MEMORY_SIZE_RI					0x20
#define MEMORY_SIZE_SI					0x1C
#define MEMORY_SIZE_C2A1				0x8000
#define MEMORY_SIZE_C1A1				0x8000
#define MEMORY_SIZE_C2A2				0x20000
#define MEMORY_SIZE_GIO_REG				0x804
#define MEMORY_SIZE_C1A3				0x8000
#define MEMORY_SIZE_PIF					0x800
#define MEMORY_SIZE_DUMMY				0x10000

/* Memory masks */
#define ADDRESS_MASK_C2A1	0x00007FFF	/* 32KB SRAM start at 0x05000000 */
#define ADDRESS_MASK_C1A1	0x00007FFF	/* 32KB SRAM start at 0x06000000 */
#define ADDRESS_MASK_C2A2	0x0001FFFF	/* 128KB FLASHRAM/SRAM start at 0x08000000 */

/* define ADDRESS_MASK_C2A2 0x00007FFF // 128KB FLASHRAM/SRAM start at 0x08000000 */
#define ADDRESS_MASK_C1A3	0x00007FFF	/* 32KB SRAM start at 0x1FD00000 */

/* Memory segment start address / end address definition */
#define MEMORY_START_RDRAM		0x00000000
#define MEMORY_START_EXRDRAM	0x00400000

/* define MEMORY_START_RDREG 0x03F00000 */
#define MEMORY_START_RAMREGS0	0x03F00000
#define MEMORY_START_RAMREGS4	0x03F04000
#define MEMORY_START_RAMREGS8	0x03F80000
#define MEMORY_START_SPMEM		0x04000000
#define MEMORY_START_SPREG_1	0x04040000
#define MEMORY_START_SPREG_2	0x04080000
#define MEMORY_START_DPC		0x04100000
#define MEMORY_START_DPS		0x04200000
#define MEMORY_START_MI			0x04300000
#define MEMORY_START_VI			0x04400000
#define MEMORY_START_AI			0x04500000
#define MEMORY_START_PI			0x04600000
#define MEMORY_START_RI			0x04700000
#define MEMORY_START_SI			0x04800000
#define MEMORY_START_C2A1		0x05000000
#define MEMORY_START_C1A1		0x06000000
#define MEMORY_START_C2A2		0x08000000
#define MEMORY_START_ROM_IMAGE	0x10000000
#define MEMORY_START_GIO		0x18000000
#define MEMORY_START_PIF		0x1FC00000
#define MEMORY_START_C1A3		0x1FD00000
#define MEMORY_START_DUMMY		0x1FFF0000

/*
 * These shift macros are to reduce the memory functions array size to use less
 * memory and presumably to use the cache better for more speed. It is probably a
 * good idea to use indexing to reduce cache misses during address translation.
 * Cache miss can be 50-80 cycles.
 */
#define SHIFTER1_READ	2				/* Shifts off insignificant bits from memory_read_functions array size. The
										 * significant bits are 0xFFFC0000 (14 significant bits) because Check_LW needs
										 * to check SPREG_1, which is at 0xA4040000. So we only need 14bit instead of
										 * 16bit. */
#define SHIFTER1_WRITE	0				/* Shifts off insignificant bits from memory write functions array size. Set to
										 * zero because of protected memory in 0x1000 blocks. = All bits are
										 * significant. */

#define SHIFTER2_READ	(16 + SHIFTER1_READ)
#define SHIFTER2_WRITE	(12 + SHIFTER1_WRITE)

extern uint32 * (*memory_read_functions[0x10000 >> SHIFTER1_READ]) ();
extern uint32 * (*memory_write_functions[0x100000 >> SHIFTER1_WRITE]) (uint32 addr);
extern uint32 write_mem_rt;

uint32 *read_mem_rdram(uint32 addr);
uint32 *read_mem_cart(uint32 addr);
uint32 *read_mem_io(uint32 addr);
uint32 *read_mem_flashram(uint32 addr);
uint32 *read_mem_sram(uint32 addr);
uint32 *read_mem_tlb(uint32 addr);
uint32 *read_mem_unmapped(uint32 addr);
uint32 *read_mem_others(uint32 addr);
uint32 *write_mem_rdram(uint32 addr);
uint32 *write_mem_cart(uint32 addr);
uint32 *write_mem_io(uint32 addr);
uint32 *write_mem_flashram(uint32 addr);
uint32 *write_mem_sram(uint32 addr);
uint32 *write_mem_tlb(uint32 addr);
uint32 *write_mem_unmapped(uint32 addr);
uint32 *write_mem_others(uint32 addr);
uint32 *write_mem_protected(uint32 addr);
void init_whole_mem_func_array(void);
void enable_exrdram_func_array(void);
void disable_exrdram_func_array(void);
void protect_memory_set_func_array(uint32 pc);
void unprotect_memory_set_func_array(uint32 pc);
uint32 *mem_read_eax_only_helper(uint32 addr);

BOOL rdram_is_at_0x20000000;

#define MEM_READ_SWORD(addr)		*(_int32 *) (mem_read_eax_only_helper(addr))
#define MEM_READ_UWORD(addr)		*(uint32 *) (mem_read_eax_only_helper(addr))
#define MEM_READ_SBYTE(addr)		*(_int8 *) (mem_read_eax_only_helper(addr ^ 3))
#define MEM_READ_UBYTE(addr)		*(uint8 *) (mem_read_eax_only_helper(addr ^ 3))
#define MEM_READ_SHALFWORD(addr)	*(_int16 *) (mem_read_eax_only_helper(addr ^ 2))
#define MEM_READ_UHALFWORD(addr)	*(uint16 *) (mem_read_eax_only_helper(addr ^ 2))
#define PMEM_READ_SWORD(addr)		(_int32 *) (mem_read_eax_only_helper(addr))
#define PMEM_READ_UWORD(addr)		(uint32 *) (mem_read_eax_only_helper(addr))
#define PMEM_WRITE_SWORD(addr)		((_int32 *) (*memory_write_functions[(uint32) addr >> SHIFTER2_WRITE]) (addr))
#define PMEM_WRITE_UWORD(addr)		((uint32 *) (*memory_write_functions[(uint32) addr >> SHIFTER2_WRITE]) (addr))
#define PMEM_WRITE_UHALFWORD(addr)	((uint16 *) (*memory_write_functions[(uint32) addr >> SHIFTER2_WRITE]) ((addr ^ 2)))
#define PMEM_WRITE_UBYTE(addr)		((uint8 *) (*memory_write_functions[(uint32) addr >> SHIFTER2_WRITE]) ((addr ^ 3)))
#include "dynarec/opcodeDebugger.h"
void	Debugger_Copy_Memory(MemoryState *target, MemoryState *source);
#endif
