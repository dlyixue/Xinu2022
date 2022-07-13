/* addargs.c - addargs */

#include <xinu.h>
#include "shprototypes.h"

/*------------------------------------------------------------------------
 *  addargs  -  Add local copy of argv-style arguments to the stack of
 *		  a command process that has been created by the shell
 *------------------------------------------------------------------------
 */
status	addargs(
	  pid32		pid,		/* ID of process to use		*/
	  int32		ntok,		/* Count of arguments		*/
	  int32		tok[],		/* Index of tokens in tokbuf	*/
	  int32		tlen,		/* Length of data in tokbuf	*/
	  char		*tokbuf,	/* Array of null-term. tokens	*/
	  void 		*dummy		/* Dummy argument that was	*/
					/*   used at creation and must	*/
					/*   be replaced by a pointer	*/
					/*   to an argument vector	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	procent *prptr;		/* Ptr to process' table entry	*/
	uint32	aloc;			/* Argument location in process	*/
					/*   stack as an integer	*/
	uint32	*argloc;		/* Location in process's stack	*/
					/*   to place args vector	*/
	char	*argstr;		/* Location in process's stack	*/
					/*   to place arg strings	*/
	uint32	*search;		/* pointer that searches for	*/
					/*   dummy argument on stack	*/
	uint32	*aptr;			/* Walks through args array	*/
	int32	i;			/* Index into tok array		*/

	mask = disable();

	/* Check argument count and data length */

	if ( (ntok <= 0) || (tlen < 0) ) {
		restore(mask);
		return SYSERR;
	}

	prptr = &proctab[pid];

	aloc = (uint32)(0x1fff000 - prptr->prstklen + sizeof(uint32));
	argloc = (uint32*) ((aloc + 3) & ~0x3);	/* round multiple of 4	*/

	uint32 stk_pg = proctab[pid].pageDir;
	General_page(stk_pg); 
	stk_pg = get_Gpage(1022); 
	General_page(stk_pg);
	stk_pg = get_Gpage(1023);
	General_page(stk_pg);

	uint32 *Usaddr = (uint32 *)0x1fff000;
	int flag = 0;

	for (search = (uint32 *)Usaddr;
	     search < (uint32 *)0x2000000; search++) {

		/* If found, replace with the address of the args vector*/

		if (*search == (uint32)dummy) {
			*search = (uint32)argloc;
			flag = 1;
			// restore(mask);
			// return OK;
		}
	}
	if(flag == 0){
		/* Argument value not found on the stack - report an error */
		restore(mask);
		return SYSERR;
	}

	/* Compute lowest location in the process stack where the	*/
	/*	args array will be stored followed by the argument	*/
	/*	strings							*/
	stk_pg = proctab[pid].pageDir;
	General_page(stk_pg); 
	stk_pg = get_Gpage(1023); 
	General_page(stk_pg);
	stk_pg = get_Gpage(1023);
	General_page(stk_pg);

	/* Compute the first location beyond args array for the strings	*/

	argstr = (char *) (argloc + (ntok+1));	/* +1 for a null ptr	*/

	/* Set each location in the args vector to be the address of	*/
	/*	string area plus the offset of this argument		*/

	for (aptr=argloc, i=0; i < ntok; i++) {
		*aptr++ = (uint32) (argstr + tok[i]);
	}

	/* Add a null pointer to the args array */

	*aptr++ = (uint32)NULL;

	/* Copy the argument strings from tokbuf into process's	stack	*/
	/*	just beyond the args vector				*/

	memcpy(aptr, tokbuf, tlen);

	restore(mask);
	return OK;
}
