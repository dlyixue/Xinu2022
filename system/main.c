/*  main.c  - main */

#include <xinu.h>

process	main(void)
{
	//printf("Hello.elf\n");
	int32 size = 16384;
	const char *filename = "HELLO.ELF";
	char buff[size];
	read_file(filename,buff,size);
	uint32 addr = get_elf_entrypoint(buff);
	uint32 start_point = (uint32)buff+addr;
	resume(create(start_point, 8192, 50, "hello", 1, printf));
	/* Run the Xinu shell */

	//printf("Hello end\n");
	recvclr();
	resume(create(shell, 8192, 50, "shell", 1, CONSOLE));

	/* Wait for shell to exit and recreate it */

	while (TRUE) {
		receive();
		sleepms(200);
		kprintf("\n\nMain process recreating shell\n\n");
		resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
	}
	return OK;
    
}
