/* memory.h - roundmb, truncmb, freestk */

#define	PAGE_SIZE	4096

/*----------------------------------------------------------------------
 * roundmb, truncmb - Round or truncate address to memory block size
 *----------------------------------------------------------------------
 */
#define	roundmb(x)	(char *)( (7 + (uint32)(x)) & (~7) )
#define	truncmb(x)	(char *)( ((uint32)(x)) & (~7) )

/*----------------------------------------------------------------------
 *  freestk  --  Free stack memory allocated by getstk
 *----------------------------------------------------------------------
 */
#define	freestk(p,len)	freemem((char *)((uint32)(p)		\
				- ((uint32)roundmb(len))	\
				+ (uint32)sizeof(uint32)),	\
				(uint32)roundmb(len) )

struct	memblk	{			/* See roundmb & truncmb	*/
	struct	memblk	*mnext;		/* Ptr to next free memory blk	*/
	uint32	mlength;		/* Size of blk (includes memblk)*/
	};
extern	struct	memblk	memlist;	/* Head of free memory list	*/
extern	void	*minheap;		/* Start of heap		*/
extern	void	*maxheap;		/* Highest valid heap address	*/

/* Added by linker */

extern	int	text;			/* Start of text segment	*/
extern	int	etext;			/* End of text segment		*/
extern	int	data;			/* Start of data segment	*/
extern	int	edata;			/* End of data segment		*/
extern	int	bss;			/* Start of bss segment		*/
extern	int	ebss;			/* End of bss segment		*/
extern	int	end;			/* End of program		*/

typedef unsigned long DWORD;
typedef struct TSS_struct {
    DWORD link; // 保存前一个 TSS 段选择子，使用 call 指令切换寄存器的时候由CPU填写。
    // 这 6 个值是固定不变的，用于提权，CPU 切换栈的时候用
    DWORD esp0; // 保存 0 环栈指针
    DWORD ss0;  // 保存 0 环栈段选择子
    DWORD esp1; // 保存 1 环栈指针
    DWORD ss1;  // 保存 1 环栈段选择子
    DWORD esp2; // 保存 2 环栈指针
    DWORD ss2;  // 保存 2 环栈段选择子
    // 下面这些都是用来做切换寄存器值用的，切换寄存器的时候由CPU自动填写。
    DWORD cr3; 
    DWORD eip;  
    DWORD eflags;
    DWORD eax;
    DWORD ecx;
    DWORD edx;
    DWORD ebx;
    DWORD esp;
    DWORD ebp;
    DWORD esi;
    DWORD edi;
    DWORD es;
    DWORD cs;
    DWORD ss;
    DWORD ds;
    DWORD fs;
    DWORD gs;
    DWORD ldt;
    // 这个暂时忽略
    DWORD io_map;
} TSS_type;
TSS_type TSS;
extern	TSS_type TSS;