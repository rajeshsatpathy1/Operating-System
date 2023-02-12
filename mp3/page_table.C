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
	page_directory = (unsigned long *) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);

	unsigned long * page_table = (unsigned long *) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);

	//    Console::puts("::page directory::"); Console::puti((int)page_directory); Console::puts("\n\n");
	//    Console::puts("::page table::"); Console::puti((int)page_table); Console::puts("\n\n");

	for(i = 0;i < ENTRIES_PER_PAGE;i++){
		page_table[i] = address | WRITE_PRESENT;
		address = address + PAGE_SIZE;
	}

	page_directory[0] = (unsigned long) page_table;
	page_directory[0] = page_directory[0] | WRITE_PRESENT;

	for(i = 1;i < ENTRIES_PER_PAGE;i++){
		page_directory[i] = WRITE;
	}


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
	// Set 31st bit as 1
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
	if ((err_code & PRESENT) == EMPTY){ // Condition takes care of 000 (0), 010 (2), 100 (4), 110 (6) -> Conditions that page is not present
		Console::puts("The page is not present fault:");
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

		unsigned long pg_tbl_idx = cr2 >> 22;			// Get first 10 bits that signify page table number
		unsigned long pg_no = (cr2 >> 12) & 0x000003FF;	// Get next 10 bits that signify page number, remove the rest of the bits from left

		unsigned long * curr_pg_dir = current_page_table->page_directory;
		// Check if page is present at index
		if ((curr_pg_dir[pg_tbl_idx] & PRESENT) == EMPTY){
			unsigned long * new_pg_tbl = (unsigned long *)(kernel_mem_pool->get_frames(1) * PAGE_SIZE);
			curr_pg_dir[pg_tbl_idx] = (unsigned long) new_pg_tbl | WRITE_PRESENT;

			for (i = 0; i < ENTRIES_PER_PAGE; i++){
				new_pg_tbl[i] = 0 | WRITE;
			}

		} else { // page is present but the pagetable entry at the index is missing
			unsigned long curr_pg = curr_pg_dir[pg_tbl_idx];
			unsigned long * page_frame = (unsigned long *)(curr_pg & 0xFFFFF000);	// Get page frame number from 31 to 12th bit
			page_frame[pg_no] = (unsigned long)(process_mem_pool->get_frames(1) * PAGE_SIZE) | WRITE_PRESENT;	// Set the write present bits for the page
		}

	} else { // Condition takes care of 001 (1), 011 (3), 101 (5), 111 (7) -> Conditions that there is a protection fault
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