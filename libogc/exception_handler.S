/*-------------------------------------------------------------

exception_handler.S -- PPC exception handling support

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/


#include <asm.h>

	.section	.rodata.exceptionhandler,"a",@progbits
	.globl	exceptionhandler_start, exceptionhandler_patch, exceptionhandler_end
exceptionhandler_start:
	mtsprg		3,sp
	subi		sp,sp,EXCEPTION_FRAME_END
	mtsprg		2,sp
	clrlwi		sp,sp,2			//make sp physical
	stw			r0,GPR0_OFFSET(sp)
	stw			r2,GPR2_OFFSET(sp)
	stw			r3,GPR3_OFFSET(sp)
	stw			r4,GPR4_OFFSET(sp)
	stw			r5,GPR5_OFFSET(sp)
	stw			r6,GPR6_OFFSET(sp)
	stw			r7,GPR7_OFFSET(sp)
	stw			r8,GPR8_OFFSET(sp)
	stw			r9,GPR9_OFFSET(sp)
	stw			r10,GPR10_OFFSET(sp)
	stw			r11,GPR11_OFFSET(sp)
	stw			r12,GPR12_OFFSET(sp)
	mfcr		r0
	stw			r0,CR_OFFSET(sp)
	mflr		r0
	stw			r0,LR_OFFSET(sp)
	mfctr		r0
	stw			r0,CTR_OFFSET(sp)
	mfxer		r0
	stw			r0,XER_OFFSET(sp)
	mfsrr0		r0
	stw			r0,SRR0_OFFSET(sp)
	mfsrr1		r0
	stw			r0,SRR1_OFFSET(sp)
	rlwinm.		r0,r0,0,30,30

exceptionhandler_patch:
	li			r3,0
	stw			r3,EXCEPTION_NUMBER(sp)

	lis			r0,default_exceptionhandler@h
	ori			r0,r0,default_exceptionhandler@l
	beq			1f
	lis			r2,exceptionhandler_table@ha-0x80000000
	addi		r2,r2,exceptionhandler_table@l
	clrlslwi	r0,r3,24,2
	lwzx		r0,r2,r0
1:
	mfsprg		r11,3
	stw			r11,0(sp)
	mfsprg		sp,2
	mtsrr0		r0
	mfmsr		r0
	ori			r0,r0,MSR_FP|MSR_IR|MSR_DR
	mtsrr1		r0
	rfi
exceptionhandler_end:

	.section	.text.default_exceptionhandler,"ax",@progbits
	.globl	default_exceptionhandler
default_exceptionhandler:
	stw			r11,GPR1_OFFSET(sp)
	stmw		r13,GPR13_OFFSET(sp)

	mr			r3,sp
	b			c_default_exceptionhandler

	.section	.text.fpu_exceptionhandler,"ax",@progbits
	.globl	fpu_exceptionhandler
fpu_exceptionhandler:
	bl			__thread_dispatch_fp

	lwz			r0,CR_OFFSET(sp)
	mtcr		r0
	lwz			r0,LR_OFFSET(sp)
	mtlr		r0
	lwz			r0,CTR_OFFSET(sp)
	mtctr		r0
	lwz			r0,XER_OFFSET(sp)
	mtxer		r0
	lwz			r0,SRR0_OFFSET(sp)
	mtsrr0		r0
	lwz			r0,SRR1_OFFSET(sp)
	ori			r0,r0,MSR_FP
	mtsrr1		r0
	lwz			r12,GPR12_OFFSET(sp)
	lwz			r11,GPR11_OFFSET(sp)
	lwz			r10,GPR10_OFFSET(sp)
	lwz			r9,GPR9_OFFSET(sp)
	lwz			r8,GPR8_OFFSET(sp)
	lwz			r7,GPR7_OFFSET(sp)
	lwz			r6,GPR6_OFFSET(sp)
	lwz			r5,GPR5_OFFSET(sp)
	lwz			r4,GPR4_OFFSET(sp)
	lwz			r3,GPR3_OFFSET(sp)
	lwz			r2,GPR2_OFFSET(sp)
	lwz			r0,GPR0_OFFSET(sp)
	addi		sp,sp,EXCEPTION_FRAME_END
	rfi
