/*$T timer.h GC 1.136 02/28/02 08:15:14 */


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
#ifndef _TIMER_H__1964_
#define _TIMER_H__1964_
#include "windows.h"

/* This option is to mark SP busy, and delay SP task for a moment of time */
#define DOSPTASKCOUNTER

/*
 * This option will precisely emulate CPU PCLOCK counter for all the multi-pclock
 * opcode £
 * Like integer MUL, DIV, TLBP and so on £
 * #define SAVEOPCOUNTER
 */
#define SPCOUNTERTOINCREASE 10						/* Number of PCLOCK to execute each instruction in r4300i */
#define SPTASKPCLOCKS		260						/* Number of PCLOCK to execute each SP Task */

#define PI_DMA_SEGMENT		0x10					/* Bytes to transfer per CPU instruction cycle */
#define SP_DMA_SEGMENT		0x10					/* Bytes to transfer per CPU instruction cycle */
#define SI_DMA_SEGMENT		0x02					/* Bytes to transfer per CPU instruction cycle */

/* SI DMA Segment must be less than 4, otherwise Cruise USA will not work */
#define SI_IO_DELAY 400								/* This value cannot be too large, Perfect Dark stop working if */

/*
 * this value is larger than 800, Conker's stops working if value £
 * is larger than 400
 */
BOOL					CPUNeedToDoOtherTask;		/* This parameter is set when CPU need to do tasks */

/*
 * such as DMA memory transfer, SP task timer count down £
 * S SI DMA transfer count down
 */
BOOL					CPUNeedToCheckInterrupt;	/* This global parameter is set when any interrupt is fired */

/* and the CPU is waiting for a COMPARE interrupts */
BOOL					CPUNeedToCheckException;	/* This global parameter is set when any interrupt is fired */

/* and the CPU is waiting for a COMPARE interrupts */
int						viframeskip;				/* This global parameter determines to skip a VI frame */

/* after every few frames */
int						viframeskipcount;
int						framecounter;				/* To count how many frames are displayed per second */
int						viCountePerSecond;			/* To count how many VI interrupts per second */
float					vips;						/* VI/s */
extern LARGE_INTEGER	Freq;
extern LARGE_INTEGER	LastVITime;
extern LARGE_INTEGER	LastSecondTime;


void					CPUDoOtherTasks(void);

#define VI_COUNTER_INC_PER_LINE vi_count_per_line

#define NTSC_MAX_VI_LINE		0x20D
#define PAL_MAX_VI_LINE			625

extern unsigned __int32 max_vi_lines;
extern unsigned __int32 max_vi_count;
extern unsigned __int32 vi_count_per_line;
extern unsigned __int32 vi_field_number;

#define NTSC_VI_MAGIC_NUMBER	625000
#define PAL_VI_MAGIC_NUMBER		777809				/* 750000 //777809 */

void					Init_VI_Counter(int tv_type);
void					Set_VI_Counter_By_VSYNC(void);
extern int				VICounterFactors[9];
extern int				CounterFactors[9];
extern int				CounterFactor;

void					Count_Down(unsigned __int32 count);
unsigned __int32		Get_COUNT_Register(void);
unsigned __int32		Get_VIcounter(void);
void					Count_Down(unsigned __int32 count);
void					Check_VI_and_COMPARE_Interrupt(void);
void					Set_COMPARE_Timer_Event(void);
void					Set_Check_Interrupt_Timer_Event(void);
void					Set_PIDMA_Timer_Event(unsigned __int32 len);
void					Set_SIDMA_Timer_Event(unsigned __int32 len);
void					Set_SPDMA_Timer_Event(unsigned __int32 len);
void					Set_SP_DLIST_Timer_Event(unsigned __int32 len);
void					Set_SP_ALIST_Timer_Event(unsigned __int32 len);
void					Set_SI_IO_Timer_Event(unsigned __int32 len);
void					Set_VI_Timer_Event(unsigned __int32 len);
void					Set_Delay_AI_Interrupt_Timer_Event(void);
void					Trigger_Timer_Event(void);
BOOL					Is_CPU_Doing_Other_Tasks(void);

void					Init_Count_Down_Counters(void);

extern __int32			countdown_counter;

#ifdef SAVEOPCOUNTER
#define SAVE_OP_COUNTER_INCREASE(val)				SUB_ImmToMemory((_u32) & countdown_counter, val);
#define SAVE_OP_COUNTER_INCREASE_INTERPRETER(val)	{ countdown_counter -= val * CounterFactors[CounterFactor] * 2; }
#else
#define SAVE_OP_COUNTER_INCREASE(val)
#define SAVE_OP_COUNTER_INCREASE_INTERPRETER(val)
#endif

/* For profiling */
enum { R4300I_PROF, VIDEO_PROF, AUDIO_PROF, COMPILER_PROF, CPU_IDLE_PROF, RSP_PROF, RDP_PROF, MAX_PROF };
void start_profiling(int proc);
void stop_profiling(void);
void format_profiler_result_msg(char *msg);
void reset_profiler(void);

#define DO_PROFILIER(proc) \
	if(guioptions.display_profiler_status) start_profiling(proc);
#define DO_PROFILIER_R4300I		DO_PROFILIER(R4300I_PROF)
#define DO_PROFILIER_VIDEO		DO_PROFILIER(VIDEO_PROF)
#define DO_PROFILIER_AUDIO		DO_PROFILIER(AUDIO_PROF)
#define DO_PROFILIER_COMPILER	DO_PROFILIER(COMPILER_PROF)
#define DO_PROFILIER_CPU_IDLE	DO_PROFILIER(CPU_IDLE_PROF)
#define DO_PROFILIER_RSP		DO_PROFILIER(RSP_PROF)
#define DO_PROFILIER_RDP		DO_PROFILIER(RDP_PROF)
#endif /* 1964_TIMER_H */
