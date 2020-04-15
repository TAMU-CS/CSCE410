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

struct Node
{
  Thread *thread;
  Node *next;
  Node *prev;
};

class Queue
{
private:
  int size;
  Node *head;
  Node *tail;

public:
  Queue()
  {
    head = new Node;
    tail = new Node;
    head->next = tail;
    size = 0;
  }

  void push(Thread *thread)
  {
    size++;

    Node *temp = new Node;
    temp->thread = thread;
    temp->next = tail;
    temp->prev = tail->prev;

    tail->prev->next = temp;
    tail->prev = temp;
  }
  Thread *pop()
  {
    size--;

    Node *temp = head->next;
    head->next = temp->next;
    temp->next->prev = head;

    Thread *thread = temp->thread;
    delete temp;
    return thread;
  }

  int size()
  {
    return size;
  }
};

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
  running = NULL;

  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield()
{
  assert(false);
}

void Scheduler::resume(Thread *_thread)
{
  assert(false);
}

void Scheduler::add(Thread *_thread)
{
  assert(false);
}

void Scheduler::terminate(Thread *_thread)
{
  assert(false);
}
