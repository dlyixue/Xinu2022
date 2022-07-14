qemu-system-i386 -S -s -kernel xinu.elf
target remote :1234
file xinu.elf

//kbdvga
//ps
//shell 在显示器上运行
//读写
//通过buf 拷贝 读写
//test命令