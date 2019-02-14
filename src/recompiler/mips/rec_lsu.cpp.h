/*
 * Copyright (c) 2009 Ulrich Hecht
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

/******************************************************************************
 * IMPORTANT: The following host registers have unique usage restrictions.    *
 *            See notes in mips_codegen.h for full details.                   *
 *  MIPSREG_AT, MIPSREG_V0, MIPSREG_V1, MIPSREG_RA                            *
 *****************************************************************************/

/***********************************************
 * Options that can be disabled for debugging: *
 ***********************************************/

// NOTE: Also see options 'USE_CONST_ADDRESSES' and 'USE_CONST_FUZZY_ADDRESSES'
//       defined elsewhere.

// Inline access to known-const HW,Scratchpad addresses (see rec_lsu_hw.cpp.h)
#define USE_DIRECT_HW_ACCESS

// Assume that stores using $k0,$k1,$gp,$sp as base registers aren't used
//  to modify code. Code invalidation sequence will not be emitted.
#define SKIP_CODE_INVALIDATION_FOR_SOME_BASE_REGS

// Assume that loads/stores using $zero,$k0,$k1,$gp,$sp as base registers always
//  go to RAM/scratchpad. Address-range-check and indirect-access sequences
//  will not be emitted.
#define SKIP_ADDRESS_RANGE_CHECK_FOR_SOME_BASE_REGS

// Allow series of loads/stores to extend into a branch delay slot.
#define INCLUDE_BD_SLOTS_IN_LSU_SERIES

// 2MB of PSX RAM (psxM) is now mirrored four times in virtual address
//  space, like a real PS1. This allows skipping mirror-region boundary
//  checks which special-cased loads/stores that crossed the boundary,
//  the 'Einhander' game fix. See notes in mem_mapping.cpp.
#define SKIP_SAME_2MB_REGION_CHECK

// Bypass 'writeok' cache-isolation check before stores. We now backup first
//  64KB of PSX RAM when cache is isolated, and restore it when unisolated.
//  We simply let the cache-flush stores go through to RAM (very temporarily).
//  See comments in psxmem.cpp psxMemWrite32_CacheCtrlPort().
#define SKIP_WRITEOK_CHECK

// If PSX mem is virtually mapped/mirrored, we only need to call C funcs when
//  accessing HW I/O ports, ROM, or cache control port. We've added support to
//  read/write funcs in psxhw.cpp to handle the latter two. Directly calling
//  psxHwRead*/Write* reduces TLB/Icache pressure because psxMemRead*/Write*
//  would only just end up calling those funcs every time.
//  NOTE: Only when the optimizations listed below are enabled are we sure
//        that all indirect accesses must be going to HW I/O, ROM, or cache port.
#if defined(USE_DIRECT_MEM_ACCESS) && defined(USE_DIRECT_HW_ACCESS) && defined(SKIP_SAME_2MB_REGION_CHECK) && defined(SKIP_WRITEOK_CHECK)
	#define USE_HW_FUNCS_FOR_INDIRECT_ACCESS
#endif

/* Cache values in host regs in load/store emitters. */
#define USE_LSU_CACHING


#define LSU_OPCODE(insn, rt, rn, imm) \
	write32((insn) | ((rn) << 21) | ((rt) << 16) | ((imm) & 0xffff))

//#define LOG_LOAD_STORE_SERIES

static s16 imm_max, imm_min;

static u32 LWL_MASKSHIFT[8] = { 0xffffff, 0xffff, 0xff, 0,
                                24, 16, 8, 0 };
static u32 LWR_MASKSHIFT[8] = { 0, 0xff000000, 0xffff0000, 0xffffff00,
                                0, 8, 16, 24 };

static u32 SWL_MASKSHIFT[8] = { 0xffffff00, 0xffff0000, 0xff000000, 0,
                                24, 16, 8, 0 };
static u32 SWR_MASKSHIFT[8] = { 0, 0xff, 0xffff, 0xffffff,
                                0, 8, 16, 24 };


/***************************
 * LSU reg caching (begin) *
 ***************************/
#ifdef USE_LSU_CACHING

#define LSU_TMP_CACHE_SIZE 2
static const u8 lsu_tmp_cache_reg_pool[LSU_TMP_CACHE_SIZE] =
	{ MIPSREG_AT, MIPSREG_V1 };

enum {
	LSU_TMP_CACHE_USE_TYPE_INVALID,
	LSU_TMP_CACHE_USE_TYPE_CONST
};

struct lsu_tmp_cache_entry_t {
	u8   host_reg;
	u8   use_type;
	u16  age;
	u32  constval;
} lsu_tmp_cache[LSU_TMP_CACHE_SIZE];

static struct lsu_tmp_cache_entry_t*
LSU_tmp_cache_get_reg(const int use_type, const u32 constval, u32 unused_param)
{
	// NOTE: 'lsu_tmp_cache_valid' is global uint8_t defined in recompiler.cpp
	for (int i=0; i < LSU_TMP_CACHE_SIZE; ++i) {
		if (lsu_tmp_cache_valid) {
			if (lsu_tmp_cache[i].use_type != LSU_TMP_CACHE_USE_TYPE_INVALID)
				lsu_tmp_cache[i].age++;
		} else {
			lsu_tmp_cache[i].host_reg = lsu_tmp_cache_reg_pool[i];
			lsu_tmp_cache[i].use_type = LSU_TMP_CACHE_USE_TYPE_INVALID;
			lsu_tmp_cache[i].age = 0;
		}
	}
	lsu_tmp_cache_valid = 1;

	switch (use_type)
	{
		case LSU_TMP_CACHE_USE_TYPE_CONST:
			{
				for (int i=0; i < LSU_TMP_CACHE_SIZE; ++i) {
					if (lsu_tmp_cache[i].use_type == use_type &&
						lsu_tmp_cache[i].constval == constval)
					{
						lsu_tmp_cache[i].age = 0;

						return &lsu_tmp_cache[i];
					}
				}
			}
			break;
		default:
			printf("%s(): Error, unexpected 'use_type': %d\n", __func__, use_type);
			exit(1);
	}

	// Couldn't find a cached value, so try to find an unused entry. If they're
	//  all used, we'll evict the oldest one.
	int oldest_entry = 0;
	int oldest_age = 0;
	for (int i=0; i < LSU_TMP_CACHE_SIZE; ++i) {
		if (lsu_tmp_cache[i].use_type == LSU_TMP_CACHE_USE_TYPE_INVALID) {
			lsu_tmp_cache[i].age = 0;

			return &lsu_tmp_cache[i];
		} else {
			if (lsu_tmp_cache[i].age > oldest_age) {
				oldest_age = lsu_tmp_cache[i].age;
				oldest_entry = i;
			}
		}
	}

	// Evict oldest entry.
	lsu_tmp_cache[oldest_entry].use_type = LSU_TMP_CACHE_USE_TYPE_INVALID;
	lsu_tmp_cache[oldest_entry].age = 0;
	return &lsu_tmp_cache[oldest_entry];
}

#endif // USE_LSU_CACHING
/*************************
 * LSU reg caching (end) *
 *************************/

static u32 emitConstBaseRegLUI(const u16 adr_hi)
{
	u32 host_reg = MIPSREG_AT;

#ifdef USE_LSU_CACHING
	struct lsu_tmp_cache_entry_t *entry =
		LSU_tmp_cache_get_reg(LSU_TMP_CACHE_USE_TYPE_CONST, (u32)adr_hi << 16, 0);

	if (entry->use_type == LSU_TMP_CACHE_USE_TYPE_INVALID) {
		// No regs had this constval, so update the reg assigned from the pool.
		entry->use_type = LSU_TMP_CACHE_USE_TYPE_CONST;
		entry->constval = (u32)adr_hi << 16;

		LUI(entry->host_reg, adr_hi);
	}

	host_reg = entry->host_reg;
#else
	LUI(host_reg, adr_hi);
#endif

	return host_reg;
}

/* Emit instruction(s) to convert address in PSX reg 'op_rs' (allocated to host
 *  reg 'rs') to a host address. Caller specifies 'tmp_reg' as a host reg we can
 *  use to help create the converted address. This should always be a different
 *  reg than 'desired_reg'.
 *
 * Returns: The host reg containing the converted address. For now, this is
 *          always 'desired_reg', but if we ever implement conversion caching,
 *          it might be different.
 *
 * IMPORTANT: We promise to emit at least one instruction here, so caller can
 *            always count on us safely filling any BD slots it needs to.
 *            We also promise to never return 'rs': the caller doesn't
 *            have to keep it locked after calling here if it doesn't want to.
 *
 * NOTE: This function is shared with rec_gte.cpp.h
 *
 */
static u32 emitAddressConversion(const u32 op_rs,
                                 const u32 rs,
                                 const u32 desired_reg,
                                 const u32 tmp_reg)
{
	if (psx_mem_mapped)
	{
		// METHOD 1 : psxM is mmap'd to fixed virtual address, RAM is mirrored,
		// (best)      and expansion-ROM/scratchpad/hardware-I/O regions (psxP,
		//             psxH) are mapped at offset 0x0f00_0000. Start of virtual
		//             mapping is an address with lower 28 bits clear.

#if (PSX_MEM_VADDR == 0)

		// Neat trick: if we've mmap'd to virtual address 0, address conversion
		//  needs just half the instructions.

	#ifdef HAVE_MIPS32R2_EXT_INS
		EXT(desired_reg, rs, 0, 28);
	#else
		SLL(desired_reg, rs, 4);
		SRL(desired_reg, desired_reg, 4);
	#endif

#else

		LUI(desired_reg, (PSX_MEM_VADDR >> 16));
	#ifdef HAVE_MIPS32R2_EXT_INS
		INS(desired_reg, rs, 0, 28);
	#else
		SLL(tmp_reg, rs, 4);
		SRL(tmp_reg, tmp_reg, 4);
		OR(desired_reg, desired_reg, tmp_reg);
	#endif

#endif // (PSXMEM_VADDR == 0)

	} else
	{
		// METHOD 2: Apparently mmap() and/or mirroring is unavailable, so psxM
		//            could be anywhere! RAM isn't mirrored virtually, so we
		//            must mask out all but lower 21 bits of rs here to
		//            handle the three 2MB PS1 RAM mirrors. Caller knows it can
		//            only use the converted base reg to access PS1 RAM.

		LW(desired_reg, PERM_REG_1, off(psxM));
#ifdef HAVE_MIPS32R2_EXT_INS
		EXT(tmp_reg, rs, 0, 21);  // tmp_reg = rs & 0x001f_ffff
#else
		SLL(tmp_reg, rs, 11);
		SRL(tmp_reg, tmp_reg, 11);
#endif
		ADDU(desired_reg, desired_reg, tmp_reg);
	}

	return desired_reg;
}


/* Emit no code invalidations for PSX base reg 'op_rs'? */
static inline uint8_t LSU_skip_code_invalidation(const u32 op_rs)
{
	// For certain games that do Icache trickery, we use a workaround that
	//  requires recompiled code does *no* invalidations of its own.
	if (!emit_code_invalidations)
		return 1;

#ifdef SKIP_CODE_INVALIDATION_FOR_SOME_BASE_REGS
	// Skip code invalidation when base reg is obviously not
	//  involved in code modification ($k0,$k1,$gp,$sp).
	if (op_rs >= 26 && op_rs <= 29)
		return 1;
#endif

#ifdef USE_CONST_FUZZY_ADDRESSES
	// Skip code invalidation when base reg value is not known-const,
	//  but at least known to be somewhere outside RAM.
	//  Probably a scratchpad/ROM static array access in original PS1 code.
	if (IsFuzzyNonramAddr(op_rs))
		return 1;
#endif

	return 0;
}

/* Emit only inlined direct accesses for PSX base reg 'op_rs'? */
static inline uint8_t LSU_use_only_direct_access(const u32 op_rs)
{
	// NOTE: If 'psx_mem_mapped' is 1, all valid PS1 addresses between begin
	//       of RAM and end of scratchpad are virtually mapped/mirrored.

#ifdef SKIP_ADDRESS_RANGE_CHECK_FOR_SOME_BASE_REGS
	if (psx_mem_mapped) {
		// Skip address range check when base register in use is obviously
		//  going to access RAM/scratchpad ($zero,$k0,$k1,$gp,$sp).
		//  Zero reg can only access lower 64KB RAM region reserved for BIOS.
		if ((op_rs >= 26 && op_rs <= 29) || (op_rs == 0))
			return 1;
	}
#endif

#ifdef USE_CONST_FUZZY_ADDRESSES
	// Skip address range check when base reg value is not known-const,
	//  but at least known to be somewhere in RAM/scratchpad.
	//  Probably a RAM/scratchpad static array access in original PS1 code.
	if (IsFuzzyRamAddr(op_rs))
		return 1;

	if (psx_mem_mapped && IsFuzzyScratchpadAddr(op_rs))
		return 1;
#endif

	return 0;
}

/* Emit only indirect C mem accesses for PSX base reg 'op_rs'? */
static inline uint8_t LSU_use_only_indirect_access(const u32 op_rs)
{
	// NOTE: If 'psx_mem_mapped' is 1, all valid PS1 addresses between begin
	//       of RAM and end of scratchpad are virtually mapped/mirrored.

#ifdef USE_CONST_FUZZY_ADDRESSES
	if (psx_mem_mapped) {
		if (IsFuzzyNonramAddr(op_rs) && !IsFuzzyScratchpadAddr(op_rs)) {
			//  Exact value of base reg is unknown, but it's at least known to
			// not be a RAM or scratchpad address. Only indirect accesses
			// should be emitted.

			return 1;
		}
	} else {
		if (IsFuzzyNonramAddr(op_rs)) {
			//  Exact value of base reg is unknown, but it's at least known to
			// not be a RAM address. Only indirect accesses should be emitted.

			return 1;
		}
	}
#endif

	return 0;
}

/* Returns 1 if a load/store in a BD slot can be emitted before the
 *  branch/jump. Allows load/store series to extend into a BD slot.
 */
static uint8_t LSU_bd_slot_OoO_possible(const u32  bj_opcode,
                                     const u32  bd_slot_opcode,
                                     const u32  bd_slot_pc,
                                     const uint8_t bd_slot_is_load)
{
	// A JR/JALR disqualifies out-of-order loads: we don't know the branch
	//  target at recompile-time, and can't check for load-delay trickery.
	if (bd_slot_is_load && opcodeIsIndirectJump(bj_opcode))
		return 0;

	// Regs accessed by the branch/jump. Don't care about zero reg.
	const u32 bj_writes = (u32)opcodeGetWrites(bj_opcode) & ~1;
	const u32 bj_reads  = (u32)opcodeGetReads(bj_opcode) & ~1;

	const u32 bd_slot_reads = (u32)opcodeGetReads(bd_slot_opcode) & ~1;

	// JAL/JALR write a return address, so worry about that first.
	if (bd_slot_reads & bj_writes)
		return 0;

	// Load in BD slot:
	//  If we emit a load out-of-order, it could incorrectly affect a branch
	//  decision (or indirect jump, but we already ruled those out). We must
	//  also ensure the opcode at branch/jump target PC does not read the reg
	//  written by the load (load delay).
	if (bd_slot_is_load)
	{
		u32 target_opcode;
		if (opcodeIsBranch(bj_opcode))
			target_opcode = OPCODE_AT(opcodeGetBranchTargetAddr(bj_opcode, bd_slot_pc));
		else
			target_opcode = OPCODE_AT(opcodeGetDirectJumpTargetAddr(bj_opcode));

		const u32 target_reads = (u32)opcodeGetReads(target_opcode) & ~1;
		const u32 bd_slot_writes = 1 << _fRt_(bd_slot_opcode);

		if (bd_slot_writes & (bj_reads | target_reads))
			return 0;
	}

	return 1;
}


/* Return count of the number of consecutive loads and/or stores starting at
   current instruction. All loads/stores in the series must share a common
   base register. Any intervening NOPs encountered are included in the count.
   If 'pc_of_last_store_in_series' is not NULL, it will be set to the last store
   found in the series, or set to impossible value of 1 if no stores were found.
 */
static int count_loads_stores(u32  *pc_of_last_store_in_series,
                              uint8_t *series_includes_bd_slot)
{
	int count = 0;
	u32 PC = pc - 4;
	const u32 shared_rs = _Rs_;

#ifdef LOG_LOAD_STORE_SERIES
	int num_nops = 0;
	int num_loads = 0;
	int num_stores = 0;
#endif

	imm_min = imm_max = _Imm_;

	uint8_t store_found = 0;
	uint8_t bd_slot_included = 0;
	int nops_at_end = 0;
	for (;;)
	{
		const u32 opcode = OPCODE_AT(PC);
		PC += 4;

		if (opcode == 0) {
#ifdef LOG_LOAD_STORE_SERIES
			num_nops++;
#endif
			nops_at_end++;
			count++;  // Include the NOP in the count
			continue;
		}

		const uint8_t is_store = opcodeIsStore(opcode);
		const uint8_t is_load  = !is_store && opcodeIsLoad(opcode);

#ifdef LOG_LOAD_STORE_SERIES
		num_stores += is_store;
		num_loads  += is_load;
#endif

		if (is_load || is_store)
		{
			// All loads/stores in series must share a base register
			if (shared_rs != _fRs_(opcode)) {
				// Break out of for-loop, we can't include the current opcode.
				break;
			}
#ifdef INCLUDE_BD_SLOTS_IN_LSU_SERIES
		} else if (opcodeIsBranchOrJump(opcode))
		{
			const u32  bd_slot_opcode = OPCODE_AT(PC);
			const uint8_t bd_slot_is_store = opcodeIsStore(bd_slot_opcode);
			const uint8_t bd_slot_is_load  = opcodeIsLoad(bd_slot_opcode);

			if ((!bd_slot_is_store && !bd_slot_is_load) ||
			    _fRs_(bd_slot_opcode) != shared_rs)
				break;

			if (LSU_bd_slot_OoO_possible(opcode, bd_slot_opcode, PC, bd_slot_is_load)) {
				// Include branch/jump opcode in count. It's emitted after series.
				count++;
				nops_at_end = 0;
				bd_slot_included = 1;

				// Loop around again for BD-slot opcode, we'll terminate after that.
				continue;
			} else {
				// Break out of for-loop, we can't include BD slot op.
				break;
			}
#endif // INCLUDE_BD_SLOTS_IN_LSU_SERIES
		} else
		{
			// Break out of for-loop, we can't include the current opcode.
			break;
		}

		nops_at_end = 0;
		count++;

		// Update min and max immediate values for the series
		if (_fImm_(opcode) > imm_max) imm_max = _fImm_(opcode);
		if (_fImm_(opcode) < imm_min) imm_min = _fImm_(opcode);

		if (is_store) {
			store_found = 1;
			if (pc_of_last_store_in_series != NULL)
				*pc_of_last_store_in_series = (PC-4);
		}

		// For loads, check if base reg got overwritten. Stop series here if so.
		if (is_load && (_fRt_(opcode) == _fRs_(opcode)))
			break;

		// If series started in a BD slot, limit count to 1
		if (branch)
			break;

		if (bd_slot_included)
			break;
	}

	// If no stores were found, always assign impossible val '1'. If we did find
	//  a store, we've already set this to correct value inside the for-loop.
	if (!store_found && (pc_of_last_store_in_series != NULL))
		*pc_of_last_store_in_series = 1;

	// Notify caller if the last load/store in the series lied in a BD slot.
	if (series_includes_bd_slot != NULL)
		*series_includes_bd_slot = bd_slot_included;

	// Don't include any NOPs at the end of sequence in the reported count..
	//  only count ones lying between the load/store opcodes
	count -= nops_at_end;

#ifdef LOG_LOAD_STORE_SERIES
	// Only log this series if we found more than one load/store
	if (count > 1) {
		num_nops -= nops_at_end;
		printf("LOAD/STORE SERIES count: %d  num_loads: %d  num_stores: %d  imm_min: %d  imm_max: %d\n",
			count, num_loads, num_stores, imm_min, imm_max);
		if (bd_slot_included)
			printf("(SERIES INCLUDES LOAD/STORE IN BD SLOT BELOW)\n");
	}
#endif

	return count;
}

/* Emit a series of loads/stores to a common base address.
 *  If there are stores, code invalidation might also be emitted.
 *  If 'force_indirect' param is 1, only indirect C func calls will
 *  be emitted. Otherwise, we'll decide here what we can get away with.
 *  Worst case scenario is that we emit both direct and indirect accesses.
 */
static void general_loads_stores(const int  count,
                                 const u32  pc_of_last_store_in_series,
                                 const uint8_t force_indirect)
{
	// All loads/stores in series share the same base reg.
	const u32 op_rs = _Rs_;
	const u32 rs = regMipsToHost(op_rs, REG_LOAD, REG_REGISTER);

	// Allocate the first 'rt' reg of the series. This helps reduce load stall
	//  on 'rs' here and on first use of 'rt' later. It also lets the direct and
	//  indirect codepaths share the code emitted for the reg allocation, if any.
	u32 first_rt_allocation = 0;
	if (opcodeIsLoad(OPCODE_AT(pc-4))) {
		// LWL/LWR load is special: needs to load existing contents of reg
		const u32  insn = OPCODE_AT(pc-4) & 0xfc000000;
		const uint8_t is_lwl_lwr = (insn == 0x88000000 || insn == 0x98000000);
		first_rt_allocation = regMipsToHost(_Rt_, (is_lwl_lwr ? REG_LOAD : REG_FIND), REG_REGISTER);
	} else {
		first_rt_allocation = regMipsToHost(_Rt_, REG_LOAD, REG_REGISTER);
	}

	u32 *backpatch_label_exit_1 = 0;
	u32 *backpatch_label_exit_2 = 0;

#ifdef USE_DIRECT_MEM_ACCESS
	const uint8_t emit_direct   = !force_indirect && !LSU_use_only_indirect_access(op_rs);
	const uint8_t emit_indirect = !emit_direct || !LSU_use_only_direct_access(op_rs);
#else
	// NOTE: If you are debugging, #undefine identifier 'USE_DIRECT_MEM_ACCESS'
	//       and recompile dynarec. Just forcing these 0/1 isn't enough.
	const uint8_t emit_direct   = 0;
	const uint8_t emit_indirect = 1;
#endif

	/*****************************************************************
	 * Emit direct loads/stores & any appropriate code invalidations *
	 *****************************************************************/
	if (emit_direct)
	{
		const uint8_t contains_store = (pc_of_last_store_in_series != 1);
		const uint8_t emit_code_invalidation   = contains_store && !LSU_skip_code_invalidation(op_rs);
		const uint8_t emit_address_range_check = emit_indirect;

		u32 *backpatch_label_hle_1 = 0;
		u32 *backpatch_label_hle_2 = 0;
		u32 *backpatch_label_hle_3 = 0;

		/*********************************************************************
		 * Emit any checks+branches (if indirect code is also being emitted) *
		 *********************************************************************/

		uint8_t reg_state_pushed = 0;

		if (emit_address_range_check)
		{
			/*****************************
			 * Emit address range check: *
			 *****************************/

			// If we are emitting an address-range-check, it means both direct
			//  and indirect code is emitted. Path to take is chosen at runtime.
			//  Reg allocation state at begin/end of both codepaths must be
			//  identical: push the state here and pop it before indirect code.
			regPushState();
			reg_state_pushed = 1;

			if (psx_mem_mapped)
			{
				// See comments in mem_mapping.cpp for full explanation.
				//  PSX addresses between [0x0000_0000 .. 0x1f80_ffff] are
				//  mapped virtually to PSX_MEM_VADDR location. The PS1's
				//  four 2MB RAM mirror regions are mirrored virtually,
				//  eliminating any need for masking, and allowing streamlined
				//  access to both RAM + scratchpad.
				//
				// NOTE: Even though 1KB scratchpad region ends at 0x?f80_03ff,
				//       we inline accesses up to 0x?f80_0fff. This makes the
				//       range check faster. This shouldn't affect anything,
				//       because there's no valid PS1 addresses in that small
				//       area beyond 0x03ff anyway.

				if (emit_code_invalidation)
				{
					// Code-invalidation sequence will be emitted, which means
					//  we will leave MIPSREG_A1 holding the max effective
					//  address shifted left by 4. It will be used to determine
					//  if we should skip the code-invalidation sequence coming
					//  directly after the direct code sequence: if MSB is set,
					//  it cannot be a RAM address and couldn't hold modifiable
					//  code, i.e., it is a scratchpad/8MB ROM-expansion
					//  address [0x?f00_0000 .. 0x?f80_03ff]

					LUI(TEMP_1, 0xf801);
					ADDIU(MIPSREG_A0, rs, imm_max);
					SLL(MIPSREG_A1, MIPSREG_A0, 4);
					SLTU(TEMP_1, MIPSREG_A1, TEMP_1);
					backpatch_label_hle_1 = (u32 *)recMem;
					BEQZ(TEMP_1, 0);

					// MIPSREG_A0 contains effective address of opcode with
					//  maximum immediate offset.
					// MIPSREG_A1 contains this same address shifted left 4:
					//  MSB determines if code-invalidation sequence is skipped.
				} else
				{
					// We can emit one less instruction if code invalidation is
					//  skipped: There is no need to keep a value in MIPSREG_A1
					//  that would otherwise be used to determine when the
					//  address is in scratchpad and code invalidation should be
					//  skipped (scratchpad never contains executable code).
					// Note that we mask away bits 27:24 before using SLTIU
					//  for range check: This is because SLTIU instruction sign-
					//  extends its 16-bit immediate, which is a bit quirky.

					ADDIU(MIPSREG_A0, rs, imm_max);
#ifdef HAVE_MIPS32R2_EXT_INS
					EXT(TEMP_1, MIPSREG_A0, 12, 12);
#else
					SRL(TEMP_1, MIPSREG_A0, 12);
					ANDI(TEMP_1, TEMP_1, 0x0fff);
#endif
					// Note the use of 0x0801 here instead of 0xf801.
					SLTIU(TEMP_1, TEMP_1, 0x0801);
					backpatch_label_hle_1 = (u32 *)recMem;
					BEQZ(TEMP_1, 0);

					// MIPSREG_A0 contains effective address of opcode with
					//  maximum immediate offset.
				}
			} else
			{
				// PSX mem is not virtually mapped/mirrored:
				//  Interpret bit 27 of max effective address as a sign bit. Any
				//  valid PS1 address that is not a RAM address has bits 27:24 set.
				//  Any addresses higher than RAM will take indirect path.

				ADDIU(MIPSREG_A0, rs, imm_max);
				SLL(MIPSREG_A1, MIPSREG_A0, 4);
				backpatch_label_hle_1 = (u32 *)recMem;
				BLTZ(MIPSREG_A1, 0);

				// MIPSREG_A0 contains effective address of opcode with
				//  maximum immediate offset.
			}

			// NOTE: Branch delay slot contains next emitted instruction.
			// IMPORTANT: BD slot here should be empty so it can hold the first
			//            instruction of the base reg conversion sequence.
		}

#ifndef SKIP_WRITEOK_CHECK
		/************************************************************
		 * Emit 'psxRegs.writeok' cache-isolation check (obsolete)  *
		 ************************************************************/

		// Check if (psxRegs.writeok != 0) to see if RAM is writeable, i.e.
		//  PS1 cache is not isolated.
		//  NOTE: This check is no longer necessary, see comments in
		//        psxmem.cpp psxMemWrite32_CacheCtrlPort()

		LW(TEMP_1, PERM_REG_1, off(writeok)); // <BD slot>
		backpatch_label_hle_2 = (u32 *)recMem;
		BEQZ(TEMP_1, 0);

		// NOTE: Branch delay slot contains next emitted instruction
#endif

#ifdef SKIP_SAME_2MB_REGION_CHECK
		//  NOTE: Virtual mapping+mirroring made this check unnecessary.
		if (!psx_mem_mapped)
#endif
		{
			/*****************************************
			 * Emit same-2MB-region check (obsolete) *
			 *****************************************/

			// Check if base_reg and base_reg+imm_min are in the same 2MB region.
			//  Some games like 'Einhander' use a base reg near a RAM mirror-region
			//  boundary to access locations in the prior mirror region. If this
			//  is found, use indirect access.

			ADDIU(TEMP_1, rs, imm_min);  // <BD slot>
#ifdef HAVE_MIPS32R2_EXT_INS
			EXT(TEMP_1, TEMP_1, 21, 3);  // TEMP_1 = (TEMP_1 >> 21) & 0x7
			EXT(TEMP_2, rs, 21, 3);      // TEMP_2 = ((rs+imm_min) >> 21) & 0x7
#else
			SRL(TEMP_1, TEMP_1, 21);     // <BD slot>
			ANDI(TEMP_1, TEMP_1, 7);
			SRL(TEMP_2, rs, 21);
			ANDI(TEMP_2, TEMP_2, 7);
#endif
			backpatch_label_hle_3 = (u32 *)recMem;
			BNE(TEMP_1, TEMP_2, 0);      // goto label_hle if not in same 2MB region

			// NOTE: Branch delay slot contains next emitted instruction
		}

		/************************************
		 * Emit base reg address conversion *
		 ************************************/

		// NOTE: Any empty BD slot will get filled here.
		//  TEMP_1 will hold converted base reg, TEMP_2 is overwritten temp reg.
		//  However, in the future, emitAddressConversion() might return a
		//  different reg, if caching of converted addresses is ever implemented.
		const u32 converted_base_reg = emitAddressConversion(op_rs, rs, TEMP_1, TEMP_2);  // <BD slot>

		// Code invalidation needs the original base register value. However,
		//  it sometimes gets overwritten by the last load in a series.
		//  This is a problem because the invalidation sequence comes after
		//  the actual loads/stores in the series. If this situation is
		//  detected, the original base reg value is moved to a dedicated
		//  register (MIPSREG_A2) before the last load is done. Normally, the
		//  invalidation sequence will use the original base reg.
		u32 unmodified_base_reg = rs;

		u32 PC = pc - 4;
		int icount = count;
		do
		{
			const u32 opcode = OPCODE_AT(PC);
			PC += 4;

			// Skip any NOPs in the series
			if (opcode == 0)
				continue;

			const uint8_t is_store = contains_store ? opcodeIsStore(opcode) : 0;
			const uint8_t is_load  = is_store ? 0 : opcodeIsLoad(opcode);

			if (!is_store && !is_load) {
				// Must be a jump/branch whose BD slot is included as the last
				//  load/store in the series: skip it.
				continue;
			}

			const s16 op_imm = _fImm_(opcode);
			const u32 op_rt  = _fRt_(opcode);

			if (is_store)
			{
				// STORE OPCODE (DIRECT)

				u32 rt;
				if (icount == count) {
					// The 'rt' reg for first opcode in series was already allocated
					rt = first_rt_allocation;
				} else {
					rt = regMipsToHost(op_rt, REG_LOAD, REG_REGISTER);
				}

				backpatch_label_exit_1 = 0;
				if (icount == 1 && (emit_code_invalidation || emit_indirect)) {
					// This is the end of the loop
					backpatch_label_exit_1 = (u32 *)recMem;
					if (emit_code_invalidation) {
						// If addresses were in scratchpad, skip both code
						//  invalidation and the indirect code.
						BLTZ(MIPSREG_A1, 0); // bltz label_exit
					} else {
						// Skip indirect code
						B(0); // b label_exit
					}
					// NOTE: Branch delay slot will contain the instruction below
				}
				LSU_OPCODE(opcode & 0xfc000000, rt, converted_base_reg, op_imm);  // <BD> (MAYBE)

				regUnlock(rt);
			} else
			{
				// LOAD OPCODE (DIRECT)

				// If load overwrites base reg, we must backup original base reg
				//  value, as it is needed for code invalidation sequence later.
				if ((op_rt == op_rs) && emit_code_invalidation) {
					MOV(MIPSREG_A2, rs);
					unmodified_base_reg = MIPSREG_A2;
				}

				u32 rt;
				if (icount == count) {
					// The 'rt' reg for first opcode in series was already allocated
					rt = first_rt_allocation;
				} else {
					// LWL/LWR load is special: it needs to load existing contents of reg
					const u32 insn = opcode & 0xfc000000;
					const uint8_t is_lwl_lwr = (insn == 0x88000000 || insn == 0x98000000);

					rt = regMipsToHost(op_rt, (is_lwl_lwr ? REG_LOAD : REG_FIND), REG_REGISTER);
				}

				if (icount == 1 && (emit_code_invalidation || emit_indirect)) {
					// This is the end of the loop
					backpatch_label_exit_1 = (u32 *)recMem;
					if (emit_code_invalidation) {
						// If addresses were in scratchpad, skip both code
						//  invalidation and the indirect code.
						BLTZ(MIPSREG_A1, 0); // bltz label_exit
					} else {
						// Skip indirect code
						B(0); // b label_exit
					}
					// NOTE: Branch delay slot will contain the instruction below
				}
				LSU_OPCODE(opcode & 0xfc000000, rt, converted_base_reg, op_imm);  // <BD> (MAYBE)

				SetUndef(op_rt);
				regMipsChanged(op_rt);
				regUnlock(rt);
			}
		} while (--icount);

		if (emit_code_invalidation)
		{
			/*****************************************************
			 * Invalidate recRAM[addr+imm16] code block pointers *
			 *****************************************************/

			uint8_t first_invalidation_done = 0;

			LUI(TEMP_3, ADR_HI(recRAM)); // temp_3 = upper code block ptr array addr

			u32 PC = pc - 4;
			int icount = count;
			do
			{
				u32 opcode = OPCODE_AT(PC);
				PC += 4;

				// Skip any loads, NOPs, or jumps/branches in the series
				if (!opcodeIsStore(opcode))
					continue;

				const s16 op_imm = _fImm_(opcode);

				// See earlier notes at declaration of 'unmodified_base_reg'

				if (first_invalidation_done || !emit_address_range_check) {
					ADDIU(MIPSREG_A0, unmodified_base_reg, op_imm);      // Code invalidation needs eff addr
				} else {
					// We already placed base_reg+imm_max in MIPSREG_A0 during
					//  initial range-checks. No need to load again if first
					//  immediate is same as imm_max.
					first_invalidation_done = 1;
					if (op_imm != imm_max)
						ADDIU(MIPSREG_A0, unmodified_base_reg, op_imm);  // Code invalidation needs eff addr
				}

#ifdef HAVE_MIPS32R2_EXT_INS
				EXT(TEMP_1, MIPSREG_A0, 0, 21); // TEMP_1 = MIPSREG_A0 & 0x1fffff
#else
				SLL(TEMP_1, MIPSREG_A0, 11);
				SRL(TEMP_1, TEMP_1, 11);
#endif

				if ((opcode & 0xfc000000) != 0xac000000) {
					// Not a SW, clear lower 2 bits to ensure addr is aligned:
#ifdef HAVE_MIPS32R2_EXT_INS
					INS(TEMP_1, 0, 0, 2);
#else
					SRL(TEMP_1, TEMP_1, 2);
					SLL(TEMP_1, TEMP_1, 2);
#endif
				}
				ADDU(TEMP_1, TEMP_1, TEMP_3);

				if ((PC-4) == pc_of_last_store_in_series && emit_direct) {
					// This is the last store in the series, so the last invalidation
					//  made and the end of all the direct code. Skip past the
					//  indirect code coming after this.

					backpatch_label_exit_2 = (u32 *)recMem;
					B(0); // b label_exit
					// NOTE: Branch delay slot will contain the instruction below
				}
				// Important: this should be the last opcode in the loop (see note above)
				SW(0, TEMP_1, ADR_LO(recRAM));  // set code block ptr to NULL


				// Last store in series? We're done.
				if ((PC-4) == pc_of_last_store_in_series)
					break;

			} while (--icount);
		}

		if (backpatch_label_hle_1)
			fixup_branch(backpatch_label_hle_1);
		if (backpatch_label_hle_2)
			fixup_branch(backpatch_label_hle_2);
		if (backpatch_label_hle_3)
			fixup_branch(backpatch_label_hle_3);

		if (reg_state_pushed)
			regPopState();
	}

	/******************************
	 * Emit indirect loads/stores *
	 ******************************/
	if (emit_indirect)
	{
		enum { WIDTH_8, WIDTH_16, WIDTH_32 };
		const uptr mem_read_func[]  = { (uptr)psxMemRead8,  (uptr)psxMemRead16,  (uptr)psxMemRead32  };
		const uptr mem_write_func[] = { (uptr)psxMemWrite8, (uptr)psxMemWrite16, (uptr)psxMemWrite32 };
		const uptr *read_func  = mem_read_func;
		const uptr *write_func = mem_write_func;
#ifdef USE_HW_FUNCS_FOR_INDIRECT_ACCESS
		// See notes at top of file
		const uptr hw_read_func[]   = { (uptr)psxHwRead8,   (uptr)psxHwRead16,   (uptr)psxHwRead32   };
		const uptr hw_write_func[]  = { (uptr)psxHwWrite8,  (uptr)psxHwWrite16,  (uptr)psxHwWrite32  };
		if (psx_mem_mapped) {
			read_func  = hw_read_func;
			write_func = hw_write_func;
		}
#endif

		u32 PC = pc - 4;
		int icount = count;
		do
		{
			const u32 opcode = OPCODE_AT(PC);
			PC += 4;

			// Skip any NOPs in the series
			if (opcode == 0)
				continue;

			const uint8_t is_store = opcodeIsStore(opcode);
			const uint8_t is_load  = is_store ? 0 : opcodeIsLoad(opcode);

			if (!is_store && !is_load) {
				// Must be a jump/branch whose BD slot is included as the last
				//  load/store in the series: skip it.
				continue;
			}

			const u32 op_rt  = _fRt_(opcode);
			const s16 op_imm = _fImm_(opcode);

			if (is_store)
			{
				// STORE OPCODE (INDIRECT)

				u32 rt;
				if (icount == count) {
					// The 'rt' reg for first opcode in series was already allocated
					rt = first_rt_allocation;
				} else {
					rt = regMipsToHost(op_rt, REG_LOAD, REG_REGISTER);
				}

				const u32 insn = opcode & 0xfc000000;
				switch (insn)
				{
					case 0xa0000000: // SB
						ADDIU(MIPSREG_A0, rs, op_imm);
						JAL(write_func[WIDTH_8]);
						MOV(MIPSREG_A1, rt); // <BD> Branch delay slot
						break;
					case 0xa4000000: // SH
						ADDIU(MIPSREG_A0, rs, op_imm);
						JAL(write_func[WIDTH_16]);
						MOV(MIPSREG_A1, rt); // <BD> Branch delay slot
						break;
					case 0xac000000: // SW
						ADDIU(MIPSREG_A0, rs, op_imm);
						JAL(write_func[WIDTH_32]);
						MOV(MIPSREG_A1, rt); // <BD> Branch delay slot
						break;
					case 0xa8000000: // SWL
					case 0xb8000000: // SWR
						ADDIU(MIPSREG_A0, rs, op_imm);
#ifdef HAVE_MIPS32R2_EXT_INS
						JAL(read_func[WIDTH_32]);       // result in MIPSREG_V0
						INS(MIPSREG_A0, 0, 0, 2);       // <BD> clear 2 lower bits of $a0
#else
						SRL(MIPSREG_A0, MIPSREG_A0, 2);
						JAL(read_func[WIDTH_32]);       // result in MIPSREG_V0
						SLL(MIPSREG_A0, MIPSREG_A0, 2); // <BD> clear lower 2 bits of $a0
#endif

						ADDIU(MIPSREG_A0, rs, op_imm);

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
							SRLV(TEMP_1, rt, TEMP_3);        // temp_1 = new_data >> shift
						else                    // SWR
							SLLV(TEMP_1, rt, TEMP_3);        // temp_1 = new_data << shift

						JAL(write_func[WIDTH_32]);
						OR(MIPSREG_A1, MIPSREG_A1, TEMP_1);  // <BD> $a1 |= temp_1
						break;
					default:
						printf("ERROR: unrecognized store opcode in %s: %x\n", __func__, opcode);
						exit(1);
						break;
				}

				regUnlock(rt);
			} else
			{
				// LOAD OPCODE (INDIRECT)

				const u32 insn = opcode & 0xfc000000;

				u32 rt;
				if (icount == count) {
					// The 'rt' reg for first opcode in series was already allocated
					rt = first_rt_allocation;
				} else {
					if (insn == 0x88000000 || insn == 0x98000000) {
						// LWL/LWR, so we need existing contents of register
						rt = regMipsToHost(op_rt, REG_LOAD, REG_REGISTER);
					} else {
						rt = regMipsToHost(op_rt, REG_FIND, REG_REGISTER);
					}
				}

				switch (insn)
				{
					case 0x80000000: // LB
						JAL(read_func[WIDTH_8]);
						ADDIU(MIPSREG_A0, rs, op_imm); // <BD> Branch delay slot
						if (op_rt) {
#ifdef HAVE_MIPS32R2_SEB_SEH
							SEB(rt, MIPSREG_V0);
#else
							SLL(rt, MIPSREG_V0, 24);
							SRA(rt, rt, 24);
#endif
						}
						break;
					case 0x90000000: // LBU
						JAL(read_func[WIDTH_8]);    // result in MIPSREG_V0
						ADDIU(MIPSREG_A0, rs, op_imm); // <BD> Branch delay slot
						if (op_rt) {
							MOV(rt, MIPSREG_V0);
						}
						break;
					case 0x84000000: // LH
						JAL(read_func[WIDTH_16]);   // result in MIPSREG_V0
						ADDIU(MIPSREG_A0, rs, op_imm); // <BD> Branch delay slot
						if (op_rt) {
#ifdef HAVE_MIPS32R2_SEB_SEH
							SEH(rt, MIPSREG_V0);
#else
							SLL(rt, MIPSREG_V0, 16);
							SRA(rt, rt, 16);
#endif
						}
						break;
					case 0x94000000: // LHU
						JAL(read_func[WIDTH_16]);   // result in MIPSREG_V0
						ADDIU(MIPSREG_A0, rs, op_imm); // <BD> Branch delay slot
						if (op_rt) {
							ANDI(rt, MIPSREG_V0, 0xffff);
						}
						break;
					case 0x8c000000: // LW
						JAL(read_func[WIDTH_32]);   // result in MIPSREG_V0
						ADDIU(MIPSREG_A0, rs, op_imm); // <BD> Branch delay slot
						if (op_rt) {
							MOV(rt, MIPSREG_V0);
						}
						break;
					case 0x88000000: // LWL
					case 0x98000000: // LWR
						ADDIU(MIPSREG_A0, rs, op_imm);
#ifdef HAVE_MIPS32R2_EXT_INS
						JAL(read_func[WIDTH_32]);   // result in MIPSREG_V0
						INS(MIPSREG_A0, 0, 0, 2);   // <BD> clear 2 lower bits of $a0 (using branch delay slot)
#else
						SRL(MIPSREG_A0, MIPSREG_A0, 2);
						JAL(read_func[WIDTH_32]);       // result in MIPSREG_V0
						SLL(MIPSREG_A0, MIPSREG_A0, 2); // <BD> clear lower 2 bits of $a0
#endif

						ADDIU(MIPSREG_A0, rs, op_imm);

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

						AND(rt, rt, TEMP_2);            // mask pre-existing contents of rt

						if (insn == 0x88000000) // LWL
							SLLV(TEMP_3, MIPSREG_V0, TEMP_3);   // temp_3 = data_read << shift
						else                    // LWR
							SRLV(TEMP_3, MIPSREG_V0, TEMP_3);   // temp_3 = data_read >> shift

						OR(rt, rt, TEMP_3);
						break;
					default:
						printf("ERROR: unrecognized load opcode in %s: %x\n", __func__, opcode);
						exit(1);
						break;
				}

				SetUndef(op_rt);
				regMipsChanged(op_rt);
				regUnlock(rt);
			}
		} while (--icount);
	}

	// label_exit:
	if (backpatch_label_exit_1)
		fixup_branch(backpatch_label_exit_1);
	if (backpatch_label_exit_2)
		fixup_branch(backpatch_label_exit_2);

	regUnlock(rs);
}


/* Emits a series of loads/stores to a known-const RAM base address.
 *  NOTE: We don't do code invalidation here: no games seem to need it.
 *        XXX - In fact, doing code invalidation here makes 'Colony Wars'
 *        freeze when returning to main menu after ship self-destruct sequence.
 */
static void const_loads_stores(const int  count,
                               const uint8_t contains_store,
                               const u32  rs_constval,
                               const u32  array_offset_reg)
{
#ifndef SKIP_WRITEOK_CHECK
	// Do nothing if (psxRegs.writeok == 0).
	u32 *backpatch_label_no_write = 0;
	if (contains_store) {
		// Check if (psxRegs.writeok != 0) to see if RAM is writeable
		LW(TEMP_1, PERM_REG_1, off(writeok));
		backpatch_label_no_write = (u32 *)recMem;
		BEQZ(TEMP_1, 0);  // if (!psxRegs.writeok) goto label_no_write
		NOP(); // <BD slot>
	}
#endif

	// We'll need to use this reg if 'array_offset_reg' is used.
	const u32 tmp_base_reg = TEMP_1;

	// If param 'array_offset_reg' is non-zero, there is an array-offset reg
	//  that gets added to the base reg. Used by emitOptimizedStaticLoad().
	u32 host_array_offset_reg = 0;
	if (array_offset_reg)
		host_array_offset_reg = regMipsToHost(array_offset_reg, REG_LOAD, REG_REGISTER);

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

		const uint8_t is_store   = contains_store && opcodeIsStore(opcode);
		const uint8_t is_load    = !is_store && opcodeIsLoad(opcode);
		const uint8_t is_lwl_lwr = is_load && opcodeIsLoadWordUnaligned(opcode);

		if (!is_store && !is_load) {
			// Must be a jump/branch whose BD slot is included as the last
			//  load/store in the series: skip it.
			continue;
		}

		const s32 imm = _fImm_(opcode);
		const u32 op_rt = _fRt_(opcode);

		u32 rt;
		// Note that LWL/LWR merge writeback with existing contents of reg.
		if (is_store || is_lwl_lwr)
			rt = regMipsToHost(op_rt, REG_LOAD, REG_REGISTER);
		else
			rt = regMipsToHost(op_rt, REG_FIND, REG_REGISTER);

		const uptr host_addr = (uptr)psxM + ((rs_constval + imm) & 0x1fffff);

		// If psxM is mapped to virtual address 0 and PS1 address is in
		//  lower 32KB address space, we can use $zero as base reg.
		uint8_t use_zero_base_reg = (ADR_HI(host_addr) == 0 && psxM == 0);

		// Emit LUI for base reg (if not cached in host reg).
		if (!use_zero_base_reg &&
		    (!base_reg_lui_emitted ||
		     base_reg_val != (ADR_HI(host_addr) << 16)))
		{
			base_reg_lui_emitted = 1;
			base_reg_val = ADR_HI(host_addr) << 16;

			base_reg = emitConstBaseRegLUI(ADR_HI(host_addr));

			if (array_offset_reg != 0) {
				ADDU(tmp_base_reg, base_reg, host_array_offset_reg);

				base_reg = tmp_base_reg;
			}
		}

		if (use_zero_base_reg) {
			if (array_offset_reg != 0)
				base_reg = array_offset_reg;
			else
				base_reg = 0;
		}

		LSU_OPCODE(opcode & 0xfc000000, rt, base_reg, ADR_LO(host_addr));

		if (is_load) {
			SetUndef(op_rt);
			regMipsChanged(op_rt);
		}
		regUnlock(rt);
	} while (--icount);

#ifndef SKIP_WRITEOK_CHECK
	// label_no_write:
	if (backpatch_label_no_write)
		fixup_branch(backpatch_label_no_write);
#endif

	if (host_array_offset_reg)
		regUnlock(host_array_offset_reg);
}


#include "rec_lsu_hw.cpp.h"  // Direct HW I/O


/* Main load/store function that calls all the others above */
static void emitLoadStoreSeries()
{
	// STATIC VARIABLE
	// See comments below regarding series ending in a BD slot
	static uint8_t next_call_emits_nothing = 0;
	if (next_call_emits_nothing) {
		// Skip emitting anything for just this one call
		next_call_emits_nothing = 0;
		return;
	}

	u32  pc_of_last_store_in_series = 1;
	uint8_t series_includes_bd_slot = 0;
	const int count = count_loads_stores(&pc_of_last_store_in_series, &series_includes_bd_slot);

	// Series can include a load/store in a BD-slot as their last opcode:
	//  The count includes the jump/branch and BD-slot opcode. At the end of
	//  the series, we emit just the BD-slot opcode. When we are next called,
	//  it will be the jump/branch emitter wanting to recompile the opcode in
	//  its BD-slot. We won't emit anything at that next call.
	if (series_includes_bd_slot)
		next_call_emits_nothing = 1;  // STATIC VARIABLE

#ifdef WITH_DISASM
	// First opcode in series was already disassembled in recRecompile().
	// Disassemble the additional opcodes we're including in the series.
	for (int i = 0; i < count-1; i++) {
		// Series can extend to include a load/store in a BD slot. If we
		//  encounter a branch/jump opcode in the series, stop disassembling:
		//  let recRecompile() disassemble the branch/jump & BD-slot load/store.
		//  Otherwise, we'll upset the order of disassembly.
		if (opcodeIsBranchOrJump(OPCODE_AT(pc + i * 4)))
			break;
		DISASM_PSX(pc + i * 4);
	}
#endif

	uint8_t const_addr = 0;
#ifdef USE_CONST_ADDRESSES
	const_addr = IsConst(_Rs_);
#endif

	if (!const_addr)
	{
		/****************************
		 * Handle non-const address *
		 ****************************/

		// Call general-case emitter for non-const address. It will need to
		//  emit address range check and emit both direct & indirect code.
		general_loads_stores(count, pc_of_last_store_in_series, 0);
	} else
	{
		/************************
		 * Handle const address *
		 ************************/

		// Is address outside lower 8MB RAM region? (2MB mirrored x4)
		const u32 base_reg_constval = GetConst(_Rs_);
		const u32 addr_max = base_reg_constval + imm_max;
		if ((addr_max & 0x0fffffff) >= 0x800000)
		{
			uint8_t is_hw_address = 0;
#ifdef USE_DIRECT_HW_ACCESS
			{
				const u16 upper = addr_max >> 16;
				if (upper == 0x1f80 || upper == 0x9f80 || upper == 0xbf80)
					is_hw_address = 1;
			}
#endif

			if (is_hw_address) {
				/**************************************
				 * Handle const scratchpad/HW address *
				 **************************************/

				// Address is in 64KB scratchpad,HW I/O port region
				const_hw_loads_stores(count, base_reg_constval);
			} else {
				/*********************************
				 * Handle const indirect address *
				 *********************************/

				// Call general-case emitter, but force indirect access since
				//  known-const address is going to ROM or cache-control port.
				general_loads_stores(count, pc_of_last_store_in_series, 1);
			}
		} else {
			/****************************
			 * Handle const RAM address *
			 ****************************/

			const uint8_t contains_store = pc_of_last_store_in_series != 1;
			const_loads_stores(count, contains_store, base_reg_constval, 0);
		}
	}

	pc += (count-1)*4;

	// If we included a load/store in a BD-slot as our last opcode, next opcode
	//  to be recompiled should be the jump/branch before the slot.
	if (series_includes_bd_slot)
		pc -= 8;
}


/* Check for load from a static address in original code. If found, we can skip
 *  emitting one or two useless opcodes. These were usually compiler-generated.
 *
 * Called from recLUI() emitter.
 *
 *  Note that we can't optimize stores this way.. stores used $at as the
 *  base reg, and the $at write is known to propagate beyond the store in
 *  some assembler routines in games (e.g. Naughty Dog studio games).
 *
 *  Sequence Type 1 (load from static address):
 *    LUI  load_dst_reg, ADDRESS_HI(const_addr)
 *    L*   load_dst_reg, ADDRESS_LO(const_addr)(load_dst_reg)
 *
 *  Sequence Type 2 (load from static array address):
 *    LUI  load_dst_reg, ADDRESS_HI(const_addr)
 *    ADDU load_dst_reg, load_dst_reg, array_offset_reg
 *    L*   load_dst_reg, ADDRESS_LO(const_addr)(load_dst_reg)
 *
 * Returns: 1 if sequence found and optimized load is emitted.
 */
static uint8_t emitOptimizedStaticLoad()
{
#ifndef USE_CONST_ADDRESSES
	return 0;
#endif

	const u32  op1 = OPCODE_AT(pc-4);
	const u32  op2 = OPCODE_AT(pc);
	const u32  op3 = OPCODE_AT(pc+4);
	const uint8_t is_array_access = (_fOp_(op2) == 0 && _fFunct_(op2) == 0x21);  // ADDU
	const u32  lsu_opcode  = is_array_access ? op3 : op2;
	const u32  lui_opcode  = op1;
	const u32  lui_addr    = (u32)_fImmU_(lui_opcode) << 16;
	const uint8_t is_ram_addr = (lui_addr & 0x0fffffff) < 0x00800000;
	const uint8_t is_hw_addr  = (lui_addr & 0x0fffffff) == 0x0f800000;

	if (!opcodeIsLoad(lsu_opcode) || opcodeIsLoadWordUnaligned(lsu_opcode))
		return 0;

	if (_fRt_(lui_opcode) != _fRs_(lsu_opcode) ||
	    _fRt_(lsu_opcode) != _fRs_(lsu_opcode))
		return 0;

	if (!is_ram_addr && !is_hw_addr)
		return 0;

	u32 rs_constval = lui_addr;
	u32 array_offset_reg = 0;

	if (is_array_access)
	{
		// Only optimize array accesses when the LUI is setting an obvious
		//  base address in RAM, i.e. 0x8xxx_xxxx.
		if ((lui_addr >> 28) != 0x8)
			return 0;

		const u32 addu_opcode = op2;
		const u32 lui_writes  = (1 << _fRt_(lui_opcode)) & ~1;
		const u32 addu_reads  = ((1 << _fRs_(addu_opcode)) | (1 << _fRt_(addu_opcode))) & ~1;
		const u32 addu_writes = (1 << _fRd_(addu_opcode)) & ~1;
		const u32 lsu_reads   = (1 << _fRs_(lsu_opcode)) & ~1;

		if (!(lui_writes & addu_reads) || !(lsu_reads & addu_writes))
			return 0;

		// Sanity check (__builtin_ctz() on 0 value is undefined)
		if ((addu_reads & ~lui_writes) == 0)
			return 0;

		array_offset_reg = __builtin_ctz(addu_reads & ~lui_writes);

		// If offset reg is const, add it now and forget about the reg entirely.
		if (IsConst(array_offset_reg)) {
			rs_constval += GetConst(array_offset_reg);
			array_offset_reg = 0;
		}
	}

	DISASM_PSX(pc);
	pc += 4;

	if (is_array_access) {
		DISASM_PSX(pc);
		pc += 4;
	}

	// We only allow loads, never stores.
	const uint8_t contains_store = 0;
	if (is_ram_addr)
		const_loads_stores(1, contains_store, rs_constval, array_offset_reg);
	else
		const_hw_loads_stores(1, rs_constval);

	return 1;
}


/***********************************************
 * Opcode emitters called during recompilation *
 ***********************************************/
static void recLB()
{
	// Rt = mem[Rs + Im] (signed)
	emitLoadStoreSeries();
}

static void recLBU()
{
	// Rt = mem[Rs + Im] (unsigned)
	emitLoadStoreSeries();
}

static void recLH()
{
	// Rt = mem[Rs + Im] (signed)
	emitLoadStoreSeries();
}

static void recLHU()
{
	// Rt = mem[Rs + Im] (unsigned)
	emitLoadStoreSeries();
}

static void recLW()
{
	// Rt = mem[Rs + Im] (unsigned)
	emitLoadStoreSeries();
}

static void recLWL()
{
	// Unaligned load (unsigned)
	// NOTE: Mem value read is merged with existing Rt contents
	emitLoadStoreSeries();
}

static void recLWR()
{
	// Unaligned load (unsigned)
	// NOTE: Mem value read is merged with existing Rt contents
	emitLoadStoreSeries();
}

static void recSB()
{
	// mem[Rs + Im] = Rt
	emitLoadStoreSeries();
}

static void recSH()
{
	// mem[Rs + Im] = Rt
	emitLoadStoreSeries();
}

static void recSW()
{
	// mem[Rs + Im] = Rt
	emitLoadStoreSeries();
}

static void recSWL()
{
	// Unaligned store
	emitLoadStoreSeries();
}

static void recSWR()
{
	// Unaligned store
	emitLoadStoreSeries();
}
