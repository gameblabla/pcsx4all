/* Optional defines (for debugging) */

/* Detect conditional branches on known-const reg vals, eliminating
 *  dead code and unnecessary branches.
 */
#define USE_CONST_BRANCH_OPTIMIZATIONS

/* Convert some small forwards conditional branches to modern conditional moves */
#define USE_CONDITIONAL_MOVE_OPTIMIZATIONS

static bool convertBranchToConditionalMoves();

static void recSYSCALL()
{
	regClearJump();

	LI32(TEMP_1, pc - 4);
	SW(TEMP_1, PERM_REG_1, off(pc));

	LI16(MIPSREG_A1, (branch == 1 ? 1 : 0));
	JAL(psxException);
	LI16(MIPSREG_A0, 0x20); // <BD> Load first param using BD slot of JAL()

	// If new PC is unknown, cannot use 'fastpath' return
	bool use_fastpath_return = false;

	rec_recompile_end_part1();

	LW(MIPSREG_V0, PERM_REG_1, off(pc)); // Block retval $v0 = new PC set by psxException()

	rec_recompile_end_part2(use_fastpath_return);

	end_block = 1;
}

/* Check if an opcode has a delayed read if in delay slot */
static int iLoadTest(u32 code)
{
	// check for load delay
	u32 op = _fOp_(code);
	switch (op) {
	case 0x10: // COP0
		switch (_fRs_(code)) {
		case 0x00: // MFC0
		case 0x02: // CFC0
			return 1;
		}
		break;
	case 0x12: // COP2
		switch (_fFunct_(code)) {
		case 0x00:
			switch (_fRs_(code)) {
			case 0x00: // MFC2
			case 0x02: // CFC2
				return 1;
			}
			break;
		}
		break;
	case 0x32: // LWC2
		return 1;
	default:
		// LB/LH/LWL/LW/LBU/LHU/LWR
		if (op >= 0x20 && op <= 0x26) {
			return 1;
		}
		break;
	}
	return 0;
}

static int DelayTest(u32 pc, u32 bpc)
{
	u32 code1 = *(u32 *)((char *)PSXM(pc));
	u32 code2 = *(u32 *)((char *)PSXM(bpc));
	u32 reg = _fRt_(code1);

//#define LOG_BRANCHLOADDELAYS
#ifdef LOG_BRANCHLOADDELAYS
	if (iLoadTest(code1)) {
		int i = psxTestLoadDelay(reg, code2);
		if (i == 1 || i == 2) {
			char buffer[512];
			printf("Case %d at %08x\n", i, pc);
			u32 jcode = *(u32 *)((char *)PSXM(pc - 4));
			disasm_mips_instruction(jcode, buffer, pc - 4, 0, 0);
			printf("%08x: %s\n", pc - 4, buffer);
			disasm_mips_instruction(code1, buffer, pc, 0, 0);
			printf("%08x: %s\n", pc, buffer);
			disasm_mips_instruction(code2, buffer, bpc, 0, 0);
			printf("%08x: %s\n\n", bpc, buffer);
		}
	}
#endif

	if (iLoadTest(code1)) {
		return psxTestLoadDelay(reg, code2);
		// 1: delayReadWrite	// the branch delay load is skipped
		// 2: delayRead		// branch delay load
		// 3: delayWrite	// no changes from normal behavior
	}

	return 0;
}

/* Revert execution order of opcodes at branch target address and in delay slot
   This emulates the effect of delayed read from COP2 happening in delay slot
   when the branch is taken. This fixes Tekken 2 (broken models). */
static void recRevDelaySlot(u32 pc, u32 bpc)
{
	branch = 1;

	psxRegs.code = *(u32 *)((char *)PSXM(bpc));
	recBSC[psxRegs.code>>26]();

	psxRegs.code = *(u32 *)((char *)PSXM(pc));
	recBSC[psxRegs.code>>26]();

	branch = 0;
}

/* Recompile opcode in delay slot */
static void recDelaySlot()
{
	branch = 1;
	psxRegs.code = *(u32 *)((char *)PSXM(pc));
	DISASM_PSX(pc);
	pc+=4;

	recBSC[psxRegs.code>>26]();
	branch = 0;
}

static void iJumpNormal(u32 bpc)
{
#ifdef LOG_BRANCHLOADDELAYS
	u32 dt = DelayTest(pc, bpc);
#endif

	recDelaySlot();

	// Can block use 'fastpath' return? (branches backward to its beginning)
	bool use_fastpath_return = rec_recompile_use_fastpath_return(bpc);

	rec_recompile_end_part1();
	regClearJump();

	// Only need to set $v0 to new PC when not returning to 'fastpath'
	if (!use_fastpath_return) {
		LI32(MIPSREG_V0, bpc); // Block retval $v0 = new PC val
	}

	rec_recompile_end_part2(use_fastpath_return);

	end_block = 1;
}

static void iJumpAL(u32 bpc, u32 nbpc)
{
#ifdef LOG_BRANCHLOADDELAYS
	u32 dt = DelayTest(pc, bpc);
#endif

	u32 ra = regMipsToHost(31, REG_FIND, REG_REGISTER);
	LI32(ra, nbpc);
	SetConst(31, nbpc);
	regMipsChanged(31);

	int dt = DelayTest(pc, bpc);
	if (dt == 2) {
		recRevDelaySlot(pc, bpc);
		bpc += 4;
	} else if (dt == 3 || dt == 0) {
		recDelaySlot();
	}

	// Can block use 'fastpath' return? (branches backward to its beginning)
	bool use_fastpath_return = rec_recompile_use_fastpath_return(bpc);

	rec_recompile_end_part1();
	regClearJump();

	// Only need to set $v0 to new PC when not returning to 'fastpath'
	if (!use_fastpath_return) {
		LI32(MIPSREG_V0, bpc);     // Block retval $v0 = new PC val
	}

	rec_recompile_end_part2(use_fastpath_return);

	end_block = 1;
}

/* Used for BLTZ, BGTZ, BLTZAL, BGEZAL, BLEZ, BGEZ */
static void emitBxxZ(int andlink, u32 bpc, u32 nbpc)
{
	u32 code = psxRegs.code;
	int dt = DelayTest(pc, bpc);

#ifdef USE_CONST_BRANCH_OPTIMIZATIONS
	// If test register is known-const, we can eliminate the branch:
	//  If branch is taken, block will end here.
	//  If not taken, we skip over it and continue emitting at delay slot.

	// Only do const-propagated branch shortcuts if no delay-slot
	//  trickery is detected.
	if (IsConst(_Rs_) && ((dt == 3) || dt == 0))
	{
		// NOTE: Unlike normally-emitted branch code, we don't execute the delay
		//  slot before doing branch tests when branch operands are known-const.
		//  The way it's done here seems it'd be the correct way to do it in all
		//  cases, but anything different causes immediate problems either way.
		s32 val = GetConst(_Rs_);
		bool branch_taken = false;

		switch (code & 0xfc1f0000) {
			case 0x04000000: /* BLTZ */
			case 0x04100000: /* BLTZAL */
				branch_taken = (val < 0);
				break;
			case 0x04010000: /* BGEZ */
			case 0x04110000: /* BGEZAL */
				branch_taken = (val >= 0);
				break;
			case 0x1c000000: /* BGTZ */
				branch_taken = (val > 0);
				break;
			case 0x18000000: /* BLEZ */
				branch_taken = (val <= 0);
				break;
			default:
				printf("Error opcode=%08x\n", code);
				exit(1);
		}

		if (branch_taken) {
			if (andlink)
				iJumpAL(bpc, nbpc);
			else
				iJumpNormal(bpc);
		} else {
			recDelaySlot();
		}

		// We're done here, stop emitting code
		return;
	}
#endif // USE_CONST_BRANCH_OPTIMIZATIONS

#ifdef USE_CONDITIONAL_MOVE_OPTIMIZATIONS
	// Attempt to convert the branch to conditional move(s).
	// IMPORTANT: There must be no delay-slot trickery, and branch must
	//            not be a 'bgezal,bltzal' branch-and-link instruction.
	if (!andlink && (dt == 3 || dt == 0))
	{
		if (convertBranchToConditionalMoves())
			return;
	}
#endif // USE_CONDITIONAL_MOVE_OPTIMIZATIONS


	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);

	if (dt == 3 || dt == 0) {
		recDelaySlot();
	}

	u32 *backpatch = (u32 *)recMem;

	// Check opcode and emit branch with REVERSED logic!
	switch (code & 0xfc1f0000) {
	case 0x04000000: /* BLTZ */
	case 0x04100000: /* BLTZAL */	BGEZ(br1, 0); break;
	case 0x04010000: /* BGEZ */
	case 0x04110000: /* BGEZAL */	BLTZ(br1, 0); break;
	case 0x1c000000: /* BGTZ */	BLEZ(br1, 0); break;
	case 0x18000000: /* BLEZ */	BGTZ(br1, 0); break;
	default:
		printf("Error opcode=%08x\n", code);
		exit(1);
	}

	// Remember location of branch delay slot so we can be sure it was filled
	const u32 *bd_slot_loc = (u32 *)recMem;

	// Can block use 'fastpath' return? (branches backward to its beginning)
	bool use_fastpath_return = rec_recompile_use_fastpath_return(bpc);

	// If indirect block returns are in use:
	// Load host $ra with block return address using BD slot. Code emitted
	//  in either branch path after this point can assume it is now loaded.
	// NOTE: rec_recompile_end_part1() will only emit an instruction if $ra
	//       is not already loaded, so ensure that next instruction emitted
	//       after rec_recompile_end_part1() is also safe to put in BD slot.
	rec_recompile_end_part1(); /* <BD> (MAYBE) */
	block_ra_loaded = 1;

	regPushState();

	if (dt == 2) {
		NOP(); /* <BD> (MAYBE) */

		// Instruction at target PC should see ra reg write
		if (andlink) {
			u32 ra = regMipsToHost(31, REG_FIND, REG_REGISTER);
			LI32(ra, nbpc);
			regMipsChanged(31);
			// Cannot set const because branch-not-taken path wouldn't see it:
			SetUndef(31);
		}

		recRevDelaySlot(pc, bpc);
		bpc += 4;

		// NOTE: Because the instruction at target PC might have emitted a JAL and
		//       wiped out host $ra, load it again if necessary.
		rec_recompile_end_part1();
		block_ra_loaded = 1;
	}

	// If rec_recompile_end_part1() did not emit an instruction,
	//  next instruction emitted here is BD slot:

	// Only need to set $v0 to new PC when not returning to 'fastpath'
	if (!use_fastpath_return) {
		if (bpc > 0xffff) {
			LUI(TEMP_1, (bpc >> 16));  /* <BD> (MAYBE) */
			ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff));
		} else {
			LI16(MIPSREG_V0, (bpc & 0xffff));  /* <BD> (MAYBE) */
		}
	}

	if (andlink && dt != 2) {
		if (!use_fastpath_return && (bpc > 0xffff) && ((bpc >> 16) == (nbpc >> 16))) {
			// Both PCs share an upper half, can save an instruction:
			ORI(TEMP_1, TEMP_1, (nbpc & 0xffff));
		} else {
			LI32(TEMP_1, nbpc);  /* <BD> (MAYBE) */
		}
		SW(TEMP_1, PERM_REG_1, offGPR(31));
	}

	regClearBranch();

	// Rarely, the branch delay slot is still empty at this point. Fill if so.
	if (bd_slot_loc == recMem) {
		NOP();  /* <BD> */
	}

	rec_recompile_end_part2(use_fastpath_return);

	regPopState();

	fixup_branch(backpatch);
	regUnlock(br1);

	if (dt != 3 && dt != 0) {
		recDelaySlot();
	}
}

/* Used for BEQ and BNE */
static void emitBxx(u32 bpc)
{
#ifdef LOG_BRANCHLOADDELAYS
	u32 dt = DelayTest(pc, bpc);
#endif

	u32 code = psxRegs.code;

	// XXX - If delay-slot trickery workarounds are ever added to this
	//       emitter, as emitBxxZ() already has, be sure to disallow
	//       the following optimizations when it is detected.

#ifdef USE_CONST_BRANCH_OPTIMIZATIONS
	// If test registers are known-const, we can eliminate the branch:
	//  If taken, block will end here.
	//  If not taken, we skip over it and continue emitting at delay slot.

	if (IsConst(_Rs_) && IsConst(_Rt_))
	{
		// NOTE: Unlike normally-emitted branch code, we don't execute the delay
		//  slot before doing branch tests when branch operands are known-const.
		//  The way it's done here seems it'd be the correct way to do it in all
		//  cases, but anything different causes immediate problems either way.
		s32 val1 = GetConst(_Rs_);
		s32 val2 = GetConst(_Rt_);
		bool branch_taken = false;

		switch (code & 0xfc000000) {
			case 0x10000000: /* BEQ */
				branch_taken = (val1 == val2);
				break;
			case 0x14000000: /* BNE */
				branch_taken = (val1 != val2);
				break;
			default:
				printf("Error opcode=%08x\n", code);
				exit(1);
		}

		if (branch_taken)
			iJumpNormal(bpc);
		else
			recDelaySlot();

		// We're done here, stop emitting code
		return;
	}
#endif // USE_CONST_BRANCH_OPTIMIZATIONS

#ifdef USE_CONDITIONAL_MOVE_OPTIMIZATIONS
	// Attempt to convert the branch to conditional move(s).
	if (convertBranchToConditionalMoves())
		return;
#endif // USE_CONDITIONAL_MOVE_OPTIMIZATIONS


	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	u32 br2 = regMipsToHost(_Rt_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	recDelaySlot();
	u32 *backpatch = (u32 *)recMem;

	// Check opcode and emit branch with REVERSED logic!
	switch (code & 0xfc000000) {
	case 0x10000000: /* BEQ */	BNE(br1, br2, 0); break;
	case 0x14000000: /* BNE */	BEQ(br1, br2, 0); break;
	default:
		printf("Error opcode=%08x\n", code);
		exit(1);
	}

	// Remember location of branch delay slot so we can be sure it was filled
	const u32 *bd_slot_loc = (u32 *)recMem;

	// Can block use 'fastpath' return? (branches backward to its beginning)
	bool use_fastpath_return = rec_recompile_use_fastpath_return(bpc);

	// If indirect block returns are in use:
	// Load host $ra with block return address using BD slot. Code emitted
	//  in either branch path after this point can assume it is now loaded.
	// NOTE: rec_recompile_end_part1() will only emit an instruction if $ra
	//       is not already loaded, so ensure that next instruction emitted
	//       after rec_recompile_end_part1() is also safe to put in BD slot.
	rec_recompile_end_part1(); /* <BD> (MAYBE) */
	block_ra_loaded = 1;

	// Only need to set $v0 to new PC when not returning to 'fastpath'
	if (!use_fastpath_return) {
		LI32(MIPSREG_V0, bpc);  /* <BD> (MAYBE) */
	}

	regClearBranch(); /* <BD> (MAYBE) */

	// Rarely, the branch delay slot is still empty at this point. Fill if so.
	if (bd_slot_loc == recMem) {
		NOP();  /* <BD> */
	}

	rec_recompile_end_part2(use_fastpath_return);

	fixup_branch(backpatch);
	regUnlock(br1);
	regUnlock(br2);
}

static void recBLTZ()
{
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_)) {
		recDelaySlot();
		return;
	}

	emitBxxZ(0, bpc, nbpc);
}

static void recBGTZ()
{
// Branch if Rs > 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_)) {
		recDelaySlot();
		return;
	}

	emitBxxZ(0, bpc, nbpc);
}

static void recBLTZAL()
{
// Branch if Rs < 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (!(_Rs_)) {
		recDelaySlot();
		return;
	}

	emitBxxZ(1, bpc, nbpc);
}

static void recBGEZAL()
{
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (!(_Rs_)) {
		iJumpAL(bpc, (pc + 4));
		return;
	}

	emitBxxZ(1, bpc, nbpc);
}

static void recJ()
{
// j target

	iJumpNormal(_Target_ * 4 + (pc & 0xf0000000));
}

static void recJAL()
{
// jal target

	iJumpAL(_Target_ * 4 + (pc & 0xf0000000), (pc + 4));
}

extern void (*psxBSC[64])(void);

/* HACK: Execute load delay in branch delay via interpreter */
static u32 execBranchLoadDelay(u32 pc, u32 bpc)
{
	u32 code1 = *(u32 *)((char *)PSXM(pc));
	u32 code2 = *(u32 *)((char *)PSXM(bpc));

	branch = 1;

#ifdef LOG_BRANCHLOADDELAYS
	int i = psxTestLoadDelay(_fRt_(code1), code2);
	if (i == 1 || i == 2) {
		char buffer[512];
		printf("Case %d at %08x\n", i, pc);
		u32 jcode = *(u32 *)((char *)PSXM(pc - 4));
		disasm_mips_instruction(jcode, buffer, pc - 4, 0, 0);
		printf("%08x: %s\n", pc - 4, buffer);
		disasm_mips_instruction(code1, buffer, pc, 0, 0);
		printf("%08x: %s\n", pc, buffer);
		disasm_mips_instruction(code2, buffer, bpc, 0, 0);
		printf("%08x: %s\n\n", bpc, buffer);
	}
#endif

	switch (psxTestLoadDelay(_fRt_(code1), code2)) {
	case 2:		// branch delay + load delay
		psxRegs.code = code2;
		psxBSC[code2 >> 26](); // first branch opcode

		bpc += 4;
		// intentional fallthrough here!
	case 0:
	case 3:		// Simple branch delay
		psxRegs.code = code1;
		psxBSC[code1 >> 26](); // branch delay load

		// again intentional fallthrough here!
	case 1:		// No branch delay
		break;
	}

	branch = 0;

	return bpc;
}

static void recJR_load_delay()
{
	regClearJump();
	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);

	LI32(MIPSREG_A0, pc);
	JAL(execBranchLoadDelay);
	MOV(MIPSREG_A1, br1); // <BD>

	// $v0 here contains jump address returned from execBranchLoadDelay()

	// If new PC is unknown, cannot use 'fastpath' return
	bool use_fastpath_return = false;

	rec_recompile_end_part1();
	pc += 4;
	regUnlock(br1);

	rec_recompile_end_part2(use_fastpath_return);

	end_block = 1;
}

static void recJR()
{
// jr Rs
	u32 code = *(u32 *)((char *)PSXM(pc)); // opcode in branch delay

	// if possible read delay in branch delay slot
	if (iLoadTest(code)) {
		recJR_load_delay();

		return;
	}

	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	recDelaySlot();

	// If new PC is unknown, cannot use 'fastpath' return
	bool use_fastpath_return = false;

	rec_recompile_end_part1();

	regClearJump();
	MOV(MIPSREG_V0, br1); // Block retval $v0 = new PC val
	regUnlock(br1);

	rec_recompile_end_part2(use_fastpath_return);

	end_block = 1;
}

static void recJALR()
{
// jalr Rs
	u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
	u32 rd = regMipsToHost(_Rd_, REG_FIND, REG_REGISTER);
	LI32(rd, pc + 4);
	SetConst(_Rd_, pc + 4);
	regMipsChanged(_Rd_);
	recDelaySlot();

	// If new PC is unknown, cannot use 'fastpath' return
	bool use_fastpath_return = false;

	rec_recompile_end_part1();

	regClearJump();
	MOV(MIPSREG_V0, br1); // Block retval $v0 = new PC val
	regUnlock(br1);

	rec_recompile_end_part2(use_fastpath_return);

	end_block = 1;
}

static void recBEQ()
{
// Branch if Rs == Rt
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (_Rs_ == _Rt_) {
		iJumpNormal(bpc);
		return;
	}

	emitBxx(bpc);
}

static void recBNE()
{
// Branch if Rs != Rt
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_) && !(_Rt_)) {
		recDelaySlot();
		return;
	}

	emitBxx(bpc);
}

static void recBLEZ()
{
// Branch if Rs <= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_)) {
		iJumpNormal(bpc);
		return;
	}

	emitBxxZ(0, bpc, nbpc);
}

static void recBGEZ()
{
// Branch if Rs >= 0
	u32 bpc = _Imm_ * 4 + pc;
	u32 nbpc = pc + 4;

	if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0) {
		return;
	}

	if (!(_Rs_)) {
		iJumpNormal(bpc);
		return;
	}

	emitBxxZ(0, bpc, nbpc);
}

static void recBREAK() { }

/* senquack - This is the one function I was unsure about during improvements
 *  to block dispatch in August 2016. I haven't found a single game yet that
 *  seems to use this opcode. I believe it is correct with new changes, though.
 */
static void recHLE()
{
	regClearJump();

	LI32(TEMP_1, pc);
	JAL(((u32)psxHLEt[psxRegs.code & 0x7]));
	SW(TEMP_1, PERM_REG_1, off(pc));        // <BD> BD slot of JAL() above

	// If new PC is unknown, cannot use 'fastpath' return
	bool use_fastpath_return = false;

	rec_recompile_end_part1();

	LW(MIPSREG_V0, PERM_REG_1, off(pc)); // <BD> Block retval $v0 = psxRegs.pc

	rec_recompile_end_part2(use_fastpath_return);

	end_block = 1;
}

/* Try to convert a small forward-branch into a series of modern
 * conditional-moves. All opcodes in the branch-not-taken path must be ALU ops.
 * Can reduce total number of blocks and total recompiled code size by 5-10%,
 * and speed the code up a bit.
 *
 * Returns: true if branch was successfully converted, false if not.
 *
 * NOTE: It is assumed 'beq/bne $zero,$zero' is caught before calling here.
 *       Is is also assumed we aren't asked to convert 'bgezal,bltzal', the
 *       branch-and-link instructions.
 */
#ifdef USE_CONDITIONAL_MOVE_OPTIMIZATIONS
static bool convertBranchToConditionalMoves()
{
	// Limit on size of branch-not-taken paths we convert. If it's too large,
	// we'd waste time analyzing not-taken-paths that are really unlikely to
	// exclusively contain ALU ops. Max of 3..5 seems to be the sweet spot.
	const int max_ops = 4;  // Up to this # opcodes total, excluding BD slot.

	// Temporary registers we can use
	const int max_renamed = 4;
	const u8  renamed_reg_pool[max_renamed] = { MIPSREG_A0, MIPSREG_A1, MIPSREG_A2, MIPSREG_A3 };
	const u8  temp_condition_reg = TEMP_1;

	const u32  branch_opcode  = OPCODE_AT(pc-4);
	const u32  bd_slot_opcode = OPCODE_AT(pc);
	const s32  branch_imm = _fImm_(branch_opcode);
	const u8   branch_rs  = _fRs_(branch_opcode);
	const u8   branch_rt  = _fRt_(branch_opcode);
	const bool is_beq     = _fOp_(branch_opcode) == 0x04;
	const bool is_bne     = _fOp_(branch_opcode) == 0x05;

	// Don't allow any backward or 0-len branches, or branches too far.
	if (branch_imm <= 1 || branch_imm > (max_ops+1))
		return false;

	// Shouldn't happen: prior functions should catch 'BEQ/BNE $zero,$zero'.
	if ((is_beq || is_bne) && (branch_rs == 0 && branch_rt == 0))
		return false;

	// Branch/jump in BD slot? Give up.
	if (opcodeIsBranchOrJump(bd_slot_opcode))
		return false;

	/*******************************************************
	 * STAGE 1: Ensure all ops are ALU, collect basic info *
	 *******************************************************/
	struct {
		u32  opcode;
		u8   dst_reg;
		bool writes_rt;
		bool reads_rs;
		bool reads_rt;
		bool rs_renamed;
		bool rt_renamed;
		u8   dst_renamed_to;
		u8   rs_renamed_to;
		u8   rt_renamed_to;

		//  Normally, it is assumed that dst reg has been renamed. However, some
		// common 'addu rd, rs, $zero' compiler/assembler-generated reg-moves
		// can avoid reg-renaming, and this gets set true.
		bool emit_as_rd_rs_direct_cond_move;
	} ops[max_ops];

	memset(ops, 0, sizeof(ops));

	// Bitfield representing any GPRs written by opcodes
	u32  op_writes = 0;

	int  num_ops = 0;
	bool success = true;

	for (int i=0; i < (branch_imm-1); ++i)
	{
		// 'pc' is currently pointing to the branch's BD slot, so
		//   start looking at opcodes one instruction after that.
		const u32 opcode = OPCODE_AT(pc + 4 + i*4);

		// Skip any NOPs
		if (opcode == 0)
			continue;

		struct ALUOpInfo info;

		if (opcodeIsALU(opcode, &info))
		{
			const u8 dst_reg = info.writes_rt ? _fRt_(opcode) : _fRd_(opcode);

			// Skip any ALU ops with $zero dst reg
			if (!dst_reg)
				continue;

			ops[num_ops].opcode = opcode;
			ops[num_ops].dst_reg = dst_reg;
			ops[num_ops].writes_rt = info.writes_rt;
			ops[num_ops].reads_rs  = info.reads_rs;
			ops[num_ops].reads_rt  = info.reads_rt;
			op_writes |= (1 << dst_reg);
			num_ops++;
		} else {
			// Found non-ALU op, give up
			success = false;
			break;
		}
	}

	if (!success || (num_ops == 0))
		return false;

	/***************************************************************
	 * STAGE 2: Register renaming, overflow-trap opcode conversion *
	 ***************************************************************/
	int num_renamed = 0;

	// Reverse mapping: renamed reg -> original PSX reg (used in last stage)
	u8 host_to_psx_map[max_renamed];

	{
		// Only needed during this renaming phase:
		struct {
			bool is_renamed;
			u8   renamed_to;
		} reg_map[32];

		memset(reg_map, 0, sizeof(reg_map));

		for (int i=0; i < num_ops; ++i)
		{
			u32 opcode = ops[i].opcode;

			// Convert any overflow-trap instructions to non-trapping ones
			switch (_fOp_(opcode)) {
				case 0:
					switch(_fFunct_(opcode)) {
						case 0x20:  /* ADD -> ADDU */
							opcode &= ~0x3f;
							opcode |= 0x21;
							break;
						case 0x22:  /* SUB -> SUBU */
							opcode &= ~0x3f;
							opcode |= 0x23;
							break;
						default:
							break;
					}
					break;
				case 0x08:  /* ADDI -> ADDIU */
					opcode &= ~0xfc000000;
					opcode |= 0x24000000;
					break;
				default:
					break;
			}
			ops[i].opcode = opcode;  // Write back opcode in case it was converted

			const u8 dst_reg = ops[i].dst_reg;
			const u8 rs_reg  = _fRs_(opcode);
			const u8 rt_reg  = _fRt_(opcode);

			//  Catch 'ADDU dst, rs, $zero', a reg move: if the src and dst regs
			// haven't been been renamed yet, we can skip renaming the dst reg
			// and emit just a simple conditional move later.
			// ADDU  rd = rs + rt
			if ((_fOp_(opcode) == 0 && _fFunct_(opcode) == 0x21) &&  /* ADDU */
			    rt_reg == 0 &&
			    !reg_map[dst_reg].is_renamed &&
			    !reg_map[rs_reg].is_renamed)
			{
				ops[i].emit_as_rd_rs_direct_cond_move = true;
				continue;
			}

			if (ops[i].reads_rs && reg_map[rs_reg].is_renamed) {
				ops[i].rs_renamed = true;
				ops[i].rs_renamed_to = reg_map[rs_reg].renamed_to;
			}

			if (ops[i].reads_rt && reg_map[rt_reg].is_renamed) {
				ops[i].rt_renamed = true;
				ops[i].rt_renamed_to = reg_map[rt_reg].renamed_to;
			}

			// Rename the dst reg last, so its renaming doesn't affect
			//  the src regs prematurely.
			if (!reg_map[dst_reg].is_renamed) {
				if (num_renamed >= max_renamed) {
					// Oops, our reg-renaming pool is empty, must give up.
					success = false;
					break;
				}

				reg_map[dst_reg].is_renamed = true;
				reg_map[dst_reg].renamed_to = renamed_reg_pool[num_renamed];
				host_to_psx_map[num_renamed] = dst_reg;
				num_renamed++;
			}

			ops[i].dst_renamed_to = reg_map[dst_reg].renamed_to;

			// NOTE: Unless 'ops[i].emit_as_rd_rs_direct_cond_move' was set
			//       true, code will assume that dst_reg was renamed.
		}
	}

	if (!success)
		return false;

	/***********************************************************
	 * STAGE 3: Allocate branch reg(s) and emit BD slot opcode *
	 ***********************************************************/

	//  XXX -  Our reg allocator is not very good yet, and the regs we are
	//        allocated for branch_rs/branch_rt should not be used past the
	//        end of this stage. The allocation mode 'REG_LOADBRANCH' is only
	//        meant to facilitate traditional branch code. They're assumed to
	//        be read-only over their lifespan. The mode's only purpose is to
	//        briefly get copies of the regs read by the branch across the
	//        execution of BD slot code, which could otherwise modify them
	//        and incorrectly affect branch decision.
	//         'REG_LOADBRANCH' allocation mode spills any dirty regs it is
	//        asked to allocate, thinking it is preparing for a possible block
	//        exit. We won't be exiting here, so that is unhelpful. Therefore,
	//        we only use that mode here for regs we know are modified by the
	//        BD slot instruction.
	// TODO -  What we probably want is a new mode we can specify to the
	//        branch allocator: maybe REG_LOADPRIVATE or REG_LOADCOPY

	const u32  bd_slot_writes = (u32)opcodeGetWrites(bd_slot_opcode);

	// All branches read reg in 'rs' field
	bool br1_locked = true;
	u32  br1 = 0;
	if (bd_slot_writes & (1 << branch_rs)) {
		// BD slot modifies a reg read by branch: must get special copy
		br1 = regMipsToHost(branch_rs, REG_LOADBRANCH, REG_REGISTERBRANCH);
	} else {
		br1 = regMipsToHost(branch_rs, REG_LOAD, REG_REGISTER);
	}

	// BEQ,BNE also read reg in 'rt' field
	bool br2_locked = false;
	u32  br2 = 0;
	if (is_beq || is_bne) {
		br2_locked = true;
		if (bd_slot_writes & (1 << branch_rt)) {
			// BD slot modifies a reg read by branch: must get special copy
			br2 = regMipsToHost(branch_rt, REG_LOADBRANCH, REG_REGISTERBRANCH);
		} else {
			br2 = regMipsToHost(branch_rt, REG_LOAD, REG_REGISTER);
		}
	}

	// Recompile opcode in BD slot (only after branch reg allocation)
	recDelaySlot();

	// NOTE: 'pc' is now at instruction after BD slot

	DISASM_MSG("CONVERTING BRANCH TO CONDITIONAL MOVE(S): NOT-TAKEN PATH BEGIN\n");

	/*********************************
	 * STAGE 4: Set up condition reg *
	 *********************************/

	// Conditional-moves will base their decision on this reg.
	// Default is a temp reg, but sometimes we can use the branch's reg.
	u32  condition_reg = temp_condition_reg;
	bool condition_reg_locked = false;

	// Use MOVZ or MOVN for conditional moves?
	bool use_movz = true;

	if (is_beq || is_bne)
	{
		/* BEQ,BNE */

		if (is_beq)
			use_movz = false;

		// NOTE: We assume any '$zero,$zero' comparisons were already caught by
		//       the primary branch emitters long before the call here.

		//  If BEQ/BNE read $zero reg, we can hopefully use the other reg read
		// as the condition reg. Otherwise, we must emit a SUBU instruction and
		// use a temp reg for the condition. Strict requirements must be met,
		// partly for simplicity's sake, partly because our reg allocator is
		// fairly limited (see previous 'XXX' note).
		//
		// REQUIREMENTS:
		//  1.)  The BD slot must not write to the reg read by the branch:
		//      MIPS branch decisions are made *before* any BD slot writeback.
		//  2.)  The reg read by the branch must not be written to inside the
		//      not-taken path. *However*, if there's only one instruction in
		//      the not-taken path, it is OK to forgo this requirement: The
		//      reg is read by the MOVN/MOVZ before it is written back.

		const u32 branch_reads = ((1 << branch_rs) | (1 << branch_rt));

		if ( (branch_rs == 0 || branch_rt == 0) &&
		     ((bd_slot_writes & branch_reads) == 0) &&
		     ((op_writes & branch_reads) == 0 || num_ops == 1) )
		{
			// Keep non-$zero condition reg locked until final MOVN/MOVZs are emitted
			regUnlock(br1);
			regUnlock(br2);
			br1_locked = br2_locked = false;
			condition_reg = regMipsToHost((branch_rt == 0 ? branch_rs : branch_rt), REG_LOAD, REG_REGISTER);
			condition_reg_locked = true;
		} else {
			SUBU(condition_reg, br1, br2);
		}
	} else
	{
		/* BLTZ,BGEZ,BGTZ,BLEZ */

		switch (branch_opcode & 0xfc1f0000)
		{
			case 0x04000000: /* BLTZ */
				SLT(condition_reg, br1, 0);
				break;
			case 0x04010000: /* BGEZ */
				SLT(condition_reg, br1, 0);
				use_movz = false;
				break;
			case 0x1c000000: /* BGTZ */
				SLTI(condition_reg, br1, 1);
				use_movz = false;
				break;
			case 0x18000000: /* BLEZ */
				SLTI(condition_reg, br1, 1);
				break;

			default:
				printf("Error in %s(): branch_opcode=0x%08x\n", __func__, branch_opcode);
				exit(1);
		}
	}

	if (br1_locked)
		regUnlock(br1);
	if (br2_locked)
		regUnlock(br2);

	/****************************************************************************
	 * STAGE 5: Emit ALU opcodes w/ renamed regs + any direct conditional-moves *
	 ****************************************************************************/
	for (int i=0; i < num_ops; ++i)
	{
		const u32 opcode  = ops[i].opcode;
		const u8 orig_dst = ops[i].dst_reg;
		const u8 orig_rs  = _fRs_(opcode);
		const u8 orig_rt  = _fRt_(opcode);

		if (ops[i].emit_as_rd_rs_direct_cond_move)
		{
			if (orig_dst == orig_rs) {
				// What, a move of a reg to itself!? Emit nothing..
				continue;
			}

			// Load *existing* contents of dest reg: they are overwritten based
			//  on the condition reg.
			const u32 dst = regMipsToHost(orig_dst, REG_LOAD, REG_REGISTER);
			const u32 rs  = regMipsToHost(orig_rs,  REG_LOAD, REG_REGISTER);

			if (use_movz)
				MOVZ(dst, rs, condition_reg);
			else
				MOVN(dst, rs, condition_reg);

			regUnlock(rs);
			regUnlock(dst);
			regMipsChanged(orig_dst);
			SetUndef(orig_dst);
		} else
		{
			u32  new_opcode = opcode;
			u32  rs = 0;
			u32  rt = 0;
			bool rs_locked = false;
			bool rt_locked = false;

			if (ops[i].reads_rs) {
				// Replace 'rs' opcode field with renamed or allocated reg
				new_opcode &= ~(0x1f << 21);
				if (ops[i].rs_renamed) {
					new_opcode |= (ops[i].rs_renamed_to << 21);
				} else {
					rs = regMipsToHost(orig_rs, REG_LOAD, REG_REGISTER);
					rs_locked = true;
					new_opcode |= (rs << 21);
				}
			}

			if (ops[i].reads_rt) {
				// Replace 'rt' opcode field with renamed or allocated reg
				new_opcode &= ~(0x1f << 16);

				if (ops[i].rt_renamed) {
					new_opcode |= (ops[i].rt_renamed_to << 16);
				} else {
					// Insert allocated reg in rt field
					rt = regMipsToHost(orig_rt, REG_LOAD, REG_REGISTER);
					rt_locked = true;
					new_opcode |= (rt << 16);
				}
			}

			// Replace dst opcode field with renamed reg
			if (ops[i].writes_rt) {
				// Opcode writes to 'rt'
				new_opcode &= ~(0x1f << 16);
				new_opcode |= (ops[i].dst_renamed_to << 16);
			} else {
				// Opcode writes to 'rd'
				new_opcode &= ~(0x1f << 11);
				new_opcode |= (ops[i].dst_renamed_to << 11);
			}

			// Emit ALU opcode w/ renamed register(s)
			write32(new_opcode);

			if (rs_locked)
				regUnlock(rs);
			if (rt_locked)
				regUnlock(rt);
		}
	}

	/************************************************************
	 * STAGE 6: Emit conditional-moves for any renamed dst regs *
	 ************************************************************/

	for (int i=0; i < num_renamed; ++i)
	{
		// Load *existing* contents of dest reg: they are overwritten based
		//  on the condition reg.
		const u32 renamed_dst_reg = renamed_reg_pool[i];
		const u32 orig_dst_reg = host_to_psx_map[i];
		const u32 dst = regMipsToHost(orig_dst_reg, REG_LOAD, REG_REGISTER);

		if (use_movz)
			MOVZ(dst, renamed_dst_reg, condition_reg);
		else
			MOVN(dst, renamed_dst_reg, condition_reg);

		regUnlock(dst);
		regMipsChanged(orig_dst_reg);
		SetUndef(orig_dst_reg);
	}

	// Did we use a branch reg as the condition reg, not a temp reg?
	if (condition_reg_locked)
		regUnlock(condition_reg);

	// NOTE: 'pc' is already at the instruction after the delay slot.

	// Disassemble all opcodes between the BD slot and the branch target PC.
	for (int i=0; i < (branch_imm-1); ++i)
		DISASM_PSX(pc + i*4);

	DISASM_MSG("CONVERTING BRANCH TO CONDITIONAL MOVE(S): NOT-TAKEN PATH END\n");

	// We're done: recompilation will resume at the branch target pc.
	pc += (branch_imm-1)*4;

	return true;
}
#endif // USE_CONDITIONAL_MOVE_OPTIMIZATIONS
