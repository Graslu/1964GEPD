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

#ifndef _1964_DLL_RSP_H
#define _1964_DLL_RSP_H

#include <windows.h>

typedef struct {
	HINSTANCE hInst;
	BOOL MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
	                          bswap on a dword (32 bits) boundry */
	BYTE * RDRAM;
	BYTE * DMEM;
	BYTE * IMEM;

	DWORD * MI__INTR_REG;

	DWORD * SP__MEM_ADDR_REG;
	DWORD * SP__DRAM_ADDR_REG;
	DWORD * SP__RD_LEN_REG;
	DWORD * SP__WR_LEN_REG;
	DWORD * SP__STATUS_REG;
	DWORD * SP__DMA_FULL_REG;
	DWORD * SP__DMA_BUSY_REG;
	DWORD * SP__PC_REG;
	DWORD * SP__SEMAPHORE_REG;

	DWORD * DPC__START_REG;
	DWORD * DPC__END_REG;
	DWORD * DPC__CURRENT_REG;
	DWORD * DPC__STATUS_REG;
	DWORD * DPC__CLOCK_REG;
	DWORD * DPC__BUFBUSY_REG;
	DWORD * DPC__PIPEBUSY_REG;
	DWORD * DPC__TMEM_REG;

	void (__cdecl *CheckInterrupts)( void );
	void (__cdecl *ProcessDlist)( void );
	void (__cdecl *ProcessAlist)( void );
	void (__cdecl *ProcessRdpList)( void );
} RSP_INFO_1_0;

typedef struct {
	HINSTANCE hInst;
	BOOL MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
	                          bswap on a dword (32 bits) boundry */
	BYTE * RDRAM;
	BYTE * DMEM;
	BYTE * IMEM;

	DWORD * MI__INTR_REG;

	DWORD * SP__MEM_ADDR_REG;
	DWORD * SP__DRAM_ADDR_REG;
	DWORD * SP__RD_LEN_REG;
	DWORD * SP__WR_LEN_REG;
	DWORD * SP__STATUS_REG;
	DWORD * SP__DMA_FULL_REG;
	DWORD * SP__DMA_BUSY_REG;
	DWORD * SP__PC_REG;
	DWORD * SP__SEMAPHORE_REG;

	DWORD * DPC__START_REG;
	DWORD * DPC__END_REG;
	DWORD * DPC__CURRENT_REG;
	DWORD * DPC__STATUS_REG;
	DWORD * DPC__CLOCK_REG;
	DWORD * DPC__BUFBUSY_REG;
	DWORD * DPC__PIPEBUSY_REG;
	DWORD * DPC__TMEM_REG;

	void ( __cdecl *CheckInterrupts)( void );
	void (__cdecl *ProcessDlist)( void );
	void (__cdecl *ProcessAlist)( void );
	void (__cdecl *ProcessRdpList)( void );
	void (__cdecl *ShowCFB)( void );
} RSP_INFO_1_1;

/********** RSP DLL: Functions *********************/

void RSPCloseDLL( void );
void RSPDllAbout( HWND hWnd );
void RSPDllConfig( HWND hWnd );
void RSPRomClosed( void );
DWORD DoRspCycles( DWORD );
void InitiateRSP_1_0( RSP_INFO_1_0 Rsp_Info, DWORD * Cycles);
void InitiateRSP_1_1( RSP_INFO_1_1 Rsp_Info, DWORD * Cycles);

void CloseRSPPlugin (void);
BOOL LoadRSPPlugin(char * libname);
void InitializeRSP (void);

extern BOOL rsp_plugin_is_loaded;

#endif

