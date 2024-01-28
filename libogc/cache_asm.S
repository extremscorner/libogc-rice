/*-------------------------------------------------------------

cache_asm.S -- Cache interface

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

	.section	.text.DCFlashInvalidate,"ax",@progbits
	.globl	DCFlashInvalidate
DCFlashInvalidate:
	mfspr	r3,HID0
	ori		r3,r3,0x0400
	mtspr	HID0,r3
	blr

	.section	.text.DCEnable,"ax",@progbits
	.globl	DCEnable
DCEnable:
	sync
	mfspr	r3,HID0
	ori		r3,r3,0x4000
	mtspr	HID0,r3
	blr

	.section	.text.DCDisable,"ax",@progbits
	.globl	DCDisable
DCDisable:
	sync
	mfspr	r3,HID0
	rlwinm	r3,r3,0,18,16
	mtspr	HID0,r3
	blr

	.section	.text.DCFreeze,"ax",@progbits
	.globl	DCFreeze
DCFreeze:
	sync
	mfspr	r3,HID0
	ori		r3,r3,0x1000
	mtspr	HID0,r3
	blr

	.section	.text.DCUnfreeze,"ax",@progbits
	.globl	DCUnfreeze
DCUnfreeze:
	mfspr	r3,HID0
	rlwinm	r3,r3,0,20,18
	mtspr	HID0,r3
	blr

	.section	.text.DCInvalidateRange,"ax",@progbits
	.globl	DCInvalidateRange
DCInvalidateRange:
	cmplwi	r4,0
	blelr
	clrlwi	r5,r3,27
	add		r4,r4,r5
	addi	r4,r4,31
	srwi	r4,r4,5
	mtctr	r4
1:	dcbi	r0,r3
	addi	r3,r3,32
	bdnz	1b
	blr

	.section	.text.DCFlushRange,"ax",@progbits
	.globl	DCFlushRange
DCFlushRange:
	cmplwi	r4,0
	blelr
	clrlwi	r5,r3,27
	add		r4,r4,r5
	addi	r4,r4,31
	srwi	r4,r4,5
	mtctr	r4
1:	dcbf	r0,r3
	addi	r3,r3,32
	bdnz	1b
	sync
	blr

	.section	.text.DCStoreRange,"ax",@progbits
	.globl	DCStoreRange
DCStoreRange:
	cmplwi	r4,0
	blelr
	clrlwi	r5,r3,27
	add		r4,r4,r5
	addi	r4,r4,31
	srwi	r4,r4,5
	mtctr	r4
1:	dcbst	r0,r3
	addi	r3,r3,32
	bdnz	1b
	sync
	blr

	.section	.text.DCFlushRangeNoSync,"ax",@progbits
	.globl	DCFlushRangeNoSync
DCFlushRangeNoSync:
	cmplwi	r4,0
	blelr
	clrlwi	r5,r3,27
	add		r4,r4,r5
	addi	r4,r4,31
	srwi	r4,r4,5
	mtctr	r4
1:	dcbf	r0,r3
	addi	r3,r3,32
	bdnz	1b
	blr

	.section	.text.DCStoreRangeNoSync,"ax",@progbits
	.globl	DCStoreRangeNoSync
DCStoreRangeNoSync:
	cmplwi	r4,0
	blelr
	clrlwi	r5,r3,27
	add		r4,r4,r5
	addi	r4,r4,31
	srwi	r4,r4,5
	mtctr	r4
1:	dcbst	r0,r3
	addi	r3,r3,32
	bdnz	1b
	blr

	.section	.text.DCZeroRange,"ax",@progbits
	.globl	DCZeroRange
DCZeroRange:
	cmplwi	r4,0
	blelr
	clrlwi	r5,r3,27
	add		r4,r4,r5
	addi	r4,r4,31
	srwi	r4,r4,5
	mtctr	r4
1:	dcbz	r0,r3
	addi	r3,r3,32
	bdnz	1b
	blr

	.section	.text.DCTouchRange,"ax",@progbits
	.globl	DCTouchRange
DCTouchRange:
	cmplwi	r4,0
	blelr
	clrlwi	r5,r3,27
	add		r4,r4,r5
	addi	r4,r4,31
	srwi	r4,r4,5
	mtctr	r4
1:	dcbt	r0,r3
	addi	r3,r3,32
	bdnz	1b
	blr

	.section	.text.ICInvalidateRange,"ax",@progbits
	.globl	ICInvalidateRange
ICInvalidateRange:
	cmplwi	r4,0
	blelr
	clrlwi	r5,r3,27
	add		r4,r4,r5
	addi	r4,r4,31
	srwi	r4,r4,5
	mtctr	r4
1:	icbi	r0,r3
	addi	r3,r3,32
	bdnz	1b
	sync
	isync
	blr

	.section	.text.ICFlashInvalidate,"ax",@progbits
	.globl	ICFlashInvalidate
ICFlashInvalidate:
	mfspr	r3,HID0
	ori		r3,r3,0x0800
	mtspr	HID0,r3
	blr

	.section	.text.ICEnable,"ax",@progbits
	.globl	ICEnable
ICEnable:
	isync
	mfspr	r3,HID0
	ori		r3,r3,0x8000
	mtspr	HID0,r3
	blr

	.section	.text.ICDisable,"ax",@progbits
	.globl	ICDisable
ICDisable:
	isync
	mfspr	r3,HID0
	rlwinm	r3,r3,0,17,15
	mtspr	HID0,r3
	blr

	.section	.text.ICFreeze,"ax",@progbits
	.globl	ICFreeze
ICFreeze:
	isync
	mfspr	r3,HID0
	ori		r3,r3,0x2000
	mtspr	HID0,r3
	blr

	.section	.text.ICUnfreeze,"ax",@progbits
	.globl	ICUnfreeze
ICUnfreeze:
	mfspr	r3,HID0
	rlwinm	r3,r3,0,19,17
	mtspr	HID0,r3
	blr

	.section	.text.LCEnable,"ax",@progbits
	.globl LCEnable
LCEnable:
	mfspr	r4,HID2
	oris	r4,r4,0x100f
	mtspr	HID2,r4
	lis		r3,__lc_bss_start@ha
	lis		r4,__lc_bss_end@ha
	addi	r3,r3,__lc_bss_start@l
	addi	r4,r4,__lc_bss_end@l
	sub		r4,r4,r3
	srwi.	r4,r4,5
	mtctr	r4
	beqlr
1:	dcbz_l	r0,r3
	addi	r3,r3,32
	bdnz	1b
	blr

	.section	.text.LCDisable,"ax",@progbits
	.globl LCDisable
LCDisable:
	lis		r3,__lc_bss_start@ha
	lis		r4,__lc_bss_end@ha
	addi	r3,r3,__lc_bss_start@l
	addi	r4,r4,__lc_bss_end@l
	sub		r4,r4,r3
	srwi.	r4,r4,5
	mtctr	r4
	beq		2f
1:	dcbi	r0,r3
	addi	r3,r3,32
	bdnz	1b
2:	mfspr	r4,HID2
	rlwinm	r4,r4,0,4,2
	mtspr	HID2,r4
	blr

	.section	.text.LCIsEnable,"ax",@progbits
	.globl LCIsEnable
LCIsEnable:
	mfspr	r4,HID2
	extrwi	r3,r4,1,3
	blr

	.section	.text.LCAllocOneTag,"ax",@progbits
	.globl LCAllocOneTag
LCAllocOneTag:
	cmpwi	r3,0
	beq		1f
	dcbi	r0,r4
1:	dcbz_l	r0,r4
	blr

	.section	.text.LCAllocTags,"ax",@progbits
	.globl LCAllocTags
LCAllocTags:
	mflr	r6
	cmplwi	r5,0
	ble		2f
	mtctr	r5
	cmpwi	r3,0
	beq		3f
1:	dcbi	r0,r4
	dcbz_l	r0,r4
	addi	r4,r4,32
	bdnz	1b
	b		2f
3:	dcbz_l	r0,r4
	addi	r4,r4,32
	bdnz	3b
2:	mtlr	r6
	blr

	.section	.text.LCLoadBlocks,"ax",@progbits
	.globl LCLoadBlocks
LCLoadBlocks:
	extrwi		r6,r5,5,25
	clrlwi		r4,r4,3
	or			r6,r6,r4
	mtspr		DMAU,r6
	clrlslwi	r6,r5,30,2
	or			r6,r6,r3
	ori			r6,r6,0x0012
	mtspr		DMAL,r6
	blr

	.section	.text.LCStoreBlocks,"ax",@progbits
	.globl LCStoreBlocks
LCStoreBlocks:
	extrwi		r6,r5,5,25
	clrlwi		r3,r3,3
	or			r6,r6,r3
	mtspr		DMAU,r6
	clrlslwi	r6,r5,30,2
	or			r6,r6,r4
	ori			r6,r6,0x0002
	mtspr		DMAL,r6
	blr

	.section	.text.LCQueueLength,"ax",@progbits
	.globl LCQueueLength
LCQueueLength:
	mfspr	r4,HID2
	extrwi	r3,r4,4,4
	blr

	.section	.text.LCQueueWait,"ax",@progbits
	.globl LCQueueWait
LCQueueWait:
1:	mfspr	r4,HID2
	extrwi	r4,r4,4,4
	cmpw	r4,r3
	bgt		1b
	blr

	.section	.text.L2Enable,"ax",@progbits
	.globl L2Enable
L2Enable:
	mfspr	r3,L2CR
	oris	r0,r3,0x8000
	rlwinm	r3,r0,0,11,9
	mtspr	L2CR,r3
	blr

	.section	.text.L2Disable,"ax",@progbits
	.globl L2Disable
L2Disable:
	sync
	mfspr	r3,L2CR
	clrlwi	r3,r3,1
	mtspr	L2CR,r3
	sync
	blr
