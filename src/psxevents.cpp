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
 * Event / interrupt scheduler queue
 *
 * Added July 2016 by senquack (Daniel Silsby)
 *
 * Queue is an array sorted by event imminency. It can have spare capacity
 * both at the front and at the back. If an event is removed from the front
 * of the queue and a new event enqueued that ends up back at front of queue,
 * the operation is O(1).
 *
 * We also handle a small bit of SPU update logic here
 *
 */

#include "psxevents.h"
#include "r3000a.h"
#include "plugin_lib.h"

// To get event-handler functions:
#include "cdrom.h"
#include "plugins.h"
#include "psxdma.h"
#include "mdec.h"

// When psxRegs.cycle is >= this figure, it gets reset to 0:
static const u32 reset_cycle_val_at = 2000000000;

///////////////////////////
// Internal helper funcs //
///////////////////////////
static void psxEvqueueResetCycleVal(void);
static void psxEvqueueSchedulePersistentEvents(void);
static void psxEvqueueAdjustTimestamps(u32 prev_cycle_val);

/////////////////////
// SPU event funcs //
/////////////////////
void SPU_resetUpdateInterval(void);
static void SPU_update(void);
static void SPU_handleIRQ(void);

///////////////////////////////////
// Internal queue implementation //
///////////////////////////////////
static const int EVQUEUE_CAPACITY = PSXINT_COUNT;

typedef void (*EventFunc)(void);

static struct {
	u8 useBeginIdx;             // Idx of first used element of queue[]
	u8 useEndIdx;               // Idx one past last used element of queue[], or
	                            //  same val as useBeginIdx when queue is empty.
	u8 queue[EVQUEUE_CAPACITY];
	EventFunc funcs[EVQUEUE_CAPACITY];
	u32 spuUpdateInterval;      // Cycles between SPU plugin updates
} evqueue;

// Unimplemented events call this (shouldn't happen)
static void EventStubFunc(void)
{
}

static inline bool EventMoreImminent(u8 lh_ev, u8 rh_ev);
static inline void evqueueClear(void);
static inline size_t evqueueSize(void);
static inline bool evqueueEmpty(void);
static inline u8 evqueueFront(void);
static inline u8* evqueueFrontPtr(void);
static inline u8* evqueueEndPtr(void);
static inline void evqueueAdd(u8 ev);
static inline bool evqueueRemove(u8 ev);
static inline void evqueueRemoveFront(void);
static inline void evqueueMoveTowardsFront(u8 *start, u8 *end);
static inline void evqueueMoveTowardsBack(u8 *start, u8 *end);
static inline void evqueueInsertFront(u8 *pos, u8 ev);
static inline void evqueueInsertBack(u8 *pos, u8 ev);
#ifdef DEBUG_EVENTS
static bool evqueueConsistencyCheck(void);
static void evqueuePrintQueue(void);
#endif

void psxEvqueueInit(void)
{
	evqueue.funcs[PSXINT_SIO]             = sioInterrupt;
	evqueue.funcs[PSXINT_CDR]             = cdrInterrupt;
	evqueue.funcs[PSXINT_CDREAD]          = cdrReadInterrupt;
	evqueue.funcs[PSXINT_GPUDMA]          = gpuInterrupt;
	evqueue.funcs[PSXINT_MDECOUTDMA]      = mdec1Interrupt;
	evqueue.funcs[PSXINT_SPUDMA]          = spuInterrupt;
	evqueue.funcs[PSXINT_GPUBUSY]         = EventStubFunc;         // STUB-UNIMPLEMENTED (TODO?)
	evqueue.funcs[PSXINT_MDECINDMA]       = mdec0Interrupt;
	evqueue.funcs[PSXINT_GPUOTCDMA]       = gpuotcInterrupt;
	evqueue.funcs[PSXINT_CDRDMA]          = cdrDmaInterrupt;
	evqueue.funcs[PSXINT_NEWDRC_CHECK]    = EventStubFunc;         // STUB-UNIMPLEMENTED (TODO?)
	evqueue.funcs[PSXINT_RCNT]            = psxRcntUpdate;
	evqueue.funcs[PSXINT_CDRLID]          = cdrLidSeekInterrupt;
	evqueue.funcs[PSXINT_CDRPLAY]         = cdrPlayInterrupt;
	evqueue.funcs[PSXINT_SPUIRQ]          = SPU_handleIRQ;
	evqueue.funcs[PSXINT_SPU_UPDATE]      = SPU_update;
	evqueue.funcs[PSXINT_RESET_CYCLE_VAL] = psxEvqueueResetCycleVal;
	evqueue.funcs[PSXINT_SIO_SYNC_MCD]    = sioSyncMcds;

	evqueueClear();
	psxEvqueueSchedulePersistentEvents();
}

// Rebuild event queue afresh from psxRegs.intCycle[] contents
void psxEvqueueInitFromFreeze(void)
{
	evqueueClear();
	for (int ev=0; ev < PSXINT_COUNT; ++ev)
		if (psxRegs.interrupt & (1 << ev))
			evqueueAdd(ev);

	psxRegs.intCycle[PSXINT_NEXT_EVENT] = psxRegs.intCycle[evqueueFront()];
	psxEvqueueSchedulePersistentEvents();

	// Don't trust io_cycle_counter from a freeze, as older savestate versions
	//  from before event queue implementation may have invalid value.
	psxRegs.io_cycle_counter = 0;
}

// Function called when PSXINT_RESET_CYCLE_VAL event occurs:
// psxRegs.cycle is now regularly reset to 0 instead of being allowed to
// overflow. This ensures that it can always be compared directly to
// psxRegs.io_cycle_counter to know if psxBranchTest() needs to be called.
static void psxEvqueueResetCycleVal(void)
{
#ifdef DEBUG_EVENTS
	printf("\n------------------------------------------------------\n");
	printf("%s() queue state on entry:\n", __func__);
	evqueuePrintQueue();
#endif

	// Any events in the queue must be adjusted to match:
	psxEvqueueAdjustTimestamps(psxRegs.cycle);

	// All root-counter timestamps must also be adjusted:
	psxRcntAdjustTimestamps(psxRegs.cycle);

	// Reset cycle counter and enqueue new reset event:
	psxRegs.cycle = 0;
	psxEvqueueAdd(PSXINT_RESET_CYCLE_VAL, reset_cycle_val_at);

#ifdef DEBUG_EVENTS
	printf("%s() queue state on exit:\n", __func__);
	evqueuePrintQueue();
	printf("io_cycle_counter: %u\n", psxRegs.io_cycle_counter);
	printf("------------------------------------------------------\n\n");
#endif
}

static void psxEvqueueSchedulePersistentEvents(void)
{
	// Call psxEvqueueResetCycleVal() once, which will take care of two things:
	//  1.) psxRegs.cycle value is reset to 0, important if loading a savestate
	//      from an older version of emu, and psxRegs.cycle might be huge value
	//  2.) schedule PSXINT_RESET_CYCLE_VAL event, which calls it again
	//      at regular, very infrequent, intervals
	psxEvqueueResetCycleVal();

	// Must schedule initial SPU update.. it will reschedule itself thereafter
	SPU_resetUpdateInterval();
}

// At very intermittent intervals, psxRegs.cycle is reset to 0 to prevent
//  it from ever overflowing (simplifies checks against it elsewhere).
//  This function fixes up timestamps of all queued events when this occurs.
static void psxEvqueueAdjustTimestamps(u32 prev_cycle_val)
{
	for (u8 *ev = evqueueFrontPtr(); ev != evqueueEndPtr(); ++ev) {
		psxRegs.intCycle[*ev].sCycle -= prev_cycle_val;
	}

	psxRegs.intCycle[PSXINT_NEXT_EVENT].sCycle -= prev_cycle_val;
}

void psxEvqueueAdd(psxEventNum ev, u32 cycles_after)
{
	// Dequeue event if it already exists, to match original emu behavior
	if (psxRegs.interrupt & (1 << ev))
		evqueueRemove(ev);

	psxRegs.interrupt |= (1 << ev);
	psxRegs.intCycle[ev].sCycle = psxRegs.cycle;
	psxRegs.intCycle[ev].cycle = cycles_after;
	evqueueAdd(ev);
	psxRegs.intCycle[PSXINT_NEXT_EVENT] = psxRegs.intCycle[evqueueFront()];

	// io_cycle_counter is used to determine next time to call psxBranchTest()
	psxRegs.io_cycle_counter = psxRegs.intCycle[PSXINT_NEXT_EVENT].sCycle +
	                           psxRegs.intCycle[PSXINT_NEXT_EVENT].cycle;
}

void psxEvqueueRemove(psxEventNum ev)
{
	if (!(psxRegs.interrupt & (1 << ev)))
		return;

	psxRegs.interrupt &= ~(1 << ev);
	evqueueRemove(ev);

#ifdef DEBUG_EVENTS
	if (evqueueEmpty()) {
		printf("ERROR: empty queue in %s()\n", __func__);
		// Shouldn't happen, set to 0 for correctness's sake
		psxRegs.io_cycle_counter = 0;
	}
#endif

	// Copy next most-imminent event's timestamp into PSXINT_NEXT_EVENT entry.
	//  At least one event will always remain in the queue, i.e. PSXINT_RCNT,
	//  PSXINT_SPU_UPDATE, or PSXINT_RESET_CYCLE_VAL.
	psxRegs.intCycle[PSXINT_NEXT_EVENT] = psxRegs.intCycle[evqueueFront()];

	// io_cycle_counter is used to determine next time to call psxBranchTest()
	psxRegs.io_cycle_counter = psxRegs.intCycle[PSXINT_NEXT_EVENT].sCycle +
							   psxRegs.intCycle[PSXINT_NEXT_EVENT].cycle;
}

// Called from psxBranchTest() when an event is due.
void psxEvqueueDispatchAndRemoveFront(psxRegisters *pr)
{
#ifdef DEBUG_EVENTS
	if (evqueueEmpty()) {
		printf("ERROR: %s() called, but queue is empty\n", __func__);
		return;
	}
#endif

	u8 ev = evqueueFront();
	evqueueRemoveFront();
	pr->interrupt &= ~(1 << ev);

#ifdef DEBUG_EVENTS
	if (evqueue.funcs[ev] == EventStubFunc) {
		printf("WARNING: EventStubFunc() called for unimplemented event %u\n", ev);
	}
#endif

	evqueue.funcs[ev]();  // Dispatch event

	// Queue can never be totally empty, as certain persistent events will
	//  always be rescheduled during dispatch above.
#ifdef DEBUG_EVENTS
	if (evqueueEmpty())
		printf("ERROR: empty queue in %s()\n", __func__);
#endif

	// Only set this after event is dispatched, as a new event could be
	//  enqueued by the dispatched event.
	pr->intCycle[PSXINT_NEXT_EVENT] = pr->intCycle[evqueueFront()];

	// NOTE: Calling function, psxBranchTest(), will take care of setting
	//  psxRegs.io_cycle_counter after all pending events are dispatched.
}

// Should be called if Config.PsxType, Config.SpuUpdateFreq is changed
void SPU_resetUpdateInterval(void)
{
	evqueue.spuUpdateInterval = PSXCLK / (FrameRate[Config.PsxType]);

	// If flexible SPU updates are configured, schedule first event,
	//  subsequent events will be scheduled automatically.
	//  If update frequency is 1, SPU updated directly in psxcounters.cpp
	if (Config.SpuUpdateFreq > SPU_UPDATE_FREQ_1) {
		evqueue.spuUpdateInterval >>= Config.SpuUpdateFreq;
		psxEvqueueAdd(PSXINT_SPU_UPDATE, evqueue.spuUpdateInterval);
	}
}

// SPU_update() is a wrapper function around SPU_async(),
// allowing handling as a generic event
void SPU_update(void)
{
	//Clear any scheduled SPUIRQ, as HW SPU IRQ will end up handled with
	// this call to SPU_async(), and new SPUIRQ scheduled if necessary.
	psxEvqueueRemove(PSXINT_SPUIRQ);

	SPU_async(psxRegs.cycle, 1);

	// If frameskip is advised, update SPU more frequently to avoid dropouts
	if (Config.SpuUpdateFreq > SPU_UPDATE_FREQ_1) {
		u32 interval = evqueue.spuUpdateInterval;
		if (pl_frameskip_advice() && Config.SpuUpdateFreq < SPU_UPDATE_FREQ_MAX)
			interval /= 2;
		psxEvqueueAdd(PSXINT_SPU_UPDATE, interval);
	}
}

// SPU_handleIRQ() is a wrapper function around SPU_async(),
// allowing handling as a generic event
static void SPU_handleIRQ(void)
{
	SPU_async(psxRegs.cycle, 0);
}

// Returns true if event 'lh_ev' is more imminent than 'rh_ev'.
static inline bool EventMoreImminent(u8 lh_ev, u8 rh_ev)
{
	// Compare the two event timestamps, interpreting the result as a signed
	//  integer in case one or both cross psxRegs.cycle overflow boundary.
	int lh_tmp = psxRegs.intCycle[lh_ev].sCycle + psxRegs.intCycle[lh_ev].cycle
	             - psxRegs.cycle;
	int rh_tmp = psxRegs.intCycle[rh_ev].sCycle + psxRegs.intCycle[rh_ev].cycle
	             - psxRegs.cycle;
	return lh_tmp < rh_tmp;
}

static inline void evqueueClear(void)
{
	evqueue.useEndIdx = evqueue.useBeginIdx = 0;
}

static inline size_t evqueueSize(void)
{
	return evqueue.useEndIdx - evqueue.useBeginIdx;
}

static inline bool evqueueEmpty(void)
{
	return evqueue.useEndIdx == evqueue.useBeginIdx;
}

static inline u8 evqueueFront(void)
{
	return evqueue.queue[evqueue.useBeginIdx];
}

// Returns pointer to first element in use
static inline u8* evqueueFrontPtr(void)
{
	return evqueue.queue + evqueue.useBeginIdx;
}

// Returns pointer to one past the last element in use,
//  but when empty, points to same element as evqueueFrontPtr()
static inline u8* evqueueEndPtr(void)
{
	return evqueue.queue + evqueue.useEndIdx;
}

// Insert new element, keeping queue sorted by event imminency.
//  Event's timestamp in psxRegs.intCycle[] must be set before call.
//  Important: two elements equivalent in imminency keep their relative order,
//  i.e., new events are inserted after existing equally-imminent events.
static inline void evqueueAdd(u8 ev)
{
#ifdef DEBUG_EVENTS
	if (evqueueSize() == EVQUEUE_CAPACITY) {
		printf("ERROR: %s() could not find space in its array\n", __func__);
		return;
	}

	if (!evqueueConsistencyCheck()) {
		printf("ERROR: Queue consistent ordering check failed in %s(),\n"
	           "before adding event %u\n", __func__, ev);
		evqueuePrintQueue();
	}
#endif

	u8 *it = evqueueFrontPtr();  // iterator

	while ((it != evqueueEndPtr()) && !EventMoreImminent(ev, *it))
		++it;

	// Rather than be fancy and compute position relative to front/end, to
	//  decide how to insert, we choose evqueueInsertFront() when possible,
	//  to keep object code size small. Our queue is usually pretty small.
	if (evqueue.useBeginIdx > 0) {
		// Insert before position 'it', move elements towards front to make room
		evqueueInsertFront(it-1, ev);
	} else {
		// Insert at position 'it', move elements towards back to make room
		evqueueInsertBack(it, ev);
	}

#ifdef DEBUG_EVENTS
	if (!evqueueConsistencyCheck()) {
		printf("ERROR: Queue consistent ordering check failed in %s(),\n"
	           "after adding event %u\n", __func__, ev);
		evqueuePrintQueue();
	}
#endif
}

// Remove the first matching element, returning false if not found
static bool evqueueRemove(u8 ev)
{
	u8 *it = evqueueFrontPtr();
	while ((it != evqueueEndPtr()) && (*it != ev))
		++it;

	if (it == evqueueEndPtr())
		return false;

	// Rather than be fancy and compute position relative to front/end, we
	//  always shrink array towards front, to keep object code size small.
	--evqueue.useEndIdx;
	evqueueMoveTowardsFront(it+1, evqueueEndPtr());

#ifdef DEBUG_EVENTS
	if (!evqueueConsistencyCheck()) {
		printf("ERROR: Queue consistent ordering check failed in %s(),\n"
	           "after removing event %u\n", __func__, ev);
		evqueuePrintQueue();
	}
#endif

	return true;
}

static inline void evqueueRemoveFront(void)
{
#ifdef DEBUG_EVENTS
	if (evqueueEmpty()) {
		printf("ERROR: %s() called when queue is empty\n", __func__);
		return;
	}
#endif

	++evqueue.useBeginIdx;
}

// Copy all elements between [start,end] to [start+1,end+1].
//  If start address is greater than end, no copy will occur.
static inline void evqueueMoveTowardsBack(u8 *start, u8 *end)
{
	for (u8 *ptr = end; ptr >= start; --ptr)
		*(ptr+1) = *ptr;
}

// Copy all elements between [start,end] to [start-1,end-1].
//  If start address is greater than end, no copy will occur.
static inline void evqueueMoveTowardsFront(u8 *start, u8 *end)
{
	for (u8 *ptr = start; ptr <= end; ++ptr)
		*(ptr-1) = *ptr;
}

// Insert new entry 'ev' at position 'pos'. Any existing elements at or
//  before position will be moved towards the front to make room.
static inline void evqueueInsertFront(u8 *pos, u8 ev)
{
	evqueueMoveTowardsFront(evqueueFrontPtr(), pos);
	--evqueue.useBeginIdx;
	*pos = ev;
}

// Insert new entry 'ev' at position 'pos'. Any existing elements at or
//  after position will be moved towards the rear to make room.
static inline void evqueueInsertBack(u8 *pos, u8 ev)
{
	evqueueMoveTowardsBack(pos, evqueueEndPtr()-1);
	++evqueue.useEndIdx;
	*pos = ev;
}

#ifdef DEBUG_EVENTS
static bool evqueueConsistencyCheck(void)
{
	if (evqueueSize() < 2)
		return true;

	for (u8 *ev = evqueueFrontPtr(); ev < (evqueueEndPtr()-1); ++ev) {
		if (!EventMoreImminent(*ev, *(ev+1))) {
			if ((psxRegs.intCycle[*ev].sCycle + psxRegs.intCycle[*ev].cycle) !=
			    (psxRegs.intCycle[*(ev+1)].sCycle + psxRegs.intCycle[*(ev+1)].cycle)) {
				printf("ERROR: %s() failed: EV %u > EV %u\n", __func__, *ev, *(ev+1));
				return false;
			}
		}
	}
	return true;
}

static void evqueuePrintQueue(void)
{
	printf("Queue contains %zu events, useBeginIdx: %u  useEndIdx: %u\n",
			evqueueSize(), evqueue.useBeginIdx, evqueue.useEndIdx);
	for (u8 *ev = evqueueFrontPtr(); ev != evqueueEndPtr(); ++ev) {
		printf("EV: %u SCYCLE: %u CYCLE: %u\n",
		       *ev, psxRegs.intCycle[*ev].sCycle, psxRegs.intCycle[*ev].cycle);
	}

	if (evqueueConsistencyCheck())
		printf("Queue consistent ordering check passes.\n");
}
#endif
