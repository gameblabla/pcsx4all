/***************************************************************************
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

/*
 * Event / interrupt scheduling
 *
 * Added July 2016 by senquack (Daniel Silsby)
 *
 */

#ifndef PSXEVENTS_H
#define PSXEVENTS_H

#include <stdio.h>
#include <stdint.h>
#include "r3000a.h"

enum psxEventNum {
	PSXINT_SIO = 0,
	PSXINT_CDR,
	PSXINT_CDREAD,
	PSXINT_GPUDMA,
	PSXINT_MDECOUTDMA,
	PSXINT_SPUDMA,
	PSXINT_GPUBUSY,        //From PCSX Rearmed, but not implemented there nor here
	PSXINT_MDECINDMA,
	PSXINT_GPUOTCDMA,
	PSXINT_CDRDMA,
	PSXINT_NEWDRC_CHECK,   //Used in PCSX Rearmed dynarec, but not implemented here (TODO?)
	PSXINT_RCNT,
	PSXINT_CDRLID,
	PSXINT_CDRPLAY,
	PSXINT_SPUIRQ,         //Check for upcoming SPU HW interrupts
	PSXINT_SPU_UPDATE,     //senquack - update and feed SPU (note that this usage
	                       // differs from Rearmed: Rearmed uses this for checking
	                       // for SPU HW interrupts and lacks a flexibly-scheduled
	                       // interval for SPU update-and-feed)
	PSXINT_RESET_CYCLE_VAL,          // Reset psxRegs.cycle value to 0 to ensure
	                                 //  it can never overflow
	PSXINT_SIO_SYNC_MCD,             // Flush/sync/close memcards opened for writing
	PSXINT_COUNT,
	PSXINT_NEXT_EVENT = PSXINT_COUNT //The most imminent event's entry is
	                                 // always copied to this slot in
	                                 // psxRegs.intCycles[] for fast checking
};

void psxEvqueueInit(void);
void psxEvqueueInitFromFreeze(void);
void psxEvqueueAdd(psxEventNum ev, u32 cycles_after);
void psxEvqueueRemove(psxEventNum ev);
void psxEvqueueDispatchAndRemoveFront(psxRegisters *pr);

// Should be called when Config.PsxType changes
void SPU_resetUpdateInterval(void);

#endif //PSXEVENTS_H
