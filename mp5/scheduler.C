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

/* Setup the scheduler. This sets up the ready queue, for example.
If the scheduler implements some sort of round-robin scheme, then the 
end_of_quantum handler is installed in the constructor as well. */

/* NOTE: We are making all functions virtual. This may come in handy when
you want to derive RRScheduler from this class. */
Scheduler::Scheduler() {
	// ready_Q = new FIFO_QUEUE();
  q_size = 0;

  Console::puts("Constructed Scheduler.\n");
}

/* Called by the currently running thread in order to give up the CPU. 
The scheduler selects the next thread from the ready queue to load onto 
the CPU, and calls the dispatcher function defined in 'Thread.H' to
do the context switch. */
void Scheduler::yield() {
  if (Machine::interrupts_enabled()){
    Machine::disable_interrupts();
  }
  

  if(q_size > 0)
  {
    Thread * first_thread = ready_Q.deQ();
    q_size--;
    
    Thread::dispatch_to(first_thread);
  }


  if (!Machine::interrupts_enabled()){
    Machine::enable_interrupts();
  }

  Console::puts("Yielded Thread.\n");
}

/* Add the given thread to the ready queue of the scheduler. This is called
for threads that were waiting for an event to happen, or that have 
to give up the CPU in response to a preemption. */

void Scheduler::resume(Thread * _thread) {
	if (Machine::interrupts_enabled()){
    Machine::disable_interrupts();
  }
		
  ready_Q.enQ(_thread);
  q_size++;
  
	if (!Machine::interrupts_enabled()){
    Machine::enable_interrupts();
  }

  Console::puts("Resumed Thread\n");
}

/* Make the given thread runnable by the scheduler. This function is called
after thread creation. Depending on implementation, this function may 
just add the thread to the ready queue, using 'resume'. */
void Scheduler::add(Thread * _thread) {
  Scheduler::resume(_thread);
  Console::puts("Added Thread\n");
}

/* Remove the given thread from the scheduler in preparation for destruction
of the thread. 
Graciously handle the case where the thread wants to terminate itself.*/
void Scheduler::terminate(Thread * _thread) {
  if (Machine::interrupts_enabled()){
    Machine::disable_interrupts();
  }

  if(_thread == (Thread::CurrentThread())){
    yield(); 
  }

  if (!Machine::interrupts_enabled()){
    Machine::enable_interrupts();
  }
  Console::puts("Terminated Thread\n");
}

