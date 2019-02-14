/*
 * Mips-to-mips recompiler for pcsx4all
 *
 * Copyright (c) 2009 Ulrich Hecht
 * Copyright (c) 2018 modified by Dmitry Smagin, Daniel Silsby
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

#ifndef MEM_MAPPING_H
#define MEM_MAPPING_H

/* This is used for direct writes in mips recompiler, as well as mapping
 *  of PS1 PC values to block code ptrs (replaces use of psxRecLUT[]).
 */



/* Lower 28 bits of this address should be zero! Offsets between 0 and
 * 0xF81_0000 from this address should be free for mapping. PSX RAM, ROM
 * expansion, and I/O regions are mapped into this virtual address region,
 * i.e. psxM,psxP,psxH
 */
#ifdef MMAP_TO_ADDRESS_ZERO
	// Allows address-conversion optimization.
	#define PSX_MEM_VADDR 0ULL
#else
	// For development: null pointer dereferences will segfault as normal.
	// Address conversions will need more instructions.
	#define PSX_MEM_VADDR 0x10000000ULL
#endif

int rec_mmap_psx_mem();
void rec_munmap_psx_mem();



/* Lower 28 bits of this virtual address should be zero!
 * Recompiler's code ptr tables are mapped into this virtual address region,
 * i.e. recRAM and recROM.
 */
#define REC_RAM_PTR_SIZE sizeof(uptr)
#define REC_RAM_SIZE (0x200000 / 4 * REC_RAM_PTR_SIZE)
#define REC_ROM_SIZE ( 0x80000 / 4 * REC_RAM_PTR_SIZE)

#define REC_RAM_VADDR 0x20000000ULL
#define REC_ROM_VADDR (REC_RAM_VADDR + (0x00c00000ULL / 4 * REC_RAM_PTR_SIZE))

int rec_mmap_rec_mem();
void rec_munmap_rec_mem();

#endif // MEM_MAPPING_H
