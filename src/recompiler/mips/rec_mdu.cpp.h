static void recMULT()
{
// Lo/Hi = Rs * Rt (signed)
  if (!(_Rs_) || !(_Rt_))
  {
    // If either operand is $r0, just store 0 in both LO/HI regs
    SW(0, PERM_REG_1, offGPR(32)); // LO
    SW(0, PERM_REG_1, offGPR(33)); // HI
  }
  else
  {
    u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
    u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

    MULT(rs, rt);
    MFLO(TEMP_1);
    MFHI(TEMP_2);

    SW(TEMP_1, PERM_REG_1, offGPR(32));
    SW(TEMP_2, PERM_REG_1, offGPR(33));
    regUnlock(rs);
    regUnlock(rt);
  }
}

static void recMULTU()
{
// Lo/Hi = Rs * Rt (unsigned)
  if (!(_Rs_) || !(_Rt_))
  {
    // If either operand is $r0, just store 0 in both LO/HI regs
    SW(0, PERM_REG_1, offGPR(32)); // LO
    SW(0, PERM_REG_1, offGPR(33)); // HI
  }
  else
  {
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
}

static void recDIV()
{
// Hi, Lo = rs / rt signed
  u32 *backpatch1, *backpatch2;
  u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
  u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

  backpatch1 = (u32*)recMem;
  BEQZ(rt, 0);
  NOP();

  DIV(rs, rt);
  MFLO(TEMP_1); /* NOTE: Hi/Lo can't be cached for now, so spill them */
  SW(TEMP_1, PERM_REG_1, offGPR(32));
  MFHI(TEMP_1);
  SW(TEMP_1, PERM_REG_1, offGPR(33));

  backpatch2 = (u32*)recMem;
  B(0);
  NOP();

  // division by zero
  fixup_branch(backpatch1);

  SLT(TEMP_1, rs, 0); // $t0 = (rs < 0 ? 1 : 0)
  ADDIU(TEMP_2, 0, -1); // $t1 = -1
  MOVZ(TEMP_1, TEMP_2, TEMP_1); // if $t0 == 0 then $t0 = $t1
  SW(TEMP_1, PERM_REG_1, offGPR(32));
  SW(rs, PERM_REG_1, offGPR(33));

  // exit
  fixup_branch(backpatch2);

  regUnlock(rs);
  regUnlock(rt);
}

static void recDIVU()
{
// Hi, Lo = rs / rt unsigned
  u32 *backpatch1, *backpatch2;
  u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);
  u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

  backpatch1 = (u32*)recMem;
  BEQZ(rt, 0);
  NOP();

  DIVU(rs, rt);
  MFLO(TEMP_1); /* NOTE: Hi/Lo can't be cached for now, so spill them */
  SW(TEMP_1, PERM_REG_1, offGPR(32));
  MFHI(TEMP_1);
  SW(TEMP_1, PERM_REG_1, offGPR(33));

  backpatch2 = (u32*)recMem;
  B(0);
  NOP();

  // division by zero
  fixup_branch(backpatch1);

  ADDIU(TEMP_1, 0, -1); // $t1 = -1
  SW(TEMP_1, PERM_REG_1, offGPR(32));
  SW(rs, PERM_REG_1, offGPR(33));

  // exit
  fixup_branch(backpatch2);

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
