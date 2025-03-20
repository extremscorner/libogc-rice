#include <asm.h>

	.section	.init

	# crt0.s file for the GameCube V1.1 by Costis (costis@gbaemu.com)!
	#
	# Updates: Added support for clearing the BSS section so that global
	#	     variables are cleared to 0 upon start-up. 
	#
	# This is start-up code for initializing the GameCube system and hardware
	# before executing the actual user program code. It clears the GPR's,
	# initializes the FPR's, initializes the Data, Code, and L2 caches, clears
	# and initializes SPR's, and disables exceptions (interrupts).
	#
	# Have fun!!! Please e-mail any suggestions or bugs to costis@gbaemu.com.

	# Entry Point

	.globl	_start
_start:
	b		1f
	.ascii	"_arg"
__argv:
	.long	0		# argv magic
	.long	0		# command line
	.long	0		# command line length
	.long	0		# argc
	.long	0		# argv
	.long	0		# end address of argv

1:	b		2f
	.ascii	"_env"
__envp:
	.long	0		# envp magic
	.long	0		# environment
	.long	0		# environment length
	.long	0		# envc
	.long	0		# envv
	.long	0		# end address of envv

2:	bl		__InitBATS			# Initialize BATs to a clear and known state
	bl		__InitGPRS			# Initialize the General Purpose Registers
	bl		__InitPS 			# Initialize Paired Singles
	bl		__InitFPRS		 	# Initialize the FPR's
	bl		__InitCache 		# Initialize the system caches

	bl		__CheckARGV		# check for argv & initialise struct

	# Clear the SBSS section!
	lis		r3,__sbss_start@ha
	lis		r5,__sbss_end@ha
	addi	r3,r3,__sbss_start@l
	addi	r5,r5,__sbss_end@l
	li		r4,0
	subf	r5,r3,r5
	bl		memset

	# Clear the BSS section!
	lis		r3,__bss_start@ha
	lis		r5,__bss_end@ha
	addi	r3,r3,__bss_start@l
	addi	r5,r5,__bss_end@l
	li		r4,0
	subf	r5,r3,r5
	bl		memset

	b      SYS_Init # Branch to the user code!

__InitGPRS:
	# Clear all of the GPR's to 0
	li      r0,0
	lis     sp,__crt0stack@ha
	addi    sp,sp,__crt0stack@l-0x8
	li      r2,0
	li      r3,0
	li      r4,0
	li      r5,0
	li      r6,0
	li      r7,0
	li      r8,0
	li      r9,0
	li      r10,0
	li      r11,0
	li      r12,0
	lis     r13,_SDA_BASE_@ha
	addi    r13,r13,_SDA_BASE_@l # Set the Small Data (Read\Write) base register.
	li      r14,0
	li      r15,0
	li      r16,0
	li      r17,0
	li      r18,0
	li      r19,0
	li      r20,0
	li      r21,0
	li      r22,0
	li      r23,0
	li      r24,0
	li      r25,0
	li      r26,0
	li      r27,0
	li      r28,0
	li      r29,0
	li      r30,0
	li      r31,0
	blr

	.section	.bss,"aw",@nobits
	.balign	8
__crt0stack_end:
	.space	0x4000
__crt0stack:

	.section	.sdata,"aw",@progbits
	.globl	__system_argv
__system_argv:
	.long	__argv

	.section	.sdata,"aw",@progbits
	.globl	__system_envp
__system_envp:
	.long	__envp
