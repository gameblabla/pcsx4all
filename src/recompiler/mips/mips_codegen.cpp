/*
 * mips_codegen.cpp
 *
 * Copyright (c) 2017 Dmitry Smagin / Daniel Silsby
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

#include "psxcommon.h"
#include "r3000a.h"

#include "mem_mapping.h"
#include "mips_codegen.h"

/* Emit code that will convert the base reg of a load/store in PS1 code
 *  into a base reg that can be used to access psxM[]. The register
 *  specified by 'tmp_reg' can be overwritten in the process.
 */
void emitAddressConversion(u32 dst_reg, u32 src_reg, u32 tmp_reg, bool psx_mem_mapped)
{
	if (psx_mem_mapped) {
		// METHOD 1 (best): psxM is mmap'd to simple fixed virtual address,
		//                  mirrored, and expansion-ROM/scratchpad/hardware-I/O
		//                  regions (psxP,psxH) are mapped at offset 0x0f00_0000.
		//                  Caller expects access to both RAM and these regions.
		//                  Thus, we need lower 28 bits of original base address.

		LUI(dst_reg, (PSX_MEM_VADDR >> 16));
#ifdef HAVE_MIPS32R2_EXT_INS
		INS(dst_reg, src_reg, 0, 28);   // dst_reg = PSX_MEM_VADDR | (src_reg & 0x0fffffff)
#else
		SLL(tmp_reg, src_reg, 4);
		SRL(tmp_reg, tmp_reg, 4);
		OR(dst_reg, dst_reg, tmp_reg);
#endif
	} else {
		// METHOD 2: Apparently mmap() and/or mirroring is unavailable, so psxM
		//           could be anywhere! RAM isn't mirrored virtually, so we
		//           must mask out all but lower 21 bits of src_reg here
		//           to handle 2MB PS1 RAM mirrors. The caller knows it can
		//           only use converted base reg to access PS1 RAM.

		LW(dst_reg, PERM_REG_1, off(psxM));
#ifdef HAVE_MIPS32R2_EXT_INS
		EXT(tmp_reg, src_reg, 0, 21);  // tmp_reg = src_reg & 0x001f_ffff
#else
		SLL(tmp_reg, src_reg, 11);
		SRL(tmp_reg, tmp_reg, 11);
#endif
		ADDU(dst_reg, dst_reg, tmp_reg);
	}
}

/* Is opcode an ALU op?
 * NOTE: only MIPS I r3000a opcodes supported (for now)
 * Returns: true if ALU op, false if not.
 *          If 'info' param is non-NULL and op is ALU, fill struct members.
 */
bool opcodeIsALU(const u32 opcode, struct ALUOpInfo *info)
{
	bool is_alu_op = false;

	struct ALUOpInfo l_info;
	l_info.writes_rt = false;
	l_info.reads_rs  = false;
	l_info.reads_rt  = false;

	if (_fOp_(opcode) == 0)
	{
		l_info.writes_rt = false;

		switch(_fFunct_(opcode))
		{
			case 0x00: // SLL    rd = rt << sa
			case 0x02: // SRL    rd = rt >> sa
			case 0x03: // SRA    rd = rt >> sa
				is_alu_op = true;
				l_info.reads_rt  = true;
				break;
			case 0x04: // SLLV   rd = rt << rs
			case 0x06: // SRLV   rd = rt >> rs
			case 0x07: // SRAV   rd = rt >> rs
			case 0x20: // ADD    rd = rs + rt
			case 0x21: // ADDU   rd = rs + rt
			case 0x22: // SUB    rd = rs - rt
			case 0x23: // SUBU   rd = rs - rt
			case 0x24: // AND    rd = rs & rt
			case 0x25: // OR     rd = rs | rt
			case 0x26: // XOR    rd = rs ^ rt
			case 0x27: // NOR    rd = ~(rs | rt)
			case 0x2a: // SLT    rd = rs < rt
			case 0x2b: // SLTU   rd = rs < rt
				is_alu_op = true;
				l_info.reads_rs  = true;
				l_info.reads_rt  = true;
				break;
			default:
				break;
		}
	} else if (_fOp_(opcode) >= 0x08 && _fOp_(opcode) <= 0x0f)
	{
		// fOp
		// 0x08 ADDI   rt = rs + imm
		// 0x09 ADDIU  rt = rs + imm
		// 0x0a SLTI   rt = rs < imm
		// 0x0b SLTIU  rt = rs < imm
		// 0x0c ANDI   rt = rs & imm
		// 0x0d ORI    rt = rs | imm
		// 0x0e XORI   rt = rs ^ imm
		// 0x0f LUI    rt = imm << 16

		is_alu_op = true;
		l_info.writes_rt = true;

		// LUI is unique: it reads nothing
		if (_fOp_(opcode) != 0x0f)
			l_info.reads_rs = true;
	}

	if (is_alu_op && info)
		*info = l_info;

	return is_alu_op;
}

/* opcodeGetReads() / opcodeGetWrites adapted from Nebuleon's Mupen64Plus JIT work
 *   with permission of author and under GPLv2 license.
 *   https://github.com/Nebuleon/mupen64plus-core/
 *  NOTE: Our adaptations only support PS1 MIPS r3000a opcodes.
 *
 *  Return value is u64 where bits 0..31 represent $zero..$ra reg read/writes,
 *   bit 32 is LO reg, bit 33 is HI reg (MDU registers). Caller can discard or
 *   ignore upper half of u64 result when only GPR info is needed.
 */
#ifdef BIT
#undef BIT
#endif
#define BIT(b) ((u64)1 << (b))

u64 opcodeGetReads(const u32 op)
{
	switch (_fOp_(op))
	{
		case 0: /* SPECIAL prefix */
			switch (_fFunct_(op))
			{
				case 0x0: /* SPECIAL opcode 0x0: SLL     */
				case 0x2: /* SPECIAL opcode 0x2: SRL     */
				case 0x3: /* SPECIAL opcode 0x3: SRA     */
					return BIT(_fRt_(op));
				case 0x4: /* SPECIAL opcode 0x4: SLLV    */
				case 0x6: /* SPECIAL opcode 0x6: SRLV    */
				case 0x7: /* SPECIAL opcode 0x7: SRAV    */
					return BIT(_fRt_(op)) | BIT(_fRs_(op));
				case 0x8: /* SPECIAL opcode 0x8: JR      */
				case 0x9: /* SPECIAL opcode 0x8: JALR    */
					return BIT(_fRs_(op));
				case 0xc: /* SPECIAL opcode 0xc: SYSCALL */
				case 0xd: /* SPECIAL opcode 0xd: BREAK   */
					return 0;
				case 0x10: /* SPECIAL opcode 0x10: MFHI */
					/* Does not read integer registers, only HI */
					return BIT(33);
				case 0x11: /* SPECIAL opcode 0x11: MTHI */
					return BIT(_fRs_(op));
				case 0x12: /* SPECIAL opcode 0x12: MFLO */
					/* Does not read integer registers, only LO */
					return BIT(32);
				case 0x13: /* SPECIAL opcode 0x13: MTLO */
					return BIT(_fRs_(op));
				case 0x18: /* SPECIAL opcode 0x18: MULT */
				case 0x19: /* SPECIAL opcode 0x19: MULTU */
				case 0x1a: /* SPECIAL opcode 0x1a: DIV */
				case 0x1b: /* SPECIAL opcode 0x1b: DIVU */
				case 0x20: /* SPECIAL opcode 0x20: ADD */
				case 0x21: /* SPECIAL opcode 0x21: ADDU */
				case 0x22: /* SPECIAL opcode 0x22: SUB */
				case 0x23: /* SPECIAL opcode 0x23: SUBU */
				case 0x24: /* SPECIAL opcode 0x24: AND */
				case 0x25: /* SPECIAL opcode 0x25: OR */
				case 0x26: /* SPECIAL opcode 0x26: XOR */
				case 0x27: /* SPECIAL opcode 0x27: NOR */
				case 0x2a: /* SPECIAL opcode 0x2a: SLT */
				case 0x2b: /* SPECIAL opcode 0x2b: SLTU */
					return BIT(_fRs_(op)) | BIT(_fRt_(op));
			}
			break;
		case 0x1: /* REGIMM prefix */
			switch (_fRt_(op))
			{
				case 0x0: /* REGIMM opcode 0x0: BLTZ */
				case 0x1: /* REGIMM opcode 0x1: BGEZ */
				case 0x10: /* REGIMM opcode 0x10: BLTZAL */
				case 0x11: /* REGIMM opcode 0x11: BGEZAL */
					return BIT(_fRs_(op));
			}
			break;
		case 0x2: /* Major opcode 0x2: J */
		case 0x3: /* Major opcode 0x3: JAL */
			return 0;
		case 0x4: /* Major opcode 0x4: BEQ */
		case 0x5: /* Major opcode 0x5: BNE */
			return BIT(_fRs_(op)) | BIT(_fRt_(op));
		case 0x6: /* Major opcode 0x6: BLEZ */
		case 0x7: /* Major opcode 0x7: BGTZ */
			return BIT(_fRs_(op));
		case 0x8: /* Major opcode 0x8: ADDI */
		case 0x9: /* Major opcode 0x9: ADDIU */
		case 0xa: /* Major opcode 0xa: SLTI */
		case 0xb: /* Major opcode 0xb: SLTIU */
		case 0xc: /* Major opcode 0xc: ANDI */
		case 0xd: /* Major opcode 0xd: ORI */
		case 0xe: /* Major opcode 0xe: XORI */
			return BIT(_fRs_(op));
		case 0xf: /* Major opcode 0xf: LUI */
			return 0;
		case 0x10: /* Coprocessor 0 prefix */
			switch (_fRs_(op))
			{
				case 0x0: /* Coprocessor 0 opcode 0x0: MFC0 */
					return 0;
				case 0x4: /* Coprocessor 0 opcode 0x4: MTC0 */
					return BIT(_fRt_(op));
				case 0x10: /* Coprocessor 0 opcode 0x10: RFE */
					return 0;
			}
			break;
		case 0x12: /* Coprocessor 2 prefix (GTE) */
			switch (_fRs_(op))
			{
				case 0x0: /* Coprocessor 2 opcode 0x0: MFC2 */
				case 0x2: /* Coprocessor 2 opcode 0x2: CFC2 */
					return 0;
				case 0x4: /* Coprocessor 2 opcode 0x4: MTC2 */
				case 0x6: /* Coprocessor 2 opcode 0x6: CTC2 */
					return BIT(_fRt_(op));
				case 0x10 ... 0x1f: /* Coprocessor 2 opcode 0x10..0x1f: GTE command */
					return 0;
			}
			break;
		case 0x20: /* Major opcode 0x20: LB */
		case 0x21: /* Major opcode 0x21: LH */
			return BIT(_fRs_(op));
		case 0x22: /* Major opcode 0x22: LWL */
			/* NOTE: Merges read from mem with dest reg */
			return BIT(_fRt_(op)) | BIT(_fRs_(op));
		case 0x23: /* Major opcode 0x23: LW */
		case 0x24: /* Major opcode 0x24: LBU */
		case 0x25: /* Major opcode 0x25: LHU */
			return BIT(_fRs_(op));
		case 0x26: /* Major opcode 0x26: LWR */
			/* NOTE: Merges read from mem with dest reg */
			return BIT(_fRt_(op)) | BIT(_fRs_(op));
		case 0x28: /* Major opcode 0x28: SB */
		case 0x29: /* Major opcode 0x29: SH */
		case 0x2a: /* Major opcode 0x2a: SWL */
		case 0x2b: /* Major opcode 0x2b: SW */
		case 0x2e: /* Major opcode 0x2e: SWR */
			return BIT(_fRs_(op)) | BIT(_fRt_(op));
		case 0x32: /* Major opcode 0x32: LWC2 (GTE) */
			return BIT(_fRs_(op));
		case 0x3a: /* Major opcode 0x32: SWC2 (GTE) */
			return BIT(_fRs_(op));
	}

	printf("Unknown opcode in %s(): %08x\n", __func__, op);

	/* We don't know what the opcode did. Assume EVERY register was read.
	 * This is a safe default for optimisation purposes, as this opcode will
	 * then act as a barrier. */
	return ~(u64)0;
}

u64 opcodeGetWrites(const u32 op)
{
	switch (_fOp_(op))
	{
		case 0x0: /* SPECIAL prefix */
			switch (_fFunct_(op))
			{
				case 0x0: /* SPECIAL opcode 0x0: SLL */
				case 0x2: /* SPECIAL opcode 0x2: SRL */
				case 0x3: /* SPECIAL opcode 0x3: SRA */
				case 0x4: /* SPECIAL opcode 0x4: SLLV */
				case 0x6: /* SPECIAL opcode 0x6: SRLV */
				case 0x7: /* SPECIAL opcode 0x7: SRAV */
					return BIT(_fRd_(op)) & ~BIT(0);
				case 0x8: /* SPECIAL opcode 0x8: JR */
					return 0;
				case 0x9: /* SPECIAL opcode 0x8: JALR */
					return BIT(_fRd_(op)) & ~BIT(0);
				case 0xc: /* SPECIAL opcode 0xc: SYSCALL */
				case 0xd: /* SPECIAL opcode 0xd: BREAK */
					return 0;
				case 0x10: /* SPECIAL opcode 0x10: MFHI */
					return BIT(_fRd_(op)) & ~BIT(0);
				case 0x11: /* SPECIAL opcode 0x11: MTHI */
					/* Does not write to integer registers, only to HI */
					return BIT(33);
				case 0x12: /* SPECIAL opcode 0x12: MFLO */
					return BIT(_fRd_(op)) & ~BIT(0);
				case 0x13: /* SPECIAL opcode 0x13: MTLO */
					/* Does not write to integer registers, only to LO */
					return BIT(32);
				case 0x18: /* SPECIAL opcode 0x18: MULT */
				case 0x19: /* SPECIAL opcode 0x19: MULTU */
				case 0x1a: /* SPECIAL opcode 0x1a: DIV */
				case 0x1b: /* SPECIAL opcode 0x1b: DIVU */
					/* Does not write to integer registers, only to HI and LO */
					return BIT(32) | BIT(33);
				case 0x20: /* SPECIAL opcode 0x20: ADD */
				case 0x21: /* SPECIAL opcode 0x21: ADDU */
				case 0x22: /* SPECIAL opcode 0x22: SUB */
				case 0x23: /* SPECIAL opcode 0x23: SUBU */
				case 0x24: /* SPECIAL opcode 0x24: AND */
				case 0x25: /* SPECIAL opcode 0x25: OR */
				case 0x26: /* SPECIAL opcode 0x26: XOR */
				case 0x27: /* SPECIAL opcode 0x27: NOR */
				case 0x2a: /* SPECIAL opcode 0x2a: SLT */
				case 0x2b: /* SPECIAL opcode 0x2b: SLTU */
					return BIT(_fRd_(op)) & ~BIT(0);
			}
			break;
		case 0x1: /* REGIMM prefix */
			switch (_fRt_(op))
			{
				case 0x0: /* REGIMM opcode 0x0: BLTZ */
				case 0x1: /* REGIMM opcode 0x1: BGEZ */
					return 0;
				case 0x10: /* REGIMM opcode 0x10: BLTZAL */
				case 0x11: /* REGIMM opcode 0x11: BGEZAL */
					return BIT(31);
			}
			break;
		case 0x2: /* Major opcode 0x2: J */
			return 0;
		case 0x3: /* Major opcode 0x3: JAL */
			return BIT(31);
		case 0x4: /* Major opcode 0x4: BEQ */
		case 0x5: /* Major opcode 0x5: BNE */
		case 0x6: /* Major opcode 0x6: BLEZ */
		case 0x7: /* Major opcode 0x7: BGTZ */
			return 0;
		case 0x8: /* Major opcode 0x8: ADDI */
		case 0x9: /* Major opcode 0x9: ADDIU */
		case 0xa: /* Major opcode 0xa: SLTI */
		case 0xb: /* Major opcode 0xb: SLTIU */
		case 0xc: /* Major opcode 0xc: ANDI */
		case 0xd: /* Major opcode 0xd: ORI */
		case 0xe: /* Major opcode 0xe: XORI */
		case 0xf: /* Major opcode 0xf: LUI */
			return BIT(_fRt_(op)) & ~BIT(0);
		case 0x10: /* Coprocessor 0 prefix */
			switch (_fRs_(op))
			{
				case 0x0: /* Coprocessor 0 opcode 0x0: MFC0 */
					return BIT(_fRt_(op)) & ~BIT(0);
				case 0x4: /* Coprocessor 0 opcode 0x4: MTC0 */
					return 0;
				case 0x10: /* Coprocessor 0 opcode 0x10: RFE */
					return 0;
			}
			break;
		case 0x12: /* Coprocessor 2 prefix (GTE) */
			switch (_fRs_(op))
			{
				case 0x0: /* Coprocessor 2 opcode 0x0: MFC2 */
				case 0x2: /* Coprocessor 2 opcode 0x2: CFC2 */
					return BIT(_fRt_(op)) & ~BIT(0);
				case 0x4: /* Coprocessor 2 opcode 0x4: MTC2 */
				case 0x6: /* Coprocessor 2 opcode 0x6: CTC2 */
				case 0x10 ... 0x1f: /* Coprocessor 2 opcode 0x10..0x1f: GTE command */
					return 0;
			}
			break;
		case 0x20: /* Major opcode 0x20: LB */
		case 0x21: /* Major opcode 0x21: LH */
		case 0x22: /* Major opcode 0x22: LWL */
		case 0x23: /* Major opcode 0x23: LW */
		case 0x24: /* Major opcode 0x24: LBU */
		case 0x25: /* Major opcode 0x25: LHU */
		case 0x26: /* Major opcode 0x26: LWR */
			return BIT(_fRt_(op)) & ~BIT(0);
		case 0x28: /* Major opcode 0x28: SB */
		case 0x29: /* Major opcode 0x29: SH */
		case 0x2a: /* Major opcode 0x2a: SWL */
		case 0x2b: /* Major opcode 0x2b: SW */
		case 0x2e: /* Major opcode 0x2e: SWR */
			return 0;
		case 0x32: /* Major opcode 0x32: LWC2 (GTE) */
		case 0x3a: /* Major opcode 0x32: SWC2 (GTE) */
			return 0;
	}

	printf("Unknown opcode in %s(): %08x\n", __func__, op);

	/* We don't know what the opcode did. Assume EVERY register was written.
	 * This is a safe default for optimisation purposes, as this opcode will
	 * then act as a barrier. */
	return ~(u64)0;
}

enum {
	DISCARD_TYPE_DIVBYZERO = 0,
	DISCARD_TYPE_DIVBYZERO_AND_OVERFLOW,
	DISCARD_TYPE_COUNT
};

static const char *mipsrec_discard_type_str[] =
{
	"GCC AUTO-GENERATED DIV-BY-ZERO EXCEPTION CHECK",
	"GCC AUTO-GENERATED DIV-BY-ZERO,OVERFLOW EXCEPTION CHECK"
};

/*
 * Scans for compiler auto-generated instruction sequences that PS1 GCC
 *  added immediately after most DIV/DIVU opcodes. The sequences check for
 *  div-by-0 (for DIV/DIVU), followed by check for signed overflow (for DIV).
 * If either condition was found, original code would have executed
 *  a BREAK opcode, causing BIOS handler to crash the PS1 with a
 *  SystemErrorUnresolvedException, so these code sequences merely bloat
 *  the recompiled code. Values written to $at in these compiler-generated
 *  sequences were not propagated to code outside them.
 * Not only can these sequences be omitted when recompiling, but their
 *  presence allows the DIV/DIVU emitters to know when they can avoid adding
 *  their own check-and-emulate-PS1-div-by-zero-result sequences.
 * NOTE: Rarely, a MFHI and/or MFLO will separate the DIV/DIVU from the
 *  check sequence, so recScanForSequentialMFHI_MFLO() is also provided.
 *  Even more rarely, they can be separated by other types of instructions,
 *  which we'll leave handled by some fancier future optimizer (TODO).
 *
 * Returns: # of opcodes found, 0 if sequence not found.
 */
int rec_scan_for_div_by_zero_check_sequence(u32 code_loc)
{
	int instr_cnt = 0;

	// Div-by-zero check always came first in sequence
	if (((OPCODE_AT(code_loc   ) & 0xfc1fffff) == 0x14000002) &&  // bne ???, zero, +8
	     (OPCODE_AT(code_loc+4 )               == 0         ) &&  // nop
	     (OPCODE_AT(code_loc+8 )               == 0x0007000d))    // break 0x1c00
	{ instr_cnt += 3; }

	// If opcode was DIV, it also checked for signed-overflow
	if ( instr_cnt &&
	     (OPCODE_AT(code_loc+12)               == 0x2401ffff) &&  // li at, -1
	    ((OPCODE_AT(code_loc+16) & 0xfc1fffff) == 0x14010004) &&  // bne ???, at, +16
	     (OPCODE_AT(code_loc+20)               == 0x3c018000) &&  // lui at, 0x8000
	    ((OPCODE_AT(code_loc+24) & 0x141fffff) == 0x14010002) &&  // bne ???, at, +8
	     (OPCODE_AT(code_loc+28)               == 0         ) &&  // nop
	     (OPCODE_AT(code_loc+32)               == 0x0006000d))    // break 0x1800
	{ instr_cnt += 6; }

	return instr_cnt;
}

/*
 * Returns: # of sequential MFHI/MFLO opcodes at PS1 code loc, 0 if none found.
 */
int rec_scan_for_MFHI_MFLO_sequence(u32 code_loc)
{
	int instr_cnt = 0;

	while (((OPCODE_AT(code_loc ) & 0xffff07ff) == 0x00000010) ||  // mfhi ???
	       ((OPCODE_AT(code_loc ) & 0xffff07ff) == 0x00000012))    // mflo ???
	{
		instr_cnt++;
		code_loc += 4;
	}

	return instr_cnt;
}

/*
 * Scans for sequential instructions at PS1 code location that can be safely
 *  ignored/discarded when recompiling. Stops when first sequence is found.
 *  Int ptr arg 'discard_type' is set to type of sequence found.
 *
 * Returns: # of sequential opcodes that can be discarded, 0 if none.
 */
int rec_discard_scan(u32 code_loc, int *discard_type)
{
	int instr_cnt;

	instr_cnt = rec_scan_for_div_by_zero_check_sequence(code_loc);
	if (instr_cnt) {
		if (discard_type) {
			if (instr_cnt > 3)
				*discard_type = DISCARD_TYPE_DIVBYZERO_AND_OVERFLOW;
			else
				*discard_type = DISCARD_TYPE_DIVBYZERO;
		}

		return instr_cnt;
	}

	return 0;
}

/*
 * Returns: Char string ptr describing discardable sequence 'discard_type'
 */
const char* rec_discard_type_str(int discard_type)
{
	if (discard_type >= 0 && discard_type < DISCARD_TYPE_COUNT)
		return mipsrec_discard_type_str[discard_type];
	else
		return "ERROR: Discard type out of range";
}
