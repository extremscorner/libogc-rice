/*-------------------------------------------------------------

irq_handler.S -- Interrupt subsystem

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

	.section	.text.irq_exceptionhandler,"ax",@progbits
	.globl	irq_exceptionhandler
irq_exceptionhandler:
	stw		r31,GPR31_OFFSET(sp)
	mfmsr	r0
	ori		r0,r0,MSR_RI
	mtmsr	r0

	mfspr	r3,SPRG0
	mr		r31,sp
	cmpwi	r3,0
	bne		1f
	mfspr	sp,SPRG1

1:	lis		r2,_thread_dispatch_disable_level@ha
	lwz		r4,_thread_dispatch_disable_level@l(r2)
	addi	r3,r3,1
	addi	r4,r4,1
	stw		r4,_thread_dispatch_disable_level@l(r2)
	mtspr	SPRG0,r3

	bl		c_irqdispatcher

	mfspr	r3,SPRG0
	mr		sp,r31
	lis		r2,_thread_dispatch_disable_level@ha
	lwz		r4,_thread_dispatch_disable_level@l(r2)
	subi	r3,r3,1
	subic.	r4,r4,1
	stw		r4,_thread_dispatch_disable_level@l(r2)
	mtspr	SPRG0,r3
	bne		2f

	lis		r2,_context_switch_want@ha
	lwz		r3,_context_switch_want@l(r2)
	cmpwi	r3,0
	beq		2f

	bl		__thread_dispatch

2:	mfmsr	r0
	rlwinm	r0,r0,0,31,29
	mtmsr	r0
	lwz		r0,CR_OFFSET(sp)
	mtcr	r0
	lwz		r0,LR_OFFSET(sp)
	mtlr	r0
	lwz		r0,CTR_OFFSET(sp)
	mtctr	r0
	lwz		r0,XER_OFFSET(sp)
	mtxer	r0
	lwz		r0,SRR0_OFFSET(sp)
	mtsrr0	r0
	lwz		r0,SRR1_OFFSET(sp)
	rlwinm	r0,r0,0,19,17
	mtsrr1	r0
	lwz		r31,GPR31_OFFSET(sp)
	lwz		r12,GPR12_OFFSET(sp)
	lwz		r11,GPR11_OFFSET(sp)
	lwz		r10,GPR10_OFFSET(sp)
	lwz		r9,GPR9_OFFSET(sp)
	lwz		r8,GPR8_OFFSET(sp)
	lwz		r7,GPR7_OFFSET(sp)
	lwz		r6,GPR6_OFFSET(sp)
	lwz		r5,GPR5_OFFSET(sp)
	lwz		r4,GPR4_OFFSET(sp)
	lwz		r3,GPR3_OFFSET(sp)
	lwz		r2,GPR2_OFFSET(sp)
	lwz		r0,GPR0_OFFSET(sp)
	addi	sp,sp,EXCEPTION_FRAME_END
	rfi
