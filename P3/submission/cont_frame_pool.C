/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

//initiate head
ContFramePool *ContFramePool::head = NULL;

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

//private helper methods
void ContFramePool::setFrameBitMask(unsigned long _frame, unsigned char val)
{
    //get frame location
    unsigned long ind = _frame / 4;
    unsigned long offset = 6 - 2 * (_frame % 4);

    //change first bit
    if ((val >> 1) & 1)
    {
        bitmap[ind] |= 1 << (offset + 1);
    }
    else
    {
        bitmap[ind] &= ~(1 << (offset + 1));
    }

    //change second bit
    if (val & 1)
    {
        bitmap[ind] |= 1 << offset;
    }
    else
    {
        bitmap[ind] &= ~(1 << offset);
    }
}

void ContFramePool::markContFrameMasks(unsigned long _base, unsigned long _n_frames, unsigned char val)
{
    for (int i = _base; i < _base + _n_frames; i++)
    {
        setFrameBitMask(i, val);
    }
}

bool ContFramePool::frameCompare(unsigned long _frame, unsigned char val)
{
    //get value of frame
    unsigned long ind = _frame / 4;
    unsigned long offset = 6 - 2 * (_frame % 4);
    return (((bitmap[ind] >> offset) & 0x03) ^ (val)) == 0;
}

void ContFramePool::printBitMask()
{

    Console::puts("bitmask: ");
    for (int i = 0; i < n_frames; i++)
    {
        //get value of frame
        unsigned long ind = i / 4;
        unsigned long offset = 6 - 2 * (i % 4);

        //print frame;
        Console::puts("(");
        Console::puti((bitmap[ind] >> offset + 1) & 1);
        Console::puti((bitmap[ind] >> offset) & 1);
        Console::puts(")");
    }
    Console::puts("\n");
}

//public methods
ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    //make sure bitmap can fit in a single frame
    //frame size is 4096 => 4096 bytes/bits * 8 bits gives us total number of frames
    //one frame can handle, assuming there is two bits per frame (free,allocated,headofsequence) however we need another step:
    //4096 bytes/bits * 8 bits / 2
    //now we need to consider the number of info frames
    if (_info_frame_no == 0 && _n_info_frames == 0)
        _n_info_frames = needed_info_frames(_n_frames);
    assert(_n_frames <= FRAME_SIZE * 4 * _n_info_frames);

    base_frame_no = _base_frame_no;
    n_frames = _n_frames;
    info_frame_no = _info_frame_no;
    n_info_frames = _n_info_frames;

    //if Info_frame_no is 0 the frame pool is free to choose
    //any frames from the pool to store management information
    if (info_frame_no == 0)
    {
        //bitmap is internal so choose first one!
        bitmap = (unsigned char *)(base_frame_no * FRAME_SIZE);
    }
    else
    {
        bitmap = (unsigned char *)(info_frame_no * FRAME_SIZE); //bitmap is external!
    }

    //make sure that number of frames fill the bitmap
    //(4 (2-bit FrameDescription) per character (8 bits))
    assert((_n_frames % 4) == 0);

    //mark all bits in bitmap as open
    markContFrameMasks(0, n_frames, 0b11);

    //mark the first n_info_frames as used if info_frame_no is 0
    //ex: info_frame_no = 0, and n_info_frames = 3, then the next
    //3 frames including the first frame will be allocated!
    //11- FREE, 10-HEAD, 00-ALLOCATED
    if (info_frame_no == 0)
    {
        markContFrameMasks(0, n_info_frames, 0b00);
    }

    //set head if head is null
    if (ContFramePool::head == NULL)
    {
        ContFramePool::head = this;
    }
    else
    {
        //insert this right after head
        next = ContFramePool::head->next;
        ContFramePool::head->next = this;
    }
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    //make sure input is acceptable
    assert(_n_frames > 0);

    //keep track of the last head address and count to search for possible address locations
    unsigned long lastHead = 0;
    unsigned long count = 0;
    bool found = false;

    //iterate through the bitmap and search for _n_frames exist contiguously:
    //bitmap is char array, which is 1 byte per element. So 4 frames are represented
    //per single character element (8 bits=  4 frames * 2bit)
    //we need to iterate by 2 bits everytime
    for (unsigned long i = 0; i < n_frames; i++)
    {

        //if frame i is free:
        if (frameCompare(i, 0b11))
        {
            count++;

            //check if count reaches desired goal
            if (count >= _n_frames)
            {
                found = true;
                break;
            }
        }
        //frame i is NOT free:
        else
        {
            //set the head to the next frame
            lastHead = i + 1;
            count = 0;
        }
    }

    //make sure a frame was found
    assert(found);

    //mark all the frames as allocated
    //set head value:
    setFrameBitMask(lastHead, 0b10);

    //starting at lastHead + 1, iterate and set bits to 00 (allocated)
    markContFrameMasks(lastHead + 1, _n_frames - 1, 0b00);

    return lastHead + base_frame_no;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    //make sure _n_frames is proper
    assert(_n_frames > 0);

    //start from _base_frame_no and calculate bit map indexing
    markContFrameMasks(_base_frame_no - base_frame_no, _n_frames, 0b01);
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    //iterate through frame pools to find if _first_frame_no is inside a pool
    ContFramePool *temp = ContFramePool::head;
    bool found = false;
    while (temp)
    {

        //check if frame num is in range
        if (temp->base_frame_no <= _first_frame_no &&
            temp->base_frame_no + temp->n_frames > _first_frame_no)
        {
            found = true;
            break;
        }
        temp = temp->next;
    }
    assert(found);

    //make sure current frame is a head
    unsigned char *bitmap = temp->bitmap;
    unsigned long frame_no = _first_frame_no - temp->base_frame_no;
    assert(temp->frameCompare(frame_no, 0b10));

    //set the head to unallocated (11)
    temp->setFrameBitMask(frame_no, 0b11);

    //release frame until another head is reached or an unallocated frame is reached
    unsigned long cur = frame_no + 1;
    while (cur < temp->n_frames)
    {

        //check if frame is 00
        if (temp->frameCompare(cur, 0b00))
        {
            //set to free
            temp->setFrameBitMask(cur, 0b11);
            cur++;
        }
        else
        {
            //exit for loop
            break;
        }
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    //return the number of frames needed to manage a frame pool of size n_frames
    //frame size: FRAME_SIZE
    //bit per frame: 2
    //bit per byte : 8
    return _n_frames / (FRAME_SIZE * 4) + (_n_frames % (FRAME_SIZE * 4) > 0 ? 1 : 0); // we are rounding up
}
