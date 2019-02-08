/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Franxis and Chui                                   *
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

#include "gte.h"

// July 2017: Some GTE instructions encode parameters inside their u32 opcode.
//            The funcs for these instructions now take the opcode value as
//            parameter shifted right 10 places (only lower 16 bits are used).
//            The ARM dynarecs are not maintained and haven't been updated:
#error "ARM dynarec has not been updated to match new GTE interface"


#define CP2_FUNC(f) \
void gte##f(); \
static void rec##f() { \
	/*iFlushRegs();*/ \
	MOV32ItoM_regs((u32)&psxRegs.code, (u32)psxRegs.code); \
	CALLFunc ((u32)gte##f); \
/*	branch = 2; */\
}

#define CP2_FUNCNC(f) \
void gte##f(); \
static void rec##f() { \
	/*iFlushRegs();*/ \
	CALLFunc ((u32)gte##f); \
/*	branch = 2; */\
}

//CP2_FUNC(MFC2);
static void recMFC2(void) {
	if (_Rt_)
	{
		MOV32ItoR(HOST_a1,_Rd_);
		CALLFunc((u32)gtecalcMFC2);
		u32 rt=WriteReg(_Rt_);
		MOV32RtoR(rt,HOST_a1);
	}
}

//CP2_FUNC(MTC2);
static void recMTC2(void) {
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_a1,iRegs[_Rt_].k);
	}
	else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_a1,rt);
	}
	MOV32ItoR(HOST_a2,_Rd_);
	CALLFunc((u32)gtecalcMTC2);
}

//CP2_FUNC(CFC2);
static void recCFC2(void) {
	if (_Rt_)
	{
		u32 rt=WriteReg(_Rt_);
		MOV32MtoR_regs(rt,&psxRegs.CP2C.r[_Rd_]);
	}
}

//CP2_FUNC(CTC2);
static void recCTC2(void) {
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_a1,iRegs[_Rt_].k);
	}
	else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_a1,rt);
	}
	MOV32ItoR(HOST_a2,_Rd_);
	CALLFunc((u32)gtecalcCTC2);
}

//CP2_FUNC(LWC2);
static void recLWC2(void)
{
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0) == 0xbfc0) {
			MOV32ItoR(HOST_a1, psxRu32(addr)); // since bios is readonly it won't change
			MOV32ItoR(HOST_a2, _Rt_);
			CALLFunc((u32)gtecalcMTC2);
			return;
		}
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			MOV32MtoR(HOST_a1, (u32)&psxM[addr & 0x1fffff]);
			MOV32ItoR(HOST_a2, _Rt_);
			CALLFunc((u32)gtecalcMTC2);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			MOV32MtoR(HOST_a1, (u32)&psxH[addr & 0xfff]);
			MOV32ItoR(HOST_a2, _Rt_);
			CALLFunc((u32)gtecalcMTC2);
			return;
		}
		if (t == 0x1f80) {
			switch (addr) {
				case 0x1f801080: case 0x1f801084: case 0x1f801088: 
				case 0x1f801090: case 0x1f801094: case 0x1f801098: 
				case 0x1f8010a0: case 0x1f8010a4: case 0x1f8010a8: 
				case 0x1f8010b0: case 0x1f8010b4: case 0x1f8010b8: 
				case 0x1f8010c0: case 0x1f8010c4: case 0x1f8010c8: 
				case 0x1f8010d0: case 0x1f8010d4: case 0x1f8010d8: 
				case 0x1f8010e0: case 0x1f8010e4: case 0x1f8010e8: 
				case 0x1f801070: case 0x1f801074:
				case 0x1f8010f0: case 0x1f8010f4:
					MOV32MtoR(HOST_a1, (u32)&psxH[addr & 0xffff]);
					MOV32ItoR(HOST_a2, _Rt_);
					CALLFunc((u32)gtecalcMTC2);
					return;

				case 0x1f801810:
					CALLFunc((u32)&GPU_readData);
					MOV32ItoR(HOST_a2, _Rt_);
					CALLFunc((u32)gtecalcMTC2);
					return;

				case 0x1f801814:
					CALLFunc((u32)&GPU_readStatus);
					MOV32ItoR(HOST_a2, _Rt_);
					CALLFunc((u32)gtecalcMTC2);
					return;
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			MOV32ItoR(HOST_a1, addr);
			CALLFunc((u32)psxHwRead32);
			MOV32ItoR(HOST_a2, _Rt_);
			CALLFunc((u32)gtecalcMTC2);
			return;
		}
	}

	iPushOfB();
	PSXMEMREAD32(); //CALLFunc((u32)psxMemRead32);
	MOV32ItoR(HOST_a2, _Rt_);
	CALLFunc((u32)gtecalcMTC2);
}

//CP2_FUNC(SWC2);
static void recSWC2(void)
{
	MOV32ItoR(HOST_a1,_Rt_);
	CALLFunc((u32)gtecalcMFC2);

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			MOV32RtoM((u32)&psxM[addr & 0x1fffff], HOST_a1);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			MOV32RtoM((u32)&psxH[addr & 0xfff], HOST_a1);
			return;
		}
		if (t == 0x1f80) {
			switch (addr) {
				case 0x1f801080: case 0x1f801084: 
				case 0x1f801090: case 0x1f801094: 
				case 0x1f8010a0: case 0x1f8010a4: 
				case 0x1f8010b0: case 0x1f8010b4: 
				case 0x1f8010c0: case 0x1f8010c4: 
				case 0x1f8010d0: case 0x1f8010d4: 
				case 0x1f8010e0: case 0x1f8010e4: 
				case 0x1f801074:
				case 0x1f8010f0:
					MOV32RtoM((u32)&psxH[addr & 0xffff], HOST_a1);
					return;

				case 0x1f801810:
					CALLFunc((u32)&GPU_writeData);
					return;

				case 0x1f801814:
					CALLFunc((u32)&GPU_writeStatus);
					return; // ???
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			MOV32RtoR(HOST_a2,HOST_a1);
			MOV32ItoR(HOST_a1,addr);
			CALLFunc((u32)psxHwWrite32);
			return;
		}
	}

	MOV32RtoR(HOST_a2,HOST_a1);
	iPushOfB();
	PSXMEMWRITE32(); // CALLFunc((u32)recMemWrite32);
}

CP2_FUNCNC(RTPS);
CP2_FUNC(OP);
CP2_FUNCNC(NCLIP);
CP2_FUNC(DPCS);
CP2_FUNC(INTPL);
CP2_FUNC(MVMVA);
CP2_FUNCNC(NCDS);
CP2_FUNCNC(NCDT);
CP2_FUNCNC(CDP);
CP2_FUNCNC(NCCS);
CP2_FUNCNC(CC);
CP2_FUNCNC(NCS);
CP2_FUNCNC(NCT);
CP2_FUNC(SQR);
CP2_FUNC(DCPL);
CP2_FUNCNC(DPCT);
CP2_FUNCNC(AVSZ3);
CP2_FUNCNC(AVSZ4);
CP2_FUNCNC(RTPT);
CP2_FUNC(GPF);
CP2_FUNC(GPL);
CP2_FUNCNC(NCCT);
