/*$T gamesave.c GC 1.136 03/09/02 17:37:23 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    FlashRAM. Thanks to F|RES and icepir8 for the reversing and info
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
#include "r4300i.h"
#include "n64rcp.h"
#include "gamesave.h"
#include "hardware.h"
#include "memory.h"
#include "iPIF.h"
#include "1964ini.h"
#include "fileio.h"
#include "win32/windebug.h"
#include "debug_option.h"

struct GAMESAVESTATUS	gamesave;

enum { FLASH_NOOP = 0, FLASH_ERASE = 1, FLASH_WRITE = 2, FLASH_STATUS = 3 };

DWORD	FlashRAM_Mode = FLASH_NOOP;
BOOL	dmastatus = TRUE;
char	FlashRAM_Buffer[128];
DWORD	FlashRAM_Offset = 0;
DWORD	FlashRAM_Status[2];

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Flashram_Init(void)
{
	FlashRAM_Mode = FLASH_NOOP;
	dmastatus = TRUE;
	FlashRAM_Offset = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Flashram_Command(unsigned __int32 data)
{
	DEBUG_FLASHRAM_TRACE(TRACE1("*** CMD = %08X ***", data));
	switch(data >> 24)
	{
	case 0x4b:	/* Set Erase Block Mode (128 Byte - aligned ) */
		DEBUG_FLASHRAM_TRACE(TRACE0("Flash Erase Mode"));
		FlashRAM_Mode = FLASH_ERASE;
		FlashRAM_Offset = (data & 0xFFFF) * 128;
		break;

	case 0x78:	/* Second part of the Erase Block */
		DEBUG_FLASHRAM_TRACE(TRACE0("Flash Erase Block"));
		FlashRAM_Status[0] = 0x11118008;
		FlashRAM_Status[1] = 0x00c20000;
		break;

	/* Init a FlashWrite */
	case 0xb4:
		DEBUG_FLASHRAM_TRACE(TRACE0("Flash FlashWrite Init"));
		FlashRAM_Mode = FLASH_WRITE;
		break;

	/* Sets the Block for the FlashWrite (128 Byte - aligned ) */
	case 0xa5:
		DEBUG_FLASHRAM_TRACE(TRACE0("Flash FlashWrite"));
		FlashRAM_Offset = (data & 0xFFFF) * 128;
		FlashRAM_Status[0] = 0x11118004;
		FlashRAM_Status[1] = 0x00c20000;
		break;

	case 0xD2:
		{		/* Flash RAM Execute */
			switch(FlashRAM_Mode)
			{
			case FLASH_NOOP:
				break;
			case FLASH_ERASE:
				DEBUG_FLASHRAM_TRACE(TRACE1("Executed Erase: %08X", FlashRAM_Offset));
				memset(pLOAD_UBYTE_PARAM_2(0xA8000000) + FlashRAM_Offset, 0xFF, 128);
				FileIO_WriteFLASHRAM(0,0,0);	//Write to disk
				break;
			case FLASH_WRITE:
				DEBUG_FLASHRAM_TRACE(TRACE1("Executed Write: %08X", FlashRAM_Offset));
				memcpy(pLOAD_UBYTE_PARAM_2(0xA8000000) + FlashRAM_Offset, &FlashRAM_Buffer[0], 128);
				FileIO_WriteFLASHRAM(0,0,0);	//Write to disk
				break;
			}
		}
		break;
	case 0xE1:
		{		/* Set FlashRAM Status */
			DEBUG_FLASHRAM_TRACE(TRACE0("Flash Status"));
			FlashRAM_Mode = FLASH_STATUS;
			FlashRAM_Status[0] = 0x11118001;
			FlashRAM_Status[1] = 0x00c20000;
		}
		break;
	case 0xF0:	/* Set FlashRAM Read */
		/* DEBUG_FLASHRAM_TRACE(TRACE0 ("Flash Read")); */
		FlashRAM_Mode = FLASH_NOOP;
		FlashRAM_Offset = 0;
		FlashRAM_Status[0] = 0x11118004;
		FlashRAM_Status[1] = 0xf0000000;
		break;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
unsigned __int32 Flashram_Get_Status(uint32 addr)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	uint32	temp = FlashRAM_Status[addr & 1];
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	DEBUG_FLASHRAM_TRACE(TRACE2("Reading Flashram status reg at %08X, val=%08X", addr, temp));
	return temp;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DMA_Flashram_To_RDRAM(unsigned __int32 rdramaddr, unsigned __int32 flashramaddr, unsigned __int32 len)
{
	/*
	 * DEBUG_FLASHRAM_TRACE(TRACE3("DMA FlashRAM->RDRAM: %08X, %08X, %i",
	 * flashramaddr, rdramaddr, len));
	 */

	FileIO_ReadFLASHRAM(0,0,0);

	if(FlashRAM_Mode == FLASH_STATUS)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint8	*rdramoffset = (uint8 *) &gMS_RDRAM[0] + (rdramaddr & 0x007FFFFF);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		memcpy(rdramoffset, &FlashRAM_Status, len);
		DEBUG_FLASHRAM_TRACE(TRACE0("STATUS"));
	}
	else
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		uint8	*rdramoffset = (uint8 *) &gMS_RDRAM[0] + (rdramaddr & 0x007FFFFF);
		uint8	*flashramoffset = pLOAD_UBYTE_PARAM_2(0x88000000);
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		flashramaddr = (flashramaddr & 0x001FFFF) * 2;
		if(flashramaddr < 0x20000)
		{
			flashramoffset += flashramaddr;
			memcpy(rdramoffset, flashramoffset, len);
			DEBUG_FLASHRAM_TRACE(TRACE1("READ: %08X", flashramaddr));
		}
		else
		{
			DEBUG_FLASHRAM_TRACE(TRACE1("READ: %08X", flashramaddr));
		}
	}

	DEBUG_FLASHRAM_TRACE
	(
		TRACE4
			(
				"DMA Read flashram: %08X-%08X-08X-08X ...",
				MEM_READ_UWORD(rdramaddr),
				MEM_READ_UWORD(rdramaddr + 4),
				MEM_READ_UWORD(rdramaddr + 8),
				MEM_READ_UWORD(rdramaddr + 12)
			)
	);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void DMA_RDRAM_To_Flashram(unsigned __int32 rdramaddr, unsigned __int32 flashramaddr, unsigned __int32 len)
{
	/*
	 * DEBUG_FLASHRAM_TRACE(TRACE3 ("DMA RDRAM->FlashRAM: %08X, %08X, %i", rdramaddr,
	 * flashramaddr, len));
	 */
	if(gamesave.firstusedsavemedia == 0) gamesave.firstusedsavemedia = FLASHRAM_SAVETYPE;

	FileIO_ReadFLASHRAM(0,0,0);

	if(flashramaddr == 0xA8000000 || flashramaddr == 0x88000000)
	{
		if(len == 128)	/* ok, we are writing into flashram write buffer */
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			uint8	*rdramoffset = (uint8 *) &gMS_RDRAM[0] + (rdramaddr & 0x007FFFFF);
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			DEBUG_FLASHRAM_TRACE
			(
				TRACE4
					(
						"Value in RDRAM: %08X-%08X-%08X-%08X ...",
						*(uint32 *) rdramoffset,
						*(uint32 *) (rdramoffset + 4),
						*(uint32 *) (rdramoffset + 8),
						*(uint32 *) (rdramoffset + 12)
					)
			);
			memcpy(&FlashRAM_Buffer[0], rdramoffset, 128);
		}
		else
		{
			DEBUG_FLASHRAM_TRACE(TRACE1("Warning, Flashram write, len<>128, len=%d", len));
		}
	}
	else
	{
		DEBUG_FLASHRAM_TRACE(TRACE1("Warning, Flashram write, addr<>0xA8000000, addr=%8X", flashramaddr));
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SW_Flashram(unsigned __int32 addr, unsigned __int32 val)
{
	/* TRACE2("SW directly to flashram: Addr=%08X, val=%08X", addr, val); */
}
