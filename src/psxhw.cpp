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
* Functions for PSX hardware control.
*/

///////////////////////////////////////////////////////////////////////////////
// May 30 2017  - Dynarec note: (senquack)                                   //
//       NOTE: MIPS dynarec implements inlined versions of some cases of     //
//       psxHwWrite8/16/32() and psxHwRead8/16/32(). Any updates of these    //
//       functions should be accompanied by updates to MIPS dynarec HW I/O.  //
//       IF YOU DON'T UPDATE MIPSREC: uncomment the #error line here to      //
//       at least provide a compile-time error for future MIPS users:        //
///////////////////////////////////////////////////////////////////////////////
#if defined(PSXREC) && defined(mips)
//#error "Updates to psxhw.cpp have been made, but MIPS recompiler has not been updated. You must update rec_lsu_hw.cpp.h to match psxhw.cpp, or ensure that inlined direct HW I/O is disabled in dynarec."
#endif


///////////////////////////////////////////////////////////////////////////////
// Aug 19 2017  - Added functionality useful to dynarecs: (senquack)         //
//       NOTE: Dynarecs that handle most RAM/scratchpad memory accesses on   //
//  on their own benefit from having psxHwRead*/psxHwWrite* handle not only  //
//  I/O ports access, but any access to ROM or cache control port as well.   //
//  This saves them having to call psxMemRead/psxMemWrite just to have those //
//  functions always end up calling here. Therefore, you will see added      //
//  '#ifdef PSXREC' blocks at the end of the read/write funcs here for this. //
//  Sorry these functions are such a mess, but they were already like that   //
//  in original emulator code and remain largely so to this day in PCSXR.    //
///////////////////////////////////////////////////////////////////////////////


#include "psxhw.h"
#include "mdec.h"
#include "cdrom.h"
#include "gpu.h"

void psxHwReset() {
	//senquack - added Config.SpuIrq option from PCSX Rearmed/Reloaded:
	if (Config.SpuIrq) psxHu32ref(0x1070) |= SWAP32(0x200);

	memset(psxH, 0, 0x10000);

	mdecInit(); //intialize mdec decoder
	sioInit(); //initialize sio
	cdrReset();
	psxRcntInit();
	HW_GPU_STATUS = 0x14802000;
}

u8 psxHwRead8(u32 add)
{
	u8 hard = 0;

	if ((add & 0x0ff00000) == 0x0f800000)
	{
		switch (add) {
		case 0x1f801040: hard = sioRead8();break; 
		//  case 0x1f801050: hard = serial_read8(); break;//for use of serial port ignore for now
		case 0x1f801800: hard = cdrRead0(); break;
		case 0x1f801801: hard = cdrRead1(); break;
		case 0x1f801802: hard = cdrRead2(); break;
		case 0x1f801803: hard = cdrRead3(); break;
		default:
			hard = psxHu8(add); 
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unkwnown 8bit read at address %x\n", add);
#endif
		}
		return hard;
	}

#ifdef PSXREC
	// See note at top of file regarding dynarecs needing added functionality.
	if ((add & 0x0ff00000) == 0x0fc00000) {
		// ROM access
		return psxRu8(add);
	} else {
		// A non-32-bit read from cache control port and probably never encountered
		return 0;
	}
#endif //PSXREC

	return hard;
}

u16 psxHwRead16(u32 add)
{
	u16 hard = 0;

	if ((add & 0x0ff00000) == 0x0f800000)
	{
		switch (add) {
#ifdef PSXHW_LOG
		case 0x1f801070: PSXHW_LOG("IREG 16bit read %x\n", psxHu16(0x1070));
			return psxHu16(0x1070);
#endif
#ifdef PSXHW_LOG
		case 0x1f801074: PSXHW_LOG("IMASK 16bit read %x\n", psxHu16(0x1074));
			return psxHu16(0x1074);
#endif

		case 0x1f801040:
			hard = sioRead16();
#ifdef PAD_LOG
			PAD_LOG("sio read16 %x; ret = %x\n", add&0xf, hard);
#endif
			return hard;
		case 0x1f801044:
			hard = sioReadStat16();
#ifdef PAD_LOG
			PAD_LOG("sio read16 %x; ret = %x\n", add&0xf, hard);
#endif
			return hard;
		case 0x1f801048:
			hard = sioReadMode16();
#ifdef PAD_LOG
			PAD_LOG("sio read16 %x; ret = %x\n", add&0xf, hard);
#endif
			return hard;
		case 0x1f80104a:
			hard = sioReadCtrl16();
#ifdef PAD_LOG
			PAD_LOG("sio read16 %x; ret = %x\n", add&0xf, hard);
#endif
			return hard;
		case 0x1f80104e:
			hard = sioReadBaud16();
#ifdef PAD_LOG
			PAD_LOG("sio read16 %x; ret = %x\n", add&0xf, hard);
#endif
			return hard;

		//Serial port stuff not support now ;P
	 // case 0x1f801050: hard = serial_read16(); break;
	 //	case 0x1f801054: hard = serial_status_read(); break;
	 //	case 0x1f80105a: hard = serial_control_read(); break;
	 //	case 0x1f80105e: hard = serial_baud_read(); break;

		case 0x1f801100:
			hard = psxRcntRcount(0);
#ifdef PSXHW_LOG
			PSXHW_LOG("T0 count read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801104:
			hard = psxRcntRmode(0);
#ifdef PSXHW_LOG
			PSXHW_LOG("T0 mode read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801108:
			hard = psxRcntRtarget(0);
#ifdef PSXHW_LOG
			PSXHW_LOG("T0 target read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801110:
			hard = psxRcntRcount(1);
#ifdef PSXHW_LOG
			PSXHW_LOG("T1 count read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801114:
			hard = psxRcntRmode(1);
#ifdef PSXHW_LOG
			PSXHW_LOG("T1 mode read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801118:
			hard = psxRcntRtarget(1);
#ifdef PSXHW_LOG
			PSXHW_LOG("T1 target read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801120:
			hard = psxRcntRcount(2);
#ifdef PSXHW_LOG
			PSXHW_LOG("T2 count read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801124:
			hard = psxRcntRmode(2);
#ifdef PSXHW_LOG
			PSXHW_LOG("T2 mode read16: %x\n", hard);
#endif
			return hard;
		case 0x1f801128:
			hard = psxRcntRtarget(2);
#ifdef PSXHW_LOG
			PSXHW_LOG("T2 target read16: %x\n", hard);
#endif
			return hard;

		//case 0x1f802030: hard =   //int_2000????
		//case 0x1f802040: hard =//dip switches...??

		default:
			if (add >= 0x1f801c00 && add < 0x1f801e00) {
				hard = SPU_readRegister(add);
			} else {
				hard = psxHu16(add); 
#ifdef PSXHW_LOG
				PSXHW_LOG("*Unkwnown 16bit read at address %x\n", add);
#endif
			}
			return hard;
		}
	}

#ifdef PSXREC
	// See note at top of file regarding dynarecs needing added functionality.
	if ((add & 0x0ff00000) == 0x0fc00000) {
		// ROM access
		return psxRu16(add);
	} else {
		// A non-32-bit read from cache control port and probably never encountered
		return 0;
	}
#endif //PSXREC

	return hard;
}

u32 psxHwRead32(u32 add)
{
	u32 hard = 0;

	if ((add & 0x0ff00000) == 0x0f800000)
	{
		switch (add) {
		case 0x1f801040:
			hard = sioRead32();
#ifdef PAD_LOG
			PAD_LOG("sio read32 ;ret = %x\n", hard);
#endif
			return hard;
			
	//	case 0x1f801050: hard = serial_read32(); break;//serial port
#ifdef PSXHW_LOG
		case 0x1f801060:
			PSXHW_LOG("RAM size read %x\n", psxHu32(0x1060));
			return psxHu32(0x1060);
#endif
#ifdef PSXHW_LOG
		case 0x1f801070: PSXHW_LOG("IREG 32bit read %x\n", psxHu32(0x1070));
			return psxHu32(0x1070);
#endif
#ifdef PSXHW_LOG
		case 0x1f801074: PSXHW_LOG("IMASK 32bit read %x\n", psxHu32(0x1074));
			return psxHu32(0x1074);
#endif

		case 0x1f801810:
			hard = GPU_readData();
#ifdef PSXHW_LOG
			PSXHW_LOG("GPU DATA 32bit read %x\n", hard);
#endif
			return hard;
		case 0x1f801814:
			//senquack - updated to PCSX Rearmed:
			gpuSyncPluginSR();
			hard = HW_GPU_STATUS;
			if (hSyncCount < 240 && (HW_GPU_STATUS & PSXGPU_ILACE_BITS) != PSXGPU_ILACE_BITS)
				hard |= PSXGPU_LCF & (psxRegs.cycle << 20);
#ifdef PSXHW_LOG
			PSXHW_LOG("GPU STATUS 32bit read %x\n", hard);
#endif
			return hard;

		case 0x1f801820:
			return mdecRead0();
		case 0x1f801824:
			return mdecRead1();

#ifdef PSXHW_LOG
		case 0x1f8010a0:
			PSXHW_LOG("DMA2 MADR 32bit read %x\n", psxHu32(0x10a0));
			return SWAPu32(HW_DMA2_MADR);
		case 0x1f8010a4:
			PSXHW_LOG("DMA2 BCR 32bit read %x\n", psxHu32(0x10a4));
			return SWAPu32(HW_DMA2_BCR);
		case 0x1f8010a8:
			PSXHW_LOG("DMA2 CHCR 32bit read %x\n", psxHu32(0x10a8));
			return SWAPu32(HW_DMA2_CHCR);
#endif

#ifdef PSXHW_LOG
		case 0x1f8010b0:
			PSXHW_LOG("DMA3 MADR 32bit read %x\n", psxHu32(0x10b0));
			return SWAPu32(HW_DMA3_MADR);
		case 0x1f8010b4:
			PSXHW_LOG("DMA3 BCR 32bit read %x\n", psxHu32(0x10b4));
			return SWAPu32(HW_DMA3_BCR);
		case 0x1f8010b8:
			PSXHW_LOG("DMA3 CHCR 32bit read %x\n", psxHu32(0x10b8));
			return SWAPu32(HW_DMA3_CHCR);
#endif

		// time for rootcounters :)
		case 0x1f801100:
			hard = psxRcntRcount(0);
#ifdef PSXHW_LOG
			PSXHW_LOG("T0 count read32: %x\n", hard);
#endif
			return hard;
		case 0x1f801104:
			hard = psxRcntRmode(0);
#ifdef PSXHW_LOG
			PSXHW_LOG("T0 mode read32: %x\n", hard);
#endif
			return hard;
		case 0x1f801108:
			hard = psxRcntRtarget(0);
#ifdef PSXHW_LOG
			PSXHW_LOG("T0 target read32: %x\n", hard);
#endif
			return hard;
		case 0x1f801110:
			hard = psxRcntRcount(1);
#ifdef PSXHW_LOG
			PSXHW_LOG("T1 count read32: %x\n", hard);
#endif
			return hard;
		case 0x1f801114:
			hard = psxRcntRmode(1);
#ifdef PSXHW_LOG
			PSXHW_LOG("T1 mode read32: %x\n", hard);
#endif
			return hard;
		case 0x1f801118:
			hard = psxRcntRtarget(1);
#ifdef PSXHW_LOG
			PSXHW_LOG("T1 target read32: %x\n", hard);
#endif
			return hard;
		case 0x1f801120:
			hard = psxRcntRcount(2);
#ifdef PSXHW_LOG
			PSXHW_LOG("T2 count read32: %x\n", hard);
#endif
			return hard;
		case 0x1f801124:
			hard = psxRcntRmode(2);
#ifdef PSXHW_LOG
			PSXHW_LOG("T2 mode read32: %x\n", hard);
#endif
			return hard;
		case 0x1f801128:
			hard = psxRcntRtarget(2);
#ifdef PSXHW_LOG
			PSXHW_LOG("T2 target read32: %x\n", hard);
#endif
			return hard;

		default:
			hard = psxHu32(add); 
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unkwnown 32bit read at address %x\n", add);
#endif
			return hard;
		}
	}

#ifdef PSXREC
	// See note at top of file regarding dynarecs needing added functionality.
	if ((add & 0x0ff00000) == 0x0fc00000) {
		// ROM access
		return psxRu32(add);
	} else {
		// Cache control port read - mimic original psxmem.cpp behavior and return 0
		return 0;
	}
#endif //PSXREC

	return hard;
}

void psxHwWrite8(u32 add, u8 value)
{
	if ((add & 0x0ff00000) == 0x0f800000)
	{
		switch (add) {
		case 0x1f801040: sioWrite8(value); break;
	//	case 0x1f801050: serial_write8(value); break;//serial port
		case 0x1f801800: cdrWrite0(value); break;
		case 0x1f801801: cdrWrite1(value); break;
		case 0x1f801802: cdrWrite2(value); break;
		case 0x1f801803: cdrWrite3(value); break;

		default:
			psxHu8(add) = value;
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unknown 8bit write at address %x value %x\n", add, value);
#endif
			return;
		}

		// NOTE: Yes, the messy and uncommented original code writes to psxH[]
		//       even when the port address is known. I won't change this behavior
		//       because it's unknown what original intent was. -senquack Aug 2017

		psxHu8(add) = value;
	}
#ifdef PSXHW_LOG
	PSXHW_LOG("*Known 8bit write at address %x value %x\n", add, value);
#endif
}

void psxHwWrite16(u32 add, u16 value)
{
	if ((add & 0x0ff00000) == 0x0f800000)
	{
		switch (add) {
		case 0x1f801040:
			sioWrite16(value);
#ifdef PAD_LOG
			PAD_LOG ("sio write16 %x, %x\n", add&0xf, value);
#endif
			return;
		case 0x1f801044:
			// Function is empty, disabled -senquack
			//sioWriteStat16(value);
#ifdef PAD_LOG
			PAD_LOG ("sio write16 %x, %x\n", add&0xf, value);
#endif
			return;
		case 0x1f801048:
			sioWriteMode16(value);
#ifdef PAD_LOG
			PAD_LOG ("sio write16 %x, %x\n", add&0xf, value);
#endif
			return;
		case 0x1f80104a: // control register
			sioWriteCtrl16(value);
#ifdef PAD_LOG
			PAD_LOG ("sio write16 %x, %x\n", add&0xf, value);
#endif
			return;
		case 0x1f80104e: // baudrate register
			sioWriteBaud16(value);
#ifdef PAD_LOG
			PAD_LOG ("sio write16 %x, %x\n", add&0xf, value);
#endif
			return;

		//serial port ;P
	//  case 0x1f801050: serial_write16(value); break;
	//	case 0x1f80105a: serial_control_write(value);break;
	//	case 0x1f80105e: serial_baud_write(value); break;
	//	case 0x1f801054: serial_status_write(value); break;

		case 0x1f801070: 
#ifdef PSXHW_LOG
			PSXHW_LOG("IREG 16bit write %x\n", value);
#endif
			//senquack - Strip all but bits 0:10, rest are 0 or garbage in docs
			value &= 0x7ff;

			//senquack - added Config.SpuIrq option from PCSX Rearmed/Reloaded:
			if (Config.SpuIrq) psxHu16ref(0x1070) |= SWAPu16(0x200);

			psxHu16ref(0x1070) &= SWAPu16(value);

			//senquack - When IRQ is pending and unmasked, ensure psxBranchTest()
			// gets called as soon as possible, so HW IRQ exception gets handled
			if (psxHu16(0x1070) & psxHu16(0x1074))
				ResetIoCycle();

			return;

		case 0x1f801074:
#ifdef PSXHW_LOG
			PSXHW_LOG("IMASK 16bit write %x\n", value);
#endif
			//senquack - Strip all but bits 0:10, rest are 0 or garbage in docs
			value &= 0x7ff;

			psxHu16ref(0x1074) = SWAPu16(value);

			//senquack - When IRQ is pending and unmasked, ensure psxBranchTest()
			// gets called as soon as possible, so HW IRQ exception gets handled
			if (psxHu16(0x1070) & psxHu16(0x1074))
				ResetIoCycle();

			return;

		case 0x1f801100:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 0 COUNT 16bit write %x\n", value);
#endif
			psxRcntWcount(0, value);
			return;
		case 0x1f801104:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 0 MODE 16bit write %x\n", value);
#endif
			psxRcntWmode(0, value);
			return;
		case 0x1f801108:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 0 TARGET 16bit write %x\n", value);
#endif
			psxRcntWtarget(0, value);
			return;

		case 0x1f801110:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 1 COUNT 16bit write %x\n", value);
#endif
			psxRcntWcount(1, value);
			return;
		case 0x1f801114:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 1 MODE 16bit write %x\n", value);
#endif
			psxRcntWmode(1, value);
			return;
		case 0x1f801118:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 1 TARGET 16bit write %x\n", value);
#endif
			psxRcntWtarget(1, value);
			return;

		case 0x1f801120:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 2 COUNT 16bit write %x\n", value);
#endif
			psxRcntWcount(2, value);
			return;
		case 0x1f801124:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 2 MODE 16bit write %x\n", value);
#endif
			psxRcntWmode(2, value);
			return;
		case 0x1f801128:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 2 TARGET 16bit write %x\n", value);
#endif
			psxRcntWtarget(2, value); 
			return;

		default:
			if (add>=0x1f801c00 && add<0x1f801e00) {
				SPU_writeRegister(add, value, psxRegs.cycle);
				return;
			}

			psxHu16ref(add) = SWAPu16(value);
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unknown 16bit write at address %x value %x\n", add, value);
#endif
			return;
		}
	}
}

#define DmaExec(n) { \
	HW_DMA##n##_CHCR = SWAPu32(value); \
\
	if (SWAPu32(HW_DMA##n##_CHCR) & 0x01000000 && SWAPu32(HW_DMA_PCR) & (8 << (n * 4))) { \
		psxDma##n(SWAPu32(HW_DMA##n##_MADR), SWAPu32(HW_DMA##n##_BCR), SWAPu32(HW_DMA##n##_CHCR)); \
	} \
}

void psxHwWrite32(u32 add, u32 value)
{
	if ((add & 0x0ff00000) == 0x0f800000)
	{
		switch (add) {
		case 0x1f801040:
			sioWrite32(value);
#ifdef PAD_LOG
			PAD_LOG("sio write32 %x\n", value);
#endif
			return;
	//	case 0x1f801050: serial_write32(value); break;//serial port
#ifdef PSXHW_LOG
		case 0x1f801060:
			PSXHW_LOG("RAM size write %x\n", value);
			psxHu32ref(add) = SWAPu32(value);
			return; // Ram size
#endif

		case 0x1f801070: 
#ifdef PSXHW_LOG
			PSXHW_LOG("IREG 32bit write %x\n", value);
#endif
			//senquack - Strip all but bits 0:10, rest are 0 or garbage in docs
			value &= 0x7ff;

			//senquack - added Config.SpuIrq option from PCSX Rearmed/Reloaded:
			if (Config.SpuIrq) psxHu32ref(0x1070) |= SWAPu32(0x200);

			psxHu32ref(0x1070) &= SWAPu32(value);

			//senquack - When IRQ is pending and unmasked, ensure psxBranchTest()
			// gets called as soon as possible, so HW IRQ exception gets handled
			if (psxHu32(0x1070) & psxHu32(0x1074))
				ResetIoCycle();

			return;
		case 0x1f801074:
#ifdef PSXHW_LOG
			PSXHW_LOG("IMASK 32bit write %x\n", value);
#endif
			//senquack - Strip all but bits 0:10, rest are 0 or garbage in docs
			value &= 0x7ff;

			psxHu32ref(0x1074) = SWAPu32(value);

			//senquack - When IRQ is pending and unmasked, ensure psxBranchTest()
			// gets called as soon as possible, so HW IRQ exception gets handled
			if (psxHu32(0x1070) & psxHu32(0x1074))
				ResetIoCycle();

			return;

#ifdef PSXHW_LOG
		case 0x1f801080:
			PSXHW_LOG("DMA0 MADR 32bit write %x\n", value);
			HW_DMA0_MADR = SWAPu32(value); // DMA0 madr
			return;
		case 0x1f801084:
			PSXHW_LOG("DMA0 BCR 32bit write %x\n", value);
			HW_DMA0_BCR  = SWAPu32(value); // DMA0 bcr
			return;
#endif
		case 0x1f801088:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA0 CHCR 32bit write %x\n", value);
#endif
			DmaExec(0);	                 // DMA0 chcr (MDEC in DMA)
			return;

#ifdef PSXHW_LOG
		case 0x1f801090:
			PSXHW_LOG("DMA1 MADR 32bit write %x\n", value);
			HW_DMA1_MADR = SWAPu32(value); // DMA1 madr
			return;
		case 0x1f801094:
			PSXHW_LOG("DMA1 BCR 32bit write %x\n", value);
			HW_DMA1_BCR  = SWAPu32(value); // DMA1 bcr
			return;
#endif
		case 0x1f801098:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA1 CHCR 32bit write %x\n", value);
#endif
			DmaExec(1);                  // DMA1 chcr (MDEC out DMA)
			return;

#ifdef PSXHW_LOG
		case 0x1f8010a0:
			PSXHW_LOG("DMA2 MADR 32bit write %x\n", value);
			HW_DMA2_MADR = SWAPu32(value); // DMA2 madr
			return;
		case 0x1f8010a4:
			PSXHW_LOG("DMA2 BCR 32bit write %x\n", value);
			HW_DMA2_BCR  = SWAPu32(value); // DMA2 bcr
			return;
#endif
		case 0x1f8010a8:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA2 CHCR 32bit write %x\n", value);
#endif
			DmaExec(2);                  // DMA2 chcr (GPU DMA)
			return;

#ifdef PSXHW_LOG
		case 0x1f8010b0:
			PSXHW_LOG("DMA3 MADR 32bit write %x\n", value);
			HW_DMA3_MADR = SWAPu32(value); // DMA3 madr
			return;
		case 0x1f8010b4:
			PSXHW_LOG("DMA3 BCR 32bit write %x\n", value);
			HW_DMA3_BCR  = SWAPu32(value); // DMA3 bcr
			return;
#endif
		case 0x1f8010b8:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA3 CHCR 32bit write %x\n", value);
#endif
			DmaExec(3);                  // DMA3 chcr (CDROM DMA)
			
			return;

#ifdef PSXHW_LOG
		case 0x1f8010c0:
			PSXHW_LOG("DMA4 MADR 32bit write %x\n", value);
			HW_DMA4_MADR = SWAPu32(value); // DMA4 madr
			return;
		case 0x1f8010c4:
			PSXHW_LOG("DMA4 BCR 32bit write %x\n", value);
			HW_DMA4_BCR  = SWAPu32(value); // DMA4 bcr
			return;
#endif
		case 0x1f8010c8:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA4 CHCR 32bit write %x\n", value);
#endif
			DmaExec(4);                  // DMA4 chcr (SPU DMA)
			return;

#ifdef PSXHW_LOG
		case 0x1f8010e0:
			PSXHW_LOG("DMA6 MADR 32bit write %x\n", value);
			HW_DMA6_MADR = SWAPu32(value); // DMA6 bcr
			return;
		case 0x1f8010e4:
			PSXHW_LOG("DMA6 BCR 32bit write %x\n", value);
			HW_DMA6_BCR  = SWAPu32(value); // DMA6 bcr
			return;
#endif
		case 0x1f8010e8:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA6 CHCR 32bit write %x\n", value);
#endif
			DmaExec(6);                   // DMA6 chcr (OT clear)
			return;

#ifdef PSXHW_LOG
		case 0x1f8010f0:
			PSXHW_LOG("DMA PCR 32bit write %x\n", value);
			HW_DMA_PCR = SWAPu32(value);
			return;
#endif

		case 0x1f8010f4:
#ifdef PSXHW_LOG
			PSXHW_LOG("DMA ICR 32bit write %x\n", value);
#endif
		{
			u32 tmp = value & 0x00ff803f;
			tmp |= (SWAPu32(HW_DMA_ICR) & ~value) & 0x7f000000;
			if ((tmp & HW_DMA_ICR_GLOBAL_ENABLE && tmp & 0x7f000000)
			    || tmp & HW_DMA_ICR_BUS_ERROR) {
				if (!(SWAPu32(HW_DMA_ICR) & HW_DMA_ICR_IRQ_SENT))
					psxHu32ref(0x1070) |= SWAP32(8);
				tmp |= HW_DMA_ICR_IRQ_SENT;
			}
			HW_DMA_ICR = SWAPu32(tmp);
			return;
		}

		case 0x1f801810:
#ifdef PSXHW_LOG
			PSXHW_LOG("GPU DATA 32bit write %x\n", value);
#endif
			GPU_writeData(value);
			return;
		case 0x1f801814:
			//senquack - updated to PCSX Rearmed:
#ifdef PSXHW_LOG
			PSXHW_LOG("GPU STATUS 32bit write %x\n", value);
#endif
			GPU_writeStatus(value);
			gpuSyncPluginSR();

			return;

		case 0x1f801820:
			mdecWrite0(value); break;
		case 0x1f801824:
			mdecWrite1(value); break;

		case 0x1f801100:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 0 COUNT 32bit write %x\n", value);
#endif
			psxRcntWcount(0, value & 0xffff);
			return;
		case 0x1f801104:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 0 MODE 32bit write %x\n", value);
#endif
			psxRcntWmode(0, value);
			return;
		case 0x1f801108:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 0 TARGET 32bit write %x\n", value);
#endif
			psxRcntWtarget(0, value & 0xffff); //  HW_DMA_ICR&= SWAP32((~value)&0xff000000);
			return;

		case 0x1f801110:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 1 COUNT 32bit write %x\n", value);
#endif
			psxRcntWcount(1, value & 0xffff);
			return;
		case 0x1f801114:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 1 MODE 32bit write %x\n", value);
#endif
			psxRcntWmode(1, value);
			return;
		case 0x1f801118:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 1 TARGET 32bit write %x\n", value);
#endif
			psxRcntWtarget(1, value & 0xffff);
			return;

		case 0x1f801120:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 2 COUNT 32bit write %x\n", value);
#endif
			psxRcntWcount(2, value & 0xffff);
			return;
		case 0x1f801124:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 2 MODE 32bit write %x\n", value);
#endif
			psxRcntWmode(2, value);
			return;
		case 0x1f801128:
#ifdef PSXHW_LOG
			PSXHW_LOG("COUNTER 2 TARGET 32bit write %x\n", value);
#endif
			psxRcntWtarget(2, value & 0xffff);
			return;

		default:
			// Dukes of Hazard 2 - car engine noise
			if (add>=0x1f801c00 && add<0x1f801e00) {
				SPU_writeRegister(add, value&0xffff, psxRegs.cycle);
				SPU_writeRegister(add + 2, value>>16, psxRegs.cycle);
				return;
			}

			psxHu32ref(add) = SWAPu32(value);
#ifdef PSXHW_LOG
			PSXHW_LOG("*Unknown 32bit write at address %x value %x\n", add, value);
#endif
			return;
		}
	}

#ifdef PSXREC
	// See note at top of file regarding dynarecs needing added functionality.
	if (add == 0xfffe0130) {
		psxMemWrite32_CacheCtrlPort(value);
		return;
	}
#endif

}

int psxHwFreeze(void* f, FreezeMode mode) {
	return 0;
}
