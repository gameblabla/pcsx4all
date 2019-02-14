/*
 * Mips-to-mips recompiler for pcsx4all
 *
 * Copyright (c) 2009 Ulrich Hecht
 * Copyright (c) 2017 modified by Dmitry Smagin, Daniel Silsby
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

#include <stddef.h>
#include "plugin_lib.h"
#include "psxcommon.h"
#include "psxhle.h"
#include "psxmem.h"
#include "psxhw.h"
#include "r3000a.h"
#include "gte.h"

/* For direct HW I/O */
#include "mdec.h"
#include "cdrom.h"
#include "gpu.h"

/* Standard console logging */
#define REC_LOG(...) printf("mipsrec: " __VA_ARGS__)
#ifndef REC_LOG
#define REC_LOG(...)
#endif

/* Verbose console logging (uncomment next line to enable) */
//#define REC_LOG_V REC_LOG
#ifndef REC_LOG_V
#define REC_LOG_V(...)
#endif

/* Use inlined-asm version of block dispatcher: */
#define ASM_EXECUTE_LOOP

/* Scan for and skip useless code in PS1 executable: */
#define USE_CODE_DISCARD

/* If HLE emulated BIOS is not in use, blocks return to dispatch loop directly */
#define USE_DIRECT_BLOCK_RETURN_JUMPS

/* If a block jumps backwards to the top of itself, use fast dispatch path.
 *  Every block's recompiled start address is saved before entry. To
 *  return to the dispatch loop, it jumps to a shorter version of dispatch
 *  loop that skips code table lookups and code invalidation checks.
 * This could *theoretically* cause a problem  with poorly-behaved
 *  self-modifying code, but so far no issues have been found.
 * NOTE: Option only has effect if USE_DIRECT_BLOCK_RETURN_JUMPS is enabled,
 *  which itself only has effect when HLE emulated BIOS is not in use.
 */
#define USE_DIRECT_FASTPATH_BLOCK_RETURN_JUMPS

/* Const propagation is applied to addresses */
#define USE_CONST_ADDRESSES

/* Const propagation is extended to optimize 'fuzzy' non-const addresses */
#define USE_CONST_FUZZY_ADDRESSES

/* Generate inline memory access or call psxMemRead/Write C functions */
#define USE_DIRECT_MEM_ACCESS

/* Virtual memory mapping options: */
#if defined(SHMEM_MIRRORING) || defined(TMPFS_MIRRORING)
	/* 2MB of PSX RAM (psxM) is now mapped+mirrored virtually, much like
	 *  a real PS1. We can skip mirror-region checks, the 'Einhander' game
	 *  fix. We can skip masking RAM addresses. We also map 0x1fxx_xxxx
	 *  regions (psxP,psxH) into this space, inlining scratchpad accesses.
	 *
	 * IMPORTANT: Don't enable if 'USE_DIRECT_MEM_ACCESS' isn't also enabled.
	 *            There'd be no benefit to a virtual mapping: all mem access
	 *            would be through indirect psxMemWrite/psxMemRead C funcs.
	 *            Even worse, we support a mapping starting at address 0, and
	 *            that is not compatible with psxMemWrite/psxMemRead funcs
	 *            because of how they handle NULL addresses in their LUTs.
	 */
	#ifdef USE_DIRECT_MEM_ACCESS
		#define USE_VIRTUAL_PSXMEM_MAPPING
	#else
		#warning "USE_DIRECT_MEM_ACCESS is undefined! Dynarec will emit slower C memory accesses."
	#endif

	/* Prefer virtually mapped/mirrored code block pointer array over using
	 *  psxRecLUT[]. This removes a layer of indirection from PC->block-ptr lookups,
	 *  allows faster block dispatch loops, and reduces cache/TLB pressure.
	 */
	#define USE_VIRTUAL_RECRAM_MAPPING

#else
	#warning "Neither SHMEM_MIRRORING nor TMPFS_MIRRORING are defined! Dynarec will use slower block dispatch loop and emit slower memory accesses. Check your Makefile!"
#endif // defined(SHMEM_MIRRORING) || defined(TMPFS_MIRRORING)

//#define WITH_DISASM
//#define DEBUGG printf

#include "mem_mapping.h"

/* Bit vector indicating which PS1 RAM pages contain the start of blocks.
 *  Used to determine when code invalidation in recClear() can be skipped.
 */
static u8 code_pages[0x200000/4096/8];

/* Pointers to the recompiled blocks go here. psxRecLUT[] uses upper 16 bits of
 *  a PC value as an index to lookup a block pointer stored in recRAM/recROM.
 */
static s8 *recRAM;
static s8 *recROM;
static uptr psxRecLUT[0x10000];

#undef PC_REC
#undef PC_REC8
#undef PC_REC16
#undef PC_REC32
#define PC_REC(x)	((uptr)psxRecLUT[(x) >> 16] + (((x) & 0xffff) * (REC_RAM_PTR_SIZE / 4)))
#define PC_REC8(x)	(*(u8 *)PC_REC(x))
#define PC_REC16(x)	(*(u16*)PC_REC(x))
#define PC_REC32(x)	(*(u32*)PC_REC(x))
/* Version of PC_REC() that uses faster virtual block ptr mapping */
#define PC_REC_MMAP(x)	(REC_RAM_VADDR | (((x) & 0x00ffffff) * (REC_RAM_PTR_SIZE / 4)))

#include "mips_codegen.h"
#include "disasm.h"
#include "host_asm.h"


/* Const-propagation data and functions */
typedef struct {
	u32  constval;
	uint8_t is_const;

	uint8_t is_fuzzy_ram_addr;        /* GPR is not known-const, but at least known
	                                   to be address somewhere in RAM? */
	uint8_t is_fuzzy_nonram_addr;     /* GPR is not known-const, but at least known
	                                   to be address somewhere outside RAM? */
	uint8_t is_fuzzy_scratchpad_addr; /* GPR is not known-const, but at least known
	                                   to be address somewhere in 1KB scratcpad? */
} iRegisters;
static iRegisters iRegs[32];
static inline void ResetConsts()
{
	memset(&iRegs, 0, sizeof(iRegs));
	iRegs[0].is_const = 1;  // $r0 is always zero val
}
static inline uint8_t IsConst(const u32 reg)  { return iRegs[reg].is_const; }
static inline u32  GetConst(const u32 reg) { return iRegs[reg].constval; }
static inline void SetUndef(const u32 reg)
{
	if (reg) {
		iRegs[reg].is_const = 0;
		iRegs[reg].is_fuzzy_ram_addr        = 0;
		iRegs[reg].is_fuzzy_nonram_addr     = 0;
		iRegs[reg].is_fuzzy_scratchpad_addr = 0;
	}
}
static inline void SetConst(const u32 reg, const u32 val)
{
	if (reg) {
		iRegs[reg].constval = val;
		iRegs[reg].is_const = 1;
		iRegs[reg].is_fuzzy_ram_addr        = 0;
		iRegs[reg].is_fuzzy_nonram_addr     = 0;
		iRegs[reg].is_fuzzy_scratchpad_addr = 0;
	}
}
static inline void SetFuzzyRamAddr(const u32 reg)        { iRegs[reg].is_fuzzy_ram_addr = 1; }
static inline uint8_t IsFuzzyRamAddr(const u32 reg)         { return iRegs[reg].is_fuzzy_ram_addr; }
static inline void SetFuzzyNonramAddr(const u32 reg)     { iRegs[reg].is_fuzzy_nonram_addr = 1; }
static inline uint8_t IsFuzzyNonramAddr(const u32 reg)      { return iRegs[reg].is_fuzzy_nonram_addr; }
static inline void SetFuzzyScratchpadAddr(const u32 reg) { iRegs[reg].is_fuzzy_scratchpad_addr = 1; }
static inline uint8_t IsFuzzyScratchpadAddr(const u32 reg)  { return iRegs[reg].is_fuzzy_scratchpad_addr; }


/* Code cache buffer
 *  Keep this statically allocated! This keeps it close to the
 *  .text section, so C code and recompiled code lie in the same 256MiB region.
 *  MIPS JAL/J opcodes in recompiled code that jump to C code require this.
 *  Dynamic allocation would get an anonymous mmap'ing, locating the recompiled
 *  code *far* too high in virtual address space.
 */
#define RECMEM_SIZE         (12 * 1024 * 1024)
#define RECMEM_SIZE_MAX     (RECMEM_SIZE-(512*1024))
static u8 recMemBase[RECMEM_SIZE] __attribute__((aligned(4)));

u32        *recMem;                /* Where does next emitted opcode in block go? */
static u32 *recMemStart;           /* Where did first emitted opcode in block go? */
static u32 pc;                     /* Recompiler pc */
static u32 oldpc;                  /* Recompiler pc at start of block */
u32 cycle_multiplier = 0x200;      /* Cycle advance per emulated instruction
                                      Default is 0x200 == 2.00 (24.8 fixed-pt) */

/* See comments in recExecute() regarding direct block returns */
static uptr block_ret_addr;                /* Non-zero when blocks are using direct return jumps */
static uptr block_fast_ret_addr;           /* Non-zero when blocks are using direct return jumps &
                                              dispatch loop fastpath is enabled */

static uint8_t psx_mem_mapped;                /* PS1 RAM mmap'd+mirrored at fixed address? (psxM) */
static uint8_t rec_mem_mapped;                /* Code ptr arrays mmap'd+mirrored at fixed address? (recRAM,recROM) */

/* Flags used during a recompilation phase */
static uint8_t branch;                        /* Current instruction lies in a BD slot? */
static uint8_t end_block;                     /* Has recompilation phase ended? */
static uint8_t skip_emitting_next_mflo;       /* Was a MULT/MULTU converted to 3-op MUL? See rec_mdu.cpp.h */
static uint8_t emit_code_invalidations;       /* Emit code invalidation for store instructions? */
static uint8_t flush_code_on_dma3_exe_load;   /* Flush code cache when psxDma3() detects EXE load? */

/* Flags/vals used to cache common values in temp regs in emitted code */
static uint8_t lsu_tmp_cache_valid;           /* LSU vals are cached in $at,$v1. See rec_lsu.cpp.h */
static uint8_t host_v0_reg_is_const;          /* PCs are cached in $v0. See rec_bcu.cpp.h */
static u32  host_v0_reg_constval;
static uint8_t host_ra_reg_has_block_retaddr; /* Indirect-return address is cached in $ra. */


#ifdef WITH_DISASM
char	disasm_buffer[512];
#endif

#include "regcache.h"

static void recReset();
static void recRecompile();
static void recClear(u32 Addr, u32 Size);
static void recNotify(int note, void *data);

extern void (*recBSC[64])();
extern void (*recSPC[64])();
extern void (*recREG[32])();
extern void (*recCP0[32])();
extern void (*recCP2[64])();
extern void (*recCP2BSC[32])();


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
  // Direct HW I/O:
  make_stub_label(cdrRead0),
  make_stub_label(cdrRead1),
  make_stub_label(cdrRead2),
  make_stub_label(cdrRead3),
  make_stub_label(cdrWrite0),
  make_stub_label(cdrWrite1),
  make_stub_label(cdrWrite2),
  make_stub_label(cdrWrite3),
  make_stub_label(GPU_writeData),
  make_stub_label(mdecRead0),
  make_stub_label(mdecRead1),
  make_stub_label(mdecWrite0),
  make_stub_label(mdecWrite1),
  make_stub_label(psxRcntRcount),
  make_stub_label(psxRcntRmode),
  make_stub_label(psxRcntRtarget),
  make_stub_label(psxRcntWcount),
  make_stub_label(psxRcntWmode),
  make_stub_label(psxRcntWtarget),
  make_stub_label(sioRead8),
  make_stub_label(sioRead16),
  make_stub_label(sioRead32),
  make_stub_label(sioReadBaud16),
  make_stub_label(sioReadCtrl16),
  make_stub_label(sioReadMode16),
  make_stub_label(sioReadStat16),
  make_stub_label(sioWrite8),
  make_stub_label(sioWrite16),
  make_stub_label(sioWrite32),
  make_stub_label(sioWriteBaud16),
  make_stub_label(sioWriteCtrl16),
  make_stub_label(sioWriteMode16),
  make_stub_label(SPU_writeRegister),
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

#define DISASM_MSG printf

#else

#define DISASM_PSX(_PC_)
#define DISASM_HOST()
#define DISASM_INIT()
#define DISASM_MSG(...)

#endif


#include "opcodes.h"

#ifndef HAVE_MIPS32R2_CACHE_OPS
#include <sys/cachectl.h>
#endif

static inline void clear_insn_cache(void *start, void *end, int flags)
{
#ifdef HAVE_MIPS32R2_CACHE_OPS
	// MIPS32r2 added fine-grained usermode cache flush ability (yes, please!)
	MIPS32R2_MakeCodeVisible(start, (char *)end - (char *)start);
#else
	// Use Linux system call (ends up flushing entire cache)
	#ifdef DYNAREC_SKIP_DCACHE_FLUSH
		// Faster, but only works if host's ICACHE pulls from DCACHE, not RAM
		int cache_to_flush = ICACHE;
	#else
		// Slower, but most compatible (flush both caches)
		int cache_to_flush = BCACHE; // ICACHE|DCACHE
	#endif

	cacheflush(start, (char *)end - (char *)start, cache_to_flush);
#endif
}


/* Set default recompilation options, and any per-game settings */
static void rec_set_options()
{
	// Default options
	emit_code_invalidations = 1;
	flush_code_on_dma3_exe_load = 0;

	// Per-game options
	// -> Use case-insensitive comparisons! Some CDs have lowercase CdromId.

	// 'Studio 33' game workarounds (other Studio 33 games seem to be OK)
	//  See comments in recNotify(), psxDma3().
	if (strncasecmp(CdromId, "SCES03886", 9) == 0  ||  // Formula 1 Arcade
	    strncasecmp(CdromId, "SLUS00870", 9) == 0  ||  // Formula 1 '99  NTSC US
	    strncasecmp(CdromId, "SCPS10101", 9) == 0  ||  // Formula 1 '99  NTSC J (untested)
	    strncasecmp(CdromId, "SCES01979", 9) == 0  ||  // Formula 1 '99  PAL  E (requires .SBI subchannel file)
	    strncasecmp(CdromId, "SLES01979", 9) == 0  ||  // Formula 1 '99  PAL  E (unknown revision, couldn't test)
	    strncasecmp(CdromId, "SCES03404", 9) == 0  ||  // Formula 1 2001 PAL  E,Fi (fixes broken AI/controls)
	    strncasecmp(CdromId, "SCES03423", 9) == 0)     // Formula 1 2001 PAL  Fr,G (fixes broken AI/controls)
	{
		REC_LOG("Using Icache workarounds for trouble games 'Formula One 99/2001/etc'.\n");
		emit_code_invalidations = 0;
		flush_code_on_dma3_exe_load = 1;
	}
}


static void recRecompile()
{
	// Notify plugin_lib that we're recompiling (affects frameskip timing)
	pl_dynarec_notify();

	if (((uptr)recMem - (uptr)recMemBase) >= RECMEM_SIZE_MAX ) {
		REC_LOG("Code cache size limit exceeded: flushing code cache.\n");
		recReset();
	}

	recMemStart = recMem;

	regReset();

	PC_REC32(psxRegs.pc) = (u32)recMem;
	oldpc = pc = psxRegs.pc;

	// If 'pc' is in PS1 RAM, mark the page of RAM as containing the start of
	//  a block. For the range check, bit 27 is interpreted as a sign bit.
	if ((s32)(pc << 4) >= 0) {
		u32 masked_pc = pc & 0x1fffff;
		code_pages[masked_pc/4096/8] |= (1 << ((masked_pc/4096) & 7));
	}

	DISASM_INIT();

	rec_recompile_start();

	// Reset const-propagation
	ResetConsts();

	// Flag indicates when recompilation should stop
	end_block = 0;

	// See convertMultiplyTo3Op() and recMFLO() in rec_mdu.cpp.h
	skip_emitting_next_mflo = 0;

	// Flag indicates when values are cached by load/store emitters in $at,$v1
	lsu_tmp_cache_valid = 0;

	// Flag indicates when a PC value is cached in $v0. All dispatch loops set
	//  $v0 to block start PC before entry. See rec_bcu.cpp.h
	host_v0_reg_is_const = 1;
	host_v0_reg_constval = psxRegs.pc;

	// Flag indicates when $ra holds block return address. This is only used
	//  by blocks returning indirectly. The indirect-return dispatch loops
	//  set $ra before block entry. See rec_recompile_end_part1().
	host_ra_reg_has_block_retaddr = (block_ret_addr == 0);

	// Number of discardable instructions we are currently skipping
	int discard_cnt = 0;

	do {
		// Flag indicates if next instruction lies in a BD slot
		branch = 0;

		psxRegs.code = OPCODE_AT(pc);

#ifdef USE_CODE_DISCARD
		// If we are not already skipping past discardable code, scan
		//  for PS1 code sequence we can discard.
		if (discard_cnt == 0) {
			int discard_type = 0;
			discard_cnt = rec_discard_scan(pc, &discard_type);
			if (discard_cnt > 0)
				DISASM_MSG(" ->BEGIN code discard: %s\n", rec_discard_type_str(discard_type));
		}
#endif

		DISASM_PSX(pc);
		pc += 4;

#ifdef USE_CODE_DISCARD
		// Skip next discardable instruction in any sequence found.
		if (discard_cnt > 0) {
			--discard_cnt;
			if (discard_cnt == 0)
				DISASM_MSG(" ->END code discard.\n");
			continue;
		}
#endif

		// Recompile next instruction.
		recBSC[psxRegs.code>>26]();
		regUpdate();
	} while (!end_block);

	DISASM_HOST();
	clear_insn_cache(recMemStart, recMem, 0);
}


static int recInit()
{
	REC_LOG("Initializing\n");

	recMem = (u32*)recMemBase;

	// Init code buffer, to allocate the RAM we need in advance. Filling with
	//  all-1's should force an exception on any accidental non-code execution.
	{
		size_t fill_size = RECMEM_SIZE_MAX + 0x1000;  // 4KB past 'maximum' we'd ever use.
		if (fill_size > RECMEM_SIZE)
			fill_size = RECMEM_SIZE;
		memset(recMemBase, 0xff, fill_size);
	}

	// The tables recRAM and recROM hold block code pointers for all valid PC
	//  values for a PS1 program, after masking away banking and/or mirroring.
	//  Originally, the dynarec used a LUT, psxRecLUT[], to accomplish this
	//  masking, which added a second layer of indirection to code lookups.
	//  Instead, we now virtually map/mirror recRAM and map recROM.
	//  This offloads the indirection onto the host's TLB.
	//  NOTE: psxRecLUT[] still is used inside recExecuteBlock(): no sense in
	//        making two versions of a function that hardly gets any real use.
#ifdef USE_VIRTUAL_RECRAM_MAPPING
	if (!rec_mem_mapped && !recRAM && !recROM) {
		if (rec_mmap_rec_mem() >= 0) {
			rec_mem_mapped = 1;
			recRAM = (s8*)REC_RAM_VADDR;
			recROM = (s8*)REC_ROM_VADDR;
		}
	}
#endif

	if (!rec_mem_mapped) {
		// Oops, somehow the virtual mapping/mirroring failed. We'll allocate
		//  recRAM/recROM here and use slower versions of recExecute*() that
		//  use psxRecLUT[] to indirectly access recRAM/recROM.
		recRAM = (s8*)malloc(REC_RAM_SIZE);
		recROM = (s8*)malloc(REC_ROM_SIZE);
		printf("WARNING: Recompiler is using slower non-virtual block ptr lookups.\n");
	}

	recReset();

	if (recRAM == NULL || recROM == NULL || recMemBase == NULL || psxRecLUT == NULL) {
		printf("Error allocating memory\n"); return -1;
	}

	for (int i = 0; i < 0x80; i++)
		psxRecLUT[i + 0x0000] = (uptr)recRAM + (((i & 0x1f) << 16) * (REC_RAM_PTR_SIZE/4));

	memcpy(&psxRecLUT[0x8000], psxRecLUT, 0x80 * sizeof(psxRecLUT[0]));
	memcpy(&psxRecLUT[0xa000], psxRecLUT, 0x80 * sizeof(psxRecLUT[0]));

	for (int i = 0; i < 0x08; i++)
		psxRecLUT[i + 0xbfc0] = (uptr)recROM + ((i << 16) * (REC_RAM_PTR_SIZE/4));

	// Map/mirror PSX RAM, other regions, i.e. psxM, psxP, psxH
	// NOTE: if mapping fails or isn't enabled at compile-time, PSX mem will be
	//       allocated using traditional methods in psxmem.cpp
#ifdef USE_VIRTUAL_PSXMEM_MAPPING
	if (!psx_mem_mapped)
		psx_mem_mapped = (rec_mmap_psx_mem() >= 0);
#endif

	if (!psx_mem_mapped)
		printf("WARNING: Recompiler is emitting slower non-virtual mem access code.\n");

	return 0;
}


static void recShutdown()
{
	REC_LOG("Shutting down\n");

	if (psx_mem_mapped)
		rec_munmap_psx_mem();
	if (rec_mem_mapped)
		rec_munmap_rec_mem();
	psx_mem_mapped = rec_mem_mapped = 0;
}


/* It seems there's no way to tell GCC that something is being called inside
 * asm() blocks and GCC doesn't bother to save temporaries to stack.
 * That's why we have two options:
 * 1. Call recompiled blocks via recFunc() trap which is strictly noinline and
 * saves registers $s[0-7], $fp and $ra on each call, or
 * 2. Code recExecute() and recExecuteBlock() entirely in assembler taking into
 * account that no registers except $ra are saved in recompiled blocks and
 * thus put all temporaries to stack. In this case $s[0-7], $fp and $ra are saved
 * in recExecute() and recExecuteBlock() only once.
 *
 * IMPORTANT: Functions containing inline ASM should have attribute 'noinline'.
 *            Crashes at callsites can occur otherwise, at least with GCC 4.xx.
 */
#ifndef ASM_EXECUTE_LOOP
__attribute__((noinline)) static void recFunc(void *fn)
{
	/* This magic code calls fn address saving registers $s[0-7], $fp and $ra. */
	/*                                                                         */
	/* Focus here is on clarity, not speed. This is the bare minimum needed to */
	/*  call blocks from within C code, which is:                              */
	/* Blocks expect $fp to be set to &psxRegs.                                */
	/* Blocks expect return address to be stored at 16($sp) and in $ra.        */
	/* Blocks expect $v0 to contain psxRegs.pc value (helps addr generation).  */
	/* Stack should have 16 bytes free at 0($sp) for use by called functions.  */
	/* Stack should be 8-byte aligned to satisfy MIPS ABI.                     */
	/*                                                                         */
	/* Blocks return these values, which are handled here:                     */
	/*  $v0 is new value for psxRegs.pc                                        */
	/*  $v1 is number of cycles to increment psxRegs.cycle by                  */
	__asm__ __volatile__ (
		"addiu  $sp, $sp, -24                   \n"
		"la     $fp, %[psxRegs]                 \n" // $fp = &psxRegs
		"lw     $v0, %[psxRegs_pc_off]($fp)     \n" // Blocks expect $v0 to contain PC val on entry
		"la     $ra, block_return_addr%=        \n" // Load $ra with block_return_addr
		"sw     $ra, 16($sp)                    \n" // Put 'block_return_addr' on stack
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


/* Execute blocks starting at psxRegs.pc
 * Blocks return indirectly, to address stored at 16($sp)
 * Block pointers are looked up using psxRecLUT[].
 * Called only from recExecute(), see notes there.
 *
 * IMPORTANT: Functions containing inline ASM should have attribute 'noinline'.
 *            Crashes at callsites can occur otherwise, at least with GCC 4.xx.
 */
__attribute__((noinline)) void recExecute_indirect_return_lut()
{
	// Set block_ret_addr to 0, so generated code uses indirect returns
	block_ret_addr = block_fast_ret_addr = 0;

#ifndef ASM_EXECUTE_LOOP
	for (;;) {
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
".set push                                    \n"
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
".balign 32                                   \n"

////////////////////////////
//       LOOP CODE:       //
////////////////////////////

// NOTE: Blocks return to top of loop.
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
// $ra = block return address
// goto $t0;
// /* Code at addr $t0 will run and return having set $v0 to new psxRegs.pc */
// /*  value and $v1 to the number of cycles to increment psxRegs.cycle by. */

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
"addu  $t2, $t0, $t1                          \n"
"lw    $t0, 0($t2)                            \n" // $t0 = address of start of block code, or
                                                  //       or 0 if block needs recompilation
                                                  // IMPORTANT: leave block ptr in $t2, it gets
                                                  // saved & re-used if recRecompile() is called.


// Must call psxBranchTest() when psxRegs.cycle >= psxRegs.io_cycle_counter
"sltu  $t4, $t3, $t4                          \n"
"beqz  $t4, call_psxBranchTest%=              \n"
"sw    $t3, %[psxRegs_cycle_off]($fp)         \n" // <BD> IMPORTANT: store new psxRegs.cycle val,
                                                  //  whether or not we are branching here

// Recompile block, if necessary
"beqz  $t0, recompile_block%=                 \n"
"sw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> Use BD slot to store new psxRegs.pc val

// Execute already-compiled block. It will return at top of loop.
"execute_block%=:                             \n"
"jr    $t0                                    \n"
"lw    $ra, 16($sp)                           \n" // <BD> Load block return address

////////////////////////////
//     NON-LOOP CODE:     //
////////////////////////////

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

// Recompile block and return to normal codepath.
"recompile_block%=:                           \n"
"jal   %[recRecompile]                        \n"
"sw    $t2, f_off_temp_var1($sp)              \n" // <BD> Save block ptr across call
"lw    $t2, f_off_temp_var1($sp)              \n" // Restore block ptr upon return
"lw    $v0, %[psxRegs_pc_off]($fp)            \n" // Blocks expect $v0 to contain PC val on entry
"b     execute_block%=                        \n" // Resume normal code path, but first we must..
"lw    $t0, 0($t2)                            \n" // <BD> ..load $t0 with ptr to block code

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
".set pop                                     \n"

: // Output
: // Input
  [psxRegs]                    "i" (&psxRegs),
  [psxRegs_pc_off]             "i" (off(pc)),
  [psxRegs_cycle_off]          "i" (off(cycle)),
  [psxRegs_io_cycle_ctr_off]   "i" (off(io_cycle_counter)),
  [recRecompile]               "i" (&recRecompile),
  [psxBranchTest]              "i" (&psxBranchTest),
  [psxRecLUT]                  "i" (psxRecLUT)
: // Clobber - No need to list anything but 'saved' regs
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "fp", "ra", "memory"
);
#endif
}


/* Execute blocks starting at psxRegs.pc
 * Blocks return indirectly, to address stored at 16($sp)
 * Block pointers are looked up using virtual address mapping of recRAM/recROM.
 * Called only from recExecute(), see notes there.
 *
 * IMPORTANT: Functions containing inline ASM should have attribute 'noinline'.
 *            Crashes at callsites can occur otherwise, at least with GCC 4.xx.
 */
__attribute__((noinline)) static void recExecute_indirect_return_mmap()
{
	// Set block_ret_addr to 0, so generated code uses indirect returns
	block_ret_addr = block_fast_ret_addr = 0;

#ifndef ASM_EXECUTE_LOOP
	for (;;) {
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
".set push                                    \n"
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
".balign 32                                   \n"

////////////////////////////
//       LOOP CODE:       //
////////////////////////////

// NOTE: Blocks return to top of loop.
// NOTE: Loop expects following values to be set:
// $v0 = new value for psxRegs.pc
// $v1 = # of cycles to increment psxRegs.cycle by

// The loop pseudocode is this, interleaving ops to reduce load stalls:
//
// loop:
// $t2 = REC_RAM_VADDR | ($v0 & 0x00ffffff)
// $t0 = *($t2)
// psxRegs.cycle += $v1
// if (psxRegs.cycle >= psxRegs.io_cycle_counter)
//    goto call_psxBranchTest;
// psxRegs.pc = $v0
// if ($t0 == 0)
//    goto recompile_block;
// $ra = block return address
// goto $t0;
// /* Code at addr $t0 will run and return having set $v0 to new psxRegs.pc */
// /*  value and $v1 to the number of cycles to increment psxRegs.cycle by. */

// Infinite loop, blocks return here
"loop%=:                                      \n"
"lw    $t3, %[psxRegs_cycle_off]($fp)         \n" // $t3 = psxRegs.cycle
"lw    $t4, %[psxRegs_io_cycle_ctr_off]($fp)  \n" // $t4 = psxRegs.io_cycle_counter

// The block ptrs are mapped virtually to address space allowing lower
//  24 bits of PS1 PC address to lookup start of any RAM or ROM code block.
"lui   $t2, %[REC_RAM_VADDR_UPPER]            \n"
#ifdef HAVE_MIPS32R2_EXT_INS
"ins   $t2, $v0, 0, 24                        \n"
#else
"sll   $t1, $v0, 8                            \n"
"srl   $t1, $t1, 8                            \n"
"or    $t2, $t2, $t1                          \n"
#endif
"lw    $t0, 0($t2)                            \n" // $t0 = address of start of block code, or
                                                  //       or 0 if block needs recompilation
                                                  // IMPORTANT: leave block ptr in $t2, it gets
                                                  // saved & re-used if recRecompile() is called.

"addu  $t3, $t3, $v1                          \n" // $t3 = psxRegs.cycle + $v1

// Must call psxBranchTest() when psxRegs.cycle >= psxRegs.io_cycle_counter
"sltu  $t4, $t3, $t4                          \n"
"beqz  $t4, call_psxBranchTest%=              \n"
"sw    $t3, %[psxRegs_cycle_off]($fp)         \n" // <BD> IMPORTANT: store new psxRegs.cycle val,
                                                  //  whether or not we are branching here

// Recompile block, if necessary
"beqz  $t0, recompile_block%=                 \n"
"sw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> Use BD slot to store new psxRegs.pc val

// Execute already-compiled block. It will return at top of loop.
"execute_block%=:                             \n"
"jr    $t0                                    \n"
"lw    $ra, 16($sp)                           \n" // <BD> Load block return address

////////////////////////////
//     NON-LOOP CODE:     //
////////////////////////////

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

// Recompile block and return to normal codepath.
"recompile_block%=:                           \n"
"jal   %[recRecompile]                        \n"
"sw    $t2, f_off_temp_var1($sp)              \n" // <BD> Save block ptr across call
"lw    $t2, f_off_temp_var1($sp)              \n" // Restore block ptr upon return
"lw    $v0, %[psxRegs_pc_off]($fp)            \n" // Blocks expect $v0 to contain PC val on entry
"b     execute_block%=                        \n" // Resume normal code path, but first we must..
"lw    $t0, 0($t2)                            \n" // <BD> ..load $t0 with ptr to block code

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
".set pop                                     \n"

: // Output
: // Input
  [psxRegs]                    "i" (&psxRegs),
  [psxRegs_pc_off]             "i" (off(pc)),
  [psxRegs_cycle_off]          "i" (off(cycle)),
  [psxRegs_io_cycle_ctr_off]   "i" (off(io_cycle_counter)),
  [recRecompile]               "i" (&recRecompile),
  [psxBranchTest]              "i" (&psxBranchTest),
  [REC_RAM_VADDR_UPPER]        "i" (REC_RAM_VADDR >> 16)
: // Clobber - No need to list anything but 'saved' regs
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "fp", "ra", "memory"
);
#endif
}


/* Execute blocks starting at psxRegs.pc
 * Blocks return directly (not indirectly via stack address var).
 * Block pointers are looked up using psxRecLUT[].
 * Called only from recExecute(), see notes there.

 * IMPORTANT: Functions containing inline ASM should have attribute 'noinline'.
 *            Crashes at callsites can occur otherwise, at least with GCC 4.xx.
 */
__attribute__((noinline)) static void recExecute_direct_return_lut()
{
__asm__ __volatile__ (
// NOTE: <BD> indicates an instruction in a branch-delay slot
".set push                                    \n"
".set noreorder                               \n"

// Set up our own stack frame. Should have 8-byte alignment, and have 16 bytes
// empty at 0($sp) for use by functions called from within recompiled code.
".equ  frame_size,                        32  \n"
".equ  f_off_branchtest_fastpath_ra,      28  \n"
".equ  f_off_temp_var1,                   24  \n"
".equ  f_off_block_start_addr,            16  \n"
"addiu $sp, $sp, -frame_size                  \n"

#ifdef USE_DIRECT_FASTPATH_BLOCK_RETURN_JUMPS
// Set the global var 'block_fast_ret_addr'.
"la    $t0, fastpath_loop%=                   \n"
"lui   $t1, %%hi(%[block_fast_ret_addr])      \n"
"sw    $t0, %%lo(%[block_fast_ret_addr])($t1) \n"

// We need a stack var to hold a return address for a 'fastpath' call to
//  psxBranchTest. This helps reduce size of code through sharing.
"la    $t0, branchtest_fastpath_retaddr%=     \n"
"sw    $t0, f_off_branchtest_fastpath_ra($sp) \n"
#endif

// Set the global var 'block_ret_addr'.
"la    $t0, loop%=                            \n"
"lui   $t1, %%hi(%[block_ret_addr])           \n"
"sw    $t0, %%lo(%[block_ret_addr])($t1)      \n"

// $fp/$s8 remains set to &psxRegs across all calls to blocks
"la    $fp, %[psxRegs]                        \n"

// Load $v0 once with psxRegs.pc, blocks will assign new value when returning
"lw    $v0, %[psxRegs_pc_off]($fp)            \n"

// Load $v1 once with zero. It is the # of cycles psxRegs.cycle should be
// incremented by when a block returns.
"move  $v1, $0                                \n"

// Align loop on cache-line boundary
".balign 32                                   \n"

////////////////////////////
//       LOOP CODE:       //
////////////////////////////

// NOTE: Blocks return to top of loop.
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
// tmp_block_start_addr = $t0
// goto $t0;
// /* Code at addr $t0 will run and return having set $v0 to new psxRegs.pc */
// /*  value and $v1 to the number of cycles to increment psxRegs.cycle by. */
// /* If block branches back to its beginning, it will return to the        */
// /*  fastpath version of loop that skips looking up a code pointer.       */

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
"addu  $t2, $t0, $t1                          \n"
"lw    $t0, 0($t2)                            \n" // $t0 = address of start of block code, or
                                                  //       or 0 if block needs recompilation
                                                  // IMPORTANT: leave block ptr in $t2, it gets
                                                  // saved & re-used if recRecompile() is called.


// Must call psxBranchTest() when psxRegs.cycle >= psxRegs.io_cycle_counter
"sltu  $t4, $t3, $t4                          \n"
"beqz  $t4, call_psxBranchTest%=              \n"
"sw    $t3, %[psxRegs_cycle_off]($fp)         \n" // <BD> IMPORTANT: store new psxRegs.cycle val,
                                                  //  whether or not we are branching here

// Recompile block, if necessary
"beqz  $t0, recompile_block%=                 \n"
"sw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> Use BD slot to store new psxRegs.pc val

// Execute already-compiled block. It returns to top of 'fastpath' loop if it
//  jumps to its own beginning PC. Otherwise, it returns to top of main loop.
"execute_block%=:                             \n"
"jr    $t0                                    \n"
#ifdef USE_DIRECT_FASTPATH_BLOCK_RETURN_JUMPS
"sw    $t0, f_off_block_start_addr($sp)       \n" // <BD> Save code address, in case block jumps back
                                                  //      to its beginning and takes 'fastpath'
#else
"nop                                          \n" // <BD>
#endif

////////////////////////////
//   BRANCH-TEST CODE:    //
////////////////////////////

// Call psxBranchTest() and go back to top of loop
"call_psxBranchTest%=:                        \n"
"jal   %[psxBranchTest]                       \n"
"sw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> Store new psxRegs.pc val before calling C
"branchtest_fastpath_retaddr%=:               \n" // Next 3 instructions shared with 'fastpath' code..
"lw    $v0, %[psxRegs_pc_off]($fp)            \n" // After psxBranchTest() returns, load psxRegs.pc
                                                  //  back into $v0, which could be different than
                                                  //  before the call if an exception was issued.
"b     loop%=                                 \n" // Go back to top to process new psxRegs.pc value..
"move  $v1, $0                                \n" // <BD> ..using BD slot to set $v1 to 0, since
                                                  //  psxRegs.cycle shouldn't be incremented again.

#ifdef USE_DIRECT_FASTPATH_BLOCK_RETURN_JUMPS
////////////////////////////
//   FASTPATH LOOP CODE:  //
////////////////////////////

// When a block's new PC (jump target) is the same as its own beginning PC,
//  i.e. it branches back to its own top, it will return to this 'fast path'
//  version of above loop. It assumes that the block is already compiled and
//  unmodified, saving cycles versus the general loop. The main loop was
//  careful to save the block start address at location in stack frame.
//  NOTE: Blocks returning this way don't bother to set $v0 to any PC value.
"fastpath_loop%=:                             \n"
"lw    $t3, %[psxRegs_cycle_off]($fp)         \n" // $t3 = psxRegs.cycle
"lw    $t4, %[psxRegs_io_cycle_ctr_off]($fp)  \n" // $t4 = psxRegs.io_cycle_counter
"lw    $t0, f_off_block_start_addr($sp)       \n" // Load block code addr saved in main loop
"addu  $t3, $t3, $v1                          \n" // $t3 = psxRegs.cycle + $v1

// Must call psxBranchTest() when psxRegs.cycle >= psxRegs.io_cycle_counter
"sltu  $t4, $t3, $t4                          \n"
"beqz  $t4, call_psxBranchTest_fastpath%=     \n"
"sw    $t3, %[psxRegs_cycle_off]($fp)         \n" // <BD> IMPORTANT: store new psxRegs.cycle val,
                                                  //  whether or not we are branching here

// Execute already-compiled block. It returns to top of 'fastpath' loop if it
//  jumps to its own beginning PC. Otherwise, it returns to top of main loop.
"jr    $t0                                    \n"
"lw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> Blocks expect $v0 to contain PC val on entry

////////////////////////////
//   BRANCH-TEST CODE:    //
////////////////////////////

// Call psxBranchTest(). We don't have a new PC in $v0 to store, so call it
//  directly, and have it return to code shared with main 'call_psxBranchTest'.
"call_psxBranchTest_fastpath%=:               \n"
"j     %[psxBranchTest]                       \n"
"lw    $ra, f_off_branchtest_fastpath_ra($sp) \n" // <BD>

#endif // USE_DIRECT_FASTPATH_BLOCK_RETURN_JUMPS

////////////////////////////
// RECOMPILE BLOCK CODE:  //
////////////////////////////

// Recompile block and return to normal codepath.
"recompile_block%=:                           \n"
"jal   %[recRecompile]                        \n"
"sw    $t2, f_off_temp_var1($sp)              \n" // <BD> Save block ptr across call
"lw    $t2, f_off_temp_var1($sp)              \n" // Restore block ptr upon return
"lw    $v0, %[psxRegs_pc_off]($fp)            \n" // Blocks expect $v0 to contain PC val on entry
"b     execute_block%=                        \n" // Resume normal code path, but first we must..
"lw    $t0, 0($t2)                            \n" // <BD> ..load $t0 with ptr to block code


// Destroy stack frame, exiting inlined ASM block
// NOTE: During block executions, we'd never reach this point because the block
//  dispatch loop is currently infinite. This could change in the future.
// TODO: Could add a way to reset a game or load a new game from within
//  the running emulator by setting a global boolean, resetting
//  psxRegs.io_cycle_counter to 0, and checking if it's been set before
//  calling pxsBranchTest() here. If set, you must jump here to
//  exit the loop, to ensure that stack frame is adjusted before return!
"exit%=:                                      \n"
"addiu $sp, $sp, frame_size                   \n"
".set pop                                     \n"

: // Output
: // Input
  [block_ret_addr]             "i" (&block_ret_addr),
  [block_fast_ret_addr]        "i" (&block_fast_ret_addr),
  [psxRegs]                    "i" (&psxRegs),
  [psxRegs_pc_off]             "i" (off(pc)),
  [psxRegs_cycle_off]          "i" (off(cycle)),
  [psxRegs_io_cycle_ctr_off]   "i" (off(io_cycle_counter)),
  [recRecompile]               "i" (&recRecompile),
  [psxBranchTest]              "i" (&psxBranchTest),
  [psxRecLUT]                  "i" (psxRecLUT)
: // Clobber - No need to list anything but 'saved' regs
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "fp", "ra", "memory"
);
}


/* Execute blocks starting at psxRegs.pc
 * Blocks return directly (not indirectly via stack address var).
 * Block pointers are looked up using virtual address mapping of recRAM/recROM.
 * Called only from recExecute(), see notes there.
 *
 * IMPORTANT: Functions containing inline ASM should have attribute 'noinline'.
 *            Crashes at callsites can occur otherwise, at least with GCC 4.xx.
 */
__attribute__((noinline)) static void recExecute_direct_return_mmap()
{
__asm__ __volatile__ (
// NOTE: <BD> indicates an instruction in a branch-delay slot
".set push                                    \n"
".set noreorder                               \n"

// Set up our own stack frame. Should have 8-byte alignment, and have 16 bytes
// empty at 0($sp) for use by functions called from within recompiled code.
".equ  frame_size,                        32  \n"
".equ  f_off_branchtest_fastpath_ra,      28  \n"
".equ  f_off_temp_var1,                   24  \n"
".equ  f_off_block_start_addr,            16  \n"
"addiu $sp, $sp, -frame_size                  \n"

#ifdef USE_DIRECT_FASTPATH_BLOCK_RETURN_JUMPS
// Set the global var 'block_fast_ret_addr'.
"la    $t0, fastpath_loop%=                   \n"
"lui   $t1, %%hi(%[block_fast_ret_addr])      \n"
"sw    $t0, %%lo(%[block_fast_ret_addr])($t1) \n"

// We need a stack var to hold a return address for a 'fastpath' call to
//  psxBranchTest. This helps reduce size of code through sharing.
"la    $t0, branchtest_fastpath_retaddr%=     \n"
"sw    $t0, f_off_branchtest_fastpath_ra($sp) \n"
#endif

// Set the global var 'block_ret_addr'.
"la    $t0, loop%=                            \n"
"lui   $t1, %%hi(%[block_ret_addr])           \n"
"sw    $t0, %%lo(%[block_ret_addr])($t1)      \n"

// $fp/$s8 remains set to &psxRegs across all calls to blocks
"la    $fp, %[psxRegs]                        \n"

// Load $v0 once with psxRegs.pc, blocks will assign new value when returning
"lw    $v0, %[psxRegs_pc_off]($fp)            \n"

// Load $v1 once with zero. It is the # of cycles psxRegs.cycle should be
// incremented by when a block returns.
"move  $v1, $0                                \n"

// Align loop on cache-line boundary
".balign 32                                   \n"

////////////////////////////
//       LOOP CODE:       //
////////////////////////////

// NOTE: Blocks return to top of loop.
// NOTE: Loop expects following values to be set:
// $v0 = new value for psxRegs.pc
// $v1 = # of cycles to increment psxRegs.cycle by

// The loop pseudocode is this, interleaving ops to reduce load stalls:
//
// loop:
// $t2 = REC_RAM_VADDR | ($v0 & 0x00ffffff)
// $t0 = *($t2)
// psxRegs.cycle += $v1
// if (psxRegs.cycle >= psxRegs.io_cycle_counter)
//    goto call_psxBranchTest;
// psxRegs.pc = $v0
// if ($t0 == 0)
//    goto recompile_block;
// tmp_block_start_addr = $t0
// goto $t0;
// /* Code at addr $t0 will run and return having set $v0 to new psxRegs.pc */
// /*  value and $v1 to the number of cycles to increment psxRegs.cycle by. */
// /* If block branches back to its beginning, it will return to the        */
// /*  fastpath version of loop that skips looking up a code pointer.       */

// Infinite loop, blocks return here
"loop%=:                                      \n"
"lw    $t3, %[psxRegs_cycle_off]($fp)         \n" // $t3 = psxRegs.cycle
"lw    $t4, %[psxRegs_io_cycle_ctr_off]($fp)  \n" // $t4 = psxRegs.io_cycle_counter

// The block ptrs are mapped virtually to address space allowing lower
//  24 bits of PS1 PC address to lookup start of any RAM or ROM code block.
"lui   $t2, %[REC_RAM_VADDR_UPPER]            \n"
#ifdef HAVE_MIPS32R2_EXT_INS
"ins   $t2, $v0, 0, 24                        \n"
#else
"sll   $t1, $v0, 8                            \n"
"srl   $t1, $t1, 8                            \n"
"or    $t2, $t2, $t1                          \n"
#endif
"lw    $t0, 0($t2)                            \n" // $t0 = address of start of block code, or
                                                  //       or 0 if block needs recompilation
                                                  // IMPORTANT: leave block ptr in $t2, it gets
                                                  // saved & re-used if recRecompile() is called.

"addu  $t3, $t3, $v1                          \n" // $t3 = psxRegs.cycle + $v1

// Must call psxBranchTest() when psxRegs.cycle >= psxRegs.io_cycle_counter
"sltu  $t4, $t3, $t4                          \n"
"beqz  $t4, call_psxBranchTest%=              \n"
"sw    $t3, %[psxRegs_cycle_off]($fp)         \n" // <BD> IMPORTANT: store new psxRegs.cycle val,
                                                  //  whether or not we are branching here

// Recompile block, if necessary
"beqz  $t0, recompile_block%=                 \n"
"sw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> Use BD slot to store new psxRegs.pc val

// Execute already-compiled block. It returns to top of 'fastpath' loop if it
//  jumps to its own beginning PC. Otherwise, it returns to top of main loop.
"execute_block%=:                             \n"
"jr    $t0                                    \n"
#ifdef USE_DIRECT_FASTPATH_BLOCK_RETURN_JUMPS
"sw    $t0, f_off_block_start_addr($sp)       \n" // <BD> Save code address, in case block jumps back
                                                  //      to its beginning and takes 'fastpath'
#else
"nop                                          \n" // <BD>
#endif

////////////////////////////
//   BRANCH-TEST CODE:    //
////////////////////////////

// Call psxBranchTest() and go back to top of loop
"call_psxBranchTest%=:                        \n"
"jal   %[psxBranchTest]                       \n"
"sw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> Store new psxRegs.pc val before calling C
"branchtest_fastpath_retaddr%=:               \n" // Next 3 instructions shared with 'fastpath' code..
"lw    $v0, %[psxRegs_pc_off]($fp)            \n" // After psxBranchTest() returns, load psxRegs.pc
                                                  //  back into $v0, which could be different than
                                                  //  before the call if an exception was issued.
"b     loop%=                                 \n" // Go back to top to process new psxRegs.pc value..
"move  $v1, $0                                \n" // <BD> ..using BD slot to set $v1 to 0, since
                                                  //  psxRegs.cycle shouldn't be incremented again.

#ifdef USE_DIRECT_FASTPATH_BLOCK_RETURN_JUMPS
////////////////////////////
//   FASTPATH LOOP CODE:  //
////////////////////////////

// When a block's new PC (jump target) is the same as its own beginning PC,
//  i.e. it branches back to its own top, it will return to this 'fast path'
//  version of above loop. It assumes that the block is already compiled and
//  unmodified, saving cycles versus the general loop. The main loop was
//  careful to save the block start address at location in stack frame.
//  NOTE: Blocks returning this way don't bother to set $v0 to any PC value.
"fastpath_loop%=:                             \n"
"lw    $t3, %[psxRegs_cycle_off]($fp)         \n" // $t3 = psxRegs.cycle
"lw    $t4, %[psxRegs_io_cycle_ctr_off]($fp)  \n" // $t4 = psxRegs.io_cycle_counter
"lw    $t0, f_off_block_start_addr($sp)       \n" // Load block code addr saved in main loop
"addu  $t3, $t3, $v1                          \n" // $t3 = psxRegs.cycle + $v1

// Must call psxBranchTest() when psxRegs.cycle >= psxRegs.io_cycle_counter
"sltu  $t4, $t3, $t4                          \n"
"beqz  $t4, call_psxBranchTest_fastpath%=     \n"
"sw    $t3, %[psxRegs_cycle_off]($fp)         \n" // <BD> IMPORTANT: store new psxRegs.cycle val,
                                                  //  whether or not we are branching here

// Execute already-compiled block. It returns to top of 'fastpath' loop if it
//  jumps to its own beginning PC. Otherwise, it returns to top of main loop.
"jr    $t0                                    \n"
"lw    $v0, %[psxRegs_pc_off]($fp)            \n" // <BD> Blocks expect $v0 to contain PC val on entry

////////////////////////////
//   BRANCH-TEST CODE:    //
////////////////////////////

// Call psxBranchTest(). We don't have a new PC in $v0 to store, so call it
//  directly, and have it return to code shared with main 'call_psxBranchTest'.
"call_psxBranchTest_fastpath%=:               \n"
"j     %[psxBranchTest]                       \n"
"lw    $ra, f_off_branchtest_fastpath_ra($sp) \n" // <BD>

#endif // USE_DIRECT_FASTPATH_BLOCK_RETURN_JUMPS

////////////////////////////
// RECOMPILE BLOCK CODE:  //
////////////////////////////

// Recompile block and return to normal codepath.
"recompile_block%=:                           \n"
"jal   %[recRecompile]                        \n"
"sw    $t2, f_off_temp_var1($sp)              \n" // <BD> Save block ptr across call
"lw    $t2, f_off_temp_var1($sp)              \n" // Restore block ptr upon return
"lw    $v0, %[psxRegs_pc_off]($fp)            \n" // Blocks expect $v0 to contain PC val on entry
"b     execute_block%=                        \n" // Resume normal code path, but first we must..
"lw    $t0, 0($t2)                            \n" // <BD> ..load $t0 with ptr to block code


// Destroy stack frame, exiting inlined ASM block
// NOTE: During block executions, we'd never reach this point because the block
//  dispatch loop is currently infinite. This could change in the future.
// TODO: Could add a way to reset a game or load a new game from within
//  the running emulator by setting a global boolean, resetting
//  psxRegs.io_cycle_counter to 0, and checking if it's been set before
//  calling pxsBranchTest() here. If set, you must jump here to
//  exit the loop, to ensure that stack frame is adjusted before return!
"exit%=:                                      \n"
"addiu $sp, $sp, frame_size                   \n"
".set pop                                     \n"

: // Output
: // Input
  [block_ret_addr]             "i" (&block_ret_addr),
  [block_fast_ret_addr]        "i" (&block_fast_ret_addr),
  [psxRegs]                    "i" (&psxRegs),
  [psxRegs_pc_off]             "i" (off(pc)),
  [psxRegs_cycle_off]          "i" (off(cycle)),
  [psxRegs_io_cycle_ctr_off]   "i" (off(io_cycle_counter)),
  [recRecompile]               "i" (&recRecompile),
  [psxBranchTest]              "i" (&psxBranchTest),
  [REC_RAM_VADDR_UPPER]        "i" (REC_RAM_VADDR >> 16)
: // Clobber - No need to list anything but 'saved' regs
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "fp", "ra", "memory"
);
}


/* Execute blocks starting at psxRegs.pc until 'target_pc' is reached.
 * Blocks return indirectly, to address stored at 16($sp)
 *
 * IMPORTANT: Functions containing inline ASM should have attribute 'noinline'.
 *            Crashes at callsites can occur otherwise, at least with GCC 4.xx.
 */
__attribute__((noinline)) static void recExecuteBlock(unsigned target_pc)
{
	// Set block_ret_addr to 0, so generated code uses indirect returns
	block_ret_addr = block_fast_ret_addr = 0;

#ifndef ASM_EXECUTE_LOOP
	do {
		u32 *p = (u32*)PC_REC(psxRegs.pc);
		if (*p == 0)
			recRecompile();

		recFunc((void *)*p);

		if (psxRegs.cycle >= psxRegs.io_cycle_counter)
			psxBranchTest();
	} while (psxRegs.pc != target_pc);
#else
__asm__ __volatile__ (
// NOTE: <BD> indicates an instruction in a branch-delay slot
".set push                                    \n"
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
".balign 32                                   \n"

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
// $ra = block return address
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
"addu  $t2, $t0, $t1                          \n"
"lw    $t0, 0($t2)                            \n" // $t0 now points to beginning of recompiled
                                                  //  block, or is 0 if block needs compiling

// Must call psxBranchTest() when psxRegs.cycle >= psxRegs.io_cycle_counter
"sltu  $t4, $t3, $t4                          \n"
"beqz  $t4, call_psxBranchTest%=              \n"
"sw    $t3, %[psxRegs_cycle_off]($fp)         \n" // <BD> IMPORTANT: store new psxRegs.cycle val,
                                                  //  whether or not we are branching here

// Recompile block, if necessary
"beqz  $t0, recompile_block%=                 \n"
"nop                                          \n" // <BD>

"execute_block%=:                             \n"
"jalr  $ra, $t0                               \n" // Block is returning here, so safe to
"nop                                          \n" // <BD> set $ra using 'jalr'

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

// Recompile block and return to normal codepath
"recompile_block%=:                           \n"
"jal   %[recRecompile]                        \n"
"sw    $t2, f_off_temp_var1($sp)              \n" // <BD> Save block ptr across call
"lw    $t2, f_off_temp_var1($sp)              \n" // Restore block ptr upon return
"lw    $v0, %[psxRegs_pc_off]($fp)            \n" // Blocks expect $v0 to contain PC val on entry
"b     execute_block%=                        \n" // Resume normal code path, but first we must..
"lw    $t0, 0($t2)                            \n" // <BD> ..load $t0 with ptr to block code

// Destroy stack frame, exiting inlined ASM block
"exit%=:                                      \n"
"addiu $sp, $sp, frame_size                   \n"
".set pop                                     \n"

: // Output
: // Input
  [target_pc]                  "r" (target_pc),
  [psxRegs]                    "i" (&psxRegs),
  [psxRegs_pc_off]             "i" (off(pc)),
  [psxRegs_cycle_off]          "i" (off(cycle)),
  [psxRegs_io_cycle_ctr_off]   "i" (off(io_cycle_counter)),
  [psxRecLUT]                  "i" (psxRecLUT),
  [recRecompile]               "i" (&recRecompile),
  [psxBranchTest]              "i" (&psxBranchTest)
: // Clobber - No need to list anything but 'saved' regs
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "fp", "ra", "memory"
);
#endif
}


static void recExecute()
{
	// Clear code cache so that all emitted code from this point forward uses
	//  the correct block-return method. This also clears out any now-dead code
	//  emitted during BIOS startup. Non-dead BIOS code gets recompiled fresh.
	recReset();

	// By default, emit code that returns to dispatch loop indirectly, using
	//  address kept on stack. This return method is safe for use with both
	//  HLE BIOS and real BIOS. In fact, HLE BIOS requires indirect return
	//  jumps: During main execution, i.e. recExecute(), HLE BIOS calls
	//  recExecuteBlock() for certain 'softcalls', so in effect, two separate
	//  dispatch loops can be active, even simultaneously! Therefore, blocks
	//  under HLE BIOS cannot know at compile-time where to return to.
	//
	// To force indirect return code, set 'block_ret_addr' to 0 here.
	uint8_t use_indirect_return_dispatch_loop = 1;
	block_ret_addr = block_fast_ret_addr = 0;

#if defined(ASM_EXECUTE_LOOP) && defined(USE_DIRECT_BLOCK_RETURN_JUMPS)
	// When using a real BIOS, recExecuteBlock() is only ever called for
	//  initial BIOS startup, and when a certain PC is reached, it returns.
	//  Soon after that, this function is called to begin real execution.
	//  At this point, blocks will always be returning to the same address.
	//  We now emit faster, denser code that returns via direct jumps.
	//
	// NOTE: On entry, direct-return versions of dispatch loops set vars
	//       'block_ret_addr' and 'block_fast_ret_addr' to correct value.
	// IMPORTANT: All existing code needs to have been cleared at this point.
	use_indirect_return_dispatch_loop = Config.HLE;
#endif

	if (use_indirect_return_dispatch_loop) {
		if (rec_mem_mapped)
			recExecute_indirect_return_mmap();
		else
			recExecute_indirect_return_lut();
	} else {
		if (rec_mem_mapped)
			recExecute_direct_return_mmap();
		else
			recExecute_direct_return_lut();
	}
}


/* Invalidate 'Size' code block pointers at word-aligned PS1 address 'Addr'. */
static void recClear(u32 Addr, u32 Size)
{
	const u32 masked_ram_addr = Addr & 0x1ffffc;

	// Check if the page(s) of PS1 RAM that 'Addr','Size' target contain the
	//  start of any blocks. If not, invalidation would have no effect and is
	//  skipped. This eliminates 99% of large unnecessary invalidations that
	//  occur when many games stream CD data in-game.
	u32 page = masked_ram_addr/4096;
	u32 end_page = ((masked_ram_addr + (Size-1)*4)/4096) + 1;
	uint8_t has_code = 0;
	do {
		u32 pflag = 1 << (page & 7);  // Each byte in code_pages[] represents 8 pages
		has_code = code_pages[page/8] & pflag;
	} while ((++page != end_page) && !has_code);

	// NOTE: If PS1 mem is mapped/mirrored, make PC_REC_MMAP use the same virtual
	//       mirror region the game is already using, reducing TLB pressure.
	uptr dst_base = !rec_mem_mapped ? (uptr)recRAM : (uptr)PC_REC_MMAP(Addr & ~0x1fffff);

	if (has_code) {
		void *dst = (void*)(dst_base + (masked_ram_addr * REC_RAM_PTR_SIZE/4));
		memset(dst, 0, Size*REC_RAM_PTR_SIZE);
	}
}


/* Notification from emulator. */
void recNotify(int note, void *data __attribute__((unused)))
{
	switch (note)
	{
		/* R3000ACPU_NOTIFY_CACHE_ISOLATED,
		 * R3000ACPU_NOTIFY_CACHE_UNISOLATED
		 *  Sent from psxMemWrite32_CacheCtrlPort(). Also see notes there.
		 */
		case R3000ACPU_NOTIFY_CACHE_ISOLATED:
			/*  There's no need to do anything here:
			 * psxMemWrite32_CacheCtrlPort() has backed up lower 64KB PS1 RAM,
			 * allowing stores in emitted code to skip checking if cache
			 * is isolated before writing to RAM (the old 'writeok' check).
			 */
			REC_LOG_V("R3000ACPU_NOTIFY_CACHE_ISOLATED\n");
			break;
		case R3000ACPU_NOTIFY_CACHE_UNISOLATED:
			/*  Flush entire code cache, game has loaded new code:
			 * BIOS or routine has finished invalidating cache lines.
			 * psxMemWrite32_CacheCtrlPort() has restored lower 64KB PS1 RAM.
			 *  Using this coarse invalidation method fixes some games that
			 * previously needed hacks in recClear(), 'Buster Bros. Collection'.
			 * It's also part of a fix/hack for certain games that did Icache
			 * trickery (see DMA3 stuff further below).
			 * TODO: With this new method, it should eventually be possible to
			 *  provide an option to allow emitted code to skip most code
			 *  invalidations after stores, boosting speed.
			 */
			recClear(0, 0x200000/4);
			REC_LOG_V("R3000ACPU_NOTIFY_CACHE_UNISOLATED\n");
			break;

		/* Sent from psxDma3(). Also see notes there. */
		case R3000ACPU_NOTIFY_DMA3_EXE_LOAD:
			/* Game has begun reading from a CDROM file whose first sector is
			 * a standard 'PS-X EXE' header. Part of a hack by senquack to fix:
			 *  'Formula One Arcade' (crash on load)
			 *  'Formula One 99'     (crash on load) (NOTE: PAL version requires .SBI file)
			 *  'Formula One 2001'   (in-game controls, AI broken.. even the PS3
			 *                        supposedly has trouble emulating this.)
			 *
			 *  The workaround is enabled on a per-game basis (using CdromId).
			 *
			 *  These games do Icache trickery and merely doing the usual
			 * code-flush when Icache is unisolated is not enough. The fix is
			 * admittedly just a lucky hack, and requires these things:
			 *  1.)  As usual, invalidate code whenever emu calls recClear(),
			 *      typically after a DMA transfer.
			 *      *However*, emit no code invalidations whatsoever.
			 *      Fixes the in-game AI/controls in 'Formula One 2001'.
			 *  2.)  As usual, flush code cache when Icache is unisolated.
			 *      *However*, also flush cache when psxDma3() notifies us here.
			 *      This fixes crashes.
			 */
			if (flush_code_on_dma3_exe_load) {
				recClear(0, 0x200000/4);
				REC_LOG_V("R3000ACPU_NOTIFY_DMA3_EXE_LOAD .. Flushing dynarec cache\n");
			} else {
				REC_LOG_V("R3000ACPU_NOTIFY_DMA3_EXE_LOAD\n");
			}
			break;

		default:
			break;
	}
}


static void recReset()
{
	memset(code_pages, 0, sizeof(code_pages));
	memset(recRAM, 0, REC_RAM_SIZE);
	memset(recROM, 0, REC_ROM_SIZE);

	recMem = (u32*)recMemBase;

	regReset();

	// Set default recompilation options and any per-game options
	rec_set_options();
}


R3000Acpu psxRec =
{
	recInit,
	recReset,
	recExecute,
	recExecuteBlock,
	recClear,
	recNotify,
	recShutdown
};
