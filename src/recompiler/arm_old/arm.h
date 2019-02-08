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
static u32 *j8Ptr[32];
static u32 *j32Ptr[32];

#define write32(val) *armPtr++ = (val);

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

#define CALLFunc(addr) { u32 offset=((((u32)(addr) - ((u32)(armPtr) + (8))) >> 2) & 0xFFFFFF); write32(BL_FWD(offset)); }
#define MOV32RtoR(dst,src) gen_mov_regs(dst,src)
#define MOV32MtoR(reg,mem) gen_mov_word_to_reg(reg,(void*)mem,true)
#define MOV32ItoR(reg,imm) gen_mov_dword_to_reg_imm(reg,imm)
#define MOV32RtoM(mem,reg) gen_mov_word_from_reg(reg,(void*)mem,true)
#define MOV32ItoM(addr,value) gen_mov_direct_dword((void*)addr,value)
#define ADD32(res,op1,op2) write32(ADD_REG_LSL_IMM(res,op1,op2,0))
#define ADD32ItoR(reg,value) gen_add_imm(reg,value)
#define ADD32ItoM(addr,value) gen_add_direct_word((void*)addr,value,true)
#define SUB32(res,op1,op2) write32(SUB_REG_LSL_IMM(res,op1,op2,0))
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
#define MOV8ItoM(mem,imm) gen_mov_byte_to_reg_low_imm_canuseword(HOST_a1,imm);gen_mov_byte_from_reg_low(HOST_a1,(void*)mem)
#define MOV8MtoR(reg,mem) gen_mov_byte_to_reg_low_canuseword(reg,(void*)mem)
#define MOV8RtoM(mem,reg) gen_mov_byte_from_reg_low(reg,(void*)mem)
#define MOV16ItoM(mem,imm) gen_mov_word_to_reg_imm(HOST_a1,imm);gen_mov_word_from_reg(HOST_a1,(void*)mem,false)
#define MOV16MtoR(reg,mem) gen_mov_word_to_reg(reg,(void*)mem,false)
#define MOV16RtoM(mem,reg) gen_mov_word_from_reg(reg,(void*)mem,false)

#define armSetJ8(addr) gen_fill_branch((Bit32u)addr)
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
#define MOV32RtoM_regs(mem,reg) write32( STR_IMM((reg), 11, (((u32)(mem))-((u32)&psxRegs))) )
#define MOV32ItoM_regs(mem,value) MOV32ItoR(HOST_ip,value);MOV32RtoM_regs(mem,HOST_ip)
												
INLINE void ADD8ItoM_regs(u32 dest,Bit8s imm) {
	if(!imm) return;
	MOV32MtoR_regs(temp3,dest);
	if (imm >= 0) {
		write32( ADD_IMM(temp3, temp3, (Bit32s)imm, 0) );      // add temp3, temp3, #(imm)
	} else {
		write32( SUB_IMM(temp3, temp3, -((Bit32s)imm), 0) );      // sub temp3, temp3, #(-imm)
	}
	MOV32RtoM_regs(dest,temp3);
}

INLINE void ADD32ItoM_regs(u32 dest,Bit32u imm) {
	if(!imm) return;
	if ( (imm<128) || (imm>=0xffffff80) ) {
		ADD8ItoM_regs(dest,(Bit8s)imm);
		return;
	}
	MOV32MtoR_regs(temp3,dest);
	gen_mov_dword_to_reg_imm(temp2, imm);
	write32( ADD_REG_LSL_IMM(temp3, temp3, temp2, 0) );      // add temp3, temp3, temp2
	MOV32RtoM_regs(dest,temp3);
}

#define GET_PTR() \
{ \
	write32(0xe1a0c821); /*	mov	ip, r1, lsr #16 */ \
	write32(0xe790210c); /* ldr	r2, [r0, ip, lsl #2] */ \
	write32(0xe1a00801); /* mov	r0, r1, lsl #16 */ \
	write32(0xe1a01820); /* mov	r1, r0, lsr #16 */ \
	write32(0xe0923001); /* adds	r3, r2, r1 */ \
	write32(0xe1a00003); /* mov	r0, r3 */ \
	write32(0x17920001); /*	ldrne	r0, [r2, r1] */ \
}

#define RET_NC() \
{ \
	write32(0xe8bd0ff0); /* ldmfd sp!, {r4-r11} */ \
	write32(0xe8bd8000); /* ldmfd sp!, {pc} */ \
}

#define RET() \
{ \
	if (block) \
	{ \
		RET_NC(); \
	} \
	else \
	{ \
		MOV32ItoR(HOST_a1,(Bit32u)psxRecLUT); \
		MOV32MtoR(HOST_a2,&psxRegs.pc); \
		GET_PTR(); \
		j8Ptr[4]=JNZ8(HOST_a1); \
		RET_NC(); \
		armSetJ8(j8Ptr[4]); \
		MOV32RtoR(HOST_pc,HOST_a1); \
	} \
}

#endif
