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

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/
const unsigned char bit_11 = 3;
const unsigned char bit_10 = 2;

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool * ContFramePool::head;
ContFramePool * ContFramePool::tail;

//  Constructor: Initialize all frames to FREE, except for any frames that you 
//  need for the management of the frame pool, if any.
ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    // Bitmap must fit in a single frame!

    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;

    // If _info_frame_no is zero then we keep management info in the first
    // frame, else we use the provided frame to keep management info (simple_frame.c)
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
    
    //  Mark all the frame as free. (simple_frame.c)
    for(int frameNum = base_frame_no; frameNum < base_frame_no + nframes; frameNum++) {
        set_state(frameNum, FrameState::Free);
    }

    // Mark first frames as used as info frame number isn't given (simple_frame.c)
    if(_info_frame_no == 0) {
        set_state(base_frame_no, FrameState::Used);
        nFreeFrames--;
    }

    // Linked list implementation (Singly linked list is implemented for now)
    
    if(head == NULL){ // Set the head if the frame pool list is not init yet
        head = this;
        tail = this;
    }else{
        tail->next = this; // Tail is the end of the frame pool list
        tail = this;
    }


    Console::puts("Initialized the frame Pool successfully\n");
    assert(true);
}


//  get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
//  sequence of at least _n_frames entries that are FREE. If you find one, 
//  mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
//  ALLOCATED.
unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    unsigned int frame_index = base_frame_no;
    bool contigBlockFound = false;
    unsigned int frameCount = 0;

    for(int fno = base_frame_no;fno < base_frame_no + nframes; fno++){

        if(get_state(fno) == FrameState::Free){
            frameCount++;
        }else{
            frameCount = 0;
            frame_index = fno+1;
        }

        if(frameCount == _n_frames){
            contigBlockFound = true;
            break;
        }
    }

    if(contigBlockFound){
        set_state(frame_index, FrameState::HoS);
        nFreeFrames--;

        for(int fno = frame_index+1;fno < frame_index+_n_frames;fno++){
            set_state(fno, FrameState::Used);
            nFreeFrames--;
        }
        // Console::puts("Frame no:::") ;Console::puti(frame_index); Console::puts() Console::puti((int)get_state(frame_index));Console::puts(":::::::::::::::::::::::");
        return frame_index;
    }else{
        Console::puts("No free frames found");
    }
    return 0;
    
}


//  mark_inaccessible(_base_frame_no, _n_frames): This is no different than
//  get_frames, without having to search for the free sequence. You tell the
//  allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
//  frames after that to mark as ALLOCATED.
void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    set_state(_base_frame_no, FrameState::HoS);
    for(int fno = _base_frame_no+1; fno < _base_frame_no + _n_frames; fno++){
        set_state(fno, FrameState::Used);
    }
}

//  release_frames(_first_frame_no): Check whether the first frame is marked as
//  HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
//  Traverse the subsequent frames until you reach one that is FREE or 
//  HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
void ContFramePool::release_frames(unsigned long _first_frame_no) 
{
    ContFramePool * currPool = ContFramePool::head;

    // Check if frame is within any of the pools
    while(~(_first_frame_no < currPool->base_frame_no+currPool->nframes && _first_frame_no >= currPool->base_frame_no) && currPool != NULL){
        currPool = currPool->next;
    }

    if(currPool->get_state(_first_frame_no) == FrameState::HoS){
        currPool->set_state(_first_frame_no, FrameState::Free);
        currPool->nFreeFrames++;
        for(int fno=_first_frame_no+1;currPool->get_state(fno) == FrameState::Used;fno++){
            currPool->set_state(fno, FrameState::Free);
            currPool->nFreeFrames++;
        }
    }
    return;
}


//  needed_info_frames(_n_frames): This depends on how many bits you need 
//  to store the state of each frame. If you use a char to represent the state
//  of a frame, then you need one info frame for each FRAME_SIZE frames.
unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    // Return 1 or 0 according to condition
    return ((_n_frames/8 + _n_frames%8) > 0);
}

// get_state(unsigned long _frame_no)
// Get the state of the frame stored in a bitmap
ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no) {
    unsigned int bitmap_idx = _frame_no / 4;
    unsigned int frame_idx = _frame_no % 4;

    // two bit frame shift
    unsigned char two_bit_fshift = 2 * frame_idx;

    if(((bitmap[bitmap_idx] >> two_bit_fshift) & bit_11) == 0){
        return FrameState::Free;
    }

    if(((bitmap[bitmap_idx] >> two_bit_fshift) & bit_10) == bit_10){
        return FrameState::HoS;
    }

    if(((bitmap[bitmap_idx] >> two_bit_fshift) & bit_11) == bit_11){
        return FrameState::Used;
    }
    return FrameState::Free;
}

// set_state(long _frame_no, _state)
// Set the state of the frame stored in a bitmap
void ContFramePool::set_state(unsigned long _frame_no, FrameState _state) {
    unsigned int bitmap_idx = _frame_no / 4;
    unsigned int frame_idx = _frame_no % 4;

    // two bit frame shift
    unsigned char two_bit_fshift = 2 * frame_idx;

    unsigned char used_mask = bit_11 << two_bit_fshift;
    unsigned char free_mask = ~(used_mask);
    unsigned char hos_mask = bit_10 << two_bit_fshift;


    switch(_state) {
        case FrameState::Free:
            bitmap[bitmap_idx] &= free_mask;
            break;
        case FrameState::Used:
            bitmap[bitmap_idx] |= used_mask;
            break;
        case FrameState::HoS:
            bitmap[bitmap_idx] |= hos_mask;
            break;

    }
    
    // Check if the right state is getting set
    // Console::puts("get_state: ");Console::puti((int)get_state(_frame_no)); Console::puts("::Frame no::"); Console::puti(_frame_no);
    // for(int i = 0;i < 100000;i++){
    //             Console::puts("");
    //         }
    //         Console::puts("\n\n");
    
}