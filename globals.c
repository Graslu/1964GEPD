/*$T globals.c GC 1.136 02/28/02 08:01:08 */


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
#include "1964ini.h"
#include "windows.h"
#include "./win32/wingui.h"

char			*CURRENT1964VERSION = "1964_085";
uint32			gAllocationLength;
uint8			HeaderDllPass[0x40];
volatile int	Rom_Loaded = 0;
t_rominfo		rominfo;				/* Rom information */
char			generalmessage[256];	/* general purpose buffer to display messages */
int				showcursor = 1;

/*
 =======================================================================================================================
    This function should be called if you want to change the cursor status £
    as calling 'ShowCursor' multiple times will glitch and have issues so £
    this prevents the multiple call problem
 =======================================================================================================================
 */
void HideCursor(int state)
{
	if(state == TRUE)
	{
		if(showcursor)
		{
			ShowCursor(FALSE);
			showcursor = 0;
		}
	}
	else
	{
		if(!showcursor)
		{
			ShowCursor(TRUE);
			showcursor = 1;
		}
	}
	ShowWindow(gui.hStatusBar, guistatus.IsFullScreen || !guioptions.display_statusbar ? SW_HIDE : SW_SHOW);
}