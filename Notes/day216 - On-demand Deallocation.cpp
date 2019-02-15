Handmade Hero Day 216 - On-demand Deallocation

Summary:
picked up where we left off on day 215.

implemented on-demand deallocation for debug frames.

this is essentially the same concept as the memory deallocation for the asset system,
essentially whenever we request for new memory in the debug memory arena, we deallocate some 
to fullfill that request
[I think Casey_s implementation of PushSizeWithDeallocation(); doesnt work....]

briefly explained what memory alignment is the Q/A

Keyword:
debug system, memory


26:21
So Casey wants to introduce the tech of inspecting variables values over frames (to help with debugging);
[essentially visual studio_s debugger watch window by frames?]

Casey introduces the concept of Debug Element. He also introduces its relationship with 
Debug Event and Debug UI


Previously, our concept of debug element were derived from debug events.
for example, if you have a DEBUG_CAMERA_DISTANCE that we are toggling,
we are actually directly editing a debug_event that contains this DEBUG_CAMERA_DISTANCE value 

Casey now wants to have a debug element that is permenant instead of a transient debug_event


so what we actually want is like some sort of a table 


         ___________________________________________________________
        |           |           |           |           |           |
        |           |  frame 0  |  frame 1  |  frame 2  |  frame 3  |
        |___________|___________|___________|___________|___________|
        |           |           |           |           |           |
        | element 0 |           |           |           |           |
        |___________|___________|___________|___________|___________|
        |           |           |           |           |           |
        | element 1 |           |           |           |           |
        |___________|___________|___________|___________|___________|
        |           |           |           |           |           |    
        | element 2 |           |           |           |           |
        |___________|___________|___________|___________|___________|
        |           |           |           |           |           |
        | element 3 |           |           |           |           |
        |___________|___________|___________|___________|___________|


[how is debug element different from debug_variable that we deleted?]



36:26
Casey debating to himself whether we should have debug_table keep a giant 2D array of debug_events vs have
debug_table just have a temporary buffer and copying all the debug events out to a separate storage 

Casey prefers the copy out method even though it is less efficient since it doubles our memory traffic. 
the reason he argues for this is becuz he thinks we can easily get into situation where we want to store a lot of 
previous values for somethings, and not many previous values for other things  

meaning there will be only specific variables that we care about its previous frames values.

for example, performance counter, we want a longer history


So Casey will go with the copy out method.






41:30
Casey coming back to the FreeFrame(); function.

Casey things the best time to call FreeFrame(); is when inside NewFrame(); when we run out of 
memory from the debug system memory arena, we will call FreeFrame();.

So its essentially on demand Deallocation. kind of like the asset system.




46:53
so now if the memory_arena is full, we will can free memory on demand. 

Casey also mentions that if we want to do this more generally, we can try to put some concept of a call back in the 
memory_arena struct. 

                handmade.h

                struct memory_arena
                {
                    memory_index Size;
                    uint8 *Base;
                    memory_index Used;

                    int32 TempCount;
                };

but Casey wont do that for now. Casey wants to do it more explicitly

48:51
Casey added a few more functions to help determine when we ran out of memory in a memory arena

                handmade.h 
    
                inline memory_index GetEffectiveSizeFor(memory_arena *Arena, memory_index SizeInit, memory_index Alignment)
                {
                    memory_index Size = SizeInit;
                        
                    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
                    Size += AlignmentOffset;

                    return(Size);
                }

                inline b32 ArenaHasRoomFor(memory_arena *Arena, memory_index SizeInit, memory_index Alignment = DEFAULT_MEMORY_ALIGNMENT)
                {
                    memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Alignment);
                    b32 Result = ((Arena->Used + Size) <= Arena->Size);
                    return(Result);
                }





51:35
here Casey added new #defines to get memory when our memory arena runs out of memory
most of time it will just be one frame.

Casey added a check for the edge case where the debug_state.OldestFrame is also the debug_state.MostRecentFrame.
so Casey added 

                debug_frame *FrameToFree = DebugState->OldestFrame;
                ...
                if(DebugState->MostRecentFrame == FrameToFree)
                {
                    ...
                }


-   full code below:

                handmade_debug.cpp

                #define DebugPushStruct(DebugState, type, ...) (type *)PushSizeWithDeallocation(DebugState, sizeof(type), ## __VA_ARGS__)
                #define DebugPushCopy(DebugState, Size, Source, ...) Copy(Size, Source, PushSizeWithDeallocation(DebugState, Size, ## __VA_ARGS__))
                #define DebugPushArray(DebugState, Count, type, ...) (type *)PushSizeWithDeallocation(DebugState, (Count)*sizeof(type), ## __VA_ARGS__)

                inline void * PushSizeWithDeallocation(debug_state *DebugState, memory_index Size, memory_index Alignment = DEFAULT_MEMORY_ALIGNMENT)
                {
                    while(!ArenaHasRoomFor(&DebugState->DebugArena, Size, Alignment) && DebugState->OldestFrame)
                    {
                        debug_frame *FrameToFree = DebugState->OldestFrame;
                        DebugState->OldestFrame = DebugState->OldestFrame->Next;
                        if(DebugState->MostRecentFrame == FrameToFree)
                        {
                            DebugState->MostRecentFrame = DebugState->MostRecentFrame->Next;
                        }
                        FreeFrame(DebugState, FrameToFree);
                    }

                    void *Result = PushSize_(&DebugState->DebugArena, Size, Alignment);
                    return(Result);
                }

[does this actually work? If your memory is out of space, we are only putting more stuff on the freelist, but you are 
    still calling PushSize_();... so whats going on?]


so now everywhere, we will be calling FREELIST_ALLOCATE(); which will free memory on demand 

                #define FREELIST_ALLOCATE(Result, FreeListPointer, AllocationCode)             \
                    (Result) = (FreeListPointer); \
                    if(Result) 
                    {
                        FreeListPointer = (Result)->NextFree;
                    } 
                    else 
                    {
                        Result = AllocationCode;
                    }

                #define FREELIST_DEALLOCATE(Pointer, FreeListPointer) \
                    if(Pointer) 
                    {
                        (Pointer)->NextFree = (FreeListPointer); 
                        (FreeListPointer) = (Pointer);
                    }


Q/A
1:06:57
Can you explain what memory alignment is?

Pretty much that refers to the memory address you are interesting.

for certain operations, on certain CPU requires those addresses to be aligned at a certain boundary.
for example, if you have the address at byte 16, which is 16 byte aligned, it might be valid for a certain operation
but if you have the address at byte 13, it may no longer be valid for that operation 

this is becuz the "alignment" thats required by the processor is 16 byte aligned boundaries.