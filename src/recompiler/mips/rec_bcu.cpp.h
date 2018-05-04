static void recSYSCALL()
{
  regClearJump();

  LI32(TEMP_1, pc - 4);
  SW(TEMP_1, PERM_REG_1, off(pc));

  LI16(MIPSREG_A1, (branch == 1 ? 1 : 0));
  JAL(psxException);
  LI16(MIPSREG_A0, 0x20); // <BD> Load first param using BD slot of JAL()

  rec_recompile_end_part1();
  LW(MIPSREG_V0, PERM_REG_1, off(pc)); // Block retval $v0 = new PC set by psxException()
  rec_recompile_end_part2();

  end_block = 1;
}

/* Check if an opcode has a delayed read if in delay slot */
static int iLoadTest(u32 code)
{
  // check for load delay
  u32 op = _fOp_(code);
  switch (op)
  {
    case 0x10: // COP0
      switch (_fRs_(code))
      {
        case 0x00: // MFC0
        case 0x02: // CFC0
          return 1;
      }
      break;
    case 0x12: // COP2
      switch (_fFunct_(code))
      {
        case 0x00:
          switch (_fRs_(code))
          {
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
      if (op >= 0x20 && op <= 0x26)
      {
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
  if (iLoadTest(code1))
  {
    int i = psxTestLoadDelay(reg, code2);
    if (i == 1 || i == 2)
    {
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

  if (iLoadTest(code1))
  {
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

/* Used for BLTZ, BGTZ, BLTZAL, BGEZAL, BLEZ, BGEZ */
static void emitBxxZ(int andlink, u32 bpc, u32 nbpc)
{
  u32 code = psxRegs.code;
  u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);

  int dt = DelayTest(pc, bpc);

  if (dt == 3 || dt == 0)
  {
    recDelaySlot();
  }

  u32 *backpatch = (u32 *)recMem;

  // Check opcode and emit branch with REVERSED logic!
  switch (code & 0xfc1f0000)
  {
    case 0x04000000: /* BLTZ */
    case 0x04100000: /* BLTZAL */
      BGEZ(br1, 0);
      break;
    case 0x04010000: /* BGEZ */
    case 0x04110000: /* BGEZAL */
      BLTZ(br1, 0);
      break;
    case 0x1c000000: /* BGTZ */
      BLEZ(br1, 0);
      break;
    case 0x18000000: /* BLEZ */
      BGTZ(br1, 0);
      break;
    default:
      printf("Error opcode=%08x\n", code);
      exit(1);
  }

  regPushState();
  u32 ra = 0;

  if (dt == 2)
  {
    NOP(); /* <BD> */

    // Instruction at target PC should see ra reg write
    if (andlink)
    {
      ra = regMipsToHost(31, REG_FIND, REG_REGISTER);
      LI32(ra, nbpc);
      regMipsChanged(31);
      // Cannot set const because branch-not-taken path wouldn't see it:
      SetUndef(31);
    }

    recRevDelaySlot(pc, bpc);
    bpc += 4;
  }

  LUI(TEMP_1, (bpc >> 16)); /* <BD> */

  rec_recompile_end_part1();
  regClearBranch();
  ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff));

  if (andlink && dt != 2)
  {
    if ((bpc >> 16) == (nbpc >> 16))
    {
      // Both PCs share an upper half, can save an instruction:
      ORI(TEMP_1, TEMP_1, (nbpc & 0xffff));
    }
    else
    {
      LI32(TEMP_1, nbpc);
    }
    SW(TEMP_1, PERM_REG_1, offGPR(31));
  }

  rec_recompile_end_part2();

  regPopState();

  fixup_branch(backpatch);
  regUnlock(br1);

  if (dt != 3 && dt != 0)
  {
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
  u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
  u32 br2 = regMipsToHost(_Rt_, REG_LOADBRANCH, REG_REGISTERBRANCH);
  recDelaySlot();
  u32 *backpatch = (u32 *)recMem;


  // Check opcode and emit branch with REVERSED logic!
  switch (code & 0xfc000000)
  {
    case 0x10000000: /* BEQ */
      BNE(br1, br2, 0);
      break;
    case 0x14000000: /* BNE */
      BEQ(br1, br2, 0);
      break;
    default:
      printf("Error opcode=%08x\n", code);
      exit(1);
  }

  LUI(TEMP_1, (bpc >> 16)); /* <BD> */

  rec_recompile_end_part1();
  regClearBranch();
  ORI(MIPSREG_V0, TEMP_1, (bpc & 0xffff));

  rec_recompile_end_part2();

  fixup_branch(backpatch);
  regUnlock(br1);
  regUnlock(br2);
}

static void iJumpNormal(u32 bpc)
{
#ifdef LOG_BRANCHLOADDELAYS
  u32 dt = DelayTest(pc, bpc);
#endif

  recDelaySlot();

  rec_recompile_end_part1();
  regClearJump();
  LI32(MIPSREG_V0, bpc); // Block retval $v0 = new PC val
  rec_recompile_end_part2();

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
  if (dt == 2)
  {
    recRevDelaySlot(pc, bpc);
    bpc += 4;
  }
  else if (dt == 3 || dt == 0)
  {
    recDelaySlot();
  }

  rec_recompile_end_part1();
  regClearJump();
  LI32(MIPSREG_V0, bpc);     // Block retval $v0 = new PC val
  rec_recompile_end_part2();

  end_block = 1;
}

static void recBLTZ()
{
// Branch if Rs < 0
  u32 bpc = _Imm_ * 4 + pc;
  u32 nbpc = pc + 4;

  if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0)
  {
    return;
  }

  if (!(_Rs_))
  {
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

  if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0)
  {
    return;
  }

  if (!(_Rs_))
  {
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

  if (!(_Rs_))
  {
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

  if (!(_Rs_))
  {
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
  if (i == 1 || i == 2)
  {
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

  switch (psxTestLoadDelay(_fRt_(code1), code2))
  {
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

  rec_recompile_end_part1();
  pc += 4;
  regUnlock(br1);
  rec_recompile_end_part2();

  end_block = 1;
}

static void recJR()
{
// jr Rs
  u32 code = *(u32 *)((char *)PSXM(pc)); // opcode in branch delay

  // if possible read delay in branch delay slot
  if (iLoadTest(code))
  {
    recJR_load_delay();

    return;
  }

  u32 br1 = regMipsToHost(_Rs_, REG_LOADBRANCH, REG_REGISTERBRANCH);
  recDelaySlot();

  rec_recompile_end_part1();
  regClearJump();
  MOV(MIPSREG_V0, br1); // Block retval $v0 = new PC val
  regUnlock(br1);
  rec_recompile_end_part2();

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

  rec_recompile_end_part1();
  regClearJump();
  MOV(MIPSREG_V0, br1); // Block retval $v0 = new PC val
  regUnlock(br1);
  rec_recompile_end_part2();

  end_block = 1;
}

static void recBEQ()
{
// Branch if Rs == Rt
  u32 bpc = _Imm_ * 4 + pc;
  u32 nbpc = pc + 4;

  if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0)
  {
    return;
  }

  if (_Rs_ == _Rt_)
  {
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

  if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0)
  {
    return;
  }

  if (!(_Rs_) && !(_Rt_))
  {
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

  if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0)
  {
    return;
  }

  if (!(_Rs_))
  {
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

  if (bpc == nbpc && psxTestLoadDelay(_Rs_, PSXMu32(bpc)) == 0)
  {
    return;
  }

  if (!(_Rs_))
  {
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

  rec_recompile_end_part1();
  LW(MIPSREG_V0, PERM_REG_1, off(pc)); // <BD> Block retval $v0 = psxRegs.pc
  rec_recompile_end_part2();

  end_block = 1;
}
