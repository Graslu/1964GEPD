/*$T timer.c GC 1.136 03/09/02 17:40:48 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Timing for triggering cyclic tasks/events
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
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "globals.h"
#include "hardware.h"
#include "r4300i.h"
#include "n64rcp.h"
#include "interrupt.h"
#include "debug_option.h"
#include "emulator.h"
#include "timer.h"
#include "win32/windebug.h"
#include "1964ini.h"
#include "compiler.h"

BOOL	CPUNeedToDoOtherTask = FALSE;
BOOL	CPUNeedToCheckInterrupt = FALSE;
int		CounterFactor = COUTERFACTOR_2;

/* Optimized new CPU COUNT and VI counter variables */
uint64	current_counter;
uint64	next_vi_counter;		/* use 64bit varible, will never overflow */
uint64	next_count_counter;		/* value in here is in the unit of VIcounter, not in the half rate */
uint32  vi_field_number = 0;;
/*
 * speed concerning about using 64bit is not a big deal here £
 * because these two variables will not be used in emu main loop £
 * only when VI/COMPARE interrrupt happens
 */
__int32 counter_leap;
__int32 countdown_counter;
void	Set_Countdown_Counter(void);
uint32	Get_COUNT_Register(void);
uint32	Get_VIcounter(void);
void	Count_Down(uint32 count);
void	Init_Count_Down_Counters(void);

void	DoDMASegment(void);
void	DynDoDMASegment(void);
void	DoPIDMASegment(void);
void	DoSPDMASegment(void);
void	DoSIDMASegment(void);

enum COUNTER_TARGET_TYPE
{
	VI_COUNTER_TYPE,
	COMPARE_COUNTER_TYPE,
	PI_DMA_COUNTER_TYPE,
	SI_DMA_COUNTER_TYPE,
	SI_IO_COUNTER_TYPE,
	SP_DMA_COUNTER_TYPE,
	SP_DLIST_COUNTER_TYPE,
	SP_ALIST_COUNTER_TYPE,
	CHECK_INTERRUPT_COUNTER_TYPE,
	DELAY_AI_INTERRUPT_COUNTER_TYPE
};

struct Counter_Node
{
	uint64				target_counter;
	int					type;
	BOOL				inuse;
	struct Counter_Node *next;	/* use double link list */
	struct Counter_Node *prev;
};

struct Counter_Node CounterTargets[20];
struct Counter_Node *Timer_Event_List_Header;
int					Timer_Event_Count;

#define NEW_COUNTER_DEBUG
#ifdef NEW_COUNTER_DEBUG
#define NEW_COUNTER_TRACE_MACRO(macro)	macro

/*
 =======================================================================================================================
    define NEW_COUNTER_TRACE1(str, arg1) TRACE1(str, arg1)
 =======================================================================================================================
 */
#define NEW_COUNTER_TRACE1(str, arg1)
#else
#define NEW_COUNTER_TRACE_MACRO(macro)
#define NEW_COUNTER_TRACE1(str, arg1)
#endif
int VICounterFactors[9] = { 1, 1, 1, 2, 2, 3, 3, 4, 8 };
int CounterFactors[9] = { 1, 1, 2, 2, 4, 3, 6, 4, 8 };	/* 1 = half rate, 2 = full rate */

/*
 =======================================================================================================================
 =======================================================================================================================
 */
struct Counter_Node *Get_New_Counter_Node(void)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < 20; i++)
	{
		if(CounterTargets[i].inuse == FALSE)
		{
			CounterTargets[i].inuse = TRUE;
			return &CounterTargets[i];
		}
	}

	DisplayError("No more CounterTargets[] to use, this should not happens");
	return NULL;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Release_Counter_Node(struct Counter_Node *node)
{
	node->inuse = FALSE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Init_Timer_Event_List(void)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < 20; i++)
	{
		CounterTargets[i].inuse = FALSE;
		CounterTargets[i].next = NULL;
		CounterTargets[i].prev = NULL;
	}

	Timer_Event_List_Header = NULL;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
BOOL Is_CPU_Doing_Other_Tasks(void)
{
	/*~~*/
	int i;
	/*~~*/

	/* if( CPUNeedToDoOtherTask ) return TRUE; */
	for(i = 0; i < 20; i++)
	{
		if
		(
			CounterTargets[i].inuse
		&&	CounterTargets[i].type != VI_COUNTER_TYPE
		&&	CounterTargets[i].type != COMPARE_COUNTER_TYPE
		) return TRUE;
	}

	return FALSE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Refresh_Count_Down_Counter(void)
{
	if(Timer_Event_List_Header == NULL)
	{
		DisplayError("Error, no timer event in the event list, this should never happen");
		TRACE0("Error, no timer event in the event list, this should never happen");
		return;
	}

	current_counter = current_counter + counter_leap - countdown_counter;

	if((__int64) Timer_Event_List_Header->target_counter - (__int64) current_counter > 0x60000000)
	{
		/* ok, next timer target is too large, must be a non-setted COMPARE counter target */
		counter_leap = 0x60000000;
		countdown_counter = counter_leap;
	}
	else
	{
		counter_leap = (__int32) ((__int64) Timer_Event_List_Header->target_counter - (__int64) current_counter);

		/*
		 * if( counter_leap < 0 ) £
		 * DisplayError("Warning, counter_leap < 0");
		 */
		countdown_counter = counter_leap;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Add_New_Timer_Event(uint64 newtimer, int type)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	struct Counter_Node *tempnode, *newnode;
	uint64				new_target;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	new_target = current_counter + counter_leap - countdown_counter + newtimer; /* new target counter */

	if(type != VI_COUNTER_TYPE && emustatus.cpucore == DYNACOMPILER)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		/*
		 * here, we need to register consider the target timer value £
		 * 1080 works after I do this
		 */
		uint32	*ptr, pc, saveitlb, blksize;
		uint8	*blk;
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		pc = gHWS_pc;

		if(NOT_IN_KO_K1_SEG(pc))
		{
			/*~~~~~~~~*/
			uint32	tpc;
			/*~~~~~~~~*/

			saveitlb = ITLB_Error;
			ITLB_Error = FALSE;
			tpc = Direct_TLB_Lookup_Table[pc >> 12];
			if(ISNOTVALIDDIRECTTLBVALUE(tpc))
			{
				tpc = TranslateTLBAddress(pc, TLB_INST);
				if(ITLB_Error)
				{
					/* DisplayError("ITLB Error when setting timer"); */
					ITLB_Error = saveitlb;
					goto step2;
				}
			}
			else
				pc = tpc + (pc & 0x00000FFF);
			ITLB_Error = saveitlb;
		}

		ptr = (uint32 *) ((uint8 *) sDYN_PC_LOOKUP[pc >> 16] + (uint16) pc);
		blk = (uint8 *) *ptr;
		if(blk)
		{
			blksize = *(uint16 *) (blk - 2) - 1;

			/*
			 * ok, now I have the block size, where am I in the block, I dunno, guess I am in
			 * the middle to end of the block
			 */
			new_target = new_target + blksize * VICounterFactors[CounterFactor];
		}
	}

step2:
	/* Manipulate the timer event list */
	tempnode = Timer_Event_List_Header;

	/* Step 1: search the whole list, delete the same type event if exists */
	while(tempnode != NULL)
	{
		if(tempnode->type == type)
		{
			if(tempnode->prev != NULL)
				tempnode->prev->next = tempnode->next;
			else
			{
				Timer_Event_List_Header = tempnode->next;
				if(tempnode->next != NULL) tempnode->next->prev = NULL;
			}

			if(tempnode->next != NULL) tempnode->next->prev = tempnode->prev;

			Release_Counter_Node(tempnode);
			break;
		}

		tempnode = tempnode->next;
	}

	newnode = Get_New_Counter_Node();
	newnode->type = type;
	newnode->target_counter = new_target;

	tempnode = Timer_Event_List_Header;

	/* Step 2: Add the new event to the list */
	if(tempnode == NULL)
	{
		NEW_COUNTER_TRACE1("Add timer event, type=%d into empty list", type);

		/* ok, this new event should be added to the end of the list */
		Timer_Event_List_Header = newnode;
		newnode->prev = NULL;
		newnode->next = NULL;
	}
	else
	{
		while(tempnode != NULL)
		{
			if(tempnode->target_counter > new_target)
			{
				NEW_COUNTER_TRACE1("Insert timer event, type=%d ", type);
				if(tempnode->prev == NULL)
				{
					NEW_COUNTER_TRACE1("Insert timer event, type=%d at beginning", type);
					Timer_Event_List_Header = newnode;
					newnode->prev = NULL;
				}
				else
				{
					NEW_COUNTER_TRACE1("Insert timer event, type=%d in the middle", type);
					tempnode->prev->next = newnode;
					newnode->prev = tempnode->prev;
				}

				newnode->next = tempnode;
				tempnode->prev = newnode;
				break;
			}

			if(tempnode->next == NULL)
			{
				NEW_COUNTER_TRACE1("Add timer event, type=%d at the end", type);

				/* ok, this new event should be added to the end of the list */
				tempnode->next = newnode;
				newnode->prev = tempnode;
				newnode->next = NULL;
				break;
			}
			else
				tempnode = tempnode->next;
		}
	}

	if(Timer_Event_List_Header == newnode) Refresh_Count_Down_Counter();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Remove_1st_Timer_Event(int type)
{
	NEW_COUNTER_TRACE1("Remove 1st timer event, type=%d", Timer_Event_List_Header->type);
	if(type != Timer_Event_List_Header->type)
	{
		DisplayError("Error, remove timer event type mismatch");
		TRACE0("Error, remove timer event type mismatch");
	}

	Release_Counter_Node(Timer_Event_List_Header);
	Timer_Event_List_Header = Timer_Event_List_Header->next;

	if(Timer_Event_List_Header != NULL) Timer_Event_List_Header->prev = NULL;

	Refresh_Count_Down_Counter();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int Get_1st_Timer_Event_Type(void)
{
	if(Timer_Event_List_Header == NULL)
	{
		DisplayError("Error, no timer event in the event list, this should never happen");
		TRACE0("Error, no timer event in the event list, this should never happen");
		return 0;
	}
	else
		return Timer_Event_List_Header->type;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_PIDMA_Timer_Event(uint32 len)
{
	/*
	 * Thanks to zilmar £
	 * ChangeTimer(PiTimer,(int)(PI_WR_LEN_REG * 8.9) + 50);
	 */
	NEW_COUNTER_TRACE_MACRO(DEBUG_PI_DMA_TRACE1("PI DMA Delays %d cycles", (int) ((float) len * 8.9 + 50)));

	/*
	 * wow, the 8.89 is very important, Fzero does not work above it, and Rush2049
	 * does not work below it
	 */
	Add_New_Timer_Event((uint32) ((float) len * 8.89 + 50), PI_DMA_COUNTER_TYPE);

	/*
	 * Add_New_Timer_Event((uint32)((float)len* 8.9 + 50), PI_DMA_COUNTER_TYPE); £
	 * Add_New_Timer_Event((uint32)((float)len* 4 + 50), PI_DMA_COUNTER_TYPE); £
	 * Add_New_Timer_Event(len/2, PI_DMA_COUNTER_TYPE); £
	 * Add_New_Timer_Event(2, PI_DMA_COUNTER_TYPE);
	 */
	NEW_COUNTER_TRACE_MACRO(DEBUG_PI_DMA_TRACE1("Current timer is: %08X", (uint32) current_counter));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_SIDMA_Timer_Event(uint32 len)
{
	NEW_COUNTER_TRACE_MACRO(DEBUG_SI_DMA_TRACE1("SI DMA Delays %d cycles", (int) ((float) len * 8.9 + 50)));

	/*
	 * Add_New_Timer_Event((uint32)((float)len* 8.9 + 500), SI_DMA_COUNTER_TYPE); £
	 * Add_New_Timer_Event(len/2, SI_DMA_COUNTER_TYPE); £
	 * Add_New_Timer_Event(2, SI_DMA_COUNTER_TYPE);
	 */
	Add_New_Timer_Event((uint32) ((float) len * 4 + 10), SI_DMA_COUNTER_TYPE);

	/*
	 * Add_New_Timer_Event((uint32)((float)len* 7.55 + 10), SI_DMA_COUNTER_TYPE); £
	 * Add_New_Timer_Event((uint32)(490), SI_DMA_COUNTER_TYPE);
	 */
	NEW_COUNTER_TRACE_MACRO(DEBUG_SI_DMA_TRACE1("Current timer is: %08X", (uint32) current_counter));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_SI_IO_Timer_Event(uint32 len)
{
	NEW_COUNTER_TRACE_MACRO(DEBUG_SI_TASK_TRACE1("Marking SI IO Busy after SI DMA for %d cycles", len));

	/* Add_New_Timer_Event(len, SI_IO_COUNTER_TYPE); */
	Add_New_Timer_Event(300, SI_IO_COUNTER_TYPE);
	NEW_COUNTER_TRACE_MACRO(DEBUG_SI_TASK_TRACE1("Current timer is: %08X", (uint32) current_counter));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_SPDMA_Timer_Event(uint32 len)
{
	/* DoSPDMASegment(); */
	NEW_COUNTER_TRACE_MACRO(DEBUG_SP_DMA_TRACE1("SP DMA Delays %d cycles", len));

	/* Add_New_Timer_Event(len*4+50, SP_DMA_COUNTER_TYPE); */
	Add_New_Timer_Event(2, SP_DMA_COUNTER_TYPE);
	NEW_COUNTER_TRACE_MACRO(DEBUG_SP_DMA_TRACE1("Current timer is: %08X", (uint32) current_counter));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_SP_DLIST_Timer_Event(uint32 len)
{
	NEW_COUNTER_TRACE_MACRO(DEBUG_SP_TASK_TRACE1("Delay to Trigger DSP_Break after SP DList %d cycles", len));

	/*
	 * Trigger_RSPBreak(); //Trigger directly without delay £
	 * This value affects Taz Express, this game does not work if I use timer=2 for SP
	 * DLIST £
	 * but this will make Snow Kids flicker
	 */
	if(len * 3 > 700)
		Add_New_Timer_Event(700, SP_DLIST_COUNTER_TYPE);
	else
		Add_New_Timer_Event(len * 3, SP_DLIST_COUNTER_TYPE);

	/*
	 * Add_New_Timer_Event(len*3, SP_DLIST_COUNTER_TYPE); £
	 * Add_New_Timer_Event(700, SP_DLIST_COUNTER_TYPE); £
	 * Add_New_Timer_Event(400, SP_DLIST_COUNTER_TYPE); £
	 * Add_New_Timer_Event(2, SP_DLIST_COUNTER_TYPE);
	 */
	NEW_COUNTER_TRACE_MACRO(DEBUG_SP_TASK_TRACE1("Current timer is: %08X", (uint32) current_counter));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_SP_ALIST_Timer_Event(uint32 len)
{
	Add_New_Timer_Event(0x100000000, SP_ALIST_COUNTER_TYPE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_VI_Timer_Event(uint32 len)
{
	Add_New_Timer_Event(0x100000000, VI_COUNTER_TYPE);
}

extern void CPU_Check_Interrupts(void);

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_Check_Interrupt_Timer_Event(void)
{
	/* CPU_Check_Interrupts(); */
	Add_New_Timer_Event(5, CHECK_INTERRUPT_COUNTER_TYPE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_Check_Interrupt_Timer_Event_Again(void)
{
	/* CPU_Check_Interrupts(); */
	Add_New_Timer_Event(100, CHECK_INTERRUPT_COUNTER_TYPE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_Delay_AI_Interrupt_Timer_Event(void)
{
	Add_New_Timer_Event(10, DELAY_AI_INTERRUPT_COUNTER_TYPE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Trigger_Timer_Event(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int type = Get_1st_Timer_Event_Type();
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	gHWS_COP0Reg[COUNT] = Get_COUNT_Register();

	switch(type)
	{
	case VI_COUNTER_TYPE:
		Remove_1st_Timer_Event(VI_COUNTER_TYPE);
		Add_New_Timer_Event(max_vi_count, VI_COUNTER_TYPE);
		OPCODE_DEBUGGER_EPILOGUE(Trigger_VIInterrupt();) CPU_Check_Interrupts();
		break;
	case COMPARE_COUNTER_TYPE:
		Remove_1st_Timer_Event(COMPARE_COUNTER_TYPE);
		DEBUG_COMPARE_INTERRUPT_TRACE(TRACE1("Trigger Compare interrupt, cur timer=%08x", (uint32) current_counter));
		Add_New_Timer_Event(0x100000000, COMPARE_COUNTER_TYPE);
		OPCODE_DEBUGGER_EPILOGUE(Trigger_CompareInterrupt();) CPU_Check_Interrupts();
		break;
	case PI_DMA_COUNTER_TYPE:
		Remove_1st_Timer_Event(PI_DMA_COUNTER_TYPE);
		NEW_COUNTER_TRACE_MACRO(DEBUG_PI_DMA_TRACE1("Finish PI DMA Delay, Current timer is: %08X", (uint32) current_counter));
		OPCODE_DEBUGGER_EPILOGUE(DoPIDMASegment();) CPU_Check_Interrupts();
		break;
	case SI_DMA_COUNTER_TYPE:
		Remove_1st_Timer_Event(SI_DMA_COUNTER_TYPE);
		NEW_COUNTER_TRACE_MACRO(DEBUG_SI_DMA_TRACE1("Finish SI DMA Delay, Current timer is: %08X", (uint32) current_counter));
		OPCODE_DEBUGGER_EPILOGUE(DoSIDMASegment();) CPU_Check_Interrupts();
		break;
	case SI_IO_COUNTER_TYPE:
		Remove_1st_Timer_Event(SI_IO_COUNTER_TYPE);
		NEW_COUNTER_TRACE_MACRO(DEBUG_SI_TASK_TRACE1("Marking SI IO idle, Current timer is: %08X", (uint32) current_counter));
		OPCODE_DEBUGGER_EPILOGUE(SI_STATUS_REG &= ~SI_STATUS_RD_BUSY;) break;
	case SP_DMA_COUNTER_TYPE:
		Remove_1st_Timer_Event(SP_DMA_COUNTER_TYPE);
		NEW_COUNTER_TRACE_MACRO(DEBUG_SP_DMA_TRACE1("Finish SP DMA Delay, Current timer is: %08X", (uint32) current_counter));
		OPCODE_DEBUGGER_EPILOGUE(DoSPDMASegment();) break;
	case SP_DLIST_COUNTER_TYPE:
		Remove_1st_Timer_Event(SP_DLIST_COUNTER_TYPE);
		NEW_COUNTER_TRACE_MACRO(DEBUG_SP_TASK_TRACE1("Now Trigger DSP_Break, Current timer is: %08X", (uint32) current_counter));
		OPCODE_DEBUGGER_EPILOGUE(Trigger_RSPBreak();) CPU_Check_Interrupts();
		break;
	case SP_ALIST_COUNTER_TYPE:
		Remove_1st_Timer_Event(SP_ALIST_COUNTER_TYPE);
		break;
	case CHECK_INTERRUPT_COUNTER_TYPE:
		Remove_1st_Timer_Event(CHECK_INTERRUPT_COUNTER_TYPE);
		CPU_Check_Interrupts();
		if(CPUNeedToCheckInterrupt) Set_Check_Interrupt_Timer_Event_Again();
		break;
	case DELAY_AI_INTERRUPT_COUNTER_TYPE:
		Remove_1st_Timer_Event(DELAY_AI_INTERRUPT_COUNTER_TYPE);
		OPCODE_DEBUGGER_EPILOGUE(Trigger_AIInterrupt();) CPU_Check_Interrupts();
		break;
	default:
		DisplayError("Invalid timer event is triggered, should never happens");
		break;
	}

	CPUNeedToDoOtherTask = Is_CPU_Doing_Other_Tasks();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_Countdown_Counter(void)
{
	current_counter = current_counter + counter_leap - countdown_counter;
	if(next_vi_counter < next_count_counter)
	{
		countdown_counter = (__int32) (next_vi_counter - current_counter);
	}
	else
	{
		countdown_counter = (__int32) (next_count_counter - current_counter);
	}

	counter_leap = countdown_counter;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 Get_VIcounter(void)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < 20; i++)
	{
		if(CounterTargets[i].type == VI_COUNTER_TYPE)
		{
			return(uint32)
				(
					(
						max_vi_count +
						(current_counter + counter_leap - countdown_counter) -
						CounterTargets[i].target_counter
					)
				) % max_vi_count;
		}
	}

	DisplayError("Cannot find a VI timer event in the event list, should never happen");
	return(uint32) (current_counter % max_vi_count);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Count_Down(uint32 count)
{
	countdown_counter -= count;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Check_VI_and_COMPARE_Interrupt(void)
{
	gHWS_COP0Reg[COUNT] = Get_COUNT_Register();
	if(next_vi_counter <= current_counter + counter_leap - countdown_counter)
	{						/* ok, should trigger a VI interrupt */
		OPCODE_DEBUGGER_EPILOGUE(Trigger_VIInterrupt(););
		next_vi_counter += max_vi_count;
	}

	if(next_count_counter <= current_counter + counter_leap - countdown_counter)
	{						/* ok, should trigger a COMPARE interrupt */
		OPCODE_DEBUGGER_EPILOGUE(Trigger_CompareInterrupt(););
		next_count_counter += 0x100000000;
	}

	Set_Countdown_Counter();
}

#define DOUBLE_COUNT	1 //2	/* 1 */

/*
 =======================================================================================================================
 =======================================================================================================================
 */
uint32 Get_COUNT_Register(void)
{
#ifdef ENABLE_OPCODE_DEBUGGER
	return(uint32)
		(
			(current_counter + counter_leap - countdown_counter) *
			CounterFactors[CounterFactor] /
			VICounterFactors[CounterFactor] /
			DOUBLE_COUNT
		);
#else
	return(uint32)
		(
			(current_counter + counter_leap - countdown_counter) *
			CounterFactors[CounterFactor] /
			VICounterFactors[CounterFactor] /
			DOUBLE_COUNT +
			rand() %
			4
		);
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_COMPARE_Timer_Event(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	compare_reg = gHWS_COP0Reg[COMPARE];
	uint32	count_reg = Get_COUNT_Register();
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(compare_reg > count_reg)
	{
		Add_New_Timer_Event
		(
			((uint64) (compare_reg - count_reg)) *
				VICounterFactors[CounterFactor] /
				CounterFactors[CounterFactor] *
				DOUBLE_COUNT,
			COMPARE_COUNTER_TYPE
		);
	}
	else
	{
		Add_New_Timer_Event
		(
			((uint64) (0x100000000 + compare_reg - count_reg)) *
				VICounterFactors[CounterFactor] /
				CounterFactors[CounterFactor] *
				DOUBLE_COUNT,
			COMPARE_COUNTER_TYPE
		);
	}

	DEBUG_COMPARE_INTERRUPT_TRACE(TRACE1("Set timer event for Compare interrupt, cur timer=%08x", (uint32) current_counter));
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_PI_DMA_Timeout_Target_Counter(uint32 pi_dma_length)
{
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Init_Count_Down_Counters(void)
{
	srand((unsigned) time(NULL));
	Init_Timer_Event_List();
	current_counter = gHWS_COP0Reg[COUNT] *
		VICounterFactors[CounterFactor] /
		CounterFactors[CounterFactor] *
		DOUBLE_COUNT;
	counter_leap = 0;
	countdown_counter = 0;
	next_vi_counter = current_counter+current_counter%max_vi_count;

	if(gHWS_COP0Reg[COMPARE] != 0)
	{
		if(gHWS_COP0Reg[COMPARE] > gHWS_COP0Reg[COUNT])
		{
			Add_New_Timer_Event
			(
				(
					gHWS_COP0Reg[COMPARE] -
					gHWS_COP0Reg[COUNT]
				) *
						VICounterFactors[CounterFactor] /
						CounterFactors[CounterFactor] *
						DOUBLE_COUNT,
				COMPARE_COUNTER_TYPE
			);
		}
		else
		{
			Add_New_Timer_Event
			(
				(
					0x100000000 +
					gHWS_COP0Reg[COMPARE] -
					gHWS_COP0Reg[COUNT]
				) *
						VICounterFactors[CounterFactor] /
						CounterFactors[CounterFactor] *
						DOUBLE_COUNT,
				COMPARE_COUNTER_TYPE
			);
		}
	}
	else
	{
		Add_New_Timer_Event
		(
			(
				0x100000000 -
				gHWS_COP0Reg[COUNT]
			) *
					VICounterFactors[CounterFactor] /
					CounterFactors[CounterFactor] *
					DOUBLE_COUNT,
			COMPARE_COUNTER_TYPE
		);
	}

	Add_New_Timer_Event(next_vi_counter - current_counter, VI_COUNTER_TYPE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Init_VI_Counter(int tv_type)
{
	if(tv_type == 0)		/* PAL */
	{
		max_vi_count = PAL_VI_MAGIC_NUMBER;
		max_vi_lines = PAL_MAX_VI_LINE;
	}
	else if(tv_type == 1)	/* NTSC */
	{
		max_vi_count = NTSC_VI_MAGIC_NUMBER;	/* 883120;//813722;//813196;//NTSC_VI_MAGIC_NUMBER; */
		max_vi_lines = NTSC_MAX_VI_LINE;
	}

	vi_count_per_line = max_vi_count / max_vi_lines;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_VI_Counter_By_VSYNC(void)
{
	max_vi_count = (VI_V_SYNC_REG + 1) * 1500;
	if((VI_V_SYNC_REG % 1) != 0)
	{
		max_vi_count -= 38;
	}

	max_vi_lines = VI_V_SYNC_REG + 1;
	vi_count_per_line = max_vi_count / max_vi_lines;
}
