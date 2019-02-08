/**************************************************************************
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

#if defined(REC_USE_NO_REPEAT_READ_WRITE) && !defined(DEBUG_CPU_OPCODES)
static u32 repeat_read_write_pc=0;
static u32 repeat_read_write_imm=0;
static u32 repeat_read_write_reg_dst=0;
static u32 repeat_read_write_reg_src=0;
#endif

#if defined(interpreter_new) || defined (interpreter_none)
// CHUI: Se podria poner en linea:
static const u32 LWL_MASK[4] = { 0xffffff, 0xffff, 0xff, 0 };
static const u32 LWL_SHIFT[4] = { 24, 16, 8, 0 };
static const u32 LWR_MASK[4] = { 0, 0xff000000, 0xffff0000, 0xffffff00 };
static const u32 LWR_SHIFT[4] = { 0, 8, 16, 24 };
static const u32 SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0 };
static const u32 SWL_SHIFT[4] = { 24, 16, 8, 0 };
static const u32 SWR_MASK[4] = { 0, 0xff, 0xffff, 0xffffff };
static const u32 SWR_SHIFT[4] = { 0, 8, 16, 24 };
#else
extern u32 LWL_MASK[4];
extern u32 LWL_SHIFT[4];
extern u32 LWR_MASK[4];
extern u32 LWR_SHIFT[4];
extern u32 SWL_MASK[4];
extern u32 SWL_SHIFT[4];
extern u32 SWR_MASK[4];
extern u32 SWR_SHIFT[4];
#endif

#if defined(REC_USE_MEMORY_FUNCS) && defined(REC_USE_DIRECT_MEM)
static unsigned func_MemRead8_direct_ptr=0;

static void recMemRead8_direct(void) {
	write32(MOV_REG_LSR_IMM(HOST_ip,HOST_r0,16));	//  mov ip, r0, lsr #16
	write32(MOV_REG_LSL_IMM(HOST_r2,HOST_r0,16));	//  mov r2, r0, lsl #16
	write32(CMP_IMM(HOST_ip,0x1f,0x18));		//  cmp ip, #0x1f00
	write32(MOV_REG_LSR_IMM(HOST_r2,HOST_r2,16));	//  mov r2, r2, lsr #16
	j32Ptr[1]=armPtr; write32(BEQ_FWD(0));		//  beq .salto1
	write32(AND_IMM(HOST_r3,HOST_r0,0x7,0x10));	//  and r3, r0, #0x70000
	write32(CMP_IMM(HOST_ip,0x7e,0x1a));		//  cmp ip, #0x1f80
	write32(ORR_REG_LSL_IMM(HOST_ip,HOST_r2,HOST_r3,0));// orr ip, r2, r3
	j32Ptr[2]=armPtr; write32(BEQ_FWD(0));		//  beq .salto2
	MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.psxR);	//  r2=psxR
	write32(LDRB_REG(HOST_r0,HOST_ip,HOST_r2));	//  ldrb r0, [ip, r2]
	write32(BX_LR());				//  bx lr
	armSetJ32(j32Ptr[1]);				// salto1:
	MOV32MtoR_regs(HOST_r3,(u32)&psxRegs.psxP);	//  r3=psxP
	write32(LDRB_REG(HOST_r0,HOST_r2,HOST_r3));	//  ldrb r0, [r2, r3]
	write32(BX_LR());				//  bx lr
	armSetJ32(j32Ptr[2]);				// salto2:
	write32(CMP_IMM(HOST_r2,0x01,0x14));		// cmp r2, #0x1000
	{ u32 offset=((((u32)(psxHwRead8) - ((u32)(armPtr) + (8))) >> 2) & 0xFFFFFF); write32(BCS_FWD_(offset)); }			// bcs psxHwRead8
	MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.psxH);	//  ip=psxH
	write32(LDRB_REG(HOST_r0,HOST_r2,HOST_ip));	//  ldrb r0, [r2, ip]
	write32(BX_LR());				//  bx lr
	UpdateImmediate(0);
}

static unsigned func_MemRead16_direct_ptr=0;

static void recMemRead16_direct(void) {
	write32(MOV_REG_LSR_IMM(HOST_ip,HOST_r0,16));	//  mov ip, r0, lsr #16
	write32(MOV_REG_LSL_IMM(HOST_r2,HOST_r0,16));	//  mov r2, r0, lsl #16
	write32(CMP_IMM(HOST_ip,0x1f,0x18));		//  cmp ip, #0x1f00
	write32(MOV_REG_LSR_IMM(HOST_r2,HOST_r2,16));	//  mov r2, r2, lsr #16
	j32Ptr[1]=armPtr; write32(BEQ_FWD(0));		//  beq .salto1
	write32(AND_IMM(HOST_r3,HOST_r0,0x7,0x10));	//  and r3, r0, #0x70000
	write32(CMP_IMM(HOST_ip,0x7e,0x1a));		//  cmp ip, #0x1f80
	write32(ORR_REG_LSL_IMM(HOST_ip,HOST_r2,HOST_r3,0));// orr ip, r2, r3
	j32Ptr[2]=armPtr; write32(BEQ_FWD(0));		//  beq .salto2
	MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.psxR);	//  r2=psxR
	write32(LDRH_REG(HOST_r0,HOST_ip,HOST_r2));	//  ldrh r0, [ip, r2]
	write32(BX_LR());				//  bx lr
	armSetJ32(j32Ptr[1]);				// salto1:
	MOV32MtoR_regs(HOST_r3,(u32)&psxRegs.psxP);	//  r3=psxP
	write32(LDRH_REG(HOST_r0,HOST_r2,HOST_r3));	//  ldrh r0, [r2, r3]
	write32(BX_LR());				//  bx lr
	armSetJ32(j32Ptr[2]);				// salto2:
	write32(CMP_IMM(HOST_r2,0x01,0x14));		// cmp r2, #0x1000
	{ u32 offset=((((u32)(psxHwRead16) - ((u32)(armPtr) + (8))) >> 2) & 0xFFFFFF); write32(BCS_FWD_(offset)); }			// bcs psxHwRead16
	MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.psxH);	//  ip=psxH
	write32(LDRH_REG(HOST_r0,HOST_r2,HOST_ip));	//  ldrh r0, [r2, ip]
	write32(BX_LR());				//  bx lr
	UpdateImmediate(0);
}

static unsigned func_MemRead32_direct_ptr=0;

static void recMemRead32_direct(void) {
	write32(MOV_REG_LSR_IMM(HOST_ip,HOST_r0,16));	//  mov ip, r0, lsr #16
	write32(MOV_REG_LSL_IMM(HOST_r2,HOST_r0,16));	//  mov r2, r0, lsl #16
	write32(CMP_IMM(HOST_ip,0x1f,0x18));		//  cmp ip, #0x1f00
	write32(MOV_REG_LSR_IMM(HOST_r2,HOST_r2,16));	//  mov r2, r2, lsr #16
	j32Ptr[1]=armPtr; write32(BEQ_FWD(0));		//  beq .salto1
	write32(AND_IMM(HOST_r3,HOST_r0,0x7,0x10));	//  and r3, r0, #0x70000
	write32(CMP_IMM(HOST_ip,0x7e,0x1a));		//  cmp ip, #0x1f80
	write32(ORR_REG_LSL_IMM(HOST_ip,HOST_r2,HOST_r3,0));// orr ip, r2, r3
	j32Ptr[2]=armPtr; write32(BEQ_FWD(0));		//  beq .salto2
	MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.psxR);	//  r2=psxR
	write32(LDR_REG(HOST_r0,HOST_ip,HOST_r2));	//  ldr r0, [ip, r2]
	write32(BX_LR());				//  bx lr
	armSetJ32(j32Ptr[1]);				// salto1:
	MOV32MtoR_regs(HOST_r3,(u32)&psxRegs.psxP);	//  r3=psxP
	write32(LDR_REG(HOST_r0,HOST_r2,HOST_r3));	//  ldr r0, [r2, r3]
	write32(BX_LR());				//  bx lr
	armSetJ32(j32Ptr[2]);				// salto2:
	write32(CMP_IMM(HOST_r2,0x01,0x14));		// cmp r2, #0x1000
	{ u32 offset=((((u32)(psxHwRead32) - ((u32)(armPtr) + (8))) >> 2) & 0xFFFFFF); write32(BCS_FWD_(offset)); }			// bcs psxHwRead32
	MOV32MtoR_regs(HOST_ip,(u32)&psxRegs.psxH);	//  ip=psxH
	write32(LDR_REG(HOST_r0,HOST_r2,HOST_ip));	//  ldr r0, [r2, ip]
	write32(BX_LR());				//  bx lr
	UpdateImmediate(0);
}

static unsigned func_MemWrite8_direct_ptr=0;

static void recMemWrite8_direct(void) {
	write32(MOV_REG_LSR_IMM(HOST_r3,HOST_r0,16));	// mov r3, r0, lsr #16
	write32(CMP_IMM(HOST_r3,0x7e,0x1a));		// cmp r3, #0x1f80
	write32(LDRNE_IMM(HOST_r3,HOST_r11,(((u32)(&psxRegs.psxP))-((u32)&psxRegs))));// ldrne   r3, [r11, #regs->psxP]
	write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));	// mov ip, r0, lsl #16
	write32(MOV_REG_LSR_IMM(HOST_ip,HOST_ip,16));	// mov ip, ip, lsr #16
	write32(STRNEB_REG(HOST_r1,HOST_ip,HOST_r3));	// strneb r1, [ip, r3]
	write32(BXNE_LR());				// bxne lr
	write32(CMP_IMM(HOST_ip,0x01,0x14));		// cmp ip, #0x1000
	write32(LDRCC_IMM(HOST_r3,HOST_r11,(((u32)(&psxRegs.psxH))-((u32)&psxRegs))));// ldrcc   r3, [r11, #regs->psxH]
	write32(STRCCB_REG(HOST_r1,HOST_ip,HOST_r3));	// strccb r1, [ip, r3]
	write32(BXCC_LR());				// bxcc lr
	JUMPFunc((u32)psxHwWrite8);			// b psxHwWrite8
	UpdateImmediate(0);
}

static unsigned func_MemWrite16_direct_ptr=0;

static void recMemWrite16_direct(void) {
	write32(MOV_REG_LSR_IMM(HOST_r3,HOST_r0,16));	// mov r3, r0, lsr #16
	write32(CMP_IMM(HOST_r3,0x7e,0x1a));		// cmp r3, #0x1f80
	write32(LDRNE_IMM(HOST_r3,HOST_r11,(((u32)(&psxRegs.psxP))-((u32)&psxRegs))));// ldrne   r3, [r11, #regs->psxP]
	write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));	// mov ip, r0, lsl #16
	write32(MOV_REG_LSR_IMM(HOST_ip,HOST_ip,16));	// mov ip, ip, lsr #16
	write32(STRNEH_REG(HOST_r1,HOST_ip,HOST_r3));	// strneh r1, [ip, r3]
	write32(BXNE_LR());				// bxne lr
	write32(CMP_IMM(HOST_ip,0x01,0x14));		// cmp ip, #0x1000
	write32(LDRCC_IMM(HOST_r3,HOST_r11,(((u32)(&psxRegs.psxH))-((u32)&psxRegs))));// ldrcc   r3, [r11, #regs->psxH]
	write32(STRCCH_REG(HOST_r1,HOST_ip,HOST_r3));	// strcch r1, [ip, r3]
	write32(BXCC_LR());				// bxcc lr
	JUMPFunc((u32)psxHwWrite16);			// b psxHwWrite16
	UpdateImmediate(0);
}

static unsigned func_MemWrite32_direct_ptr=0;

static void recMemWrite32_direct(void) {
	write32(MOV_REG_LSR_IMM(HOST_r3,HOST_r0,16));	// mov r3, r0, lsr #16
	write32(CMP_IMM(HOST_r3,0x7e,0x1a));		// cmp r3, #0x1f80
	write32(LDRNE_IMM(HOST_r3,HOST_r11,(((u32)(&psxRegs.psxP))-((u32)&psxRegs))));// ldrne   r3, [r11, #regs->psxP]
	write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));	// mov ip, r0, lsl #16
	write32(MOV_REG_LSR_IMM(HOST_ip,HOST_ip,16));	// mov ip, ip, lsr #16
	write32(STRNE_REG(HOST_r1,HOST_ip,HOST_r3));	// strne r1, [ip, r3]
	write32(BXNE_LR());				// bxne lr
	write32(CMP_IMM(HOST_ip,0x01,0x14));		// cmp ip, #0x1000
	write32(LDRCC_IMM(HOST_r3,HOST_r11,(((u32)(&psxRegs.psxH))-((u32)&psxRegs))));// ldrcc   r3, [r11, #regs->psxH]
	write32(STRCC_REG(HOST_r1,HOST_ip,HOST_r3));	// strcc r1, [ip, r3]
	write32(BXCC_LR());				// bxcc lr
	JUMPFunc((u32)psxHwWrite32);			// b psxHwWrite32
	UpdateImmediate(0);
}
#endif

#ifdef REC_USE_DIRECT_MEM
#ifdef REC_USE_MEMORY_FUNCS
#define MEMREAD8_FUNC func_MemRead8_direct_ptr
#define MEMREAD16_FUNC func_MemRead16_direct_ptr
#define MEMREAD32_FUNC func_MemRead32_direct_ptr
#else
#define MEMREAD8_FUNC psxMemRead8_direct
#define MEMREAD16_FUNC psxMemRead16_direct
#define MEMREAD32_FUNC psxMemRead32_direct
#endif
#if defined(REC_USE_MEMORY_FUNCS) && !defined(DEBUG_CPU_OPCODES)
#define MEMWRITE8_FUNC func_MemWrite8_direct_ptr
#define MEMWRITE16_FUNC func_MemWrite16_direct_ptr
#define MEMWRITE32_FUNC func_MemWrite32_direct_ptr
#else
#define MEMWRITE8_FUNC psxMemWrite8_direct
#define MEMWRITE16_FUNC psxMemWrite16_direct
#define MEMWRITE32_FUNC psxMemWrite32_direct
#endif
#else
#define MEMREAD8_FUNC psxMemRead8
#define MEMREAD16_FUNC psxMemRead16
#define MEMREAD32_FUNC psxMemRead32
#define MEMWRITE8_FUNC psxMemWrite8
#define MEMWRITE16_FUNC psxMemWrite16
#define MEMWRITE32_FUNC psxMemWrite32
#endif

#ifndef REC_USE_MEMORY_FUNCS
INLINE void PSXMEMREAD8(void){
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMREAD8");
#endif
	if (autobias) cycles_pending+=1;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
	write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp r3, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
	write32(LDRB_REG(HOST_r0,HOST_r2,HOST_r1));	//  ldrb r0, [r2, r1]
	j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
#ifdef REC_USE_DIRECT_MEM
	MOV32RtoR(HOST_r1,HOST_r11);
#endif
	CALLFunc((u32)MEMREAD8_FUNC);			//  bl MEMREAD8_FUNC
	armSetJ32(j32Ptr[10]);				// fin:
	r2_is_dirty=1;
}
#else

static unsigned func_MemRead8_ptr=0;

static void recMemRead8(void) {
	MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
	write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic ip, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp ip, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	write32(LDRCCB_REG(HOST_r0,HOST_r2,HOST_r1));   // ldrccb r0, [r2, r1]
	write32(BXCC_LR());				// bxcc lr
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32RtoM_regs((u32)&psxRegs.cycle_add,HOST_r3);
#endif
#ifdef REC_USE_DIRECT_MEM
	func_MemRead8_direct_ptr=(unsigned)armPtr;
	recMemRead8_direct();
#else
	JUMPFunc((u32)MEMREAD8_FUNC);                   //  b MEMREAD8_FUNC
	UpdateImmediate(0);
#endif

}

INLINE void PSXMEMREAD8(void) {
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMREAD8");
#endif
	if (autobias) cycles_pending+=1;
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32ItoR(HOST_r3,(((pc - pcold) / 4) * BIAS));
#else
	iPutCyclesAdd(0);
#endif
	CALLFunc(func_MemRead8_ptr);
	r2_is_dirty=1;
}
#endif

#ifndef REC_USE_MEMORY_FUNCS
INLINE void PSXMEMREAD16(void){
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMREAD16");
#endif 
	if (autobias) cycles_pending+=1;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
	write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp r3, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
	write32(LDRH_REG(HOST_r0,HOST_r2,HOST_r1));	//  ldrh r0, [r2, r1]
	j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
#ifdef REC_USE_DIRECT_MEM
	MOV32RtoR(HOST_r1,HOST_r11);
#endif
	CALLFunc((u32)MEMREAD16_FUNC);				//  bl MEMREAD16_FUNC
	armSetJ32(j32Ptr[10]);						// fin:
	r2_is_dirty=1;
}
#else

static unsigned func_MemRead16_ptr=0;

static void recMemRead16(void) {
	MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
	write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic ip, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp ip, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	write32(LDRCCH_REG(HOST_r0,HOST_r2,HOST_r1));   // ldrcch r0, [r2, r1]
	write32(BXCC_LR());				// bxcc lr
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32RtoM_regs((u32)&psxRegs.cycle_add,HOST_r3);
#endif
#ifdef REC_USE_DIRECT_MEM
	func_MemRead16_direct_ptr=(unsigned)armPtr;
	recMemRead16_direct();
#else
	JUMPFunc((u32)MEMREAD16_FUNC);                  //  b MEMREAD16_FUNC
	UpdateImmediate(0);
#endif

}

INLINE void PSXMEMREAD16(void) {
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMREAD16");
#endif
	if (autobias) cycles_pending+=1;
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32ItoR(HOST_r3,(((pc - pcold) / 4) * BIAS));
#else
	iPutCyclesAdd(0);
#endif
	CALLFunc(func_MemRead16_ptr);
	r2_is_dirty=1;
}
#endif

#ifndef REC_USE_MEMORY_FUNCS
INLINE void PSXMEMREAD32(unsigned dest_reg){
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMREAD32");
#endif 
	if (autobias) cycles_pending+=1;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
	write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp r3, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
	write32(LDR_REG(dest_reg,HOST_r2,HOST_r1));	//  ldr r0, [r2, r1]
	j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
#ifdef REC_USE_DIRECT_MEM
	MOV32RtoR(HOST_r1,HOST_r11);
#endif
	CALLFunc((u32)MEMREAD32_FUNC);			//  bl MEMREAD32_FUNC
	if (dest_reg!=HOST_r0)
		MOV32RtoR(dest_reg, HOST_r0);
	armSetJ32(j32Ptr[10]);				// fin:
	r2_is_dirty=1;
}
#else

static unsigned func_MemRead32_ptr=0;

static void recMemRead32(void) {
	MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
	write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic ip, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp ip, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	write32(LDRCC_REG(HOST_r0,HOST_r2,HOST_r1));   // ldrcc r0, [r2, r1]
	write32(BXCC_LR());				// bxcc lr
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32RtoM_regs((u32)&psxRegs.cycle_add,HOST_r3);
#endif
#ifdef REC_USE_DIRECT_MEM
	func_MemRead32_direct_ptr=(unsigned)armPtr;
	recMemRead32_direct();
#else
	JUMPFunc((u32)MEMREAD32_FUNC);                  //  b MEMREAD32_FUNC
	UpdateImmediate(0);
#endif

}

INLINE void PSXMEMREAD32(unsigned dest_reg) {
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMREAD32");
#endif
	if (autobias) cycles_pending+=1;
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32ItoR(HOST_r3,(((pc - pcold) / 4) * BIAS));
#else
	iPutCyclesAdd(0);
#endif
	CALLFunc(func_MemRead32_ptr);
	if (dest_reg!=HOST_r0)
		MOV32RtoR(dest_reg, HOST_r0);
	r2_is_dirty=1;
}
#endif

#ifndef REC_USE_MEMORY_FUNCS
INLINE void PSXHWREAD8(void){ 	
#ifdef DEBUG_CPU
	dbg("\t\tPSXHWREAD8");
#endif
	if (autobias) cycles_pending+=1;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));
	write32(0xe1a02800); /*	mov	r2, r0, lsl #16 */ 
	write32(CMP_IMM(HOST_ip, 0x7e, 0x1a));
	write32(0xe1a02822); /*	mov	r2, r2, lsr #16 */ 
	write32(BNE_FWD(8)); /* bne fin */ 
	write32(0xe3520a01); /*	cmp	r2, #4096	; 0x1000 */ 
	write32(BCC_FWD(0)); /* bcc fin */ 
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	CALLFunc((u32)psxHwRead8);
	r2_is_dirty=1;
}
#else

static unsigned func_HWRead8_ptr=0;

static void recHWRead8(void) {
	write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));
	write32(0xe1a02800); /*	mov	r2, r0, lsl #16 */ 
	write32(CMP_IMM(HOST_ip, 0x7e, 0x1a));
	write32(0xe1a02822); /*	mov	r2, r2, lsr #16 */ 
	write32(BXNE_LR());  /* bxne lr */ 
	write32(0xe3520a01); /*	cmp	r2, #4096	; 0x1000 */ 
	write32(BXCC_LR());	 /* bxcc lr */
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32RtoM_regs((u32)&psxRegs.cycle_add,HOST_r3);
#endif
	JUMPFunc((u32)psxHwRead8);			//  b psxHwRead8
	UpdateImmediate(0);
}

INLINE void PSXHWREAD8(void){ 	
#ifdef DEBUG_CPU
	dbg("\t\tPSXHWREAD8");
#endif
	if (autobias) cycles_pending+=1;
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32ItoR(HOST_r3,(((pc - pcold) / 4) * BIAS));
#else
	iPutCyclesAdd(0);
#endif
	CALLFunc(func_HWRead8_ptr);
	r2_is_dirty=1;
}
#endif

#ifndef REC_USE_MEMORY_FUNCS
INLINE void PSXHWREAD16(void){
#ifdef DEBUG_CPU
	dbg("\t\tPSXHWREAD16");
#endif
	if (autobias) cycles_pending+=1;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));
	write32(0xe1a02800); /*	mov	r2, r0, lsl #16 */ 
	write32(CMP_IMM(HOST_ip, 0x7e, 0x1a));
	write32(0xe1a02822); /*	mov	r2, r2, lsr #16 */ 
	write32(BNE_FWD(8)); /* bne fin */ 
	write32(0xe3520a01); /*	cmp	r2, #4096	; 0x1000 */ 
	write32(BCC_FWD(0)); /* bcc fin */ 
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	CALLFunc((u32)psxHwRead16);
	r2_is_dirty=1; 
}
#else

static unsigned func_HWRead16_ptr=0;

static void recHWRead16(void) {
	write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));
	write32(0xe1a02800); /*	mov	r2, r0, lsl #16 */ 
	write32(CMP_IMM(HOST_ip, 0x7e, 0x1a));
	write32(0xe1a02822); /*	mov	r2, r2, lsr #16 */ 
	write32(BXNE_LR());  /* bxne lr */ 
	write32(0xe3520a01); /*	cmp	r2, #4096	; 0x1000 */ 
	write32(BXCC_LR());	 /* bxcc lr */
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32RtoM_regs((u32)&psxRegs.cycle_add,HOST_r3);
#endif
	JUMPFunc((u32)psxHwRead16);			//  b psxHwRead16
	UpdateImmediate(0);
}

INLINE void PSXHWREAD16(void){ 	
#ifdef DEBUG_CPU
	dbg("\t\tPSXHWREAD16");
#endif
	if (autobias) cycles_pending+=1;
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32ItoR(HOST_r3,(((pc - pcold) / 4) * BIAS));
#else
	iPutCyclesAdd(0);
#endif
	CALLFunc(func_HWRead16_ptr);
	r2_is_dirty=1;
}
#endif

#ifndef REC_USE_MEMORY_FUNCS	
INLINE void PSXHWREAD32(void){ 	
#ifdef DEBUG_CPU
	dbg("\t\tPSXHWREAD32");
#endif
	if (autobias) cycles_pending+=1;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));
	write32(0xe1a02800); /*	mov	r2, r0, lsl #16 */ 
	write32(CMP_IMM(HOST_ip, 0x7e, 0x1a));
	write32(0xe1a02822); /*	mov	r2, r2, lsr #16 */ 
	write32(BNE_FWD(8)); /* bne fin */ 
	write32(0xe3520a01); /*	cmp	r2, #4096	; 0x1000 */ 
	write32(BCC_FWD(0)); /* bcc fin */ 
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	CALLFunc((u32)psxHwRead32);
	r2_is_dirty=1; 
}
#else

static unsigned func_HWRead32_ptr=0;

static void recHWRead32(void) {
	write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));
	write32(0xe1a02800); /*	mov	r2, r0, lsl #16 */ 
	write32(CMP_IMM(HOST_ip, 0x7e, 0x1a));
	write32(0xe1a02822); /*	mov	r2, r2, lsr #16 */ 
	write32(BXNE_LR());  /* bxne lr */ 
	write32(0xe3520a01); /*	cmp	r2, #4096	; 0x1000 */ 
	write32(BXCC_LR());	 /* bxcc lr */
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32RtoM_regs((u32)&psxRegs.cycle_add,HOST_r3);
#endif
	JUMPFunc((u32)psxHwRead32);			//  b psxHwRead32
	UpdateImmediate(0);
}

INLINE void PSXHWREAD32(void){ 	
#ifdef DEBUG_CPU
	dbg("\t\tPSXHWREAD32");
#endif
	if (autobias) cycles_pending+=1;
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	MOV32ItoR(HOST_r3,(((pc - pcold) / 4) * BIAS));
#else
	iPutCyclesAdd(0);
#endif
	CALLFunc(func_HWRead32_ptr);
	r2_is_dirty=1;
}
#endif

#if !defined(REC_USE_MEMORY_FUNCS) || defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES) 
INLINE void PSXMEMWRITE8(unsigned src_reg){
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMWRITE8");
#endif 
	if (autobias) cycles_pending+=2;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	has_been_written=1;
	MOV32MtoR_regs(HOST_ip,&psxRegs.psxM);		// ip=psxM
	write32(BIC_IMM(HOST_lr,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_lr,0x02,0xa)); 		//  cmp r3, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
	write32(STRB_REG(src_reg,HOST_r2,HOST_ip));	//  strb r1, [r2, ip]
	if (rec_secure_writes) {
		write32(BIC_IMM(HOST_r0,HOST_r0,3,0));  // bic r0, r0, #3
		write32(MOV_REG_LSR_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsr #16
		MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.reserved); // r2=psxRecLUT
		write32(LDR_REG_LSL(HOST_r2, HOST_r2, HOST_ip, 2));// ldr r2, [r2, ip, lsl #2]
		write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsl #16
		write32(MOV_REG_LSR_IMM(HOST_r0,HOST_ip,16));// mov r0, ip, lsr #16
		write32(MOV_IMM(HOST_ip,0,0));          // mov ip, #0
		write32(STR_REG(HOST_ip,HOST_r0,HOST_r2));// str ip, [r0, r2]
	}
	j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[9]);						// salto:
	if (src_reg!=HOST_r1)
		MOV32RtoR(HOST_r1,src_reg);
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
#ifdef REC_USE_DIRECT_MEM
	MOV32RtoR(HOST_r2,HOST_r11);
#endif
	CALLFunc((u32)MEMWRITE8_FUNC);				//  bl MEMWRITE8_FUNC
	armSetJ32(j32Ptr[10]);						// fin:
	r2_is_dirty=1;
}
#else

static unsigned func_MemWrite8_ptr=0;

static void recMemWrite8(void) {
	MOV32MtoR_regs(HOST_ip,&psxRegs.psxM);		// ip=psxM
	write32(BIC_IMM(HOST_r3,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_r3,0x02,0xa)); 		//  cmp r3, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	if (rec_secure_writes) {
		j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
		write32(STRB_REG(HOST_r1,HOST_r2,HOST_ip));	//  strb r1, [r2, ip]
		write32(BIC_IMM(HOST_r0,HOST_r0,3,0));  // bic r0, r0, #3
		write32(MOV_REG_LSR_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsr #16
		MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.reserved); // r2=psxRecLUT
		write32(LDR_REG_LSL(HOST_r2, HOST_r2, HOST_ip, 2));// ldr r2, [r2, ip, lsl #2]
		write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsl #16
		write32(MOV_REG_LSR_IMM(HOST_r0,HOST_ip,16));// mov r0, ip, lsr #16
		write32(MOV_IMM(HOST_ip,0,0));          // mov ip, #0
		write32(STR_REG(HOST_ip,HOST_r0,HOST_r2));// str ip, [r0, r2]
		write32(BX_LR());			//  bx lr
		armSetJ32(j32Ptr[9]);			// salto:
	} else {
		write32(STRCCB_REG(HOST_r1,HOST_r2,HOST_ip));   // strccb r0, [r2, r1]
		write32(BXCC_LR());				// bxcc lr
	}
#ifdef REC_USE_DIRECT_MEM
	func_MemWrite8_direct_ptr=(unsigned)armPtr;
	recMemWrite8_direct();
#else
	JUMPFunc((u32)MEMWRITE8_FUNC);                  //  b MEMWRITE8_FUNC
	UpdateImmediate(0);
#endif

}

INLINE void PSXMEMWRITE8(unsigned src_reg){
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMWRITE8");
#endif 
	if (autobias) cycles_pending+=2;
	iPutCyclesAdd(0);
	has_been_written=1;
	if (src_reg!=HOST_r1)
		MOV32RtoR(HOST_r1,src_reg);
	CALLFunc(func_MemWrite8_ptr);
	r2_is_dirty=1;
}
#endif

#if !defined(REC_USE_MEMORY_FUNCS) || defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES) 
INLINE void PSXMEMWRITE16(unsigned src_reg){
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMWRITE16");
#endif 
	if (autobias) cycles_pending+=2;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	has_been_written=1;
	MOV32MtoR_regs(HOST_ip,&psxRegs.psxM);		// ip=psxM
	write32(BIC_IMM(HOST_lr,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_lr,0x02,0xa)); 		//  cmp r3, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
	write32(STRH_REG(src_reg,HOST_r2,HOST_ip));	//  strh r1, [r2, ip]
	if (rec_secure_writes) {
		write32(BIC_IMM(HOST_r0,HOST_r0,3,0));  // bic r0, r0, #3
		write32(MOV_REG_LSR_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsr #16
		MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.reserved); // r2=psxRecLUT
		write32(LDR_REG_LSL(HOST_r2, HOST_r2, HOST_ip, 2));// ldr r2, [r2, ip, lsl #2]
		write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsl #16
		write32(MOV_REG_LSR_IMM(HOST_r0,HOST_ip,16));// mov r0, ip, lsr #16
		write32(MOV_IMM(HOST_ip,0,0));          // mov ip, #0
		write32(STR_REG(HOST_ip,HOST_r0,HOST_r2));// str ip, [r0, r2]
	}
	j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[9]);						// salto:
	if (src_reg!=HOST_r1)
		MOV32RtoR(HOST_r1,src_reg);
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
#ifdef REC_USE_DIRECT_MEM
	MOV32RtoR(HOST_r2,HOST_r11);
#endif
	CALLFunc((u32)MEMWRITE16_FUNC);				//  bl MEMWRITE16_FUNC
	armSetJ32(j32Ptr[10]);						// fin:
	r2_is_dirty=1;
}
#else

static unsigned func_MemWrite16_ptr=0;

static void recMemWrite16(void) {
	MOV32MtoR_regs(HOST_ip,&psxRegs.psxM);		// ip=psxM
	write32(BIC_IMM(HOST_r3,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_r3,0x02,0xa)); 		//  cmp r3, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	if (rec_secure_writes) {
		j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
		write32(STRH_REG(HOST_r1,HOST_r2,HOST_ip));	//  strh r1, [r2, ip]
		write32(BIC_IMM(HOST_r0,HOST_r0,3,0));  // bic r0, r0, #0
		write32(MOV_REG_LSR_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsr #16
		MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.reserved); // r2=psxRecLUT
		write32(LDR_REG_LSL(HOST_r2, HOST_r2, HOST_ip, 2));// ldr r2, [r2, ip, lsl #2]
		write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsl #16
		write32(MOV_REG_LSR_IMM(HOST_r0,HOST_ip,16));// mov r0, ip, lsr #16
		write32(MOV_IMM(HOST_ip,0,0));          // mov ip, #0
		write32(STR_REG(HOST_ip,HOST_r0,HOST_r2));// str ip, [r0, r2]
		write32(BX_LR());			//  bx lr
		armSetJ32(j32Ptr[9]);			// salto:
	} else {
		write32(STRCCH_REG(HOST_r1,HOST_r2,HOST_ip));   // strcch r0, [r2, r1]
		write32(BXCC_LR());				// bxcc lr
	}
#ifdef REC_USE_DIRECT_MEM
	func_MemWrite16_direct_ptr=(unsigned)armPtr;
	recMemWrite16_direct();
#else
	JUMPFunc((u32)MEMWRITE16_FUNC);                 //  b MEMWRITE16_FUNC
	UpdateImmediate(0);
#endif
}

INLINE void PSXMEMWRITE16(unsigned src_reg){
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMWRITE16");
#endif 
	if (autobias) cycles_pending+=2;
	iPutCyclesAdd(0);
	has_been_written=1;
	if (src_reg!=HOST_r1)
		MOV32RtoR(HOST_r1,src_reg);
	CALLFunc(func_MemWrite16_ptr);
	r2_is_dirty=1;
}
#endif

#if !defined(REC_USE_MEMORY_FUNCS) || defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES) 
INLINE void PSXMEMWRITE32(unsigned src_reg){
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMWRITE32");
#endif
	if (autobias) cycles_pending+=2;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
	has_been_written=1;
	MOV32MtoR_regs(HOST_ip,&psxRegs.psxM);		// ip=psxM
	write32(BIC_IMM(HOST_lr,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_lr,0x02,0xa)); 		//  cmp r3, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
	write32(STR_REG(src_reg,HOST_r2,HOST_ip));	//  str r1, [r2, ip]
	if (rec_secure_writes) {
		write32(MOV_REG_LSR_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsr #16
		MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.reserved); // r2=psxRecLUT
		write32(LDR_REG_LSL(HOST_r2, HOST_r2, HOST_ip, 2));// ldr r2, [r2, ip, lsl #2]
		write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsl #16
		write32(MOV_REG_LSR_IMM(HOST_r0,HOST_ip,16));// mov r0, ip, lsr #16
		write32(MOV_IMM(HOST_ip,0,0));          // mov ip, #0
		write32(STR_REG(HOST_ip,HOST_r0,HOST_r2));// str ip, [r0, r2]
	}
	j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
	UpdateImmediate(0);
	armSetJ32(j32Ptr[9]);						// salto:
	if (src_reg!=HOST_r1)
		MOV32RtoR(HOST_r1,src_reg);
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	iPutCyclesAdd(0);
#endif
#ifdef REC_USE_DIRECT_MEM
	MOV32RtoR(HOST_r2,HOST_r11);
#endif
	CALLFunc((u32)MEMWRITE32_FUNC);				//  bl MEMWRITE32_FUNC
	armSetJ32(j32Ptr[10]);						// fin:
	r2_is_dirty=1;
}
#else

static unsigned func_MemWrite32_ptr=0;

static void recMemWrite32(void) {
	MOV32MtoR_regs(HOST_ip,&psxRegs.psxM);		// ip=psxM
	write32(BIC_IMM(HOST_r3,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
	write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
	write32(CMP_IMM(HOST_r3,0x02,0xa)); 		//  cmp r3, #0x800000
	write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
	if (rec_secure_writes) {
		j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
		write32(STR_REG(HOST_r1,HOST_r2,HOST_ip));	//  str r1, [r2, ip]
		write32(MOV_REG_LSR_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsr #16
		MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.reserved); // r2=psxRecLUT
		write32(LDR_REG_LSL(HOST_r2, HOST_r2, HOST_ip, 2));// ldr r2, [r2, ip, lsl #2]
		write32(MOV_REG_LSL_IMM(HOST_ip,HOST_r0,16));// mov ip, r0, lsl #16
		write32(MOV_REG_LSR_IMM(HOST_r0,HOST_ip,16));// mov r0, ip, lsr #16
		write32(MOV_IMM(HOST_ip,0,0));          // mov ip, #0
		write32(STR_REG(HOST_ip,HOST_r0,HOST_r2));// str ip, [r0, r2]
		write32(BX_LR());			//  bx lr
		armSetJ32(j32Ptr[9]);			// salto:
	} else {
		write32(STRCC_REG(HOST_r1,HOST_r2,HOST_ip));   // strcc r0, [r2, r1]
		write32(BXCC_LR());				// bxcc lr
	}
#ifdef REC_USE_DIRECT_MEM
	func_MemWrite32_direct_ptr=(unsigned)armPtr;
	recMemWrite32_direct();
#else
	JUMPFunc((u32)MEMWRITE32_FUNC);                 //  b MEMWRITE32_FUNC
	UpdateImmediate(0);
#endif

}

INLINE void PSXMEMWRITE32(unsigned src_reg){
#ifdef DEBUG_CPU
	dbg("\t\tPSXMEMWRITE32");
#endif 
	if (autobias) cycles_pending+=2;
	iPutCyclesAdd(0);
	has_been_written=1;
	if (src_reg!=HOST_r1)
		MOV32RtoR(HOST_r1,src_reg);
	CALLFunc(func_MemWrite32_ptr);
	r2_is_dirty=1;
}
#endif

/* Push OfB for Stores/Loads */
static void iPushOfB() {
	if (IsConst(_Rs_)) {
		MOV32ItoR(HOST_r0,iRegs[_Rs_].k + _Imm_);
	} else {
// CHUI: Hacemos MOV y ADD en el mismo opcode
		MOVADD32ItoR(HOST_r0, ReadReg(_Rs_), _Imm_);
	}
}

//REC_FUNC(LB);
static void recLB() {
// Rt = mem[Rs + Im] (signed)
	if (!rec_phase) {
		if (autobias) cycles_pending+=1;
		if (IsConst(_Rs_)) {
			u32 addr = iRegs[_Rs_].k + _Imm_;
			int t = addr >> 16;
			if ((t & 0xfff0) == 0xbfc0) {
				if (_Rt_) {
					MapConst(_Rt_, psxRs8(addr)); // since bios is readonly it won't change
				}
				return;
			}
		} else {
			ReadReg(_Rs_);
		}
		if (_Rt_) {
			WriteReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecLB R%i = mem[R%i + %i]\n",_Rt_,_Rs_,_Imm_);
#endif
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0) == 0xbfc0) {
			if (_Rt_) {
				MapConst(_Rt_, psxRs8(addr)); // since bios is readonly it won't change
			}
			return;
		}
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOVSX32M8toR(rt, (u32)&psxM[addr & 0x1fffff]);
			}
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOVSX32M8toR(rt, (u32)&psxH[addr & 0xfff]);
			}
			return;
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			if (autobias) cycles_pending+=1;
			iPutCyclesAdd(0);
			MOV32ItoR(HOST_r0,addr);
			iLockReg(3);
			CALLFunc((u32)psxHwRead8);
			iUnlockReg(3);
			r2_is_dirty=1;
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 24) ); // mov reg, reg, lsl #24
				write32( MOV_REG_ASR_IMM(rt, HOST_r0, 24) ); // mov reg, reg, asr #24
			}
			return;
		}
	}

// CHUI: Buscamos si hay otros LB tras este
	unsigned bcode=psxRegs.code;
	unsigned bpc=pc;
	unsigned n=0;
	unsigned immbase=_Imm_;
	unsigned imm[REC_MAX_RWLOOP+1];
	unsigned rt[REC_MAX_RWLOOP+1];
	unsigned rs=_Rs_;
	while((psxRegs.code>>26)==32 && rs==_Rs_ && _Rt_ && _Rs_!=_Rt_ && n<REC_MAX_RWLOOP && !branch) { // Mientras sea LB y el mismo registro fuente
		if (_Imm_<immbase) {
			if (immbase-_Imm_>REC_MAX_RWLOOP_SIZE) break;
		} else {
			if (_Imm_-immbase>REC_MAX_RWLOOP_SIZE) break;
		}
#ifdef REC_USE_2ND_PHASE
		if (recPrev[rec_total_opcodes+rec_opcodes+n].jumped) break;
#endif
		imm[n]=_Imm_;
		rt[n]=_Rt_;
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		pc += 4;
		n++;
	}
	psxRegs.code=bcode;
	pc=bpc;

	iLockReg(3);
	iPushOfB();
	
	if (n<=1) {
			if (_Rt_) {
				PSXMEMREAD8();
				u32 rrt=WriteReg(_Rt_);
				write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 24) ); // mov reg, reg, lsl #24
				write32( MOV_REG_ASR_IMM(rrt, HOST_r0, 24) ); // mov reg, reg, asr #24
			} else {
				PSXHWREAD8();
			}
	} else {
			if (autobias) cycles_pending+=n;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
			iPutCyclesAdd(0);
#endif
			unsigned i;
			unsigned dest_reg0=WriteReg(_Rt_);
			unsigned dest_reg;
			MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
			write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
			write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
			write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp r3, #0x800000
			write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
			j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
			write32(LDRB_REG(HOST_ip,HOST_r2,HOST_r1));	//  ldrb r0, [r2, r1]
			write32( MOV_REG_LSL_IMM(HOST_ip, HOST_ip, 24) ); // mov reg, reg, lsl #24
			write32( MOV_REG_ASR_IMM(dest_reg0, HOST_ip, 24) ); // mov reg, reg, asr #24
			memcpy(iRegsS, iRegs, sizeof(iRegs));		// Guardamos el contexto
			memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
			for(i=1;i<n;i++) {
				int diff=imm[i]-imm[i-1];
				if (diff>0) {
					ADD32ItoR(HOST_r2,diff);
				} else if (diff<0) {
					SUB32ItoR(HOST_r2,-diff);
				}
				if (!IsConst(_Rs_)) ReadReg(_Rs_);
				dest_reg=WriteReg(rt[i]);
				write32(LDRB_REG(HOST_ip,HOST_r2,HOST_r1));	//  ldrb r0, [r2, r1]
				write32( MOV_REG_LSL_IMM(HOST_ip, HOST_ip, 24) ); // mov reg, reg, lsl #24
				write32( MOV_REG_ASR_IMM(dest_reg, HOST_ip, 24) ); // mov reg, reg, asr #24
			}
			memcpy(iRegs, iRegsS, sizeof(iRegs));		// Recuperamos el contexto
			memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
			j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
			UpdateImmediate(0);
			armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
			iPutCyclesAdd(0);
#endif
#ifndef REC_USE_MEMORY_FUNCS
			MOV32RtoR(HOST_r1,HOST_r11);
#endif
			CALLFunc((u32)MEMREAD8_FUNC);			//  bl MEMREAD8_FUNC
			write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 24) ); // mov reg, reg, lsl #24
			write32( MOV_REG_ASR_IMM(dest_reg0, HOST_r0, 24) ); // mov reg, reg, asr #24
			for(i=1;i<n;i++) {
				int diff=imm[i]-imm[0];
				if (IsConst(_Rs_)) {
					MOV32ItoR(HOST_r0,iRegs[_Rs_].k + imm[i]);
				} else {
					MOVADD32ItoR(HOST_r0, ReadReg(_Rs_), imm[i]);
				}
				dest_reg=WriteReg(rt[i]);
#ifndef REC_USE_MEMORY_FUNCS
				MOV32RtoR(HOST_r1,HOST_r11);
#endif
				CALLFunc((u32)MEMREAD8_FUNC);			//  bl MEMREAD8_FUNC
				write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 24) ); // mov reg, reg, lsl #24
				write32( MOV_REG_ASR_IMM(dest_reg, HOST_r0, 24) ); // mov reg, reg, asr #24
			}					
			armSetJ32(j32Ptr[10]);				// fin:
			pc+=(n-1)*4;
			rec_opcodes=n;
	}
	iUnlockReg(3);
	r2_is_dirty=1;
}

//REC_FUNC(LBU);
static void recLBU() {
// Rt = mem[Rs + Im] (unsigned)
	if (!rec_phase) {
		if (autobias) cycles_pending+=1;
		if (IsConst(_Rs_)) {
			u32 addr = iRegs[_Rs_].k + _Imm_;
			int t = addr >> 16;
			if ((t & 0xfff0) == 0xbfc0) {
				if (_Rt_) {
					MapConst(_Rt_, psxRu8(addr)); // since bios is readonly it won't change
				}
				return;
			}
		} else {
			ReadReg(_Rs_);
		}
		if (_Rt_) {
			WriteReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecLBU R%i = mem[R%i + %i]\n",_Rt_,_Rs_,_Imm_);
#endif
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0) == 0xbfc0) {
			if (_Rt_) {
				MapConst(_Rt_, psxRu8(addr)); // since bios is readonly it won't change
			}
			return;
		}
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOVZX32M8toR(rt, (u32)&psxM[addr & 0x1fffff]);
			}
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOVZX32M8toR(rt, (u32)&psxH[addr & 0xfff]);
			}
			return;
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			if (autobias) cycles_pending+=1;
			iPutCyclesAdd(0);
			MOV32ItoR(HOST_r0,addr);
			iLockReg(3);
			CALLFunc((u32)psxHwRead8);
			iUnlockReg(3);
			r2_is_dirty=1;
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				write32( AND_IMM(rt, HOST_r0, 0xff, 0) );      // and reg, reg, #0xff
			}
			return;
		}
	}

// CHUI: Buscamos si hay otros LBU tras este
	unsigned bcode=psxRegs.code;
	unsigned bpc=pc;
	unsigned n=0;
	unsigned immbase=_Imm_;
	unsigned imm[REC_MAX_RWLOOP+1];
	unsigned rt[REC_MAX_RWLOOP+1];
	unsigned rs=_Rs_;
	while((psxRegs.code>>26)==36 && rs==_Rs_ && _Rt_ && _Rs_!=_Rt_ && n<REC_MAX_RWLOOP && !branch) { // Mientras sea LW y el mismo registro fuente
		if (_Imm_<immbase) {
			if (immbase-_Imm_>REC_MAX_RWLOOP_SIZE) break;
		} else {
			if (_Imm_-immbase>REC_MAX_RWLOOP_SIZE) break;
		}
#ifdef REC_USE_2ND_PHASE
		if (recPrev[rec_total_opcodes+rec_opcodes+n].jumped) break;
#endif
		imm[n]=_Imm_;
		rt[n]=_Rt_;
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		pc += 4;
		n++;
	}
	psxRegs.code=bcode;
	pc=bpc;

	iLockReg(3);
	iPushOfB();

	if (n<=1) {
			if (_Rt_) {
				PSXMEMREAD8();
				u32 rrt=WriteReg(_Rt_);
				write32( AND_IMM(rrt, HOST_r0, 0xff, 0) );      // and reg, reg, #0xff
			} else {
				PSXHWREAD8();
			}
	} else {
			if (autobias) cycles_pending+=n;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
			iPutCyclesAdd(0);
#endif
			unsigned i;
			unsigned dest_reg0=WriteReg(_Rt_);
			unsigned dest_reg;
			MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
			write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
			write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
			write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp r3, #0x800000
			write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
			j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
			write32(LDRB_REG(HOST_ip,HOST_r2,HOST_r1));	//  ldrb r0, [r2, r1]
			write32( AND_IMM(dest_reg0, HOST_ip, 0xff, 0) );      // and reg, reg, #0xff
			memcpy(iRegsS, iRegs, sizeof(iRegs));		// Guardamos el contexto
			memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
			for(i=1;i<n;i++) {
				int diff=imm[i]-imm[i-1];
				if (diff>0) {
					ADD32ItoR(HOST_r2,diff);
				} else if (diff<0) {
					SUB32ItoR(HOST_r2,-diff);
				}
				if (!IsConst(_Rs_)) ReadReg(_Rs_);
				dest_reg=WriteReg(rt[i]);
				write32(LDRB_REG(HOST_ip,HOST_r2,HOST_r1));	//  ldrb r0, [r2, r1]
				write32( AND_IMM(dest_reg, HOST_ip, 0xff, 0) );      // and reg, reg, #0xff
			}
			memcpy(iRegs, iRegsS, sizeof(iRegs));		// Recuperamos el contexto
			memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
			j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
			UpdateImmediate(0);
			armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
			iPutCyclesAdd(0);
#endif
#ifndef REC_USE_MEMORY_FUNCS
			MOV32RtoR(HOST_r1,HOST_r11);
#endif
			CALLFunc((u32)MEMREAD8_FUNC);			//  bl MEMREAD8_FUNC
			write32( AND_IMM(dest_reg0, HOST_r0, 0xff, 0) );      // and reg, reg, #0xff

			for(i=1;i<n;i++) {
				int diff=imm[i]-imm[0];
				if (IsConst(_Rs_)) {
					MOV32ItoR(HOST_r0,iRegs[_Rs_].k + imm[i]);
				} else {
					MOVADD32ItoR(HOST_r0, ReadReg(_Rs_), imm[i]);
				}
				dest_reg=WriteReg(rt[i]);
#ifndef REC_USE_MEMORY_FUNCS
				MOV32RtoR(HOST_r1,HOST_r11);
#endif
				CALLFunc((u32)MEMREAD8_FUNC);			//  bl MEMREAD8_FUNC
				write32( AND_IMM(dest_reg, HOST_r0, 0xff, 0) );      // and reg, reg, #0xff
			}					
			armSetJ32(j32Ptr[10]);				// fin:
			pc+=(n-1)*4;
			rec_opcodes=n;
	}
	iUnlockReg(3);
	r2_is_dirty=1;
}

//REC_FUNC(LH);
static void recLH() {
// Rt = mem[Rs + Im] (signed)
	if (!rec_phase) {
		if (autobias) cycles_pending+=1;
		if (IsConst(_Rs_)) {
			u32 addr = iRegs[_Rs_].k + _Imm_;
			int t = addr >> 16;
			if ((t & 0xfff0) == 0xbfc0) {
				if (_Rt_) {
					MapConst(_Rt_, psxRs16(addr)); // since bios is readonly it won't change
				}
				return;
			}
		} else {
			ReadReg(_Rs_);
		}
		if (_Rt_) {
			WriteReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecLH R%i = mem[R%i + %i]\n",_Rt_,_Rs_,_Imm_);
#endif
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0) == 0xbfc0) {
			if (_Rt_) {
				MapConst(_Rt_, psxRs16(addr)); // since bios is readonly it won't change
			}
			return;
		}
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOVSX32M16toR(rt, (u32)&psxM[addr & 0x1fffff]);
			}
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOVSX32M16toR(rt, (u32)&psxH[addr & 0xfff]);
			}
			return;
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			if (autobias) cycles_pending+=1;
			iPutCyclesAdd(0);
			MOV32ItoR(HOST_r0,addr);
			iLockReg(3);
			CALLFunc((u32)psxHwRead16);
			iUnlockReg(3);
			r2_is_dirty=1;
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) );      // mov reg, reg, lsl #16
				write32( MOV_REG_ASR_IMM(rt, HOST_r0, 16) );      // mov reg, reg, asr #16
			}
			return;
		}
	}

// CHUI: Buscamos si hay otros LH tras este
	unsigned bcode=psxRegs.code;
	unsigned bpc=pc;
	unsigned n=0;
	unsigned immbase=_Imm_;
	unsigned imm[REC_MAX_RWLOOP+1];
	unsigned rt[REC_MAX_RWLOOP+1];
	unsigned rs=_Rs_;
	while((psxRegs.code>>26)==33 && rs==_Rs_ && _Rt_ && _Rs_!=_Rt_ && n<REC_MAX_RWLOOP && !branch) { // Mientras sea LH y el mismo registro fuente
		if (_Imm_<immbase) {
			if (immbase-_Imm_>REC_MAX_RWLOOP_SIZE) break;
		} else {
			if (_Imm_-immbase>REC_MAX_RWLOOP_SIZE) break;
		}
#ifdef REC_USE_2ND_PHASE
		if (recPrev[rec_total_opcodes+rec_opcodes+n].jumped) break;
#endif
		imm[n]=_Imm_;
		rt[n]=_Rt_;
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		pc += 4;
		n++;
	}
	psxRegs.code=bcode;
	pc=bpc;

	iLockReg(3);
	iPushOfB();
	
	if (n<=1) {
			if (_Rt_) {
				PSXMEMREAD16();
				u32 rrt=WriteReg(_Rt_);
				write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) ); // mov reg, reg, lsl #16
				write32( MOV_REG_ASR_IMM(rrt, HOST_r0, 16) ); // mov reg, reg, asr #16
			} else {
				PSXHWREAD16();
			}
	} else {
			if (autobias) cycles_pending+=n;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
			iPutCyclesAdd(0);
#endif
			unsigned i;
			unsigned dest_reg0=WriteReg(_Rt_);
			unsigned dest_reg;
			MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
			write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
			write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
			write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp r3, #0x800000
			write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
			j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
			write32(LDRH_REG(HOST_ip,HOST_r2,HOST_r1));	//  ldrh r0, [r2, r1]
			write32( MOV_REG_LSL_IMM(HOST_ip, HOST_ip, 16) ); // mov reg, reg, lsl #16
			write32( MOV_REG_ASR_IMM(dest_reg0, HOST_ip, 16) ); // mov reg, reg, asr #16
			memcpy(iRegsS, iRegs, sizeof(iRegs));		// Guardamos el contexto
			memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
			for(i=1;i<n;i++) {
				int diff=imm[i]-imm[i-1];
				if (diff>0) {
					ADD32ItoR(HOST_r2,diff);
				} else if (diff<0) {
					SUB32ItoR(HOST_r2,-diff);
				}
				if (!IsConst(_Rs_)) ReadReg(_Rs_);
				dest_reg=WriteReg(rt[i]);
				write32(LDRH_REG(HOST_ip,HOST_r2,HOST_r1));	//  ldrh r0, [r2, r1]
				write32( MOV_REG_LSL_IMM(HOST_ip, HOST_ip, 16) ); // mov reg, reg, lsl #16
				write32( MOV_REG_ASR_IMM(dest_reg, HOST_ip, 16) ); // mov reg, reg, asr #16
			}
			memcpy(iRegs, iRegsS, sizeof(iRegs));		// Recuperamos el contexto
			memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
			j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
			UpdateImmediate(0);
			armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
			iPutCyclesAdd(0);
#endif
#ifndef REC_USE_MEMORY_FUNCS
			MOV32RtoR(HOST_r1,HOST_r11);
#endif
			CALLFunc((u32)MEMREAD16_FUNC);			//  bl MEMREAD16_FUNC
			write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) ); // mov reg, reg, lsl #16
			write32( MOV_REG_ASR_IMM(dest_reg0, HOST_r0, 16) ); // mov reg, reg, asr #16
			for(i=1;i<n;i++) {
				int diff=imm[i]-imm[0];
				if (IsConst(_Rs_)) {
					MOV32ItoR(HOST_r0,iRegs[_Rs_].k + imm[i]);
				} else {
					MOVADD32ItoR(HOST_r0, ReadReg(_Rs_), imm[i]);
				}
				dest_reg=WriteReg(rt[i]);
#ifndef REC_USE_MEMORY_FUNCS
				MOV32RtoR(HOST_r1,HOST_r11);
#endif
				CALLFunc((u32)MEMREAD16_FUNC);			//  bl MEMREAD16_FUNC
				write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) ); // mov reg, reg, lsl #16
				write32( MOV_REG_ASR_IMM(dest_reg, HOST_r0, 16) ); // mov reg, reg, asr #16
			}					
			armSetJ32(j32Ptr[10]);				// fin:
			pc+=(n-1)*4;
			rec_opcodes=n;
	}
	iUnlockReg(3);
	r2_is_dirty=1;
}

//REC_FUNC(LHU);
static void recLHU() {
// Rt = mem[Rs + Im] (unsigned)
	if (!rec_phase) {
		if (autobias) cycles_pending+=1;
		if (IsConst(_Rs_)) {
			u32 addr = iRegs[_Rs_].k + _Imm_;
			int t = addr >> 16;
			if ((t & 0xfff0) == 0xbfc0) {
				if (_Rt_) {
					MapConst(_Rt_, psxRu16(addr)); // since bios is readonly it won't change
				}
				return;
			}
		} else {
			ReadReg(_Rs_);
		}
		if (_Rt_) {
			WriteReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecLHU R%i = mem[R%i + %i]\n",_Rt_,_Rs_,_Imm_);
#endif
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0) == 0xbfc0) {
			if (_Rt_) {
				MapConst(_Rt_, psxRu16(addr)); // since bios is readonly it won't change
			}
			return;
		}
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOVZX32M16toR(rt, (u32)&psxM[addr & 0x1fffff]);
			}
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOVZX32M16toR(rt, (u32)&psxH[addr & 0xfff]);
			}
			return;
		}
		if (t == 0x1f80) {
			if (addr >= 0x1f801c00 && addr < 0x1f801e00) {
				if (_Rt_) {
					MOV32ItoR(HOST_r0, addr);
					iLockReg(3);
					CALLFunc((u32)&SPU_readRegister);
					iUnlockReg(3);
					r2_is_dirty=1;
					u32 rt=WriteReg(_Rt_);
					write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) );      // mov reg, reg, lsl #16
					write32( MOV_REG_LSR_IMM(rt, HOST_r0, 16) );      // mov reg, reg, lsr #16
				}
				return;
			}
			switch (addr) {
				case 0x1f801100: case 0x1f801110: case 0x1f801120:
					if (_Rt_) {
						if (autobias) cycles_pending+=1;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
						iPutCyclesAdd(0);
#else
						ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
						cycles_pending=0;
						pcold=pc;
#endif
						MOV32ItoR(HOST_r0, (addr >> 4) & 0x3);
						iLockReg(3);
						CALLFunc((u32)psxRcntRcount);
						iUnlockReg(3);
						r2_is_dirty=1;
						u32 rt=WriteReg(_Rt_);
						write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) );      // mov reg, reg, lsl #16
						write32( MOV_REG_LSR_IMM(rt, HOST_r0, 16) );      // mov reg, reg, lsr #16
					}
					return;

				case 0x1f801104: case 0x1f801114: case 0x1f801124:
					if (_Rt_) {
						if (autobias) cycles_pending+=1;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
						iPutCyclesAdd(0);
#else
						ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
						cycles_pending=0;
						pcold=pc;
#endif
						MOV32ItoR(HOST_r0, (addr >> 4) & 0x3);
						iLockReg(3);
						CALLFunc((u32)psxRcntRmode);
						iUnlockReg(3);
						r2_is_dirty=1;
						u32 rt=WriteReg(_Rt_);
						write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) );      // mov reg, reg, lsl #16
						write32( MOV_REG_LSR_IMM(rt, HOST_r0, 16) );      // mov reg, reg, lsr #16
					}
					return;

				case 0x1f801108: case 0x1f801118: case 0x1f801128:
					if (_Rt_) {
						if (autobias) cycles_pending+=1;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
						iPutCyclesAdd(0);
#else
						ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
						cycles_pending=0;
						pcold=pc;
#endif
						MOV32ItoR(HOST_r0, (addr >> 4) & 0x3);
						iLockReg(3);
						CALLFunc((u32)psxRcntRtarget);
						iUnlockReg(3);
						r2_is_dirty=1;
						u32 rt=WriteReg(_Rt_);
						write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) );      // mov reg, reg, lsl #16
						write32( MOV_REG_LSR_IMM(rt, HOST_r0, 16) );      // mov reg, reg, lsr #16
					}
					return;
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			if (autobias) cycles_pending+=1;
			iPutCyclesAdd(0);
			MOV32ItoR(HOST_r0,addr);
			iLockReg(3);
			CALLFunc((u32)psxHwRead16);
			iUnlockReg(3);
			r2_is_dirty=1;
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) );      // mov reg, reg, lsl #16
				write32( MOV_REG_LSR_IMM(rt, HOST_r0, 16) );      // mov reg, reg, lsr #16
			}
			return;
		}
	}

// CHUI: Buscamos si hay otros LHU tras este
	unsigned bcode=psxRegs.code;
	unsigned bpc=pc;
	unsigned n=0;
	unsigned immbase=_Imm_;
	unsigned imm[REC_MAX_RWLOOP+1];
	unsigned rt[REC_MAX_RWLOOP+1];
	unsigned rs=_Rs_;
	while((psxRegs.code>>26)==37 && rs==_Rs_ && _Rt_ && _Rs_!=_Rt_ && n<REC_MAX_RWLOOP && !branch) { // Mientras sea LHU y el mismo registro fuente
		if (_Imm_<immbase) {
			if (immbase-_Imm_>REC_MAX_RWLOOP_SIZE) break;
		} else {
			if (_Imm_-immbase>REC_MAX_RWLOOP_SIZE) break;
		}
#ifdef REC_USE_2ND_PHASE
		if (recPrev[rec_total_opcodes+rec_opcodes+n].jumped) break;
#endif
		imm[n]=_Imm_;
		rt[n]=_Rt_;
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		pc += 4;
		n++;
	}
	psxRegs.code=bcode;
	pc=bpc;
	
	iLockReg(3);
	iPushOfB();
	
	if (n<=1) {
			if (_Rt_) {
				PSXMEMREAD16();
				u32 rrt=WriteReg(_Rt_);
				write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) ); // mov reg, reg, lsl #16
				write32( MOV_REG_LSR_IMM(rrt, HOST_r0, 16) ); // mov reg, reg, lsr #16
			} else {
				PSXHWREAD16();
			}
	} else {
			if (autobias) cycles_pending+=n;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
			iPutCyclesAdd(0);
#endif
			unsigned i;
			unsigned dest_reg0=WriteReg(_Rt_);
			unsigned dest_reg;
			MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
			write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
			write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
			write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp r3, #0x800000
			write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
			j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
			write32(LDRH_REG(HOST_ip,HOST_r2,HOST_r1));	//  ldrh r0, [r2, r1]
			write32( MOV_REG_LSL_IMM(HOST_ip, HOST_ip, 16) ); // mov reg, reg, lsl #16
			write32( MOV_REG_LSR_IMM(dest_reg0, HOST_ip, 16) ); // mov reg, reg, lsr #16
			memcpy(iRegsS, iRegs, sizeof(iRegs));		// Guardamos el contexto
			memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
			for(i=1;i<n;i++) {
				int diff=imm[i]-imm[i-1];
				if (diff>0) {
					ADD32ItoR(HOST_r2,diff);
				} else if (diff<0) {
					SUB32ItoR(HOST_r2,-diff);
				}
				if (!IsConst(_Rs_)) ReadReg(_Rs_);
				dest_reg=WriteReg(rt[i]);
				write32(LDRH_REG(HOST_ip,HOST_r2,HOST_r1));	//  ldrh r0, [r2, r1]
				write32( MOV_REG_LSL_IMM(HOST_ip, HOST_ip, 16) ); // mov reg, reg, lsl #16
				write32( MOV_REG_LSR_IMM(dest_reg, HOST_ip, 16) ); // mov reg, reg, lsr #16
			}
			memcpy(iRegs, iRegsS, sizeof(iRegs));		// Recuperamos el contexto
			memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
			j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
			UpdateImmediate(0);
			armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
			iPutCyclesAdd(0);
#endif
#ifndef REC_USE_MEMORY_FUNCS
			MOV32RtoR(HOST_r1,HOST_r11);
#endif
			CALLFunc((u32)MEMREAD16_FUNC);			//  bl MEMREAD16_FUNC
			write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) ); // mov reg, reg, lsl #16
			write32( MOV_REG_LSR_IMM(dest_reg0, HOST_r0, 16) ); // mov reg, reg, lsr #16
			for(i=1;i<n;i++) {
				int diff=imm[i]-imm[0];
				if (IsConst(_Rs_)) {
					MOV32ItoR(HOST_r0,iRegs[_Rs_].k + imm[i]);
				} else {
					MOVADD32ItoR(HOST_r0, ReadReg(_Rs_), imm[i]);
				}
				dest_reg=WriteReg(rt[i]);
#ifndef REC_USE_MEMORY_FUNCS
				MOV32RtoR(HOST_r1,HOST_r11);
#endif
				CALLFunc((u32)MEMREAD16_FUNC);			//  bl MEMREAD16_FUNC
				write32( MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 16) ); // mov reg, reg, lsl #16
				write32( MOV_REG_LSR_IMM(dest_reg, HOST_r0, 16) ); // mov reg, reg, lsr #16
			}					
			armSetJ32(j32Ptr[10]);				// fin:
			pc+=(n-1)*4;
			rec_opcodes=n;			
	}
	iUnlockReg(3);
	r2_is_dirty=1;
}


//REC_FUNC(LW);
static void recLW() {
	if (!rec_phase) {
		if (autobias) cycles_pending+=1;
		if (IsConst(_Rs_)) {
			u32 addr = iRegs[_Rs_].k + _Imm_;
			int t = addr >> 16;
			if ((t & 0xfff0) == 0xbfc0) {
				if (_Rt_) {
					MapConst(_Rt_, psxRu32(addr)); // since bios is readonly it won't change
				}
				return;
			}
		} else {
			ReadReg(_Rs_);
		}
		if (_Rt_) {
			WriteReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecLW R%i = mem[R%i + %i]\n",_Rt_,_Rs_,_Imm_);
#endif
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0xfff0) == 0xbfc0) {
			if (_Rt_) {
				MapConst(_Rt_, psxRu32(addr)); // since bios is readonly it won't change
			}
			return;
		}
		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (_Rt_) {
#if defined(REC_USE_NO_REPEAT_READ_WRITE) && !defined(DEBUG_CPU_OPCODES)
				if ((repeat_read_write_pc+4==pc)&&(repeat_read_write_imm==_Imm_)&&(repeat_read_write_reg_src==_Rs_)) {
					if (_Rt_ && _Rt_!=repeat_read_write_reg_dst) {
						u32 rt=WriteReg(_Rt_);
						u32 rt_back=ReadReg(repeat_read_write_reg_dst);
						MOV32RtoR(rt,rt_back);
					}
				} else
#endif
				{
					u32 rt=WriteReg(_Rt_);
					MOV32MtoR(rt, (u32)&psxM[addr & 0x1fffff]);
				}
			}
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOV32MtoR(rt, (u32)&psxH[addr & 0xfff]);
			}
			return;
		}
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
					if (_Rt_) {
						u32 rt=WriteReg(_Rt_);
						MOV32MtoR(rt, (u32)&psxH[addr & 0xffff]);
					}
					return;

				case 0x1f801810:
					if (_Rt_) {
						iLockReg(3);
						CALLFunc((u32)&GPU_readData);
						iUnlockReg(3);
						r2_is_dirty=1;
						u32 rt=WriteReg(_Rt_);
						MOV32RtoR(rt, HOST_r0);
					}
					return;

				case 0x1f801814:
					if (_Rt_) {
						iLockReg(3);
						CALLFunc((u32)&GPU_readStatus);
						iUnlockReg(3);
						r2_is_dirty=1;
						u32 rt=WriteReg(_Rt_);
						MOV32RtoR(rt, HOST_r0);
					}
					return;
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			if (autobias) cycles_pending+=1;
			iPutCyclesAdd(0);
			MOV32ItoR(HOST_r0,addr);
			iLockReg(3);
			CALLFunc((u32)psxHwRead32);
			iUnlockReg(3);
			r2_is_dirty=1;
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOV32RtoR(rt,HOST_r0);
			}
			return;
		}
	}


// CHUI: Buscamos si hay otros LW tras este
	unsigned bcode=psxRegs.code;
	unsigned bpc=pc;
	unsigned n=0;
	unsigned immbase=_Imm_;
	unsigned imm[REC_MAX_RWLOOP+1];
	unsigned rt[REC_MAX_RWLOOP+1];
	unsigned rs=_Rs_;
	while((psxRegs.code>>26)==35 && rs==_Rs_ && _Rt_ && _Rs_!=_Rt_ && n<REC_MAX_RWLOOP && !branch) { // Mientras sea LW y el mismo registro fuente
		if (_Imm_<immbase) {
			if (immbase-_Imm_>REC_MAX_RWLOOP_SIZE) break;
		} else {
			if (_Imm_-immbase>REC_MAX_RWLOOP_SIZE) break;
		}
#ifdef REC_USE_2ND_PHASE
		if (recPrev[rec_total_opcodes+rec_opcodes+n].jumped) break;
#endif
		imm[n]=_Imm_;
		rt[n]=_Rt_;
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		pc += 4;
		n++;
	}
	psxRegs.code=bcode;
	pc=bpc;

	iLockReg(3);
	iPushOfB(); // Carga r0 con la direccion de memoria (RS+IMM)
	
	if (n<=1) {
#if defined(REC_USE_NO_REPEAT_READ_WRITE) && !defined(DEBUG_CPU_OPCODES)
			if ((repeat_read_write_pc+4==pc)&&(repeat_read_write_imm==_Imm_)&&(repeat_read_write_reg_src==_Rs_)) {
				write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic ip, r0, #0xe0000000
				write32(CMP_IMM(HOST_ip,0x02,0xa));             //  cmp ip, #0x800000
				if (_Rt_ && _Rt_!=repeat_read_write_reg_dst) {
					j32Ptr[9]=armPtr; write32(BCS_FWD(0));          //  bcs salto
					u32 rt=WriteReg(_Rt_);
					u32 rt_back=ReadReg(repeat_read_write_reg_dst);
					MOV32RtoR(rt,rt_back);
					j32Ptr[8]=armPtr; write32(B_FWD(0));
					armSetJ32(j32Ptr[9]);
				} else {
					j32Ptr[8]=armPtr; write32(BCC_FWD(0));
				}
			}
#endif
			if (_Rt_) {
				PSXMEMREAD32(WriteReg(_Rt_));
			} else {
				PSXHWREAD32();
			}
#if defined(REC_USE_NO_REPEAT_READ_WRITE) && !defined(DEBUG_CPU_OPCODES)
			if ((repeat_read_write_pc+4==pc)&&(repeat_read_write_imm==_Imm_)&&(repeat_read_write_reg_src==_Rs_)) {
				armSetJ32(j32Ptr[8]);
			}
#endif
	} else {
			if (autobias) cycles_pending+=n;
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
			iPutCyclesAdd(0);
#endif
			unsigned i;
			unsigned dest_reg0=WriteReg(_Rt_);
			unsigned dest_reg;
			MOV32MtoR_regs(HOST_r1,&psxRegs.psxM);		// r1=psxM
			write32(BIC_IMM(HOST_ip,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
			write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
			write32(CMP_IMM(HOST_ip,0x02,0xa)); 		//  cmp r3, #0x800000
			write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
			j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
			write32(LDR_REG(dest_reg0,HOST_r2,HOST_r1));	//  ldr r0, [r2, r1]
			memcpy(iRegsS, iRegs, sizeof(iRegs));		// Guardamos el contexto
			memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
			for(i=1;i<n;i++) {
				int diff=imm[i]-imm[i-1];
				if (diff>0) {
					ADD32ItoR(HOST_r2,diff);
				} else if (diff<0) {
					SUB32ItoR(HOST_r2,-diff);
				}
				if (!IsConst(_Rs_)) ReadReg(_Rs_);
				dest_reg=WriteReg(rt[i]);
				write32(LDR_REG(dest_reg,HOST_r2,HOST_r1));	//  ldr r0, [r2, r1]
			}
			memcpy(iRegs, iRegsS, sizeof(iRegs));		// Recuperamos el contexto
			memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
			j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
			UpdateImmediate(0);
			armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
			iPutCyclesAdd(0);
#endif
#ifndef REC_USE_MEMORY_FUNCS
			MOV32RtoR(HOST_r1,HOST_r11);
#endif
			CALLFunc((u32)MEMREAD32_FUNC);			//  bl MEMREAD32_FUNC
			MOV32RtoR(dest_reg0, HOST_r0);
			for(i=1;i<n;i++) {
				int diff=imm[i]-imm[0];
				if (IsConst(_Rs_)) {
					MOV32ItoR(HOST_r0,iRegs[_Rs_].k + imm[i]);
				} else {
					MOVADD32ItoR(HOST_r0, ReadReg(_Rs_), imm[i]);
				}
				dest_reg=WriteReg(rt[i]);
#ifndef REC_USE_MEMORY_FUNCS
				MOV32RtoR(HOST_r1,HOST_r11);
#endif
				CALLFunc((u32)MEMREAD32_FUNC);			//  bl MEMREAD32_FUNC
				MOV32RtoR(dest_reg, HOST_r0);
			}					
			armSetJ32(j32Ptr[10]);				// fin:
			pc+=(n-1)*4;
			rec_opcodes=n;
	}
	iUnlockReg(3);
	r2_is_dirty=1;
}

//REC_FUNC(SB);
static void recSB() {
// mem[Rs + Im] = Rt
	if (!rec_phase) {
		if (autobias) cycles_pending+=2;
		if (!IsConst(_Rs_)) {
			ReadReg(_Rs_);
		}
		if (!IsConst(_Rt_)) {
			ReadReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecSB mem[R%i + %i] = R%i\n",_Rs_,_Imm_,_Rt_);
#endif

	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (IsConst(_Rt_)) {
				MOV8ItoM((u32)&psxM[addr & 0x1fffff], (u8)iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV8RtoM((u32)&psxM[addr & 0x1fffff], rt);
			}
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (IsConst(_Rt_)) {
				MOV8ItoM((u32)&psxH[addr & 0xfff], (u8)iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV8RtoM((u32)&psxH[addr & 0xfff], rt);
			}
			return;
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			MOV32ItoR(HOST_r0,addr);
			if (IsConst(_Rt_)) {
				MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV32RtoR(HOST_r1, rt);
			}
			if (autobias) cycles_pending+=2;
			iPutCyclesAdd(0);
			iLockReg(3);
			CALLFunc((u32)psxHwWrite8);
			iUnlockReg(3);
			r2_is_dirty=1;
			return;
		}
	}

// CHUI: Buscamos si hay otros SB tras este
	unsigned bcode=psxRegs.code;
	unsigned bpc=pc;
	unsigned n=0;
	unsigned immbase=_Imm_;
	unsigned imm[REC_MAX_RWLOOP+1];
	unsigned rt[REC_MAX_RWLOOP+1];
	unsigned rs=_Rs_;
	while((psxRegs.code>>26)==40 && rs==_Rs_ && n<REC_MAX_RWLOOP && !branch) { // Mientras sea SB y el mismo registro fuente
		if (_Imm_<immbase) {
			if (immbase-_Imm_>REC_MAX_RWLOOP_SIZE) break;
		} else {
			if (_Imm_-immbase>REC_MAX_RWLOOP_SIZE) break;
		}
#ifdef REC_USE_2ND_PHASE
		if (recPrev[rec_total_opcodes+rec_opcodes+n].jumped) break;
#endif
		imm[n]=_Imm_;
		rt[n]=_Rt_;
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		pc += 4;
		n++;
	}
	psxRegs.code=bcode;
	pc=bpc;

	iLockReg(3);
	iPushOfB(); // Carga r0 con la direccion de memoria (RS+IMM)
	
	if ((n<=1)||(!Config.HLE)||(rec_secure_writes)) {
		if (IsConst(_Rt_)) {
			MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
			if (Config.HLE) {
				PSXMEMWRITE8(HOST_r1);
			} else {
				if (autobias) cycles_pending+=2;
				iPutCyclesAdd(0);
				CALLFunc((u32)psxMemWrite8);
			}
		} else {
			if (Config.HLE) {
				PSXMEMWRITE8(ReadReg(_Rt_));
			} else {
				if (autobias) cycles_pending+=2;
				iPutCyclesAdd(0);
				MOV32RtoR(HOST_r1,ReadReg(_Rt_));
				CALLFunc((u32)psxMemWrite8);
			}
		}
	} else {
		if (autobias) cycles_pending+=(n*2);
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
		iPutCyclesAdd(0);
#endif
		has_been_written=1;
		unsigned i;
		unsigned dest_reg0=HOST_r1;
		unsigned dest_reg;
		if (IsConst(rt[0])) {
			MOV32ItoR(HOST_r1, iRegs[rt[0]].k);
		} else {
			dest_reg0=ReadReg(rt[0]);
		}

		MOV32MtoR_regs(HOST_ip,&psxRegs.psxM);		// ip=psxM
		write32(BIC_IMM(HOST_lr,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
		write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
		write32(CMP_IMM(HOST_lr,0x02,0xa)); 		//  cmp r3, #0x800000
		write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
		j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
		write32(STRB_REG(dest_reg0,HOST_r2,HOST_ip));	//  strb r1, [r2, ip]
		memcpy(iRegsS, iRegs, sizeof(iRegs));		// Guardamos el contexto
		memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
		for(i=1;i<n;i++) {
			int diff=imm[i]-imm[i-1];
			if (diff>0) {
				ADD32ItoR(HOST_r2,diff);
			} else if (diff<0) {
				SUB32ItoR(HOST_r2,-diff);
			}
			if (!IsConst(_Rs_)) ReadReg(_Rs_);
			if (IsConst(rt[i])) {
				MOV32ItoR(HOST_r1, iRegs[rt[i]].k);
				dest_reg=HOST_r1;
			} else {
				dest_reg=ReadReg(rt[i]);
			}
			write32(STRB_REG(dest_reg,HOST_r2,HOST_ip));	//  strb r1, [r2, ip]
		}
		memcpy(iRegs, iRegsS, sizeof(iRegs));		// Recuperamos el contexto
		memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
		j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
		UpdateImmediate(0);
		armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
		iPutCyclesAdd(0);
#endif
		if (dest_reg0!=HOST_r1)
			MOV32RtoR(HOST_r1,dest_reg0);
#ifdef REC_USE_DIRECT_MEM
		MOV32RtoR(HOST_r2,HOST_r11);
#endif
#ifndef REC_USE_MEMORY_FUNCS
		MOV32RtoR(HOST_r2,HOST_r11);
#endif
		CALLFunc((u32)MEMWRITE8_FUNC);				//  bl MEMWRITE32_FUNC
		for(i=1;i<n;i++) {
			int diff=imm[i]-imm[0];
			if (IsConst(_Rs_)) {
				MOV32ItoR(HOST_r0,iRegs[_Rs_].k + imm[i]);
			} else {
				MOVADD32ItoR(HOST_r0, ReadReg(_Rs_), imm[i]);
			}
			if (IsConst(rt[i])) {
				MOV32ItoR(HOST_r1, iRegs[rt[i]].k);
			} else {
				MOV32RtoR(HOST_r1,ReadReg(rt[i]));
			}
#ifndef REC_USE_MEMORY_FUNCS
			MOV32RtoR(HOST_r2,HOST_r11);
#endif
			CALLFunc((u32)MEMWRITE8_FUNC);			//  bl MEMWRITE8_FUNC
		}					
		armSetJ32(j32Ptr[10]);				// fin:
		pc+=(n-1)*4;
		rec_opcodes=n;
	}
	iUnlockReg(3);
	r2_is_dirty=1;	
}

//REC_FUNC(SH);
static void recSH() {
// mem[Rs + Im] = Rt
	if (!rec_phase) {
		if (autobias) cycles_pending+=2;
		if (!IsConst(_Rs_)) {
			ReadReg(_Rs_);
		}
		if (!IsConst(_Rt_)) {
			ReadReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecSH mem[R%i + %i] = R%i\n",_Rs_,_Imm_,_Rt_);
#endif
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (IsConst(_Rt_)) {
				MOV16ItoM((u32)&psxM[addr & 0x1fffff], (u16)iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV16RtoM((u32)&psxM[addr & 0x1fffff], rt);
			}
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (IsConst(_Rt_)) {
				MOV16ItoM((u32)&psxH[addr & 0xfff], (u16)iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV16RtoM((u32)&psxH[addr & 0xfff], rt);
			}
			return;
		}
		if (t == 0x1f80) {
			if (addr >= 0x1f801c00 && addr < 0x1f801e00) {
				if (IsConst(_Rt_)) {
					MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
				} else {
					u32 rt=ReadReg(_Rt_);
					MOV32RtoR(HOST_r1, rt);
				}
				MOV32ItoR(HOST_r0, addr);
				iLockReg(3);
// XXX Dec 2016: SPU of PCSX Rearmed has been adopted as standard SPU.
//               Some SPU functions now take additional 'cycles' param and
//               no effort has been made to update old ARM dynarecs to match.
#error "ARM dynarec has not been updated to match new SPU interface of spu_pcsxrearmed"
				CALLFunc((u32)&SPU_writeRegister);
				iUnlockReg(3);
				r2_is_dirty=1;
				return;
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			MOV32ItoR(HOST_r0,addr);
			if (IsConst(_Rt_)) {
				MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV32RtoR(HOST_r1, rt);
			}
			if (autobias) cycles_pending+=2;
			iPutCyclesAdd(0);
			iLockReg(3);
			CALLFunc((u32)psxHwWrite16);
			iUnlockReg(3);
			r2_is_dirty=1;
			return;
		}
	}

// CHUI: Buscamos si hay otros SH tras este
	unsigned bcode=psxRegs.code;
	unsigned bpc=pc;
	unsigned n=0;
	unsigned immbase=_Imm_;
	unsigned imm[REC_MAX_RWLOOP+1];
	unsigned rt[REC_MAX_RWLOOP+1];
	unsigned rs=_Rs_;
	while((psxRegs.code>>26)==41 && rs==_Rs_ && n<REC_MAX_RWLOOP && !branch) { // Mientras sea SH y el mismo registro fuente
		if (_Imm_<immbase) {
			if (immbase-_Imm_>REC_MAX_RWLOOP_SIZE) break;
		} else {
			if (_Imm_-immbase>REC_MAX_RWLOOP_SIZE) break;
		}
#ifdef REC_USE_2ND_PHASE
		if (recPrev[rec_total_opcodes+rec_opcodes+n].jumped) break;
#endif
		imm[n]=_Imm_;
		rt[n]=_Rt_;
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		pc += 4;
		n++;
	}
	psxRegs.code=bcode;
	pc=bpc;

	iLockReg(3);
	iPushOfB(); // Carga r0 con la direccion de memoria (RS+IMM)
	
	if ((n<=1)||(!Config.HLE)||(rec_secure_writes)) {
		if (IsConst(_Rt_)) {
			MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
			if (Config.HLE) {
				PSXMEMWRITE16(HOST_r1);
			} else {
				if (autobias) cycles_pending+=2;
				iPutCyclesAdd(0);
				CALLFunc((u32)psxMemWrite16);
			}
		} else {
			if (Config.HLE) {
				PSXMEMWRITE16(ReadReg(_Rt_));
			} else {
				if (autobias) cycles_pending+=2;
				iPutCyclesAdd(0);
				MOV32RtoR(HOST_r1,ReadReg(_Rt_));
				CALLFunc((u32)psxMemWrite16);
			}
		}
	} else {
		if (autobias) cycles_pending+=(n*2);
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
		iPutCyclesAdd(0);
#endif
		has_been_written=1;
		unsigned i;
		unsigned dest_reg0=HOST_r1;
		unsigned dest_reg;
		if (IsConst(rt[0])) {
			MOV32ItoR(HOST_r1, iRegs[rt[0]].k);
		} else {
			dest_reg0=ReadReg(rt[0]);
		}

		MOV32MtoR_regs(HOST_ip,&psxRegs.psxM);		// ip=psxM
		write32(BIC_IMM(HOST_lr,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
		write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
		write32(CMP_IMM(HOST_lr,0x02,0xa)); 		//  cmp r3, #0x800000
		write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
		j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
		write32(STRH_REG(dest_reg0,HOST_r2,HOST_ip));	//  strh r1, [r2, ip]
		memcpy(iRegsS, iRegs, sizeof(iRegs));		// Guardamos el contexto
		memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
		for(i=1;i<n;i++) {
			int diff=imm[i]-imm[i-1];
			if (diff>0) {
				ADD32ItoR(HOST_r2,diff);
			} else if (diff<0) {
				SUB32ItoR(HOST_r2,-diff);
			}
			if (!IsConst(_Rs_)) ReadReg(_Rs_);
			if (IsConst(rt[i])) {
				MOV32ItoR(HOST_r1, iRegs[rt[i]].k);
				dest_reg=HOST_r1;
			} else {
				dest_reg=ReadReg(rt[i]);
			}
			write32(STRH_REG(dest_reg,HOST_r2,HOST_ip));	//  strh r1, [r2, ip]
		}
		memcpy(iRegs, iRegsS, sizeof(iRegs));		// Recuperamos el contexto
		memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
		j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
		UpdateImmediate(0);
		armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
		iPutCyclesAdd(0);
#endif
		if (dest_reg0!=HOST_r1)
			MOV32RtoR(HOST_r1,dest_reg0);
#ifndef REC_USE_MEMORY_FUNCS
		MOV32RtoR(HOST_r2,HOST_r11);
#endif
		CALLFunc((u32)MEMWRITE16_FUNC);				//  bl MEMWRITE16_FUNC
		for(i=1;i<n;i++) {
			int diff=imm[i]-imm[0];
			if (IsConst(_Rs_)) {
				MOV32ItoR(HOST_r0,iRegs[_Rs_].k + imm[i]);
			} else {
				MOVADD32ItoR(HOST_r0, ReadReg(_Rs_), imm[i]);
			}
			if (IsConst(rt[i])) {
				MOV32ItoR(HOST_r1, iRegs[rt[i]].k);
			} else {
				MOV32RtoR(HOST_r1,ReadReg(rt[i]));
			}
#ifndef REC_USE_MEMORY_FUNCS
			MOV32RtoR(HOST_r2,HOST_r11);
#endif
			CALLFunc((u32)MEMWRITE16_FUNC);			//  bl MEMWRITE16_FUNC
		}					
		armSetJ32(j32Ptr[10]);				// fin:
		pc+=(n-1)*4;
		rec_opcodes=n;
	}
	iUnlockReg(3);
	r2_is_dirty=1;
}

//REC_FUNC(SW);
static void recSW() {
// mem[Rs + Im] = Rt
	if (!rec_phase) {
		if (autobias) cycles_pending+=2;
		if (!IsConst(_Rs_)) {
			ReadReg(_Rs_);
		}
		if (!IsConst(_Rt_)) {
			ReadReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecSW mem[R%i + %i] = R%i\n",_Rs_,_Imm_,_Rt_);
#endif
	if (IsConst(_Rs_)) {
		u32 addr = iRegs[_Rs_].k + _Imm_;
		int t = addr >> 16;

		if ((t & 0x1fe0) == 0 && (t & 0x1fff) != 0) {
			if (IsConst(_Rt_)) {
				MOV32ItoM((u32)&psxM[addr & 0x1fffff], iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV32RtoM((u32)&psxM[addr & 0x1fffff], rt);
			}
#if defined(REC_USE_NO_REPEAT_READ_WRITE) && !defined(DEBUG_CPU_OPCODES)
			repeat_read_write_pc=pc;
			repeat_read_write_reg_dst=_Rt_;
			repeat_read_write_reg_src=_Rs_;
			repeat_read_write_imm=_Imm_;
#endif
			return;
		}
		if (t == 0x1f80 && addr < 0x1f801000) {
			if (IsConst(_Rt_)) {
				MOV32ItoM((u32)&psxH[addr & 0xfff], iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV32RtoM((u32)&psxH[addr & 0xfff], rt);
			}
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
					if (IsConst(_Rt_)) {
						MOV32ItoM((u32)&psxH[addr & 0xffff], iRegs[_Rt_].k);
					} else {
						u32 rt=ReadReg(_Rt_);
						MOV32RtoM((u32)&psxH[addr & 0xffff], rt);
					}
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
					if (IsConst(_Rt_)) {
						MOV32ItoR(HOST_r0,iRegs[_Rt_].k);
					} else {
						u32 rt=ReadReg(_Rt_);
						MOV32RtoR(HOST_r0,rt);
					}
					iLockReg(3);
					CALLFunc((u32)&GPU_writeData);
					iUnlockReg(3);
					r2_is_dirty=1;
					return;

				case 0x1f801814:
					if (IsConst(_Rt_)) {
						MOV32ItoR(HOST_r0,iRegs[_Rt_].k);
					} else {
						u32 rt=ReadReg(_Rt_);
						MOV32RtoR(HOST_r0,rt);
					}
					iLockReg(3);
					CALLFunc((u32)&GPU_writeStatus);
					iUnlockReg(3);
					r2_is_dirty=1;
					return; // ???
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			MOV32ItoR(HOST_r0,addr);
			if (IsConst(_Rt_)) {
				MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV32RtoR(HOST_r1, rt);
			}
			if (autobias) cycles_pending+=2;
			iPutCyclesAdd(0);
			iLockReg(3);
			CALLFunc((u32)psxHwWrite32);
			iUnlockReg(3);
			r2_is_dirty=1;
			return;
		}
	}

// CHUI: Buscamos si hay otros SW tras este
	unsigned bcode=psxRegs.code;
	unsigned bpc=pc;
	unsigned n=0;
	unsigned immbase=_Imm_;
	unsigned imm[REC_MAX_RWLOOP+1];
	unsigned rt[REC_MAX_RWLOOP+1];
	unsigned rs=_Rs_;
	while((psxRegs.code>>26)==43 && rs==_Rs_ && n<REC_MAX_RWLOOP && !branch) { // Mientras sea LW y el mismo registro fuente
		if (_Imm_<immbase) {
			if (immbase-_Imm_>REC_MAX_RWLOOP_SIZE) break;
		} else {
			if (_Imm_-immbase>REC_MAX_RWLOOP_SIZE) break;
		}
#ifdef REC_USE_2ND_PHASE
		if (recPrev[rec_total_opcodes+rec_opcodes+n].jumped) break;
#endif
		imm[n]=_Imm_;
		rt[n]=_Rt_;
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		pc += 4;
		n++;
	}
	psxRegs.code=bcode;
	pc=bpc;

	iLockReg(3);
	iPushOfB(); // Carga r0 con la direccion de memoria (RS+IMM)
	
	if ((n<=1)||(!Config.HLE)||(rec_secure_writes)) {
		if (IsConst(_Rt_)) {
			MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
			if (Config.HLE) {
				PSXMEMWRITE32(HOST_r1);
			} else {
				if (autobias) cycles_pending+=2;
				iPutCyclesAdd(0);
				CALLFunc((u32)psxMemWrite32);
			}
		} else {
			if (Config.HLE) {
				PSXMEMWRITE32(ReadReg(_Rt_));
			} else {
				if (autobias) cycles_pending+=2;
				iPutCyclesAdd(0);
				MOV32RtoR(HOST_r1,ReadReg(_Rt_));
				CALLFunc((u32)psxMemWrite32);
			}
		}
#if defined(REC_USE_NO_REPEAT_READ_WRITE) && !defined(DEBUG_CPU_OPCODES)
		repeat_read_write_pc=pc;
		repeat_read_write_reg_dst=_Rt_;
		repeat_read_write_reg_src=_Rs_;
		repeat_read_write_imm=_Imm_;
#endif
	} else {
		if (autobias) cycles_pending+=(n*2);
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
		iPutCyclesAdd(0);
#endif
		has_been_written=1;
		unsigned i;
		unsigned dest_reg0=HOST_r1;
		unsigned dest_reg;
		if (IsConst(rt[0])) {
			MOV32ItoR(HOST_r1, iRegs[rt[0]].k);
		} else {
			dest_reg0=ReadReg(rt[0]);
		}

		MOV32MtoR_regs(HOST_ip,&psxRegs.psxM);		// ip=psxM
		write32(BIC_IMM(HOST_lr,HOST_r0,0x0e,0x04));//  bic r3, r0, #0xe0000000
		write32(BIC_IMM(HOST_r2,HOST_r0,0x7f,0x09));//  bic r2, r0, #0xff000000
		write32(CMP_IMM(HOST_lr,0x02,0xa)); 		//  cmp r3, #0x800000
		write32(BIC_IMM(HOST_r2,HOST_r2,0x0e,0x0c));//  bic r2, r2, #0xe00000
		j32Ptr[9]=armPtr; write32(BCS_FWD(0));		//  bcs salto
		write32(STR_REG(dest_reg0,HOST_r2,HOST_ip));	//  str r1, [r2, ip]
		memcpy(iRegsS, iRegs, sizeof(iRegs));		// Guardamos el contexto
		memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
		for(i=1;i<n;i++) {
			int diff=imm[i]-imm[i-1];
			if (diff>0) {
				ADD32ItoR(HOST_r2,diff);
			} else if (diff<0) {
				SUB32ItoR(HOST_r2,-diff);
			}
			if (!IsConst(_Rs_)) ReadReg(_Rs_);
			if (IsConst(rt[i])) {
				MOV32ItoR(HOST_r1, iRegs[rt[i]].k);
				dest_reg=HOST_r1;
			} else {
				dest_reg=ReadReg(rt[i]);
			}
			write32(STR_REG(dest_reg,HOST_r2,HOST_ip));	//  str r1, [r2, ip]
		}
		memcpy(iRegs, iRegsS, sizeof(iRegs));		// Recuperamos el contexto
		memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
		j32Ptr[10]=armPtr; write32(B_FWD(0));		//  b fin
		UpdateImmediate(0);
		armSetJ32(j32Ptr[9]);				// salto:
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
		iPutCyclesAdd(0);
#endif
		if (dest_reg0!=HOST_r1)
			MOV32RtoR(HOST_r1,dest_reg0);
#ifndef REC_USE_MEMORY_FUNCS
		MOV32RtoR(HOST_r2,HOST_r11);
#endif
		CALLFunc((u32)MEMWRITE32_FUNC);				//  bl MEMWRITE32_FUNC
		for(i=1;i<n;i++) {
			int diff=imm[i]-imm[0];
			if (IsConst(_Rs_)) {
				MOV32ItoR(HOST_r0,iRegs[_Rs_].k + imm[i]);
			} else {
				MOVADD32ItoR(HOST_r0, ReadReg(_Rs_), imm[i]);
			}
			if (IsConst(rt[i])) {
				MOV32ItoR(HOST_r1, iRegs[rt[i]].k);
			} else {
				MOV32RtoR(HOST_r1,ReadReg(rt[i]));
			}
#ifndef REC_USE_MEMORY_FUNCS
			MOV32RtoR(HOST_r2,HOST_r11);
#endif
			CALLFunc((u32)MEMWRITE32_FUNC);			//  bl MEMWRITE32_FUNC
		}					
		armSetJ32(j32Ptr[10]);				// fin:
		pc+=(n-1)*4;
		rec_opcodes=n;
	}
	iUnlockReg(3);
	r2_is_dirty=1;
}


//REC_FUNC(LWL);
static void recLWL() {
// Rt = Rt Merge mem[Rs + Im]
	if (!rec_phase) {
		if (autobias) cycles_pending+=1;
		if (!IsConst(_Rs_)) {
			ReadReg(_Rs_);
		}
		if (!IsConst(_Rt_)) {
			if (_Rt_) {
				ReadWriteReg(_Rt_);
			}
		} else {
			if (_Rt_) {
				WriteReg(_Rt_);
			}
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecLWL R%i = R%i Merge mem[R%i + %i]\n",_Rt_,_Rt_,_Rs_,_Imm_);
#endif
	iLockReg(3);
	if (_Rt_) {
		iPushOfB();

		u32 temp=TempReg();
		write32(AND_IMM(temp, HOST_r0, 3, 0));
		write32(BIC_IMM(HOST_r0, HOST_r0, 3, 0));
		PSXMEMREAD32(HOST_r0);
		MOV32RtoR(HOST_r2,temp);
		ReleaseTempReg(temp);
		
		u32 rt;
		if (IsConst(_Rt_)) {
			MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
		} else {
			rt=ReadReg(_Rt_);
			MOV32RtoR(HOST_r1, rt);
		}

		write32(LDR_IMM(HOST_lr, HOST_pc, 20)); /* LWL_MASK */
		write32(0xe79ec102); /* ldr	ip, [lr, r2, lsl #2] */
		write32(LDR_IMM(HOST_lr, HOST_pc, 16)); /* LWL_SHIFT */
		write32(0xe00cc001); /* and	ip, ip, r1 */
		write32(0xe79e2102); /* ldr	r2, [lr, r2, lsl #2] */
		write32(0xe18c0210); /* orr	r0, ip, r0, lsl r2 */
		write32(B_FWD(4)); /* fin */
		disarm_immediates=1;
		/* extern u32 LWL_MASK[4]; */ write32((u32)LWL_MASK);
		/* extern u32 LWL_SHIFT[4];*/ write32((u32)LWL_SHIFT);
		disarm_immediates=0;
		
		rt=WriteReg(_Rt_);
		MOV32RtoR(rt,HOST_r0);
	}
	else
	{
		iPushOfB();
		write32(BIC_IMM(HOST_r0, HOST_r0, 3, 0));
		PSXHWREAD32();
	}
	iUnlockReg(3);
	r2_is_dirty=1;
}


//REC_FUNC(LWR);
static void recLWR() {
// Rt = Rt Merge mem[Rs + Im]
	if (!rec_phase) {
		if (autobias) cycles_pending+=1;
		if (!IsConst(_Rs_)) {
			ReadReg(_Rs_);
		}
		if (!IsConst(_Rt_)) {
			if (_Rt_) {
				ReadWriteReg(_Rt_);
			}
		} else {
			if (_Rt_) {
				WriteReg(_Rt_);
			}
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecLWR R%i = R%i Merge mem[R%i + %i]\n",_Rt_,_Rt_,_Rs_,_Imm_);
#endif
	iLockReg(3);
	if (_Rt_) {
		iPushOfB();
		u32 temp=TempReg();
		write32(AND_IMM(temp, HOST_r0, 3, 0));
		write32(BIC_IMM(HOST_r0, HOST_r0, 3, 0));
		PSXMEMREAD32(HOST_r0);
		MOV32RtoR(HOST_r2,temp);
		ReleaseTempReg(temp);

		u32 rt;
		if (IsConst(_Rt_)) {
			MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
		} else {
			rt=ReadReg(_Rt_);
			MOV32RtoR(HOST_r1, rt);
		}

		write32(LDR_IMM(HOST_lr, HOST_pc, 20)); /* LWR_MASK */
		write32(0xe79ec102); /* ldr	ip, [lr, r2, lsl #2] */
		write32(LDR_IMM(HOST_lr, HOST_pc, 16)); /* LWR_SHIFT */
		write32(0xe00cc001); /* and	ip, ip, r1 */
		write32(0xe79e2102); /* ldr	r2, [lr, r2, lsl #2] */
		write32(0xe18c0230); /* orr	r0, ip, r0, lsr r2 */
		write32(B_FWD(4)); /* fin */
		disarm_immediates=1;
		/* extern u32 LWR_MASK[4]; */ write32((u32)LWR_MASK);
		/* extern u32 LWR_SHIFT[4]; */ write32((u32)LWR_SHIFT);
		disarm_immediates=0;

		rt=WriteReg(_Rt_);
		MOV32RtoR(rt,HOST_r0);
	}
	else
	{
		iPushOfB();
		write32(BIC_IMM(HOST_r0, HOST_r0, 3, 0));
		PSXHWREAD32();
	}
	iUnlockReg(3);
	r2_is_dirty=1;
}


//REC_FUNC(SWL);
static void recSWL() {
// mem[Rs + Im] = Rt Merge mem[Rs + Im]
	if (!rec_phase) {
		if (autobias) cycles_pending+=3;
		if (!IsConst(_Rs_)) {
			ReadReg(_Rs_);
		}
		if (!IsConst(_Rt_)) {
			ReadReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecSWL mem[R%i + %i] = R%i Merge mem\n",_Rs_,_Imm_,_Rt_);
#endif
	iLockReg(3);
	iPushOfB();
	u32 temp=TempReg();
	write32(AND_IMM(temp, HOST_r0, 3, 0));
	write32(BIC_IMM(HOST_r0, HOST_r0, 3, 0));
	write32(0xe92d0001); /* stmdb	sp!, {r0} */
	PSXMEMREAD32(HOST_r0);
	MOV32RtoR(HOST_r2,temp);
	ReleaseTempReg(temp);
	
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_r1, rt);
	}

	write32(LDR_IMM(HOST_lr, HOST_pc, 20)); /* SWL_MASK */
	write32(0xe79ec102); /* ldr	ip, [lr, r2, lsl #2] */
	write32(LDR_IMM(HOST_lr, HOST_pc, 16)); /* SWL_SHIFT */
	write32(0xe00cc000); /* and	ip, ip, r0 */
	write32(0xe79e2102); /* ldr	r2, [lr, r2, lsl #2] */
	write32(0xe18c0231); /* orr	r0, ip, r1, lsr r2 */
	write32(B_FWD(4)); /* fin */
	disarm_immediates=1;
	/* extern u32 SWL_MASK[4]; */ write32((u32)SWL_MASK);
	/* extern u32 SWL_SHIFT[4]; */ write32((u32)SWL_SHIFT);
	disarm_immediates=0;
	
	MOV32RtoR(HOST_r1,HOST_r0);
	write32(0xe8bd0001); /* ldmia	sp!, {r0} */
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


//REC_FUNC(SWR);
static void recSWR() {
// mem[Rs + Im] = Rt Merge mem[Rs + Im]
	if (!rec_phase) {
		if (autobias) cycles_pending+=3;
		if (!IsConst(_Rs_)) {
			ReadReg(_Rs_);
		}
		if (!IsConst(_Rt_)) {
			ReadReg(_Rt_);
		}
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\trecSWR mem[R%i + %i] = R%i Merge mem\n",_Rs_,_Imm_,_Rt_);
#endif
	iLockReg(3);
	iPushOfB();

	u32 temp=TempReg();
	write32(AND_IMM(temp, HOST_r0, 3, 0));
	write32(BIC_IMM(HOST_r0, HOST_r0, 3, 0));
	write32(0xe92d0001); /* stmdb	sp!, {r0} */
	PSXMEMREAD32(HOST_r0);
	MOV32RtoR(HOST_r2,temp);
	ReleaseTempReg(temp);
	
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_r1, iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_r1, rt);
	}

	write32(LDR_IMM(HOST_lr, HOST_pc, 20)); /* SWR_MASK */
	write32(0xe79ec102); /* ldr	ip, [lr, r2, lsl #2] */
	write32(LDR_IMM(HOST_lr, HOST_pc, 16)); /* SWR_SHIFT */
	write32(0xe00cc000); /* and	ip, ip, r0 */
	write32(0xe79e2102); /* ldr	r1, [lr, r2, lsl #2] */
	write32(0xe18c0211); /* orr	r0, ip, r1, lsl r3 */
	write32(B_FWD(4)); /* fin */
	disarm_immediates=1;
	/* extern u32 SWR_MASK[4]; */ write32((u32)SWR_MASK);
	/* extern u32 SWR_SHIFT[4]; */ write32((u32)SWR_SHIFT);
	disarm_immediates=0;
	
	MOV32RtoR(HOST_r1,HOST_r0);
	write32(0xe8bd0001); /* ldmia	sp!, {r0} */
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
