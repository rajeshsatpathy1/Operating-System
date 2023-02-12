/*
 File: scheduler.C
 
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

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "blocking_disk.H"




/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/
ThreadNode* Scheduler::head;
int Scheduler::thread_count; 

/* Setup the scheduler. This sets up the ready queue, for example.
      If the scheduler implements some sort of round-robin scheme, then the 
      end_of_quantum handler is installed in the constructor as well. */
Scheduler::Scheduler() {
  Console::puts("Constructed Scheduler.\n");
}

/* Called by the currently running thread in order to give up the CPU. 
      The scheduler selects the next thread from the ready queue to load onto 
      the CPU, and calls the dispatcher function defined in 'Thread.H' to
      do the context switch. */
void Scheduler::yield() {
	if ((thread_count==0) || (head==NULL)){
		Console::puts("No threads to yield\n");
		return;	
	}

	if ((blocking_disk->disk_q_size>0) && (blocking_disk->is_ready())){
	   Thread::dispatch_to(blocking_disk->get_blockdisk_head_thread());	
	} else {
	    Thread* next_thr;
        Thread * current_thr = Thread::CurrentThread();

        if (head->node->ThreadId() == current_thr->ThreadId()){
            ThreadNode * current_head = head;
            ThreadNode * next_thr_node = head->next;

            next_thr = next_thr_node->node;
            
            head = next_thr_node;
            thread_count -= 1;
        } else {
            next_thr=Scheduler::head->node;
        }

        Thread::dispatch_to(next_thr);
    }
    Console::puts("Thread yielded successfully\n");
}

/* Add the given thread to the ready queue of the scheduler. This is called
      for threads that were waiting for an event to happen, or that have 
      to give up the CPU in response to a preemption. */
void Scheduler::resume(Thread * _thread) {
	ThreadNode* new_thread_node = new ThreadNode;

	new_thread_node->node = _thread;
	new_thread_node->next = NULL;
	
	if (head == NULL) {
		head = new_thread_node;
	} else {
        ThreadNode * temp = head;
        while(temp->next != NULL){
            temp = temp->next;
        }
	    temp->next = new_thread_node;
	}

	thread_count += 1;
    Console::puts("Thread resumed successfully\n");
}

/* Make the given thread runnable by the scheduler. This function is called
      after thread creation. Depending on implementation, this function may 
      just add the thread to the ready queue, using 'resume'. */
void Scheduler::add(Thread * _thread) {
	resume(_thread);
    Console::puts("Thread added successfully\n");
}

/* Remove the given thread from the scheduler in preparation for destruction
      of the thread. 
      Graciously handle the case where the thread wants to terminate itself.*/
void Scheduler::terminate(Thread * _thread) {
    if (_thread->ThreadId() == head->node->ThreadId()){
    Scheduler::yield();
    }

    ThreadNode * new_thread_node = head;
    ThreadNode * finished_node;

    while(new_thread_node->next != NULL){

        Thread* next_thr = new_thread_node->next->node;

        if (_thread->ThreadId() == next_thr->ThreadId()){
            finished_node = new_thread_node->next;
            new_thread_node->next = finished_node->next;
            
            delete (void *) finished_node;
        }

    }
    thread_count -= 1;
    Console::puts("Thread terminated successfully\n");
}

/* Update the thread with info on blocking disk
    */
void Scheduler::update_disk(BlockingDisk * _disk){
	blocking_disk = _disk;

    Console::puts("Thread updated with blocking disk info successfully\n");
}
