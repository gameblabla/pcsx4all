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

INLINE void PSXMEMREAD8(void)
{ 
	write32(MOV_REG_LSR_IMM(HOST_r1, HOST_r0, 16)); /* mov	r1, r0, lsr #16 */ 
	write32(MOV_REG_LSL_IMM(HOST_ip, HOST_r0, 16)); /* mov	ip, r0, lsl #16 */ 
	write32(0xe3510d7e); /* cmp	r1, #8064	; 0x1f80 */ 
	write32(MOV_REG_LSR_IMM(HOST_ip, HOST_ip, 16)); /* mov	ip, ip, lsr #16 */ 
	write32(BEQ_FWD(16)); /* beq xxx */ 
	write32( LDR_IMM(HOST_r2, HOST_pc, 8) ); 
	write32(0xe7921101); /* ldr	r1, [r2, r1, lsl #2] */ 
	write32(0xe7dc0001); /* ldrb r0, [ip, r1] */ 
	write32(B_FWD(28)); /* b fin */ 
	write32( (Bit32u)psxMemRLUT ); 
	write32(0xe35c0a01); /* cmp	ip, #4096	; 0x1000 */  /* xxx */ 
	write32(BCS_FWD(12)); /* bcs yyy */ 
	write32( LDR_IMM(HOST_r1, HOST_pc, 4) ); 
	write32(0xe7dc0001); /* ldrb r0, [ip, r1] */ 
	write32(B_FWD(4)); /* b fin */ 
	write32( (Bit32u)psxH ); 
	CALLFunc((u32)psxHwRead8); /* yyy */ 
}

INLINE void PSXMEMREAD16(void)
{
	write32(MOV_REG_LSR_IMM(HOST_r1, HOST_r0, 16)); /* mov	r1, r0, lsr #16 */ 
	write32(MOV_REG_LSL_IMM(HOST_ip, HOST_r0, 16)); /* mov	ip, r0, lsl #16 */ 
	write32(0xe3510d7e); /* cmp	r1, #8064	; 0x1f80 */ 
	write32(MOV_REG_LSR_IMM(HOST_ip, HOST_ip, 16)); /* mov	ip, ip, lsr #16 */ 
	write32(BEQ_FWD(16)); /* beq xxx */ 
	write32( LDR_IMM(HOST_r2, HOST_pc, 8) ); 
	write32(0xe7921101); /* ldr	r1, [r2, r1, lsl #2] */ 
	write32(0xe19c00b1); /* ldrh r0, [ip, r1] */ 
	write32(B_FWD(28)); /* b fin */ 
	write32( (Bit32u)psxMemRLUT ); 
	write32(0xe35c0a01); /* cmp	ip, #4096	; 0x1000 */  /* xxx */ 
	write32(BCS_FWD(12)); /* bcs yyy */ 
	write32( LDR_IMM(HOST_r1, HOST_pc, 4) ); 
	write32(0xe19c00b1); /* ldrh r0, [ip, r1] */ 
	write32(B_FWD(4)); /* b fin */ 
	write32( (Bit32u)psxH ); 
	CALLFunc((u32)psxHwRead16); /* yyy */
}

INLINE void PSXMEMREAD32(void)
{
	write32(MOV_REG_LSR_IMM(HOST_r1, HOST_r0, 16)); /* mov	r1, r0, lsr #16 */ 
	write32(MOV_REG_LSL_IMM(HOST_ip, HOST_r0, 16)); /* mov	ip, r0, lsl #16 */ 
	write32(0xe3510d7e); /* cmp	r1, #8064	; 0x1f80 */ 
	write32(MOV_REG_LSR_IMM(HOST_ip, HOST_ip, 16)); /* mov	ip, ip, lsr #16 */ 
	write32(BEQ_FWD(16)); /* beq xxx */ 
	write32( LDR_IMM(HOST_r2, HOST_pc, 8) ); 
	write32(0xe7921101); /* ldr	r1, [r2, r1, lsl #2] */ 
	write32(0xe79c0001); /* ldr	r0, [ip, r1] */ 
	write32(B_FWD(28)); /* b fin */ 
	write32( (Bit32u)psxMemRLUT ); 
	write32(0xe35c0a01); /* cmp	ip, #4096	; 0x1000 */ /* xxx */
	write32(BCS_FWD(12)); /* bcs yyy */ 
	write32( LDR_IMM(HOST_r1, HOST_pc, 4) ); 
	write32(0xe79c0001); /* ldr	r0, [ip, r1] */ 
	write32(B_FWD(4)); /* b fin */ 
	write32( (Bit32u)psxH ); 
	CALLFunc((u32)psxHwRead32);  /* yyy */ 
}

INLINE void PSXHWREAD8(void)
{ 	
	write32(0xe1a03820); /* mov	r3, r0, lsr #16 */ 
	write32(0xe1a02800); /*	mov	r2, r0, lsl #16 */ 
	write32(0xe3530d7e); /*	cmp	r3, #8064	; 0x1f80 */ 
	write32(0xe1a02822); /*	mov	r2, r2, lsr #16 */ 
	write32(BNE_FWD(8)); /* bne fin */ 
    write32(0xe3520a01); /*	cmp	r2, #4096	; 0x1000 */ 
	write32(BCC_FWD(0)); /* bcc fin */ 
	CALLFunc((u32)psxHwRead8); 
}

INLINE void PSXHWREAD16(void)
{
	write32(0xe1a03820); /* mov	r3, r0, lsr #16 */ 
	write32(0xe1a02800); /*	mov	r2, r0, lsl #16 */ 
	write32(0xe3530d7e); /*	cmp	r3, #8064	; 0x1f80 */ 
	write32(0xe1a02822); /*	mov	r2, r2, lsr #16 */ 
	write32(BNE_FWD(8)); /* bne fin */ 
    write32(0xe3520a01); /*	cmp	r2, #4096	; 0x1000 */ 
	write32(BCC_FWD(0)); /* bcc fin */ 
	CALLFunc((u32)psxHwRead16); 
}
	
INLINE void PSXHWREAD32(void)
{ 	
	write32(0xe1a03820); /* mov	r3, r0, lsr #16 */ 
	write32(0xe1a02800); /*	mov	r2, r0, lsl #16 */ 
	write32(0xe3530d7e); /*	cmp	r3, #8064	; 0x1f80 */ 
	write32(0xe1a02822); /*	mov	r2, r2, lsr #16 */ 
	write32(BNE_FWD(8)); /* bne fin */ 
    write32(0xe3520a01); /*	cmp	r2, #4096	; 0x1000 */ 
	write32(BCC_FWD(0)); /* bcc fin */ 
	CALLFunc((u32)psxHwRead32); 
}

INLINE void PSXMEMWRITE8(void)
{ 
	write32(0xe1a0e820); /* mov	lr, r0, lsr #16 (t=lr) */ 
	write32(0xe1a0c800); /* mov	ip, r0, lsl #16 */ 
	write32(0xe35e0d7e); /* cmp	lr, #8064	; 0x1f80 */ 
	write32(0xe1a0c82c); /* mov	ip, ip, lsr #16 (m=ip) */ 
	write32(BEQ_FWD(48)); /* beq xxx */ 
	write32( LDR_IMM(HOST_r2, HOST_pc, 36) ); /* x */ 
	write32(0xe792310e); /* ldr	r3, [r2, lr, lsl #2] */ 
	write32(0xe3530000); /* cmp	r3, #0	; 0x0 */ 
	write32(BEQ_FWD(60)); /* beq fin */ 
	write32(0xe7cc1003); /* strb	r1, [ip, r3] */ 
	write32( LDR_IMM(HOST_r2, HOST_pc, 20) ); /* y */ 
	write32(0xe792110e); /* ldr	r1, [r2, lr, lsl #2] */ 
	write32(BIC_IMM(HOST_ip, HOST_ip, 3, 0)); /* bic ip,ip,#3 */ 
	write32(0xe3a00000); /* mov	r0, #0	; 0x0 */ 
	write32(0xe781000c); /* str	r0, [r1, ip] */ 
	write32(B_FWD(32)); /* b fin */ 
	write32( (Bit32u)psxMemWLUT ); /* x */ 
	write32( (Bit32u)psxRecLUT ); /* y */ 
	write32(0xe35c0a01); /* cmp	ip, #4096	; 0x1000 */ /* xxx */
	write32(BCS_FWD(12)); /* bcs zzz */ 
	write32( LDR_IMM(HOST_r3, HOST_pc, 4) ); /* z */ 
	write32(0xe7cc1003); /* strn	r1, [ip, r3] */ 
	write32(B_FWD(4)); /* b fin */ 
	write32( (Bit32u)psxH ); /* z */ 
	CALLFunc((u32)psxHwWrite8); /* zzz */
}

INLINE void PSXMEMWRITE16(void)
{ 
	write32(0xe1a0e820); /* mov	lr, r0, lsr #16 (t=lr) */ 
	write32(0xe1a0c800); /* mov	ip, r0, lsl #16 */ 
	write32(0xe35e0d7e); /* cmp	lr, #8064	; 0x1f80 */ 
	write32(0xe1a0c82c); /* mov	ip, ip, lsr #16 (m=ip) */ 
	write32(BEQ_FWD(48)); /* beq xxx */ 
	write32( LDR_IMM(HOST_r2, HOST_pc, 36) ); /* x */ 
	write32(0xe792310e); /* ldr	r3, [r2, lr, lsl #2] */ 
	write32(0xe3530000); /* cmp	r3, #0	; 0x0 */ 
	write32(BEQ_FWD(60)); /* beq fin */ 
	write32(0xe18c10b3); /* strh	r1, [ip, r3] */ 
	write32( LDR_IMM(HOST_r2, HOST_pc, 20) ); /* y */ 
	write32(0xe792110e); /* ldr	r1, [r2, lr, lsl #2] */ 
	write32(BIC_IMM(HOST_ip, HOST_ip, 1, 0)); /* bic ip,ip,#1 */ 
	write32(0xe3a00000); /* mov	r0, #0	; 0x0 */ 
	write32(0xe781000c); /* str	r0, [r1, ip] */ 
	write32(B_FWD(32)); /* b fin */ 
	write32( (Bit32u)psxMemWLUT ); /* x */ 
	write32( (Bit32u)psxRecLUT ); /* y */ 
	write32(0xe35c0a01); /* cmp	ip, #4096	; 0x1000 */ /* xxx */
	write32(BCS_FWD(12)); /* bcs zzz */ 
	write32( LDR_IMM(HOST_r3, HOST_pc, 4) ); /* z */ 
	write32(0xe18c10b3); /* strh	r1, [ip, r3] */ 
	write32(B_FWD(4)); /* b fin */ 
	write32( (Bit32u)psxH ); /* z */ 
	CALLFunc((u32)psxHwWrite16); /* zzz */
}

INLINE void PSXMEMWRITE32(void)
{ 
	write32(0xe1a0e820); /* mov	lr, r0, lsr #16 (t=lr) */ 
	write32(0xe1a0c800); /* mov	ip, r0, lsl #16 */ 
	write32(0xe35e0d7e); /* cmp	lr, #8064	; 0x1f80 */ 
	write32(0xe1a0c82c); /* mov	ip, ip, lsr #16 (m=ip) */ 
	write32(BEQ_FWD(52)); /* beq xxx */ 
	write32( LDR_IMM(HOST_r2, HOST_pc, 32) ); /* x */ 
	write32(0xe792310e); /* ldr	r3, [r2, lr, lsl #2] */ 
	write32(0xe3530000); /* cmp	r3, #0	; 0x0 */ 
	write32(BEQ_FWD(28)); /* beq yyy */ 
	write32(0xe78c1003); /* str	r1, [ip, r3] */ 
	write32( LDR_IMM(HOST_r2, HOST_pc, 16) ); /* y */ 
	write32(0xe792110e); /* ldr	r1, [r2, lr, lsl #2] */ 
	write32(0xe3a00000); /* mov	r0, #0	; 0x0 */ 
	write32(0xe781000c); /* str	r0, [r1, ip] */ 
	write32(B_FWD(40)); /* b fin */ 
	write32( (Bit32u)psxMemWLUT ); /* x */ 
	write32( (Bit32u)psxRecLUT ); /* y */ 
	extern void psxMemWrite32_error(u32 mem, u32 value); CALLFunc((u32)psxMemWrite32_error); /* yyy */
	write32(B_FWD(24)); /* b fin */ 
	write32(0xe35c0a01); /* cmp	ip, #4096	; 0x1000 */ /* xxx */
	write32(BCS_FWD(12)); /* bcs zzz */ 
	write32( LDR_IMM(HOST_r3, HOST_pc, 4) ); /* z */ 
	write32(0xe78c1003); /* str	r1, [ip, r3] */ 
	write32(B_FWD(4)); /* b fin */ 
	write32( (Bit32u)psxH ); /* z */ 
	CALLFunc((u32)psxHwWrite32); /* zzz */
}

/* Push OfB for Stores/Loads */
static void iPushOfB() {
	if (IsConst(_Rs_)) {
		MOV32ItoR(HOST_a1,iRegs[_Rs_].k + _Imm_);
	} else {
		u32 rs=ReadReg(_Rs_);
		MOV32RtoR(HOST_a1, rs);
		if (_Imm_) {
			ADD32ItoR(HOST_a1, _Imm_);
		}
	}
}

//REC_FUNC(LB);
static void recLB() {
// Rt = mem[Rs + Im] (signed)

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
			MOV32ItoR(HOST_a1,addr);
			CALLFunc((u32)psxHwRead8);
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				write32( MOV_REG_LSL_IMM(HOST_a1, HOST_a1, 24) ); // mov reg, reg, lsl #24
				write32( MOV_REG_ASR_IMM(rt, HOST_a1, 24) ); // mov reg, reg, asr #24
			}
			return;
		}
	}

	iPushOfB();
	
	if (_Rt_)
	{
		PSXMEMREAD8();
		u32 rt=WriteReg(_Rt_);
		write32( MOV_REG_LSL_IMM(HOST_a1, HOST_a1, 24) ); // mov reg, reg, lsl #24
		write32( MOV_REG_ASR_IMM(rt, HOST_a1, 24) ); // mov reg, reg, asr #24
	}
	else
	{
		PSXHWREAD8();
	}
}

//REC_FUNC(LBU);
static void recLBU() {
// Rt = mem[Rs + Im] (unsigned)

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
			MOV32ItoR(HOST_a1,addr);
			CALLFunc((u32)psxHwRead8);
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				write32( AND_IMM(rt, HOST_a1, 0xff, 0) );      // and reg, reg, #0xff
			}
			return;
		}
	}

	iPushOfB();

	if (_Rt_) 
	{
		PSXMEMREAD8();
		u32 rt=WriteReg(_Rt_);
		write32( AND_IMM(rt, HOST_a1, 0xff, 0) );      // and reg, reg, #0xff
	}
	else
	{
		PSXHWREAD8();
	}
}

//REC_FUNC(LH);
static void recLH() {
// Rt = mem[Rs + Im] (signed)

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
			MOV32ItoR(HOST_a1,addr);
			CALLFunc((u32)psxHwRead16);
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				write32( MOV_REG_LSL_IMM(HOST_a1, HOST_a1, 16) );      // mov reg, reg, lsl #16
				write32( MOV_REG_ASR_IMM(rt, HOST_a1, 16) );      // mov reg, reg, asr #16
			}
			return;
		}
	}

	iPushOfB();
	
	if (_Rt_)
	{
		PSXMEMREAD16();
		u32 rt=WriteReg(_Rt_);
		write32( MOV_REG_LSL_IMM(HOST_a1, HOST_a1, 16) );      // mov reg, reg, lsl #16
		write32( MOV_REG_ASR_IMM(rt, HOST_a1, 16) );      // mov reg, reg, asr #16
	}
	else
	{
		PSXHWREAD16();
	}
}

//REC_FUNC(LHU);
static void recLHU() {
// Rt = mem[Rs + Im] (unsigned)

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
					MOV32ItoR(HOST_a1, addr);
					CALLFunc((u32)&SPU_readRegister);
					u32 rt=WriteReg(_Rt_);
					write32( MOV_REG_LSL_IMM(HOST_a1, HOST_a1, 16) );      // mov reg, reg, lsl #16
					write32( MOV_REG_LSR_IMM(rt, HOST_a1, 16) );      // mov reg, reg, lsr #16
				}
				return;
			}
			switch (addr) {
				case 0x1f801100: case 0x1f801110: case 0x1f801120:
					if (_Rt_) {
						MOV32ItoR(HOST_a1, (addr >> 4) & 0x3);
						CALLFunc((u32)psxRcntRcount);
						u32 rt=WriteReg(_Rt_);
						write32( MOV_REG_LSL_IMM(HOST_a1, HOST_a1, 16) );      // mov reg, reg, lsl #16
						write32( MOV_REG_LSR_IMM(rt, HOST_a1, 16) );      // mov reg, reg, lsr #16
					}
					return;

				case 0x1f801104: case 0x1f801114: case 0x1f801124:
					if (_Rt_) {
						MOV32ItoR(HOST_a1, (addr >> 4) & 0x3);
						CALLFunc((u32)psxRcntRmode);
						u32 rt=WriteReg(_Rt_);
						write32( MOV_REG_LSL_IMM(HOST_a1, HOST_a1, 16) );      // mov reg, reg, lsl #16
						write32( MOV_REG_LSR_IMM(rt, HOST_a1, 16) );      // mov reg, reg, lsr #16
					}
					return;

				case 0x1f801108: case 0x1f801118: case 0x1f801128:
					if (_Rt_) {
						MOV32ItoR(HOST_a1, (addr >> 4) & 0x3);
						CALLFunc((u32)psxRcntRtarget);
						u32 rt=WriteReg(_Rt_);
						write32( MOV_REG_LSL_IMM(HOST_a1, HOST_a1, 16) );      // mov reg, reg, lsl #16
						write32( MOV_REG_LSR_IMM(rt, HOST_a1, 16) );      // mov reg, reg, lsr #16
					}
					return;
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			MOV32ItoR(HOST_a1,addr);
			CALLFunc((u32)psxHwRead16);
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				write32( MOV_REG_LSL_IMM(HOST_a1, HOST_a1, 16) );      // mov reg, reg, lsl #16
				write32( MOV_REG_LSR_IMM(rt, HOST_a1, 16) );      // mov reg, reg, lsr #16
			}
			return;
		}
	}

	iPushOfB();
	
	if (_Rt_) 
	{
		PSXMEMREAD16();
		u32 rt=WriteReg(_Rt_);
		write32( MOV_REG_LSL_IMM(HOST_a1, HOST_a1, 16) );      // mov reg, reg, lsl #16
		write32( MOV_REG_LSR_IMM(rt, HOST_a1, 16) );      // mov reg, reg, lsr #16
	}
	else
	{
		PSXHWREAD16();
	}
}

//REC_FUNC(LW);
static void recLW() {
// Rt = mem[Rs + Im] (unsigned)

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
				u32 rt=WriteReg(_Rt_);
				MOV32MtoR(rt, (u32)&psxM[addr & 0x1fffff]);
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
						CALLFunc((u32)&GPU_readData);
						u32 rt=WriteReg(_Rt_);
						MOV32RtoR(rt, HOST_a1);
					}
					return;

				case 0x1f801814:
					if (_Rt_) {
						CALLFunc((u32)&GPU_readStatus);
						u32 rt=WriteReg(_Rt_);
						MOV32RtoR(rt, HOST_a1);
					}
					return;
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			MOV32ItoR(HOST_a1,addr);
			CALLFunc((u32)psxHwRead32);
			if (_Rt_) {
				u32 rt=WriteReg(_Rt_);
				MOV32RtoR(rt,HOST_a1);
			}
			return;
		}
	}

	iPushOfB();
	
	if (_Rt_) 
	{
		PSXMEMREAD32();
		u32 rt=WriteReg(_Rt_);
		MOV32RtoR(rt, HOST_a1);
	}
	else
	{
		PSXHWREAD32();
	}
}

//REC_FUNC(SB);
static void recSB() {
// mem[Rs + Im] = Rt

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
			MOV32ItoR(HOST_a1,addr);
			if (IsConst(_Rt_)) {
				MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV32RtoR(HOST_a2, rt);
			}
			CALLFunc((u32)psxHwWrite8);
			return;
		}
	}

	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_a2, rt);
	}
	iPushOfB();
	PSXMEMWRITE8();
}

//REC_FUNC(SH);
static void recSH() {
// mem[Rs + Im] = Rt

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
					MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
				} else {
					u32 rt=ReadReg(_Rt_);
					MOV32RtoR(HOST_a2, rt);
				}
				MOV32ItoR(HOST_a1, addr);
// XXX Dec 2016: SPU of PCSX Rearmed has been adopted as standard SPU.
//               Some SPU functions now take additional 'cycles' param and
//               no effort has been made to update old ARM dynarecs to match.
#error "ARM dynarec has not been updated to match new SPU interface of spu_pcsxrearmed"
				CALLFunc((u32)&SPU_writeRegister);
				return;
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			MOV32ItoR(HOST_a1,addr);
			if (IsConst(_Rt_)) {
				MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV32RtoR(HOST_a2, rt);
			}
			CALLFunc((u32)psxHwWrite16);
			return;
		}
	}

	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_a2, rt);
	}
	iPushOfB();
	PSXMEMWRITE16();
}

//REC_FUNC(SW);
static void recSW() {
// mem[Rs + Im] = Rt

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
					return;

				case 0x1f801810:
					if (IsConst(_Rt_)) {
						MOV32ItoR(HOST_a1,iRegs[_Rt_].k);
					} else {
						u32 rt=ReadReg(_Rt_);
						MOV32RtoR(HOST_a1,rt);
					}
					CALLFunc((u32)&GPU_writeData);
					return;

				case 0x1f801814:
					if (IsConst(_Rt_)) {
						MOV32ItoR(HOST_a1,iRegs[_Rt_].k);
					} else {
						u32 rt=ReadReg(_Rt_);
						MOV32RtoR(HOST_a1,rt);
					}
					CALLFunc((u32)&GPU_writeStatus);
					return; // ???
			}
		}
		if (t == 0x1f80 && addr >= 0x1f801000) {
			MOV32ItoR(HOST_a1,addr);
			if (IsConst(_Rt_)) {
				MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
			} else {
				u32 rt=ReadReg(_Rt_);
				MOV32RtoR(HOST_a2, rt);
			}
			CALLFunc((u32)psxHwWrite32);
			return;
		}
	}

	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_a2, rt);
	}
	iPushOfB();
	PSXMEMWRITE32();
}


//REC_FUNC(LWL);
static void recLWL() {
// Rt = Rt Merge mem[Rs + Im]
	if (_Rt_) {
		if (IsConst(_Rs_)) {
			MOV32ItoR(HOST_a1, iRegs[_Rs_].k + _Imm_);
		}
		else {
			u32 rs=ReadReg(_Rs_);
			MOV32RtoR(HOST_a1, rs);
			if (_Imm_) ADD32ItoR(HOST_a1, _Imm_);
		}

		u32 temp=TempReg();
		write32(AND_IMM(temp, HOST_a1, 3, 0));
		write32(BIC_IMM(HOST_a1, HOST_a1, 3, 0));
		PSXMEMREAD32();
		MOV32RtoR(HOST_a3,temp);
		
		u32 rt;
		if (IsConst(_Rt_)) {
			MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
		} else {
			rt=ReadReg(_Rt_);
			MOV32RtoR(HOST_a2, rt);
		}

		write32(LDR_IMM(HOST_lr, HOST_pc, 20)); /* LWL_MASK */
		write32(0xe79ec102); /* ldr	ip, [lr, r2, lsl #2] */
		write32(LDR_IMM(HOST_lr, HOST_pc, 16)); /* LWL_SHIFT */
		write32(0xe00cc001); /* and	ip, ip, r1 */
		write32(0xe79e3102); /* ldr	r3, [lr, r2, lsl #2] */
		write32(0xe18c0310); /* orr	r0, ip, r0, lsl r3 */
		write32(B_FWD(4)); /* fin */
		extern u32 LWL_MASK[4]; write32((u32)LWL_MASK);
		extern u32 LWL_SHIFT[4]; write32((u32)LWL_SHIFT);
		
		rt=WriteReg(_Rt_);
		MOV32RtoR(rt,HOST_a1);
	}
	else
	{
		if (IsConst(_Rs_)) MOV32ItoR(HOST_a1, iRegs[_Rs_].k + _Imm_);
		else {
			u32 rs=ReadReg(_Rs_);
			MOV32RtoR(HOST_a1, rs);
			if (_Imm_) ADD32ItoR(HOST_a1, _Imm_);
		}
		write32(BIC_IMM(HOST_a1, HOST_a1, 3, 0));
		PSXHWREAD32();
	}
}

//REC_FUNC(LWR);
static void recLWR() {
// Rt = Rt Merge mem[Rs + Im]

	if (_Rt_) {
		if (IsConst(_Rs_)) {
			MOV32ItoR(HOST_a1, iRegs[_Rs_].k + _Imm_);
		}
		else {
			u32 rs=ReadReg(_Rs_);
			MOV32RtoR(HOST_a1, rs);
			if (_Imm_) ADD32ItoR(HOST_a1, _Imm_);
		}

		u32 temp=TempReg();
		write32(AND_IMM(temp, HOST_a1, 3, 0));
		write32(BIC_IMM(HOST_a1, HOST_a1, 3, 0));
		PSXMEMREAD32();
		MOV32RtoR(HOST_a3,temp);

		u32 rt;
		if (IsConst(_Rt_)) {
			MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
		} else {
			rt=ReadReg(_Rt_);
			MOV32RtoR(HOST_a2, rt);
		}

		write32(LDR_IMM(HOST_lr, HOST_pc, 20)); /* LWR_MASK */
		write32(0xe79ec102); /* ldr	ip, [lr, r2, lsl #2] */
		write32(LDR_IMM(HOST_lr, HOST_pc, 16)); /* LWR_SHIFT */
		write32(0xe00cc001); /* and	ip, ip, r1 */
		write32(0xe79e3102); /* ldr	r3, [lr, r2, lsl #2] */
		write32(0xe18c0330); /* orr	r0, ip, r0, lsr r3 */
		write32(B_FWD(4)); /* fin */
		extern u32 LWR_MASK[4]; write32((u32)LWR_MASK);
		extern u32 LWR_SHIFT[4]; write32((u32)LWR_SHIFT);

		rt=WriteReg(_Rt_);
		MOV32RtoR(rt,HOST_a1);
	}
	else
	{
		if (IsConst(_Rs_))
			MOV32ItoR(HOST_a1, iRegs[_Rs_].k + _Imm_);
		else {
			u32 rs=ReadReg(_Rs_);
			MOV32RtoR(HOST_a1, rs);
			if (_Imm_) ADD32ItoR(HOST_a1, _Imm_);
		}
		write32(BIC_IMM(HOST_a1, HOST_a1, 3, 0));
		PSXHWREAD32();
	}
}

//REC_FUNC(SWL);
static void recSWL() {
// mem[Rs + Im] = Rt Merge mem[Rs + Im]

	if (IsConst(_Rs_)) {
		MOV32ItoR(HOST_a1, iRegs[_Rs_].k + _Imm_);
	} else {
		u32 rs=ReadReg(_Rs_);
		MOV32RtoR(HOST_a1, rs);
		if (_Imm_) ADD32ItoR(HOST_a1, _Imm_);
	}

	u32 temp=TempReg();
	write32(AND_IMM(temp, HOST_a1, 3, 0));
	write32(BIC_IMM(HOST_a1, HOST_a1, 3, 0));
	write32(0xe92d0001); /* stmdb	sp!, {r0} */
	PSXMEMREAD32();
	MOV32RtoR(HOST_a3,temp);
	
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_a2, rt);
	}

	write32(LDR_IMM(HOST_lr, HOST_pc, 20)); /* SWL_MASK */
	write32(0xe79ec102); /* ldr	ip, [lr, r2, lsl #2] */
	write32(LDR_IMM(HOST_lr, HOST_pc, 16)); /* SWL_SHIFT */
	write32(0xe00cc000); /* and	ip, ip, r0 */
	write32(0xe79e3102); /* ldr	r3, [lr, r2, lsl #2] */
	write32(0xe18c0331); /* orr	r0, ip, r1, lsr r3 */
	write32(B_FWD(4)); /* fin */
	extern u32 SWL_MASK[4]; write32((u32)SWL_MASK);
	extern u32 SWL_SHIFT[4]; write32((u32)SWL_SHIFT);
	
	MOV32RtoR(HOST_a2,HOST_a1);
	write32(0xe8bd0001); /* ldmia	sp!, {r0} */
	PSXMEMWRITE32();
}

//REC_FUNC(SWR);
static void recSWR() {
// mem[Rs + Im] = Rt Merge mem[Rs + Im]

	if (IsConst(_Rs_)) {
		MOV32ItoR(HOST_a1, iRegs[_Rs_].k + _Imm_);
	} else {
		u32 rs=ReadReg(_Rs_);
		MOV32RtoR(HOST_a1, rs);
		if (_Imm_) ADD32ItoR(HOST_a1, _Imm_);
	}

	u32 temp=TempReg();
	write32(AND_IMM(temp, HOST_a1, 3, 0));
	write32(BIC_IMM(HOST_a1, HOST_a1, 3, 0));
	write32(0xe92d0001); /* stmdb	sp!, {r0} */
	PSXMEMREAD32();
	MOV32RtoR(HOST_a3,temp);
	
	if (IsConst(_Rt_)) {
		MOV32ItoR(HOST_a2, iRegs[_Rt_].k);
	} else {
		u32 rt=ReadReg(_Rt_);
		MOV32RtoR(HOST_a2, rt);
	}

	write32(LDR_IMM(HOST_lr, HOST_pc, 20)); /* SWR_MASK */
	write32(0xe79ec102); /* ldr	ip, [lr, r2, lsl #2] */
	write32(LDR_IMM(HOST_lr, HOST_pc, 16)); /* SWR_SHIFT */
	write32(0xe00cc000); /* and	ip, ip, r0 */
	write32(0xe79e3102); /* ldr	r3, [lr, r2, lsl #2] */
	write32(0xe18c0311); /* orr	r0, ip, r1, lsl r3 */
	write32(B_FWD(4)); /* fin */
	extern u32 SWR_MASK[4]; write32((u32)SWR_MASK);
	extern u32 SWR_SHIFT[4]; write32((u32)SWR_SHIFT);
	
	MOV32RtoR(HOST_a2,HOST_a1);
	write32(0xe8bd0001); /* ldmia	sp!, {r0} */
	PSXMEMWRITE32();
}
