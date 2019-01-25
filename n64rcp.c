/*$T n64rcp.c GC 1.136 03/09/02 17:40:09 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Resets a few memory registers
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
#include "n64rcp.h"

/*
 =======================================================================================================================
 =======================================================================================================================
 */

void SP_Reset(void)
{
	/* Clear all STATUS Registers */
	SP_STATUS_REG = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void SI_Reset(void)
{
	SI_STATUS_REG = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void PI_Reset(void)
{
	PI_STATUS_REG = 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void RCP_Reset(void)
{
	SP_Reset();
	SI_Reset();
	PI_Reset();

	/*
	 * VI_Reset(); £
	 * AI_Reset();
	 */
	MI_INTR_REG_R = 0;
}
