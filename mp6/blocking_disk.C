/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"
#include "thread.H"

extern Scheduler * SYSTEM_SCHEDULER;



/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

/* Creates a BlockingDisk device with the given size connected to the 
      MASTER or SLAVE slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */
BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
    Console::puts("Blockdisk constructed successfully\n");
}

/*--------------------------------------------------------------------------*/
/* BLOCKING_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

/* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */
void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  wait_until_ready();
  SimpleDisk::read(_block_no, _buf);
  Console::puts("Blockdisk read successfully\n");
}

/* Writes 512 Bytes from the buffer to the given block on the disk. */
void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  wait_until_ready();
  SimpleDisk::write(_block_no, _buf);

  Console::puts("Blockdisk written successfully\n");
}

/* Add disk thread to the scheduler (Option 3&4)
   */
void BlockingDisk::add_threadnode_blockdisk(Thread* _thread){
  ThreadNode * new_thread_node = new ThreadNode;
  new_thread_node->node = _thread;
  new_thread_node->next = NULL;

  if (blockdisk_thread_head == NULL){
    blockdisk_thread_head=new_thread_node;
  } else {
    blockdisk_thread_head->next = new_thread_node;
    blockdisk_thread_head = new_thread_node;
  }

  disk_q_size += 1;

  Console::puts("Added block disk to concurrent thread successfully\n");
}

/* Get the head thread from the blockdisk queue (Option 3&4)
   */
Thread* BlockingDisk::get_blockdisk_head_thread(){
  ThreadNode* current_head = blockdisk_thread_head;
  ThreadNode* next_thread_node=blockdisk_thread_head->next;
  Thread* next_thread = next_thread_node->node;
	
  blockdisk_thread_head = next_thread_node;
  disk_q_size -= 1;

  Console::puts("Removed block disk head thread and returned the next head successfully\n");
  return current_head->node;
}

/* A modification of the wait_until_ready from the simple disk implementation
  */
void BlockingDisk::wait_until_ready() {
    if (!BlockingDisk::is_ready()) {
        Thread * current_thread = Thread::CurrentThread();
        add_threadnode_blockdisk(current_thread);
        
        SYSTEM_SCHEDULER->yield();
    }
}

/* Check if disk is ready for read/write operations. Calls the simple disk implementation
   */
bool BlockingDisk::is_ready() {
    return SimpleDisk::is_ready();
}