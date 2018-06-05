/*
 * (C) Steward Fu, 2018
 *
 * This work is licensed under the terms of  GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */
.text
.align 2
.set noreorder

.global _memcpy
.global draw_spr16_full
.ent draw_spr16_full

_memcpy: # (void *dst, void *src, int len)
	andi $t0, $a2, 0x03
	bnez $t0, 0f
	nop
0:
	lh $t0, 0($a1)
	sh $t0, 0($a0)
	addi $a0, $a0, 2
	addi $a1, $a1, 2
	sub $a2, $a2, 2
	bnez $a2, 0b
	nop
	jr $ra
1:
	lw $t0, 0($a1)
	sw $t0, 0($a0)
	addi $a0, $a0, 4
	addi $a1, $a1, 4
	sub $a2, $a2, 4
	bnez $a2, 1b
	nop
	jr $ra

.macro do_4_pixels rs ibase obase # in: r0=dst, r2=pal, r12=0x1e
.if \ibase - 1 < 0
	sll $t6, \rs, 1
	and $t2, $t7, $t6
.else
	srl $t6, \rs, \ibase-1
	and $t2, $t7, $t6
.endif
	srl $t6, \rs, \ibase+3
	and $t3, $t7, $t6
	srl $t6, \rs, \ibase+7
	and $t4, $t7, $t6
	srl $t6, \rs, \ibase+11
	and $t5, $t7, $t6

	add $t6, $a2, $t2
	lh $t2, 0($t6)
	add $t6, $a2, $t3
	lh $t3, 0($t6)
	add $t6, $a2, $t4
	lh $t4, 0($t6)
	add $t6, $a2, $t5
	lh $t5, 0($t6)

	beqz $t2, 1f
	nop
	sh $t2, \obase+0($a0)
1:
	beqz $t3, 2f
	nop
	sh $t3, \obase+2($a0)
2:
	beqz $t4, 3f
	nop
	sh $t4, \obase+4($a0)
3:
	beqz $t5, 4f
	nop
	sh $t5, \obase+6($a0)
4:
.endm

draw_spr16_full: # (u16 *d, void *s, u16 *pal, int lines)
	li $t7, 0x1e
0:
	lw $t0, 0($a1)
	lw $t1, 4($a1)
  do_4_pixels $t0, 0,  0
  do_4_pixels $t0, 16, 8
  do_4_pixels $t1, 0,  16
  do_4_pixels $t1, 16, 24
	li $t6, 1
  subu $a3, $a3, $t6
	addiu $a0, $a0, 2048
	addiu $a1, $a1, 2048
	bnez $a3, 0b
	nop
	jr $ra
	.end draw_spr16_full

