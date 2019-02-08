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

#ifndef __PSXDMA_H__
#define __PSXDMA_H__

#include "psxcommon.h"
#include "r3000a.h"
#include "psxevents.h"
#include "psxhw.h"
#include "psxmem.h"

//senquack - NOTE: These macros have been updated to use new PSXINT_*
// interrupts enum and intCycle struct (much cleaner than before)
// from PCSX Reloaded/Rearmed as well as new event queue (psxEvqueues.h)
#define GPUDMA_INT(eCycle) { \
	psxEvqueueAdd(PSXINT_GPUDMA, eCycle); \
}

#define SPUDMA_INT(eCycle) { \
	psxEvqueueAdd(PSXINT_SPUDMA, eCycle); \
}

#define MDECOUTDMA_INT(eCycle) { \
	psxEvqueueAdd(PSXINT_MDECOUTDMA, eCycle); \
}

#define MDECINDMA_INT(eCycle) { \
	psxEvqueueAdd(PSXINT_MDECINDMA, eCycle); \
}

#define GPUOTCDMA_INT(eCycle) { \
	psxEvqueueAdd(PSXINT_GPUOTCDMA, eCycle); \
}

#define CDRDMA_INT(eCycle) { \
	psxEvqueueAdd(PSXINT_CDRDMA, eCycle); \
}

void psxDma2(u32 madr, u32 bcr, u32 chcr);
void psxDma3(u32 madr, u32 bcr, u32 chcr);
void psxDma4(u32 madr, u32 bcr, u32 chcr);
void psxDma6(u32 madr, u32 bcr, u32 chcr);
void gpuInterrupt(void);
void spuInterrupt(void);
void gpuotcInterrupt();

#endif /* __PSXDMA_H__ */
