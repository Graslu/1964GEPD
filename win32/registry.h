/*$T registry.h GC 1.136 02/28/02 07:50:39 */


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
#ifndef _REGISTRY_H__1964_
#define _REGISTRY_H__1964_

typedef struct
{
	int		WindowXPos;
	int		WindowYPos;
	int		Maximized;
	int		ClientWidth;
	int		ThreadPriority;
	char	ROMPath[MAX_PATH];
	char	AudioPlugin[80];
	char	InputPlugin[80];
	char	VideoPlugin[80];
	char	RSPPlugin[80];
} RegSettingsTyp;

RegSettingsTyp	gRegSettings;

extern int		ReadConfiguration(void);
extern void		WriteConfiguration(void);
#endif
