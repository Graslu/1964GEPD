/*$T debug_option.h GC 1.136 02/28/02 07:56:50 */


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
#ifndef _DEBUG_OPTION_H__1964_
#define _DEBUG_OPTION_H__1964_

/* TRACE macros */
#ifdef DEBUG_COMMON
extern char tracemessage[256];
#define TRACE0(str)						{ RefreshOpList(str); }
#define TRACE1(str, arg1)				{ sprintf(tracemessage, str, arg1); RefreshOpList(tracemessage); }
#define TRACE2(str, arg1, arg2)			{ sprintf(tracemessage, str, arg1, arg2); RefreshOpList(tracemessage); }
#define TRACE3(str, arg1, arg2, arg3)	{ sprintf(tracemessage, str, arg1, arg2, arg3); RefreshOpList(tracemessage); }
#define TRACE4(str, arg1, arg2, arg3, arg4) \
	{ \
		sprintf(tracemessage, str, arg1, arg2, arg3, arg4); \
		RefreshOpList(tracemessage); \
	}
#else
#define TRACE0(str)
#define TRACE1(str, arg1)
#define TRACE2(str, arg1, arg2)
#define TRACE3(str, arg1, arg2, arg3)
#define TRACE4(str, arg1, arg2, arg3, arg4)
#endif
#ifdef DEBUG_COMMON
#define DEBUG_RANGE_ERROR
#define DEBUG_IO_READ
#define DEBUG_IO_WRITE

#define DEBUG_DYNA_COMPARE_INTERRUPT
#define DEBUG_COMPARE_INTERRUPT
#define DEBUG_CPU_COUNTER

#define DEBUG_SI_DMA
#define DEBUG_SP_DMA
#define DEBUG_PI_DMA
#define DEBUG_SP_TASK
#define DEBUG_SI_TASK

#define DEBUG_DYNA_CODE_DETECT

#define DEBUG_PROTECT_MEMORY

/*
 * This option to control if debugger will display debug information for si mempak
 * operation
 */
#define DEBUG_SI_MEMPAK
#ifdef DEBUG_SI_MEMPAK

/* define DEBUG_DUMP_MEMPAK */
#endif

/* This option will dump SRAM operation messages */
#define DEBUG_SRAM

/*
 * This option to control if debugger will display debug information for si eeprom
 * operation
 */
#define DEBUG_SI_EEPROM

/*
 * This option to control if debugger will display debug information for si
 * controller operation
 */
#define DEBUG_SI_CONTROLLER

/*
 * This option to control if debugger will display debug information for TLB
 * operation
 */
#define DEBUG_TLB

#ifdef DEBUG_TLB
#define DEBUG_TLB_DETAIL
#endif
#define DEBUG_DYNA_SPMEM

/*
 * This option to control if debugger will display debug information for TRAP
 * operation
 */
#define DEBUG_TRAP

/*
 * This option to control if debugger will display debug information for IO memory
 * operation
 */
#define DEBUG_IO
#ifdef DEBUG_IO
#define DEBUG_IO_VI
#define DEBUG_IO_SP
#define DEBUG_IO_PI
#define DEBUG_IO_AI
#define DEBUG_IO_MI
#define DEBUG_IO_SI
#define DEBUG_IO_RI
#define DEBUG_IO_DP
#define DEBUG_IO_DPS
#define DEBUG_IO_RDRAM
#endif
#define DEBUG_DYNA
#define DEBUG_DYNAEXECUTION

struct DEBUGOPTIONS
{
	int debug_io;
	int debug_io_vi;
	int debug_io_sp;
	int debug_io_pi;
	int debug_io_ai;
	int debug_io_mi;
	int debug_io_si;
	int debug_io_ri;
	int debug_io_dp;
	int debug_io_dps;
	int debug_io_rdram;
	int debug_audio;
	int debug_trap;
	int debug_compare_interrupt;
	int debug_cpu_counter;
	int debug_vi_interrupt;
	int debug_ai_interrupt;
	int debug_si_interrupt;
	int debug_pi_interrupt;
	int debug_interrupt;
	int debug_sp_task;
	int debug_si_task;
	int debug_sp_dma;
	int debug_si_dma;
	int debug_pi_dma;
	int debug_si_mempak;
	int debug_si_controller;
	int debug_dump_mempak;
	int debug_si_eeprom;
	int debug_tlb;
	int debug_tlb_detail;
	int debug_tlb_extra;
	int debug_sram;
	int debug_dyna_compiler;
	int debug_dyna_execution;
	int debug_dyna_log;
	int debug_64bit_fpu;
	int debug_cache;
	int debug_dyna_mod_code;
	int debug_protect_memory;
	int debug_exception_services;
};
extern struct DEBUGOPTIONS	debugoptions;

/*
 * This option to control if debugger will display debug information for audio
 * operation
 */
#define DEBUG_AUDIO
#endif
#ifdef DEBUG_AUDIO
#define DEBUG_AUDIO_MACRO(macro) \
	if(debugoptions.debug_audio) \
	{ \
		macro \
	}
#define DEBUG_AUDIO_TRACE0(str) \
	if(debugoptions.debug_audio) \
	{ \
		TRACE0(str); \
	}
#define DEBUG_AUDIO_TRACE1(str, arg1) \
	if(debugoptions.debug_audio) \
	{ \
		TRACE1(str, arg1); \
	}
#else
#define DEBUG_AUDIO_MACRO(macro)
#define DEBUG_AUDIO_TRACE0(str)
#define DEBUG_AUDIO_TRACE1(str, arg1)
#endif
#ifdef DEBUG_SI_DMA
#define DEBUG_SI_DMA_MACRO(macro) \
	if(debugoptions.debug_si_dma) \
	{ \
		macro \
	}
#define DEBUG_SI_DMA_TRACE0(str) \
	if(debugoptions.debug_si_dma) \
	{ \
		TRACE0(str); \
	}
#define DEBUG_SI_DMA_TRACE1(str, arg1) \
	if(debugoptions.debug_si_dma) \
	{ \
		TRACE1(str, arg1); \
	}
#else
#define DEBUG_SI_DMA_MACRO(macro)
#define DEBUG_SI_DMA_TRACE0(str)
#define DEBUG_SI_DMA_TRACE1(str, arg1)
#endif
#ifdef DEBUG_PI_DMA
#define DEBUG_PI_DMA_MACRO(macro) \
	if(debugoptions.debug_pi_dma) \
	{ \
		macro \
	}
#define DEBUG_PI_DMA_TRACE0(str) \
	if(debugoptions.debug_pi_dma) \
	{ \
		TRACE0(str); \
	}
#define DEBUG_PI_DMA_TRACE1(str, arg1) \
	if(debugoptions.debug_pi_dma) \
	{ \
		TRACE1(str, arg1); \
	}
#else
#define DEBUG_PI_DMA_MACRO(macro)
#define DEBUG_PI_DMA_TRACE0(str)
#define DEBUG_PI_DMA_TRACE1(str, arg1)
#endif
#ifdef DEBUG_SP_DMA
#define DEBUG_SP_DMA_MACRO(macro) \
	if(debugoptions.debug_sp_dma) \
	{ \
		macro \
	}
#define DEBUG_SP_DMA_TRACE0(str) \
	if(debugoptions.debug_sp_dma) \
	{ \
		TRACE0(str); \
	}
#define DEBUG_SP_DMA_TRACE1(str, arg1) \
	if(debugoptions.debug_sp_dma) \
	{ \
		TRACE1(str, arg1); \
	}
#else
#define DEBUG_SP_DMA_MACRO(macro)
#define DEBUG_SP_DMA_TRACE0(str)
#define DEBUG_SP_DMA_TRACE1(str, arg1)
#endif
#ifdef DEBUG_SP_TASK
#define DEBUG_SP_TASK_MACRO(macro) \
	if(debugoptions.debug_sp_task) \
	{ \
		macro \
	}
#define DEBUG_SP_TASK_TRACE0(str) \
	if(debugoptions.debug_sp_task) \
	{ \
		TRACE0(str); \
	}
#define DEBUG_SP_TASK_TRACE1(str, arg1) \
	if(debugoptions.debug_sp_task) \
	{ \
		TRACE1(str, arg1); \
	}
#else
#define DEBUG_SP_TASK_MACRO(macro)
#define DEBUG_SP_TASK_TRACE0(str)
#define DEBUG_SP_TASK_TRACE1(str, arg1)
#endif
#ifdef DEBUG_SI_TASK
#define DEBUG_SI_TASK_MACRO(macro) \
	if(debugoptions.debug_si_task) \
	{ \
		macro \
	}
#define DEBUG_SI_TASK_TRACE0(str) \
	if(debugoptions.debug_si_task) \
	{ \
		TRACE0(str); \
	}
#define DEBUG_SI_TASK_TRACE1(str, arg1) \
	if(debugoptions.debug_si_task) \
	{ \
		TRACE1(str, arg1); \
	}
#else
#define DEBUG_SI_TASK_MACRO(macro)
#define DEBUG_SI_TASK_TRACE0(str)
#define DEBUG_SI_TASK_TRACE1(str, arg1)
#endif
#ifdef DEBUG_COMMON
#define DEBUG_INTERRUPT_TRACE(othermacro) \
	if(debugoptions.debug_interrupt) \
	{ \
		othermacro \
	};
#define DEBUG_AI_INTERRUPT_TRACE(othermacro) \
	if(debugoptions.debug_interrupt && debugoptions.debug_ai_interrupt) \
	{ \
		othermacro \
	};
#define DEBUG_VI_INTERRUPT_TRACE(othermacro) \
	if(debugoptions.debug_interrupt && debugoptions.debug_vi_interrupt) \
	{ \
		othermacro \
	};
#define DEBUG_PI_INTERRUPT_TRACE(othermacro) \
	if(debugoptions.debug_interrupt && debugoptions.debug_pi_interrupt) \
	{ \
		othermacro \
	};
#define DEBUG_SI_INTERRUPT_TRACE(othermacro) \
	if(debugoptions.debug_interrupt && debugoptions.debug_si_interrupt) \
	{ \
		othermacro \
	};
#define DEBUG_COMPARE_INTERRUPT_TRACE(othermacro) \
	if \
	( \
		debugoptions.debug_interrupt \
	&&	debugoptions.debug_compare_interrupt \
	) \
	{ \
		othermacro \
	};
#define DEBUG_CPU_COUNTER_TRACE(othermacro) \
	if(debugoptions.debug_cpu_counter) \
	{ \
		othermacro \
	};
#define DEBUG_CACHE_TRACE(othermacro) \
	if(debugoptions.debug_cache) \
	{ \
		othermacro \
	};
#define DEBUG_DYNA_MOD_CODE_TRACE(othermacro) \
	if(debugoptions.debug_dyna_mod_code) \
	{ \
		othermacro \
	};
#define DEBUG_SRAM_TRACE(othermacro) \
	if(debugoptions.debug_sram) \
	{ \
		othermacro \
	};
#define DEBUG_FLASHRAM_TRACE(othermacro) \
	if(debugoptions.debug_sram) \
	{ \
		othermacro \
	};
#define DEBUG_EXCEPTION_TRACE(othermacro) \
	if(debugoptions.debug_exception_services) \
	{ \
		othermacro \
	};
#define DEBUG_CONTROLLER_TRACE(othermacro) \
	if(debugoptions.debug_si_controller) \
	{ \
		othermacro \
	};
#else
#define DEBUG_INTERRUPT_TRACE(othermacro)
#define DEBUG_AI_INTERRUPT_TRACE(othermacro)
#define DEBUG_VI_INTERRUPT_TRACE(othermacro)
#define DEBUG_PI_INTERRUPT_TRACE(othermacro)
#define DEBUG_SI_INTERRUPT_TRACE(othermacro)
#define DEBUG_COMPARE_INTERRUPT_TRACE(othermacro)
#define DEBUG_CPU_COUNTER_TRACE(othermacro)
#define DEBUG_CACHE_TRACE(othermacro)
#define DEBUG_DYNA_MOD_CODE_TRACE(othermacro)
#define DEBUG_SRAM_TRACE(othermacro)
#define DEBUG_FLASHRAM_TRACE(othermacro)
#define DEBUG_EXCEPTION_TRACE(othermacro)
#define DEBUG_CONTROLLER_TRACE(othermacro)
#endif
#ifdef DEBUG_TLB
#define TLB_TRACE(macro)	{ if(debugoptions.debug_tlb) { macro } }
#else
#define TLB_TRACE(macro)
#endif
#ifdef DEBUG_TLB_DETAIL
#define TLB_DETAIL_TRACE(macro) { if(debugoptions.debug_tlb_detail) { macro } }
#define TLB_EXTRA_TRACE(macro)	{ if(debugoptions.debug_tlb_extra) { macro } }
#else
#define TLB_DETAIL_TRACE(macro) { }
#define TLB_EXTRA_TRACE(macro)	{ }
#endif
#ifdef DEBUG_DYNA_CODE_DETECT
#define CODE_DETECT_TRACE(macro)	{ if(debugoptions.debug_dyna_mod_code) { macro } }
#else
#define CODE_DETECT_TRACE(macro)
#endif
#ifdef DEBUG_PROTECT_MEMORY
#define PROTECT_MEMORY_TRACE(macro) { if(debugoptions.debug_protect_memory) { macro } }
#else
#define PROTECT_MEMORY_TRACE(macro)
#endif
extern int					debug_opcode;
extern int					debug_opcode_block;
extern int					debug_dirty_only;
extern int					debug_annoying_messages;
#endif
