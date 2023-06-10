#include <asm.h>

	.section	.text.dbg_exceptionhandler,"ax",@progbits
	.globl	dbg_exceptionhandler
dbg_exceptionhandler:
	stw		r11,GPR1_OFFSET(sp)
	stmw	r13,GPR13_OFFSET(sp)

	mr		r3,sp
	bl		_cpu_context_save_fp
	bl		c_debug_handler

	mr		r3,sp
	bl		_cpu_context_restore_fp

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

	.section	.text.__set_iabr,"ax",@progbits
	.globl	__set_iabr
__set_iabr:
	mfmsr	r4
	rlwinm	r5,r4,0,18,16
	mtmsr	r5
	clrrwi	r3,r3,2
	mtspr	IABR,r3
	isync
	sync
	mtmsr	r4
	blr

	.section	.text.__enable_iabr,"ax",@progbits
	.globl	__enable_iabr
__enable_iabr:
	mfmsr	r4
	rlwinm	r5,r4,0,18,16
	mtmsr	r5
	mfspr	r3,IABR
	ori		r3,r3,0x0003
	mtspr	IABR,r3
	isync
	sync
	mtmsr	r4
	blr

	.section	.text.__disable_iabr,"ax",@progbits
	.globl	__disable_iabr
__disable_iabr:
	mfmsr	r4
	rlwinm	r5,r4,0,18,16
	mtmsr	r5
	mfspr	r3,IABR
	clrrwi	r3,r3,2
	mtspr	IABR,r3
	isync
	sync
	mtmsr	r4
	blr

	.section	.text.__clr_iabr,"ax",@progbits
	.globl	__clr_iabr
__clr_iabr:
	mfmsr	r4
	rlwinm	r5,r4,0,18,16
	mtmsr	r5
	mtspr	IABR,0
	isync
	sync
	mtmsr	r4
	blr
