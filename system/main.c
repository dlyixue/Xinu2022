/*  main.c  - main */

#include <xinu.h>

// running code in experiment 
void sndA(void) {
	while(1){
		putc(CONSOLE,'A');
	}
}

void sndB(void) {
	while(1){
		putc(CONSOLE,'B');
	}
}

process	main(void)
{
	//qemu-system-i386 -S -s -kernel xinu.elf

	// running code in experiment 
	//	getpid & getprio
	
	//printf("pid: %d, prio: %d\n",getpid(),getprio(getpid()));
	resume(create(sndA, 1024, 20, "process 1", 0));
	resume(create(sndB, 1024, 20, "process 2", 0));
	//resume(create(sndA, 1024, 60, "process 1", 0));
	//resume(create(sndB, 1024, 60, "process 2", 0));
	
	/* Run the Xinu shell */
	recvclr();
	resume(create(shell, 8192, 50, "shell", 1, CONSOLE));


	/* Wait for shell to exit and recreate it */

	// while (TRUE) {
	// 	receive();
	// 	sleepms(200);
	// 	kprintf("\n\nMain process recreating shell\n\n");
	// 	resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
	// }
	return OK;
    
}
