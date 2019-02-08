/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *   schultz.ryan@gmail.com, http://rschultz.ath.cx/code.php               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA.            *
 ***************************************************************************/

/*
* PSX memory functions.
*/

#include <sys/types.h>
#include <dirent.h>

#include "psxmem.h"
#include "r3000a.h"
#include "psxhw.h"


/* Memory statistics (for development purposes) */

//#define DEBUG_MEM_STATS

enum MemstatType   { MEMSTAT_TYPE_READ, MEMSTAT_TYPE_WRITE, MEMSTAT_TYPE_COUNT };
enum MemstatWidth  { MEMSTAT_WIDTH_8, MEMSTAT_WIDTH_16, MEMSTAT_WIDTH_32, MEMSTAT_WIDTH_COUNT };
enum MemstatRegion { MEMSTAT_REGION_ANY, MEMSTAT_REGION_RAM, MEMSTAT_REGION_BLOCKED,
                     MEMSTAT_REGION_PPORT, MEMSTAT_REGION_SCRATCHPAD, MEMSTAT_REGION_HW,
                     MEMSTAT_REGION_ROM, MEMSTAT_REGION_CACHE, MEMSTAT_REGION_COUNT };
#ifdef DEBUG_MEM_STATS
static void memstats_reset();
static void memstats_print();
static void memstats_add_read(u32 addr, MemstatWidth width);
static void memstats_add_write(u32 addr, MemstatWidth width);
#else
static inline void memstats_reset() {}
static inline void memstats_print() {}
static inline void memstats_add_read(u32 addr, MemstatWidth width) {}
static inline void memstats_add_write(u32 addr, MemstatWidth width) {}
#endif // DEBUG_MEM_STATS


s8 *psxM = NULL;
s8 *psxP = NULL;
s8 *psxR = NULL;
s8 *psxH = NULL;

u8 **psxMemWLUT = NULL;
u8 **psxMemRLUT = NULL;
u8 *psxNULLread=NULL;

/*  Playstation Memory Map (from Playstation doc by Joshua Walker)
0x0000_0000-0x0000_ffff		Kernel (64K)	
0x0001_0000-0x001f_ffff		User Memory (1.9 Meg)	

0x1f00_0000-0x1f00_ffff		Parallel Port (64K)	

0x1f80_0000-0x1f80_03ff		Scratch Pad (1024 bytes)	

0x1f80_1000-0x1f80_2fff		Hardware Registers (8K)	

0x8000_0000-0x801f_ffff		Kernel and User Memory Mirror (2 Meg) Cached	

0xa000_0000-0xa01f_ffff		Kernel and User Memory Mirror (2 Meg) Uncached	

0xbfc0_0000-0xbfc7_ffff		BIOS (512K)
*/

int psxMemInit() {
	int i;

	psxMemRLUT = (u8 **)malloc(0x10000 * sizeof(void *));
	psxMemWLUT = (u8 **)malloc(0x10000 * sizeof(void *));
	memset(psxMemRLUT, 0, 0x10000 * sizeof(void *));
	memset(psxMemWLUT, 0, 0x10000 * sizeof(void *));

	if (psxM == NULL)
		psxM = (s8 *)malloc(0x200000);

	// Allocate 64K each for 0x1f00_0000 and 0x1f80_0000 regions
	if (psxP == NULL)
		psxP = (s8 *)malloc(0x10000);
	if (psxH == NULL)
		psxH = (s8 *)malloc(0x10000);

	if (psxR == NULL)
		psxR = (s8 *)malloc(0x80000);

	if (psxNULLread == NULL)
		psxNULLread=(u8*)malloc(0x10000);

	memset(psxNULLread, 0, 0x10000);
	
	if (psxMemRLUT == NULL || psxMemWLUT == NULL || 
		psxM == NULL || psxP == NULL || psxH == NULL ||
		psxNULLread == NULL) {
		printf("Error allocating memory!");
		return -1;
	}

// MemR
	for (i = 0; i< 0x10000; i++) psxMemRLUT[i]=psxNULLread;
	for (i = 0; i < 0x80; i++) psxMemRLUT[i + 0x0000] = (u8 *)&psxM[(i & 0x1f) << 16];

	memcpy(psxMemRLUT + 0x8000, psxMemRLUT, 0x80 * sizeof(void *));
	memcpy(psxMemRLUT + 0xa000, psxMemRLUT, 0x80 * sizeof(void *));

	psxMemRLUT[0x1f00] = (u8 *)psxP;
	psxMemRLUT[0x1f80] = (u8 *)psxH;

	for (i = 0; i < 0x08; i++) psxMemRLUT[i + 0x1fc0] = (u8 *)&psxR[i << 16];

	memcpy(psxMemRLUT + 0x9fc0, psxMemRLUT + 0x1fc0, 0x08 * sizeof(void *));
	memcpy(psxMemRLUT + 0xbfc0, psxMemRLUT + 0x1fc0, 0x08 * sizeof(void *));

// MemW
	for (i = 0; i < 0x80; i++) psxMemWLUT[i + 0x0000] = (u8 *)&psxM[(i & 0x1f) << 16];
	memcpy(psxMemWLUT + 0x8000, psxMemWLUT, 0x80 * sizeof(void *));
	memcpy(psxMemWLUT + 0xa000, psxMemWLUT, 0x80 * sizeof(void *));

	psxMemWLUT[0x1f00] = (u8 *)psxP;
	psxMemWLUT[0x1f80] = (u8 *)psxH;

	return 0;
}

void psxMemReset() {
	DIR *dirstream = NULL;
	struct dirent *direntry;
	boolean biosfound = FALSE;
	FILE *f = NULL;
	char bios[MAXPATHLEN];

	memstats_reset();

	memset(psxM, 0, 0x200000);
	memset(psxP, 0, 0x10000);
	memset(psxR, 0, 0x80000);    // Bios memory

	if (Config.HLE==FALSE) {
		dirstream = opendir(Config.BiosDir);
		if (dirstream == NULL) {
			printf("Could not open BIOS directory: \"%s\". Enabling HLE Bios!\n", Config.BiosDir);
			Config.HLE = TRUE;
			return;
		}

		while ((direntry = readdir(dirstream))) {
			if (!strcasecmp(direntry->d_name, Config.Bios)) {
				if (snprintf(bios, MAXPATHLEN, "%s/%s", Config.BiosDir, direntry->d_name) >= MAXPATHLEN)
					continue;

				f = fopen(bios, "rb");

				if (f == NULL) {
					continue;
				} else {
					size_t bytes_read, bytes_expected = 0x80000;
					bytes_read = fread(psxR, 1, bytes_expected, f);
					if (bytes_read == 0) {
						printf("Error: skipping empty BIOS file %s!\n", bios);
						fclose(f);
						continue;
					} else if (bytes_read < bytes_expected) {
						printf("Warning: size of BIOS file %s is smaller than expected!\n", bios);
						printf("Expected %zu bytes and got only %zu\n", bytes_expected, bytes_read);
					} else {
						printf("Loaded BIOS image: %s\n", bios);
					}

					fclose(f);
					Config.HLE = FALSE;
					biosfound = TRUE;
					break;
				}
			}
		}
		closedir(dirstream);

		if (!biosfound) {
			printf("Could not locate BIOS: \"%s\". Enabling HLE BIOS!\n", Config.Bios);
			Config.HLE = TRUE;
		}
	}

	if (Config.HLE)
		printf("Using HLE emulated BIOS functions. Expect incompatibilities.\n");
}

void psxMemShutdown()
{
	free(psxM);         psxM = NULL;
	free(psxP);         psxP = NULL;
	free(psxH);         psxH = NULL;
	free(psxR);         psxR = NULL;
	free(psxMemRLUT);   psxMemRLUT = NULL;
	free(psxMemWLUT);   psxMemWLUT = NULL;
	free(psxNULLread);  psxNULLread = NULL;

	memstats_print();
}

u8 psxMemRead8(u32 mem)
{
	memstats_add_read(mem, MEMSTAT_WIDTH_8);
	u8 ret;
	u32 t = mem >> 16;
	u32 m = mem & 0xffff;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if (m < 0x400)
			ret = psxHu8(mem);
		else
			ret = psxHwRead8(mem);
	} else {
		u8 *p = (u8*)(psxMemRLUT[t]);
		if (p != NULL) {
			return *(u8*)(p + m);
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err lb %8.8lx\n", mem);
#endif
			ret = 0;
		}
	}

	return ret;
}

u16 psxMemRead16(u32 mem)
{
	memstats_add_read(mem, MEMSTAT_WIDTH_16);
	u16 ret;
	u32 t = mem >> 16;
	u32 m = mem & 0xffff;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if (m < 0x400)
			ret = psxHu16(mem);
		else
			ret = psxHwRead16(mem);
	} else {
		u8 *p = (u8*)(psxMemRLUT[t]);
		if (p != NULL) {
			ret = SWAPu16(*(u16*)(p + m));
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err lh %8.8lx\n", mem);
#endif
			ret = 0;
		}
	}

	return ret;
}

u32 psxMemRead32(u32 mem)
{
	memstats_add_read(mem, MEMSTAT_WIDTH_32);
	u32 ret;
	u32 t = mem >> 16;
	u32 m = mem & 0xffff;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if (m < 0x400)
			ret = psxHu32(mem);
		else
			ret = psxHwRead32(mem);
	} else {
		u8 *p = (u8*)(psxMemRLUT[t]);
		if (p != NULL) {
			ret = SWAPu32(*(u32*)(p + m));
		} else {
#ifdef PSXMEM_LOG
			if (psxRegs.writeok) { PSXMEM_LOG("err lw %8.8lx\n", mem); }
#endif
			ret = 0;
		}
	}

	return ret;
}

void psxMemWrite8(u32 mem, u8 value)
{
	memstats_add_write(mem, MEMSTAT_WIDTH_8);
	u32 t = mem >> 16;
	u32 m = mem & 0xffff;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if (m < 0x400)
			psxHu8(mem) = value;
		else
			psxHwWrite8(mem, value);
	} else {
		u8 *p = (u8*)(psxMemWLUT[t]);
		if (p != NULL) {
			*(u8*)(p + m) = value;
#ifdef PSXREC
			psxCpu->Clear((mem & (~3)), 1);
#endif
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err sb %8.8lx\n", mem);
#endif
		}
	}
}

void psxMemWrite16(u32 mem, u16 value)
{
	memstats_add_write(mem, MEMSTAT_WIDTH_16);
	u32 t = mem >> 16;
	u32 m = mem & 0xffff;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if (m < 0x400)
			psxHu16ref(mem) = SWAPu16(value);
		else
			psxHwWrite16(mem, value);
	} else {
		u8 *p = (u8*)(psxMemWLUT[t]);
		if (p != NULL) {
			*(u16*)(p + m) = SWAPu16(value);
#ifdef PSXREC
			psxCpu->Clear((mem & (~3)), 1);
#endif
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err sh %8.8lx\n", mem);
#endif
		}
	}
}

void psxMemWrite32(u32 mem, u32 value)
{
	memstats_add_write(mem, MEMSTAT_WIDTH_32);
	u32 t = mem >> 16;
	u32 m = mem & 0xffff;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if (m < 0x400)
			psxHu32ref(mem) = SWAPu32(value);
		else
			psxHwWrite32(mem, value);
	} else {
		u8 *p = (u8*)(psxMemWLUT[t]);
		if (p != NULL) {
			*(u32*)(p + m) = SWAPu32(value);
#ifdef PSXREC
			psxCpu->Clear(mem, 1);
#endif
		} else {
			if (mem != 0xfffe0130) {
#ifdef PSXREC
				if (!psxRegs.writeok) psxCpu->Clear(mem, 1);
#endif

#ifdef PSXMEM_LOG
				if (psxRegs.writeok) { PSXMEM_LOG("err sw %8.8lx\n", mem); }
#endif
			} else {
				// Write to cache control port 0xfffe0130
				psxMemWrite32_CacheCtrlPort(value);
			}
		}
	}
}

// Write to cache control port 0xfffe0130
void psxMemWrite32_CacheCtrlPort(u32 value)
{
#ifdef PSXREC
	//   For dynarecs, they can choose to omit the check of 'writeok' before
	// each store. To allow this, when cache is isolated, we backup the first
	// 64KB of PS1 RAM. Then, the dynarec can allow all subsequent stores to go
	// to RAM without any checks. After cache control port status returns to
	// normal, i.e. cache is unisolated, we restore the backup.
	//   The stores that come after cache isolation are meant soley to
	// invalidate I-cache lines. The addresses written to are assumed to be
	// within the first 64KB of addresses. The BIOS CacheFlush() 0x44 A0 routine
	// is known to do this, and this also should work for games that use their
	// own cache-flush routines, like '007 Tomorrow Never Dies'. Because some
	// games do use their own cache-flush routines, we cannot rely merely on
	// patching BIOS code or its 0x44 CacheFlush() A0 jumptable entry.
	//   If we wanted to be paranoid, we could backup the entire 2MB of PS1 RAM
	// instead of just 64KB. But, until it is proven a single game needs this,
	// we'll stick with just 64KB.
	//   Note that we alter the psxMemRLUT[] entries for this 64KB region to
	// point to the backup while cache is isolated: this is done in case the
	// dynarec needs to recompile some code during the sequence. It will read
	// from the backed-up 64KB PS1 RAM, not from RAM overwritten by sequence.
	// Of course, this assumes the recompiler is using psxMemRLUT[] or PSXM()
	// macro to read PS1 code.   -senquack

	static u8 mem_bak[0x10000];
#endif //PSXREC

	switch (value)
	{
		case 0x800: case 0x804:
			if (psxRegs.writeok == 0) break;
			psxRegs.writeok = 0;
			memset(psxMemWLUT + 0x0000, 0, 0x80 * sizeof(void *));
			memset(psxMemWLUT + 0x8000, 0, 0x80 * sizeof(void *));
			memset(psxMemWLUT + 0xa000, 0, 0x80 * sizeof(void *));

#ifdef PSXREC
			// Cache is now isolated, pending cache-flush sequence:
			//  Backup lower 64KB of PS1 RAM, and point RLUT to it.
			memcpy(mem_bak, psxM, sizeof(mem_bak));
			psxMemRLUT[0x0000] = psxMemRLUT[0x0020] = psxMemRLUT[0x0040] = psxMemRLUT[0x0060] = mem_bak;
			psxMemRLUT[0x8000] = psxMemRLUT[0x8020] = psxMemRLUT[0x8040] = psxMemRLUT[0x8060] = mem_bak;
			psxMemRLUT[0xa000] = psxMemRLUT[0xa020] = psxMemRLUT[0xa040] = psxMemRLUT[0xa060] = mem_bak;
#endif
			break;
		case 0x00: case 0x1e988:
			if (psxRegs.writeok == 1) break;
			psxRegs.writeok = 1;
			for (int i = 0; i < 0x80; i++) psxMemWLUT[i + 0x0000] = (u8*)&psxM[(i & 0x1f) << 16];
			memcpy(psxMemWLUT + 0x8000, psxMemWLUT, 0x80 * sizeof(void *));
			memcpy(psxMemWLUT + 0xa000, psxMemWLUT, 0x80 * sizeof(void *));

#ifdef PSXREC
			// Cache is now unisolated: Restore backup of lower 64KB RAM,
			//  and point loads to their original locations
			memcpy(psxM, mem_bak, sizeof(mem_bak));
			psxMemRLUT[0x0000] = psxMemRLUT[0x0020] = psxMemRLUT[0x0040] = psxMemRLUT[0x0060] = (u8 *)psxM;
			psxMemRLUT[0x8000] = psxMemRLUT[0x8020] = psxMemRLUT[0x8040] = psxMemRLUT[0x8060] = (u8 *)psxM;
			psxMemRLUT[0xa000] = psxMemRLUT[0xa020] = psxMemRLUT[0xa040] = psxMemRLUT[0xa060] = (u8 *)psxM;
#endif
			break;
		default:
#ifdef PSXMEM_LOG
			PSXMEM_LOG("unk %8.8lx = %x\n", mem, value);
#endif
			break;
	}
}


///////////////////////////////////////////////////////////////////////////////
//NOTE: Following *_direct() are old funcs used by unmaintained ARM dynarecs
///////////////////////////////////////////////////////////////////////////////
u8 psxMemRead8_direct(u32 mem,void *_regs) {
	const psxRegisters *regs=(psxRegisters *)_regs;
	u32 m = mem & 0xffff;
	switch(mem>>16) {
		case 0x1f80:
			if (m<0x1000)
				return (*(u8*) &regs->psxH[m]);
			return psxHwRead8(mem);
		case 0x1f00:
			return (*(u8*) &regs->psxP[m]);
		default:
			m|=mem&0x70000;
			return (*(u8*) &regs->psxR[m]);
	}
}

u16 psxMemRead16_direct(u32 mem,void *_regs) {
	const psxRegisters *regs=(psxRegisters *)_regs;
	u32 m = mem & 0xffff;
	switch(mem>>16) {
		case 0x1f80:
			if (m<0x1000)
				return (*(u16*) &regs->psxH[m]);
			return psxHwRead16(mem);
		case 0x1f00:
			return (*(u16*) &regs->psxP[m]);
		default:
			m|=mem&0x70000;
			return (*(u16*) &regs->psxR[m]);
	}
}

u32 psxMemRead32_direct(u32 mem,void *_regs) {
	const psxRegisters *regs=(psxRegisters *)_regs;
	u32 m = mem & 0xffff;
	switch(mem>>16) {
		case 0x1f80:
			if (m<0x1000)
				return (*(u32*) &regs->psxH[m]);
			return psxHwRead32(mem);
		case 0x1f00:
			return (*(u32*) &regs->psxP[m]);
		default:
			m|=mem&0x70000;
			return (*(u32*) &regs->psxR[m]);
	}
}

void psxMemWrite8_direct(u32 mem, u8 value,void *_regs) {
	const psxRegisters *regs=(psxRegisters *)_regs;
	u32 m = mem & 0xffff;
	switch(mem>>16) {
		case 0x1f80:
			if (m<0x1000) *((u8 *)&regs->psxH[m]) = value;
			else psxHwWrite8(mem,value);
			break;
		default:
			*((u8 *)&regs->psxP[m]) = value;
			break;
	}
}

void psxMemWrite16_direct(u32 mem, u16 value,void *_regs) {
	const psxRegisters *regs=(psxRegisters *)_regs;
	u32 m = mem & 0xffff;
	switch(mem>>16) {
		case 0x1f80:
			if (m<0x1000) *((u16 *)&regs->psxH[m]) = value;
			else psxHwWrite16(mem,value);
			break;
		default:
			*((u16 *)&regs->psxP[m]) = value;
	}
}

void psxMemWrite32_direct(u32 mem, u32 value,void *_regs) {
	const psxRegisters *regs=(psxRegisters *)_regs;
	u32 m = mem & 0xffff;
	switch(mem>>16) {
		case 0x1f80:
			if (m<0x1000) *((u32 *)&regs->psxH[m]) = value;
			else psxHwWrite32(mem,value);
			break;
		default:
			*((u32 *)&regs->psxP[m]) = value;
	}
}


#ifdef DEBUG_MEM_STATS
///////////////////////////////////////////////////////////////////////////////
// -BEGIN- Memory statistics (for development purposes)
///////////////////////////////////////////////////////////////////////////////
long long unsigned int memstats[MEMSTAT_TYPE_COUNT][MEMSTAT_REGION_COUNT][MEMSTAT_WIDTH_COUNT];

static void memstats_reset()
{
	memset((void*)memstats, 0, sizeof(memstats));
}

static void memstats_region_print(const char *region_description, MemstatRegion region)
{
	char separator_line[81];
	strncpy(separator_line, region_description, 80);
	separator_line[80] = '\0';
	int i = strlen(separator_line);
	if (i < (sizeof(separator_line)-1))
		memset(separator_line+i, '-', sizeof(separator_line)-1-i);

	printf("%s\n"
	       "  reads:%23llu %23llu %23llu\n"
	       " writes:%23llu %23llu %23llu\n",
	       separator_line,
	       memstats[MEMSTAT_TYPE_READ][region][MEMSTAT_WIDTH_8],
	       memstats[MEMSTAT_TYPE_READ][region][MEMSTAT_WIDTH_16],
	       memstats[MEMSTAT_TYPE_READ][region][MEMSTAT_WIDTH_32],
	       memstats[MEMSTAT_TYPE_WRITE][region][MEMSTAT_WIDTH_8],
	       memstats[MEMSTAT_TYPE_WRITE][region][MEMSTAT_WIDTH_16],
	       memstats[MEMSTAT_TYPE_WRITE][region][MEMSTAT_WIDTH_32]);
}

static void memstats_print()
{
	printf("MEMORY STATS:              byte                   short                    word\n");
	memstats_region_print("BLOCKED RAM (ISOLATED CACHE)", MEMSTAT_REGION_BLOCKED);
	memstats_region_print("PPORT (ROM EXPANSION)",        MEMSTAT_REGION_PPORT);
	memstats_region_print("ROM",                          MEMSTAT_REGION_ROM);
	memstats_region_print("CACHE CTRL PORT",              MEMSTAT_REGION_CACHE);
	memstats_region_print("RAM",                          MEMSTAT_REGION_RAM);
	memstats_region_print("SCRATCHPAD",                   MEMSTAT_REGION_SCRATCHPAD);
	memstats_region_print("HW I/O",                       MEMSTAT_REGION_HW);
	memstats_region_print("TOTAL",                        MEMSTAT_REGION_ANY);
}

static inline void memstats_add_read(u32 addr, MemstatWidth width)
{
	MemstatRegion region;
	addr &= 0xfffffff;
	switch (addr >> 16) {
		case 0x0000 ... 0x007f:
			region = MEMSTAT_REGION_RAM;
			break;
		case 0x0f00 ... 0x0f7f:
			region = MEMSTAT_REGION_PPORT;
			break;
		case 0x0f80:
			if ((addr & 0xffff) < 0x0400)
				region = MEMSTAT_REGION_SCRATCHPAD;
			else
				region = MEMSTAT_REGION_HW;
			break;
		case 0x0ffe:
			region = MEMSTAT_REGION_CACHE;
			break;
		default:
			region = MEMSTAT_REGION_ROM;
			break;
	}
	memstats[MEMSTAT_TYPE_READ][region][width]++;
	memstats[MEMSTAT_TYPE_READ][MEMSTAT_REGION_ANY][width]++;
}

static inline void memstats_add_write(u32 addr, MemstatWidth width)
{
	MemstatRegion region;
	addr &= 0xfffffff;
	switch (addr >> 16) {
		case 0x0000 ... 0x007f:
			if (psxRegs.writeok)
				region = MEMSTAT_REGION_RAM;
			else
				region = MEMSTAT_REGION_BLOCKED;
			break;
		case 0x0f00 ... 0x0f7f:
			region = MEMSTAT_REGION_PPORT;
			break;
		case 0x0f80:
			if ((addr & 0xffff) < 0x0400)
				region = MEMSTAT_REGION_SCRATCHPAD;
			else
				region = MEMSTAT_REGION_HW;
			break;
		case 0x0ffe:
			region = MEMSTAT_REGION_CACHE;
			break;
		default:
			region = MEMSTAT_REGION_ROM;
			break;
	}
	memstats[MEMSTAT_TYPE_WRITE][region][width]++;
	memstats[MEMSTAT_TYPE_WRITE][MEMSTAT_REGION_ANY][width]++;
}
///////////////////////////////////////////////////////////////////////////////
// -END- Memory statistics (for development purposes)
///////////////////////////////////////////////////////////////////////////////
#endif // DEBUG_MEM_STATS
