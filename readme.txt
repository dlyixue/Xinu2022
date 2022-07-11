qemu-system-i386 -S -s -kernel xinu.elf
target remote :1234
file xinu.elf

//先映射页目录
//在映射0-8M，不需要重新创建
//使用父进程创建的0，1页表即可。
//构建栈，映射过去
//然后在栈空间进行读写
//ctxsw