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

/*
* ARM assembly functions for R3000A core.
*/

#include "psxcommon.h"
#include "psxhw.h"
#include "r3000a.h"
#include "psxmem.h"
#include "arm.h"
#include "port.h"

static u32 psxRecLUT[0x010000];

#undef PC_REC
#undef PC_REC8
#undef PC_REC16
#undef PC_REC32
#define PC_REC(x)	(psxRecLUT[x >> 16] + (x & 0xffff))
#define PC_REC8(x)	(*(u8 *)PC_REC(x))
#define PC_REC16(x) (*(u16*)PC_REC(x))
#define PC_REC32(x) (*(u32*)PC_REC(x))

#define RECMEM_SIZE		(12 * 1024 * 1024)

static char recMem[RECMEM_SIZE + 0x1000];	/* the recompiled blocks will be here */
static char recRAM[0x200000];	/* and the ptr to the blocks here */
static char recROM[0x080000];	/* and here */

static u32 pc;			/* recompiler pc */
static u32 pcold;		/* recompiler oldpc */
static int count;		/* recompiler intruction count */
static int branch;		/* set for branch */
static u32 target;		/* branch target */
static u32 block=0;

typedef struct {
	u32 state;
	u32 k;
	u32 reg;
	u32 regw;
} iRegisters;

static iRegisters iRegs[34];
static iRegisters iRegsS[34];
static iRegisters HWRegs[16];
static iRegisters HWRegsS[16];

#define ST_UNK    0
#define ST_CONST  1
#define ST_MAPPED 2

#define IsConst(reg)  (iRegs[reg].state == ST_CONST)
#define IsMapped(reg) (iRegs[reg].state == ST_MAPPED)

extern void (*recBSC[64])();
extern void (*recSPC[64])();
extern void (*recREG[32])();
extern void (*recCP0[32])();
extern void (*recCP2[64])();
extern void (*recCP2BSC[32])();

static void MapConst(u32 reg, u32 _const) {
	if (IsMapped(reg)) HWRegs[iRegs[reg].reg].state=ST_UNK;
	iRegs[reg].k = _const;
	iRegs[reg].state = ST_CONST;
}

static void Unmap(u32 reg)
{
	if (IsMapped(reg)) HWRegs[iRegs[reg].reg].state=ST_UNK;
	iRegs[reg].state = ST_UNK;
}

static void iFlushReg(u32 reg) {
	if (IsConst(reg)) {
		MOV32ItoM_regs((u32)&psxRegs.GPR.r[reg], iRegs[reg].k);
	} else if (IsMapped(reg)) {
		if (iRegs[reg].regw)
		{
			MOV32RtoM_regs((u32)&psxRegs.GPR.r[reg], iRegs[reg].reg);
		}
		HWRegs[iRegs[reg].reg].state=ST_UNK;
	}
	iRegs[reg].state = ST_UNK;
}

static void iFlushRegs() {
	int i;

	if (IsMapped(0)) Unmap(0);
	iRegs[0].k = 0; iRegs[0].state = ST_CONST;
	for (i=1; i<34; i++) {
		iFlushReg(i);
	}
}

static u32 GetReg(u32 reg)
{
	int i,j=0;
	u32 k=0;
	for (i=0;i<16;i++)
	{
		if (HWRegs[i].state==ST_UNK)
		{
			HWRegs[i].state=ST_MAPPED;
			HWRegs[i].k=0;
			HWRegs[i].reg=reg;
			j=i;
			for (i=0;i<16;i++) HWRegs[i].k++;
			return j;
		}
		if ((HWRegs[i].state==ST_MAPPED) && (HWRegs[i].k>k))
		{
			j=i;
			k=HWRegs[i].k;
		}
	}
	iFlushReg(HWRegs[j].reg);
	HWRegs[j].state=ST_MAPPED;
	HWRegs[j].k=0;
	HWRegs[j].reg=reg;
	for (i=0;i<16;i++) HWRegs[i].k++;
	return j;
}

static u32 TempReg(void)
{
	int i,j=0;
	u32 k=0;
	for (i=0;i<16;i++)
	{
		if (HWRegs[i].state==ST_UNK)
		{
			return i;
		}
		if ((HWRegs[i].state==ST_MAPPED) && (HWRegs[i].k>k))
		{
			j=i;
			k=HWRegs[i].k;
		}
	}
	iFlushReg(HWRegs[j].reg);
	return j;
}

static u32 ReadReg(u32 reg) {
	if (iRegs[reg].state!=ST_MAPPED)
	{
		iRegs[reg].reg=GetReg(reg);
		iRegs[reg].state=ST_MAPPED;
		MOV32MtoR_regs(iRegs[reg].reg,&psxRegs.GPR.r[reg]);
		iRegs[reg].regw=0; /* not written */
	}
	else
	{
		HWRegs[iRegs[reg].reg].k=1; /* no flush */
	}
	return iRegs[reg].reg;
}

static u32 WriteReg(u32 reg) {
	if (iRegs[reg].state!=ST_MAPPED)
	{
		iRegs[reg].reg=GetReg(reg);
		iRegs[reg].state=ST_MAPPED;
		iRegs[reg].regw=1; /* written */
	}
	else
	{
		iRegs[reg].regw=1; /* written */
		HWRegs[iRegs[reg].reg].k=1; /* no flush */
	}
	return iRegs[reg].reg;
}

static u32 ReadWriteReg(u32 reg) {
	if (iRegs[reg].state!=ST_MAPPED)
	{
		iRegs[reg].reg=GetReg(reg);
		iRegs[reg].state=ST_MAPPED;
		MOV32MtoR_regs(iRegs[reg].reg,&psxRegs.GPR.r[reg]);
		iRegs[reg].regw=1; /* written */
	}
	else
	{
		iRegs[reg].regw=1; /* written */
		HWRegs[iRegs[reg].reg].k=1; /* no flush */
	}
	return iRegs[reg].reg;
}

static void iRet() {
	/* store cycle */
	count = ((pc - pcold) / 4) * BIAS;
	ADD32ItoM_regs((u32)&psxRegs.cycle, count);
	RET();
}

static int iLoadTest() {
	u32 tmp;

	// check for load delay
	tmp = psxRegs.code >> 26;
	switch (tmp) {
		case 0x10: // COP0
			switch (_Rs_) {
				case 0x00: // MFC0
				case 0x02: // CFC0
					return 1;
			}
			break;
		case 0x12: // COP2
			switch (_Funct_) {
				case 0x00:
					switch (_Rs_) {
						case 0x00: // MFC2
						case 0x02: // CFC2
							return 1;
					}
					break;
			}
			break;
		case 0x32: // LWC2
			return 1;
		default:
			if (tmp >= 0x20 && tmp <= 0x26) { // LB/LH/LWL/LW/LBU/LHU/LWR
				return 1;
			}
			break;
	}
	return 0;
}

/* set a pending branch */
static void SetBranch() {
	branch = 1;
	psxRegs.code = PSXMu32(pc);
	pc += 4;

	if (iLoadTest() == 1) {
		iFlushRegs();
		MOV32ItoM_regs((u32)&psxRegs.code, psxRegs.code);
		/* store cycle */
		count = ((pc - pcold) / 4) * BIAS;
		ADD32ItoM_regs((u32)&psxRegs.cycle, count);

		MOV32MtoR(HOST_a2,(u32)&target);
		MOV32ItoR(HOST_a1,_Rt_);
		CALLFunc((u32)psxDelayTest);

		RET();
		return;
	}

	recBSC[psxRegs.code>>26]();

	iFlushRegs();
	MOV32MtoR(HOST_a1, (u32)&target);
	MOV32RtoM_regs((u32)&psxRegs.pc, HOST_a1);
	CALLFunc((u32)psxBranchTest);

	iRet();
}

static void iJump(u32 branchPC) {
	branch = 1;
	psxRegs.code = PSXMu32(pc);
	pc+=4;

	if (iLoadTest() == 1) {
		iFlushRegs();
		MOV32ItoM_regs((u32)&psxRegs.code, psxRegs.code);
		/* store cycle */
		count = ((pc - pcold) / 4) * BIAS;
		ADD32ItoM_regs((u32)&psxRegs.cycle, count);

		MOV32ItoR(HOST_a2,branchPC);
		MOV32ItoR(HOST_a1,_Rt_);
		CALLFunc((u32)psxDelayTest);

		RET();
		return;
	}

	recBSC[psxRegs.code>>26]();

	iFlushRegs();
	MOV32ItoM_regs((u32)&psxRegs.pc, branchPC);
	CALLFunc((u32)psxBranchTest);
	/* store cycle */
	count = ((pc - pcold) / 4) * BIAS;
	ADD32ItoM_regs((u32)&psxRegs.cycle, count);

	// maybe just happened an interruption, check so
	MOV32MtoR_regs(HOST_a1,(u32)&psxRegs.pc);
	MOV32ItoR(HOST_a2,branchPC);
	write32(CMP_REGS(HOST_a1,HOST_a2));
	j8Ptr[0]=armPtr; write32(BEQ_FWD(0));
	RET_NC();
	armSetJ8(j8Ptr[0]);
	
	MOV32MtoR(HOST_a1,PC_REC(branchPC));
	j8Ptr[1]=JNZ8(HOST_a1);
	RET_NC();
	armSetJ8(j8Ptr[1]);

	MOV32RtoR(HOST_pc,HOST_a1);
}

static void iBranch(u32 branchPC, int savectx) {

	if (savectx) {
		memcpy(iRegsS, iRegs, sizeof(iRegs));
		memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
	}

	branch = 1;
	psxRegs.code = PSXMu32(pc);

	// the delay test is only made when the branch is taken
	// savectx == 0 will mean that :)
	if (savectx == 0 && iLoadTest() == 1) {
		iFlushRegs();
		MOV32ItoM_regs((u32)&psxRegs.code, psxRegs.code);
		/* store cycle */
		count = (((pc+4) - pcold) / 4) * BIAS;
		ADD32ItoM_regs((u32)&psxRegs.cycle, count);

		MOV32ItoR(HOST_a2,branchPC);
		MOV32ItoR(HOST_a1,_Rt_);
		CALLFunc((u32)psxDelayTest);

		RET();
		return;
	}

	pc+= 4;
	recBSC[psxRegs.code>>26]();

	iFlushRegs();
	MOV32ItoM_regs((u32)&psxRegs.pc, branchPC);
	CALLFunc((u32)psxBranchTest);
	/* store cycle */
	count = ((pc - pcold) / 4) * BIAS;
	ADD32ItoM_regs((u32)&psxRegs.cycle, count);

	// maybe just happened an interruption, check so
	MOV32MtoR_regs(HOST_a1,(u32)&psxRegs.pc);
	MOV32ItoR(HOST_a2,branchPC);
	write32(CMP_REGS(HOST_a1,HOST_a2));
	j8Ptr[1]=armPtr; write32(BEQ_FWD(0));
	RET_NC();
	armSetJ8(j8Ptr[1]);

	MOV32MtoR(HOST_a1,PC_REC(branchPC));
	j8Ptr[2]=JNZ8(HOST_a1);
	RET_NC();
	armSetJ8(j8Ptr[2]);

	MOV32RtoR(HOST_pc,HOST_a1);

	pc-= 4;
	if (savectx) {
		memcpy(iRegs, iRegsS, sizeof(iRegs));
		memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
	}
}

static int recInit() {
	int i;

	memset(recMem,0,RECMEM_SIZE + 0x1000);

	for (i=0; i<0x80; i++) psxRecLUT[i + 0x0000] = (u32)&recRAM[(i & 0x1f) << 16];
	memcpy(psxRecLUT + 0x8000, psxRecLUT, 0x80 * 4);
	memcpy(psxRecLUT + 0xa000, psxRecLUT, 0x80 * 4);

	for (i=0; i<0x08; i++) psxRecLUT[i + 0xbfc0] = (u32)&recROM[i << 16];

	return 0;
}

static void recReset() {
	memset(recRAM, 0, 0x200000);
	memset(recROM, 0, 0x080000);

	armPtr=(u32*)recMem;

	branch = 0;
	memset(iRegs, 0, sizeof(iRegs));
	iRegs[0].state = ST_CONST;
	iRegs[0].k     = 0;
	
	memset(HWRegs, 0, sizeof(HWRegs));
	HWRegs[0].state = ST_CONST; /* do not map r0 */
	HWRegs[1].state = ST_CONST; /* do not map r1 */
	HWRegs[2].state = ST_CONST; /* do not map r2 */
	HWRegs[3].state = ST_CONST; /* do not map r3 */
	HWRegs[11].state = ST_CONST; /* do not map r11 (v8) - psxRegs pointer */
	HWRegs[12].state = ST_CONST; /* do not map r12 (ip) */
	HWRegs[13].state = ST_CONST; /* do not map r13 (sp) */
	HWRegs[14].state = ST_CONST; /* do not map r14 (lr) */
	HWRegs[15].state = ST_CONST; /* do not map r15 (pc) */
}

static void recShutdown() {
}

#include "opcodes.h"

static void recRecompile() {
	u32 *armPtr_old;

	/* if armPtr reached the mem limit reset whole mem */
	if (((u32)armPtr - (u32)recMem) >= (RECMEM_SIZE - 0x10000))
		recReset();

	armPtr = (u32*)(((u32)armPtr + 32) & ~(31));
	armPtr_old=armPtr;
		
	PC_REC32(psxRegs.pc) = (u32)armPtr;
	pc = psxRegs.pc;
	pcold = pc;

	for (count = 0; count < 500;) {
		psxRegs.code = *(u32 *)((char *)PSXM(pc));

		pc += 4;
		count++;
		recBSC[psxRegs.code >> 26]();

		if (branch) {
			branch = 0;
			sys_cacheflush(armPtr_old,armPtr);
			return;
		}
	}

	iFlushRegs();

	MOV32ItoM_regs((u32)&psxRegs.pc, pc);

	iRet();
	
	sys_cacheflush(armPtr_old,armPtr);
}

static void recExecute() {
	for (;;)
	{
		u32 *p = (u32*)PC_REC(psxRegs.pc);
		if (*p == 0) recRecompile();
		recRun(*p,(u32)&psxRegs);
	}
}

static void recExecuteBlock(unsigned target_pc) {
	block=1;
	do{
		u32 *p = (u32*)PC_REC(psxRegs.pc);
		if (*p == 0) recRecompile();
		recRun(*p,(u32)&psxRegs);
	}while(psxRegs.pc!=target_pc);
	block=0;
}

static void recClear(u32 Addr, u32 Size) {
	memset((void*)PC_REC(Addr), 0, Size * 4);
}

R3000Acpu psxRec = {
	recInit,
	recReset,
	recExecute,
	recExecuteBlock,
	recClear,
	recShutdown
};
