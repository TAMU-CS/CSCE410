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

#include "scheduler.H"      /* WE WILL NEED A SCHEDULER WITH BlockingDisk */

#include "simple_disk.H"    /* DISK DEVICE */
#include "blocking_disk.H"

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
/* SCHEDULER */
/*--------------------------------------------------------------------------*/

/* -- A POINTER TO THE SYSTEM SCHEDULER */
Scheduler * SYSTEM_SCHEDULER;


/*--------------------------------------------------------------------------*/
/* DISK */
/*--------------------------------------------------------------------------*/

/* -- A POINTER TO THE SYSTEM DISK */
SimpleDisk * SYSTEM_DISK;

#define SYSTEM_DISK_SIZE (10 MB)

#define DISK_BLOCK_SIZE ((1 KB) / 2)

/*--------------------------------------------------------------------------*/
/* JUST AN AUXILIARY FUNCTION */
/*--------------------------------------------------------------------------*/

void pass_on_CPU(Thread * _to_thread) {
    /* We use a scheduler. Instead of dispatching to the next thread,
       we pre-empt the current thread by putting it onto the ready
       queue and yielding the CPU. */
    
    SYSTEM_SCHEDULER->resume(Thread::CurrentThread()); 
    SYSTEM_SCHEDULER->yield();
}

/*--------------------------------------------------------------------------*/
/* A FEW THREADS (pointer to TCB's and thread functions) */
/*--------------------------------------------------------------------------*/

Thread * thread1;
Thread * thread2;
Thread * thread3;
Thread * thread4;

const unsigned int NB_ITERATIONS = 20;
    
void fun1() {
    Console::puts("THREAD: "); Console::puti(Thread::CurrentThread()->ThreadId()); Console::puts("\n");
    debug_out_E9_msg_value("THREAD: ", Thread::CurrentThread()->ThreadId());
    Console::puts("FUN 1 INVOKED! I WILL RUN FOREVER\n");
    debug_out_E9("FUN 1 INVOKED!  I WILL RUN FOREVER\n");
    
    for(int j = 0; ; j++) { // this thread is going to run forever

       Console::puts("FUN 1 IN ITERATION["); Console::puti(j); Console::puts("]\n");
       debug_out_E9_msg_value("FUN 1 IN ITERATION ", j);
       
       for (int i = 0; i < 10; i++) {
           Console::puts("FUN 1: TICK ["); Console::puti(i); Console::puts("]\n");
	   debug_out_E9_msg_value("FUN 1: TICK ", i);
       }

       pass_on_CPU(thread2);
    }
}

void fun2() {
    Console::puts("THREAD: "); Console::puti(Thread::CurrentThread()->ThreadId()); Console::puts("\n");
    debug_out_E9_msg_value("THREAD: ", Thread::CurrentThread()->ThreadId());
    Console::puts("FUN 2 INVOKED. I'M POWERFUL: I USE THE DISK!\n");
    debug_out_E9("FUN 2 INVOKED! I'M POWERFUL: I USE THE DISK\n");
    
    unsigned char* buf = new unsigned char[DISK_BLOCK_SIZE];
    int  read_block  = 1;
    int  write_block = 0;

    bool checking_first_write_read = true;

    for(unsigned int j = 0; j < NB_ITERATIONS; j++) {

       Console::puts("FUN 2 IN ITERATION["); Console::puti(j); Console::puts("]\n");
       debug_out_E9_msg_value("FUN 2 IN ITERATION ", j);
       
       /* -- Read */
       Console::puts("Reading a block from disk...\n");
       debug_out_E9("Reading a block from disk...\n");
       SYSTEM_DISK->read(read_block, buf);

       /* -- Display. Comment it out if you don't want all this data in the output file */
       Console::puts("Loop in FUN 2 will display the buf content in the output file.\nCheck there if you want to see it.\n");
       debug_out_E9("Displaying the data read from the disk\n");
       for (int i = 0; i < DISK_BLOCK_SIZE; i++) {
	   debug_out_E9_msg_value(" " , buf[i]);
       }
       debug_out_E9("\nEnd of buf\n");
       
       Console::puts("Writing a block to disk...\n");
       debug_out_E9("Writing a block to disk...\n");
       SYSTEM_DISK->write(write_block, buf);

       /* When we do our first write, we will check if we actually wrote  the data */
       if (checking_first_write_read) {
	   Console::puts("Reading the block we just wrote ...\n");
	   debug_out_E9("Reading the block we just wrote ...\n");
	   unsigned char* aux = new unsigned char[DISK_BLOCK_SIZE];
	   SYSTEM_DISK->read(write_block, aux);
	   for (int k = 0; k < DISK_BLOCK_SIZE; k++) {
	       if (aux[k] != buf[k]) {
		   debug_out_E9_msg_value("aux/buf comparison failed for k " , k);		   
		   Console::puts("aux/buf comparison failed for k ");
		   Console::puti(k);
		   Console::puts("\n");
		   assert(false);
	       }
	   }
	   delete aux;
	   Console::puts("Data matches! All is fine.\n");
	   debug_out_E9("Data matches! All is fine.\n");
	   checking_first_write_read = false;
       }

       /* -- Move to next block */
       write_block = read_block;
       read_block  = (read_block + 1) % 10;

       /* -- Give up the CPU */
       pass_on_CPU(thread3);
    }

    Console::puts("FUN 2 IS DONE!\n");
    debug_out_E9("FUN 2 IS DONE!\n");
    delete buf;
}

void fun3() {
    Console::puts("THREAD: "); Console::puti(Thread::CurrentThread()->ThreadId()); Console::puts("\n");
    debug_out_E9_msg_value("THREAD: ", Thread::CurrentThread()->ThreadId());
    Console::puts("FUN 3 INVOKED!\n");
    debug_out_E9("FUN 3 INVOKED!\n");

     for(unsigned int j = 0; j < NB_ITERATIONS; j++) {
       Console::puts("FUN 3 IN BURST["); Console::puti(j); Console::puts("]\n");
       debug_out_E9_msg_value("FUN 3 IN BURST ", j);
       for (int i = 0; i < 10; i++) {
           Console::puts("FUN 3: TICK ["); Console::puti(i); Console::puts("]\n");
	   debug_out_E9_msg_value("FUN 3: TICK ", i);
       }
    
       pass_on_CPU(thread4);
    }

     Console::puts("FUN 3 IS DONE!\n");
     debug_out_E9("FUN 3 IS DONE!\n");
}

void fun4() {
    Console::puts("THREAD: "); Console::puti(Thread::CurrentThread()->ThreadId()); Console::puts("\n");
    debug_out_E9_msg_value("THREAD: ", Thread::CurrentThread()->ThreadId());
    Console::puts("FUN 4 INVOKED!\n");
    debug_out_E9("FUN 4 INVOKED!\n");
    
    for(unsigned int j = 0; j < NB_ITERATIONS; j++) {

       Console::puts("FUN 4 IN BURST["); Console::puti(j); Console::puts("]\n");
       debug_out_E9_msg_value("FUN 4 IN BURST ", j);
       for (int i = 0; i < 10; i++) {
           Console::puts("FUN 4: TICK ["); Console::puti(i); Console::puts("]\n");
	   debug_out_E9_msg_value("FUN 4: TICK ", i);
       }

       pass_on_CPU(thread1);
    }

    Console::puts("FUN 4 IS DONE!\n");
    debug_out_E9("FUN 4 IS DONE!\n");
}

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

    /* -- INITIALIZE THE TIMER (we use a very simple timer).-- */

    /* Question: Why do we want a timer? We have it to make sure that 
                 we enable interrupts correctly. If we forget to do it,
                 the timer "dies". */

    SimpleTimer timer(100); /* timer ticks every 10ms. */
    InterruptHandler::register_handler(0, &timer);
    /* The Timer is implemented as an interrupt handler. */

    SYSTEM_SCHEDULER = new Scheduler();

    /* -- DISK DEVICE -- */

    SYSTEM_DISK = new SimpleDisk(MASTER, SYSTEM_DISK_SIZE);
   
    /* NOTE: The timer chip starts periodically firing as 
             soon as we enable interrupts.
             It is important to install a timer handler, as we 
             would get a lot of uncaptured interrupts otherwise. */  

    /* -- ENABLE INTERRUPTS -- */

     Machine::enable_interrupts();

    /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */

    Console::puts("Hello World!\n");


    /* -- LET'S CREATE SOME THREADS... */
    Console::puts("Only thread 1 will run forever\n");
    debug_out_E9("Only thread 1 will run forever\n");
		 
    Console::puts("CREATING THREAD 1...\n");
    char * stack1 = new char[1024];
    thread1 = new Thread(fun1, stack1, 1024);
    Console::puts("DONE\n");
    debug_out_E9_msg_value("First thread created ", (unsigned long) thread1);
    
    Console::puts("CREATING THREAD 1...");
    char * stack2 = new char[1024];
    thread2 = new Thread(fun2, stack2, 1024);
    Console::puts("DONE\n");
    debug_out_E9_msg_value("Second thread created ", (unsigned long)  thread2);
    
    Console::puts("CREATING THREAD 2...");
    char * stack3 = new char[1024];
    thread3 = new Thread(fun3, stack3, 1024);
    Console::puts("DONE\n");
    debug_out_E9_msg_value("Third thread created ", (unsigned long) thread3);
    
    Console::puts("CREATING THREAD 3...");
    char * stack4 = new char[1024];
    thread4 = new Thread(fun4, stack4, 1024);
    Console::puts("DONE\n");
    debug_out_E9_msg_value("Fourth thread created ", (unsigned long)  thread4);
    
    SYSTEM_SCHEDULER->add(thread2);
    SYSTEM_SCHEDULER->add(thread3);
    SYSTEM_SCHEDULER->add(thread4);

    /* -- KICK-OFF THREAD1 ... */

    Console::puts("STARTING THREAD 1 ...\n");
    Thread::dispatch_to(thread1);

    /* -- AND ALL THE REST SHOULD FOLLOW ... */
 
    assert(false); /* WE SHOULD NEVER REACH THIS POINT. */

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}
