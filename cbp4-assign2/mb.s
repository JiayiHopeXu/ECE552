	.file	1 "mb.c"

 # GNU C 2.7.2.3 [AL 1.1, MM 40, tma 0.1] SimpleScalar running sstrix compiled by GNU C

 # Cc1 defaults:
 # -mgas -mgpOPT

 # Cc1 arguments (-G value = 8, Cpu = default, ISA = 1):
 # -quiet -dumpbase -O -o

gcc2_compiled.:
__gnu_compiled_c:
	.sdata
	.align	2
$LC0:
	.ascii	"%d\000"
	.text
	.align	2
	.globl	main

	.extern	stdin, 4
	.extern	stdout, 4

	.text

	.loc	1 4
	.ent	main
main:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, extra= 0
	.mask	0x80000000,-8
	.fmask	0x00000000,0
	subu	$sp,$sp,24
	sw	$31,16($sp)
	jal	__main
	move	$5,$0
	move	$7,$0
	li	$8,0x2aaaaaab		# 715827883
$L17:
	li	$6,0x000003e8		# 1000
$L21:
	mult	$6,$8
	.set	noreorder
	mfhi	$3
	mflo	$2
	#nop
	#nop
	.set	reorder
	srl	$2,$3,0
	move	$3,$0
	sra	$2,$2,1
	sra	$4,$6,31
	subu	$2,$2,$4
	sll	$4,$2,1
	addu	$4,$4,$2
	sll	$4,$4,2
	bne	$6,$4,$L20
	addu	$5,$5,1
$L20:
	subu	$6,$6,2
	bgtz	$6,$L21
	addu	$7,$7,2
	slt	$2,$7,1000
	bne	$2,$0,$L17
	la	$4,$LC0
	jal	printf
	move	$2,$0
	lw	$31,16($sp)
	addu	$sp,$sp,24
	j	$31
	.end	main
