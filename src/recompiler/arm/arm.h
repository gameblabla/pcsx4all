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

#ifndef __ARMREC_H__
#define __ARMREC_H__

// include basic types
#include "../psxcommon.h"
#include "../r3000a.h"
#include "../psxhle.h"

#ifdef __cplusplus
extern "C" {
#endif
extern void sys_cacheflush(void *start_addr, void *end_addr);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern void recRun(unsigned int ptr, unsigned int regs);
#ifdef __cplusplus
}
#endif

static u32  *armPtr;
static u32 *j32Ptr[32];

// CHUI: El maximo de immediatos
#define IMM_MAX 1024
static unsigned immPtr[IMM_MAX]; // immediate pointers
static unsigned immData[IMM_MAX];     // immediate values
static unsigned immPtrValue[IMM_MAX];	// immediate pointer to value
static unsigned immCount=0;		// Counts the number of immediate values

#define write32(val) { \
	if (rec_phase) { \
		*armPtr = (val); \
		disarm((unsigned)armPtr,4); \
		armPtr++; \
	} \
}

typedef         double		Real64;
typedef  unsigned char		Bit8u;
typedef    signed char		Bit8s;
typedef unsigned short		Bit16u;
typedef   signed short		Bit16s;
typedef  unsigned long		Bit32u;
typedef    signed long		Bit32s;
typedef uint64_t	Bit64u;
typedef   int64_t	Bit64s;
typedef unsigned int		Bitu;
typedef signed int			Bits;
#include "risc_armv4le.h"

#define CALLFunc(addr) { if (rec_phase) { u32 offset=((((u32)(addr) - ((u32)(armPtr) + (8))) >> 2) & 0xFFFFFF); write32(BL_FWD(offset)); } }
#define CALLFuncCC(addr) { if (rec_phase) { u32 offset=((((u32)(addr) - ((u32)(armPtr) + (8))) >> 2) & 0xFFFFFF); write32(BLCC_FWD(offset)); } }
#define CALLFuncCS(addr) { if (rec_phase) { u32 offset=((((u32)(addr) - ((u32)(armPtr) + (8))) >> 2) & 0xFFFFFF); write32(BLCS_FWD(offset)); } }
#define JUMPFunc(addr) { if (rec_phase) { u32 offset=((((u32)(addr) - ((u32)(armPtr) + (8))) >> 2) & 0xFFFFFF); write32(B_FWD_(offset)); } }
#define JUMPFuncNE(addr) { if (rec_phase) { u32 offset=((((u32)(addr) - ((u32)(armPtr) + (8))) >> 2) & 0xFFFFFF); write32(BNE_FWD_(offset)); } }
#define MOV32RtoR(dst,src) gen_mov_regs(dst,src)
#define MOV32MtoR(reg,mem) gen_mov_word_to_reg(reg,(void*)mem,true)
#define MOV32ItoR(reg,imm) gen_mov_dword_to_reg_imm(reg,imm)
#define MOV32RtoM(mem,reg) gen_mov_word_from_reg(reg,(void*)mem,true)
#define MOV32ItoM(addr,value) gen_mov_direct_dword((void*)addr,value)
#define ADD32(res,op1,op2) write32(ADD_REG_LSL_IMM(res,op1,op2,0))
#define ADD32ItoR(reg,value) gen_add_imm(reg,reg,value)
#define MOVADD32ItoR(reg_dest,reg_src,value) gen_add_imm(reg_dest,reg_src,value)
#define ADD32ItoM(addr,value) gen_add_direct_word((void*)addr,value,true)
#define SUB32(res,op1,op2) write32(SUB_REG_LSL_IMM(res,op1,op2,0))
#define SUB32ItoR(reg,value) gen_sub_imm(reg,reg,value)
#define MOVSUB32ItoR(reg_dst,reg_src,value) gen_sub_imm(reg_dst,reg_src,value)
#define AND32(res,op1,op2) write32(AND_REG_LSL_IMM(res,op1,op2,0))
#define AND32ItoR(reg,imm) gen_and_imm(reg,imm)
#define OR32(res,op1,op2) write32(ORR_REG_LSL_IMM(res,op1,op2,0))
#define XOR32(res,op1,op2) write32(EOR_REG_LSL_IMM(res,op1,op2,0))
#define SHR32(res,op1,op2) write32(MOV_REG_LSR_REG(res,op1,op2))
#define SHL32(res,op1,op2) write32(MOV_REG_LSL_REG(res,op1,op2))
#define SAR32(res,op1,op2) write32(MOV_REG_ASR_REG(res,op1,op2))
#define SHRI32(res,op,imm) write32(MOV_REG_LSR_IMM(res,op,imm))
#define SHLI32(res,op,imm) write32(MOV_REG_LSL_IMM(res,op,imm))
#define SARI32(res,op,imm) write32(MOV_REG_ASR_IMM(res,op,imm))

#define MOVSX32M8toR(reg,mem) gen_mov_byte_to_reg_low_canuseword(reg,(void*)mem);gen_extend_byte(true,reg)
#define MOVSX32R8toR(reg) gen_extend_byte(true,reg)
#define MOVZX32M8toR(reg,mem) gen_mov_byte_to_reg_low_canuseword(reg,(void*)mem);gen_extend_byte(false,reg)
#define MOVZX32R8toR(reg) gen_extend_byte(false,reg)
#define MOVSX32M16toR(reg,mem) gen_mov_word_to_reg(reg,(void*)mem,false);gen_extend_word(true,reg)
#define MOVSX32R16toR(reg) gen_extend_word(true,reg)
#define MOVZX32M16toR(reg,mem) gen_mov_word_to_reg(reg,(void*)mem,false);gen_extend_word(false,reg)
#define MOVZX32R16toR(reg) gen_extend_word(false,reg)
#define MOV8ItoM(mem,imm) gen_mov_byte_to_reg_low_imm_canuseword(HOST_r0,imm);gen_mov_byte_from_reg_low(HOST_r0,(void*)mem)
#define MOV8MtoR(reg,mem) gen_mov_byte_to_reg_low_canuseword(reg,(void*)mem)
#define MOV8RtoM(mem,reg) gen_mov_byte_from_reg_low(reg,(void*)mem)
#define MOV16ItoM(mem,imm) gen_mov_word_to_reg_imm(HOST_r0,imm);gen_mov_word_from_reg(HOST_r0,(void*)mem,false)
#define MOV16MtoR(reg,mem) gen_mov_word_to_reg(reg,(void*)mem,false)
#define MOV16RtoM(mem,reg) gen_mov_word_from_reg(reg,(void*)mem,false)

#define JEZ8(reg) (u32*)gen_create_branch_on_zero(reg,true)
#define JNZ8(reg) (u32*)gen_create_branch_on_nonzero(reg,true)
#define armSetJ32(addr) gen_fill_branch((Bit32u)addr)
#define JEZ32(reg) (u32*)gen_create_branch_on_zero(reg,true)
#define JNZ32(reg) (u32*)gen_create_branch_on_nonzero(reg,true)
#define JLTZ32(reg) (u32*)gen_create_branch_on_ltz(reg,true)
#define JGTZ32(reg) (u32*)gen_create_branch_on_gtz(reg,true)
#define JLEZ32(reg) (u32*)gen_create_branch_on_letz(reg,true)
#define JGEZ32(reg) (u32*)gen_create_branch_on_getz(reg,true)

#define MOV32MtoR_regs(reg,mem) write32( LDR_IMM((reg), 11, (((u32)(mem))-((u32)&psxRegs))) )
#define MOV16MtoR_regs(reg,mem) { \
	if (rec_phase) { \
		if ((((u32)(mem))-((u32)&psxRegs))<256) { \
			write32( LDRH_IMM((reg), 11, (((u32)(mem))-((u32)&psxRegs)))); \
		} else { \
			MOV32ItoR(temp2,(((u32)(mem))-((u32)&psxRegs))); \
			write32( LDRH_REG((reg), 11, (temp2)) ); \
		} \
	} \
}
#define MOV16sMtoR_regs(reg,mem) { \
	if (rec_phase) { \
		if ((((u32)(mem))-((u32)&psxRegs))<256) { \
			write32( LDRSH_IMM((reg),11, (((u32)(mem))-((u32)&psxRegs)))); \
		}else{ \
			MOV32ItoR(temp2,(((u32)(mem))-((u32)&psxRegs))); \
			write32( LDRSH_REG((reg), 11, (temp2)) ); \
		} \
	} \
}
#define MOV8MtoR_regs(reg,mem) write32( LDRB_IMM((reg), 11, (((u32)(mem))-((u32)&psxRegs))) )
#define MOV16sRtoR_regs(reg1,mem2) write32( LDRSH_REG((reg1), 11, (reg2)) )
#define MOV32RtoM_regs(mem,reg) write32( STR_IMM((reg), 11, (((u32)(mem))-((u32)&psxRegs))) )
#define MOV16RtoM_regs(mem,reg) { \
	if (rec_phase) { \
		if ((((u32)(mem))-((u32)&psxRegs))<256) { \
			write32( STRH_IMM((reg), 11, (((u32)(mem))-((u32)&psxRegs)))); \
		} else { \
			MOV32ItoR(temp2,(((u32)(mem))-((u32)&psxRegs))); \
			write32( STRH_REG((reg), 11, (temp2)) ); \
		} \
	} \
}
#define MOV8RtoM_regs(mem,reg) write32( STRB_IMM((reg), 11, (((u32)(mem))-((u32)&psxRegs))) )
#define MOV32ItoM_regs(mem,value) MOV32ItoR(HOST_ip,value);MOV32RtoM_regs(mem,HOST_ip)
#define MOV16ItoM_regs(mem,value) MOV32ItoR(temp1,value);MOV16RtoM_regs(mem,temp1)
#define MOV8ItoM_regs(mem,value) MOV32ItoR(temp1,value);MOV8RtoM_regs(mem,temp1)
												
INLINE void ADD8ItoM_regs(u32 dest,Bit8s imm) {
	if(!imm || !rec_phase) return;
	MOV32MtoR_regs(temp1,dest);
	if (imm >= 0) {
		write32( ADD_IMM(temp1, temp1, (Bit32s)imm, 0) );      // add temp1, temp1, #(imm)
	} else {
		write32( SUB_IMM(temp1, temp1, -((Bit32s)imm), 0) );      // sub temp1, temp1, #(-imm)
	}
	MOV32RtoM_regs(dest,temp1);
}

INLINE void ADD32ItoM_regs(u32 dest,Bit32u imm) {
	if(!imm || !rec_phase) return;
	if ( (imm<128) || (imm>=0xffffff80) ) {
		ADD8ItoM_regs(dest,(Bit8s)imm);
		return;
	}
	MOV32MtoR_regs(temp1,dest);
	gen_mov_dword_to_reg_imm(temp2, imm);
	write32( ADD_REG_LSL_IMM(temp1, temp1, temp2, 0) );      // add temp1, temp1, temp2
	MOV32RtoM_regs(dest,temp1);
}

// CHUI: Actualiza la tabla de immediates
INLINE void UpdateImmediate(int forced) {
	unsigned i,j,ptrBase=(unsigned)armPtr;
	if (!rec_phase || (!forced && (((unsigned)armPtr)-immPtr[0] < REC_MAX_IMMEDIATE_LONG))) return;
#ifdef DEBUG_CPU
	unsigned optr=(unsigned)armPtr;
	dbg("\t\t\tUpdateImmediate");
#endif
	disarm_immediates=1;
	for (i = 0; i < immCount; i++) {
		immPtrValue[i]=(unsigned)armPtr;
		for (j=0;j<i;j++)
			if (immData[i]==immData[j])
				break;
		unsigned value=immPtrValue[j]-immPtr[i]-8;
#ifdef DEBUG_CPU
			if (value>4095)
				dbgf("\t\t\t\tVALUE %i OVERFLOW!!!!\n",value);
#endif
			*(unsigned *)(immPtr[i])|=value;
			if (j==i)
				write32(immData[i])
			else
				immPtrValue[i]=immPtrValue[j];
//		}
	}
#ifdef DEBUG_CPU
	immsize+=(((unsigned)armPtr)-optr);
	dbgf("\t\t\t!UpdateImmediate %i\n",(((unsigned)armPtr)-optr));
#endif
	disarm_immediates=0;
	immCount = 0;
}

INLINE void GET_PTR(void) {
	write32(0xe1a0c820); /*	mov     ip, r0, lsr #16 */
	write32(0xe791210c); /*	ldr     r2, [r1, ip, lsl #2] */
	write32(0xe1a01800); /*	mov     r1, r0, lsl #16 */
	write32(0xe1a00821); /*	mov     r0, r1, lsr #16 */
	write32(0xe0923000); /*	adds    r3, r2, r0 */
	write32(0xe1a01003); /*	mov     r1, r3 */
	write32(0x17921000); /*	ldrne   r1, [r2, r0] */
}

INLINE void RET_NC(void) {
#ifdef DEBUG_CPU
	dbg("\t\t\tRET_NC");
#endif
	write32(0xe8bd0ff0); /* ldmfd sp!, {r4-r11} */
	write32(0xe8bd8000); /* ldmfd sp!, {pc} */
	UpdateImmediate(0); // Creamos la tabla de inmediatos.
}

#ifdef REC_USE_RETURN_FUNCS
#define RET_RETURN() \
	JUMPFunc(func_Return_ptr); \
	UpdateImmediate(0)
#define RET_RETURN_with_HOST_r0() \
	JUMPFunc(func_Return_ptr+4); \
	UpdateImmediate(0)
#else
#define RET_RETURN() \
	recReturn(0)
#define RET_RETURN_with_HOST_r0() \
	recReturn(1)
#endif

#define RET() { \
	if (block) \
		RET_NC(); \
	else  \
	{  \
		RET_RETURN(); \
	} \
}

#define RET_with_HOST_r0() { \
	if (block) \
		RET_NC(); \
	else  \
	{  \
		RET_RETURN_with_HOST_r0(); \
	} \
}

// CHUI: Sale del entorno de recompilacion cuando el PC ha cambiado.
#ifdef REC_USE_RETURN_FUNCS
#define ExitPChange() { \
	MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc); \
	MOV32ItoR(HOST_r1,pc); \
	write32(CMP_REGS(HOST_r0,HOST_r1)); \
	JUMPFuncNE(func_Return_ptr+4); \
}
#else
#define ExitPChange() { \
	MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc); \
	MOV32ItoR(HOST_r1,pc); \
	write32(CMP_REGS(HOST_r0,HOST_r1)); \
	j32Ptr[11]=armPtr; write32(BEQ_FWD(0)); \
	RET_with_HOST_r0(); \
	armSetJ32(j32Ptr[11]); \
}
#endif

#define RET_cycles() { \
	ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS)); \
	cycles_pending=0; \
	RET(); \
}


#endif
