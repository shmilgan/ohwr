/*
 * Alessandro Rubini, 2007, GPL 2 or later
 *
 * This file implements entry/exit from the fiq handler.
 * It must save r0..r8 (since R8 is used to preserve LR in the few lines
 * I added in kernel code, before calling this with "bl").
 */

	.globl fiq_entry
        .type  fiq_entry, #function

	/* here, we can use r10..r13 to our pleasure */
fiq_entry:
	ldr	r11,	fiq_handler
	movs	r11,	r11
	moveq	pc,	lr /* nobody registered, return */
		
	/* count the fiq:  it doesn't cost too much cpu time */
	ldr	r10,	fiqcount
	add	r10,	r10,	#1
	str	r10,	fiqcount
	
	/* save all registers, build sp and branch to C */
	adr	r9,	regpool
	stmia	r9,	{r0 - r8, lr}
	adr	sp,	fiq_sp
	ldr	sp,	[sp]
	mov	lr,	pc
	mov	pc,	r11
	adr	r9,	regpool
	ldmia	r9,	{r0 - r8, pc}

	
	/* data words*/
	.globl fiqcount
fiqcount: .long 0
fiq_sp:	 .long fiq_stack+1020
	
	/* The pointer */
	.globl fiq_handler
fiq_handler:
	.long 0
	
regpool:
	.space 40 /* 10 registers */

.pool

.align 5
fiq_stack:
	.space 1024
