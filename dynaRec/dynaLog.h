/*$T dynaLog.h GC 1.136 02/28/02 08:31:38 */


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
#ifndef __DYNA_LOG__
#define __DYNA_LOG__

/* Debugging tools (Disable these for public releases) */
//#define ENABLE_DISPLAY_ERROR
#ifdef DEBUG_COMMON
#define LOG_DYNA	/* mandatory. do not comment. */
#else

/*
 * define LOG_DYNA //comment for release, uncomment for windiff £
 * #define LOG_REGISTERS //comment for release, optional for windiff
 */
#endif
extern void __cdecl LogDyna(char *debug, ...);
extern void			InitLogDyna(void);
extern void			CloseLogDyna(void);

#ifdef LOG_DYNA
#define LOGGING_DYNA(macro) macro;
#else
#define LOGGING_DYNA(macro)
#endif LOG_DYNA
#endif
