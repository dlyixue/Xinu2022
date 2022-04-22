/*  main.c  - main */

#include <xinu.h>
void sndA(void) {
	while (1);
}
process	main(void)
{

	/* Run the Xinu shell */
	//resume(create(sndA, 1024, 40, "process 1", 0));

	syscall_recvclr();
	syscall_resume(syscall_create(shell, 8192, 50, "shell", 1, CONSOLE));

	return OK;    
}
