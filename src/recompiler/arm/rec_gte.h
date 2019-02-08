/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Franxis and Chui                                   *
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

#include "gte.h"

// July 2017: Some GTE instructions encode parameters inside their u32 opcode.
//            The funcs for these instructions now take the opcode value as
//            parameter shifted right 10 places (only lower 16 bits are used).
//            The ARM dynarecs are not maintained and haven't been updated:
#error "ARM dynarec has not been updated to match new GTE interface"


#ifdef DEBUG_CPU
#define dbgte(name) dbg("\t\trec" name)
#else
#define dbgte(name)
#endif

// #define USE_OLD_GTE_WITHOUT_PATCH

static unsigned func_GTE_updateMACs_lm0_ptr=0;
static unsigned func_GTE_updateMACs_lm1_ptr=0;
static unsigned func_GTE_updateMACs_lm0_shift12_ptr=0;
static unsigned func_GTE_updateMACs_lm1_shift12_ptr=0;
static unsigned func_GTE_updateMAC3_lm0_flag_ptr=0;
#ifdef USE_GTE_FLAG
#define recGTE_updateMACs(LM,SHIFT) recGTE_updateMACs_flag(LM,SHIFT,1)
#define func_GTE_updateMACs_lm0_flag_ptr func_GTE_updateMACs_lm0_ptr
#define func_GTE_updateMACs_lm1_flag_ptr func_GTE_updateMACs_lm1_ptr
#else
#define recGTE_updateMACs(LM,SHIFT) recGTE_updateMACs_flag(LM,SHIFT,0)
static unsigned func_GTE_updateMACs_lm0_flag_ptr=0;
static unsigned func_GTE_updateMACs_lm1_flag_ptr=0;
#endif


#define MapCtrlGte(CREG,GREG) MapGte((CREG),(GREG))
#define MapDataGte(CREG,GREG) MapGte((CREG),(GREG)+32)
#define GetGteCtrlMapped(GREG) GetGteMapped((GREG))
#define GetGteDataMapped(GREG) GetGteMapped((GREG)+32)
#define UnmapCtrlGte(GREG) UnmapGte((GREG))
#define UnmapDataGte(GREG) UnmapGte((GREG)+32)
#define PullGteCtrlMapped(CREG, GREG, HREG) PullGteMapped((CREG), (GREG), (HREG)) 
#define PullGteDataMapped(CREG, GREG, HREG) PullGteMapped((CREG), (GREG)+32, (HREG)) 

#if (!defined(gte_new) || !defined(REC_USE_GTECALC_INLINE)) && defined(REC_USE_GTE_MAP_REGS)
#undef REC_USE_GTE_MAP_REGS
#endif

#ifdef REC_USE_GTE_MAP_REGS

static u8 iMapGteReg[64];

static u32 GetGteMapped(u32 gte_reg) {
	return iMapGteReg[gte_reg];
}

static u32 GetGteMappedReg(u32 cpu_reg) {
	u32 i;
	for(i=0;i<64;i++) {
		if (iMapGteReg[i] == cpu_reg)
			return i;
	}
	return 0xff;
}

static void MapGte(u32 cpu_reg, u32 gte_reg) {
	iMapGteReg[gte_reg]=cpu_reg;
}

static void UnmapGte(u32 gte_reg) {
	iMapGteReg[gte_reg]=0;
}

static void UnmapGteReg(u32 cpu_reg) {
	u32 i;
	for(i=0;i<64;i++) {
		if (iMapGteReg[i]==cpu_reg)
			iMapGteReg[i]=0;
	}
}

static void ResetMapGteRegs(void) {
	u32 i;
	for(i=0;i<64;i++)
		iMapGteReg[i]=0;
}

static void UnmapDataGteMACs(void) {
	UnmapDataGte(9);  // gteIR1
	UnmapDataGte(10); // gteIR2
	UnmapDataGte(11); // gteIR3
	UnmapDataGte(25); // gteMAC1
	UnmapDataGte(26); // gteMAC2
	UnmapDataGte(27); // gteMAC3
}

static void UnmapDataGteCODEs(void) {
	u32 rmap1=GetGteDataMapped(21); // gteRGB1
	u32 rmap2=GetGteDataMapped(22); // gteRGB2
	if (rmap1 && rmap1!=rmap2) {
		MapDataGte(rmap1,20);
	} else {
		UnmapDataGte(20);
	}
	if (rmap2 && rmap1!=rmap2) {
		MapDataGte(rmap2,21);
	} else {
		UnmapDataGte(21);
	}
	UnmapDataGte(22); // gteRGB2
}

static void PullGteMapped(u32 cpu_reg, u32 gte_reg, u32 host_reg) {
		if (cpu_reg && (IsConst(cpu_reg)||IsMapped(cpu_reg))) {
			if (IsConst(cpu_reg)) {
				MOV32ItoR(host_reg,iRegs[cpu_reg].k);
			} else {
				MOV32RtoR(host_reg,iRegs[cpu_reg].reg);
			}
		} else {
			if (cpu_reg) {
				MOV32MtoR_regs(host_reg,(u32)&psxRegs.GPR.r[cpu_reg]);
			} else {
				if (gte_reg<32) {
					MOV32MtoR_regs(host_reg,(u32)&psxRegs.CP2C.r[gte_reg]);
				} else {
					MOV32MtoR_regs(host_reg,(u32)&psxRegs.CP2D.r[gte_reg-32]);
				}
			}
		}
}

#else
static u32 GetGteMapped(u32 gte_reg) { return 0; }
static u32 GetGteMappedReg(u32 cpu_reg) { return 0xff; }
static void MapGte(u32 cpu_reg, u32 gte_reg) { }
static void UnmapGte(u32 gte_reg) { }
static void UnmapGteReg(u32 cpu_reg) { }
static void ResetMapGteRegs(void) { }
static void UnmapDataGteMACs(void) { }
static void UnmapDataGteCODEs(void) { }
static void PullGteMapped(u32 cpu_reg, u32 gte_reg, u32 host_reg) {
	if (gte_reg<32) {
		MOV32MtoR_regs(host_reg,(u32)&psxRegs.CP2C.r[gte_reg]);
	} else {
		MOV32MtoR_regs(host_reg,(u32)&psxRegs.CP2D.r[gte_reg-32]);
	}
}
#endif

#if !defined(gte_new) || !defined(REC_USE_GTE_FUNCS) || !defined(REC_USE_GTECALC_INLINE) || defined(USE_GTE_FLAG)
#ifdef REC_USE_GTE_DELAY_CALC
#undef REC_USE_GTE_DELAY_CALC
#endif
static void UpdateGteDelay(int clear){ }
#else
#ifdef REC_USE_GTE_DELAY_CALC
static void UpdateGteDelay(int clear){
	if (func_GTE_delay_ptr) {
		int holded=(HWRegs[3].state==ST_HOLD);
		if (!holded)
			iLockReg(3);
		CALLFunc(func_GTE_delay_ptr);
		if (clear)
			func_GTE_delay_ptr=0;
		if (!holded)
			iUnlockReg(3);
		r2_is_dirty=1;
	}
}
#else
static void UpdateGteDelay(int clear){ }
#endif
#endif

#ifdef gte_new
#define CP2_FUNC(f,name,cycle) \
static void rec##f() { \
	if (autobias) cycles_pending+=cycle; \
	if (rec_phase) { \
		/*iFlushRegs();*/ \
		dbgte(name) \
		iLockReg(3); \
		MOV32ItoM_regs((u32)&psxRegs.code, (u32)psxRegs.code); \
		MOV32RtoR(HOST_r0,HOST_r11); \
		CALLFunc((u32)_gte##f); \
		iUnlockReg(3); \
		r2_is_dirty = 1; \
	/*	branch = 2; */ \
	} \
}

#define CP2_FUNCNC(f,name,cycle) \
static void rec##f() { \
	if (autobias) cycles_pending+=cycle; \
	if (rec_phase) { \
		/*iFlushRegs();*/ \
		dbgte(name) \
		iLockReg(3); \
		MOV32RtoR(HOST_r0,HOST_r11); \
		CALLFunc((u32)_gte##f); \
		iUnlockReg(3); \
		r2_is_dirty = 1; \
	/*	branch = 2; */ \
	} \
}
#else
#define CP2_FUNC(f,name,cycle) \
void gte##f(); \
static void rec##f() { \
	if (autobias) cycles_pending+=cycle; \
	if (rec_phase) { \
		/*iFlushRegs();*/ \
		dbgte(name) \
		iLockReg(3); \
		MOV32ItoM_regs((u32)&psxRegs.code, (u32)psxRegs.code); \
		CALLFunc((u32)gte##f); \
		iUnlockReg(3); \
		r2_is_dirty = 1; \
	/*	branch = 2; */ \
	} \
}

#define CP2_FUNCNC(f,name,cycle) \
void gte##f(); \
static void rec##f() { \
	if (autobias) cycles_pending+=cycle; \
	if (rec_phase) { \
		/*iFlushRegs();*/ \
		dbgte(name) \
		iLockReg(3); \
		CALLFunc((u32)gte##f); \
		iUnlockReg(3); \
		r2_is_dirty = 1; \
	/*	branch = 2; */ \
	} \
}
#endif

static unsigned func_GTE_MFC2_29_ptr=0;
static void recGTE_MFC2_29(u32 rt);

static void recMFC2(void) {
	if (autobias) cycles_pending+=2;
	if (!rec_phase) {
		WriteReg(_Rt_);
#ifdef REC_USE_GTECALC_INLINE
		if (_Rd_==30 || _Rd_==28) {
			MapConst(_Rt_,0);
		}
#endif
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecMFC2 R%i=CP2D%i\n",_Rt_,_Rd_);
#endif
#ifndef gte_new
	if (_Rt_) {
		iLockReg(3);
		MOV32ItoR(HOST_r0,_Rd_);
		CALLFunc((u32)gtecalcMFC2);
		iUnlockReg(3);
		r2_is_dirty=1;
		u32 rt=WriteReg(_Rt_);
		MOV32RtoR(rt,HOST_r0);
	}
#else
#ifdef REC_USE_GTECALC_INLINE
	if (!_Rt_) return;
#ifdef REC_USE_GTE_DELAY_CALC
#ifdef USE_OLD_GTE_WITHOUT_PATCH
	if (_Rd_==29 || _Rd_==9 || _Rd_==10 || _Rd_==11) {
#else
	if (_Rd_==29 || _Rd_==28 || _Rd_==9 || _Rd_==10 || _Rd_==11) {
#endif
		iLockReg(3);
	}
#endif
	u32 rt=WriteReg(_Rt_);
	UnmapGteReg(_Rt_);
	switch(_Rd_) {
		case 9:
		case 10:
		case 11:
			UpdateGteDelay(1);
		case 1:
		case 3:
		case 5:
		case 8:
			MOV16sMtoR_regs(rt,(u32)&psxRegs.CP2D.p[_Rd_].sw.l); 
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[_Rd_],rt); // regs->CP2D.r[reg] = (s32)regs->CP2D.p[reg].sw.l;
			UnmapDataGte(_Rd_);
			break;
		case 7:
		case 16:
		case 17:
		case 18:
		case 19:
			MOV16MtoR_regs(rt,(u32)&psxRegs.CP2D.p[_Rd_].sw.l); 
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[_Rd_],rt); // regs->CP2D.r[reg] = (u32)regs->CP2D.p[reg].w.l;
			UnmapDataGte(_Rd_);
			break;
		case 15:
			{
#ifdef REC_USE_GTE_MAP_REGS
				u32 rmap=GetGteDataMapped(14);
				if (rmap && rmap!=rt) {
					MOV32RtoR(rt,rmap);
				} else {
#else
				{
#endif
					MOV32MtoR_regs(rt,(u32)&psxRegs.CP2D.r[14]); 
				}
				MOV32RtoM_regs((u32)&psxRegs.CP2D.r[_Rd_],rt); // regs->CP2D.r[reg] = gteSXY2;
				UnmapDataGte(_Rd_);
			}
			break;
		case 28:
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		case 30:
			MapConst(_Rt_,0);
			break;
#endif
		case 29:
			UpdateGteDelay(1);
#ifdef REC_USE_GTE_FUNCS
			CALLFunc(func_GTE_MFC2_29_ptr);
			MOV32RtoR(rt,HOST_r0);
#else
			recGTE_MFC2_29(rt);
#endif
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[_Rd_],rt); // regs->CP2D.r[reg] = rt
			UnmapDataGte(_Rd_);
			break;
		default:
			{
#ifdef REC_USE_GTE_MAP_REGS
				u32 rmap=GetGteDataMapped(_Rd_);
#ifdef REC_USE_GTE_DELAY_CALC
#ifdef USE_OLD_GTE_WITHOUT_PATCH
				if (rmap && (!(_Rd_==29 || _Rd_==9 || _Rd_==10 || _Rd_==11)) && (IsConst(rmap) || IsMapped(rmap)) ) {
#else
				if (rmap && (!(_Rd_==29 || _Rd_==28 || _Rd_==9 || _Rd_==10 || _Rd_==11)) && (IsConst(rmap) || IsMapped(rmap)) ) {
#endif
#else
				if (rmap && (IsConst(rmap) || IsMapped(rmap)) ) {
#endif
					if (IsConst(rmap)) {
						MOV32ItoR(rt,iRegs[rmap].k);
					} else {
						MOV32RtoR(rt,iRegs[rmap].reg);
					}
				} else {
#else
				{
#endif
					MOV32MtoR_regs(rt,(u32)&psxRegs.CP2D.r[_Rd_]); 
				}
			}
	}
#ifdef USE_OLD_GTE_WITHOUT_PATCH
	if (_Rt_!=28 && _Rt_!=30)
#endif
	{
		MapDataGte(_Rt_,_Rd_);
	}
#ifdef REC_USE_GTE_DELAY_CALC
#ifdef USE_OLD_GTE_WITHOUT_PATCH
	if (_Rd_==29 || _Rd_==9 || _Rd_==10 || _Rd_==11) {
#else
	if (_Rd_==29 || _Rd_==28 || _Rd_==9 || _Rd_==10 || _Rd_==11) {
#endif
		iUnlockReg(3);
	}
#endif

#else
	if (_Rt_) {
		iLockReg(3);
		MOV32ItoR(HOST_r0,_Rd_);
		MOV32RtoR(HOST_r1,HOST_r11);
		CALLFunc((u32)_gtecalcMFC2);
		iUnlockReg(3);
		r2_is_dirty=1;
		u32 rt=WriteReg(_Rt_);
		MOV32RtoR(rt,HOST_r0);
	}
#endif
#endif

}

static unsigned func_GTE_MTC2_15_ptr=0;
static unsigned func_GTE_MTC2_28_ptr=0;
static void recGTE_MTC2_15(u32 rt);
static void recGTE_MTC2_28(u32 rt);

static void recMTC2(void) {
	if (autobias) cycles_pending+=2;
	if (!rec_phase) {
		if (!IsConst(_Rt_)) {
			ReadReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecMTC2 CP2D%i=R%i\n",_Rd_,_Rt_);
#endif

#ifndef gte_new
	iLockReg(3);
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_r0,iRegs[_Rt_].k);
	}
	else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_r0,rt);
	}
	MOV32ItoR(HOST_r1,_Rd_);
	CALLFunc((u32)gtecalcMTC2);
	iUnlockReg(3);
	r2_is_dirty=1;
#else
#ifdef REC_USE_GTECALC_INLINE
	unsigned rt;
#ifdef REC_USE_GTE_DELAY_CALC
	if (_Rd_==11) {
		iLockReg(3);
	}
#endif
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_r1,iRegs[_Rt_].k);
		rt=HOST_r1;
	} else {
		rt=ReadReg(_Rt_);
	}
	switch (_Rd_) {
		case 15:
#ifdef REC_USE_GTE_FUNCS
			if (rt!=HOST_r1) {
				MOV32RtoR(HOST_r1,rt);
			}
			CALLFunc(func_GTE_MTC2_15_ptr);
#else
			recGTE_MTC2_15(rt);
#endif
#ifdef REC_USE_GTE_MAP_REGS
			{
				u32 rmap1=GetGteDataMapped(13); // gteSXY1
				u32 rmap2=GetGteDataMapped(14); // gteSXY2
				if (rmap1 && rmap1!=_Rt_ && rmap1!=rmap2) {
					MapDataGte(rmap1,12); // gteSXY0 = gteSXY1;
				} else {
					UnmapDataGte(12);
				}
				if (rmap2 && rmap2!=_Rt_ && rmap1!=rmap2) {
					MapDataGte(rmap2,13); // gteSXY1 = gteSXY2;
				} else {
					UnmapDataGte(13);
				}
				MapDataGte(_Rt_,14); // gteSXY2 = value;
				MapDataGte(_Rt_,15); // gteSXYP = value;
			}
#endif
			break;
		case 28:
			func_GTE_delay_ptr=0;
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[28],rt); // gteIRGB = value;
#ifdef REC_USE_GTE_FUNCS
			if (rt!=HOST_r1) {
				MOV32RtoR(HOST_r1,rt);
			}
			CALLFunc(func_GTE_MTC2_28_ptr);
#else
			recGTE_MTC2_28(rt);
#endif
#ifdef REC_USE_GTE_MAP_REGS
			MapDataGte(_Rt_,28); // gteIRGB = value;
			UnmapDataGte(9);
			UnmapDataGte(10);
			UnmapDataGte(11);
#endif
			break;
		case 30:
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[30],rt); // gteLZCS = value;
			write32(CLZ(HOST_r0,rt));
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[31],HOST_r0); // = gteLZCR
#ifdef REC_USE_GTE_MAP_REGS
			UnmapDataGte(30);
#endif
			break;
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		case 7:
		case 29:
#endif
		case 31:
			break;
		case 9:
		case 10:
		case 11:
		case 25:
		case 26:
		case 27:
			UpdateGteDelay(1);
		default:
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[_Rd_],rt); // regs->CP2D.r[_Rd_] = value;
#ifdef REC_USE_GTE_MAP_REGS
			MapDataGte(_Rt_,_Rd_);
#endif
	}
#ifdef REC_USE_GTE_DELAY_CALC
	if (_Rd_==11) {
		iUnlockReg(3);
	}
#endif
#else
	iLockReg(3);
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_r0,iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_r0,rt);
	}
	MOV32ItoR(HOST_r1,_Rd_);
	MOV32RtoR(HOST_r2,HOST_r11);
	CALLFunc((u32)_gtecalcMTC2);
	iUnlockReg(3);
	r2_is_dirty=1;
#endif
#endif
}

static void recCFC2(void) {
	if (autobias) cycles_pending+=2;
	if (!rec_phase) {
		WriteReg(_Rt_);
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecCFC2");
#endif

	if (_Rt_) {
		if (_Rd_==31) {
#if defined(REC_USE_GTE_DELAY_CALC) && defined(gte_new) && defined(REC_USE_GTE_FUNCS) && defined(REC_USE_GTECALC_INLINE)
#ifdef USE_GTE_FLAG
			UpdateGteDelay(1);
#else
			if ((func_GTE_delay_ptr==func_GTE_updateMAC3_lm0_flag_ptr) || (func_GTE_delay_ptr==func_GTE_updateMACs_lm0_flag_ptr) || (func_GTE_delay_ptr==func_GTE_updateMACs_lm1_flag_ptr)) {
				UpdateGteDelay(1);
			}
#endif
#endif
		}
		u32 rt=WriteReg(_Rt_);
		MOV32MtoR_regs(rt,&psxRegs.CP2C.r[_Rd_]);
#ifdef REC_USE_GTE_MAP_REGS
		UnmapGteReg(_Rt_);
		MapCtrlGte(_Rt_,_Rd_);
#endif
	}
}

static void recCTC2(void) {
	if (autobias) cycles_pending+=2;
	if (!rec_phase) {
		if (!IsConst(_Rt_)) {
			ReadReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecCTC2");
#endif

#ifndef gte_new
	iLockReg(3);
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_r0,iRegs[_Rt_].k);
	}
	else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_r0,rt);
	}
	MOV32ItoR(HOST_r1,_Rd_);
	CALLFunc((u32)gtecalcCTC2);
	iUnlockReg(3);
	r2_is_dirty=1;
#else
#ifdef REC_USE_GTECALC_INLINE
	if (IsConst(_Rt_)) {
		u32 value=iRegs[_Rt_].k;
		switch (_Rd_) {
			case 4:
			case 12:
			case 20:
			case 26:
			case 27:
			case 29:
			case 30:
				value = (s32)(s16)value;
				break;
			case 31:
#if defined(REC_USE_GTE_DELAY_CALC) && defined(gte_new) && defined(REC_USE_GTE_FUNCS) && defined(REC_USE_GTECALC_INLINE)
#ifdef USE_GTE_FLAG
				UpdateGteDelay(1);
#else
				if ((func_GTE_delay_ptr==func_GTE_updateMAC3_lm0_flag_ptr) || (func_GTE_delay_ptr==func_GTE_updateMACs_lm0_flag_ptr) || (func_GTE_delay_ptr==func_GTE_updateMACs_lm1_flag_ptr)) {
					UpdateGteDelay(1);
				}
#endif
#endif
				value = value & 0x7ffff000;
				if (value & 0x7f87e000) value |= 0x80000000;
				break;
		}
		MOV32ItoM_regs(&psxRegs.CP2C.r[_Rd_],value);
	}
	else {
#ifdef REC_USE_GTE_DELAY_CALC
		if (_Rd_==31) {
			iLockReg(3);
		}
#endif
		switch (_Rd_) {
			case 4:
			case 12:
			case 20:
			case 26:
			case 27:
			case 29:
			case 30:
				MOV32RtoR(HOST_r1,ReadReg(_Rt_));
				write32( MOV_REG_LSL_IMM(HOST_r1, HOST_r1, 16) );      // mov reg, reg, lsl #16
				write32( MOV_REG_ASR_IMM(HOST_r1, HOST_r1, 16) );      // mov reg, reg, asr #16
				MOV32RtoM_regs(&psxRegs.CP2C.r[_Rd_],HOST_r1);
				break;
			case 31:
				MOV32RtoR(HOST_r1,ReadReg(_Rt_));
#if defined(REC_USE_GTE_DELAY_CALC) && defined(gte_new) && defined(REC_USE_GTE_FUNCS) && defined(REC_USE_GTECALC_INLINE)
#ifdef USE_GTE_FLAG
				UpdateGteDelay(1);
#else
				if ((func_GTE_delay_ptr==func_GTE_updateMAC3_lm0_flag_ptr) || (func_GTE_delay_ptr==func_GTE_updateMACs_lm0_flag_ptr) || (func_GTE_delay_ptr==func_GTE_updateMACs_lm1_flag_ptr)) {
					UpdateGteDelay(1);
				}
#endif
#endif
				MOV32ItoR(HOST_r0,0x7ffff000);
				AND32(HOST_r1,HOST_r1,HOST_r0);
				MOV32ItoR(HOST_r0,0x7f87e000);
				AND32(HOST_r2,HOST_r0,HOST_r1);
				write32(CMP_IMM(HOST_r2,0,0));
				write32(ORRNE_IMM(HOST_r1,HOST_r1,2,2));
				MOV32RtoM_regs(&psxRegs.CP2C.r[_Rd_],HOST_r1);
				r2_is_dirty=1;
				break;
			default:
				{
					u32 rt=ReadReg(_Rt_);
					MOV32RtoM_regs(&psxRegs.CP2C.r[_Rd_], rt);
				}
		}
#ifdef REC_USE_GTE_DELAY_CALC
		if (_Rd_==31) {
			iUnlockReg(3);
		}
#endif
	}

#ifdef REC_USE_GTE_MAP_REGS
	if (_Rd_!=31) {
		MapCtrlGte(_Rt_,_Rd_);
	}
#endif
#else
	iLockReg(3);
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_r0,iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_r0,rt);
	}		
	MOV32ItoR(HOST_r1,_Rd_);
	MOV32RtoR(HOST_r2,HOST_r11);
	CALLFunc((u32)_gtecalcCTC2);
	iUnlockReg(3);
	r2_is_dirty=1;
#endif
#endif
}

static void recLWC2(void)
{
	if (!rec_phase) {
		if (autobias) cycles_pending+=3;
		if (!IsConst(_Rs_)) {
			ReadReg(_Rs_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecLWC2");
#endif
	if (autobias) cycles_pending+=2;

	if (_Rt_==9 || _Rt_==10 || _Rt_==11 || _Rt_==25 || _Rt_==26 || _Rt_==27) {
			UpdateGteDelay(1);
	}
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;
		int pasado=0;

		if ((t & 0xfff0) == 0xbfc0) {
			MOV32ItoR(HOST_r0, psxRu32(addr)); // since bios is readonly it won't change
			pasado=1;
		} else
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			MOV32MtoR(HOST_r0, (u32)&psxM[addr & 0x1fffff]);
			pasado=1;
		} else
		if (t == 0x1f80 && addr < 0x1f801000) {
			MOV32MtoR(HOST_r0, (u32)&psxH[addr & 0xfff]);
			pasado=1;
		} else
		if (t == 0x1f80) {
			switch (addr) {
				case 0x1f801080: case 0x1f801084: case 0x1f801088: 
				case 0x1f801090: case 0x1f801094: case 0x1f801098: 
				case 0x1f8010a0: case 0x1f8010a4: case 0x1f8010a8: 
				case 0x1f8010b0: case 0x1f8010b4: case 0x1f8010b8: 
				case 0x1f8010c0: case 0x1f8010c4: case 0x1f8010c8: 
				case 0x1f8010d0: case 0x1f8010d4: case 0x1f8010d8: 
				case 0x1f8010e0: case 0x1f8010e4: case 0x1f8010e8: 
				case 0x1f801070: case 0x1f801074:
				case 0x1f8010f0: case 0x1f8010f4:
					MOV32MtoR(HOST_r0, (u32)&psxH[addr & 0xffff]);
					pasado=1;
					break;
				case 0x1f801810:
					iLockReg(3);
					CALLFunc((u32)&GPU_readData);
					iUnlockReg(3);
					r2_is_dirty=1;
					pasado=1;
					break;
				case 0x1f801814:
					iLockReg(3);
					CALLFunc((u32)&GPU_readStatus);
					iUnlockReg(3);
					r2_is_dirty=1;
					pasado=1;
					break;
				default:
					MOV32MtoR(HOST_r0, (u32)&psxH[addr & 0xfff]);
			}
		}
		if ((!pasado)&&(t == 0x1f80 && addr >= 0x1f801000)) {
			iLockReg(3);
			if (autobias) cycles_pending+=1;
			iPutCyclesAdd(0);
			MOV32ItoR(HOST_r0, addr);
			CALLFunc((u32)psxHwRead32);
			iUnlockReg(3);
			r2_is_dirty=1;
		}
	} else {
		iLockReg(3);
		iPushOfB();
		PSXMEMREAD32(HOST_r0);
		iUnlockReg(3);
		r2_is_dirty=1;
	}
#ifndef gte_new
	iLockReg(3);
	MOV32ItoR(HOST_r1, _Rt_);
	CALLFunc((u32)gtecalcMTC2);
	iUnlockReg(3);
	r2_is_dirty=1;
#else
#ifdef REC_USE_GTECALC_INLINE
	unsigned rt=HOST_r0;
	switch (_Rt_) {
		case 15:
			MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.CP2D.r[13]); 
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[12],HOST_r0); // gteSXY0 = gteSXY1;
			MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.CP2D.r[14]); 
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[13],HOST_r0); // gteSXY1 = gteSXY2;
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[14],rt); // gteSXY2 = value;
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[15],rt); // gteSXYP = value;
#ifdef REC_USE_GTE_MAP_REGS
			{
				u32 rmap1=GetGteDataMapped(13); // gteSXY1
				u32 rmap2=GetGteDataMapped(14); // gteSXY2
				if (rmap1 && rmap1!=rmap2) {
					MapDataGte(rmap1,12); // gteSXY0 = gteSXY1;
				} else {
					UnmapDataGte(12);
				}
				if (rmap2 && rmap1!=rmap2) {
					MapDataGte(rmap2,13); // gteSXY1 = gteSXY2;
				} else {
					UnmapDataGte(13);
				}
				UnmapDataGte(14); // gteSXY2 = value;
				UnmapDataGte(15); // gteSXYP = value;
			}
#endif
			break;
		case 28:
			func_GTE_delay_ptr=0;
			MOV32ItoM_regs((u32)&psxRegs.CP2D.r[28],rt); // gteIRGB = value;
			MOV32RtoR(HOST_r0,rt);
			AND32ItoR(HOST_r0,0x1f);
			SHLI32(HOST_r0,HOST_r0,7);
			MOV16RtoM_regs((u32)&psxRegs.CP2D.p[9].sw.l,HOST_r0); // gteIR1 = (value & 0x1f) << 7;
			MOV32RtoR(HOST_r0,rt);
			AND32ItoR(HOST_r0,0x3e0);
			SHLI32(HOST_r0,HOST_r0,2);
			MOV16RtoM_regs((u32)&psxRegs.CP2D.p[10].sw.l,HOST_r0); // gteIR2 = (value & 0x3e0) << 2;
			MOV32RtoR(HOST_r0,rt);
			AND32ItoR(HOST_r0,0x7c00);
			SHRI32(HOST_r0,HOST_r0,3);
			MOV16RtoM_regs((u32)&psxRegs.CP2D.p[11].sw.l,HOST_r0); // gteIR3 = (value & 0x7c00) >> 3;
#ifdef REC_USE_GTE_MAP_REGS
			UnmapDataGte(9);
			UnmapDataGte(10);
			UnmapDataGte(11);
			UnmapDataGte(28);
#endif

			break;
		case 30:
			MOV32RtoR(HOST_r0,rt);
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[30],HOST_r0); // gteLZCS = value;
			write32(0xe16f0f10); // clz     r0, r0
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[31],HOST_r0); // = gteLZCR
#ifdef REC_USE_GTE_MAP_REGS
			UnmapDataGte(30);
#endif
			break;
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		case 7:
		case 29:
#endif
		case 31:
			break;
		case 9:
		case 10:
		case 11:
		default:
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[_Rt_],rt); // regs->CP2D.r[_Rd_] = value;
#ifdef REC_USE_GTE_MAP_REGS
			UnmapDataGte(_Rt_);
#endif
	}
#else
	iLockReg(3);
	MOV32ItoR(HOST_r1, _Rt_);
	MOV32RtoR(HOST_r2,HOST_r11);
	CALLFunc((u32)_gtecalcMTC2);
	iUnlockReg(3);
	r2_is_dirty=1;
#endif
#endif
}

static void recSWC2(void)
{
	if (!rec_phase) {
		if (autobias) cycles_pending+=4;
		if (!IsConst(_Rs_)) {
			ReadReg(_Rs_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecSWC2");
#endif
	if (autobias) cycles_pending+=2;

#ifndef gte_new
	iLockReg(3);
	MOV32ItoR(HOST_r0,_Rt_);
	CALLFunc((u32)gtecalcMFC2);
	iUnlockReg(3);
	r2_is_dirty=1;
#else
#ifdef REC_USE_GTECALC_INLINE
	u32 rt=HOST_r0;
	switch(_Rt_) {
		case 9:
		case 10:
		case 11:
			UpdateGteDelay(1);
		case 1:
		case 3:
		case 5:
		case 8:
			MOV16sMtoR_regs(rt,(u32)&psxRegs.CP2D.p[_Rt_].sw.l); 
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[_Rt_],rt); // regs->CP2D.r[reg] = (s32)regs->CP2D.p[reg].sw.l;
			UnmapDataGte(_Rt_);
			break;
		case 7:
		case 16:
		case 17:
		case 18:
		case 19:
			MOV16MtoR_regs(rt,(u32)&psxRegs.CP2D.p[_Rt_].sw.l); 
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[_Rt_],rt); // regs->CP2D.r[reg] = (u32)regs->CP2D.p[reg].w.l;
			UnmapDataGte(_Rt_);
			break;
		case 15:
			{
#ifdef REC_USE_GTE_MAP_REGS
				u32 rmap=GetGteDataMapped(14);
				if (rmap && rmap!=rt) {
					MOV32RtoR(rt,rmap);	
				} else {
#else
				{
#endif
					MOV32MtoR_regs(rt,(u32)&psxRegs.CP2D.r[14]);
				}
				MOV32RtoM_regs((u32)&psxRegs.CP2D.r[_Rt_],rt); // regs->CP2D.r[reg] = gteSXY2;
				UnmapDataGte(_Rt_);
			}
			break;
		case 28:
#ifdef USE_OLD_GTE_WITHOUT_PATCH
		case 30:
			MOV32ItoR(rt,0);
			break;
#endif
		case 29:
			UpdateGteDelay(1);
#ifdef REC_USE_GTE_FUNCS
			CALLFunc(func_GTE_MFC2_29_ptr);
#else
			recGTE_MFC2_29(rt);
#endif
			MOV32RtoM_regs((u32)&psxRegs.CP2D.r[_Rt_],rt); // regs->CP2D.r[reg] = rt
			UnmapDataGte(_Rt_);
			break;
		default:
			{
#ifdef REC_USE_GTE_MAP_REGS
				u32 rmap=GetGteDataMapped(_Rt_);
				if (rmap && (IsConst(rmap) || IsMapped(rmap))) {
					if (IsConst(rmap)) {
						MOV32ItoR(rt,iRegs[rmap].k);
					} else {
						MOV32RtoR(rt,iRegs[rmap].reg);
					}
				} else {
#else
				{
#endif
					MOV32MtoR_regs(rt,(u32)&psxRegs.CP2D.r[_Rt_]);
				}	
			} 
	}
#else
	iLockReg(3);
	MOV32RtoR(HOST_r1,HOST_r11);
	MOV32ItoR(HOST_r0,_Rt_);
	CALLFunc((u32)_gtecalcMFC2);
	iUnlockReg(3);
	r2_is_dirty=1;
#endif
#endif

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			MOV32RtoM((u32)&psxM[addr & 0x1fffff], HOST_r0);
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			MOV32RtoM((u32)&psxH[addr & 0xfff], HOST_r0);
			return;
		}
		if (t == 0x1f80) {
			switch (addr) {
				case 0x1f801080: case 0x1f801084: 
				case 0x1f801090: case 0x1f801094: 
				case 0x1f8010a0: case 0x1f8010a4: 
				case 0x1f8010b0: case 0x1f8010b4: 
				case 0x1f8010c0: case 0x1f8010c4: 
				case 0x1f8010d0: case 0x1f8010d4: 
				case 0x1f8010e0: case 0x1f8010e4: 
				case 0x1f801074:
				case 0x1f8010f0:
					MOV32RtoM((u32)&psxH[addr & 0xffff], HOST_r0);
// CHUI: Añado ResetIoCycle para permite que en el proximo salto entre en psxBranchTest
#ifdef REC_USE_R2
					MOV32ItoR(HOST_r2,0);
					MOV32RtoM_regs(&psxRegs.io_cycle_counter,HOST_r2);
					r2_is_dirty=0;
#else
					MOV32ItoM_regs(&psxRegs.io_cycle_counter,0);
#endif
					return;

				case 0x1f801810:
					iLockReg(3);
					CALLFunc((u32)&GPU_writeData);
					iUnlockReg(3);
					r2_is_dirty=1;
					return;

				case 0x1f801814:
					iLockReg(3);
					CALLFunc((u32)&GPU_writeStatus);
					iUnlockReg(3);
					r2_is_dirty=1;
					return; // ???
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			iLockReg(3);
			if (autobias) cycles_pending+=2;
			iPutCyclesAdd(0);
			MOV32RtoR(HOST_r1,HOST_r0);
			MOV32ItoR(HOST_r0,addr);
			CALLFunc((u32)psxHwWrite32);
			iUnlockReg(3);
			r2_is_dirty=1;
			return;
		}
	}

	iLockReg(3);
	MOV32RtoR(HOST_r1,HOST_r0);
	iPushOfB();
	if (Config.HLE) {
		PSXMEMWRITE32(HOST_r1);
	} else {
		if (autobias) cycles_pending+=2;
		iPutCyclesAdd(0);
		CALLFunc((u32)psxMemWrite32);
	}
	iUnlockReg(3);
	r2_is_dirty=1;
}

#ifdef gte_new

static void recGTE_MFC2_29(u32 rt) {
	MOV16sMtoR_regs(HOST_r1,(u32)&psxRegs.CP2D.p[9].sw.l);
	SARI32(HOST_r1,HOST_r1,7); // mov r0, #(gteIR1>>7);
	write32(CMP_IMM(HOST_r1, 0x1f, 0)); // cmp r0, #0x1f
	MOV32ItoR(rt,0x1f); // mov rt, #0x1f
	j32Ptr[1]=armPtr; write32(BGT_FWD(0)); // bgt fin
	write32(CMP_IMM(HOST_r1, 0, 0)); // cmp r0, #0
	write32(MOVGE_REGS(rt,HOST_r1)); // movge rt,r0
	write32(MOVLT_IMM(rt,0,0)); // movlt  rt, #0
	armSetJ32(j32Ptr[1]); // fin:

	MOV16sMtoR_regs(HOST_r1,(u32)&psxRegs.CP2D.p[10].sw.l);
	SARI32(HOST_r1,HOST_r1,7); // mov r0, #(gteIR2>>7);
	write32(CMP_IMM(HOST_r1, 0x1f, 0)); // cmp r0, #0x1f
	MOV32ItoR(HOST_ip,0x1f); // mov r3, #0x1f
	j32Ptr[1]=armPtr; write32(BGT_FWD(0)); // bgt fin
	write32(CMP_IMM(HOST_r1, 0, 0)); // cmp r0, #0
	write32(MOVGE_REGS(HOST_ip,HOST_r1)); // movge r3,r0
	write32(MOVLT_IMM(HOST_ip,0,0)); // movlt  r3, #0
	armSetJ32(j32Ptr[1]); // fin:
	SHLI32(HOST_r1,HOST_ip,5); // r0 = limMCFC2(gteIR2 >> 7) << 5
	OR32(rt,rt,HOST_r1); // rt |= r0

	MOV16sMtoR_regs(HOST_r1,(u32)&psxRegs.CP2D.p[11].sw.l);
	SARI32(HOST_r1,HOST_r1,7); // mov r0, #(gteIR3>>7);
	write32(CMP_IMM(HOST_r1, 0x1f, 0)); // cmp r0, #0x1f
	MOV32ItoR(HOST_ip,0x1f); // mov r3, #0x1f
	j32Ptr[1]=armPtr; write32(BGT_FWD(0)); // bgt fin
	write32(CMP_IMM(HOST_r1, 0, 0)); // cmp r0, #0
	write32(MOVGE_REGS(HOST_ip,HOST_r1)); // movge r3,r0
	write32(MOVLT_IMM(HOST_ip,0,0)); // movlt  r3, #0
	armSetJ32(j32Ptr[1]); // fin:
	SHLI32(HOST_r1,HOST_ip,10); // r0 = limMCFC2(gteIR2>>7)<<10 
	OR32(rt,rt,HOST_r1); // rt |= r0
}

static void recGTE_MTC2_15(u32 rt) {
	MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.CP2D.r[13]); 
	MOV32RtoM_regs((u32)&psxRegs.CP2D.r[12],HOST_r0); // gteSXY0 = gteSXY1;
	MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.CP2D.r[14]); 
	MOV32RtoM_regs((u32)&psxRegs.CP2D.r[13],HOST_r0); // gteSXY1 = gteSXY2;
	MOV32RtoM_regs((u32)&psxRegs.CP2D.r[14],rt); // gteSXY2 = value;
	MOV32RtoM_regs((u32)&psxRegs.CP2D.r[15],rt); // gteSXYP = value;
}

static void recGTE_MTC2_28(u32 rt) {
	write32(AND_IMM(HOST_r0,rt,0x1f,0));
	SHLI32(HOST_r0,HOST_r0,7);
	MOV32RtoM_regs((u32)&psxRegs.CP2D.p[9].sw.l,HOST_r0); // gteIR1 = (value & 0x1f) << 7;
	write32(AND_IMM(HOST_r0,rt,0x3e,0x1c));
	SHLI32(HOST_r0,HOST_r0,2);
	MOV32RtoM_regs((u32)&psxRegs.CP2D.p[10].sw.l,HOST_r0); // gteIR2 = (value & 0x3e0) << 2;
	write32(AND_IMM(HOST_r0,rt,0x1f,0x16));
	SHRI32(HOST_r0,HOST_r0,3);
	MOV32RtoM_regs((u32)&psxRegs.CP2D.p[11].sw.l,HOST_r0); // gteIR3 = (value & 0x7c00) >> 3;
}

static void recGTE_updateMACs_flag(int lm, int shift, int flag) {
#ifdef REC_USE_GTE_FUNCS
	MOV32RtoR(HOST_r0,HOST_lr);
#endif
	MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.CP2D.r[25]); // r1 = gteMAC1
	if (shift) {
		SARI32(HOST_r1,HOST_r1,shift);
	}
	MOV32ItoR(HOST_r2,0x7fff); 						//  r2 = 0x7fff
	write32(CMP_REGS(HOST_r1,HOST_r2));				//  cmp r1, r2
	j32Ptr[1]=armPtr; write32(BLE_FWD(0));			//  ble mayor
	if (flag) {
		MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
	}
	MOV32RtoR(HOST_r1,HOST_r2);						//  r1 = r2
	if (flag) {
		write32(ORR_IMM(HOST_r3,HOST_ip,1,9));			//  or r3,ip,#0x81000000
		MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
	}
	j32Ptr[2]=armPtr; write32(B_FWD(0));				//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[1]);								// mayor:
	if (lm) {
		write32(CMP_IMM(HOST_r1,0,0));					//  cmp r1, #0
		if (flag) {
			j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
			MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
			MOV32ItoR(HOST_r1,0);							//  r1 = 0
		} else {
			write32(MOVLT_IMM(HOST_r1,0,0));
		}
	} else {
		write32(CMN_IMM(HOST_r1,2,0x12));				//  cmn r1, #0x8000
		j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
		if (flag) {
			MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
		}
		MOV32ItoR(HOST_r1,0xffff8000);					//  r1 = -0x8000
	}
	if (flag) {
		write32(ORR_IMM(HOST_r3,HOST_ip,1,9));			//  or r3,ip,#0x81000000
		MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
		armSetJ32(j32Ptr[3]);
	} else {
		if (!lm) {
			armSetJ32(j32Ptr[3]);
		}
	}
	armSetJ32(j32Ptr[2]);								// fin:
	MOV16RtoM_regs((u32)&psxRegs.CP2D.p[9].sw.l,HOST_r1); // gteIR1 = limB1(gteMAC1, lm);

	MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.CP2D.r[26]); // r1 = gteMAC2
	if (shift) {
		SARI32(HOST_r1,HOST_r1,shift);
	}
	MOV32ItoR(HOST_r2,0x7fff); 						//  r2 = 0x7fff
	write32(CMP_REGS(HOST_r1,HOST_r2));				//  cmp r1, r2
	j32Ptr[1]=armPtr; write32(BLE_FWD(0));			//  ble mayor
	if (flag) {
		MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG	
	}
	MOV32RtoR(HOST_r1,HOST_r2);						//  r1 = r2
	if (flag) {
		write32(ORR_IMM(HOST_r3,HOST_ip,2,0x2));		//  or r3,ip,#0x80000000
		write32(ORR_IMM(HOST_r3,HOST_r3,2,0xa));		//  or r3,r3,#0x800000
		MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
	}
	j32Ptr[2]=armPtr; write32(B_FWD(0));				//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[1]);								// mayor:
	if (lm) {
		write32(CMP_IMM(HOST_r1,0,0));					//  cmp r1, #0
		if (flag) {
			j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
			MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
			MOV32ItoR(HOST_r1,0);							//  r1 = 0
		} else {
			write32(MOVLT_IMM(HOST_r1,0,0));
		}
	} else {
		write32(CMN_IMM(HOST_r1,2,0x12));				//  cmn r1, #0x8000
		j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
		if (flag) {
			MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
		}
		MOV32ItoR(HOST_r1,0xffff8000);					//  r1 = -0x8000
	}
	if (flag) {
		write32(ORR_IMM(HOST_r3,HOST_ip,2,0x2));		//  or r3,ip,#0x80000000
		write32(ORR_IMM(HOST_r3,HOST_r3,2,0xa));		//  or r3,r3,#0x800000
		MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
		armSetJ32(j32Ptr[3]);
	} else {
		if (!lm) {
			armSetJ32(j32Ptr[3]);
		}
	}
	armSetJ32(j32Ptr[2]);								// fin:
	MOV16RtoM_regs((u32)&psxRegs.CP2D.p[10].sw.l,HOST_r1); // gteIR2 = limB2(gteMAC2, lm);

	MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.CP2D.r[27]); // r1 = gteMAC3
	if (shift) {
		SARI32(HOST_r1,HOST_r1,shift);
	}
	MOV32ItoR(HOST_r2,0x7fff); 						//  r2 = 0x7fff
	write32(CMP_REGS(HOST_r1,HOST_r2));				//  cmp r1, r2
	j32Ptr[1]=armPtr; write32(BLE_FWD(0));			//  ble mayor
	if (flag) {
		MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
	}
	MOV32RtoR(HOST_r1,HOST_r2);						//  r1 = r2
	if (flag) {
		write32(ORR_IMM(HOST_r3,HOST_ip,1,0xa));		//  or r3,ip,#0x400000
		MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
	}
	j32Ptr[2]=armPtr; write32(B_FWD(0));				//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[1]);								// mayor:
	if (lm) {
		write32(CMP_IMM(HOST_r1,0,0));					//  cmp r1, #0
		if (flag) {
			j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
			MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
			MOV32ItoR(HOST_r1,0);							//  r1 = 0
		} else {
			write32(MOVLT_IMM(HOST_r1,0,0));
		}
	} else {
		write32(CMN_IMM(HOST_r1,2,0x12));				//  cmn r1, #0x8000
		j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
		if (flag) {
			MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
		}
		MOV32ItoR(HOST_r1,0xffff8000);					//  r1 = -0x8000
	}
	if (flag) {
		write32(ORR_IMM(HOST_r3,HOST_ip,1,0xa));		//  or r3,ip,#0x400000
		MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
		armSetJ32(j32Ptr[3]);
	} else {
		if (!lm) {
			armSetJ32(j32Ptr[3]);
		}
	}	
	armSetJ32(j32Ptr[2]);								// fin:
	MOV16RtoM_regs((u32)&psxRegs.CP2D.p[11].sw.l,HOST_r1); // gteIR3 = limB3(gteMAC3, lm);
#ifdef REC_USE_GTE_FUNCS
	MOV32RtoR(HOST_lr,HOST_r0);
#endif
}

static void recGTE_updateMAC3_flag(int lm, int shift, int flag) {
#ifdef REC_USE_GTE_FUNCS
	MOV32RtoR(HOST_r0,HOST_lr);
#endif
	MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.CP2D.r[27]); // r1 = gteMAC3
	if (shift) {
		SARI32(HOST_r1,HOST_r1,shift);
	}
	MOV32ItoR(HOST_r2,0x7fff); 						//  r2 = 0x7fff
	write32(CMP_REGS(HOST_r1,HOST_r2));				//  cmp r1, r2
	j32Ptr[1]=armPtr; write32(BLE_FWD(0));			//  ble mayor
	if (flag) {
		MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
	}
	MOV32RtoR(HOST_r1,HOST_r2);						//  r1 = r2
	if (flag) {
		write32(ORR_IMM(HOST_r3,HOST_ip,1,0xa));		//  or r3,ip,#0x400000
		MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
	}
	j32Ptr[2]=armPtr; write32(B_FWD(0));				//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[1]);								// mayor:
	if (lm) {
		write32(CMP_IMM(HOST_r1,0,0));					//  cmp r1, #0
		if (flag) {
			j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
			MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
			MOV32ItoR(HOST_r1,0);							//  r1 = 0
		} else {
			write32(MOVLT_IMM(HOST_r1,0,0));
		}
	} else {
		write32(CMN_IMM(HOST_r1,2,0x12));				//  cmn r1, #0x8000
		j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
		if (flag) {
			MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
		}
		MOV32ItoR(HOST_r1,0xffff8000);					//  r1 = -0x8000
	}
	if (flag) {
		write32(ORR_IMM(HOST_r3,HOST_ip,1,0xa));		//  or r3,ip,#0x400000
		MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
		armSetJ32(j32Ptr[3]);
	} else {
		if (!lm) {
			armSetJ32(j32Ptr[3]);
		}
	}	
	armSetJ32(j32Ptr[2]);								// fin:
	MOV16RtoM_regs((u32)&psxRegs.CP2D.p[11].sw.l,HOST_r1); // gteIR3 = limB3(gteMAC3, lm);
#ifdef REC_USE_GTE_FUNCS
	MOV32RtoR(HOST_lr,HOST_r0);
#endif
}

static unsigned func_GTE_updateCODEs_ptr=0;

static void recGTE_updateCODEs() {
	MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.CP2D.r[21]);
	MOV32RtoM_regs((u32)&psxRegs.CP2D.r[20],HOST_r0);	// gteRGB0 = gteRGB1
	MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.CP2D.r[22]);
	MOV32RtoM_regs((u32)&psxRegs.CP2D.r[21],HOST_r0);	// gteRGB1 = gteRGB2
	MOV8MtoR_regs(HOST_r0,(u32)&psxRegs.CP2D.p[6].b.h3);
	MOV8RtoM_regs((u32)&psxRegs.CP2D.p[22].b.h3,HOST_r0);// gteCODE2 = gteCODE

	MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.CP2D.r[25]);
	write32(MOV_REG_ASR_IMM(HOST_r1, HOST_r1, 4));	  // r1 = gteMAC1>>4
	MOV32ItoR(HOST_r2,0xff); 						//  r2 = 0xff
	write32(CMP_REGS(HOST_r1,HOST_r2));				//  cmp r1, r2
	j32Ptr[1]=armPtr; write32(BLE_FWD(0));			//  ble mayor
#ifdef USE_GTE_FLAG
	MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
#endif
	MOV32RtoR(HOST_r1,HOST_r2);						//  r1 = r2
#ifdef USE_GTE_FLAG
	write32(ORR_IMM(HOST_r3,HOST_ip,2,0xc));		//  or r3,ip,#0x200000
	MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
#endif
	j32Ptr[2]=armPtr; write32(B_FWD(0));				//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[1]);								// mayor:
	write32(CMP_IMM(HOST_r1,0,0));					//  cmp r1, #0
#ifdef USE_GTE_FLAG
	j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
	MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
	MOV32ItoR(HOST_r1,0);							//  r1 = 0
	write32(ORR_IMM(HOST_r3,HOST_ip,2,0xc));		//  or r3,ip,#0x200000
	MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
	armSetJ32(j32Ptr[3]);
#else
	write32(MOVLT_IMM(HOST_r1,0,0));
#endif
	armSetJ32(j32Ptr[2]);								// fin:
	MOV8RtoM_regs((u32)&psxRegs.CP2D.p[22].b.l,HOST_r1); // gteR2 = limC1(gteMAC1 >> 4);

	MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.CP2D.r[26]);
	write32(MOV_REG_ASR_IMM(HOST_r1, HOST_r1, 4));	  // r1 = gteMAC2>>4
	MOV32ItoR(HOST_r2,0xff); 						//  r2 = 0xff
	write32(CMP_REGS(HOST_r1,HOST_r2));				//  cmp r1, r2
	j32Ptr[1]=armPtr; write32(BLE_FWD(0));			//  ble mayor
#ifdef USE_GTE_FLAG
	MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
#endif
	MOV32RtoR(HOST_r1,HOST_r2);						//  r1 = r2
#ifdef USE_GTE_FLAG
	write32(ORR_IMM(HOST_r3,HOST_ip,1,0xc));		//  or r3,ip,#0x100000
	MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
#endif
	j32Ptr[2]=armPtr; write32(B_FWD(0));				//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[1]);								// mayor:
	write32(CMP_IMM(HOST_r1,0,0));					//  cmp r1, #0
#ifdef USE_GTE_FLAG
	j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
	MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
	MOV32ItoR(HOST_r1,0);							//  r1 = 0
	write32(ORR_IMM(HOST_r3,HOST_ip,1,0xc));		//  or r3,ip,#0x100000	
	MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
	armSetJ32(j32Ptr[3]);
#else
	write32(MOVLT_IMM(HOST_r1,0,0));
#endif
	armSetJ32(j32Ptr[2]);								// fin:
	MOV8RtoM_regs((u32)&psxRegs.CP2D.p[22].b.h,HOST_r1); // gteG2 = limC2(gteMAC2 >> 4);

	MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.CP2D.r[27]);
	write32(MOV_REG_ASR_IMM(HOST_r1, HOST_r1, 4));	  // r1 = gteMAC3>>4
	MOV32ItoR(HOST_r2,0xff); 						//  r2 = 0xff
	write32(CMP_REGS(HOST_r1,HOST_r2));				//  cmp r1, r2
	j32Ptr[1]=armPtr; write32(BLE_FWD(0));			//  ble mayor
#ifdef USE_GTE_FLAG
	MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
#endif
	MOV32RtoR(HOST_r1,HOST_r2);						//  r1 = r2
#ifdef USE_GTE_FLAG
	write32(ORR_IMM(HOST_r3,HOST_ip,2,0xe));		//  or r3,ip,#0x80000
	MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
#endif
	j32Ptr[2]=armPtr; write32(B_FWD(0));				//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[1]);								// mayor:
	write32(CMP_IMM(HOST_r1,0,0));					//  cmp r1, #0
#ifdef USE_GTE_FLAG
	j32Ptr[3]=armPtr; write32(BGE_FWD(0));			//  bge fin
	MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.CP2C.r[31]);// ip = gteFLAG
	MOV32ItoR(HOST_r1,0);							//  r1 = 0
	write32(ORR_IMM(HOST_r3,HOST_ip,2,0xe));		//  or r3,ip,#0x80000
	MOV32RtoM_regs((u32)&psxRegs.CP2C.r[31], HOST_r3);// gteFLAG = r3
	armSetJ32(j32Ptr[3]);
#else
	write32(MOVLT_IMM(HOST_r1,0,0));
#endif
	armSetJ32(j32Ptr[2]);								// fin:
	MOV8RtoM_regs((u32)&psxRegs.CP2D.p[22].b.h2,HOST_r1); // gteB2 = limC3(gteMAC3 >> 4);
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteMVMVA_cv0_mx0_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx0_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx1_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx1_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx2_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx2_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv0_mx3_s12_(psxRegisters *regs);
extern void _gteMVMVA_cv0_mx3_s0_(psxRegisters *regs);
extern void _gteMVMVA_cv1_mx0_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx0_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx1_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx1_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx2_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx2_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv1_mx3_s12_(psxRegisters *regs);
extern void _gteMVMVA_cv1_mx3_s0_(psxRegisters *regs);
extern void _gteMVMVA_cv2_mx0_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx0_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx1_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx1_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx2_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx2_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv2_mx3_s12_(psxRegisters *regs);
extern void _gteMVMVA_cv2_mx3_s0_(psxRegisters *regs);
extern void _gteMVMVA_cv3_mx0_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx0_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx1_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx1_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx2_s12_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx2_s0_(psxRegisters *regs, u32 vxy, s32 vz);
extern void _gteMVMVA_cv3_mx3_s12_(psxRegisters *regs);
extern void _gteMVMVA_cv3_mx3_s0_(psxRegisters *regs);
}
#else
#define _gteMVMVA_cv0_mx0_s12_ _gteMVMVA_cv0_mx0_s12
#define _gteMVMVA_cv0_mx0_s0_ _gteMVMVA_cv0_mx0_s0
#define _gteMVMVA_cv0_mx1_s12_ _gteMVMVA_cv0_mx1_s12
#define _gteMVMVA_cv0_mx1_s0_ _gteMVMVA_cv0_mx1_s0
#define _gteMVMVA_cv0_mx2_s12_ _gteMVMVA_cv0_mx2_s12
#define _gteMVMVA_cv0_mx2_s0_ _gteMVMVA_cv0_mx2_s0
#define _gteMVMVA_cv0_mx3_s12_ _gteMVMVA_cv0_mx3_s12
#define _gteMVMVA_cv0_mx3_s0_ _gteMVMVA_cv0_mx3_s0
#define _gteMVMVA_cv1_mx0_s12_ _gteMVMVA_cv1_mx0_s12
#define _gteMVMVA_cv1_mx0_s0_ _gteMVMVA_cv1_mx0_s0
#define _gteMVMVA_cv1_mx1_s12_ _gteMVMVA_cv1_mx1_s12
#define _gteMVMVA_cv1_mx1_s0_ _gteMVMVA_cv1_mx1_s0
#define _gteMVMVA_cv1_mx2_s12_ _gteMVMVA_cv1_mx2_s12
#define _gteMVMVA_cv1_mx2_s0_ _gteMVMVA_cv1_mx2_s0
#define _gteMVMVA_cv1_mx3_s12_ _gteMVMVA_cv1_mx3_s12
#define _gteMVMVA_cv1_mx3_s0_ _gteMVMVA_cv1_mx3_s0
#define _gteMVMVA_cv2_mx0_s12_ _gteMVMVA_cv2_mx0_s12
#define _gteMVMVA_cv2_mx0_s0_ _gteMVMVA_cv2_mx0_s0
#define _gteMVMVA_cv2_mx1_s12_ _gteMVMVA_cv2_mx1_s12
#define _gteMVMVA_cv2_mx1_s0_ _gteMVMVA_cv2_mx1_s0
#define _gteMVMVA_cv2_mx2_s12_ _gteMVMVA_cv2_mx2_s12
#define _gteMVMVA_cv2_mx2_s0_ _gteMVMVA_cv2_mx2_s0
#define _gteMVMVA_cv2_mx3_s12_ _gteMVMVA_cv2_mx3_s12
#define _gteMVMVA_cv2_mx3_s0_ _gteMVMVA_cv2_mx3_s0
#define _gteMVMVA_cv3_mx0_s12_ _gteMVMVA_cv3_mx0_s12
#define _gteMVMVA_cv3_mx0_s0_ _gteMVMVA_cv3_mx0_s0
#define _gteMVMVA_cv3_mx1_s12_ _gteMVMVA_cv3_mx1_s12
#define _gteMVMVA_cv3_mx1_s0_ _gteMVMVA_cv3_mx1_s0
#define _gteMVMVA_cv3_mx2_s12_ _gteMVMVA_cv3_mx2_s12
#define _gteMVMVA_cv3_mx2_s0_ _gteMVMVA_cv3_mx2_s0
#define _gteMVMVA_cv3_mx3_s12_ _gteMVMVA_cv3_mx3_s12
#define _gteMVMVA_cv3_mx3_s0_ _gteMVMVA_cv3_mx3_s0
#endif

static void recMVMVA(void) {
	if (autobias) cycles_pending+=8;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecMVMVA");
#endif
	int gteop=(psxRegs.code & 0x1ffffff);
	int cv=((gteop >> 13) & 3);
	int mx=((gteop >> 17) & 3);
	int shift=((gteop >> 19) & 1)?12:0;
	int v=((gteop >> 15) & 3);
	int lm=((gteop >> 10 ) & 1);
	
	iLockReg(3);
	if (v==3) {
		UpdateGteDelay(1);
	}
#if defined(USE_GTE_FLAG) || !defined(REC_USE_GTE_FUNCS) || !defined(REC_USE_GTE_DELAY_CALC)
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	if (mx!=3) {
#ifdef REC_USE_GTE_MAP_REGS
		if (v<3) {
			PullGteDataMapped(GetGteDataMapped(v<<1),v<<1,HOST_r1);
			PullGteDataMapped(GetGteDataMapped((v<<1)+1),(v<<1)+1,HOST_r2);
			SHLI32(HOST_r2,HOST_r2,16);
			SARI32(HOST_r2,HOST_r2,16);		
		} else {
			PullGteDataMapped(GetGteDataMapped(9),9,HOST_r1);
			PullGteDataMapped(GetGteDataMapped(10),10,HOST_r2);
			SHLI32(HOST_r3, HOST_r1, 16);
			SHRI32(HOST_r3, HOST_r3, 16);
			write32(ORR_REG_LSL_IMM(HOST_r1,HOST_r3,HOST_r2,16));
			PullGteDataMapped(GetGteDataMapped(11),11,HOST_r2);
			SHLI32(HOST_r2,HOST_r2,16);
			SARI32(HOST_r2,HOST_r2,16);
		}
#else
		if (v<3) {
			MOV32MtoR_regs(HOST_r1,&psxRegs.CP2D.r[v<<1]);
		} else {
			MOV32MtoR_regs(HOST_r1,&psxRegs.CP2D.r[9]);
			MOV32MtoR_regs(HOST_r2,&psxRegs.CP2D.r[10]);
			SHLI32(HOST_r3, HOST_r1, 16);
			SHRI32(HOST_r3, HOST_r3, 16);
			write32(ORR_REG_LSL_IMM(HOST_r1,HOST_r3,HOST_r2,16));
		}
		MOV16sMtoR_regs(HOST_r2,(v<3? &psxRegs.CP2D.p[(v << 1) + 1].sw.l : &psxRegs.CP2D.p[11].sw.l));
#endif
	}
#ifndef REC_USE_GTE_ASM_FUNCS
	MOV32RtoR(HOST_r0,HOST_r11);
#else
	u32 saveregs=0;
	if (mx!=3) {
		saveregs=iGetSaveMask(4,4);
		if (saveregs) {
			write32(ARM_SAVEREG(saveregs));
		}
	}
#endif
	switch(cv) {
		case 0:		
			switch(mx) {
				case 0:	CALLFunc((u32)(shift?_gteMVMVA_cv0_mx0_s12_:_gteMVMVA_cv0_mx0_s0_)); break;
				case 1:	CALLFunc((u32)(shift?_gteMVMVA_cv0_mx1_s12_:_gteMVMVA_cv0_mx1_s0_)); break;
				case 2:	CALLFunc((u32)(shift?_gteMVMVA_cv0_mx2_s12_:_gteMVMVA_cv0_mx2_s0_)); break;
				default: CALLFunc((u32)(shift?_gteMVMVA_cv0_mx3_s12_:_gteMVMVA_cv0_mx3_s0_));
			}
			break;
		case 1:
			switch(mx) {
				case 0:	CALLFunc((u32)(shift?_gteMVMVA_cv1_mx0_s12_:_gteMVMVA_cv1_mx0_s0_)); break;
				case 1:	CALLFunc((u32)(shift?_gteMVMVA_cv1_mx1_s12_:_gteMVMVA_cv1_mx1_s0_)); break;
				case 2:	CALLFunc((u32)(shift?_gteMVMVA_cv1_mx2_s12_:_gteMVMVA_cv1_mx2_s0_)); break;
				default: CALLFunc((u32)(shift?_gteMVMVA_cv1_mx3_s12_:_gteMVMVA_cv1_mx3_s0_));
			}
			break;
		case 2:
			switch(mx) {
				case 0:	CALLFunc((u32)(shift?_gteMVMVA_cv2_mx0_s12_:_gteMVMVA_cv2_mx0_s0_)); break;
				case 1:	CALLFunc((u32)(shift?_gteMVMVA_cv2_mx1_s12_:_gteMVMVA_cv2_mx1_s0_)); break;
				case 2:	CALLFunc((u32)(shift?_gteMVMVA_cv2_mx2_s12_:_gteMVMVA_cv2_mx2_s0_)); break;
				default: CALLFunc((u32)(shift?_gteMVMVA_cv2_mx3_s12_:_gteMVMVA_cv2_mx3_s0_));
			}
			break;
		default:
			switch(mx) {
				case 0:	CALLFunc((u32)(shift?_gteMVMVA_cv3_mx0_s12_:_gteMVMVA_cv3_mx0_s0_)); break;
				case 1:	CALLFunc((u32)(shift?_gteMVMVA_cv3_mx1_s12_:_gteMVMVA_cv3_mx1_s0_)); break;
				case 2:	CALLFunc((u32)(shift?_gteMVMVA_cv3_mx2_s12_:_gteMVMVA_cv3_mx2_s0_)); break;
				default: CALLFunc((u32)(shift?_gteMVMVA_cv3_mx3_s12_:_gteMVMVA_cv3_mx3_s0_));
			}
	}
#ifdef REC_USE_GTE_ASM_FUNCS
	if (saveregs) {
		write32(ARM_RESTOREREG(saveregs));
	}
#endif

#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	if (lm)
		func_GTE_delay_ptr=func_GTE_updateMACs_lm1_flag_ptr;
	else
		func_GTE_delay_ptr=func_GTE_updateMACs_lm0_flag_ptr;
#else
	if (lm) {
		CALLFunc(func_GTE_updateMACs_lm1_flag_ptr);
	} else {
		CALLFunc(func_GTE_updateMACs_lm0_flag_ptr);
	}
#endif
#else
	recGTE_updateMACs_flag(lm,0,1);
#endif
	UnmapDataGteMACs();
	UnmapCtrlGte(31); // gteFLAG

	iUnlockReg(3);
	r2_is_dirty=1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteOP_s12_(void);
extern void _gteOP_s0_(void);
}
#else
#define _gteOP_s12_ _gteOP_s12
#define _gteOP_s0_ _gteOP_s0
#endif

static void recOP(void){
	if (autobias) cycles_pending+=6;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecOP");
#endif
	int gteop=(psxRegs.code & 0x1ffffff);
	int shift=((gteop >> 19) & 1)?12:0;
	int lm=((gteop >> 10 ) & 1);

	iLockReg(3);
	UpdateGteDelay(1);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif

	if (shift) {
		CALLFunc((u32)_gteOP_s12_);
	} else {
		CALLFunc((u32)_gteOP_s0_);
	} 
	
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	if (lm)
		func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
	else
		func_GTE_delay_ptr=func_GTE_updateMACs_lm0_ptr;
#else
	if (lm) {
		CALLFunc(func_GTE_updateMACs_lm1_ptr);
	} else {
		CALLFunc(func_GTE_updateMACs_lm0_ptr);
	}
#endif
#else
	recGTE_updateMACs(lm,0);
#endif
	UnmapDataGteMACs();

	iUnlockReg(3);
	r2_is_dirty=1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteDPCS_s12_(void);
extern void _gteDPCS_s0_(void);
}
#else
#define _gteDPCS_s12_ _gteDPCS_s12
#define _gteDPCS_s0_ _gteDPCS_s0
#endif

static void recDPCS(void){
	if (autobias) cycles_pending+=8;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecDPCS");
#endif
	int gteop=(psxRegs.code & 0x1ffffff);
	int shift=((gteop >> 19) & 1)?12:0;

	iLockReg(3);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	if (shift) {
		CALLFunc((u32)_gteDPCS_s12_);
	} else {
		CALLFunc((u32)_gteDPCS_s0_);
	} 
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm0_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm0_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(0,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();
	
	iUnlockReg(3);
	r2_is_dirty=1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteGPF_s12_(void);
extern void _gteGPF_s0_(void);
}
#else
#define _gteGPF_s12_ _gteGPF_s12
#define _gteGPF_s0_ _gteGPF_s0
#endif

static void recGPF(void){
	if (autobias) cycles_pending+=5;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecGPF");
#endif
	int gteop=(psxRegs.code & 0x1ffffff);
	int shift=((gteop >> 19) & 1)?12:0;

	iLockReg(3);
	UpdateGteDelay(1);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif

#ifndef REC_USE_GTE_ASM_FUNCS
	MOV32RtoR(HOST_r0,HOST_r11);	
#endif
	if (shift) {
		CALLFunc((u32)_gteGPF_s12_);
	} else {
		CALLFunc((u32)_gteGPF_s0_);
	} 

#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm0_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm0_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(0,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty=1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteSQR_s12_(void);
extern void _gteSQR_s0_(void);
}
#else
#define _gteSQR_s12_ _gteSQR_s12
#define _gteSQR_s0_ _gteSQR_s0
#endif

static void recSQR(void){
	if (autobias) cycles_pending+=3;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecSQR");
#endif
	int gteop=(psxRegs.code & 0x1ffffff);
	int shift=((gteop >> 19) & 1)?12:0;
	int lm=((gteop >> 10 ) & 1);

	iLockReg(3);
	UpdateGteDelay(1);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	
#ifndef REC_USE_GTE_ASM_FUNCS
	MOV32RtoR(HOST_r0,HOST_r11);	
#endif
	if (shift) {
		CALLFunc((u32)_gteSQR_s12_);
	} else {
		CALLFunc((u32)_gteSQR_s0_);
	} 

#ifdef USE_OLD_GTE_WITHOUT_PATCH	
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	if (shift) {
		if (lm)
			func_GTE_delay_ptr=func_GTE_updateMACs_lm1_shift12_ptr;
		else
			func_GTE_delay_ptr=func_GTE_updateMACs_lm0_shift12_ptr;
	} else {
		if (lm)
			func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
		else
			func_GTE_delay_ptr=func_GTE_updateMACs_lm0_ptr;
	}
#else
	if (shift) {
		if (lm) {
			CALLFunc(func_GTE_updateMACs_lm1_shift12_ptr);
		} else {
			CALLFunc(func_GTE_updateMACs_lm0_shift12_ptr);
		}
	} else {
		if (lm) {
			CALLFunc(func_GTE_updateMACs_lm1_ptr);
		} else {
			CALLFunc(func_GTE_updateMACs_lm0_ptr);
		}
	}
#endif
#else
	recGTE_updateMACs(lm,shift);
#endif
#else // USE_OLD_GTE_WITHOUT_PATCH
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	if (lm)
		func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
	else
		func_GTE_delay_ptr=func_GTE_updateMACs_lm0_ptr;
#else
	if (lm) {
		CALLFunc(func_GTE_updateMACs_lm1_ptr);
	} else {
		CALLFunc(func_GTE_updateMACs_lm0_ptr);
	}
#endif
#else
	recGTE_updateMACs(lm,0);
#endif
#endif // USE_OLD_GTE_WITHOUT_PATCH
	UnmapDataGteMACs();

	iUnlockReg(3);
	r2_is_dirty=1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" { extern void _gteINTPL_s0_(void); extern void _gteINTPL_s12_(void); extern void _gteINTPL_block_(void); }
#endif

static void recINTPL(void){
	if (autobias) cycles_pending+=8;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecINTPL");
#endif

	int gteop=(psxRegs.code & 0x1ffffff);
	int shift=((gteop >> 19) & 1)?12:0;
	int lm=((gteop >> 10 ) & 1);

	iLockReg(3);
	UpdateGteDelay(1);

#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif

#if defined(REC_USE_GTE_MAP_REGS) && defined(REC_USE_GTE_ASM_FUNCS)
	u32 regIR0=GetGteDataMapped(8);
	u32 regIR1=GetGteDataMapped(9);
	u32 regIR2=GetGteDataMapped(10);
	u32 regIR3=GetGteDataMapped(11);
	u32 regRFC=GetGteCtrlMapped(21);
	u32 regGFC=GetGteCtrlMapped(22);
	u32 regBFC=GetGteCtrlMapped(23);

	if (regIR0 || regIR1 || regIR2 || regIR3 || regRFC || regGFC || regBFC) {
		PullGteDataMapped(regIR1,9,HOST_r0); // gteIR1
		PullGteDataMapped(regIR0,8,HOST_r1); // gteIR0
		PullGteCtrlMapped(regRFC,21,HOST_r2); // gteRFC
		CALLFunc((u32)_gteINTPL_block_);
		if (shift) {
			SARI32(HOST_r0,HOST_r0,12);
		}
		MOV32RtoM_regs((u32)&psxRegs.CP2D.r[25],HOST_r0); // gteMAC1

		PullGteDataMapped(regIR2,10,HOST_r0); // gteIR2
		PullGteCtrlMapped(regGFC,22,HOST_r2); // gteGFC
		CALLFunc((u32)_gteINTPL_block_);
		if (shift) {
			SARI32(HOST_r0,HOST_r0,12);
		}
		MOV32RtoM_regs((u32)&psxRegs.CP2D.r[26],HOST_r0); // gteMAC2

		PullGteDataMapped(regIR3,11,HOST_r0); // gteIR3
		PullGteCtrlMapped(regBFC,23,HOST_r2); // gteBFC
		CALLFunc((u32)_gteINTPL_block_);
		if (shift) {
			SARI32(HOST_r0,HOST_r0,12);
		}
		MOV32RtoM_regs((u32)&psxRegs.CP2D.r[27],HOST_r0); // gteMAC3
	} else {
#else
	{
#endif
		if (shift) {
#ifdef REC_USE_GTE_ASM_FUNCS
			u32 saveregs=iGetSaveMask(4,7);
			if (saveregs) {
				write32(ARM_SAVEREG(saveregs));
			}
			CALLFunc((u32)_gteINTPL_s12_);
			if (saveregs) {
				write32(ARM_RESTOREREG(saveregs));
			}
#else
			MOV32RtoR(HOST_r0,HOST_r11);
			CALLFunc((u32)_gteINTPL_s12);
#endif
		} else {
#ifdef REC_USE_GTE_ASM_FUNCS
			u32 saveregs=iGetSaveMask(4,7);
			if (saveregs) {
				write32(ARM_SAVEREG(saveregs));
			}
			CALLFunc((u32)_gteINTPL_s0_);
			if (saveregs) {
				write32(ARM_RESTOREREG(saveregs));
			}
#else
			MOV32RtoR(HOST_r0,HOST_r11);
			CALLFunc((u32)_gteINTPL_s0);
#endif
		}
	}

#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	if (lm)
		func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
	else
		func_GTE_delay_ptr=func_GTE_updateMACs_lm0_ptr;
#else
	if (lm) {
		CALLFunc(func_GTE_updateMACs_lm1_ptr);
	} else {
		CALLFunc(func_GTE_updateMACs_lm0_ptr);
	}
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(lm,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty=1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteDCPL__(void);
}
#else
#define _gteDCPL__ _gteDCPL_
#endif

static void recDCPL(void){
	if (autobias) cycles_pending+=8;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecDCPL");
#endif
	int gteop=(psxRegs.code & 0x1ffffff);
	int lm=((gteop >> 10 ) & 1);

	iLockReg(3);
	UpdateGteDelay(1);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	CALLFunc((u32)_gteDCPL__);

#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	if (lm)
		func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
	else
		func_GTE_delay_ptr=func_GTE_updateMACs_lm0_ptr;
#else
	if (lm) {
		CALLFunc(func_GTE_updateMACs_lm1_ptr);
	} else {
		CALLFunc(func_GTE_updateMACs_lm0_ptr);
	}
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(lm,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty=1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteGPL_s12_(void);
extern void _gteGPL_s0_(void);
}
#else
#define _gteGPL_s12_ _gteGPL_s12
#define _gteGPL_s0_ _gteGPL_s0
#endif

static void recGPL(void){
	if (autobias) cycles_pending+=5;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecGPL");
#endif
	int gteop=(psxRegs.code & 0x1ffffff);
	int shift=((gteop >> 19) & 1)?12:0;

	iLockReg(3);
	UpdateGteDelay(1);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	
#ifndef REC_USE_GTE_ASM_FUNCS
	MOV32RtoR(HOST_r0,HOST_r11);	
#endif
	if (shift) {
		CALLFunc((u32)_gteGPL_s12_);
	} else {
		CALLFunc((u32)_gteGPL_s0_);
	} 

#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm0_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm0_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(0,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty=1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteRTPS__(void);
}
#else
#define _gteRTPS__ _gteRTPS
#endif

static void recRTPS(void){
	if (autobias) cycles_pending+=15;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecRTPS");
#endif

	iLockReg(3);
	CALLFunc((u32)_gteRTPS__);
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=0;
#endif
#endif
	UnmapDataGteMACs();
#ifdef REC_USE_GTE_MAP_REGS
	u32 rmap1=GetGteDataMapped(17); // gteSZ1
	u32 rmap2=GetGteDataMapped(18); // gteSZ2
	u32 rmap3=GetGteDataMapped(19); // gteSZ3
	if (rmap1 && rmap1!=rmap2) {
		MapDataGte(rmap1,16);
	} else {
		UnmapDataGte(16); // gteSZ0
	}
	if (rmap2 && rmap2!=rmap3) {
		MapDataGte(rmap2,17);
	} else {
		UnmapDataGte(17); // gteSZ1
	}
	if (rmap3 && rmap2!=rmap3) {
		MapDataGte(rmap3,18);
	} else {
		UnmapDataGte(18); // gteSZ2
	}
	UnmapDataGte(19); // gteSZ3
	rmap1=GetGteDataMapped(13); // gteSXY1
	rmap2=GetGteDataMapped(14); // gteSXY2
	if (rmap1 && rmap1!=rmap2) {
		MapDataGte(rmap1,12);
	} else {
		UnmapDataGte(12); // gteSXY0
	}
	if (rmap2 && rmap1!=rmap2) {
		MapDataGte(rmap2,13);
	} else {
		UnmapDataGte(13); // gteSXY1
	}
	UnmapDataGte(14); // gteSXY2
	UnmapDataGte(24); // gteMAC0
	UnmapDataGte(8);  // gteIR0
#endif

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteNCDS__(void);
}
#else
#define _gteNCDS__ _gteNCDS_
#endif

static void recNCDS(void){
	if (autobias) cycles_pending+=19;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecNCDS");
#endif

	iLockReg(3);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	CALLFunc((u32)_gteNCDS__);
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm1_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(1,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteNCDT__(void);
}
#else
#define _gteNCDT__ _gteNCDT_
#endif

static void recNCDT(void){
	if (autobias) cycles_pending+=44;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecNCDT");
#endif

	iLockReg(3);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	CALLFunc((u32)_gteNCDT__);
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm1_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(1,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteCDP__(void);
}
#else
#define _gteCDP__ _gteCDP_
#endif

static void recCDP(void){
	if (autobias) cycles_pending+=13;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecCDP");
#endif

	iLockReg(3);
	UpdateGteDelay(1);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	CALLFunc((u32)_gteCDP__);
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm1_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(1,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteNCCS__(void);
}
#else
#define _gteNCCS__ _gteNCCS_
#endif

static void recNCCS(void){
	if (autobias) cycles_pending+=17;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecNCCS");
#endif

	iLockReg(3);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
#ifndef REC_USE_GTE_ASM_FUNCS
	MOV32RtoR(HOST_r0,HOST_r11);
#else
	u32 saveregs=iGetSaveMask(4,7);
	if (saveregs) {
		write32(ARM_SAVEREG(saveregs));
	}
#endif
	CALLFunc((u32)_gteNCCS__);
#ifdef REC_USE_GTE_ASM_FUNCS
	if (saveregs) {
		write32(ARM_RESTOREREG(saveregs));
	}
#endif
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm1_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(1,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteCC__(void);
}
#else
#define _gteCC__ _gteCC_
#endif

static void recCC(void){
	if (autobias) cycles_pending+=11;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecCC");
#endif

	iLockReg(3);
	UpdateGteDelay(1);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
#ifndef REC_USE_GTE_ASM_FUNCS
	MOV32RtoR(HOST_r0,HOST_r11);
#else
	u32 saveregs=iGetSaveMask(4,6);
	if (saveregs) {
		write32(ARM_SAVEREG(saveregs));
	}
#endif
	CALLFunc((u32)_gteCC__);
#ifdef REC_USE_GTE_ASM_FUNCS
	if (saveregs) {
		write32(ARM_RESTOREREG(saveregs));
	}
#endif
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm1_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(1,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteNCS__(void);
}
#else
#define _gteNCS__ _gteNCS_
#endif

static void recNCS(void){
	if (autobias) cycles_pending+=14;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecNCS");
#endif

	iLockReg(3);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
#ifndef REC_USE_GTE_ASM_FUNCS
	MOV32RtoR(HOST_r0,HOST_r11);
#else
	u32 saveregs=iGetSaveMask(4,6);
	if (saveregs) {
		write32(ARM_SAVEREG(saveregs));
	}
#endif
	CALLFunc((u32)_gteNCS__);
#ifdef REC_USE_GTE_ASM_FUNCS
	if (saveregs) {
		write32(ARM_RESTOREREG(saveregs));
	}
#endif
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm1_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(1,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteNCT__(void);
}
#else
#define _gteNCT__ _gteNCT_
#endif

static void recNCT(void){
	if (autobias) cycles_pending+=30;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecNCT");
#endif

	iLockReg(3);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	CALLFunc((u32)_gteNCT__);
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm1_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(1,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteDPCT__(void);
}
#else
#define _gteDPCT__ _gteDPCT_
#endif

static void recDPCT(void){
	if (autobias) cycles_pending+=17;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecDPCT");
#endif

	iLockReg(3);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	CALLFunc((u32)_gteDPCT__);
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm0_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm0_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(0,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteNCCT__(void);
}
#else
#define _gteNCCT__ _gteNCCT_
#endif

static void recNCCT(void){
	if (autobias) cycles_pending+=39;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecNCCT");
#endif

	iLockReg(3);
#ifdef USE_GTE_FLAG
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	CALLFunc((u32)_gteNCCT__);
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMACs_lm1_ptr;
#else
	CALLFunc(func_GTE_updateMACs_lm1_ptr);
#endif
	CALLFunc(func_GTE_updateCODEs_ptr);
#else
	recGTE_updateMACs(1,0);
	recGTE_updateCODEs();
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteRTPT__(void);
}
#else
#define _gteRTPT__ _gteRTPT_
#endif

static void recRTPT(void){
	if (autobias) cycles_pending+=23;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecRTPT");
#endif

	iLockReg(3);
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
	CALLFunc((u32)_gteRTPT__);
#ifdef REC_USE_GTE_FUNCS
#ifdef REC_USE_GTE_DELAY_CALC
	func_GTE_delay_ptr=func_GTE_updateMAC3_lm0_flag_ptr;
#else
	CALLFunc(func_GTE_updateMAC3_lm0_flag_ptr);
#endif
#else
	recGTE_updateMAC3_flag(0,0,1);
#endif
	UnmapDataGteMACs();
	UnmapDataGteCODEs();
	UnmapCtrlGte(31); // gteFLAG
	UnmapDataGte(17); // gteSZ1
	UnmapDataGte(18); // gteSZ2
	UnmapDataGte(19); // gteSZ3
	UnmapDataGte(12); // gteSXY0
	UnmapDataGte(13); // gteSXY1
	UnmapDataGte(14); // gteSXY2
	UnmapDataGte(24); // gteMAC0
	UnmapDataGte(8);  // gteIR0	

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" { extern void _gteNCLIP_(void); }
#endif

static void recNCLIP(void){
	if (autobias) cycles_pending+=8;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecNCLIP");
#endif
	iLockReg(3);
#ifdef REC_USE_GTE_ASM_FUNCS
	CALLFunc((u32)_gteNCLIP_);
#else
	MOV32RtoR(HOST_r0,HOST_r11);
	CALLFunc((u32)_gteNCLIP);
#endif
	UnmapDataGte(24); // gteMAC0

	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteAVSZ3__(void);
}
#else
#define _gteAVSZ3__ _gteAVSZ3
#endif

static void recAVSZ3(void){
	if (autobias) cycles_pending+=5;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecAVSZ3");
#endif
	iLockReg(3);
#ifndef REC_USE_GTE_ASM_FUNCS
	MOV32RtoR(HOST_r0,HOST_r11);
#endif
	CALLFunc((u32)_gteAVSZ3__);
	UnmapDataGte(24); // gteMAC0
	UnmapDataGte(7);  // gteOTZ
	iUnlockReg(3);
	r2_is_dirty = 1;
}

#ifdef REC_USE_GTE_ASM_FUNCS
extern "C" {
extern void _gteAVSZ4__(void);
}
#else
#define _gteAVSZ4__ _gteAVSZ4
#endif

static void recAVSZ4(void){
	if (autobias) cycles_pending+=6;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	dbg("\t\trecAVSZ4");
#endif
	iLockReg(3);
#ifndef REC_USE_GTE_ASM_FUNCS
	MOV32RtoR(HOST_r0,HOST_r11);
#endif
	CALLFunc((u32)_gteAVSZ4__);
	UnmapDataGte(24); // gteMAC0
	UnmapDataGte(7);  // gteOTZ
	iUnlockReg(3);
	r2_is_dirty = 1;
}


#else
CP2_FUNC(OP,"OP",6);
CP2_FUNC(MVMVA,"MVMVA",8);
CP2_FUNC(DPCS,"DPCS",8);
CP2_FUNC(GPF,"GPF",5);
CP2_FUNC(SQR,"SQR",3);
CP2_FUNC(INTPL,"INTPL",8);
CP2_FUNC(DCPL,"DCPL",8);
CP2_FUNC(GPL,"GPL",5);
CP2_FUNCNC(RTPS,"RTPS",15);
CP2_FUNCNC(NCDS,"NCDS",19);
CP2_FUNCNC(NCDT,"NCDT",44);
CP2_FUNCNC(CDP,"CDP",13);
CP2_FUNCNC(NCCS,"NCCS",17);
CP2_FUNCNC(CC,"CC",11);
CP2_FUNCNC(NCS,"NCS",14);
CP2_FUNCNC(NCT,"NCT",30);
CP2_FUNCNC(DPCT,"DPCT",17);
CP2_FUNCNC(NCCT,"NCCT",39);
CP2_FUNCNC(RTPT,"RTPT",23);
CP2_FUNCNC(NCLIP,"NCLIP",8);
CP2_FUNCNC(AVSZ3,"AVSZ3",5);
CP2_FUNCNC(AVSZ4,"AVSZ4",6);
#endif


