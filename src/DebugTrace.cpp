/****************************************************************
BeebEm - BBC Micro and Master 128 Emulator
Copyright (C) 2021  Chris Needham

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

#ifdef BEEBWIN
#include <windows.h>
#else
#include <stdlib.h>
#endif
#include <stdarg.h>
#include <stdio.h>

#ifndef BEEBWIN

// version for mac
int _vscprintf (const char * format, va_list pargs) {
    int retval;
    va_list argcopy;
    va_copy(argcopy, pargs);
    retval = vsnprintf(NULL, 0, format, argcopy);
    va_end(argcopy);
    return retval;
 }
#endif

#include "DebugTrace.h"

#if !defined(NDEBUG)

void DebugTrace(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	// Calculate required length, +1 is for NUL terminator
	const int length = _vscprintf(format, args) + 1;

	char *buffer = (char*)malloc(length);

	if (buffer != nullptr)
	{
		vsprintf(buffer, format, args);
#ifdef BEEBWIN
        OutputDebugString(buffer);
#endif
        free(buffer);
	}

	va_end(args);
    
}

#endif
