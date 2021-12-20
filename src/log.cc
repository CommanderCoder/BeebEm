/****************************************************************
BeebEm - BBC Micro and Master 128 Emulator
Copyright (C) 1994  David Alan Gilbert
Copyright (C) 1994  Nigel Magnay
Copyright (C) 1997  Mike Wyatt

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public
License along with this program; if not, write to the Free
Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA  02110-1301, USA.
****************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "log.h"

static FILE *tlog = nullptr;

static const char* const mon[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void OpenLog()
{
	tlog = nullptr;
	//  tlog = fopen("\\trace.log", "wt");
}

void CloseLog()
{
	if (tlog != nullptr) {
		fclose(tlog);
		tlog = nullptr;
	}
}

void WriteLog(char *fmt, ...)
{
	if (tlog)
	{
		va_list argptr;
		va_start(argptr, fmt);

		char buff[256];
		vsprintf(buff, fmt, argptr);

		va_end(argptr);

        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		fprintf(tlog, "%s", buff);
	}
}
