/*$T gamesave.h GC 1.136 02/28/02 08:00:55 */


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
#ifndef _GAMESAVE_H__1964_
#define _GAMESAVE_H__1964_

/* SRam */
#define SRAM_SIZE	0x8000
#define SRAM_MASK	0x7FFF

/* FlashRam */
#define FLASHRAM_SIZE	0x20000
#define FLASHRAM_MASK	0x1FFFF

struct GAMESAVESTATUS
{
	BOOL	EEprom_used;
	BOOL	EEprom_written;
	BOOL	Sram_used;
	BOOL	Sram_written;
	BOOL	FlashRam_written;
	uint8	EEprom[0x1000];				/* 4KB */
	uint8	SRam[SRAM_SIZE];			/* 0x8000 Bytes */
	int		firstusedsavemedia;
	uint8	mempak[4][1024 * 32];		/* Define 4 mempak, each one is 32K */
	BOOL	mempak_used[4];				/* status of each mempak */
	BOOL	mempak_written[4];			/* write status of each mempak. If it is never writen, will not save it */
};
extern struct GAMESAVESTATUS	gamesave;

#define FLASHRAM_STATUS_REG_WORD1_ADDR	0xA8000000
#define FLASHRAM_STATUS_REG_WORD2_ADDR	0xA8000004
#define FLASHRAM_COMMAND_REG_ADDR		0xA8010000
#define FLASHRAM_STATUS_REG_1			(LOAD_UWORD_PARAM_2(FLASHRAM_STATUS_REG_WORD1_ADDR))
#define FLASHRAM_STATUS_REG_2			(LOAD_UWORD_PARAM_2(FLASHRAM_STATUS_REG_WORD2_ADDR))

void							Flashram_Command(unsigned __int32 val);
unsigned __int32				Flashram_Get_Status(uint32 addr);

/*
 =======================================================================================================================
    Trigger from r4300i opcode LW and SW
 =======================================================================================================================
 */
#define CHECK_FLASHRAM_SW(addr, val) \
	if(addr == FLASHRAM_COMMAND_REG_ADDR) \
	{ \
		Flashram_Command(val); \
		return; \
	}
#define CHECK_FLASHRAM_LW(addr) \
	if((addr & 0xFFFFFFF8) == FLASHRAM_STATUS_REG_WORD1_ADDR) \
	{ \
		return(Flashram_Get_Status(addr)); \
	}

/*
 * if( addr == FLASHRAM_STATUS_REG_WORD1_ADDR || addr ==
 * FLASHRAM_STATUS_REG_WORD2_ADDR) \ { \ if( addr ==
 * FLASHRAM_STATUS_REG_ADDR_WORD2 ) DisplayError("Reading flashram statue word
 * 2"); \ reg = Flashram_Get_Status(); \ }
 */
void	DMA_Flashram_To_RDRAM(unsigned __int32 rdramaddr, unsigned __int32 flashramaddr, unsigned __int32 len);
void	DMA_RDRAM_To_Flashram(unsigned __int32 rdramaddr, unsigned __int32 flashramaddr, unsigned __int32 len);
void	SW_Flashram(unsigned __int32 addr, unsigned __int32 val);

void	Flashram_Init(void);
#endif
