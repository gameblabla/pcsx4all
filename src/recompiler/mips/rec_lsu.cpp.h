// Generate inline psxMemRead/Write or call them as-is
#define USE_DIRECT_MEM_ACCESS
#define USE_CONST_ADDRESSES

// Upper/lower 64K of PSX RAM (psxM[]) is now mirrored to virtual address
//  regions surrounding psxM[]. This allows skipping mirror-region boundary
//  check which special-cased loads/stores that crossed the boundary, the
//  Einhander game fix. See notes in psxmem.cpp.
#if defined(SHMEM_MIRRORING) || defined(TMPFS_MIRRORING)
  #define SKIP_SAME_2MB_REGION_CHECK
#endif

#define OPCODE(insn, rt, rn, imm) \
  write32((insn) | ((rn) << 21) | ((rt) << 16) | ((imm) & 0xffff))

//#define LOG_WL_WR
//#define LOG_LOADS
//#define LOG_STORES

#if defined(LOG_LOADS) || defined (LOG_LOADS) || defined (LOG_WL_WR)
static void disasm_psx(u32 pc)
{
  static char buffer[512];
  u32 opcode = *(u32 *)((char *)PSXM(pc));
  disasm_mips_instruction(opcode, buffer, pc, 0, 0);
  printf("%08x: %08x %s\n", pc, opcode, buffer);
}
#endif

/* Emit address calculation and store to TEMP_2 */
static void emitAddrCalc(u32 r1)
{
  // IMPORTANT: emitAddrCalc() promises to only write to TEMP_1,TEMP_2

  if ((u32)psxM == 0x10000000)
  {
    // psxM base is mmap'd at virtual address 0x10000000
    LUI(TEMP_2, 0x1000);
#ifdef HAVE_MIPS32R2_EXT_INS
    INS(TEMP_2, r1, 0, 0x15); // TEMP_2 = 0x10000000 | (r1 & 0x1fffff)
#else
    SLL(TEMP_1, r1, 11);
    SRL(TEMP_1, TEMP_1, 11);
    OR(TEMP_2, TEMP_2, TEMP_1);
#endif
  }
  else
  {
    LW(TEMP_2, PERM_REG_1, off(psxM));
#ifdef HAVE_MIPS32R2_EXT_INS
    EXT(TEMP_1, r1, 0, 0x15);
#else
    SLL(TEMP_1, r1, 11);
    SRL(TEMP_1, TEMP_1, 11);
#endif
    ADDU(TEMP_2, TEMP_2, TEMP_1);
  }
}

s32 imm_max, imm_min;

static int calc_loads()
{
  int count = 0;
  u32 PC = pc;
  u32 opcode = psxRegs.code;
  u32 rs = _Rs_;

  imm_min = imm_max = _fImm_(opcode);

  /* If in delay slot, set count to 1 */
  if (branch)
    return 1;

  /* Allow LB, LBU, LH, LHU and LW */
  /* rs should be the same, imm and rt could be different */
  while ((_fOp_(opcode) == 0x20 || _fOp_(opcode) == 0x24 ||
          _fOp_(opcode) == 0x21 || _fOp_(opcode) == 0x25 ||
          _fOp_(opcode) == 0x23) && (rs == _fRs_(opcode)))
  {

    /* Update min and max immediate values */
    if (_fImm_(opcode) > imm_max) imm_max = _fImm_(opcode);
    if (_fImm_(opcode) < imm_min) imm_min = _fImm_(opcode);

    opcode = *(u32 *)((char *)PSXM(PC));

    PC += 4;
    count++;

    /* Extra paranoid check if rt == rs */
    if (_fRt_(opcode) == _fRs_(opcode))
      break;
  }

#ifdef LOG_LOADS
  if (count)
  {
    printf("\nFOUND %d loads, min: %d, max: %d\n", count, imm_min, imm_max);
    u32 dpc = pc - 4;
    for (; dpc < PC - 4; dpc += 4)
      disasm_psx(dpc);
  }
#endif

  return count;
}

static int calc_stores()
{
  int count = 0;
  u32 PC = pc;
  u32 opcode = psxRegs.code;
  u32 rs = _Rs_;

  imm_min = imm_max = _fImm_(opcode);

  /* If in delay slot, set count to 1 */
  if (branch)
    return 1;

  /* Allow SB, SH and SW */
  /* rs should be the same, imm and rt could be different */
  while (((_fOp_(opcode) == 0x28) || (_fOp_(opcode) == 0x29) ||
          (_fOp_(opcode) == 0x2b)) && (rs == _fRs_(opcode)))
  {

    /* Update min and max immediate values */
    if (_fImm_(opcode) > imm_max) imm_max = _fImm_(opcode);
    if (_fImm_(opcode) < imm_min) imm_min = _fImm_(opcode);

    opcode = *(u32 *)((char *)PSXM(PC));

    PC += 4;
    count++;
  }

#ifdef LOG_STORES
  if (count)
  {
    printf("\nFOUND %d stores, min: %d, max: %d\n", count, imm_min, imm_max);
    u32 dpc = pc - 4;
    for (; dpc < PC - 4; dpc += 4)
      disasm_psx(dpc);
  }
#endif

  return count;
}

static void LoadFromAddr(int count, bool force_indirect)
{
  // Rt = [Rs + imm16]
  u32 r1 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
  u32 PC = pc - 4;

#ifdef WITH_DISASM
  for (int i = 0; i < count-1; i++)
    DISASM_PSX(pc + i * 4);
#endif

#ifdef USE_DIRECT_MEM_ACCESS
  u32 *backpatch_label_exit_1 = 0;
  if (!force_indirect)
  {
    regPushState();

    // Is address in lower 8MB region? (2MB mirrored x4)
    //  We check only the effective address of first load in the series,
    // seeing if bits 27:24 are unset to determine if it is in lower 8MB.
    // See comments in StoreToAddr().

    // Get the effective address of first load in the series.
    // ---> NOTE: leave value in MIPSREG_A0, it will be used later!
    ADDIU(MIPSREG_A0, r1, _Imm_);

#ifdef HAVE_MIPS32R2_EXT_INS
    EXT(MIPSREG_A1, MIPSREG_A0, 24, 4);
#else
    LUI(MIPSREG_A1, 0x0f00);
    AND(MIPSREG_A1, MIPSREG_A1, MIPSREG_A0);
#endif
    u32 *backpatch_label_hle_1 = (u32 *)recMem;
    BGTZ(MIPSREG_A1, 0); // beqz MIPSREG_A1, label_hle

    // NOTE: Branch delay slot contains next emitted instruction,
    //       which should not write to MIPSREG_A1

#ifndef SKIP_SAME_2MB_REGION_CHECK
    /* Check if addr and addr+imm are in the same 2MB region */
#ifdef HAVE_MIPS32R2_EXT_INS
    EXT(TEMP_3, r1, 21, 3);             // <BD slot> TEMP_3 = (r1 >> 21) & 0x7
    EXT(MIPSREG_A1, MIPSREG_A0, 21, 3); // MIPSREG_A1 = (MIPSREG_A0 >> 21) & 0x7
#else
    SRL(TEMP_3, r1, 21);                // <BD slot>
    ANDI(TEMP_3, TEMP_3, 7);
    SRL(MIPSREG_A1, MIPSREG_A0, 21);
    ANDI(MIPSREG_A1, MIPSREG_A1, 7);
#endif
    u32 *backpatch_label_hle_2 = (u32 *)recMem;
    BNE(MIPSREG_A1, TEMP_3, 0);         // goto label_hle if not in same 2MB region

    // NOTE: Branch delay slot contains next emitted instruction,
    //       which should not write to MIPSREG_A1, TEMP_3
#endif

    // NOTE: emitAddrCalc() promises to only write to TEMP_1, TEMP_2
    emitAddrCalc(r1); // TEMP_2 == recalculated addr

    int icount = count;
    do
    {
      u32 opcode = *(u32 *)((char *)PSXM(PC));
      s32 imm = _fImm_(opcode);
      u32 rt = _fRt_(opcode);
      u32 r2 = regMipsToHost(rt, REG_FIND, REG_REGISTER);

      if (icount == 1)
      {
        // This is the end of the loop
        backpatch_label_exit_1 = (u32 *)recMem;
        B(0); // b label_exit
        // NOTE: Branch delay slot will contain the instruction below
      }
      // Important: this should be the last opcode in the loop (see note above)
      OPCODE(opcode & 0xfc000000, r2, TEMP_2, imm);

      SetUndef(rt);
      regMipsChanged(rt);
      regUnlock(r2);

      PC += 4;
    }
    while (--icount);

    PC = pc - 4;

    regPopState();

    // label_hle:
    fixup_branch(backpatch_label_hle_1);
#ifndef SKIP_SAME_2MB_REGION_CHECK
    fixup_branch(backpatch_label_hle_2);
#endif
  }
#endif //USE_DIRECT_MEM_ACCESS

  do
  {
    u32 opcode = *(u32 *)((char *)PSXM(PC));
    u32 rt = _fRt_(opcode);
    s32 imm = _fImm_(opcode);
    u32 r2 = regMipsToHost(rt, REG_FIND, REG_REGISTER);

    switch (opcode & 0xfc000000)
    {
      case 0x80000000: // LB
        JAL(psxMemRead8);
        ADDIU(MIPSREG_A0, r1, imm); // <BD> Branch delay slot
#ifdef HAVE_MIPS32R2_SEB_SEH
        SEB(r2, MIPSREG_V0);
#else
        SLL(r2, MIPSREG_V0, 24);
        SRA(r2, r2, 24);
#endif
        break;
      case 0x90000000: // LBU
        JAL(psxMemRead8);
        ADDIU(MIPSREG_A0, r1, imm); // <BD> Branch delay slot
        MOV(r2, MIPSREG_V0);
        break;
      case 0x84000000: // LH
        JAL(psxMemRead16);
        ADDIU(MIPSREG_A0, r1, imm); // <BD> Branch delay slot
#ifdef HAVE_MIPS32R2_SEB_SEH
        SEH(r2, MIPSREG_V0);
#else
        SLL(r2, MIPSREG_V0, 16);
        SRA(r2, r2, 16);
#endif
        break;
      case 0x94000000: // LHU
        JAL(psxMemRead16);
        ADDIU(MIPSREG_A0, r1, imm); // <BD> Branch delay slot
        MOV(r2, MIPSREG_V0);
        break;
      case 0x8c000000: // LW
        JAL(psxMemRead32);
        ADDIU(MIPSREG_A0, r1, imm); // <BD> Branch delay slot
        MOV(r2, MIPSREG_V0);
        break;
    }

    SetUndef(rt);
    regMipsChanged(rt);
    regUnlock(r2);

    PC += 4;
  }
  while (--count);

#ifdef USE_DIRECT_MEM_ACCESS
  // label_exit:
  if (!force_indirect)
    fixup_branch(backpatch_label_exit_1);
#endif

  pc = PC;
  regUnlock(r1);
}

static void LoadFromConstAddr()
{
  int count = calc_loads();

  bool const_addr = false;
#ifdef USE_CONST_ADDRESSES
  const_addr = IsConst(_Rs_);
#endif

  if (!const_addr)
  {
    // Call general-case emitter for non-const addr
    LoadFromAddr(count, false);
    return;
  }

  // Is address in lower 8MB region? (2MB mirrored x4)
  u32 addr_max = iRegs[_Rs_].r + imm_max;
  if ((addr_max & 0x1fffffff) >= 0x800000)
  {
    // Call general-case emitter, but force indirect access since
    //  known-const address is outside lower 8MB RAM.
    LoadFromAddr(count, true);
    return;
  }

  //////////////////////////////
  // Handle const RAM address //
  //////////////////////////////

#ifdef WITH_DISASM
  for (int i = 0; i < count-1; i++)
    DISASM_PSX(pc + i * 4);
#endif

  u32 PC = pc - 4;

  // Keep upper mem address in a register, but track its current value
  // so we avoid unnecessarily loading same value repeatedly
  u16 mem_addr_hi = 0;

  int icount = count;
  do
  {
    // Paranoia check: was base reg written to by last iteration?
    if (!IsConst(_Rs_))
      break;

    u32 opcode = *(u32 *)((char *)PSXM(PC));
    s32 imm = _fImm_(opcode);
    u32 rt = _fRt_(opcode);
    u32 r2 = regMipsToHost(rt, REG_FIND, REG_REGISTER);

    u32 mem_addr = (u32)psxM + ((iRegs[_Rs_].r + imm) & 0x1fffff);

    if ((icount == count) || (ADR_HI(mem_addr) != mem_addr_hi))
    {
      mem_addr_hi = ADR_HI(mem_addr);
      LUI(TEMP_2, ADR_HI(mem_addr));
    }

    OPCODE(opcode & 0xfc000000, r2, TEMP_2, ADR_LO(mem_addr));

    SetUndef(rt);
    regMipsChanged(rt);
    regUnlock(r2);
    PC += 4;
  }
  while (--icount);

  pc = PC;
}

static void StoreToAddr(int count, bool force_indirect)
{
  int icount;
  u32 r1 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
  u32 PC = pc - 4;

#ifdef WITH_DISASM
  for (int i = 0; i < count-1; i++)
    DISASM_PSX(pc + i * 4);
#endif

#ifdef USE_DIRECT_MEM_ACCESS
  // Load psxRegs.writeok to see if RAM is writeable
  // (load it early to reduce load stall at check below)
  if (!force_indirect)
  {
    if (!Config.HLE)
      LW(MIPSREG_A1, PERM_REG_1, off(writeok));
  }
#endif

  // Get the effective address of first store in the series.
  // ---> NOTE: leave value in MIPSREG_A0, it will be used later!
  ADDIU(MIPSREG_A0, r1, _Imm_);

#ifdef USE_DIRECT_MEM_ACCESS
  u32 *backpatch_label_exit_1 = 0;
  if (!force_indirect)
  {
    regPushState();

    // Check if memory is writable and also check if address is in lower 8MB:
    //
    //   This is just slightly tricky. All addresses in lower 8MB of RAM will
    //  have bits 27:24 unset, and all other valid addresses have them set.
    //  The check below only passes when 'writeok' is 1 and bits 27:24 are 0.
    //   It is not possible for a series of stores sharing a base register to
    //  write both inside and outside of lower-8MB RAM region: signed 16bit imm
    //  offset is not large enough to reach a valid mapped address in both.
    //   Writes to 0xFFFF_xxxx and 0x0080_xxxx regions would cause an exception
    //  on a real PS1, so no need to worry about imm offsets reaching them.
    //  Base addresses with offsets wrapping to next/prior mirror region are
    //  handled either with masking further below, or by emulation of mirrors
    //  using mmap'd virtual mem (SKIP_SAME_2MB_REGION_CHECK, see psxmem.cpp).
    //    MIPSREG_A0 is left set to the eff address of first store in series,
    //  saving emitting an instruction in first loop iterations further below.
    // ---- Equivalent C code: ----
    // if ( !((r1+imm_of_first_store) & 0x0f00_0000) < writeok) )
    //    goto label_hle_1;

    u32 *backpatch_label_hle_1;
#ifdef HAVE_MIPS32R2_EXT_INS
    EXT(TEMP_3, MIPSREG_A0, 24, 4);
#else
    LUI(TEMP_3, 0x0f00);
    AND(TEMP_3, TEMP_3, MIPSREG_A0);
#endif
    if (!Config.HLE)
    {
      SLTU(MIPSREG_A1, TEMP_3, MIPSREG_A1);
      backpatch_label_hle_1 = (u32 *)recMem;
      BEQZ(MIPSREG_A1, 0);
    }
    else
    {
      backpatch_label_hle_1 = (u32 *)recMem;
      BNE(TEMP_3, 0, 0);
    }
    // NOTE: Branch delay slot contains next emitted instruction,
    //       which should not write to MIPSREG_A1

#ifndef SKIP_SAME_2MB_REGION_CHECK
    /* Check if addr and addr+imm are in the same 2MB region */
#ifdef HAVE_MIPS32R2_EXT_INS
    EXT(TEMP_3, r1, 21, 3);             // <BD slot> TEMP_3 = (r1 >> 21) & 0x7
    EXT(MIPSREG_A1, MIPSREG_A0, 21, 3); // MIPSREG_A1 = (MIPSREG_A0 >> 21) & 0x7
#else
    SRL(TEMP_3, r1, 21);                // <BD slot>
    ANDI(TEMP_3, TEMP_3, 7);
    SRL(MIPSREG_A1, MIPSREG_A0, 21);
    ANDI(MIPSREG_A1, MIPSREG_A1, 7);
#endif
    u32 *backpatch_label_hle_2 = (u32 *)recMem;
    BNE(MIPSREG_A1, TEMP_3, 0);         // goto label_hle if not in same 2MB region
    // NOTE: Branch delay slot contains next emitted instruction,
    //       which should not write to MIPSREG_A1, TEMP_3
#endif

    // NOTE: emitAddrCalc() promises to only write to TEMP_1, TEMP_2
    emitAddrCalc(r1); // TEMP_2 == recalculated addr

    icount = count;
    LUI(TEMP_3, ADR_HI(recRAM)); // temp_3 = upper code block ptr array addr
    do
    {
      u32 opcode = *(u32 *)((char *)PSXM(PC));
      s32 imm = _fImm_(opcode);
      u32 rt = _fRt_(opcode);
      u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

      OPCODE(opcode & 0xfc000000, r2, TEMP_2, imm);

      /* Invalidate recRAM[addr+imm16] pointer */
      if (icount != count)
      {
        // No need to do this for the first store of the series,
        //  as it was already done for us during initial checks.
        ADDIU(MIPSREG_A0, r1, imm);
      }

#ifdef HAVE_MIPS32R2_EXT_INS
      EXT(TEMP_1, MIPSREG_A0, 0, 0x15); // and 0x1fffff
#else
      SLL(TEMP_1, MIPSREG_A0, 11);
      SRL(TEMP_1, TEMP_1, 11);
#endif

      if ((opcode & 0xfc000000) != 0xac000000)
      {
        // Not a SW, clear lower 2 bits to ensure addr is aligned:
#ifdef HAVE_MIPS32R2_EXT_INS
        INS(TEMP_1, 0, 0, 2);
#else
        SRL(TEMP_1, TEMP_1, 2);
        SLL(TEMP_1, TEMP_1, 2);
#endif
      }
      ADDU(TEMP_1, TEMP_1, TEMP_3);

      backpatch_label_exit_1 = 0;
      if (icount == 1)
      {
        // This is the end of the loop
        backpatch_label_exit_1 = (u32 *)recMem;
        B(0); // b label_exit
        // NOTE: Branch delay slot will contain the instruction below
      }
      // Important: this should be the last opcode in the loop (see note above)
      SW(0, TEMP_1, ADR_LO(recRAM));  // set code block ptr to NULL

      PC += 4;

      regUnlock(r2);
    }
    while (--icount);

    PC = pc - 4;

    regPopState();

    // label_hle:
    fixup_branch(backpatch_label_hle_1);
#ifndef SKIP_SAME_2MB_REGION_CHECK
    fixup_branch(backpatch_label_hle_2);
#endif
  }
#endif // USE_DIRECT_MEM_ACCESS

  icount = count;
  do
  {
    u32 opcode = *(u32 *)((char *)PSXM(PC));
    u32 rt = _fRt_(opcode);
    s32 imm = _fImm_(opcode);
    u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

    if (icount != count)
    {
      // No need to do this for the first store of the series,
      //  as it was already done for us during initial checks.
      ADDIU(MIPSREG_A0, r1, imm);
    }

    switch (opcode & 0xfc000000)
    {
      case 0xa0000000:
        JAL(psxMemWrite8);
        MOV(MIPSREG_A1, r2); // <BD> Branch delay slot
        break;
      case 0xa4000000:
        JAL(psxMemWrite16);
        MOV(MIPSREG_A1, r2); // <BD> Branch delay slot
        break;
      case 0xac000000:
        JAL(psxMemWrite32);
        MOV(MIPSREG_A1, r2); // <BD> Branch delay slot
        break;
      default:
        break;
    }

    PC += 4;

    regUnlock(r2);
  }
  while (--icount);

#ifdef USE_DIRECT_MEM_ACCESS
  // label_exit:
  if (!force_indirect)
    fixup_branch(backpatch_label_exit_1);
#endif

  pc = PC;
  regUnlock(r1);
}

static void StoreToConstAddr()
{
  int count = calc_stores();

  bool const_addr = false;
#ifdef USE_CONST_ADDRESSES
  const_addr = IsConst(_Rs_);
#endif

  if (!const_addr)
  {
    // Call general-case emitter for non-const addr
    StoreToAddr(count, false);
    return;
  }

  // Is address in lower 8MB region? (2MB mirrored x4)
  u32 addr_max = iRegs[_Rs_].r + imm_max;
  if ((addr_max & 0x1fffffff) >= 0x800000)
  {
    // Call general-case emitter, but force indirect access since
    //  known-const address is outside lower 8MB RAM.
    StoreToAddr(count, true);
    return;
  }

  //////////////////////////////
  // Handle const RAM address //
  //////////////////////////////

#ifdef WITH_DISASM
  for (int i = 0; i < count-1; i++)
    DISASM_PSX(pc + i * 4);
#endif

  u32 PC = pc - 4;

  u32 *backpatch_label_no_write = NULL;
  if (!Config.HLE)
    LW(TEMP_1, PERM_REG_1, off(writeok));

  // Keep upper half of last code block and RAM addresses in regs,
  // tracking current values so we can avoid loading same val repeatedly.
  u16 mem_addr_hi = 0;
  u16 code_addr_hi = 0;

  u32 last_code_addr = 0;

  int icount = count;
  do
  {
    u32 opcode = *(u32 *)((char *)PSXM(PC));
    s32 imm = _fImm_(opcode);
    u32 rt = _fRt_(opcode);
    u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

    u32 mem_addr = (u32)psxM + ((iRegs[_Rs_].r + imm) & 0x1fffff);
    u32 code_addr = (u32)recRAM + ((iRegs[_Rs_].r + imm) & 0x1fffff);
    code_addr &= ~3;  // Align code block ptr address

    if ((icount == count) || (ADR_HI(code_addr) != code_addr_hi))
    {
      code_addr_hi = ADR_HI(code_addr);
      LUI(TEMP_3, ADR_HI(code_addr));
    }

    // Skip RAM write if psxRegs.writeok == 0
    if (!Config.HLE)
    {
      backpatch_label_no_write = (u32 *)recMem;
      BEQZ(TEMP_1, 0);  // if (!psxRegs.writeok) goto label_no_write
      // NOTE: Branch delay slot contains next instruction emitted below
    }

    if (code_addr != last_code_addr)
    {
      // Set code block ptr to NULL
      last_code_addr = code_addr;
      SW(0, TEMP_3, ADR_LO(code_addr));  // <BD slot>

      if ((icount == count) || (ADR_HI(mem_addr) != mem_addr_hi))
      {
        mem_addr_hi = ADR_HI(mem_addr);
        LUI(TEMP_2, ADR_HI(mem_addr));
      }
    }
    else
    {
      // Last code address is same. Rather than spam store buffer
      //  with duplicate write, use BD slot to load upper mem address
      //  even if it might be same as last iteration.
      mem_addr_hi = ADR_HI(mem_addr);
      LUI(TEMP_2, ADR_HI(mem_addr));  // <BD slot>
    }

    // Write to RAM:
    OPCODE(opcode & 0xfc000000, r2, TEMP_2, ADR_LO(mem_addr));

    // label_no_write:
    if (!Config.HLE)
      fixup_branch(backpatch_label_no_write);

    regUnlock(r2);
    PC += 4;
  }
  while (--icount);

  pc = PC;
}

static void recLB()
{
  // Rt = mem[Rs + Im] (signed)
  LoadFromConstAddr();
}

static void recLBU()
{
  // Rt = mem[Rs + Im] (unsigned)
  LoadFromConstAddr();
}

static void recLH()
{
  // Rt = mem[Rs + Im] (signed)
  LoadFromConstAddr();
}

static void recLHU()
{
  // Rt = mem[Rs + Im] (unsigned)
  LoadFromConstAddr();
}

static void recLW()
{
  // Rt = mem[Rs + Im] (unsigned)
  LoadFromConstAddr();
}

static void recSB()
{
  // mem[Rs + Im] = Rt
  StoreToConstAddr();
}

static void recSH()
{
  // mem[Rs + Im] = Rt
  StoreToConstAddr();
}

static void recSW()
{
  // mem[Rs + Im] = Rt
  StoreToConstAddr();
}

static u32 LWL_MASKSHIFT[8] = { 0xffffff, 0xffff, 0xff, 0,
                                24, 16, 8, 0
                              };
static u32 LWR_MASKSHIFT[8] = { 0, 0xff000000, 0xffff0000, 0xffffff00,
                                0, 8, 16, 24
                              };

static void gen_LWL_LWR(int count, bool force_indirect)
{
  int icount;
  u32 r1 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
  u32 PC = pc - 4;

#ifdef WITH_DISASM
  for (int i = 0; i < count-1; i++)
    DISASM_PSX(pc + i * 4);
#endif

  // Get the effective address of first store in the series.
  // ---> NOTE: leave value in MIPSREG_A0, it will be used later!
  ADDIU(MIPSREG_A0, r1, _Imm_);

#ifdef USE_DIRECT_MEM_ACCESS
  u32 *backpatch_label_exit_1 = 0;
  if (!force_indirect)
  {
    regPushState();

    // Is address in lower 8MB region? (2MB mirrored x4)
    //  We check only the effective address of first load in the series,
    // seeing if bits 27:24 are unset to determine if it is in lower 8MB.
    // See comments in StoreToAddr() for explanation of check.

#ifdef HAVE_MIPS32R2_EXT_INS
    EXT(MIPSREG_A1, MIPSREG_A0, 24, 4);
#else
    LUI(MIPSREG_A1, 0x0f00);
    AND(MIPSREG_A1, MIPSREG_A1, MIPSREG_A0);
#endif
    u32 *backpatch_label_hle_1 = (u32 *)recMem;
    BGTZ(MIPSREG_A1, 0); // beqz MIPSREG_A1, label_hle

    // NOTE: Branch delay slot contains next emitted instruction,
    //       which should not write to MIPSREG_A1

    // NOTE: emitAddrCalc() promises to only write to TEMP_1, TEMP_2
    emitAddrCalc(r1); // TEMP_2 == recalculated addr

    icount = count;
    do
    {
      u32 opcode = *(u32 *)((char *)PSXM(PC));
      s32 imm = _fImm_(opcode);
      u32 rt = _fRt_(opcode);
      u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

      if (icount == 1)
      {
        // This is the end of the loop
        backpatch_label_exit_1 = (u32 *)recMem;
        B(0); // b label_exit
        // NOTE: Branch delay slot will contain the instruction below
      }
      // Important: this should be the last instruction in the loop (see note above)
      OPCODE(opcode & 0xfc000000, r2, TEMP_2, imm);

      SetUndef(rt);
      regMipsChanged(rt);
      regUnlock(r2);

      PC += 4;
    }
    while (--icount);

    PC = pc - 4;

    regPopState();

    // label_hle:
    fixup_branch(backpatch_label_hle_1);
  }
#endif // USE_DIRECT_MEM_ACCESS

  icount = count;
  do
  {
    u32 opcode = *(u32 *)((char *)PSXM(PC));
    u32 insn = opcode & 0xfc000000;
    u32 rt = _fRt_(opcode);
    s32 imm = _fImm_(opcode);
    u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

    if (icount != count)
    {
      // No need to do this for the first load of the series, as value
      //  is already in $a0 from earlier direct-mem address range check.
      ADDIU(MIPSREG_A0, r1, imm);
    }

#ifdef HAVE_MIPS32R2_EXT_INS
    JAL(psxMemRead32);          // result in MIPSREG_V0
    INS(MIPSREG_A0, 0, 0, 2);   // <BD> clear 2 lower bits of $a0 (using branch delay slot)
#else
    SRL(MIPSREG_A0, MIPSREG_A0, 2);
    JAL(psxMemRead32);              // result in MIPSREG_V0
    SLL(MIPSREG_A0, MIPSREG_A0, 2); // <BD> clear lower 2 bits of $a0
#endif

    ADDIU(MIPSREG_A0, r1, imm);

    if (insn == 0x88000000) // LWL
      LUI(TEMP_2, ADR_HI(LWL_MASKSHIFT));
    else                    // LWR
      LUI(TEMP_2, ADR_HI(LWR_MASKSHIFT));

    // Lower 2 bits of dst addr are index into u32 mask/shift arrays:
#ifdef HAVE_MIPS32R2_EXT_INS
    INS(TEMP_2, MIPSREG_A0, 2, 2);
#else
    ANDI(TEMP_1, MIPSREG_A0, 3);    // temp_1 = addr & 3
    SLL(TEMP_1, TEMP_1, 2);         // temp_1 *= 4
    OR(TEMP_2, TEMP_2, TEMP_1);
#endif

    ADDIU(TEMP_3, TEMP_2, 16);      // array entry of shift amount is
    // 16 bytes past mask entry
    if (insn == 0x88000000)   // LWL
    {
      LW(TEMP_2, TEMP_2, ADR_LO(LWL_MASKSHIFT)); // temp_2 = mask
      LW(TEMP_3, TEMP_3, ADR_LO(LWL_MASKSHIFT)); // temp_3 = shift
    }
    else                      // LWR
    {
      LW(TEMP_2, TEMP_2, ADR_LO(LWR_MASKSHIFT)); // temp_2 = mask
      LW(TEMP_3, TEMP_3, ADR_LO(LWR_MASKSHIFT)); // temp_3 = shift
    }

    AND(r2, r2, TEMP_2);            // mask pre-existing contents of r2

    if (insn == 0x88000000) // LWL
      SLLV(TEMP_3, MIPSREG_V0, TEMP_3);   // temp_3 = data_read << shift
    else                    // LWR
      SRLV(TEMP_3, MIPSREG_V0, TEMP_3);   // temp_3 = data_read >> shift

    OR(r2, r2, TEMP_3);

    SetUndef(rt);
    regMipsChanged(rt);
    regUnlock(r2);

    PC += 4;
  }
  while (--icount);

#ifdef USE_DIRECT_MEM_ACCESS
  // label_exit:
  if (!force_indirect)
    fixup_branch(backpatch_label_exit_1);
#endif

  pc = PC;
  regUnlock(r1);
}

static u32 SWL_MASKSHIFT[8] = { 0xffffff00, 0xffff0000, 0xff000000, 0,
                                24, 16, 8, 0
                              };
static u32 SWR_MASKSHIFT[8] = { 0, 0xff, 0xffff, 0xffffff,
                                0, 8, 16, 24
                              };

static void gen_SWL_SWR(int count, bool force_indirect)
{
  int icount;
  u32 r1 = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
  u32 PC = pc - 4;

#ifdef WITH_DISASM
  for (int i = 0; i < count-1; i++)
    DISASM_PSX(pc + i * 4);
#endif

  // Get the effective address of first store in the series.
  // ---> NOTE: leave value in MIPSREG_A0, it will be used later!
  ADDIU(MIPSREG_A0, r1, _Imm_);

#ifdef USE_DIRECT_MEM_ACCESS
  u32 *backpatch_label_exit_1 = 0;
  if (!force_indirect)
  {
    regPushState();

    // Check if memory is writable and also check if address is in lower 8MB:
    // See comments in StoreToAddr() for explanation of check.
    // ---- Equivalent C code: ----
    // if ( !((r1+imm_of_first_store) & 0x0f00_0000) < writeok) )
    //    goto label_hle_1;

    u32 *backpatch_label_hle_1;
    if (!Config.HLE)
      LW(MIPSREG_A1, PERM_REG_1, off(writeok));

#ifdef HAVE_MIPS32R2_EXT_INS
    EXT(TEMP_3, MIPSREG_A0, 24, 4);
#else
    LUI(TEMP_3, 0x0f00);
    AND(TEMP_3, TEMP_3, MIPSREG_A0);
#endif
    if (!Config.HLE)
    {
      SLTU(MIPSREG_A1, TEMP_3, MIPSREG_A1);
      backpatch_label_hle_1 = (u32 *)recMem;
      BEQZ(MIPSREG_A1, 0);
    }
    else
    {
      backpatch_label_hle_1 = (u32 *)recMem;
      BNE(TEMP_3, 0, 0);
    }
    // NOTE: Branch delay slot contains next emitted instruction,
    //       which should not write to MIPSREG_A1

    // NOTE: emitAddrCalc() promises to only write to TEMP_1, TEMP_2
    emitAddrCalc(r1); // TEMP_2 == recalculated addr

    LUI(TEMP_3, ADR_HI(recRAM)); // temp_3 = upper code block ptr array addr
    icount = count;
    do
    {
      u32 opcode = *(u32 *)((char *)PSXM(PC));
      s32 imm = _fImm_(opcode);
      u32 rt = _fRt_(opcode);
      u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

      /* Invalidate recRAM[addr+imm16] pointer */
      if (icount != count)
      {
        // No need to do this for the first store of the series,
        //  as it was already done for us during initial checks.
        ADDIU(MIPSREG_A0, r1, imm);
      }

#ifdef HAVE_MIPS32R2_EXT_INS
      EXT(TEMP_1, MIPSREG_A0, 0, 0x15); // and 0x1fffff
#else
      SLL(TEMP_1, MIPSREG_A0, 11);
      SRL(TEMP_1, TEMP_1, 11);
#endif

#ifdef HAVE_MIPS32R2_EXT_INS
      INS(TEMP_1, 0, 0, 2); // clear 2 lower bits
#else
      SRL(TEMP_1, TEMP_1, 2);
      SLL(TEMP_1, TEMP_1, 2);
#endif
      ADDU(TEMP_1, TEMP_1, TEMP_3);

      OPCODE(opcode & 0xfc000000, r2, TEMP_2, imm);

      if (icount == 1)
      {
        // This is the end of the loop
        backpatch_label_exit_1 = (u32 *)recMem;
        B(0); // b label_exit
        // NOTE: Branch delay slot will contain the instruction below
      }
      // Important: this should be the last instruction in the loop (is BD slot of exit branch)
      SW(0, TEMP_1, ADR_LO(recRAM));  // set code block ptr to NULL

      PC += 4;

      regUnlock(r2);
    }
    while (--icount);

    PC = pc - 4;

    regPopState();

    // label_hle:
    fixup_branch(backpatch_label_hle_1);
  }
#endif // USE_DIRECT_MEM_ACCESS

  icount = count;
  do
  {
    u32 opcode = *(u32 *)((char *)PSXM(PC));
    u32 insn = opcode & 0xfc000000;
    u32 rt = _fRt_(opcode);
    s32 imm = _fImm_(opcode);
    u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

    if (icount != count)
    {
      // No need to do this for the first store of the series, as value
      //  is already in $a0 from earlier direct-mem address range check.
      ADDIU(MIPSREG_A0, r1, imm);
    }

#ifdef HAVE_MIPS32R2_EXT_INS
    JAL(psxMemRead32);              // result in MIPSREG_V0
    INS(MIPSREG_A0, 0, 0, 2);       // <BD> clear 2 lower bits of $a0
#else
    SRL(MIPSREG_A0, MIPSREG_A0, 2);
    JAL(psxMemRead32);              // result in MIPSREG_V0
    SLL(MIPSREG_A0, MIPSREG_A0, 2); // <BD> clear lower 2 bits of $a0
#endif

    ADDIU(MIPSREG_A0, r1, imm);

    if (insn == 0xa8000000)   // SWL
      LUI(TEMP_2, ADR_HI(SWL_MASKSHIFT));
    else                      // SWR
      LUI(TEMP_2, ADR_HI(SWR_MASKSHIFT));

    // Lower 2 bits of dst addr are index into u32 mask/shift arrays:
#ifdef HAVE_MIPS32R2_EXT_INS
    INS(TEMP_2, MIPSREG_A0, 2, 2);
    INS(MIPSREG_A0, 0, 0, 2);       // clear 2 lower bits of addr
#else
    ANDI(TEMP_1, MIPSREG_A0, 3);    // temp_1 = addr & 3
    SLL(TEMP_1, TEMP_1, 2);         // temp_1 *= 4
    OR(TEMP_2, TEMP_2, TEMP_1);

    SRL(MIPSREG_A0, MIPSREG_A0, 2); // clear 2 lower bits of addr
    SLL(MIPSREG_A0, MIPSREG_A0, 2);
#endif

    ADDIU(TEMP_3, TEMP_2, 16);      // array entry of shift amount is
    // 16 bytes past mask entry

    if (insn == 0xa8000000)   // SWL
    {
      LW(TEMP_2, TEMP_2, ADR_LO(SWL_MASKSHIFT)); // temp_2 = mask
      LW(TEMP_3, TEMP_3, ADR_LO(SWL_MASKSHIFT)); // temp_3 = shift
    }
    else                      // SWR
    {
      LW(TEMP_2, TEMP_2, ADR_LO(SWR_MASKSHIFT)); // temp_2 = mask
      LW(TEMP_3, TEMP_3, ADR_LO(SWR_MASKSHIFT)); // temp_3 = shift
    }

    AND(MIPSREG_A1, MIPSREG_V0, TEMP_2); // $a1 = mem_val & mask

    if (insn == 0xa8000000) // SWL
      SRLV(TEMP_1, r2, TEMP_3);        // temp_1 = new_data >> shift
    else                    // SWR
      SLLV(TEMP_1, r2, TEMP_3);        // temp_1 = new_data << shift

    JAL(psxMemWrite32);
    OR(MIPSREG_A1, MIPSREG_A1, TEMP_1);  // <BD> $a1 |= temp_1

    PC += 4;

    regUnlock(r2);
  }
  while (--icount);

#ifdef USE_DIRECT_MEM_ACCESS
  // label_exit:
  if (!force_indirect)
    fixup_branch(backpatch_label_exit_1);
#endif

  pc = PC;
  regUnlock(r1);
}

/* Calculate number of lwl/lwr or swl/swr opcodes */
enum
{
  CALC_SWL_SWR,
  CALC_LWL_LWR
};

static int calc_wl_wr(int type)
{
  u32 op1, op2;
  if (type == CALC_LWL_LWR)
  {
    op1 = 0x22; // LWL
    op2 = 0x26; // LWR
  }
  else
  {
    op1 = 0x2a; // SWL
    op2 = 0x2e; // SWR
  }

  int count = 0;
  u32 PC = pc;
  u32 opcode = psxRegs.code;
  u32 rs = _Rs_; // base reg

  imm_min = imm_max = _fImm_(opcode);

  /* If in delay slot, set count to 1 */
  if (branch)
    return 1;

  while ((_fOp_(opcode) == op1 || _fOp_(opcode) == op2) && (_fRs_(opcode) == rs))
  {

    /* Update min and max immediate values */
    if (_fImm_(opcode) > imm_max) imm_max = _fImm_(opcode);
    if (_fImm_(opcode) < imm_min) imm_min = _fImm_(opcode);

    opcode = *(u32 *)((char *)PSXM(PC));
    PC += 4;
    count++;

    /* Check if base reg overwritten by load, i.e. rt == rs */
    if (type == CALC_LWL_LWR && _fRt_(opcode) == _fRs_(opcode))
      break;
  }

#ifdef LOG_WL_WR
  if (count)
  {
    printf("\nFOUND %d opcodes, min: %d, max: %d\n", count, imm_min, imm_max);
    u32 dpc = pc - 4;
    for (; dpc < PC - 4; dpc += 4)
      disasm_psx(dpc);
  }
#endif

  return count;
}

static void recLWL()
{
  int count = calc_wl_wr(CALC_LWL_LWR);

  bool const_addr = false;
#ifdef USE_CONST_ADDRESSES
  const_addr = IsConst(_Rs_);
#endif
  if (!const_addr)
  {
    // Call general-case emitter for non-const addr
    gen_LWL_LWR(count, false);
    return;
  }

  // Is address in lower 8MB region? (2MB mirrored x4)
  u32 addr_max = iRegs[_Rs_].r + imm_max;
  if ((addr_max & 0x1fffffff) >= 0x800000)
  {
    // Call general-case emitter, but force indirect access since
    //  known-const address is outside lower 8MB RAM.
    gen_LWL_LWR(count, true);
    return;
  }

  //////////////////////////////
  // Handle const RAM address //
  //////////////////////////////

#ifdef WITH_DISASM
  for (int i = 0; i < count-1; i++)
    DISASM_PSX(pc + i * 4);
#endif

  u32 PC = pc - 4;

  // Keep upper mem address in a register, but track its current value
  // so we avoid unnecessarily loading same value repeatedly
  u16 mem_addr_hi = 0;

  int icount = count;
  do
  {
    // Paranoia check: was base reg written to by last iteration?
    if (!IsConst(_Rs_))
      break;

    u32 opcode = *(u32 *)((char *)PSXM(PC));
    s32 imm = _fImm_(opcode);
    u32 rt = _fRt_(opcode);
    u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

    u32 mem_addr = (u32)psxM + ((iRegs[_Rs_].r + imm) & 0x1fffff);

    if ((icount == count) || (ADR_HI(mem_addr) != mem_addr_hi))
    {
      mem_addr_hi = ADR_HI(mem_addr);
      LUI(TEMP_2, ADR_HI(mem_addr));
    }

    OPCODE(opcode & 0xfc000000, r2, TEMP_2, ADR_LO(mem_addr));

    SetUndef(rt);
    regMipsChanged(rt);
    regUnlock(r2);
    PC += 4;
  }
  while (--icount);

  pc = PC;
}

static void recLWR()
{
  recLWL();
}

static void recSWL()
{
  int count = calc_wl_wr(CALC_SWL_SWR);

  bool const_addr = false;
#ifdef USE_CONST_ADDRESSES
  const_addr = IsConst(_Rs_);
#endif
  if (!const_addr)
  {
    // Call general-case emitter for non-const addr
    gen_SWL_SWR(count, false);
    return;
  }

  // Is address in lower 8MB region? (2MB mirrored x4)
  u32 addr_max = iRegs[_Rs_].r + imm_max;
  if ((addr_max & 0x1fffffff) >= 0x800000)
  {
    // Call general-case emitter, but force indirect access since
    //  known-const address is outside lower 8MB RAM.
    gen_SWL_SWR(count, true);
    return;
  }

  //////////////////////////////
  // Handle const RAM address //
  //////////////////////////////

#ifdef WITH_DISASM
  for (int i = 0; i < count-1; i++)
    DISASM_PSX(pc + i * 4);
#endif

  u32 PC = pc - 4;

  u32 *backpatch_label_no_write = NULL;
  if (!Config.HLE)
    LW(TEMP_1, PERM_REG_1, off(writeok));

  // Keep upper half of last code block and RAM addresses in regs,
  // tracking current values so we can avoid loading same val repeatedly.
  u16 mem_addr_hi = 0;
  u16 code_addr_hi = 0;

  u32 last_code_addr = 0;

  int icount = count;
  do
  {
    u32 opcode = *(u32 *)((char *)PSXM(PC));
    s32 imm = _fImm_(opcode);
    u32 rt = _fRt_(opcode);
    u32 r2 = regMipsToHost(rt, REG_LOAD, REG_REGISTER);

    u32 mem_addr = (u32)psxM + ((iRegs[_Rs_].r + imm) & 0x1fffff);
    u32 code_addr = (u32)recRAM + ((iRegs[_Rs_].r + imm) & 0x1fffff);
    code_addr &= ~3;  // Align code block ptr address

    if ((icount == count) || (ADR_HI(code_addr) != code_addr_hi))
    {
      code_addr_hi = ADR_HI(code_addr);
      LUI(TEMP_3, ADR_HI(code_addr));
    }

    if ((icount == count) || (ADR_HI(mem_addr) != mem_addr_hi))
    {
      mem_addr_hi = ADR_HI(mem_addr);
      LUI(TEMP_2, ADR_HI(mem_addr));
    }

    // Skip RAM write if psxRegs.writeok == 0
    if (!Config.HLE)
    {
      backpatch_label_no_write = (u32 *)recMem;
      BEQZ(TEMP_1, 0);  // if (!psxRegs.writeok) goto label_no_write
      // NOTE: Branch delay slot contains next instruction emitted below
    }

    if (code_addr != last_code_addr)
    {
      // Set code block ptr to NULL
      last_code_addr = code_addr;
      SW(0, TEMP_3, ADR_LO(code_addr));  // <BD slot>
    }
    else
    {
      // Last code address is same. Rather than spam store buffer
      //  with duplicate write, put NOP() in branch delay slot.
      NOP();  // <BD slot>
    }

    // Write to RAM
    OPCODE(opcode & 0xfc000000, r2, TEMP_2, ADR_LO(mem_addr));

    // label_no_write:
    if (!Config.HLE)
      fixup_branch(backpatch_label_no_write);

    regUnlock(r2);
    PC += 4;
  }
  while (--count);

  pc = PC;
}

static void recSWR()
{
  recSWL();
}
