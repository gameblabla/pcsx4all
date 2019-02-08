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

//REC_SYS(SYSCALL);
static void recSYSCALL() {
//	dump = 1;
	iFlushRegs();

	MOV32ItoR(HOST_a1, pc - 4);
	MOV32RtoM_regs((u32)&psxRegs.pc, HOST_a1);
	MOV32ItoR(HOST_a2,(branch == 1 ? 1 : 0));
	MOV32ItoR(HOST_a1,0x20);
	CALLFunc ((u32)psxException);
	branch = 2;
	iRet();
}

//REC_SYS(BREAK);
static void recBREAK() {
}

//REC_FUNC(MFHI);
static void recMFHI() {
// Rd = Hi
	if (_Rd_)
	{
		if (IsConst(33)) { // psxRegs.GPR.n.hi
			MapConst(_Rd_, iRegs[33].k);
		} else {
			u32 hi=ReadReg(33); // psxRegs.GPR.n.hi
			u32 rd=WriteReg(_Rd_);
			MOV32RtoR(rd,hi);
		}
	}
}

//REC_FUNC(MTHI);
static void recMTHI() {
// Hi = Rs
	if (IsConst(_Rs_)) {
		MapConst(33, iRegs[_Rs_].k); // psxRegs.GPR.n.hi
	} else {
		u32 rs=ReadReg(_Rs_);
		u32 hi=WriteReg(33); // psxRegs.GPR.n.hi
		MOV32RtoR(hi,rs);
	}
}

//REC_FUNC(MFLO);
static void recMFLO() {
// Rd = Lo
	if (_Rd_)
	{
		if (IsConst(32)) { // psxRegs.GPR.n.lo
			MapConst(_Rd_, iRegs[32].k);
		} else {
			u32 lo=ReadReg(32); // psxRegs.GPR.n.lo
			u32 rd=WriteReg(_Rd_);
			MOV32RtoR(rd,lo);
		}
	}
}

//REC_FUNC(MTLO);
static void recMTLO() {
// Lo = Rs
	if (IsConst(_Rs_)) {
		MapConst(32, iRegs[_Rs_].k); // psxRegs.GPR.n.lo
	} else {
		u32 rs=ReadReg(_Rs_);
		u32 lo=WriteReg(32); // psxRegs.GPR.n.lo
		MOV32RtoR(lo,rs);
	}
}

//REC_FUNC(MFC0);
static void recMFC0() {
// Rt = Cop0->Rd
	if (_Rt_)
	{
		u32 rt=WriteReg(_Rt_);
		MOV32MtoR_regs(rt, (u32)&psxRegs.CP0.r[_Rd_]);
	}
}

//REC_FUNC(CFC0);
static void recCFC0() {
// Rt = Cop0->Rd

	recMFC0();
}

//REC_SYS(MTC0);
static void recMTC0() {
// Cop0->Rd = Rt

	if (IsConst(_Rt_)) {
		if (_Rd_==13)
		{
			MOV32ItoM_regs((u32)&psxRegs.CP0.r[_Rd_], iRegs[_Rt_].k & ~(0xfc00));
		}
		else
		{
			MOV32ItoM_regs((u32)&psxRegs.CP0.r[_Rd_], iRegs[_Rt_].k);
		}
	} else {
		u32 rt=ReadReg(_Rt_);
		if (_Rd_==13)
		{
			MOV32RtoR(HOST_a1, rt);
			AND32ItoR(HOST_a1, ~(0xfc00));
			MOV32RtoM_regs((u32)&psxRegs.CP0.r[_Rd_], HOST_a1);
		}
		else
		{
			MOV32RtoM_regs((u32)&psxRegs.CP0.r[_Rd_], rt);
		}
	}

	if (_Rd_ == 12 || _Rd_ == 13) {
		iFlushRegs();
		MOV32ItoM_regs((u32)&psxRegs.pc, (u32)pc);
		CALLFunc((u32)psxTestSWInts);
		if (branch == 0) {
			branch = 2;
			iRet();
		}
	}
}

//REC_SYS(CTC0);
static void recCTC0() {
// Cop0->Rd = Rt

	recMTC0();
}

//REC_FUNC(RFE);
static void recRFE() {

	MOV32MtoR_regs(HOST_ip,&psxRegs.CP0.n.Status);
	write32(0xe20c203c); //	and	r2, ip, #60	; 0x3c
    write32(0xe3cc100f); // bic	r1, ip, #15	; 0xf
    write32(0xe1813122); // orr	r3, r1, r2, lsr #2
	MOV32RtoM_regs(&psxRegs.CP0.n.Status,HOST_a4);

	iFlushRegs();
	MOV32ItoM_regs((u32)&psxRegs.pc, (u32)pc);
	CALLFunc((u32)psxTestSWInts);
	if (branch == 0) {
		branch = 2;
		iRet();
	}
}

static void recHLE() {
	iFlushRegs();
	CALLFunc((u32)psxHLEt[psxRegs.code & 0xffff]);
	branch = 2;
	iRet();
}
