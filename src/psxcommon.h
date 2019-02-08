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
* This file contains common definitions and includes for all parts of the 
* emulator core.
*/

#ifndef __PSXCOMMON_H__
#define __PSXCOMMON_H__

/* System includes */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <assert.h>

/* Port includes */
#include "port.h"

/* Define types */
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef intptr_t sptr;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t uptr;

typedef uint8_t boolean;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "1.9"
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif

enum {
	PSXTYPE_NTSC = 0,
	PSXTYPE_PAL  = 1
};


// SPU update frequency  (accomodates slow devices)
// Values used for Config.SpuUpdateFreq
enum {
	SPU_UPDATE_FREQ_MIN = 0,
	SPU_UPDATE_FREQ_1   = 0,
	SPU_UPDATE_FREQ_2   = 1,
	SPU_UPDATE_FREQ_4   = 2,
	SPU_UPDATE_FREQ_8   = 3,
	SPU_UPDATE_FREQ_16  = 4,
	SPU_UPDATE_FREQ_32  = 5,
	SPU_UPDATE_FREQ_MAX = SPU_UPDATE_FREQ_32
};

#ifndef SPU_UPDATE_FREQ_DEFAULT
#define SPU_UPDATE_FREQ_DEFAULT SPU_UPDATE_FREQ_1
#endif

// Forced XA audio update frequency (accomodates slow devices)
// Values used for Config.ForcedXAUpdates
enum {
	FORCED_XA_UPDATES_MIN     = 0,
	FORCED_XA_UPDATES_OFF     = 0,
	FORCED_XA_UPDATES_AUTO    = 1,
	FORCED_XA_UPDATES_2       = 2,
	FORCED_XA_UPDATES_4       = 3,
	FORCED_XA_UPDATES_8       = 4,
	FORCED_XA_UPDATES_16      = 5,
	FORCED_XA_UPDATES_32      = 6,
	FORCED_XA_UPDATES_MAX     = FORCED_XA_UPDATES_32
};

#ifndef FORCED_XA_UPDATES_DEFAULT
#define FORCED_XA_UPDATES_DEFAULT FORCED_XA_UPDATES_OFF
#endif

enum {
	FRAMESKIP_MIN  = -1,
	FRAMESKIP_AUTO = -1,
	FRAMESKIP_OFF  = 0,
	FRAMESKIP_MAX  = 3
};

typedef struct {
	char Mcd1[MAXPATHLEN];
	char Mcd2[MAXPATHLEN];
	char Bios[MAXPATHLEN];
	char BiosDir[MAXPATHLEN];
	char LastDir[MAXPATHLEN];
	char PatchesDir[MAXPATHLEN];  // PPF patch files
	boolean Xa; /* 0=XA enabled, 1=XA disabled */
	boolean Mdec; /* 0=Black&White Mdecs Only Disabled, 1=Black&White Mdecs Only Enabled */
	boolean PsxAuto; /* 1=autodetect system (pal or ntsc) */
	boolean Cdda; /* 0=Enable Cd audio, 1=Disable Cd audio */
	boolean HLE; /* 1=HLE, 0=bios */
	boolean SlowBoot; /* 0=skip bios logo sequence on boot  1=show sequence (does not apply to HLE) */
	boolean RCntFix; /* 1=Parasite Eve 2, Vandal Hearts 1/2 Fix */
	boolean VSyncWA; /* 1=InuYasha Sengoku Battle Fix */
	u8 Cpu; /* 0=recompiler, 1=interpreter */
	u8 PsxType; /* 0=ntsc, 1=pal */

	//senquack - added Config.SpuIrq option from PCSX Rearmed/Reloaded:
	boolean SpuIrq; /* 1=SPU IRQ always enabled (needed for audio in some games) */

	//senquack - Added audio syncronization option; if audio buffer is full,
	//           main thread blocks
	boolean SyncAudio;

	s8      SpuUpdateFreq; // Frequency of SPU updates
	                       // 0: once per frame  1: twice per frame etc
	                       // (Use SPU_UPDATE_FREQ_* enum to set)

	//senquack - Added option to allow queuing CDREAD_INT interrupts sooner
	//           than they'd normally be issued when SPU's XA buffer is not
	//           full. This fixes droupouts in music/speech on slow devices.
	s8      ForcedXAUpdates;

	boolean ShowFps;     // Show FPS
	boolean FrameLimit;  // Limit to NTSC/PAL framerate

	s8      FrameSkip;	// -1: AUTO  0: OFF  1-3: FIXED

	// Options for performance monitor
	boolean PerfmonConsoleOutput;
	boolean PerfmonDetailedStats;
	// add new ...
	boolean AnalogArrow;
} PcsxConfig;

extern PcsxConfig Config;

/////////////////////////////
// Savestate file handling //
/////////////////////////////
struct PcsxSaveFuncs {
	void *(*open)(const char *name, boolean writing);
	int   (*read)(void *file, void *buf, u32 len);
	int   (*write)(void *file, const void *buf, u32 len);
	long  (*seek)(void *file, long offs, int whence);
	int   (*close)(void *file);

#if !(defined(_WIN32) && !defined(__CYGWIN__))
	int   fd;         // The fd we receive from OS's open()
	int   lib_fd;     // The dupe'd fd we tell compression lib to use
#endif
};

// Defined in misc.cpp:
#ifdef _cplusplus
extern "C" {
#endif
enum FreezeMode {
	FREEZE_LOAD = 0,
	FREEZE_SAVE = 1,
	FREEZE_INFO = 2    // Query plugin for amount of ram to allocate for freeze
};
int freeze_rw(void *file, enum FreezeMode mode, void *buf, unsigned len);
#ifdef _cplusplus
}
#endif

extern struct PcsxSaveFuncs SaveFuncs;

#define BIAS	2
#define PSXCLK	33868800	/* 33.8688 Mhz */

enum {
	PSX_TYPE_NTSC = 0,
	PSX_TYPE_PAL
}; // PSX Types

enum {
	CPU_DYNAREC = 0,
	CPU_INTERPRETER
}; // CPU Types

void EmuUpdate();

#endif /* __PSXCOMMON_H__ */
