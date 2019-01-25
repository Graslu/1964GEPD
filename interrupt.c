/*$T interrupt.c GC 1.136 03/09/02 17:29:10 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Interrupt and Exception service routines
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
#include "interrupt.h"
#include "r4300i.h"
#include "n64rcp.h"
#include "timer.h"
#include "debug_option.h"
#include "emulator.h"
#include "1964ini.h"
#include "win32/DLL_Video.h"
#include "win32/DLL_Audio.h"
#include "win32/DLL_RSP.h"
#include "win32/wingui.h"
#include "win32/windebug.h"
#include "cheatcode.h"
#include "dynarec/dynacpu.h"

extern void		Init_Timer_Event_List(void);
extern uint32	TLB_Error_Vector;

uint32			sp_hle_task = 0;
LARGE_INTEGER	LastVITime = { 0, 0 };
LARGE_INTEGER	LastSecondTime = { 0, 0 };
LARGE_INTEGER	Freq;
LARGE_INTEGER	CurrentTime;
LARGE_INTEGER	Elapsed;
double			sleeptime;
double			tempvips;


void rdp_fullsync()
//Hack..gets rsp working for basic video plugin+rsp+starfox.
{
	MI_INTR_REG_R |= MI_INTR_DP;

	if((MI_INTR_MASK_REG_R) & MI_INTR_MASK_DP)
	{
		SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP3;
		HandleInterrupts(0x80000180);
	}
}
/*
 =======================================================================================================================
    This function is called when an Exception Interrupt happens. It sets Coprocessor0 registers EPC and CAUSE, then
    returns the correct interrupt vector address
 =======================================================================================================================
 */
uint32 SetException_Interrupt(uint32 pc)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	newpc = 0x80000180;
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	Count_Down(4 * VICounterFactors[CounterFactor]);

	if(CPUdelay != 0)				/* are we in branch delay slot? */
	{								/* yes */
		gHWS_COP0Reg[CAUSE] |= BD;
		gHWS_COP0Reg[EPC] = pc - 4;
		CPUdelay = 0;
	}
	else
	{								/* no */
		gHWS_COP0Reg[CAUSE] &= NOT_BD;
		gHWS_COP0Reg[EPC] = pc;
	}

	gHWS_COP0Reg[STATUS] |= EXL;	/* set EXL = 1 */

	/* to disable further interrupts */
	if((gHWS_COP0Reg[CAUSE] & EXCCODE) == TLBL_Miss || (gHWS_COP0Reg[CAUSE] & EXCCODE) == TLBS_Miss)
	{
		newpc = TLB_Error_Vector;
	}

	return newpc;
}

/*
 =======================================================================================================================
    Plugins will call this function to trigger an Audio Interface Interrupt, Display Processor Interrupt, or Signal
    Processor Interrupt.
 =======================================================================================================================
 */
void CheckInterrupts(void)
{
	if((MI_INTR_REG_R & MI_INTR_SP) != 0)
	{
		OPCODE_DEBUGGER_EPILOGUE(Trigger_RSPBreak();)
	}

	if((MI_INTR_REG_R & MI_INTR_DP) != 0)
	{
		OPCODE_DEBUGGER_EPILOGUE(Trigger_DPInterrupt();)
	}

	/*
	 * if ((MI_INTR_REG_R & MI_INTR_VI) != 0) { OPCODE_DEBUGGER_BEGIN_EPILOGUE
	 * Trigger_VIInterrupt(); OPCODE_DEBUGGER_END_EPILOGUE }
	 */
	if((MI_INTR_REG_R & MI_INTR_AI) != 0)
	{
		if(currentromoptions.DMA_Segmentation == USEDMASEG_YES)
			Set_Delay_AI_Interrupt_Timer_Event();
		else
		{
			OPCODE_DEBUGGER_EPILOGUE(Trigger_AIInterrupt();)
		}
	}
}

/*
 =======================================================================================================================
    A write to the MIPS® Interface Interrupt Mask Register will set or clear an interrupt bitmask flag(s) in this
    register.
 =======================================================================================================================
 */
void Handle_MI(uint32 value)
{
	if((value & MI_INTR_MASK_SP_CLR)) MI_INTR_MASK_REG_R &= ~MI_INTR_SP;
	if((value & MI_INTR_MASK_SI_CLR)) MI_INTR_MASK_REG_R &= ~MI_INTR_SI;
	if((value & MI_INTR_MASK_AI_CLR)) MI_INTR_MASK_REG_R &= ~MI_INTR_AI;
	if((value & MI_INTR_MASK_VI_CLR)) MI_INTR_MASK_REG_R &= ~MI_INTR_VI;
	if((value & MI_INTR_MASK_PI_CLR)) MI_INTR_MASK_REG_R &= ~MI_INTR_PI;
	if((value & MI_INTR_MASK_DP_CLR)) MI_INTR_MASK_REG_R &= ~MI_INTR_DP;
	if((value & MI_INTR_MASK_SI_SET)) MI_INTR_MASK_REG_R |= MI_INTR_SI;
	if((value & MI_INTR_MASK_VI_SET)) MI_INTR_MASK_REG_R |= MI_INTR_VI;
	if((value & MI_INTR_MASK_AI_SET)) MI_INTR_MASK_REG_R |= MI_INTR_AI;
	if((value & MI_INTR_MASK_PI_SET)) MI_INTR_MASK_REG_R |= MI_INTR_PI;
	if((value & MI_INTR_MASK_DP_SET)) MI_INTR_MASK_REG_R |= MI_INTR_DP;
	if((value & MI_INTR_MASK_SP_SET)) MI_INTR_MASK_REG_R |= MI_INTR_SP;

	/* Check MI interrupt again. This is important, otherwise we will lose interrupts. */
	if(MI_INTR_MASK_REG_R & 0x0000003F & MI_INTR_REG_R)
	{
		/* Trigger an MI interrupt since don't know what it is */
		SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP3;
		HandleInterrupts(0x80000180);
	}
}

/*
 =======================================================================================================================
    A write to the MIPS® Interface Mode Register will set or clear an interrupt bitmask flag(s) in this register.
 =======================================================================================================================
 */
void WriteMI_ModeReg(uint32 value)
{
	if(value & MI_CLR_RDRAM) MI_INIT_MODE_REG_R &= ~MI_MODE_RDRAM;
	if(value & MI_CLR_INIT) MI_INIT_MODE_REG_R &= ~MI_MODE_INIT;
	if(value & MI_CLR_EBUS) MI_INIT_MODE_REG_R &= ~MI_MODE_EBUS;
	if(value & MI_CLR_DP_INTR)
	{
		Clear_MIInterrupt(NOT_MI_INTR_DP);
	}

	if(value & MI_SET_INIT) MI_INIT_MODE_REG_R |= MI_MODE_INIT;
	if(value & MI_SET_EBUS) MI_INIT_MODE_REG_R |= MI_MODE_EBUS;
	if(value & MI_SET_RDRAM) MI_INIT_MODE_REG_R |= MI_MODE_RDRAM;

	if((MI_INTR_REG_R & MI_INTR_MASK_REG_R) == 0)
	{
		gHWS_COP0Reg[CAUSE] &= NOT_CAUSE_IP3;

		/* CPUNeedToCheckInterrupt = FALSE; */
	}
}

/*
 =======================================================================================================================
    A write to the Signal Processor Status Register will set or clear an interrupt bitmask flag(s) in this register.
 =======================================================================================================================
 */
void Handle_SP(uint32 value)
{
	if(value & SP_SET_HALT) (SP_STATUS_REG) |= SP_STATUS_HALT;
	if(value & SP_CLR_BROKE) (SP_STATUS_REG) &= ~SP_STATUS_BROKE;
	if(value & SP_CLR_INTR){Clear_MIInterrupt(NOT_MI_INTR_SP);}
	if(value & SP_SET_INTR){(MI_INTR_REG_R) |= MI_INTR_SP;
	if((MI_INTR_REG_R & MI_INTR_MASK_REG_R) != 0)
		{
			SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP3;
			HandleInterrupts(0x80000180);
		}
	}

	if(value & SP_CLR_SSTEP) (SP_STATUS_REG) &= ~SP_STATUS_SSTEP;
	if(value & SP_SET_SSTEP) (SP_STATUS_REG) |= SP_STATUS_SSTEP;
	if(value & SP_CLR_INTR_BREAK) (SP_STATUS_REG) &= ~SP_STATUS_INTR_BREAK;
	if(value & SP_SET_INTR_BREAK) (SP_STATUS_REG) |= SP_STATUS_INTR_BREAK;
	if(value & SP_CLR_YIELD) (SP_STATUS_REG) &= ~SP_STATUS_YIELD;
	if(value & SP_SET_YIELD) (SP_STATUS_REG) |= SP_STATUS_YIELD;
	if(value & SP_CLR_YIELDED) (SP_STATUS_REG) &= ~SP_STATUS_YIELDED;
	if(value & SP_SET_YIELDED) (SP_STATUS_REG) |= SP_STATUS_YIELDED;
	if(value & SP_CLR_TASKDONE) (SP_STATUS_REG) &= ~SP_STATUS_TASKDONE;
	if(value & SP_SET_TASKDONE) (SP_STATUS_REG) |= SP_STATUS_TASKDONE;
	if(value & SP_CLR_SIG3) (SP_STATUS_REG) &= ~SP_STATUS_SIG3;
	if(value & SP_SET_SIG3) (SP_STATUS_REG) |= SP_STATUS_SIG3;
	if(value & SP_CLR_SIG4) (SP_STATUS_REG) &= ~SP_STATUS_SIG4;
	if(value & SP_SET_SIG4) (SP_STATUS_REG) |= SP_STATUS_SIG4;
	if(value & SP_CLR_SIG5) (SP_STATUS_REG) &= ~SP_STATUS_SIG5;
	if(value & SP_SET_SIG5) (SP_STATUS_REG) |= SP_STATUS_SIG5;
	if(value & SP_CLR_SIG6) (SP_STATUS_REG) &= ~SP_STATUS_SIG6;
	if(value & SP_SET_SIG6) (SP_STATUS_REG) |= SP_STATUS_SIG6;
	if(value & SP_CLR_SIG7) (SP_STATUS_REG) &= ~SP_STATUS_SIG7;
	if(value & SP_SET_SIG7) (SP_STATUS_REG) |= SP_STATUS_SIG7;

	if(value & SP_CLR_HALT)
	{
		if ( ( value & SP_STATUS_BROKE ) == 0 )
		{
			(SP_STATUS_REG) &= ~SP_STATUS_HALT;
			DEBUG_SP_TASK_MACRO(TRACE0("SP Task is triggered"));
			sp_hle_task = HLE_DMEM_TASK;
			RunSPTask();
		}
	}

	/* Add by Rice, 2001.08.10 */
	SP_STATUS_REG |= SP_STATUS_HALT;
}

/*
 =======================================================================================================================
    A write to the DPC Status Register will set or clear an interrupt bitmask flag(s) in this register.
 =======================================================================================================================
 */
void Handle_DPC(uint32 value)
{
	if(value & DPC_CLR_XBUS_DMEM_DMA) (DPC_STATUS_REG) &= ~DPC_STATUS_XBUS_DMEM_DMA;
	if(value & DPC_SET_XBUS_DMEM_DMA) (DPC_STATUS_REG) |= DPC_STATUS_XBUS_DMEM_DMA;
	if(value & DPC_CLR_FREEZE) (DPC_STATUS_REG) &= ~DPC_STATUS_FREEZE;

	/*
	 * Modified by Rice. 2001.08.10 £
	 * if (value & DPC_SET_FREEZE) (DPC_STATUS_REG) |= DPC_STATUS_FREEZE;
	 */
	if(value & DPC_CLR_FLUSH) (DPC_STATUS_REG) &= ~DPC_STATUS_FLUSH;
	if(value & DPC_SET_FLUSH) (DPC_STATUS_REG) |= DPC_STATUS_FLUSH;
	if(value & DPC_CLR_TMEM_REG) (DPC_TMEM_REG) = 0;
	if(value & DPC_CLR_PIPEBUSY_REG) (DPC_PIPEBUSY_REG) = 0;
	if(value & DPC_CLR_BUFBUSY_REG) (DPC_BUFBUSY_REG) = 0;
	if(value & DPC_CLR_CLOCK_REG) (DPC_CLOCK_REG) = 0;
}

/*
 =======================================================================================================================
    Process a Signal Processor task. This is where we call audio/video plugin execution routines.
 =======================================================================================================================
 */
void RunSPTask(void)
{
	switch(sp_hle_task)
	{
	case GFX_TASK:
#ifndef CRASHABLE
		__try
#endif
		{
			DO_PROFILIER_VIDEO;
			if( rsp_plugin_is_loaded == TRUE && emuoptions.UsingRspPlugin == TRUE )
			{
				DoRspCycles(100);
				//RunRSP(100);
			}
			else
			{
				//RunRSP(100);
				VIDEO_ProcessDList();
			}

			emustatus.DListCount++;
			DPC_STATUS_REG = 0x801; /* Makes Banjo Kazooie work - Azimer */
			if((MI_INTR_REG_R & MI_INTR_DP) != 0) Trigger_DPInterrupt();
		}
#ifndef CRASHABLE
		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			strcpy(generalmessage, "Video Plugin exception.\nDo you want to continue emulation?");
			if((MessageBox(NULL, generalmessage, "Error", MB_YESNO | MB_ICONERROR)) == IDYES)
			{
				/* VIDEO_RomOpen(); */
			}
			else
			{
				emustatus.Emu_Keep_Running = FALSE;
				emustatus.reason_to_stop = VIDEOCRASH;
				countdown_counter = 0;
			}
		}
#endif
		DEBUG_SP_TASK_MACRO(TRACE0("SP GRX Task finished"));
		break;
	case SND_TASK:
		__try
		{
			DO_PROFILIER_AUDIO;

			if( rsp_plugin_is_loaded == TRUE && emuoptions.UsingRspPlugin == TRUE )
			{
				//RunRSP(100);
				DoRspCycles(100);
			}
			else
			{
				//RunRSP(100);
				AUDIO_ProcessAList();
			}

			emustatus.AListCount++;
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			/* DisplayError("Memory exception fires to process AUDIO DList"); */
			DO_PROFILIER_R4300I
		}

		if((MI_INTR_REG_R & MI_INTR_DP) != 0) Trigger_DPInterrupt();
		DEBUG_SP_TASK_MACRO(TRACE0("SP SND Task finished"));
		break;
	default:
		__try
		{
			DO_PROFILIER_RDP;
			if( rsp_plugin_is_loaded == TRUE && emuoptions.UsingRspPlugin == TRUE )
			{
				//RunRSP(100);
				DoRspCycles(100);
				//VIDEO_ProcessRDPList();
			}
			else
			{
				//RunRSP(100);
				VIDEO_ProcessRDPList();
			}

			/* VIDEO_ProcessDList(); */
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			/* DisplayError("Memory exception fires to process RDP List"); */
		}

		if((MI_INTR_REG_R & MI_INTR_DP) != 0) Trigger_DPInterrupt();

		DEBUG_SP_TASK_MACRO(TRACE0("Unknown SP Taks"));
		break;
	}

	DO_PROFILIER_R4300I;
	if(currentromoptions.DMA_Segmentation == USEDMASEG_YES && emuoptions.UsingRspPlugin == FALSE ) 
		Set_SP_DLIST_Timer_Event(SPTASKPCLOCKS);
	else
		Trigger_RSPBreak();
}

/*
 =======================================================================================================================
    Flags an Audio Interface Interrupt for later execution.
 =======================================================================================================================
 */
void Trigger_AIInterrupt(void)
{
	DEBUG_AI_INTERRUPT_TRACE(TRACE0("AI Interrupt is triggered"););

	/* set the interrupt to fire */
	(MI_INTR_REG_R) |= MI_INTR_AI;
	if((MI_INTR_MASK_REG_R) & MI_INTR_AI)
	{
		SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP3;
		HandleInterrupts(0x80000180);
	}
}

/*
 =======================================================================================================================
    Flags a Compare Interrupt for later execution. (COUNT register = COMPARE register)
 =======================================================================================================================
 */
void Trigger_CompareInterrupt(void)
{
	DEBUG_COMPARE_INTERRUPT_TRACE
	(
	TRACE0("COUNT Interrupt is triggered"); TRACE3
		(
			"COUNT = %08X, COMPARE= %08X, PC = %08X",
			gHWS_COP0Reg[COUNT],
			gHWS_COP0Reg[COMPARE],
			gHWS_pc
		);
	);

	/* set the compare interrupt flag (ip7) */
	SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP8;

	HandleInterrupts(0x80000180);
}

/*
 =======================================================================================================================
    Flags an Display Processor Interrupt for later execution.
 =======================================================================================================================
 */
void Trigger_DPInterrupt(void)
{
	/* set the interrupt to fire */
	(MI_INTR_REG_R) |= MI_INTR_DP;
	if((MI_INTR_MASK_REG_R) & MI_INTR_DP)
	{
		SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP3;
		HandleInterrupts(0x80000180);
	}
}

/*
 =======================================================================================================================
    Flags a Video Interface Interrupt for later execution. (color frame buffer (CFB) display)
 =======================================================================================================================
 */
void Trigger_VIInterrupt(void)
{

#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY9
	if((debug_opcode != 1) || ((debug_opcode) && (p_gHardwareState == &gHardwareState)))
#endif

	/* only do the VI updatescreen for dyna, not for interpreter compare */
	{
		DEBUG_VI_INTERRUPT_TRACE(TRACE0("VI Interrupt is triggered"););

		/* DEBUG_VI_INTERRUPT_TRACE(TRACE1("VI counter = %08X", Get_VIcounter()); */
		DO_PROFILIER_VIDEO;

		VIDEO_UpdateScreen();

		DO_PROFILIER_R4300I;
		viCountePerSecond++;
		if(Audio_Is_Initialized == 1)
		{
			DO_PROFILIER_AUDIO;
			if( CoreDoingAIUpdate )
			{
				AUDIO_AiUpdate(FALSE);
			}

			DO_PROFILIER_R4300I;
		}
	}

	/* Speed Sync */
	if(currentromoptions.Max_FPS != MAXFPS_NONE && emuoptions.SyncVI)
	{
		QueryPerformanceCounter(&CurrentTime);
		Elapsed.QuadPart = CurrentTime.QuadPart - LastSecondTime.QuadPart;
		tempvips = viCountePerSecond / ((double) Elapsed.QuadPart / (double) Freq.QuadPart);

		/* if( tempvips > vips_speed_limits[currentromoptions.Max_FPS]+1.0) */
		if(tempvips > vips_speed_limits[currentromoptions.Max_FPS])
		{
			DO_PROFILIER_CPU_IDLE

			/* TRACE1("tempvips = %f", (float)tempvips); */
			Elapsed.QuadPart = CurrentTime.QuadPart -
			LastVITime.QuadPart;

			sleeptime = 1000.00f /
			(vips_speed_limits[currentromoptions.Max_FPS]) -
			(double) Elapsed.QuadPart /
			(double) Freq.QuadPart *
			1000.00;
			do
			{
				if(sleeptime > 0)
				{
					if(sleeptime > 1.5)
					{
						Sleep(1);
					}
					else
					{
						/*~~~~~~*/
						uint32	i;
						/*~~~~~~*/

						for(i = 0; i < 1000 * sleeptime; i++)
						{ ;
						}	/* busy wait */
					}
				}

				QueryPerformanceCounter(&CurrentTime);
				Elapsed.QuadPart = CurrentTime.QuadPart -
				LastVITime.QuadPart;
				sleeptime = 1000.00f /
				(vips_speed_limits[currentromoptions.Max_FPS]) -
				(float) Elapsed.QuadPart /
				(float) Freq.QuadPart *
				1000.00;
			} while(sleeptime > 0.1);

			DO_PROFILIER_R4300I
		}

		LastVITime = CurrentTime;
	}

	
	if ((VI_STATUS_REG & 0x00000010) != 0) // Bit [4]     = divot_enable (normally on if antialiased, unless decal lines)
	{
		vi_field_number = 1 - vi_field_number;
	} 
	else 
	{
		vi_field_number = 0;
	}

	/* Apply the hack codes */
	if(emuoptions.auto_apply_cheat_code)
	{
#ifndef CHEATCODE_LOCK_MEMORY
		CodeList_ApplyAllCode(INGAME);
#endif
	}

	/* set the interrupt to fire */
	(MI_INTR_REG_R) |= MI_INTR_VI;
	if((MI_INTR_MASK_REG_R) & MI_INTR_MASK_VI)
	{
		SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP3;
		HandleInterrupts(0x80000180);
	}
}

/*
 =======================================================================================================================
    Flags a Serial Interface Controller interrupt for later execution
 =======================================================================================================================
 */
void Trigger_SIInterrupt(void)
{
	MI_INTR_REG_R |= MI_INTR_SI;

	if((MI_INTR_MASK_REG_R) & MI_INTR_MASK_SI)
	{
		SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP3;
		HandleInterrupts(0x80000180);
	}
}

/*
 =======================================================================================================================
    Flags a Peripheral Interface interrupt for later execution
 =======================================================================================================================
 */
void Trigger_PIInterrupt(void)
{
	MI_INTR_REG_R |= MI_INTR_PI;	/* Set PI Interrupt */

	if((MI_INTR_MASK_REG_R) & MI_INTR_MASK_PI)
	{
		SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP3;
		HandleInterrupts(0x80000180);
	}
}

/*
 =======================================================================================================================
    Flags a Signal Processor interrupt for later execution
 =======================================================================================================================
 */
void Trigger_SPInterrupt(void)
{
	MI_INTR_REG_R |= MI_INTR_SP;

	if((MI_INTR_MASK_REG_R) & MI_INTR_MASK_SP)
	{
		SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP3;
		HandleInterrupts(0x80000180);
	}
}

/*
 =======================================================================================================================
    Halts the Signal Processor
 =======================================================================================================================
 */
void Trigger_RSPBreak(void)
{
	/* set the status flags */
	(SP_STATUS_REG) |= 0x00000203;

	/* set the sp interrupt when wanted */
	if((SP_STATUS_REG) & SP_STATUS_INTR_BREAK)
	{
		Trigger_SPInterrupt();
	}

	/* Add by Rice 2001.08.10 */
	SP_STATUS_REG |= SP_STATUS_HALT;
}

/*
 =======================================================================================================================
    Call this function to clear AI/VI/SI/SP/DP interrupts
 =======================================================================================================================
 */
void Clear_MIInterrupt(uint32 clear_mask)
{
	MI_INTR_REG_R &= clear_mask;
	if((MI_INTR_REG_R & MI_INTR_MASK_REG_R) == 0)
	{
		gHWS_COP0Reg[CAUSE] &= NOT_CAUSE_IP3;
		if((gHWS_COP0Reg[CAUSE] & gHWS_COP0Reg[STATUS] & SR_IMASK) == 0) CPUNeedToCheckInterrupt = FALSE;
	}
}

/*
 =======================================================================================================================
    Should flag an Address Error exception, but just displays an error right now.
 =======================================================================================================================
 */
void Trigger_Address_Error_Exception(uint32 addr, char *opcode, int exception)
{
	DisplayError("Memory access not aligned, PC=%08X, opcode=%s", gHWS_pc, opcode);
	TRACE3("Memory access not aligned, PC=%08X, opcode=%s, Bad Vaddr=%08X", gHWS_pc, opcode, addr);

	TRACE0("Should fire Address Error Exception, but we skipped here");
#ifdef DEBUG_COMMON
	/*
	 * SET_EXCEPTION(exception) £
	 * gHWS_COP0Reg[BADVADDR] = addr; £
	 * HandleExceptions(0x80000180);
	 */
#endif
}

/*
 =======================================================================================================================
    Call this function to clear AI/VI/SI/SP/DP interrupts
 =======================================================================================================================
 */

void TriggerFPUUnusableException(void)
{
	SET_EXCEPTION(EXC_CPU) 
	gHWS_COP0Reg[CAUSE] &= 0xCFFFFFFF;
	gHWS_COP0Reg[CAUSE] |= CAUSE_CE1;

	/*
	 * Should test the CPU Delay slot bit £
	 * gHWS_COP0Reg[EPC] = gHWS_pc;
	 */
	TRACE0("FPU Unusable Exception");

#ifdef ENABLE_OPCODE_DEBUGGER
	if(emustatus.cpucore == DYNACOMPILER)
		Dyna_Exception_Service_Routine(0x80000180);
	else
	{
		CPUNeedToDoOtherTask = TRUE;
		CPUNeedToCheckException = TRUE;
	}

#else
	HandleExceptions(0x80000180);
#endif
}

/*
 =======================================================================================================================
    Calling this function will enable detection of Coprocessor Unusable Exceptions.
 =======================================================================================================================
 */
void EnableFPUUnusableException(void)
{
	/* TRACE0("Enable FPU Exception"); */
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY10
	if(debug_opcode != 1)
#endif
		dyna_instruction[0x11] = dyna4300i_cop1_with_exception; /* this is for dyna */

#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY11
	else
		now_do_dyna_instruction[0x11] = dyna4300i_cop1_with_exception;
#endif
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY12
	if(debug_opcode != 1 || emustatus.cpucore == INTERPRETER)
#endif
		CPU_instruction[0x11] = COP1_NotAvailable_instr;		/* this is for interpreter */
}

/*
 =======================================================================================================================
    Calling this function will disable detection of Coprocessor Unusable Exceptions.
 =======================================================================================================================
 */
void DisableFPUUnusableException(void)
{
	/* TRACE0("Disable FPU Exception"); */
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY13
	if(debug_opcode != 1)
#endif
	{
		dyna_instruction[0x11] = dyna4300i_cop1;	/* this is for dyna */
	}

#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY14
	else
		now_do_dyna_instruction[0x11] = dyna4300i_cop1;
#endif
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY15
	if(debug_opcode != 1 || emustatus.cpucore == INTERPRETER)
#endif
		CPU_instruction[0x11] = COP1_instr;			/* this is for interpreter */
}

/*
 =======================================================================================================================
    This exception is triggered when an integer overflow or divide by zero condition happens
 =======================================================================================================================
 */
void TriggerIntegerOverflowException(void)
{
	SET_EXCEPTION(EXC_OV) TRACE0("Integer Overflow Exception is triggered");
	HandleExceptions(0x80000180);
}

/*
 =======================================================================================================================
    This exception is triggered when an fpu exception happens
 =======================================================================================================================
 */
void TriggerGeneralFPUException(void)
{
	SET_EXCEPTION(EXC_FPE) TRACE0("FPU Exception is triggered");
	HandleExceptions(0x80000180);
}

/*
 =======================================================================================================================
    Call this function to setup interrupt triggers
 =======================================================================================================================
 */
void HandleInterrupts(uint32 vt)
{
	/* Set flag for interrupt service */
	CPUNeedToCheckInterrupt = TRUE;
	CPUNeedToDoOtherTask = TRUE;
	Set_Check_Interrupt_Timer_Event();
}

/*
 =======================================================================================================================
    Call this function to setup exception triggers
 =======================================================================================================================
 */
void HandleExceptions(uint32 evt)
{
	if(gHWS_COP0Reg[STATUS] & EXL_OR_ERL)	/* Exception in exception */
	{
		TRACE1("Warning, Exception happens in exception, the new exception is %d", (0x7C & gHWS_COP0Reg[CAUSE]) >> 2);
	}

	if(emustatus.cpucore == DYNACOMPILER)
	{
		Dyna_Exception_Service_Routine(evt);
	}
	else
	{
		CPUNeedToDoOtherTask = TRUE;
		CPUNeedToCheckException = TRUE;
	}
}

/*
 =======================================================================================================================
    This function is for debug purposes
 =======================================================================================================================
 */
void Trigger_Interrupt_Without_Mask(uint32 interrupt)
{
	/* set the interrupt to fire */
	(MI_INTR_REG_R) |= interrupt;
	SET_EXCEPTION(EXC_INT) gHWS_COP0Reg[CAUSE] |= CAUSE_IP3;
	HandleInterrupts(0x80000180);
}
