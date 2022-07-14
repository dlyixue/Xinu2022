#include <xinu.h>
#include "kbd.h"

#define vga (80*24)
#define width 80
#define addr_base 0xb8000
#define putc_vga(ptr, ch) (*ptr = ((int)ch & 0xff) | 0x0700)
#define get_addr0(row,col) (0xb8000+2*(80*row+col))
#define get_addr1(pos) (0xb8000+2*pos)

int get_pos(){
	//获取光标位置
	//pos = col + 80*row
	int pos = 0;
	outb(0x3D4, 14);
	pos = inb(0x3D5) << 8;
	outb(0x3D4, 15);
	pos |= inb(0x3D5);
	return pos;
}

void set_pos(int pos){
	//设置光标
	outb(0x3D4, 14);
	outb(0x3D5, pos >> 8);
	outb(0x3D4, 15);
	outb(0x3D5, pos);
	*((uint16*)0xB8000 + pos) = ' ' | 0x0700;
}


devcall kbdvgainit(void){
	struct dentry *devptr = (struct dentry *) &devtab[KBDVGA];
	set_evec(devptr->dvirq, (uint32)devptr->dvintr);
	for(int row = 0; row < 24 ;row++){
		for(int col = 0; col <80 ;col++){
			uint16* addr = get_addr0(row,col);
			putc_vga(addr,' ');
		}
	}
	set_pos(0);
}

devcall vgaputc(struct dentry *devptr, char ch){
	int pos = get_pos();
	if(ch == TY_NEWLINE){
		pos += width - pos%width;
	}
	else{
		uint16 *addr = get_addr1(pos);
		putc_vga(addr,ch);
		pos++;
	}
	if(pos > vga - 1){
		memcpy(addr_base, addr_base + width * 2, 2 * width * 23);
		pos -= width;
		memset(addr_base + width * 23 * 2, 0, width * 2);
	}
	set_pos(pos);
}

void fb_erase1(){
	int pos = get_pos();
	pos--;
	uint16 * addr = get_addr1(pos);
	putc_vga(addr,' ');
	set_pos(pos);
}

void kbdhandler(void) {
	struct dentry *devptr = (struct dentry *) &devtab[KBDVGA];
	int ch,pos;
	while ((ch = kbdgetc(devptr)) > 0){
		pos = get_pos();
		if(ch == TY_BACKSP){
			if(pos > 0){
				fb_erase1();
			}
		}
		else if(ch == TY_NEWLINE || ch == TY_RETURN){
			vgaputc(devptr,TY_NEWLINE);
		}
		else{
			vgaputc(devptr,ch);
		}
	}
}

