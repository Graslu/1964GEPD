/*$T cheatcode.h GC 1.136 02/28/02 07:49:38 */


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
#ifndef _CHEATCODE_H__1964_
#define _CHEATCODE_H__1964_

#define MAX_CHEATCODE_PER_GROUP		100
#define MAX_CHEATCODE_GROUP_PER_ROM 254	//Cannot exceed 254 groups, must be represented by using 1 byte

//Option to apply cheat code and lock memory
//#define CHEATCODE_LOCK_MEMORY

enum APPLYCHEATMODE { INGAME, BOOTUPONCE, GSBUTTON, ONLYIN1964 };

struct CODENODE_STRUCT
{
	uint32	addr;
	uint16	val;
};

typedef struct CODENODE_STRUCT	CHEATCODENODE;

struct CODEGROUP_STRUCT
{
	int				country;
	int				codecount;
	BOOL			active;
	char			name[80];
	char			note[256];
	CHEATCODENODE	codelist[MAX_CHEATCODE_PER_GROUP];
};
typedef struct CODEGROUP_STRUCT CODEGROUP;

extern int						codegroupcount;
extern CODEGROUP				*codegrouplist;
extern char						current_cheatcode_rom_internal_name[30];

extern void						InitCodeListForCurrentGame(void);
extern void						CodeList_Clear(void);
extern void						CodeList_GotoBeginning(void);
extern BOOL						CodeList_ApplyAllCode(enum APPLYCHEATMODE mode);
extern BOOL						CodeList_ReadCode(char *intername_rom_name);
BOOL							IsCodeMatchRomCountryCode(int cheat_country_code, int rom_country_code);

#ifdef CHEATCODE_LOCK_MEMORY
extern uint16 *cheatCodeBlockMap[0x800];
void InitCheatCodeEngineMemoryLock(void);
void CloseCheatCodeEngineMemoryLock(void);
BOOL CodeList_ApplyCode_At_Address(int index, uint32 addr_to_apply);

#define BYTE_AFFECTED_BY_CHEAT_CODES	0xFF
#endif

#endif
