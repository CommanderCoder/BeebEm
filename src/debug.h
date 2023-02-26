/****************************************************************
BeebEm - BBC Micro and Master 128 Emulator
Copyright (C) 2004  Mike Wyatt
Copyright (C) 2004  Rob O'Donnell
Copyright (C) 2009  Steve Pick

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
//
// BeebEm debugger
//

#ifndef DEBUG_HEADER
#define DEBUG_HEADER

#include "viastate.h"
#include <MacTypes.h>

extern bool DebugEnabled;

//enum DebugType {
//	DEBUG_VIDEO,
//	DEBUG_USERVIA,
//	DEBUG_SYSVIA,
//	DEBUG_TUBE,
//	DEBUG_SERIAL,
//	DEBUG_ECONET
//};

enum class DebugType {
    None,
    Video, 
    UserVIA,
    SysVIA,
    Tube,
    Serial,
    Econet,
    Teletext,
    RemoteServer,
    Manual,
    breakpoint,
    BRK
};

int GetCheckBoxValue(OSType box);
void SetCheckBoxValue(OSType box, int State);
void DebugOpenDialog(void);
void DebugCloseDialog(void);
bool DebugDisassembler(int addr, int Accumulator, int XReg, int YReg, int PSR, bool host);
void DebugDisplayTrace(DebugType type, bool host, const char *info);
void DebugDisplayInfo(const char *info);
void DebugVideoState(void);
void DebugUserViaState(void);
void DebugSysViaState(void);
void DebugViaState(const char *s, VIAState *v);
void DebugWriteMem(int addr, int data, bool host);
int DebugReadMem(int addr, bool host);
int DebugDisassembleInstruction(int addr, bool host, char *opstr);

#endif
