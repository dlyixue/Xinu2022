/* ctxsw.S - ctxsw (for x86) */

		.text
		.globl	ctxsw
		.globl  ret_k2u
		.globl	ltss
		.globl	do_sys

/*------------------------------------------------------------------------
 * ctxsw -  X86 context switch; the call is ctxsw(&old_sp, &new_sp)
 *------------------------------------------------------------------------
 */
ctxsw:
		pushl	%ebp		/* Push ebp onto stack		*/
		movl	%esp,%ebp	/* Record current SP in ebp	*/
		pushfl			/* Push flags onto the stack	*/
		pushal			/* Push general regs. on stack	*/

		/* Save old segment registers here, if multiple allowed */

		movl	8(%ebp),%eax	/* Get mem location in which to	*/
					/*   save the old process's SP	*/
		movl	%esp,(%eax)	/* Save old process's SP	*/
		movl	12(%ebp),%ebx	/* Get location from which to	*/
					/*   restore new process's SP	*/
		movl    16(%ebp),%eax //pgDir
		movl    %eax, %cr3
		/* The next instruction switches from the old process's	*/
		/*   stack to the new process's stack.			*/

		movl	(%ebx),%esp	/* Pop up new process's SP	*/

		/* Restore new seg. registers here, if multiple allowed */

		popal			/* Restore general registers	*/
		movl	4(%esp),%ebp	/* Pick up ebp before restoring	*/
					/*   interrupts			*/
		popfl			/* Restore interrupt mask	*/
		add	$4,%esp		/* Skip saved value of ebp	*/
		ret			/* Return to new process	*/

ret_k2u:
		mov (%esp),%ds
		mov 2(%esp),%es
		mov 4(%esp),%fs
		mov 6(%esp),%gs
		add $8,%esp
		iret
ltss:
	movl 4(%esp),%eax
	ltr %ax
	ret
do_sys:
	movl 4(%esp),%ebx	/*func*/
	movl 8(%esp),%esi	/*count*/
	movl 12(%esp),%edi	/* arg*/
	int $46
	ret

