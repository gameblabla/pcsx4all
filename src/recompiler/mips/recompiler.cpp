/*
 * Mips-to-mips recompiler for pcsx4all
 *
 * Copyright (c) 2009 Ulrich Hecht
 * Copyright (c) 2016 modified by Dmitry Smagin
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

#include "plugin_lib.h"
#include "psxcommon.h"
#include "psxhle.h"
#include "psxmem.h"
#include "psxhw.h"
#include "r3000a.h"
#include "gte.h"

/* Use inlined-asm version of block dispatcher: */
#define ASM_EXECUTE_LOOP

//#define WITH_DISASM
//#define DEBUGG printf

#include "mips_codegen.h"
#include "disasm.h"

typedef struct
{
  u32 s;
  u32 r;
} iRegisters;

#define IsConst(reg) (iRegs[reg].s)
#define SetUndef(reg) do { if (reg != 0) iRegs[reg].s = 0; } while (0)
#define SetConst(reg, val) do { if (reg != 0) { iRegs[reg].s = 1; iRegs[reg].r = (val); } } while (0)

static iRegisters iRegs[32]; /* used for imm caching and back up of regs in dynarec */

static u32 psxRecLUT[0x010000];

#undef PC_REC
#undef PC_REC8
#undef PC_REC16
#undef PC_REC32
#define PC_REC(x)	(psxRecLUT[(x) >> 16] + ((x) & 0xffff))
#define PC_REC8(x)	(*(u8 *)PC_REC(x))
#define PC_REC16(x)	(*(u16*)PC_REC(x))
#define PC_REC32(x)	(*(u32*)PC_REC(x))

#define RECMEM_SIZE		(12 * 1024 * 1024)
#define RECMEM_SIZE_MAX 	(RECMEM_SIZE-(512*1024))
#define REC_MAX_OPCODES		80

static u8 recMemBase[RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000] __attribute__ ((__aligned__ (32)));
static u32 *recMem; /* the recompiled blocks will be here */
static s8 recRAM[0x200000] __attribute__((aligned(4))); /* and the ptr to the blocks here */
static s8 recROM[0x080000] __attribute__((aligned(4))); /* and here */
static u32 pc;					/* recompiler pc */
static u32 oldpc;
static u32 branch = 0;

#ifdef WITH_DISASM
  char	disasm_buffer[512];
#endif

#include "regcache.h"

static void recReset();
static void recRecompile();
static void recClear(u32 Addr, u32 Size);

extern void (*recBSC[64])();
extern void (*recSPC[64])();
extern void (*recREG[32])();
extern void (*recCP0[32])();
extern void (*recCP2[64])();
extern void (*recCP2BSC[32])();

u32	*recMemStart;
u32	end_block = 0;
u32	cycle_multiplier = 0x200; // 0x200 == 2.00

#ifdef WITH_DISASM

#define make_stub_label(name) \
  { (void *)name, (char*)#name }

disasm_label stub_labels[] =
{
  make_stub_label(gteMFC2),
  make_stub_label(gteMTC2),
  make_stub_label(gteLWC2),
  make_stub_label(gteSWC2),
  make_stub_label(gteRTPS),
  make_stub_label(gteOP),
  make_stub_label(gteNCLIP),
  make_stub_label(gteDPCS),
  make_stub_label(gteINTPL),
  make_stub_label(gteMVMVA),
  make_stub_label(gteNCDS),
  make_stub_label(gteNCDT),
  make_stub_label(gteCDP),
  make_stub_label(gteNCCS),
  make_stub_label(gteCC),
  make_stub_label(gteNCS),
  make_stub_label(gteNCT),
  make_stub_label(gteSQR),
  make_stub_label(gteDCPL),
  make_stub_label(gteDPCT),
  make_stub_label(gteAVSZ3),
  make_stub_label(gteAVSZ4),
  make_stub_label(gteRTPT),
  make_stub_label(gteGPF),
  make_stub_label(gteGPL),
  make_stub_label(gteNCCT),
  make_stub_label(psxMemRead8),
  make_stub_label(psxMemRead16),
  make_stub_label(psxMemRead32),
  make_stub_label(psxMemWrite8),
  make_stub_label(psxMemWrite16),
  make_stub_label(psxMemWrite32),
  make_stub_label(psxHwRead8),
  make_stub_label(psxHwRead16),
  make_stub_label(psxHwRead32),
  make_stub_label(psxHwWrite8),
  make_stub_label(psxHwWrite16),
  make_stub_label(psxHwWrite32),
  make_stub_label(psxException),
};

const u32 num_stub_labels = sizeof(stub_labels) / sizeof(disasm_label);

#define DISASM_INIT() \
  do { \
    printf("Block PC %x (MIPS) -> %p\n", pc, recMemStart); \
  } while (0)

#define DISASM_PSX(_PC_) \
  do { \
    u32 opcode = *(u32 *)((char *)PSXM(_PC_)); \
    disasm_mips_instruction(opcode, disasm_buffer, _PC_, 0, 0); \
    printf("%08x: %08x %s\n", _PC_, opcode, disasm_buffer); \
  } while (0)

#define DISASM_HOST() \
  do { \
    printf("\n"); \
    u8 *tr_ptr = (u8*)recMemStart; \
    for (; (u32)tr_ptr < (u32)recMem; tr_ptr += 4) { \
      u32 opcode = *(u32*)tr_ptr; \
      disasm_mips_instruction(opcode, disasm_buffer, \
                              (u32)tr_ptr, stub_labels, \
                              num_stub_labels); \
      printf("%08x: %s\t(0x%08x)\n", \
             (u32)tr_ptr, disasm_buffer, opcode); \
    } \
    printf("\n"); \
  } while (0)

#else

#define DISASM_PSX(_PC_)
#define DISASM_HOST()
#define DISASM_INIT()

#endif

#include "opcodes.h"
#include <sys/cachectl.h>

void clear_insn_cache(void *start, void *end, int flags)
{
  cacheflush(start, (char *)end - (char *)start, ICACHE);
}

static void recRecompile()
{
  // Notify plugin_lib that we're recompiling (affects frameskip timing)
  pl_dynarec_notify();

  if ((u32)recMem - (u32)recMemBase >= RECMEM_SIZE_MAX )
    recReset();

  recMem = (u32*)(((u32)recMem + 64) & ~(63));
  recMemStart = recMem;

  regReset();

  PC_REC32(psxRegs.pc) = (u32)recMem;
  oldpc = pc = psxRegs.pc;

  DISASM_INIT();

  rec_recompile_start();
  memset(iRegs, 0, sizeof(iRegs));
  iRegs[0].s = 1;  // $r0 is always zero val

  do
  {
    psxRegs.code = *(u32 *)((char *)PSXM(pc));
    DISASM_PSX(pc);
    pc += 4;
    recBSC[psxRegs.code>>26]();
    regUpdate();
    branch = 0;
  }
  while (!end_block);

  end_block = 0;
  DISASM_HOST();
  clear_insn_cache(recMemStart, recMem, 0);
}

static int recInit()
{
  int i;

  recMem = (u32*)recMemBase;
  memset(recMem, 0, RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000);

  recReset();

  if (recRAM == NULL || recROM == NULL || recMemBase == NULL || psxRecLUT == NULL)
  {
    printf("Error allocating memory\n");
    return -1;
  }

  for (i = 0; i < 0x80; i++)
    psxRecLUT[i + 0x0000] = (u32)&recRAM[(i & 0x1f) << 16];

  memcpy(psxRecLUT + 0x8000, psxRecLUT, 0x80 * 4);
  memcpy(psxRecLUT + 0xa000, psxRecLUT, 0x80 * 4);

  for (i = 0; i < 0x08; i++)
    psxRecLUT[i + 0xbfc0] = (u32)&recROM[i << 16];

  return 0;
}

static void recShutdown() { }

/* It seems there's no way to tell GCC that something is being called inside
 * asm() blocks and GCC doesn't bother to save temporaries to stack.
 * That's why we have two options:
 * 1. Call recompiled blocks via recFunc() trap which is strictly noinline and
 * saves registers $s[0-7], $fp and $ra on each call, or
 * 2. Code recExecute() and recExecuteBlock() entirely in assembler taking into
 * account that no registers except $ra are saved in recompiled blocks and
 * thus put all temporaries to stack. In this case $s[0-7], $fp and $ra are saved
 * in recExecute() and recExecuteBlock() only once.
 */
#ifndef ASM_EXECUTE_LOOP
static __attribute__ ((noinline)) void recFunc(void *fn)
{
  /* This magic code calls fn address saving registers $s[0-7], $fp and $ra. */
  /*                                                                         */
  /* Focus here is on clarity, not speed. This is the bare minimum needed to */
  /*  call blocks from within C code, which is:                              */
  /* Blocks expect $fp to be set to &psxRegs.                                */
  /* Blocks expect return address to be stored at 16($sp).                   */
  /* Stack should have 16 bytes free at 0($sp) for use by called functions.  */
  /* Stack should be 8-byte aligned to satisfy MIPS ABI.                     */
  /*                                                                         */
  /* Blocks return these values, which are handled here:                     */
  /*  $v0 is new value for psxRegs.pc                                        */
  /*  $v1 is number of cycles to increment psxRegs.cycle by                  */
  __asm__ __volatile__ (
    "addiu  $sp, $sp, -24                   \n"
    "la     $fp, %[psxRegs]                 \n" // $fp = &psxRegs
    "la     $t0, block_return_addr%=        \n"
    "sw     $t0, 16($sp)                    \n" // Put 'block_return_addr' on stack
    "jr     %[fn]                           \n" // Execute block

    "block_return_addr%=:                   \n"
    "sw     $v0, %[psxRegs_pc_off]($fp)     \n" // psxRegs.pc = $v0
    "lw     $t1, %[psxRegs_cycle_off]($fp)  \n" //
    "addu   $t1, $t1, $v1                   \n" //
    "sw     $t1, %[psxRegs_cycle_off]($fp)  \n" // psxRegs.cycle += $v1
    "addiu  $sp, $sp, 24                    \n"
    : // Output
    : // Input
    [fn]                   "r" (fn),
    [psxRegs]              "i" (&psxRegs),
    [psxRegs_pc_off]       "i" (off(pc)),   // Offset of psxRegs.pc in psxRegs
    [psxRegs_cycle_off]    "i" (off(cycle)) // Offset of psxRegs.cycle in psxRegs
    : // Clobber - No need to list anything but 'saved' regs
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "fp", "ra", "memory"
  );
}
#endif

/* Execute blocks starting at psxRegs.pc */
static void recExecute()
{
#ifndef ASM_EXECUTE_LOOP
  for (;;)
  {
    u32 *p = (u32*)PC_REC(psxRegs.pc);
    if (*p == 0)
      recRecompile();

    recFunc((void *)*p);

    if (psxRegs.cycle >= psxRegs.io_cycle_counter)
      psxBranchTest();
  }
#else
  __asm__ __volatile__ (
// NOTE: <BD> indicates an instruction in a branch-delay slot
    ".set noreorder                               \n"

// $fp/$s8 remains set to &psxRegs across all calls to blocks
    "la    $fp, %[psxRegs]                        \n"

// Set up our own stack frame. Should have 8-byte alignment, and have 16 bytes
// empty at 0($sp) for use by functions called from within recompiled code.
    ".equ  frame_size,                  24        \n"
    ".equ  f_off_temp_var1,             20        \n"
    ".equ  f_off_block_ret_addr,        16        \n" // NOTE: blocks assume this is at 16($sp)!
    "addiu $sp, $sp, -frame_size                  \n"

// Store const block return address at fixed location in stack frame
    "la    $t0, loop%=                            \n"
    "sw    $t0, f_off_block_ret_addr($sp)         \n"

// Load $v0 once with psxRegs.pc, blocks will assign new value when returning
    "lw    $v0, %[psxRegs_pc_off]($fp)            \n"

// Load $v1 once with zero. It is the # of cycles psxRegs.cycle should be
// incremented by when a block returns.
    "move  $v1, $0                                \n"

// Align loop on cache-line boundary
    ".balign 32, 0, 31                            \n"

////////////////////////////
//       LOOP CODE:       //
////////////////////////////

// NOTE: Loop expects following values to be set:
// $v0 = new value for psxRegs.pc
// $v1 = # of cycles to increment psxRegs.cycle by

// The loop pseudocode is this, interleaving ops to reduce load stalls:
//
// loop:
// $t2 = psxRecLUT[pxsRegs.pc >> 16] + (psxRegs.pc & 0xffff)
// $t0 = *($t2)
// psxRegs.cycle += $v1
// if (psxRegs.cycle >= psxRegs.io_cycle_counter)
//    goto call_psxBranchTest;
// psxRegs.pc = $v0
// if ($t0 == 0)
//    goto recompile_block;
// goto $t0;
// /* Code at addr $t0 will run and return having set $v0 to new psxRegs.pc */
// /*  value and $v1 to the number of cycles to increment psxRegs.cycle by. */
// goto loop;

// Infinite loop, blocks return here
    "loop%=:                                      \n"
    "lw    $t3, %[psxRegs_cycle_off]($fp)         \n" // $t3 = psxRegs.cycle
    "lui   $t1, %%hi(%[psxRecLUT])                \n"
    "srl   $t2, $v0, 16                           \n"
    "sll   $t2, $t2, 2                            \n" // sizeof() psxRecLUT[] elements is 4
    "addu  $t1, $t1, $t2                          \n"
    "lw    $t1, %%lo(%[psxRecLUT])($t1)           \n" // $t1 = psxRecLUT[psxRegs.pc >> 16]
    "lw    $t4, %[psxRegs_io_cycle_ctr_off]($fp)  \n" // $t4 = psxRegs.io_cycle_counter
    "addu  $t3, $t3, $v1                          \n" // $t3 = psxRegs.cycle + $v1
    "andi  $t0, $v0, 0xffff                       \n"
    "addu  $t2, $t0, $t1                          \n" // 1-cycle load stall on $t1 here
    "lw    $t0, 0($t2)                            \n" // $t0 now points to beginning of recompiled
    //  block, or is 0 if block needs recompiling

// Must call psxBranchTest() when psxRegs.cycle >= psxRegs.io_cycle_counter
    "sltu  $t4, $t3, $t4                          \n"
    "beqz  $t4, call_psxBranchTest%=              \n"
    "sw    $t3, %[psxRegs_cycle_off]($fp)         \n" // <BD> IMPORTANT: store new psxRegs.cycle val,
    //  whether or not we are branching here

// Recompile block, if necessary
    "recompile_block_check%=:                     \n"
    "beqz  $t0, recompile_block%=                 \n" // 1-cycle load stall on $t0 here
    "sw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> Use BD slot to store new psxRegs.pc val

// Execute already-compiled block. It will return at top of loop.
    "execute_block%=:                             \n"
    "jr    $t0                                    \n"
    "nop                                          \n" // <BD>

////////////////////////////
//     NON-LOOP CODE:     //
////////////////////////////

// Recompile block and return to normal codepath.
    "recompile_block%=:                           \n"
    "jal   %[recRecompile]                        \n"
    "sw    $t2, f_off_temp_var1($sp)              \n" // <BD> Save block ptr across call
    "lw    $t2, f_off_temp_var1($sp)              \n" // Restore block ptr upon return
    "b     execute_block%=                        \n" // Resume normal code path, but first we must..
    "lw    $t0, 0($t2)                            \n" // <BD> ..load $t0 with ptr to block code

// Call psxBranchTest() and go back to top of loop
    "call_psxBranchTest%=:                        \n"
    "jal   %[psxBranchTest]                       \n"
    "sw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> Use BD slot to store new psxRegs.pc val,
    //  as psxBranchTest() might issue an exception.
    "lw    $v0, %[psxRegs_pc_off]($fp)            \n" // After psxBranchTest() returns, load psxRegs.pc
    //  back into $v0, which could be different than
    //  before the call if an exception was issued.
    "b     loop%=                                 \n" // Go back to top to process psxRegs.pc again..
    "move  $v1, $0                                \n" // <BD> ..using BD slot to set $v1 to 0, since
    //  psxRegs.cycle shouldn't be incremented again.

// Destroy stack frame, exiting inlined ASM block
// NOTE: We'd never reach this point because the block dispatch loop is
//  currently infinite. This could change in the future.
// TODO: Could add a way to reset a game or load a new game from within
//  the running emulator by setting a global boolean, resetting
//  psxRegs.io_cycle_counter to 0, and checking if it's been set before
//  calling pxsBranchTest() here. If set, you must jump here to
//  exit the loop, to ensure that stack frame is adjusted before return!
    "exit%=:                                      \n"
    "addiu $sp, $sp, frame_size                   \n"
    ".set reorder                                 \n"

    : // Output
    : // Input
    [psxRegs]                    "i" (&psxRegs),
    [psxRegs_pc_off]             "i" (off(pc)),
    [psxRegs_cycle_off]          "i" (off(cycle)),
    [psxRegs_io_cycle_ctr_off]   "i" (off(io_cycle_counter)),
    [psxRecLUT]                  "i" (&psxRecLUT),
    [recRecompile]               "i" (&recRecompile),
    [psxBranchTest]              "i" (&psxBranchTest)
    : // Clobber - No need to list anything but 'saved' regs
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "fp", "ra", "memory"
  );
#endif
}

/* Execute blocks starting at psxRegs.pc until target_pc is reached */
static void recExecuteBlock(unsigned target_pc)
{
#ifndef ASM_EXECUTE_LOOP
  do
  {
    u32 *p = (u32*)PC_REC(psxRegs.pc);
    if (*p == 0)
      recRecompile();

    recFunc((void *)*p);

    if (psxRegs.cycle >= psxRegs.io_cycle_counter)
      psxBranchTest();
  }
  while (psxRegs.pc != target_pc);
#else
  __asm__ __volatile__ (
// NOTE: <BD> indicates an instruction in a branch-delay slot
    ".set noreorder                               \n"

// $fp/$s8 remains set to &psxRegs across all calls to blocks
    "la     $fp, %[psxRegs]                       \n"

// Set up our own stack frame. Should have 8-byte alignment, and have 16 bytes
// empty at 0($sp) for use by functions called from within recompiled code.
    ".equ  frame_size,                  32        \n"
    ".equ  f_off_target_pc,             24        \n"
    ".equ  f_off_temp_var1,             20        \n"
    ".equ  f_off_block_ret_addr,        16        \n" // NOTE: blocks assume this is at 16($sp)!
    "addiu $sp, $sp, -frame_size                  \n"

// Store const copy of 'target_pc' parameter in stack frame
    "sw    %[target_pc], f_off_target_pc($sp)     \n"

// Store const block return address at fixed location in stack frame
    "la    $t0, return_from_block%=               \n"
    "sw    $t0, f_off_block_ret_addr($sp)         \n"

// Load $v0 once with psxRegs.pc, blocks will assign new value when returning
    "lw    $v0, %[psxRegs_pc_off]($fp)            \n"

// Load $v1 once with zero. It is the # of cycles psxRegs.cycle should be
// incremented by when a block returns.
    "move  $v1, $0                                \n"

// Align loop on cache-line boundary
    ".balign 32, 0, 31                            \n"

////////////////////////////
//       LOOP CODE:       //
////////////////////////////

// NOTE: Loop expects following values to be set:
// $v0 = new value for psxRegs.pc
// $v1 = # of cycles to increment psxRegs.cycle by

// The loop pseudocode is this, interleaving ops to reduce load stalls:
//
// loop:
// $t2 = psxRecLUT[pxsRegs.pc >> 16] + (psxRegs.pc & 0xffff)
// $t0 = *($t2)
// psxRegs.cycle += $v1
// if (psxRegs.cycle >= psxRegs.io_cycle_counter)
//    goto call_psxBranchTest;
// if ($t0 == 0)
//    goto recompile_block;
// goto $t0;
// /* Code at addr $t0 will run and return having set $v0 to new psxRegs.pc */
// /*  value and $v1 to the number of cycles to increment psxRegs.cycle by. */
// psxRegs.pc = $v0
// if (psxRegs.pc == target_pc)
//    goto exit;
// else
//    goto loop;

// Loop until psxRegs.pc == target_pc
    "loop%=:                                      \n"
    "lw    $t3, %[psxRegs_cycle_off]($fp)         \n" // $t3 = psxRegs.cycle
    "lui   $t1, %%hi(%[psxRecLUT])                \n"
    "srl   $t2, $v0, 16                           \n"
    "sll   $t2, $t2, 2                            \n" // sizeof() psxRecLUT[] elements is 4
    "addu  $t1, $t1, $t2                          \n"
    "lw    $t1, %%lo(%[psxRecLUT])($t1)           \n" // $t1 = psxRecLUT[psxRegs.pc >> 16]
    "lw    $t4, %[psxRegs_io_cycle_ctr_off]($fp)  \n" // $t4 = psxRegs.io_cycle_counter
    "addu  $t3, $t3, $v1                          \n" // $t3 = new psxRegs.cycle val
    "andi  $t0, $v0, 0xffff                       \n"
    "addu  $t2, $t0, $t1                          \n" // 1-cycle load stall on $t1 here
    "lw    $t0, 0($t2)                            \n" // $t0 now points to beginning of recompiled
    //  block, or is 0 if block needs compiling

// Must call psxBranchTest() when psxRegs.cycle >= psxRegs.io_cycle_counter
    "sltu  $t4, $t3, $t4                          \n"
    "beqz  $t4, call_psxBranchTest%=              \n"
    "sw    $t3, %[psxRegs_cycle_off]($fp)         \n" // <BD> IMPORTANT: store new psxRegs.cycle val,
    //  whether or not we are branching here

// Recompile block, if necessary
    "recompile_block_check%=:                     \n"
    "beqz  $t0, recompile_block%=                 \n" // 1-cycle load stall on $t0 here
    "nop                                          \n" // <BD>

// Execute already-compiled block.
    "execute_block%=:                             \n"
    "jr    $t0                                    \n"
    "nop                                          \n" // <BD>

    "return_from_block%=:                         \n"
// Return point for all executed blocks, which will have set:
// $v0 to new value for psxRegs.pc
// $v1 to the # of cycles to increment psxRegs.cycle by

// Check if target_pc has been reached, looping if not
    "lw    $t0, f_off_target_pc($sp)              \n" // $t0 = target_pc
    "bne   $v0, $t0, loop%=                       \n" // Loop if target_pc hasn't been reached, using..
    "sw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> ..BD slot to store new psxRegs.pc val

// Before cleanup/exit, ensure psxRegs.cycle is incremented by block's $v1 retval
    "lw    $t3, %[psxRegs_cycle_off]($fp)         \n" // $t3 = psxRegs.cycle
    "addu  $t3, $t3, $v1                          \n"
    "b     exit%=                                 \n" // Goto cleanup/exit code, using BD slot..
    "sw    $t3, %[psxRegs_cycle_off]($fp)         \n" // <BD> ..to store new psxRegs.cycle val

////////////////////////////
//     NON-LOOP CODE:     //
////////////////////////////

// Recompile block and return to normal codepath
    "recompile_block%=:                           \n"
    "jal   %[recRecompile]                        \n"
    "sw    $t2, f_off_temp_var1($sp)              \n" // <BD> Save block ptr across call
    "lw    $t2, f_off_temp_var1($sp)              \n" // Restore block ptr upon return
    "b     execute_block%=                        \n" // Resume normal code path, but first we must..
    "lw    $t0, 0($t2)                            \n" // <BD> ..load $t0 with ptr to block code

// Call psxBranchTest() and go back to top of loop
    "call_psxBranchTest%=:                        \n"
    "jal   %[psxBranchTest]                       \n"
    "nop                                          \n" // <BD>
    "lw    $v0, %[psxRegs_pc_off]($fp)            \n" // After psxBranchTest() returns, load psxRegs.pc
    //  back into $v0, which could be different than
    //  before the call if an exception was issued.
    "b     loop%=                                 \n" // Go back to top to process psxRegs.pc again..
    "move  $v1, $0                                \n" // <BD> ..using BD slot to set $v1 to 0, since
    //  psxRegs.cycle shouldn't be incremented again.

// Destroy stack frame, exiting inlined ASM block
    "exit%=:                                      \n"
    "addiu $sp, $sp, frame_size                   \n"
    ".set reorder                                 \n"

    : // Output
    : // Input
    [target_pc]                  "r" (target_pc),
    [psxRegs]                    "i" (&psxRegs),
    [psxRegs_pc_off]             "i" (off(pc)),
    [psxRegs_cycle_off]          "i" (off(cycle)),
    [psxRegs_io_cycle_ctr_off]   "i" (off(io_cycle_counter)),
    [psxRecLUT]                  "i" (&psxRecLUT),
    [recRecompile]               "i" (&recRecompile),
    [psxBranchTest]              "i" (&psxBranchTest)
    : // Clobber - No need to list anything but 'saved' regs
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "fp", "ra", "memory"
  );
#endif
}

static void recClear(u32 Addr, u32 Size)
{
  memset((u32*)PC_REC(Addr), 0, (Size * 4));

  if (Addr == 0x8003d000)
  {
    // temp fix for Buster Bros Collection and etc.
    memset(recRAM+0x4d88, 0, 0x8);
  }
}

static void recReset()
{
  memset(recRAM, 0, 0x200000);
  memset(recROM, 0, 0x080000);

  recMem = (u32*)recMemBase;

  regReset();

  branch = 0;
  end_block = 0;
}

R3000Acpu psxRec =
{
  recInit,
  recReset,
  recExecute,
  recExecuteBlock,
  recClear,
  recShutdown
};
