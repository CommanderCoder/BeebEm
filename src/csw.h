/****************************************************************
BeebEm - BBC Micro and Master 128 Emulator
Copyright (C) 2006  Jon Welch

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

/*
 *  csw.h
 *  BeebEm3
 *
 *  Created by Jon Welch on 27/08/2006.
 *
 */

 enum class CSWResult {
     Success,
     OpenFailed,
     InvalidCSWFile,
     InvalidHeaderExtension
 };

#define BUFFER_LEN	256

CSWResult LoadCSW(const char *file);
void CloseCSW(void);
int csw_data(void);
int csw_poll(int clock);
void map_csw_file(void);

enum class CSWState {
    WaitingForTone,
    Tone,
    Data,
    Undefined
};

extern CSWState csw_state;

extern int csw_bit;
extern int csw_pulselen;
extern int csw_ptr;
extern unsigned long csw_bufflen;
extern int csw_pulsecount;
extern bool CSWOpen;
extern int CSW_BUF;
extern int CSW_CYCLES;

