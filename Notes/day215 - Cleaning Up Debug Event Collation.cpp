Handmade Hero Day 215 - Cleaning Up Debug Event Collation

Summary:

changed the events array in debug_table to a temporary storage instead of a buffer of the past 8 frames.

changed the debug_frame data structure to a linked list. Everytime we get a new DebugType_FrameMarker, we 
add a new debug_frame to our debug_frame linkes list


Keyword:
debug system



2:38
Casey addressing the problem with our current debug_table 


currently we have a 2D array for debug_event

                #define MAX_DEBUG_THREAD_COUNT 256
                #define MAX_DEBUG_EVENT_ARRAY_COUNT 8
                #define MAX_DEBUG_EVENT_COUNT (16*65536)
                struct debug_table
                {
                    // TODO(casey): No attempt is currently made to ensure that the final
                    // debug records being written to the event array actually complete
                    // their output prior to the swap of the event array index.
                    
                    u32 CurrentEventArrayIndex;
                    u64 volatile EventArrayIndex_EventIndex;
                    u32 EventCount[MAX_DEBUG_EVENT_ARRAY_COUNT];
                    debug_event Events[MAX_DEBUG_EVENT_ARRAY_COUNT][MAX_DEBUG_EVENT_COUNT];
                };

MAX_DEBUG_EVENT_ARRAY_COUNT is 8, so we have 8 frames worth of debug events.

so all the thread writes to one, while the debug system reads the other    
everything is done atomically.

    thread 1, 2, 3, ...
          
         |  |  |
         |  |  |
         v  v  v 
         _______________________________________________________________________________
        |               |               |               |               |               |
        |               |               |               |               |               |
        |               |               |               |               |               |
        |_______________|_______________|_______________|_______________|_______________|


one concern is that if we want to save the memory bandwidth of copying debug events out of this array, 
then you should just directly read debug_event data from this array.

on the otherhand, we can argue that we can leave this debug_event table as a temporary storage.
so there will be only two of the arrays as a pingpong buffer 

                debug_event Events[2][MAX_DEBUG_EVENT_COUNT];


[Im slightly confused. So you would have something like:

                debug_event temporaryStorage[2][MAX_DEBUG_EVENT_COUNT];
                debug_event Events[MAX_DEBUG_EVENT_ARRAY_COUNT][MAX_DEBUG_EVENT_COUNT];? 

but you would still have this MAX_DEBUG_EVENT_ARRAY_COUNT x MAX_DEBUG_EVENT_COUNT array?]


there really isnt much difference between these two cases, only that in the 2nd case, we can save specific frame data 

theres a reason why Casey likes this approach. 

the problem with our current approach is that, we thought that each of the bucket represents a frame worth of data,
but that is not really true, becuz the debug system fires before the end of a frame occurs 

on a timeline, we have 

    thread 1, 2, 3, ...
          
         |  |  |
         |  |  |
         v  v  v 
         _________________________________________________________________________________________________________
        |                                                    |                                                    |
        |                                                    |                                                    |
        |                                                    |                                                    |
        |____________________________________________________|____________________________________________________|
        
        ^                                ^        ^         ^                                                                              
        |   we do a bunch of work        | debug  | win32   |                                                                                       
        |                                | system | display |                                                                                            
                                         |
    frame begin                          |
                                         |

                                       flipping     


so the time the debug buffer flips is when we first call the debug system
which we do it in DEBUG_GAME_FRAME_END

                handmade_debug.cpp

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {    
                    ++GlobalDebugTable->CurrentEventArrayIndex;
                    if(GlobalDebugTable->CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable->Events))
                    {
                        GlobalDebugTable->CurrentEventArrayIndex = 0;
                    }
                    
                    ...
                    ...
                }


7:27
adding to the problem is that we have events happening asynchronously
in which the task will span across frames 

         _________________________________________________________________________________________________________
        |                                                    |                                                    |
        |                                                    |                                                    |
        |                                                    |                                                    |
        |____________________________________________________|____________________________________________________|
                                                
                                                |                          |
                                                ----------------------------

essentially, the frame data and our debug buffered data is not really the same thing 

which is why Casey wants to change debug_table to just be a temporary buffer, and have the actualy debug_event by frames storage 
somewhere else 

Casey doesnt know if this is a good idea. The increased memory traffic is a real concern, since we need to copy all the debug_events 
from the temporary buffer to our permeanent buffer


9:52
now Casey will make debug_table Events just to be a scratch buffer. When we collate, we will copy information we need, 
and we are throwing away everything else.



12:00
so now our debug_table looks like:

                handmade_debug_interface.h

                struct debug_table
                {
                    // TODO(casey): No attempt is currently made to ensure that the final
                    // debug records being written to the event array actually complete
                    // their output prior to the swap of the event array index.    
                    u32 CurrentEventArrayIndex;
                    // TODO(casey): This could actually be a u32 atomic now, since we
                    // only need 1 bit to store which array we're using...
                    u64 volatile EventArrayIndex_EventIndex;
                    debug_event Events[2][16*65536];
                };


16:06
so in the debug_state, we still have the "debug_frame *Frames" array 

                
                handmade_debug.h 
                struct debug_state
                {
                    ...
                    ...
                
                    debug_frame* Frames;
                    ...
                };

the idea is that everytime a new comes in, we put in this Frames array, and we get rid of the oldest one


17:26
Casey proceeds to change the array into a linkedlist

the CollationFrame is gonna be the frame that we are working on, and we havent quite seen the end FrameMark

                handmade_debug.h

                struct debug_state
                {
                    ...
                    ...

                    char *ScopeToRecord;

                    u32 FrameCount;
                    debug_frame *OldestFrame;
                    debug_frame *MostRecentFrame;
                    debug_frame *FirstFreeFrame;

                    debug_frame *CollationFrame;

                    u32 FrameBarLaneCount;
                    debug_thread *FirstThread;
                    debug_thread *FirstFreeThread;
                    open_debug_block *FirstFreeBlock;
                };


36:14
1:06:58
Casey writing the NewFrame(); function 

                handmade_debug.cpp

                internal debug_frame * NewFrame(debug_state *DebugState, u64 BeginClock)
                {
                    // TODO(casey): Simplify this once regions are more reasonable!
                    debug_frame *Result = DebugState->FirstFreeFrame;
                    if(Result)
                    {
                        DebugState->FirstFreeFrame = Result->NextFree;
                        debug_frame_region *Regions = Result->Regions;
                        ZeroStruct(*Result);
                        Result->Regions = Regions;
                    }
                    else
                    {
                        Result = PushStruct(&DebugState->DebugArena, debug_frame);
                        ZeroStruct(*Result);
                        Result->Regions = PushArray(&DebugState->DebugArena, MAX_REGIONS_PER_FRAME, debug_frame_region);
                    }

                    Result->FrameBarScale = 1.0f;
                    Result->RootGroup = CreateVariableGroup(DebugState);

                    Result->BeginClock = BeginClock;

                    return(Result);
                }



42:52
so in the DebugCollation part, when we see a DebugType_FrameMarker, we just update our linked-list


                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray)
                {   

                    ...
                    ...

                    else if(Event->Type == DebugType_FrameMarker)
                    {
                        Assert(DebugState->CollationFrame);

                        DebugState->CollationFrame->EndClock = Event->Clock;
                        DebugState->CollationFrame->WallSecondsElapsed = Event->Value_r32;

                        if(DebugState->Paused)
                        {
                            FreeFrame(DebugState, DebugState->CollationFrame);
                        }
                        else
                        {
                            if(DebugState->MostRecentFrame)
                            {
                                DebugState->MostRecentFrame->Next = DebugState->CollationFrame;
                            }
                            else
                            {
                                DebugState->OldestFrame = DebugState->MostRecentFrame = DebugState->CollationFrame;
                            }                    
                            ++DebugState->FrameCount;
                        }

                        DebugState->CollationFrame = NewFrame(DebugState, Event->Clock);

                        ...
                    }
                }


1:00:28
So now Casey will write the FreeFrame(); function. This is the first time we will have to reclaim some memory 
99.9% of the time, memory isnt something that you think about. This is the first destructor style of code we will do.

this is where we actually want to free a thing. 

                handmade_debug.cpp

                internal void FreeFrame(debug_state *DebugState, debug_frame *Frame)
                {
                    FreeVariableGroup(DebugState, Frame->RootGroup);
                    FREELIST_DEALLOCATE(Frame, DebugState->FirstFreeFrame);
                }

Casey added the FREELIST_DEALLOCATE(); function

                handmade.h

                #define FREELIST_DEALLOCATE(Pointer, FreeListPointer) \
                    if(Pointer) {(Pointer)->NextFree = (FreeListPointer); (FreeListPointer) = (Pointer);}


write now, this macro is literraly just adding a block of memory that you want to deallocate to the FreeList
pretty straight-forward


Right now if we keep the game running, we will run out of memory for debug_frames. so we will have to start 
reuse some of the free list frames.

This will be accomplished next epsiode.
