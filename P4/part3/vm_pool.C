/*
 File: vm_pool.C
 
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

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

void VMPool::shift_regions(int start_region, int offset)
{
    // get last region in list
    int last_region = 0;
    for (int j = start_region; j < MAX_REGION_LIST_SIZE; j++)
    {
        if (regions[j * 2 + 1] == 0)
        { // this is an empty region (last region)
            last_region = j;
            break;
        }
    }
    if (offset > 0)
    { // shifting [regi, regi+1, regi+2,...] right by offset
        for (int i = last_region; i >= start_region; i--)
        { // for all regions starting at the last_region to the start_region
            regions[(i + offset) * 2] = regions[i * 2];
            regions[(i + offset) * 2 + 1] = regions[i * 2 + 1];
        }
    }
    else
    { // shifting [regi, ...] left by offset
        for (int i = start_region; i <= last_region; i++)
        { // for all regions starting at the start_region to the last_region
            regions[(i + offset) * 2] = regions[i * 2];
            regions[(i + offset) * 2 + 1] = regions[i * 2 + 1];
        }
    }
}

VMPool::VMPool(unsigned long _base_address,
               unsigned long _size,
               ContFramePool *_frame_pool,
               PageTable *_page_table)
{
    base_address = _base_address;
    size = _size / Machine::PAGE_SIZE;
    page_table = _page_table;
    frame_pool = _frame_pool;
    page_table->register_pool(this);

    // initialize regions and create first region that holds *regions list
    regions = (unsigned long *)_base_address;
    regions[0] = _base_address;
    regions[1] = Machine::PAGE_SIZE;

    for (int i = 1; i < MAX_REGION_LIST_SIZE; i++)
    { // for all regions after first set size to 0, and addr to 0
        regions[i * 2] = 0;
        regions[i * 2 + 1] = 0;
    }
}

unsigned long VMPool::allocate(unsigned long _size)
{ // first fit algorithm

    // convert size from number of bytes to number of frames
    _size = _size / Machine::PAGE_SIZE + (_size % Machine::PAGE_SIZE == 0 ? 0 : 1);

    int i = 0;
    for (; regions[i * 2 + 1] != 0 && i < MAX_REGION_LIST_SIZE - 1; i++)
    { // for all sections between two consecutive regions
        unsigned long region_1_base_addr = regions[i * 2];
        unsigned long region_1_size = regions[i * 2 + 1];
        unsigned long region_2_base_addr = regions[(i + 1) * 2];

        unsigned long size_between = region_2_base_addr - (region_1_base_addr + region_1_size);

        if (_size <= size_between)
        { // _size fits inbetween two allocated regions
            // create region with:
            // base address = region_1_base_addr + region_1_size
            // size = _size

            // shift regions after i to the right by 1
            shift_regions(i + 1, 1);

            regions[(i + 1) * 2] = region_1_base_addr + region_1_size;
            regions[(i + 1) * 2 + 1] = _size * Machine::PAGE_SIZE;

            Console::puts("Allocated region of memory.\n");
            return regions[(i + 1) * 2];
        }
    }

    // allocate for region i, addr = addr right after last region
    regions[i * 2] = regions[(i - 1) * 2] + regions[(i - 1) * 2 + 1];
    regions[i * 2 + 1] = _size * Machine::PAGE_SIZE;

    Console::puts("Allocated region of memory.\n");
    return regions[i * 2];
}

void VMPool::release(unsigned long _start_address)
{
    Console::puts("Released region of memory.\n");
    for (int i = 0; i < MAX_REGION_LIST_SIZE; i++)
    { // for all regions
        unsigned long region_base_addr = regions[i * 2];
        unsigned long region_size = regions[i * 2 + 1];

        if (region_base_addr == _start_address)
        { // region matches start address

            unsigned long num_frames = region_size >> 12;
            for (int j = 0; j < num_frames; j++)
            { // for each frame, free the frame
                unsigned long frame_no = region_base_addr + (j << 12);
                page_table->free_page(frame_no);
            }

            // destroy the region
            shift_regions(i + 1, -1);
            return;
        }
    }
    return;
}

bool VMPool::is_legitimate(unsigned long _address)
{
    Console::puts("Checked whether address is part of an allocated region.\n");

    if (_address == base_address)
    { // first frame is legit by default
        return true;
    }

    for (int i = 0; i < MAX_REGION_LIST_SIZE; i++)
    { // for all regions:
        unsigned long region_base_addr = regions[i * 2];
        unsigned long region_size = regions[i * 2 + 1];

        if (region_base_addr <= _address && (region_base_addr + region_size) > _address)
        { // _address is in this region
            return true;
        }
    }

    return false;
}
