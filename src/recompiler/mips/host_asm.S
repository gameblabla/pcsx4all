/*
 * ASM functions for
 * Mips-to-mips recompiler for pcsx4all
 *
 * Copyright (c) 2009 Ulrich Hecht
 * Copyright (c) 2017 modified by Dmitry Smagin, Daniel Silsby
 *
 * MIPS_MakeCodeVisible:
 * Copyright (c) 2017 Nebuleon Fumika <nebuleon.fumika@gmail.com>
 *
 * It is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with it.  If not, see <http://www.gnu.org/licenses/>.
*/

// Mips32r2 introduced useful instructions:
#if (defined(__mips_isa_rev) && (__mips_isa_rev >= 2)) || \
    (defined(_MIPS_ARCH_MIPS32R2) || defined(_MIPS_ARCH_MIPS32R3) || \
     defined(_MIPS_ARCH_MIPS32R5) || defined(_MIPS_ARCH_MIPS32R6))
#define HAVE_MIPS32R2_EXT_INS
#define HAVE_MIPS32R2_SEB_SEH
#define HAVE_MIPS32R2_CACHE_OPS
#endif


#ifdef HAVE_MIPS32R2_CACHE_OPS
/* MIPS32R2_MakeCodeVisible authored and generously provided by
 *  Nebuleon Fumika <nebuleon.fumika@gmail.com>, used with permission. (GPLv2)
 *  https://github.com/Nebuleon/ReGBA/blob/master/source/opendingux/port-asm.S
 *
 * MIPS32r2 provided new fine-grained usermode cache-flush abilities. This
 *  is better than cacheflush() syscall, which flushes the entire cache.
 */
.global MIPS32R2_MakeCodeVisible
.set push
.set noat
.set noreorder
.ent MIPS32R2_MakeCodeVisible

# Register assignment:
# $4 = parameter #1: void* Code
# $5 = parameter #2: unsigned int CodeLength
# Clobbered: $1, $2, $4, $5
MIPS32R2_MakeCodeVisible:
  rdhwr $1, $1            # read SYNCI_Step into $1
  beq   $1, $0, 2f        # no need to use SYNCI? return
  addiu $2, $1, -1

  beq   $5, $0, 2f
  add   $5, $4, $5        # $5 is now End, derived from CodeLength (delay)
  nor   $2, $2, $0        # $2 is now the mask to use to round $4 down
  and   $4, $4, $2        # Round $4 down to the cache line containing it

  # The rounding down was necessary above in order to allow a partial
  # cache line near the end of the code to be flushed. Otherwise, it may
  # have been ignored after flushing one cache line's worth of bytes.
  # Consider:
  # Cache line 1 |                [flushflushflushf]
  # Cache line 2 | lushflushflu
  # Cache line 2 does not start a line to be flushed, so it isn't flushed.
  # The rounding down is required to make it this instead:
  # Cache line 1 |[padpadpadpadpad flushglushflushf]
  # Cache line 2 |[lushflushflu                    ]

  # Now git 'er done.
1:
  synci ($4)              # Combined Data Writeback-Instruction Invalidate (R2)
  addu  $4, $4, $1        # go to the next cache line
  sltu  $2, $4, $5        # if Code < End
  bne   $2, $0, 1b        #   goto 1
  nop                     # cannot delay usefully here

  sync                    # guard against memory hazards

2:
  jr.hb $ra               # return to caller, while guarding against
                          #   instruction hazards
  nop                     # cannot delay usefully here

.set pop
.end MIPS32R2_MakeCodeVisible
#endif // HAVE_MIPS32R2_CACHE_OPS

// vim:shiftwidth=2:expandtab
