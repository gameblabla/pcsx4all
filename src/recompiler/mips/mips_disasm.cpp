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
  "sb",   "sh",    "swl",  "sw",    "???",  "???", "swr",  "???"
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
  "mult", "multu", "div",  "divu", "madd", "maddu", "???",  "???",
  // 20    21       22      23      24      25       26      27
  "add",  "addu",  "sub",  "subu", "and",  "or",    "xor",  "not",
  // 28    29       2a      2b      2c      2d       2e      2f
  "???",  "???",   "slt",  "sltu", "???",   "???",  "???",  "???"
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

const char *mips_cop2_rs_names[] =
{
  // 0     1      2       3      4       5      6       7
  "mfc2", "???", "cfc2", "???", "mtc2", "???", "ctc2", "???",
  // 8     9      a       b      c       d      e      f
  "???",  "???", "???",  "???", "???",  "???", "???",  "???",
  "???",  "???", "???",  "???", "???",  "???", "???",  "???",
  "???",  "???", "???",  "???", "???",  "???", "???",  "???",
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
  MIPS_OPCODE_CP2,
  MIPS_OPCODE_UNKNOWN
} mips_opcode_type;

typedef enum
{
  MIPS_SPECIAL_FUNCTION_ALU,
  MIPS_SPECIAL_FUNCTION_MUL_DIV,
  MIPS_SPECIAL_FUNCTION_JALR,
  MIPS_SPECIAL_FUNCTION_JR,
  MIPS_SPECIAL_FUNCTION_HI_LO,
  MIPS_SPECIAL_FUNCTION_SHIFT,
  MIPS_SPECIAL_FUNCTION_SYSCALL,
  MIPS_SPECIAL_FUNCTION_UNKNOWN
} mips_function_special_type;


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

  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
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
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,

  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,

  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN,
  MIPS_OPCODE_UNKNOWN,  MIPS_OPCODE_UNKNOWN
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
  MIPS_SPECIAL_FUNCTION_SYSCALL, MIPS_SPECIAL_FUNCTION_UNKNOWN,
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
  // madd                         maddu
  MIPS_SPECIAL_FUNCTION_MUL_DIV, MIPS_SPECIAL_FUNCTION_MUL_DIV,
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

      case MIPS_OPCODE_CP2:
      {
        // check func field
        if (op_bits(0, 0x3f) == 0)
        {
          sprintf(buffer, "%s %s, reg(%d)",
                  mips_cop2_rs_names[op_bits(reg_rs, 0x1f)],
                  reg_op(reg_rt),
                  op_bits(reg_rd, 0x1f)
                 );
        }
        else
        {
          sprintf(buffer, "unknown");
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
