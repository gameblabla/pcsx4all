/*
 *  Copyright (C) 2002-2008  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef DEBUG_CPU
static unsigned immediates=0;
static unsigned immsize=0;
#endif

// CHUI: El maximo de opodes por immediate sin tabla
#define MAX_IMM_OPCODES 2

/* ARMv4 (little endian) backend by M-HT */

// type with the same size as a pointer
#define DRC_PTR_SIZE_IM Bit32u

// register mapping
typedef Bit8u HostReg;

// "lo" registers
#define HOST_r0		 0
#define HOST_r1		 1
#define HOST_r2		 2
#define HOST_r3		 3
#define HOST_r4		 4
#define HOST_r5		 5
#define HOST_r6		 6
#define HOST_r7		 7
// "hi" registers
#define HOST_r8		 8
#define HOST_r9		 9
#define HOST_r10	10
#define HOST_r11	11
#define HOST_r12	12
#define HOST_r13	13
#define HOST_r14	14
#define HOST_r15	15

// register aliases
// "lo" registers
#define HOST_a1 HOST_r0
#define HOST_a2 HOST_r1
#define HOST_a3 HOST_r2
#define HOST_a4 HOST_r3
#define HOST_v1 HOST_r4
#define HOST_v2 HOST_r5
#define HOST_v3 HOST_r6
#define HOST_v4 HOST_r7
// "hi" registers
#define HOST_v5 HOST_r8
#define HOST_v6 HOST_r9
#define HOST_v7 HOST_r10
#define HOST_v8 HOST_r11
#define HOST_ip HOST_r12
#define HOST_sp HOST_r13
#define HOST_lr HOST_r14
#define HOST_pc HOST_r15

// temporary registers
#define temp1 HOST_ip
#define temp2 HOST_lr
// CHUI: Ya no es necesario
//#define temp3 HOST_a4

// register that holds function return values
#define FC_RETOP HOST_a1

// register that holds the first parameter
#define FC_OP1 HOST_a1

// register that holds the second parameter
#define FC_OP2 HOST_a2

// helper macro
#define ROTATE_SCALE(x) ( (x)?(32 - x):(0) )

// instruction encodings

// move
// mov dst, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define MOV_IMM(dst, imm, rimm) (0xe3a00000 + ((dst) << 12) + (imm) + ((rimm) << 7) )
// mov dst, src, lsl #imm
#define MOV_REG_LSL_IMM(dst, src, imm) (0xe1a00000 + ((dst) << 12) + (src) + ((imm) << 7) )
// movs dst, src, lsl #imm
#define MOVS_REG_LSL_IMM(dst, src, imm) (0xe1b00000 + ((dst) << 12) + (src) + ((imm) << 7) )
// movcc dst, src, lsl #imm
#define MOVCC_REG_LSL_IMM(dst, src, imm) (0x31a00000 + ((dst) << 12) + (src) + ((imm) << 7) )
// mov dst, src, lsr #imm
#define MOV_REG_LSR_IMM(dst, src, imm) (imm?(0xe1a00020 + ((dst) << 12) + (src) + ((imm) << 7)):(MOV_REG_LSL_IMM(dst,src,0)))
// mov dst, src, asr #imm
#define MOV_REG_ASR_IMM(dst, src, imm) (imm?(0xe1a00040 + ((dst) << 12) + (src) + ((imm) << 7)):(MOV_REG_LSL_IMM(dst,src,0)))
// movcc dst, src, asr #imm
#define MOVCC_REG_ASR_IMM(dst, src, imm) (imm?(0x31a00040 + ((dst) << 12) + (src) + ((imm) << 7)):(MOVCC_REG_LSL_IMM(dst,src,0)))
// mov dst, src, lsl rreg
#define MOV_REG_LSL_REG(dst, src, rreg) (0xe1a00010 + ((dst) << 12) + (src) + ((rreg) << 8) )
// mov dst, src, lsr rreg
#define MOV_REG_LSR_REG(dst, src, rreg) (0xe1a00030 + ((dst) << 12) + (src) + ((rreg) << 8) )
// mov dst, src, asr rreg
#define MOV_REG_ASR_REG(dst, src, rreg) (0xe1a00050 + ((dst) << 12) + (src) + ((rreg) << 8) )
// mov dst, src, ror rreg
#define MOV_REG_ROR_REG(dst, src, rreg) (0xe1a00070 + ((dst) << 12) + (src) + ((rreg) << 8) )
// mvn dst, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define MVN_IMM(dst, imm, rimm) (0xe3e00000 + ((dst) << 12) + (imm) + ((rimm) << 7) )

// arithmetic
// add dst, src, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define ADD_IMM(dst, src, imm, rimm) (0xe2800000 + ((dst) << 12) + ((src) << 16) + (imm) + ((rimm) << 7) )
// add dst, src1, src2, lsl #imm
#define ADD_REG_LSL_IMM(dst, src1, src2, imm) (0xe0800000 + ((dst) << 12) + ((src1) << 16) + (src2) + ((imm) << 7) )
// sub dst, src, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define SUB_IMM(dst, src, imm, rimm) (0xe2400000 + ((dst) << 12) + ((src) << 16) + (imm) + ((rimm) << 7) )
// sub dst, src1, src2, lsl #imm
#define SUB_REG_LSL_IMM(dst, src1, src2, imm) (0xe0400000 + ((dst) << 12) + ((src1) << 16) + (src2) + ((imm) << 7) )
// rsb dst, src, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define RSB_IMM(dst, src, imm, rimm) (0xe2600000 + ((dst) << 12) + ((src) << 16) + (imm) + ((rimm) << 7) )
// cmp src, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define CMP_IMM(src, imm, rimm) (0xe3500000 + ((src) << 16) + (imm) + ((rimm) << 7) )
// cmn src, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define CMN_IMM(src, imm, rimm) (0xe3700000 + ((src) << 16) + (imm) + ((rimm) << 7) )
// nop
#define NOP MOV_REG_LSL_IMM(HOST_r0, HOST_r0, 0)

// logical
// tst src, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define TST_IMM(src, imm, rimm) (0xe3100000 + ((src) << 16) + (imm) + ((rimm) << 7) )
// and dst, src, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define AND_IMM(dst, src, imm, rimm) (0xe2000000 + ((dst) << 12) + ((src) << 16) + (imm) + ((rimm) << 7) )
// and dst, src1, src2, lsl #imm
#define AND_REG_LSL_IMM(dst, src1, src2, imm) (0xe0000000 + ((dst) << 12) + ((src1) << 16) + (src2) + ((imm) << 7) )
// orr dst, src, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define ORR_IMM(dst, src, imm, rimm) (0xe3800000 + ((dst) << 12) + ((src) << 16) + (imm) + ((rimm) << 7) )
// orrne dst, src, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define ORRNE_IMM(dst, src, imm, rimm) (0x13800000 + ((dst) << 12) + ((src) << 16) + (imm) + ((rimm) << 7) )
// orr dst, src1, src2, lsl #imm
#define ORR_REG_LSL_IMM(dst, src1, src2, imm) (0xe1800000 + ((dst) << 12) + ((src1) << 16) + (src2) + ((imm) << 7) )
// orr dst, src1, src2, lsr #imm
#define ORR_REG_LSR_IMM(dst, src1, src2, imm) (imm?(0xe1800020 + ((dst) << 12) + ((src1) << 16) + (src2) + ((imm) << 7)):(ORR_REG_LSL_IMM(dst,src1,src2,0)))
// eor dst, src1, src2, lsl #imm
#define EOR_REG_LSL_IMM(dst, src1, src2, imm) (0xe0200000 + ((dst) << 12) + ((src1) << 16) + (src2) + ((imm) << 7) )
// bic dst, src, #(imm ror rimm)		@	0 <= imm <= 255	&	rimm mod 2 = 0
#define BIC_IMM(dst, src, imm, rimm) (0xe3c00000 + ((dst) << 12) + ((src) << 16) + (imm) + ((rimm) << 7) )
// clz dst, src
#define CLZ(dst,src) (0xe16f0f10 + ((dst) << 12) + (src))

// load
// ldr reg, [addr, #imm]		@	0 <= imm < 4096
#define LDR_IMM(reg, addr, imm) (0xe5900000 + ((reg) << 12) + ((addr) << 16) + (imm) )
// ldr reg, [addr, #-imm]		@	0 <= imm < 4096
#define LDR_IMM_NEG(reg, addr, imm) (0xe5100000 + ((reg) << 12) + ((addr) << 16) + (imm) )
// ldrne reg, [addr, #imm]              @       0 <= imm < 4096
#define LDRNE_IMM(reg, addr, imm) (0x15900000 + ((reg) << 12) + ((addr) << 16) + (imm) )
// ldrcc reg, [addr, #imm]              @       0 <= imm < 4096
#define LDRCC_IMM(reg, addr, imm) (0x35900000 + ((reg) << 12) + ((addr) << 16) + (imm) )
// ldr reg, [reg, reg]
#define LDR_REG(reg_dest, reg_src1, reg_src2) (0xe7900000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// ldrcc reg, [reg, reg]
#define LDRCC_REG(reg_dest, reg_src1, reg_src2) (0x37900000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// ldr reg, [reg, reg, lsl #imm]
#define LDR_REG_LSL(reg_dest, reg_src1, reg_src2, imm) (0xe7900000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (imm << 7) + (reg_src2) )
// ldrh reg, [addr, #imm]		@	0 <= imm < 256
#define LDRH_IMM(reg, addr, imm) (0xe1d000b0 + ((reg) << 12) + ((addr) << 16) + (((imm) & 0xf0) << 4) + ((imm) & 0x0f) )
// ldrh reg, [reg, reg]
#define LDRH_REG(reg_dest, reg_src1, reg_src2) (0xe19000b0 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// ldrcch reg, [reg, reg]
#define LDRCCH_REG(reg_dest, reg_src1, reg_src2) (0x319000b0 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// ldrsh reg, [addr, #imm]		@	0 <= imm < 256
#define LDRSH_IMM(reg, addr, imm) (0xe1d000f0 + ((reg) << 12) + ((addr) << 16) + (((imm) & 0xf0) << 4) + ((imm) & 0x0f) )
// ldrsh reg, [reg, reg]
#define LDRSH_REG(reg_dest, reg_src1, reg_src2) (0xe19000f0 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// ldrb reg, [addr, #imm]		@	0 <= imm < 4096
#define LDRB_IMM(reg, addr, imm) (0xe5d00000 + ((reg) << 12) + ((addr) << 16) + (imm) )
// ldrb reg, [reg, reg]
#define LDRB_REG(reg_dest, reg_src1, reg_src2) (0xe7d00000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// ldrccb reg, [reg, reg]
#define LDRCCB_REG(reg_dest, reg_src1, reg_src2) (0x37d00000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )

// store
// str reg, [addr, #imm]		@	0 <= imm < 4096
#define STR_IMM(reg, addr, imm) (0xe5800000 + ((reg) << 12) + ((addr) << 16) + (imm) )
// str reg, [addr, #-imm]		@	0 <= imm < 4096
#define STR_IMM_NEG(reg, addr, imm) (0xe5000000 + ((reg) << 12) + ((addr) << 16) + (imm) )
// str reg, [reg, reg]
#define STR_REG(reg_dest, reg_src1, reg_src2) (0xe7800000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// strne reg, [reg, reg]
#define STRNE_REG(reg_dest, reg_src1, reg_src2) (0x17800000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// strcc reg, [reg, reg]
#define STRCC_REG(reg_dest, reg_src1, reg_src2) (0x37800000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// strh reg, [addr, #imm]		@	0 <= imm < 256
#define STRH_IMM(reg, addr, imm) (0xe1c000b0 + ((reg) << 12) + ((addr) << 16) + (((imm) & 0xf0) << 4) + ((imm) & 0x0f) )
// strh reg, [reg, reg]
#define STRH_REG(reg_dest, reg_src1, reg_src2) (0xe18000b0 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// strneh reg, [reg, reg]
#define STRNEH_REG(reg_dest, reg_src1, reg_src2) (0x118000b0 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// strcch reg, [reg, reg]
#define STRCCH_REG(reg_dest, reg_src1, reg_src2) (0x318000b0 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// strb reg, [addr, #imm]		@	0 <= imm < 4096
#define STRB_IMM(reg, addr, imm) (0xe5c00000 + ((reg) << 12) + ((addr) << 16) + (imm) )
// strb reg, [reg, reg]
#define STRB_REG(reg_dest, reg_src1, reg_src2) (0xe7c00000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// strneb reg, [reg, reg]
#define STRNEB_REG(reg_dest, reg_src1, reg_src2) (0x17c00000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// strccb reg, [reg, reg]
#define STRCCB_REG(reg_dest, reg_src1, reg_src2) (0x37c00000 + ((reg_dest) << 12) + ((reg_src1) << 16) + (reg_src2) )
// stmfd sp!, [regmask]
#define ARM_SAVEREG(regmask) (0xe92d0000 + regmask)
// ldmfd sp!, [regmask]
#define ARM_RESTOREREG(regmask) (0xe8bd0000 + regmask)
// stmfd sp!, {r0-r12}
#define ARM_SAVEREGS 0xe92d1fff
// ldmfd sp!, {r0-r12}
#define ARM_RESTOREREGS 0xe8bd1fff

// branch
// beq pc+imm		@	0 <= imm < 32M	&	imm mod 4 = 0
#define BEQ_FWD(imm) (0x0a000000 + ((imm) >> 2) )
// bne pc+imm		@	0 <= imm < 32M	&	imm mod 4 = 0
#define BNE_FWD(imm) (0x1a000000 + ((imm) >> 2) )
// carry set (unsigned higher or same)
#define BCS_FWD(imm) (0x2a000000 + ((imm) >> 2) )
// carry clear (unsigned lower)
#define BCC_FWD(imm) (0x3a000000 + ((imm) >> 2) )
// unsigned higher or same
#define BHS_FWD(imm) (0x2a000000 + ((imm) >> 2) )
// unsigned lower
#define BLO_FWD(imm) (0x3a000000 + ((imm) >> 2) )
// unsigned higher
#define BHI_FWD(imm) (0x8a000000 + ((imm) >> 2) )
// unsigned lower or same
#define BLS_FWD(imm) (0x9a000000 + ((imm) >> 2) )
// negative
#define BMI_FWD(imm) (0x4a000000 + ((imm) >> 2) )
// positive or zero
#define BPL_FWD(imm) (0x5a000000 + ((imm) >> 2) )
// signed greater or equal
#define BGE_FWD(imm) (0xaa000000 + ((imm) >> 2) )
// signed less than
#define BLT_FWD(imm) (0xba000000 + ((imm) >> 2) )
// signed greater than
#define BGT_FWD(imm) (0xca000000 + ((imm) >> 2) )
// signed less or equal
#define BLE_FWD(imm) (0xda000000 + ((imm) >> 2) )
// b pc+imm		@	0 <= imm < 32M	&	imm mod 4 = 0
#define B_FWD(imm) (0xea000000 + ((imm) >> 2) )
#define B_FWD_(imm) (0xea000000 + (imm))
#define BNE_FWD_(imm) (0x1a000000 + (imm))
#define BCS_FWD_(imm) (0x2a000000 + (imm))

#define MOV_IMM_INT(dst, imm) (0xe3a00000 + ((dst) << 12) + (imm) )
#define MVN_IMM_INT(dst, imm) (0xe3e00000 + ((dst) << 12) + (imm) )
#define CMP_REGS(reg1,reg2) (0xe1500000 + (reg1<<16) + (reg2))
#define MOVGE_REGS(reg1, reg2) (0xa1a00000 + (reg1<<12) + (reg2))
#define MOVGE_IMM(dst, imm, rimm) (0xa3a00000 + ((dst) << 12) + (imm) + ((rimm) << 7) )
#define MOVLT_IMM(dst, imm, rimm) (0xb3a00000 + ((dst) << 12) + (imm) + ((rimm) << 7) )
#define MOVCS_IMM(dst, imm, rimm) (0x23a00000 + ((dst) << 12) + (imm) + ((rimm) << 7) )
#define MOVCC_IMM(dst, imm, rimm) (0x33a00000 + ((dst) << 12) + (imm) + ((rimm) << 7) )
#define BL_FWD(imm) (0xeb000000 + (imm))
#define BLCC_FWD(imm) (0x3b000000 + (imm))
#define BLCS_FWD(imm) (0x2b000000 + (imm))
#define BX_LR()   (0xe12fff1e)
#define BXCC_LR() (0x312fff1e)
#define BXNE_LR() (0x112fff1e)

#define PLD(dst,imm) (0xf5d0f000 + ((dst) << 16) + ((imm)&0xfff) )

// move a full register from reg_src to reg_dst
static void gen_mov_regs(HostReg reg_dst,HostReg reg_src) {
	if(reg_src == reg_dst || !rec_phase) return;
	write32( MOV_REG_LSL_IMM(reg_dst, reg_src, 0) );      // mov reg_dst, reg_src
}

static Bit32u genimm(Bit32u imm,Bit32u *encoded) {
  if(imm==0) {*encoded=0;return 1;}
  int i=32;
  while(i>0)
  {
    if(imm<256) {
      *encoded=((i&30)<<7)|imm;
      return 1;
    }
    imm=(imm>>2)|(imm<<30);i-=2;
  }
  return 0;
}


// CHUI: Cuenta cuantos opcodes usara para poner un immediate
static int getImmOpcodes(Bit32u imm) {
	int ret=0;
	Bits scale = 0;
	while (imm) {
		while ((imm & 3) == 0) {
			imm>>=2;
			scale+=2;
		}
		ret++;
		imm>>=8;
		scale+=8;
	}
	return ret;
}

// move a 32bit constant value into dest_reg using table
static void gen_mov_dword_to_reg_imm_table(HostReg dest_reg,Bit32u imm) {
	immPtr[immCount]=(unsigned)armPtr;
	immData[immCount++]=imm;
	write32(LDR_IMM((dest_reg), HOST_pc, 0));

}

// move a 32bit constant value into dest_reg
static void gen_mov_dword_to_reg_imm(HostReg dest_reg,Bit32u imm) {
	Bit32u armval;
	if (!rec_phase) return;
#ifdef DEBUG_CPU
	unsigned optr=(unsigned)armPtr;
	dbgf("\t\t\tgen_mov_dword_to_reg_imm(r%i,%p)\n",dest_reg,imm);
#endif
	if(genimm(imm,&armval))
	{
		write32( MOV_IMM_INT(dest_reg,armval) );
	}
	else if(genimm(~imm,&armval))
	{
		write32( MVN_IMM_INT(dest_reg,armval) );
	}
	else {
// CHUI: Se sustituye por tabla de inmediatos si es necesario
		if (getImmOpcodes(imm)<=MAX_IMM_OPCODES) {
			Bits first, scale;
			scale = 0;
			first = 1;
			while (imm) {
				while ((imm & 3) == 0) {
					imm>>=2;
					scale+=2;
				}
				if (first) {
					write32( MOV_IMM(dest_reg, imm & 0xff, ROTATE_SCALE(scale)) );      // mov dest_reg, #((imm & 0xff) << scale)
					first = 0;
				} else {
					write32( ORR_IMM(dest_reg, dest_reg, imm & 0xff, ROTATE_SCALE(scale)) );      // orr dest_reg, dest_reg, #((imm & 0xff) << scale)
				}
				imm>>=8;
				scale+=8;
			}
		} else
			gen_mov_dword_to_reg_imm_table(dest_reg,imm);
	}
#ifdef DEBUG_CPU
	immediates++;
	immsize+=(((unsigned)armPtr)-optr);
#endif
}

// helper function for gen_mov_word_to_reg
static void gen_mov_word_to_reg_helper(HostReg dest_reg,void* data,bool dword,HostReg data_reg) {
#ifdef DEBUG_CPU
	unsigned optr=(unsigned)armPtr;
#endif
	// alignment....
	if (dword) {
		if ((Bit32u)data & 3) {
			if ( ((Bit32u)data & 3) == 2 ) {
				write32( LDRH_IMM(dest_reg, data_reg, 0) );      // ldrh dest_reg, [data_reg]
				write32( LDRH_IMM(temp2, data_reg, 2) );      // ldrh temp2, [data_reg, #2]
				write32( ORR_REG_LSL_IMM(dest_reg, dest_reg, temp2, 16) );      // orr dest_reg, dest_reg, temp2, lsl #16
			} else {
				write32( LDRB_IMM(dest_reg, data_reg, 0) );      // ldrb dest_reg, [data_reg]
				write32( LDRH_IMM(temp2, data_reg, 1) );      // ldrh temp2, [data_reg, #1]
				write32( ORR_REG_LSL_IMM(dest_reg, dest_reg, temp2, 8) );      // orr dest_reg, dest_reg, temp2, lsl #8
				write32( LDRB_IMM(temp2, data_reg, 3) );      // ldrb temp2, [data_reg, #3]
				write32( ORR_REG_LSL_IMM(dest_reg, dest_reg, temp2, 24) );      // orr dest_reg, dest_reg, temp2, lsl #24
			}
		} else {
			write32( LDR_IMM(dest_reg, data_reg, 0) );      // ldr dest_reg, [data_reg]
		}
	} else {
		if ((Bit32u)data & 1) {
			write32( LDRB_IMM(dest_reg, data_reg, 0) );      // ldrb dest_reg, [data_reg]
			write32( LDRB_IMM(temp2, data_reg, 1) );      // ldrb temp2, [data_reg, #1]
			write32( ORR_REG_LSL_IMM(dest_reg, dest_reg, temp2, 8) );      // orr dest_reg, dest_reg, temp2, lsl #8
		} else {
			write32( LDRH_IMM(dest_reg, data_reg, 0) );      // ldrh dest_reg, [data_reg]
		}
	}
#ifdef DEBUG_CPU
	immsize+=(((unsigned)armPtr)-optr);
#endif
}

// move a 32bit (dword==true) or 16bit (dword==false) value from memory into dest_reg
// 16bit moves may destroy the upper 16bit of the destination register
static void gen_mov_word_to_reg(HostReg dest_reg,void* data,bool dword) {
	if (!rec_phase) return;
	gen_mov_dword_to_reg_imm(temp1, (Bit32u)data);
	gen_mov_word_to_reg_helper(dest_reg, data, dword, temp1);
}

// move a 16bit constant value into dest_reg
// the upper 16bit of the destination register may be destroyed
INLINE void gen_mov_word_to_reg_imm(HostReg dest_reg,Bit16u imm) {
	if (!rec_phase) return;
	gen_mov_dword_to_reg_imm(dest_reg, (Bit32u)imm);
}

// helper function for gen_mov_word_from_reg
static void gen_mov_word_from_reg_helper(HostReg src_reg,void* dest,bool dword, HostReg data_reg) {
#ifdef DEBUG_CPU
	unsigned optr=(unsigned)armPtr;
#endif
	// alignment....
	if (dword) {
		if ((Bit32u)dest & 3) {
			if ( ((Bit32u)dest & 3) == 2 ) {
				write32( STRH_IMM(src_reg, data_reg, 0) );      // strh src_reg, [data_reg]
				write32( MOV_REG_LSR_IMM(temp2, src_reg, 16) );      // mov temp2, src_reg, lsr #16
				write32( STRH_IMM(temp2, data_reg, 2) );      // strh temp2, [data_reg, #2]
			} else {
				write32( STRB_IMM(src_reg, data_reg, 0) );      // strb src_reg, [data_reg]
				write32( MOV_REG_LSR_IMM(temp2, src_reg, 8) );      // mov temp2, src_reg, lsr #8
				write32( STRH_IMM(temp2, data_reg, 1) );      // strh temp2, [data_reg, #1]
				write32( MOV_REG_LSR_IMM(temp2, temp2, 16) );      // mov temp2, temp2, lsr #16
				write32( STRB_IMM(temp2, data_reg, 3) );      // strb temp2, [data_reg, #3]
			}
		} else {
			write32( STR_IMM(src_reg, data_reg, 0) );      // str src_reg, [data_reg]
		}
	} else {
		if ((Bit32u)dest & 1) {
			write32( STRB_IMM(src_reg, data_reg, 0) );      // strb src_reg, [data_reg]
			write32( MOV_REG_LSR_IMM(temp2, src_reg, 8) );      // mov temp2, src_reg, lsr #8
			write32( STRB_IMM(temp2, data_reg, 1) );      // strb temp2, [data_reg, #1]
		} else {
			write32( STRH_IMM(src_reg, data_reg, 0) );      // strh src_reg, [data_reg]
		}
	}
#ifdef DEBUG_CPU
	immsize+=(((unsigned)armPtr)-optr);
#endif
}

// move 32bit (dword==true) or 16bit (dword==false) of a register into memory
static void gen_mov_word_from_reg(HostReg src_reg,void* dest,bool dword) {
	if (!rec_phase) return;
	gen_mov_dword_to_reg_imm(temp1, (Bit32u)dest);
	gen_mov_word_from_reg_helper(src_reg, dest, dword, temp1);
}

// move an 8bit value from memory into dest_reg
// the upper 24bit of the destination register can be destroyed
// this function does not use FC_OP1/FC_OP2 as dest_reg as these
// registers might not be directly byte-accessible on some architectures
static void gen_mov_byte_to_reg_low(HostReg dest_reg,void* data) {
	if (!rec_phase) return;
	gen_mov_dword_to_reg_imm(temp1, (Bit32u)data);
	write32( LDRB_IMM(dest_reg, temp1, 0) );      // ldrb dest_reg, [temp1]
}

// move an 8bit value from memory into dest_reg
// the upper 24bit of the destination register can be destroyed
// this function can use FC_OP1/FC_OP2 as dest_reg which are
// not directly byte-accessible on some architectures
INLINE void gen_mov_byte_to_reg_low_canuseword(HostReg dest_reg,void* data) {
	if (!rec_phase) return;
	gen_mov_byte_to_reg_low(dest_reg, data);
}

// move an 8bit constant value into dest_reg
// the upper 24bit of the destination register can be destroyed
// this function does not use FC_OP1/FC_OP2 as dest_reg as these
// registers might not be directly byte-accessible on some architectures
static void gen_mov_byte_to_reg_low_imm(HostReg dest_reg,Bit8u imm) {
	if (!rec_phase) return;
	write32( MOV_IMM(dest_reg, imm, 0) );      // mov dest_reg, #(imm)
}

// move an 8bit constant value into dest_reg
// the upper 24bit of the destination register can be destroyed
// this function can use FC_OP1/FC_OP2 as dest_reg which are
// not directly byte-accessible on some architectures
INLINE void gen_mov_byte_to_reg_low_imm_canuseword(HostReg dest_reg,Bit8u imm) {
	if (!rec_phase) return;
	gen_mov_byte_to_reg_low_imm(dest_reg, imm);
}

// move the lowest 8bit of a register into memory
static void gen_mov_byte_from_reg_low(HostReg src_reg,void* dest) {
	if (!rec_phase) return;
	gen_mov_dword_to_reg_imm(temp1, (Bit32u)dest);
	write32( STRB_IMM(src_reg, temp1, 0) );      // strb src_reg, [temp1]
}

// convert an 8bit word to a 32bit dword
// the register is zero-extended (sign==false) or sign-extended (sign==true)
static void gen_extend_byte(bool sign,HostReg reg) {
	if (!rec_phase) return;
	if (sign) {
		write32( MOV_REG_LSL_IMM(reg, reg, 24) );      // mov reg, reg, lsl #24
		write32( MOV_REG_ASR_IMM(reg, reg, 24) );      // mov reg, reg, asr #24
	} else {
		write32( AND_IMM(reg, reg, 0xff, 0) );      // and reg, reg, #0xff
	}
}

// convert a 16bit word to a 32bit dword
// the register is zero-extended (sign==false) or sign-extended (sign==true)
static void gen_extend_word(bool sign,HostReg reg) {
	if (!rec_phase) return;
	if (sign) {
		write32( MOV_REG_LSL_IMM(reg, reg, 16) );      // mov reg, reg, lsl #16
		write32( MOV_REG_ASR_IMM(reg, reg, 16) );      // mov reg, reg, asr #16
	} else {
		write32( MOV_REG_LSL_IMM(reg, reg, 16) );      // mov reg, reg, lsl #16
		write32( MOV_REG_LSR_IMM(reg, reg, 16) );      // mov reg, reg, lsr #16
	}
}


// add a 32bit constant value to a full register using table
static void gen_add_imm_table(HostReg reg_dest,HostReg reg_src,Bit32u imm) {
	immPtr[immCount]=(unsigned)armPtr;
	immData[immCount++]=imm;
	write32(LDR_IMM((temp1), HOST_pc, 0));
	write32(ADD_REG_LSL_IMM((reg_dest),(reg_src),(temp1),0));
}

// add a 32bit constant value to a full register
static void gen_add_imm(HostReg reg_dest,HostReg reg_src,Bit32u imm) {
	if (!rec_phase) return;
	if(!imm) {
		if (reg_dest!=reg_src)
			write32( MOV_REG_LSL_IMM(reg_dest, reg_src, 0) );      // mov reg_dst, reg_src
		return;
	}
#ifdef DEBUG_CPU
	unsigned optr=(unsigned)armPtr;
	dbgf("\t\t\tgen_add_imm(r%i,r%i,%p)\n",reg_dest,reg_src,imm);
#endif
	if (imm == 0xffffffff) {
		write32( SUB_IMM(reg_dest, reg_src, 1, 0) );      // sub reg, reg, #1
	} else {
		if (imm<0x100) {
			write32(ADD_IMM(reg_dest, reg_src, imm, 0));
		} else {
// CHUI: Se sustituye por tabla de inmediatos si es necesario
			if (getImmOpcodes(imm)<=MAX_IMM_OPCODES) {
				Bits scale = 0;
				int first = 1;
				while (imm) {
					while ((imm & 3) == 0) {
						imm>>=2;
						scale+=2;
					}
					if (first) {
						write32( ADD_IMM(reg_dest, reg_src, imm & 0xff, ROTATE_SCALE(scale)) );      // add reg, reg, #((imm & 0xff) << scale)
						first=0;
					} else
						write32( ADD_IMM(reg_dest, reg_dest, imm & 0xff, ROTATE_SCALE(scale)) );      // add reg, reg, #((imm & 0xff) << scale)
					imm>>=8;
					scale+=8;
				}
			} else
				gen_add_imm_table(reg_dest,reg_src,imm);
		}
	}
#ifdef DEBUG_CPU
	immediates++;
	immsize+=(((unsigned)armPtr)-optr);
#endif
}


// sub a 32bit constant value to a full register using table
static void gen_sub_imm_table(HostReg reg_dest,HostReg reg_src,Bit32u imm) {
	immPtr[immCount]=(unsigned)armPtr;
	immData[immCount++]=imm;
	write32(LDR_IMM((temp1), HOST_pc, 0));
	write32(SUB_REG_LSL_IMM((reg_dest),(reg_src),(temp1),0));
}

// sub a 32bit constant value to a full register
static void gen_sub_imm(HostReg reg_dest,HostReg reg_src,Bit32u imm) {
	if (!rec_phase) return;
	if(!imm) {
		if (reg_dest!=reg_src)
			write32( MOV_REG_LSL_IMM(reg_dest, reg_src, 0) );      // mov reg_dst, reg_src
		return;
	}
#ifdef DEBUG_CPU
	unsigned optr=(unsigned)armPtr;
	dbgf("\t\t\tgen_sub_imm(r%i,r%i,%p)\n",reg_dest,reg_src,imm);
#endif
	if (imm == 0xffffffff) {
		write32( ADD_IMM(reg_dest, reg_src, 1, 0) );      // add reg, reg, #1
	} else {
		if (imm<0x100) {
			write32(SUB_IMM(reg_dest, reg_src, imm, 0));
		} else {
// CHUI: Se sustituye por tabla de inmediatos si es necesario
			if (getImmOpcodes(imm)<=MAX_IMM_OPCODES) {
				Bits scale = 0;
				int first = 1;
				while (imm) {
					while ((imm & 3) == 0) {
						imm>>=2;
						scale+=2;
					}
					if (first) {
						write32( SUB_IMM(reg_dest, reg_src, imm & 0xff, ROTATE_SCALE(scale)) );      // sub reg, reg, #((imm & 0xff) << scale)
						first=0;
					} else
						write32( SUB_IMM(reg_dest, reg_dest, imm & 0xff, ROTATE_SCALE(scale)) );      // sub reg, reg, #((imm & 0xff) << scale)
					imm>>=8;
					scale+=8;
				}
			} else
				gen_sub_imm_table(reg_dest,reg_src,imm);
		}
	}
#ifdef DEBUG_CPU
	immediates++;
	immsize+=(((unsigned)armPtr)-optr);
#endif
}


// CHUI: Cuenta cuantos opcodes usara para poner un immediate
static int getImmOpcodesAnd(Bit32u imm) {
	int ret=0;
	Bit32u imm2 = ~imm;
	Bits scale = 0;
	while (imm2) {
		while ((imm2 & 3) == 0) {
			imm2>>=2;
			scale+=2;
		}
		ret++;
		imm2>>=8;
		scale+=8;
	}
	return ret;
}

// and a 32bit constant value to a full register using table
static void gen_and_imm_table(HostReg reg,Bit32u imm) {
	immPtr[immCount]=(unsigned)armPtr;
	immData[immCount++]=imm;
	write32(LDR_IMM((temp1), HOST_pc, 0));
	write32(AND_REG_LSL_IMM((reg),(reg),(temp1),0));
}

// and a 32bit constant value with a full register
static void gen_and_imm(HostReg reg,Bit32u imm) {
	if (!rec_phase) return;
	Bit32u imm2 = ~imm;
	if(!imm2) return;
#ifdef DEBUG_CPU
	unsigned optr=(unsigned)armPtr;
	dbgf("\t\t\tgen_and_imm(r%i,%p)\n",reg,imm);
#endif
	if (!imm) {
		write32( MOV_IMM(reg, 0, 0) );      // mov reg, #0
	} else {
		if (imm<0x100) {
			write32(AND_IMM(reg, reg, imm, 0));
		} else {
// CHUI: Se sustituye por tabla de inmediatos si es necesario
			if (getImmOpcodesAnd(imm)<=MAX_IMM_OPCODES) {
				Bits scale = 0;
				while (imm2) {
					while ((imm2 & 3) == 0) {
						imm2>>=2;
						scale+=2;
					}
					write32( BIC_IMM(reg, reg, imm2 & 0xff, ROTATE_SCALE(scale)) );      // bic reg, reg, #((imm2 & 0xff) << scale)
					imm2>>=8;
					scale+=8;
				}
			} else
				gen_and_imm_table(reg,imm);
		}
	}
#ifdef DEBUG_CPU
	immediates++;
	immsize+=(((unsigned)armPtr)-optr);
#endif
}

// move a 32bit constant value into memory
static void gen_mov_direct_dword(void* dest,Bit32u imm) {
	if (!rec_phase) return;
	gen_mov_dword_to_reg_imm(temp2, imm);
	gen_mov_word_from_reg(temp2, dest, 1);
}

// conditional jump if register is zero
// the destination is set by gen_fill_branch() later
/*
static Bit32u gen_create_branch_on_zero(HostReg reg,bool dword) {
	if (dword) {
		write32( CMP_IMM(reg, 0, 0) );      // cmp reg, #0
	} else {
		write32( MOVS_REG_LSL_IMM(temp1, reg, 16) );      // movs temp1, reg, lsl #16
	}
	write32( BEQ_FWD(0) );      // beq j
	return ((Bit32u)armPtr-4);
}
*/

// conditional jump if register is nonzero
// the destination is set by gen_fill_branch() later
static Bit32u gen_create_branch_on_nonzero(HostReg reg,bool dword) {
	if (dword) {
		write32( CMP_IMM(reg, 0, 0) );      // cmp reg, #0
	} else {
		write32( MOVS_REG_LSL_IMM(temp1, reg, 16) );      // movs temp1, reg, lsl #16
	}
	write32( BNE_FWD(0) );      // bne j
	return ((Bit32u)armPtr-4);
}

static Bit32u gen_create_branch_on_zero(HostReg reg,bool dword) {
	if (dword) {
		write32( CMP_IMM(reg, 0, 0) );      // cmp reg, #0
	} else {
		write32( MOVS_REG_LSL_IMM(temp1, reg, 16) );      // movs temp1, reg, lsl #16
	}
	write32( BEQ_FWD(0) );      // bne j
	return ((Bit32u)armPtr-4);
}


static Bit32u gen_create_branch_on_ltz(HostReg reg,bool dword) {
	if (dword) {
		write32( CMP_IMM(reg, 0, 0) );
	} else {
		write32( MOVS_REG_LSL_IMM(temp1, reg, 16) );
	}
	write32( BLT_FWD(0) );
	return ((Bit32u)armPtr-4);
}

static Bit32u gen_create_branch_on_gtz(HostReg reg,bool dword) {
	if (dword) {
		write32( CMP_IMM(reg, 0, 0) );
	} else {
		write32( MOVS_REG_LSL_IMM(temp1, reg, 16) );
	}
	write32( BGT_FWD(0) );
	return ((Bit32u)armPtr-4);
}

static Bit32u gen_create_branch_on_letz(HostReg reg,bool dword) {
	if (dword) {
		write32( CMP_IMM(reg, 0, 0) );
	} else {
		write32( MOVS_REG_LSL_IMM(temp1, reg, 16) );
	}
	write32( BLE_FWD(0) );
	return ((Bit32u)armPtr-4);
}

static Bit32u gen_create_branch_on_getz(HostReg reg,bool dword) {
	if (dword) {
		write32( CMP_IMM(reg, 0, 0) );
	} else {
		write32( MOVS_REG_LSL_IMM(temp1, reg, 16) );
	}
	write32( BGE_FWD(0) );
	return ((Bit32u)armPtr-4);
}

// calculate relative offset and fill it into the location pointed to by data
INLINE void gen_fill_branch(DRC_PTR_SIZE_IM data) {
	if (!rec_phase) return;
// CHUI: Si es necesario rellenamos de NOP para no tener problemas
	while((u32)armPtr < (data+8))
		write32(NOP);
	*(Bit32u*)data=( (*(Bit32u*)data) & 0xff000000 ) | ( ( ((Bit32u)armPtr - (data+8)) >> 2 ) & 0x00ffffff );
}

INLINE void gen_align4(void) {
#ifdef REC_USE_ALIGN
	if (!rec_phase) return;
	while(((u32)armPtr)&0x1f)
		write32(NOP);
#endif
}
