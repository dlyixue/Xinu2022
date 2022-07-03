/* meminit.c - memory bounds and free list init */

#include <xinu.h>

/* Memory bounds */

void	*minheap;		/* Start of heap			*/
void	*maxheap;		/* Highest valid heap address		*/

/* Memory map structures */

uint32	bootsign = 1;		/* Boot signature of the boot loader	*/

struct	mbootinfo *bootinfo = (struct mbootinfo *)1;
				/* Base address of the multiboot info	*/
				/*  provided by GRUB, initialized just	*/
				/*  to guarantee it is in the DATA	*/
				/*  segment and not the BSS		*/

/* Segment table structures */

/* Segment Descriptor */

struct __attribute__ ((__packed__)) sd {
	unsigned short	sd_lolimit;
	unsigned short	sd_lobase;
	unsigned char	sd_midbase;
	unsigned char   sd_access;
	unsigned char	sd_hilim_fl;
	unsigned char	sd_hibase;
};

#define	NGD			8	/* Number of global descriptor entries	*/
#define FLAGS_GRANULARITY	0x80
#define FLAGS_SIZE		0x40
#define	FLAGS_SETTINGS		(FLAGS_GRANULARITY | FLAGS_SIZE)

struct sd gdt_copy[NGD] = {
/*   sd_lolimit  sd_lobase   sd_midbase  sd_access   sd_hilim_fl sd_hibase */
/* 0th entry NULL */
{            0,          0,           0,         0,            0,        0, },
/* 1st, Kernel Code Segment */
{       0xffff,          0,           0,      0x9a,         0xcf,        0, },
/* 2nd, Kernel Data Segment */
{       0xffff,          0,           0,      0x92,         0xcf,        0, },
/* 3rd, Kernel Stack Segment */
{       0xffff,          0,           0,      0x92,         0xcf,        0, },
/* 4st, User Code Segment */
{       0xffff,          0,           0,      0xfa,         0xcf,        0, },
/* 5nd, User Data Segment */
{       0xffff,          0,           0,      0xf2,         0xcf,        0, },
/* 6nd, User stack Segment */
{       0xffff,          0,           0,      0xf2,         0xcf,        0, },
/* 7nd, TR */
{       0xffff,          0,           0,      0x89,         0x00,        0, },
};
extern	struct	sd gdt[];	/* Global segment table			*/

uint32 freememlist[1024*1024];
int fmid;

uint32 align_page(uint32 block){
	int temp = block - block%PAGE_SIZE;
	temp = temp + PAGE_SIZE;
	return temp;
}

int push_fm(int *fm){
	fmid += 1;
	if( fmid >= 1024*1024){
		return 0;
	}
	freememlist[fmid] = fm;
	return 1;
}

uint32 get_fm(){
	return freememlist[fmid--];
}

pageTable* init_pgTb(){
	pageTable* pgTb = get_fm();
	for(int i = 0; i < 1024; i++){
		pgTb->page[i] = NULL;
	}
	return pgTb;
}

pageDir init_pgDir(){
	pageDir pgDir = get_fm();

	for(int i = 0; i < 1024; i++){
		pgDirpageTbs[i] = NULL;
	}
	//物理内存对应--0,1:8MB
	for(uint32 i=0;i<2;i++){
		pgDir->pageTbs[i] = get_fm();
		for(uint32 j=0;j<1024;j++){
			pgDir->pageTbs[i]->page[j] = ((i << 10) + j) << 12 | PTE_P | PTE_W ;
		}
	}
	return pgDir;
}

int get_vm_eg(pageDir* pgDir){
	pageTable* pgTb;
	int idTb;
	int idPg;

	for(int i = 4; i < 1020; i++){
		if(pgDir->pageTbs[i] == NULL){
			pgDir->pageTbs[i] = init_pgTb();
		}
		pgTb = pgDir->pageTbs[i];
		idTb = i;

		for(int j = 0; j < 1024; j++){
			if(pgTb->page[j] == NULL){
				idPg = j;
				
			}
		}
	}
}

/*------------------------------------------------------------------------
 * meminit - initialize memory bounds and the free memory list
 *------------------------------------------------------------------------
 */
void	meminit(void) {

	struct	memblk	*memptr;	/* Ptr to memory block		*/
	struct	mbmregion	*mmap_addr;	/* Ptr to mmap entries		*/
	struct	mbmregion	*mmap_addrend;	/* Ptr to end of mmap region	*/
	struct	memblk	*next_memptr;	/* Ptr to next memory block	*/
	uint32	next_block_length;	/* Size of next memory block	*/

	mmap_addr = (struct mbmregion*)NULL;
	mmap_addrend = (struct mbmregion*)NULL;

	/* Initialize the free list */
	memptr = &memlist;
	memptr->mnext = (struct memblk *)NULL;
	memptr->mlength = 0;

	/* Initialize the memory counters */
	/*    Heap starts at the end of Xinu image */
	minheap = (void*)&end;
	maxheap = minheap;

	/* Check if Xinu was loaded using the multiboot specification	*/
	/*   and a memory map was included				*/
	if(bootsign != MULTIBOOT_SIGNATURE) {
		panic("could not find multiboot signature");
	}
	if(!(bootinfo->flags & MULTIBOOT_BOOTINFO_MMAP)) {
		panic("no mmap found in boot info");
	}

	/* Get base address of mmap region (passed by GRUB) */
	mmap_addr = (struct mbmregion*)bootinfo->mmap_addr;

	/* Calculate address that follows the mmap block */
	mmap_addrend = (struct mbmregion*)((uint8*)mmap_addr + bootinfo->mmap_length);

	/* Read mmap blocks and initialize the Xinu free memory list	*/
	while(mmap_addr < mmap_addrend) {

		/* If block is not usable, skip to next block */
		if(mmap_addr->type != MULTIBOOT_MMAP_TYPE_USABLE) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		if((uint32)maxheap < ((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length)) {
			maxheap = (void*)((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length);
		}

		/* Ignore memory blocks within the Xinu image */
		if((mmap_addr->base_addr + mmap_addr->length) < ((uint32)minheap)) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		/* The block is usable, so add it to Xinu's memory list */

		/* This block straddles the end of the Xinu image */
		if((mmap_addr->base_addr <= (uint32)minheap) &&
		  ((mmap_addr->base_addr + mmap_addr->length) >
		  (uint32)minheap)) {

			/* This is the first free block, base address is the minheap */
			next_memptr = (struct memblk *)roundmb(minheap);

			/* Subtract Xinu image from length of block */
			next_block_length = (uint32)truncmb(mmap_addr->base_addr + mmap_addr->length - (uint32)minheap);
		} else {

			/* Handle a free memory block other than the first one */
			next_memptr = (struct memblk *)roundmb(mmap_addr->base_addr);

			/* Initialize the length of the block */
			next_block_length = (uint32)truncmb(mmap_addr->length);
		}

		/* Add then new block to the free list */
		memptr->mnext = next_memptr;
		memptr = memptr->mnext;
		memptr->mlength = next_block_length;
		memlist.mlength += next_block_length;

		/* Move to the next mmap block */
		mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
	}

	/* End of all mmap blocks, and so end of Xinu free list */
	if(memptr) {
		memptr->mnext = (struct memblk *)NULL;
	}
}

void vminit(void) {

	struct	memblk	*memptr;	/* Ptr to memory block		*/
	struct	mbmregion	*mmap_addr;	/* Ptr to mmap entries		*/
	struct	mbmregion	*mmap_addrend;	/* Ptr to end of mmap region	*/
	struct	memblk	*next_memptr;	/* Ptr to next memory block	*/
	uint32	next_block_length;	/* Size of next memory block	*/

	mmap_addr = (struct mbmregion*)NULL;
	mmap_addrend = (struct mbmregion*)NULL;

	/* Initialize the free list */
	memptr = &memlist;
	memptr->mnext = (struct memblk *)NULL;
	memptr->mlength = 0;

	/* Initialize the memory counters */
	/*    Heap starts at the end of Xinu image */
	minheap = (void*)&end;
	maxheap = minheap;

	/* Check if Xinu was loaded using the multiboot specification	*/
	/*   and a memory map was included				*/
	if(bootsign != MULTIBOOT_SIGNATURE) {
		panic("could not find multiboot signature");
	}
	if(!(bootinfo->flags & MULTIBOOT_BOOTINFO_MMAP)) {
		panic("no mmap found in boot info");
	}

	/* Get base address of mmap region (passed by GRUB) */
	mmap_addr = (struct mbmregion*)bootinfo->mmap_addr;

	/* Calculate address that follows the mmap block */
	mmap_addrend = (struct mbmregion*)((uint8*)mmap_addr + bootinfo->mmap_length);

	/* Read mmap blocks and initialize the Xinu free memory list	*/
	while(mmap_addr < mmap_addrend) {

		/* If block is not usable, skip to next block */
		if(mmap_addr->type != MULTIBOOT_MMAP_TYPE_USABLE) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		if((uint32)maxheap < ((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length)) {
			maxheap = (void*)((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length);
		}

		/* Ignore memory blocks within the Xinu image */
		if((mmap_addr->base_addr + mmap_addr->length) < ((uint32)minheap)) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		/* The block is usable, so add it to Xinu's memory list */

		/* This block straddles the end of the Xinu image */
		if((mmap_addr->base_addr <= (uint32)minheap) &&
		  ((mmap_addr->base_addr + mmap_addr->length) >
		  (uint32)minheap)) {

			/* This is the first free block, base address is the minheap */
			next_memptr = (struct memblk *)roundmb(minheap);

			/* Subtract Xinu image from length of block */
			next_block_length = (uint32)truncmb(mmap_addr->base_addr + mmap_addr->length - (uint32)minheap);
		} else {

			/* Handle a free memory block other than the first one */
			next_memptr = (struct memblk *)roundmb(mmap_addr->base_addr);

			/* Initialize the length of the block */
			next_block_length = (uint32)truncmb(mmap_addr->length);
		}

		uint32 freeblock = (uint32)next_memptr;
		freeblock = align_page(freeblock);
		//4096 get physical page
		for(int i = 0; i < next_block_length/PAGE_SIZE;i++){
			int tmp = push_fm(freeblock);
			//printf("%x\n",freeblock);
			if(tmp == 0){
				break;
			}
			freeblock += PAGE_SIZE;
		}

		/* Add then new block to the free list */
		memptr->mnext = next_memptr;
		memptr = memptr->mnext;
		memptr->mlength = next_block_length;
		memlist.mlength += next_block_length;

		/* Move to the next mmap block */
		mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
	}

	/* End of all mmap blocks, and so end of Xinu free list */
	if(memptr) {
		memptr->mnext = (struct memblk *)NULL;
	}
}

/*------------------------------------------------------------------------
 * setsegs  -  Initialize the global segment table
 *------------------------------------------------------------------------
 */
void	setsegs()
{
	extern int	etext;
	struct sd	*psd;
	uint32		np, ds_end;		

	ds_end = 0xffffffff/PAGE_SIZE; /* End page number of Data segment */

	psd = &gdt_copy[1];	/* Kernel code segment: identity map from address
				   0 to etext */
	np = ((int)&etext - 0 + PAGE_SIZE-1) / PAGE_SIZE;	/* Number of code pages */
	psd->sd_lolimit = np;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((np >> 16) & 0xff);

	psd = &gdt_copy[2];	/* Kernel data segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_copy[3];	/* Kernel stack segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_copy[4];	/* User code segment: identity map from address
				   0 to etext */
	np = ((int)&etext - 0 + PAGE_SIZE-1) / PAGE_SIZE;	/* Number of code pages */
	psd->sd_lolimit = np;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((np >> 16) & 0xff);

	psd = &gdt_copy[5];	/* User data segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_copy[6];	/* User stack segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	//TR初始化
	psd = &gdt_copy[7];
	uint32 tss = &TSS;
	psd->sd_lobase = tss;
	psd->sd_midbase = tss>>16;
	psd->sd_hibase = tss>>24;
	psd->sd_lolimit = 0xffff & (sizeof(TSS)-1);

	memcpy(gdt, gdt_copy, sizeof(gdt_copy));
}
