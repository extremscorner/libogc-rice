/*-------------------------------------------------------------

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

	.section	.text._cpu_context_save_fp,"ax",@progbits
	.globl	_cpu_context_save_fp
_cpu_context_save_fp:
	psq_st	fr0,PSR0_OFFSET(r3),0,0
	stfd	fr0,FPR0_OFFSET(r3)
	psq_st	fr1,PSR1_OFFSET(r3),0,0
	stfd	fr1,FPR1_OFFSET(r3)
	psq_st	fr2,PSR2_OFFSET(r3),0,0
	stfd	fr2,FPR2_OFFSET(r3)
	psq_st	fr3,PSR3_OFFSET(r3),0,0
	stfd	fr3,FPR3_OFFSET(r3)
	psq_st	fr4,PSR4_OFFSET(r3),0,0
	stfd	fr4,FPR4_OFFSET(r3)
	psq_st	fr5,PSR5_OFFSET(r3),0,0
	stfd	fr5,FPR5_OFFSET(r3)
	psq_st	fr6,PSR6_OFFSET(r3),0,0
	stfd	fr6,FPR6_OFFSET(r3)
	psq_st	fr7,PSR7_OFFSET(r3),0,0
	stfd	fr7,FPR7_OFFSET(r3)
	psq_st	fr8,PSR8_OFFSET(r3),0,0
	stfd	fr8,FPR8_OFFSET(r3)
	psq_st	fr9,PSR9_OFFSET(r3),0,0
	stfd	fr9,FPR9_OFFSET(r3)
	psq_st	fr10,PSR10_OFFSET(r3),0,0
	stfd	fr10,FPR10_OFFSET(r3)
	psq_st	fr11,PSR11_OFFSET(r3),0,0
	stfd	fr11,FPR11_OFFSET(r3)
	psq_st	fr12,PSR12_OFFSET(r3),0,0
	stfd	fr12,FPR12_OFFSET(r3)
	psq_st	fr13,PSR13_OFFSET(r3),0,0
	stfd	fr13,FPR13_OFFSET(r3)
	psq_st	fr14,PSR14_OFFSET(r3),0,0
	stfd	fr14,FPR14_OFFSET(r3)
	psq_st	fr15,PSR15_OFFSET(r3),0,0
	stfd	fr15,FPR15_OFFSET(r3)
	psq_st	fr16,PSR16_OFFSET(r3),0,0
	stfd	fr16,FPR16_OFFSET(r3)
	psq_st	fr17,PSR17_OFFSET(r3),0,0
	stfd	fr17,FPR17_OFFSET(r3)
	psq_st	fr18,PSR18_OFFSET(r3),0,0
	stfd	fr18,FPR18_OFFSET(r3)
	psq_st	fr19,PSR19_OFFSET(r3),0,0
	stfd	fr19,FPR19_OFFSET(r3)
	psq_st	fr20,PSR20_OFFSET(r3),0,0
	stfd	fr20,FPR20_OFFSET(r3)
	psq_st	fr21,PSR21_OFFSET(r3),0,0
	stfd	fr21,FPR21_OFFSET(r3)
	psq_st	fr22,PSR22_OFFSET(r3),0,0
	stfd	fr22,FPR22_OFFSET(r3)
	psq_st	fr23,PSR23_OFFSET(r3),0,0
	stfd	fr23,FPR23_OFFSET(r3)
	psq_st	fr24,PSR24_OFFSET(r3),0,0
	stfd	fr24,FPR24_OFFSET(r3)
	psq_st	fr25,PSR25_OFFSET(r3),0,0
	stfd	fr25,FPR25_OFFSET(r3)
	psq_st	fr26,PSR26_OFFSET(r3),0,0
	stfd	fr26,FPR26_OFFSET(r3)
	psq_st	fr27,PSR27_OFFSET(r3),0,0
	stfd	fr27,FPR27_OFFSET(r3)
	psq_st	fr28,PSR28_OFFSET(r3),0,0
	stfd	fr28,FPR28_OFFSET(r3)
	psq_st	fr29,PSR29_OFFSET(r3),0,0
	stfd	fr29,FPR29_OFFSET(r3)
	psq_st	fr30,PSR30_OFFSET(r3),0,0
	stfd	fr30,FPR30_OFFSET(r3)
	psq_st	fr31,PSR31_OFFSET(r3),0,0
	stfd	fr31,FPR31_OFFSET(r3)
	mffs	fr0
	stfd	fr0,FPSCR_OFFSET(r3)
	blr

	.section	.text._cpu_context_restore_fp,"ax",@progbits
	.globl	_cpu_context_restore_fp
_cpu_context_restore_fp:
	lfd		fr0,FPSCR_OFFSET(r3)
	mtfsf	0xFF,fr0
	psq_l	fr31,PSR31_OFFSET(r3),0,0
	lfd		fr31,FPR31_OFFSET(r3)
	psq_l	fr30,PSR30_OFFSET(r3),0,0
	lfd		fr30,FPR30_OFFSET(r3)
	psq_l	fr29,PSR29_OFFSET(r3),0,0
	lfd		fr29,FPR29_OFFSET(r3)
	psq_l	fr28,PSR28_OFFSET(r3),0,0
	lfd		fr28,FPR28_OFFSET(r3)
	psq_l	fr27,PSR27_OFFSET(r3),0,0
	lfd		fr27,FPR27_OFFSET(r3)
	psq_l	fr26,PSR26_OFFSET(r3),0,0
	lfd		fr26,FPR26_OFFSET(r3)
	psq_l	fr25,PSR25_OFFSET(r3),0,0
	lfd		fr25,FPR25_OFFSET(r3)
	psq_l	fr24,PSR24_OFFSET(r3),0,0
	lfd		fr24,FPR24_OFFSET(r3)
	psq_l	fr23,PSR23_OFFSET(r3),0,0
	lfd		fr23,FPR23_OFFSET(r3)
	psq_l	fr22,PSR22_OFFSET(r3),0,0
	lfd		fr22,FPR22_OFFSET(r3)
	psq_l	fr21,PSR21_OFFSET(r3),0,0
	lfd		fr21,FPR21_OFFSET(r3)
	psq_l	fr20,PSR20_OFFSET(r3),0,0
	lfd		fr20,FPR20_OFFSET(r3)
	psq_l	fr19,PSR19_OFFSET(r3),0,0
	lfd		fr19,FPR19_OFFSET(r3)
	psq_l	fr18,PSR18_OFFSET(r3),0,0
	lfd		fr18,FPR18_OFFSET(r3)
	psq_l	fr17,PSR17_OFFSET(r3),0,0
	lfd		fr17,FPR17_OFFSET(r3)
	psq_l	fr16,PSR16_OFFSET(r3),0,0
	lfd		fr16,FPR16_OFFSET(r3)
	psq_l	fr15,PSR15_OFFSET(r3),0,0
	lfd		fr15,FPR15_OFFSET(r3)
	psq_l	fr14,PSR14_OFFSET(r3),0,0
	lfd		fr14,FPR14_OFFSET(r3)
	psq_l	fr13,PSR13_OFFSET(r3),0,0
	lfd		fr13,FPR13_OFFSET(r3)
	psq_l	fr12,PSR12_OFFSET(r3),0,0
	lfd		fr12,FPR12_OFFSET(r3)
	psq_l	fr11,PSR11_OFFSET(r3),0,0
	lfd		fr11,FPR11_OFFSET(r3)
	psq_l	fr10,PSR10_OFFSET(r3),0,0
	lfd		fr10,FPR10_OFFSET(r3)
	psq_l	fr9,PSR9_OFFSET(r3),0,0
	lfd		fr9,FPR9_OFFSET(r3)
	psq_l	fr8,PSR8_OFFSET(r3),0,0
	lfd		fr8,FPR8_OFFSET(r3)
	psq_l	fr7,PSR7_OFFSET(r3),0,0
	lfd		fr7,FPR7_OFFSET(r3)
	psq_l	fr6,PSR6_OFFSET(r3),0,0
	lfd		fr6,FPR6_OFFSET(r3)
	psq_l	fr5,PSR5_OFFSET(r3),0,0
	lfd		fr5,FPR5_OFFSET(r3)
	psq_l	fr4,PSR4_OFFSET(r3),0,0
	lfd		fr4,FPR4_OFFSET(r3)
	psq_l	fr3,PSR3_OFFSET(r3),0,0
	lfd		fr3,FPR3_OFFSET(r3)
	psq_l	fr2,PSR2_OFFSET(r3),0,0
	lfd		fr2,FPR2_OFFSET(r3)
	psq_l	fr1,PSR1_OFFSET(r3),0,0
	lfd		fr1,FPR1_OFFSET(r3)
	psq_l	fr0,PSR0_OFFSET(r3),0,0
	lfd		fr0,FPR0_OFFSET(r3)
	blr

	.section	.text._cpu_context_switch,"ax",@progbits
	.globl	_cpu_context_switch
_cpu_context_switch:
	stw		sp,GPR1_OFFSET(r3)
	stmw	r14,GPR14_OFFSET(r3)
	lwz		sp,GPR1_OFFSET(r4)
	lmw		r14,GPR14_OFFSET(r4)

	mfcr	r5
	stw		r5,CR_OFFSET(r3)
	lwz		r6,CR_OFFSET(r4)
	mtcrf	0xFF,r6

	mflr	r5
	stw		r5,LR_OFFSET(r3)
	lwz		r6,LR_OFFSET(r4)
	mtlr	r6

	mfmsr	r5
	stw		r5,MSR_OFFSET(r3)
	lwz		r6,MSR_OFFSET(r4)
	mtmsr	r6

	psq_st	fr14,PSR14_OFFSET(r3),0,0
	stfd	fr14,FPR14_OFFSET(r3)
	psq_st	fr15,PSR15_OFFSET(r3),0,0
	stfd	fr15,FPR15_OFFSET(r3)
	psq_st	fr16,PSR16_OFFSET(r3),0,0
	stfd	fr16,FPR16_OFFSET(r3)
	psq_st	fr17,PSR17_OFFSET(r3),0,0
	stfd	fr17,FPR17_OFFSET(r3)
	psq_st	fr18,PSR18_OFFSET(r3),0,0
	stfd	fr18,FPR18_OFFSET(r3)
	psq_st	fr19,PSR19_OFFSET(r3),0,0
	stfd	fr19,FPR19_OFFSET(r3)
	psq_st	fr20,PSR20_OFFSET(r3),0,0
	stfd	fr20,FPR20_OFFSET(r3)
	psq_st	fr21,PSR21_OFFSET(r3),0,0
	stfd	fr21,FPR21_OFFSET(r3)
	psq_st	fr22,PSR22_OFFSET(r3),0,0
	stfd	fr22,FPR22_OFFSET(r3)
	psq_st	fr23,PSR23_OFFSET(r3),0,0
	stfd	fr23,FPR23_OFFSET(r3)
	psq_st	fr24,PSR24_OFFSET(r3),0,0
	stfd	fr24,FPR24_OFFSET(r3)
	psq_st	fr25,PSR25_OFFSET(r3),0,0
	stfd	fr25,FPR25_OFFSET(r3)
	psq_st	fr26,PSR26_OFFSET(r3),0,0
	stfd	fr26,FPR26_OFFSET(r3)
	psq_st	fr27,PSR27_OFFSET(r3),0,0
	stfd	fr27,FPR27_OFFSET(r3)
	psq_st	fr28,PSR28_OFFSET(r3),0,0
	stfd	fr28,FPR28_OFFSET(r3)
	psq_st	fr29,PSR29_OFFSET(r3),0,0
	stfd	fr29,FPR29_OFFSET(r3)
	psq_st	fr30,PSR30_OFFSET(r3),0,0
	stfd	fr30,FPR30_OFFSET(r3)
	psq_st	fr31,PSR31_OFFSET(r3),0,0
	stfd	fr31,FPR31_OFFSET(r3)
	psq_l	fr14,PSR14_OFFSET(r4),0,0
	lfd		fr14,FPR14_OFFSET(r4)
	psq_l	fr15,PSR15_OFFSET(r4),0,0
	lfd		fr15,FPR15_OFFSET(r4)
	psq_l	fr16,PSR16_OFFSET(r4),0,0
	lfd		fr16,FPR16_OFFSET(r4)
	psq_l	fr17,PSR17_OFFSET(r4),0,0
	lfd		fr17,FPR17_OFFSET(r4)
	psq_l	fr18,PSR18_OFFSET(r4),0,0
	lfd		fr18,FPR18_OFFSET(r4)
	psq_l	fr19,PSR19_OFFSET(r4),0,0
	lfd		fr19,FPR19_OFFSET(r4)
	psq_l	fr20,PSR20_OFFSET(r4),0,0
	lfd		fr20,FPR20_OFFSET(r4)
	psq_l	fr21,PSR21_OFFSET(r4),0,0
	lfd		fr21,FPR21_OFFSET(r4)
	psq_l	fr22,PSR22_OFFSET(r4),0,0
	lfd		fr22,FPR22_OFFSET(r4)
	psq_l	fr23,PSR23_OFFSET(r4),0,0
	lfd		fr23,FPR23_OFFSET(r4)
	psq_l	fr24,PSR24_OFFSET(r4),0,0
	lfd		fr24,FPR24_OFFSET(r4)
	psq_l	fr25,PSR25_OFFSET(r4),0,0
	lfd		fr25,FPR25_OFFSET(r4)
	psq_l	fr26,PSR26_OFFSET(r4),0,0
	lfd		fr26,FPR26_OFFSET(r4)
	psq_l	fr27,PSR27_OFFSET(r4),0,0
	lfd		fr27,FPR27_OFFSET(r4)
	psq_l	fr28,PSR28_OFFSET(r4),0,0
	lfd		fr28,FPR28_OFFSET(r4)
	psq_l	fr29,PSR29_OFFSET(r4),0,0
	lfd		fr29,FPR29_OFFSET(r4)
	psq_l	fr30,PSR30_OFFSET(r4),0,0
	lfd		fr30,FPR30_OFFSET(r4)
	psq_l	fr31,PSR31_OFFSET(r4),0,0
	lfd		fr31,FPR31_OFFSET(r4)
	blr
