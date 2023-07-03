/*$T emulator.h GC 1.136 02/28/02 07:57:45 */


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
#ifndef _EMULATOR_H__1964_
#define _EMULATOR_H__1964_

#include "debug_option.h"
#include "dynarec/dynalog.h"

enum { WORDTYPE, HWORDTYPE, BYTETYPE, DWORDTYPE, NOCHECKTYPE };
enum { INIT_EMU_AFTER_PAUSE, REFRESH_DYNA_AFTER_PAUSE, DO_NOTHING_AFTER_PAUSE };
enum GAME_STOP_REASON { EMURUNNING, EMUSTOP, EMUPAUSE, EMUSWITCHCORE, EMURESUME, VIDEOCRASH, CPUCRASH };
enum GAME_HACK_DETECTED { GHACK_NONE, GHACK_GE, GHACK_PD };

extern HANDLE CPUThreadHandle;
extern int Audio_Is_Initialized;

struct EmuStatus
{
	int						DListCount;
	int						AListCount;
	int						PIDMACount;
	int						ControllerReadCount;
	BOOL					Emu_Is_Running;
	BOOL					Emu_Is_Paused;	/* means the emu is still in running state, but just paused */
	BOOL					Emu_Is_Resetting;	/* means the emu is still in running state, but just paused */
	int						CodeCheckMethod;
	int						exception_entry_count;
	int						cpucore;
	enum GAME_STOP_REASON	reason_to_stop;
	int						action_after_resume;
	volatile BOOL			Emu_Keep_Running;
	int						gepd_pause;
	enum GAME_HACK_DETECTED	game_hack;
};
extern struct EmuStatus emustatus;

struct EmuOptions
{
	BOOL	auto_apply_cheat_code;
	BOOL	auto_run_rom;
	BOOL	auto_full_screen;
	BOOL	dma_in_segments;
	BOOL	SyncVI;
	BOOL	UsingRspPlugin;
	int		OverclockFactor;
	BOOL	GEFiringRateHack;
	BOOL	GEDisableHeadRoll;
	BOOL	PDSpeedHack;
};
extern struct EmuOptions	emuoptions;

extern uint8				*RDRAM_Copy;
extern uint8				HeaderDllPass[0x40];

void						RunEmulator(unsigned _int32 WhichCore);
void						ClearCPUTasks(void);
void						InterpreterStepCPU(void);
uint32						FetchInstruction(void);
__forceinline void			RunDynaBlock(void);
void						PauseEmulating(void);
BOOL						PauseEmulator(void);
void						ResumeEmulator(int action_after_pause);
void						StopEmulator(void);
void						EmulatorSetCore(int core);
void (*Dyna_Code_Check[]) ();
void (*Dyna_Check_Codes) ();
void	Dyna_Code_Check_None_Boot(void);
void	Dyna_Code_Check_QWORD(void);
void	Dyna_Exception_Service_Routine(uint32 vector);
void	Invalidate4KBlock(uint32 addr, char *opcodename, int type, uint64 newvalue);
void	CPU_Task_To_String(char *str);

#ifdef DEBUG_COMMON
#define INTERPRETER_DEBUG_INSTRUCTION(inst) \
	if(DebuggerActive) \
	{ \
		HandleBreakpoint(inst); \
		if(DebuggerOpcodeTraceEnabled) \
		{ \
			DebugPrintInstruction(inst); \
			RefreshDebugger(); \
		} \
	}
#else
#define INTERPRETER_DEBUG_INSTRUCTION(inst)
#endif
#ifdef DEBUG_COMMON
#define DYNA_DEBUG_INSTRUCTION(inst) \
	/* FlushAllRegisters(); */ \
	if(DebuggerActive && (DebuggerOpcodeTraceEnabled || DebuggerBreakPointActive)) \
	{ \
		rc_DYNDEBUG_UPDATE(inst) DEBUG_BPT(inst) \
	}
#else
#define DYNA_DEBUG_INSTRUCTION(inst)
#endif
#ifdef DEBUG_COMMON
#ifdef LOG_DYNA
#define DYNA_LOG_INSTRUCTION	if(debugoptions.debug_dyna_log) \
		LogDyna("\n%s\n", DebugPrintInstructionWithOutRefresh(gHWS_code));
#else
#define DYNA_LOG_INSTRUCTION
#endif
#else /* release */
#ifdef LOG_DYNA
#define DYNA_LOG_INSTRUCTION	if((gMultiPass.WhichPass != COMPILE_MAP_ONLY) || (gMultiPass.UseOnePassOnly == 1)) \
		LogDyna("\n%s\n", DebugPrintInstructionWithOutRefresh(gHWS_code));
#else
#define DYNA_LOG_INSTRUCTION
#endif
#endif
#ifdef DEBUG_DYNAEXECUTION
#define DEBUG_PRINT_DYNA_EXECUTION_INFO if(debugoptions.debug_dyna_execution) \
	{ \
		sprintf(generalmessage, "Dyna execution: PC = %08X", gHWS_pc); \
		RefreshOpList(generalmessage); \
	}
#else
#define DEBUG_PRINT_DYNA_EXECUTION_INFO
#endif
#ifdef DEBUG_DYNA
#define DEBUG_PRINT_DYNA_COMPILE_INFO	if(debugoptions.debug_dyna_compiler) \
	{ \
		sprintf(generalmessage, "Dyna compile: memory %08X - %08X", compilerstatus.TempPC, gHWS_pc); \
		RefreshOpList(generalmessage); \
	}
#else
#define DEBUG_PRINT_DYNA_COMPILE_INFO
#endif
#define C_CALL( /* address */ OPCODE) \
	MOV_ImmToReg(1, Reg_EAX, (uint32) /* & */ OPCODE); \
	CALL_Reg(Reg_EAX);

#ifdef WINDEBUG_1964
#define rc_DYNDEBUG_UPDATE(Inst) \
	/* FlushAllRegisters(); */ \
	MOV_ImmToMemory(1, ModRM_disp32, (_u32) & gHWS_pc, gHWS_pc); \
	MOV_ImmToReg(1, Reg_ECX, Inst); \
	C_CALL((uint32) & WinDynDebugPrintInstruction)
#define DEBUG_BPT(inst) \
	MOV_ImmToReg(1, Reg_ECX, inst); \
	C_CALL((uint32) & HandleBreakpoint);

#else
#define rc_DYNDEBUG_UPDATE
#define DEBUG_BPT
#endif
#endif
