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

#include "../interrupt.h"
#include "../n64rcp.h"
#include "../memory.h"
#include "DLL_Rsp.h"
#include "Dll_Audio.h"
#include "Dll_Video.h"
#include "wingui.h"

/********** RSP DLL: Functions *********************/
void	(__cdecl *_RSPCloseDLL)			( void ) = NULL;
void	(__cdecl *_RSPDllAbout)			( HWND hWnd ) = NULL;
void	(__cdecl *_RSPDllConfig)		( HWND hWnd ) = NULL;
void	(__cdecl *_RSPRomClosed)		( void ) = NULL;
DWORD	(__cdecl *_DoRspCycles)			( DWORD ) = NULL;
void	(__cdecl *_InitiateRSP_1_0)		( RSP_INFO_1_0 rspinfo, DWORD * cycles) = NULL;
void	(__cdecl *_InitiateRSP_1_1)		( RSP_INFO_1_1 rspinfo, DWORD * cycles) = NULL;
void	(__cdecl *_RSP_GetDllInfo)		(PLUGIN_INFO *) = NULL;


uint16		RSPVersion = 0;;
DWORD		RspTaskValue = 0;;
HINSTANCE	hRSPHandle = NULL;
BOOL		rsp_plugin_is_loaded = FALSE;


//
// ----------------------------------------------------------------------------
// Functions called by RSP Plugin
// ----------------------------------------------------------------------------
//
void __cdecl ProcessDList(void)
{
	VIDEO_ProcessDList();
}

void __cdecl ShowCFB(void)
{
	VIDEO_ShowCFB();
}

void __cdecl ProcessRDPList(void)
{
	VIDEO_ProcessRDPList();
}

void __cdecl ProcessAList(void)
{
	AUDIO_ProcessAList();
}

void __cdecl RspCheckInterrupts(void)
{
	CheckInterrupts();
}

//
// ----------------------------------------------------------------------------
// Wrapper functions from the RSP Plugin
// ----------------------------------------------------------------------------
//
void RSPCloseDLL( void )
{
	if( _RSPCloseDLL )
		_RSPCloseDLL();
}

void RSPDllAbout( HWND hWnd )
{
	if( _RSPDllAbout )
		_RSPDllAbout(hWnd);
}

void RSPDllConfig( HWND hWnd )
{
	if( _RSPDllConfig )
		_RSPDllConfig(hWnd);
}

void RSPRomClosed( void )
{
	if( _RSPRomClosed )
		_RSPRomClosed();
}
DWORD DoRspCycles( DWORD cycles )
{
	if( _DoRspCycles )
	{
		DWORD retval;
		DWORD sp_pc_save = SP_PC_REG;
		SP_PC_REG &= 0xFFC;
		retval = _DoRspCycles(cycles);
		SP_PC_REG = sp_pc_save;
		return retval;
	}
	else
		return 0;
}

void InitiateRSP_1_0( RSP_INFO_1_0 rspinfo, DWORD * cycles)
{
	if( _InitiateRSP_1_0 )
		_InitiateRSP_1_0(rspinfo, cycles );
}
void InitiateRSP_1_1( RSP_INFO_1_1 rspinfo, DWORD * cycles)
{
	if( _InitiateRSP_1_1 )
		_InitiateRSP_1_1(rspinfo, cycles );
}

//
// ----------------------------------------------------------------------------
// Supporting functions For the RSP Plugin
// ----------------------------------------------------------------------------
//

void InitializeRSP (void) 
{
	RSP_INFO_1_0 RspInfo10;
	RSP_INFO_1_1 RspInfo11;

	RspInfo10.CheckInterrupts = RspCheckInterrupts;
	RspInfo11.CheckInterrupts = RspCheckInterrupts;
	RspInfo10.ProcessDlist = ProcessDList;
	RspInfo11.ProcessDlist = ProcessDList;
	RspInfo10.ProcessAlist = ProcessAList;
	RspInfo11.ProcessAlist = ProcessAList;
	RspInfo10.ProcessRdpList = ProcessRDPList;
	RspInfo11.ProcessRdpList = ProcessRDPList;
	RspInfo11.ShowCFB = ShowCFB;

	RspInfo10.hInst = gui.hInst;
	RspInfo11.hInst = gui.hInst;
	RspInfo10.RDRAM = gMS_RDRAM;
	RspInfo11.RDRAM = gMS_RDRAM;
	RspInfo10.DMEM = (uint8*)&SP_DMEM;
	RspInfo11.DMEM = (uint8*)&SP_DMEM;
	RspInfo10.IMEM = (uint8*)&SP_IMEM;
	RspInfo11.IMEM = (uint8*)&SP_IMEM;
	RspInfo10.MemoryBswaped = FALSE;
	RspInfo11.MemoryBswaped = FALSE;

	RspInfo10.MI__INTR_REG = &MI_INTR_REG_R;
	RspInfo11.MI__INTR_REG = &MI_INTR_REG_R;
		
	RspInfo10.SP__MEM_ADDR_REG = &SP_MEM_ADDR_REG;
	RspInfo11.SP__MEM_ADDR_REG = &SP_MEM_ADDR_REG;
	RspInfo10.SP__DRAM_ADDR_REG = &SP_DRAM_ADDR_REG;
	RspInfo11.SP__DRAM_ADDR_REG = &SP_DRAM_ADDR_REG;
	RspInfo10.SP__RD_LEN_REG = &SP_RD_LEN_REG;
	RspInfo11.SP__RD_LEN_REG = &SP_RD_LEN_REG;
	RspInfo10.SP__WR_LEN_REG = &SP_WR_LEN_REG;
	RspInfo11.SP__WR_LEN_REG = &SP_WR_LEN_REG;
	RspInfo10.SP__STATUS_REG = &SP_STATUS_REG;
	RspInfo11.SP__STATUS_REG = &SP_STATUS_REG;
	RspInfo10.SP__DMA_FULL_REG = &SP_DMA_FULL_REG;
	RspInfo11.SP__DMA_FULL_REG = &SP_DMA_FULL_REG;
	RspInfo10.SP__DMA_BUSY_REG = &SP_DMA_BUSY_REG;
	RspInfo11.SP__DMA_BUSY_REG = &SP_DMA_BUSY_REG;
	RspInfo10.SP__PC_REG = &SP_PC_REG;
	RspInfo11.SP__PC_REG = &SP_PC_REG;
	RspInfo10.SP__SEMAPHORE_REG = &SP_SEMAPHORE_REG;
	RspInfo11.SP__SEMAPHORE_REG = &SP_SEMAPHORE_REG;
		
	RspInfo10.DPC__START_REG = &DPC_START_REG;
	RspInfo11.DPC__START_REG = &DPC_START_REG;
	RspInfo10.DPC__END_REG = &DPC_END_REG;
	RspInfo11.DPC__END_REG = &DPC_END_REG;
	RspInfo10.DPC__CURRENT_REG = &DPC_CURRENT_REG;
	RspInfo11.DPC__CURRENT_REG = &DPC_CURRENT_REG;
	RspInfo10.DPC__STATUS_REG = &DPC_STATUS_REG;
	RspInfo11.DPC__STATUS_REG = &DPC_STATUS_REG;
	RspInfo10.DPC__CLOCK_REG = &DPC_CLOCK_REG;
	RspInfo11.DPC__CLOCK_REG = &DPC_CLOCK_REG;
	RspInfo10.DPC__BUFBUSY_REG = &DPC_BUFBUSY_REG;
	RspInfo11.DPC__BUFBUSY_REG = &DPC_BUFBUSY_REG;
	RspInfo10.DPC__PIPEBUSY_REG = &DPC_PIPEBUSY_REG;
	RspInfo11.DPC__PIPEBUSY_REG = &DPC_PIPEBUSY_REG;
	RspInfo10.DPC__TMEM_REG = &DPC_TMEM_REG;
	RspInfo11.DPC__TMEM_REG = &DPC_TMEM_REG;
	
	if (RSPVersion == 0x0100) 
	{ 
		InitiateRSP_1_0(RspInfo10, &RspTaskValue); 
	}
	else
	{ 
		InitiateRSP_1_1(RspInfo11, &RspTaskValue); 
	}
#ifndef _DEBUG
	//InitiateInternalRSP(RspInfo11, &RspTaskValue);
#endif
}

BOOL LoadRSPPlugin(char * libname) 
{
	PLUGIN_INFO RSPPluginInfo;

	hRSPHandle = LoadLibrary(libname);
	if (hRSPHandle == NULL) 
	{  
		return FALSE; 
	}

	_RSP_GetDllInfo = (void (__cdecl *)(PLUGIN_INFO *))GetProcAddress( hRSPHandle, "GetDllInfo" );
	if( _RSP_GetDllInfo == NULL) 
	{ 
		return FALSE; 
	}

	_RSP_GetDllInfo(&RSPPluginInfo);
	RSPVersion = RSPPluginInfo.Version;

	if(RSPPluginInfo.Type == PLUGIN_TYPE_RSP) /* Check if this is a video plugin */
	{
		if (RSPVersion == 1) 
		{ 
			RSPVersion = 0x0100; 
		}

		if (RSPVersion == 0x100) {
			_InitiateRSP_1_0 = (void (__cdecl *)(RSP_INFO_1_0,DWORD *))GetProcAddress( hRSPHandle, "InitiateRSP" );
		}
		else
		{
			_InitiateRSP_1_1 = (void (__cdecl *)(RSP_INFO_1_1,DWORD *))GetProcAddress( hRSPHandle, "InitiateRSP" );
		}

		_DoRspCycles = (DWORD (__cdecl *)(DWORD))GetProcAddress( hRSPHandle, "DoRspCycles" );
		_RSPRomClosed = (void (__cdecl *)(void))GetProcAddress( hRSPHandle, "RomClosed" );
		_RSPCloseDLL = (void (__cdecl *)(void))GetProcAddress( hRSPHandle, "CloseDLL" );
		_RSPDllConfig = (void (__cdecl *)(HWND))GetProcAddress( hRSPHandle, "DllConfig" );
		_RSPDllAbout = (void (__cdecl *)(HWND))GetProcAddress( hRSPHandle, "DllAbout" );

		if( _DoRspCycles == NULL	||
			_RSPRomClosed == NULL	||
			_RSPCloseDLL == NULL	||
			_RSPDllConfig == NULL	||
			( _InitiateRSP_1_0 == NULL && _InitiateRSP_1_1 == NULL ) )
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}

void CloseRSPPlugin (void) 
{
	if (hRSPHandle != NULL) 
	{ 
		RSPCloseDLL(); 
		FreeLibrary(hRSPHandle);
		hRSPHandle = NULL;
	}
}

