/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Franxis                                            *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

#include "psxcommon.h"
#include "psxmem.h"
#include "r3000a.h"

typedef struct tagGlobalData
{
    uint8_t CurByte1;
    uint8_t CmdLen1;
    uint8_t CurByte2;
    uint8_t CmdLen2;
} GLOBALDATA;

static GLOBALDATA g={0,0,0,0};

unsigned char PAD1_startPoll(void) {
	g.CurByte1 = 0; return 0xFF;
}

unsigned char PAD2_startPoll(void) {
	g.CurByte2 = 0; return 0xFF;
}

unsigned char PAD1_poll(void) {
	static uint8_t		buf[8] = {0xFF, 0x5A, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80};

	if (g.CurByte1 == 0) {
		uint16_t			n;
		g.CurByte1++;

		n = pad_read(0);

		buf[2] = n & 0xFF;
		buf[3] = n >> 8;

		g.CmdLen1 = 4;

		return 0x41;
	}

	if (g.CurByte1 >= g.CmdLen1) return 0xFF;
	return buf[g.CurByte1++];
}

unsigned char PAD2_poll(void) {
	static uint8_t		buf[8] = {0xFF, 0x5A, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80};

	if (g.CurByte2 == 0) {
		uint16_t			n;
		g.CurByte2++;

		n = pad_read(1);

		buf[2] = n & 0xFF;
		buf[3] = n >> 8;

		g.CmdLen2 = 4;

		return 0x41;
	}

	if (g.CurByte2 >= g.CmdLen2) return 0xFF;
	return buf[g.CurByte2++];
}
