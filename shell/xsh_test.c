/* xsh_help.c - xsh_help */

#include <xinu.h>
#include <stdio.h>
#include <string.h>

/*------------------------------------------------------------------------
 * xhs_help - display help message that lists shell commands
 *------------------------------------------------------------------------
 */
shellcmd xsh_test(int nargs, char *args[])
{
	int32	i;
	char	*argv[2];		/* argument vector for call	*/
	char	*src, *cmp;		/* used for string compare	*/
	int32	len;			/* length of a command name	*/
	int32	maxlen;			/* maximum length of all	*/
					/*   command names		*/
	int32	cols;			/* number of columns in the	*/
					/*   formatted command list	*/
	int32	spac;			/* space per column in the	*/
					/*   formatted command list	*/
	int32	lines;			/* total lines of output in the	*/
					/*   formatted command list	*/
	int32	j;			/* index of commands across one	*/
					/*   line of formatted output	*/

	/* No arguments -- print a list of shell commands */

	syscall_printf("\ntest ouput are:\n\n");

	syscall_printf("abcdefghigklmn\n");
	syscall_printf("!@#@!$!EWFQASGGQQWEWQ\n");
	syscall_printf("abc\ndefg\thig\nklm\tn\n");
	syscall_printf("size = 27\n");
	syscall_printf("aaaaaaaaaaaaaaaaaaaaaaaaa\n");
	for(int i = 0;i <7 ;i++){
		syscall_printf("%d \n",i);
	}
	syscall_printf("test_over\n");
	return 0;
}
