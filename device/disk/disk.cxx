#include <xinu.h>

typedef struct{
	int sector;
	int pointer;
}disk;

disk dk;
extern "C"
{
devcall	diskread(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char	*buff,			/* Buffer of characters		*/
	  int32	count 			/* Count of character to read	*/
	)
{
	int sector_num = (count+dk.pointer)/512;
	if(((count + dk.pointer) % 512) !=0 ){
		sector_num++;
	}
    //printf("sector:%d\noffset:%d\ncount:%d\n",dk.sector,dk.pointer,count);
    int start = 0;
    int end = count;
	for(int i = 0;i<sector_num;i++){
		int eax = dk.sector;
		int t1 = (eax>>24) | 0b11100000;
		outb(0x01F6, t1);
		//kprintf("1F6:%d\n",t1);

		int cl = 1;
		outb(0x1F2, cl);
		//kprintf("1F2:%d\n",cl);

		int t3 = eax;
		outb(0x1F3, t3);
		//kprintf("1F3:%d\n",t3);

		int t4 = eax >> 8;
		outb(0x1F4, t4);
		//kprintf("1F4:%d\n",t4);

		int t5 = eax >> 16;
		outb(0x1F5, t5);
		//kprintf("1F5:%d\n",t5);

		int t7 = 0x20;
		outb(0x1F7, t7);
		//kprintf("1F7:%d\n",t7);

		uint8 tmp;
		
		while(1){
			tmp = inb(0x1F7);
			//kprintf("%d\n",tmp);
			if((tmp & 8) != 0){
				break;
			}
		}

		char buff_tmp[512];
		insw(0x1f0,(int32)buff_tmp,256);
		
        for(start;start<end;start++){
            if(dk.pointer == 512){
                dk.sector++;
                dk.pointer = 0;
            }
            buff[start] = buff_tmp[dk.pointer];
            dk.pointer++;
        }
	}
	return count;
}

devcall	diskgetc(
	  struct dentry	*devptr		/* Entry in device switch table	*/
    )
{
    // TODO
	char buff[512];
	int32 count = 1;
	diskread(devptr,buff,count);
	char ch = buff[0];
	return ch;
}

devcall	diskseek (
	  struct dentry *devptr,	/* Entry in device switch table */
	  uint32	offset		/* Byte position in the file	*/
	)
{
    // TODO
	dk.sector = offset/512;
	dk.pointer = offset%512;
	return dk.sector;
}

}