/* create.c - create, newpid */

#include <xinu.h>

local	int newpid();
/*------------------------------------------------------------------------
 *  create  -  Create a process to start running a function on x86
 *------------------------------------------------------------------------
 */

pid32	create(
	  void		*funcaddr,	/* Address of the function	*/
	  uint32	ssize,		/* Stack size in bytes		*/
	  pri16		priority,	/* Process priority > 0		*/
	  char		*name,		/* Name (for debugging)		*/
	  uint32	nargs,		/* Number of args that follow	*/
	  ...
	)
{
	uint32		savsp, *pushsp, uesp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		/* Points to list of args	*/
	uint32		*saddr;		/* Stack address		*/
	uint32		*Usaddr;	/* user Stack*/

	mask = disable();

	pgDir phy_add = get_pgDir();
	if((priority < 1) || ((pid=newpid()) == SYSERR)){
		restore(mask);
		return SYSERR;
	}
	prcount++;
	prptr = &proctab[pid];
	prptr->pageDir = phy_add;

	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundmb(ssize);
	ssize = 8*1024;//固定8192
	if (((saddr = (uint32 *)alloc_kstk(ssize, prptr->pageDir)) == (uint32 *)SYSERR) 
		 ||((Usaddr = (uint32 *)alloc_ustk(ssize, prptr->pageDir)) == (uint32 *)SYSERR) ) {
		restore(mask);
		return SYSERR;
	}
	
	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prstkbase = (char *)saddr;
	prptr->prUstkbase = (char *)Usaddr;
	prptr->prstklen = ssize;
	prptr->heap_ptr = 8;
	prptr->prname[PNMLEN-1] = NULLCH;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;

	/* Initialize stack as if the process was called		*/

	uint32 *saddr_cp = saddr;
	uint32 *Usaddr_cp = Usaddr;
	saddr = (uint32 *)0x1ffeffc;
	Usaddr = (uint32 *)0x1ffdffc;
	*saddr = STACKMAGIC;
	*Usaddr = STACKMAGIC;
	savsp = (uint32)saddr_cp;

	/* Push arguments */
	a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	a += nargs -1;			/* Last argument		*/
	uint32 tmp;
	for ( ; nargs > 0 ; nargs--){	/* Machine dependent; copy args	*/
		tmp = *a--;
		*--saddr = tmp;
		*--Usaddr = tmp;
		--saddr_cp;
		--Usaddr_cp;
	}		/* onto created process's stack	*/

	*--saddr = (long)INITRET;	/* Push on return address	*/
	*--Usaddr = (long)INITRET;
	--saddr_cp;
	--Usaddr_cp;

	TSS.ss0 = (0x3 << 3);
	prptr->esp0 = saddr_cp;
	TSS.io_map = (uint16)0xffff;

	uesp = (uint32)Usaddr_cp;
	prptr->prUstkptr = (char *)Usaddr_cp;

	// push 中断返回堆栈
	*--saddr = 0x33; // ss
	--saddr_cp;
	*--saddr = uesp; //esp
	--saddr_cp;
	*--saddr = 0x00000200;//eflags
	--saddr_cp;
	*--saddr = 0x23; //cs
	--saddr_cp;
	//
	/* The following entries on the stack must match what ctxsw	*/
	/*   expects a saved process state to contain: ret address,	*/
	/*   ebp, interrupt mask, flags, registers, and an old SP	*/

	*--saddr = (long)funcaddr;	/* Make the stack look like it's*///eip
					/*   half-way through a call to	*/
					/*   ctxsw that "returns" to the*/
					/*   new process		*/
	--saddr_cp;
	*--saddr = 0x002a002a;
	--saddr_cp;
	*--saddr = 0x002a002a; // ds/gs/fs/es
	--saddr_cp;
	*--saddr = (long)&ret_k2u;
	--saddr_cp;
	*--saddr = savsp;		/* This will be register ebp	*/
					/*   for process exit		*/
	--saddr_cp;
	savsp = (uint32) saddr_cp;		/* Start of frame for ctxsw	*/
	*--saddr = 0x00000200;		/* New process runs with	*/
	--saddr_cp;
					/*   interrupts enabled		*/

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--saddr = 0;			/* %eax */
	--saddr_cp;
	*--saddr = 0;			/* %ecx */
	--saddr_cp;
	*--saddr = 0;			/* %edx */
	--saddr_cp;
	*--saddr = 0;			/* %ebx */
	--saddr_cp;
	*--saddr = 0;			/* %esp; value filled in below	*/
	--saddr_cp;
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
	--saddr_cp;
	*--saddr = 0;			/* %esi */
	--saddr_cp;
	*--saddr = 0;			/* %edi */
	--saddr_cp;
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr_cp);
	restore(mask);
	return pid;
}

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}kprintf("newpid error\n");
	return (pid32) SYSERR;
}
