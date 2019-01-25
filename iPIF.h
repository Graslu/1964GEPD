/*$T iPIF.h GC 1.136 02/28/02 08:16:35 */


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
#ifndef __PIF_H
#define __PIF_H

#define PIF_RAM_PHYS	0x7C0
#define IPIF_EXIT		6

typedef struct
{
	int enabled;
	int mempak;
	int use_mempak;
} t_controller;

void	Init_iPIF(void);
void	Close_iPIF(void);
void	iPifCheck(void);

void	ReadControllerPak(int device, char *cmd);
void	WriteControllerPak(int device, char *cmd);
int		ControllerCommand(unsigned __int8 *cmd, int device);

void	ReadEEprom(char *dest, long offset);
void	WriteEEprom(char *src, long offset);
int		EEpromCommand(unsigned __int8 *cmd, int device);

void	LogPIFData(char *data, int input);
#endif
