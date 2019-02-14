/******************************************************************************
 * IMPORTANT: The following host registers have unique usage restrictions.    *
 *            See notes in mips_codegen.h for full details.                   *
 *  MIPSREG_AT, MIPSREG_V0, MIPSREG_V1, MIPSREG_RA                            *
 *****************************************************************************/

/***********************************************
 * Options that can be disabled for debugging: *
 ***********************************************/

/* If your platform gives the same results as PS1 did in the HI register
 *   when dividing by zero, define this to eliminate some fixup code.
 *   Ingenic MIPS32 jz-series CPUs do. See recDIV(),recDIVU().
 */
#if defined(GCW_ZERO)
	#define OMIT_DIV_BY_ZERO_HI_FIXUP
#endif

/* Detect GCC compiler-generated div-by-zero check+exception, indicating that
 * emitting div-by-zero results fixup is pointless. See recDIV(),recDIVU().
 */
#define OMIT_DIV_BY_ZERO_FIXUP_IF_EXCEPTION_SEQUENCE_FOUND

/* Detect and optimize all-const multiplies. See recMULT(),recMULTU(). */
#define USE_CONST_MULT_OPTIMIZATIONS

/* Detect and optimize all-const divides. See recDIV(),recDIVU(). */
#define USE_CONST_DIV_OPTIMIZATIONS

#ifdef HAVE_MIPS32_3OP_MUL
/* Try to convert MULT/MULTU to modern 3-op MUL. See convertMultiplyTo3Op(). */
	#define USE_3OP_MUL_OPTIMIZATIONS
/* Convert more MULT/MULTU ops to MUL by considering JAL,JR,JALR as barriers. */
	#define USE_3OP_MUL_JUMP_OPTIMIZATIONS
#endif // HAVE_MIPS32_3OP_MUL

static uint8_t convertMultiplyTo3Op();


static void recMULT()
{
// Lo/Hi = Rs * Rt (signed)

#ifdef USE_CONST_MULT_OPTIMIZATIONS
	// First, check if either or both operands are const values
	uint8_t rs_const = IsConst(_Rs_);
	uint8_t rt_const = IsConst(_Rt_);

	if (rs_const || rt_const)
	{
		// Check rs_const or rt_const before using either value here!
		s32 rs_val = GetConst(_Rs_);
		s32 rt_val = GetConst(_Rt_);

		uint8_t const_res = 0;
		s32 lo_res = 0;
		s32 hi_res = 0;

		if ((rs_const && !rs_val) || (rt_const && !rt_val)) {
			// If either operand is 0, both LO/HI result is 0
			const_res = 1;
			lo_res = 0;
			hi_res = 0;
		} else if (rs_const && rt_const) {
			// If both operands are known-const, compute result statically
			s64 res = (s64)rs_val * (s64)rt_val;
			const_res = 1;
			lo_res = (s32)res;
			hi_res = (s32)(res >> 32);
		} else if ((rs_const && (abs(rs_val) == 1)) || (rt_const && (abs(rt_val) == 1))) {
			// If one of the operands is known to be const-val '+/-1', result is identity
			//  (with negation if val is -1)

			u32 ident_reg_psx = rs_const ? _Rt_ : _Rs_;
			u32 ident_reg = regMipsToHost(ident_reg_psx, REG_LOAD, REG_REGISTER);

			u32 work_reg = ident_reg;
			uint8_t negate_res = rs_const ? (rs_val < 0) : (rt_val < 0);

			if (negate_res) {
				SUBU(TEMP_1, 0, work_reg);
				work_reg = TEMP_1;
			}

			SW(work_reg, PERM_REG_1, offGPR(32)); // LO
			// Upper word is all 0s or 1s depending on sign of LO result
			SRA(TEMP_1, work_reg, 31);
			SW(TEMP_1, PERM_REG_1, offGPR(33));   // HI

			regUnlock(ident_reg);

			// We're done
			return;
		} else {
			// If one of the operands is a const power-of-two, we can get result by shifting

			// Determine which operand is const power-of-two value, if any
			uint8_t rs_pot = rs_const && (rs_val != 0x80000000) && ((abs(rs_val) & (abs(rs_val) - 1)) == 0);
			uint8_t rt_pot = rt_const && (rt_val != 0x80000000) && ((abs(rt_val) & (abs(rt_val) - 1)) == 0);

			if (rs_pot || rt_pot) {
				u32 npot_reg_psx = rs_pot ? _Rt_ : _Rs_;
				u32 npot_reg = regMipsToHost(npot_reg_psx, REG_LOAD, REG_REGISTER);

				// Count trailing 0s of const power-of-two operand to get left-shift amount
				u32 pot_val = rs_pot ? (u32)abs(rs_val) : (u32)abs(rt_val);
				u32 shift_amt = __builtin_ctz(pot_val);

				u32 work_reg = npot_reg;
				uint8_t negate_res = rs_pot ? (rs_val < 0) : (rt_val < 0);
				if (negate_res) {
					SUBU(TEMP_2, 0, npot_reg);
					work_reg = TEMP_2;
				}

				SLL(TEMP_1, work_reg, shift_amt);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
				// Sign-extend here when computing upper word of result
				SRA(TEMP_1, work_reg, (32 - shift_amt));
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI

				regUnlock(npot_reg);

				// We're done
				return;
			}
		}

		if (const_res) {
			if (lo_res) {
				LI32(TEMP_1, (u32)lo_res);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			} else {
				SW(0, PERM_REG_1, offGPR(32)); // LO
			}

			if (hi_res) {
				LI32(TEMP_1, (u32)hi_res);
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI
			} else {
				SW(0, PERM_REG_1, offGPR(33)); // HI
			}

			// We're done
			return;
		}

		// We couldn't emit faster code, so fall-through here
	}
#endif // USE_CONST_MULT_OPTIMIZATIONS

#ifdef USE_3OP_MUL_OPTIMIZATIONS
	if (convertMultiplyTo3Op()) {
		// MULT/MULTU was converted to modern 3-op MUL.
		// Furthermore, no code will be emitted for next MFLO.
		return;
	}
#endif

	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	MULT(rs, rt);
	MFLO(TEMP_1);
	MFHI(TEMP_2);
	SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
	SW(TEMP_2, PERM_REG_1, offGPR(33)); // HI

	regUnlock(rs);
	regUnlock(rt);
}


static void recMULTU()
{
// Lo/Hi = Rs * Rt (unsigned)

	// First, check if either or both operands are const values
#ifdef USE_CONST_MULT_OPTIMIZATIONS
	uint8_t rs_const = IsConst(_Rs_);
	uint8_t rt_const = IsConst(_Rt_);

	if (rs_const || rt_const)
	{
		// Check rs_const or rt_const before using either value here!
		u32 rs_val = GetConst(_Rs_);
		u32 rt_val = GetConst(_Rt_);

		uint8_t const_res = 0;
		u32 lo_res = 0;
		u32 hi_res = 0;

		if ((rs_const && !rs_val) || (rt_const && !rt_val)) {
			// If either operand is 0, both LO/HI result is 0
			const_res = 1;
			lo_res = 0;
			hi_res = 0;
		} else if (rs_const && rt_const) {
			// If both operands are known-const, compute result statically
			u64 res = (u64)rs_val * (u64)rt_val;
			const_res = 1;
			lo_res = (u32)res;
			hi_res = (u32)(res >> 32);
		} else if ((rs_const && (rs_val == 1)) || (rt_const && (rt_val == 1))) {
			// If one of the operands is known to be const-val '1', result is identity
			u32 ident_reg_psx = rs_const ? _Rt_ : _Rs_;
			u32 ident_reg = regMipsToHost(ident_reg_psx, REG_LOAD, REG_REGISTER);

			SW(0, PERM_REG_1, offGPR(33));         // HI
			SW(ident_reg, PERM_REG_1, offGPR(32)); // LO

			regUnlock(ident_reg);

			// We're done
			return;
		} else {
			// If one of the operands is a const power-of-two, we can get result by shifting

			// Determine which operand is const power-of-two value, if any
			uint8_t rs_pot = rs_const && ((rs_val & (rs_val - 1)) == 0);
			uint8_t rt_pot = rt_const && ((rt_val & (rt_val - 1)) == 0);

			if (rs_pot || rt_pot) {
				u32 npot_reg_psx = rs_pot ? _Rt_ : _Rs_;
				u32 npot_reg = regMipsToHost(npot_reg_psx, REG_LOAD, REG_REGISTER);

				// Count trailing 0s of const power-of-two operand to get left-shift amount
				u32 pot_val = rs_pot ? rs_val : rt_val;
				u32 shift_amt = __builtin_ctz(pot_val);

				SLL(TEMP_1, npot_reg, shift_amt);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
				SRL(TEMP_1, npot_reg, (32 - shift_amt));
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI

				regUnlock(npot_reg);

				// We're done
				return;
			}
		}

		if (const_res) {
			if (lo_res) {
				LI32(TEMP_1, lo_res);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			} else {
				SW(0, PERM_REG_1, offGPR(32)); // LO
			}

			if (hi_res) {
				LI32(TEMP_1, hi_res);
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI
			} else {
				SW(0, PERM_REG_1, offGPR(33)); // HI
			}

			// We're done
			return;
		}

		// We couldn't emit faster code, so fall-through here
	}
#endif // USE_CONST_MULT_OPTIMIZATIONS

#ifdef USE_3OP_MUL_OPTIMIZATIONS
	if (convertMultiplyTo3Op()) {
		// MULT/MULTU was converted to modern 3-op MUL.
		// Furthermore, no code will be emitted for next MFLO.
		return;
	}
#endif

	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	MULTU(rs, rt);
	MFLO(TEMP_1);
	MFHI(TEMP_2);
	SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
	SW(TEMP_2, PERM_REG_1, offGPR(33)); // HI

	regUnlock(rs);
	regUnlock(rt);
}


static void recDIV()
{
// Hi, Lo = rs / rt signed

#ifdef USE_CONST_DIV_OPTIMIZATIONS
	uint8_t rs_const = IsConst(_Rs_);
	uint8_t rt_const = IsConst(_Rt_);

	// First, check if divisor operand is const value
	if (rt_const)
	{
		// Check rs_const before using rs_val value here!
		u32 rs_val = GetConst(_Rs_);
		u32 rt_val = GetConst(_Rt_);

		if (!rt_val) {
			// If divisor operand is const 0:
			//  LO result is -1 if dividend is positive or zero,
			//               +1 if dividend is negative
			//  HI result is Rs val
			u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

			ADDIU(TEMP_2, 0, -1);
			SLT(TEMP_1, rs, 0);           // TEMP_1 = dividend < 0
			MOVN(TEMP_1, TEMP_2, TEMP_1); // if (TEMP_1 != 0) TEMP_1 = TEMP_2
			SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			SW(rs, PERM_REG_1, offGPR(33));     // HI

			regUnlock(rs);

			// We're done
			return;
		} else if (rt_val == 1) {
			// If divisor is const-val '1', result is identity
			u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

			SW(0, PERM_REG_1, offGPR(33));  // HI
			SW(rs, PERM_REG_1, offGPR(32)); // LO

			regUnlock(rs);

			// We're done
			return;
		} else if (rs_const) {
			// If both operands are known-const, compute result statically
			u32 lo_res = rs_val / rt_val;
			u32 hi_res = rs_val % rt_val;

			if (lo_res) {
				LI32(TEMP_1, lo_res);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			} else {
				SW(0, PERM_REG_1, offGPR(32)); // LO
			}

			if (hi_res) {
				LI32(TEMP_1, hi_res);
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI
			} else {
				SW(0, PERM_REG_1, offGPR(33)); // HI
			}

			// We're done
			return;
		} else {
			// If divisor is a const power-of-two, we can get result by shifting
			uint8_t rt_pot = (rt_val != 0x80000000) && ((abs(rt_val) & (abs(rt_val) - 1)) == 0);

			if (rt_pot) {
				u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

				// Count trailing 0s of const power-of-two divisor to get right-shift amount
				u32 pot_val = (u32)abs(rt_val);
				u32 shift_amt = __builtin_ctz(pot_val);

				u32 work_reg = rs;
				uint8_t negate_res = (rt_val < 0);

				if (negate_res) {
					SUBU(TEMP_2, 0, rs);
					work_reg = TEMP_2;
				}

				SRA(TEMP_1, work_reg, shift_amt);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO

				// Subtract one from pot divisor to get remainder modulo mask
				if ((pot_val-1) > 0xffff) {
					LI32(TEMP_1, (pot_val-1));
					AND(TEMP_1, rs, TEMP_1);
				} else {
					ANDI(TEMP_1, rs, (pot_val-1));
				}
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI

				regUnlock(rs);

				// We're done
				return;
			}
		}

		// We couldn't emit faster code, so fall-through here
	}
#endif // USE_CONST_DIV_OPTIMIZATIONS

	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	// Test if divisor is 0, emulating correct results for PS1 CPU.
	// NOTE: we don't bother checking for signed division overflow (the
	//       last entry in this list), as it seems to work the same on
	//       modern MIPS32 CPUs like the jz4770 of GCW Zero. Unfortunately,
	//       division-by-zero results only match PS1 in the 'hi' reg.
	//  Rs              Rt       Hi/Remainder  Lo/Result
	//  0..+7FFFFFFFh    0   -->  Rs           -1
	//  -80000000h..-1   0   -->  Rs           +1
	//  -80000000h      -1   -->  0           -80000000h

	uint8_t omit_div_by_zero_fixup = 0;

	if (IsConst(_Rt_) && GetConst(_Rt_) != 0) {
		// If divisor is known-const val and isn't 0, no need to fixup
		omit_div_by_zero_fixup = 1;
	} else if (!branch) {
#ifdef OMIT_DIV_BY_ZERO_FIXUP_IF_EXCEPTION_SEQUENCE_FOUND
		// If we're not inside a branch-delay slot, we scan ahead for a
		//  PS1 GCC auto-generated code sequence that was inserted in most
		//  binaries after a DIV/DIVU instruction. The sequence checked for
		//  div-by-zero (and signed overflow, for DIV) and issued an
		//  exception that went unhandled by PS1 BIOS, which would crash.
		//  If found, don't bother to emulate PS1 div-by-zero result.
		//
		// Sequence is sometimes preceeded by one or two MFHI/MFLO opcodes.
		u32 code_loc = pc;
		code_loc += 4 * rec_scan_for_MFHI_MFLO_sequence(code_loc);
		omit_div_by_zero_fixup = (rec_scan_for_div_by_zero_check_sequence(code_loc) > 0);
#endif
	}

	if (omit_div_by_zero_fixup) {
		DIV(rs, rt);
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);
	} else {
		DIV(rs, rt);
		ADDIU(MIPSREG_A1, 0, -1);
		SLT(TEMP_3, rs, 0);        // TEMP_3 = (rs < 0 ? 1 : 0)
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);

		// If divisor was 0, set LO result (quotient) to 1 if dividend was < 0
		// If divisor was 0, set LO result (quotient) to -1 if dividend was >= 0
		MOVN(MIPSREG_A0, TEMP_3, TEMP_3);      // if (TEMP_3 != 0) then MIPSREG_A1 = TEMP_3
		MOVZ(MIPSREG_A0, MIPSREG_A1, TEMP_3);  // if (TEMP_3 == 0) then MIPSREG_A1 = MIPSREG_A0
		MOVZ(TEMP_1, MIPSREG_A0, rt);          // if (rt == 0) then TEMP_1 = MIPSREG_A0

#ifndef OMIT_DIV_BY_ZERO_HI_FIXUP
		// If divisor was 0, set HI result (remainder) to rs
		MOVZ(TEMP_2, rs, rt);
#endif
	}

	SW(TEMP_1, PERM_REG_1, offGPR(32));
	SW(TEMP_2, PERM_REG_1, offGPR(33));
	regUnlock(rs);
	regUnlock(rt);
}


static void recDIVU()
{
// Hi, Lo = rs / rt unsigned

	// First, check if divisor operand is const value
#ifdef USE_CONST_DIV_OPTIMIZATIONS
	uint8_t rt_const = IsConst(_Rt_);

	if (rt_const)
	{
		uint8_t rs_const = IsConst(_Rs_);

		// Check rs_const before using rs_val value here!
		u32 rs_val = GetConst(_Rs_);
		u32 rt_val = GetConst(_Rt_);

		if (!rt_val) {
			// If divisor operand is const 0:
			//  LO result is 0xffff_ffff
			//  HI result is Rs val
			u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

			ADDIU(TEMP_1, 0, -1);
			SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			SW(rs, PERM_REG_1, offGPR(33));     // HI

			regUnlock(rs);

			// We're done
			return;
		} else if (rt_val == 1) {
			// If divisor is const-val '1', result is identity
			u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

			SW(0, PERM_REG_1, offGPR(33));  // HI
			SW(rs, PERM_REG_1, offGPR(32)); // LO

			regUnlock(rs);

			// We're done
			return;
		} else if (rs_const) {
			// If both operands are known-const, compute result statically
			u32 lo_res = rs_val / rt_val;
			u32 hi_res = rs_val % rt_val;

			if (lo_res) {
				LI32(TEMP_1, lo_res);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO
			} else {
				SW(0, PERM_REG_1, offGPR(32)); // LO
			}

			if (hi_res) {
				LI32(TEMP_1, hi_res);
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI
			} else {
				SW(0, PERM_REG_1, offGPR(33)); // HI
			}

			// We're done
			return;
		} else {
			// If divisor is a const power-of-two, we can get result by shifting
			uint8_t rt_pot = (rt_val & (rt_val - 1)) == 0;

			if (rt_pot) {
				u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

				// Count trailing 0s of const power-of-two divisor to get right-shift amount
				u32 pot_val = rt_val;
				u32 shift_amt = __builtin_ctz(pot_val);

				SRL(TEMP_1, rs, shift_amt);
				SW(TEMP_1, PERM_REG_1, offGPR(32)); // LO

				// Subtract one from pot divisor to get remainder modulo mask
				if ((pot_val-1) > 0xffff) {
					LI32(TEMP_1, (pot_val-1));
					AND(TEMP_1, rs, TEMP_1);
				} else {
					ANDI(TEMP_1, rs, (pot_val-1));
				}
				SW(TEMP_1, PERM_REG_1, offGPR(33)); // HI

				regUnlock(rs);

				// We're done
				return;
			}
		}

		// We couldn't emit faster code, so fall-through here
	}
#endif // USE_CONST_DIV_OPTIMIZATIONS

	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	// Test if divisor is 0, emulating correct results for PS1 CPU.
	//  Rs              Rt       Hi/Remainder  Lo/Result
	//  0..FFFFFFFFh    0   -->  Rs            FFFFFFFFh

	uint8_t omit_div_by_zero_fixup = 0;

	if (IsConst(_Rt_) && GetConst(_Rt_) != 0) {
		// If divisor is known-const val and isn't 0, no need to fixup
		omit_div_by_zero_fixup = 1;
	} else if (!branch) {
#ifdef OMIT_DIV_BY_ZERO_FIXUP_IF_EXCEPTION_SEQUENCE_FOUND
		// See notes in recDIV() emitter
		u32 code_loc = pc;
		code_loc += 4 * rec_scan_for_MFHI_MFLO_sequence(code_loc);
		omit_div_by_zero_fixup = (rec_scan_for_div_by_zero_check_sequence(code_loc) > 0);
#endif
	}

	if (omit_div_by_zero_fixup) {
		DIVU(rs, rt);
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);
	} else {
		DIVU(rs, rt);
		ADDIU(TEMP_3, 0, -1);
		MFLO(TEMP_1);              // NOTE: Hi/Lo can't be cached for now, so spill them
		MFHI(TEMP_2);

		// If divisor was 0, set LO result (quotient) to 0xffff_ffff
		MOVZ(TEMP_1, TEMP_3, rt);  // if (rt == 0) then TEMP_1 = TEMP_3

#ifndef OMIT_DIV_BY_ZERO_HI_FIXUP
		// If divisor was 0, set HI result (remainder) to rs
		MOVZ(TEMP_2, rs, rt);
#endif
	}

	SW(TEMP_1, PERM_REG_1, offGPR(32));
	SW(TEMP_2, PERM_REG_1, offGPR(33));
	regUnlock(rs);
	regUnlock(rt);
}

static void recMFHI()
{
// Rd = Hi
	if (!_Rd_) return;
	SetUndef(_Rd_);
	u32 rd = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);

	LW(rd, PERM_REG_1, offGPR(33));
	regMipsChanged(_Rd_);
	regUnlock(rd);
}

static void recMTHI()
{
// Hi = Rs
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	SW(rs, PERM_REG_1, offGPR(33));
	regUnlock(rs);
}


static void recMFLO()
{
// Rd = Lo

#ifdef USE_3OP_MUL_OPTIMIZATIONS
	if (skip_emitting_next_mflo) {
		// A prior MULT/MULTU was converted to 3-op MUL. We emit nothing
		//  for this MFLO.
		skip_emitting_next_mflo = 0;
		return;
	}
#endif

	if (!_Rd_) return;

	SetUndef(_Rd_);
	u32 rd = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);

	LW(rd, PERM_REG_1, offGPR(32));
	regMipsChanged(_Rd_);
	regUnlock(rd);
}


static void recMTLO()
{
// Lo = Rs
	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	SW(rs, PERM_REG_1, offGPR(32));
	regUnlock(rs);
}


#ifdef USE_3OP_MUL_OPTIMIZATIONS
/*  recMULT(),recMULTU() call this to try to convert a MULT/MULTU to a modern
 * 3-op MUL. The 32-bit result goes to dest reg of the original MFLO (if found).
 *  Analysis must find a clear path from the multiply to the MFLO. The GPR
 * written by the MFLO must not be accessed between the two ops. It must be
 * clear that the HI result is never read, the LO result is only read once,
 * and the HI/LO contents aren't used between or after.
 *  Until the dynarec gets general dataflow analysis, we use this limited
 * peephole optimizer. On average, we succeed ~30-50% of the time. If
 * 'USE_3OP_MUL_JUMP_OPTIMIZATIONS' is defined, this rate is boosted to ~40-70%.
 *
 * Returns: 1 if 3-op MUL was emitted, 0 if conversion failed.
 *
 * NOTE: If conversion succeeds, flag 'skip_emitting_next_mflo' is set, telling
 *       recMFLO() to emit nothing at its next call.
 */
static uint8_t convertMultiplyTo3Op()
{
	// Max number of opcodes to scan ahead in each stage/path
	const int scan_max = 16;

	// Bitfield indicating which GPR regs have been read/written between the
	// multiply and the MFLO instruction.
	u32 gpr_accesses = 0;

	uint8_t convertible = 1;

	// 'PC' start address is the instruction after the initial multiply
	u32 PC = pc;

	/****************************************************************
	 * STAGE 1: Preliminary work needed if multiply is in a BD slot *
	 ****************************************************************/
	if (branch)
	{
		// NOTE: the branch/jump is at PC-8 and MULT/MULTU in BD slot at PC-4.

		// To make things simpler, don't convert multplies lying in BD slot of a
		// jump, backwards-branch, large forwards-branch, or zero-length branch.

		if (opcodeIsJump(OPCODE_AT(PC-8)))
			return 0;

		const int branch_imm = _fImm_(OPCODE_AT(PC-8));
		if (branch_imm <= 0 || branch_imm > 8)
			return 0;

		// OK, the branch before the multiply is a small forwards-branch:
		// Scan the branch-not-taken path to ensure there are no accesses
		// of HI/LO, or any branches/jumps/syscall/etc before branch target.
		//  ALSO: Track which GPRs are accessed in this path. We'll continue
		//        tracking these through the next step.

		// 'BPC' is the beginning of the branch-taken path, lying close ahead.
		const u32 BPC = (PC-4) + (branch_imm * 4);

		while (PC < BPC)
		{
			const u32 opcode = OPCODE_AT(PC);
			PC += 4;

			// Skip any NOPs
			if (opcode == 0)
				continue;

			// Is opcode a branch, jump, SYSCALL, or HLE instruction?
			if ( _fOp_(opcode) == 0x3b ||  /* HLE */
			    (_fOp_(opcode) == 0 && _fFunct_(opcode) == 0xc) ||  /* SYSCALL */
			     opcodeIsBranchOrJump(opcode) )
			{
				convertible = 0;
				break;
			}

			// Bits 32,33 of u64 retval are LO,HI respectively. Lower bits are GPRs.
			const u64 op_reg_accesses = opcodeGetReads(opcode) | opcodeGetWrites(opcode);

			// Ensure there are no accesses of HI/LO in the branch-not-taken path.
			if ((op_reg_accesses >> 32) & 3) {
				convertible = 0;
				break;
			}

			gpr_accesses |= (u32)op_reg_accesses;
		}

		// 'PC' is left set to the branch target pc: proceed to next stage
	}

	if (!convertible)
		return 0;

	/*****************************************
	 * STAGE 2: Scan for an MFLO instruction *
	 *****************************************/

	//  Scan ahead for the MFLO and ensure no other instructions access HI/LO
	// inbetween. Keep a running union of all GPR accesses so we know that the
	// MFLO's dest reg is also not accessed between the multiply and MFLO.
	//  To keep things simpler and fast, if we come to a jump/branch, we'll give
	// up unless the MFLO is in its BD slot.

	uint8_t in_bd_slot = 0;
	u32  rd_of_mflo = 0;

	int left = scan_max;
	while (left-- > 0)
	{
		const u32 opcode = OPCODE_AT(PC);
		PC += 4;

		// Skip any NOPs
		if (opcode == 0)
			continue;

		// Is opcode MFLO?
		if (_fOp_(opcode) == 0 && _fFunct_(opcode) == 0x12) {  /* MFLO */
			rd_of_mflo = _fRd_(opcode);

			//  For the multiply to be convertible to a 3-op MUL, the dest reg of
			// the MFLO must not be accessed between the multiply and the MFLO.
			if (gpr_accesses & (1 << rd_of_mflo))
				convertible = 0;

			break;
		}

		// Bits 32,33 of u64 retval are LO,HI respectively. Lower bits are GPRs.
		const u64 op_reg_accesses = opcodeGetReads(opcode) | opcodeGetWrites(opcode);

		// Ensure there are no reads or writes of HI/LO whatsoever before MFLO.
		if ((op_reg_accesses >> 32) & 3) {
			convertible = 0;
			break;
		}

		// Give up scanning at a SYSCALL or HLE instruction.
		if ( _fOp_(opcode) == 0x3b || /* HLE */
		     (_fOp_(opcode) == 0 && _fFunct_(opcode) == 0xc) ) /* SYSCALL */
		{
			convertible = 0;
			break;
		}

		if (opcodeIsBranchOrJump(opcode)) {
			// Branch/jump in BD slot? Give up.
			if (in_bd_slot) {
				convertible = 0;
				break;
			}

			// Scan just the BD slot for an MFLO and stop.
			in_bd_slot = 1;
			left = 1;
		}

		gpr_accesses |= (u32)op_reg_accesses;
	}

	// Give up if we haven't found MFLO, or MFLO's dest reg was
	//  accessed between the multiply and the MFLO.
	if (!rd_of_mflo || !convertible)
		return 0;

	// NOTE: We leave PC set to the instruction after the MFLO we found.

	/********************************************************
	 * STAGE 3: Scan for a HI+LO overwrite or a branch/jump *
	 ********************************************************/

	//  Now, we must scan after the MFLO for a MULT/MULTU/DIV/DIVU, while
	// ensuring that the HI/LO results of the initial multiply aren't used
	// after the MFLO we found. To make things simpler, we merely check for
	// any HI/LO access while scanning for the mult/div, and give up if found.
	//  If we encounter a branch/jump, or the MFLO we found in last stage was
	// in a BD slot, we stop and proceed to the next stage.

	uint8_t found_hi_lo_overwrite = 0;

	if (!in_bd_slot)
	{
		left = scan_max;
		while (left-- > 0)
		{
			const u32 opcode = OPCODE_AT(PC);
			PC += 4;

			// Skip any NOPs
			if (opcode == 0)
				continue;

			// Bits 32,33 of u64 retval are LO,HI respectively. Lower bits are GPRs.
			const u64 op_reg_reads    = opcodeGetReads(opcode);
			const u64 op_reg_writes   = opcodeGetWrites(opcode);
			const u64 op_reg_accesses = op_reg_reads | op_reg_writes;

			// Check for a HI and LO overwrite (MULT,MULTU,DIV,DIVU)
			if (((op_reg_writes >> 32) & 3) == 3) {
				found_hi_lo_overwrite = 1;
				break;
			}

			// Check for a HI or LO access (only *after* overwrite check above)
			if (((op_reg_accesses >> 32) & 3)) {
				convertible = 0;
				break;
			}

			// Give up scanning at a SYSCALL or HLE instruction.
			if ( _fOp_(opcode) == 0x3b || /* HLE */
			    (_fOp_(opcode) == 0 && _fFunct_(opcode) == 0xc) ) /* SYSCALL */
			{
				convertible = 0;
				break;
			}

			// Stop at a branch/jump.
			if (opcodeIsBranchOrJump(opcode))
			{
				// Branch/jump in BD slot? Give up.
				if (in_bd_slot) {
					convertible = 0;
					break;
				}

#ifdef USE_3OP_MUL_JUMP_OPTIMIZATIONS
				//  This boosts conversion success rate:
				//  We consider a JAL/JR/JALR as a barrier that indicates HI/LO
				// contents are no longer used. Of course, this is only after
				// the MFLO has been found and has met our requirements.
				//  Theoretically, tricky PS1 ASM routines could want HI/LO
				// values even after a JAL/JR/JALR. But it seems quite unlikely
				// that we'd run afoul of them here, given how strict we are
				// otherwise.. if they even exist (cue ominous music).
				//
				// NOTE: 'J' is not included: simple jumps are analyzed with the
				//       same scrutiny as branches. Furthermore, the BD slot of
				//       *any* jump/branch is always checked for a HI/LO access.

				if ( _fOp_(opcode) == 0x3 ||  /* JAL */
				    (_fOp_(opcode) == 0 && (_fFunct_(opcode) == 0x8 || _fFunct_(opcode) == 0x9)))  /* JR, JALR */
				{
					found_hi_lo_overwrite = 1;

					// NOTE: we still scan the BD slot for a HI/LO access
				}
#endif // USE_3OP_MUL_JUMP_OPTIMIZATIONS

				// Scan just the BD slot and stop.
				in_bd_slot = 1;
				left = 1;
			}
		}
	}

	if (!convertible)
		return 0;

	//  NOTE: We leave PC set to the instruction after the last one analyzed.

	/*****************************************************************
	 * STAGE 4: Scan for a HI/LO overwrite along branch/jump path(s) *
	 *****************************************************************/

	//  If we haven't found a HI/LO overwrite yet, and are now at a BD slot,
	// we analyze the code path(s) beyond the branch/jump to find one.
	// If we find yet another branch/jump, we'll give up.

	if (in_bd_slot && !found_hi_lo_overwrite)
	{
		int num_paths = 2;
		uint8_t path_has_hi_lo_overwrite[2] = { 0, 0 };

		// Begin/end PCs of each codepath's scan range
		u32 path_start[2];
		u32 path_end[2];

		// Opcode of branch or jump instruction before the BD slot
		const u32 bj_opcode = OPCODE_AT(PC-8);

		if (opcodeIsJump(bj_opcode))
		{
			if (_fOp_(bj_opcode) == 0x2 || _fOp_(bj_opcode) == 0x3) {  /* J, JAL */
				// Note that the upper 4 bits of target PC come from BD slot's PC
				path_start[0] = (_fTarget_(bj_opcode) * 4) | ((PC-4) & 0xf0000000);
				path_end[0] = path_start[0] + scan_max*4;
				num_paths = 1;
			} else {
				// Give up on a JR, JALR: target PC is unknown
				convertible = 0;
			}
		} else
		{
			// Opcode is branch

			const int branch_imm = _fImm_(bj_opcode);

			if (((bj_opcode >> 16) == 0x1000) || ((bj_opcode >> 16) == 0x0411)) {  /* B, BAL */
				// Unconditional branch: one path to analyze
				path_start[0] = (PC-4) + branch_imm*4;
				path_end[0] = path_start[0] + scan_max*4;
				num_paths = 1;
			} else {
				// Conditional branch: two paths to analyze
				// NOTE: index 0 is branch-not-taken path, index 1 is branch-taken path

				path_start[0] = PC;
				path_start[1] = (PC-4) + branch_imm*4;

				path_end[0] = path_start[0] + scan_max*4;
				path_end[1] = path_start[1] + scan_max*4;

				if (path_start[0] == path_start[1])
					num_paths = 1;
			}

			// If one path overlaps the other, trim its length
			if (num_paths > 1) {
				if (branch_imm > 0) {
					// Path 0 is the not-taken path of a forwards branch
					// Path 1 is the taken path
					if (path_end[0] > path_start[1])
						path_end[0] = path_start[1];
				} else {
					// Path 0 is the not-taken path of a backwards branch
					// Path 1 is the taken path
					if (path_end[1] > path_start[0])
						path_end[1] = path_start[0];
				}
			}
		}

		if (!convertible)
			return 0;

		for (int path=0; path < num_paths; ++path)
		{
			in_bd_slot = 0;
			PC = path_start[path];

			int left = (path_end[path] - path_start[path]) / 4;
			while (left-- > 0)
			{
				const u32 opcode = OPCODE_AT(PC);
				PC += 4;

				// Skip any NOPs
				if (opcode == 0)
					continue;

				// Bits 32,33 of u64 retval are LO,HI respectively. Lower bits are GPRs.
				const u64 op_reg_reads    = opcodeGetReads(opcode);
				const u64 op_reg_writes   = opcodeGetWrites(opcode);
				const u64 op_reg_accesses = op_reg_reads | op_reg_writes;

				// Check for a HI and LO overwrite (MULT,MULTU,DIV,DIVU)
				if (((op_reg_writes >> 32) & 3) == 3) {
					path_has_hi_lo_overwrite[path] = 1;
					break;
				}

				// Check for a HI or LO access (only *after* overwrite check above)
				if (((op_reg_accesses >> 32) & 3)) {
					convertible = 0;
					break;
				}

				// Give up scanning at a SYSCALL or HLE instruction.
				if ( _fOp_(opcode) == 0x3b || /* HLE */
				    (_fOp_(opcode) == 0 && _fFunct_(opcode) == 0xc) ) /* SYSCALL */
				{
					convertible = 0;
					break;
				}

				if (opcodeIsBranchOrJump(opcode))
				{
					// Branch/jump in BD slot? Give up.
					if (in_bd_slot) {
						convertible = 0;
						break;
					}

#ifdef USE_3OP_MUL_JUMP_OPTIMIZATIONS
					// See comments further above regarding this optimization.
					if ( _fOp_(opcode) == 0x3 || /* JAL */
					    (_fOp_(opcode) == 0 && (_fFunct_(opcode) == 0x8 || _fFunct_(opcode) == 0x9)))  /* JR, JALR */
					{
						path_has_hi_lo_overwrite[path] = 1;

						// NOTE: we still scan the BD slot for a HI/LO access
					}
#endif // USE_3OP_MUL_JUMP_OPTIMIZATIONS

					// Scan just the BD slot for a HI/LO access or overwrite and stop.
					in_bd_slot = 1;
					left = 1;
				}
			}
		}

		if (num_paths > 1)
		{
			// If one path ends where another begins, we only need to ensure
			//  the path with the higher start address had a HI+LO overwrite.
			if (path_end[0] == path_start[1]) {
				found_hi_lo_overwrite = path_has_hi_lo_overwrite[1];
			} else if (path_end[1] == path_start[0]) {
				found_hi_lo_overwrite = path_has_hi_lo_overwrite[0];
			} else {
				// One path does not flow into the other: we must ensure HI+LO
				//  were overwritten in *both* paths.
				found_hi_lo_overwrite = path_has_hi_lo_overwrite[0] &&
				                        path_has_hi_lo_overwrite[1];
			}
		} else {
			found_hi_lo_overwrite = path_has_hi_lo_overwrite[0];
		}
	}

	if (!convertible || !found_hi_lo_overwrite)
		return 0;

	/***********************************************************
	 * STAGE 5: Emit a 3-op MUL instead of original MULT/MULTU *
	 ***********************************************************/

	// We also set a flag telling recMFLO() to emit nothing at its next call.
	// NOTE: There is no unsigned 3-op MUL. The 32-bit result would be identical.

	DISASM_MSG("CONVERTING MULT/MULTU TO 3-OP MUL\n");

	u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	u32 rd = 0;
	if (rd_of_mflo == _Rs_) {
		rd = rs;
	} else if (rd_of_mflo == _Rt_) {
		rd = rt;
	} else {
		rd = regMipsToHost(rd_of_mflo, REG_FIND, REG_REGISTER);
	}

	MUL(rd, rs, rt);

	// If the multiply instruction we converted was in a BD slot, we must spill
	//  the result. Other blocks might start at or before the MFLO instruction
	//  in the original code.
	if (branch) {
		SW(rd, PERM_REG_1, offGPR(32)); // LO
	}

	SetUndef(rd_of_mflo);
	regMipsChanged(rd_of_mflo);

	regUnlock(rs);
	regUnlock(rt);
	regUnlock(rd);

	// Set flag telling recMFLO() to emit nothing at its next call.
	skip_emitting_next_mflo = 1;

	// TODO - In the future, instead of using a global flag like above, it'd be
	//  nicer to be able to flag certain PC locations in a block as
	//  'should-be-skipped', catching them before ever calling their emitter.

	return 1;
}
#endif // USE_3OP_MUL_OPTIMIZATIONS
