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

//REC_BRANCH(J);
static void recJ() {
// j target

	iJump(_Target_ * 4 + (pc & 0xf0000000));
}

//REC_BRANCH(JR);
static void recJR() {
// jr Rs

	if (IsConst(_Rs_)) {
		MOV32ItoM((u32)&target, iRegs[_Rs_].k);
	} else {
		u32 rs=ReadReg(_Rs_);
		MOV32RtoM((u32)&target, rs);
	}

	SetBranch();
}

//REC_BRANCH(JAL);
static void recJAL() {
// jal target

	MapConst(31, pc + 4);

	iJump(_Target_ * 4 + (pc & 0xf0000000));
}

//REC_BRANCH(JALR);
static void recJALR() {
// jalr Rs

	if (IsConst(_Rs_)) {
		MOV32ItoM((u32)&target, iRegs[_Rs_].k);
	} else {
		u32 rs=ReadReg(_Rs_);
		MOV32RtoM((u32)&target, rs);
	}

	if (_Rd_) {
		MapConst(_Rd_, pc + 4);
	}
	
	SetBranch();
}

//REC_BRANCH(BLTZ);
static void recBLTZ() {
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k < 0) {
			iJump(bpc);
			return;
		} else {
			iJump(pc + 4);
			return;
		}
	}

	u32 rs=ReadReg(_Rs_);
	j32Ptr[4] = JLTZ32(rs);

	iBranch(pc+4, 1);

	armSetJ32(j32Ptr[4]);

	iBranch(bpc, 0);
	pc += 4;
}


//REC_BRANCH(BGTZ);
static void recBGTZ() {
// Branch if Rs > 0
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc + 4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k > 0) {
			iJump(bpc);
			return;
		} else {
			iJump(pc + 4);
			return;
		}
	}

	u32 rs=ReadReg(_Rs_);
	j32Ptr[4] = JGTZ32(rs);

	iBranch(pc + 4, 1);

	armSetJ32(j32Ptr[4]);

	iBranch(bpc, 0);
	pc+=4;
}

//REC_BRANCH(BLTZAL);
static void recBLTZAL() {
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc + 4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k < 0) {
			u32 r31=WriteReg(31);
			MOV32ItoR(r31, pc + 4);
			iJump(bpc); return;
		} else {
			iJump(pc + 4); return;
		}
	}

	u32 rs=ReadReg(_Rs_);
	j32Ptr[4] = JLTZ32(rs);

	iBranch(pc + 4, 1);

	armSetJ32(j32Ptr[4]);
	
	u32 r31=WriteReg(31);
	MOV32ItoR(r31, pc + 4);
	iBranch(bpc, 0);
	pc += 4;
}

//REC_BRANCH(BGEZAL);
static void recBGEZAL() {
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc + 4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k >= 0) {
			u32 r31=WriteReg(31);
			MOV32ItoR(r31, pc + 4);
			iJump(bpc); return;
		} else {
			iJump(pc+4); return;
		}
	}

	u32 rs=ReadReg(_Rs_);
	j32Ptr[4] = JGEZ32(rs);

	iBranch(pc+4, 1);

	armSetJ32(j32Ptr[4]);

	u32 r31=WriteReg(31);
	MOV32ItoR(r31, pc + 4);
	iBranch(bpc, 0);
	pc+=4;
}

//REC_BRANCH(BNE);
static void recBNE() {
// Branch if Rs != Rt
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc + 4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		if (iRegs[_Rs_].k != iRegs[_Rt_].k) {
			iJump(bpc);
			return;
		} else {
			iJump(pc + 4);
			return;
		}
	}

	u32 rs,rt;
	
	if (IsConst(_Rs_)) {
		MOV32ItoR(HOST_a1,iRegs[_Rs_].k);
		rs=HOST_a1;
	} else {
		rs=ReadReg(_Rs_);
	}
	
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_a2,iRegs[_Rt_].k);
		rt=HOST_a2;
	} else {
		rt=ReadReg(_Rt_);
	}

	write32(CMP_REGS(rs,rt));
	j32Ptr[4] = armPtr; write32(BNE_FWD(0));
	
	iBranch(pc + 4, 1);

	armSetJ32(j32Ptr[4]);

	iBranch(bpc, 0);
	pc += 4;
}

//REC_BRANCH(BEQ);
static void recBEQ() {
// Branch if Rs == Rt
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc + 4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (_Rs_ == _Rt_) {
		iJump(bpc);
	} else {
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			if (iRegs[_Rs_].k == iRegs[_Rt_].k) {
				iJump(bpc);
				return;
			} else {
				iJump(pc + 4);
				return;
			}
		}

		u32 rs,rt;

		if (IsConst(_Rs_)) {
			MOV32ItoR(HOST_a1,iRegs[_Rs_].k);
			rs=HOST_a1;
		} else {
			rs=ReadReg(_Rs_);
		}
		
		if (IsConst(_Rt_)) {
			MOV32ItoR(HOST_a2,iRegs[_Rt_].k);
			rt=HOST_a2;
		} else {
			rt=ReadReg(_Rt_);
		}
		
		write32(CMP_REGS(rs,rt));
		j32Ptr[4] = armPtr; write32(BEQ_FWD(0));

		iBranch(pc + 4, 1);

		armSetJ32(j32Ptr[4]);

		iBranch(bpc, 0);
		pc += 4;
	}
}

//REC_BRANCH(BLEZ);
static void recBLEZ() {
// Branch if Rs <= 0
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc + 4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k <= 0) {
			iJump(bpc);
			return;
		} else {
			iJump(pc + 4);
			return;
		}
	}

	u32 rs=ReadReg(_Rs_);
	j32Ptr[4] = JLEZ32(rs);

	iBranch(pc + 4, 1);

	armSetJ32(j32Ptr[4]);

	iBranch(bpc, 0);
	pc += 4;
}

//REC_BRANCH(BGEZ);
static void recBGEZ() {
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc + 4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k >= 0) {
			iJump(bpc);
			return;
		} else {
			iJump(pc + 4);
			return;
		}
	}

	u32 rs=ReadReg(_Rs_);
	j32Ptr[4] = JGEZ32(rs);

	iBranch(pc + 4, 1);

	armSetJ32(j32Ptr[4]);

	iBranch(bpc, 0);
	pc += 4;
}
