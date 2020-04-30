/* 
    File: kernel.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 2017/05/02


    This file has the main entry point to the operating system.

    MAIN FILE FOR MACHINE PROBLEM "KERNEL-LEVEL DEVICE MANAGEMENT"

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- COMMENT/UNCOMMENT THE FOLLOWING LINE TO EXCLUDE/INCLUDE SCHEDULER CODE */

//#define _USES_SCHEDULER_
/* This macro is defined when we want to force the code below to use 
   a scheduler.
   Otherwise, no scheduler is used, and the threads pass control to each 
   other in a co-routine fashion.
*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"         /* LOW-LEVEL STUFF   */
#include "console.H"
#include "gdt.H"
#include "idt.H"             /* EXCEPTION MGMT.   */
#include "irq.H"
#include "exceptions.H"     
#include "interrupts.H"

#include "simple_timer.H"    /* TIMER MANAGEMENT  */

#include "frame_pool.H"      /* MEMORY MANAGEMENT */
#include "mem_pool.H"

#include "thread.H"         /* THREAD MANAGEMENT */

#include "simple_disk.H"    /* DISK DEVICE */
                            /* YOU MAY NEED TO INCLUDE blocking_disk.H
/*--------------------------------------------------------------------------*/
/* MEMORY MANAGEMENT */
/*--------------------------------------------------------------------------*/

/* -- A POOL OF FRAMES FOR THE SYSTEM TO USE */
FramePool * SYSTEM_FRAME_POOL;

/* -- A POOL OF CONTIGUOUS MEMORY FOR THE SYSTEM TO USE */
MemPool * MEMORY_POOL;

typedef unsigned int size_t;

//replace the operator "new"
void * operator new (size_t size) {
    unsigned long a = MEMORY_POOL->allocate((unsigned long)size);
    return (void *)a;
}

//replace the operator "new[]"
void * operator new[] (size_t size) {
    unsigned long a = MEMORY_POOL->allocate((unsigned long)size);
    return (void *)a;
}

//replace the operator "delete"
void operator delete (void * p) {
    MEMORY_POOL->release((unsigned long)p);
}

//replace the operator "delete[]"
void operator delete[] (void * p) {
    MEMORY_POOL->release((unsigned long)p);
}

/*--------------------------------------------------------------------------*/
/* DISK */
/*--------------------------------------------------------------------------*/

/* -- A POINTER TO THE SYSTEM DISK */
SimpleDisk * SYSTEM_DISK;
/* -- A POINTER TO THE SLAVE DISK */
SimpleDisk * SLAVE_DISK;

#define SYSTEM_DISK_SIZE (10 MB)

#define DISK_BLOCK_SIZE ((1 KB) / 2)

/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

int main() {

    GDT::init();
    Console::init();
    IDT::init();
    ExceptionHandler::init_dispatcher();
    IRQ::init();
    InterruptHandler::init_dispatcher();

    /* -- EXAMPLE OF AN EXCEPTION HANDLER -- */

    class DBZ_Handler : public ExceptionHandler {
      public:
      virtual void handle_exception(REGS * _regs) {
        Console::puts("DIVISION BY ZERO!\n");
        for(;;);
      }
    } dbz_handler;

    ExceptionHandler::register_handler(0, &dbz_handler);

    /* -- INITIALIZE MEMORY -- */
    /*    NOTE: We don't have paging enabled in this MP. */
    /*    NOTE2: This is not an exercise in memory management. The implementation
                of the memory management is accordingly *very* primitive! */

    /* ---- Initialize a frame pool; details are in its implementation */
    FramePool system_frame_pool;
    SYSTEM_FRAME_POOL = &system_frame_pool;
   
    /* ---- Create a memory pool of 256 frames. */
    MemPool memory_pool(SYSTEM_FRAME_POOL, 256);
    MEMORY_POOL = &memory_pool;

    /* -- MEMORY ALLOCATOR SET UP. WE CAN NOW USE NEW/DELETE! -- */

    /* We are not even enabling interrupts, because SimpleDisk does not need interrupts.
     * When you do your BlockingDisk, you will need to have the interrupts (as we have in kernel.C)
     */
    
    /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */
    
    Console::puts("Hello World!\n");
    

    /* We declare the master disk, but this simple example is not going to use it.
    ** We will write to the second disk only */
    SYSTEM_DISK = new SimpleDisk(MASTER, SYSTEM_DISK_SIZE);
    SLAVE_DISK = new SimpleDisk(SLAVE, SYSTEM_DISK_SIZE);
    
    unsigned char write_buf[DISK_BLOCK_SIZE];
    unsigned char read_buf[DISK_BLOCK_SIZE];
    for (int i =0; i < DISK_BLOCK_SIZE; i++) {
	write_buf[i] = '*';
    }
    Console::puts("It is going to write to block number 1 of the slave disk\n");
    SLAVE_DISK->write(1, write_buf); // using the second block on the disk, it could be any

    /* Now let's read the block and check it has the data we put there */
    Console::puts("It is going to read to block number 1 of the slave disk\n");
    SLAVE_DISK->read(1, read_buf);
    for (int i =0; i < DISK_BLOCK_SIZE; i++) {
	if (write_buf[i] != read_buf[i]) {
	    Console::puts("PROBLEM: byte read is not what we wrote for i ");
	    Console::puti(i);
	    Console::puts("\n");
	    abort();
	}
    }
	    
    Console::puts("Usage of SimpleDisk:read and SimpleDisk::write worked\n");
    Console::puts("System is going to be busy doing nothing forever\n");
    for(;;);

    return 1; // never reached, just making the compiler happy with our program
}
