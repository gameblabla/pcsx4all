#define CP2_FUNC(f) \
  extern void gte##f(); \
  void rec##f() \
  { \
    LI32(TEMP_1, psxRegs.code); \
    JAL(gte##f); \
    SW(TEMP_1, PERM_REG_1, off(code)); /* <BD> Branch delay slot of JAL() */ \
  } \

  CP2_FUNC(DCPL);
  CP2_FUNC(RTPS);
  CP2_FUNC(OP);
  CP2_FUNC(NCLIP);
  CP2_FUNC(DPCS);
  CP2_FUNC(INTPL);
  CP2_FUNC(MVMVA);
  CP2_FUNC(NCDS);
  CP2_FUNC(NCDT);
  CP2_FUNC(CDP);
  CP2_FUNC(NCCS);
  CP2_FUNC(CC);
  CP2_FUNC(NCS);
  CP2_FUNC(NCT);
  CP2_FUNC(SQR);
  CP2_FUNC(DPCT);
  CP2_FUNC(AVSZ3);
  CP2_FUNC(AVSZ4);
  CP2_FUNC(RTPT);
  CP2_FUNC(GPF);
  CP2_FUNC(GPL);
  CP2_FUNC(NCCT);

  static void recCFC2()
  {
    if (!_Rt_) return;

    SetUndef(_Rt_);
    u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

    LW(rt, PERM_REG_1, offCP2C(_Rd_));
    regMipsChanged(_Rt_);
    regUnlock(rt);
  }

  static void emitCTC2(u32 rt, u32 reg)
  {
    switch (reg)
    {
      case 4:
      case 12:
      case 20:
      case 26:
      case 27:
      case 29:
      case 30:
#ifdef HAVE_MIPS32R2_SEB_SEH
        SEH(TEMP_1, rt);
#else
        SLL(TEMP_1, rt, 16);
        SRA(TEMP_1, TEMP_1, 16);
#endif
        SW(TEMP_1, PERM_REG_1, offCP2C(reg));
        break;

      case 31:
        //value = value & 0x7ffff000;
        //if (value & 0x7f87e000) value |= 0x80000000;
        LI32(TEMP_1, 0x7ffff000);
        AND(TEMP_1, rt, TEMP_1);	// $t0 = rt & $t0
        LI32(TEMP_2, 0x7f87e000);
        AND(TEMP_2, TEMP_1, TEMP_2);	// $t1 = $t0 & $t2
        LUI(TEMP_3, 0x8000);		// $t2 = 0x80000000
        OR(TEMP_3, TEMP_1, TEMP_3);	// $t2 = $t0 | $t2
        MOVN(TEMP_1, TEMP_3, TEMP_2);   // if ($t1) $t0 = $t2

        SW(TEMP_1, PERM_REG_1, offCP2C(reg));
        break;

      default:
        SW(rt, PERM_REG_1, offCP2C(reg));
        break;
    }
  }

  static void recCTC2()
  {
    u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
    emitCTC2(rt, _Rd_);
    regUnlock(rt);
  }

  /* Limit rt to [0 .. 0x1f] */
  static void emitLIM(u32 rt)
  {
    u32 *patch;

    SLT(1, rt, 0);			// $at = (rt < 0 ? 1 : 0)
    patch = (u32 *)recMem;
    BNE(1, 0, 0);			// if ($at != 0) goto end
    MOVN(rt, 0, 1);			// <BD> if ($at != 0) rt = 0

    LI16(MIPSREG_V0, 0x1f);		// $v0 = 0x1f
    SLTI(1, rt, 0x20);		// $at = (rt < 0x20 ? 1 : 0)
    MOVZ(rt, MIPSREG_V0, 1);	// if ($at == 0) rt = $v0

    // end:
    fixup_branch(patch);
  }

  /* move from cp2 reg to host rt */
  static void emitMFC2(u32 rt, u32 reg)
  {
    switch (reg)
    {
      case 1:
      case 3:
      case 5:
      case 8:
      case 9:
      case 10:
      case 11:
        LH(rt, PERM_REG_1, off(CP2D.r[reg]));
        SW(rt, PERM_REG_1, off(CP2D.r[reg]));
        break;

      case 7:
      case 16:
      case 17:
      case 18:
      case 19:
        LHU(rt, PERM_REG_1, off(CP2D.r[reg]));
        SW(rt, PERM_REG_1, off(CP2D.r[reg]));
        break;

      case 15:
        LW(rt, PERM_REG_1, off(CP2D.r[14])); // gteSXY2
        SW(rt, PERM_REG_1, off(CP2D.r[reg]));
        break;

      //senquack - Applied fix, see comment in gte.cpp gtecalcMFC2()
      case 28:
      case 29:
        LH(rt, PERM_REG_1, off(CP2D.p[9].sw.l)); // gteIR1
        SRA(rt, rt, 7);
        emitLIM(rt);
        LH(TEMP_1, PERM_REG_1, off(CP2D.p[10].sw.l)); // gteIR2
        SRA(TEMP_1, TEMP_1, 7);
        emitLIM(TEMP_1);
#ifdef HAVE_MIPS32R2_EXT_INS
        INS(rt, TEMP_1, 5, 5);
#else
        SLL(TEMP_1, TEMP_1, 5);
        OR(rt, rt, TEMP_1);
#endif
        LH(TEMP_1, PERM_REG_1, off(CP2D.p[11].sw.l)); // gteIR3
        SRA(TEMP_1, TEMP_1, 7);
        emitLIM(TEMP_1);
#ifdef HAVE_MIPS32R2_EXT_INS
        INS(rt, TEMP_1, 10, 5);
#else
        SLL(TEMP_1, TEMP_1, 10);
        OR(rt, rt, TEMP_1);
#endif
        SW(rt, PERM_REG_1, off(CP2D.r[29]));
        /*
        psxRegs.CP2D.r[reg] = LIM(gteIR1 >> 7, 0x1f, 0, 0) |
        		(LIM(gteIR2 >> 7, 0x1f, 0, 0) << 5) |
        		(LIM(gteIR3 >> 7, 0x1f, 0, 0) << 10);
        */
        break;
      default:
        LW(rt, PERM_REG_1, off(CP2D.r[reg]));
        break;
    }
  }

  /* move from host rt to cp2 reg */
  static void emitMTC2(u32 rt, u32 reg)
  {
    switch (reg)
    {
      case 15:
        LW(TEMP_1, PERM_REG_1, off(CP2D.p[13]));
        SW(TEMP_1, PERM_REG_1, off(CP2D.p[12])); // gteSXY0 = gteSXY1;
        LW(TEMP_1, PERM_REG_1, off(CP2D.p[14]));
        SW(TEMP_1, PERM_REG_1, off(CP2D.p[13])); // gteSXY1 = gteSXY2;

        SW(rt, PERM_REG_1, off(CP2D.p[14])); // gteSXY2 = value;
        SW(rt, PERM_REG_1, off(CP2D.p[15])); // gteSXYP = value;
        break;

      case 28:
        SW(rt, PERM_REG_1, off(CP2D.r[reg]));
#ifdef HAVE_MIPS32R2_EXT_INS
        EXT(TEMP_1, rt, 0, 5);
#else
        ANDI(TEMP_1, rt, 0x1f);
#endif
        SLL(TEMP_1, TEMP_1, 7);
        // gteIR1 = ((value      ) & 0x1f) << 7;
        SW(TEMP_1, PERM_REG_1, off(CP2D.r[9]));
#ifdef HAVE_MIPS32R2_EXT_INS
        EXT(TEMP_1, rt, 5, 5);
        SLL(TEMP_1, TEMP_1, 7);
#else
        ANDI(TEMP_1, rt, 0x1f << 5);
        SLL(TEMP_1, TEMP_1, 2);
#endif
        // gteIR2 = ((value >>  5) & 0x1f) << 7;
        SW(TEMP_1, PERM_REG_1, off(CP2D.r[10]));
#ifdef HAVE_MIPS32R2_EXT_INS
        EXT(TEMP_1, rt, 10, 5);
        SLL(TEMP_1, TEMP_1, 7);
#else
        ANDI(TEMP_1, rt, 0x1f << 10);
        SRL(TEMP_1, TEMP_1, 3);
#endif
        // gteIR3 = ((value >> 10) & 0x1f) << 7;
        SW(TEMP_1, PERM_REG_1, off(CP2D.r[11]));
        break;

      case 30:
        u32 *backpatch;
        SW(rt, PERM_REG_1, off(CP2D.r[30]));
        SLT(1, rt, 0);
        backpatch = (u32 *)recMem;
        BEQZ(1, 0);
        MOV(TEMP_1, rt); // delay slot

        NOR(TEMP_1, 0, rt); // temp_1 = rt ^ -1
        fixup_branch(backpatch);
        CLZ(TEMP_1, TEMP_1);
        SW(TEMP_1, PERM_REG_1, off(CP2D.r[31]));
        break;

      //senquack - Applied fix, see comment in gte.cpp gtecalcMTC2()
      case 31:
        return;

      default:
        SW(rt, PERM_REG_1, off(CP2D.r[reg]));
        break;
    }
  }

  static void recMFC2()
  {
    if (!_Rt_) return;

    SetUndef(_Rt_);
    u32 rt = regMipsToHost(_Rt_, REG_FIND, REG_REGISTER);

    emitMFC2(rt, _Rd_);

    regMipsChanged(_Rt_);
    regUnlock(rt);
  }

  static void recMTC2()
  {
    u32 rt = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);

    emitMTC2(rt, _Rd_);

    regUnlock(rt);
  }

  static void recLWC2()
  {
    u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

    JAL(psxMemRead32);
    ADDIU(MIPSREG_A0, rs, _Imm_); /* <BD> Branch delay slot of JAL() */

    emitMTC2(MIPSREG_V0, _Rt_);

    regUnlock(rs);
  }

  static void recSWC2()
  {
    u32 rs = regMipsToHost(_Rs_, REG_LOAD, REG_REGISTER);

    emitMFC2(MIPSREG_A1, _Rt_);

    JAL(psxMemWrite32);
    ADDIU(MIPSREG_A0, rs, _Imm_); /* <BD> Branch delay slot of JAL() */

    regUnlock(rs);
  }
