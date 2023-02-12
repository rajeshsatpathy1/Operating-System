/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/
#define NUM_SPACES 256

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

/* Initializes the data structures needed for the management of this
    * virtual-memory pool.
    * _base_address is the logical start address of the pool.
    * _size is the size of the pool in bytes.
    * _frame_pool points to the frame pool that provides the virtual
    * memory pool with physical memory frames.
    * _page_table points to the page table that maps the logical memory
    * references to physical addresses. */
VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) 
{
    base_address = _base_address;
    vmpool_size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
    page_table->register_pool(this);

    alloc_space = (struct alloc_space *) base_address;
    alloc_space[0].start_address = base_address;
    // Console::puti(alloc_space[0].start_address);Console::puts("Hererere\n");
    alloc_space[0].size = Machine::PAGE_SIZE;

    // The 1st space is allocated 
    space_iter = 1;

    Console::puts("Constructed VMPool object.\n");
}

/* Allocates a region of _size bytes of memory from the virtual
    * memory pool. If successful, returns the virtual address of the
    * start of the allocated region of memory. If fails, returns 0. */
unsigned long VMPool::allocate(unsigned long _size) 
{
    if (_size == 0)
    {
        Console::puts("Nothing allocated");
        return 0;
    }

    unsigned int page_count = _size / Machine::PAGE_SIZE;
    unsigned rem = _size % Machine::PAGE_SIZE;

    if (rem > 0)
    {
        page_count += 1;
    }

    unsigned long final_size = page_count * Machine::PAGE_SIZE;


    alloc_space[space_iter].start_address = alloc_space[space_iter-1].start_address + alloc_space[space_iter-1].size;

    alloc_space[space_iter].size = final_size;

    space_iter += 1;

    Console::puts("Allocated region of memory.\n");
    return alloc_space[space_iter-1].start_address + final_size;
}

/* Releases a region of previously allocated memory. The region
    * is identified by its start address, which was returned when the
    * region was allocated. */
void VMPool::release(unsigned long _start_address) 
{
    unsigned int idx;
    unsigned int i;

    // Find the allocated space that has the start address that is to be released
    for(i = 0;i < NUM_SPACES;i++)
    {
        if (alloc_space[i].start_address == _start_address)
        {
            idx = i;
            break;    
        }
    }

    unsigned int page_count = alloc_space[idx].size / Machine::PAGE_SIZE;
    unsigned long addr;

    for(i = 0;i < page_count;i++){
        addr = alloc_space[idx].start_address + i*Machine::PAGE_SIZE;
        page_table->free_page(addr);
    }

    // left shift the allocated spaces to the left after removing allocated space at index.
    for(i=idx;i<space_iter-1;i++){
        alloc_space[i] = alloc_space[i+1];
    }
    space_iter -= 1;
    page_table->load();

    Console::puts("Released region of memory.\n");
}

/* Returns false if the address is not valid. An address is not valid
    * if it is not part of a region that is currently allocated. */
bool VMPool::is_legitimate(unsigned long _address) {
    if(_address >= base_address && _address < (base_address+vmpool_size))
    {
        return true;
    }
    return false;
}

