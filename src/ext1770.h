/****************************************************************
BeebEm - BBC Micro and Master 128 Emulator
Copyright (C) 2001  Richard Gellman

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
// Opus DDFS Board Drive Controller Chip DLL
// (C) September 2001 - Richard Gellman

struct DriveControlBlock {
	int FDCAddress; // 1770 FDC chip address
	int DCAddress; // Drive Control Register Address
	char *BoardName; // FDC Board name
	bool TR00_ActiveHigh; // Set true if the TR00 input is Active High
};

#define EDCB struct DriveControlBlock
