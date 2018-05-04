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

/* asm void recRun(unsigned int ptr, unsigned int regs) */
.text
.align 4
.globl recRun
.globl _gteINTPL_s0_
.globl _gteINTPL_s12_
.globl _gteINTPL_block_
.globl _gteNCLIP_
.globl _gteMVMVA_cv0_mx0_s12_
.globl _gteMVMVA_cv0_mx0_s0_
.globl _gteMVMVA_cv0_mx1_s12_
.globl _gteMVMVA_cv0_mx1_s0_
.globl _gteMVMVA_cv0_mx2_s12_
.globl _gteMVMVA_cv0_mx2_s0_
.globl _gteMVMVA_cv0_mx3_s12_
.globl _gteMVMVA_cv0_mx3_s0_
.globl _gteMVMVA_cv1_mx0_s12_
.globl _gteMVMVA_cv1_mx0_s0_
.globl _gteMVMVA_cv1_mx1_s12_
.globl _gteMVMVA_cv1_mx1_s0_
.globl _gteMVMVA_cv1_mx2_s12_
.globl _gteMVMVA_cv1_mx2_s0_
.globl _gteMVMVA_cv1_mx3_s12_
.globl _gteMVMVA_cv1_mx3_s0_
.globl _gteMVMVA_cv2_mx0_s12_
.globl _gteMVMVA_cv2_mx0_s0_
.globl _gteMVMVA_cv2_mx1_s12_
.globl _gteMVMVA_cv2_mx1_s0_
.globl _gteMVMVA_cv2_mx2_s12_
.globl _gteMVMVA_cv2_mx2_s0_
.globl _gteMVMVA_cv2_mx3_s12_
.globl _gteMVMVA_cv2_mx3_s0_
.globl _gteMVMVA_cv3_mx0_s12_
.globl _gteMVMVA_cv3_mx0_s0_
.globl _gteMVMVA_cv3_mx1_s12_
.globl _gteMVMVA_cv3_mx1_s0_
.globl _gteMVMVA_cv3_mx2_s12_
.globl _gteMVMVA_cv3_mx2_s0_
.globl _gteMVMVA_cv3_mx3_s12_
.globl _gteMVMVA_cv3_mx3_s0_
.globl _gteOP_s12_
.globl _gteOP_s0_
.globl _gteDPCS_s12_
.globl _gteDPCS_s0_
.globl _gteGPF_s12_
.globl _gteGPF_s0_
.globl _gteSQR_s12_
.globl _gteSQR_s0_
.globl _gteDCPL__
.globl _gteGPL_s12_
.globl _gteGPL_s0_
.globl _gteRTPS__
.globl _gteNCDS__
.globl _gteNCDT__
.globl _gteCDP__
.globl _gteNCCS__
.globl _gteCC__
.globl _gteNCS__
.globl _gteNCT__
.globl _gteDPCT__
.globl _gteNCCT__
.globl _gteRTPT__
.globl _gteAVSZ3__
.globl _gteAVSZ4__
.extern __gte_reciprocals__

recRun:
stmfd sp!, {lr}
stmfd sp!, {r4-r11}
	
/* execute code */
mov v8, r1
mov pc, r0
nop
nop
nop
nop

.align 4
_gteINTPL_s0_:
	ldr	r1, ._gteINTPL_ffff8000
	add	r4, r11, #392
	mov	r2, #300
	ldrsh	r6, [r11, r2]
	ldr	r5, [r4, #84]
	mov	ip, #296
	ldr	r2, ._gteINTPL_00007fff
	ldrsh	r7, [r11, ip]
	rsb	r0, r6, r5
	cmp	r0, r1
	movlt	r0, r1
	mov 	r3, r6, lsl #12
	cmp	r0, r2
	movge	r0, r2
	mla	r0, r7, r0, r3
	mov	r6, #304
	add	r3, r11, #264
	str	r0, [r3, #100]
	ldrsh	r5, [r11, r6]
	ldr	r7, [r4, #88]
	ldrsh	r0, [r11, ip]
	rsb	r7, r5, r7
	cmp	r7, r1
	movlt	r7, r1
	cmp	r7, r2
	movge	r7, r2
	mov	r6, r5, lsl #12
	mla	r5, r0, r7, r6
	str	r5, [r3, #104]
	mov	r5, #308
	ldrsh	r5, [r11, r5]
	ldr	r4, [r4, #92]
	ldrsh	r0, [r11, ip]
	rsb	ip, r5, r4
	cmp	ip, r1
	movge	r1, ip
	movlt	r1, r1
	cmp	r1, r2
	movlt	r2, r1
	movge	r2, r2
	mov	r5, r5, lsl #12
	mla	r2, r0, r2, r5
 	str	r2, [r3, #108]
	bx	lr

_gteINTPL_s12_:
	ldr	r1, ._gteINTPL_ffff8000
	add	r4, r11, #392
	mov	r2, #300
	ldrsh	r6, [r11, r2]
	ldr	r0, [r4, #84]
	ldr	r2, ._gteINTPL_00007fff
	mov	ip, #296
	ldrsh	r7, [r11, ip]
	rsb	r5, r6, r0
	cmp	r5, r1
	movlt	r5, r1
	mov 	r3, r6, lsl #12
	cmp	r5, r2
	movge	r5, r2
	mla	r5, r7, r5, r3
	mov	r6, #304
	add	r3, r11, #264
	mov	r0, r5, asr #12
	str	r0, [r3, #100]
	ldrsh	r5, [r11, r6]
	ldr	r7, [r4, #88]
	ldrsh	r0, [r11, ip]
	rsb	r7, r5, r7
	cmp	r7, r1
	movlt	r7, r1
	cmp	r7, r2
	movge	r7, r2
	mov	r6, r5, lsl #12
	mla	r5, r0, r7, r6
	mov	r5, r5, asr #12
	str	r5, [r3, #104]
	mov	r5, #308
	ldrsh	r5, [r11, r5]
	ldr	r4, [r4, #92]
	ldrsh	r0, [r11, ip]
	rsb	ip, r5, r4
	cmp	ip, r1
	movge	r1, ip
	movlt	r1, r1
	cmp	r1, r2
	movlt	ip, r1
	movge	ip, r2
	mov	r5, r5, lsl #12
	mla	ip, r0, ip, r5
	mov	r2, ip, asr #12
	str	r2, [r3, #108]
	bx	lr

_gteINTPL_block_:
	ldr     r3, ._gteINTPL_ffff8000
	ldr     ip, ._gteINTPL_00007fff
	rsb	r2, r0, r2
	cmp	r2, r3
	movge	r3, r2
	movlt	r3, r3
	cmp	r3, ip
	movge	r3, ip
	mov	r0, r0, lsl #12
	mla	r0, r1, r3, r0
	bx	lr

._gteINTPL_ffff8000:	.long	0xffff8000
._gteINTPL_00007fff:	.long	0x00007fff

.align 4
_gteNCLIP_:
	ldr	r0, [r11, #0x13c]
	ldr	r2, [r11, #0x140]
	ldr	ip, [r11, #0x138]
	mov	r1, ip, lsr #16
	mov	r3, r0, lsr #16
	mov 	r0, r2, lsr #16
	rsb	r2, r1, r0
	rsb	r0, r0, r3
	smulbb	ip, r0, ip
	ldr	r0, [r11, #0x13c]
	smulbb	r0, r2, r0
	qadd	r0, r0, ip
	ldr	r2, [r11, #0x140]
	rsb	r3, r3, r1
	smulbb	r1, r3, r2
	qadd	r2, r1, r0
	str	r2, [r11, #0x168]
	bx	lr

.align 4
_gteMVMVA_cv0_mx0_s12_:
	mov	r0, #0x188
 	add	r3, r0, #2
	ldrsh	ip, [r11, r0]
	ldrsh	r0, [r11, r3]
	mov 	r4, r1, lsl #16
	mov	r4, r4, asr #16
	mov	r1, r1, asr #16
	mul	ip, r4, ip
	mul	r0, r1, r0
	qadd	ip, r0, ip
	mov	r3, #0x18c
	ldrsh	r0, [r11, r3]
	mul	r0, r2, r0
	qadd	ip, r0, ip
	ldr	r0, [r11, #0x19c]
	mov 	r3, ip, asr #12
	qadd	ip, r0, r3
	add	r3, r11, #0x108
	str	ip, [r3, #0x64]
	ldr	ip, ._gteMVMVA_cv0_mx0_0000018e
	ldrsh	r0, [r11, ip]
	mov	ip, #0x190
	ldrsh	ip, [r11, ip]
	mul	r0, r4, r0
	mul	ip, r1, ip
	qadd	ip, ip, r0
	ldr	r0, ._gteMVMVA_cv0_mx0_00000192
	ldrsh	r0, [r11, r0]
	mul	r0, r2, r0
	qadd	ip, r0, ip
	ldr	r0, [r11, #0x1a0]
	mov 	ip, ip, asr #12
	qadd	ip, r0, ip
	str	ip, [r3, #0x68]
	mov	ip, #0x194
	ldrsh	r0, [r11, ip]
	add	ip, ip, #2
	ldrsh	ip, [r11, ip]
	mul	r4, r0, r4
	mul	r1, ip, r1
	qadd	r1, r1, r4
	mov	ip, #0x198
	ldrsh	ip, [r11, ip]
	mul	r2, ip, r2
	qadd	ip, r2, r1
	ldr	r0, [r11, #0x1a4]
	mov	r1, ip, asr #12
	qadd	r2, r0, r1
	str	r2, [r3, #0x6c]
	bx	lr

_gteMVMVA_cv0_mx0_s0_:
	mov	r0, #0x188
	add	r3, r0, #2
	ldrsh	ip, [r11, r0]
	ldrsh	r0, [r11, r3]
	mov	r4, r1, lsl #16
	mov	r4, r4, asr #16
	mov	r1, r1, asr #16
	mul	ip, r4, ip
	mul	r0, r1, r0
	qadd	ip, r0, ip
	mov	r3, #0x18c
	ldrsh	r0, [r11, r3]
	mul	r0, r2, r0
	qadd	r3, r0, ip
	ldr	r0, [r11, #0x19c]
	mov	r0, r0, lsl #12
	qadd	ip, r0, r3
	add	r3, r11, #0x108
	str	ip, [r3, #0x64]
	ldr	ip, ._gteMVMVA_cv0_mx0_0000018e
	ldrsh	r0, [r11, ip]
	mov	ip, #0x190
	ldrsh	ip, [r11, ip]
	mul	r0, r4, r0
	mul	ip, r1, ip
	qadd	ip, ip, r0
	ldr	r0, ._gteMVMVA_cv0_mx0_00000192
	ldrsh	r0, [r11, r0]
	mul	r0, r2, r0
	qadd	ip, r0, ip
	ldr	r0, [r11, #0x1a0]
	mov	r0, r0, lsl #12
	qadd	ip, r0, ip
	str	ip, [r3, #0x68]
	mov	ip, #0x194
	ldrsh	r0, [r11, ip]
	add	ip, ip, #2
	ldrsh	ip, [r11, ip]
	mul	r4, r0, r4
	mul	r1, ip, r1
	qadd	r1, r1, r4
	mov	ip, #0x198
	ldrsh	ip, [r11, ip]
	mul	r2, ip, r2
	qadd	ip, r2, r1
	ldr	r1, [r11, #0x1a4]
	mov	r0, r1, lsl #12
	qadd	r2, r0, ip
	str	r2, [r3, #0x6c]
	bx	lr

._gteMVMVA_cv0_mx0_0000018e: 	.long 0x0000018e
._gteMVMVA_cv0_mx0_00000192:	.long 0x00000192

_gteMVMVA_cv0_mx1_s12_:
	mov	r0, #0x1a8
	add	r3, r0, #2
	ldrsh	ip, [r11, r0]
	ldrsh	r0, [r11, r3]
	mov	r4, r1, lsl #16
	mov	r4, r4, asr #16
	mov	r1, r1, asr #16
	mul	ip, r4, ip
	mul	r0, r1, r0
	qadd	ip, r0, ip
	mov	r3, #0x1ac
	ldrsh	r0, [r11, r3]
	mul	r0, r2, r0
	qadd	ip, r0, ip
	ldr	r0, [r11, #0x19c]
	mov	r3, ip, asr #12
	qadd	ip, r0, r3
	add	r3, r11, #0x108
	str	ip, [r3, #0x64]
	ldr	ip, ._gteMVMVA_cv0_mx1_000001ae
	ldrsh	r0, [r11, ip]
	mov	ip, #0x1b0
	ldrsh	ip, [r11, ip]
	mul	r0, r4, r0
	mul	ip, r1, ip
	qadd	ip, ip, r0
	ldr	r0, ._gteMVMVA_cv0_mx1_000001b2
	ldrsh	r0, [r11, r0]
	mul	r0, r2, r0
	qadd	ip, r0, ip
	ldr	r0, [r11, #0x1a0]
	mov	ip, ip, asr #12
	qadd	ip, r0, ip
	str	ip, [r3, #0x68]
	mov	ip, #0x1b4
	ldrsh	r0, [r11, ip]
	add	ip, ip, #2
	ldrsh	ip, [r11, ip]
	mul	r4, r0, r4
	mul	r1, ip, r1
	qadd	r1, r1, r4
	mov	ip, #0x1b8
	ldrsh	ip, [r11, ip]
	mul	r2, ip, r2
	qadd	ip, r2, r1
	ldr	r0, [r11, #0x1a4]
	mov	r1, ip, asr #12
	qadd	r2, r0, r1
	str	r2, [r3, #0x6c]
	bx	lr

_gteMVMVA_cv0_mx1_s0_:
	mov	r0, #0x1a8
	add	r3, r0, #2
	ldrsh	ip, [r11, r0]
	ldrsh	r0, [r11, r3]
	mov	r4, r1, lsl #16
	mov	r4, r4, asr #16
	mov	r1, r1, asr #16
	mul	ip, r4, ip
	mul	r0, r1, r0
	qadd	ip, r0, ip
	mov	r3, #0x1ac
	ldrsh	r0, [r11, r3]
	mul	r0, r2, r0
	qadd	r3, r0, ip
	ldr	r0, [r11, #0x19c]
	mov	r0, r0, lsl #12
	qadd	ip, r0, r3
	add	r3, r11, #0x108
	str	ip, [r3, #0x64]
	ldr	ip, ._gteMVMVA_cv0_mx1_000001ae
	ldrsh	r0, [r11, ip]
	mov	ip, #0x1b0
	ldrsh	ip, [r11, ip]
	mul	r0, r4, r0
	mul	ip, r1, ip
	qadd	ip, ip, r0
	ldr	r0, ._gteMVMVA_cv0_mx1_000001b2
	ldrsh	r0, [r11, r0]
	mul	r0, r2, r0
	qadd	ip, r0, ip
	ldr	r0, [r11, #0x1a0]
	mov	r0, r0, lsl #12
	qadd	ip, r0, ip
	str	ip, [r3, #0x68]
	mov	ip, #0x1b4
	ldrsh	r0, [r11, ip]
	add	ip, ip, #2
	ldrsh	ip, [r11, ip]
	mul	r4, r0, r4
	mul	r1, ip, r1
	qadd	r1, r1, r4
	mov	ip, #0x1b8
	ldrsh	ip, [r11, ip]
	mul	r2, ip, r2
	qadd	ip, r2, r1
	ldr	r1, [r11, #0x1a4]
	mov	r0, r1, lsl #12
	qadd	r2, r0, ip
	str	r2, [r3, #0x6c]
	bx	lr

._gteMVMVA_cv0_mx1_000001ae:	.long 0x000001ae
._gteMVMVA_cv0_mx1_000001b2:	.long 0x000001b2

_gteMVMVA_cv0_mx2_s12_:
	mov	r0, #0x1c8
	add	r3, r0, #2
	ldrsh	ip, [r11, r0]
	ldrsh	r0, [r11, r3]
	mov	r4, r1, lsl #16
	mov	r4, r4, asr #16
	mov	r1, r1, asr #16
	mul	ip, r4, ip
	mul	r0, r1, r0
	qadd	ip, r0, ip
	mov	r3, #0x1cc
	ldrsh	r0, [r11, r3]
	mul	r0, r2, r0
	qadd	ip, r0, ip
	ldr	r0, [r11, #0x19c]
	mov	r3, ip, asr #12
	qadd	ip, r0, r3
	add	r3, r11, #0x108
	str	ip, [r3, #0x64]
	ldr	ip, ._gteMVMVA_cv0_mx2_000001ce
	ldrsh	r0, [r11, ip]
	mov	ip, #0x1d0
	ldrsh	ip, [r11, ip]
	mul	r0, r4, r0
	mul	ip, r1, ip
	qadd	ip, ip, r0
	ldr	r0, ._gteMVMVA_cv0_mx2_000001d2
	ldrsh	r0, [r11, r0]
	mul	r0, r2, r0
	qadd	ip, r0, ip
	ldr	r0, [r11, #0x1a0]
	mov	ip, ip, asr #12
	qadd	ip, r0, ip
	str	ip, [r3, #0x68]
	mov	ip, #0x1d4
	ldrsh	r0, [r11, ip]
	add	ip, ip, #2
	ldrsh	ip, [r11, ip]
	mul	r4, r0, r4
	mul	r1, ip, r1
	qadd	r1, r1, r4
	mov	ip, #0x1d8
	ldrsh	ip, [r11, ip]
	mul	r2, ip, r2
	qadd	ip, r2, r1
	ldr	r0, [r11, #0x1a4]
	mov	r1, ip, asr #12
	qadd	r2, r0, r1
	str	r2, [r3, #0x6c]
	bx	lr

_gteMVMVA_cv0_mx2_s0_:
	mov	r0, #0x1c8
	add	r3, r0, #2
	ldrsh	ip, [r11, r0]
	ldrsh	r0, [r11, r3]
	mov	r4, r1, lsl #16
	mov	r4, r4, asr #16
	mov	r1, r1, asr #16
	mul	ip, r4, ip
	mul	r0, r1, r0
	qadd	ip, r0, ip
	mov	r3, #0x1cc
	ldrsh	r0, [r11, r3]
	mul	r0, r2, r0
	qadd	r3, r0, ip
	ldr	r0, [r11, #0x19c]
	mov	r0, r0, lsl #12
	qadd	ip, r0, r3
	add	r3, r11, #0x108
	str	ip, [r3, #0x64]
	ldr	ip, ._gteMVMVA_cv0_mx2_000001ce
	ldrsh	r0, [r11, ip]
	mov	ip, #0x1d0
	ldrsh	ip, [r11, ip]
	mul	r0, r4, r0
	mul	ip, r1, ip
	qadd	ip, ip, r0
	ldr	r0, ._gteMVMVA_cv0_mx2_000001d2
	ldrsh	r0, [r11, r0]
	mul	r0, r2, r0
	qadd	ip, r0, ip
	ldr	r0, [r11, #0x1a0]
	mov	r0, r0, lsl #12
	qadd	ip, r0, ip
	str	ip, [r3, #0x68]
	mov	ip, #0x1d4
	ldrsh	r0, [r11, ip]
	add	ip, ip, #2
	ldrsh	ip, [r11, ip]
	mul	r4, r0, r4
	mul	r1, ip, r1
	qadd	r1, r1, r4
	mov	ip, #0x1d8
	ldrsh	ip, [r11, ip]
	mul	r2, ip, r2
	qadd	ip, r2, r1
	ldr	r1, [r11, #0x1a4]
	mov	r0, r1, lsl #12
	qadd	r2, r0, ip
	str	r2, [r3, #0x6c]
	bx	lr

._gteMVMVA_cv0_mx2_000001ce:	.long	0x000001ce
._gteMVMVA_cv0_mx2_000001d2:	.long	0x000001d2

_gteMVMVA_cv0_mx3_s12_:
	ldr	ip, [r11, #0x19c]
	add	r3, r11, #0x108
	str	ip, [r3, #0x64]
	ldr	r1, [r11, #0x1a0]
	str	r1, [r3, #0x68]
	ldr	r2, [r11, #0x1a4]
	str	r2, [r3, #0x6c]
	bx	lr

_gteMVMVA_cv0_mx3_s0_:
	ldr	ip, [r11, #0x19c]
	add	r3, r11, #0x108
	mov	r1, ip, lsl #12
	str	r1, [r3, #0x64]
	ldr	r2, [r11, #0x1a0]
	mov	ip, r2, lsl #12
	str	ip, [r3, #0x68]
	ldr	r1, [r11, #0x1a4]
	mov	r2, r1, lsl #12
	str	r2, [r3, #0x6c]
	bx	lr

_gteMVMVA_cv1_mx0_s12_:	       
	mov	r0, #0x188      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x18c      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1bc]     
	mov	r3, ip, asr #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	ip, ._gteMVMVA_cv1_mx0_0000018e      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x190      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv1_mx0_00000192      
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1c0]     
	mov	ip, ip, asr #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x194      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x198      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r0, [r11, #0x1c4]     
	mov	r1, ip, asr #12    
	qadd	r2, r0, r1     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
_gteMVMVA_cv1_mx0_s0_:	       
	mov	r0, #0x188      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x18c      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	r3, r0, ip     
	ldr	r0, [r11, #0x1bc]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	ip, ._gteMVMVA_cv1_mx0_0000018e      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x190      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv1_mx0_00000192      
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1c0]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x194      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x198      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r1, [r11, #0x1c4]     
	mov	r0, r1, lsl #12    
	qadd	r2, r0, ip     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
._gteMVMVA_cv1_mx0_0000018e:	.long 0x0000018e      
._gteMVMVA_cv1_mx0_00000192:	.long 0x00000192      
		       
_gteMVMVA_cv1_mx1_s12_:	       
	mov	r0, #0x1a8      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x1ac      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1bc]     
	mov	r3, ip, asr #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	ip, ._gteMVMVA_cv1_mx1_000001ae      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x1b0      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv1_mx1_000001b2      
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1c0]     
	mov	ip, ip, asr #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x1b4      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x1b8      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r0, [r11, #0x1c4]     
	mov	r1, ip, asr #12    
	qadd	r2, r0, r1     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
_gteMVMVA_cv1_mx1_s0_:	       
	mov	r0, #0x1a8      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x1ac      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	r3, r0, ip     
	ldr	r0, [r11, #0x1bc]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]
	ldr	ip, ._gteMVMVA_cv1_mx1_000001ae      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x1b0      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv1_mx1_000001b2      
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1c0]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x1b4      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x1b8      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r1, [r11, #0x1c4]     
	mov	r0, r1, lsl #12    
	qadd	r2, r0, ip     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
._gteMVMVA_cv1_mx1_000001ae:	.long 0x000001ae      
._gteMVMVA_cv1_mx1_000001b2:	.long 0x000001b2      
		       
_gteMVMVA_cv1_mx2_s12_:	       
	mov	r0, #0x1c8      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x1cc      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1bc]     
	mov	r3, ip, asr #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	ip, ._gteMVMVA_cv1_mx2_000001ce      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x1d0      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv1_mx2_000001d2      
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1c0]     
	mov	ip, ip, asr #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x1d4      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x1d8      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r0, [r11, #0x1c4]     
	mov	r1, ip, asr #12    
	qadd	r2, r0, r1     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
_gteMVMVA_cv1_mx2_s0_:	       
	mov	r0, #0x1c8      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x1cc      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	r3, r0, ip     
	ldr	r0, [r11, #0x1bc]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	ip, ._gteMVMVA_cv1_mx2_000001ce      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x1d0      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv1_mx2_000001d2      
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1c0]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x1d4      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x1d8      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r1, [r11, #0x1c4]     
	mov	r0, r1, lsl #12    
	qadd	r2, r0, ip     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
._gteMVMVA_cv1_mx2_000001ce:	.long 0x000001ce      
._gteMVMVA_cv1_mx2_000001d2:	.long 0x000001d2      
		       
_gteMVMVA_cv1_mx3_s12_:	       
	ldr	ip, [r11, #0x1bc]     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	r1, [r11, #0x1c0]     
	str	r1, [r3, #0x68]     
	ldr	r2, [r11, #0x1c4]     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
_gteMVMVA_cv1_mx3_s0_:	       
	ldr	ip, [r11, #0x1bc]     
	add	r3, r11, #0x108     
	mov	r1, ip, lsl #12    
	str	r1, [r3, #0x64]     
	ldr	r2, [r11, #0x1c0]     
	mov	ip, r2, lsl #12    
	str	ip, [r3, #0x68]     
	ldr	r1, [r11, #0x1c4]     
	mov	r2, r1, lsl #12    
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
_gteMVMVA_cv2_mx0_s12_:	       
	mov	r0, #0x188      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x18c      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1dc]     
	mov	r3, ip, asr #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	ip, ._gteMVMVA_cv2_mx0_0000018e      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x190      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv2_mx0_00000192
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1e0]     
	mov	ip, ip, asr #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x194      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x198      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r0, [r11, #0x1e4]     
	mov	r1, ip, asr #12    
	qadd	r2, r0, r1     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
_gteMVMVA_cv2_mx0_s0_:	       
	mov	r0, #0x188      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x18c      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	r3, r0, ip     
	ldr	r0, [r11, #0x1dc]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	ip, ._gteMVMVA_cv2_mx0_0000018e      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x190      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv2_mx0_00000192
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1e0]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x194      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x198      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r1, [r11, #0x1e4]     
	mov	r0, r1, lsl #12    
	qadd	r2, r0, ip     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
._gteMVMVA_cv2_mx0_0000018e:	.long 0x0000018e      
._gteMVMVA_cv2_mx0_00000192:	.long 0x00000192      
		       
_gteMVMVA_cv2_mx1_s12_:	       
	mov	r0, #0x1a8      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x1ac      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1dc]     
	mov	r3, ip, asr #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]
	ldr	ip, ._gteMVMVA_cv2_mx1_000001ae      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x1b0      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv2_mx1_000001b2      
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1e0]     
	mov	ip, ip, asr #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x1b4      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x1b8      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r0, [r11, #0x1e4]     
	mov	r1, ip, asr #12    
	qadd	r2, r0, r1     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
_gteMVMVA_cv2_mx1_s0_:	       
	mov	r0, #0x1a8      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x1ac      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	r3, r0, ip     
	ldr	r0, [r11, #0x1dc]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108
	str	ip, [r3, #0x64]     
	ldr	ip, ._gteMVMVA_cv2_mx1_000001ae      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x1b0      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv2_mx1_000001b2      
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1e0]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x1b4      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x1b8      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r1, [r11, #0x1e4]     
	mov	r0, r1, lsl #12    
	qadd	r2, r0, ip     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
._gteMVMVA_cv2_mx1_000001ae:	.long 0x000001ae      
._gteMVMVA_cv2_mx1_000001b2:	.long 0x000001b2      
		       
_gteMVMVA_cv2_mx2_s12_:	       
	mov	r0, #0x1c8      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x1cc      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1dc]     
	mov	r3, ip, asr #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	ip, ._gteMVMVA_cv2_mx2_000001ce      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x1d0      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv2_mx2_000001d2      
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1e0]     
	mov	ip, ip, asr #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x1d4      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x1d8      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r0, [r11, #0x1e4]     
	mov	r1, ip, asr #12    
	qadd	r2, r0, r1     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
_gteMVMVA_cv2_mx2_s0_:	       
	mov	r0, #0x1c8      
	add	r3, r0, #2     
	ldrsh	ip, [r11, r0]     
	ldrsh	r0, [r11, r3]     
	mov	r4, r1, lsl #16    
	mov	r4, r4, asr #16    
	mov	r1, r1, asr #16    
	mul	ip, r4, ip     
	mul	r0, r1, r0     
	qadd	ip, r0, ip     
	mov	r3, #0x1cc      
	ldrsh	r0, [r11, r3]     
	mul	r0, r2, r0     
	qadd	r3, r0, ip     
	ldr	r0, [r11, #0x1dc]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, r3     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	ip, ._gteMVMVA_cv2_mx2_000001ce      
	ldrsh	r0, [r11, ip]     
	mov	ip, #0x1d0      
	ldrsh	ip, [r11, ip]     
	mul	r0, r4, r0     
	mul	ip, r1, ip     
	qadd	ip, ip, r0     
	ldr	r0, ._gteMVMVA_cv2_mx2_000001d2      
	ldrsh	r0, [r11, r0]     
	mul	r0, r2, r0     
	qadd	ip, r0, ip     
	ldr	r0, [r11, #0x1e0]     
	mov	r0, r0, lsl #12    
	qadd	ip, r0, ip     
	str	ip, [r3, #0x68]     
	mov	ip, #0x1d4      
	ldrsh	r0, [r11, ip]     
	add	ip, ip, #2     
	ldrsh	ip, [r11, ip]     
	mul	r4, r0, r4     
	mul	r1, ip, r1     
	qadd	r1, r1, r4     
	mov	ip, #0x1d8      
	ldrsh	ip, [r11, ip]     
	mul	r2, ip, r2     
	qadd	ip, r2, r1     
	ldr	r1, [r11, #0x1e4]     
	mov	r0, r1, lsl #12    
	qadd	r2, r0, ip     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
._gteMVMVA_cv2_mx2_000001ce:	.long 0x000001ce      
._gteMVMVA_cv2_mx2_000001d2:	.long 0x000001d2      
		       
_gteMVMVA_cv2_mx3_s12_:	       
	ldr	ip, [r11, #0x1dc]     
	add	r3, r11, #0x108     
	str	ip, [r3, #0x64]     
	ldr	r1, [r11, #0x1e0]     
	str	r1, [r3, #0x68]     
	ldr	r2, [r11, #0x1e4]     
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
_gteMVMVA_cv2_mx3_s0_:	       
	ldr	ip, [r11, #0x1dc]     
	add	r3, r11, #0x108
	mov	r1, ip, lsl #12    
	str	r1, [r3, #0x64]     
	ldr	r2, [r11, #0x1e0]     
	mov	ip, r2, lsl #12    
	str	ip, [r3, #0x68]     
	ldr	r1, [r11, #0x1e4]     
	mov	r2, r1, lsl #12    
	str	r2, [r3, #0x6c]     
	bx	lr       
		       
_gteMVMVA_cv3_mx0_s12_:	        
	mov	r0, #0x188       
	add	r3, r0, #2      
	ldrsh	ip, [r11, r0]      
	ldrsh	r0, [r11, r3]      
	mov	r4, r1, lsl #16      
	mov 	r4, r4, asr #16      
	mov	r1, r1, asr #16      
	mul	ip, r4, ip      
	mul	r0, r1, r0      
	qadd	ip, r0, ip      
	mov	r3, #0x18c       
	ldrsh	r0, [r11, r3]      
	mul	r0, r2, r0      
	qadd	r3, r0, ip      
	ldr	ip, ._gteMVMVA_cv3_mx0_0000018e       
	mov	r0, r3, asr #12      
	add	r3, r11, #0x108      
	str	r0, [r3, #0x64]      
	ldrsh	r0, [r11, ip]      
	mov	ip, #0x190       
	ldrsh	ip, [r11, ip]      
	mul	r0, r4, r0      
	mul	ip, r1, ip      
	qadd	ip, ip, r0      
	ldr	r0, ._gteMVMVA_cv3_mx0_00000192       
	ldrsh	r0, [r11, r0]      
	mul	r0, r2, r0      
	qadd	ip, r0, ip      
	mov	r0, ip, asr #12      
	mov	ip, #0x194       
	str	r0, [r3, #0x68]      
	ldrsh	r0, [r11, ip]      
	add	ip, ip, #2      
	ldrsh	ip, [r11, ip]      
	mul	r4, r0, r4      
	mul	r1, ip, r1      
	qadd	r1, r1, r4      
	mov	ip, #0x198       
	ldrsh	r0, [r11, ip]      
	mul	r2, r0, r2      
	qadd	r0, r2, r1      
	mov	r2, r0, asr #12      
	str	r2, [r3, #0x6c]      
	bx	lr        

_gteMVMVA_cv3_mx0_s0_:	        
	mov	r0, #0x188       
	add	r3, r0, #2      
	ldrsh	ip, [r11, r0]      
	ldrsh	r0, [r11, r3]      
	mov	r4, r1, lsl #16      
	mov	r4, r4, asr #16      
	mov	r1, r1, asr #16      
	mul	ip, r4, ip      
	mul	r0, r1, r0      
	qadd	ip, r0, ip      
	mov	r3, #0x18c       
	ldrsh	r0, [r11, r3]      
	mul	r0, r2, r0      
	qadd	ip, r0, ip      
	add	r3, r11, #0x108      
	str	ip, [r3, #0x64]      
	ldr	ip, ._gteMVMVA_cv3_mx0_0000018e       
	ldrsh	r0, [r11, ip]      
	mov	ip, #0x190       
	ldrsh	ip, [r11, ip]      
	mul	r0, r4, r0      
	mul	ip, r1, ip      
	qadd	ip, ip, r0      
	ldr	r0, ._gteMVMVA_cv3_mx0_00000192       
	ldrsh	r0, [r11, r0]      
	mul	r0, r2, r0      
	qadd	ip, r0, ip      
	str	ip, [r3, #0x68]      
	mov	ip, #0x194       
	ldrsh	r0, [r11, ip]      
	add	ip, ip, #2      
	ldrsh	ip, [r11, ip]      
	mul	r4, r0, r4      
	mul	r1, ip, r1      
	qadd	r1, r1, r4      
	mov	ip, #0x198       
	ldrsh	r0, [r11, ip]      
	mul	r2, r0, r2      
	qadd	r2, r2, r1      
	str	r2, [r3, #0x6c]      
	bx	lr        

._gteMVMVA_cv3_mx0_0000018e:	.long 0x0000018e       
._gteMVMVA_cv3_mx0_00000192:	.long 0x00000192       

_gteMVMVA_cv3_mx1_s12_:	        
	mov	r0, #0x1a8       
	add	r3, r0, #2      
	ldrsh	ip, [r11, r0]      
	ldrsh	r0, [r11, r3]      
	mov	r4, r1, lsl #16      
	mov	r4, r4, asr #16      
	mov	r1, r1, asr #16      
	mul	ip, r4, ip      
	mul	r0, r1, r0      
	qadd	ip, r0, ip      
	mov	r3, #0x1ac       
	ldrsh	r0, [r11, r3]      
	mul	r0, r2, r0      
	qadd	r3, r0, ip      
	ldr	ip, ._gteMVMVA_cv3_mx1_000001ae       
	mov	r0, r3, asr #12      
	add	r3, r11, #0x108      
	str	r0, [r3, #0x64]      
	ldrsh	r0, [r11, ip]      
	mov	ip, #0x1b0       
	ldrsh	ip, [r11, ip]      
	mul	r0, r4, r0      
	mul	ip, r1, ip      
	qadd	ip, ip, r0      
	ldr	r0, ._gteMVMVA_cv3_mx1_000001b2       
	ldrsh	r0, [r11, r0]      
	mul	r0, r2, r0      
	qadd	ip, r0, ip      
	mov	r0, ip, asr #12      
	mov	ip, #0x1b4       
	str	r0, [r3, #0x68]      
	ldrsh	r0, [r11, ip]      
	add	ip, ip, #2      
	ldrsh	ip, [r11, ip]      
	mul	r4, r0, r4      
	mul	r1, ip, r1      
	qadd	r1, r1, r4      
	mov	ip, #0x1b8       
	ldrsh	r0, [r11, ip]      
	mul	r2, r0, r2      
	qadd	r0, r2, r1      
	mov	r2, r0, asr #12      
	str	r2, [r3, #0x6c]      
	bx	lr        

_gteMVMVA_cv3_mx1_s0_:	        
	mov	r0, #0x1a8       
	add	r3, r0, #2      
	ldrsh	ip, [r11, r0]      
	ldrsh	r0, [r11, r3]      
	mov	r4, r1, lsl #16      
	mov	r4, r4, asr #16      
	mov	r1, r1, asr #16      
	mul	ip, r4, ip      
	mul	r0, r1, r0      
	qadd	ip, r0, ip      
	mov	r3, #0x1ac       
	ldrsh	r0, [r11, r3]      
	mul	r0, r2, r0      
	qadd	ip, r0, ip      
	add	r3, r11, #0x108      
	str	ip, [r3, #0x64]      
	ldr	ip, ._gteMVMVA_cv3_mx1_000001ae       
	ldrsh	r0, [r11, ip]      
	mov	ip, #0x1b0       
	ldrsh	ip, [r11, ip]      
	mul	r0, r4, r0      
	mul	ip, r1, ip      
	qadd	ip, ip, r0      
	ldr	r0, ._gteMVMVA_cv3_mx1_000001b2       
	ldrsh	r0, [r11, r0]      
	mul	r0, r2, r0      
	qadd	ip, r0, ip      
	str	ip, [r3, #0x68]      
	mov	ip, #0x1b4       
	ldrsh	r0, [r11, ip]      
	add	ip, ip, #2      
	ldrsh	ip, [r11, ip]      
	mul	r4, r0, r4      
	mul	r1, ip, r1      
	qadd	r1, r1, r4      
	mov	ip, #0x1b8       
	ldrsh	r0, [r11, ip]      
	mul	r2, r0, r2      
	qadd	r2, r2, r1      
	str	r2, [r3, #0x6c]      
	bx	lr        

._gteMVMVA_cv3_mx1_000001ae:	.long 0x000001ae       
._gteMVMVA_cv3_mx1_000001b2:	.long 0x000001b2       

_gteMVMVA_cv3_mx2_s12_:	        
	mov	r0, #0x1c8       
	add	r3, r0, #2      
	ldrsh	ip, [r11, r0]      
	ldrsh	r0, [r11, r3]      
	mov	r4, r1, lsl #16      
	mov	r4, r4, asr #16      
	mov	r1, r1, asr #16      
	mul	ip, r4, ip      
	mul	r0, r1, r0      
	qadd	ip, r0, ip      
	mov	r3, #0x1cc       
	ldrsh	r0, [r11, r3]      
	mul	r0, r2, r0      
	qadd	r3, r0, ip      
	ldr	ip, ._gteMVMVA_cv3_mx2_000001ce       
	mov	r0, r3, asr #12      
	add	r3, r11, #0x108      
	str	r0, [r3, #0x64]      
	ldrsh	r0, [r11, ip]      
	mov	ip, #0x1d0       
	ldrsh	ip, [r11, ip]      
	mul	r0, r4, r0      
	mul	ip, r1, ip      
	qadd	ip, ip, r0      
	ldr	r0, ._gteMVMVA_cv3_mx2_000001d2       
	ldrsh	r0, [r11, r0]      
	mul	r0, r2, r0      
	qadd	ip, r0, ip      
	mov	r0, ip, asr #12      
	mov	ip, #0x1d4       
	str	r0, [r3, #0x68]      
	ldrsh	r0, [r11, ip]      
	add	ip, ip, #2      
	ldrsh	ip, [r11, ip]      
	mul	r4, r0, r4      
	mul	r1, ip, r1      
	qadd	r1, r1, r4      
	mov	ip, #0x1d8       
	ldrsh	r0, [r11, ip]      
	mul	r2, r0, r2      
	qadd	r0, r2, r1      
	mov	r2, r0, asr #12      
	str	r2, [r3, #0x6c]      
	bx	lr        

_gteMVMVA_cv3_mx2_s0_:	        
	mov	r0, #0x1c8       
	add	r3, r0, #2      
	ldrsh	ip, [r11, r0]      
	ldrsh	r0, [r11, r3]      
	mov	r4, r1, lsl #16      
	mov	r4, r4, asr #16      
	mov	r1, r1, asr #16      
	mul	ip, r4, ip      
	mul	r0, r1, r0      
	qadd	ip, r0, ip      
	mov	r3, #0x1cc       
	ldrsh	r0, [r11, r3]      
	mul	r0, r2, r0      
	qadd	ip, r0, ip      
	add	r3, r11, #0x108      
	str	ip, [r3, #0x64]      
	ldr	ip, ._gteMVMVA_cv3_mx2_000001ce       
	ldrsh	r0, [r11, ip]      
	mov	ip, #0x1d0       
	ldrsh	ip, [r11, ip]      
	mul	r0, r4, r0      
	mul	ip, r1, ip      
	qadd	ip, ip, r0      
	ldr	r0, ._gteMVMVA_cv3_mx2_000001d2       
	ldrsh	r0, [r11, r0]      
	mul	r0, r2, r0      
	qadd	ip, r0, ip      
	str	ip, [r3, #0x68]      
	mov	ip, #0x1d4       
	ldrsh	r0, [r11, ip]      
	add	ip, ip, #2      
	ldrsh	ip, [r11, ip]      
	mul	r4, r0, r4      
	mul	r1, ip, r1      
	qadd	r1, r1, r4      
	mov	ip, #0x1d8       
	ldrsh	r0, [r11, ip]      
	mul	r2, r0, r2      
	qadd	r2, r2, r1      
	str	r2, [r3, #0x6c]      
	bx	lr        

._gteMVMVA_cv3_mx2_000001ce:	.long 0x000001ce       
._gteMVMVA_cv3_mx2_000001d2:	.long 0x000001d2       


_gteMVMVA_cv3_mx3_s12_:	        
_gteMVMVA_cv3_mx3_s0_:	        
	add	r11, r11, #0x108      
	mov	r3, #0       
	str	r3, [r11, #0x6c]      
	str	r3, [r11, #0x64]      
	str	r3, [r11, #0x68]      
	bx	lr        

.align 4
_gteOP_s12_:		     
	stmdb	sp!,	{r4, r5, r6, r7}  
	mov	ip,	#0x130     
	mov	r0,	#0x134     
	mov	r4,	#0x190     
	mov	r5,	#0x198     
	ldrh	r1,	[r11, r0]    
	ldrh	r2,	[r11, r4]    
	ldrh	r6,	[r11, ip]    
	ldrh	r7,	[r11, r5]    
	smulbb	r3,	r1, r2    
	smulbb	r1,	r6, r7    
	qsub	r2,	r3, r1    
	mov	r1,	#0x188     
	mov	r7,	r2, asr #12   
	add	r3,	r11, #0x108    
	mov	r2,	#0x12c     
	str	r7,	[r3, #0x64]    
	ldrh	r6,	[r11, r5]    
	ldrh	r7,	[r11, r1]    
	ldrh	r5,	[r11, r0]    
	ldrh	r0,	[r11, r2]    
	smulbb	r5,	r5, r7    
	smulbb	r6,	r0, r6    
	qsub	r5,	r6, r5    
	mov	r5,	r5, asr #12   
	str	r5,	[r3, #0x68]    
	ldrh	ip,	[r11, ip]    
	ldrh	r4,	[r11, r4]    
	ldrh	r1,	[r11, r1]    
	ldrh	r2,	[r11, r2]    
	smulbb	r1,	ip, r1    
	smulbb	ip,	r2, r4    
	qsub	r0,	r1, ip    
	mov	r2,	r0, asr #12   
	str	r2,	[r3, #0x6c]    
	ldmia	sp!,	{r4, r5, r6, r7}  
	bx	lr	     
			     
_gteOP_s0_:		     
	stmdb	sp!,	{r4, r5, r6, r7}  
	mov	ip,	#0x130     
	mov	r0,	#0x134     
	mov	r4,	#0x190     
	mov	r5,	#0x198     
	ldrh	r1,	[r11, r0]    
	ldrh	r7,	[r11, r4]    
	ldrh	r6,	[r11, ip]    
	ldrh	r2,	[r11, r5]    
	smulbb	r3,	r1, r7    
	smulbb	r1,	r6, r2    
	qsub	r7,	r3, r1    
	mov	r2,	#0x12c     
	add	r3,	r11, #0x108    
	mov	r1,	#0x188     
	str	r7,	[r3, #0x64]    
	ldrh	r6,	[r11, r5]    
	ldrh	r7,	[r11, r1]    
	ldrh	r5,	[r11, r0]    
	ldrh	r0,	[r11, r2]    
	smulbb	r5,	r5, r7    
	smulbb	r6,	r0, r6    
	qsub	r5,	r6, r5    
	str	r5,	[r3, #0x68]    
	ldrh	ip,	[r11, ip]    
	ldrh	r4,	[r11, r4]    
	ldrh	r1,	[r11, r1]    
	ldrh	r2,	[r11, r2]    
	smulbb	r1,	ip, r1    
	smulbb	r0,	r2, r4    
	qsub	r2,	r1, r0    
	str	r2,	[r3, #0x6c]    
	ldmia	sp!,	{r4, r5, r6, r7}  
	bx	lr	     

.align 4
_gteDPCS_s12_:
	stmdb	sp!,	{r4, r5, r6, r7}  
	ldrb	r0,	[r11, #0x120]    
	add	r4,	r11, #0x188    
	ldr	r2,	[r4, #0x54]    
	mov	r1,	r0, lsl #4   
	qsub	r5,	r2, r1    
	ldr	r1,	._gteDPCS_ffff8000     
	ldr	r2,	._gteDPCS_00007fff     
	mov	ip,	#0x128     
	ldrsh	r7,	[r11, ip]    
	cmp	r5,	r1     
	movlt	r5,	r1     
	mov	r3,	r0, lsl #16   
	cmp	r5,	r2     
	movge	r5,	r2     
	mla	r5,	r7, r5, r3   
	add	r3,	r11, #0x108    
	mov	r6,	r5, asr #12   
	str	r6,	[r3, #0x64]    
	ldrb	r5,	[r11, #0x121]    
	ldr	r0,	[r4, #0x58]    
	mov	r7,	r5, lsl #4   
	qsub	r7,	r0, r7    
	ldrsh	r0,	[r11, ip]    
	cmp	r7,	r1     
	movlt	r7,	r1     
	cmp	r7,	r2     
	movge	r7,	r2     
	mov	r6,	r5, lsl #16   
	mla	r5,	r0, r7, r6   
	mov	r6,	r5, asr #12   
	str	r6,	[r3, #0x68]    
	ldrb	r5,	[r11, #0x122]    
	ldr	r4,	[r4, #0x5c]    
	mov	r6,	r5, lsl #4   
	qsub	r4,	r4, r6    
	ldrsh	r0,	[r11, ip]    
	cmp	r4,	r1     
	movge	r1,	r4     
	movlt	r1,	r1     
	cmp	r1,	r2     
	movlt	ip,	r1     
	movge	ip,	r2     
	mov	r5,	r5, lsl #16   
	mla	ip,	r0, ip, r5   
	mov	r2,	ip, asr #12   
	str	r2,	[r3, #0x6c]    
	ldmia	sp!,	{r4, r5, r6, r7}  
	bx	lr	     
			     
_gteDPCS_s0_:
	stmdb	sp!,	{r4, r5, r6, r7}  
	ldrb	r6,	[r11, #0x120]    
	add	r4,	r11, #0x188
	ldr	r2,	[r4, #0x54]    
	mov	r1,	r6, lsl #4   
	qsub	r5,	r2, r1    
	ldr	r1,	._gteDPCS_ffff8000     
	ldr	r2,	._gteDPCS_00007fff     
	mov	ip,	#0x128     
	ldrsh	r0,	[r11, ip]    
	mov	r7,	r5, lsl #12   
	cmp	r7,	r1     
	movlt	r7,	r1     
	mov	r3,	r6, lsl #16   
	cmp	r7,	r2     
	movge	r7,	r2     
	mla	r7,	r0, r7, r3   
	add	r3,	r11, #0x108    
	mov	r5,	r7, asr #12   
	str	r5,	[r3, #0x64]    
	ldrb	r5,	[r11, #0x121]    
	ldr	r6,	[r4, #0x58]    
	mov	r0,	r5, lsl #4   
	qsub	r7,	r6, r0    
	ldrsh	r0,	[r11, ip]    
	mov	r7,	r7, lsl #12   
	cmp	r7,	r1     
	movlt	r7,	r1     
	cmp	r7,	r2     
	movge	r7,	r2     
	mov	r6,	r5, lsl #16   
	mla	r5,	r0, r7, r6   
	mov	r6,	r5, asr #12   
	str	r6,	[r3, #0x68]    
	ldrb	r5,	[r11, #0x122]    
	ldr	r4,	[r4, #0x5c]    
	mov	r6,	r5, lsl #4   
	qsub	r4,	r4, r6    
	ldrsh	r0,	[r11, ip]    
	mov	ip,	r4, lsl #12   
	cmp	ip,	r1     
	movge	r1,	ip     
	movlt	r1,	r1     
	cmp	r1,	r2     
	movlt	ip,	r1     
	movge	ip,	r2     
	mov	r5,	r5, lsl #16   
	mla	ip,	r0, ip, r5   
	mov	r2,	ip, asr #12   
	str	r2,	[r3, #0x6c]    
	ldmia	sp!,	{r4, r5, r6, r7}  
	bx	lr	     
._gteDPCS_ffff8000:	.long	0xffff8000     
._gteDPCS_00007fff:	.long	0x00007fff     
			     
.align 4
_gteGPF_s12_:
	mov	r2,	#0x128     
	mov	r3,	#0x12c     
	ldrh	ip,	[r11, r3]    
	ldrh	r1,	[r11, r2]    
	add	r3,	r11, #0x108    
	smulbb	r1,	ip, r1    
	mov	ip,	r1, asr #12   
	mov	r1,	#0x130     
	str	ip,	[r3, #0x64]    
	ldrh	ip,	[r11, r1]    
	ldrh	r1,	[r11, r2]    
	smulbb	r1,	ip, r1    
	mov	ip,	r1, asr #12   
	mov	r1,	#0x134     
	str	ip,	[r3, #0x68]    
	ldrh	ip,	[r11, r2]    
	ldrh	r1,	[r11, r1]    
	smulbb	r0,	r1, ip    
	mov	r2,	r0, asr #12   
	str	r2,	[r3, #0x6c]    
	bx	lr	     
			     
_gteGPF_s0_:
	mov	r2,	#0x128     
	mov	r3,	#0x12c     
	ldrh	ip,	[r11, r3]    
	ldrh	r1,	[r11, r2]    
	add	r3,	r11, #0x108    
	smulbb	r1,	ip, r1    
	str	r1,	[r3, #0x64]    
	mov	r1,	#0x130     
	ldrh	ip,	[r11, r1]    
	ldrh	r1,	[r11, r2]    
	smulbb	r1,	ip, r1    
	str	r1,	[r3, #0x68]    
	mov	r1,	#0x134     
	ldrh	ip,	[r11, r2]    
	ldrh	r1,	[r11, r1]    
	smulbb	r2,	r1, ip    
	str	r2,	[r3, #0x6c]    
	bx	lr	    

.align 4
_gteSQR_s12_:
	mov	r3,	#0x12c     
	ldrsh	r1,	[r11, r3]    
	add	r3,	r11, #0x108    
	mul	ip,	r1, r1    
	mov	r1,	ip, asr #12   
	mov	ip,	#0x130     
	str	r1,	[r3, #0x64]    
	ldrsh	r2,	[r11, ip]    
	mul	r1,	r2, r2    
	mov	r2,	#0x134     
	mov	r1,	r1, asr #12   
	str	r1,	[r3, #0x68]    
	ldrsh	ip,	[r11, r2]    
	mul	r1,	ip, ip    
	mov	r2,	r1, asr #12   
	str	r2,	[r3, #0x6c]    
	bx	lr	     
			     
.align 4
_gteSQR_s0_:
	mov	r3,	#0x12c     
	ldrsh	ip,	[r11, r3]    
	add	r3,	r11, #0x108    
	mul	r1,	ip, ip    
	mov	ip,	#0x130     
	str	r1,	[r3, #0x64]    
	ldrsh	r2,	[r11, ip]    
	mul	r1,	r2, r2    
	mov	r2,	#0x134     
	str	r1,	[r3, #0x68]    
	ldrsh	ip,	[r11, r2]    
	mul	r1,	ip, ip    
	str	r1,	[r3, #0x6c]    
	bx	lr	     

.align 4
_gteDCPL__:
	stmdb	sp!,	{r4, r5, r6, r7, r8} 
	ldrb	r2,	[r11, #0x120]    
	mov	r7,	#0x12c     
	ldrh	r1,	[r11, r7]    
	sub	sp,	sp, #8    
	smull	r6,	r7, r2, r1   
	mov	r4,	#0x130     
	mov	r3,	r6, lsr #8    
	ldrh	r8,	[r11, r4]    
	ldrb	r0,	[r11, #0x121]    
	orr	r2,	r3, r7, lsl #24  
	smull	r4,	r5, r0, r8   
	mov	r1,	r4     
	mov	r7,	r5     
	mov	r6,	#0x134     
	mov	r5,	r1, lsr #8    
	ldrh	ip,	[r11, r6]    
	ldrb	r8,	[r11, #0x122]    
	orr	r4,	r5, r7, lsl #24  
	smull	r6,	r7, r8, ip   
	add	r8,	r11, #0x188    
	ldr	r0,	[r8, #0x54]    
	strd	r6,	[sp]     
	ldr	r6,	._gteDCPL__ffff8000     
	ldr	ip,	._gteDCPL__00007fff     
	mov	r7,	#0x128     
	ldrsh	r1,	[r11, r7]    
	rsb	r0,	r2, r0    
	cmp	r0,	r6     
	movlt	r0,	r6     
	cmp	r0,	ip     
	movge	r0,	ip     
	mul	r0,	r1, r0    
	add	r1,	r11, #0x108    
	mov	r0,	r0, asr #12   
	adds	r2,	r2, r0    
	str	r2,	[r1, #0x64]    
	ldr	r2,	[r8, #0x58]    
	ldrsh	r3,	[r11, r7]    
	rsb	r0,	r4, r2    
	cmp	r0,	r6     
	movlt	r0,	r6     
	cmp	r0,	ip     
	movge	r0,	ip     
	mul	r0,	r3, r0    
	ldr	r5,	[sp]     
	mov	r0,	r0, asr #12   
	adds	r2,	r4, r0    
	mov	r3,	r5, lsr #8    
	ldr	r5,	[sp, #4]    
	str	r2,	[r1, #0x68]    
	ldr	r4,	[r8, #0x5c]    
	orr	r2,	r3, r5, lsl #24  
	ldrsh	r0,	[r11, r7]    
	rsb	r4,	r2, r4    
	cmp	r4,	r6     
	movge	r6,	r4     
	movlt	r6,	r6     
	cmp	r6,	ip     
	movlt	ip,	r6     
	movge	ip,	ip     
	mul	ip,	r0, ip    
	mov	ip,	ip, asr #12   
	adds	r2,	r2, ip    
	str	r2,	[r1, #0x6c]    
	add	sp,	sp, #8    
	ldmia	sp!,	{r4, r5, r6, r7, r8} 
	bx	lr	     
._gteDCPL__ffff8000:	.long	0xffff8000     
._gteDCPL__00007fff:	.long	0x00007fff     
			     
.align 4
_gteGPL_s12_:
	mov	r1,	#0x12c     
	mov	r3,	#0x128     
	ldrh	r2,	[r11, r1]    
	ldrh	r3,	[r11, r3]    
	ldr	r1,	[r11, #0x16c]    
	smulbb	ip,	r2, r3    
	mov	r2,	ip, asr #12   
	qadd	r2,	r1, r2    
	mov	ip,	#0x130     
	ldrh	r1,	[r11, ip]    
	str	r2,	[r11, #0x16c]    
	smulbb	ip,	r1, r3    
	ldr	r2,	[r11, #0x170]    
	mov	r1,	ip, asr #12   
	qadd	r1,	r2, r1    
	mov	ip,	#0x134     
	ldrh	r2,	[r11, ip]    
	str	r1,	[r11, #0x170]    
	smulbb	ip,	r2, r3    
	ldr	r2,	[r11, #0x174]    
	mov	r1,	ip, asr #12   
	qadd	r3,	r2, r1    
	str	r3,	[r11, #0x174]    
	bx	lr	     
			     
.align 4
_gteGPL_s0_:
	mov	ip,	#0x12c     
	mov	r3,	#0x128     
	ldrh	r2,	[r11, ip]    
	ldrh	ip,	[r11, r3]    
	ldr	r1,	[r11, #0x16c]    
	smulbb	r2,	r2, ip    
	qadd	r2,	r1, r2    
	mov	r1,	#0x130     
	ldrh	r1,	[r11, r1]    
	str	r2,	[r11, #0x16c]    
	smulbb	r1,	r1, ip    
	ldr	r2,	[r11, #0x170]    
	qadd	r1,	r2, r1    
	mov	r2,	#0x134     
	ldrh	r2,	[r11, r2]    
	str	r1,	[r11, #0x170]    
	smulbb	r1,	r2, ip    
	ldr	r2,	[r11, #0x174]    
	qadd	r3,	r2, r1    
	str	r3,	[r11, #0x174]    
	bx	lr	     

.align 4
_gteRTPS__:		     
/*			     
#ifdef USE_OLD_GTE_WITHOUT_PATCH
	mov	r2,	#0x188     
	sub	r3,	r2, #0x7e    
	stmdb   sp!,	{r4, r5, r6, r7, r8, r9, r10}
	ldrh	r6,	[r11, r2]    
	mov	r4,	#0x108     
	add	r7,	r3, #0x80    
	ldrh	ip,	[r11, r4]    
	ldrh	r2,	[r11, r3]    
	ldrh	r5,	[r11, r7]    
	mov	r1,	#0     
	str	r1,	[r11, #0x204]    
	smulbb	r4,	ip, r6    
	smulbb	r8,	r2, r5    
	qadd	r5,	r8, r4    
	mov	r7,	#0x18c     
	mov	r3,	#0x10c     
	ldrh	r3,	[r11, r3]    
	ldrh	r6,	[r11, r7]    
	smulbb	r1,	r3, r6    
	qadd	r8,	r1, r5    
	add	r1,	r11, #0x188    
	ldr	r7,	[r1, #20]    
	mov	r4,	r8, asr #12   
	qadd	r4,	r7, r4    
	ldr	r6,	._gteRTPS__0000018e     
	mov	r5,	#0x190     
	ldrh	r8,	[r11, r6]    
	ldrh	r7,	[r11, r5]    
	smulbb	r8,	ip, r8    
	str	r4,	[r11, #0x16c]    
	smulbb	r6,	r2, r7    
	add	r5,	r11, #0x108    
	qadd	r7,	r6, r8    
	ldr	r6,	._gteRTPS__00000192     
	ldrh	r6,	[r11, r6]    
	smulbb	r6,	r3, r6    
	qadd	r8,	r6, r7    
	ldr	r7,	[r1, #24]    
	mov	r8,	r8, asr #12   
	qadd	r8,	r7, r8    
	mov	r6,	#0x194     
	ldrh	r7,	[r11, r6]    
	add	r6,	r6, #2    
	ldrh	r6,	[r11, r6]    
	smulbb	ip,	ip, r7    
	str	r8,	[r11, #0x170]    
	smulbb	r2,	r2, r6    
	qadd	r2,	r2, ip    
	mov	r6,	#0x198     
	ldrh	ip,	[r11, r6]    
	smulbb	r3,	r3, ip    
	qadd	r6,	r3, r2    
	ldr	ip,	[r1, #28]    
	mov	r3,	r6, asr #12   
	qadd	r2,	ip, r3    
	ldr	r3,	._gteRTPS__00007fff     
	str	r2,	[r11, #0x174]    
	cmp	r4,	r3     
	movgt	r2,	#0x81000000     
	strgt	r2,	[r11, #0x204]    
	movgt	r4,	r3     
	bgt	._gteRTPS__450	     
	cmn	r4,	#0x8000     
	movge	r4,	r4, lsl #16   
	movlt	r3,	#0x81000000     
	movge	r4,	r4, lsr #16   
	movlt	r4,	#0x8000     
	strlt	r3,	[r11, #0x204]    
._gteRTPS__450:		     
	ldr	r3,	[r11, #0x170]    
	ldr	r2,	._gteRTPS__00007fff     
	mov	ip,	#0x12c     
	cmp	r3,	r2     
	strh	r4,	[r11, ip]    
	ble	._gteRTPS__680	     
	ldr	r4,	[r11, #0x204]    
	mov	r3,	r2     
	orr	r2,	r4, #0x80000000    
	orr	r6,	r2, #0x800000    
	str	r6,	[r11, #0x204]    
._gteRTPS__47c:		     
	ldr	r2,	[r11, #0x174]    
	ldr	ip,	._gteRTPS__00007fff     
	mov	r6,	#0x130     
	cmp	r2,	ip     
	strh	r3,	[r11, r6]    
	ble	._gteRTPS__660	     
	ldr	r4,	[r11, #0x204]    
	mov	r7,	ip     
	orr	r3,	r4, #0x400000    
	str	r3,	[r11, #0x204]    
._gteRTPS__4a4:		     
	mov	r4,	#0x14c     
	mov	r6,	#0x134     
	strh	r7,	[r11, r6]    
	ldrh	r7,	[r11, r4]    
	mov	ip,	#0x150     
	mov	r6,	#0x148     
	strh	r7,	[r11, r6]    
	ldrh	r6,	[r11, ip]    
	ldr	r3,	._gteRTPS__0000ffff     
	strh	r6,	[r11, r4]    
	mov	r4,	#0x154     
	ldrh	r4,	[r11, r4]    
	cmp	r2,	r3     
	strh	r4,	[r11, ip]    
	ble	._gteRTPS__5f4	     
	ldr	r4,	[r11, #0x204]    
	orr	ip,	r4, #0x80000000    
	orr	r2,	ip, #0x40000    
	str	r2,	[r11, #0x204]    
._gteRTPS__4f0:		     
	mov	r4,	#0x1f0     
	ldrh	ip,	[r11, r4]    
	mov	r2,	#0x154     
	mov	ip,	ip, lsl #16   
	movs	r4,	ip, asr #16   
	strh	r3,	[r11, r2]    
	bmi	._gteRTPS__514	     
	cmp	r4,	r3, lsl #1   
	blt	._gteRTPS__6a4	     
._gteRTPS__514:		     
	ldr	r3,	._gteRTPS__0001ffff     
._gteRTPS__518:		     
	mov	r2,	#0x12c     
	ldrsh	r2,	[r11, r2]    
	ldr	r4,	[r11, #0x13c]    
	ldr	ip,	[r11, #0x140]    
	str	r4,	[r11, #0x138]    
	str	ip,	[r11, #0x13c]    
	mul	r2,	r3, r2    
	ldr	ip,	[r1, #0x60]    
	qadd	r2,	ip, r2    
	ldr	ip,	._gteRTPS__000003ff     
	mov	r2,	r2, asr #16   
	cmp	r2,	ip     
	ble	._gteRTPS__63c	     
	ldr	r4,	[r11, #0x204]    
	orr	r2,	r4, #0x80000000    
	orr	r4,	r2, #0x4000    
	str	r4,	[r11, #0x204]    
._gteRTPS__55c:		     
	mov	r4,	#0x130     
	ldrsh	r2,	[r11, r4]    
	mov	r4,	#0x140     
	strh	ip,	[r11, r4]    
	mul	r2,	r3, r2    
	ldr	ip,	[r1, #0x64]    
	qadd	r2,	ip, r2    
	ldr	ip,	._gteRTPS__000003ff     
	mov	r2,	r2, asr #16   
	cmp	r2,	ip     
	ble	._gteRTPS__618	     
	ldr	r2,	[r11, #0x204]    
	orr	r4,	r2, #0x80000000    
	orr	r2,	r4, #0x2000    
	str	r2,	[r11, #0x204]    
._gteRTPS__598:		     
	mov	r2,	#0x1f4     
	ldrsh	r4,	[r11, r2]    
	mul	r3,	r4, r3    
	ldr	r4,	._gteRTPS__00000142     
	strh	ip,	[r11, r4]    
	ldr	ip,	[r1, #0x70]    
	qadd	r1,	ip, r3    
	ldr	r2,	._gteRTPS__00000fff     
	mov	r3,	r1, asr #12   
	cmp	r3,	r2     
	str	r3,	[r5, #0x60]    
	bgt	._gteRTPS__740	     
	cmp	r3,	#0     
	ldrlt	r3,	[r11, #0x204]    
	movge	r2,	r3, lsl #16   
	orrlt	r3,	r3, #0x1000    
	movge	r2,	r2, lsr #16   
	movlt	r2,	#0     
	strlt	r3,	[r11, #0x204]    
._gteRTPS__5e4:		     
	mov	ip,	#0x128     
	strh	r2,	[r11, ip]    
	ldmia   sp!,	{r4, r5, r6, r7, r8, r9, r10}
	bx	lr	     
._gteRTPS__5f4:		     
	cmp	r2,	#0     
	ldrlt	r2,	[r11, #0x204]    
	movge	r3,	r2, lsl #16   
	orrlt	r2,	r2, #0x80000000    
	orrlt	r2,	r2, #0x40000    
	movge	r3,	r3, lsr #16   
	movlt	r3,	#0     
	strlt	r2,	[r11, #0x204]    
	b	._gteRTPS__4f0	     
._gteRTPS__618:		     
	cmn	r2,	#0x400     
	ldrlt	r2,	[r11, #0x204]    
	movge	ip,	r2, lsl #16   
	orrlt	r2,	r2, #0x80000000    
	orrlt	r2,	r2, #0x2000    
	movge	ip,	ip, lsr #16   
	movlt	ip,	#0xfc00     
	strlt	r2,	[r11, #0x204]    
	b	._gteRTPS__598	     
._gteRTPS__63c:		     
	cmn	r2,	#0x400     
	ldrlt	r2,	[r11, #0x204]    
	movge	ip,	r2, lsl #16   
	orrlt	r2,	r2, #0x80000000    
	orrlt	r2,	r2, #0x4000    
	movge	ip,	ip, lsr #16   
	movlt	ip,	#0xfc00     
	strlt	r2,	[r11, #0x204]    
	b	._gteRTPS__55c	     
._gteRTPS__660:		     
	cmn	r2,	#0x8000     
	ldrlt	r3,	[r11, #0x204]    
	movge	r7,	r2, lsl #16   
	orrlt	r3,	r3, #0x400000    
	movge	r7,	r7, lsr #16   
	movlt	r7,	#0x8000     
	strlt	r3,	[r11, #0x204]    
	b	._gteRTPS__4a4	     
._gteRTPS__680:		     
	cmn	r3,	#0x8000     
	ldrlt	r2,	[r11, #0x204]    
	movge	r3,	r3, lsl #16   
	orrlt	r2,	r2, #0x80000000    
	orrlt	r2,	r2, #0x800000    
	movge	r3,	r3, lsr #16   
	movlt	r3,	#0x8000     
	strlt	r2,	[r11, #0x204]    
	b	._gteRTPS__47c	     
._gteRTPS__6a4:		     
	cmp	r3,	#0x8000     
	mov	r2,	#0     
	bhi	._gteRTPS__6c0	     
._gteRTPS__6b0:		     
	mov	r3,	r3, lsl #1   
	cmp	r3,	#0x8000     
	add	r2,	r2, #1    
	bls	._gteRTPS__6b0	     
._gteRTPS__6c0:		     
	mov	r4,	r3, lsl #17   
	ldr	r7,	._gteRTPS__reciprocals     
	mov	r8,	r4, lsr #17   
	mov	r3,	r8, lsl #1   
	ldrh	r6,	[r3, r7]    
	mov	r9,	ip, asr #16   
	orr	r6,	r6, #0x10000    
	mov	r7,	r6, asr #31   
	mov	r0,	r7, lsl r2   
	mov	r8,	r6, lsl r2   
	rsb	r10,	r2, #32    
	orr	r4,	r0, r6, lsr r10  
	mov	r10,	r9     
	mov	r0,	r10, asr #31   
	subs	r2,	r2, #32    
	mul	ip,	r8, r0    
	movmi	r7,	r4     
	movpl	r7,	r6, lsl r2   
	umull	r2,	r3, r8, r10   
	mla	r10,	r7, r10, ip   
	mov	r6,	#0x8000     
	adds	r6,	r6, r2    
	add	r9,	r10, r3    
	mov	r8,	r2     
	mov	r7,	#0     
	ldr	r2,	._gteRTPS__0001ffff     
	adc	r7,	r7, r9    
	mov	r3,	r6, lsr #16   
	orr	r3,	r3, r7, lsl #16  
	cmp	r3,	r2     
	movcs	r3,	r2     
	b	._gteRTPS__518	     
._gteRTPS__740:		     
	ldr	r1,	[r11, #0x204]    
	orr	r3,	r1, #0x1000    
	str	r3,	[r11, #0x204]    
	b	._gteRTPS__5e4	     
._gteRTPS__0000018e:	.long	0x0000018e     
._gteRTPS__00000192:	.long	0x00000192     
._gteRTPS__00007fff:	.long	0x00007fff     
._gteRTPS__0000ffff:	.long	0x0000ffff     
._gteRTPS__0001ffff:	.long	0x0001ffff     
._gteRTPS__000003ff:	.long	0x000003ff     
._gteRTPS__00000142:	.long	0x00000142     
._gteRTPS__00000fff:	.long	0x00000fff     
._gteRTPS__reciprocals:	.long	__gte_reciprocals__     
#else
*/
	stmdb	sp!,	{r5, r6, r7, r8, r9, lr} 
	mov	ip,	#392
	mov	lr,	#264
	add	r3,	ip, #2
	add	r2,	lr, #2
	ldrh	r7,	[r11, lr]      
	ldrh	r8,	[r11, ip]      
	ldrh	r5,	[r11, r3]      
	ldrh	lr,	[r11, r2]      
	mov	r0,	#0
	str	r0,	[r11, #516]      
	smulbb	r1,	r8, r7      
	smulbb	r9,	r5, lr      
	qadd	r0,	r9, r1      
	mov	r1,	#268
	add	ip,	r2, #130
	ldrh	r3,	[r11, ip]      
	ldrh	ip,	[r11, r1]      
	smulbb	r8,	r3, ip      
	qadd	r5,	r8, r0      
	add	r8,	r11, #392
	mov	r9,	r5, asr #12     
	ldr	r0,	[r8, #20]      
	qadd	r5,	r0, r9      
	mov	r3,	#400
	add	r1,	r1, #130
	ldrh	r2,	[r11, r1]      
	ldrh	r9,	[r11, r3]      
	smulbb	r0,	r2, r7      
	str	r5,	[r11, #364]      
	smulbb	r2,	r9, lr      
	add	r9,	r11, #264
	qadd	r2,	r2, r0      
	add	r1,	r1, #4
	ldrh	r3,	[r11, r1]      
	smulbb	r0,	r3, ip      
	qadd	r1,	r0, r2      
	mov	r3,	r1, asr #12     
	ldr	r0,	[r8, #24]      
	qadd	r1,	r0, r3      
	ldr	r3,	._gteRTPS__89d0
	mov	r2,	#404
	ldrh	r0,	[r11, r2]      
	ldrh	r2,	[r11, r3]      
	smulbb	r3,	r0, r7      
	str	r1,	[r11, #368]      
	smulbb	r7,	r2, lr      
	add	lr,	r11, #368
	qadd	r2,	r7, r3      
	mov	r1,	#408
	ldrh	r0,	[r11, r1]      
	smulbb	r3,	r0, ip      
	qadd	r7,	r3, r2      
	mov	r1,	r7, asr #12     
	ldr	r0,	[r8, #28]      
	qadd	r2,	r0, r1      
	ldr	r3,	._gteRTPS__89d4
	str	r2,	[r11, #372]      
	cmp	r5,	r3       
	movgt	r2,	r3       
	movgt	r3,	#0x81000000     
	add	ip,	r11, #0x174    
	strgt	r3,	[r11, #516]      
	bgt	._gteRTPS__86bc	       
	cmn	r5,	#0x8000     
	movge	r3,	r5, lsl #16     
	movlt	r3,	#0x81000000     
	movge	r2,	r3, lsr #16     
	movlt	r2,	#0x8000     
	strlt	r3,	[r11, #516]      
._gteRTPS__86bc:		       
	mov	r7,	#0x12c     
	strh	r2,	[r11, r7]      
	ldr	r2,	._gteRTPS__89d4
	ldr	r3,	[lr]       
	cmp	r3,	r2       
	ble	._gteRTPS__8914	       
	ldr	lr,	[r11, #516]      
	orr	r0,	lr, #0x80000000    
	orr	r1,	r0, #0x800000    
	str	r1,	[r11, #516]      
._gteRTPS__86e4:		       
	mov	r3,	#0x130     
	strh	r2,	[r11, r3]      
	ldr	r2,	._gteRTPS__89d4
	ldr	r3,	[ip]       
	cmp	r3,	r2       
	ble	._gteRTPS__88f4	       
	ldr	r7,	[r11, #516]      
	mov	r0,	r2       
	orr	r2,	r7, #0x400000    
	str	r2,	[r11, #516]      
._gteRTPS__870c:		       
	mov	r2,	#0x14c     
	ldrh	lr,	[r11, r2]      
	mov	r7,	#0x150     
	mov	r1,	#0x148     
	strh	lr,	[r11, r1]      
	ldrh	r3,	[r11, r7]      
	mov	r1,	#0x154     
	strh	r3,	[r11, r2]      
	ldrh	r3,	[r11, r1]      
	sub	r1,	r2, #0x18    
	strh	r3,	[r11, r7]      
	strh	r0,	[r11, r1]      
	ldr	r1,	._gteRTPS__89d8
	ldr	r3,	[ip]       
	cmp	r3,	r1       
	ble	._gteRTPS__8860	       
	ldr	r7,	[r11, #516]      
	mov	ip,	r1       
	orr	r2,	r7, #0x80000000    
	orr	r0,	r2, #0x40000    
	str	r0,	[r11, #516]      
._gteRTPS__8760:		       
	mov	r7,	#0x1f0     
	ldrh	r2,	[r11, r7]      
	mov	r1,	ip, lsl #16     
	mov	r0,	r2, lsl #16     
	sub	r3,	r7, #0x9c    
	movs	r2,	r0, asr #16     
	strh	ip,	[r11, r3]      
	mov	r1,	r1, lsr #16     
	bmi	._gteRTPS__878c	       
	cmp	r2,	r1, lsl #1     
	blt	._gteRTPS__8938	       
._gteRTPS__878c:		       
	ldr	r7,	._gteRTPS__89dc
._gteRTPS__8790:		       
	mov	r1,	#0x12c     
	ldrsh	ip,	[r11, r1]      
	ldr	r2,	[r11, #316]      
	ldr	r3,	[r11, #320]      
	str	r2,	[r11, #312]      
	str	r3,	[r11, #316]      
	mul	r0,	r7, ip      
	ldr	r1,	[r8, #96]      
	qadd	ip,	r1, r0      
	ldr	r2,	._gteRTPS__89e0
	mov	r3,	ip, asr #16     
	cmp	r3,	r2       
	ble	._gteRTPS__88d0	       
	ldr	r3,	[r11, #516]      
	mov	r1,	r2       
	orr	ip,	r3, #0x80000000    
	orr	r0,	ip, #0x4000    
	str	r0,	[r11, #516]      
._gteRTPS__87d8:		       
	mov	r0,	#0x130     
	ldrsh	ip,	[r11, r0]      
	mov	r3,	#0x140     
	strh	r1,	[r11, r3]      
	mul	r1,	r7, ip      
	ldr	r2,	[r8, #100]      
	qadd	r0,	r2, r1      
	ldr	r2,	._gteRTPS__89e0
	mov	r3,	r0, asr #16     
	cmp	r3,	r2       
	ble	._gteRTPS__88ac	       
	ldr	r3,	[r11, #516]      
	mov	ip,	r2       
	orr	r1,	r3, #0x80000000    
	orr	r2,	r1, #0x2000    
	str	r2,	[r11, #516]      
._gteRTPS__8818:		       
	mov	r3,	#0x1f4     
	ldrsh	r1,	[r11, r3]      
	ldr	r2,	._gteRTPS__89e4
	mul	r0,	r7, r1      
	strh	ip,	[r11, r2]      
	ldr	ip,	[r8, #112]      
	qadd	r0,	ip, r0      
	ldr	r2,	._gteRTPS__89e8
	str	r0,	[r9, #96]      
	cmp	r0,	r2       
	ble	._gteRTPS__8884	       
	ldr	r1,	[r11, #516]      
	mov	r0,	r2       
	orr	ip,	r1, #0x1000    
	mov	r2,	#0x128     
	str	ip,	[r11, #516]      
	strh	r0,	[r11, r2]      
	ldmia	sp!,	{r5, r6, r7, r8, r9, pc} 
._gteRTPS__8860:		       
	cmp	r3,	#0     
	ldrlt	r3,	[r11, #516]      
	movge	r3,	r3, lsl #16     
	orrlt	r3,	r3, #0x80000000    
	orrlt	r3,	r3, #0x40000    
	movge	ip,	r3, lsr #16     
	movlt	ip,	#0     
	strlt	r3,	[r11, #516]      
	b	._gteRTPS__8760	       
._gteRTPS__8884:		       
	cmp	r0,	#0     
	ldrlt	r3,	[r11, #516]      
	movge	r3,	r0, lsl #16     
	movge	r0,	r3, lsr #16     
	movlt	r0,	#0     
	orrlt	r3,	r3, #0x1000    
	mov	r2,	#0x128     
	strlt	r3,	[r11, #516]      
	strh	r0,	[r11, r2]      
	ldmia	sp!,	{r5, r6, r7, r8, r9, pc} 
._gteRTPS__88ac:		       
	cmn	r3,	#0x400     
	ldrlt	r3,	[r11, #516]      
	movge	r3,	r3, lsl #16     
	orrlt	r3,	r3, #0x80000000    
	orrlt	r3,	r3, #0x2000    
	movge	ip,	r3, lsr #16     
	movlt	ip,	#0xfc00     
	strlt	r3,	[r11, #516]      
	b	._gteRTPS__8818	       
._gteRTPS__88d0:		       
	cmn	r3,	#0x400     
	ldrlt	r3,	[r11, #516]      
	movge	r3,	r3, lsl #16     
	orrlt	r3,	r3, #0x80000000    
	orrlt	r3,	r3, #0x4000    
	movge	r1,	r3, lsr #16     
	movlt	r1,	#0xfc00     
	strlt	r3,	[r11, #516]      
	b	._gteRTPS__87d8	       
._gteRTPS__88f4:		       
	cmn	r3,	#0x8000     
	ldrlt	r3,	[r11, #516]      
	movge	r3,	r3, lsl #16     
	orrlt	r3,	r3, #0x400000    
	movge	r0,	r3, lsr #16     
	movlt	r0,	#0x8000     
	strlt	r3,	[r11, #516]      
	b	._gteRTPS__870c	       
._gteRTPS__8914:		       
	cmn	r3,	#0x8000     
	ldrlt	r3,	[r11, #516]      
	movge	r3,	r3, lsl #16     
	orrlt	r3,	r3, #0x80000000    
	orrlt	r3,	r3, #0x800000    
	movge	r2,	r3, lsr #16     
	movlt	r2,	#0x8000     
	strlt	r3,	[r11, #516]      
	b	._gteRTPS__86e4	       
._gteRTPS__8938:		       
	cmp	r1,	#0x8000     
	movhi	ip,	#0     
	bhi	._gteRTPS__8958	       
	mov	ip,	#0     
._gteRTPS__8948:		       
	mov	r1,	r1, lsl #1     
	cmp	r1,	#0x8000     
	add	ip,	ip, #1    
	bls	._gteRTPS__8948	       
._gteRTPS__8958:		       
	mov	r2,	r1, lsl #17     
	ldr	r5,	._gteRTPS__89ec
	mov	r3,	r2, lsr #17     
	mov	r7,	r3, lsl #1     
	ldrh	r6,	[r7, r5]      
	mov	r5,	r0, asr #16     
	orr	r3,	r6, #0x10000    
	mov	r7,	r3, asr #31     
	mov	r0,	r7, lsl ip     
	rsb	r2,	ip, #0x20    
	orr	r7,	r0, r3, lsr r2    
	subs	r1,	ip, #0x20    
	movpl	r7,	r3, lsl r1     
	mov	r3,	r3, lsl ip     
	mov	r6,	r5, asr #31     
	umull	r1,	r2, r3, r5     
	mul	ip,	r6, r3      
	mul	r3,	r7, r5      
	add	r0,	ip, r2      
	mov	ip,	#0x8000     
	add	r2,	r0, r3      
	mov	r7,	#0x0     
	adds	r3,	ip, r1      
	adc	r1,	r7, r2      
	mov	r0,	r3, lsr #16     
	orr	ip,	r0, r1, lsl #16    
	cmp	ip,	#0x20000     
	movcc	r7,	ip       
	bcc	._gteRTPS__8790	       
	b	._gteRTPS__878c	       
._gteRTPS__89d0:	.long	0x00000196
._gteRTPS__89d4:	.long	0x00007fff
._gteRTPS__89d8:	.long	0x0000ffff
._gteRTPS__89dc:	.long	0x0001ffff
._gteRTPS__89e0:	.long	0x000003ff
._gteRTPS__89e4:	.long	0x00000142
._gteRTPS__89e8:	.long	0x00000fff
._gteRTPS__89ec:	.long	__gte_reciprocals__       
/*
#endif
*/
			       
			     
.align 4
_gteNCDS__:		     
	mov	r2,	#0x1a8     
	stmdb	sp!,	{r4, r5, r6, r7, r8}
	ldrsh	r0,	[r11, r2]    
	sub	r5,	r2, #0x9e    
	mov	r4,	#0x108     
	add	ip,	r5, #0xa0    
	ldrsh	r1,	[r11, r4]    
	ldrsh	r2,	[r11, r5]    
	ldrsh	r8,	[r11, ip]    
	mul	r0,	r1, r0    
	mul	r8,	r2, r8    
	qadd	r4,	r8, r0    
	mov	r7,	#0x1ac     
	mov	r3,	#0x10c     
	ldrsh	r3,	[r11, r3]    
	ldrsh	r6,	[r11, r7]    
	mul	r6,	r3, r6    
	qadd	r7,	r6, r4    
	ldr	r5,	._gteNCDS__000001ae     
	mov	ip,	#0x1b0     
	ldrsh	r8,	[r11, r5]    
	ldrsh	r0,	[r11, ip]    
	mul	r8,	r1, r8    
	mul	r0,	r2, r0    
	qadd	ip,	r0, r8    
	ldr	r6,	._gteNCDS__000001b2     
	ldrsh	r5,	[r11, r6]    
	mul	r5,	r3, r5    
	qadd	ip,	r5, ip    
	mov	r8,	#0x1b4     
	add	r0,	r8, #2    
	ldrsh	r6,	[r11, r8]    
	ldrsh	r5,	[r11, r0]    
	mul	r1,	r6, r1    
	mul	r2,	r5, r2    
	qadd	r1,	r2, r1    
	mov	r8,	#0x1b8     
	ldrsh	r0,	[r11, r8]    
	mul	r3,	r0, r3    
	qadd	r0,	r3, r1    
	ldr	r3,	._gteNCDS__00007fff     
	mov	r4,	r7, asr #12   
	mov	r6,	ip, asr #12   
	mov	r5,	#0x1c8     
	bic	r8,	r4, r4, asr #31  
	bic	r7,	r6, r6, asr #31  
	add	r2,	r5, #2    
	ldrsh	r6,	[r11, r5]    
	ldrsh	r2,	[r11, r2]    
	cmp	r7,	r3     
	movge	r7,	r3     
	cmp	r8,	r3     
	movge	r8,	r3     
	mov	r4,	r8, lsl #16   
	mov	ip,	r7, lsl #16   
	mov	r5,	r0, asr #12   
	mov	ip,	ip, asr #16   
	mov	r0,	r4, asr #16   
	bic	r7,	r5, r5, asr #31  
	mul	r6,	r0, r6    
	mul	r2,	ip, r2    
	qadd	r5,	r2, r6    
	mov	r8,	#0x1cc     
	ldrsh	r2,	[r11, r8]    
	cmp	r7,	r3     
	movge	r7,	r3     
	mov	r1,	r7, lsl #16   
	mov	r1,	r1, asr #16   
	mul	r2,	r1, r2    
	qadd	r8,	r2, r5    
	add	r2,	r11, #0x188    
	ldr	r7,	[r2, #0x34]    
	mov	r6,	r8, asr #12   
	qadd	r6,	r7, r6    
	ldr	r5,	._gteNCDS__000001ce     
	mov	r8,	#0x1d0     
	ldrsh	r7,	[r11, r5]    
	ldrsh	r5,	[r11, r8]    
	mul	r7,	r0, r7    
	mul	r5,	ip, r5    
	qadd	r8,	r5, r7    
	ldr	r5,	._gteNCDS__000001d2     
	ldrsh	r5,	[r11, r5]    
	mul	r5,	r1, r5    
	qadd	r7,	r5, r8    
	ldr	r8,	[r2, #0x38]    
	mov	r7,	r7, asr #12   
	qadd	r7,	r8, r7    
	mov	r5,	#0x1d4     
	ldrsh	r8,	[r11, r5]    
	add	r5,	r5, #2    
	ldrsh	r5,	[r11, r5]    
	mul	r0,	r8, r0    
	mul	ip,	r5, ip    
	qadd	r8,	ip, r0    
	mov	r4,	#0x1d8     
	ldrsh	ip,	[r11, r4]    
	mul	r1,	ip, r1    
	qadd	r0,	r1, r8    
	ldr	r5,	[r2, #0x3c]    
	mov	r8,	r0, asr #12   
	qadd	r8,	r5, r8    
	bic	r4,	r6, r6, asr #31  
	ldrb	r5,	[r11, #0x120]    
	cmp	r4,	r3     
	movge	r4,	r3     
	mov	ip,	r4, lsl #16   
	mov	r4,	ip, asr #16   
	ldr	r6,	[r2, #0x54]    
	mul	r0,	r5, r4    
	ldr	ip,	._gteNCDS__ffff8000     
	mov	r1,	#0x128     
	ldrsh	r1,	[r11, r1]    
	sub	r6,	r6, r0, asr #8  
	cmp	r6,	ip     
	movlt	r6,	ip     
	mov	r0,	r5, lsl #4   
	cmp	r6,	r3     
	movge	r6,	r3     
	mul	r4,	r0, r4    
	bic	r5,	r7, r7, asr #31  
	bic	r8,	r8, r8, asr #31  
	mul	r6,	r1, r6    
	qadd	r6,	r4, r6    
	ldrb	r4,	[r11, #0x121]    
	cmp	r5,	r3     
	movge	r5,	r3     
	mov	r0,	r5, lsl #16   
	mov	r5,	r0, asr #16   
	mul	r7,	r4, r5    
	ldr	r0,	[r2, #0x58]    
	mov	r4,	r4, lsl #4   
	sub	r7,	r0, r7, asr #8  
	cmp	r7,	ip     
	movlt	r7,	ip     
	cmp	r7,	r3     
	movge	r7,	r3     
	mul	r5,	r4, r5    
	mul	r7,	r1, r7    
	qadd	r7,	r5, r7    
	ldrb	r5,	[r11, #0x122]    
	cmp	r8,	r3     
	movlt	r4,	r8     
	movge	r4,	r3     
	mov	r4,	r4, lsl #16   
	mov	r4,	r4, asr #16   
	ldr	r8,	[r2, #0x5c]    
	mul	r2,	r5, r4    
	mov	r5,	r5, lsl #4   
	sub	r2,	r8, r2, asr #8  
	cmp	r2,	ip     
	movge	ip,	r2     
	movlt	ip,	ip     
	cmp	ip,	r3     
	movlt	r2,	ip     
	movge	r2,	r3     
	mul	r4,	r5, r4    
	mul	r1,	r2, r1    
	qadd	r3,	r4, r1    
	add	r0,	r11, #0x108    
	mov	ip,	r6, asr #12   
	mov	r2,	r7, asr #12   
	mov	r1,	r3, asr #12   
	str	ip,	[r0, #0x64]    
	str	r2,	[r0, #0x68]    
	str	r1,	[r0, #0x6c]    
	ldmia	sp!,	{r4, r5, r6, r7, r8}
	bx	lr	     
._gteNCDS__000001ae:	.long	0x000001ae     
._gteNCDS__000001b2:	.long	0x000001b2     
._gteNCDS__00007fff:	.long	0x00007fff     
._gteNCDS__000001ce:	.long	0x000001ce     
._gteNCDS__000001d2:	.long	0x000001d2     
._gteNCDS__ffff8000:	.long	0xffff8000     
			     
.align 4
_gteNCDT__:		     
	stmdb	sp!,	{r4, r5, r6, r7, r8, r9, r10}
	ldr	r2,	._gteNCDT__000001aa     
	mov	r6,	#0x1a8     
	ldrsh	r1,	[r11, r6]    
	mov	r7,	#0x108     
	sub	sp,	sp, #0x58    
	add	r4,	r7, #2    
	ldrsh	r9,	[r11, r7]    
	ldrsh	r7,	[r11, r4]    
	str	r1,	[sp]     
	ldrsh	r8,	[r11, r2]    
	ldr	r10,	[sp]     
	str	r8,	[sp, #4]    
	ldr	r0,	[sp, #4]    
	mov	r3,	#0x10c     
	ldrsh	r3,	[r11, r3]    
	mul	r10,	r9, r10    
	mul	r0,	r7, r0    
	qadd	r4,	r0, r10    
	mov	r5,	#0x1ac     
	ldrsh	ip,	[r11, r5]    
	str	ip,	[sp, #8]    
	mul	ip,	r3, ip    
	qadd	r4,	ip, r4    
	ldr	r6,	._gteNCDT__000001ae     
	mov	r2,	#0x1b0     
	ldrsh	r1,	[r11, r6]    
	str	r1,	[sp, #12]    
	ldr	r8,	[sp, #12]    
	ldrsh	r0,	[r11, r2]    
	mul	r8,	r9, r8    
	str	r0,	[sp, #16]    
	mul	r0,	r7, r0    
	qadd	ip,	r0, r8    
	ldr	r10,	._gteNCDT__000001b2     
	ldrsh	r5,	[r11, r10]    
	str	r5,	[sp, #20]    
	mul	r5,	r3, r5    
	qadd	r10,	r5, ip    
	mov	r6,	#0x1b4     
	ldrsh	r1,	[r11, r6]    
	ldr	r2,	._gteNCDT__000001b6     
	str	r1,	[sp, #24]    
	ldrsh	r8,	[r11, r2]    
	ldr	r0,	[sp, #24]    
	str	r8,	[sp, #28]    
	ldr	r5,	[sp, #28]    
	mul	r9,	r0, r9    
	mul	r7,	r5, r7    
	qadd	r2,	r7, r9    
	mov	r6,	#0x1b8     
	ldrsh	r1,	[r11, r6]    
	str	r1,	[sp, #32]    
	mul	r3,	r1, r3    
	qadd	r6,	r3, r2    
	mov	r9,	#0x1c8     
	ldrsh	r7,	[r11, r9]    
	ldr	r3,	._gteNCDT__00007fff     
	ldr	ip,	._gteNCDT__000001ca     
	mov	r8,	r4, asr #12   
	mov	r0,	r10, asr #12   
	str	r7,	[sp, #0x24]    
	bic	r10,	r8, r8, asr #31  
	bic	r5,	r0, r0, asr #31  
	cmp	r10,	r3     
	movge	r10,	r3     
	ldrsh	r0,	[r11, ip]    
	ldr	r9,	[sp, #0x24]    
	cmp	r5,	r3     
	movge	r5,	r3     
	mov	r4,	r10, lsl #16   
	mov	r1,	r5, lsl #16   
	mov	r8,	r6, asr #12   
	mov	r4,	r4, asr #16   
	mov	r1,	r1, asr #16   
	str	r0,	[sp, #0x28]    
	bic	r10,	r8, r8, asr #31  
	mul	r9,	r4, r9    
	mul	r0,	r1, r0    
	qadd	r5,	r0, r9    
	mov	r7,	#0x1cc     
	cmp	r10,	r3     
	movge	r10,	r3     
	ldrsh	r6,	[r11, r7]    
	mov	r2,	r10, lsl #16   
	mov	r10,	r2, asr #16   
	str	r6,	[sp, #0x2c]    
	mul	r6,	r10, r6    
	qadd	r8,	r6, r5    
	ldr	r7,	[r11, #0x1bc]    
	mov	ip,	r8, asr #12   
	qadd	ip,	r7, ip    
	ldr	r0,	._gteNCDT__000001ce     
	mov	r7,	#0x1d0     
	ldrsh	r9,	[r11, r0]    
	str	r9,	[sp, #0x30]    
	ldr	r6,	[sp, #0x30]    
	ldrsh	r5,	[r11, r7]    
	mul	r6,	r4, r6    
	str	r5,	[sp, #0x34]    
	mul	r5,	r1, r5    
	qadd	r0,	r5, r6    
	ldr	r8,	._gteNCDT__000001d2     
	ldrsh	r9,	[r11, r8]    
	str	r9,	[sp, #0x38]    
	mul	r9,	r10, r9    
	qadd	r7,	r9, r0    
	ldr	r8,	[r11, #0x1c0]    
	mov	r5,	r7, asr #12   
	qadd	r5,	r8, r5    
	mov	r8,	#0x1d4     
	ldrsh	r6,	[r11, r8]    
	ldr	r0,	._gteNCDT__000001d6     
	str	r6,	[sp, #0x3c]    
	ldr	r9,	[sp, #0x3c]    
	ldrsh	r7,	[r11, r0]    
	mul	r4,	r9, r4    
	str	r7,	[sp, #0x40]    
	mul	r1,	r7, r1    
	qadd	r0,	r1, r4    
	mov	r8,	#0x1d8     
	ldrsh	r6,	[r11, r8]    
	str	r6,	[sp, #0x44]    
	mul	r10,	r6, r10    
	qadd	r7,	r10, r0    
	ldr	r10,	[r11, #0x1c4]    
	mov	r4,	r7, asr #12   
	qadd	r6,	r10, r4    
	bic	r9,	ip, ip, asr #31  
	ldrb	r4,	[r11, #0x120]    
	cmp	r9,	r3     
	movge	r9,	r3     
	mov	r1,	r9, lsl #16   
	mov	ip,	r1, asr #16   
	mul	r10,	r4, ip    
	ldr	r8,	[r11, #0x1dc]    
	ldr	r1,	._gteNCDT__ffff8000     
	mov	r2,	#0x128     
	ldrsh	r2,	[r11, r2]    
	sub	r7,	r8, r10, asr #8  
	cmp	r7,	r1     
	movlt	r7,	r1     
	mov	r0,	r4, lsl #4   
	cmp	r7,	r3     
	movge	r7,	r3     
	str	r0,	[sp, #0x48]    
	bic	r10,	r5, r5, asr #31  
	bic	r6,	r6, r6, asr #31  
	mul	ip,	r0, ip    
	mul	r9,	r2, r7    
	qadd	r9,	ip, r9    
	ldrb	ip,	[r11, #0x121]    
	cmp	r10,	r3     
	movge	r10,	r3     
	mov	r5,	r10, lsl #16   
	mov	r5,	r5, asr #16   
	mul	r0,	ip, r5    
	ldr	r8,	[r11, #0x1e0]    
	mov	r10,	ip, lsl #4   
	sub	r7,	r8, r0, asr #8  
	cmp	r7,	r1     
	movlt	r7,	r1     
	cmp	r7,	r3     
	movge	r7,	r3     
	str	r10,	[sp, #0x4c]    
	mul	r5,	r10, r5    
	mul	r0,	r2, r7    
	qadd	r10,	r5, r0    
	ldrb	r0,	[r11, #0x122]    
	cmp	r6,	r3     
	movge	r6,	r3     
	mov	r6,	r6, lsl #16   
	mov	r6,	r6, asr #16   
	ldr	r8,	[r11, #0x1e4]    
	mul	r7,	r0, r6    
	mov	r5,	r0, lsl #4   
	sub	r7,	r8, r7, asr #8  
	cmp	r7,	r1     
	movlt	r7,	r1     
	cmp	r7,	r3     
	movge	r7,	r3     
	str	r5,	[sp, #0x50]    
	mul	r6,	r5, r6    
	mul	r8,	r2, r7    
	qadd	r5,	r6, r8    
	ldrb	r8,	[r11, #0x123]    
	ldr	r7,	[r11, #0x160]    
	mov	r9,	r9, asr #16   
	mov	r10,	r10, asr #16   
	mov	r5,	r5, asr #16   
	mov	r6,	#0x110     
	bic	r9,	r9, r9, asr #31  
	bic	r10,	r10, r10, asr #31  
	bic	r5,	r5, r5, asr #31  
	strb	r8,	[r11, #0x163]    
	str	r7,	[r11, #0x158]    
	cmp	r10,	#0xff     
	movge	r10,	#0xff     
	ldrsh	r7,	[r11, r6]    
	cmp	r5,	#0xff     
	movge	r5,	#0xff     
	cmp	r9,	#0xff     
	movge	r9,	#0xff     
	add	r6,	r6, #2    
	ldrsh	r6,	[r11, r6]    
	strb	r9,	[r11, #0x160]    
	strb	r5,	[r11, #0x162]    
	strb	r10,	[r11, #0x161]    
	mov	r5,	#0x114     
	ldrsh	r5,	[r11, r5]    
	ldr	r10,	[sp]     
	strb	r8,	[sp, #0x57]    
	ldr	r8,	[sp, #4]    
	mul	r10,	r7, r10    
	mul	r8,	r6, r8    
	qadd	r8,	r8, r10    
	ldr	r9,	[sp, #8]    
	mul	r9,	r5, r9    
	qadd	r8,	r9, r8    
	ldr	r9,	[sp, #12]    
	ldr	r10,	[sp, #16]    
	mul	r9,	r7, r9    
	mul	r10,	r6, r10    
	qadd	r9,	r10, r9    
	ldr	r10,	[sp, #20]    
	mul	r10,	r5, r10    
	qadd	r10,	r10, r9    
	ldr	r9,	[sp, #24]    
	mul	r7,	r9, r7    
	ldr	r9,	[sp, #28]    
	mul	r6,	r9, r6    
	qadd	r7,	r6, r7    
	ldr	r9,	[sp, #32]    
	mul	r5,	r9, r5    
	qadd	r9,	r5, r7    
	mov	r7,	r8, asr #12   
	mov	r6,	r10, asr #12   
	bic	r8,	r7, r7, asr #31  
	bic	r10,	r6, r6, asr #31  
	cmp	r8,	r3     
	movge	r8,	r3     
	cmp	r10,	r3     
	movge	r10,	r3     
	mov	r7,	r8, lsl #16   
	mov	r6,	r10, lsl #16   
	ldr	r8,	[sp, #0x28]    
	ldr	r10,	[sp, #0x24]    
	mov	r5,	r9, asr #12   
	mov	r7,	r7, asr #16   
	mov	r6,	r6, asr #16   
	bic	r9,	r5, r5, asr #31  
	mul	r10,	r7, r10    
	mul	r8,	r6, r8    
	qadd	r10,	r8, r10    
	ldr	r8,	[sp, #0x2c]    
	cmp	r9,	r3     
	movge	r9,	r3     
	mov	r5,	r9, lsl #16   
	mov	r5,	r5, asr #16   
	mul	r8,	r5, r8    
	qadd	r9,	r8, r10    
	mov	r8,	r9, asr #12   
	ldr	r9,	[r11, #0x1bc]    
	qadd	r8,	r9, r8    
	ldr	r9,	[sp, #0x30]    
	ldr	r10,	[sp, #0x34]    
	mul	r9,	r7, r9    
	mul	r10,	r6, r10    
	qadd	r9,	r10, r9    
	ldr	r10,	[sp, #0x38]    
	mul	r10,	r5, r10    
	qadd	r10,	r10, r9    
	ldr	r9,	[r11, #0x1c0]    
	mov	r10,	r10, asr #12   
	qadd	r10,	r9, r10    
	ldr	r9,	[sp, #0x3c]    
	mul	r7,	r9, r7    
	ldr	r9,	[sp, #0x40]    
	mul	r6,	r9, r6    
	qadd	r7,	r6, r7    
	ldr	r9,	[sp, #0x44]    
	mul	r5,	r9, r5    
	qadd	r6,	r5, r7    
	ldr	r7,	[r11, #0x1c4]    
	mov	r5,	r6, asr #12   
	qadd	r5,	r7, r5    
	bic	r8,	r8, r8, asr #31  
	cmp	r8,	r3     
	movge	r8,	r3     
	mov	r9,	r8, lsl #16   
	mov	r8,	r9, asr #16   
	mul	r6,	r4, r8    
	bic	r9,	r10, r10, asr #31  
	ldr	r10,	[r11, #0x1dc]    
	ldr	r7,	[sp, #0x48]    
	sub	r6,	r10, r6, asr #8  
	cmp	r6,	r1     
	movlt	r6,	r1     
	cmp	r6,	r3     
	movge	r6,	r3     
	bic	r5,	r5, r5, asr #31  
	mul	r8,	r7, r8    
	mul	r10,	r2, r6    
	qadd	r10,	r8, r10    
	cmp	r9,	r3     
	movlt	r6,	r9     
	movge	r6,	r3     
	mov	r8,	r6, lsl #16   
	mov	r8,	r8, asr #16   
	mul	r6,	ip, r8    
	ldr	r7,	[r11, #0x1e0]    
	ldr	r9,	[sp, #0x4c]    
	sub	r6,	r7, r6, asr #8  
	cmp	r6,	r1     
	movlt	r6,	r1     
	cmp	r6,	r3     
	movge	r6,	r3     
	mul	r8,	r9, r8    
	mul	r6,	r2, r6    
	qadd	r8,	r8, r6    
	cmp	r5,	r3     
	movge	r5,	r3     
	mov	r5,	r5, lsl #16   
	mov	r5,	r5, asr #16   
	mul	r6,	r0, r5    
	ldr	r7,	[r11, #0x1e4]    
	ldr	r9,	[sp, #0x50]    
	sub	r6,	r7, r6, asr #8  
	cmp	r6,	r1     
	movlt	r6,	r1     
	cmp	r6,	r3     
	movge	r6,	r3     
	mul	r5,	r9, r5    
	mul	r6,	r2, r6    
	qadd	r9,	r5, r6    
	mov	r5,	r9, asr #16   
	ldr	r9,	[r11, #0x160]    
	mov	r6,	#0x118     
	ldrsh	r7,	[r11, r6]    
	add	r6,	r6, #2    
	ldrsh	r6,	[r11, r6]    
	mov	r10,	r10, asr #16   
	str	r9,	[r11, #0x15c]    
	mov	r8,	r8, asr #16   
	ldrb	r9,	[sp, #0x57]    
	bic	r10,	r10, r10, asr #31  
	bic	r8,	r8, r8, asr #31  
	bic	r5,	r5, r5, asr #31  
	cmp	r10,	#0xff     
	movge	r10,	#0xff     
	cmp	r8,	#0xff     
	movge	r8,	#0xff     
	cmp	r5,	#0xff     
	movge	r5,	#0xff     
	strb	r5,	[r11, #0x162]    
	strb	r9,	[r11, #0x163]    
	strb	r10,	[r11, #0x160]    
	strb	r8,	[r11, #0x161]    
	ldmia	sp,	{r8, r10}    
	mov	r5,	#0x11c     
	ldrsh	r5,	[r11, r5]    
	mul	r8,	r7, r8    
	mul	r10,	r6, r10    
	qadd	r8,	r10, r8    
	ldr	r9,	[sp, #8]    
	mul	r9,	r5, r9    
	qadd	r8,	r9, r8    
	ldr	r9,	[sp, #12]    
	ldr	r10,	[sp, #16]    
	mul	r9,	r7, r9    
	mul	r10,	r6, r10    
	qadd	r9,	r10, r9    
	ldr	r10,	[sp, #20]    
	mul	r10,	r5, r10    
	qadd	r10,	r10, r9    
	ldr	r9,	[sp, #24]    
	mul	r7,	r9, r7    
	ldr	r9,	[sp, #28]    
	mul	r6,	r9, r6    
	qadd	r7,	r6, r7    
	ldr	r9,	[sp, #32]    
	mul	r5,	r9, r5    
	qadd	r9,	r5, r7    
	mov	r7,	r8, asr #12   
	mov	r6,	r10, asr #12   
	bic	r8,	r7, r7, asr #31  
	bic	r10,	r6, r6, asr #31  
	cmp	r8,	r3     
	movge	r8,	r3     
	cmp	r10,	r3     
	movge	r10,	r3     
	mov	r7,	r8, lsl #16   
	mov	r6,	r10, lsl #16   
	ldr	r8,	[sp, #0x28]    
	ldr	r10,	[sp, #0x24]    
	mov	r5,	r9, asr #12   
	mov	r7,	r7, asr #16   
	mov	r6,	r6, asr #16   
	bic	r9,	r5, r5, asr #31  
	mul	r10,	r7, r10    
	mul	r8,	r6, r8    
	qadd	r10,	r8, r10    
	ldr	r8,	[sp, #0x2c]    
	cmp	r9,	r3     
	movge	r9,	r3     
	mov	r5,	r9, lsl #16   
	mov	r5,	r5, asr #16   
	mul	r8,	r5, r8    
	qadd	r9,	r8, r10    
	mov	r8,	r9, asr #12   
	ldr	r9,	[r11, #0x1bc]    
	qadd	r8,	r9, r8    
	ldr	r9,	[sp, #0x30]    
	ldr	r10,	[sp, #0x34]    
	mul	r9,	r7, r9    
	mul	r10,	r6, r10    
	qadd	r9,	r10, r9    
	ldr	r10,	[sp, #0x38]    
	mul	r10,	r5, r10    
	qadd	r10,	r10, r9    
	ldr	r9,	[r11, #0x1c0]    
	mov	r10,	r10, asr #12   
	qadd	r10,	r9, r10    
	ldr	r9,	[sp, #0x3c]    
	mul	r7,	r9, r7    
	ldr	r9,	[sp, #0x40]    
	mul	r6,	r9, r6    
	qadd	r7,	r6, r7    
	ldr	r9,	[sp, #0x44]    
	mul	r5,	r9, r5    
	qadd	r6,	r5, r7    
	ldr	r7,	[r11, #0x1c4]    
	mov	r5,	r6, asr #12   
	qadd	r5,	r7, r5    
	bic	r9,	r8, r8, asr #31  
	ldr	r6,	[sp, #0x48]    
	cmp	r9,	r3     
	movge	r9,	r3     
	mov	r7,	r9, lsl #16   
	mov	r8,	r7, asr #16   
	mul	r4,	r8, r4    
	mul	r8,	r6, r8    
	ldr	r6,	[r11, #0x1dc]    
	bic	r9,	r10, r10, asr #31  
	sub	r4,	r6, r4, asr #8  
	cmp	r4,	r1     
	movlt	r4,	r1     
	cmp	r4,	r3     
	movge	r4,	r3     
	bic	r5,	r5, r5, asr #31  
	mul	r4,	r2, r4    
	qadd	r8,	r8, r4    
	cmp	r9,	r3     
	movge	r9,	r3     
	mov	r7,	r9, lsl #16   
	mov	r10,	r7, asr #16   
	ldr	r9,	[r11, #0x1e0]    
	mul	ip,	r10, ip    
	ldr	r7,	[sp, #0x4c]    
	sub	ip,	r9, ip, asr #8  
	cmp	ip,	r1     
	movlt	ip,	r1     
	cmp	ip,	r3     
	movge	ip,	r3     
	mul	r10,	r7, r10    
	mul	ip,	r2, ip    
	qadd	r10,	r10, ip    
	ldr	r4,	[sp, #0x50]    
	cmp	r5,	r3     
	movge	r5,	r3     
	mov	r5,	r5, lsl #16   
	mov	r5,	r5, asr #16   
	mul	r0,	r5, r0    
	mul	r5,	r4, r5    
	ldr	r4,	[r11, #0x1e4]    
	sub	r0,	r4, r0, asr #8  
	cmp	r0,	r1     
	movge	ip,	r0     
	movlt	ip,	r1     
	cmp	ip,	r3     
	movlt	r3,	ip     
	movge	r3,	r3     
	mul	r2,	r3, r2    
	qadd	r1,	r5, r2    
	add	r0,	r11, #0x108    
	mov	r8,	r8, asr #12   
	mov	r10,	r10, asr #12   
	mov	r2,	r1, asr #12   
	str	r2,	[r0, #0x6c]    
	str	r8,	[r0, #0x64]    
	str	r10,	[r0, #0x68]    
	add	sp,	sp, #0x58    
	ldmia	sp!,	{r4, r5, r6, r7, r8, r9, r10}
	bx	lr	     
._gteNCDT__000001aa:	.long	0x000001aa     
._gteNCDT__000001ae:	.long	0x000001ae     
._gteNCDT__000001b2:	.long	0x000001b2     
._gteNCDT__000001b6:	.long	0x000001b6     
._gteNCDT__00007fff:	.long	0x00007fff     
._gteNCDT__000001ca:	.long	0x000001ca     
._gteNCDT__000001ce:	.long	0x000001ce     
._gteNCDT__000001d2:	.long	0x000001d2     
._gteNCDT__000001d6:	.long	0x000001d6     
._gteNCDT__ffff8000:	.long	0xffff8000     
		        
.align 4
_gteCDP__:	        
	stmdb	sp!, {r4, r5, r6, r7, r8}   
	mov	r0, #0x130       
	mov	r5, #0x12c       
	mov	r1, #0x1c8       
	add	r6, r0, #0x9a      
	ldrsh	r8, [r11, r1]      
	ldrsh	ip, [r11, r5]      
	ldrsh	r1, [r11, r0]      
	ldrsh	r4, [r11, r6]      
	mul	r8, ip, r8      
	mul	r4, r1, r4      
	qadd	r5, r4, r8      
	mov	r7, #0x1cc       
	mov	r3, #0x134       
	ldrsh	r3, [r11, r3]      
	ldrsh	r2, [r11, r7]      
	mul	r2, r3, r2      
	qadd	r0, r2, r5      
	add	r2, r11, #0x188      
	ldr	r6, [r2, #0x34]      
	mov	r4, r0, asr #12     
	qadd	r0, r6, r4      
	ldr	r8, ._gteCDP__000001ce       
	mov	r7, #0x1d0       
	ldrsh	r6, [r11, r8]      
	ldrsh	r5, [r11, r7]      
	mul	r6, ip, r6      
	mul	r5, r1, r5      
	qadd	r7, r5, r6      
	ldr	r8, ._gteCDP__000001d2       
	ldrsh	r5, [r11, r8]      
	mul	r5, r3, r5      
	qadd	r6, r5, r7      
	ldr	r8, [r2, #0x38]      
	mov	r7, r6, asr #12     
	qadd	r7, r8, r7      
	mov	r5, #0x1d4       
	add	r6, r5, #2      
	ldrsh	r8, [r11, r5]      
	ldrsh	r5, [r11, r6]      
	mul	ip, r8, ip      
	mul	r1, r5, r1      
	qadd	r6, r1, ip      
	mov	ip, #0x1d8       
	ldrsh	r1, [r11, ip]      
	mul	r3, r1, r3      
	qadd	r8, r3, r6      
	ldr	r5, [r2, #0x3c]      
	mov	r3, r8, asr #12     
	qadd	r8, r5, r3      
	ldr	r3, ._gteCDP__00007fff       
	bic	r4, r0, r0, asr #31    
	ldrb	r5, [r11, #0x120]      
	cmp	r4, r3       
	movge	r4, r3       
	mov	ip, r4, lsl #16     
	mov	r4, ip, asr #16     
	ldr	r6, [r2, #0x54]      
	mul	r0, r5, r4      
	ldr	ip, ._gteCDP__ffff8000       
	mov	r1, #0x128       
	ldrsh	r1, [r11, r1]      
	sub	r6, r6, r0, asr #8    
	cmp	r6, ip       
	movlt	r6, ip       
	mov	r0, r5, lsl #4     
	cmp	r6, r3       
	movge	r6, r3       
	mul	r4, r0, r4      
	bic	r5, r7, r7, asr #31    
	bic	r8, r8, r8, asr #31    
	mul	r6, r1, r6      
	qadd	r6, r4, r6      
	cmp	r5, r3       
	movge	r5, r3       
	ldrb	r4, [r11, #0x121]      
	mov	r0, r5, lsl #16     
	mov	r5, r0, asr #16     
	mul	r7, r4, r5      
	ldr	r0, [r2, #0x58]      
	mov	r4, r4, lsl #4     
	sub	r7, r0, r7, asr #8    
	cmp	r7, ip       
	movlt	r7, ip       
	cmp	r7, r3       
	movge	r7, r3       
	mul	r5, r4, r5      
	mul	r7, r1, r7      
	qadd	r7, r5, r7      
	cmp	r8, r3       
	movlt	r4, r8       
	movge	r4, r3       
	ldrb	r5, [r11, #0x122]      
	mov	r4, r4, lsl #16     
	mov	r4, r4, asr #16     
	ldr	r8, [r2, #0x5c]      
	mul	r2, r5, r4      
	mov	r5, r5, lsl #4     
	sub	r2, r8, r2, asr #8    
	cmp	r2, ip       
	movge	ip, r2       
	movlt	ip, ip       
	cmp	ip, r3       
	movlt	r2, ip       
	movge	r2, r3       
	mul	r4, r5, r4      
	mul	r1, r2, r1      
	qadd	r3, r4, r1      
	add	r0, r11, #0x108      
	mov	ip, r6, asr #12     
	mov	r2, r7, asr #12     
	mov	r1, r3, asr #12     
	str	r1, [r0, #0x6c]      
	str	ip, [r0, #0x64]      
	str	r2, [r0, #0x68]      
	ldmia	sp!, {r4, r5, r6, r7, r8}   
	bx	lr        
._gteCDP__000001ce:	.long 0x000001ce       
._gteCDP__000001d2:	.long 0x000001d2       
._gteCDP__00007fff:	.long 0x00007fff       
._gteCDP__ffff8000:	.long 0xffff8000       
		        
.align 4
_gteNCCS__:	        
	mov	r0, #0x1a8       
	sub	r4, r0, #0x9e      
	mov	ip, #0x108       
	add	r2, r4, #0xa0      
	ldrsh	r5, [r11, r0]      
	ldrsh	r1, [r11, ip]      
	ldrsh	r0, [r11, r4]      
	ldrsh	r6, [r11, r2]      
	mul	r5, r1, r5      
	mul	r6, r0, r6      
	qadd	r4, r6, r5      
	mov	r7, #0x1ac       
	mov	r3, #0x10c       
	ldrsh	r3, [r11, r3]      
	ldrsh	ip, [r11, r7]      
	mul	ip, r3, ip      
	qadd	r7, ip, r4      
	ldr	r2, ._gteNCCS__000001ae       
	mov	r6, #0x1b0       
	ldrsh	r5, [r11, r2]      
	ldrsh	ip, [r11, r6]      
	mul	r5, r1, r5      
	mul	ip, r0, ip      
	qadd	ip, ip, r5      
	ldr	r2, ._gteNCCS__000001b2       
	ldrsh	r6, [r11, r2]      
	mul	r6, r3, r6      
	qadd	ip, r6, ip      
	mov	r5, #0x1b4       
	add	r2, r5, #2      
	ldrsh	r6, [r11, r5]      
	ldrsh	r5, [r11, r2]      
	mul	r1, r6, r1      
	mul	r0, r5, r0      
	qadd	r1, r0, r1      
	mov	r2, #0x1b8       
	ldrsh	r0, [r11, r2]      
	mul	r3, r0, r3      
	qadd	r1, r3, r1      
	ldr	r3, ._gteNCCS__00007fff       
	mov	r4, r7, asr #12     
	mov	r6, ip, asr #12     
	mov	r5, #0x1c8       
	bic	r0, r4, r4, asr #31    
	bic	r7, r6, r6, asr #31    
	add	r2, r5, #2      
	ldrsh	r6, [r11, r5]      
	ldrsh	r2, [r11, r2]      
	cmp	r7, r3       
	movge	r7, r3       
	cmp	r0, r3       
	movge	r0, r3       
	mov	r4, r0, lsl #16     
	mov	ip, r7, lsl #16     
	mov	r5, r1, asr #12     
	mov	r4, r4, asr #16     
	mov	ip, ip, asr #16     
	bic	r7, r5, r5, asr #31    
	mul	r6, r4, r6      
	mul	r2, ip, r2      
	qadd	r5, r2, r6      
	mov	r0, #0x1cc       
	ldrsh	r2, [r11, r0]      
	cmp	r7, r3       
	movge	r7, r3       
	mov	r1, r7, lsl #16     
	mov	r1, r1, asr #16     
	mul	r2, r1, r2      
	qadd	r0, r2, r5      
	add	r2, r11, #0x188      
	ldr	r7, [r2, #0x34]      
	mov	r6, r0, asr #12     
	qadd	r6, r7, r6      
	ldr	r5, ._gteNCCS__000001ce       
	mov	r0, #0x1d0       
	ldrsh	r7, [r11, r5]      
	ldrsh	r5, [r11, r0]      
	mul	r7, r4, r7      
	mul	r5, ip, r5      
	qadd	r0, r5, r7      
	ldr	r7, ._gteNCCS__000001d2       
	ldrsh	r7, [r11, r7]      
	mul	r7, r1, r7      
	qadd	r5, r7, r0      
	ldr	r7, [r2, #0x38]      
	mov	r0, r5, asr #12     
	qadd	r5, r7, r0      
	mov	r7, #0x1d4       
	ldrsh	r0, [r11, r7]      
	add	r7, r7, #2      
	ldrsh	r7, [r11, r7]      
	mul	r4, r0, r4      
	mul	ip, r7, ip      
	qadd	ip, ip, r4      
	mov	r4, #0x1d8       
	ldrsh	r4, [r11, r4]      
	mul	r1, r4, r1      
	qadd	r4, r1, ip      
	ldr	r2, [r2, #0x3c]      
	mov	r1, r4, asr #12     
	qadd	ip, r2, r1      
	bic	r4, r5, r5, asr #31    
	bic	r1, ip, ip, asr #31    
	bic	r2, r6, r6, asr #31    
	cmp	r1, r3       
	movge	r1, r3       
	cmp	r2, r3       
	movge	r2, r3       
	ldrb	r5, [r11, #0x120]      
	cmp	r4, r3       
	movlt	r3, r4       
	movge	r3, r3       
	ldrb	ip, [r11, #0x122]      
	ldrb	r4, [r11, #0x121]      
	mov	r2, r2, lsl #16     
	mov	r3, r3, lsl #16     
	mov	r1, r1, lsl #16     
	mov	r2, r2, asr #16     
	mov	r3, r3, asr #16     
	mov	r1, r1, asr #16     
	mul	r2, r5, r2      
	mul	r3, r4, r3      
	mul	r1, ip, r1      
	add	r0, r11, #0x108      
	mov	r2, r2, asr #8     
	mov	r3, r3, asr #8     
	mov	r1, r1, asr #8     
	str	r1, [r0, #0x6c]      
	str	r2, [r0, #0x64]      
	str	r3, [r0, #0x68]      
	bx	lr        
._gteNCCS__000001ae:	.long 0x000001ae       
._gteNCCS__000001b2:	.long 0x000001b2       
._gteNCCS__00007fff:	.long 0x00007fff       
._gteNCCS__000001ce:	.long 0x000001ce       
._gteNCCS__000001d2:	.long 0x000001d2       
		        
.align 4
_gteCC__:	        
	mov	r3, #0x130       
	mov	ip, #0x1c8       
	mov	r5, #0x12c       
	add	r2, r3, #0x9a      
	ldrsh	r1, [r11, ip]      
	ldrsh	r4, [r11, r5]      
	ldrsh	ip, [r11, r3]      
	ldrsh	r6, [r11, r2]      
	mul	r1, r4, r1      
	mul	r6, ip, r6      
	qadd	r2, r6, r1      
	mov	r0, #0x134       
	mov	r5, #0x1cc       
	ldrsh	r1, [r11, r0]      
	ldrsh	r3, [r11, r5]      
	mul	r3, r1, r3      
	qadd	r0, r3, r2      
	add	r2, r11, #0x188      
	ldr	r5, [r2, #0x34]      
	mov	r6, r0, asr #12     
	qadd	r6, r5, r6      
	ldr	r3, ._gteCC__000001ce       
	mov	r0, #0x1d0       
	ldrsh	r5, [r11, r3]      
	ldrsh	r3, [r11, r0]      
	mul	r5, r4, r5      
	mul	r3, ip, r3      
	qadd	r0, r3, r5      
	ldr	r3, ._gteCC__000001d2       
	ldrsh	r3, [r11, r3]      
	mul	r3, r1, r3      
	qadd	r5, r3, r0      
	ldr	r3, [r2, #0x38]      
	mov	r0, r5, asr #12     
	qadd	r5, r3, r0      
	mov	r3, #0x1d4       
	ldrsh	r0, [r11, r3]      
	add	r3, r3, #2      
	ldrsh	r3, [r11, r3]      
	mul	r4, r0, r4      
	mul	ip, r3, ip      
	qadd	ip, ip, r4      
	mov	r4, #0x1d8       
	ldrsh	r3, [r11, r4]      
	mul	r1, r3, r1      
	qadd	r4, r1, ip      
	ldr	r3, [r2, #0x3c]      
	mov	r2, r4, asr #12     
	qadd	r1, r3, r2      
	ldr	r4, ._gteCC__00007fff       
	bic	ip, r5, r5, asr #31    
	bic	r2, r6, r6, asr #31    
	bic	r1, r1, r1, asr #31    
	cmp	ip, r4       
	movlt	r3, ip       
	movge	r3, r4       
	cmp	r1, r4       
	movge	r1, r4       
	cmp	r2, r4       
	movge	r2, r4       
	ldrb	r5, [r11, #0x120]      
	ldrb	r4, [r11, #0x121]      
	ldrb	ip, [r11, #0x122]      
	mov	r2, r2, lsl #16     
	mov	r3, r3, lsl #16     
	mov	r1, r1, lsl #16     
	mov	r2, r2, asr #16     
	mov	r3, r3, asr #16     
	mov	r1, r1, asr #16     
	mul	r2, r5, r2      
	mul	r3, r4, r3      
	mul	r1, ip, r1      
	add	r0, r11, #0x108      
	mov	r2, r2, asr #8     
	mov	r3, r3, asr #8     
	mov	r1, r1, asr #8     
	str	r1, [r0, #0x6c]      
	str	r2, [r0, #0x64]      
	str	r3, [r0, #0x68]      
	bx	lr        
._gteCC__000001ce:	.long 0x000001ce       
._gteCC__000001d2:	.long 0x000001d2       
._gteCC__00007fff:	.long 0x00007fff       
		        
.align 4
_gteNCS__:	        
	mov	r2, #0x1a8       
	ldrsh	r6, [r11, r2]      
	sub	r4, r2, #0x9e      
	mov	r5, #0x108       
	add	ip, r4, #0xa0      
	ldrsh	r1, [r11, r5]      
	ldrsh	r2, [r11, r4]      
	ldrsh	r0, [r11, ip]      
	mul	r6, r1, r6      
	mul	r0, r2, r0      
	qadd	ip, r0, r6      
	mov	r5, #0x1ac       
	mov	r3, #0x10c       
	ldrsh	r3, [r11, r3]      
	ldrsh	r4, [r11, r5]      
	mul	r4, r3, r4      
	qadd	ip, r4, ip      
	ldr	r0, ._gteNCS__000001ae       
	mov	r6, #0x1b0       
	ldrsh	r5, [r11, r0]      
	ldrsh	r4, [r11, r6]      
	mul	r5, r1, r5      
	mul	r4, r2, r4      
	qadd	r4, r4, r5      
	ldr	r0, ._gteNCS__000001b2       
	ldrsh	r6, [r11, r0]      
	mul	r6, r3, r6      
	qadd	r0, r6, r4      
	mov	r5, #0x1b4       
	ldrsh	r6, [r11, r5]      
	add	r5, r5, #2      
	ldrsh	r5, [r11, r5]      
	mul	r1, r6, r1      
	mul	r2, r5, r2      
	qadd	r2, r2, r1      
	mov	r5, #0x1b8       
	ldrsh	r6, [r11, r5]      
	mul	r3, r6, r3      
	qadd	r6, r3, r2      
	ldr	r3, ._gteNCS__00007fff       
	mov	r1, ip, asr #12     
	mov	r5, r0, asr #12     
	mov	r4, #0x1c8       
	bic	ip, r1, r1, asr #31    
	bic	r0, r5, r5, asr #31    
	ldrsh	r5, [r11, r4]      
	add	r4, r4, #2      
	ldrsh	r4, [r11, r4]      
	cmp	ip, r3       
	movge	ip, r3       
	cmp	r0, r3       
	movge	r0, r3       
	mov	r1, r0, lsl #16     
	mov	ip, ip, lsl #16     
	mov	r2, r6, asr #12     
	mov	ip, ip, asr #16     
	mov	r1, r1, asr #16     
	bic	r6, r2, r2, asr #31    
	mul	r5, ip, r5      
	mul	r4, r1, r4      
	qadd	r5, r4, r5      
	mov	r0, #0x1cc       
	ldrsh	r4, [r11, r0]      
	cmp	r6, r3       
	movge	r6, r3       
	mov	r2, r6, lsl #16     
	mov	r2, r2, asr #16     
	mul	r3, r4, r2      
	qadd	r0, r3, r5      
	add	r3, r11, #0x188      
	ldr	r6, [r3, #0x34]      
	mov	r5, r0, asr #12     
	qadd	r5, r6, r5      
	ldr	r4, ._gteNCS__000001ce       
	mov	r0, #0x1d0       
	ldrsh	r6, [r11, r4]      
	ldrsh	r4, [r11, r0]      
	mul	r6, ip, r6      
	mul	r4, r1, r4      
	qadd	r0, r4, r6      
	ldr	r6, ._gteNCS__000001d2       
	ldrsh	r6, [r11, r6]      
	mul	r6, r2, r6      
	qadd	r4, r6, r0      
	ldr	r0, [r3, #0x38]      
	mov	r4, r4, asr #12     
	qadd	r4, r0, r4      
	mov	r6, #0x1d4       
	ldrsh	r0, [r11, r6]      
	add	r6, r6, #2      
	ldrsh	r6, [r11, r6]      
	mul	ip, r0, ip      
	mul	r1, r6, r1      
	qadd	r1, r1, ip      
	mov	ip, #0x1d8       
	ldrsh	ip, [r11, ip]      
	mul	r2, ip, r2      
	qadd	ip, r2, r1      
	ldr	r3, [r3, #0x3c]      
	mov	r1, ip, asr #12     
	qadd	r2, r3, r1      
	add	r0, r11, #0x108      
	str	r2, [r0, #0x6c]      
	str	r5, [r0, #0x64]      
	str	r4, [r0, #0x68]      
	bx	lr        
._gteNCS__000001ae:	.long 0x000001ae       
._gteNCS__000001b2:	.long 0x000001b2       
._gteNCS__00007fff:	.long 0x00007fff       
._gteNCS__000001ce:	.long 0x000001ce       
._gteNCS__000001d2:	.long 0x000001d2       
		        
.align 4
_gteNCT__:	        
	stmdb	sp!, {r4, r5, r6, r7, r8, r9, r10} 
	mov	ip, #0x1a8       
	mov	r5, #0x108       
	add	r4, r5, #2      
	add	r9, ip, #2      
	ldrsh	r6, [r11, r5]      
	ldrsh	r5, [r11, r4]      
	ldrsh	r4, [r11, ip]      
	ldrsh	ip, [r11, r9]      
	mov	r3, #0x10c       
	ldrsh	r3, [r11, r3]      
	sub	sp, sp, #0x40      
	mul	r1, r6, r4      
	mul	r10, r5, ip      
	qadd	r8, r10, r1      
	mov	r7, #0x1ac       
	ldrsh	r1, [r11, r7]      
	mul	r0, r3, r1      
	qadd	r8, r0, r8      
	ldr	r2, ._gteNCT__000001ae       
	mov	r9, #0x1b0       
	ldrsh	r2, [r11, r2]      
	ldrsh	r7, [r11, r9]      
	mul	r10, r6, r2      
	str	r7, [sp, #8]      
	mul	r7, r5, r7      
	qadd	r7, r7, r10      
	ldr	r0, ._gteNCT__000001b2       
	ldrsh	r9, [r11, r0]      
	str	r9, [sp, #12]      
	mul	r9, r3, r9      
	qadd	r0, r9, r7      
	mov	r10, #0x1b4       
	ldrsh	r9, [r11, r10]      
	ldr	r10, ._gteNCT__000001b6       
	str	r9, [sp, #16]      
	ldr	r9, [sp, #16]      
	ldrsh	r10, [r11, r10]      
	mul	r6, r9, r6      
	str	r10, [sp, #20]      
	mul	r5, r10, r5      
	qadd	r6, r5, r6      
	mov	r10, #0x1b8       
	ldrsh	r9, [r11, r10]      
	str	r9, [sp, #24]      
	mul	r3, r9, r3      
	qadd	r6, r3, r6      
	mov	r5, #0x1c8       
	ldrsh	r10, [r11, r5]      
	ldr	r3, ._gteNCT__00007fff       
	mov	r8, r8, asr #12     
	ldr	r5, ._gteNCT__000001ca       
	mov	r7, r0, asr #12     
	bic	r9, r8, r8, asr #31    
	str	r10, [sp, #28]      
	cmp	r9, r3       
	movge	r9, r3       
	bic	r0, r7, r7, asr #31    
	ldrsh	r5, [r11, r5]      
	cmp	r0, r3       
	movge	r0, r3       
	mov	r8, r9, lsl #16     
	ldr	r9, [sp, #28]      
	mov	r7, r0, lsl #16     
	mov	r10, r6, asr #12     
	mov	r8, r8, asr #16     
	mov	r7, r7, asr #16     
	str	r5, [sp, #32]      
	bic	r0, r10, r10, asr #31    
	mul	r9, r8, r9      
	mul	r5, r7, r5      
	qadd	r10, r5, r9      
	mov	r5, #0x1cc       
	ldrsh	r9, [r11, r5]      
	cmp	r0, r3       
	movge	r0, r3       
	mov	r6, r0, lsl #16     
	mov	r6, r6, asr #16     
	str	r9, [sp, #0x24]      
	mul	r9, r6, r9      
	qadd	r0, r9, r10      
	ldr	r10, [r11, #0x1bc]      
	mov	r5, r0, asr #12     
	qadd	r5, r10, r5      
	ldr	r10, ._gteNCT__000001ce       
	mov	r0, #0x1d0       
	ldrsh	r9, [r11, r10]      
	str	r9, [sp, #0x28]      
	ldr	r9, [sp, #0x28]      
	ldrsh	r10, [r11, r0]      
	mul	r9, r8, r9      
	str	r10, [sp, #0x2c]      
	mul	r10, r7, r10      
	qadd	r9, r10, r9      
	ldr	r0, ._gteNCT__000001d2       
	ldrsh	r10, [r11, r0]      
	str	r10, [sp, #0x30]      
	mul	r10, r6, r10      
	qadd	r0, r10, r9      
	ldr	r10, [r11, #0x1c0]      
	mov	r9, r0, asr #12     
	qadd	r9, r10, r9      
	mov	r10, #0x1d4       
	ldrsh	r0, [r11, r10]      
	ldr	r10, ._gteNCT__000001d6       
	str	r0, [sp, #0x34]      
	ldrsh	r0, [r11, r10]      
	ldr	r10, [sp, #0x34]      
	mul	r7, r0, r7      
	mul	r8, r10, r8      
	qadd	r7, r7, r8      
	mov	r10, #0x1d8       
	ldrsh	r8, [r11, r10]      
	str	r8, [sp, #0x38]      
	mul	r6, r8, r6      
	qadd	r10, r6, r7      
	ldr	r6, [r11, #0x1c4]      
	mov	r7, r10, asr #12     
	qadd	r8, r6, r7      
	mov	r10, r8, asr #4     
	mov	r5, r5, asr #4     
	mov	r6, #0x110       
	bic	r5, r5, r5, asr #31    
	bic	r10, r10, r10, asr #31    
	ldrsh	r7, [r11, r6]      
	cmp	r5, #0xff       
	movge	r5, #0xff       
	cmp	r10, #0xff       
	movge	r10, #0xff       
	add	r6, r6, #2      
	ldrsh	r6, [r11, r6]      
	ldrb	r8, [r11, #0x123]      
	str	r5, [sp, #0x3c]      
	str	r10, [sp, #4]      
	ldr	r10, [r11, #0x160]      
	mov	r5, #0x114       
	ldrsh	r5, [r11, r5]      
	strb	r8, [r11, #0x163]      
	str	r10, [r11, #0x158]      
	ldr	r10, [sp, #0x3c]      
	mov	r9, r9, asr #4     
	bic	r9, r9, r9, asr #31    
	cmp	r9, #0xff       
	movge	r9, #0xff       
	strb	r10, [r11, #0x160]      
	strb	r9, [r11, #0x161]      
	ldr	r9, [sp, #4]      
	mul	r10, r7, r4      
	strb	r9, [r11, #0x162]      
	strb	r8, [sp, #0x3c]      
	mul	r8, r6, ip      
	qadd	r8, r8, r10      
	mul	r9, r5, r1      
	qadd	r8, r9, r8      
	ldr	r10, [sp, #8]      
	mul	r9, r7, r2      
	mul	r10, r6, r10      
	qadd	r9, r10, r9      
	ldr	r10, [sp, #12]      
	mul	r10, r5, r10      
	qadd	r10, r10, r9      
	ldr	r9, [sp, #16]      
	mul	r7, r9, r7      
	ldr	r9, [sp, #20]      
	mul	r6, r9, r6      
	qadd	r7, r6, r7      
	ldr	r9, [sp, #24]      
	mul	r5, r9, r5      
	qadd	r9, r5, r7      
	mov	r7, r8, asr #12     
	mov	r6, r10, asr #12     
	bic	r8, r7, r7, asr #31    
	bic	r10, r6, r6, asr #31    
	cmp	r8, r3       
	movge	r8, r3       
	cmp	r10, r3       
	movge	r10, r3       
	mov	r7, r8, lsl #16     
	mov	r6, r10, lsl #16     
	ldr	r8, [sp, #32]      
	ldr	r10, [sp, #28]      
	mov	r5, r9, asr #12     
	mov	r7, r7, asr #16     
	mov	r6, r6, asr #16     
	bic	r9, r5, r5, asr #31    
	mul	r10, r7, r10      
	mul	r8, r6, r8      
	qadd	r10, r8, r10      
	ldr	r8, [sp, #0x24]      
	cmp	r9, r3       
	movge	r9, r3       
	mov	r5, r9, lsl #16     
	mov	r5, r5, asr #16     
	mul	r8, r5, r8      
	qadd	r9, r8, r10      
	mov	r8, r9, asr #12     
	ldr	r9, [r11, #0x1bc]      
	qadd	r8, r9, r8      
	ldr	r9, [sp, #0x28]      
	ldr	r10, [sp, #0x2c]      
	mul	r9, r7, r9      
	mul	r10, r6, r10      
	qadd	r9, r10, r9      
	ldr	r10, [sp, #0x30]      
	mul	r10, r5, r10      
	qadd	r10, r10, r9      
	ldr	r9, [r11, #0x1c0]      
	mov	r10, r10, asr #12     
	qadd	r10, r9, r10      
	ldr	r9, [sp, #0x34]      
	mul	r6, r0, r6      
	mul	r7, r9, r7      
	qadd	r9, r6, r7      
	ldr	r6, [sp, #0x38]      
	mul	r5, r6, r5      
	qadd	r5, r5, r9      
	ldr	r9, [r11, #0x1c4]      
	mov	r7, r5, asr #12     
	qadd	r5, r9, r7      
	mov	r9, #0x118       
	ldrsh	r7, [r11, r9]      
	add	r6, r9, #2      
	ldr	r9, [r11, #0x160]      
	ldrsh	r6, [r11, r6]      
	mov	r8, r8, asr #4     
	str	r9, [r11, #0x15c]      
	mov	r10, r10, asr #4     
	ldrb	r9, [sp, #0x3c]      
	mov	r5, r5, asr #4     
	bic	r8, r8, r8, asr #31    
	bic	r10, r10, r10, asr #31    
	bic	r5, r5, r5, asr #31    
	cmp	r5, #0xff       
	movge	r5, #0xff       
	cmp	r8, #0xff       
	movge	r8, #0xff       
	cmp	r10, #0xff       
	movge	r10, #0xff       
	strb	r5, [r11, #0x162]      
	strb	r9, [r11, #0x163]      
	strb	r8, [r11, #0x160]      
	strb	r10, [r11, #0x161]      
	mov	r5, #0x11c       
	ldrsh	r5, [r11, r5]      
	mul	r4, r7, r4      
	mul	ip, r6, ip      
	qadd	r4, ip, r4      
	mul	r1, r5, r1      
	qadd	r4, r1, r4      
	ldr	r9, [sp, #8]      
	mul	r2, r7, r2      
	mul	r9, r6, r9      
	qadd	ip, r9, r2      
	ldr	r8, [sp, #12]      
	mul	r8, r5, r8      
	qadd	r2, r8, ip      
	ldr	r10, [sp, #16]      
	ldr	r1, [sp, #20]      
	mul	r7, r10, r7      
	mul	r6, r1, r6      
	qadd	r7, r6, r7      
	ldr	r6, [sp, #24]      
	mul	r5, r6, r5      
	qadd	r10, r5, r7      
	mov	r9, r4, asr #12     
	mov	r8, r2, asr #12     
	bic	ip, r9, r9, asr #31    
	cmp	ip, r3       
	movge	ip, r3       
	bic	r4, r8, r8, asr #31    
	cmp	r4, r3       
	movge	r4, r3       
	mov	r1, ip, lsl #16     
	ldr	r8, [sp, #28]      
	ldr	ip, [sp, #32]      
	mov	r2, r4, lsl #16     
	mov	r9, r10, asr #12     
	mov	r1, r1, asr #16     
	mov	r2, r2, asr #16     
	bic	r5, r9, r9, asr #31    
	mul	r8, r1, r8      
	mul	ip, r2, ip      
	qadd	r9, ip, r8      
	cmp	r5, r3       
	movlt	r4, r5       
	movge	r4, r3       
	ldr	r10, [sp, #0x24]      
	mov	r3, r4, lsl #16     
	mov	r3, r3, asr #16     
	mul	r10, r3, r10      
	qadd	r8, r10, r9      
	mov	ip, r8, asr #12     
	ldr	r8, [r11, #0x1bc]      
	qadd	ip, r8, ip      
	ldr	r4, [sp, #0x28]      
	ldr	r5, [sp, #0x2c]      
	mul	r4, r1, r4      
	mul	r5, r2, r5      
	qadd	r10, r5, r4      
	ldr	r5, [sp, #0x30]      
	mul	r5, r3, r5      
	qadd	r9, r5, r10      
	mov	r4, r9, asr #12     
	ldr	r9, [r11, #0x1c0]      
	qadd	r4, r9, r4      
	ldr	r10, [sp, #0x34]      
	mul	r0, r2, r0      
	mul	r1, r10, r1      
	qadd	r2, r0, r1      
	ldr	r1, [sp, #0x38]      
	mul	r3, r1, r3      
	qadd	r3, r3, r2      
	mov	r1, r3, asr #12     
	ldr	r2, [r11, #0x1c4]      
	qadd	r3, r2, r1      
	add	r0, r11, #0x108      
	str	r3, [r0, #0x6c]      
	str	ip, [r0, #0x64]      
	str	r4, [r0, #0x68]      
	add	sp, sp, #0x40      
	ldmia	sp!, {r4, r5, r6, r7, r8, r9, r10} 
	bx	lr        
._gteNCT__000001ae:	.long 0x000001ae       
._gteNCT__000001b2:	.long 0x000001b2       
._gteNCT__000001b6:	.long 0x000001b6       
._gteNCT__00007fff:	.long 0x00007fff       
._gteNCT__000001ca:	.long 0x000001ca       
._gteNCT__000001ce:	.long 0x000001ce       
._gteNCT__000001d2:	.long 0x000001d2       
._gteNCT__000001d6:	.long 0x000001d6       
		        
.align 4
_gteDPCT__:	        
	stmdb	sp!, {r4, r5, r6, r7, r8, r9, r10} 
	ldr	r5, [r11, #0x1dc]      
	ldrb	r10, [r11, #0x158]      
	ldr	r1, ._gteDPCT__ffff8000       
	ldr	r2, ._gteDPCT__00007fff       
	mov	r3, #0x128       
	ldrsh	r3, [r11, r3]      
	sub	r4, r5, r10, lsl #4    
	cmp	r4, r1       
	movlt	r4, r1       
	cmp	r4, r2       
	movge	r4, r2       
	sub	sp, sp, #16      
	mov	r0, r10, lsl #16     
	mul	r4, r3, r4      
	qadd	r10, r0, r4      
	ldrb	ip, [r11, #0x159]      
	ldr	r4, [r11, #0x1e0]      
	mov	r6, ip, lsl #16     
	sub	r8, r4, ip, lsl #4    
	cmp	r8, r1       
	movlt	r8, r1       
	cmp	r8, r2       
	movge	r8, r2       
	mul	r8, r3, r8      
	qadd	r8, r6, r8      
	ldrb	r9, [r11, #0x15a]      
	ldr	ip, [r11, #0x1e4]      
	mov	r7, r9, lsl #16     
	sub	r0, ip, r9, lsl #4    
	cmp	r0, r1       
	movlt	r0, r1       
	cmp	r0, r2       
	movge	r0, r2       
	mul	r0, r3, r0      
	qadd	r9, r7, r0      
	ldr	r6, [r11, #0x15c]      
	mov	r0, r9, asr #16     
	str	r6, [r11, #0x158]      
	ldrb	r7, [r11, #0x158]      
	ldrb	r6, [r11, #0x123]      
	sub	r9, r5, r7, lsl #4    
	mov	r7, r7, lsl #16     
	str	r7, [sp, #4]      
	mov	r10, r10, asr #16     
	ldr	r7, [r11, #0x160]      
	mov	r8, r8, asr #16     
	bic	r10, r10, r10, asr #31    
	bic	r8, r8, r8, asr #31    
	bic	r0, r0, r0, asr #31    
	cmp	r8, #0xff       
	movge	r8, #0xff       
	cmp	r9, r1       
	movlt	r9, r1       
	cmp	r10, #0xff       
	movge	r10, #0xff       
	cmp	r0, #0xff       
	movge	r0, #0xff       
	str	r7, [sp, #12]      
	cmp	r9, r2       
	movge	r9, r2       
	strb	r10, [r11, #0x160]      
	strb	r8, [r11, #0x161]      
	strb	r0, [r11, #0x162]      
	strb	r6, [r11, #0x163]      
	ldr	r8, [sp, #4]      
	mul	r9, r3, r9      
	qadd	r7, r8, r9      
	ldrb	r9, [r11, #0x159]      
	sub	r10, r4, r9, lsl #4    
	cmp	r10, r1       
	movlt	r10, r1       
	cmp	r10, r2       
	movge	r10, r2       
	mov	r0, r9, lsl #16     
	mul	r6, r3, r10      
	qadd	r10, r0, r6      
	ldrb	r8, [r11, #0x15a]      
	sub	r9, ip, r8, lsl #4    
	cmp	r9, r1       
	movlt	r9, r1       
	cmp	r9, r2       
	movge	r9, r2       
	mov	r0, r8, lsl #16     
	mul	r9, r3, r9      
	qadd	r6, r0, r9      
	ldr	r9, [sp, #12]      
	mov	r8, r6, asr #16     
	str	r9, [r11, #0x158]      
	ldrb	r0, [r11, #0x158]      
	ldr	r9, [r11, #0x160]      
	sub	r5, r5, r0, lsl #4    
	mov	r7, r7, asr #16     
	mov	r10, r10, asr #16     
	cmp	r5, r1       
	movlt	r5, r1       
	bic	r7, r7, r7, asr #31    
	bic	r10, r10, r10, asr #31    
	bic	r8, r8, r8, asr #31    
	cmp	r7, #0xff       
	movge	r7, #0xff       
	cmp	r10, #0xff       
	movge	r10, #0xff       
	cmp	r8, #0xff       
	movge	r8, #0xff       
	cmp	r5, r2       
	movge	r5, r2       
	str	r9, [r11, #0x15c]      
	strb	r7, [r11, #0x160]      
	strb	r10, [r11, #0x161]      
	strb	r8, [r11, #0x162]      
	mov	r0, r0, lsl #16     
	mul	r5, r3, r5      
	qadd	r5, r0, r5      
	ldrb	r6, [r11, #0x159]      
	sub	r4, r4, r6, lsl #4    
	cmp	r4, r1       
	movlt	r4, r1       
	cmp	r4, r2       
	movge	r4, r2       
	mov	r6, r6, lsl #16     
	mul	r4, r3, r4      
	qadd	r6, r6, r4      
	ldrb	r4, [r11, #0x15a]      
	sub	ip, ip, r4, lsl #4    
	cmp	ip, r1       
	movge	r1, ip       
	movlt	r1, r1       
	cmp	r1, r2       
	movlt	r2, r1       
	movge	r2, r2       
	mul	r3, r2, r3      
	mov	r4, r4, lsl #16     
	qadd	r1, r4, r3      
	add	r0, r11, #0x108      
	mov	r5, r5, asr #12     
	mov	r6, r6, asr #12     
	mov	r3, r1, asr #12     
	str	r3, [r0, #0x6c]      
	str	r5, [r0, #0x64]      
	str	r6, [r0, #0x68]      
	add	sp, sp, #16      
	ldmia	sp!, {r4, r5, r6, r7, r8, r9, r10} 
	bx	lr        
._gteDPCT__ffff8000:	.long 0xffff8000       
._gteDPCT__00007fff:	.long 0x00007fff       
		        
.align 4
_gteNCCT__:	        
	stmdb	sp!, {r4, r5, r6, r7, r8, r9, r10} 
	mov	ip, #0x1a8       
	mov	r5, #0x108       
	add	r4, r5, #2      
	add	r9, ip, #2      
	ldrsh	r6, [r11, r5]      
	ldrsh	r5, [r11, r4]      
	ldrsh	r4, [r11, ip]      
	ldrsh	ip, [r11, r9]      
	mov	r3, #0x10c       
	ldrsh	r3, [r11, r3]      
	sub	sp, sp, #0x50      
	mul	r1, r6, r4      
	mul	r10, r5, ip      
	qadd	r8, r10, r1      
	mov	r7, #0x1ac       
	ldrsh	r1, [r11, r7]      
	mul	r0, r3, r1      
	qadd	r8, r0, r8      
	ldr	r2, ._gteNCCT__000001ae       
	mov	r9, #0x1b0       
	ldrsh	r2, [r11, r2]      
	ldrsh	r7, [r11, r9]      
	mul	r10, r6, r2      
	str	r7, [sp, #12]      
	mul	r7, r5, r7      
	qadd	r7, r7, r10      
	ldr	r0, ._gteNCCT__000001b2       
	ldrsh	r9, [r11, r0]      
	str	r9, [sp, #16]      
	mul	r9, r3, r9      
	qadd	r0, r9, r7      
	mov	r10, #0x1b4       
	ldrsh	r9, [r11, r10]      
	ldr	r10, ._gteNCCT__000001b6       
	str	r9, [sp, #20]      
	ldr	r9, [sp, #20]      
	ldrsh	r10, [r11, r10]      
	mul	r6, r9, r6      
	str	r10, [sp, #24]      
	mul	r5, r10, r5      
	qadd	r6, r5, r6      
	mov	r5, #0x1b8       
	ldrsh	r9, [r11, r5]      
	str	r9, [sp, #28]      
	mul	r3, r9, r3      
	qadd	r9, r3, r6      
	mov	r5, #0x1c8       
	ldr	r3, ._gteNCCT__00007fff       
	mov	r7, r0, asr #12     
	ldrsh	r0, [r11, r5]      
	mov	r10, r8, asr #12     
	ldr	r5, ._gteNCCT__000001ca       
	bic	r8, r10, r10, asr #31    
	bic	r10, r7, r7, asr #31    
	str	r0, [sp, #32]      
	cmp	r10, r3       
	movge	r10, r3       
	ldrsh	r5, [r11, r5]      
	cmp	r8, r3       
	movge	r8, r3       
	mov	r7, r10, lsl #16     
	ldr	r10, [sp, #32]      
	mov	r0, r8, lsl #16     
	mov	r8, r0, asr #16     
	mov	r6, r9, asr #12     
	mov	r0, r7, asr #16     
	str	r5, [sp, #0x24]      
	bic	r9, r6, r6, asr #31    
	mul	r10, r8, r10      
	mul	r5, r0, r5      
	qadd	r10, r5, r10      
	mov	r5, #0x1cc       
	ldrsh	r5, [r11, r5]      
	cmp	r9, r3       
	movge	r9, r3       
	mov	r6, r9, lsl #16     
	mov	r6, r6, asr #16     
	str	r5, [sp, #0x28]      
	mul	r5, r6, r5      
	qadd	r9, r5, r10      
	ldr	r10, [r11, #0x1bc]      
	mov	r5, r9, asr #12     
	qadd	r5, r10, r5      
	ldr	r9, ._gteNCCT__000001ce       
	ldrsh	r10, [r11, r9]      
	mov	r9, #0x1d0       
	str	r10, [sp, #0x2c]      
	ldrsh	r10, [r11, r9]      
	ldr	r9, [sp, #0x2c]      
	str	r10, [sp, #0x30]      
	mul	r9, r8, r9      
	mul	r10, r0, r10      
	qadd	r9, r10, r9      
	ldr	r10, ._gteNCCT__000001d2       
	ldrsh	r10, [r11, r10]      
	str	r10, [sp, #0x34]      
	mul	r10, r6, r10      
	qadd	r9, r10, r9      
	ldr	r10, [r11, #0x1c0]      
	mov	r9, r9, asr #12     
	qadd	r9, r10, r9      
	mov	r10, #0x1d4       
	ldrsh	r10, [r11, r10]      
	str	r10, [sp, #0x38]      
	ldr	r10, ._gteNCCT__000001d6       
	ldrsh	r10, [r11, r10]      
	str	r10, [sp, #0x3c]      
	ldr	r10, [sp, #0x38]      
	mul	r8, r10, r8      
	ldr	r10, [sp, #0x3c]      
	mul	r0, r10, r0      
	qadd	r10, r0, r8      
	mov	r8, #0x1d8       
	ldrsh	r0, [r11, r8]      
	mul	r6, r0, r6      
	qadd	r8, r6, r10      
	ldr	r6, [r11, #0x1c4]      
	mov	r7, r8, asr #12     
	qadd	r10, r6, r7      
	ldrb	r6, [r11, #0x120]      
	bic	r8, r9, r9, asr #31    
	str	r6, [sp, #0x40]      
	ldrb	r6, [r11, #0x121]      
	bic	r7, r10, r10, asr #31    
	str	r6, [sp, #0x44]      
	ldrb	r6, [r11, #0x122]      
	cmp	r8, r3       
	movge	r8, r3       
	str	r6, [sp, #0x48]      
	cmp	r7, r3       
	movge	r7, r3       
	bic	r5, r5, r5, asr #31    
	cmp	r5, r3       
	movge	r5, r3       
	mov	r9, r8, lsl #16     
	mov	r10, r7, lsl #16     
	ldr	r8, [sp, #0x40]      
	ldr	r7, [sp, #0x48]      
	mov	r5, r5, lsl #16     
	mov	r5, r5, asr #16     
	mov	r10, r10, asr #16     
	mul	r5, r8, r5      
	mul	r10, r7, r10      
	ldr	r6, [sp, #0x44]      
	mov	r8, #0x110       
	mov	r9, r9, asr #16     
	mov	r5, r5, asr #12     
	mov	r10, r10, asr #12     
	cmp	r5, #0xff       
	movge	r5, #0xff       
	cmp	r10, #0xff       
	movge	r10, #0xff       
	mul	r9, r6, r9      
	add	r6, r8, #2      
	ldrsh	r7, [r11, r8]      
	ldrsh	r6, [r11, r6]      
	ldrb	r8, [r11, #0x123]      
	str	r5, [sp, #0x4c]      
	str	r10, [sp, #4]      
	ldr	r10, [r11, #0x160]      
	mov	r5, #0x114       
	ldrsh	r5, [r11, r5]      
	strb	r8, [r11, #0x163]      
	str	r10, [r11, #0x158]      
	ldr	r10, [sp, #0x4c]      
	mov	r9, r9, asr #12     
	cmp	r9, #0xff       
	movge	r9, #0xff       
	strb	r10, [r11, #0x160]      
	strb	r9, [r11, #0x161]      
	ldr	r9, [sp, #4]      
	mul	r10, r7, r4      
	strb	r9, [r11, #0x162]      
	strb	r8, [sp, #0x4c]      
	mul	r8, r6, ip      
	qadd	r8, r8, r10      
	mul	r9, r5, r1      
	qadd	r8, r9, r8      
	ldr	r10, [sp, #12]      
	mul	r9, r7, r2      
	mul	r10, r6, r10      
	qadd	r9, r10, r9      
	ldr	r10, [sp, #16]      
	mul	r10, r5, r10      
	qadd	r10, r10, r9      
	ldr	r9, [sp, #20]      
	mul	r7, r9, r7      
	ldr	r9, [sp, #24]      
	mul	r6, r9, r6      
	qadd	r7, r6, r7      
	ldr	r9, [sp, #28]      
	mul	r5, r9, r5      
	qadd	r9, r5, r7      
	mov	r7, r8, asr #12     
	mov	r6, r10, asr #12     
	bic	r8, r7, r7, asr #31    
	bic	r10, r6, r6, asr #31    
	cmp	r8, r3       
	movge	r8, r3       
	cmp	r10, r3       
	movge	r10, r3       
	mov	r7, r8, lsl #16     
	mov	r6, r10, lsl #16     
	ldr	r8, [sp, #0x24]      
	ldr	r10, [sp, #32]      
	mov	r5, r9, asr #12     
	mov	r7, r7, asr #16     
	mov	r6, r6, asr #16     
	bic	r9, r5, r5, asr #31    
	mul	r10, r7, r10      
	mul	r8, r6, r8      
	qadd	r10, r8, r10      
	ldr	r8, [sp, #0x28]      
	cmp	r9, r3       
	movge	r9, r3       
	mov	r5, r9, lsl #16     
	mov	r5, r5, asr #16     
	mul	r8, r5, r8      
	qadd	r9, r8, r10      
	mov	r8, r9, asr #12     
	ldr	r9, [r11, #0x1bc]      
	qadd	r8, r9, r8      
	ldr	r9, [sp, #0x2c]      
	ldr	r10, [sp, #0x30]      
	mul	r9, r7, r9      
	mul	r10, r6, r10      
	qadd	r9, r10, r9      
	ldr	r10, [sp, #0x34]      
	mul	r10, r5, r10      
	qadd	r9, r10, r9      
	ldr	r10, [r11, #0x1c0]      
	mov	r9, r9, asr #12     
	qadd	r9, r10, r9      
	ldr	r10, [sp, #0x38]      
	mul	r7, r10, r7      
	ldr	r10, [sp, #0x3c]      
	mul	r6, r10, r6      
	qadd	r7, r6, r7      
	mul	r5, r0, r5      
	qadd	r10, r5, r7      
	ldr	r5, [r11, #0x1c4]      
	mov	r6, r10, asr #12     
	qadd	r7, r5, r6      
	bic	r10, r8, r8, asr #31    
	bic	r5, r9, r9, asr #31    
	bic	r6, r7, r7, asr #31    
	ldr	r9, [sp, #0x40]      
	cmp	r5, r3       
	movge	r5, r3       
	cmp	r6, r3       
	movge	r6, r3       
	cmp	r10, r3       
	movge	r10, r3       
	mov	r8, r5, lsl #16     
	mov	r7, r6, lsl #16     
	ldr	r5, [sp, #0x44]      
	ldr	r6, [sp, #0x48]      
	mov	r10, r10, lsl #16     
	mov	r10, r10, asr #16     
	mul	r10, r9, r10      
	mov	r8, r8, asr #16     
	mov	r9, #0x118       
	mov	r7, r7, asr #16     
	mul	r8, r5, r8      
	mul	r7, r6, r7      
	add	r5, r9, #2      
	ldrsh	r6, [r11, r9]      
	ldr	r9, [r11, #0x160]      
	ldrsh	r5, [r11, r5]      
	str	r9, [r11, #0x15c]      
	ldrb	r9, [sp, #0x4c]      
	mov	r10, r10, asr #12     
	mov	r8, r8, asr #12     
	mov	r7, r7, asr #12     
	cmp	r7, #0xff       
	movge	r7, #0xff       
	cmp	r10, #0xff       
	movge	r10, #0xff       
	cmp	r8, #0xff       
	movge	r8, #0xff       
	strb	r7, [r11, #0x162]      
	strb	r9, [r11, #0x163]      
	strb	r10, [r11, #0x160]      
	strb	r8, [r11, #0x161]      
	mov	r7, #0x11c       
	ldrsh	r7, [r11, r7]      
	mul	r4, r6, r4      
	mul	ip, r5, ip      
	qadd	r4, ip, r4      
	mul	r1, r7, r1      
	qadd	r4, r1, r4      
	ldr	ip, [sp, #12]      
	mul	r2, r6, r2      
	mul	ip, r5, ip      
	qadd	r2, ip, r2      
	ldr	r1, [sp, #16]      
	mul	r1, r7, r1      
	qadd	r1, r1, r2      
	ldr	r10, [sp, #20]      
	ldr	r9, [sp, #24]      
	mul	r6, r10, r6      
	mul	r5, r9, r5      
	qadd	ip, r5, r6      
	ldr	r2, [sp, #28]      
	mul	r2, r7, r2      
	qadd	r9, r2, ip      
	mov	r6, r4, asr #12     
	bic	r5, r6, r6, asr #31    
	mov	r4, r1, asr #12     
	cmp	r5, r3       
	movge	r5, r3       
	bic	r10, r4, r4, asr #31    
	cmp	r10, r3       
	movge	r10, r3       
	mov	ip, r5, lsl #16     
	ldr	r4, [sp, #0x24]      
	ldr	r5, [sp, #32]      
	mov	r1, r10, lsl #16     
	mov	r6, r9, asr #12     
	mov	ip, ip, asr #16     
	mov	r1, r1, asr #16     
	bic	r10, r6, r6, asr #31    
	mul	r5, ip, r5      
	mul	r4, r1, r4      
	qadd	r9, r4, r5      
	cmp	r10, r3       
	movge	r10, r3       
	ldr	r6, [sp, #0x28]      
	mov	r2, r10, lsl #16     
	mov	r2, r2, asr #16     
	mul	r6, r2, r6      
	qadd	r10, r6, r9      
	mov	r4, r10, asr #12     
	ldr	r5, [r11, #0x1bc]      
	qadd	r4, r5, r4      
	ldr	r9, [sp, #0x2c]      
	ldr	r5, [sp, #0x30]      
	mul	r9, ip, r9      
	mul	r5, r1, r5      
	qadd	r6, r5, r9      
	ldr	r10, [sp, #0x34]      
	mul	r10, r2, r10      
	qadd	r9, r10, r6      
	mov	r5, r9, asr #12     
	ldr	r6, [r11, #0x1c0]      
	qadd	r5, r6, r5      
	ldr	r10, [sp, #0x38]      
	mul	ip, r10, ip      
	ldr	r10, [sp, #0x3c]      
	mul	r1, r10, r1      
	qadd	r6, r1, ip      
	mul	r0, r2, r0      
	qadd	r1, r0, r6      
	mov	r9, r1, asr #12     
	ldr	ip, [r11, #0x1c4]      
	qadd	r6, ip, r9      
	bic	r5, r5, r5, asr #31    
	bic	r1, r6, r6, asr #31    
	bic	r2, r4, r4, asr #31    
	cmp	r5, r3       
	movlt	ip, r5       
	movge	ip, r3       
	cmp	r1, r3       
	movge	r1, r3       
	cmp	r2, r3       
	movge	r2, r3       
	mov	r6, ip, lsl #16     
	mov	r5, r1, lsl #16     
	mov	r9, r2, lsl #16     
	mov	r2, r9, asr #16     
	mov	r3, r6, asr #16     
	mov	ip, r5, asr #16     
	add	r5, sp, #0x40      
	ldmia	r5, {r5, r6, r9}     
	mul	r2, r5, r2      
	mul	r3, r6, r3      
	mul	ip, r9, ip      
	add	r0, r11, #0x108      
	mov	r2, r2, asr #8     
	mov	r3, r3, asr #8     
	mov	r1, ip, asr #8     
	str	r1, [r0, #0x6c]      
	str	r2, [r0, #0x64]      
	str	r3, [r0, #0x68]      
	add	sp, sp, #0x50      
	ldmia	sp!, {r4, r5, r6, r7, r8, r9, r10} 
	bx	lr        
._gteNCCT__000001ae:	.long 0x000001ae       
._gteNCCT__000001b2:	.long 0x000001b2       
._gteNCCT__000001b6:	.long 0x000001b6       
._gteNCCT__00007fff:	.long 0x00007fff       
._gteNCCT__000001ca:	.long 0x000001ca       
._gteNCCT__000001ce:	.long 0x000001ce       
._gteNCCT__000001d2:	.long 0x000001d2       
._gteNCCT__000001d6:	.long 0x000001d6       
		        
.align 4
_gteRTPT__:	        
/*			     
#ifdef USE_OLD_GTE_WITHOUT_PATCH
	mov	r1, #0x108       
	mov	r2, #0x188       
	stmdb	sp!, {r4, r5, r6, r7, r8, r9, r10} 
	ldrsh	r7, [r11, r1]      
	add	ip, r1, #2      
	add	r8, r2, #2      
	mov	r4, #0x154       
	ldrsh	r1, [r11, ip]      
	ldrsh	r5, [r11, r2]      
	ldrsh	r6, [r11, r8]      
	ldrh	r10, [r11, r4]      
	mov	r0, #0x148       
	mov	r3, #0x10c       
	strh	r10, [r11, r0]      
	sub	sp, sp, #16      
	ldrsh	r0, [r11, r3]      
	mul	r5, r7, r5      
	mul	r6, r1, r6      
	qadd	r8, r6, r5      
	mov	ip, #0x18c       
	ldrsh	r2, [r11, ip]      
	mul	r2, r0, r2      
	qadd	r4, r2, r8      
	mov	r10, r4, asr #12     
	add	r2, r11, #0x188      
	ldr	r4, [r11, #0x19c]      
	qadd	r10, r4, r10      
	ldr	r6, ._gteRTPT__0000018e       
	mov	r5, #0x190       
	ldrsh	r8, [r11, r6]      
	ldrsh	ip, [r11, r5]      
	mul	r8, r7, r8      
	mul	ip, r1, ip      
	qadd	r5, ip, r8      
	ldr	r6, ._gteRTPT__00000192       
	ldrsh	ip, [r11, r6]      
	mul	ip, r0, ip      
	qadd	r6, ip, r5      
	mov	r8, r6, asr #12     
	ldr	ip, [r11, #0x1a0]      
	qadd	r8, ip, r8      
	mov	r5, #0x194       
	ldrsh	r6, [r11, r5]      
	add	r5, r5, #2      
	ldrsh	r5, [r11, r5]      
	mul	r7, r6, r7      
	mul	r1, r5, r1      
	qadd	r1, r1, r7      
	mov	r5, #0x198       
	ldrsh	r5, [r11, r5]      
	mul	r0, r5, r0      
	qadd	r6, r0, r1      
	mov	r3, r6, asr #12     
	ldr	r1, [r11, #0x1a4]      
	qadd	r3, r1, r3      
	ldr	r6, ._gteRTPT__00007fff       
	cmp	r10, r6       
	ble	._gteRTPT__de4        
	ldr	r0, [r11, #0x204]      
	orr	r5, r0, #0x81000000      
	str	r5, [r11, #0x204]      
._gteRTPT__894:	        
	ldr	r5, ._gteRTPT__00007fff       
	cmp	r8, r5       
	ble	._gteRTPT__dc4        
	ldr	r8, [r11, #0x204]      
	orr	r10, r8, #0x80000000      
	orr	r7, r10, #0x800000      
	str	r7, [r11, #0x204]      
._gteRTPT__8b0:	        
	ldr	r7, ._gteRTPT__0000ffff       
	cmp	r3, r7       
	ble	._gteRTPT__da0        
	ldr	r10, [r11, #0x204]      
	orr	r0, r10, #0x80000000      
	orr	r3, r0, #0x40000      
	str	r3, [r11, #0x204]      
._gteRTPT__8cc:	        
	mov	r0, #0x1f0       
	ldrh	r8, [r11, r0]      
	mov	r3, #0x14c       
	mov	r10, r8, lsl #16     
	movs	r8, r10, asr #16     
	strh	r7, [r11, r3]      
	bmi	._gteRTPT__8f0        
	cmp	r8, r7, lsl #1     
	blt	._gteRTPT__f78        
._gteRTPT__8f0:	        
	ldr	r7, ._gteRTPT__0001ffff       
._gteRTPT__8f4:	        
	ldr	r3, [r2, #0x60]      
	mul	r10, r6, r7      
	qadd	r6, r3, r10      
	ldr	r10, ._gteRTPT__fffffc00       
	ldr	r3, ._gteRTPT__000003ff       
	mov	r8, r6, asr #16     
	cmp	r8, r10       
	movlt	r8, r10       
	cmp	r8, r3       
	movge	r8, r3       
	mov	r0, #0x138       
	strh	r8, [r11, r0]      
	mul	r5, r7, r5      
	ldr	r6, [r2, #0x64]      
	qadd	r8, r6, r5      
	mov	r7, #0x110       
	mov	r6, r8, asr #16     
	add	r0, r7, #2      
	mov	r5, #0x188       
	ldrsh	r8, [r11, r7]      
	cmp	r6, r10       
	movge	r10, r6       
	movlt	r10, r10       
	ldrsh	r7, [r11, r0]      
	ldr	r6, ._gteRTPT__0000013a       
	ldrsh	r0, [r11, r5]      
	add	r5, r5, #2      
	ldrsh	r5, [r11, r5]      
	cmp	r10, r3       
	movlt	r3, r10       
	movge	r3, r3       
	strh	r3, [r11, r6]      
	mov	r3, #0x114       
	ldrsh	r3, [r11, r3]      
	mul	r0, r8, r0      
	mul	r5, r7, r5      
	qadd	r6, r5, r0      
	mov	r10, #0x18c       
	ldrsh	r5, [r11, r10]      
	mul	r5, r3, r5      
	qadd	r10, r5, r6      
	mov	r0, r10, asr #12     
	qadd	r0, r4, r0      
	ldr	r5, ._gteRTPT__0000018e       
	mov	r6, #0x190       
	ldrsh	r10, [r11, r5]      
	ldrsh	r5, [r11, r6]      
	mul	r10, r8, r10      
	mul	r5, r7, r5      
	qadd	r10, r5, r10      
	ldr	r6, ._gteRTPT__00000192       
	ldrsh	r5, [r11, r6]      
	mul	r5, r3, r5      
	qadd	r6, r5, r10      
	mov	r10, r6, asr #12     
	qadd	r10, ip, r10      
	mov	r5, #0x194       
	ldrsh	r6, [r11, r5]      
	add	r5, r5, #2      
	ldrsh	r5, [r11, r5]      
	mul	r8, r6, r8      
	mul	r7, r5, r7      
	qadd	r7, r7, r8      
	mov	r5, #0x198       
	ldrsh	r5, [r11, r5]      
	mul	r3, r5, r3      
	qadd	r3, r3, r7      
	mov	r6, r3, asr #12     
	qadd	r3, r1, r6      
	ldr	r6, ._gteRTPT__00007fff       
	cmp	r0, r6       
	ble	._gteRTPT__d84        
	ldr	r0, [r11, #0x204]      
	orr	r5, r0, #0x81000000      
	str	r5, [r11, #0x204]      
._gteRTPT__a20:	        
	ldr	r5, ._gteRTPT__00007fff       
	cmp	r10, r5       
	ble	._gteRTPT__d64        
	ldr	r10, [r11, #0x204]      
	orr	r8, r10, #0x80000000      
	orr	r7, r8, #0x800000      
	str	r7, [r11, #0x204]      
._gteRTPT__a3c:	        
	ldr	r7, ._gteRTPT__0000ffff       
	cmp	r3, r7       
	ble	._gteRTPT__d40        
	ldr	r8, [r11, #0x204]      
	orr	r0, r8, #0x80000000      
	orr	r3, r0, #0x40000      
	str	r3, [r11, #0x204]      
._gteRTPT__a58:	        
	mov	r0, #0x1f0       
	ldrh	r10, [r11, r0]      
	mov	r3, #0x150       
	mov	r10, r10, lsl #16     
	movs	r8, r10, asr #16     
	strh	r7, [r11, r3]      
	bmi	._gteRTPT__a7c        
	cmp	r8, r7, lsl #1     
	blt	._gteRTPT__ebc        
._gteRTPT__a7c:	        
	ldr	r7, ._gteRTPT__0001ffff       
._gteRTPT__a80:	        
	ldr	r3, [r2, #0x60]      
	mul	r8, r6, r7      
	qadd	r0, r3, r8      
	ldr	r8, ._gteRTPT__fffffc00       
	ldr	r3, ._gteRTPT__000003ff       
	mov	r10, r0, asr #16     
	cmp	r10, r8       
	movlt	r10, r8       
	cmp	r10, r3       
	movge	r10, r3       
	mov	r6, #0x13c       
	strh	r10, [r11, r6]      
	mul	r0, r5, r7      
	ldr	r5, [r2, #0x64]      
	qadd	r7, r5, r0      
	mov	r6, #0x118       
	mov	r0, r7, asr #16     
	add	r10, r6, #2      
	mov	r5, #0x188       
	cmp	r0, r8       
	movge	r8, r0       
	movlt	r8, r8       
	ldrsh	r7, [r11, r6]      
	ldr	r0, ._gteRTPT__0000013e       
	ldrsh	r6, [r11, r10]      
	ldrsh	r10, [r11, r5]      
	add	r5, r5, #2      
	ldrsh	r5, [r11, r5]      
	cmp	r8, r3       
	movlt	r3, r8       
	movge	r3, r3       
	strh	r3, [r11, r0]      
	mov	r3, #0x11c       
	ldrsh	r3, [r11, r3]      
	mul	r8, r7, r10      
	mul	r5, r6, r5      
	qadd	r10, r5, r8      
	mov	r0, #0x18c       
	ldrsh	r8, [r11, r0]      
	mul	r8, r3, r8      
	qadd	r0, r8, r10      
	mov	r5, r0, asr #12     
	qadd	r4, r4, r5      
	ldr	r10, ._gteRTPT__0000018e       
	mov	r0, #0x190       
	ldrsh	r8, [r11, r10]      
	ldrsh	r5, [r11, r0]      
	mul	r8, r7, r8      
	mul	r5, r6, r5      
	qadd	r0, r5, r8      
	ldr	r10, ._gteRTPT__00000192       
	ldrsh	r8, [r11, r10]      
	mul	r8, r3, r8      
	qadd	r10, r8, r0      
	mov	r5, r10, asr #12     
	qadd	ip, ip, r5      
	mov	r0, #0x194       
	add	r10, r0, #2      
	ldrsh	r8, [r11, r0]      
	ldrsh	r5, [r11, r10]      
	mul	r7, r8, r7      
	mul	r6, r5, r6      
	qadd	r6, r6, r7      
	mov	r0, #0x198       
	ldrsh	r10, [r11, r0]      
	mul	r3, r10, r3      
	qadd	r5, r3, r6      
	mov	r3, r5, asr #12     
	qadd	r1, r1, r3      
	ldr	r6, ._gteRTPT__00007fff       
	cmp	r4, r6       
	ble	._gteRTPT__d24        
	ldr	r7, [r11, #0x204]      
	orr	r3, r7, #0x81000000      
	str	r3, [r11, #0x204]      
._gteRTPT__bac:	        
	ldr	r5, ._gteRTPT__00007fff       
	cmp	ip, r5       
	ble	._gteRTPT__d04        
	ldr	r0, [r11, #0x204]      
	orr	r10, r0, #0x80000000      
	orr	r3, r10, #0x800000      
	str	r3, [r11, #0x204]      
._gteRTPT__bc8:	        
	ldr	r7, ._gteRTPT__0000ffff       
	cmp	r1, r7       
	ble	._gteRTPT__ce0        
	ldr	r10, [r11, #0x204]      
	orr	r3, r10, #0x80000000      
	orr	r8, r3, #0x40000      
	str	r8, [r11, #0x204]      
._gteRTPT__be4:	        
	mov	r3, #0x1f0       
	ldrh	r8, [r11, r3]      
	mov	r0, #0x154       
	mov	r10, r8, lsl #16     
	movs	r8, r10, asr #16     
	strh	r7, [r11, r0]      
	bmi	._gteRTPT__c08        
	cmp	r8, r7, lsl #1     
	blt	._gteRTPT__e00        
._gteRTPT__c08:	        
	ldr	r3, ._gteRTPT__0001ffff       
._gteRTPT__c0c:	        
	ldr	r7, [r2, #0x60]      
	mul	r8, r6, r3      
	qadd	r10, r7, r8      
	ldr	r8, ._gteRTPT__fffffc00       
	ldr	r7, ._gteRTPT__000003ff       
	mov	r0, r10, asr #16     
	cmp	r0, r8       
	movlt	r0, r8       
	cmp	r0, r7       
	movge	r0, r7       
	mov	r10, #0x140       
	strh	r0, [r11, r10]      
	mul	r0, r5, r3      
	ldr	r10, [r2, #0x64]      
	qadd	r10, r10, r0      
	mov	r10, r10, asr #16     
	cmp	r10, r8       
	movge	r8, r10       
	movlt	r8, r8       
	ldr	r10, ._gteRTPT__00000142       
	cmp	r8, r7       
	movge	r8, r7       
	add	r7, r11, #0x108      
	strh	r8, [r11, r10]      
	str	r1, [r7, #0x6c]      
	mov	r1, #0x1f4       
	str	r4, [r7, #0x64]      
	str	ip, [r7, #0x68]      
	ldrsh	ip, [r11, r1]      
	ldr	r2, [r2, #0x70]      
	mul	r3, ip, r3      
	qadd	r3, r2, r3      
	ldr	r2, ._gteRTPT__00000fff       
	mov	r3, r3, asr #12     
	mov	r1, #0x12c       
	mov	ip, #0x130       
	cmp	r3, r2       
	strh	r6, [r11, r1]      
	strh	r5, [r11, ip]      
	str	r3, [r11, #0x168]      
	bgt	._gteRTPT__1034        
	cmp	r3, #0       
	ldrlt	r3, [r11, #0x204]      
	movge	r2, r3, lsl #16     
	orrlt	r3, r3, #0x1000      
	movge	r2, r2, lsr #16     
	movlt	r2, #0       
	strlt	r3, [r11, #0x204]      
._gteRTPT__ccc:	        
	mov	r1, #0x128       
	strh	r2, [r11, r1]      
	add	sp, sp, #16      
	ldmia	sp!, {r4, r5, r6, r7, r8, r9, r10} 
	bx	lr        
._gteRTPT__ce0:	        
	cmp	r1, #0       
	ldrlt	r3, [r11, #0x204]      
	movge	r7, r1, lsl #16     
	orrlt	r3, r3, #0x80000000      
	orrlt	r3, r3, #0x40000      
	movge	r7, r7, lsr #16     
	movlt	r7, #0       
	strlt	r3, [r11, #0x204]      
	b	._gteRTPT__be4        
._gteRTPT__d04:	        
	cmn	ip, #0x8000       
	ldrlt	r3, [r11, #0x204]      
	movge	r5, ip       
	orrlt	r3, r3, #0x80000000      
	orrlt	r3, r3, #0x800000      
	ldrlt	r5, ._gteRTPT__ffff8000       
	strlt	r3, [r11, #0x204]      
	b	._gteRTPT__bc8        
._gteRTPT__d24:	        
	cmn	r4, #0x8000       
	ldrlt	r3, [r11, #0x204]      
	movge	r6, r4       
	orrlt	r3, r3, #0x81000000      
	ldrlt	r6, ._gteRTPT__ffff8000       
	strlt	r3, [r11, #0x204]      
	b	._gteRTPT__bac        
._gteRTPT__d40:	        
	cmp	r3, #0       
	ldrlt	r3, [r11, #0x204]      
	movge	r7, r3, lsl #16     
	orrlt	r3, r3, #0x80000000      
	orrlt	r3, r3, #0x40000      
	movge	r7, r7, lsr #16     
	movlt	r7, #0       
	strlt	r3, [r11, #0x204]      
	b	._gteRTPT__a58        
._gteRTPT__d64:	        
	cmn	r10, #0x8000       
	ldrlt	r7, [r11, #0x204]      
	movge	r5, r10       
	orrlt	r7, r7, #0x80000000      
	orrlt	r7, r7, #0x800000      
	ldrlt	r5, ._gteRTPT__ffff8000       
	strlt	r7, [r11, #0x204]      
	b	._gteRTPT__a3c        
._gteRTPT__d84:	        
	cmn	r0, #0x8000       
	ldrlt	r5, [r11, #0x204]      
	movge	r6, r0       
	orrlt	r5, r5, #0x81000000      
	ldrlt	r6, ._gteRTPT__ffff8000       
	strlt	r5, [r11, #0x204]      
	b	._gteRTPT__a20        
._gteRTPT__da0:	        
	cmp	r3, #0       
	ldrlt	r3, [r11, #0x204]      
	movge	r7, r3, lsl #16     
	orrlt	r3, r3, #0x80000000      
	orrlt	r3, r3, #0x40000      
	movge	r7, r7, lsr #16     
	movlt	r7, #0       
	strlt	r3, [r11, #0x204]      
	b	._gteRTPT__8cc        
._gteRTPT__dc4:	        
	cmn	r8, #0x8000       
	ldrlt	r7, [r11, #0x204]      
	movge	r5, r8       
	orrlt	r7, r7, #0x80000000      
	orrlt	r7, r7, #0x800000      
	ldrlt	r5, ._gteRTPT__ffff8000       
	strlt	r7, [r11, #0x204]      
	b	._gteRTPT__8b0        
._gteRTPT__de4:	        
	cmn	r10, #0x8000       
	ldrlt	r5, [r11, #0x204]      
	movge	r6, r10       
	orrlt	r5, r5, #0x81000000      
	ldrlt	r6, ._gteRTPT__ffff8000       
	strlt	r5, [r11, #0x204]      
	b	._gteRTPT__894        
._gteRTPT__e00:	        
	cmp	r7, #0x8000       
	mov	r3, #0       
	bhi	._gteRTPT__e1c        
._gteRTPT__e0c:	        
	mov	r7, r7, lsl #1     
	cmp	r7, #0x8000       
	add	r3, r3, #1      
	bls	._gteRTPT__e0c        
._gteRTPT__e1c:	        
	mov	r9, r7, lsl #17     
	ldr	r0, ._gteRTPT__reciprocals       
	mov	r8, r9, lsr #17     
	mov	r7, r8, lsl #1     
	ldrh	r8, [r7, r0]      
	mov	r9, r10, asr #16     
	orr	r8, r8, #0x10000      
	str	r9, [sp, #8]      
	mov	r9, r8, asr #31     
	mov	r7, r8, lsl r3     
	mov	r0, r9, lsl r3     
	rsb	r10, r3, #32      
	ldr	r9, [sp, #8]      
	str	r7, [sp, #4]      
	orr	r0, r0, r8, lsr r10    
	ldr	r7, [sp, #4]      
	str	r0, [sp, #12]      
	mov	r10, r9       
	mov	r0, r10, asr #31     
	mul	r7, r0, r7      
	ldr	r9, [sp, #12]      
	subs	r3, r3, #32      
	ldr	r0, [sp, #4]      
	str	r7, [sp, #8]      
	movpl	r9, r8, lsl r3     
	ldr	r7, [sp, #8]      
	mov	r3, r9       
	umull	r8, r9, r0, r10     
	mla	r3, r10, r3, r7     
	mov	r0, #0x8000       
	adds	r10, r0, r8      
	mov	r7, #0       
	add	r9, r3, r9      
	adc	r0, r7, r9      
	ldr	r7, ._gteRTPT__0001ffff       
	mov	r8, r10, lsr #16     
	orr	r3, r8, r0, lsl #16    
	cmp	r3, r7       
	movcs	r3, r7       
	b	._gteRTPT__c0c        
._gteRTPT__ebc:	        
	cmp	r7, #0x8000       
	mov	r3, #0       
	bhi	._gteRTPT__ed8        
._gteRTPT__ec8:	        
	mov	r7, r7, lsl #1     
	cmp	r7, #0x8000       
	add	r3, r3, #1      
	bls	._gteRTPT__ec8        
._gteRTPT__ed8:	        
	mov	r9, r7, lsl #17     
	ldr	r0, ._gteRTPT__reciprocals       
	mov	r8, r9, lsr #17     
	mov	r7, r8, lsl #1     
	ldrh	r8, [r7, r0]      
	mov	r9, r10, asr #16     
	orr	r8, r8, #0x10000      
	str	r9, [sp, #8]      
	mov	r9, r8, asr #31     
	mov	r7, r8, lsl r3     
	mov	r0, r9, lsl r3     
	rsb	r10, r3, #32      
	ldr	r9, [sp, #8]      
	str	r7, [sp, #4]      
	orr	r0, r0, r8, lsr r10    
	ldr	r7, [sp, #4]      
	str	r0, [sp, #12]      
	mov	r10, r9       
	mov	r0, r10, asr #31     
	mul	r7, r0, r7      
	ldr	r9, [sp, #12]      
	subs	r3, r3, #32      
	str	r7, [sp, #8]      
	ldr	r7, [sp, #4]      
	movpl	r9, r8, lsl r3     
	ldr	r0, [sp, #8]      
	mov	r3, r9       
	umull	r8, r9, r7, r10     
	mla	r3, r10, r3, r0     
	mov	r7, #0x8000       
	adds	r10, r7, r8      
	mov	r0, #0       
	add	r9, r3, r9      
	ldr	r8, ._gteRTPT__0001ffff       
	adc	r3, r0, r9      
	mov	r7, r10, lsr #16     
	orr	r7, r7, r3, lsl #16    
	cmp	r7, r8       
	movcs	r7, r8       
	b	._gteRTPT__a80        
._gteRTPT__f78:	        
	cmp	r7, #0x8000       
	mov	r3, #0       
	bhi	._gteRTPT__f94        
._gteRTPT__f84:	        
	mov	r7, r7, lsl #1     
	cmp	r7, #0x8000       
	add	r3, r3, #1      
	bls	._gteRTPT__f84        
._gteRTPT__f94:	        
	mov	r9, r7, lsl #17     
	ldr	r0, ._gteRTPT__reciprocals       
	mov	r8, r9, lsr #17     
	mov	r7, r8, lsl #1     
	ldrh	r8, [r7, r0]      
	mov	r9, r10, asr #16     
	orr	r8, r8, #0x10000      
	str	r9, [sp, #8]      
	mov	r9, r8, asr #31     
	mov	r7, r8, lsl r3     
	mov	r0, r9, lsl r3     
	rsb	r10, r3, #32      
	ldr	r9, [sp, #8]      
	str	r7, [sp, #4]      
	orr	r0, r0, r8, lsr r10    
	ldr	r7, [sp, #4]      
	str	r0, [sp, #12]      
	mov	r10, r9       
	mov	r0, r10, asr #31     
	mul	r7, r0, r7      
	ldr	r9, [sp, #12]      
	subs	r3, r3, #32      
	str	r7, [sp, #8]      
	ldr	r7, [sp, #4]      
	movpl	r9, r8, lsl r3     
	ldr	r0, [sp, #8]      
	mov	r3, r9       
	umull	r8, r9, r7, r10     
	mla	r3, r10, r3, r0     
	mov	r7, #0x8000       
	adds	r10, r7, r8      
	mov	r7, r10, lsr #16     
	mov	r0, #0       
	add	r9, r3, r9      
	ldr	r10, ._gteRTPT__0001ffff       
	adc	r3, r0, r9      
	orr	r7, r7, r3, lsl #16    
	cmp	r7, r10       
	movcs	r7, r10       
	b	._gteRTPT__8f4        
._gteRTPT__1034:	        
	ldr	ip, [r11, #0x204]      
	orr	r3, ip, #0x1000      
	str	r3, [r11, #0x204]      
	b	._gteRTPT__ccc        
._gteRTPT__0000018e:	.long 0x0000018e       
._gteRTPT__00000192:	.long 0x00000192       
._gteRTPT__00007fff:	.long 0x00007fff       
._gteRTPT__0000ffff:	.long 0x0000ffff       
._gteRTPT__0001ffff:	.long 0x0001ffff       
._gteRTPT__fffffc00:	.long 0xfffffc00       
._gteRTPT__000003ff:	.long 0x000003ff       
._gteRTPT__0000013a:	.long 0x0000013a       
._gteRTPT__0000013e:	.long 0x0000013e       
._gteRTPT__00000142:	.long 0x00000142       
._gteRTPT__00000fff:	.long 0x00000fff       
._gteRTPT__ffff8000:	.long 0xffff8000       
._gteRTPT__reciprocals:	.long __gte_reciprocals__       
#else
*/
	ldr	r2,	._gteRTPT__9318       
	stmdb	sp!,	{r4, r5, r6, r7, r8, r9, sl, lr}
	ldrsh	r4,	[r11, r2]      
	mov	r9,	#0x108       
	ldrsh	r5,	[r11, r9]      
	add	lr,	r2, #0x80      
	add	r0,	r9, #0x80      
	mov	r7,	#0x154       
	ldrsh	r8,	[r11, lr]      
	ldrsh	ip,	[r11, r0]      
	ldrh	sl,	[r11, r7]      
	sub	r6,	r0, #0x40      
	sub	r1,	lr, #0x7e      
	strh	sl,	[r11, r6]      
	sub	sp,	sp, #0x10      
	ldrsh	lr,	[r11, r1]      
	mul	r3,	ip, r5      
	mul	r9,	r8, r4      
	qadd	r7,	r9, r3      
	mov	r1,	#0x18c       
	ldrsh	r0,	[r11, r1]      
	mul	sl,	r0, lr      
	qadd	r6,	sl, r7      
	mov	r8,	r6, asr #12     
	add	r7,	r11, #0x188      
	add	sl,	r11, #0x19c      
	ldr	r2,	[r11, #412]      
	qadd	r6,	r2, r8      
	mov	r3,	#0x190       
	add	ip,	r1, #2      
	ldrsh	r9,	[r11, ip]      
	ldrsh	r0,	[r11, r3]      
	mul	r8,	r9, r5      
	mul	r2,	r0, r4      
	qadd	r3,	r2, r8      
	ldr	r1,	._gteRTPT__931c       
	ldrsh	ip,	[r11, r1]      
	mul	r9,	ip, lr      
	qadd	r0,	r9, r3      
	add	r8,	r11, #0x1a0      
	mov	r2,	r0, asr #12     
	str	r8,	[sp]       
	ldr	ip,	[r11, #416]      
	qadd	r8,	ip, r2      
	ldr	r3,	._gteRTPT__9320       
	mov	r1,	#0x194       
	ldrsh	r9,	[r11, r1]      
	ldrsh	r0,	[r11, r3]      
	mul	r2,	r9, r5      
	mul	ip,	r0, r4      
	qadd	r4,	ip, r2      
	mov	r9,	#0x198       
	ldrsh	r5,	[r11, r9]      
	mul	r1,	r5, lr      
	qadd	r0,	r1, r4      
	mov	r3,	r0, asr #12     
	add	r9,	r11, #0x1a4      
	ldr	r2,	[r11, #420]      
	qadd	r0,	r2, r3      
	ldr	r2,	._gteRTPT__9324       
	cmp	r6,	r2       
	ble	._gteRTPT__9134	       
	ldr	r5,	[r11, #516]      
	mov	lr,	r2       
	orr	r4,	r5, #0x81000000      
	str	r4,	[r11, #516]      
._gteRTPT__8b2c:		       
	ldr	r2,	._gteRTPT__9324       
	cmp	r8,	r2       
	ble	._gteRTPT__9118	       
	ldr	r1,	[r11, #516]      
	mov	r8,	r2       
	orr	ip,	r1, #0x80000000      
	orr	r3,	ip, #0x800000      
	str	r3,	[r11, #516]      
._gteRTPT__8b4c:		       
	ldr	r1,	._gteRTPT__9328       
	cmp	r0,	r1       
	ble	._gteRTPT__90f4	       
	ldr	r2,	[r11, #516]      
	mov	ip,	r1       
	orr	r0,	r2, #0x80000000      
	orr	r6,	r0, #0x40000      
	str	r6,	[r11, #516]      
._gteRTPT__8b6c:		       
	mov	r3,	#0x1f0       
	ldrh	r1,	[r11, r3]      
	mov	r4,	ip, lsl #16     
	mov	r0,	r1, lsl #16     
	sub	r5,	r3, #0xa4      
	movs	r2,	r0, asr #16     
	strh	ip,	[r11, r5]      
	mov	r1,	r4, lsr #16     
	bmi	._gteRTPT__8b98	       
	cmp	r2,	r1, lsl #1     
	blt	._gteRTPT__9280	       
._gteRTPT__8b98:		       
	ldr	r4,	._gteRTPT__932c       
._gteRTPT__8b9c:		       
	add	r0,	r7, #0x60      
	mul	r1,	r4, lr      
	str	r0,	[sp, #4]      
	ldr	ip,	[r7, #96]      
	qadd	lr,	ip, r1      
	ldr	r2,	._gteRTPT__9330       
	mov	r3,	lr, asr #16     
	cmp	r3,	r2       
	bgt	._gteRTPT__8bd0	       
	cmn	r3,	#0x400       
	movge	r3,	r3, lsl #16     
	movlt	r2,	#0xfc00       
	movge	r2,	r3, lsr #16     
._gteRTPT__8bd0:		       
	mov	lr,	#0x138       
	strh	r2,	[r11, lr]      
	mul	r3,	r4, r8      
	add	r4,	r7, #0x64      
	str	r4,	[sp, #8]      
	ldr	r5,	[r7, #100]      
	qadd	r6,	r5, r3      
	ldr	r8,	._gteRTPT__9330       
	mov	r2,	r6, asr #16     
	cmp	r2,	r8       
	movgt	ip,	r8       
	bgt	._gteRTPT__8c10	       
	cmn	r2,	#0x400       
	movge	r3,	r2, lsl #16     
	movlt	ip,	#0xfc00       
	movge	ip,	r3, lsr #16     
._gteRTPT__8c10:		       
	ldr	r2,	._gteRTPT__9334       
	mov	r3,	#0x110       
	add	r8,	r3, #0x78      
	add	lr,	r2, #0x78      
	ldrsh	r1,	[r11, lr]      
	ldrsh	r4,	[r11, r2]      
	ldrsh	r6,	[r11, r8]      
	ldrsh	r5,	[r11, r3]      
	sub	r0,	r8, #0x4e      
	strh	ip,	[r11, r0]      
	sub	ip,	lr, #0x76      
	ldrsh	lr,	[r11, ip]      
	mul	r3,	r6, r5      
	mul	r8,	r1, r4      
	qadd	r2,	r8, r3      
	mov	r8,	#0x18c       
	ldrsh	r0,	[r11, r8]      
	mul	ip,	r0, lr      
	qadd	r6,	ip, r2      
	mov	r3,	r6, asr #12     
	ldr	r1,	[sl]       
	qadd	r6,	r1, r3      
	mov	r0,	#0x190       
	add	ip,	r8, #2      
	ldrsh	r2,	[r11, ip]      
	ldrsh	r1,	[r11, r0]      
	mul	r3,	r2, r5      
	mul	r8,	r1, r4      
	qadd	r2,	r8, r3      
	ldr	ip,	._gteRTPT__931c       
	ldrsh	r0,	[r11, ip]      
	mul	r3,	r0, lr      
	qadd	r1,	r3, r2      
	ldr	r8,	[sp]       
	mov	ip,	r1, asr #12     
	ldr	r0,	[r8]       
	qadd	r8,	r0, ip      
	ldr	r3,	._gteRTPT__9320       
	mov	r2,	#0x194       
	ldrsh	ip,	[r11, r2]      
	ldrsh	r1,	[r11, r3]      
	mul	r0,	ip, r5      
	mul	r2,	r1, r4      
	qadd	r3,	r2, r0      
	mov	r4,	#0x198       
	ldrsh	r0,	[r11, r4]      
	mul	r5,	r0, lr      
	qadd	r1,	r5, r3      
	mov	ip,	r1, asr #12     
	ldr	r2,	[r9]       
	qadd	r0,	r2, ip      
	ldr	r2,	._gteRTPT__9324       
	cmp	r6,	r2       
	ble	._gteRTPT__90d8	       
	ldr	r5,	[r11, #516]      
	mov	lr,	r2       
	orr	r4,	r5, #0x81000000      
	str	r4,	[r11, #516]      
._gteRTPT__8cf8:		       
	ldr	r2,	._gteRTPT__9324       
	cmp	r8,	r2       
	ble	._gteRTPT__90bc	       
	ldr	r3,	[r11, #516]      
	mov	r8,	r2       
	orr	ip,	r3, #0x80000000      
	orr	r1,	ip, #0x800000      
	str	r1,	[r11, #516]      
._gteRTPT__8d18:		       
	ldr	r1,	._gteRTPT__9328       
	cmp	r0,	r1       
	ble	._gteRTPT__9098	       
	ldr	r4,	[r11, #516]      
	mov	ip,	r1       
	orr	r2,	r4, #0x80000000      
	orr	r0,	r2, #0x40000      
	str	r0,	[r11, #516]      
._gteRTPT__8d38:		       
	mov	r3,	#0x1f0       
	ldrh	r2,	[r11, r3]      
	sub	r1,	r3, #0xa0      
	mov	r0,	r2, lsl #16     
	mov	r5,	ip, lsl #16     
	movs	r2,	r0, asr #16     
	strh	ip,	[r11, r1]      
	mov	r1,	r5, lsr #16     
	bmi	._gteRTPT__8d64	       
	cmp	r2,	r1, lsl #1     
	blt	._gteRTPT__91e8	       
._gteRTPT__8d64:		       
	ldr	r4,	._gteRTPT__932c       
._gteRTPT__8d68:		       
	ldr	r1,	[sp, #4]      
	mul	r5,	lr, r4      
	ldr	r0,	[r1]       
	qadd	ip,	r0, r5      
	ldr	lr,	._gteRTPT__9330       
	mov	r2,	ip, asr #16     
	cmp	r2,	lr       
	movgt	r2,	lr       
	bgt	._gteRTPT__8d9c	       
	cmn	r2,	#0x400       
	movge	r3,	r2, lsl #16     
	movlt	r2,	#0xfc00       
	movge	r2,	r3, lsr #16     
._gteRTPT__8d9c:		       
	mov	ip,	#0x13c       
	strh	r2,	[r11, ip]      
	ldr	lr,	[sp, #8]      
	mul	r2,	r8, r4      
	ldr	r3,	[lr]       
	qadd	r4,	r3, r2      
	ldr	r8,	._gteRTPT__9330       
	mov	r2,	r4, asr #16     
	cmp	r2,	r8       
	movgt	ip,	r8       
	bgt	._gteRTPT__8dd8	       
	cmn	r2,	#0x400       
	movge	r3,	r2, lsl #16     
	movlt	ip,	#0xfc00       
	movge	ip,	r3, lsr #16     
._gteRTPT__8dd8:		       
	ldr	r2,	._gteRTPT__9338       
	mov	r1,	#0x118       
	add	r3,	r1, #0x70      
	add	lr,	r2, #0x70      
	ldrsh	r5,	[r11, r1]      
	ldrsh	r4,	[r11, r2]      
	ldrsh	r1,	[r11, lr]      
	ldrsh	r8,	[r11, r3]      
	sub	r0,	r3, #0x4a      
	sub	r3,	lr, #0x6e      
	strh	ip,	[r11, r0]      
	ldrsh	lr,	[r11, r3]      
	mul	ip,	r8, r5      
	mul	r2,	r1, r4      
	qadd	r8,	r2, ip      
	mov	r1,	#0x18c       
	ldrsh	r0,	[r11, r1]      
	mul	r3,	r0, lr      
	qadd	ip,	r3, r8      
	mov	r0,	ip, asr #12     
	ldr	r2,	[sl]       
	qadd	sl,	r2, r0      
	mov	r3,	#0x190       
	add	r8,	r1, #2      
	ldrsh	ip,	[r11, r8]      
	ldrsh	r1,	[r11, r3]      
	mul	r0,	ip, r5      
	mul	r2,	r1, r4      
	qadd	r3,	r2, r0      
	ldr	r8,	._gteRTPT__931c       
	ldrsh	r0,	[r11, r8]      
	mul	ip,	r0, lr      
	qadd	r1,	ip, r3      
	ldr	r8,	[sp]       
	mov	r2,	r1, asr #12     
	ldr	r0,	[r8]       
	qadd	r8,	r0, r2      
	ldr	ip,	._gteRTPT__9320       
	mov	r3,	#0x194       
	ldrsh	r2,	[r11, r3]      
	ldrsh	r1,	[r11, ip]      
	mul	r0,	r2, r5      
	mul	r3,	r1, r4      
	qadd	r2,	r3, r0      
	mov	r0,	#0x198       
	ldrsh	ip,	[r11, r0]      
	mul	r3,	ip, lr      
	qadd	r4,	r3, r2      
	mov	r1,	r4, asr #12     
	ldr	r0,	[r9]       
	qadd	lr,	r0, r1      
	ldr	r2,	._gteRTPT__9324       
	cmp	sl,	r2       
	ble	._gteRTPT__9078	       
	ldr	r4,	[r11, #516]      
	str	r2,	[sp, #12]      
	orr	r9,	r4, #0x81000000      
	str	r9,	[r11, #516]      
._gteRTPT__8ec0:		       
	ldr	r2,	._gteRTPT__9324       
	cmp	r8,	r2       
	ble	._gteRTPT__9058	       
	ldr	r3,	[r11, #516]      
	mov	r9,	r2       
	orr	r0,	r3, #0x80000000      
	orr	ip,	r0, #0x800000      
	str	ip,	[r11, #516]      
._gteRTPT__8ee0:		       
	ldr	r1,	._gteRTPT__9328       
	cmp	lr,	r1       
	ble	._gteRTPT__9014	       
	ldr	r4,	[r11, #516]      
	mov	ip,	r1       
	orr	r1,	r4, #0x80000000      
	orr	r2,	r1, #0x40000      
	str	r2,	[r11, #516]      
._gteRTPT__8f00:		       
	mov	r4,	#0x1f0       
	ldrh	r2,	[r11, r4]      
	mov	r1,	ip, lsl #16     
	mov	r0,	r2, lsl #16     
	sub	r3,	r4, #0x9c      
	movs	r2,	r0, asr #16     
	strh	ip,	[r11, r3]      
	mov	r1,	r1, lsr #16     
	bmi	._gteRTPT__8f2c	       
	cmp	r2,	r1, lsl #1     
	blt	._gteRTPT__9150	       
._gteRTPT__8f2c:		       
	ldr	r4,	._gteRTPT__932c       
._gteRTPT__8f30:		       
	ldr	r2,	[sp, #4]      
	ldr	r3,	[sp, #12]      
	ldr	r0,	[r2]       
	mul	r3,	r4, r3      
	qadd	r1,	r0, r3      
	ldr	ip,	._gteRTPT__9330       
	mov	r2,	r1, asr #16     
	cmp	r2,	ip       
	movgt	r2,	ip       
	bgt	._gteRTPT__8f68	       
	cmn	r2,	#0x400       
	movge	r3,	r2, lsl #16     
	movlt	r2,	#0xfc00       
	movge	r2,	r3, lsr #16     
._gteRTPT__8f68:		       
	mov	ip,	#0x140       
	strh	r2,	[r11, ip]      
	ldr	r2,	[sp, #8]      
	mul	r3,	r4, r9      
	ldr	r0,	[r2]       
	qadd	r1,	r0, r3      
	ldr	ip,	._gteRTPT__9330       
	mov	r2,	r1, asr #16     
	cmp	r2,	ip       
	movgt	r1,	ip       
	bgt	._gteRTPT__8fa4	       
	cmn	r2,	#0x400       
	movge	r3,	r2, lsl #16     
	movlt	r1,	#0xfc00       
	movge	r1,	r3, lsr #16     
._gteRTPT__8fa4:		       
	ldr	ip,	._gteRTPT__933c       
	add	r2,	r11, #0x108      
	add	r3,	ip, #0xb2      
	strh	r1,	[r11, ip]      
	str	lr,	[r2, #108]      
	str	sl,	[r2, #100]      
	str	r8,	[r2, #104]      
	ldrsh	r1,	[r11, r3]      
	ldr	ip,	[r7, #112]      
	mul	r0,	r1, r4      
	qadd	r4,	ip, r0      
	ldr	r1,	._gteRTPT__9340       
	ldr	r2,	[sp, #12]      
	mov	r3,	#0x12c       
	mov	r0,	#0x130       
	cmp	r4,	r1       
	strh	r2,	[r11, r3]      
	strh	r9,	[r11, r0]      
	str	r4,	[r11, #360]      
	ble	._gteRTPT__9038	       
	ldr	r3,	[r11, #516]      
	mov	r4,	r1       
	orr	r0,	r3, #0x1000      
	str	r0,	[r11, #516]      
._gteRTPT__9004:		       
	mov	r1,	#0x128       
	strh	r4,	[r11, r1]      
	add	sp,	sp, #0x10      
	ldmia	sp!,	{r4, r5, r6, r7, r8, r9, sl, pc}
._gteRTPT__9014:		       
	cmp	lr,	#0       
	ldrlt	r3,	[r11, #516]      
	movge	r3,	lr, lsl #16     
	orrlt	r3,	r3, #0x80000000      
	orrlt	r3,	r3, #0x40000      
	movge	ip,	r3, lsr #16     
	movlt	ip,	#0       
	strlt	r3,	[r11, #516]      
	b	._gteRTPT__8f00	       
._gteRTPT__9038:		       
	cmp	r4,	#0       
	ldrlt	r3,	[r11, #516]      
	movge	r3,	r4, lsl #16     
	orrlt	r3,	r3, #0x1000      
	movge	r4,	r3, lsr #16     
	movlt	r4,	#0       
	strlt	r3,	[r11, #516]      
	b	._gteRTPT__9004	       
._gteRTPT__9058:		       
	cmn	r8,	#0x8000       
	ldrlt	r3,	[r11, #516]      
	ldrlt	r9,	._gteRTPT__9344       
	orrlt	r3,	r3, #0x80000000      
	orrlt	r3,	r3, #0x800000      
	movge	r9,	r8       
	strlt	r3,	[r11, #516]      
	b	._gteRTPT__8ee0	       
._gteRTPT__9078:		       
	cmn	sl,	#0x8000       
	ldrlt	r3,	[r11, #516]      
	ldrlt	r2,	._gteRTPT__9344       
	orrlt	r3,	r3, #0x81000000      
	strlt	r2,	[sp, #12]      
	strge	sl,	[sp, #12]      
	strlt	r3,	[r11, #516]      
	b	._gteRTPT__8ec0	       
._gteRTPT__9098:		       
	cmp	r0,	#0       
	ldrlt	r3,	[r11, #516]      
	movge	r3,	r0, lsl #16     
	orrlt	r3,	r3, #0x80000000      
	orrlt	r3,	r3, #0x40000      
	movge	ip,	r3, lsr #16     
	movlt	ip,	#0       
	strlt	r3,	[r11, #516]      
	b	._gteRTPT__8d38	       
._gteRTPT__90bc:		       
	cmn	r8,	#0x8000       
	ldrlt	r3,	[r11, #516]      
	ldrlt	r8,	._gteRTPT__9344       
	orrlt	r3,	r3, #0x80000000      
	orrlt	r3,	r3, #0x800000      
	strlt	r3,	[r11, #516]      
	b	._gteRTPT__8d18	       
._gteRTPT__90d8:		       
	cmn	r6,	#0x8000       
	ldrlt	r3,	[r11, #516]      
	ldrlt	lr,	._gteRTPT__9344       
	orrlt	r3,	r3, #0x81000000      
	movge	lr,	r6       
	strlt	r3,	[r11, #516]      
	b	._gteRTPT__8cf8	       
._gteRTPT__90f4:		       
	cmp	r0,	#0       
	ldrlt	r3,	[r11, #516]      
	movge	r3,	r0, lsl #16     
	orrlt	r3,	r3, #0x80000000      
	orrlt	r3,	r3, #0x40000      
	movge	ip,	r3, lsr #16     
	movlt	ip,	#0       
	strlt	r3,	[r11, #516]      
	b	._gteRTPT__8b6c	       
._gteRTPT__9118:		       
	cmn	r8,	#0x8000       
	ldrlt	r3,	[r11, #516]      
	ldrlt	r8,	._gteRTPT__9344       
	orrlt	r3,	r3, #0x80000000      
	orrlt	r3,	r3, #0x800000      
	strlt	r3,	[r11, #516]      
	b	._gteRTPT__8b4c	       
._gteRTPT__9134:		       
	cmn	r6,	#0x8000       
	ldrlt	r3,	[r11, #516]      
	ldrlt	lr,	._gteRTPT__9344       
	orrlt	r3,	r3, #0x81000000      
	movge	lr,	r6       
	strlt	r3,	[r11, #516]      
	b	._gteRTPT__8b2c	       
._gteRTPT__9150:		       
	cmp	r1,	#0x8000       
	movhi	ip,	#0       
	bhi	._gteRTPT__9170	       
	mov	ip,	#0       
._gteRTPT__9160:		       
	mov	r1,	r1, lsl #1     
	cmp	r1,	#0x8000       
	add	ip,	ip, #1      
	bls	._gteRTPT__9160	       
._gteRTPT__9170:		       
	mov	r2,	r1, lsl #17     
	ldr	r5,	._gteRTPT__9348       
	mov	r3,	r2, lsr #17     
	mov	r4,	r3, lsl #1     
	ldrh	r6,	[r4, r5]      
	mov	r5,	r0, asr #16     
	orr	r3,	r6, #0x10000      
	mov	r4,	r3, asr #31     
	mov	r0,	r4, lsl ip     
	rsb	r2,	ip, #0x20      
	orr	r4,	r0, r3, lsr r2    
	subs	r1,	ip, #0x20      
	movpl	r4,	r3, lsl r1     
	mov	r3,	r3, lsl ip     
	mov	r6,	r5, asr #31     
	umull	r1,	r2, r3, r5     
	mul	ip,	r6, r3      
	mul	r3,	r4, r5      
	add	r0,	ip, r2      
	mov	ip,	#0x8000       
	add	r2,	r0, r3      
	mov	r4,	#0       
	adds	r3,	ip, r1      
	adc	r1,	r4, r2      
	mov	r0,	r3, lsr #16     
	orr	ip,	r0, r1, lsl #16    
	cmp	ip,	#0x20000       
	movcc	r4,	ip       
	bcc	._gteRTPT__8f30	       
	b	._gteRTPT__8f2c	       
._gteRTPT__91e8:		       
	cmp	r1,	#0x8000       
	movhi	ip,	#0       
	bhi	._gteRTPT__9208	       
	mov	ip,	#0       
._gteRTPT__91f8:		       
	mov	r1,	r1, lsl #1     
	cmp	r1,	#0x8000       
	add	ip,	ip, #1      
	bls	._gteRTPT__91f8	       
._gteRTPT__9208:		       
	mov	r2,	r1, lsl #17     
	ldr	r5,	._gteRTPT__9348       
	mov	r3,	r2, lsr #17     
	mov	r4,	r3, lsl #1     
	ldrh	r6,	[r4, r5]      
	mov	r5,	r0, asr #16     
	orr	r3,	r6, #0x10000      
	mov	r4,	r3, asr #31     
	mov	r0,	r4, lsl ip     
	rsb	r2,	ip, #0x20      
	orr	r4,	r0, r3, lsr r2    
	subs	r1,	ip, #0x20      
	movpl	r4,	r3, lsl r1     
	mov	r3,	r3, lsl ip     
	mov	r6,	r5, asr #31     
	umull	r1,	r2, r3, r5     
	mul	r0,	r6, r3      
	mul	ip,	r4, r5      
	add	r0,	r0, r2      
	mov	r3,	#0x8000       
	add	r2,	r0, ip      
	adds	r5,	r3, r1      
	mov	ip,	#0       
	adc	r4,	ip, r2      
	mov	r0,	r5, lsr #16     
	orr	ip,	r0, r4, lsl #16    
	cmp	ip,	#0x20000       
	movcc	r4,	ip       
	bcc	._gteRTPT__8d68	       
	b	._gteRTPT__8d64	       
._gteRTPT__9280:		       
	cmp	r1,	#0x8000       
	movhi	ip,	#0       
	bhi	._gteRTPT__92a0	       
	mov	ip,	#0       
._gteRTPT__9290:		       
	mov	r1,	r1, lsl #1     
	cmp	r1,	#0x8000       
	add	ip,	ip, #0x1      
	bls	._gteRTPT__9290	       
._gteRTPT__92a0:		       
	mov	r2,	r1, lsl #17     
	ldr	r5,	._gteRTPT__9348       
	mov	r3,	r2, lsr #17     
	mov	r4,	r3, lsl #1     
	ldrh	r6,	[r4, r5]      
	mov	r5,	r0, asr #16     
	orr	r3,	r6, #0x10000      
	mov	r4,	r3, asr #31     
	mov	r0,	r4, lsl ip     
	rsb	r2,	ip, #0x20      
	orr	r4,	r0, r3, lsr r2    
	subs	r1,	ip, #0x20      
	movpl	r4,	r3, lsl r1     
	mov	r3,	r3, lsl ip     
	mov	r6,	r5, asr #31     
	umull	r1,	r2, r3, r5     
	mul	r0,	r6, r3      
	mul	ip,	r4, r5      
	add	r6,	r0, r2      
	mov	r3,	#0x8000       
	add	r2,	r6, ip      
	adds	r5,	r3, r1      
	mov	r4,	#0       
	adc	r0,	r4, r2      
	mov	r6,	r5, lsr #16     
	orr	ip,	r6, r0, lsl #16    
	cmp	ip,	#0x20000       
	movcc	r4,	ip       
	bcc	._gteRTPT__8b9c	       
	b	._gteRTPT__8b98	       
._gteRTPT__9318:	.long	0x0000010a
._gteRTPT__931c:	.long	0x00000192
._gteRTPT__9320:	.long	0x00000196
._gteRTPT__9324:	.long	0x00007fff
._gteRTPT__9328:	.long	0x0000ffff
._gteRTPT__932c:	.long	0x0001ffff
._gteRTPT__9330:	.long	0x000003ff
._gteRTPT__9334:	.long	0x00000112
._gteRTPT__9338:	.long	0x0000011a
._gteRTPT__933c:	.long	0x00000142
._gteRTPT__9340:	.long	0x00000fff
._gteRTPT__9344:	.long	0xffff8000
._gteRTPT__9348:	.long	__gte_reciprocals__       
/*
#endif
*/
		        
.align 4
_gteAVSZ3__:	        
	mov	r2, #0x14c       
	mov	r1, #0x150       
	mov	r3, #0x1fc       
	ldrsh	r3, [r11, r3]      
	ldrh	ip, [r11, r2]      
	ldrh	r2, [r11, r1]      
	mul	ip, r3, ip      
	mul	r2, r3, r2      
	qadd	r2, r2, ip      
	mov	r1, #0x154       
	ldrh	ip, [r11, r1]      
	mul	r3, ip, r3      
	qadd	r3, r3, r2      
	ldr	ip, ._gteAVSZ__0000ffff       
	mov	r1, r3, asr #12     
	bic	r1, r1, r1, asr #31    
	cmp	r1, ip       
	movge	r1, ip       
	mov	r2, #0x124       
	strh	r1, [r11, r2]      
	str	r3, [r11, #0x168]      
	bx	lr        
		        
.align 4
_gteAVSZ4__:	        
	mov	r3, #0x14c       
	mov	ip, #0x148       
	ldrh	r2, [r11, r3]      
	ldrh	r3, [r11, ip]      
	mov	r1, #0x150       
	ldrh	ip, [r11, r1]      
	mov	r1, #0x154       
	ldrh	r1, [r11, r1]      
	add	r3, r2, r3      
	mov	r2, #0x200       
	ldrsh	r2, [r11, r2]      
	add	r3, r3, ip      
	add	r3, r3, r1      
	mul	r3, r2, r3      
	ldr	ip, ._gteAVSZ__0000ffff       
	mov	r1, r3, asr #12     
	bic	r1, r1, r1, asr #31    
	cmp	r1, ip       
	movge	r1, ip       
	mov	r2, #0x124       
	strh	r1, [r11, r2]      
	str	r3, [r11, #0x168]      
	bx	lr        
._gteAVSZ__0000ffff:	.long 0x0000ffff       

.align 4
