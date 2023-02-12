#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

#define EMPTY 0
#define PRESENT 1
#define WRITE 2
#define WRITE_PRESENT 3
#define USER_LEVEL 4


PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;


/* Set the global parameters for the paging subsystem. */
void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
   Console::puts("Initialized Paging System\n");
}

  /* Initializes a page table with a given location for the directory and the
     page table proper.
     NOTE: The PageTable object still needs to be stored somewhere! 
     Probably it is best to have it on the stack, as there is no 
     memory manager yet...
     NOTE2: It may also be simpler to create the first page table *before* 
     paging has been enabled.
  */
PageTable::PageTable()
{
	unsigned int i;
	unsigned long address = 0;
	// Get the first frame of pool and set its location for page directory
	page_directory = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);

	// Get page table from process mem pool
	unsigned long * page_table = (unsigned long *) (process_mem_pool->get_frames(1) * PAGE_SIZE);

	//    Console::puts("::page directory::"); Console::puti((int)page_directory); Console::puts("\n\n");
	//    Console::puts("::page table::"); Console::puti((int)page_table); Console::puts("\n\n");

	for(i = 0;i < ENTRIES_PER_PAGE;i++){
		page_table[i] = address | WRITE_PRESENT;
		address = address + PAGE_SIZE;
	}

	page_directory[0] = (unsigned long) page_table | WRITE_PRESENT;

	// Mark the last entry to be write present as it is a recursive page table
	page_directory[ENTRIES_PER_PAGE - 1] = (unsigned long) (page_directory) | WRITE_PRESENT;

	// Set write bit for all entries except the 1st and last entry, which are already set
	for(i = 1;i < ENTRIES_PER_PAGE-1;i++){
		page_directory[i] = WRITE;
	}
	
	// for(i = 0;i < vmpool_max_size;i++){
	// 	list_vmpool[i] = NULL;
	// }

	vmpool_size = 0;


	Console::puts("Constructed Page Table object\n");
}

/* Makes the given page table the current table. This must be done once during
   system startup and whenever the address space is switched (e.g. during
   process switching). */
void PageTable::load()
{
	PageTable::current_page_table = this;
	write_cr3((unsigned long) page_directory);
	Console::puts("Loaded page table\n");
}

/* Enable paging on the CPU. Typically, a CPU start with paging disabled, and
   memory is accessed by addressing physical memory directly. After paging is
   enabled, memory is addressed logically. */
void PageTable::enable_paging()
{
	PageTable::paging_enabled = 1;

	write_cr0(read_cr0() | 0x80000000); 
	Console::puts("Enabled paging\n");
}

/*  The page fault handler.
	Handle page faults when the page requested is not present in page table or if page is not present at index
	Read the cr2 register to get the fault
*/ 
void PageTable::handle_fault(REGS * _r)
{
	unsigned long err_code = _r->err_code;

	// Check if page present in memory
	if ((err_code & PRESENT) == EMPTY)
	{ // Condition takes care of 000 (0), 010 (2), 100 (4), 110 (6) -> Conditions that page is not present, No protection fault
		Console::puts("The page is not present fault: ");
		if(err_code == 0){
			Console::puts("Kernel, Read accessible and page is not present\n");
		} else if(err_code == 2){
			Console::puts("Kernel, Write accessible and page is not present\n"); // I think the permission should be considered if the entry is write accessible for all further operations, but not sure
		} else if(err_code == 4){
			Console::puts("User, Read accessible and page is not present\n");
		} else if(err_code == 6){
			Console::puts("User, Write accessible and page is not present\n");
		}

		unsigned int i;
		unsigned long cr2 = read_cr2();

		bool legitimate_flag = false;
		// Console::puti(vmpool_size); Console::puts("-------------|||\n\n");
		for(i = 0;i <= current_page_table->vmpool_size;i++){
			if(current_page_table->list_vmpool[i] != NULL){
				if(current_page_table->list_vmpool[i]->is_legitimate(cr2)){
					legitimate_flag = true;
					break;
				}
			}
		}

		// Console::puti(legitimate_flag);Console::puts("--------------\t\t");Console::puti(vmpool_size);Console::puts("--------------\n\n");
		if(legitimate_flag == false && current_page_table->vmpool_size > 0){
			Console::puts("Illegitimate page\n");
			return;
		}



		unsigned long pg_tbl_idx = cr2 >> 22;			// Get first 10 bits that signify page table number
		unsigned long pg_no = (cr2 >> 12) & 0x000003FF;	// Get next 10 bits that signify page number, remove the rest of the bits from left

		unsigned long * curr_pg_dir = current_page_table->PDE_address(cr2);
		unsigned long pde_index = (cr2 & 0xFFC00000) >> 22;	// Get first 10 bits PDE
		unsigned long pte_index = (cr2 & 0x003FF000) >> 12; // Get next 10 bits PTE
		unsigned long * page_table = (unsigned long *) (0xFFC00000 | (pde_index << 12));

		bool frame_found = false;

		if((*curr_pg_dir & PRESENT) != PRESENT)
		{
			frame_found = true;
			*curr_pg_dir = (process_mem_pool->get_frames(1) << 12);
			*curr_pg_dir |= WRITE_PRESENT;
		}

		if(frame_found == true)
		{
			for(i = 0;i < ENTRIES_PER_PAGE;i++)
			{
				*(page_table + i) = 0 | WRITE;
			}
		}

		unsigned long frame = (process_mem_pool->get_frames(1)) << 12;
		*(page_table + pte_index) = frame | WRITE_PRESENT;

	}
	else 
	{ // Condition takes care of 001 (1), 011 (3), 101 (5), 111 (7) -> Conditions that there is a protection fault
		Console::puts("There is a protection fault:");
		if(err_code == 1){
			Console::puts("Kernel, Read Protection Fault\n");
		} else if(err_code == 3){
			Console::puts("Kernel, Write Protection Fault\n");
		} else if(err_code == 5){
			Console::puts("User, Read Protection Fault\n");
		} else if(err_code == 7){
			Console::puts("User, Write Protection Fault\n");
		}
	}

	Console::puts("handled page fault\n");
}

/*
	Returns the Page Directory Entry's Address
*/
unsigned long * PageTable::PDE_address(unsigned long addr)
{
    unsigned long pde_idx = (addr & 0xFFC00000) >> 22;
    return (unsigned long *)(0xFFFFF000 + (pde_idx << 2)); 
}

/*
	Return the Page Tabel Entry Address
*/
unsigned long * PageTable::PTE_address(unsigned long addr)
{
    unsigned long pte_entry = addr>>12;
    pte_entry = 0x03FF0000|pte_entry;
    
    unsigned long* pte_addr_ptr = (unsigned long*) pte_entry;
	return pte_addr_ptr;
}

/* If page is valid, release frame and mark page invalid. */
void PageTable::free_page(unsigned long _page_no)
{
	unsigned long pde_idx = (_page_no & 0xFFC00000) >> 22;
	unsigned long pte_idx = (_page_no & 0x003FF000) >> 12;
	unsigned long * page_table_entry = (unsigned long *)(0xFFC00000 | (pde_idx << 12) | (pte_idx << 2));

	if (* page_table_entry & PRESENT)
	{
		ContFramePool::release_frames(* page_table_entry >> 12);
		* page_table_entry = * page_table_entry & 0xFFFFFFFE;
		write_cr3((unsigned long) page_directory);
	}
	Console::puts("Freed page successfully\n");
}

/* Register a virtual memory pool with the page table. */
void PageTable::register_pool(VMPool * _vm_pool)
{
	list_vmpool[vmpool_size] = _vm_pool;
	vmpool_size += 1;
	
	Console::puts("VM Pool registered successfully\n");
}
