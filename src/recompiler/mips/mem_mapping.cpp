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

/* This is used for direct writes in mips recompiler, as well as mapping
 *  of PS1 PC values to block code ptrs (replaces use of psxRecLUT[]).
 */

#include <stdio.h>
#include "mem_mapping.h"
#include "psxmem.h"

#if defined(SHMEM_MIRRORING) || defined(TMPFS_MIRRORING)
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#ifdef SHMEM_MIRRORING
#include <sys/shm.h>   // For Posix shared mem
#endif

/* Map PSX RAM regions 0x0000_0000..0x007f_ffff and Expansion-ROM/HW-I/O
 *  regions 0x1f00_0000..0x1f80_ffff to a fixed virtual address region
 *  starting at PSX_MEM_VADDR, which is typically 0x1000_0000.
 * Once mapped, we assign emu global ptr vars 'psxM' (RAM), 'psxP'
 *  (Parallel port ROM expansion), and 'psxH' (1KB scratchpad + HW I/O).
 *
 *  This allows three things for the dynarec:
 *   1.) 2MB RAM region is mirrored 4X just like on the real hardware.
 *       Games like Einhander need the mirroring, where the effective addr
 *       of a base reg + negative offset can cross into the prior region.
 *   2.) 1KB scratchpad region access can be rolled into RAM accesses.
 *   3.) Virtual addr can be generated with single LUI() and the lower
 *       28 bits of PSX effective addr can just be inserted.
 *
 *  Note that we don't bother to map in the primary 512KB BIOS region starting
 *   at 0xbfc0_0000 on a PS1 (psxR) into this virtual address space. We let
 *   indirect C functions take care of these accesses, along with access to the
 *   cache-control port at 0xfffe_0130. Analysis shows that BIOS accesses
 *   account for only a tiny portion of total accesses during gameplay. The only
 *   reason we map the rarely-accessed Expansion-ROM region (psxP) is that
 *   it lies between the RAM and scratchpad regions (what we really care about).
 */
int rec_mmap_psx_mem()
{
	bool l_psx_mem_mapped = false;
	bool  success = true;
	int   memfd = -1;
	void* mmap_retval = NULL;
	const char* mem_fname = NULL;
	bool  l_psxM_mirrored = false;

	// Everything done here with mmap() is with a granularity of 64KB, so
	//  make sure the platform has a page size that will allow this
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size > 65536) {
		printf("ERROR: %s expects system page size <= 65536 bytes\n"
		       "       System reported page size: %ld bytes\n", __func__, page_size);
		success = false;
		goto exit;
	}

#ifdef SHMEM_MIRRORING
	// Get a POSIX shared memory object fd
	printf("Mapping/mirroring 2MB PSX RAM using POSIX shared mem\n");
	mem_fname = "/pcsx4all_psxmem";
	memfd = shm_open(mem_fname, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
#else
	// Use tmpfs file - TMPFS_DIR string literal should be defined in Makefile
	//  CFLAGS with escaped quotes (alter if needed): -DTMPFS_DIR=\"/tmp\"
	mem_fname = TMPFS_DIR "/pcsx4all_psxmem";
	printf("Mapping/mirroring 2MB PSX RAM using tmpfs file %s\n", mem_fname);
	memfd = open(mem_fname, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
#endif

	if (memfd < 0) {
#ifdef SHMEM_MIRRORING
		printf("Error acquiring POSIX shared memory file descriptor\n");
#else
		printf("Error creating tmpfs file: %s\n", mem_fname);
#endif
		success = false;
		goto exit;
	}

	// We want 2MB of PSX RAM
	if (ftruncate(memfd, 0x200000) < 0) {
		printf("Error in call to ftruncate(), could not get 2MB of PSX RAM\n");
		success = false;
		goto exit;
	}

	// Map PSX RAM to start at fixed virtual address specified in mem_mapping.h
	//  (usually 0x1000_0000)
	mmap_retval = mmap((void*)PSX_MEM_VADDR, 0x200000,
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0);
	if (mmap_retval == MAP_FAILED) {
		printf("Error: mmap() to %p of %uMB failed.\n", (void*)(PSX_MEM_VADDR), 0x200000/(1024*1024));
		success = false;
		goto exit;
	}
	psxM = (s8*)mmap_retval;
	l_psx_mem_mapped = true;

	// Create three mirrors of the 2MB RAM, all the way up to 0x7fffff
	mmap_retval = mmap((void*)(PSX_MEM_VADDR+0x200000), 0x200000,
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0);
	if (mmap_retval == MAP_FAILED) {
		printf("Error: creating 1st mmap() mirror of 2MB PSX RAM failed.\n");
		goto exit;
	}
	mmap_retval = mmap((void*)(PSX_MEM_VADDR+0x400000), 0x200000,
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0);
	if (mmap_retval == MAP_FAILED) {
		printf("Error: creating 2nd mmap() mirror of 2MB PSX RAM failed.\n");
		goto exit;
	}
	mmap_retval = mmap((void*)(PSX_MEM_VADDR+0x600000), 0x200000,
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0);
	if (mmap_retval == MAP_FAILED) {
		printf("Error: creating 3rd mmap() mirror of 2MB PSX RAM failed.\n");
		goto exit;
	}
	l_psxM_mirrored = true;
	printf(" ..success!\n");

	printf("Mapping 8MB Expansion ROM + 64KB PSX HW I/O regions using mmap\n");
	// Map regions to start at offset past psxM that matches PSX mapping,
	//  i.e. if psxM starts at 0x1000_0000, expansion region will be at
	//  0x1f00_0000 and HW I/O region will be at 0x1f80_0000
	// NOTE: For 8MB Expansion region, we expect programs/BIOS to not write
	//       to this region, thereby never actually allocating any pages
	//       of real host RAM (or very few). It should be safe to assume this.
	mmap_retval = mmap((void*)(PSX_MEM_VADDR+0x0f000000), 0x0f810000-0x0f000000,
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED|MAP_ANONYMOUS, -1, 0);
	if (mmap_retval == MAP_FAILED) {
		printf("Error: mmap() to %p of %uKB failed.\n",
				(void*)(PSX_MEM_VADDR+0x0f000000), (0x0f810000-0x0f000000)/1024);
		success = false;
		goto exit;
	}
	psxP = (s8*)mmap_retval;                          // ROM expansion region (parallel port)
	psxH = (s8*)((uintptr_t)mmap_retval+0x00800000);  // HW I/O region
	printf(" ..success!\n");

exit:
	if (!success) {
		// Oops, couldn't do everything we wanted to do
		perror(__func__);
		printf("ERROR: Failed to map/mirror PSX memory, falling back to malloc().\n"
			   "Dynarec will emit slower code for loads/stores.\n");

		// Abandon any mappings that were created
		if (l_psx_mem_mapped) {
			// Unmap 2MB PSX RAM and its three mirrors (if mirrors got created)
			munmap((void*)PSX_MEM_VADDR, l_psxM_mirrored ? 0x800000 : 0x200000);
		}
		psxM = psxP = psxH = NULL;
	}

	// Close/unlink file: RAM is released when munmap()'ed or pid terminates
#ifdef SHMEM_MIRRORING
	shm_unlink(mem_fname);
#else
	if (memfd >= 0)
		close(memfd);
	unlink(mem_fname);
#endif

	return success ? 0 : -1;
}

void rec_munmap_psx_mem()
{
	// Unmap 2MB PSX RAM and its three mirrors
	munmap((void*)PSX_MEM_VADDR, 0x800000);
	// Unmap 8MB ROM Expansion and 64KB HW I/O regions
	munmap((void*)(PSX_MEM_VADDR+0x0f000000), 0x0f810000-0x0f000000);
	psxM = psxP = psxH = NULL;
}


/* Map/mirror recRAM code pointer table to fixed virtual address REC_RAM_VADDR,
 *  typically 0x2000_0000. Map recROM code pointer table to offset from this
 *  same fixed virtual address to match where ROM lies in PS1 address space,
 *  typically 0x20c0_0000. Note that we do not map ROM at 0x2fc0_0000. This is
 *  because we'd like to support future 64-bit platforms with 8-byte pointers.
 *  On those future platforms, ROM ptrs would end up mapped at 0x2000_0000 +
 *  (0x00c0_0000 * 2). Leaving ROM ptrs mapped at this lower end of the fixed
 *  address space allows this flexibility. Only RAM or ROM addresses ever get
 *  executed on PS1, so we have more freedom here than with the virtual memory
 *  mapping in rec_mmap_psx_mem(). As a result, only the lower 24 bits of a PS1
 *  PC address are used to index into recRAM/recROM.
 */
int rec_mmap_rec_mem()
{
	bool  l_rec_mem_mapped = false;
	bool  success = true;
	int   memfd = -1;
	void* mmap_retval = NULL;
	const char* mem_fname = NULL;
	bool  l_recRAM_mirrored = false;

	// Everything done here with mmap() is with a granularity of 512KB, so
	//  make sure the platform has a page size that will allow this
	//  (512KB will be mapped for recROM, recRAM has 2MB granularity)
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size > 524288) {
		printf("ERROR: %s expects system page size <= 524288 bytes\n"
		       "       System reported page size: %ld bytes\n", __func__, page_size);
		success = false;
		goto exit;
	}

#ifdef SHMEM_MIRRORING
	// Get a POSIX shared memory object fd
	printf("Mapping/mirroring %uMB recRAM using POSIX shared mem\n", REC_RAM_SIZE/(1024*1024));
	mem_fname = "/pcsx4all_recram";
	memfd = shm_open(mem_fname, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
#else
	// Use tmpfs file - TMPFS_DIR string literal should be defined in Makefile
	//  CFLAGS with escaped quotes (alter if needed): -DTMPFS_DIR=\"/tmp\"
	mem_fname = TMPFS_DIR "/pcsx4all_recram";
	printf("Mapping/mirroring %uMB recRAM using tmpfs file %s\n", REC_RAM_SIZE/(1024*1024), mem_fname);
	memfd = open(mem_fname, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
#endif

	if (memfd < 0) {
#ifdef SHMEM_MIRRORING
		printf("Error acquiring POSIX shared memory file descriptor\n");
#else
		printf("Error creating tmpfs file: %s\n", mem_fname);
#endif
		success = false;
		goto exit;
	}

	// We want 2MB for recRAM (assuming 4-byte host pointers)
	if (ftruncate(memfd, REC_RAM_SIZE) < 0) {
		printf("Error in call to ftruncate(), could not get %uMB of recRAM\n", REC_RAM_SIZE/(1024*1024));
		success = false;
		goto exit;
	}

	// Map recRAM to start at fixed virtual address specified in mem_mapping.h
	//  (usually 0x2000_0000)
	mmap_retval = mmap((void*)REC_RAM_VADDR, REC_RAM_SIZE,
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0);
	if (mmap_retval == MAP_FAILED) {
		printf("Error: mmap() to %p of %uMB failed.\n", (void*)(REC_RAM_VADDR), REC_RAM_SIZE/(1024*1024));
		success = false;
		goto exit;
	}
	l_rec_mem_mapped = true;

	// Create three mirrors of the recRAM, to reflect PS1's RAM mirroring
	mmap_retval = mmap((void*)(REC_RAM_VADDR+(REC_RAM_SIZE*1)), REC_RAM_SIZE,
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0);
	if (mmap_retval == MAP_FAILED) {
		printf("Error: creating 1st mmap() mirror of recRAM failed.\n");
		goto exit;
	}
	mmap_retval = mmap((void*)(REC_RAM_VADDR+(REC_RAM_SIZE*2)), REC_RAM_SIZE,
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0);
	if (mmap_retval == MAP_FAILED) {
		printf("Error: creating 2nd mmap() mirror of recRAM failed.\n");
		goto exit;
	}
	mmap_retval = mmap((void*)(REC_RAM_VADDR+(REC_RAM_SIZE*3)), REC_RAM_SIZE,
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0);
	if (mmap_retval == MAP_FAILED) {
		printf("Error: creating 3rd mmap() mirror of recRAM failed.\n");
		goto exit;
	}
	l_recRAM_mirrored = true;
	printf(" ..success!\n");

	printf("Mapping %uKB recROM using mmap\n", REC_ROM_SIZE/1024);
	// Map recROM to start at offset past recRAM that matches PSX mapping,
	//  i.e. if recRAM starts at 0x2000_0000, 512KB ROM region will be at
	//  0x2000_0000 + 0x00c0_0000 (assuming 4-byte pointer size). Note that
	//  only the lower 24 bits of the PS1 PC value is used as offset. This
	//  allows future 64-bit platforms to use the same virtual mappings
	//  while using a larger offset multiplier that 8-byte pointers require,
	//  i.e. 0x2000_0000 + (0x00c0_0000 * 2).
	mmap_retval = mmap((void*)REC_ROM_VADDR, REC_ROM_SIZE,
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED|MAP_ANONYMOUS, -1, 0);
	if (mmap_retval == MAP_FAILED) {
		printf("Error: mmap() to %p of %uKB failed.\n",
				(void*)REC_ROM_VADDR, REC_ROM_SIZE/1024);
		success = false;
		goto exit;
	}
	printf(" ..success!\n");

exit:
	if (!success) {
		// Oops, couldn't do everything we wanted to do
		perror(__func__);
		printf("ERROR: Failed to map/mirror recRAM/recROM, falling back to malloc().\n"
			   "Dynarec will use slower block dispatch loop & code lookups.\n");

		// Abandon any mappings that were created
		if (l_rec_mem_mapped) {
			// Unmap 2MB recRAM and its three mirrors (if mirrors got created)
			munmap((void*)REC_RAM_VADDR, l_recRAM_mirrored ? REC_RAM_SIZE*4 : REC_RAM_SIZE);
		}
	}

	// Close/unlink file: RAM is released when munmap()'ed or pid terminates
#ifdef SHMEM_MIRRORING
	shm_unlink(mem_fname);
#else
	if (memfd >= 0)
		close(memfd);
	unlink(mem_fname);
#endif

	return success ? 0 : -1;
}

void rec_munmap_rec_mem()
{
	// Unmap recRAM code ptr array and its three mirrors (hence REC_RAM_SIZE*4)
	munmap((void*)REC_RAM_VADDR, REC_RAM_SIZE*4);

	// Unmap recROM code ptr array
	munmap((void*)REC_ROM_VADDR, REC_ROM_SIZE);
}

#else

/* Stub funcs to call when mmap/mirroring is not supported on a platform. */

// psxMemInit() will be left to allocate psxM,psxP,psxH on its own.
int rec_mmap_psx_mem()
{
#warning "Neither SHMEM_MIRRORING nor TMPFS_MIRRORING are defined! Dynarec will emit slower code for loads/stores. Check your Makefile!"
	printf("WARNING: Neither SHMEM_MIRRORING nor TMPFS_MIRRORING were defined at\n"
	       "         compile-time! Dynarec will emit slower code for loads/stores.\n");
	return -1;
}

void rec_munmap_psx_mem()
{
}

// Dynarec will be forced to use psxRecLUT[] as further layer of indirection
//  when looking up code block pointers.
int rec_mmap_rec_mem()
{
#warning "Neither SHMEM_MIRRORING nor TMPFS_MIRRORING are defined! Dynarec will use slower block dispatch loop. Check your Makefile!"
	printf("WARNING: Neither SHMEM_MIRRORING nor TMPFS_MIRRORING were defined at\n"
	       "         compile-time! Dynarec will use slower block dispatch loop.\n");
	return -1;
}

void rec_munmap_rec_mem()
{
}

#endif // defined(SHMEM_MIRRORING) || defined(TMPFS_MIRRORING)
