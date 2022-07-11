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
	uint32 temp = block - block%PAGE_SIZE;
	temp = temp + PAGE_SIZE;
	return temp;
}

void MkpgAccessibleby0x1fff000(uint32 phy_add){
    intmask mask = disable();
    ((pgTab)(freememlist[2]))->entries[1023] = phy_add | PTE_P | PTE_W; // now we hope that pddr's physical page can be accessed by 0x1fff000
    update_tlb((pgTab)0x1fff000);
    restore(mask);
}

void map_phy(uint32 phy_add,int i){
	intmask mask = disable();
	if(i == 1022){
		((pgTab)(freememlist[2]))->entries[1022] = phy_add | PTE_P | PTE_W;
		update_tlb((pgTab)0x1ffe000);
	}
	if(i == 1021){
		((pgTab)(freememlist[2]))->entries[1021] = phy_add | PTE_P | PTE_W;
		update_tlb((pgTab)0x1ffd000);
	}
    restore(mask);
}

void write_phy(int i,int entry){
	if(i == 1022){
		((pgTab)0x1ffe000)->entries[i] = entry;
	}
	if(i == 1021){
		((pgTab)0x1ffd000)->entries[i] = entry;
	}
}

int push_fm(int fm){
	if( fmid >= 1024*1024 -1 || fm == 0){
		return 0;
	}
	freememlist[fmid++] = fm;
	return 1;
}

uint32 get_fm(){
	for(int i = fmid - 1; i >=3; i--){
		if((freememlist[i] & 1) == 0){
			uint32 temp = freememlist[i];
			freememlist[i] = freememlist[i] | 1;
			kprintf("used physical address: 0x%x\n", (uint32)temp);
			return temp;
		}
	}
	return 0;
}

void init_kernel_mem(){
	// kernel is same 0-4M 4-8M
	for(uint32 i=0;i<2;i++){
		pgTab page = (pgTab)freememlist[i];
		for(uint32 j=0;j<1024;j++){
			page->entries[j] = ((i << 10) + j) << 12 | PTE_kernel;
		}
	}
}


pgDir init_pgDir(){
	pgDir pageDir = get_fm();
	//物理内存对应--0,1:8MB
	pageDir->entries[0] = freememlist[0] | PTE_kernel;
	pageDir->entries[1] = freememlist[1] | PTE_kernel;
	init_kernel_mem();
	//固化2个临时页，用于临时表的映射
	pgTab temp_page = freememlist[2];
	temp_page->entries[1023] = get_fm() | PTE_kernel;//temp;
	temp_page->entries[1022] = get_fm() | PTE_kernel;//kernel-stack;
	temp_page->entries[1021] = get_fm() | PTE_kernel;//user-stack
	int temp = temp_page;
	pageDir->entries[7] = temp | PTE_kernel;
	return pageDir;
}

void set_cr3(pgDir pageDir) {
    asm volatile (
        "mov %0, %%cr3\n\t"
        :
        :"r"(pageDir)
        :
    );
}

void update_tlb(void* page){
	asm volatile(
        "invlpg (%0)\n\t"
        :
        : "r"(page)
        : "memory");
}

pgDir get_pgDir(){
	pgDir pageDir = get_fm();
	MkpgAccessibleby0x1fff000(pageDir);
	for (int i = 0; i <= 2; i++){
        Write0x1fff000(i, freememlist[i] | PTE_P | PTE_W); // copy 0 - 32 MB
    }
	return pageDir;
}

char *alloc_kstk(uint32 nbytes, uint32 pgdir){
	//use1023
    intmask mask = disable();
    uint32 pages_needed = 1024;
    MkpgAccessibleby0x1fff000(pgdir);

    uint32 stk_pgtb = (uint32)get_fm(); // one page table is enough for 4MB‘s stack
    MkpgAccessibleby0x1fff000(pgdir);
    Write0x1fff000(1023, stk_pgtb | PTE_P | PTE_W); // virtual high 10 bits: 1111111111 -> 1023
    MkpgAccessibleby0x1fff000(stk_pgtb);            // now the only stack page table page can be accessed by 0x1fff000
    memset((void *)0x1fff000, 0, PAGE_SIZE);
    uint32 stk_pg, stk_pg_one;
    for (int i = 0; i < pages_needed; i++)
    {
        stk_pg = (uint32)get_fm();
        if (i == 0)
        {
            stk_pg_one = stk_pg;
        }
        MkpgAccessibleby0x1fff000(stk_pgtb);              // now the only stack page table page can be accessed by 0x1fff000
        Write0x1fff000(1023 - i, stk_pg | PTE_P | PTE_W); // virtual low 10 bits from 1111111111 to 0
    }
    //MkpgAccessibleby0x1fff000(stk_pg_one); // still use temp, then we can initialize the stack by 0x1fff000 in the rest of create
    map_phy(stk_pg_one,1022);
	restore(mask);
    return (char *)0xfffffffc;             // 4GB - 4
}

char *alloc_ustk(uint32 nbytes, uint32 pgdir){
	//use1022
	intmask mask = disable();
    uint32 pages_needed = 1024;
    MkpgAccessibleby0x1fff000(pgdir);

    uint32 stk_pgtb = (uint32)get_fm(); // one page table is enough for 4MB‘s stack
    MkpgAccessibleby0x1fff000(pgdir);
    Write0x1fff000(1022, stk_pgtb | PTE_P | PTE_W); // virtual high 10 bits: 1111111111 -> 1023
    MkpgAccessibleby0x1fff000(stk_pgtb);            // now the only stack page table page can be accessed by 0x1fff000
    memset((void *)0x1fff000, 0, PAGE_SIZE);
    uint32 stk_pg, stk_pg_one;
    for (int i = 0; i < pages_needed; i++)
    {
        stk_pg = (uint32)get_fm();
        if (i == 0)
        {
            stk_pg_one = stk_pg;
        }
        MkpgAccessibleby0x1fff000(stk_pgtb);              // now the only stack page table page can be accessed by 0x1fff000
        Write0x1fff000(1023 - i, stk_pg | PTE_P | PTE_W); // virtual low 10 bits from 1111111111 to 0
    }
    //MkpgAccessibleby0x1fff000(stk_pg_one); // still use temp, then we can initialize the stack by 0x1fff000 in the rest of create
    map_phy(stk_pg_one,1021);
	restore(mask);
    return (char *)0xffbffffc;             // 4GB - 4M - 8
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
		for(int i = 0; i < next_block_length/PAGE_SIZE - 1;i++){
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
