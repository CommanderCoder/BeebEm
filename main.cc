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

#include <stdio.h>
#include <stdarg.h>
#include <new>

#ifdef BEEBWIN
#include <windows.h>
#endif

#include "6502core.h"
#include "beebwin.h"
#include "log.h"
#include "serial.h"
#ifdef BEEBWIN
#include "SelectKeyDialog.h"
#endif

Model MachineType;
BeebWin *mainWin = nullptr;

#ifdef BEEBWIN
HINSTANCE hInst;
HWND hCurrentDialog = nullptr;
HACCEL hCurrentAccelTable = nullptr;

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */,
                     LPSTR /* lpszCmdLine */, int /* nCmdShow */)
{
	hInst = hInstance;

	OpenLog();

	mainWin = new(std::nothrow) BeebWin();

	if (mainWin == nullptr)
	{
		return 1;
	}

	if (!mainWin->Initialise())
	{
		delete mainWin;
		return 1;
	}

	// Create serial threads
	SerialInit();

	for (;;)
	{
		MSG msg;

		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) || mainWin->IsFrozen())
		{
			if (!GetMessage(&msg,   // message structure
			                NULL,   // handle of window receiving the message
			                0,      // lowest message to examine
			                0))
				break; // Quit the app on WM_QUIT

			if (hCurrentDialog != nullptr && hCurrentAccelTable != nullptr)
			{
				TranslateAccelerator(hCurrentDialog, hCurrentAccelTable, &msg);
			}

			if (hCurrentDialog == nullptr)
			{
				TranslateMessage(&msg); // Translates virtual key codes
				DispatchMessage(&msg); // Dispatches message to window
			}
			else
			{
				bool handled = false;

				if (selectKeyDialog != nullptr)
				{
					handled = selectKeyDialog->HandleMessage(msg);
				}

				if (!handled && !IsDialogMessage(hCurrentDialog, &msg))
				{
					TranslateMessage(&msg); // Translates virtual key codes
					DispatchMessage(&msg); // Dispatches message to window
				}
			}
		}

		if (!mainWin->IsFrozen() && !mainWin->IsPaused()) {
			Exec6502Instruction();
		}
	}

	mainWin->KillDLLs();

	CloseLog();

	Kill_Serial();

	delete mainWin;

	return 0;
}

#else

int _vscprintf (const char * format, va_list pargs) {
    int retval;
    va_list argcopy;
    va_copy(argcopy, pargs);
    retval = vsnprintf(NULL, 0, format, argcopy);
    va_end(argcopy);
    return retval;
 }


int mainInit()
{
    OpenLog();

    mainWin = new(std::nothrow) BeebWin();

    if (mainWin == nullptr)
    {
        return 1;
    }

    if (!mainWin->Initialise())
    {
        delete mainWin;
        mainWin=0;
        return 1;
    }

    // Create serial threads
    SerialInit();
    
    return 0;
}

int mainStep()
//    for (;;)
{
    
    // Windows MAIN
    // keep processing instructions until WM_QUIT
    // When there is a mesage or beeb is frozen, process Messages
    // either to TranslateAccelerator
    // Translates virtual key codes,Dispatches message to window
    // to CurrentDialogue
    
    bool done = false;
    if (done)
        return 1;

    if (!mainWin->IsFrozen() && !mainWin->IsPaused()) {
        Exec6502Instruction();
    }
    
    return 0;
}

int mainEnd()
{
    mainWin->KillDLLs();

    CloseLog();

    Kill_Serial();

    delete mainWin;

    return 0;
}


#endif


