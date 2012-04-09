/*-------------------------------------------------------------

cache_asm.S -- Cache interface

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

	.section	.text.DCFlashInvalidate,"ax",@progbits
	.globl	DCFlashInvalidate
DCFlashInvalidate:
	mfspr	r3,HID0
	ori		r3,r3,0x0400
	mtspr	HID0,r3
	isync
	blr

	.section	.text.DCEnable,"ax",@progbits
	.globl	DCEnable
DCEnable:
	mfspr	r3,HID0
	ori		r3,r3,0x4000
	mtspr	HID0,r3
	isync
	blr

	.section	.text.DCDisable,"ax",@progbits
	.globl	DCDisable
DCDisable:
	mfspr	r3,HID0
	rlwinm	r3,r3,0,18,16
	mtspr	HID0,r3
	isync
	blr

	.section	.text.DCFreeze,"ax",@progbits
	.globl	DCFreeze
DCFreeze:
	mfspr	r3,HID0
	ori		r3,r3,0x1000
	mtspr	HID0,r3
	isync
	blr

	.section	.text.DCUnfreeze,"ax",@progbits
	.globl	DCUnfreeze
DCUnfreeze:
	mfspr	r3,HID0
	rlwinm	r3,r3,0,20,18
	mtspr	HID0,r3
	isync
	blr

	.section	.text.DCInvalidateRange,"ax",@progbits
	.globl	DCInvalidateRange
DCInvalidateRange:
	cmplwi r4, 0   # zero or negative size?
	blelr
	clrlwi. r5, r3, 27  # check for lower bits set in address
	beq 1f
	addi r4, r4, 0x20 
1:
	addi r4, r4, 0x1f
	srwi r4, r4, 5
	mtctr r4
2:
	dcbi r0, r3
	addi r3, r3, 0x20
	bdnz 2b
	blr

	.section	.text.DCFlushRange,"ax",@progbits
	.globl	DCFlushRange
DCFlushRange:
	cmplwi r4, 0   # zero or negative size?
	blelr
	clrlwi. r5, r3, 27  # check for lower bits set in address
	beq 1f
	addi r4, r4, 0x20 
1:
	addi r4, r4, 0x1f
	srwi r4, r4, 5
	mtctr r4
2:
	dcbf r0, r3
	addi r3, r3, 0x20
	bdnz 2b
	sync
	blr

	.section	.text.DCStoreRange,"ax",@progbits
	.globl	DCStoreRange
DCStoreRange:
	cmplwi r4, 0   # zero or negative size?
	blelr
	clrlwi. r5, r3, 27  # check for lower bits set in address
	beq 1f
	addi r4, r4, 0x20 
1:
	addi r4, r4, 0x1f
	srwi r4, r4, 5
	mtctr r4
2:
	dcbst r0, r3
	addi r3, r3, 0x20
	bdnz 2b
	sync
	blr

	.section	.text.DCFlushRangeNoSync,"ax",@progbits
	.globl	DCFlushRangeNoSync
DCFlushRangeNoSync:
	cmplwi r4, 0   # zero or negative size?
	blelr
	clrlwi. r5, r3, 27  # check for lower bits set in address
	beq 1f
	addi r4, r4, 0x20 
1:
	addi r4, r4, 0x1f
	srwi r4, r4, 5
	mtctr r4
2:
	dcbf r0, r3
	addi r3, r3, 0x20
	bdnz 2b
	blr

	.section	.text.DCStoreRangeNoSync,"ax",@progbits
	.globl	DCStoreRangeNoSync
DCStoreRangeNoSync:
	cmplwi r4, 0   # zero or negative size?
	blelr
	clrlwi. r5, r3, 27  # check for lower bits set in address
	beq 1f
	addi r4, r4, 0x20 
1:
	addi r4, r4, 0x1f
	srwi r4, r4, 5
	mtctr r4
2:
	dcbst r0, r3
	addi r3, r3, 0x20
	bdnz 2b
	blr

	.section	.text.DCZeroRange,"ax",@progbits
	.globl	DCZeroRange
DCZeroRange:
	cmplwi r4, 0   # zero or negative size?
	blelr
	clrlwi. r5, r3, 27  # check for lower bits set in address
	beq 1f
	addi r4, r4, 0x20 
1:
	addi r4, r4, 0x1f
	srwi r4, r4, 5
	mtctr r4
2:
	dcbz r0, r3
	addi r3, r3, 0x20
	bdnz 2b
	blr

	.section	.text.DCTouchRange,"ax",@progbits
	.globl	DCTouchRange
DCTouchRange:
	cmplwi r4, 0   # zero or negative size?
	blelr
	clrlwi. r5, r3, 27  # check for lower bits set in address
	beq 1f
	addi r4, r4, 0x20 
1:
	addi r4, r4, 0x1f
	srwi r4, r4, 5
	mtctr r4
2:
	dcbt r0, r3
	addi r3, r3, 0x20
	bdnz 2b
	blr

	.section	.text.ICInvalidateRange,"ax",@progbits
	.globl	ICInvalidateRange
ICInvalidateRange:
	cmplwi r4, 0   # zero or negative size?
	blelr
	clrlwi. r5, r3, 27  # check for lower bits set in address
	beq 1f
	addi r4, r4, 0x20 
1:
	addi r4, r4, 0x1f
	srwi r4, r4, 5
	mtctr r4
2:
	icbi r0, r3
	addi r3, r3, 0x20
	bdnz 2b
	sync
	isync
	blr

	.section	.text.ICFlashInvalidate,"ax",@progbits
	.globl	ICFlashInvalidate
ICFlashInvalidate:
	mfspr	r3,HID0
	ori		r3,r3,0x0800
	mtspr	HID0,r3
	isync
	blr

	.section	.text.ICEnable,"ax",@progbits
	.globl	ICEnable
ICEnable:
	mfspr	r3,HID0
	ori		r3,r3,0x8000
	mtspr	HID0,r3
	isync
	blr

	.section	.text.ICDisable,"ax",@progbits
	.globl	ICDisable
ICDisable:
	mfspr	r3,HID0
	rlwinm	r3,r3,0,17,15
	mtspr	HID0,r3
	isync
	blr

	.section	.text.ICFreeze,"ax",@progbits
	.globl	ICFreeze
ICFreeze:
	mfspr	r3,HID0
	ori		r3,r3,0x2000
	mtspr	HID0,r3
	isync
	blr

	.section	.text.ICUnfreeze,"ax",@progbits
	.globl	ICUnfreeze
ICUnfreeze:
	mfspr	r3,HID0
	rlwinm	r3,r3,0,19,17
	mtspr	HID0,r3
	isync
	blr
