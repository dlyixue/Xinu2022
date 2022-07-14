/* kbdread.c - kbdread */

#include <xinu.h>
#include "kbd.h"

/*------------------------------------------------------------------------
 *  kbdread  -  Read character(s) from a kbd device (interrupts disabled)
 *------------------------------------------------------------------------
 */
devcall	kbdread(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char	*buff,			/* Buffer of characters		*/
	  int32	count 			/* Count of character to read	*/
	)
{
	int32	avail;			/* Characters available in buff.*/
	int32	nread;			/* Number of characters read	*/
	int32	firstch;		/* First input character on line*/
	char	ch;			/* Next input character		*/

	if (count < 0) {
		return SYSERR;
	}

	/* Block until input arrives */
	nread = 0;
	kbdcb.last_c = ' ';
	kbdcb.tyicursor = 0;
	while (1){
		wait(kbdcb.tyisem);
		firstch = kbdcb.last_c;
		nread++;
		if (firstch == EOF) {
			return EOF;
		}
		/* Read up to a line */
		ch = (char) firstch;
		if(ch == TY_NEWLINE){
			//buff = kbdcb.tyibuff;
			break;
		}
	}
	for(int i = 0;i<kbdcb.tyicursor;i++){
		buff[i] = kbdcb.tyibuff[i];
	}
	return kbdcb.tyicursor;
}
