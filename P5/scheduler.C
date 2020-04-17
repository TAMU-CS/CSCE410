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

Scheduler::Scheduler()
{
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield()
{
  Machine::disable_interrupts();
  Thread *first = NULL;
  if (ready.size() > 0)
  {
    first = ready.pop();
  }
  else
  {
    Console::puts("ready queue is empty!\n");
    return;
  }

  Machine::enable_interrupts();
  Thread::dispatch_to(first);
}

void Scheduler::resume(Thread *_thread)
{
  Machine::disable_interrupts();
  ready.push(_thread);
  Machine::enable_interrupts();
}

void Scheduler::add(Thread *_thread)
{
  resume(_thread);
}

void Scheduler::terminate(Thread *_thread)
{
  Machine::disable_interrupts();

  if (_thread != Thread::CurrentThread())
  { // terminating a thread that is not currently running

    // attempt to remove it from the ready queue
    ready.delete_thread(_thread);
  }

  delete _thread;
  Machine::enable_interrupts();
  yield();
}
