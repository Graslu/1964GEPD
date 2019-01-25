/*$T Globals.h GC 1.136 02/28/02 08:01:26 */


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
#ifndef _GLOBALS_H__1964_
#define _GLOBALS_H__1964_


/* Data Types */
#define uint64	unsigned __int64
#define uint32	unsigned __int32
#define uint16	unsigned __int16
#define uint8	unsigned __int8

/* unsigned types */
#define _u64	unsigned __int64	/* 64 bit */
#define _u32	unsigned __int32	/* 32 bit */
#define _u16	unsigned __int16	/* 16 bit */
#define _u8		unsigned __int8		/* 8 bit */

/* signed types */
#define _s64	__int64				/* 64 bit */
#define _s32	__int32				/* 32 bit */
#define _s16	__int16				/* 16 bit */
#define _s8		__int8				/* 8 bit */

extern void __cdecl DisplayError(char *Message, ...);
extern void HideCursor(int state);

typedef struct
{
	char	*real_name;
	char	*dmem_name;
	char	*imem_name;
	char	rom_name[256];
	uint16	validation;				/* 0x00 */
	uint8	compression;			/* 0x02 */
	uint8	unknown1;				/* 0x03 */
	uint32	clockrate;				/* 0x04 */
	uint32	programcounter;			/* 0x08 */
	uint32	release;				/* 0x0c */
	uint32	crc1;					/* 0x10 */
	uint32	crc2;					/* 0x14 */
	uint64	unknown2;				/* 0x18 */
	uint8	name[20];				/* 0x20 - 0x33 */
	uint8	unknown3;				/* 0x34 */
	uint8	unknown4;				/* 0x35 */
	uint8	unknown5;				/* 0x36 */
	uint8	unknown6;				/* 0x37 */
	uint8	unknown7;				/* 0x38 */
	uint8	unknown8;				/* 0x39 */
	uint8	unknown9;				/* 0x3a */
	uint8	manufacturerid;			/* 0x3b */
	uint16	cartridgeid;			/* 0x3c */
	uint8	countrycode;			/* 0x3e */
	uint8	unknown10;				/* 0x3f */
	uint64	CIC;
	uint64	TV_System;
	uint32	RDRam_Size_Hack;
} t_rominfo;

extern uint32			gAllocationLength;
extern t_rominfo		rominfo;
extern volatile int		Rom_Loaded;
extern char				*CURRENT1964VERSION;
extern char				generalmessage[256];
int						showcursor;
#endif /* _GLOBALS_H__1964_ */
