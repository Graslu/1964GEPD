/*$T fileio.h GC 1.136 03/09/02 15:51:03 */


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
#ifndef _FILEIO_H__1964_
#define _FILEIO_H__1964_

#include "1964ini.h"

/* function declarations */
BOOL		ReadRomData(char *RomPath);
long		ReadRomHeader(char *RomPath, INI_ENTRY *ini_entry);
long		ReadZippedRomHeader(char *RomPath, INI_ENTRY *ini_entry);
BOOL		ReadZippedRomData(char *RomPath);
BOOL		ByteSwap(uint32 Size, uint8 *Image);
int			LoadGNUDistConditions(char *ConditionsBuf);
void		FileIO_WriteMemPak(int pak_no);
void		FileIO_LoadMemPak(int pak_no);
void		FileIO_WriteEEprom(void);
void		FileIO_LoadEEprom(void);
void		FileIO_WriteFLASHRAM(int, int, int);
void		FileIO_ReadFLASHRAM(int, int, int);
void		FileIO_SaveState(void);
void		FileIO_LoadState(void);
void		FileIO_gzSaveState(void);
void		FileIO_gzLoadState(void);
void		FileIO_gzSaveStateFile(const char *filename);
void		FileIO_gzLoadStateFile(const char *filename);
void		FileIO_ImportPJ64State(const char *filename);
void		FileIO_ExportPJ64State(const char *filename);
BOOL		FileIO_Load1964Ini(void);
BOOL		FileIO_Write1964Ini(void);
void		SwapRomName(uint8 *name);

extern BOOL Is_Reading_Rom_File;
extern BOOL To_Stop_Reading_Rom_File;
extern void Close_Save();
#endif
