/*$T dynaLog.c GC 1.136 02/28/02 08:31:26 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Functions for logging the dynarec in x86 assembly form.
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
#include <stdio.h>
#include "../globals.h"
#include "../debug_option.h"
#include "regcache.h"
#include "../win32/wingui.h"

extern MultiPass	gMultiPass;

int					dynalog_count;
BOOL				dynalog_fileisopen;
FILE				*dynalog_stream;

const int			dynalog_maxcount = 1000;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void __cdecl LogDyna(char *debug, ...)
{
#ifdef _DEBUG
	if(debugoptions.debug_dyna_log)
#endif
		if(gMultiPass.WriteCode)
		{
			/*~~~~~~~~~~~~~~~~~~*/
			va_list argptr;
			char	text[1024];
			char	filename[256];
			/*~~~~~~~~~~~~~~~~~~*/

			strcpy(filename, directories.main_directory);
			strcat(filename, "dyna.log");

			if(!dynalog_fileisopen)
			{
				dynalog_stream = fopen(filename, "at");
				if(dynalog_stream == NULL) return;
				dynalog_fileisopen = TRUE;
			}

			va_start(argptr, debug);
			vsprintf(text, debug, argptr);
			va_end(argptr);

			fprintf(dynalog_stream, "%s", text);

			dynalog_count++;
			if(dynalog_count % dynalog_maxcount == 0)
			{
				fclose(dynalog_stream);
				dynalog_fileisopen = FALSE;
			}
		}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void InitLogDyna(void)
{
#ifdef _DEBUG
	if(debugoptions.debug_dyna_log)
#endif
	{
		/*~~~~~~~~~~~~~~~~~~*/
		char	filename[256];
		/*~~~~~~~~~~~~~~~~~~*/

		strcpy(filename, directories.main_directory);
		strcat(filename, "dyna.log");

		dynalog_stream = fopen(filename, "wt");
		if(dynalog_stream == NULL) return;

		fprintf(dynalog_stream, "1964 Dynarec Log -- ");
		if(gMultiPass.UseOnePassOnly == 0)
			fprintf(dynalog_stream, "Multiple Pass\n");
		else
			fprintf(dynalog_stream, "Single Pass\n");

		fprintf(dynalog_stream, "Image Name = ");
		fprintf(dynalog_stream, rominfo.name);
		fprintf(dynalog_stream, "\n\n");

		dynalog_count = 0;
		fclose(dynalog_stream);
		dynalog_fileisopen = FALSE;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CloseLogDyna(void)
{
	if(dynalog_fileisopen)
	{
		fclose(dynalog_stream);
		dynalog_fileisopen = FALSE;
	}
}
