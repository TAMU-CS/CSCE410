/*
 File: kernel.C
 
 Author: R. Bettati
 Department of Computer Science
 Texas A&M University
 Date  : 12/09/03
 
 
 This file has the main entry point to the operating system.
 
 */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H" /* LOW-LEVEL STUFF   */
#include "console.H"
#include "gdt.H"
#include "idt.H" /* LOW-LEVEL EXCEPTION MGMT. */
#include "irq.H"
#include "exceptions.H"
#include "interrupts.H"

#include "simple_keyboard.H" /* SIMPLE KB DRIVER */
#include "simple_timer.H"    /* TIMER MANAGEMENT */

#include "page_table.H"
#include "paging_low.H"
#include "utils.H"

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB *(0x1 << 20)
#define KB *(0x1 << 10)
#define KERNEL_POOL_START_FRAME ((2 MB) / Machine::PAGE_SIZE)
#define KERNEL_POOL_SIZE ((2 MB) / Machine::PAGE_SIZE)
#define PROCESS_POOL_START_FRAME ((4 MB) / Machine::PAGE_SIZE)
#define PROCESS_POOL_SIZE ((28 MB) / Machine::PAGE_SIZE)
/* definition of the kernel and process memory pools */

#define MEM_HOLE_START_FRAME ((15 MB) / Machine::PAGE_SIZE)
#define MEM_HOLE_SIZE ((1 MB) / Machine::PAGE_SIZE)
/* we have a 1 MB hole in physical memory starting at address 15 MB */

#define FAULT_ADDR (4 MB)
/* used in the code later as address referenced to cause page faults. */
#define NACCESS ((1 MB) / 4)
/* NACCESS integer access (i.e. 4 bytes in each access) are made starting at address FAULT_ADDR */

/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* FORWARD REFERENCES FOR P4 TEST CODE */
/*--------------------------------------------------------------------------*/

void TestPassed();
void TestFailed();

void GeneratePageTableMemoryReferences(unsigned long start_address, int n_references);
void GenerateVMPoolMemoryReferences(VMPool *pool, int size1, int size2);

int main()
{

  GDT::init();
  Console::init();
  IDT::init();
  ExceptionHandler::init_dispatcher();
  IRQ::init();
  InterruptHandler::init_dispatcher();

  /* -- EXAMPLE OF AN EXCEPTION HANDLER -- */

  class DBZ_Handler : public ExceptionHandler
  {
    /* We derive Division-by-Zero handler from ExceptionHandler 
         and overoad the method handle_exception. */
  public:
    virtual void handle_exception(REGS *_regs)
    {
      Console::puts("DIVISION BY ZERO!\n");
      for (;;)
        ;
    }
  } dbz_handler;

  /* Register the DBZ handler for exception no.0 
       with the exception dispatcher. */
  ExceptionHandler::register_handler(0, &dbz_handler);

  /* -- INITIALIZE THE TIMER (we use a very simple timer).-- */

  SimpleTimer timer(100); /* timer ticks every 10ms. */

  /* ---- Register timer handler for interrupt no.0 
            with the interrupt dispatcher. */
  InterruptHandler::register_handler(0, &timer);

  /* NOTE: The timer chip starts periodically firing as
     soon as we enable interrupts.
     It is important to install a timer handler, as we
     would get a lot of uncaptured interrupts otherwise. */

  /* -- INSTALL KEYBOARD HANDLER -- */
  SimpleKeyboard::init();

  Console::puts("after installing keyboard handler\n");

  /* -- ENABLE INTERRUPTS -- */

  Machine::enable_interrupts();

  /* -- INITIALIZE FRAME POOLS -- */

  ContFramePool kernel_mem_pool(KERNEL_POOL_START_FRAME,
                                KERNEL_POOL_SIZE,
                                0,
                                0);

  //kernel_mem_pool.print_bitmap();

  unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);

  unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);

  Console::puts("pmpif = ");
  Console::puti(process_mem_pool_info_frame);
  Console::puts("\n");
  ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                 PROCESS_POOL_SIZE,
                                 process_mem_pool_info_frame,
                                 n_info_frames);

  /* Take care of the hole in the memory. */
  process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);

  /* -- INITIALIZE MEMORY (PAGING) -- */

  /* ---- INSTALL PAGE FAULT HANDLER -- */

  class PageFault_Handler : public ExceptionHandler
  {
    /* We derive the page fault handler from ExceptionHandler 
           and overload the method handle_exception. */
  public:
    virtual void handle_exception(REGS *_regs)
    {
      PageTable::handle_fault(_regs);
    }
  } pagefault_handler;

  /* ---- Register the page fault handler for exception no.14 
            with the exception dispatcher. */
  ExceptionHandler::register_handler(14, &pagefault_handler);

  /* ---- INITIALIZE THE PAGE TABLE -- */

  PageTable::init_paging(&kernel_mem_pool,
                         &process_mem_pool,
                         4 MB);

  PageTable pt;

  pt.load();

  Console::puts("after pt.load\n");

  PageTable::enable_paging();

  Console::puts("after enable_paging\n");

  /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */

  Console::puts("Hello World! P4 part II test will start.\n");

  /* WE TEST JUST THE PAGE TABLE */
  GeneratePageTableMemoryReferences(FAULT_ADDR, NACCESS);
  //GeneratePageTableMemoryReferences(FAULT_ADDR, 128);

  TestPassed();

  /* -- STOP HERE */
  Console::puts("YOU CAN SAFELY TURN OFF THE MACHINE NOW.\n");
  for (;;)
    ;

  /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
  return 1;
}

void GeneratePageTableMemoryReferences(unsigned long start_address, int n_references)
{
  int *foo = (int *)start_address;

  for (int i = 0; i < n_references; i++)
  {
    foo[i] = i;
  }

  Console::puts("DONE WRITING TO MEMORY. Now testing...\n");

  for (int i = 0; i < n_references; i++)
  {
    if (foo[i] != i)
    {
      TestFailed();
    }
  }
}

void TestFailed()
{
  Console::puts("Test Failed\n");
  Console::puts("YOU CAN TURN OFF THE MACHINE NOW.\n");
  for (;;)
    ;
}

void TestPassed()
{
  Console::puts("Test Passed! Congratulations!\n");
  Console::puts("YOU CAN SAFELY TURN OFF THE MACHINE NOW.\n");
  for (;;)
    ;
}
