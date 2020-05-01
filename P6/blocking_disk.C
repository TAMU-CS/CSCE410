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

extern Scheduler *SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size)
    : SimpleDisk(_disk_id, _size)
{
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char *_buf)
{
  // while operation is not ready, yield the thread
  while (!SimpleDisk::is_ready())
  {
    SYSTEM_SCHEDULER->yield();
  }

  SimpleDisk::read(_block_no, _buf);
}

void BlockingDisk::write(unsigned long _block_no, unsigned char *_buf)
{
  // while operation is not ready, yield the thread
  while (!SimpleDisk::is_ready())
  {
    SYSTEM_SCHEDULER->yield();
  }

  SimpleDisk::write(_block_no, _buf);
}
