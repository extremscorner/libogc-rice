#include <asm.h>

	.section	.text.__InitBATS,"ax",@progbits
	.globl	__InitBATS
__InitBATS:
	lis		r0,configBATS@h-0x80000000
	ori		r0,r0,configBATS@l
	mtsrr0	r0
	mfmsr	r0
	rlwinm	r0,r0,0,28,25
	mtsrr1	r0
	rfi

configBATS:
	// HID0 = 00910EEC:
	// bus checkstops off, doze mode on,
	// caches off, caches invalidate,
	// store gathering on, enable data cache
	// flush assist, enable branch target cache,
	// enable branch history table
	lis		r3,0x0091
	ori		r3,r3,0x0EEC
	mtspr	HID0,r3
	isync

	li		r0,0
	mtspr	L2CR,r0
	sync

#if defined(HW_RVL)
	lis		r3,0xA390		// set bits: H4A, 64B L2FM, SBE, ST0, LPE, L2CFI
	mtspr	HID4,r3
	isync
#endif

	// clear all BATs
	mtspr	IBAT0U,r0; mtspr	IBAT1U,r0; mtspr	IBAT2U,r0; mtspr	IBAT3U,r0		// IBAT0...3
	mtspr	DBAT0U,r0; mtspr	DBAT1U,r0; mtspr	DBAT2U,r0; mtspr	DBAT3U,r0		// DBAT0...3
#if defined(HW_RVL)
	mtspr	IBAT4U,r0; mtspr	IBAT5U,r0; mtspr	IBAT6U,r0; mtspr	IBAT7U,r0		// IBAT4...7
	mtspr	DBAT4U,r0; mtspr	DBAT5U,r0; mtspr	DBAT6U,r0; mtspr	DBAT7U,r0		// DBAT4...7
#endif
	isync

	// clear all SRs
	lis		r0,0x8000
	mtsr	 0,r0; mtsr	 1,r0; mtsr	 2,r0; mtsr	 3,r0; mtsr	 4,r0; mtsr	 5,r0; mtsr	 6,r0
	mtsr	 7,r0; mtsr	 8,r0; mtsr	 9,r0; mtsr	10,r0; mtsr	11,r0; mtsr	12,r0; mtsr	13,r0
	mtsr	14,r0; mtsr	15,r0
	isync

#if defined(HW_DOL)
	// set [DI]BAT0 for 64MB@80000000,
	// real 00000000, WIMG=0000, R/W
	lis		r4,0x8000
	li		r3,0x0002
	ori		r4,r4,0x07FF
	mtspr	IBAT0L,r3
	mtspr	IBAT0U,r4
	mtspr	DBAT0L,r3
	mtspr	DBAT0U,r4
	isync

	// set DBAT1 for 64MB@C0000000,
	// real 00000000, WIMG=0100, R/W
	oris	r6,r4,0x4000
	ori		r5,r3,0x0020
	mtspr	DBAT1L,r5
	mtspr	DBAT1U,r6
	isync

	// set DBAT2 for 16MB@C8000000,
	// real 08000000, WIMG=0101, R/W
	lis		r3,0x0800
	lis		r4,0xC800
	ori		r3,r3,0x002A
	ori		r4,r4,0x01FE
	mtspr	DBAT2L,r3
	mtspr	DBAT2U,r4
	isync

	// set DBAT3 for 16MB@CC000000,
	// real 0C000000, WIMG=0101, R/W
	addis	r3,r3,0x0400
	addis	r4,r4,0x0400
	mtspr	DBAT3L,r3
	mtspr	DBAT3U,r4
	isync
#elif defined(HW_RVL)
	// set [DI]BAT0 for 16MB@80000000,
	// real 00000000, WIMG=0000, R/W
	lis		r4,0x8000
	li		r3,0x0002
	ori		r4,r4,0x01FF
	mtspr	IBAT0L,r3
	mtspr	IBAT0U,r4
	mtspr	DBAT0L,r3
	mtspr	DBAT0U,r4
	isync

	// set DBAT1 for 16MB@C0000000,
	// real 00000000, WIMG=0100, R/W
	oris	r6,r4,0x4000
	ori		r5,r3,0x0020
	mtspr	DBAT1L,r5
	mtspr	DBAT1U,r6
	isync

	// set [DI]BAT2 for 8MB@81000000,
	// real 01000000, WIMG=0000, R/W
	addis	r3,r3,0x0100
	lis		r4,0x8100
	ori		r4,r4,0x00FF
	mtspr	IBAT1L,r3
	mtspr	IBAT1U,r4
	mtspr	DBAT2L,r3
	mtspr	DBAT2U,r4
	isync

	// set DBAT3 for 8MB@C1000000,
	// real 01000000, WIMG=0100, R/W
	oris	r6,r4,0x4000
	ori		r5,r3,0x0020
	mtspr	DBAT3L,r5
	mtspr	DBAT3U,r6
	isync

	// set [DI]BAT4 for 64MB@90000000,
	// real 10000000, WIMG=0000, R/W
	addis	r3,r3,0x0F00
	lis		r4,0x9000
	ori		r4,r4,0x07FF
	mtspr	IBAT2L,r3
	mtspr	IBAT2U,r4
	mtspr	DBAT4L,r3
	mtspr	DBAT4U,r4
	isync

	// set DBAT5 for 64MB@D0000000,
	// real 10000000, WIMG=0100, R/W
	oris	r6,r4,0x4000
	ori		r5,r3,0x0020
	mtspr	DBAT5L,r5
	mtspr	DBAT5U,r6
	isync

	// set DBAT6 for 16MB@C8000000,
	// real 08000000, WIMG=0101, R/W
	lis		r3,0x0800
	lis		r4,0xC800
	ori		r3,r3,0x002A
	ori		r4,r4,0x01FE
	mtspr	DBAT6L,r3
	mtspr	DBAT6U,r4
	isync

	// set DBAT7 for 32MB@CC000000,
	// real 0C000000, WIMG=0101, R/W
	addis	r3,r3,0x0400
	lis		r4,0xCC00
	ori		r4,r4,0x03FE
	mtspr	DBAT7L,r3
	mtspr	DBAT7U,r4
	isync
#endif

	mflr	r0
	oris	r0,r0,0x8000
	mtsrr0	r0
	li		r0,MSR_FP|MSR_IR|MSR_DR
	mtsrr1	r0
	rfi

	.section	.text.__InitPS,"ax",@progbits
	.globl	__InitPS
__InitPS:
	li		r0,0
	lis		r3,0xA000
	mtspr	HID2,r3
	isync

	lis		r4,4
	lis		r5,5
	ori		r4,r4,4
	ori		r5,r5,5
	lis		r6,6
	lis		r7,7
	ori		r6,r6,6
	ori		r7,r7,7

	mtspr	GQR0,r0
	mtspr	GQR1,r0
	mtspr	GQR2,r0
	mtspr	GQR3,r0
	mtspr	GQR4,r4
	mtspr	GQR5,r5
	mtspr	GQR6,r6
	mtspr	GQR7,r7
	blr

	.section	.text.__InitFPRS,"ax",@progbits
	.globl	__InitFPRS
__InitFPRS:
	lis		r3,zeroPS@ha
	addi	r3,r3,zeroPS@l
	
	# Clear all of the PS FPR's to 0
	psq_l	fr0,0(r3),0,0
	ps_mr	fr1,fr0
	ps_mr	fr2,fr0
	ps_mr	fr3,fr0
	ps_mr	fr4,fr0
	ps_mr	fr5,fr0
	ps_mr	fr6,fr0
	ps_mr	fr7,fr0
	ps_mr	fr8,fr0
	ps_mr	fr9,fr0
	ps_mr	fr10,fr0
	ps_mr	fr11,fr0
	ps_mr	fr12,fr0
	ps_mr	fr13,fr0
	ps_mr	fr14,fr0
	ps_mr	fr15,fr0
	ps_mr	fr16,fr0
	ps_mr	fr17,fr0
	ps_mr	fr18,fr0
	ps_mr	fr19,fr0
	ps_mr	fr20,fr0
	ps_mr	fr21,fr0
	ps_mr	fr22,fr0
	ps_mr	fr23,fr0
	ps_mr	fr24,fr0
	ps_mr	fr25,fr0
	ps_mr	fr26,fr0
	ps_mr	fr27,fr0
	ps_mr	fr28,fr0
	ps_mr	fr29,fr0
	ps_mr	fr30,fr0
	ps_mr	fr31,fr0

	# Clear all of the FPR's to 0
	lfd		fr0,0(r3)
	fmr		fr1,fr0
	fmr		fr2,fr0
	fmr		fr3,fr0
	fmr		fr4,fr0
	fmr		fr5,fr0
	fmr		fr6,fr0
	fmr		fr7,fr0
	fmr		fr8,fr0
	fmr		fr9,fr0
	fmr		fr10,fr0
	fmr		fr11,fr0
	fmr		fr12,fr0
	fmr		fr13,fr0
	fmr		fr14,fr0
	fmr		fr15,fr0
	fmr		fr16,fr0
	fmr		fr17,fr0
	fmr		fr18,fr0
	fmr		fr19,fr0
	fmr		fr20,fr0
	fmr		fr21,fr0
	fmr		fr22,fr0
	fmr		fr23,fr0
	fmr		fr24,fr0
	fmr		fr25,fr0
	fmr		fr26,fr0
	fmr		fr27,fr0
	fmr		fr28,fr0
	fmr		fr29,fr0
	fmr		fr30,fr0
	fmr		fr31,fr0

	mtfsf	0xFF,fr0
	blr

	.section	.text.__InitCache,"ax",@progbits
	.globl	__InitCache
__InitCache:
	mfspr	r3,HID0
	ori		r3,r3,0xC000
	mtspr	HID0,r3
	isync

	lis		r3,0x0020
	mtspr	L2CR,r3
1:	mfspr	r3,L2CR
	clrlwi.	r0,r3,31
	bne		1b

	lis		r3,0x8008
	mtspr	L2CR,r3
	sync
	blr

	.section	.text.SYS_SwitchFiber,"ax",@progbits
	.globl	SYS_SwitchFiber
SYS_SwitchFiber:
	mflr	r0
	mr		r9,sp
	stwu	r9,-8(r8)
	mr		sp,r8
	stw		r0,4(r9)
	mtlr	r7
	blrl
	lwz		r5,0(sp)
	lwz		r0,4(r5)
	mtlr	r0
	mr		sp,r5
	blr

	.section	.rodata.cst8,"aM",@progbits,8
	.balign	4
zeroF:
	.double	0.0
zeroPS:
	.float	0.0, 0.0
