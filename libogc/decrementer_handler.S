/*-------------------------------------------------------------

decrementer_handler.S -- PPC decrementer exception support

Copyright (C) 2004 - 2025
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)
Extrems' Corner.org

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

	.section	.text.dec_exceptionhandler,"ax",@progbits
	.globl	dec_exceptionhandler
dec_exceptionhandler:
	stw		r31,GPR31_OFFSET(sp)
	psq_st	fr0,PSFPR0_OFFSET(sp),0,0
	stfd	fr0,FPR0_OFFSET(sp)
	psq_st	fr1,PSFPR1_OFFSET(sp),0,0
	stfd	fr1,FPR1_OFFSET(sp)
	psq_st	fr2,PSFPR2_OFFSET(sp),0,0
	stfd	fr2,FPR2_OFFSET(sp)
	psq_st	fr3,PSFPR3_OFFSET(sp),0,0
	stfd	fr3,FPR3_OFFSET(sp)
	psq_st	fr4,PSFPR4_OFFSET(sp),0,0
	stfd	fr4,FPR4_OFFSET(sp)
	psq_st	fr5,PSFPR5_OFFSET(sp),0,0
	stfd	fr5,FPR5_OFFSET(sp)
	psq_st	fr6,PSFPR6_OFFSET(sp),0,0
	stfd	fr6,FPR6_OFFSET(sp)
	psq_st	fr7,PSFPR7_OFFSET(sp),0,0
	stfd	fr7,FPR7_OFFSET(sp)
	psq_st	fr8,PSFPR8_OFFSET(sp),0,0
	stfd	fr8,FPR8_OFFSET(sp)
	psq_st	fr9,PSFPR9_OFFSET(sp),0,0
	stfd	fr9,FPR9_OFFSET(sp)
	psq_st	fr10,PSFPR10_OFFSET(sp),0,0
	stfd	fr10,FPR10_OFFSET(sp)
	psq_st	fr11,PSFPR11_OFFSET(sp),0,0
	stfd	fr11,FPR11_OFFSET(sp)
	psq_st	fr12,PSFPR12_OFFSET(sp),0,0
	stfd	fr12,FPR12_OFFSET(sp)
	psq_st	fr13,PSFPR13_OFFSET(sp),0,0
	stfd	fr13,FPR13_OFFSET(sp)
	mffs	fr0
	stfd	fr0,FPSCR_OFFSET(sp)

	mfmsr	r0
	ori		r0,r0,MSR_RI
	mtmsr	r0

	mfspr	r3,SPRG0
	mr		r31,sp
	cmpwi	r3,0
	bne		1f
	mfspr	sp,SPRG1

1:	lwz		r4,_thread_dispatch_disable_level@sdarel(r13)
	addi	r3,r3,1
	addi	r4,r4,1
	stw		r4,_thread_dispatch_disable_level@sdarel(r13)
	mtspr	SPRG0,r3

	mr		r3,r31
	bl		c_decrementerhandler

	mfspr	r3,SPRG0
	mr		sp,r31
	lwz		r4,_thread_dispatch_disable_level@sdarel(r13)
	subi	r3,r3,1
	subic.	r4,r4,1
	stw		r4,_thread_dispatch_disable_level@sdarel(r13)
	mtspr	SPRG0,r3
	bne		2f

	lwz		r3,_context_switch_want@sdarel(r13)
	cmpwi	r3,0
	beq		2f

	bl		__thread_dispatch

2:	mfmsr	r0
	rlwinm	r0,r0,0,31,29
	mtmsr	r0

	lfd		fr0,FPSCR_OFFSET(sp)
	mtfsf	0xFF,fr0
	psq_l	fr13,PSFPR13_OFFSET(sp),0,0
	lfd		fr13,FPR13_OFFSET(sp)
	psq_l	fr12,PSFPR12_OFFSET(sp),0,0
	lfd		fr12,FPR12_OFFSET(sp)
	psq_l	fr11,PSFPR11_OFFSET(sp),0,0
	lfd		fr11,FPR11_OFFSET(sp)
	psq_l	fr10,PSFPR10_OFFSET(sp),0,0
	lfd		fr10,FPR10_OFFSET(sp)
	psq_l	fr9,PSFPR9_OFFSET(sp),0,0
	lfd		fr9,FPR9_OFFSET(sp)
	psq_l	fr8,PSFPR8_OFFSET(sp),0,0
	lfd		fr8,FPR8_OFFSET(sp)
	psq_l	fr7,PSFPR7_OFFSET(sp),0,0
	lfd		fr7,FPR7_OFFSET(sp)
	psq_l	fr6,PSFPR6_OFFSET(sp),0,0
	lfd		fr6,FPR6_OFFSET(sp)
	psq_l	fr5,PSFPR5_OFFSET(sp),0,0
	lfd		fr5,FPR5_OFFSET(sp)
	psq_l	fr4,PSFPR4_OFFSET(sp),0,0
	lfd		fr4,FPR4_OFFSET(sp)
	psq_l	fr3,PSFPR3_OFFSET(sp),0,0
	lfd		fr3,FPR3_OFFSET(sp)
	psq_l	fr2,PSFPR2_OFFSET(sp),0,0
	lfd		fr2,FPR2_OFFSET(sp)
	psq_l	fr1,PSFPR1_OFFSET(sp),0,0
	lfd		fr1,FPR1_OFFSET(sp)
	psq_l	fr0,PSFPR0_OFFSET(sp),0,0
	lfd		fr0,FPR0_OFFSET(sp)
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
