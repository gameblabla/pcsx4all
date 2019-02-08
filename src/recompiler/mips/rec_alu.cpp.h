/***********************************************
 * Options that can be disabled for debugging: *
 ***********************************************/

/* Convert various MIPS I opcode sequences to a modern opcode */
#if defined(HAVE_MIPS32R2_EXT_INS) && defined(HAVE_MIPS32R2_SEB_SEH)
#define USE_MIPS32R2_ALU_OPCODE_CONVERSION
#endif


/* NOTE: There's no need to do zero register optimizations since we have
         native zero reg on mips. */
#define REC_ITYPE_RT_RS_I16(insn, _rt_, _rs_, _imm_) \
do { \
	u32 rt  = _rt_; \
	u32 rs  = _rs_; \
	s32 imm = _imm_; \
	if (!rt) break; \
	SetUndef(_rt_); \
	u32 r1, r2; \
	if (rs == rt) { \
		r1 = regMipsToHost(rt, REG_LOAD, REG_REGISTER); \
		r2 = r1; \
	} else { \
		r1 = regMipsToHost(rt, REG_FIND, REG_REGISTER); \
		r2 = regMipsToHost(rs, REG_LOAD, REG_REGISTER); \
	} \
	insn(r1, r2, imm); \
	regMipsChanged(rt); \
	regUnlock(r1); \
	regUnlock(r2); \
} while (0)

static void recADDIU()
{
	// rt = rs + (s32)imm

	const bool set_const = IsConst(_Rs_);

	/* Catch ADDIU reg, $0, imm */
	/* Exit if const already loaded */
	if (!_Rs_ && IsConst(_Rt_) && GetConst(_Rt_) == (s32)_Imm_)
		return;

	REC_ITYPE_RT_RS_I16(ADDIU,  _Rt_, _Rs_, _Imm_);

	if (set_const)
		SetConst(_Rt_, GetConst(_Rs_) + (s32)_Imm_);
}
static void recADDI() { recADDIU(); }

static void recSLTI()
{
	// rt = (s32)rs < (s32)imm

	const bool set_const = IsConst(_Rs_);

	REC_ITYPE_RT_RS_I16(SLTI, _Rt_, _Rs_, _Imm_);

	if (set_const)
		SetConst(_Rt_, (s32)GetConst(_Rs_) < (s32)_Imm_);
}

static void recSLTIU()
{
	// rt = (u32)rs < (u32)((s32)imm)
	// NOTE: SLTIU sign-extends its immediate before the unsigned comparison

	const bool set_const = IsConst(_Rs_);

	REC_ITYPE_RT_RS_I16(SLTIU, _Rt_, _Rs_, _Imm_);

	if (set_const)
		SetConst(_Rt_, GetConst(_Rs_) < (u32)((s32)_Imm_));
}


#define REC_ITYPE_RT_RS_U16(insn, _rt_, _rs_, _imm_) \
do { \
	u32 rt  = _rt_; \
	u32 rs  = _rs_; \
	u32 imm = _imm_; \
	if (!rt) break; \
	SetUndef(_rt_); \
	u32 r1, r2; \
	if (rs == rt) { \
		r1 = regMipsToHost(rt, REG_LOAD, REG_REGISTER); \
		r2 = r1; \
	} else { \
		r1 = regMipsToHost(rt, REG_FIND, REG_REGISTER); \
		r2 = regMipsToHost(rs, REG_LOAD, REG_REGISTER); \
	} \
	insn(r1, r2, imm); \
	regMipsChanged(rt); \
	regUnlock(r1); \
	regUnlock(r2); \
} while (0)

static void recANDI()
{
	// rt = rs & (u32)imm

	const bool set_const = IsConst(_Rs_);

	REC_ITYPE_RT_RS_U16(ANDI, _Rt_, _Rs_, _ImmU_);

	if (set_const)
		SetConst(_Rt_, GetConst(_Rs_) & (u32)_ImmU_);
}

static void recORI()
{
	// rt = rs | (u32)imm

	bool set_const = IsConst(_Rs_);

	/* Catch ORI reg, $0, imm */
	/* Exit if const already loaded */
	if (!_Rs_ && IsConst(_Rt_) && GetConst(_Rt_) == (u32)_ImmU_)
		return;

	REC_ITYPE_RT_RS_U16(ORI,  _Rt_, _Rs_, _ImmU_);

	if (set_const)
		SetConst(_Rt_, GetConst(_Rs_) | (u32)(_ImmU_));
}

static void recXORI()
{
	// rt = rs ^ (u32)imm

	const bool set_const = IsConst(_Rs_);

	REC_ITYPE_RT_RS_U16(XORI, _Rt_, _Rs_, _ImmU_);

	if (set_const)
		SetConst(_Rt_, GetConst(_Rs_) ^ (u32)(_ImmU_));
}


#define REC_ITYPE_RT_U16(insn, _rt_, _imm_) \
do { \
	u32 rt  = _rt_; \
	u32 imm = _imm_; \
	if (!rt) break; \
	u32 r1 = regMipsToHost(rt, REG_FIND, REG_REGISTER); \
	insn(r1, imm); \
	regMipsChanged(rt); \
	regUnlock(r1); \
} while (0)

static void recLUI()
{
	// rt = (u32)imm << 16

	/* Avoid loading the same constant more than once */
	if (IsConst(_Rt_) && GetConst(_Rt_) == ((u32)_ImmU_ << 16))
		return;

	REC_ITYPE_RT_U16(LUI, _Rt_, _ImmU_);

	SetConst(_Rt_, (u32)_ImmU_ << 16);
}


#define REC_RTYPE_RD_RS_RT(insn, _rd_, _rs_, _rt_) \
do { \
	u32 rd  = _rd_; \
	u32 rt  = _rt_; \
	u32 rs  = _rs_; \
	if (!rd) break; \
	u32 r1, r2, r3; \
	SetUndef(_rd_); \
	if (rs == rd) { \
		r1 = regMipsToHost(rd, REG_LOAD, REG_REGISTER); \
		r2 = r1; \
		r3 = (rd == rt ? r1 : regMipsToHost(rt, REG_LOAD, REG_REGISTER)); \
	} else if (rt == rd) { \
		r1 = regMipsToHost(rd, REG_LOAD, REG_REGISTER); \
		r3 = r1; \
		r2 = regMipsToHost(rs, REG_LOAD, REG_REGISTER); \
	} else { \
		r1 = regMipsToHost(rd, REG_FIND, REG_REGISTER); \
		r2 = regMipsToHost(rs, REG_LOAD, REG_REGISTER); \
		r3 = (rs == rt ? r2 : regMipsToHost(rt, REG_LOAD, REG_REGISTER)); \
	} \
	insn(r1, r2, r3); \
	regMipsChanged(rd); \
	regUnlock(r1); \
	regUnlock(r2); \
	regUnlock(r3); \
} while (0)

static void recADDU()
{
	// rd = rs + rt

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RS_RT(ADDU, _Rd_, _Rs_, _Rt_);

	if (set_const)
		SetConst(_Rd_, GetConst(_Rs_) + GetConst(_Rt_));
}
static void recADD()  { recADDU(); }

static void recSUBU()
{
	// rd = rs - rt

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RS_RT(SUBU, _Rd_, _Rs_, _Rt_);

	if (set_const)
		SetConst(_Rd_, GetConst(_Rs_) - GetConst(_Rt_));
}
static void recSUB()  { recSUBU(); }

static void recAND()
{
	// rd = rs & rt

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RS_RT(AND, _Rd_, _Rs_, _Rt_);

	if (set_const)
		SetConst(_Rd_, GetConst(_Rs_) & GetConst(_Rt_));
}

static void recOR()
{
	// rd = rs | rt

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RS_RT(OR,  _Rd_, _Rs_, _Rt_);

	if (set_const)
		SetConst(_Rd_, GetConst(_Rs_) | GetConst(_Rt_));
}

static void recXOR()
{
	// rd = rs ^ rt

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RS_RT(XOR, _Rd_, _Rs_, _Rt_);

	if (set_const)
		SetConst(_Rd_, GetConst(_Rs_) ^ GetConst(_Rt_));
}

static void recNOR()
{
	// rd = ~(rs | rt)

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RS_RT(NOR, _Rd_, _Rs_, _Rt_);

	if (set_const)
		SetConst(_Rd_, ~(GetConst(_Rs_) | GetConst(_Rt_)));
}

static void recSLT()
{
	// rd = rs < rt (SIGNED)

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RS_RT(SLT,  _Rd_, _Rs_, _Rt_);

	if (set_const)
		SetConst(_Rd_, (s32)GetConst(_Rs_) < (s32)GetConst(_Rt_));
}

static void recSLTU()
{
	// rd = rs < rt (UNSIGNED)

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RS_RT(SLTU, _Rd_, _Rs_, _Rt_);

	if (set_const)
		SetConst(_Rd_, GetConst(_Rs_) < GetConst(_Rt_));
}


#define REC_RTYPE_RD_RT_SA(insn, _rd_, _rt_, _sa_) \
do { \
	u32 rd = _rd_; \
	u32 rt = _rt_; \
	u32 sa = _sa_; \
	if (!rd) break; \
	SetUndef(_rd_); \
	u32 r1, r2; \
	if (rd == rt) { \
		if (!sa) break; \
		r1 = regMipsToHost(rd, REG_LOAD, REG_REGISTER); \
		r2 = r1; \
	} else { \
		r1 = regMipsToHost(rd, REG_FIND, REG_REGISTER); \
		r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER); \
	} \
	insn(r1, r2, sa); \
	regMipsChanged(rd); \
	regUnlock(r1); \
	regUnlock(r2); \
} while (0)

static void recSLL()
{
	// rd = rt << sa

	const bool set_const = IsConst(_Rt_);

#ifdef USE_MIPS32R2_ALU_OPCODE_CONVERSION
	if (!branch)
	{
		const u32 next_opcode = OPCODE_AT(pc);

		// If next opcode is SRA, see if we can convert this SLL,SRA sequence
		//  into a newer MIPS32r2 'SEB' or 'SEH' instruction instead.
		// Sequence we're looking for is:
		//   SLL   rd = rt << sa    (this opcode)
		//   SRA   rd = rt >> sa    (next opcode)
		// Where the rd of SLL is the same as the rd,rt of ANDI,
		//  and both shift amounts are equal and are either 24 or 16.

		if (_fOp_(next_opcode) == 0 && _fFunct_(next_opcode) == 0x03 &&  // SRA
		    (_Sa_ == 24 || _Sa_ == 16) && _Sa_ == _fSa_(next_opcode) &&
		    _Rd_ == _fRd_(next_opcode) && _Rd_ == _fRt_(next_opcode))
		{
			u32 r1, r2;
			if (_Rd_ == _Rt_) {
				r1 = regMipsToHost(_Rd_, REG_LOAD, REG_REGISTER);
				r2 = r1;
			} else {
				r1 = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);
				r2 = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
			}

			if (_Sa_ == 16) {
				SEH(r1, r2);
			} else {
				SEB(r1, r2);
			}

			regMipsChanged(_Rd_);
			regUnlock(r1);
			regUnlock(r2);

			// Propagate constness of result.
			if (set_const)
				SetConst(_Rd_, ((s32)(GetConst(_Rt_) << _Sa_) >> _Sa_));
			else
				SetUndef(_Rd_);

			// Skip the second opcode, disassembling it if disasm is enabled.
			DISASM_PSX(pc);
			DISASM_MSG("CONVERTING SLL,SRA SEQUENCE TO %s\n", _Sa_==16 ? "SEH" : "SEB");
			pc += 4;

			// Success.. we're done
			return;
		}

		// If next opcode is SRL, see if we can convert this SLL,SRL sequence
		//  into a newer MIPS32r2 'INS' instruction instead (inserting 0s).
		// Sequence we're looking for is:
		//   SLL   rd = rt << sa    (this opcode)
		//   SRL   rd = rt >> sa    (next opcode)
		// Where all the rd,rt of both opcodes are all the same and both
		//  shift amounts are equal.
		if ((_fOp_(next_opcode) == 0 && _fFunct_(next_opcode) == 0x2) &&  // SRL
		    _Sa_ != 0                                                 &&
		    _Sa_ == _fSa_(next_opcode)                                &&
		    _Rd_ == _Rt_                                              &&
		    _Rd_ == _fRd_(next_opcode)                                &&
		    _Rd_ == _fRt_(next_opcode))
		{
			u32 r1, r2;
			if (_Rd_ == _Rt_) {
				r1 = regMipsToHost(_Rd_, REG_LOAD, REG_REGISTER);
				r2 = r1;
			} else {
				r1 = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);
				r2 = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
			}

			INS(r1, 0, (32 - _Sa_), _Sa_);

			regMipsChanged(_Rd_);
			regUnlock(r1);
			regUnlock(r2);

			// Propagate constness of result.
			if (set_const)
				SetConst(_Rd_, (((u32)GetConst(_Rt_) << _Sa_) >> _Sa_));
			else
				SetUndef(_Rd_);

			// Skip the second opcode, disassembling it if disasm is enabled.
			DISASM_PSX(pc);
			DISASM_MSG("CONVERTING SLL,SRL SEQUENCE TO MIPS32R2 INS\n");
			pc += 4;

			// Success.. we're done
			return;
		}
	}
#endif // USE_MIPS32R2_ALU_OPCODE_CONVERSION

	REC_RTYPE_RD_RT_SA(SLL, _Rd_, _Rt_, _Sa_);

	if (set_const)
		SetConst(_Rd_, GetConst(_Rt_) << _Sa_);
}

static void recSRL()
{
	// rd = rt >> sa

	const bool set_const = IsConst(_Rt_);

#ifdef USE_MIPS32R2_ALU_OPCODE_CONVERSION
	if (!branch)
	{
		const u32 next_opcode = OPCODE_AT(pc);

		// If next opcode is ANDI, see if we can convert this SRL,ANDI sequence
		//  into a newer MIPS32r2 'EXT' instruction instead.
		// Sequence we're looking for is:
		//   SRL   rd = rt >> sa    (this opcode)
		//   ANDI  rt = rs & imm    (next opcode)
		// Where the rd of SRL is the same as the rs,rt of ANDI

		if (_fOp_(next_opcode) == 0x0c &&  // ANDI
		    _fImmU_(next_opcode) != 0  &&
		    _Rd_ == _fRs_(next_opcode) && _Rd_ == _fRt_(next_opcode))
		{
			// Determine if ANDI's imm is one less than a power of two,
			//  i.e. it is 0x1,0x3,0x7,0xf, ... 0x3fff,0x7fff, 0xffff,
			//  i.e., with no trailing or intervening 0s.
			//  Note that we've already checked that imm is not 0.
			const u32 imm = _fImmU_(next_opcode);
			if (((imm+1) & imm) == 0)
			{
				// Number of bits for 'ext' to extract: count trailing zeroes
				//  of imm+1, starting at LSB.
				const u32 num_bits = __builtin_ctz(imm+1);
				// Position to start extracting from is the SRL's shift-amount
				const u32 pos_to_extract = _Sa_;

				u32 r1, r2;
				if (_Rd_ == _Rt_) {
					r1 = regMipsToHost(_Rd_, REG_LOAD, REG_REGISTER);
					r2 = r1;
				} else {
					r1 = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);
					r2 = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
				}

				EXT(r1, r2, pos_to_extract, num_bits);

				regMipsChanged(_Rd_);
				regUnlock(r1);
				regUnlock(r2);

				// Propagate constness of result.
				if (set_const)
					SetConst(_Rd_, ((u32)GetConst(_Rt_) >> _Sa_) & _fImmU_(next_opcode));
				else
					SetUndef(_Rd_);

				// Skip the second opcode, disassembling it if disasm is enabled.
				DISASM_PSX(pc);
				DISASM_MSG("CONVERTING SRL,ANDI SEQUENCE TO MIPS32R2 EXT\n");
				pc += 4;

				// Success.. we're done
				return;
			}
		}

		// If next opcode is SLL, see if we can convert this SRL,SLL sequence
		//  into a newer MIPS32r2 'INS' instruction instead (inserting 0s).
		// Sequence we're looking for is:
		//   SRL   rd = rt >> sa    (this opcode)
		//   SLL   rd = rt << sa    (next opcode)
		// Where all the rd,rt of both opcodes are all the same and both
		//  shift amounts are equal.
		if ((_fOp_(next_opcode) == 0 && _fFunct_(next_opcode) == 0) &&  // SLL
		    _Sa_ != 0                                               &&
		    _Sa_ == _fSa_(next_opcode)                              &&
		    _Rd_ == _Rt_                                            &&
		    _Rd_ == _fRd_(next_opcode)                              &&
		    _fRd_(next_opcode) == _fRt_(next_opcode))
		{
			u32 r1, r2;
			if (_Rd_ == _Rt_) {
				r1 = regMipsToHost(_Rd_, REG_LOAD, REG_REGISTER);
				r2 = r1;
			} else {
				r1 = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);
				r2 = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
			}

			INS(r1, 0, 0, _Sa_);

			regMipsChanged(_Rd_);
			regUnlock(r1);
			regUnlock(r2);

			// Propagate constness of result.
			if (set_const)
				SetConst(_Rd_, ((u32)GetConst(_Rt_) >> _Sa_) << _Sa_);
			else
				SetUndef(_Rd_);

			// Skip the second opcode, disassembling it if disasm is enabled.
			DISASM_PSX(pc);
			DISASM_MSG("CONVERTING SRL,SLL SEQUENCE TO MIPS32R2 INS\n");
			pc += 4;

			// Success.. we're done
			return;
		}
	}
#endif // USE_MIPS32R2_ALU_OPCODE_CONVERSION

	REC_RTYPE_RD_RT_SA(SRL, _Rd_, _Rt_, _Sa_);

	if (set_const)
		SetConst(_Rd_, (u32)GetConst(_Rt_) >> _Sa_);
}


static void recSRA()
{
	// rd = (s32)rt >> sa

	const bool set_const = IsConst(_Rt_);

#ifdef USE_MIPS32R2_ALU_OPCODE_CONVERSION
	if (!branch)
	{
		const u32 next_opcode = OPCODE_AT(pc);

		// If next opcode is SLL, see if we can convert this SRA,SLL sequence
		//  into a newer MIPS32r2 'INS' instruction instead (inserting 0s).
		// Sequence we're looking for is:
		//   SRA   rd = rt >> sa    (this opcode)
		//   SLL   rd = rt << sa    (next opcode)
		// Where all the rd,rt of both opcodes are all the same and both
		//  shift amounts are equal.
		if ((_fOp_(next_opcode) == 0 && _fFunct_(next_opcode) == 0) &&  // SLL
		    _Sa_ != 0                                               &&
		    _Sa_ == _fSa_(next_opcode)                              &&
		    _Rd_ == _Rt_                                            &&
		    _Rd_ == _fRd_(next_opcode)                              &&
		    _fRd_(next_opcode) == _fRt_(next_opcode))
		{
			u32 r1, r2;
			if (_Rd_ == _Rt_) {
				r1 = regMipsToHost(_Rd_, REG_LOAD, REG_REGISTER);
				r2 = r1;
			} else {
				r1 = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);
				r2 = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
			}

			INS(r1, 0, 0, _Sa_);

			regMipsChanged(_Rd_);
			regUnlock(r1);
			regUnlock(r2);

			// Propagate constness of result.
			if (set_const)
				SetConst(_Rd_, (((s32)GetConst(_Rt_) >> _Sa_) << _Sa_));
			else
				SetUndef(_Rd_);

			// Skip the second opcode, disassembling it if disasm is enabled.
			DISASM_PSX(pc);
			DISASM_MSG("CONVERTING SRA,SLL SEQUENCE TO MIPS32R2 INS\n");
			pc += 4;

			// Success.. we're done
			return;
		}
	}
#endif // USE_MIPS32R2_ALU_OPCODE_CONVERSION

	REC_RTYPE_RD_RT_SA(SRA, _Rd_, _Rt_, _Sa_);

	if (set_const)
		SetConst(_Rd_, (s32)GetConst(_Rt_) >> _Sa_);
}


#define REC_RTYPE_RD_RT_RS(insn, _rd_, _rt_, _rs_) \
do { \
	u32 rd = _rd_; \
	u32 rt = _rt_; \
	u32 rs = _rs_; \
	if (!rd) break; \
	SetUndef(_rd_); \
	u32 r1, r2, r3; \
	if (rd == rt) { \
		if (!rs) break; \
		r1 = regMipsToHost(rd, REG_LOAD, REG_REGISTER); \
		r2 = r1; \
		r3 = (rs == rd ? r1 : regMipsToHost(rs, REG_LOAD, REG_REGISTER)); \
	} else if (rd == rs) { \
		r1 = regMipsToHost(rd, REG_LOAD, REG_REGISTER); \
		r3 = r1; \
		r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER); \
	} else { \
		r1 = regMipsToHost(rd, REG_FIND, REG_REGISTER); \
		r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER); \
		r3 = (rs == rt) ? r2 : regMipsToHost(rs, REG_LOAD, REG_REGISTER); \
	} \
	insn(r1, r2, r3); \
	regMipsChanged(rd); \
	regUnlock(r1); \
	regUnlock(r2); \
	regUnlock(r3); \
} while (0)

static void recSLLV()
{
	// rd = rt << rs

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RT_RS(SLLV, _Rd_, _Rt_, _Rs_);

	if (set_const)
		SetConst(_Rd_, GetConst(_Rt_) << GetConst(_Rs_));
}

static void recSRLV()
{
	// rd = (u32)rt >> rs

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RT_RS(SRLV, _Rd_, _Rt_, _Rs_);

	if (set_const)
		SetConst(_Rd_, GetConst(_Rt_) >> GetConst(_Rs_));
}

static void recSRAV()
{
	// rd = (s32)rt >> rs

	const bool set_const = IsConst(_Rs_) && IsConst(_Rt_);

	REC_RTYPE_RD_RT_RS(SRAV, _Rd_, _Rt_, _Rs_);

	if (set_const)
		SetConst(_Rd_, (s32)GetConst(_Rt_) >> GetConst(_Rs_));
}
