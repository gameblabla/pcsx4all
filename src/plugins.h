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

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include "psxcommon.h"
#include "r3000a.h"

#ifndef _WIN32
#define CALLBACK
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "psemu_plugin_defs.h"
#include "decode_xa.h"

int  LoadPlugins(void);
void ReleasePlugins(void);

// GPU structures

typedef struct {
	uint32_t ulFreezeVersion;
	uint32_t ulStatus;
	uint32_t ulControl[256];
	unsigned char psxVRam[1024*512*2];
} GPUFreeze_t;

typedef struct {
	uint8_t *vram;
	uint16_t x, y, w, h, hres, vres;
	bool depth24, pal;
} GPUScreenInfo_t;

/// GPU functions

long GPU_init(void);
long GPU_shutdown(void);
void GPU_writeStatus(uint32_t);
void GPU_writeData(uint32_t);
void GPU_writeDataMem(uint32_t *, int);
uint32_t GPU_readStatus(void);
uint32_t GPU_readData(void);
void GPU_readDataMem(uint32_t *, int);
long GPU_dmaChain(uint32_t *,uint32_t);
void GPU_updateLace(void);
long GPU_freeze(uint32_t, GPUFreeze_t *);
void GPU_requestScreenRedraw(void);
void GPU_getScreenInfo(GPUScreenInfo_t *sinfo);

#ifdef USE_GPULIB
void GPU_vBlank(int is_vblank, int lcf);
#endif

// CDROM structures

struct CdrStat {
	uint32_t Type;
	uint32_t Status;
	unsigned char Time[3];
};

// Updated to newer PCSX Reloaded/Rearmed code:
struct SubQ {
	char res0[12];
	unsigned char ControlAndADR;
	unsigned char TrackNumber;
	unsigned char IndexNumber;
	unsigned char TrackRelativeAddress[3];
	unsigned char Filler;
	unsigned char AbsoluteAddress[3];
	unsigned char CRC[2];
	char res1[72];
};

// CDROM functions

long CDR_init(void);
long CDR_shutdown(void);
long CDR_open(void);
long CDR_close(void);
long CDR_getTN(unsigned char *);
long CDR_getTD(unsigned char , unsigned char *);
long CDR_readTrack(unsigned char *);
extern unsigned char *(*CDR_getBuffer)(void);
long CDR_play(unsigned char *);
long CDR_stop(void);
long CDR_getStatus(struct CdrStat *);
unsigned char *CDR_getBufferSub(void);

// SPU structures

typedef struct {
	unsigned char PluginName[8];
	uint32_t PluginVersion;
	uint32_t Size;
	unsigned char SPUPorts[0x200];
	unsigned char SPURam[0x80000];
	xa_decode_t xa;
	unsigned char *SPUInfo;
} SPUFreeze_t;

// SPU functions, should have C linkage
#ifdef __cplusplus
extern "C" {
#endif

// These two implemented in plugins.cpp for use by SPU plugins
void CALLBACK Trigger_SPU_IRQ(void);
void CALLBACK Schedule_SPU_IRQ(unsigned int cycles_after);

long CALLBACK SPUinit(void);
long CALLBACK SPUopen(void);
long CALLBACK SPUshutdown(void);
long CALLBACK SPUclose(void);
void CALLBACK SPUwriteRegister(unsigned long, unsigned short, unsigned int);
unsigned short CALLBACK SPUreadRegister(unsigned long);
void CALLBACK SPUwriteDMA(unsigned short);
unsigned short CALLBACK SPUreadDMA(void);
void CALLBACK SPUwriteDMAMem(unsigned short *, int, unsigned int);
void CALLBACK SPUreadDMAMem(unsigned short *, int, unsigned int);
void CALLBACK SPUplayADPCMchannel(xa_decode_t *);
unsigned int CALLBACK SPUgetADPCMBufferRoom(void); //senquack - added function
int  CALLBACK SPUplayCDDAchannel(short *, int);
long CALLBACK SPUconfigure(void);
long CALLBACK SPUfreeze(uint32_t, SPUFreeze_t *, uint32_t);
void CALLBACK SPUasync(uint32_t, uint32_t);

#ifdef SPU_PCSXREARMED
void CALLBACK SPUregisterCallback(void CALLBACK (*callback)(void));
void CALLBACK SPUregisterScheduleCb(void CALLBACK (*callback)(unsigned int));

// We provide our own private SPU_init() in plugins.cpp that will call
// spu_pcsxrearmed plugin's SPUinit() and then set its settings.
long CALLBACK SPU_init(void);
#endif //SPU_PCSXREARMED

#ifdef __cplusplus
}
#endif

// ** See comment for #ifdef above regarding SPUinit
#ifndef SPU_PCSXREARMED
#define SPU_init SPUinit
#endif

#define SPU_open SPUopen
#define SPU_shutdown SPUshutdown
#define SPU_close SPUclose
#define SPU_writeRegister SPUwriteRegister
#define SPU_readRegister SPUreadRegister
#define SPU_writeDMA SPUwriteDMA
#define SPU_readDMA SPUreadDMA
#define SPU_writeDMAMem SPUwriteDMAMem
#define SPU_readDMAMem SPUreadDMAMem
#define SPU_playADPCMchannel SPUplayADPCMchannel
#define SPU_getADPCMBufferRoom SPUgetADPCMBufferRoom
#define SPU_playCDDAchannel SPUplayCDDAchannel
#define SPU_registerCallback SPUregisterCallback
#define SPU_registerScheduleCb SPUregisterScheduleCb
#define SPU_configure SPUconfigure
#define SPU_freeze SPUfreeze
#define SPU_async SPUasync


// PAD functions

unsigned char PAD1_startPoll(void);
unsigned char PAD2_startPoll(void);
unsigned char PAD1_poll(void);
unsigned char PAD2_poll(void);

// ISO functions

void SetIsoFile(const char *filename);
const char *GetIsoFile(void);
boolean UsingIso(void);
void SetCdOpenCaseTime(s64 time);
s64 GetCdOpenCaseTime(void);
int ReloadCdromPlugin();

#endif /* __PLUGINS_H__ */
