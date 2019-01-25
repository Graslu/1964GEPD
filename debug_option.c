/*$T debug_option.c GC 1.136 03/09/02 17:36:09 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Global variables representing flags for switching on/off debugger output
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
#include "debug_option.h"

#ifdef DEBUG_COMMON
struct DEBUGOPTIONS debugoptions;

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void init_debug_options(void)
{
	debugoptions.debug_io = 0;
	debugoptions.debug_io_vi = 0;
	debugoptions.debug_io_sp = 0;
	debugoptions.debug_io_pi = 0;
	debugoptions.debug_io_ai = 0;
	debugoptions.debug_io_mi = 0;
	debugoptions.debug_io_si = 0;
	debugoptions.debug_io_ri = 0;
	debugoptions.debug_io_dp = 0;
	debugoptions.debug_io_dps = 0;
	debugoptions.debug_io_rdram = 0;
	debugoptions.debug_audio = 0;
	debugoptions.debug_trap = 1;
	debugoptions.debug_si_controller = 1;
	debugoptions.debug_compare_interrupt = 0;
	debugoptions.debug_cpu_counter = 0;
	debugoptions.debug_sp_task = 1;
	debugoptions.debug_si_task = 0;
	debugoptions.debug_sp_dma = 0;
	debugoptions.debug_si_dma = 0;
	debugoptions.debug_pi_dma = 1;
	debugoptions.debug_si_mempak = 1;
	debugoptions.debug_dump_mempak = 0;
	debugoptions.debug_tlb = 1;
	debugoptions.debug_tlb_detail = 0;
	debugoptions.debug_tlb_extra = 0;
	debugoptions.debug_si_eeprom = 1;
	debugoptions.debug_vi_interrupt = 0;
	debugoptions.debug_ai_interrupt = 0;
	debugoptions.debug_si_interrupt = 0;
	debugoptions.debug_pi_interrupt = 0;
	debugoptions.debug_interrupt = 0;
	debugoptions.debug_sram = 1;
	debugoptions.debug_dyna_compiler = 0;
	debugoptions.debug_dyna_execution = 0;
	debugoptions.debug_dyna_log = 0;
	debugoptions.debug_64bit_fpu = 0;
	debugoptions.debug_cache = 0;
	debugoptions.debug_dyna_mod_code = 0;
	debugoptions.debug_protect_memory = 0;
	debugoptions.debug_exception_services = 1;
}
#endif
#ifdef ENABLE_OPCODE_DEBUGGER
int		debug_opcode = 1;
#else
int		debug_opcode = 0;
#endif
int		debug_opcode_block = 0;
int		debug_dirty_only = 1;
int		debug_annoying_messages = 0;

char	tracemessage[256];	/* message buffer to display message into debug box */
