/*
 * Copyright (c) 2018 Dmitry Smagin / Daniel Silsby
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/* Implements inlined direct HW I/O. Any changes to psxhw.cpp in main emu
 *  should be followed by updates here.
 *  NOTE: If any additional HW I/O functions are called here, please add
 *        them to disasm_label stub_labels[] array.
 * Last updated: Aug 4 2017
 */

/******************************************************************************
 * IMPORTANT: The following host registers have unique usage restrictions.    *
 *            See notes in mips_codegen.h for full details.                   *
 *  MIPSREG_AT, MIPSREG_V0, MIPSREG_V1, MIPSREG_RA                            *
 *****************************************************************************/

/* Will emit code for any indirect stores (calls to C). Bool at ptr param
 *  'C_func_called' will be set to 1 if a call to C is made, 0 if not.
 * Returns: 1 if caller should do a direct store to psxH[]
 */
static uint8_t emit_const_hw_store(u32 addr, u32 r2, u32 opcode, uint8_t *C_func_called)
{
	u32 rs = _fRs_(opcode);
	uint8_t direct = 0;
	uint8_t indirect = 0;
	u32 upper = addr >> 16;
	u32 lower = addr & 0xffff;

	int width;
	switch (opcode & 0xfc000000) {
		case 0xa8000000:  // SWL
		case 0xb8000000:  // SWR
			width = 0;
			break;
		case 0xa0000000:  // SB
			width = 8;
			break;
		case 0xa4000000:  // SH
			width = 16;
			break;
		case 0xac000000:  // SW
			width = 32;
			break;
		default:
			printf("ERROR: unrecognized opcode in %s: %x\n", __func__, opcode);
			exit(1);
			break;
	}

	if (lower < 0x400) {
		// 1KB scratchpad data cache area is always direct access
		direct = 1;
	} else if (upper == 0x1f80)
	{
		if (width == 0) {
			// Somehow, we got a SWL/SWR to a HW I/O port: always use indirect
			indirect = 1;
		}
		else if (width == 8)
		{
			// NOTE: Code here follows psxHwWrite8(): The Joy/CDROM ports
			//       write to psxH[] after calling the handler funcs.
			//       It seemed likely that psxHwWrite8() was just carelessly
			//       allowing that by using break instead of return in switch{}.
			//       (psxhw.cpp switch block has no comments clarifying this)
			switch (lower)
			{
				case 0x1040:  // JOY_DATA
					JAL(sioWrite8);
					ANDI(MIPSREG_A0, r2, 0xff); // <BD>
					*C_func_called = 1;
					// NOTE: we also write to psxH[], as psxhw.cpp (perhaps unnecessarily?) does
					direct = 1;
					break;

				case 0x1800:  // CD Index/Status Register (Bit0-1 R/W, Bit2-7 Read Only)
					JAL(cdrWrite0);
					ANDI(MIPSREG_A0, r2, 0xff); // <BD>
					*C_func_called = 1;
					// NOTE: we also write to psxH[], as psxhw.cpp (perhaps unnecessarily?) does
					direct = 1;
					break;

				case 0x1801:  // CD Command Register (W)
					JAL(cdrWrite1);
					ANDI(MIPSREG_A0, r2, 0xff); // <BD>
					*C_func_called = 1;
					// NOTE: we also write to psxH[], as psxhw.cpp (perhaps unnecessarily?) does
					direct = 1;
					break;

				case 0x1802:  // CD Parameter Fifo (W)
					JAL(cdrWrite2);
					ANDI(MIPSREG_A0, r2, 0xff); // <BD>
					*C_func_called = 1;
					// NOTE: we also write to psxH[], as psxhw.cpp (perhaps unnecessarily?) does
					direct = 1;
					break;

				case 0x1803:  // CD Request Register (W)
					JAL(cdrWrite3);
					ANDI(MIPSREG_A0, r2, 0xff); // <BD>
					*C_func_called = 1;
					// NOTE: we also write to psxH[], as psxhw.cpp (perhaps unnecessarily?) does
					direct = 1;
					break;

				default:
					direct = 1;
					break;
			}
		}
		else if (width == 16)
		{
			switch (lower)
			{
				case 0x1040:  // JOY_DATA
					JAL(sioWrite16);
					ANDI(MIPSREG_A0, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1044:  // JOY_STAT
					// Function is empty, and disabled
					//JAL(sioWriteStat16);
					//ANDI(MIPSREG_A0, r2, 0xffff); // <BD>
					//*C_func_called = 1;
					break;

				case 0x1048:  // JOY_MODE
					JAL(sioWriteMode16);
					ANDI(MIPSREG_A0, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x104a:  // JOY_CTRL
					JAL(sioWriteCtrl16);
					ANDI(MIPSREG_A0, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x104e:  // JOY_BAUD
					JAL(sioWriteBaud16);
					ANDI(MIPSREG_A0, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1070:  // IREG
				case 0x1074:  // IMASK
					indirect = 1;
					break;

				case 0x1100:  // Timer 0 Current Counter Value (R/W)
					LI16(MIPSREG_A0, 0);
					JAL(psxRcntWcount);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1104:  // Timer 0 Counter Mode (R/W)
					LI16(MIPSREG_A0, 0);
					JAL(psxRcntWmode);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1108:  // Timer 0 Counter Target Value (R/W)
					LI16(MIPSREG_A0, 0);
					JAL(psxRcntWtarget);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1110:  // Timer 1 Current Counter Value (R/W)
					LI16(MIPSREG_A0, 1);
					JAL(psxRcntWcount);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1114:  // Timer 1 Counter Mode (R/W)
					LI16(MIPSREG_A0, 1);
					JAL(psxRcntWmode);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1118:  // Timer 1 Counter Target Value (R/W)
					LI16(MIPSREG_A0, 1);
					JAL(psxRcntWtarget);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1120:  // Timer 2 Current Counter Value (R/W)
					LI16(MIPSREG_A0, 2);
					JAL(psxRcntWcount);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1124:  // Timer 2 Counter Mode (R/W)
					LI16(MIPSREG_A0, 2);
					JAL(psxRcntWmode);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1128:  // Timer 2 Counter Target Value (R/W)
					LI16(MIPSREG_A0, 2);
					JAL(psxRcntWtarget);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1c00 ... 0x1dff: // SPU reg
				{
					// Call w/ params: SPU_writeRegister(addr, value, psxRegs.cycle)
					// NOTE: called func only cares about lower 16 bits of addr param
					LI16(MIPSREG_A0, lower);
					ANDI(MIPSREG_A1, r2, 0xffff);
					JAL(SPU_writeRegister);
					LW(MIPSREG_A2, PERM_REG_1, off(cycle));  // <BD>
					*C_func_called = 1;
					break;
				}

				default:
					direct = 1;
					break;
			}
		}
		else if (width == 32)
		{
			switch (lower)
			{
				case 0x1040:  // JOY_DATA
					JAL(sioWrite32);
					MOV(MIPSREG_A0, r2); // <BD>
					*C_func_called = 1;
					break;

				case 0x1070:  // IREG
				case 0x1074:  // IMASK
				case 0x1088:  // DMA0 CHCR (MDEC in)
				case 0x1098:  // DMA1 CHCR (MDEC out)
				case 0x10a8:  // DMA2 CHCR (GPU)
				case 0x10b8:  // DMA3 CHCR (CDROM)
				case 0x10c8:  // DMA4 CHCR (SPU)
					              // NOTE: DMA5 Parallel I/O not implemented in emu
				case 0x10e8:  // DMA6 CHCR (GPU OT CLEAR)
				case 0x10f4:  // DMA ICR
					indirect = 1;
					break;

				case 0x1810:  // GPU DATA (Send GP0 Commands/Packets (Rendering and VRAM Access))
					JAL(GPU_writeData);
					MOV(MIPSREG_A0, r2); // <BD>
					*C_func_called = 1;
					break;

				case 0x1814:  // GPU STATUS (Send GP1 Commands (Display Control))
					indirect = 1;
					break;

				case 0x1820:  // MDEC Command/Parameter Register (W)
					JAL(mdecWrite0);
					MOV(MIPSREG_A0, r2); // <BD>
					*C_func_called = 1;
					// NOTE: we also write to psxH[], as psxhw.cpp (perhaps unnecessarily?) does
					direct = 1;
					break;

				case 0x1824:  // MDEC Control/Reset Register (W)
					JAL(mdecWrite1);
					MOV(MIPSREG_A0, r2); // <BD>
					*C_func_called = 1;
					// NOTE: we also write to psxH[], as psxhw.cpp (perhaps unnecessarily?) does
					direct = 1;
					break;

				case 0x1100:  // Timer 0 Current Counter Value (R/W)
					LI16(MIPSREG_A0, 0);
					JAL(psxRcntWcount);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1104:  // Timer 0 Counter Mode (R/W)
					LI16(MIPSREG_A0, 0);
					JAL(psxRcntWmode);
					MOV(MIPSREG_A1, r2); // <BD>
					*C_func_called = 1;
					break;

				case 0x1108:  // Timer 0 Counter Target Value (R/W)
					LI16(MIPSREG_A0, 0);
					JAL(psxRcntWtarget);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1110:  // Timer 1 Current Counter Value (R/W)
					LI16(MIPSREG_A0, 1);
					JAL(psxRcntWcount);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1114:  // Timer 1 Counter Mode (R/W)
					LI16(MIPSREG_A0, 1);
					JAL(psxRcntWmode);
					MOV(MIPSREG_A1, r2); // <BD>
					*C_func_called = 1;
					break;

				case 0x1118:  // Timer 1 Counter Target Value (R/W)
					LI16(MIPSREG_A0, 1);
					JAL(psxRcntWtarget);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1120:  // Timer 2 Current Counter Value (R/W)
					LI16(MIPSREG_A0, 2);
					JAL(psxRcntWcount);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1124:  // Timer 2 Counter Mode (R/W)
					LI16(MIPSREG_A0, 2);
					JAL(psxRcntWmode);
					MOV(MIPSREG_A1, r2); // <BD>
					*C_func_called = 1;
					break;

				case 0x1128:  // Timer 2 Counter Target Value (R/W)
					LI16(MIPSREG_A0, 2);
					JAL(psxRcntWtarget);
					ANDI(MIPSREG_A1, r2, 0xffff); // <BD>
					*C_func_called = 1;
					break;

				case 0x1c00 ... 0x1dff: // SPU reg
				{
					// Call w/ params:
					//  SPU_writeRegister(addr, value&0xffff, psxRegs.cycle);
					//  SPU_writeRegister(addr + 2, value>>16, psxRegs.cycle);
					// NOTE: called func only cares about lower 16 bits of addr param
					LI16(MIPSREG_A0, lower);
					ANDI(MIPSREG_A1, r2, 0xffff);
					JAL(SPU_writeRegister);
					LW(MIPSREG_A2, PERM_REG_1, off(cycle));  // <BD>

					LI16(MIPSREG_A0, lower+2);
					SRL(MIPSREG_A1, r2, 16);
					JAL(SPU_writeRegister);
					LW(MIPSREG_A2, PERM_REG_1, off(cycle));  // <BD>

					*C_func_called = 1;
					break;
				}

				default:
					direct = 1;
					break;
			}
		}
	} else {
		// If upper is 0x9f80 or 0xbf80, mimic psxHwWrite__() behavior and
		//  do direct store.
		direct = 1;
	}

	if (indirect)
	{
		s16 imm = _fImm_(opcode);
		u32 r1 = regMipsToHost(rs, REG_LOAD, REG_REGISTER);
		switch (width) {
			case 0:
			{
				// SWL/SWR to HW I/O port
				//  Probably never encountered. We'll try to support it anyway..

				u32 insn = opcode & 0xfc000000;
				ADDIU(MIPSREG_A0, r1, imm);
#ifdef HAVE_MIPS32R2_EXT_INS
				JAL(psxHwRead32);               // result in MIPSREG_V0
				INS(MIPSREG_A0, 0, 0, 2);       // <BD> clear 2 lower bits of $a0
#else
				SRL(MIPSREG_A0, MIPSREG_A0, 2);
				JAL(psxHwRead32);               // result in MIPSREG_V0
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

				if (insn == 0xa8000000) { // SWL
					LW(TEMP_2, TEMP_2, ADR_LO(SWL_MASKSHIFT)); // temp_2 = mask
					LW(TEMP_3, TEMP_3, ADR_LO(SWL_MASKSHIFT)); // temp_3 = shift
				} else {                  // SWR
					LW(TEMP_2, TEMP_2, ADR_LO(SWR_MASKSHIFT)); // temp_2 = mask
					LW(TEMP_3, TEMP_3, ADR_LO(SWR_MASKSHIFT)); // temp_3 = shift
				}

				AND(MIPSREG_A1, MIPSREG_V0, TEMP_2); // $a1 = mem_val & mask

				if (insn == 0xa8000000) // SWL
					SRLV(TEMP_1, r2, TEMP_3);        // temp_1 = new_data >> shift
				else                    // SWR
					SLLV(TEMP_1, r2, TEMP_3);        // temp_1 = new_data << shift

				JAL(psxHwWrite32);
				OR(MIPSREG_A1, MIPSREG_A1, TEMP_1);  // <BD> $a1 |= temp_1

				break;
			}
			case 8:
				ADDIU(MIPSREG_A0, r1, imm);
				JAL(psxHwWrite8);
				ANDI(MIPSREG_A1, r2, 0xff);  // <BD>
				break;
			case 16:
				ADDIU(MIPSREG_A0, r1, imm);
				JAL(psxHwWrite16);
				ANDI(MIPSREG_A1, r2, 0xffff);  // <BD>
				break;
			case 32:
				ADDIU(MIPSREG_A0, r1, imm);
				JAL(psxHwWrite32);
				MOV(MIPSREG_A1, r2);  // <BD>
				break;
		}
		regUnlock(r1);
		*C_func_called = 1;
	}

	return direct;
}


/* Will emit code for any indirect loads (calls to C). Bool at ptr param
 *  'C_func_called' will be set to 1 if a call to C is made, 0 if not.
 * Returns: 1 if caller should do a direct store to psxH[]
 */
static uint8_t emit_const_hw_load(u32 addr, u32 r2, u32 opcode, uint8_t *C_func_called)
{
	u32 rt = _fRt_(opcode);
	uint8_t direct = 0;
	uint8_t indirect = 0;
	u32 upper = addr >> 16;
	u32 lower = addr & 0xffff;

	uint8_t sign_extend = 0;
	int width;
	switch (opcode & 0xfc000000) {
		case 0x88000000:  // LWL
		case 0x98000000:  // LWR
			width = 0;
			break;
		case 0x80000000:  // LB
			width = 8;
			sign_extend = 1;
			break;
		case 0x90000000:  // LBU
			width = 8;
			break;
		case 0x84000000:  // LH
			sign_extend = 1;
			width = 16;
			break;
		case 0x94000000:  // LHU
			width = 16;
			break;
		case 0x8c000000:  // LW
			width = 32;
			break;
		default:
			printf("ERROR: unrecognized opcode in %s: %x\n", __func__, opcode);
			exit(1);
			break;
	}

	if (lower < 0x400) {
		// 1KB scratchpad data cache area is always direct access
		direct = 1;
	} else if (upper == 0x1f80)
	{
		if (width == 0) {
			// Somehow, we got a LWL/LWR from a HW I/O port: always use indirect
			indirect = 1;
		}
		else if (width == 8)
		{
			uint8_t move_result_out_of_v0 = 0;
			switch (lower)
			{
				case 0x1040:  // JOY_DATA
					JAL(sioRead8);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1800:  // CD Index/Status Register (Bit0-1 R/W, Bit2-7 Read Only)
					JAL(cdrRead0);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1801:  // CD Command Register (W)
					JAL(cdrRead1);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1802:  // CD Parameter Fifo (W)
					JAL(cdrRead2);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1803:  // CD Request Register (W)
					JAL(cdrRead3);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				default:
					direct = 1;
					break;
			}

			if (move_result_out_of_v0 && rt) {
				if (sign_extend) {
#ifdef HAVE_MIPS32R2_SEB_SEH
					SEB(r2, MIPSREG_V0);
#else
					SLL(r2, MIPSREG_V0, 24);
					SRA(r2, r2, 24);
#endif
				} else {
					ANDI(r2, MIPSREG_V0, 0xff);
				}
			}
		}
		else if (width == 16)
		{
			uint8_t move_result_out_of_v0 = 0;
			switch (lower)
			{
				case 0x1040:  // JOY_DATA
					JAL(sioRead16);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1044:  // JOY_STAT
					JAL(sioReadStat16);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1048:  // JOY_MODE
					JAL(sioReadMode16);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x104a:  // JOY_CTRL
					JAL(sioReadCtrl16);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x104e:  // JOY_BAUD
					JAL(sioReadBaud16);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1100:  // Timer 0 Current Counter Value (R/W)
					JAL(psxRcntRcount);
					LI16(MIPSREG_A0, 0);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1104:  // Timer 0 Counter Mode (R/W)
					JAL(psxRcntRmode);
					LI16(MIPSREG_A0, 0);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1108:  // Timer 0 Counter Target Value (R/W)
					JAL(psxRcntRtarget);
					LI16(MIPSREG_A0, 0);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1110:  // Timer 1 Current Counter Value (R/W)
					JAL(psxRcntRcount);
					LI16(MIPSREG_A0, 1);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1114:  // Timer 1 Counter Mode (R/W)
					JAL(psxRcntRmode);
					LI16(MIPSREG_A0, 1);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1118:  // Timer 1 Counter Target Value (R/W)
					JAL(psxRcntRtarget);
					LI16(MIPSREG_A0, 1);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1120:  // Timer 2 Current Counter Value (R/W)
					JAL(psxRcntRcount);
					LI16(MIPSREG_A0, 2);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1124:  // Timer 2 Counter Mode (R/W)
					JAL(psxRcntRmode);
					LI16(MIPSREG_A0, 2);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1128:  // Timer 2 Counter Target Value (R/W)
					JAL(psxRcntRtarget);
					LI16(MIPSREG_A0, 2);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1c00 ... 0x1dff: // SPU reg
					// Call w/ params:
					//  SPU_readRegister(addr);
					// NOTE: called func only cares about lower 16 bits of addr param
					JAL(SPU_readRegister);
					LI16(MIPSREG_A0, lower);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				default:
					direct = 1;
					break;
			}

			if (move_result_out_of_v0 && rt) {
				if (sign_extend) {
#ifdef HAVE_MIPS32R2_SEB_SEH
					SEH(r2, MIPSREG_V0);
#else
					SLL(r2, MIPSREG_V0, 16);
					SRA(r2, r2, 16);
#endif
				} else {
					ANDI(r2, MIPSREG_V0, 0xffff);
				}
			}
		}
		else if (width == 32)
		{
			uint8_t move_result_out_of_v0 = 0;
			switch (lower)
			{
				case 0x1040:  // JOY_DATA
					JAL(sioRead32);
					NOP();  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1810:  // GPU DATA (Send GP0 Commands/Packets (Rendering and VRAM Access))
					JAL(GPU_readData);
					NOP();
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1814:  // GPU STATUS (Send GP1 Commands (Display Control))
					indirect = 1;
					break;

				case 0x1820:  // MDEC Data/Response Register (R)
					JAL(mdecRead0);
					NOP();
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1824:  // MDEC Status Register (R)
					JAL(mdecRead1);
					NOP();
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1100:  // Timer 0 Current Counter Value (R/W)
					JAL(psxRcntRcount);
					LI16(MIPSREG_A0, 0);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1104:  // Timer 0 Counter Mode (R/W)
					JAL(psxRcntRmode);
					LI16(MIPSREG_A0, 0);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1108:  // Timer 0 Counter Target Value (R/W)
					JAL(psxRcntRtarget);
					LI16(MIPSREG_A0, 0);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1110:  // Timer 1 Current Counter Value (R/W)
					JAL(psxRcntRcount);
					LI16(MIPSREG_A0, 1);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1114:  // Timer 1 Counter Mode (R/W)
					JAL(psxRcntRmode);
					LI16(MIPSREG_A0, 1);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1118:  // Timer 1 Counter Target Value (R/W)
					JAL(psxRcntRtarget);
					LI16(MIPSREG_A0, 1);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1120:  // Timer 2 Current Counter Value (R/W)
					JAL(psxRcntRcount);
					LI16(MIPSREG_A0, 2);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1124:  // Timer 2 Counter Mode (R/W)
					JAL(psxRcntRmode);
					LI16(MIPSREG_A0, 2);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				case 0x1128:  // Timer 2 Counter Target Value (R/W)
					JAL(psxRcntRtarget);
					LI16(MIPSREG_A0, 2);  // <BD>
					*C_func_called = 1;
					move_result_out_of_v0 = 1;
					break;

				default:
					direct = 1;
					break;
			}

			if (move_result_out_of_v0 && rt) {
				MOV(r2, MIPSREG_V0);
			}
		}
	} else {
		// If upper is 0x9f80 or 0xbf80, mimic psxHwRead__() behavior and
		//  do direct load.
		direct = 1;
	}

	if (indirect)
	{
		s16 imm = _fImm_(opcode);
		u32 rs = _fRs_(opcode);
		u32 r1 = regMipsToHost(rs, REG_LOAD, REG_REGISTER);
		switch (width) {
			case 0:
			{
				// LWL/LWR from HW I/O port
				//  Probably never encountered. We'll try to support it anyway..

				u32 insn = opcode & 0xfc000000;
				ADDIU(MIPSREG_A0, r1, imm);
#ifdef HAVE_MIPS32R2_EXT_INS
				JAL(psxHwRead32);           // result in MIPSREG_V0
				INS(MIPSREG_A0, 0, 0, 2);   // <BD> clear 2 lower bits of $a0 (using branch delay slot)
#else
				SRL(MIPSREG_A0, MIPSREG_A0, 2);
				JAL(psxHwRead32);               // result in MIPSREG_V0
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
				if (insn == 0x88000000) { // LWL
					LW(TEMP_2, TEMP_2, ADR_LO(LWL_MASKSHIFT)); // temp_2 = mask
					LW(TEMP_3, TEMP_3, ADR_LO(LWL_MASKSHIFT)); // temp_3 = shift
				} else {                  // LWR
					LW(TEMP_2, TEMP_2, ADR_LO(LWR_MASKSHIFT)); // temp_2 = mask
					LW(TEMP_3, TEMP_3, ADR_LO(LWR_MASKSHIFT)); // temp_3 = shift
				}

				AND(r2, r2, TEMP_2);            // mask pre-existing contents of r2

				if (insn == 0x88000000) // LWL
					SLLV(TEMP_3, MIPSREG_V0, TEMP_3);   // temp_3 = data_read << shift
				else                    // LWR
					SRLV(TEMP_3, MIPSREG_V0, TEMP_3);   // temp_3 = data_read >> shift

				OR(r2, r2, TEMP_3);

				break;
			}
			case 8:
				JAL(psxHwRead8);
				ADDIU(MIPSREG_A0, r1, imm);  // <BD>
				if (rt) {
					if (sign_extend) {
#ifdef HAVE_MIPS32R2_SEB_SEH
						SEB(r2, MIPSREG_V0);
#else
						SLL(r2, MIPSREG_V0, 24);
						SRA(r2, r2, 24);
#endif
					} else {
						ANDI(r2, MIPSREG_V0, 0xff);
					}
				}
				break;
			case 16:
				JAL(psxHwRead16);
				ADDIU(MIPSREG_A0, r1, imm);  // <BD>
				if (rt) {
					if (sign_extend) {
#ifdef HAVE_MIPS32R2_SEB_SEH
						SEH(r2, MIPSREG_V0);
#else
						SLL(r2, MIPSREG_V0, 16);
						SRA(r2, r2, 16);
#endif
					} else {
						ANDI(r2, MIPSREG_V0, 0xffff);
					}
				}
				break;
			case 32:
				JAL(psxHwRead32);
				ADDIU(MIPSREG_A0, r1, imm);  // <BD>
				if (rt) {
					MOV(r2, MIPSREG_V0);
				}
				break;
		}
		regUnlock(r1);
		*C_func_called = 1;
	}

	return direct;
}


/* Emits a series of loads/stores to a known-const scratcpad/HW I/O base
 * address, i.e. address is in [0xXf80_0000 .. 0xXf80_ffff] range.
 */
static void const_hw_loads_stores(const int count,
                                  u32 rs_constval)
{
	// Keep upper half of last effective address in reg, tracking current
	//  value so we can avoid loading same val repeatedly.
	u32  base_reg = 0;
	u32  base_reg_val = 0xffffffff;  // Initialize with impossible value
	uint8_t base_reg_lui_emitted = 0;

	u32 PC = pc - 4;
	int icount = count;
	do {
		const u32 opcode = OPCODE_AT(PC);
		PC += 4;

		// Skip any NOPs in the series
		if (opcode == 0)
			continue;

		const uint8_t is_store   = opcodeIsStore(opcode);
		const uint8_t is_load    = !is_store && opcodeIsLoad(opcode);
		const uint8_t is_lwl_lwr = is_load && opcodeIsLoadWordUnaligned(opcode);

		if (!is_store && !is_load) {
			// Must be a jump/branch whose BD slot is included as the last
			//  load/store in the series: skip it.
			continue;
		}

		const u32 op_rt = _fRt_(opcode);
		const u32 psx_eff_addr = rs_constval + _fImm_(opcode);

		uint8_t C_func_called = 0;
		uint8_t emit_direct;

		u32  rt;
		if (is_store) {
			rt = regMipsToHost(op_rt, REG_LOAD, REG_REGISTER);

			emit_direct = emit_const_hw_store(psx_eff_addr, rt, opcode, &C_func_called);
		} else {
			if (is_lwl_lwr) {
				// LWL/LWR, so we need existing contents of register
				rt = regMipsToHost(op_rt, REG_LOAD, REG_REGISTER);
			} else {
				rt = regMipsToHost(op_rt, REG_FIND, REG_REGISTER);
			}

			emit_direct = emit_const_hw_load(psx_eff_addr, rt, opcode, &C_func_called);
		}

		if (emit_direct)
		{
			const uptr host_addr = (uptr)psxH + (psx_eff_addr & 0xffff);

			// Emit LUI for base reg (if not cached in host reg).
			if (C_func_called ||
			    !base_reg_lui_emitted ||
			    base_reg_val != (ADR_HI(host_addr) << 16))
			{
				base_reg_lui_emitted = 1;
				base_reg_val = ADR_HI(host_addr) << 16;

				base_reg = emitConstBaseRegLUI(ADR_HI(host_addr));
			}

			LSU_OPCODE(opcode & 0xfc000000, rt, base_reg, ADR_LO(host_addr));
		}

		if (is_load) {
			SetUndef(op_rt);
			regMipsChanged(op_rt);
		}
		regUnlock(rt);
	} while (--icount);
}
