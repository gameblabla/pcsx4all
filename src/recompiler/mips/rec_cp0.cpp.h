static void recMFC0()
{
// Rt = Cop0->Rd
	if (!_Rt_) return;
	SetUndef(_Rt_);
	u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

	LW(rt, PERM_REG_1, offCP0(_Rd_));
	regMipsChanged(_Rt_);
	regUnlock(rt);
}

static void recCFC0()
{
// Rt = Cop0->Rd

	recMFC0();
}

// Tests for SW interrupts/exceptions after writes to MTC0 CP0 reg 12,13
// Expects CP0 Cause reg 13 in MIPSREG_A0, Status reg 12 in MIPSREG_A1
static void emitTestSWInts()
{
	// ---- Equivalent C code: ----
	// if ((psxRegs.CP0.n.Cause & psxRegs.CP0.n.Status & 0x0300) &&
	//     psxRegs.CP0.n.Status & 0x1))
	// {
	//     psxRegs.CP0.n.Cause &= ~0x7c;
	//     psxException(psxRegs.CP0.n.Cause, branch);
	//
	//     /* return to block dispatch loop with exception's new PC */
	// }

	AND(TEMP_1, MIPSREG_A0, MIPSREG_A1);   // TEMP_1 = Cause & Status
	ANDI(MIPSREG_A1, MIPSREG_A1, 0x1);     // MIPSREG_A1 = Status & 0x1

	u32 *backpatch1 = (u32 *)recMem;
	BEQZ(MIPSREG_A1, 0);
	ANDI(TEMP_1, TEMP_1, 0x300);  // <BD slot> TEMP_1 = (Cause & Status) & 0x300

	u32 *backpatch2 = (u32 *)recMem;
	BEQZ(TEMP_1, 0);
	// NOTE: Branch delay slot contains next emitted instruction

	// Clear bits 6:2 of Cause reg value (ExcCode field), indicating
	//  cause of exception is 'Interrupt'
#ifdef HAVE_MIPS32R2_EXT_INS
	INS(MIPSREG_A0, 0, 2, 5);            // <BD slot> MIPSREG_A0 = Cause & ~0x7c
#else
	LI16(TEMP_2, 0x7c);                  // <BD slot>
	NOR(TEMP_2, 0, TEMP_2);              // TEMP_1 = ~0x7c;
	AND(MIPSREG_A0, MIPSREG_A0, TEMP_2); // MIPSREG_A0 = Cause & ~0x7c
#endif

	// Must set psxRegs.CP0.n.Cause, as psxException won't overwrite
	//  bits 8,9 itself.
	SW(MIPSREG_A0, PERM_REG_1, offCP0(13)); // psxRegs.CP0.n.Cause = MIPSREG_A0

	// psxRegs.pc is set to instruction that cause the exception
	LI32(TEMP_1, pc - 4);
	SW(TEMP_1, PERM_REG_1, off(pc));

	regClearBranch();
	JAL(psxException);

	// Use JAL's BD slot to set arg telling if instruction lied in BD slot :)
	LI16(MIPSREG_A1, (branch == 1 ? 1 : 0)); // <BD slot>

	// If new PC is unknown, cannot use 'fastpath' return
	bool use_fastpath = false;

	rec_recompile_end_part1();

	LW(MIPSREG_V0, PERM_REG_1, off(pc)); // Block retval $v0 = new PC set by psxException()

	rec_recompile_end_part2(use_fastpath);

	fixup_branch(backpatch1);
	fixup_branch(backpatch2);
}

static void recMTC0()
{
// Cop0->Rd = Rt

	u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

	switch (_Rd_) {
		case 12: // Status
			// Store new Status reg val, while also checking if new value
			//  enables HW irqs/exceptions. Reset psxRegs.io_cycle_counter
			//  if so, so that psxBranchTest() is called as soon as possible.

			if (IsConst(_Rt_)) {
				SW(rt, PERM_REG_1, offCP0(12)); // Store new CP0 Status reg val
				if ((GetConst(_Rt_) & 0x401) == 0x401) {
					SW(0, PERM_REG_1, off(io_cycle_counter));
				}
			} else {
				ANDI(TEMP_1, rt, 0x401);
				LI16(TEMP_2, 0x401);
				u32 *backpatch = (u32 *)recMem;
				BNE(TEMP_1, TEMP_2, 0);
				SW(rt, PERM_REG_1, offCP0(12)); // <BD slot> Store new CP0 Status reg val
				SW(0, PERM_REG_1, off(io_cycle_counter));
				fixup_branch(backpatch);
			}

			// Modification of CP0 reg 12 or 13 must be followed by test for
			//  software-generated IRQ/exception, unless value written is
			//  known const val that would not generate one.
			//  ** Fixes freeze at start of 'Jackie Chan Stuntmaster'

			if (!IsConst(_Rt_) ||
			    ((GetConst(_Rt_) & 0x300) && (GetConst(_Rt_) & 0x1))) {

				// Load CP0 Cause reg (13), as emitTestSWInts() expects
				//  Cause in MIPSREG_A0 and Status in MIPSREG_A1
				LW(MIPSREG_A0, PERM_REG_1, offCP0(13));
				MOV(MIPSREG_A1, rt);

				emitTestSWInts();
			}
			break;

		case 13: // Cause
			// Only bits 8,9 are writable
			// ---- Equivalent C code: ----
			// u32 val = _Rt_;
			// psxRegs.CP0.n.Cause &= ~0x300;
			// psxRegs.CP0.n.Cause |= val & 0x300;

			LW(MIPSREG_A0, PERM_REG_1, offCP0(13)); // MIPSREG_A0 = psxRegs.CP0.n.Cause

#ifdef HAVE_MIPS32R2_EXT_INS
			EXT(TEMP_1, rt, 8, 2);               // TEMP_1 = bits 9:8 of _Rt_
			INS(MIPSREG_A0, TEMP_1, 8, 2);       // MIPSREG_A0 9:8 = bits 1:0 of TEMP_1
#else
			LI16(TEMP_1, 0x300);                 // TEMP_1 = 0x300
			AND(TEMP_2, rt, TEMP_1);             // TEMP_2 = _Rt_ & 0x300
			NOR(TEMP_1, 0, TEMP_1);              // TEMP_1 = ~0x300
			AND(MIPSREG_A0, MIPSREG_A0, TEMP_1); // MIPSREG_A0 = psxRegs.CP0.n.Cause & ~0x300
			OR(MIPSREG_A0, MIPSREG_A0, TEMP_2);  // MIPSREG_A0 |= (_Rt_ & 0x300)
#endif
			SW(MIPSREG_A0, PERM_REG_1, offCP0(13));

			// See notes above regarding test for software-generated exception

			if (!IsConst(_Rt_) || (GetConst(_Rt_) & 0x300)) {
				// Load CP0 Status reg (12), as emitTestSWInts() expects
				//  Cause in MIPSREG_A0 and Status in MIPSREG_A1
				LW(MIPSREG_A1, PERM_REG_1, offCP0(12));

				emitTestSWInts();
			}
			break;

		default:
			SW(rt, PERM_REG_1, offCP0(_Rd_));
			break;
	}

	regUnlock(rt);
}

static void recCTC0()
{
// Cop0->Rd = Rt

	recMTC0();
}

static void recRFE()
{
// 'Return from exception' opcode
//  Inside CP0 Status register (12), RFE atomically copies bits 5:2 to
//  bits 3:0 , unwinding the exception 'stack'

	LW(TEMP_1, PERM_REG_1, offCP0(12));

	// Reset psxRegs.io_cycle_counter, so that psxBranchTest() is called as
	//  soon as possible to handle any pending interrupts/events
	SW(0, PERM_REG_1, off(io_cycle_counter));

#ifdef HAVE_MIPS32R2_EXT_INS
	EXT(TEMP_2, TEMP_1, 2, 4);   // Copy bits 5:2 of TEMP_1 to 3:0 of TEMP_2
	INS(TEMP_1, TEMP_2, 0, 4);   // Copy those bits back to 3:0 of TEMP_1
#else
	SRL(TEMP_2, TEMP_1, 4);
	SLL(TEMP_2, TEMP_2, 4);      // TEMP_2 = orig SR value with bits 3:0 cleared

	ANDI(TEMP_1, TEMP_1, 0x3c);  // TEMP_1 = just bits 5:2 from orig SR value
	SRL(TEMP_1, TEMP_1, 2);      // Shift them right two places
	OR(TEMP_1, TEMP_2, TEMP_1);  // TEMP_1 = new SR value
#endif

	SW(TEMP_1, PERM_REG_1, offCP0(12));
}
