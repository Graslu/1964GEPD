/*$T windebug.c GC 1.136 03/09/02 17:33:20 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Windows debugger output and user interface
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
#ifdef WINDEBUG_1964
#include <windows.h>
#include <stdio.h>
#include "../timer.h"
#include "windebug.h"
#include "wingui.h"
#include "../globals.h"
#include "../r4300i.h"
#include "../hardware.h"
#include "../n64rcp.h"
#include "../interrupt.h"
#include "../DbgPrint.h"
#include "../emulator.h"
#include "../memory.h"
#include "../1964ini.h"

#define MIPS_VIEW	0
#define INTEL_VIEW	1
int				EndianView;
BOOL			DebuggerActive;						/* is the debugger on? */
int				DebuggerOpcodeTraceEnabled;			/* Flag to toggle debug opcode trace printing on/off */

extern HANDLE	CPUThreadHandle;

#define LOGGING
static FILE		*fp = NULL;
static BOOL		LoggingEnabled;

static BOOL		DisplayInterpreterCompareReg = 0;	/* Use by opcode debugger */
static BOOL		DisplayDynaCompareReg = 0;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT CALLBACK VIREGS(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	/*~~~~~~~~~~*/
	static int	i;
	/*~~~~~~~~~~*/

	switch(message)
	{
	case WM_CLOSE:
		PostMessage(hDlg, WM_DESTROY, 0, 0L);
		break;
	case WM_DESTROY:
		hVIRegwnd = NULL;
		EndDialog(hDlg, TRUE);
		break;
	case WM_INITDIALOG:
		VIREGEDIT[0] = GetDlgItem(hDlg, IDC_VIWIDTH);
		VIREGEDIT[1] = GetDlgItem(hDlg, IDC_VIHEIGHT);
		VIREGEDIT[2] = GetDlgItem(hDlg, IDC_VIORIGIN);
		VIREGEDIT[3] = GetDlgItem(hDlg, IDC_VIPIXSIZE);
		UpdateVIReg();
		return(TRUE);
		break;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:	EndDialog(hDlg, TRUE); break;
		}
		break;
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT CALLBACK DEBUGGER(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	/*~~~~~~~~~~*/
	static int	i;
	/*~~~~~~~~~~*/

	switch(message)
	{
	case WM_CLOSE:
		PostMessage(hDlg, WM_DESTROY, 0, 0L);
		break;
	case WM_DESTROY:
		hRegswnd = NULL;
		EndDialog(hDlg, TRUE);
		break;
	case WM_INITDIALOG:
		for(i = 0; i < 32; i++)
		{
			GPREDIT[i] = GetDlgItem(hDlg, IDC_GPR1 + i);
			COP0EDIT[i] = GetDlgItem(hDlg, IDC_COP01 + i);
			COP1EDIT[i] = GetDlgItem(hDlg, IDC_FPR0 + i);
		}

		MISCEDIT[0] = GetDlgItem(hDlg, IDC_PC);
		MISCEDIT[1] = GetDlgItem(hDlg, IDC_LLBIT);
		MISCEDIT[2] = GetDlgItem(hDlg, IDC_MULTHI);
		MISCEDIT[3] = GetDlgItem(hDlg, IDC_MULTLO);
		UpdateGPR();
		UpdateCOP0();
		UpdateFPR();
		UpdateMisc();

		if(debug_opcode!=0)
		{
			DisplayInterpreterCompareReg = 0;
			DisplayDynaCompareReg = 0;
		}

		return(TRUE);
		break;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			CloseDebugger();
			break;
		case IDC_STEPCPU:	/* ..TODO */
			InterpreterStepCPU();
			UpdateGPR();
			UpdateCOP0();
			UpdateFPR();
			UpdateMisc();
			UpdateVIReg();
			break;
		case ID_RUNTO:
			Set_Breakpoint();
			break;

		case IDC_USE_INTERPRETER_REG:
			DisplayInterpreterCompareReg = 1 - DisplayInterpreterCompareReg;
			break;
		case IDC_OPCODE_DYNA_COMPARE:
			DisplayDynaCompareReg = 1 - DisplayDynaCompareReg;
			break;
		case IDC_OPCODETRACE:
			if(DebuggerOpcodeTraceEnabled == 0)
			{
				DebuggerOpcodeTraceEnabled = 1;
				if(emustatus.Emu_Is_Running && emustatus.cpucore == DYNACOMPILER)
				{
					if(PauseEmulator()) ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);	/* Need to init emu */
				}
			}
			else
			{
				DebuggerOpcodeTraceEnabled = 0;
				if(emustatus.Emu_Is_Running && emustatus.cpucore == DYNACOMPILER)
				{
					if(PauseEmulator()) ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);	/* Need to init emu */
				}
			}
			break;

#ifdef LOGGING
		case IDC_CHECK_LOGGING:
			if(LoggingEnabled)
				LoggingEnabled = FALSE;
			else
			{
				LoggingEnabled = TRUE;
				if(fp == NULL)	/* log file is not opened yet */
				{
					/* Open log file to write */
					if((fp = fopen("debug.log", "w")) == NULL)
					{
						DisplayError("Error to create debug log file");
						LoggingEnabled = FALSE;
					}
				}
			}
			break;
#endif
		case IDC_TRIGGER_SP_INTERRUPT:
			Trigger_SPInterrupt();
			break;
		case IDC_REGS_FLUSH:
			UpdateGPR();
			UpdateCOP0();
			UpdateFPR();
			UpdateMisc();
			UpdateVIReg();
			break;
		}
		break;
	}

	return(FALSE);
}

extern char op_str[0xff];
extern void (__cdecl *_AUDIO_AiUpdate) (BOOL)

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void HandleBreakpoint(uint32 Instruction)
{
	if(DebuggerBreakPointActive)
	{
		if(gHWS_pc == BreakAddress)
		{
			/*
			 * DebuggerBreakPointActive = FALSE; £
			 * BreakAddress = 0;
			 */
			RefreshDebugger();

			/* DebuggerOpcodeTraceEnabled = 1; */
			WinDynDebugPrintInstruction(Instruction);
			RefreshDebugger();
			TRACE1("breakpoint %08X is hit", BreakAddress);
			DisplayError("%s\nBreakpoint Encountered.", op_str);

			/* SuspendThread(CPUThreadHandle); */
			emustatus.reason_to_stop = EMUPAUSE;
			emustatus.Emu_Keep_Running = FALSE;
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void PrintTLB(BOOL all)
{
	/*~~~~~~*/
	int		i;
	uint32	g;
	/*~~~~~~*/

	for(i = 0; i <= NTLBENTRIES; i++)
	{
		if(gMS_TLB[i].valid || all == TRUE)
		{
			g = (gMS_TLB[i].EntryLo0 & gMS_TLB[i].EntryLo1 & 0x01);

			TRACE3("TLB [%d], G=%d, V=%d", i, g, gMS_TLB[i].valid);
			TRACE2("PAGEMASK = 0x%08X, ENTRYHI = 0x%08X", (uint32) gMS_TLB[i].PageMask, gMS_TLB[i].EntryHi);
			TRACE2("ENTRYLO1 = 0x%08X, ENTRYLO0 = 0x%08X", gMS_TLB[i].EntryLo1, gMS_TLB[i].EntryLo0);
			TRACE2("LoCompare = 0x%08X, MyHiMask = 0x%08X", gMS_TLB[i].LoCompare, gMS_TLB[i].MyHiMask);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateMemList(void)
{
	/*~~~~~~~~~~~~~~~~*/
	char	final[80];
	LPSTR	mem[12];
	uint8	*realmemloc;
	uint32	memloc;
	int		i;
	/*~~~~~~~~~~~~~~~~*/

	/* clear memory list */
	SendMessage(MEMLISTBOX, LB_RESETCONTENT, 0, 0);

	/* get address */
	SendMessage(MEMLOCEDIT, WM_GETTEXT, 10, (LPARAM) (LPSTR) mem);

	if(Rom_Loaded == 0)
	{
		sprintf(final, "--- Please load ROM first ---");
		SendMessage(MEMLISTBOX, LB_INSERTSTRING, (WPARAM) - 1, (LPARAM) final);
		return;
	}

	memloc = StrToHex((char *) mem) & 0xFFFFFFF0;
	realmemloc = pLOAD_UBYTE_PARAM(memloc);
	__try
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint8	dummy = *realmemloc;
		/*~~~~~~~~~~~~~~~~~~~~~~~~*/

		dummy = *(realmemloc + 0xFF);
	}

	__except(NULL, EXCEPTION_EXECUTE_HANDLER)
	{
		realmemloc = pLOAD_UBYTE_PARAM_2(memloc);
		__try
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~*/
			uint8	dummy = *realmemloc;
			/*~~~~~~~~~~~~~~~~~~~~~~~~*/

			dummy = *(realmemloc + 0xFF);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			realmemloc = NULL;
		}
	}

	if(realmemloc == NULL)
	{
		sprintf(final, "--- Invalid Segment ---");
		SendMessage(MEMLISTBOX, LB_INSERTSTRING, (WPARAM) - 1, (LPARAM) final);
		return;
	}

	for(i = 0; i < 11; i++)
	{
		if(EndianView == INTEL_VIEW)	/* intel view on intel machine */
		{
			sprintf
			(
				final,
				"%08X:  %02X %02X %02X %02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X %02X %02X %02X  %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c",
				memloc,
				realmemloc[0x00],
				realmemloc[0x01],
				realmemloc[0x02],
				realmemloc[0x03],
				realmemloc[0x04],
				realmemloc[0x05],
				realmemloc[0x06],
				realmemloc[0x07],
				realmemloc[0x08],
				realmemloc[0x09],
				realmemloc[0x0A],
				realmemloc[0x0B],
				realmemloc[0x0C],
				realmemloc[0x0D],
				realmemloc[0x0E],
				realmemloc[0x0F],
				realmemloc[0x00],
				realmemloc[0x01],
				realmemloc[0x02],
				realmemloc[0x03],
				realmemloc[0x04],
				realmemloc[0x05],
				realmemloc[0x06],
				realmemloc[0x07],
				realmemloc[0x08],
				realmemloc[0x09],
				realmemloc[0x0A],
				realmemloc[0x0B],
				realmemloc[0x0C],
				realmemloc[0x0D],
				realmemloc[0x0E],
				realmemloc[0x0F]
			);
		}
		else	/* it is MIPS view (on an intel machine) */
		{
			sprintf
			(
				final,
				"%08X:  %02X %02X %02X %02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X %02X %02X %02X  %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c",
				memloc,
				realmemloc[0x03],
				realmemloc[0x02],
				realmemloc[0x01],
				realmemloc[0x00],
				realmemloc[0x07],
				realmemloc[0x06],
				realmemloc[0x05],
				realmemloc[0x04],
				realmemloc[0x0B],
				realmemloc[0x0A],
				realmemloc[0x09],
				realmemloc[0x08],
				realmemloc[0x0F],
				realmemloc[0x0E],
				realmemloc[0x0D],
				realmemloc[0x0C],
				realmemloc[0x03],
				realmemloc[0x02],
				realmemloc[0x01],
				realmemloc[0x00],
				realmemloc[0x07],
				realmemloc[0x06],
				realmemloc[0x05],
				realmemloc[0x04],
				realmemloc[0x0B],
				realmemloc[0x0A],
				realmemloc[0x09],
				realmemloc[0x08],
				realmemloc[0x0F],
				realmemloc[0x0E],
				realmemloc[0x0D],
				realmemloc[0x0C]
			);
		}

		SendMessage(MEMLISTBOX, LB_INSERTSTRING, (WPARAM) - 1, (LPARAM) final);
		realmemloc += 0x10;
		memloc += 0x10;
	}			/* end for */
}

#define LINETODASM	0x100

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MemoryDeAssemble(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/
	LPSTR	mem[12];
	int		i;
	uint32	Instruction;
	uint32	temppc, translatepc;
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(Rom_Loaded == 0)
	{
		sprintf(op_str, "--- Please load ROM first ---");
		SendMessage(MEMLISTBOX, LB_INSERTSTRING, (WPARAM) - 1, (LPARAM) op_str);
		return;
	}

	/* clear memory list */
	SendMessage(MEMLISTBOX, LB_RESETCONTENT, 0, 0);

	/* get address */
	SendMessage(MEMLOCEDIT, WM_GETTEXT, 10, (LPARAM) (LPSTR) mem);
	translatepc = StrToHex((char *) mem) & 0xFFFFFE00;

	__try
	{
		/*~~~~~~~~~~*/
		uint32	dummy;
		/*~~~~~~~~~~*/

		dummy = LOAD_UWORD_PARAM(translatepc);
	}

	__except(NULL, EXCEPTION_EXECUTE_HANDLER)
	{
		sprintf(op_str, "--- Invalid Segment ---");
		SendMessage(MEMLISTBOX, LB_INSERTSTRING, (WPARAM) - 1, (LPARAM) op_str);
		return;
	}

	temppc = gHWS_pc;
	gHWS_pc = translatepc;
	for(i = 0; i < LINETODASM; i++, gHWS_pc += 4)
	{
		translatepc = gHWS_pc;
		if(NOT_IN_KO_K1_SEG(translatepc))
		{
			ITLB_Error = FALSE;
			translatepc = TranslateITLBAddress(translatepc);
			if(ITLB_Error)
			{
				sprintf(op_str, "%08X: TLB unmapped", gHWS_pc);
				SendMessage(MEMLISTBOX, LB_INSERTSTRING, (WPARAM) i, (LPARAM) op_str);
				continue;
			}
		}

		__try
		{
			Instruction = LOAD_UWORD_PARAM(translatepc);
			DebugPrintInstructionWithOutRefresh(Instruction);
		}

		__except(NULL, EXCEPTION_EXECUTE_HANDLER)
		{
			sprintf(op_str, "%08X: invalid memory address", gHWS_pc);
		}

		SendMessage(MEMLISTBOX, LB_INSERTSTRING, (WPARAM) i, (LPARAM) op_str);
		DisplayCriticalMessage(op_str);
	}

	gHWS_pc = temppc;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MemoryDeAssembleNextPage(void)
{
	/*~~~~~~~~~~~~*/
	LPSTR	mem[12];
	uint32	memloc;
	/*~~~~~~~~~~~~*/

	/* get address */
	SendMessage(MEMLOCEDIT, WM_GETTEXT, 10, (LPARAM) (LPSTR) mem);

	memloc = StrToHex((char *) mem) & 0xFFFFFC00;
	memloc += 4 * LINETODASM;

	sprintf((char *) mem, "%X", memloc);

	SendMessage(MEMLOCEDIT, WM_SETTEXT, 10, (LPARAM) (LPSTR) mem);

	MemoryDeAssemble();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void MemoryDeAssemblePrevPage(void)
{
	/*~~~~~~~~~~~~*/
	LPSTR	mem[12];
	uint32	memloc;
	/*~~~~~~~~~~~~*/

	/* get address */
	SendMessage(MEMLOCEDIT, WM_GETTEXT, 10, (LPARAM) (LPSTR) mem);

	memloc = StrToHex((char *) mem) & 0xFFFFFC00;
	memloc -= 4 * LINETODASM;

	sprintf((char *) mem, "%X", memloc);

	SendMessage(MEMLOCEDIT, WM_SETTEXT, 10, (LPARAM) (LPSTR) mem);

	MemoryDeAssemble();
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT CALLBACK MEMORYPROC(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CLOSE:
		PostMessage(hDlg, WM_DESTROY, 0, 0L);
		break;
	case WM_DESTROY:
		hMemorywnd = NULL;
		EndDialog(hDlg, TRUE);
		break;
	case WM_INITDIALOG:
		MEMLISTBOX = GetDlgItem(hDlg, IDC_MEMLIST);
		MEMLOCEDIT = GetDlgItem(hDlg, IDC_MEMEDIT);
		break;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDC_UPDATE:		UpdateMemList(); break;
		case IDC_VIEWMIPS:		EndianView = MIPS_VIEW; UpdateMemList(); break;
		case IDC_VIEWINTEL:		EndianView = INTEL_VIEW; UpdateMemList(); break;
		case IDC_DEASSEMBLE:	MemoryDeAssemble(); break;
		case IDC_DASM_NEXTPAGE: MemoryDeAssembleNextPage(); break;
		case IDC_DASM_PREVPAGE: MemoryDeAssemblePrevPage(); break;
		}
		break;
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT APIENTRY CODELISTPROC(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	switch(message)
	{
	case WM_CLOSE:
		PostMessage(hDlg, WM_DESTROY, 0, 0L);
		break;
	case WM_DESTROY:
		hCodeListwnd = NULL;
		EndDialog(hDlg, TRUE);
		break;
	case WM_INITDIALOG:
		CODEEDIT[0] = GetDlgItem(hDlg, IDC_CODELIST);
		return(TRUE);

	case WM_COMMAND:
		switch(wParam)
		{
		case IDC_CLEAR_CODELIST:
			SendMessage(CODEEDIT[0], LB_RESETCONTENT, 0, 0);
			OpCount = 0;
			break;
		case IDC_PAUSE_CODELIST:
			Pause();
			break;
		case IDC_RUN_CODELIST:
			Play(emuoptions.auto_full_screen, 0);
			break;
		case IDC_PRINT_TLB:
			PrintTLB(FALSE);
			break;
		case IDC_PRINT_TLBALL:
			PrintTLB(TRUE);
			break;
		case IDC_STEPCPU_CODELIST:
			InterpreterStepCPU();
			UpdateGPR();
			UpdateCOP0();
			UpdateFPR();
			UpdateMisc();
			UpdateVIReg();
			break;
		case IDC_MANUAL_SI:
			Trigger_Interrupt_Without_Mask(MI_INTR_SI);
			break;
		case IDC_MANUAL_AI:
			Trigger_Interrupt_Without_Mask(MI_INTR_AI);
			break;
		case IDC_MANUAL_VI:
			Trigger_Interrupt_Without_Mask(MI_INTR_VI);
			break;
		case IDC_MANUAL_SP:
			Trigger_Interrupt_Without_Mask(MI_INTR_SP);
			break;
		case IDC_MANUAL_DP:
			Trigger_Interrupt_Without_Mask(MI_INTR_DP);
			break;
		case IDC_MANUAL_COMPARE:
			Trigger_CompareInterrupt();
			break;
		}
		break;
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT APIENTRY COP2VEC1(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	/*~~~*/
	int el;
	/*~~~*/

	switch(message)
	{
	case WM_INITDIALOG:
		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[0][el] = GetDlgItem(hDlg, IDC_VEC11 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[1][el] = GetDlgItem(hDlg, IDC_VEC21 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[2][el] = GetDlgItem(hDlg, IDC_VEC31 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[3][el] = GetDlgItem(hDlg, IDC_VEC41 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[4][el] = GetDlgItem(hDlg, IDC_VEC51 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[5][el] = GetDlgItem(hDlg, IDC_VEC61 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[6][el] = GetDlgItem(hDlg, IDC_VEC71 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[7][el] = GetDlgItem(hDlg, IDC_VEC81 + el);
		}

		UpdateCOP2Vec1();

		return(TRUE);
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateCOP2Vec1(void)
{
	/*~~~~*/
	/* char String[30]; */
	int el;
	int vec;
	/*~~~~*/

	for(vec = 0; vec < 8; vec++)
	{
		for(el = 0; el < 8; el++)
		{
			/*
			 * sprintf(String, "%08X", RSPVec[vec][el]); £
			 * SendMessage(COP2VECEDIT[vec][el],WM_SETTEXT,0,(LPARAM)(LPCTSTR)String);
			 */
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT APIENTRY COP2VEC2(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	/*~~~*/
	int el;
	/*~~~*/

	switch(message)
	{
	case WM_INITDIALOG:
		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[8][el] = GetDlgItem(hDlg, IDC_VEC91 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[9][el] = GetDlgItem(hDlg, IDC_VEC101 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[10][el] = GetDlgItem(hDlg, IDC_VEC111 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[11][el] = GetDlgItem(hDlg, IDC_VEC121 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[12][el] = GetDlgItem(hDlg, IDC_VEC131 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[13][el] = GetDlgItem(hDlg, IDC_VEC141 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[14][el] = GetDlgItem(hDlg, IDC_VEC151 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[15][el] = GetDlgItem(hDlg, IDC_VEC161 + el);
		}

		UpdateCOP2Vec2();

		return(TRUE);
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateCOP2Vec2(void)
{
	/*~~~~*/
	/* char String[30]; */
	int el;
	int vec;
	/*~~~~*/

	for(vec = 8; vec < 16; vec++)
	{
		for(el = 0; el < 8; el++)
		{
			/*
			 * sprintf(String, "%08X", RSPVec[vec][el]); £
			 * SendMessage(COP2VECEDIT[vec][el],WM_SETTEXT,0,(LPARAM)(LPCTSTR)String);
			 */
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT APIENTRY COP2VEC3(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	/*~~~*/
	int el;
	/*~~~*/

	switch(message)
	{
	case WM_INITDIALOG:
		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[16][el] = GetDlgItem(hDlg, IDC_VEC171 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[17][el] = GetDlgItem(hDlg, IDC_VEC181 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[18][el] = GetDlgItem(hDlg, IDC_VEC191 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[19][el] = GetDlgItem(hDlg, IDC_VEC201 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[20][el] = GetDlgItem(hDlg, IDC_VEC211 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[21][el] = GetDlgItem(hDlg, IDC_VEC221 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[22][el] = GetDlgItem(hDlg, IDC_VEC231 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[23][el] = GetDlgItem(hDlg, IDC_VEC241 + el);
		}

		UpdateCOP2Vec3();

		return(TRUE);
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateCOP2Vec3(void)
{
	/*~~~~*/
	/* char String[30]; */
	int el;
	int vec;
	/*~~~~*/

	for(vec = 16; vec < 24; vec++)
	{
		for(el = 0; el < 8; el++)
		{
			/*
			 * sprintf(String, "%08X", RSPVec[vec][el]); £
			 * SendMessage(COP2VECEDIT[vec][el],WM_SETTEXT,0,(LPARAM)(LPCTSTR)String);
			 */
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT APIENTRY COP2VEC4(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	/*~~~*/
	int el;
	/*~~~*/

	switch(message)
	{
	case WM_INITDIALOG:
		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[24][el] = GetDlgItem(hDlg, IDC_VEC251 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[25][el] = GetDlgItem(hDlg, IDC_VEC261 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[26][el] = GetDlgItem(hDlg, IDC_VEC271 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[27][el] = GetDlgItem(hDlg, IDC_VEC281 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[28][el] = GetDlgItem(hDlg, IDC_VEC291 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[29][el] = GetDlgItem(hDlg, IDC_VEC301 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[30][el] = GetDlgItem(hDlg, IDC_VEC311 + el);
		}

		for(el = 0; el < 8; el++)
		{
			COP2VECEDIT[31][el] = GetDlgItem(hDlg, IDC_VEC321 + el);
		}

		UpdateCOP2Vec4();

		return(TRUE);
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateCOP2Vec4(void)
{
	/*~~~~*/
	/* char String[30]; */
	int el;
	int vec;
	/*~~~~*/

	for(vec = 24; vec < 32; vec++)
	{
		for(el = 0; el < 8; el++)
		{
			/*
			 * sprintf(String, "%08X", RSPVec[vec][el]); £
			 * SendMessage(COP2VECEDIT[vec][el],WM_SETTEXT,0,(LPARAM)(LPCTSTR)String);
			 */
		}
	}
}

/*
 =======================================================================================================================
    This is a very specific StrToHex(). We assume that the length of HexStr is 8 bytes. This works 4 now.
 =======================================================================================================================
 */
uint32 StrToHex(char *HexStr)
{
	/*~~~~~~~~~~~~~*/
	int		k;
	_int32	temp = 0;
	/*~~~~~~~~~~~~~*/

	for(k = 0; k < 8; k++)
	{
		if(HexStr[k] <= '9' && HexStr[k] >= '0')
		{
			temp = temp << 4;
			temp += HexStr[k] - '0';
		}
		else if(HexStr[k] <= 'F' && HexStr[k] >= 'A')
		{
			temp = temp << 4;
			temp += HexStr[k] - 'A' + 10;
		}
		else
		{
			return(0);
		}
	}

	return(temp);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LRESULT APIENTRY GETHEX(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
	/*~~~~~~~~~~~~*/
	char	no[50];
	_int32	tempint;
	/*~~~~~~~~~~~~*/

	switch(message)
	{
	case WM_INITDIALOG:
		sprintf(generalmessage, "%08X", BreakAddress);
		SetDlgItemText(hDlg, IDC_VALUE, generalmessage);
		return(TRUE);
		break;

	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			SendDlgItemMessage(hDlg, IDC_VALUE, WM_GETTEXT, 10, (LPARAM) (LPSTR) no);

			/* Set The break address */
			if((tempint = StrToHex(no)) != 0)
			{
				DebuggerBreakPointActive = TRUE;
				BreakAddress = tempint;
			}
			else
			{
				DebuggerBreakPointActive = FALSE;

				/* MessageBox(hwnd1964main, "Illegal Break Address", "Error:", MB_OK); */
				MessageBox(gui.hwnd1964main, "Break Point is disabled.", "Note:", MB_OK);
			}

			EndDialog(hDlg, TRUE);
			hEnterHexwnd = NULL;
			break;

		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			hEnterHexwnd = NULL;
			break;
		}
		break;
	}

	return(FALSE);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateVIReg(void)
{
	/*~~~~~~~~~~~~~~~~~~~*/
	char	String[30];
	uint32	height, status;
	/*~~~~~~~~~~~~~~~~~~~*/

	sprintf(String, "%i", VI_WIDTH_REG);
	SendMessage(VIREGEDIT[0], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);

	height = (VI_WIDTH_REG * 3) >> 2;
	sprintf(String, "%i", height);
	SendMessage(VIREGEDIT[1], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);

	sprintf(String, "%08X", VI_ORIGIN_REG);
	SendMessage(VIREGEDIT[2], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);

	status = VI_STATUS_REG;
	status &= 0x0003;
	switch(status)
	{
	case 0: sprintf(String, "No Data / Sync"); break;
	case 1: sprintf(String, "Reserved!"); break;
	case 2: sprintf(String, "5/5/5/1 : 16 bit"); break;
	case 3: sprintf(String, "8/8/8/8 : 32 bit"); break;
	}

	SendMessage(VIREGEDIT[3], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Set_Breakpoint(void)
{
	if(!hEnterHexwnd) hEnterHexwnd = CreateDialog(gui.hInst, "ENTERHEX", gui.hwnd1964main, (DLGPROC) GETHEX);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Clear_Breakpoint(void)
{
	DebuggerBreakPointActive = FALSE;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RefreshOpList(char *opcode)
{
	/*~~~~~~~~~~~~~~*/
	static	count = 0;
	/*~~~~~~~~~~~~~~*/

	if(DebuggerActive == FALSE) return;
	strcpy(opBuffer, opcode);

#ifdef LOGGING
	if(LoggingEnabled && count < 800000)	/* Not to create too big a log file */
	{
		fprintf(fp, opcode);
		fputc(13, fp);
		fputc(10, fp);
		count++;
	}
	else
	{
		SendMessage(CODEEDIT[0], LB_INSERTSTRING, (WPARAM) - 1, (LPARAM) opcode);
		OpCount++;
		SendMessage(CODEEDIT[0], LB_SETCURSEL, (WPARAM) OpCount - 1, (LPARAM) 0);

		if(gHWS_COP0Reg[COUNT] == NextClearCode)
		{
			SendMessage(CODEEDIT[0], LB_RESETCONTENT, 0, 0);
			NextClearCode += 250;
			OpCount = 0;
		}
	}
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateMisc(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	char			String[17];
	HardwareState	*pstate;
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	pstate = &gHardwareState;

	if(debug_opcode!=0)
	{
		if(DisplayInterpreterCompareReg)
		{
			pstate = &gHardwareState_Interpreter_Compare;
		}
		else if(DisplayDynaCompareReg)
		{
			pstate = &gHardwareState_Flushed_Dynarec_Compare;
		}
	}

	/* PC */
	sprintf(String, "%08X", (uint32) pstate->pc);
	SendMessage(MISCEDIT[0], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);

	/* LLBit */
	sprintf(String, "%08X", (uint32) pstate->LLbit);
	SendMessage(MISCEDIT[1], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);

	/* MultHI */
	sprintf(String, "%016I64X", pstate->GPR[__HI]);
	SendMessage(MISCEDIT[2], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);

	/* MultLO */
	sprintf(String, "%016I64X", pstate->GPR[__LO]);
	SendMessage(MISCEDIT[3], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateGPR(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	int				i;
	char			String[17];
	HardwareState	*pstate;
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	pstate = &gHardwareState;

	if(debug_opcode!=0)
	{
		if(DisplayInterpreterCompareReg)
		{
			pstate = &gHardwareState_Interpreter_Compare;
		}
		else if(DisplayDynaCompareReg)
		{
			pstate = &gHardwareState_Flushed_Dynarec_Compare;
		}
	}

	for(i = 0; i < 32; i++)
	{
		/*
		 * if (gHWS_GPR[i] == 0) £
		 * sprintf(String, "........"); £
		 * else
		 */
		sprintf(String, "%016I64X", pstate->GPR[i]);

		SendMessage(GPREDIT[i], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateCOP0(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	char			String[10];
	int				i;
	HardwareState	*pstate;
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	pstate = &gHardwareState;

	if(debug_opcode!=0)
	{
		if(DisplayInterpreterCompareReg)
		{
			pstate = &gHardwareState_Interpreter_Compare;
		}
		else if(DisplayDynaCompareReg)
		{
			pstate = &gHardwareState_Flushed_Dynarec_Compare;
		}
	}

	for(i = 0; i < 32; i++)
	{
		/*
		 * if (gHWS_COP0Reg[i] == 0) £
		 * sprintf(String, "........"); £
		 * else
		 */
		sprintf(String, "%08X", (uint32) pstate->COP0Reg[i]);

		SendMessage(COP0EDIT[i], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void UpdateFPR(void)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	char			String[17];
	int				i;
	HardwareState	*pstate;
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	pstate = &gHardwareState;

	if(debug_opcode!=0)
	{
		if(DisplayInterpreterCompareReg)
		{
			pstate = &gHardwareState_Interpreter_Compare;
		}
		else if(DisplayDynaCompareReg)
		{
			pstate = &gHardwareState_Flushed_Dynarec_Compare;
		}
	}

	if(FR_reg_offset == 1)	/* 64bit FPU is off */
	{
		for(i = 0; i < 32; i++)
		{
			sprintf(String, "00000000%08X", pstate->fpr32[i]);
			SendMessage(COP1EDIT[i], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);
		}
	}
	else					/* 64bit FPU is on */
	{
		for(i = 0; i < 32; i++)
		{
			sprintf(String, "%08X%08X", pstate->fpr32[i + 32], pstate->fpr32[i]);
			SendMessage(COP1EDIT[i], WM_SETTEXT, 0, (LPARAM) (LPCTSTR) String);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RefreshDebugger(void)
{
#ifndef LOGGING
	if(hRegswnd != NULL)
	{
		UpdateGPR();
		UpdateCOP0();
		UpdateFPR();
		UpdateMisc();
		UpdateVIReg();
	}
#endif
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CreateCOP2Page(void)
{
	/*~~~~~~~~~~~~~~~~~~~*/
	PROPSHEETPAGE	psp[6];
	PROPSHEETHEADER psh;
	/*~~~~~~~~~~~~~~~~~~~*/

	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_USETITLE;
	psp[0].hInstance = gui.hInst;
	psp[0].pszTemplate = "CODELIST";
	psp[0].pszIcon = NULL;
	psp[0].pfnDlgProc = (DLGPROC) CODELISTPROC;
	psp[0].pszTitle = "CodeList";
	psp[0].lParam = 0;

	psp[1].dwSize = sizeof(PROPSHEETPAGE);
	psp[1].dwFlags = PSP_USETITLE;
	psp[1].hInstance = gui.hInst;
	psp[1].pszTemplate = "MEMORY";
	psp[1].pszIcon = NULL;
	psp[1].pfnDlgProc = (DLGPROC) MEMORYPROC;
	psp[1].pszTitle = "Memory";
	psp[1].lParam = 0;

	psp[2].dwSize = sizeof(PROPSHEETPAGE);
	psp[2].dwFlags = PSP_USETITLE;
	psp[2].hInstance = gui.hInst;
	psp[2].pszTemplate = "COP2REGS1";
	psp[2].pszIcon = NULL;
	psp[2].pfnDlgProc = (DLGPROC) COP2VEC1;
	psp[2].pszTitle = "Vectors 1-8";
	psp[2].lParam = 0;

	psp[3].dwSize = sizeof(PROPSHEETPAGE);
	psp[3].dwFlags = PSP_USETITLE;
	psp[3].hInstance = gui.hInst;
	psp[3].pszTemplate = "COP2REGS2";
	psp[3].pszIcon = NULL;
	psp[3].pfnDlgProc = (DLGPROC) COP2VEC2;
	psp[3].pszTitle = "Vectors 9-16";
	psp[3].lParam = 0;

	psp[4].dwSize = sizeof(PROPSHEETPAGE);
	psp[4].dwFlags = PSP_USETITLE;
	psp[4].hInstance = gui.hInst;
	psp[4].pszTemplate = "COP2REGS3";
	psp[4].pszIcon = NULL;
	psp[4].pfnDlgProc = (DLGPROC) COP2VEC3;
	psp[4].pszTitle = "Vectors 17-24";
	psp[4].lParam = 0;

	psp[5].dwSize = sizeof(PROPSHEETPAGE);
	psp[5].dwFlags = PSP_USETITLE;
	psp[5].hInstance = gui.hInst;
	psp[5].pszTemplate = "COP2REGS4";
	psp[5].pszIcon = NULL;
	psp[5].pfnDlgProc = (DLGPROC) COP2VEC4;
	psp[5].pszTitle = "Vectors 25-32";
	psp[5].lParam = 0;

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS | PSH_NOAPPLYNOW;
	psh.hwndParent = NULL;
	psh.hInstance = gui.hInst;
	psh.pszIcon = NULL;
	psh.pszCaption = (LPSTR) "Debugger";
	psh.nStartPage = 0;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.ppsp = (LPCPROPSHEETPAGE) & psp;

	hCOP2Vecswnd = (HWND) PropertySheet(&psh);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void OpenDebugger(void)
{
	if(!DebuggerActive)
	{
		if(!hRegswnd)
		{
			/* Create the debugger dialog box */
			hRegswnd = CreateDialog(gui.hInst, "REGS", NULL, (DLGPROC) DEBUGGER);
		}

		CreateCOP2Page();
		SetActiveWindow(gui.hwnd1964main);

		DebuggerActive = TRUE;
		OpCount = 0;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CloseDebugger(void)
{
	if(DebuggerActive)
	{
		DestroyWindow(hCodeListwnd);
		DestroyWindow(hRegswnd);
		DestroyWindow(hVIRegwnd);
		DestroyWindow(hCOP2Vecswnd);

		DebuggerActive = FALSE;
		if(fp != NULL)
		{
			fclose(fp);
			LoggingEnabled = FALSE;
		}
	}
}
#endif /* ifdef WINDEBUG_1964 */

#ifdef DEBUG_COMMON

/*
 =======================================================================================================================
    Toggle the debug options according to menu commands
 =======================================================================================================================
 */
void ToggleDebugOptions(WPARAM wParam)
{
	switch(wParam)
	{
	case ID_DEBUGTLB:
		debugoptions.debug_tlb = 1 - debugoptions.debug_tlb;
		if(debugoptions.debug_tlb)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGTLB, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGTLB, MF_UNCHECKED);
		break;
	case ID_DEBUGTLBINDETAIL:
		debugoptions.debug_tlb_detail = 1 - debugoptions.debug_tlb_detail;
		if(debugoptions.debug_tlb_detail)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGTLBINDETAIL, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGTLBINDETAIL, MF_UNCHECKED);
		break;
	case ID_DEBUG_TLB_EXTRA:
		debugoptions.debug_tlb_extra = 1 - debugoptions.debug_tlb_extra;
		if(debugoptions.debug_tlb_extra)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_TLB_EXTRA, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_TLB_EXTRA, MF_UNCHECKED);
		break;
	case ID_DEBUGAUDIOTASK:
		debugoptions.debug_audio = 1 - debugoptions.debug_audio;
		if(debugoptions.debug_audio)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGAUDIOTASK, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGAUDIOTASK, MF_UNCHECKED);
		break;
	case ID_DEBUGCOMPAREINTERRUPTS:
		debugoptions.debug_compare_interrupt = 1 - debugoptions.debug_compare_interrupt;
		if(debugoptions.debug_compare_interrupt)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGCOMPAREINTERRUPTS, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGCOMPAREINTERRUPTS, MF_UNCHECKED);
		break;
	case ID_DEBUGCPUCOUNTER:
		debugoptions.debug_cpu_counter = 1 - debugoptions.debug_cpu_counter;
		if(debugoptions.debug_cpu_counter)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGCPUCOUNTER, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGCPUCOUNTER, MF_UNCHECKED);
		break;
	case ID_DEBUGIO:
		debugoptions.debug_io = 1 - debugoptions.debug_io;
		if(debugoptions.debug_io)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIO, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIO, MF_UNCHECKED);
		break;
	case ID_DEBUGIOSI:
		debugoptions.debug_io_si = 1 - debugoptions.debug_io_si;
		if(debugoptions.debug_io_si)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOSI, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOSI, MF_UNCHECKED);
		break;
	case ID_DEBUGIOSP:
		debugoptions.debug_io_sp = 1 - debugoptions.debug_io_sp;
		if(debugoptions.debug_io_sp)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOSP, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOSP, MF_UNCHECKED);
		break;
	case ID_DEBUGIOMI:
		debugoptions.debug_io_mi = 1 - debugoptions.debug_io_mi;
		if(debugoptions.debug_io_mi)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOMI, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOMI, MF_UNCHECKED);
		break;
	case ID_DEBUGIOVI:
		debugoptions.debug_io_vi = 1 - debugoptions.debug_io_vi;
		if(debugoptions.debug_io_vi)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOVI, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOVI, MF_UNCHECKED);
		break;
	case ID_DEBUGIOAI:
		debugoptions.debug_io_ai = 1 - debugoptions.debug_io_ai;
		if(debugoptions.debug_io_ai)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOAI, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOAI, MF_UNCHECKED);
		break;
	case ID_DEBUGIORI:
		debugoptions.debug_io_ri = 1 - debugoptions.debug_io_ri;
		if(debugoptions.debug_io_ri)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIORI, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIORI, MF_UNCHECKED);
		break;
	case ID_DEBUGIOPI:
		debugoptions.debug_io_pi = 1 - debugoptions.debug_io_pi;
		if(debugoptions.debug_io_pi)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOPI, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIOPI, MF_UNCHECKED);
		break;
	case ID_DEBUGIODP:
		debugoptions.debug_io_dp = 1 - debugoptions.debug_io_dp;
		if(debugoptions.debug_io_dp)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIODP, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIODP, MF_UNCHECKED);
		break;
	case ID_DEBUGIODPS:
		debugoptions.debug_io_dps = 1 - debugoptions.debug_io_dps;
		if(debugoptions.debug_io_dps)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIODPS, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIODPS, MF_UNCHECKED);
		break;
	case ID_DEBUGIORDRAM:
		debugoptions.debug_io_rdram = 1 - debugoptions.debug_io_rdram;
		if(debugoptions.debug_io_rdram)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIORDRAM, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGIORDRAM, MF_UNCHECKED);
		break;
	case ID_DEBUGSITASK:
		debugoptions.debug_si_task = 1 - debugoptions.debug_si_task;
		if(debugoptions.debug_si_task)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGSITASK, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGSITASK, MF_UNCHECKED);
		break;
	case ID_DEBUGSPTASK:
		debugoptions.debug_sp_task = 1 - debugoptions.debug_sp_task;
		if(debugoptions.debug_sp_task)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGSPTASK, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGSPTASK, MF_UNCHECKED);
		break;
	case ID_DEBUGPIDMA:
		debugoptions.debug_pi_dma = 1 - debugoptions.debug_pi_dma;
		if(debugoptions.debug_pi_dma)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGPIDMA, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGPIDMA, MF_UNCHECKED);
		break;
	case ID_DEBUGSIDMA:
		debugoptions.debug_si_dma = 1 - debugoptions.debug_si_dma;
		if(debugoptions.debug_si_dma)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGSIDMA, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGSIDMA, MF_UNCHECKED);
		break;
	case ID_DEBUGSPDMA:
		debugoptions.debug_sp_dma = 1 - debugoptions.debug_sp_dma;
		if(debugoptions.debug_sp_dma)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGSPDMA, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGSPDMA, MF_UNCHECKED);
		break;
	case ID_DEBUGMEMPAK:
		debugoptions.debug_si_mempak = 1 - debugoptions.debug_si_mempak;
		if(debugoptions.debug_si_mempak)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGMEMPAK, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGMEMPAK, MF_UNCHECKED);
		break;
	case ID_DEBUGEEPROM:
		debugoptions.debug_si_eeprom = 1 - debugoptions.debug_si_eeprom;
		if(debugoptions.debug_si_eeprom)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGEEPROM, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGEEPROM, MF_UNCHECKED);
		break;
	case ID_DEBUG_CONTROLLER:
		debugoptions.debug_si_controller = 1 - debugoptions.debug_si_controller;
		if(debugoptions.debug_si_controller)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_CONTROLLER, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_CONTROLLER, MF_UNCHECKED);
		break;
	case ID_DEBUG_SRAM:
		debugoptions.debug_sram = 1 - debugoptions.debug_sram;
		if(debugoptions.debug_sram)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_SRAM, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_SRAM, MF_UNCHECKED);
		break;
	case ID_INTERRUPTDEBUGGING:
		debugoptions.debug_interrupt = 1 - debugoptions.debug_interrupt;
		if(debugoptions.debug_interrupt)
			CheckMenuItem(gui.hMenu1964main, ID_INTERRUPTDEBUGGING, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_INTERRUPTDEBUGGING, MF_UNCHECKED);
		break;
	case ID_DEBUGVIINTERRUPTS:
		debugoptions.debug_vi_interrupt = 1 - debugoptions.debug_vi_interrupt;
		if(debugoptions.debug_vi_interrupt)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGVIINTERRUPTS, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGVIINTERRUPTS, MF_UNCHECKED);
		break;
	case ID_DEBUGPIINTERRUPTS:
		debugoptions.debug_pi_interrupt = 1 - debugoptions.debug_pi_interrupt;
		if(debugoptions.debug_pi_interrupt)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGPIINTERRUPTS, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGPIINTERRUPTS, MF_UNCHECKED);
		break;
	case ID_DEBUGAIINTERRUPTS:
		debugoptions.debug_ai_interrupt = 1 - debugoptions.debug_ai_interrupt;
		if(debugoptions.debug_ai_interrupt)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGAIINTERRUPTS, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGAIINTERRUPTS, MF_UNCHECKED);
		break;
	case ID_DEBUGSIINTERRUPTS:
		debugoptions.debug_si_interrupt = 1 - debugoptions.debug_si_interrupt;
		if(debugoptions.debug_si_interrupt)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGSIINTERRUPTS, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGSIINTERRUPTS, MF_UNCHECKED);
		break;
	case ID_DEBUGDYNA:
		debugoptions.debug_dyna_compiler = 1 - debugoptions.debug_dyna_compiler;
		if(debugoptions.debug_dyna_compiler)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGDYNA, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGDYNA, MF_UNCHECKED);
		break;
	case ID_DEBUGDYNAEXECUTION:
		debugoptions.debug_dyna_execution = 1 - debugoptions.debug_dyna_execution;
		if(debugoptions.debug_dyna_execution)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGDYNAEXECUTION, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGDYNAEXECUTION, MF_UNCHECKED);
		break;
	case ID_DYNALOG:
		debugoptions.debug_dyna_log = 1 - debugoptions.debug_dyna_log;
		if(debugoptions.debug_dyna_log)
			CheckMenuItem(gui.hMenu1964main, ID_DYNALOG, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DYNALOG, MF_UNCHECKED);
		break;
	case ID_DEBUG_64BITFPU:
		debugoptions.debug_64bit_fpu = 1 - debugoptions.debug_64bit_fpu;
		if(debugoptions.debug_64bit_fpu)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_64BITFPU, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_64BITFPU, MF_UNCHECKED);
		break;
	case ID_DEBUG_SELFMODCODE:
		debugoptions.debug_dyna_mod_code = 1 - debugoptions.debug_dyna_mod_code;
		if(debugoptions.debug_dyna_mod_code)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_SELFMODCODE, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_SELFMODCODE, MF_UNCHECKED);
		break;
	case ID_DEBUGPROTECTMEMORY:
		debugoptions.debug_protect_memory = 1 - debugoptions.debug_protect_memory;
		if(debugoptions.debug_protect_memory)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGPROTECTMEMORY, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUGPROTECTMEMORY, MF_UNCHECKED);
		break;
	case ID_DEBUG_EXCEPTION_SERVICES:
		debugoptions.debug_exception_services = 1 - debugoptions.debug_exception_services;
		if(debugoptions.debug_exception_services)
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_EXCEPTION_SERVICES, MF_CHECKED);
		else
			CheckMenuItem(gui.hMenu1964main, ID_DEBUG_EXCEPTION_SERVICES, MF_UNCHECKED);
		break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ProcessDebugMenuCommand(WPARAM wParam)
{
	switch(wParam)
	{
	case ID_RUNTO:
		Set_Breakpoint();
		break;
	case ID_DEBUGGER_OPEN:
		if(emustatus.Emu_Is_Running)
		{
			if(PauseEmulator())
			{
				OpenDebugger();
				ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);						/* Need to init emu */
			}
		}
		else
		{
			OpenDebugger();
		}
		break;
	case ID_DEBUGGER_CLOSE:
		if(emustatus.Emu_Is_Running)
		{
			if(PauseEmulator())
			{
				CloseDebugger();
				ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);						/* Need to init emu */
			}
		}
		else
		{
			CloseDebugger();
		}
		break;
	case ID_DEBUGTLB:
	case ID_DEBUGTLBINDETAIL:
	case ID_DEBUG_TLB_EXTRA:
	case ID_DEBUGAUDIOTASK:
	case ID_DEBUGIO:
	case ID_DEBUGIOSI:
	case ID_DEBUGIOSP:
	case ID_DEBUGIOVI:
	case ID_DEBUGIOAI:
	case ID_DEBUGIOMI:
	case ID_DEBUGIORI:
	case ID_DEBUGIOPI:
	case ID_DEBUGIODP:
	case ID_DEBUGIODPS:
	case ID_DEBUGIORDRAM:
	case ID_DEBUGSPTASK:
	case ID_DEBUGSITASK:
	case ID_DEBUGPIDMA:
	case ID_DEBUGSIDMA:
	case ID_DEBUGSPDMA:
	case ID_DEBUGMEMPAK:
	case ID_DEBUGEEPROM:
	case ID_DEBUG_CONTROLLER:
	case ID_DEBUG_SRAM:
	case ID_INTERRUPTDEBUGGING:
	case ID_DEBUGVIINTERRUPTS:
	case ID_DEBUGPIINTERRUPTS:
	case ID_DEBUGAIINTERRUPTS:
	case ID_DEBUGSIINTERRUPTS:
	case ID_DEBUGCOMPAREINTERRUPTS:
	case ID_DEBUGCPUCOUNTER:
	case ID_DEBUGDYNA:
	case ID_DEBUGDYNAEXECUTION:
	case ID_DYNALOG:
	case ID_DEBUG_64BITFPU:
	case ID_DEBUG_SELFMODCODE:
	case ID_DEBUGPROTECTMEMORY:
	case ID_DEBUG_EXCEPTION_SERVICES:
		ToggleDebugOptions(wParam);
		break;
	case ID_SETBREAKPOINT:
		{
			Set_Breakpoint();
			if(emustatus.Emu_Is_Running)
			{
				if(PauseEmulator()) ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);	/* Need to init emu */
			}
		}
		break;
	case ID_CLEAR_BREAKPOINT:
		if(DebuggerBreakPointActive)
		{
			Clear_Breakpoint();
			if(emustatus.Emu_Is_Running)
			{
				if(PauseEmulator()) ResumeEmulator(REFRESH_DYNA_AFTER_PAUSE);	/* Need to init emu */
			}
		}
		break;
	}
}
#endif
