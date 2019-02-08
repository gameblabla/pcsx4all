/***************************************************************************
 *   Copyright (C) 2010 by Blade_Arma                                      *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

/*
 * Internal PSX counters.
 */

///////////////////////////////////////////////////////////////////////////////
//senquack - NOTE: Root counters code here has been updated to match Notaz's
// PCSX Rearmed where possible. Important changes include:
// * Proper handling of counter overflows.
// * VBlank root counter (counter 3) is triggered only as often as needed,
//   not every HSync.
// * SPU updates occur using new event queue (psxevents.cpp)
// * Some optimizations, more accurate calculation of timer updates.
//
// TODO : Implement direct rootcounter mem access of Rearmed dynarec?
//        (see https://github.com/notaz/pcsx_rearmed/commit/b1be1eeee94d3547c20719acfa6b0082404897f1 )
//        Seems to make Parasite Eve 2 RCntFix hard to implement, though.
// TODO : Implement Rearmed's auto-frameskip so SPU doesn't need to
//        hackishly be updated twice per emulated frame.
// TODO : Implement Rearmed's frame limiter

#include "psxcounters.h"
#include "psxevents.h"
#include "gpu.h"

/******************************************************************************/

enum
{
    Rc0Gate           = 0x0001, // 0    not implemented
    Rc1Gate           = 0x0001, // 0    not implemented
    Rc2Disable        = 0x0001, // 0    partially implemented
    RcUnknown1        = 0x0002, // 1    ?
    RcUnknown2        = 0x0004, // 2    ?
    RcCountToTarget   = 0x0008, // 3
    RcIrqOnTarget     = 0x0010, // 4
    RcIrqOnOverflow   = 0x0020, // 5
    RcIrqRegenerate   = 0x0040, // 6
    RcUnknown7        = 0x0080, // 7    ?
    Rc0PixelClock     = 0x0100, // 8    fake implementation
    Rc1HSyncClock     = 0x0100, // 8
    Rc2Unknown8       = 0x0100, // 8    ?
    Rc0Unknown9       = 0x0200, // 9    ?
    Rc1Unknown9       = 0x0200, // 9    ?
    Rc2OneEighthClock = 0x0200, // 9
    RcUnknown10       = 0x0400, // 10   ?
    RcCountEqTarget   = 0x0800, // 11
    RcOverflow        = 0x1000, // 12
    RcUnknown13       = 0x2000, // 13   ? (always zero)
    RcUnknown14       = 0x4000, // 14   ? (always zero)
    RcUnknown15       = 0x8000, // 15   ? (always zero)
};

static const u32 CounterQuantity  = 4;
static const u32 CountToOverflow  = 0;
static const u32 CountToTarget    = 1;

const u32 FrameRate[2] = { 60, 50 };

//senquack - Originally {262,312}, updated to match Rearmed:
const u32 HSyncTotal[2] = { 263, 313 };

//senquack - TODO: PCSX Reloaded uses {243,256} here, and Rearmed
// does away with array completely and uses 240 in all cases:
//static const u32 VBlankStart[]    = { 240, 256 };
static const u32 VBlankStart = 240;

/******************************************************************************/

static Rcnt rcnts[ CounterQuantity ];

u32 hSyncCount = 0;

u32 frame_counter = 0;
static u32 hsync_steps = 0;
static u32 base_cycle = 0;
static bool rcntFreezeLoaded = false;

//senquack - Originally separate variables, now handled together with
// all other scheduled emu events as new event type PSXINT_RCNT
#define psxNextCounter psxRegs.intCycle[PSXINT_RCNT].cycle
#define psxNextsCounter psxRegs.intCycle[PSXINT_RCNT].sCycle

/******************************************************************************/

static inline void setIrq( u32 irq )
{
	psxHu32ref(0x1070) |= SWAPu32(irq);
    	ResetIoCycle();
}

//senquack - Added verboseLog & VERBOSE_LEVEL from PCSX Rearmed:
#define VERBOSE_LEVEL 0

static void verboseLog( u32 level, const char *str, ... )
{
#if VERBOSE_LEVEL > 0
    if( level <= VerboseLevel )
    {
        va_list va;
        char buf[ 4096 ];

        va_start( va, str );
        vsprintf( buf, str, va );
        va_end( va );

        printf( "%s", buf );
        fflush( stdout );
    }
#endif
}

/******************************************************************************/

INLINE void _psxRcntWcount( u32 index, u32 value )
{
    if( value > 0xffff )
    {
        verboseLog( 1, "[RCNT %i] wcount > 0xffff: %x\n", index, value );
        value &= 0xffff;
    }

    rcnts[index].cycleStart  = psxRegs.cycle;
    rcnts[index].cycleStart -= value * rcnts[index].rate;

    // TODO: <=.
    if( value < rcnts[index].target )
    {
        rcnts[index].cycle = rcnts[index].target * rcnts[index].rate;
        rcnts[index].counterState = CountToTarget;
    }
    else
    {
        rcnts[index].cycle = 0x10000 * rcnts[index].rate;
        rcnts[index].counterState = CountToOverflow;
    }
}

static inline u32 _psxRcntRcount( u32 index )
{
    u32 count;

    count  = psxRegs.cycle;
    count -= rcnts[index].cycleStart;
    if (rcnts[index].rate > 1)
        count /= rcnts[index].rate;

    if( count > 0x10000 )
    {
        verboseLog( 1, "[RCNT %i] rcount > 0x10000: %x\n", index, count );
    }
    count &= 0xffff;

    return count;
}

//senquack - Added from PCSX Rearmed:
static void _psxRcntWmode( u32 index, u32 value )
{
    rcnts[index].mode = value;

    switch( index )
    {
        case 0:
            if( value & Rc0PixelClock )
            {
                rcnts[index].rate = 5;
            }
            else
            {
                rcnts[index].rate = 1;
            }
        break;
        case 1:
            if( value & Rc1HSyncClock )
            {
                rcnts[index].rate = (PSXCLK / (FrameRate[Config.PsxType] * HSyncTotal[Config.PsxType]));
            }
            else
            {
                rcnts[index].rate = 1;
            }
        break;
        case 2:
            if( value & Rc2OneEighthClock )
            {
                rcnts[index].rate = 8;
            }
            else
            {
                rcnts[index].rate = 1;
            }

            // TODO: wcount must work.
            if( value & Rc2Disable )
            {
                rcnts[index].rate = 0xffffffff;
            }
        break;
    }
}

/******************************************************************************/

static void psxRcntSet(void)
{
    s32 countToUpdate;
    u32 i;

    psxNextsCounter = psxRegs.cycle;
    psxNextCounter  = 0x7fffffff;

    for( i = 0; i < CounterQuantity; ++i )
    {
        countToUpdate = rcnts[i].cycle - (psxNextsCounter - rcnts[i].cycleStart);

        if( countToUpdate < 0 )
        {
            psxNextCounter = 0;
            break;
        }

        if( countToUpdate < (s32)psxNextCounter )
        {
            psxNextCounter = countToUpdate;
        }
    }

    // Any previously queued PSXINT_RCNT event will be replaced
    psxEvqueueAdd(PSXINT_RCNT, psxNextCounter);
}

/******************************************************************************/

static void psxRcntReset( u32 index )
{
    u32 rcycles;

    rcnts[index].mode |= RcUnknown10;

    if( rcnts[index].counterState == CountToTarget )
    {
        rcycles = psxRegs.cycle - rcnts[index].cycleStart;
        if( rcnts[index].mode & RcCountToTarget )
        {
            rcycles -= rcnts[index].target * rcnts[index].rate;
            rcnts[index].cycleStart = psxRegs.cycle - rcycles;
        }
        else
        {
            rcnts[index].cycle = 0x10000 * rcnts[index].rate;
            rcnts[index].counterState = CountToOverflow;
        }

        if( rcnts[index].mode & RcIrqOnTarget )
        {
            if( (rcnts[index].mode & RcIrqRegenerate) || (!rcnts[index].irqState) )
            {
                verboseLog( 3, "[RCNT %i] irq\n", index );
                setIrq( rcnts[index].irq );
                rcnts[index].irqState = 1;
            }
        }

        rcnts[index].mode |= RcCountEqTarget;

        if( rcycles < 0x10000 * rcnts[index].rate )
            return;
    }

    if( rcnts[index].counterState == CountToOverflow )
    {
        rcycles = psxRegs.cycle - rcnts[index].cycleStart;
        rcycles -= 0x10000 * rcnts[index].rate;

        rcnts[index].cycleStart = psxRegs.cycle - rcycles;

        if( rcycles < rcnts[index].target * rcnts[index].rate )
        {
            rcnts[index].cycle = rcnts[index].target * rcnts[index].rate;
            rcnts[index].counterState = CountToTarget;
        }

        if( rcnts[index].mode & RcIrqOnOverflow )
        {
            if( (rcnts[index].mode & RcIrqRegenerate) || (!rcnts[index].irqState) )
            {
                verboseLog( 3, "[RCNT %i] irq\n", index );
                setIrq( rcnts[index].irq );
                rcnts[index].irqState = 1;
            }
        }

        rcnts[index].mode |= RcOverflow;
    }
}

void psxRcntUpdate()
{
    u32 cycle;

    cycle = psxRegs.cycle;

    // rcnt 0.
    if( cycle - rcnts[0].cycleStart >= rcnts[0].cycle )
    {
        psxRcntReset( 0 );
    }

    // rcnt 1.
    if( cycle - rcnts[1].cycleStart >= rcnts[1].cycle )
    {
        psxRcntReset( 1 );
    }

    // rcnt 2.
    if( cycle - rcnts[2].cycleStart >= rcnts[2].cycle )
    {
        psxRcntReset( 2 );
    }

    // rcnt base.
    if( cycle - rcnts[3].cycleStart >= rcnts[3].cycle )
    {
        u32 leftover_cycles = cycle - rcnts[3].cycleStart - rcnts[3].cycle;
        u32 next_vsync;

        hSyncCount += hsync_steps;

        // VSync irq.
        if( hSyncCount == VBlankStart )
        {
            HW_GPU_STATUS &= ~PSXGPU_LCF;

#ifdef USE_GPULIB
            GPU_vBlank( 1, 0 );
#endif
            setIrq( 0x01 );

            // Do framelimit, frameskip, perf stats, controls, etc:
            // NOTE: this is point of control transfer to frontend menu
            EmuUpdate();

            // If frontend called LoadState(), loading a savestate, do not
            //  proceed further: Rootcounter state has been altered.
            if (rcntFreezeLoaded) {
                rcntFreezeLoaded = false;
                return;
            }

            GPU_updateLace();

            //senquack - PCSX Rearmed updates its SPU plugin once per emulated
            // frame. However, we target slower platforms and update SPU plugin
            // at flexible interval (scheduled event) to avoid audio dropouts.
            if (Config.SpuUpdateFreq == SPU_UPDATE_FREQ_1)
                SPU_async(cycle, 1);
        }

        // Update lace. (with InuYasha fix)
        if( hSyncCount >= (Config.VSyncWA ? HSyncTotal[Config.PsxType]/BIAS : HSyncTotal[Config.PsxType]) )
        {
            hSyncCount = 0;
            frame_counter++;

            gpuSyncPluginSR();
            if( (HW_GPU_STATUS & PSXGPU_ILACE_BITS) == PSXGPU_ILACE_BITS )
                HW_GPU_STATUS |= frame_counter << 31;

#ifdef USE_GPULIB
            GPU_vBlank( 0, HW_GPU_STATUS >> 31 );
#endif
        }

        // Schedule next call, in hsyncs
        hsync_steps = HSyncTotal[Config.PsxType] - hSyncCount;
        next_vsync = VBlankStart - hSyncCount; // ok to overflow
        if( next_vsync && next_vsync < hsync_steps )
            hsync_steps = next_vsync;

        rcnts[3].cycleStart = cycle - leftover_cycles;
        if (Config.PsxType)
                // 20.12 precision, clk / 50 / 313 ~= 2164.14
                base_cycle += hsync_steps * 8864320;
        else
                // clk / 60 / 263 ~= 2146.31
                base_cycle += hsync_steps * 8791293;
        rcnts[3].cycle = base_cycle >> 12;
        base_cycle &= 0xfff;
    }

    psxRcntSet();
}

/******************************************************************************/

void psxRcntWcount( u32 index, u32 value )
{
    verboseLog( 2, "[RCNT %i] wcount: %x\n", index, value );

    _psxRcntWcount( index, value );
    psxRcntSet();
}

void psxRcntWmode( u32 index, u32 value )
{
    verboseLog( 1, "[RCNT %i] wmode: %x\n", index, value );

    _psxRcntWmode( index, value );
    _psxRcntWcount( index, 0 );

    rcnts[index].irqState = 0;
    psxRcntSet();
}

void psxRcntWtarget( u32 index, u32 value )
{
    verboseLog( 1, "[RCNT %i] wtarget: %x\n", index, value );

    rcnts[index].target = value;

    _psxRcntWcount( index, _psxRcntRcount( index ) );
    psxRcntSet();
}

/******************************************************************************/

u32 psxRcntRcount( u32 index )
{
    u32 count;

    count = _psxRcntRcount( index );

    // Parasite Eve 2 fix.
    if( Config.RCntFix ) {
        if( index == 2 ) {
            if( rcnts[index].counterState == CountToTarget )
                count /= BIAS;
        }
    }

    verboseLog( 2, "[RCNT %i] rcount: %x\n", index, count );

    return count;
}

u32 psxRcntRmode( u32 index )
{
    u16 mode;

    mode = rcnts[index].mode;
    rcnts[index].mode &= 0xe7ff;

    verboseLog( 2, "[RCNT %i] rmode: %x\n", index, mode );

    return mode;
}

u32 psxRcntRtarget( u32 index )
{
    verboseLog( 2, "[RCNT %i] rtarget: %x\n", index, rcnts[index].target );

    return rcnts[index].target;
}

/******************************************************************************/

void psxRcntInit(void)
{
    s32 i;

    // rcnt 0.
    rcnts[0].rate   = 1;
    rcnts[0].irq    = 0x10;

    // rcnt 1.
    rcnts[1].rate   = 1;
    rcnts[1].irq    = 0x20;

    // rcnt 2.
    rcnts[2].rate   = 1;
    rcnts[2].irq    = 0x40;

    // rcnt base.
    rcnts[3].rate   = 1;
    rcnts[3].mode   = RcCountToTarget;
    rcnts[3].target = (PSXCLK / (FrameRate[Config.PsxType] * HSyncTotal[Config.PsxType]));

    for( i = 0; i < CounterQuantity; ++i )
    {
        _psxRcntWcount( i, 0 );
    }

    hSyncCount = 0;
    hsync_steps = 1;

    psxRcntSet();
}

/******************************************************************************/

int psxRcntFreeze(void *f, FreezeMode mode)
{
    // Old var left from when SPU was updated by very old psxcounters code,
    //  now this is 0 placeholder to maintain savestate compatibilty
    u32 spuSyncCount = 0;

    if (    freeze_rw(f, mode, &rcnts, sizeof(rcnts))
         || freeze_rw(f, mode, &hSyncCount, sizeof(hSyncCount))
         || freeze_rw(f, mode, &spuSyncCount, sizeof(spuSyncCount))
         || freeze_rw(f, mode, &psxNextCounter, sizeof(psxNextCounter))
         || freeze_rw(f, mode, &psxNextsCounter, sizeof(psxNextsCounter)) )
    return -1;

    if (mode == FREEZE_LOAD)
        psxRcntInitFromFreeze();

    return 0;
}

/******************************************************************************/
// Normally called by psxRcntFreeze() when loading sstate, however
//  older buggy savestates may require it to be called directly.
//  (see comments regarding save version 0x8b410004 in misc.cpp)
void psxRcntInitFromFreeze(void)
{
	// don't trust things from a savestate
	for (int i = 0; i < CounterQuantity; ++i)
	{
		_psxRcntWmode( i, rcnts[i].mode );
		u32 count = (psxRegs.cycle - rcnts[i].cycleStart) / rcnts[i].rate;
		_psxRcntWcount( i, count );
	}
	hsync_steps = (psxRegs.cycle - rcnts[3].cycleStart) / rcnts[3].target;
	psxRcntSet();

	base_cycle = 0;

	// psxRcntUpdate() needs notification when state is altered:
	rcntFreezeLoaded = true;
}

/******************************************************************************/
// Called before psxRegs.cycle is adjusted back to zero
//  by PSXINT_RESET_CYCLE_VAL event in psxevents.cpp
void psxRcntAdjustTimestamps(const uint32_t prev_cycle_val)
{
	for (int i=0; i < CounterQuantity; ++i) {
		rcnts[i].cycleStart -= prev_cycle_val;
	}
}
