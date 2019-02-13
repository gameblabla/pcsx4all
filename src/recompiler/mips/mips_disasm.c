/* gameplaySP
 *
 * Copyright (C) 2007 Exophase <exophase@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "psxcommon.h"
#include "disasm.h"

const char *mips_opcode_names[] =
{
  // 0     1        2       3        4       5      6       7
  "spec", "regim", "j",    "jal",   "beq",  "bne", "blez", "bgtz",
  // 8     9        a       b        c       d      e       f
  "addi", "addiu", "slti", "sltiu", "andi", "ori", "xori", "lui",
  // 10    11       12      13       14      15     16      17
  "???",  "???",   "???",  "???",   "???",  "???", "???",  "???",
  // 18    19       1a      1b       1c      1d     1e      1f
  "llo?", "lhi?",  "trap", "???",   "spc2", "???", "???",  "spc3",
  // 20    21       22      23       24      25     26      27
  "lb",   "lh",    "lwl",  "lw",    "lbu",  "lhu", "lwr",  "???",
  // 28    29       2a      2b       2c      2d     2e      2f
  "sb",   "sh",    "swl",  "sw",    "???",  "???", "swr",  "???",
  // 30    31       32      33      34      35       36      37
  "???",  "???",   "lwc2", "???",  "???",  "???",   "???",   "???",
  // 38    39       3a      3b      3c      3d       3e      3f
  "???",  "???",   "swc2", "???",  "???",  "???",   "???",   "???"
};

const char *mips_function_special_names[] =
{
  // 0     1        2       3       4       5        6       7
  "sll",  "???",   "srl",  "sra",  "sllv", "???",   "srlv", "srav",
  // 8     9        a       b       c       d        e       f
  "jr",   "jalr",  "movz", "movn", "syscall",  "break", "???",  "???",
  // 10    11       12      13      14      15       16      17
  "mfhi", "mthi",  "mflo", "mtlo", "???",  "???",   "???",  "???",
  // 18    19       1a      1b      1c      1d       1e      1f
  "mult", "multu", "div",  "divu", "???",  "???",   "???",  "???",
  // 20    21       22      23      24      25       26      27
  "add",  "addu",  "sub",  "subu", "and",  "or",    "xor",  "nor",
  // 28    29       2a      2b      2c      2d       2e      2f
  "???",  "???",   "slt",  "sltu", "???",   "???",  "???",  "???"
};

const char *mips_function_special2_names[] =
{
  // 0     1        2       3       4       5        6       7
  "madd", "maddu", "mul",  "???",  "msub", "msubu", "???",   "???",
  // 8     9        a       b       c       d        e       f
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 10    11       12      13      14      15       16      17
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 18    19       1a      1b      1c      1d       1e      1f
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 20    21       22      23      24      25       26      27
  "clz",  "clo",   "???",  "???",  "???",  "???",   "???",   "???",
  // 28    29       2a      2b      2c      2d       2e      2f
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 30    31       32      33      34      35       36      37
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 38    39       3a      3b      3c      3d       3e      3f
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???"
};

// Special3 instructions introduced in MIPS32r2
const char *mips_function_special3_names[] =
{
  // 0     1        2       3       4       5        6       7
  "ext",  "???",   "???",  "???",  "ins",  "???",   "???",  "???",
  // 8     9        a       b       c       d        e       f
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 10    11       12      13      14      15       16      17
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 18    19       1a      1b      1c      1d       1e      1f
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 20    21       22      23      24      25       26      27
  "bshfl","???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 28    29       2a      2b      2c      2d       2e      2f
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 30    31       32      33      34      35       36      37
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???",
  // 38    39       3a      3b      3c      3d       3e      3f
  "???",  "???",   "???",  "???",  "???",  "???",   "???",   "???"
};

// Special3 instruction entry 'bshfl' points to a further group of MIPS32r2
//  instructions. Bits 10:6 of opcode specify which entry here.
const char *mips_function_special3_bshfl_names[] =
{
  // 0     1      2       3      4       5      6       7
  "???",  "???", "???",  "???", "???",  "???", "???",  "???",
  // 8     9      a       b      c       d      e      f
  "???",  "???", "???",  "???", "???",  "???", "???",  "???",
  // 10    11     12      13     14      15     16     17
  "seb",  "???", "???",  "???", "???",  "???", "???",  "???",
  // 18    19     1a      1b     1c      1d     1e     1f
  "seh",  "???", "???",  "???", "???",  "???", "???",  "???"
};

const char *mips_function_regimm_names[] =
{
  // 0       1         2          3          4      5      6      7
  "bltz",   "bgez",   "bltzl",   "bgezl",   "???", "???", "???", "???",
  // 8       9         a          b          c      d      e      f
  "???",    "???",    "???",     "???",     "???", "???", "???", "???",
  // 10      11        12         13         14     15     16     17
  "bltzal", "bgezal", "bltzall", "bgezall", "???", "???", "???", "???",
  // 18      19        1a         1b         1c     1d     1e     1f
  "???",    "???",    "???",     "???",     "???", "???", "???", "???",
};

const char *mips_cop0_rs_names[] =
{
  // 0     1      2      3      4       5      6       7
  "mfc0", "???", "???", "???", "mtc0","???", "???",  "???",
  // 8     9      a      b      c      d      e      f
  "???",  "???", "???", "???", "???", "???", "???",  "???",
  // 10    11     12     13     14     15     16     17
  "rfe",  "???", "???", "???", "???", "???", "???",  "???",
  // 18    19     1a     1b     1c     1d     1e     1f
  "???",  "???", "???", "???", "???", "???", "???",  "???"
};

const char *mips_cop2_rs_names[] =
{
  // 0     1      2      3      4      5      6      7
  "mfc2", "???", "cfc2","???", "mtc2","???", "ctc2","???",
  // 8     9      a      b      c      d      e      f
  "???",  "???", "???", "???", "???", "???", "???",  "???",
  // 10    11     12     13     14     15     16     17
  "cop2","cop2","cop2","cop2","cop2", "cop2","cop2", "cop2",
  // 18    19     1a     1b     1c     1d     1e     1f
  "cop2","cop2","cop2","cop2","cop2", "cop2","cop2", "cop2"
};

const char *mips_cop2_gte_names[] =
{
  // 0     1       2       3       4       5       6       7
  "???",  "RTPS", "???",  "???",  "???",  "???",  "NCLIP","???",
  // 8     9       a       b       c       d       e       f
  "???",  "???",  "???",  "???",  "OP",   "???",  "???",   "???",
  // 10    11      12      13      14      15      16      17
  "DPCS", "INTPL","MVMVA","NCDS", "CDP",  "???",  "NCDT",  "???",
  // 18    19      1a      1b      1c      1d      1e      1f
  "???",  "???",  "???",  "NCCS", "CC",   "???",  "NCS",   "???",
  // 20    21      22      23      24      25      26      27
  "NCT",  "???",  "???",  "???",  "???",  "???",  "???",   "???",
  // 28    29      2a      2b      2c      2d      2e      2f
  "SQR",  "DCPL", "DPCT", "???",  "???",  "AVSZ3","AVSZ4", "???",
  // 30    31      32      33      34      35      36      37
  "RTPT", "???",  "???",  "???",  "???",  "???",  "???",   "???",
  // 38    39      3a      3b      3c      3d      3e      3f
  "???",  "???",  "???",  "???",  "???",  "GPF",  "GPL",   "NCCT"
};

typedef enum
{
  MIPS_OPCODE_ALU_IMMS,
  MIPS_OPCODE_ALU_IMMU,
  MIPS_OPCODE_ALU2_IMMU,
  MIPS_OPCODE_REGIMM,
  MIPS_OPCODE_BRANCH,
  MIPS_OPCODE_BRANCHC,
  MIPS_OPCODE_BRANCHC2,
  MIPS_OPCODE_SPECIAL,
  MIPS_OPCODE_SPECIAL2,
  MIPS_OPCODE_SPECIAL3,
  MIPS_OPCODE_MEM,
  MIPS_OPCODE_CP0,
  MIPS_OPCODE_CP2,
  MIPS_OPCODE_LWC2,
  MIPS_OPCODE_SWC2,
  MIPS_OPCODE_UNKNOWN
} mips_opcode_type;

typedef enum
{
  MIPS_CP0_FUNCTION_MFC0,
  MIPS_CP0_FUNCTION_MTC0,
  MIPS_CP0_FUNCTION_RFE,
  MIPS_CP0_FUNCTION_UNKNOWN
} mips_function_cp0_type;

typedef enum
{
  MIPS_SPECIAL_FUNCTION_ALU,
  MIPS_SPECIAL_FUNCTION_MUL_DIV,
  MIPS_SPECIAL_FUNCTION_JALR,
  MIPS_SPECIAL_FUNCTION_JR,
  MIPS_SPECIAL_FUNCTION_HI_LO,
  MIPS_SPECIAL_FUNCTION_SHIFT,
  MIPS_SPECIAL_FUNCTION_SYSCALL,
  MIPS_SPECIAL_FUNCTION_BREAK,
  MIPS_SPECIAL_FUNCTION_UNKNOWN
} mips_function_special_type;

typedef enum
{
  MIPS_SPECIAL2_FUNCTION_MUL_DIV,
  MIPS_SPECIAL2_FUNCTION_MUL_3OP,
  MIPS_SPECIAL2_FUNCTION_CLO_CLZ,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN
} mips_function_special2_type;

typedef enum
{
  MIPS_SPECIAL3_FUNCTION_EXT,
  MIPS_SPECIAL3_FUNCTION_INS,
  MIPS_SPECIAL3_FUNCTION_BSHFL,       // BSHFL points to group of instructions below
  MIPS_SPECIAL3_FUNCTION_UNKNOWN
} mips_function_special3_type;

typedef enum
{
  MIPS_SPECIAL3_BSHFL_FUNCTION_SEB,
  MIPS_SPECIAL3_BSHFL_FUNCTION_SEH,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN
} mips_function_special3_bshfl_type;

mips_opcode_type mips_opcode_types[] =
{
  // special             regimm
  MIPS_OPCODE_SPECIAL,  MIPS_OPCODE_REGIMM,
  // j                   jal
  MIPS_OPCODE_BRANCH,   MIPS_OPCODE_BRANCH,
  // beq                 bne
  MIPS_OPCODE_BRANCHC2, MIPS_OPCODE_BRANCHC2,
  // blez                bgetz
  MIPS_OPCODE_BRANCHC,  MIPS_OPCODE_BRANCHC,

  // addi                addiu
  MIPS_OPCODE_ALU_IMMS, MIPS_OPCODE_ALU_IMMS,
  // slti                sltiu
  MIPS_OPCODE_ALU_IMMS, MIPS_OPCODE_ALU_IMMS,
  // andi                ori
  MIPS_OPCODE_ALU_IMMU, MIPS_OPCODE_ALU_IMMU,
  // xori                lui
  MIPS_OPCODE_ALU_IMMU, MIPS_OPCODE_ALU2_IMMU,

  MIPS_OPCODE_CP0,      MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_CP2,      MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,

  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  // special2
  MIPS_OPCODE_SPECIAL2, MIPS_OPCODE_UNKNOWN,
  //                     special 3
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_SPECIAL3,

  // lb                  lh
  MIPS_OPCODE_MEM,      MIPS_OPCODE_MEM,
  // lwl                 lw
  MIPS_OPCODE_MEM,      MIPS_OPCODE_MEM,
  // lbu                 lhu
  MIPS_OPCODE_MEM,      MIPS_OPCODE_MEM,
  // lwr
  MIPS_OPCODE_MEM,      MIPS_OPCODE_UNKNOWN,

  // sb                  sh
  MIPS_OPCODE_MEM,      MIPS_OPCODE_MEM,
  // swl                 sw
  MIPS_OPCODE_MEM,      MIPS_OPCODE_MEM,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  // swr
  MIPS_OPCODE_MEM,      MIPS_OPCODE_UNKNOWN,

  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  // lwc2
  MIPS_OPCODE_LWC2,     MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,

  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  // swc2
  MIPS_OPCODE_SWC2,     MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN
};

mips_function_cp0_type mips_function_cp0_types[] =
{
  // mfc0
  MIPS_CP0_FUNCTION_MFC0,    MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,
  // mtc0
  MIPS_CP0_FUNCTION_MTC0,    MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,

  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,

  // rfe
  MIPS_CP0_FUNCTION_RFE,     MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,

  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN,
  MIPS_CP0_FUNCTION_UNKNOWN, MIPS_CP0_FUNCTION_UNKNOWN
};

mips_function_special_type mips_function_special_types[] =
{
  // sll
  MIPS_SPECIAL_FUNCTION_SHIFT,   MIPS_SPECIAL_FUNCTION_UNKNOWN,
  // srl                          sra
  MIPS_SPECIAL_FUNCTION_SHIFT,   MIPS_SPECIAL_FUNCTION_SHIFT,
  // sllv
  MIPS_SPECIAL_FUNCTION_ALU,     MIPS_SPECIAL_FUNCTION_UNKNOWN,
  // srlv                         srav
  MIPS_SPECIAL_FUNCTION_ALU,     MIPS_SPECIAL_FUNCTION_ALU,

  // jr                           jalr
  MIPS_SPECIAL_FUNCTION_JR,      MIPS_SPECIAL_FUNCTION_JALR,
  // movz                         movn
  MIPS_SPECIAL_FUNCTION_ALU,     MIPS_SPECIAL_FUNCTION_ALU,
  MIPS_SPECIAL_FUNCTION_SYSCALL, MIPS_SPECIAL_FUNCTION_BREAK,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,

  // mfhi                         mthi
  MIPS_SPECIAL_FUNCTION_HI_LO,   MIPS_SPECIAL_FUNCTION_HI_LO,
  // mflo                         mtlo
  MIPS_SPECIAL_FUNCTION_HI_LO,   MIPS_SPECIAL_FUNCTION_HI_LO,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,

  // mult                         multu
  MIPS_SPECIAL_FUNCTION_MUL_DIV, MIPS_SPECIAL_FUNCTION_MUL_DIV,
  // div                          divu
  MIPS_SPECIAL_FUNCTION_MUL_DIV, MIPS_SPECIAL_FUNCTION_MUL_DIV,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,

  // add                          addu
  MIPS_SPECIAL_FUNCTION_ALU,     MIPS_SPECIAL_FUNCTION_ALU,
  // sbu                          subu
  MIPS_SPECIAL_FUNCTION_ALU,     MIPS_SPECIAL_FUNCTION_ALU,
  // and                          or
  MIPS_SPECIAL_FUNCTION_ALU,     MIPS_SPECIAL_FUNCTION_ALU,
  // xor                          not
  MIPS_SPECIAL_FUNCTION_ALU,     MIPS_SPECIAL_FUNCTION_ALU,

  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
  // slt                          sltu
  MIPS_SPECIAL_FUNCTION_ALU,     MIPS_SPECIAL_FUNCTION_ALU,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,

  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,

  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL_FUNCTION_UNKNOWN, MIPS_SPECIAL_FUNCTION_UNKNOWN,
};

mips_function_special2_type mips_function_special2_types[] =
{
  // madd                          maddu
  MIPS_SPECIAL2_FUNCTION_MUL_DIV, MIPS_SPECIAL2_FUNCTION_MUL_DIV,
  // mul
  MIPS_SPECIAL2_FUNCTION_MUL_3OP, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  // msub                          msubu
  MIPS_SPECIAL2_FUNCTION_MUL_DIV, MIPS_SPECIAL2_FUNCTION_MUL_DIV,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,

  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,

  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,

  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,

  // clz                           clo
  MIPS_SPECIAL2_FUNCTION_CLO_CLZ, MIPS_SPECIAL2_FUNCTION_CLO_CLZ,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,

  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,

  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,

  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
  MIPS_SPECIAL2_FUNCTION_UNKNOWN, MIPS_SPECIAL2_FUNCTION_UNKNOWN,
};

mips_function_special3_type mips_function_special3_types[] =
{
  // ext
  MIPS_SPECIAL3_FUNCTION_EXT,     MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  // ins
  MIPS_SPECIAL3_FUNCTION_INS,     MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,

  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,

  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,

  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,

  // 'bshfl' instruction group
  MIPS_SPECIAL3_FUNCTION_BSHFL,   MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,

  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,

  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,

  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_FUNCTION_UNKNOWN, MIPS_SPECIAL3_FUNCTION_UNKNOWN,
};

mips_function_special3_bshfl_type mips_function_special3_bshfl_types[] =
{
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,

  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,

  // seb
  MIPS_SPECIAL3_BSHFL_FUNCTION_SEB,     MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,

  // seh
  MIPS_SPECIAL3_BSHFL_FUNCTION_SEH,     MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN,
  MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN, MIPS_SPECIAL3_BSHFL_FUNCTION_UNKNOWN
};

// Change these to whatever you want.

// GAS names

const char *mips_reg_names[] =
{
  "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
  "t0",   "t1", "t2", "t3", "t4", "t5", "t6", "t7",
  "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7",
  "t8",   "t9", "k0", "k1", "gp", "sp", "s8", "ra",
};


/*

// Raw names

char *mips_reg_names[] =
{
  "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
  "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
  "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
};

*/


#define bits(value, offset, mask)                                             \
  (unsigned int)((value >> (offset)) & (mask))                                              \

#define op_bits(offset, mask)                                                 \
  bits(opcode, offset, mask)                                                  \

#define reg_rs 21
#define reg_rt 16
#define reg_rd 11

#define reg_op(offset)                                                        \
  (mips_reg_names[op_bits(offset, 0x1F)])                                          \

#define immu()                                                                \
  (unsigned int)op_bits(0, 0xFFFF)                                                          \

#define imms()                                                                \
  ((int)(op_bits(0, 0xFFFF) << 16) >> 16)                                     \

#define signed_offset()                                                       \
  imms()                                                                      \

#define pc_offset()                                                           \
  (unsigned int)((signed_offset() << 2) + (pc + 4))                                         \


void disasm_mips_instruction(u32 opcode, char *buffer, u32 pc,
  disasm_label *labels, u32 num_labels)
{
  int i;
  int opcode_type = opcode >> 26;

  if(opcode == 0)
  {
    sprintf(buffer, "nop");
    return;
  }

  switch(mips_opcode_types[opcode_type])
  {
    case MIPS_OPCODE_ALU_IMMS:
    {
      u32 rs = op_bits(reg_rs, 0x1F);

      if((opcode_type == 0x9) && (rs == 0))
      {
        sprintf(buffer, "li %s, %d",
         reg_op(reg_rt), imms());
      }
      else
      {
        sprintf(buffer, "%s %s, %s, %d",
         mips_opcode_names[opcode_type], reg_op(reg_rt),
         mips_reg_names[rs], imms());
      }
      break;
    }

    case MIPS_OPCODE_ALU_IMMU:
    {
      sprintf(buffer, "%s %s, %s, 0x%x",
       mips_opcode_names[opcode_type], reg_op(reg_rt), reg_op(reg_rs),
       immu());
      break;
    }

    case MIPS_OPCODE_ALU2_IMMU:
    {
      sprintf(buffer, "%s %s, 0x%x",
       mips_opcode_names[opcode_type], reg_op(reg_rt), immu());
      break;
    }


    case MIPS_OPCODE_REGIMM:
    {
      u32 function = op_bits(16, 0x1F);

      for(i = 0; i < num_labels; i++)
      {
        //DEBUGG("label 0x%x pcoff 0x%x\n", (u32)labels[i].address, pc_offset());
        if((u32)labels[i].address == pc_offset())
        {
          sprintf(buffer, "%s %s, %s", mips_function_regimm_names[function], reg_op(reg_rs),
           labels[i].name);
          break;
        }
      }

      sprintf(buffer, "%s %s, %08x",
       mips_function_regimm_names[function], reg_op(reg_rs),
       pc_offset());
      break;
    }

    case MIPS_OPCODE_SPECIAL:
    {
      mips_function_special_type function = (mips_function_special_type)op_bits(0, 0x3F);

      switch(mips_function_special_types[function])
      {
        case MIPS_SPECIAL_FUNCTION_ALU:
        {
          sprintf(buffer, "%s %s, %s, %s",
           mips_function_special_names[function], reg_op(reg_rd),
           reg_op(reg_rs), reg_op(reg_rt));
          break;
        }

        case MIPS_SPECIAL_FUNCTION_SYSCALL:
        {
          sprintf(buffer,"syscall");
          break;
        }
        
        case MIPS_SPECIAL_FUNCTION_BREAK:
        {
          sprintf(buffer,"break 0x%x", op_bits(6, 0xFFFFF));
          break;
        }

        case MIPS_SPECIAL_FUNCTION_MUL_DIV:
        {
          sprintf(buffer, "%s %s, %s",
           mips_function_special_names[function], reg_op(reg_rs),
           reg_op(reg_rt));
          break;
        }

        case MIPS_SPECIAL_FUNCTION_JALR: 
        { 
          u32 rd = op_bits(reg_rd, 0x1F); 
 
          if(rd == 31) 
          { 
            sprintf(buffer, "%s %s", 
             mips_function_special_names[function], reg_op(reg_rs)); 
          } 
          else 
          { 
            sprintf(buffer, "%s %s, %s", 
             mips_function_special_names[function], mips_reg_names[rd], 
             reg_op(reg_rs)); 
          } 
          break; 
        } 

        case MIPS_SPECIAL_FUNCTION_JR:
        {
          sprintf(buffer, "%s %s",
           mips_function_special_names[function], reg_op(reg_rs));
          break;
        }

        case MIPS_SPECIAL_FUNCTION_HI_LO:
        {
          sprintf(buffer, "%s %s",
           mips_function_special_names[function], reg_op(reg_rd));
          break;
        }

        case MIPS_SPECIAL_FUNCTION_SHIFT:
        {
          sprintf(buffer, "%s %s, %s, %d",
           mips_function_special_names[function], reg_op(reg_rd),
           reg_op(reg_rt), op_bits(6, 0x1F));
          break;
        }

        default:
        {
          sprintf(buffer, "unknown");
          break;
        }
      }
      break;
    }

	case MIPS_OPCODE_SPECIAL2:
    {
      mips_function_special2_type function = (mips_function_special2_type)op_bits(0, 0x3F);

      switch(mips_function_special2_types[function])
      {
        case MIPS_SPECIAL2_FUNCTION_MUL_DIV:
        {
          sprintf(buffer, "%s %s, %s",
           mips_function_special2_names[function], reg_op(reg_rs), reg_op(reg_rt));
          break;
        }

		case MIPS_SPECIAL2_FUNCTION_MUL_3OP:
        {
          sprintf(buffer, "%s %s, %s, %s",
           mips_function_special2_names[function], reg_op(reg_rd), reg_op(reg_rs), reg_op(reg_rt));
          break;
        }

		case MIPS_SPECIAL2_FUNCTION_CLO_CLZ:
        {
          sprintf(buffer, "%s %s, %s",
           mips_function_special2_names[function], reg_op(reg_rd), reg_op(reg_rs));
          break;
        }

        default:
        {
          sprintf(buffer, "unknown");
          break;
        }
      }
      break;
    }

    case MIPS_OPCODE_SPECIAL3:
    {
      mips_function_special3_type function = (mips_function_special3_type)op_bits(0, 0x3F);

      switch(mips_function_special3_types[function])
      {
        case MIPS_SPECIAL3_FUNCTION_EXT:
        case MIPS_SPECIAL3_FUNCTION_INS:
        {
          unsigned int pos = op_bits(6, 0x1F);
          unsigned int size = op_bits(11, 0x1F) + 1;
          if (mips_function_special3_types[function] == MIPS_SPECIAL3_FUNCTION_INS)
            size -= pos;
          sprintf(buffer, "%s %s, %s, %u, %u",
              mips_function_special3_names[function], reg_op(reg_rt),
              reg_op(reg_rs), pos, size);
          break;
        }

        // BSHFL points to further group of instructions encoded in bits 10:6
        case MIPS_SPECIAL3_FUNCTION_BSHFL:
        {
          mips_function_special3_bshfl_type function = (mips_function_special3_bshfl_type)op_bits(6, 0x1F);
          const char *name = mips_function_special3_bshfl_names[function];

          switch(mips_function_special3_bshfl_types[function])
          {
            case MIPS_SPECIAL3_BSHFL_FUNCTION_SEB:
            case MIPS_SPECIAL3_BSHFL_FUNCTION_SEH:
            {
              sprintf(buffer, "%s %s, %s", name, reg_op(reg_rd), reg_op(reg_rt));
              break;
            }

            default:
            {
              sprintf(buffer, "unknown");
              break;
            }
          }
          break;
        }

        default:
        {
          sprintf(buffer, "unknown");
          break;
        }
      }
      break;
    }

    case MIPS_OPCODE_BRANCH:
    {
      u32 offset = op_bits(0, 0x3FFFFFF);
      offset = (offset << 2) | ((pc + 4) & 0xFC000000);

      for(i = 0; i < num_labels; i++)
      {
        //DEBUGG("label 0x%x pcoff 0x%x\n", (u32)labels[i].address, offset);
        if((u32)labels[i].address == offset)
        {
          sprintf(buffer, "%s %s", mips_opcode_names[opcode_type],
           labels[i].name);
          break;
        }
      }
      if (i == num_labels) sprintf(buffer, "%s %08x",
       mips_opcode_names[opcode_type], (unsigned int)offset);

      break;
    }

    case MIPS_OPCODE_BRANCHC:
    {
      for(i = 0; i < num_labels; i++)
      {
        //DEBUGG("label 0x%x pcoff 0x%x\n", (u32)labels[i].address, pc_offset());
        if((u32)labels[i].address == pc_offset())
        {
          sprintf(buffer, "%s %s, %s", mips_opcode_names[opcode_type], reg_op(reg_rs),
           labels[i].name);
          break;
        }
      }

      if (i == num_labels) sprintf(buffer, "%s %s, %08x",
       mips_opcode_names[opcode_type], reg_op(reg_rs), pc_offset());
      break;
    }

    case MIPS_OPCODE_BRANCHC2:
    {
      for(i = 0; i < num_labels; i++)
      {
        //DEBUGG("label 0x%x pcoff 0x%x\n", (u32)labels[i].address, pc_offset());
        if((u32)labels[i].address == pc_offset())
        {
          sprintf(buffer, "%s %s, %s, %s", mips_opcode_names[opcode_type], reg_op(reg_rs),
           reg_op(reg_rt), labels[i].name);
          break;
        }
      }

      if (i == num_labels) sprintf(buffer, "%s %s, %s, %08x",
       mips_opcode_names[opcode_type], reg_op(reg_rs),
       reg_op(reg_rt), pc_offset());
      break;
    }

    case MIPS_OPCODE_MEM:
    {
      s32 offset = signed_offset();

      if(offset < 0)
      {
        sprintf(buffer, "%s %s, [%s - %d]",
         mips_opcode_names[opcode_type], reg_op(reg_rt), reg_op(reg_rs),
         (int)-offset);
      }
      else
      {
        sprintf(buffer, "%s %s, [%s + %d]",
         mips_opcode_names[opcode_type], reg_op(reg_rt), reg_op(reg_rs),
         (int)offset);
      }
      break;
    }

    case MIPS_OPCODE_LWC2:
    case MIPS_OPCODE_SWC2:
    {
      s32 offset = signed_offset();

      if(offset < 0)
      {
        sprintf(buffer, "%s reg(%u), [%s - %d]",
         mips_opcode_names[opcode_type], op_bits(reg_rt, 0x1f), reg_op(reg_rs),
         (int)-offset);
      }
      else
      {
        sprintf(buffer, "%s reg(%u), [%s + %d]",
         mips_opcode_names[opcode_type], op_bits(reg_rt, 0x1f), reg_op(reg_rs),
         (int)offset);
      }
      break;
    }

    case MIPS_OPCODE_CP0:
    {
      mips_function_cp0_type function = (mips_function_cp0_type)op_bits(reg_rs, 0x1F);
      const char *name = mips_cop0_rs_names[function];

      switch(mips_function_cp0_types[function])
      {
        case MIPS_CP0_FUNCTION_MFC0:
        case MIPS_CP0_FUNCTION_MTC0:
        {
          // check func field
          sprintf(buffer, "%s %s, reg(%d)",
              name, reg_op(reg_rt), op_bits(reg_rd, 0x1f));
          break;
        }

        case MIPS_CP0_FUNCTION_RFE:
        {
          sprintf(buffer, name);
          break;
        }

        default:
        {
          sprintf(buffer, "unknown");
          break;
        }
      }
      break;
    }

    case MIPS_OPCODE_CP2:
    {
        // check func field
        if (op_bits(0, 0x3f) == 0) {
            sprintf(buffer, "%s %s, reg(%d)",
                 mips_cop2_rs_names[op_bits(reg_rs, 0x1f)],
                 reg_op(reg_rt),
                 op_bits(reg_rd, 0x1f)
                );
        } else {
            // GTE command
            sprintf(buffer, "cop2 0x%x (GTE %s)",
                 op_bits(0, 0x3f),
                 mips_cop2_gte_names[op_bits(0, 0x3f)]
                );
        }
        break;
    }

    default:
    {
      sprintf(buffer, "unknown");
      break;
    }
  }
}


/*
int main()
{
  char buffer[1024];
  int opcode;
  int pc = 0;

  while(1)
  {
    if(scanf("%x", &opcode) == EOF)
      break;

    disasm_mips_instruction(opcode, buffer, pc + 4);
    printf("%08x\t%08x\t%s\n", pc, opcode, buffer);
    pc += 4;
  }

  return 0;
}
*/
