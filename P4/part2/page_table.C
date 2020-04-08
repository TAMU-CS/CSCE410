#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable *PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool *PageTable::kernel_mem_pool = NULL;
ContFramePool *PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;

void PageTable::init_paging(ContFramePool *_kernel_mem_pool,
                            ContFramePool *_process_mem_pool,
                            const unsigned long _shared_size)
{
    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;
    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
    head = NULL;

    // page directory frame allocation
    unsigned int pd_frame_num = (unsigned int)kernel_mem_pool->get_frames(1);
    page_directory = (unsigned long *)(pd_frame_num * PAGE_SIZE);

    // first page table: do direct mapping for first 4MB
    unsigned int pt_frame_num = (unsigned int)kernel_mem_pool->get_frames(1);
    unsigned long *page_table = (unsigned long *)(pt_frame_num * PAGE_SIZE);
    for (unsigned int frame_num = 0; frame_num < ENTRIES_PER_PAGE; frame_num++)
    {
        // mark address for first 20 bits, and set last 12 metadata bits
        // 3: ...011 |supervisor|r/w|present|
        page_table[frame_num] = (frame_num << 12) + 3;
    }

    // set page table address
    // 3: ...011 |supervisor|r/w|present|
    page_directory[0] = (unsigned long)page_table | 3;

    // fill out last 1023 page tables
    for (unsigned int page_table_num = 1; page_table_num < ENTRIES_PER_PAGE; page_table_num++)
    {
        // 2: ...010 |supervisor|r/w|present|
        page_directory[page_table_num] = 2;
    }

    Console::puts("Constructed Page Table object\n");
}

void PageTable::load()
{
    current_page_table = this;
    write_cr3((unsigned long)page_directory);
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
    // set paging bit to 1
    paging_enabled = 1;
    write_cr0(read_cr0() | (1 << 31));
    Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS *_r)
{
    unsigned long *page_directory = current_page_table->page_directory;
    unsigned long access_addr = read_cr2();
    unsigned long p1 = access_addr >> 22;
    unsigned long p2 = (access_addr & 0x003FFFFF) >> 12;

    if (!current_page_table->check_address(access_addr))
    {
        Console::puts("check_address failed for address: ");
        Console::putui(access_addr);
        Console::puts(" (this in hexa ");
        char hexastr[9];
        ulong2hexstr(access_addr, hexastr);
        Console::puts(hexastr);
        Console::puts(")\n");
        abort();
    }

    if (!(page_directory[p1] & 1))
    {
        // allocate frame for page table
        unsigned int pt_frame_num = (unsigned int)kernel_mem_pool->get_frames(1);
        unsigned long *page_table = (unsigned long *)(pt_frame_num * PAGE_SIZE);
        for (unsigned int frame_num = 0; frame_num < ENTRIES_PER_PAGE; frame_num++)
        {
            // 6: ...110 |supervisor|r/w|present|
            page_table[frame_num] = 6;
        }

        // 3: ...011 |supervisor|r/w|present|
        page_directory[p1] = (pt_frame_num << 12) | 3;
    }

    // allocate single page in page table
    unsigned int page_table_frame = page_directory[p1] >> 12;
    unsigned long *page_table = (unsigned long *)(page_table_frame << 12);
    unsigned int frame_num = (unsigned int)process_mem_pool->get_frames(1);

    page_table[p2] = (frame_num << 12) | 7;
    Console::puts("handled page fault\n");
}

bool PageTable::check_address(unsigned long address)
{
    VMPool *cur = head;

    while (cur != NULL)
    {
        if (cur->is_legitimate(address))
        {
            return true;
        }

        cur = cur->next;
    }

    return false;
}

void PageTable::register_pool(VMPool *_vm_pool)
{

    if (head != NULL)
    {
        _vm_pool->next = head;
    }

    head = _vm_pool;
    Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no)
{
    unsigned long *page_directory = current_page_table->page_directory;
    unsigned long p1 = _page_no >> 22;
    unsigned long p2 = (_page_no & 0x003FFFFF) >> 12;

    unsigned long *page_table = (unsigned long *)page_directory[p1];
    if (!page_table[p2] & 1)
    {
        return;
    }

    process_mem_pool->release_frames(page_table[p2] >> 12);

    // 6: ...110 |supervisor|r/w|present|
    page_table[p2] = 6;

    // flush TLB
    write_cr3((unsigned long)page_directory);
}
