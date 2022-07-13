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

	int* x = syscall_allocmem(sizeof(int)*5);
	x[0] = 1;
	x[1] = 2;
	int a = x[1];
	int* y = syscall_allocmem(sizeof(int)*1025);
	syscall_printf("allocmem add:%x -> %d\n",x,a);

	syscall_resume(syscall_create(shell, 8192, 50, "shell", 1, CONSOLE));

	return OK;    
}
