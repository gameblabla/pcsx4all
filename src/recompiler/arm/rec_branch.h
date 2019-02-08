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

// CHUI: Añadimos el codigo de psxTestLoadDelay cuando usamos el interprete nuevo
#if defined (interpreter_new) || defined (interpreter_none)
// this defines shall be used with the tmp 
// of the next func (instead of _Funct_...)
#define _tFunct_  ((tmp      ) & 0x3F)  // The funct part of the instruction register 
#define _tRd_     ((tmp >> 11) & 0x1F)  // The rd part of the instruction register 
#define _tRt_     ((tmp >> 16) & 0x1F)  // The rt part of the instruction register 
#define _tRs_     ((tmp >> 21) & 0x1F)  // The rs part of the instruction register 
#define _tSa_     ((tmp >>  6) & 0x1F)  // The sa part of the instruction register

int psxTestLoadDelay(int reg, u32 tmp) {
	if (tmp == 0) return 0; // NOP
	switch (tmp >> 26) {
		case 0x00: // SPECIAL
			switch (_tFunct_) {
				case 0x00: // SLL
				case 0x02: case 0x03: // SRL/SRA
					if (_tRd_ == reg && _tRt_ == reg) return 1; else
					if (_tRt_ == reg) return 2; else
					if (_tRd_ == reg) return 3;
					break;

				case 0x08: // JR
					if (_tRs_ == reg) return 2;
					break;
				case 0x09: // JALR
					if (_tRd_ == reg && _tRs_ == reg) return 1; else
					if (_tRs_ == reg) return 2; else
					if (_tRd_ == reg) return 3;
					break;

				// SYSCALL/BREAK just a break;

				case 0x20: case 0x21: case 0x22: case 0x23:
				case 0x24: case 0x25: case 0x26: case 0x27: 
				case 0x2a: case 0x2b: // ADD/ADDU...
				case 0x04: case 0x06: case 0x07: // SLLV...
					if (_tRd_ == reg && (_tRt_ == reg || _tRs_ == reg)) return 1; else
					if (_tRt_ == reg || _tRs_ == reg) return 2; else
					if (_tRd_ == reg) return 3;
					break;

				case 0x10: case 0x12: // MFHI/MFLO
					if (_tRd_ == reg) return 3;
					break;
				case 0x11: case 0x13: // MTHI/MTLO
					if (_tRs_ == reg) return 2;
					break;

				case 0x18: case 0x19:
				case 0x1a: case 0x1b: // MULT/DIV...
					if (_tRt_ == reg || _tRs_ == reg) return 2;
					break;
			}
			break;

		case 0x01: // REGIMM
			switch (_tRt_) {
				case 0x00: case 0x02:
				case 0x10: case 0x12: // BLTZ/BGEZ...
					if (_tRs_ == reg) return 2;
					break;
			}
			break;

		// J would be just a break;
		case 0x03: // JAL
			if (31 == reg) return 3;
			break;

		case 0x04: case 0x05: // BEQ/BNE
			if (_tRs_ == reg || _tRt_ == reg) return 2;
			break;

		case 0x06: case 0x07: // BLEZ/BGTZ
			if (_tRs_ == reg) return 2;
			break;

		case 0x08: case 0x09: case 0x0a: case 0x0b:
		case 0x0c: case 0x0d: case 0x0e: // ADDI/ADDIU...
			if (_tRt_ == reg && _tRs_ == reg) return 1; else
			if (_tRs_ == reg) return 2; else
			if (_tRt_ == reg) return 3;
			break;

		case 0x0f: // LUI
			if (_tRt_ == reg) return 3;
			break;

		case 0x10: // COP0
			switch (_tFunct_) {
				case 0x00: // MFC0
					if (_tRt_ == reg) return 3;
					break;
				case 0x02: // CFC0
					if (_tRt_ == reg) return 3;
					break;
				case 0x04: // MTC0
					if (_tRt_ == reg) return 2;
					break;
				case 0x06: // CTC0
					if (_tRt_ == reg) return 2;
					break;
				// RFE just a break;
			}
			break;

		case 0x12: // COP2
			switch (_tFunct_) {
				case 0x00: 
					switch (_tRs_) {
						case 0x00: // MFC2
							if (_tRt_ == reg) return 3;
							break;
						case 0x02: // CFC2
							if (_tRt_ == reg) return 3;
							break;
						case 0x04: // MTC2
							if (_tRt_ == reg) return 2;
							break;
						case 0x06: // CTC2
							if (_tRt_ == reg) return 2;
							break;
					}
					break;
				// RTPS... break;
			}
			break;

		case 0x22: case 0x26: // LWL/LWR
			if (_tRt_ == reg) return 3; else
			if (_tRs_ == reg) return 2;
			break;

		case 0x20: case 0x21: case 0x23:
		case 0x24: case 0x25: // LB/LH/LW/LBU/LHU
			if (_tRt_ == reg && _tRs_ == reg) return 1; else
			if (_tRs_ == reg) return 2; else
			if (_tRt_ == reg) return 3;
			break;

		case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2e: // SB/SH/SWL/SW/SWR
			if (_tRt_ == reg || _tRs_ == reg) return 2;
			break;

		case 0x32: case 0x3a: // LWC2/SWC2
			if (_tRs_ == reg) return 2;
			break;
	}

	return 0;
}
#endif


//REC_BRANCH(J);
static void recJ() {
// j target
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecJ %p\n",_Target_ * 4 + (pc & 0xf0000000));
	}
#endif
// CHUI: No hay mas remedio que hacer iJump porque puede no tener en este momento el codigo MIPS recompilado donde salta.
	iJump(_Target_ * 4 + (pc & 0xf0000000));
}

//REC_BRANCH(JR);
static void recJR() {
// jr Rs
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecJR R%i\n",_Rs_);
	}
#endif
// CHUI: No hay mas remedio que hacer SetBranch porque puede no tener en este momento el codigo MIPS recompilado donde salta.
	if (IsConst(_Rs_)) {
		iJump(iRegs[_Rs_].k);
	} else {
		u32 rs=ReadReg(_Rs_);
		MOV32RtoM((u32)&target, rs);
		SetBranch();
	}
}

//REC_BRANCH(JAL);
static void recJAL() {
// jal target
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecJAL %p\n",_Target_ * 4 + (pc & 0xf0000000));
	}
#endif
	MapConst(31, pc+4);

// CHUI: No hay mas remedio que hacer iJump porque puede no tener en este momento el codigo MIPS recompilado donde salta.
	iJump(_Target_ * 4 + (pc & 0xf0000000));
}

//REC_BRANCH(JALR);
static void recJALR() {
// jalr Rs
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecJALR R%i\n",_Rs_);
	}
#endif
	if (_Rd_) {
		MapConst(_Rd_, pc+4);
	}
// CHUI: No hay mas remedio que hacer SetBranch porque puede no tener en este momento el codigo MIPS recompilado donde salta.
	if (IsConst(_Rs_)) {
		iJump(iRegs[_Rs_].k);
	} else {
		u32 rs=ReadReg(_Rs_);
		MOV32RtoM((u32)&target, rs);
		SetBranch();
	}
}

//REC_BRANCH(BLTZ);
static void recBLTZ() {
// Branch if Rs < 0
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecBLTZ %p if R%i < 0\n",_Imm_ * 4 + pc,_Rs_);
	}
#endif
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k < 0) {
// CHUI: Es posible saltarse psxBranchTest?
			iBranch(bpc,0);
			pc+=4;
		} else {
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		}
		return;
	}
	u32 rs=ReadReg(_Rs_);
#ifndef REC_USE_2ND_PHASE
// CHUI: Si PC es el inicio del bloque recompilado y no supera el maximo hacemos unroll-loop
	if (bpc == pcinit && rec_inloop<REC_MAX_LOOPS && pcabs(pc,bpc)<REC_MAX_TO_TEST && rec_total_opcodes<REC_MAX_OPCODES_LIMIT) {
		rec_inloop++;
		j32Ptr[4] = JLTZ32(rs);
		iLoop();
		armSetJ32(j32Ptr[4]);
		iBranch(bpc, 0);
	} else
#endif
	{
		u32 pcback=pc;
		if (rec_phase) {
			j32Ptr[4] = JGEZ32(rs);
		}
		iBranch(bpc, 1);
		if (rec_phase) {
			armSetJ32(j32Ptr[4]);
		}
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		pc=pcback-4;
		branch=0;
	}
	pc += 4;
}


//REC_BRANCH(BGTZ);
static void recBGTZ() {
// Branch if Rs > 0
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecBGTZ %p if R%i > 0\n",_Imm_ * 4 + pc,_Rs_);
	}
#endif
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k > 0) {
// CHUI: Es posible saltarse psxBranchTest?
			iBranch(bpc,0);
			pc+=4;
		} else {
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		}
		return;
	}
	u32 rs=ReadReg(_Rs_);
#ifndef REC_USE_2ND_PHASE
// CHUI: Si PC es el inicio del bloque recompilado y no supera el maximo hacemos unroll-loop
	if (bpc == pcinit && rec_inloop<REC_MAX_LOOPS && pcabs(pc,bpc)<REC_MAX_TO_TEST && rec_total_opcodes<REC_MAX_OPCODES_LIMIT) {
		rec_inloop++;
		j32Ptr[4] = JGTZ32(rs);
		iLoop();
		armSetJ32(j32Ptr[4]);
		iBranch(bpc, 0);
	} else
#endif
	{
		u32 pcback=pc;
		if (rec_phase) {
			j32Ptr[4] = JLEZ32(rs);
		}
		iBranch(bpc, 1);
		if (rec_phase) {
			armSetJ32(j32Ptr[4]);
		}
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		pc=pcback-4;
		branch=0;
	}
	pc+=4;
}


//REC_BRANCH(BLTZAL);
static void recBLTZAL() {
// Branch if Rs < 0
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecBLTZAL %p if R%i < 0\n",_Imm_ * 4 + pc,_Rs_);
	}
#endif
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k < 0) {
			u32 r31=WriteReg(31);
			MOV32ItoR(r31, pc+4);
// CHUI: Es posible saltarse psxBranchTest?
			iBranch(bpc,0);
			pc+=4;
		} else {
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		}
		return;
	}

	u32 rs=ReadReg(_Rs_);
#ifndef REC_USE_2ND_PHASE
// CHUI: Si PC es el inicio del bloque recompilado y no supera el maximo hacemos unroll-loop
	if (bpc == pcinit && rec_inloop<REC_MAX_LOOPS && pcabs(pc,bpc)<REC_MAX_TO_TEST && rec_total_opcodes<REC_MAX_OPCODES_LIMIT) {
		rec_inloop++;
		j32Ptr[4] = JLTZ32(rs);
		iLoop();
		armSetJ32(j32Ptr[4]);
		u32 r31=WriteReg(31);
		MOV32ItoR(r31, pc+4);
		iBranch(bpc, 0);
	} else
#endif
	{
		u32 pcback=pc;
		if (rec_phase) {
			j32Ptr[4] = JGEZ32(rs);
		}
		memcpy(iRegsS, iRegs, sizeof(iRegs));
		memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
#ifdef REC_USE_2ND_PHASE
		if (!rec_phase) {
			memcpy(recPrev_totalS, recPrev_total, sizeof(recPrev_total));
		}
#endif	
		u32 r31=WriteReg(31);
		MOV32ItoR(r31, pc+4);
		iBranch(bpc, 2);
		if (rec_phase) {
			armSetJ32(j32Ptr[4]);
		}
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		pc=pcback-4;
		branch=0;
	}
	pc += 4;
}

//REC_BRANCH(BGEZAL);
static void recBGEZAL() {
// Branch if Rs >= 0
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecBGEZAL %p if R%i >= 0\n",_Imm_ * 4 + pc,_Rs_);
	}
#endif
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k >= 0) {
			u32 r31=WriteReg(31);
			MOV32ItoR(r31, pc+4);
// CHUI: Es posible saltarse psxBranchTest?
			iBranch(bpc,0);
			pc+=4;
		} else {
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		}
		return;
	}

	u32 rs=ReadReg(_Rs_);
#ifndef REC_USE_2ND_PHASE
// CHUI: Si PC es el inicio del bloque recompilado y no supera el maximo hacemos unroll-loop
	if (bpc == pcinit && rec_inloop<REC_MAX_LOOPS && pcabs(pc,bpc)<REC_MAX_TO_TEST && rec_total_opcodes<REC_MAX_OPCODES_LIMIT) {
		rec_inloop++;
		j32Ptr[4] = JGEZ32(rs);
		iLoop();
		armSetJ32(j32Ptr[4]);
		u32 r31=WriteReg(31);
		MOV32ItoR(r31, pc+4);
		iBranch(bpc, 0);
	} else
#endif
	{
		u32 pcback=pc;
		if (rec_phase) {
			j32Ptr[4] = JLTZ32(rs);
		}
		memcpy(iRegsS, iRegs, sizeof(iRegs));
		memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
#ifdef REC_USE_2ND_PHASE
		if (!rec_phase) {
			memcpy(recPrev_totalS, recPrev_total, sizeof(recPrev_total));
		}
#endif
		u32 r31=WriteReg(31);
		MOV32ItoR(r31, pc+4);
		iBranch(bpc, 2);
		if (rec_phase) {
			armSetJ32(j32Ptr[4]);
		}
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		pc=pcback-4;
		branch=0;
	}
	pc+=4;
}

//REC_BRANCH(BNE);
static void recBNE() {
// Branch if Rs != Rt
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecBNE %p if R%i != R%i\n",_Imm_ * 4 + pc,_Rs_,_Rt_);
	}
#endif
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_) && IsConst(_Rt_)) {
		if (iRegs[_Rs_].k != iRegs[_Rt_].k) {
// CHUI: Es posible saltarse psxBranchTest?
			iBranch(bpc,0);
			pc+=4;
		} else {
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		}
		return;
	}

	u32 rs,rt;
	
	if (IsConst(_Rs_)) {
		MOV32ItoR(HOST_r0,iRegs[_Rs_].k);
		rs=HOST_r0;
	} else {
		rs=ReadReg(_Rs_);
	}
	
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_r1,iRegs[_Rt_].k);
		rt=HOST_r1;
	} else {
		rt=ReadReg(_Rt_);
	}

	write32(CMP_REGS(rs,rt));
#ifndef REC_USE_2ND_PHASE
// CHUI: Si PC es el inicio del bloque recompilado y no supera el maximo hacemos unroll-loop
	if (bpc == pcinit && rec_inloop<REC_MAX_LOOPS && pcabs(pc,bpc)<REC_MAX_TO_TEST && rec_total_opcodes<REC_MAX_OPCODES_LIMIT) {
		rec_inloop++;
		j32Ptr[4] = armPtr; write32(BNE_FWD(0));
		iLoop();
		armSetJ32(j32Ptr[4]);
		iBranch(bpc, 0);
	} else
#endif
	{
		u32 pcback=pc;
		if (rec_phase) {
			j32Ptr[4] = armPtr; write32(BEQ_FWD(0));
		}
		iBranch(bpc, 1);
		if (rec_phase) {
			armSetJ32(j32Ptr[4]);
		}
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		pc=pcback-4;
		branch=0;
	}
	pc += 4;
}

//REC_BRANCH(BEQ);
static void recBEQ() {
// Branch if Rs == Rt
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecBEQ %p if R%i == R%i\n",_Imm_ * 4 + pc,_Rs_,_Rt_);
	}
#endif
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (_Rs_ == _Rt_) {
// CHUI: Es posible saltarse psxBranchTest?
		iBranch(bpc,0);
		pc+=4;
		return;
	} else {
		if (IsConst(_Rs_) && IsConst(_Rt_)) {
			if (iRegs[_Rs_].k == iRegs[_Rt_].k) {
				iBranch(bpc,0);
				pc+=4;
			} else {
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
			}
			return;
		}

		u32 rs,rt;

		if (IsConst(_Rs_)) {
			MOV32ItoR(HOST_r0,iRegs[_Rs_].k);
			rs=HOST_r0;
		} else {
			rs=ReadReg(_Rs_);
		}
		
		if (IsConst(_Rt_)) {
			MOV32ItoR(HOST_r1,iRegs[_Rt_].k);
			rt=HOST_r1;
		} else {
			rt=ReadReg(_Rt_);
		}
		write32(CMP_REGS(rs,rt));
#ifndef REC_USE_2ND_PHASE
// CHUI: Si PC es el inicio del bloque recompilado y no supera el maximo hacemos unroll-loop
		if (bpc == pcinit && rec_inloop<REC_MAX_LOOPS && pcabs(pc,bpc)<REC_MAX_TO_TEST && rec_total_opcodes<REC_MAX_OPCODES_LIMIT) {
			rec_inloop++;
			j32Ptr[4] = armPtr; write32(BEQ_FWD(0));
			iLoop();
			armSetJ32(j32Ptr[4]);
			iBranch(bpc, 0);
		} else
#endif
		{
			u32 pcback=pc;
			if (rec_phase) {
				j32Ptr[4] = armPtr; write32(BNE_FWD(0));
			}
			iBranch(bpc, 1);
			if (rec_phase) {
				armSetJ32(j32Ptr[4]);
			}
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
			pc=pcback-4;
			branch=0;
		}
		pc += 4;
	}
}

//REC_BRANCH(BLEZ);
static void recBLEZ() {
// Branch if Rs <= 0
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecBLEZ %p if R%i <= 0\n",_Imm_ * 4 + pc,_Rs_);
	}
#endif
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k <= 0) {
			iBranch(bpc,0);
			pc+=4;
		} else {
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		}
		return;
	}

	u32 rs=ReadReg(_Rs_);
#ifndef REC_USE_2ND_PHASE
// CHUI: Si PC es el inicio del bloque recompilado y no supera el maximo hacemos unroll-loop
	if (bpc == pcinit && rec_inloop<REC_MAX_LOOPS && pcabs(pc,bpc)<REC_MAX_TO_TEST && rec_total_opcodes<REC_MAX_OPCODES_LIMIT) {
		rec_inloop++;
		j32Ptr[4] = JLEZ32(rs);
		iLoop();
		armSetJ32(j32Ptr[4]);
		iBranch(bpc, 0);
	} else
#endif
	{
		u32 pcback=pc;
		if (rec_phase) {
			j32Ptr[4] = JGTZ32(rs);
		}
		iBranch(bpc, 1);
		if (rec_phase) {
			armSetJ32(j32Ptr[4]);
		}
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		pc=pcback-4;
		branch=0;
	}
	pc += 4;
}

//REC_BRANCH(BGEZ);
static void recBGEZ() {
// Branch if Rs >= 0
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\trecBGEZ %p if R%i >= 0\n",_Imm_ * 4 + pc,_Rs_);
	}
#endif
	u32 bpc = _Imm_ * 4 + pc;

	if (bpc == pc+4 && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (IsConst(_Rs_)) {
		if ((s32)iRegs[_Rs_].k >= 0) {
			iBranch(bpc,0);
			pc+=4;
		} else {
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		}
		return;
	}

	u32 rs=ReadReg(_Rs_);
#ifndef REC_USE_2ND_PHASE
// CHUI: Si PC es el inicio del bloque recompilado y no supera el maximo hacemos unroll-loop
	if (bpc == pcinit && rec_inloop<REC_MAX_LOOPS && pcabs(pc,bpc)<REC_MAX_TO_TEST && rec_total_opcodes<REC_MAX_OPCODES_LIMIT) {
		rec_inloop++;
		j32Ptr[4] = JGEZ32(rs);
		iLoop();
		armSetJ32(j32Ptr[4]);
		iBranch(bpc, 0);
	} else
#endif
	{
		u32 pcback=pc;
		if (rec_phase) {
			j32Ptr[4] = JLTZ32(rs);
		}
		iBranch(bpc, 1);
		if (rec_phase) {
			armSetJ32(j32Ptr[4]);
		}
// CHUI: Si no cumple la condicion seguimos sin mas como hace el interprete
		pc=pcback-4;
		branch=0;
	}
	pc += 4;
}

