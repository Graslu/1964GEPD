/*
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    This file boots the n64 and starts the emulation thread.
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
#include "globals.h"
#include "debug_option.h"
#include "emulator.h"
#include "interrupt.h"
#include "r4300i.h"
#include "n64rcp.h"
#include "dma.h"
#include "timer.h"
#include "memory.h"
#include "cheatcode.h"
#include "1964ini.h"
#include "compiler.h"
#include "win32/DLL_Video.h"
#include "win32/DLL_Audio.h"
#include "win32/DLL_Input.h"
#include "win32/DLL_Rsp.h"
#include "win32/windebug.h"
#include "win32/wingui.h"
#include "dynarec/dynaLog.h"
#include "dynarec/dynarec.h"
#include "netplay.h"
#include "romlist.h"

#ifdef DEBUG_COMMON
#include "win32/windebug.h"
extern char			*DebugPrintInstruction(uint32 instruction);
extern char		    *Get_Interrupt_Name(void);
#endif
extern char			*DebugPrintInstructionWithOutRefresh(uint32 Instruction);
extern char			*DebugPrintInstr(uint32 Instruction);
extern uint32		SetException_Interrupt(uint32 pc);
extern void			rc_Intr_Common(void);

void				RunTheInterpreter(void);
void				RunTheRegCacheWithoutOpcodeDebugger(void);
void				RunTheRegCacheWithOpcodeDebugger(void);
void				RunTheRegCacheNoCheck(void);
HANDLE				CPUThreadHandle = NULL;

LONG				Audio_Thread_Keep_Running = FALSE;
struct EmuStatus	emustatus;
struct EmuOptions	emuoptions;
uint32				CPUdelayPC;		/* the saved Program Counter at CPU load/branch delay mode */
uint32				CPUdelay;		/* Describer if the CPU is in load/branch delay mode */
HardwareState		gHardwareState, *p_gHardwareState;
MemoryState			gMemoryState, *p_gMemoryState;
uint32				*g_LookupPtr;	/* This global will be set at returning from a block */
uint32				g_pc_is_rdram;	/* This global will be set at returning from a block */
BOOL				IsBooting = FALSE;
BOOL				NeedToApplyRomWriteHack = FALSE;

void (__cdecl StartCPUThread) (void *pVoid);

//This only mutes when paused.
void __cdecl MuteTest(void* dummy)
{
	int k;

	//This is enough times to cleanup the azi audio
	//and enough times to keep jabo directsound 1.5
	//from crashing. This is a shortcoming of the spec.
	for (k=0;k<10;k++)
	{
		AUDIO_AiUpdate(TRUE);
	}
	AUDIO_AiUpdate(FALSE);
	_endthread();
}

void Mute()
{
	AUDIO_AiUpdate(FALSE);
	_beginthread(MuteTest, 0, NULL);
}

/*
 =======================================================================================================================
    Called by GUI thread to start emulating £
 =======================================================================================================================
 */
void RunEmulator(uint32 core)
{
	if(!Rom_Loaded)
	{
		return;
	}

	emustatus.Emu_Keep_Running = TRUE;
	Audio_Thread_Keep_Running = TRUE;
	AUDIO_Initialize(Audio_Info);
	emustatus.cpucore = core;
#ifndef CRASHABLE
    __try {
#endif
    CPUThreadHandle = (HANDLE) _beginthread(StartCPUThread, 0, NULL);
#ifndef CRASHABLE
    }__except(NULL, EXCEPTION_EXECUTE_HANDLER)
	{
		DisplayError("Emulation stopped. Please restart 1964.exe");
		emustatus.reason_to_stop = CPUCRASH;	/* Quit like VIDEO plugin crash */
	}
#endif

}

/*
 =======================================================================================================================
    Called by GUI thread to pause emulating £
 =======================================================================================================================
 */
BOOL PauseEmulator(void)
{
	if(emustatus.Emu_Is_Paused) return TRUE;

	if(emustatus.exception_entry_count)			/* || ITLB_Error || (gHWS_COP0Reg[STATUS] & EXL) ) */
	{
		/*~~*/
		int i;
		/*~~*/

		for(i = 0; i < 100; i++)
		{
			Sleep(1);
			if(emustatus.exception_entry_count) /* || ITLB_Error || (gHWS_COP0Reg[STATUS] & EXL) ) */
				continue;
			else
				goto step2;
		}

		return FALSE;
	}

step2:
	emustatus.reason_to_stop = EMUPAUSE;
	emustatus.Emu_Keep_Running = FALSE;
	CPU_Task_To_String(generalmessage);
	TRACE1("Try to pause, CPU is busy doing: %s", generalmessage);

	while(!emustatus.Emu_Is_Paused && emustatus.Emu_Is_Running)
	{
		/*~~~~*/
		MSG msg;
		/*~~~~*/

		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
			{
				if(GetMessage(&msg, NULL, 0, 0)) DispatchMessage(&msg);
			}
		}

		emustatus.Emu_Keep_Running = FALSE;
		emustatus.reason_to_stop = EMUPAUSE;
		Sleep(50);
	}

	sprintf(generalmessage, "%s - Paused", gui.szWindowTitle);
	SetStatusBarText(0, generalmessage);
	SetWindowText(gui.hwnd1964main, generalmessage);
	Mute();
	return TRUE;
}

/*
 =======================================================================================================================
    Called by GUI thread to resume emulating from pausing £
    the global variable "needinit" is to pass information to the CPU thread £
    to do InitEmu() in CPU thread. Reason behind this is that OpenGL is thread-safe, £
	initialization must be done in the CPU thread £
 =======================================================================================================================
 */
void ResumeEmulator(int action_after_pause)
{
	if(!emustatus.Emu_Is_Paused) return;

	emustatus.action_after_resume = action_after_pause;

	/* Apply the hack codes */
	if(emuoptions.auto_apply_cheat_code)
	{
		CodeList_ApplyAllCode(INGAME);
#ifdef CHEATCODE_LOCK_MEMORY
		InitCheatCodeEngineMemoryLock();
#endif
	}

	emustatus.Emu_Keep_Running = TRUE;
	sprintf(generalmessage, "%s - Running", gui.szWindowTitle);
	SetStatusBarText(0, generalmessage);
	SetWindowText(gui.hwnd1964main, generalmessage);
	CheckButton(ID_BUTTON_PLAY, TRUE);
	CheckButton(ID_BUTTON_PAUSE, FALSE);
	QueryPerformanceCounter(&LastSecondTime);
}

/*
 =======================================================================================================================
    Called by GUI thread to stop emulating £
 =======================================================================================================================
 */
void StopEmulator(void)
{
	emustatus.Emu_Keep_Running = FALSE;
	emustatus.reason_to_stop = EMUSTOP;
	CPU_Task_To_String(generalmessage);
	TRACE1("Try to pause, CPU is busy doing: %s", generalmessage);

	while(emustatus.Emu_Is_Running)
	{
		/*~~~~*/
		MSG msg;
		/*~~~~*/

		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
			{
				if(GetMessage(&msg, NULL, 0, 0)) DispatchMessage(&msg);
			}
		}

		emustatus.Emu_Keep_Running = FALSE;
		emustatus.reason_to_stop = EMUSTOP;
		Sleep(50);
	}

	if (!emustatus.Emu_Is_Resetting)
	{
		VIDEO_RomClosed();
		AUDIO_RomClosed();
		CONTROLLER_RomClosed();
		if( emuoptions.UsingRspPlugin )
		{
			RSPRomClosed();
		}
		netplay_rom_closed();
	}
}

/*
 =======================================================================================================================
    Called by GUI thread to switch CPU core while emulating £
 =======================================================================================================================
 */
void EmulatorSetCore(int core)
{
	if(emustatus.Emu_Is_Running)
	{
		if(emustatus.cpucore != core)
		{
			if(PauseEmulator())
			{
				TRACE2("Switch CPU Core to %s, PC=%08X", emulator_type_names[core], gHWS_pc);
				emustatus.cpucore = core;
				ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);
			}
		}
	}
	else	/* Emulator is not running, then change default CPU core */
	{
		defaultoptions.Emulator = core;
	}

	SetStatusBarText(4, core == DYNACOMPILER ? "D" : "I");
}

/*
 * All functions above are called by GUI £
 * All functions below are used in the CPU thread £
 * £
 */
uint32	RDRamSizeHackSavedDWord1 = 0;
uint32	RDRamSizeHackSavedDWord2 = 0;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CloseEmulator(void)
{
	Audio_Thread_Keep_Running = FALSE;	/* this will quit the Audio thread */
	if(currentromoptions.Code_Check == CODE_CHECK_PROTECT_MEMORY) UnprotectAllBlocks();

	if(emustatus.reason_to_stop == VIDEOCRASH || emustatus.reason_to_stop == CPUCRASH)
	{
		PostMessage(gui.hwnd1964main, WM_COMMAND, ID_ROM_STOP, 0);
		emustatus.Emu_Keep_Running = TRUE;
		while(emustatus.Emu_Keep_Running)
		{
			Sleep(50);					/* wait until wingui processing the STOP command */
		}
	}

	emustatus.Emu_Is_Running = FALSE;
	Free_Dynarec();

	*(uint32 *) &gMS_RDRAM[rominfo.RDRam_Size_Hack] = RDRamSizeHackSavedDWord1;
	*(uint32 *) &gMS_RDRAM[0x2FE1C0] = RDRamSizeHackSavedDWord2;
}

/*
 =======================================================================================================================
    £
 =======================================================================================================================
 */
void RefreshDynaDuringGamePlay(void)
{
	Init_Dynarec();
	Set_Translate_PC();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
extern BOOL write_to_rom_flag;

void InitEmu(void)
{
	if(GetVersion() < 0x80000000)	/* Windows NT */
		SetThreadPriority(CPUThreadHandle, THREAD_PRIORITY_NORMAL);
	else
		SetThreadPriority(CPUThreadHandle, THREAD_PRIORITY_HIGHEST);

	FR_reg_offset = (gHWS_COP0Reg[STATUS] & 0x04000000) ? 32 : 1;
	CPUdelay = 0;
	CPUdelayPC = 0;

	CPUNeedToDoOtherTask = FALSE;
	CPUNeedToCheckInterrupt = FALSE;
	emustatus.Emu_Is_Paused = FALSE;
	emustatus.gepd_pause = 0;
	write_to_rom_flag = FALSE;
	emustatus.exception_entry_count = 0;
	emustatus.action_after_resume = DO_NOTHING_AFTER_PAUSE;
	compilerstatus.lCodePosition = 0;
	Block = 0;
	FPU_Is_Enabled = FALSE;
	compilerstatus.Is_Compiling = 0;
	vips = vips_speed_limits[currentromoptions.Max_FPS];
	framecounter = 0;
	viCountePerSecond = 0;
	vi_field_number = 0;
	QueryPerformanceCounter(&LastVITime);
	QueryPerformanceCounter(&LastSecondTime);

	Init_Count_Down_Counters();

	memcpy(&HeaderDllPass[0], &gMS_ROM_Image[0], 0x40);
	
	if (!emustatus.Emu_Is_Resetting)
	{
		VIDEO_RomOpen();
		CONTROLLER_RomOpen();
		netplay_rom_open();
	}
	else
		emustatus.Emu_Is_Resetting = 0;

	RefreshDynaDuringGamePlay();
}

/*
 =======================================================================================================================
    This function is called at the beginning of emulating, runs until boot successfully £
 =======================================================================================================================
 */
void N64_Boot(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	int		RDRam_Hacked = 0;
	uint32	bootaddr = (*(uint32 *) (gMS_ROM_Image + 8) & 0x007FFFFF) + 0x80000000;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	emustatus.Emu_Is_Running = TRUE;
	IsBooting = TRUE;
	
	NeedToApplyRomWriteHack = FALSE;
	if( strnicmp(currentromoptions.Game_Name, "A Bug's Life", 12) == 0 ||
		strnicmp(currentromoptions.Game_Name, "Toy Story 2", 11) == 0 )
	{
		NeedToApplyRomWriteHack = TRUE;
		TRACE0("Using Rom Write Hack");
	}

	while
	(
		(emustatus.Emu_Keep_Running && gHWS_pc != bootaddr && (gHWS_pc & 0x00FFFFFF) < 0x2000)
	||	((uint32) (*Dyna_Check_Codes) == (uint32) Dyna_Code_Check_None_Boot)
	)
	{
		if(emustatus.cpucore == INTERPRETER)
		{
			InterpreterStepCPU();
		}
		else
			RunDynaBlock();

		if(RDRam_Hacked == 0)
		{
			if
			(
				(currentromoptions.DMA_Segmentation == USEDMASEG_YES && DMAInProgress)
			||	(emuoptions.dma_in_segments == FALSE && (MI_INTR_REG_R & MI_INTR_PI))
			)
			{
				RDRam_Hacked = 1;

				RDRamSizeHackSavedDWord1 = *(uint32 *) &gMS_RDRAM[rominfo.RDRam_Size_Hack];

				OPCODE_DEBUGGER_EPILOGUE(*(uint32 *) &gMS_RDRAM[rominfo.RDRam_Size_Hack] = current_rdram_size;) RDRamSizeHackSavedDWord2 = *(uint32 *) &gMS_RDRAM[0x2FE1C0];

				/*
				 * Azimer - DK64 Hack to break out of infinite loop £
				 * I believe this memory location is some sort of copyright protection which £
				 * is written to using the RSP on bootup. The only issue I see is if it £
				 * affects any other roms?
				 */
				if(strncmp(currentromoptions.Game_Name, "DONKEY KONG 64", 14) == 0)
				/*
				 * if( currentromoptions.crc1 == 0xEC58EABF && currentromoptions.crc2 ==
				 * 0xAD7C7169 ) //DK64
				 */
				{
					OPCODE_DEBUGGER_EPILOGUE(*(uint32 *) &gMS_RDRAM[0x2FE1C0] = 0xAD170014;)
				}
			}
		}
	}

	if(emuoptions.auto_apply_cheat_code)
	{
		CodeList_ApplyAllCode(BOOTUPONCE);
		CodeList_ApplyAllCode(INGAME);
#ifdef CHEATCODE_LOCK_MEMORY
		InitCheatCodeEngineMemoryLock();
#endif
	}

	emustatus.Emu_Is_Running = FALSE;
	IsBooting = FALSE;

	if(gHWS_pc == bootaddr)
	{
		TRACE1("N64 boot successfully, start run from %08X", bootaddr)
	}
	else
	{
		TRACE1("N64 boot failed, start run from %08X", gHWS_pc)
	}
}

void __cdecl	LogDyna(char *debug, ...);

/*
 * This is the entry point for CPU emulating thread £
 */
void (__cdecl StartCPUThread) (void *pVoid)
{
    TRACE0("");
	TRACE0("");
	TRACE0("");
	TRACE1("Starting ROM %s", rominfo.name)
	TRACE0("");
	TRACE0("");
	TRACE0("");

	p_gHardwareState = (HardwareState *) &gHardwareState;
	p_gMemoryState = (MemoryState *) &gMemoryState;
	InitEmu();
	N64_Boot();
	if(mouseinjectorpresent)
		CONTROLLER_HookROM((DWORD *)gMemoryState.ROM_Image); // hot patch rom here

	emustatus.reason_to_stop = EMURUNNING;
	DO_PROFILIER_R4300I
	{
START_CPU_THREAD:
		emustatus.Emu_Is_Running = TRUE;
		switch(emustatus.cpucore)
		{
		case INTERPRETER:
			TRACE0("Start Interpreter");
			RunTheInterpreter();
			break;
		case DYNACOMPILER:
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY4
			if(debug_opcode != 0)
			{
                TRACE0("Start RunTheRegCacheWithOpcodeDebugger");
                RunTheRegCacheWithOpcodeDebugger();
			}
			else
#endif
				if((uint32) (*Dyna_Check_Codes) == (uint32) Dyna_Code_Check_None)
				{
					TRACE0("Start RunTheRegCacheNoCheck");
					RunTheRegCacheNoCheck();
				}
				else
				{
					TRACE0("Start RunTheRegCacheWithoutOpcodeDebugger");
					RunTheRegCacheWithoutOpcodeDebugger();
				}
			break;
		}
		if(emustatus.reason_to_stop == EMUPAUSE)
		{
			/*
			 * If user is saving state after pausing, need to update the COUNT register here £
			 * so we can return back to the original timer value £
			 * This make Donkey Kong can be saved state
			 */
			gHWS_COP0Reg[COUNT] = Get_COUNT_Register();

			PauseEmulating();
			goto START_CPU_THREAD;
		}
	}
	stop_profiling();
	CloseEmulator();
    _endthreadex(0);

}

/*
 =======================================================================================================================
    Use in CPU thread, will pausing emu and wait for resume £
 =======================================================================================================================
 */
void PauseEmulating(void)
{
	emustatus.Emu_Is_Paused = TRUE;
	while(emustatus.Emu_Keep_Running == FALSE && emustatus.reason_to_stop == EMUPAUSE)
	{
		emustatus.Emu_Is_Paused = TRUE;
		Sleep(200);

		/*
		 * VIDEO_UpdateScreen(); £
		 * VIDEO_DrawScreen();
		 */
	}

	emustatus.Emu_Is_Paused = FALSE;

	if(emustatus.Emu_Keep_Running)
	{
		Free_Dynarec();

		if(emustatus.action_after_resume == INIT_EMU_AFTER_PAUSE)
			InitEmu();
		else if(emustatus.action_after_resume == REFRESH_DYNA_AFTER_PAUSE)
			RefreshDynaDuringGamePlay();

		/* else //do nothing */
	}
}

/*
 =======================================================================================================================
    This is a main loop for emulating in interpreter £
 =======================================================================================================================
 */
void RunTheInterpreter(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/
	register uint32 Instruction;
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/

_DoOtherTask:
	Instruction = FetchInstruction();	/* Fetch instruction at PC */
	CPU_instruction[_OPCODE_](Instruction);
	gHWS_GPR[0] = 0;
	INTERPRETER_DEBUG_INSTRUCTION(Instruction);

	if(CPUNeedToCheckException)
	{
		gHWS_pc = SetException_Interrupt(gHWS_pc);
		TRACE3
		(
			"Start Exception %d, EPC=%08X, PC=%08X",
			(gHWS_COP0Reg[CAUSE] & EXCCODE) >> 2,
			gHWS_COP0Reg[EPC],
			gHWS_pc
		) CPUNeedToCheckException = FALSE;
	}
	else
	{
		switch(CPUdelay)
		{
		case 0:		gHWS_pc += 4; break;
		case 1:		gHWS_pc += 4; CPUdelay = 2; break;
		default:	gHWS_pc = CPUdelayPC; CPUdelay = 0; if(!emustatus.Emu_Keep_Running) goto out; break;
		}

		if(countdown_counter <= 0) Trigger_Timer_Event();
	}

	countdown_counter -= VICounterFactors[CounterFactor];
	goto _DoOtherTask;

out:
	if(Is_CPU_Doing_Other_Tasks() || CPUdelay != 0) goto _DoOtherTask;
}

/*
 * This is a main loop for emulating in Dynarec £
 */
uint32	HardwareStart = (uint32) & gHardwareState + 128;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RunTheRegCacheWithoutOpcodeDebugger(void)
{
	/*
	 * __asm pushad // nono. This causes stack overflow in Minimize size compiler
	 * option.
	 */

	__asm mov ebp, HardwareStart
	while(emustatus.Emu_Keep_Running)
	{
_DoOtherTask:
		Block = (uint8 *) *g_LookupPtr;
		if(Block != NULL && g_pc_is_rdram) 
			Dyna_Check_Codes();

		if(Block == NULL)
		{
			Dyna_Compile_Block();
		}

		/* Run the compiled code in the Block */
		DEBUG_PRINT_DYNA_EXECUTION_INFO;
		__asm call Block;
		
		if(countdown_counter > 0)
			goto _DoOtherTask;

		Trigger_Timer_Event();
	}

	if( emustatus.reason_to_stop != EMUSTOP && Is_CPU_Doing_Other_Tasks() ) 
		goto _DoOtherTask;

	/* __asm popad //nono. */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RunTheRegCacheWithOpcodeDebugger(void)
{
	/*
	 * __asm pushad // nono. This causes stack overflow in Minimize size compiler
	 * option.
	 */

	__asm mov ebp, HardwareStart
	while(emustatus.Emu_Keep_Running)
	{
_NextBlock:
		__asm
		{
			mov eax, g_LookupPtr
			mov eax, [eax]
			mov Block, eax
			cmp eax, 0
			je l2
			mov eax, g_pc_is_rdram
			cmp eax, 0
			je l3
			call Dyna_Check_Codes
			mov eax, Block
			cmp eax, 0
			jnz l3
			l2 : call Dyna_Compile_Block
			l3 :
		}

		BlockStartPC = gHardwareState.pc;

		/* Run the compiled code in the Block */
		DEBUG_PRINT_DYNA_EXECUTION_INFO;
		__asm call Block

		if(debug_opcode!=0)
		{
			if(CPUdelay == 1) gHardwareState_Interpreter_Compare.pc += 4;

			if(gHardwareState.pc != CPUdelayPC && CPUdelay != 0)
			{
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
				uint32	Inst;
				uint32	savedpc = gHardwareState.pc;
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

				gHardwareState.pc = gHardwareState_Interpreter_Compare.pc;
				Inst = FetchInstruction();

				sprintf
				(
					generalmessage,
					"Compare Target PC error: pc=%08X, DelayPC=%08X, Before Jump PC=%08X\nDyna:%s\nInterpreter:",
					savedpc,
					CPUdelayPC,
					gHardwareState_Interpreter_Compare.pc,
					DebugPrintInstr(Inst)
				);
				gHardwareState.pc = savedpc;
				COMPARE_SwitchToInterpretive();
				strcat(generalmessage, DebugPrintInstr(Inst));
				TRACE0(generalmessage);
				DisplayError(generalmessage);
				COMPARE_SwitchToDynarec();
			}

			CPUdelay = 0;
			PcBeforeBranch = gHardwareState_Interpreter_Compare.pc;
			gHardwareState_Interpreter_Compare.pc = gHardwareState.pc;
		}

		if(countdown_counter > 0)
			goto _NextBlock;

		Trigger_Timer_Event();
	}

	if( emustatus.reason_to_stop != EMUSTOP && Is_CPU_Doing_Other_Tasks() ) 
		goto _NextBlock;

	/* __asm popad //nono. */
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RunTheRegCacheNoCheck(void)
{
	__asm mov ebp, HardwareStart

	while(emustatus.Emu_Keep_Running)
	{
_NextBlock:
		__asm
		{
l1:			mov eax, g_LookupPtr
			mov eax, [eax]
			test eax, eax
			je l2
			call eax
			cmp countdown_counter, 0
			jg l1
			jmp l3
l2 :		call Dyna_Compile_Block
			call eax
			cmp countdown_counter, 0
			jg l1
l3 :
		}

		Trigger_Timer_Event();
	}

	if( emustatus.reason_to_stop != EMUSTOP && Is_CPU_Doing_Other_Tasks() ) 
		goto _NextBlock;
}

/*
 =======================================================================================================================
    Check and execute all other tasks, called by emulating main loop £
    Will do DMA, interrupt checking and so on. £
 =======================================================================================================================
 */
void CPU_Check_Interrupts(void)
{
//	CPUNeedToCheckInterrupt = FALSE;		/* We will not check the second time, only one time */
	if(emustatus.cpucore == INTERPRETER)	/* intepreter mode */
	{
		if
		(
			(gHWS_COP0Reg[STATUS] & EXL_OR_ERL /* 0x00000004 */ ) == 0	/* No in another interrupt routine */
		&&	(
				(gHWS_COP0Reg[STATUS] & 0x00000001) != 0				/* Interrupts are enabled */
			&&	((gHWS_COP0Reg[CAUSE] & gHWS_COP0Reg[STATUS] & 0x0000FF00) != 0)
			)
		)
		{
			gHWS_pc = SetException_Interrupt(gHWS_pc);
			CPUNeedToCheckInterrupt = FALSE;
			DEBUG_INTERRUPT_TRACE(TRACE1("Interrupt is being served, Interrupt=%s", Get_Interrupt_Name()));
		}
	}
	else	/* Dyna mode */
	{
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY6
		if(debug_opcode!=0)
		{
			COMPARE_SwitchToInterpretive();
			if
			(
				(gHWS_COP0Reg[STATUS] & EXL_OR_ERL) == 0		/* No in another interrupt routine */
			&&	(
					(gHWS_COP0Reg[STATUS] & 0x00000001) != 0	/* Interrupts are enabled */
				&&	((gHWS_COP0Reg[CAUSE] & gHWS_COP0Reg[STATUS] & 0x0000FF00) != 0)
				)
			)
			{
				gHWS_COP0Reg[EPC] = gHWS_pc;
				gHWS_COP0Reg[STATUS] |= EXL;					/* set EXL = 1 */
				gHWS_pc = 0x80000180;
				gHWS_COP0Reg[CAUSE] &= NOT_BD;					/* clear BD */
				DEBUG_INTERRUPT_TRACE(TRACE1("Interrupt is being served, Interrupt=%s", Get_Interrupt_Name()));
			}

			COMPARE_SwitchToDynarec();
			if
			(
				(gHWS_COP0Reg[STATUS] & EXL_OR_ERL) == 0		/* No in another interrupt routine */
			&&	(
					(gHWS_COP0Reg[STATUS] & 0x00000001) != 0	/* Interrupts are enabled */
				&&	((gHWS_COP0Reg[CAUSE] & gHWS_COP0Reg[STATUS] & 0x0000FF00) != 0)
				)
			)
			{
				DEBUG_INTERRUPT_TRACE(TRACE1("Interrupt is being served, Interrupt=%s", Get_Interrupt_Name()));
				rc_Intr_Common();
				Set_Translate_PC();
				CPUNeedToCheckInterrupt = FALSE;
			}
		}
		else
#endif
			if
			(
				(gHWS_COP0Reg[STATUS] & EXL_OR_ERL) == 0		/* No in another interrupt routine */
			&&	(
					(gHWS_COP0Reg[STATUS] & 0x00000001) != 0	/* Interrupts are enabled */
				&&	((gHWS_COP0Reg[CAUSE] & gHWS_COP0Reg[STATUS] & 0x0000FF00) != 0)
				)
			)
			{
				DEBUG_INTERRUPT_TRACE(TRACE1("Interrupt is being served, Interrupt=%s", Get_Interrupt_Name()));
				rc_Intr_Common();
				Set_Translate_PC();
				CPUNeedToCheckInterrupt = FALSE;
			}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CPUDoOtherTasks(void)
{
	CPUNeedToDoOtherTask = FALSE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CPU_Task_To_String(char *str)
{
	if(!CPUNeedToDoOtherTask)
	{
		strcpy(str, "Nothing");
		return;
	}

	str[0] = '\0';

	if(DMAInProgress || CPUNeedToCheckInterrupt)
	{
		if(DMAInProgress) strcat(str, " DMA");
		if(CPUNeedToCheckInterrupt) strcat(str, " Interrupt");
	}
	else
		CPUNeedToDoOtherTask = FALSE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ClearCPUTasks(void)
{
	CPUNeedToDoOtherTask = FALSE;
	if(currentromoptions.DMA_Segmentation == USEDMASEG_YES)
	{
		DMAInProgress = FALSE;
		PIDMAInProgress = NO_DMA_IN_PROGRESS;
		SIDMAInProgress = NO_DMA_IN_PROGRESS;
		SPDMAInProgress = NO_DMA_IN_PROGRESS;
	}

	CPUNeedToCheckInterrupt = FALSE;
}

/*
 =======================================================================================================================
    Step and run one opcode in interpreter mode £
 =======================================================================================================================
 */
void InterpreterStepCPU(void)
{
	/*~~~~~~~~~~~~~~~~*/
	uint32	Instruction;
	/*~~~~~~~~~~~~~~~~*/

	Instruction = FetchInstruction();
	CPU_instruction[_OPCODE_](Instruction);
	INTERPRETER_DEBUG_INSTRUCTION(Instruction);

	if(CPUNeedToCheckException)
	{
		gHWS_pc = SetException_Interrupt(gHWS_pc);
		TRACE3
		(
			"Start Exception %d, EPC=%08X, PC=%08X",
			(gHWS_COP0Reg[CAUSE] & EXCCODE) >> 2,
			gHWS_COP0Reg[EPC],
			gHWS_pc
		) CPUNeedToCheckException = FALSE;
	}
	else
	{
		switch(CPUdelay)
		{
		case 0:		gHWS_pc += 4; break;
		case 1:		gHWS_pc += 4; CPUdelay = 2; break;
		default:	gHWS_pc = CPUdelayPC; CPUdelay = 0; break;
		}
	}
	gHWS_GPR[0] = 0;

	countdown_counter -= VICounterFactors[CounterFactor];
	if(countdown_counter <= 0) Trigger_Timer_Event();
	if(Is_CPU_Doing_Other_Tasks()) CPUDoOtherTasks();
}

/*
 =======================================================================================================================
    Return the instruction at current PC £
    This is a utility function, called by some debug function £
 =======================================================================================================================
 */
uint32 FetchInstruction(void)
{
	if(NOT_IN_KO_K1_SEG(gHWS_pc))
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~*/
		register uint32 translatepc;
		/*~~~~~~~~~~~~~~~~~~~~~~~~*/

		ITLB_Error = FALSE;
		translatepc = Direct_TLB_Lookup(gHWS_pc, TLB_INST);
		if(ITLB_Error)
		{
			if((gHWS_COP0Reg[STATUS] & EXL) == 0)	/* Exception not in exception */
			{
				if(CPUdelay != 0)					/* are we in branch delay slot? */
				{	/* yes */
					TLB_TRACE(TRACE1("ITLB exception happens in CPU delay slot, pc=%08X", gHWS_pc));
					gHWS_COP0Reg[CAUSE] |= BD;
					gHWS_COP0Reg[EPC] = gHWS_pc - 4;
					CPUdelay = 0;
				}
				else
				{	/* no */
					gHWS_COP0Reg[CAUSE] &= NOT_BD;
					gHWS_COP0Reg[EPC] = gHWS_pc;
				}

				gHWS_COP0Reg[STATUS] |= EXL;	/* set EXL = 1 */
			}
			else
			{
				DisplayError("ITLB exception happens in exception");
			}

			/*
			 * if( CPUdelay != 0 ) DisplayError("ITLB exception happens in CPU delay slot,
			 * should never happens");
			 */
			TLB_TRACE(TRACE1("ITLB exception, PC=%08X", gHWS_pc));
			gHWS_pc = TLB_Error_Vector;
			gHWS_COP0Reg[CAUSE] &= NOT_BD;		/* clear BD */
			return MEM_READ_UWORD(gHWS_pc);
		}
		else
		{
			return MEM_READ_UWORD(translatepc);
		}
	}
	else
		return MEM_READ_UWORD(gHWS_pc);
}

/*
 =======================================================================================================================
    This inline function will run a block of code in Dyna, will compile it first if needed £
 =======================================================================================================================
 */
void RunDynaBlock(void)
{
#ifndef CRASHABLE
	__try
	{
#endif

		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint32	HardwareStart = (uint32) & gHardwareState + 128;
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		Block = (uint8 *) *g_LookupPtr;
		if(Block != NULL && g_pc_is_rdram) Dyna_Check_Codes();
		if(Block == NULL) { Dyna_Compile_Block(); }
		DEBUG_PRINT_DYNA_EXECUTION_INFO 
		__asm pushad
		__asm mov ebp, HardwareStart
		__asm call Block
		__asm popad
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY7
		if(debug_opcode!=0)
		{
			if(CPUdelay == 1) gHardwareState_Interpreter_Compare.pc += 4;

			if(gHardwareState.pc != CPUdelayPC && CPUdelay == 1)
			{
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
				uint32	savepc = gHardwareState.pc;
				uint32	Inst;
				/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

				gHardwareState.pc = gHardwareState_Interpreter_Compare.pc;
				Inst = FetchInstruction();

				sprintf
				(
					generalmessage,
					"Compare PC error: pc=%08X, DelayPC=%08X, oldPC=%08X\nDyna:%s\nInterpreter:",
					gHardwareState.pc,
					CPUdelayPC,
					gHardwareState_Interpreter_Compare.pc,
					DebugPrintInstr(Inst)
				);
				gHardwareState.pc = savepc;
				COMPARE_SwitchToInterpretive();
				strcat(generalmessage, DebugPrintInstr(Inst));
				DisplayError(generalmessage);
				COMPARE_SwitchToDynarec();
			}

			CPUdelay = 0;
			PcBeforeBranch = gHardwareState_Interpreter_Compare.pc;
			gHardwareState_Interpreter_Compare.pc = gHardwareState.pc;
		}
#endif
		if(countdown_counter <= 0) 
		{
			Trigger_Timer_Event();
		}
#ifndef CRASHABLE
	}

	__except(NULL, EXCEPTION_EXECUTE_HANDLER)
	{
		DisplayError("Unknown error happens in DynaRunBlock()");
		emustatus.reason_to_stop = CPUCRASH;	/* Quit like VIDEO plugin crash */
	}
#endif
}

#define EXCEPTION_MAX_ENTRY 10

extern void __cdecl error(char *Message, ...);
/*
 =======================================================================================================================
    This routine serves exceptions in dynarec £
    This service routine could be re-entered
 =======================================================================================================================
 */
void Dyna_Exception_Service_Routine(uint32 vector)
{
	emustatus.exception_entry_count++;

	if(emustatus.exception_entry_count < EXCEPTION_MAX_ENTRY)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint8	*SavedBlock = Block;
		uint32	temppc = gHWS_pc;
		/*~~~~~~~~~~~~~~~~~~~~~~~~*/
		int k=0;

		if(emustatus.exception_entry_count == 1) SetStatusBarText(4, "E");

		if((gHWS_COP0Reg[STATUS] & EXL) == 0)	/* Exception not in exception code */
		{
			gHWS_COP0Reg[EPC] = gHWS_pc;
			gHWS_COP0Reg[STATUS] |= EXL;		/* set EXL = 1 */
		}
		else
		{
			/*
			 * If exception is within another exception, service the exception £
			 * without setting EPC
			 */
			gHWS_COP0Reg[EPC] = gHWS_pc;

			/* temppc = gHWS_COP0Reg[EPC]; */
		}

		if(gHWS_COP0Reg[STATUS] & BEV)
		{
			DisplayError("Exception happens during boot.");
		}

		/* How about branch delay ?? */
		gHWS_pc = vector;

        Set_Translate_PC(); //Rice, that can cause nested TLB errors ?

		gHWS_COP0Reg[CAUSE] &= NOT_BD;			/* clear BD */

		DEBUG_EXCEPTION_TRACE
		(
			TRACE2
				(
					"Start Exception Service in Dyna, exception=%d, EPC=%08X",
					(gHWS_COP0Reg[CAUSE] & EXCCODE) >> 2,
					gHWS_COP0Reg[EPC]
				)
		);

		while((temppc != gHWS_pc) && (emustatus.Emu_Keep_Running || emustatus.reason_to_stop == EMUPAUSE))
		{
			int k=0;
//			error("%08X %08X", temppc, gHWS_pc);
				/* to disable further interrupts */

#ifdef ENABLE_OPCODE_DEBUGGER
#ifndef TEST_OPCODE_DEBUGGER_INTEGRITY8
			if(debug_opcode != 0 && emustatus.cpucore == DYNACOMPILER)
			{
				if(p_gMemoryState == &gMemoryState)
					RunDynaBlock();
				else
					break;
			}
			else
#endif
#endif

				if((emustatus.cpucore == INTERPRETER) || (compilerstatus.Is_Compiling > 0))
				{
					InterpreterStepCPU();
				}
				else
				{
					RunDynaBlock();
				}
		}

		if(!emustatus.Emu_Keep_Running && emustatus.reason_to_stop != EMUPAUSE) /* User has stopped emulating, will
																				 * quit emu thread */
		{
			AUDIO_RomClosed();
			CONTROLLER_RomClosed();
			netplay_rom_closed();

			emustatus.Emu_Is_Running = FALSE;
			CloseEmulator();
		}

		Block = SavedBlock;

		DEBUG_EXCEPTION_TRACE(TRACE0("Finish Exception Service in Dyna"));
	}
	else
	{
		if(emustatus.exception_entry_count >= EXCEPTION_MAX_ENTRY)
		{
			DisplayError("Exception service routine reentry exceeds 10 times, skipped");
		}

		/* We will not worry about this new exception */
	}

	emustatus.exception_entry_count--;
	if(emustatus.exception_entry_count == 0) SetStatusBarText(4, emustatus.cpucore == DYNACOMPILER ? "D" : "I");
}

void DoOthersBeforeSaveState()
{
	AI_LEN_REG = AUDIO_AiReadLength();
}

void DoOthersAfterLoadState()
{
	CPUNeedToCheckInterrupt = TRUE;
	CPUNeedToDoOtherTask = TRUE;
	Set_Check_Interrupt_Timer_Event();

	/* Check FPU usage bit */
	if(currentromoptions.FPU_Hack == USEFPUHACK_YES )
	{
		if( gHWS_COP0Reg[STATUS] & SR_CU1)
		{
			EnableFPUUnusableException();
		}
		else
		{
			DisableFPUUnusableException();
		}
	}

	AUDIO_AiLenChanged();

	if( CoreDoingAIUpdate )
	{
		AUDIO_AiUpdate(FALSE);
	}
}