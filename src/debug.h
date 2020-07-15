//
// BeebEm debugger
//

#ifndef DEBUG_HEADER
#define DEBUG_HEADER

#include "viastate.h"
#include <MacTypes.h>

extern int DebugEnabled;

enum DebugType {
	DEBUG_VIDEO,
	DEBUG_USERVIA,
	DEBUG_SYSVIA,
	DEBUG_TUBE,
	DEBUG_SERIAL,
	DEBUG_ECONET
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
