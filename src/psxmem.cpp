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
#ifndef WIN32
  #include <sys/mman.h>
#endif


/* This is used for direct writes in mips recompiler */
#if defined(PSXREC) && \
  (defined(SHMEM_MIRRORING) || defined(TMPFS_MIRRORING))
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/types.h>
  #ifdef SHMEM_MIRRORING
    #include <sys/shm.h>   // For Posix shared mem
  #endif
  bool psxM_mirrored = false;
  static bool psxM_mapped = false;
  static bool psxM_lower_mirror = false;
  static bool psxM_upper_mirror = false;
  static s8*  mmap_psxM();
  static void munmap_psxM();
#endif


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

int psxMemInit()
{
  int i;

  psxMemRLUT = (u8 **)malloc(0x10000 * sizeof(void *));
  psxMemWLUT = (u8 **)malloc(0x10000 * sizeof(void *));
  memset(psxMemRLUT, 0, 0x10000 * sizeof(void *));
  memset(psxMemWLUT, 0, 0x10000 * sizeof(void *));

#if defined(PSXREC) && \
	(defined(SHMEM_MIRRORING) || defined(TMPFS_MIRRORING))
  psxM = mmap_psxM();
#endif

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
      psxNULLread == NULL)
  {
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

void psxMemReset()
{
  DIR *dirstream = NULL;
  struct dirent *direntry;
  boolean biosfound = FALSE;
  FILE *f = NULL;
  char bios[MAXPATHLEN];

  memset(psxM, 0, 0x200000);
  memset(psxP, 0, 0x10000);
  memset(psxR, 0, 0x80000);    // Bios memory

  if (Config.HLE==FALSE)
  {
    dirstream = opendir(Config.BiosDir);
    if (dirstream == NULL)
    {
      printf("Could not open BIOS directory: \"%s\". Enabling HLE Bios!\n", Config.BiosDir);
      Config.HLE = TRUE;
      return;
    }

    while ((direntry = readdir(dirstream)))
    {
      if (!strcasecmp(direntry->d_name, Config.Bios))
      {
        if (snprintf(bios, MAXPATHLEN, "%s/%s", Config.BiosDir, direntry->d_name) >= MAXPATHLEN)
          continue;

        f = fopen(bios, "rb");

        if (f == NULL)
        {
          continue;
        }
        else
        {
          size_t bytes_read, bytes_expected = 0x80000;
          bytes_read = fread(psxR, 1, bytes_expected, f);
          if (bytes_read == 0)
          {
            printf("Error: skipping empty BIOS file %s!\n", bios);
            fclose(f);
            continue;
          }
          else if (bytes_read < bytes_expected)
          {
            printf("Warning: size of BIOS file %s is smaller than expected!\n", bios);
            printf("Expected %zu bytes and got only %zu\n", bytes_expected, bytes_read);
          }
          else
          {
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

    if (!biosfound)
    {
      printf("Could not locate BIOS: \"%s\". Enabling HLE BIOS!\n", Config.Bios);
      Config.HLE = TRUE;
    }
  }

  if (Config.HLE)
    printf("Using HLE emulated BIOS functions. Expect incompatibilities.\n");
}

void psxMemShutdown()
{
#if defined(PSXREC) && \
	(defined(SHMEM_MIRRORING) || defined(TMPFS_MIRRORING))
  munmap_psxM();
#endif

  free(psxM);
  psxM = NULL;
  free(psxP);
  psxP = NULL;
  free(psxH);
  psxH = NULL;
  free(psxR);
  psxR = NULL;
  free(psxMemRLUT);
  psxMemRLUT = NULL;
  free(psxMemWLUT);
  psxMemWLUT = NULL;
  free(psxNULLread);
  psxNULLread = NULL;
}

u8 psxMemRead8(u32 mem)
{
  u8 ret;
  u32 t = mem >> 16;
  u32 m = mem & 0xffff;
  if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80)
  {
    if (m < 0x400)
      ret = psxHu8(mem);
    else
      ret = psxHwRead8(mem);
  }
  else
  {
    u8 *p = (u8*)(psxMemRLUT[t]);
    if (p != NULL)
    {
      return *(u8*)(p + m);
    }
    else
    {
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
  u16 ret;
  u32 t = mem >> 16;
  u32 m = mem & 0xffff;
  if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80)
  {
    if (m < 0x400)
      ret = psxHu16(mem);
    else
      ret = psxHwRead16(mem);
  }
  else
  {
    u8 *p = (u8*)(psxMemRLUT[t]);
    if (p != NULL)
    {
      ret = SWAPu16(*(u16*)(p + m));
    }
    else
    {
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
  u32 ret;
  u32 t = mem >> 16;
  u32 m = mem & 0xffff;
  if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80)
  {
    if (m < 0x400)
      ret = psxHu32(mem);
    else
      ret = psxHwRead32(mem);
  }
  else
  {
    u8 *p = (u8*)(psxMemRLUT[t]);
    if (p != NULL)
    {
      ret = SWAPu32(*(u32*)(p + m));
    }
    else
    {
#ifdef PSXMEM_LOG
      if (psxRegs.writeok)
      {
        PSXMEM_LOG("err lw %8.8lx\n", mem);
      }
#endif
      ret = 0;
    }
  }

  return ret;
}

void psxMemWrite8(u32 mem, u8 value)
{
  u32 t = mem >> 16;
  u32 m = mem & 0xffff;
  if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80)
  {
    if (m < 0x400)
      psxHu8(mem) = value;
    else
      psxHwWrite8(mem, value);
  }
  else
  {
    u8 *p = (u8*)(psxMemWLUT[t]);
    if (p != NULL)
    {
      *(u8*)(p + m) = value;
#ifdef PSXREC
      psxCpu->Clear((mem & (~3)), 1);
#endif
    }
    else
    {
#ifdef PSXMEM_LOG
      PSXMEM_LOG("err sb %8.8lx\n", mem);
#endif
    }
  }
}

void psxMemWrite16(u32 mem, u16 value)
{
  u32 t = mem >> 16;
  u32 m = mem & 0xffff;
  if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80)
  {
    if (m < 0x400)
      psxHu16ref(mem) = SWAPu16(value);
    else
      psxHwWrite16(mem, value);
  }
  else
  {
    u8 *p = (u8*)(psxMemWLUT[t]);
    if (p != NULL)
    {
      *(u16*)(p + m) = SWAPu16(value);
#ifdef PSXREC
      psxCpu->Clear((mem & (~3)), 1);
#endif
    }
    else
    {
#ifdef PSXMEM_LOG
      PSXMEM_LOG("err sh %8.8lx\n", mem);
#endif
    }
  }
}

void psxMemWrite32(u32 mem, u32 value)
{
  u32 t = mem >> 16;
  u32 m = mem & 0xffff;
  if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80)
  {
    if (m < 0x400)
      psxHu32ref(mem) = SWAPu32(value);
    else
      psxHwWrite32(mem, value);
  }
  else
  {
    u8 *p = (u8*)(psxMemWLUT[t]);
    if (p != NULL)
    {
      *(u32*)(p + m) = SWAPu32(value);
#ifdef PSXREC
      psxCpu->Clear(mem, 1);
#endif
    }
    else
    {
      if (mem != 0xfffe0130)
      {
#ifdef PSXREC
        if (!psxRegs.writeok) psxCpu->Clear(mem, 1);
#endif

#ifdef PSXMEM_LOG
        if (psxRegs.writeok)
        {
          PSXMEM_LOG("err sw %8.8lx\n", mem);
        }
#endif
      }
      else
      {
        // Write to cache control port 0xfffe0130
        switch (value)
        {
          case 0x800:
          case 0x804:
            if (psxRegs.writeok == 0) break;
            psxRegs.writeok = 0;
            memset(psxMemWLUT + 0x0000, 0, 0x80 * sizeof(void *));
            memset(psxMemWLUT + 0x8000, 0, 0x80 * sizeof(void *));
            memset(psxMemWLUT + 0xa000, 0, 0x80 * sizeof(void *));
            break;
          case 0x00:
          case 0x1e988:
            if (psxRegs.writeok == 1) break;
            psxRegs.writeok = 1;
            for (int i = 0; i < 0x80; i++) psxMemWLUT[i + 0x0000] = (u8*)&psxM[(i & 0x1f) << 16];
            memcpy(psxMemWLUT + 0x8000, psxMemWLUT, 0x80 * sizeof(void *));
            memcpy(psxMemWLUT + 0xa000, psxMemWLUT, 0x80 * sizeof(void *));
            break;
          default:
#ifdef PSXMEM_LOG
            PSXMEM_LOG("unk %8.8lx = %x\n", mem, value);
#endif
            break;
        }
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// -BEGIN- psxM allocation and mirroring via mmap()
// This is used for direct writes in mips recompiler
///////////////////////////////////////////////////////////////////////////////
#if defined(PSXREC) && \
	(defined(SHMEM_MIRRORING) || defined(TMPFS_MIRRORING))

s8* mmap_psxM()
{
  // Already mapped?
  if (psxM_mapped)
    return psxM;

  bool  success = true;
  int   memfd = -1;
  s8*   ptr = NULL;
  void* mmap_retval = NULL;

#ifdef SHMEM_MIRRORING
  // Get a POSIX shared memory object fd
  printf("Mapping/mirroring 2MB PSX RAM using POSIX shared mem\n");
  const char* mem_fname = "/pcsx4all_psxmem";
  memfd = shm_open(mem_fname, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
#else
  // Use tmpfs file - TMPFS_DIR string literal should be defined in Makefile
  //  CFLAGS with escaped quotes (alter if needed): -DTMPFS_DIR=\"/tmp\"
  const char* mem_fname = TMPFS_DIR "/pcsx4all_psxmem";
  printf("Mapping/mirroring 2MB PSX RAM using tmpfs file %s\n", mem_fname);
  memfd = open(mem_fname, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
#endif

  if (memfd < 0)
  {
#ifdef SHMEM_MIRRORING
    printf("Error acquiring POSIX shared memory file descriptor\n");
#else
    printf("Error creating tmpfs file: %s\n", mem_fname);
#endif
    success = false;
    goto exit;
  }

  // We want 2MB of PSX RAM
  if (ftruncate(memfd, 0x200000) < 0)
  {
    printf("Error in call to ftruncate(), could not get 2MB of PSX RAM\n");
    success = false;
    goto exit;
  }

  // Map PSX RAM to start at fixed virtual address 0x1000_0000
  mmap_retval = mmap((void*)0x10000000, 0x200000,
                     PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0);
  if (mmap_retval == MAP_FAILED)
  {
    printf("Warning: mmap() to 0x10000000 of 2MB PSX RAM mmap fd failed.\n"
           "Dynarec will emit slower code for direct writes.\n");
    success = false;
    goto exit;
  }
  ptr = (s8*)mmap_retval;
  psxM_mapped = true;

  // Mirror upper 64KB PSX RAM to the fixed virtual region before psxM[].
  //  This allows recompiler to skip special-casing certain loads/stores
  //  of/to immediate($reg) address, where $reg is a value near a RAM
  //  mirror-region boundary and the immediate is a negative value large
  //  enough to cross to the region before it, i.e. (-16)(0x8020_0000).
  //  This occurs in games like Einhander. Normally, 0x8020_0000 on a PSX
  //  would be referencing the beginning of a 2MB mirror in the KSEG0 cached
  //  mirror region. After emu code masks the address, it maps to address
  //  0, and accessing -16(&psxM[0]) would be out-of-bounds.
  //  The mirror here maps it to &psxM[1ffff0], like a real PSX.

  mmap_retval = mmap((void*)(ptr-0x10000), 0x10000,
                     PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0x200000-0x10000);
  if (mmap_retval == MAP_FAILED)
  {
    printf("Warning: creating mmap() mirror of upper 64KB of PSX RAM failed.\n");
  }
  else
  {
    psxM_upper_mirror = true;
  }

  // And, for correctness's sake, mirror lower 64K of PSX RAM to region after
  //  psxM[], though in practice it's unknown if any games truly need this.
  mmap_retval = mmap((void*)(ptr+0x200000), 0x10000,
                     PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, memfd, 0);
  if (mmap_retval == MAP_FAILED)
  {
    printf("Warning: creating mmap() mirror of lower 64KB of PSX RAM failed.\n");
  }
  else
  {
    psxM_lower_mirror = true;
  }

exit:
  if (success)
  {
    psxM_mirrored = (psxM_lower_mirror && psxM_upper_mirror);
    printf(" ..success!\n");
  }
  else
  {
    perror(__func__);
    printf("ERROR: Failed to mmap() 2MB PSX RAM, falling back to malloc()\n");
    ptr = NULL;
  }

  // Close/unlink file: RAM is released when munmap()'ed or pid terminates
#ifdef SHMEM_MIRRORING
  shm_unlink(mem_fname);
#else
  if (memfd >= 0)
    close(memfd);
  unlink(mem_fname);
#endif

  psxM_mapped = success;
  return ptr;
}

void munmap_psxM()
{
  if (!psxM_mapped)
    return;

  if (psxM_upper_mirror)
    munmap((void*)((u8*)psxM-0x10000), 0x10000);  // Unmap mirror of upper 64KB
  if (psxM_lower_mirror)
    munmap((void*)((u8*)psxM+0x200000), 0x10000); // Unmap mirror of lower 64KB
  munmap((void*)psxM, 0x200000);
  psxM_mapped = psxM_mirrored = psxM_upper_mirror = psxM_lower_mirror = false;
  psxM = NULL;
}

#endif
///////////////////////////////////////////////////////////////////////////////
// -END- psxM allocation and mirroring via mmap()
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//NOTE: Following *_direct() are old funcs used by unmaintained ARM dynarecs
///////////////////////////////////////////////////////////////////////////////
u8 psxMemRead8_direct(u32 mem,void *_regs)
{
  const psxRegisters *regs=(psxRegisters *)_regs;
  u32 m = mem & 0xffff;
  switch(mem>>16)
  {
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

u16 psxMemRead16_direct(u32 mem,void *_regs)
{
  const psxRegisters *regs=(psxRegisters *)_regs;
  u32 m = mem & 0xffff;
  switch(mem>>16)
  {
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

u32 psxMemRead32_direct(u32 mem,void *_regs)
{
  const psxRegisters *regs=(psxRegisters *)_regs;
  u32 m = mem & 0xffff;
  switch(mem>>16)
  {
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

void psxMemWrite8_direct(u32 mem, u8 value,void *_regs)
{
  const psxRegisters *regs=(psxRegisters *)_regs;
  u32 m = mem & 0xffff;
  switch(mem>>16)
  {
    case 0x1f80:
      if (m<0x1000) *((u8 *)&regs->psxH[m]) = value;
      else psxHwWrite8(mem,value);
      break;
    default:
      *((u8 *)&regs->psxP[m]) = value;
      break;
  }
}

void psxMemWrite16_direct(u32 mem, u16 value,void *_regs)
{
  const psxRegisters *regs=(psxRegisters *)_regs;
  u32 m = mem & 0xffff;
  switch(mem>>16)
  {
    case 0x1f80:
      if (m<0x1000) *((u16 *)&regs->psxH[m]) = value;
      else psxHwWrite16(mem,value);
      break;
    default:
      *((u16 *)&regs->psxP[m]) = value;
  }
}

void psxMemWrite32_direct(u32 mem, u32 value,void *_regs)
{
  const psxRegisters *regs=(psxRegisters *)_regs;
  u32 m = mem & 0xffff;
  switch(mem>>16)
  {
    case 0x1f80:
      if (m<0x1000) *((u32 *)&regs->psxH[m]) = value;
      else psxHwWrite32(mem,value);
      break;
    default:
      *((u32 *)&regs->psxP[m]) = value;
  }
}
