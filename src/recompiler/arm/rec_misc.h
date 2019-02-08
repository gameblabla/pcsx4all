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

// CHUI: Se podria poner en linea
static void testSWInts(psxRegisters *regs, int _branch) {
	// the next code is untested, if u know please
	// tell me if it works ok or not (linuzappz)
	if (regs->CP0.n.Cause & regs->CP0.n.Status & 0x0300 &&
		regs->CP0.n.Status & 0x1) {
		psxException(regs->CP0.n.Cause, _branch);
	}
}

//REC_SYS(SYSCALL);
static void recSYSCALL() {
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\trecSYSCALL");
	}
#endif	
//	dump = 1;
	iLockReg(3);
	iFlushRegs();

	MOV32ItoR(HOST_r0, pc - 4);
	MOV32RtoM_regs((u32)&psxRegs.pc, HOST_r0);
	ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
	cycles_pending=0;
	MOV32ItoR(HOST_r1,(branch == 1 ? 1 : 0));
	MOV32ItoR(HOST_r0,0x20);
	CALLFunc ((u32)psxException);
// CHUI: Realmente es necesario salir siempre del entorno recompilado?
	ExitPChange();
	iUnlockReg(3);
	iClearRegs();
	pcold=pc;
	r2_is_dirty=1;
}

//REC_SYS(BREAK);
static void recBREAK() {
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\trecBREAK");
	}
#endif
}

//REC_FUNC(MFHI);
static void recMFHI() {
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\trecMFHI");
	}
#endif
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
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\trecMTHI");
	}
#endif
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
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\trecMFLO");
	}
#endif
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
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\trecMTLO");
	}
#endif
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
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\trecMFC0");
	}
#endif
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
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\trecMTC0");
	}
#endif
	if (IsConst(_Rt_)) {
		if (_Rd_==13) {
			MOV32ItoM_regs((u32)&psxRegs.CP0.r[_Rd_], iRegs[_Rt_].k & ~(0xfc00));
		} else {
			MOV32ItoM_regs((u32)&psxRegs.CP0.r[_Rd_], iRegs[_Rt_].k);
		}
	} else {
		u32 rt=ReadReg(_Rt_);
		if (_Rd_==13) {
			MOV32RtoR(HOST_r0, rt);
			AND32ItoR(HOST_r0, ~(0xfc00));
			MOV32RtoM_regs((u32)&psxRegs.CP0.r[_Rd_], HOST_r0);
		} else {
			MOV32RtoM_regs((u32)&psxRegs.CP0.r[_Rd_], rt);
		}
	}

	if (_Rd_ == 12 || _Rd_ == 13) {
		iLockReg(3);
		iUpdateRegs(1);
		MOV32ItoM_regs((u32)&psxRegs.pc, (u32)pc);
		ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
		cycles_pending=0;
		MOV32ItoR(HOST_r1,branch);
		MOV32RtoR(HOST_r0,HOST_r11);
		CALLFunc((u32)testSWInts);
// CHUI: Realmente es necesario salir siempre del entorno recompilado?
		if (branch == 0) {
			ExitPChange();
		}
		iUnlockReg(3);
		r2_is_dirty=1;
		iClearRegs();
		pcold=pc;
	}
}

//REC_SYS(CTC0);
static void recCTC0() {
// Cop0->Rd = Rt
	recMTC0();
}

//REC_FUNC(RFE);
static void recRFE() {
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\trecRFE");
	}
#endif
	MOV32MtoR_regs(HOST_ip,&psxRegs.CP0.n.Status);
	write32(0xe20c203c); //	and	r2, ip, #60	; 0x3c
	write32(0xe3cc100f); // bic	r1, ip, #15	; 0xf
	write32(ORR_REG_LSR_IMM(HOST_r2,HOST_r1,HOST_r2,2));
	MOV32RtoM_regs(&psxRegs.CP0.n.Status,HOST_r2);
// CHUI: Añado ResetIoCycle para permite que en el proximo salto entre en psxBranchTest
#ifdef REC_USE_R2
	MOV32ItoR(HOST_r2,0);
	MOV32RtoM_regs(&psxRegs.io_cycle_counter,HOST_r2);
	r2_is_dirty=0;
#else
	MOV32ItoM_regs(&psxRegs.io_cycle_counter,0);
#endif

	iLockReg(3);
	iUpdateRegs(1);
	MOV32ItoM_regs((u32)&psxRegs.pc, (u32)pc);
	ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
	cycles_pending=0;
	MOV32ItoR(HOST_r1,branch);
	MOV32RtoR(HOST_r0,HOST_r11);
	CALLFunc((u32)testSWInts);
// CHUI: Realmente es necesario salir siempre del entorno recompilado?
	if (branch == 0) {
		ExitPChange();
	}
	iUnlockReg(3);
	r2_is_dirty=1;
	iClearRegs();
	pcold=pc;
}

static void recHLE() {
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecHLE(%.4X)\n",psxRegs.code&0xffff);
	}
#endif
// CHUI: Disabled for security reasons
#if 0
	if (!block && IsConst(31) && !branch) {
		iLockReg(3);
		UpdateGteDelay(1);
		iUpdateRegs(1);
		MOV32ItoM_regs((u32)&psxRegs.pc, pc);
		ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
		cycles_pending=0;
		CALLFunc((u32)psxHLEt[psxRegs.code & 0xffff]);
		MOV32ItoR(HOST_r1,iRegs[31].k);
		MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
		write32(CMP_REGS(HOST_r0,HOST_r1));
#ifndef REC_USE_RETURN_FUNCS
		j32Ptr[5]=armPtr; write32(BEQ_FWD(0));
		RET();
		armSetJ32(j32Ptr[5]);
#else
		JUMPFuncNE(func_Return_ptr+4);
#endif
		pc=iRegs[31].k;
		iUnlockReg(3);
		r2_is_dirty=1;
		iClearRegs();
		pcold=pc;
		rec_skips=rec_total_opcodes;
	} else
#endif
	{
		iFlushRegs();
		UpdateGteDelay(1);
		MOV32ItoM_regs((u32)&psxRegs.pc, pc);
		ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
		cycles_pending=0;
		CALLFunc((u32)psxHLEt[psxRegs.code & 0xffff]);
		branch = 2;
		RET();
	}
}
