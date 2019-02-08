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

//REC_FUNC(ADDIU);
static void recADDIU()  {
// Rt = Rs + Im
	if (_Rt_)
	{
		if (_Rs_ == _Rt_) {
			if (IsConst(_Rt_)) {
				iRegs[_Rt_].k+= _Imm_;
			} else {
				if (_Imm_) {
					u32 rt=ReadWriteReg(_Rt_);
					ADD32ItoR(rt, _Imm_);
				}
			}
		} else {
			if (IsConst(_Rs_)) {
				MapConst(_Rt_, iRegs[_Rs_].k + _Imm_);
			} else {
				u32 rs=ReadReg(_Rs_);
				u32 rt=WriteReg(_Rt_);
				MOV32RtoR(rt,rs);
				if (_Imm_) {
					ADD32ItoR(rt,_Imm_);
				}
			}
		}
	}
}

//REC_FUNC(ADDI);
static void recADDI()  {
// Rt = Rs + Im
	recADDIU();
}

//REC_FUNC(ANDI);
static void recANDI() {
// Rt = Rs And Im
	if (_Rt_)
	{
		if (_Rs_ == _Rt_) {
			if (IsConst(_Rt_)) {
				iRegs[_Rt_].k&= _ImmU_;
			} else {
				u32 rt=ReadWriteReg(_Rt_);
				AND32ItoR(rt, _ImmU_);
			}
		} else {
			if (IsConst(_Rs_)) {
				MapConst(_Rt_, iRegs[_Rs_].k & _ImmU_);
			} else {
				u32 rs=ReadReg(_Rs_);
				u32 rt=WriteReg(_Rt_);
				MOV32RtoR(rt,rs);
				AND32ItoR(rt,_ImmU_);
			}
		}
	}
}

//REC_FUNC(ORI);
static void recORI() {
// Rt = Rs Or Im
	if (_Rt_)
	{
		if (_Rs_ == _Rt_) {
			if (IsConst(_Rt_)) {
				iRegs[_Rt_].k|= _ImmU_;
			} else {
				u32 rt=ReadWriteReg(_Rt_);
				MOV32ItoR(HOST_a1,_ImmU_);
				OR32(rt,rt,HOST_a1);
			}
		} else {
			if (IsConst(_Rs_)) {
				MapConst(_Rt_, iRegs[_Rs_].k | _ImmU_);
			} else {
				u32 rs=ReadReg(_Rs_);
				u32 rt=WriteReg(_Rt_);
				MOV32RtoR(rt,rs);
				if (_ImmU_)
				{
					MOV32ItoR(HOST_a1,_ImmU_);
					OR32(rt,rt,HOST_a1);
				}
			}
		}
	}
}

//REC_FUNC(XORI);
static void recXORI() {
// Rt = Rs Xor Im
	if (_Rt_)
	{
		if (_Rs_ == _Rt_) {
			if (IsConst(_Rt_)) {
				iRegs[_Rt_].k^= _ImmU_;
			} else {
				u32 rt=ReadWriteReg(_Rt_);
				MOV32ItoR(HOST_a1,_ImmU_);
				XOR32(rt,rt,HOST_a1);
			}
		} else {
			if (IsConst(_Rs_)) {
				MapConst(_Rt_, iRegs[_Rs_].k ^ _ImmU_);
			} else {
				u32 rs=ReadReg(_Rs_);
				MOV32ItoR(HOST_a1, _ImmU_);
				u32 rt=WriteReg(_Rt_);
				XOR32(rt,rs,HOST_a1);
			}
		}
	}
}

#define SLT(dst,op1,op2) write32(CMP_REGS(op1,op2));write32(MOVGE_IMM(dst, 0, 0));write32(MOVLT_IMM(dst, 1, 0))
#define SLTU(dst,op1,op2) write32(CMP_REGS(op1,op2));write32(MOVCS_IMM(dst, 0, 0));write32(MOVCC_IMM(dst, 1, 0))

//REC_FUNC(SLTI);
static void recSLTI() {
// Rt = Rs < Im (signed)
	if (_Rt_)
	{
		if (IsConst(_Rs_)) {
			MapConst(_Rt_, (s32)iRegs[_Rs_].k < _Imm_);
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=WriteReg(_Rt_);
			MOV32ItoR(HOST_a2, _Imm_);
			SLT(rt,rs,HOST_a2);
		}
	}
}

//REC_FUNC(SLTIU);
static void recSLTIU() {
// Rt = Rs < Im (unsigned)
	if (_Rt_)
	{
		if (IsConst(_Rs_)) {
			MapConst(_Rt_, iRegs[_Rs_].k < _ImmU_);
		} else {
			u32 rs=ReadReg(_Rs_);
			MOV32ItoR(HOST_a2, _Imm_);
			u32 rt=WriteReg(_Rt_);
			SLTU(rt,rs,HOST_a2);
		}
	}
}

//REC_FUNC(LUI);
static void recLUI()  {
// Rt = Imm << 16
	if (_Rt_) MapConst(_Rt_, psxRegs.code << 16);
}

//REC_FUNC(ADDU);
static void recADDU() {
// Rd = Rs + Rt 
	if (_Rd_)
	{
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			MapConst(_Rd_, iRegs[_Rs_].k + iRegs[_Rt_].k);
		} else if (IsConst(_Rs_)) {
			if (_Rt_ == _Rd_) {
				if (iRegs[_Rs_].k) {
					u32 rd=ReadWriteReg(_Rd_);
					ADD32ItoR(rd, iRegs[_Rs_].k);
				}
			} else {
				u32 rt=ReadReg(_Rt_);
				u32 rd=WriteReg(_Rd_);
				MOV32RtoR(rd,rt);
				if (iRegs[_Rs_].k) {
					ADD32ItoR(rd, iRegs[_Rs_].k);
				}
			}
		} else if (IsConst(_Rt_)) {
			if (_Rs_ == _Rd_) {
				if (iRegs[_Rt_].k) {
					u32 rd=ReadWriteReg(_Rd_);
					ADD32ItoR(rd, iRegs[_Rt_].k);
				}
			} else {
				u32 rs=ReadReg(_Rs_);
				u32 rd=WriteReg(_Rd_);
				MOV32RtoR(rd,rs);
				if (iRegs[_Rt_].k) {
					ADD32ItoR(rd, iRegs[_Rt_].k);
				}
			}
		} else {
			if (_Rs_ == _Rd_) { // Rd+= Rt
				u32 rt=ReadReg(_Rt_);
				u32 rd=ReadWriteReg(_Rd_);
				ADD32(rd,rd,rt);
			} else if (_Rt_ == _Rd_) { // Rd+= Rs
				u32 rs=ReadReg(_Rs_);
				u32 rd=ReadWriteReg(_Rd_);
				ADD32(rd,rd,rs);
			} else { // Rd = Rs + Rt
				u32 rs=ReadReg(_Rs_);
				u32 rt=ReadReg(_Rt_);
				u32 rd=WriteReg(_Rd_);
				ADD32(rd,rs,rt);
			}
		}
	}
}

//REC_FUNC(ADD);
static void recADD() {
// Rd = Rs + Rt
	recADDU();
}

//REC_FUNC(SUBU);
static void recSUBU() {
// Rd = Rs - Rt
	if (_Rd_)
	{
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			MapConst(_Rd_, iRegs[_Rs_].k - iRegs[_Rt_].k);
		} else if (IsConst(_Rs_)) {
			MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SUB32(rd,HOST_a1,rt);
		} else if (IsConst(_Rt_)) {
			u32 rs=ReadReg(_Rs_);
			MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
			u32 rd=WriteReg(_Rd_);
			SUB32(rd,rs,HOST_a2);
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SUB32(rd,rs,rt);
		}
	}
}   

//REC_FUNC(SUB);
static void recSUB() {
// Rd = Rs - Rt
	recSUBU();
}   

//REC_FUNC(AND);
static void recAND() {
// Rd = Rs And Rt
	if (_Rd_)
	{
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			MapConst(_Rd_, iRegs[_Rs_].k & iRegs[_Rt_].k);
		} else if (IsConst(_Rs_)) {
			if (_Rd_ == _Rt_) { // Rd&= Rs
				u32 rd=ReadWriteReg(_Rd_);
				AND32ItoR(rd,iRegs[_Rs_].k);
			} else {
				MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
				u32 rt=ReadReg(_Rt_);
				u32 rd=WriteReg(_Rd_);
				AND32(rd,HOST_a1,rt);
			}
		} else if (IsConst(_Rt_)) {
			if (_Rd_ == _Rs_) { // Rd&= kRt
				u32 rd=ReadWriteReg(_Rd_);
				AND32ItoR(rd,iRegs[_Rt_].k);
			} else { // Rd = Rs & kRt
				u32 rs=ReadReg(_Rs_);
				MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
				u32 rd=WriteReg(_Rd_);
				AND32(rd,rs,HOST_a2);
			}
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			AND32(rd,rs,rt);
		}
	}
}   

//REC_FUNC(OR);
static void recOR() {
// Rd = Rs Or Rt
	if (_Rd_)
	{
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			MapConst(_Rd_, iRegs[_Rs_].k | iRegs[_Rt_].k);
		} else if (IsConst(_Rs_)) {
			MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			OR32(rd,HOST_a1,rt);
		} else if (IsConst(_Rt_)) {
			u32 rs=ReadReg(_Rs_);
			MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
			u32 rd=WriteReg(_Rd_);
			OR32(rd,rs,HOST_a2);
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			OR32(rd,rs,rt);
		}
	}
}   

//REC_FUNC(XOR);
static void recXOR() {
// Rd = Rs Xor Rt
	if (_Rd_)
	{
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			MapConst(_Rd_, iRegs[_Rs_].k ^ iRegs[_Rt_].k);
		} else if (IsConst(_Rs_)) {
			MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			XOR32(rd,HOST_a1,rt);
		} else if (IsConst(_Rt_)) {
			u32 rs=ReadReg(_Rs_);
			MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
			u32 rd=WriteReg(_Rd_);
			XOR32(rd,rs,HOST_a2);
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			XOR32(rd,rs,rt);
		}
	}
}

//REC_FUNC(NOR);
static void recNOR() {
// Rd = Rs Nor Rt
	if (_Rd_)
	{
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			MapConst(_Rd_, ~(iRegs[_Rs_].k | iRegs[_Rt_].k));
		} else if (IsConst(_Rs_)) {
			MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			OR32(rd,HOST_a1,rt);
			MOV32ItoR(HOST_a2,0xffffffff);
			XOR32(rd,rd,HOST_a2);
		} else if (IsConst(_Rt_)) {
			u32 rs=ReadReg(_Rs_);
			MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
			u32 rd=WriteReg(_Rd_);
			OR32(rd,rs,HOST_a2);
			MOV32ItoR(HOST_a2,0xffffffff);
			XOR32(rd,rd,HOST_a2);
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			OR32(rd,rs,rt);
			MOV32ItoR(HOST_a2,0xffffffff);
			XOR32(rd,rd,HOST_a2);
		}
	}
}

//REC_FUNC(SLT);
static void recSLT() {
// Rd = Rs < Rt (signed)
	if (_Rd_)
	{
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			MapConst(_Rd_, (s32)iRegs[_Rs_].k < (s32)iRegs[_Rt_].k);
		} else if (IsConst(_Rs_)) {
			MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SLT(rd,HOST_a1,rt);
		} else if (IsConst(_Rt_)) {
			u32 rs=ReadReg(_Rs_);
			MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
			u32 rd=WriteReg(_Rd_);
			SLT(rd,rs,HOST_a2);
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SLT(rd,rs,rt);
		}
	}
}  

//REC_FUNC(SLTU);
static void recSLTU() { 
// Rd = Rs < Rt (unsigned)
	if (_Rd_)
	{
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			MapConst(_Rd_, iRegs[_Rs_].k < iRegs[_Rt_].k);
		} else if (IsConst(_Rs_)) {
			MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SLTU(rd,HOST_a1,rt);
		} else if (IsConst(_Rt_)) {
			u32 rs=ReadReg(_Rs_);
			MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
			u32 rd=WriteReg(_Rd_);
			SLTU(rd,rs,HOST_a2);
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SLTU(rd,rs,rt);
		}
	}
}

#define SMULL(lo,hi,op1,op2) write32((0xe0c00090 + ((hi)<<16) + ((lo)<<12) + ((op2)<<8) + (op1) ))

//REC_FUNC(MULT);
static void recMULT() {
// Lo/Hi = Rs * Rt (signed)

	if ((IsConst(_Rs_) && iRegs[_Rs_].k == 0) ||
		(IsConst(_Rt_) && iRegs[_Rt_].k == 0)) {
		MapConst(32,0); // psxRegs.GPR.n.lo
		MapConst(33,0); // psxRegs.GPR.n.hi
		return;
	}
	
	if (IsConst(_Rs_) && IsConst(_Rt_))
	{
		u64 res = (s64)((s64)(s32)iRegs[_Rs_].k * (s64)(s32)iRegs[_Rt_].k);
		MapConst(32,(u32)(res & 0xffffffff)); // psxRegs.GPR.n.lo
		MapConst(33,(u32)((res >> 32) & 0xffffffff)); // psxRegs.GPR.n.hi
		return;
	}

	u32 lo=WriteReg(32);
	u32 hi=WriteReg(33);
	
	if (IsConst(_Rs_)) {
		MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
		u32 rt=ReadReg(_Rt_);
		SMULL(lo,hi,HOST_a1,rt);
	}
	else if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_a1, iRegs[_Rt_].k);
		u32 rs=ReadReg(_Rs_);
		SMULL(lo,hi,HOST_a1,rs);
	}
	else
	{
		u32 rs=ReadReg(_Rs_);
		u32 rt=ReadReg(_Rt_);
		SMULL(lo,hi,rs,rt);
	}
}

#define UMULL(lo,hi,op1,op2) write32((0xe0800090 + ((hi)<<16) + ((lo)<<12) + ((op2)<<8) + (op1) ))

//REC_FUNC(MULTU);
static void recMULTU() {
// Lo/Hi = Rs * Rt (unsigned)

	if ((IsConst(_Rs_) && iRegs[_Rs_].k == 0) ||
		(IsConst(_Rt_) && iRegs[_Rt_].k == 0)) {
		MapConst(32,0); // psxRegs.GPR.n.lo
		MapConst(33,0); // psxRegs.GPR.n.hi
		return;
	}

	if (IsConst(_Rs_) && IsConst(_Rt_))
	{
		u64 res = (u64)((u64)(u32)iRegs[_Rs_].k * (u64)(u32)iRegs[_Rt_].k);
		MapConst(32,(u32)(res & 0xffffffff)); // psxRegs.GPR.n.lo
		MapConst(33,(u32)((res >> 32) & 0xffffffff)); // psxRegs.GPR.n.hi
		return;
	}
	
	u32 lo=WriteReg(32);
	u32 hi=WriteReg(33);
	
	if (IsConst(_Rs_)) {
		MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
		u32 rt=ReadReg(_Rt_);
		UMULL(lo,hi,HOST_a1,rt);
	}
	else if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_a1, iRegs[_Rt_].k);
		u32 rs=ReadReg(_Rs_);
		UMULL(lo,hi,HOST_a1,rs);
	}
	else
	{
		u32 rs=ReadReg(_Rs_);
		u32 rt=ReadReg(_Rt_);
		UMULL(lo,hi,rs,rt);
	}
}

static void _DIV(s32 op1,s32 op2) { if (op2) { _i32(_rLo_) = op1 / op2; _i32(_rHi_) = op1 % op2; } }

//REC_FUNC(DIV);
static void recDIV() {
// Lo/Hi = Rs / Rt (signed)

	if (IsConst(_Rt_) && IsConst(_Rs_)) {
		if (iRegs[_Rt_].k)
		{
			MapConst(32, (s32)iRegs[_Rs_].k / (s32)iRegs[_Rt_].k); // psxRegs.GPR.n.lo
			MapConst(33, (s32)iRegs[_Rs_].k % (s32)iRegs[_Rt_].k); // psxRegs.GPR.n.hi
		}
		return;
	}

	if (IsConst(_Rt_)) {
		if (iRegs[_Rt_].k == 0) return;
		MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_a2, rt);
	}
	if (IsConst(_Rs_)) {
		MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
	} else {
		u32 rs=ReadReg(_Rs_);
		MOV32RtoR(HOST_a1, rs);
	}
	
	iFlushReg(32);
	iFlushReg(33);
	CALLFunc((u32)_DIV);
}

static void _DIVU(u32 op1,u32 op2) { if (op2) { psxRegs.GPR.r[32] = op1 / op2; psxRegs.GPR.r[33] = op1 % op2; } }

//REC_FUNC(DIVU);
static void recDIVU() {
// Lo/Hi = Rs / Rt (unsigned)

	if (IsConst(_Rt_) && IsConst(_Rs_)) {
		if (iRegs[_Rt_].k)
		{
			MapConst(32, (u32)iRegs[_Rs_].k / (u32)iRegs[_Rt_].k); // psxRegs.GPR.n.lo
			MapConst(33, (u32)iRegs[_Rs_].k % (u32)iRegs[_Rt_].k); // psxRegs.GPR.n.hi
		}
		return;
	}

	if (IsConst(_Rt_)) {
		if (iRegs[_Rt_].k == 0) return;
		MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_a2, rt);
	}
	if (IsConst(_Rs_)) {
		MOV32ItoR(HOST_a1, iRegs[_Rs_].k);
	} else {
		u32 rs=ReadReg(_Rs_);
		MOV32RtoR(HOST_a1, rs);
	}
	iFlushReg(32);
	iFlushReg(33);
	CALLFunc((u32)_DIVU);
}

//REC_FUNC(SLL);
static void recSLL() {
// Rd = Rt << Sa
	if (_Rd_)
	{
		if (IsConst(_Rt_)) {
			MapConst(_Rd_, iRegs[_Rt_].k << _Sa_);
		} else {
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			if (_Sa_)
			{
				SHLI32(rd,rt,_Sa_);
			}
			else
			{
				MOV32RtoR(rd,rt);
			}
		}
	}
}

//REC_FUNC(SRL);
static void recSRL() {
// Rd = Rt >> Sa
	if (_Rd_)
	{
		if (IsConst(_Rt_)) {
			MapConst(_Rd_, iRegs[_Rt_].k >> _Sa_);
		} else {
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			if (_Sa_)
			{
				SHRI32(rd,rt,_Sa_);
			}
			else
			{
				MOV32RtoR(rd,rt);
			}
		}
	}
}

//REC_FUNC(SRA);
static void recSRA() {
// Rd = Rt >> Sa
	if (_Rd_)
	{
		if (IsConst(_Rt_)) {
			MapConst(_Rd_, (s32)iRegs[_Rt_].k >> _Sa_);
		} else {
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			if (_Sa_)
			{
				SARI32(rd,rt,_Sa_);
			}
			else
			{
				MOV32RtoR(rd,rt);
			}
		}
	}
}

//REC_FUNC(SLLV);
static void recSLLV() {
// Rd = Rt << Rs
	if (_Rd_)
	{
		if (IsConst(_Rt_) && IsConst(_Rs_)) {
			MapConst(_Rd_, iRegs[_Rt_].k << iRegs[_Rs_].k);
		} else if (IsConst(_Rs_)) {
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SHLI32(rd,rt,iRegs[_Rs_].k);
		} else if (IsConst(_Rt_)) {
			u32 rs=ReadReg(_Rs_);
			MOV32ItoR(HOST_a1, iRegs[_Rt_].k);
			u32 rd=WriteReg(_Rd_);
			SHL32(rd,HOST_a1,rs);
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SHL32(rd,rt,rs);
		}
	}
}

//REC_FUNC(SRLV);
static void recSRLV() {
// Rd = Rt >> Rs
	if (_Rd_)
	{
		if (IsConst(_Rt_) && IsConst(_Rs_)) {
			MapConst(_Rd_, iRegs[_Rt_].k >> iRegs[_Rs_].k);
		} else if (IsConst(_Rs_)) {
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SHRI32(rd,rt,iRegs[_Rs_].k);
		} else if (IsConst(_Rt_)) {
			u32 rs=ReadReg(_Rs_);
			MOV32ItoR(HOST_a1, iRegs[_Rt_].k);
			u32 rd=WriteReg(_Rd_);
			SHR32(rd,HOST_a1,rs);
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SHR32(rd,rt,rs);
		}
	}
}

//REC_FUNC(SRAV);
static void recSRAV() {
// Rd = Rt >> Rs
	if (_Rd_)
	{
		if (IsConst(_Rt_) && IsConst(_Rs_)) {
			MapConst(_Rd_, (s32)iRegs[_Rt_].k >> iRegs[_Rs_].k);
		} else if (IsConst(_Rs_)) {
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SARI32(rd,rt,iRegs[_Rs_].k);
		} else if (IsConst(_Rt_)) {
			u32 rs=ReadReg(_Rs_);
			MOV32ItoR(HOST_a1, iRegs[_Rt_].k);
			u32 rd=WriteReg(_Rd_);
			SAR32(rd,HOST_a1,rs);
		} else {
			u32 rs=ReadReg(_Rs_);
			u32 rt=ReadReg(_Rt_);
			u32 rd=WriteReg(_Rd_);
			SAR32(rd,rt,rs);
		}
	}
}
