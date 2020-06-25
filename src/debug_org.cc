//
// BeebEm debugger
//
// Mike Wyatt - Nov 2004
//

#include <ctype.h>
#include <string.h>
#include "main.h"
#include "beebmem.h"
#include "6502core.h"
#include "debug.h"

int DebugEnabled = false;        // Debug dialog visible

//*******************************************************************
// Data structs

enum PSRFlags {
  FlagC=1,
  FlagZ=2,
  FlagI=4,
  FlagD=8,
  FlagB=16,
  FlagV=64,
  FlagN=128
};

#define MAX_LINES 4096          // Max lines in info window
#define LINES_IN_INFO 43        // Visible lines in info window
#define MAX_COMMAND_LEN 20      // Max debug command length
#define MAX_BPS 50				// Max num of breakpoints

// Breakpoints
struct Breakpoint
{
	int start;
	int end;
};

Breakpoint Breakpoints[MAX_BPS];

// Where control goes
#define NORM 1
#define JUMP 2
#define FORK 4
#define STOP 8
#define CTLMASK (NORM|JUMP|FORK|STOP)

// Instruction format
#define IMM  0x20
#define ABS  0x40
#define ACC  0x80
#define IMP  0x100
#define INX  0x200
#define INY  0x400
#define ZPX  0x800
#define ABX  0x1000
#define ABY  0x2000
#define REL  0x4000
#define IND  0x8000
#define ZPY  0x10000
#define ZPG  0x20000
#define ILL  0x40000

#define ADRMASK (IMM|ABS|ACC|IMP|INX|INY|ZPX|ABX|ABY|REL|IND|ZPY|ZPG|ILL)

// Instruction data
struct InstInfo
{
	char opn[4];
	int  nb;
	int  flag;
	int  c6502;
};

InstInfo optable[256] =
{
	/* 00 */	{ "BRK",  1, IMP|STOP, 0, },
	/* 01 */	{ "ORA",  2, INX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 04 */	{ "TSB",  2, ZPG|NORM, 1, },
	/* 05 */	{ "ORA",  2, ZPG|NORM, 0, },
	/* 06 */	{ "ASL",  2, ZPG|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 08 */	{ "PHP",  1, IMP|NORM, 0, },
	/* 09 */	{ "ORA",  2, IMM|NORM, 0, },
	/* 0a */	{ "ASL",  1, ACC|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 0c */	{ "TSB",  3, ABS|NORM, 1, },
	/* 0d */	{ "ORA",  3, ABS|NORM, 0, },
	/* 0e */	{ "ASL",  3, ABS|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 10 */	{ "BPL",  2, REL|FORK, 0, },
	/* 11 */	{ "ORA",  2, INY|NORM, 0, },
	/* 12 */	{ "ORA",  2, IND|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 14 */	{ "TRB",  2, ZPG|NORM, 1, },
	/* 15 */	{ "ORA",  2, ZPX|NORM, 0, },
	/* 16 */	{ "ASL",  2, ZPX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 18 */	{ "CLC",  1, IMP|NORM, 0, },
	/* 19 */	{ "ORA",  3, ABY|NORM, 0, },
	/* 1a */	{ "INC",  1, ACC|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 1c */	{ "TRB",  3, ABS|NORM, 1, },
	/* 1d */	{ "ORA",  3, ABX|NORM, 0, },
	/* 1e */	{ "ASL",  3, ABX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 20 */	{ "JSR",  3, ABS|FORK, 0, },
	/* 21 */	{ "AND",  2, INX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 24 */	{ "BIT",  2, ZPG|NORM, 0, },
	/* 25 */	{ "AND",  2, ZPG|NORM, 0, },
	/* 26 */	{ "ROL",  2, ZPG|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 28 */	{ "PLP",  1, IMP|NORM, 0, },
	/* 29 */	{ "AND",  2, IMM|NORM, 0, },
	/* 2a */	{ "ROL",  1, ACC|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 2c */	{ "BIT",  3, ABS|NORM, 0, },
	/* 2d */	{ "AND",  3, ABS|NORM, 0, },
	/* 2e */	{ "ROL",  3, ABS|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 30 */	{ "BMI",  2, REL|FORK, 0, },
	/* 31 */	{ "AND",  2, INY|NORM, 0, },
	/* 32 */	{ "AND",  2, IND|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 34 */	{ "BIT",  2, ZPX|NORM, 1, },
	/* 35 */	{ "AND",  2, ZPX|NORM, 0, },
	/* 36 */	{ "ROL",  2, ZPX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 38 */	{ "SEC",  1, IMP|NORM, 0, },
	/* 39 */	{ "AND",  3, ABY|NORM, 0, },
	/* 3a */	{ "DEC",  1, ACC|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 3c */	{ "BIT",  3, ABX|NORM, 1, },
	/* 3d */	{ "AND",  3, ABX|NORM, 0, },
	/* 3e */	{ "ROL",  3, ABX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 40 */	{ "RTI",  1, IMP|STOP, 0, },
	/* 41 */	{ "EOR",  2, INX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 45 */	{ "EOR",  2, ZPG|NORM, 0, },
	/* 46 */	{ "LSR",  2, ZPG|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 48 */	{ "PHA",  1, IMP|NORM, 0, },
	/* 49 */	{ "EOR",  2, IMM|NORM, 0, },
	/* 4a */	{ "LSR",  1, ACC|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 4c */	{ "JMP",  3, ABS|JUMP, 0, },
	/* 4d */	{ "EOR",  3, ABS|NORM, 0, },
	/* 4e */	{ "LSR",  3, ABS|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 50 */	{ "BVC",  2, REL|FORK, 0, },
	/* 51 */	{ "EOR",  2, INY|NORM, 0, },
	/* 52 */	{ "EOR",  2, IND|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 55 */	{ "EOR",  2, ZPX|NORM, 0, },
	/* 56 */	{ "LSR",  2, ZPX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 58 */	{ "CLI",  1, IMP|NORM, 0, },
	/* 59 */	{ "EOR",  3, ABY|NORM, 0, },
	/* 5a */	{ "PHY",  1, IMP|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 5d */	{ "EOR",  3, ABX|NORM, 0, },
	/* 5e */	{ "LSR",  3, ABX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 60 */	{ "RTS",  1, IMP|STOP, 0, },
	/* 61 */	{ "ADC",  2, INX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 64 */	{ "STZ",  2, ZPG|NORM, 1, },
	/* 65 */	{ "ADC",  2, ZPG|NORM, 0, },
	/* 66 */	{ "ROR",  2, ZPG|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 68 */	{ "PLA",  1, IMP|NORM, 0, },
	/* 69 */	{ "ADC",  2, IMM|NORM, 0, },
	/* 6a */	{ "ROR",  1, ACC|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 6c */	{ "JMP",  3, IND|STOP, 0, },
	/* 6d */	{ "ADC",  3, ABS|NORM, 0, },
	/* 6e */	{ "ROR",  3, ABS|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 70 */	{ "BVS",  2, REL|FORK, 0, },
	/* 71 */	{ "ADC",  2, INY|NORM, 0, },
	/* 72 */	{ "ADC",  2, IND|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 74 */	{ "STZ",  2, ZPX|NORM, 1, },
	/* 75 */	{ "ADC",  2, ZPX|NORM, 0, },
	/* 76 */	{ "ROR",  2, ZPX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 78 */	{ "SEI",  1, IMP|NORM, 0, },
	/* 79 */	{ "ADC",  3, ABY|NORM, 0, },
	/* 7a */	{ "PLY",  1, IMP|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 7c */	{ "JMP",  3, INX|NORM, 1, },
	/* 7d */	{ "ADC",  3, ABX|NORM, 0, },
	/* 7e */	{ "ROR",  3, ABX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 80 */	{ "BRA",  2, REL|FORK, 1, },
	/* 81 */	{ "STA",  2, INX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 84 */	{ "STY",  2, ZPG|NORM, 0, },
	/* 85 */	{ "STA",  2, ZPG|NORM, 0, },
	/* 86 */	{ "STX",  2, ZPG|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 88 */	{ "DEY",  1, IMP|NORM, 0, },
	/* 89 */	{ "BIT",  2, IMM|NORM, 0, },
	/* 8a */	{ "TXA",  1, IMP|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 8c */	{ "STY",  3, ABS|NORM, 0, },
	/* 8d */	{ "STA",  3, ABS|NORM, 0, },
	/* 8e */	{ "STX",  3, ABS|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 90 */	{ "BCC",  2, REL|FORK, 0, },
	/* 91 */	{ "STA",  2, INY|NORM, 0, },
	/* 92 */	{ "STA",  2, IND|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 94 */	{ "STY",  2, ZPX|NORM, 0, },
	/* 95 */	{ "STA",  2, ZPX|NORM, 0, },
	/* 96 */	{ "STX",  2, ZPY|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 98 */	{ "TYA",  1, IMP|NORM, 0, },
	/* 99 */	{ "STA",  3, ABY|NORM, 0, },
	/* 9a */	{ "TXS",  1, IMP|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 9c */	{ "STZ",  3, ABS|NORM, 1, },
	/* 9d */	{ "STA",  3, ABX|NORM, 0, },
	/* 9e */	{ "STZ",  3, ABX|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* a0 */	{ "LDY",  2, IMM|NORM, 0, },
	/* a1 */	{ "LDA",  2, INX|NORM, 0, },
	/* a2 */	{ "LDX",  2, IMM|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* a4 */	{ "LDY",  2, ZPG|NORM, 0, },
	/* a5 */	{ "LDA",  2, ZPG|NORM, 0, },
	/* a6 */	{ "LDX",  2, ZPG|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* a8 */	{ "TAY",  1, IMP|NORM, 0, },
	/* a9 */	{ "LDA",  2, IMM|NORM, 0, },
	/* aa */	{ "TAX",  1, IMP|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* ac */	{ "LDY",  3, ABS|NORM, 0, },
	/* ad */	{ "LDA",  3, ABS|NORM, 0, },
	/* ae */	{ "LDX",  3, ABS|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* b0 */	{ "BCS",  2, REL|FORK, 0, },
	/* b1 */	{ "LDA",  2, INY|NORM, 0, },
	/* b2 */	{ "LDA",  2, IND|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* b4 */	{ "LDY",  2, ZPX|NORM, 0, },
	/* b5 */	{ "LDA",  2, ZPX|NORM, 0, },
	/* b6 */	{ "LDX",  2, ZPY|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* b8 */	{ "CLV",  1, IMP|NORM, 0, },
	/* b9 */	{ "LDA",  3, ABY|NORM, 0, },
	/* ba */	{ "TSX",  1, IMP|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* bc */	{ "LDY",  3, ABX|NORM, 0, },
	/* bd */	{ "LDA",  3, ABX|NORM, 0, },
	/* be */	{ "LDX",  3, ABY|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* c0 */	{ "CPY",  2, IMM|NORM, 0, },
	/* c1 */	{ "CMP",  2, INX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* c4 */	{ "CPY",  2, ZPG|NORM, 0, },
	/* c5 */	{ "CMP",  2, ZPG|NORM, 0, },
	/* c6 */	{ "DEC",  2, ZPG|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* c8 */	{ "INY",  1, IMP|NORM, 0, },
	/* c9 */	{ "CMP",  2, IMM|NORM, 0, },
	/* ca */	{ "DEX",  1, IMP|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* cc */	{ "CPY",  3, ABS|NORM, 0, },
	/* cd */	{ "CMP",  3, ABS|NORM, 0, },
	/* ce */	{ "DEC",  3, ABS|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* d0 */	{ "BNE",  2, REL|FORK, 0, },
	/* d1 */	{ "CMP",  2, INY|NORM, 0, },
	/* d2 */	{ "CMP",  2, IND|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* d5 */	{ "CMP",  2, ZPX|NORM, 0, },
	/* d6 */	{ "DEC",  2, ZPX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* d8 */	{ "CLD",  1, IMP|NORM, 0, },
	/* d9 */	{ "CMP",  3, ABY|NORM, 0, },
	/* da */	{ "PHX",  1, IMP|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* dd */	{ "CMP",  3, ABX|NORM, 0, },
	/* de */	{ "DEC",  3, ABX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* e0 */	{ "CPX",  2, IMM|NORM, 0, },
	/* e1 */	{ "SBC",  2, INX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* e4 */	{ "CPX",  2, ZPG|NORM, 0, },
	/* e5 */	{ "SBC",  2, ZPG|NORM, 0, },
	/* e6 */	{ "INC",  2, ZPG|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* e8 */	{ "INX",  1, IMP|NORM, 0, },
	/* e9 */	{ "SBC",  2, IMM|NORM, 0, },
	/* ea */	{ "NOP",  1, IMP|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* ec */	{ "CPX",  3, ABS|NORM, 0, },
	/* ed */	{ "SBC",  3, ABS|NORM, 0, },
	/* ee */	{ "INC",  3, ABS|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* f0 */	{ "BEQ",  2, REL|FORK, 0, },
	/* f1 */	{ "SBC",  2, INY|NORM, 0, },
	/* f2 */	{ "SBC",  2, IND|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* f5 */	{ "SBC",  2, ZPX|NORM, 0, },
	/* f6 */	{ "INC",  2, ZPX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* f8 */	{ "SED",  1, IMP|NORM, 0, },
	/* f9 */	{ "SBC",  3, ABY|NORM, 0, },
	/* fa */	{ "PLX",  1, IMP|NORM, 1, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, },
	/* fd */	{ "SBC",  3, ABX|NORM, 0, },
	/* fe */	{ "INC",  3, ABX|NORM, 0, },
	/* 00 */	{ "???",  1, ILL|NORM, 0, }
};


int DebugReadMem(int addr)
{
	return WholeRam[addr];
}

int DebugDisassembleInstruction(int addr, char *opstr)
{
	char *s;
	int opcode;
	InstInfo *ip; 
	int operand;
	int l;

	sprintf(opstr, "%04X ", addr);

	opcode = DebugReadMem(addr);
	ip = &optable[opcode];

	s=opstr+strlen(opstr);

	switch (ip->nb) {
		case 1:
			sprintf(s, "%02X        ", DebugReadMem(addr));
			break;
		case 2:
			sprintf(s, "%02X %02X     ", DebugReadMem(addr), DebugReadMem(addr+1));
			break;
		case 3:
			sprintf(s, "%02X %02X %02X  ", DebugReadMem(addr), DebugReadMem(addr+1), DebugReadMem(addr+2));
			break;
	}

	sprintf(opstr + strlen(opstr), "            ");

	// Deal with 65C02 instructions
	if (1)
	{
		sprintf(opstr + strlen(opstr), "%s ", ip->opn);
		addr++;

		switch(ip->nb)
		{
			case 1:
				l = 0;
				break;
			case 2:
				operand = DebugReadMem(addr);
				l = 2;
				break;
			case 3:
				operand = DebugReadMem(addr) + (DebugReadMem(addr+1) << 8);
				l = 4;
				break;
		}

		if (ip->flag & REL)
		{
			if (operand > 127) 
				operand = (~0xff | operand);
			operand = operand + ip->nb + addr - 1;
			l = 4;
		}

		s=opstr+strlen(opstr);

		switch (ip->flag & ADRMASK)
		{
		case IMM:
			sprintf(s, "#%0*X    ", l, operand);
			break;
		case REL:
		case ABS:
		case ZPG:
			sprintf(s, "%0*X     ", l, operand);
			break;
		case IND:
			sprintf(s, "(%0*X)   ", l, operand);
			break;
		case ABX:
		case ZPX:
			sprintf(s, "%0*X,X   ", l, operand);
			break;
		case ABY:
		case ZPY:
			sprintf(s, "%0*X,Y   ", l, operand);
			break;
		case INX:
			sprintf(s, "(%0*X,X) ", l, operand);
			break;
		case INY:
			sprintf(s, "(%0*X),Y ", l, operand);
			break;
		case ACC:
			sprintf(s, "A        ");
			break;
		case IMP:
		default:
			sprintf(s, "         ");
			break;
		}

		if (l == 2)
			sprintf(opstr + strlen(opstr), "  ");
	}
	else
	{
		sprintf(opstr + strlen(opstr), "???          ");
	}

	return(ip->nb);
}
