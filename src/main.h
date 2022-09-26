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

/* Mike Wyatt and NRM's port to win32 - 7/6/97 */

#ifndef MAIN_HEADER
#define MAIN_HEADER

#ifdef BEEBWIN
#include <windows.h>
#endif
#include "beebwin.h"
#include "model.h"

extern Model MachineType;
extern BeebWin *mainWin;

#ifdef BEEBWIN
extern HINSTANCE hInst;
extern HWND hCurrentDialog;
extern HACCEL hCurrentAccelTable;
#endif

#ifndef BEEBWIN

template < typename T, size_t N >
size_t countof( T ( & arr )[ N ] )
{
    return std::extent< T[ N ] >::value;
}

#define _countof countof
#define _stricmp strcasecmp
#define _strerror(x) strerror(errno)

int _vscprintf (const char * format, va_list pargs);

int mainInit();
int mainStep();
int mainEnd();

int BeebMainKeyUpDown(UINT, long,long);

#endif

#endif
