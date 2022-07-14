/* ttydispatch.S - ttydispatch */

#include <icu.s>
		.text
		.globl  kbddisp	
		.globl	kbdhandler

/*------------------------------------------------------------------------
 *  ttydispatch  -  Dispatch an interrupt to the tty handler function
 *	仿照时钟中断分派函数，编写键盘中断分派函数kbddisp如下
 *------------------------------------------------------------------------
 */
kbddisp:
		pushal			/* Save general-purpose regs.	*/
		pushfl			/* Save the flags register	*/
		cli			/* Prevent further interrupts	*/
		movb	$EOI,%al	/* Clear the interrupt		*/
		outb	%al,$OCW1_2
		//pushl	$0x0
		call	kbdhandler	/* Call the handler		*/
		//addl	$4, %esp
		sti
		popfl			/* Restore the flags register	*/
		popal			/* Restore general-purpose regs.*/
		iret			/* Return from interrupt	*/
