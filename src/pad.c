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

/*
 * Additional code by  Gameblabla, JamesOFarrel and code based on dfinput for Dual analog/shock. 
*/

#include "psxcommon.h"
#include "psxmem.h"
#include "r3000a.h"

unsigned char CurPad = 0, CurCmd = 0;
unsigned char configmode = 0, padmode = 0;

typedef struct tagGlobalData
{
    uint8_t CurByte1;
    uint8_t CmdLen1;
    uint8_t CurByte2;
    uint8_t CmdLen2;
} GLOBALDATA;

static GLOBALDATA g= {0,0,0,0};

enum {
	CMD_READ_DATA_AND_VIBRATE = 0x42,
	CMD_CONFIG_MODE = 0x43,
	CMD_SET_MODE_AND_LOCK = 0x44,
	CMD_QUERY_MODEL_AND_MODE = 0x45,
	CMD_QUERY_ACT = 0x46, // ??
	CMD_QUERY_COMB = 0x47, // ??
	CMD_QUERY_MODE = 0x4C, // QUERY_MODE ??
	CMD_VIBRATION_TOGGLE = 0x4D,
};

#define PSE_PAD_TYPE_STANDARD		4
#define PSE_PAD_TYPE_ANALOGJOY		5
#define PSE_PAD_TYPE_ANALOGPAD		7

unsigned char PAD1_startPoll(void) {
	g.CurByte1 = 0; return 0xFF;
}

unsigned char PAD2_startPoll(void) {
	g.CurByte2 = 0; return 0xFF;
}

static uint8_t stdmode[8]  = {0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t stdcfg[8]   = {0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t stdmodel[8] = { 
	 0xFF,
	 0x5A,
	 0x01, // 03 - dualshock2, 01 - dualshock
	 0x02, // number of modes
	 0x01, // current mode: 01 - analog, 00 - digital
	 0x02,
	 0x01,
	 0x00
};
static uint8_t unk46[8] =  {0xFF, 0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0A};
static uint8_t unk47[8] = {0xFF, 0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00};
static uint8_t unk4c[8] =  {0xFF, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t unk4d[8] = {0xFF, 0x5A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

unsigned char PAD1_poll(unsigned char value) {
	static uint8_t buf[8] = {0xFF, 0x5A, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80};
	uint32_t size_buf = 8;

	if (g.CurByte1 == 0) {
		uint64_t n;
		CurCmd = value;
		g.CurByte1++;

		n = pad_read(0);
		g.CmdLen1 = 8;

		switch (CurCmd) 
		{
			case CMD_SET_MODE_AND_LOCK:
				memcpy(buf, stdmode,  8);
			return 0xF3;
			case CMD_QUERY_MODEL_AND_MODE:
				memcpy(buf, stdmodel,  8);
				buf[4] = 0x01;
			return 0xF3;
			case CMD_QUERY_ACT:
				memcpy(buf, unk46,  8);
				return 0xF3;

			case CMD_QUERY_COMB:
				memcpy(buf, unk47,  8);
				return 0xF3;

			case CMD_QUERY_MODE:
				memcpy(buf, unk4c,  8);
				return 0xF3;

			case CMD_VIBRATION_TOGGLE:
				memcpy(buf, unk4d,  8);
				return 0xF3;
				
			case CMD_CONFIG_MODE:
			if (configmode) {
				memcpy(buf, stdcfg,  8);
				return 0xF3;
			}
			// else FALLTHROUGH
			case CMD_READ_DATA_AND_VIBRATE:
			default:
			if (buf[0] == 0x41) size_buf = 4;
			for(uint32_t i=0;i<size_buf;i=i+2)
			{
				buf[i] = (n >> ((7-i-1) * 8)) & 0xFF;
				buf[i+1] = (n >> ((7-i) * 8)) & 0xFF;
			}
			break;
		}

		return buf[0];
	}

	if (g.CurByte1 >= g.CmdLen1)
		return 0xFF;
		
	if (g.CurByte1 == 2)
	{
		switch (CurCmd) 
		{
			case CMD_CONFIG_MODE:
				configmode = value;
				break;

			case CMD_SET_MODE_AND_LOCK:
				padmode = value;
				break;

			case CMD_QUERY_ACT:
				switch (value) {
					case 0: // default
						buf[5] = 0x02;
						buf[6] = 0x00;
						buf[7] = 0x0A;
						break;

					case 1: // Param std conf change
						buf[5] = 0x01;
						buf[6] = 0x01;
						buf[7] = 0x14;
						break;
				}
				break;

			case CMD_QUERY_MODE:
				switch (value) {
					case 0: // mode 0 - digital mode
						buf[5] = PSE_PAD_TYPE_STANDARD;
						break;

					case 1: // mode 1 - analog mode
						buf[5] = PSE_PAD_TYPE_ANALOGPAD;
						break;
				}
				break;
		}
	}
		
	return buf[g.CurByte1++];
}

unsigned char PAD2_poll(unsigned char value) {
	static uint8_t buf[8] = {0xFF, 0x5A, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80};

	if (g.CurByte2 == 0) {
		uint64_t n;
		g.CurByte2++;

		n = pad_read(0);
		for(int i=0;i<8;i=i+2)
		{
			buf[i] = (n >> ((7-i-1) * 8)) & 0xFF;
			buf[i+1] = (n >> ((7-i) * 8)) & 0xFF;
		}
		g.CmdLen2 = 8;
		return buf[0];
	}

	if (g.CurByte2 >= g.CmdLen2) return 0xFF;
	return buf[g.CurByte2++];
}
