Handmade Hero Day 185 - Finishing Basic Debug Collation

Summary:

did a lot of work on CollateDebugRecords();

starts to tackle the question of how to find matching BEGIN(); block and END(); block in the debug_events array

made the constraint that we wont have interleaved BEGIN(); and END(); blocks, that way we can always 
use a stack, and the top of the stack will always be the matching block.

assigned a stack for each thread, to find matching blocks. 

implemented the stack using a linked list, wrote the struct open_debug_block for the block nodes 


Keyword:
debug system, profiling



3:33
we continue on the work we started in day 184

in the CollateDebugRecords(); we allocate memory for our debug_frames. Since we already allocated memory 
for the DebugState, we allocate memory from the DebugState->CollateArena.

-   as for a rough estimate, we just give it MAX_DEBUG_EVENT_ARRAY_COUNT * 4.

    we changed from MAX_DEBUG_FRAME_COUNT to MAX_DEBUG_EVENT_ARRAY_COUNT. 

    so this allows us to generation 4 debug_frames per frame. not that we will be generating 2 debug_frames per frame,
    but nothing is preventing us from doing so. 

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {
                    DebugState->Frames = PushArray(&DebugState->CollateArena, MAX_DEBUG_EVENT_ARRAY_COUNT*4, debug_frame);
                    DebugState->FrameBarLaneCount = 0;
                    DebugState->FrameCount = 0;    
                    DebugState->FrameBarScale = 1.0f / 60000000.0f;

                    ...
                    ...
                }




6:51
so now we want to allocate memory for the debug_frame_region for each debug_frame

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {
                    DebugState->Frames = PushArray(&DebugState->CollateArena, MAX_DEBUG_EVENT_ARRAY_COUNT*4, debug_frame);
                    DebugState->FrameBarLaneCount = 0;
                    DebugState->FrameCount = 0;    
                    DebugState->FrameBarScale = 1.0f / 60000000.0f;

                    debug_frame *CurrentFrame = 0;
                    for(u32 EventArrayIndex = InvalidEventArrayIndex + 1; ; ++EventArrayIndex)
                    {
                        ...
                        ...

                        for(u32 EventIndex = 0; EventIndex < GlobalDebugTable->EventCount[EventArrayIndex]; ++EventIndex)
                        {
                            debug_event *Event = GlobalDebugTable->Events[EventArrayIndex] + EventIndex;            
                            debug_record *Source = (GlobalDebugTable->Records[Event->TranslationUnit] +
                                                    Event->DebugRecordIndex);

                            if(Event->Type == DebugEvent_FrameMarker)
                            {
                                ...
                                ...

                                CurrentFrame = DebugState->Frames + DebugState->FrameCount++;
                                CurrentFrame->BeginClock = Event->Clock;
                                CurrentFrame->EndClock = 0;
                                CurrentFrame->RegionCount = 0;
        --------------->        CurrentFrame->Regions = PushArray(&DebugState->CollateArena, MAX_REGIONS_PER_FRAME, debug_frame_region);
                            }
                        }
                    }
                }


MAX_REGIONS_PER_FRAME is 256 which we define in handmade_debug.h 
                
                handmade_debug.h

                #define MAX_REGIONS_PER_FRAME 256 



15:25 
Casey starting to consider how to group BEGIN and END blocks in the debug events


                TID     Functions 
                0       BEGIN Foo();
                1       BEGIN Foo();
                0       END Foo();
                1       BEGIN Foo();
                1       END Foo();
                1       END Foo();

                
so consider multithreading, whenever we see any debug_event, the combination of ThreadID and CounterID
uniquley identifies a debug_event


[this reminds me of the "valid parenthesis string" leetcode problem]


So what Casey plans to do is just per threadId, keep a linked list of what blocks are open, then for 
any incoming END block, we just search through the linked list 


the reason why we are not using the stack here (which is the solution to the 'valid parenthesis string' problem);
is becuz consider the special case where, you can have interleaved blocks on the same thread. 

        +------ 0   BEGIN Foo();
        |
        |   +-- 0   BEGIN Bar();
        |   |    
        +---|-- 0   END Foo();
            |   
            +-- 0   END Bar();


meaning we have a parent block, calling another block begin, then the next end block we get is not the interior end block 
but rather the parent block

if we dont allow then, that we can just use stack, where the stack_s top entry is always the matching BEGIN(); block 
for the upcoming END(); block.

but becuz its probably not too hard to search throu the linked list. In rare case that this happens, we might as well support.



upon further considerations, Casey thinks its actually quite hard to visaulize that in the stacked bar graph

cuz if you allow interleaving blocks, there is no clear parent, child hierarchy. so how do you render that?

[so does that mean we can just use stack if we decide not to allow interleaving blocks?]


20:13
Casey starting to define linked list.
we define the struct open_debug_block. 
inside, we store a pointer and the "debug_event* OpeningEvent" variable. that will represent the BEGIN(); block debug_event.

we also store a "open_debug_block *Parent;" that way we know the block above me

                struct open_debug_block
                {
                    u32 StartingFrameIndex;
                    debug_event *OpeningEvent;
                    open_debug_block *Parent;

                    open_debug_block *NextFree;
                };




then we define a debug_thread struct. that will contain the head of our linked list.
also the debug_thread is also a list of linked list. 

                struct debug_thread
                {
                    u32 ID;
                    u32 LaneIndex;
                    open_debug_block *FirstOpenBlock;
                    debug_thread *Next;
                };




for the debug_state, we will store a linked list for every thread
we also store a pointer to the free ones. We have done this techinique numerous times. 

                struct debug_state
                {
                    b32 Initialized;

                    // NOTE(casey): Collation
                    memory_arena CollateArena;
                    temporary_memory CollateTemp;

                    u32 FrameBarLaneCount;
                    u32 FrameCount;
                    r32 FrameBarScale;

                    debug_frame *Frames;
                    debug_thread *FirstThread;
                    open_debug_block *FirstFreeBlock;
                };



so in summary, debug_stae stores a linked list of debug_thread.
the debug_thread stores a linked list of open_debug_block.





26:03
so then in CollateDebugRecors(); once we are reading DebugEvent_BeginBlock or DebugEvent_EndBlock, we want to interact 
with our debug_thread and open_debug_block linked list 

-   so in the non DebugEvent_FrameMarker case, 
    we first grab the DeubgThread 

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {
                    ...
                    ...

                    for(u32 EventIndex = 0; EventIndex < GlobalDebugTable->EventCount[EventArrayIndex]; ++EventIndex)
                    {
                        debug_event *Event = GlobalDebugTable->Events[EventArrayIndex] + EventIndex;            
                        debug_record *Source = (GlobalDebugTable->Records[Event->TranslationUnit] +
                                                Event->DebugRecordIndex);

                        if(Event->Type == DebugEvent_FrameMarker)
                        {
                            ...
                            ...
                        }
                        else if(CurrentFrame)
                        {
                            u32 FrameIndex = DebugState->FrameCount - 1;
            ----------->    debug_thread *Thread = GetDebugThread(DebugState, Event->ThreadID);
                            u64 RelativeClock = Event->Clock - CurrentFrame->BeginClock;
                            if(Event->Type == DebugEvent_BeginBlock)
                            {
                                ...
                                ...
                            }
                            else if(Event->Type == DebugEvent_EndBlock)
                            {
                                ...
                                ...
                            }
                            else
                            {
                                Assert(!"Invalid event type");
                            }
                        }
                    }
                }





45:40
we implement the GetDebugThread(debug_state *DebugState, u32 ThreadID);
should be pretty straight forward. we try to search it by ThreadID. If we can find it, we return it. 
if not, we create a new one.


                handmade_debug.cpp

                internal debug_thread * GetDebugThread(debug_state *DebugState, u32 ThreadID)
                {
                    debug_thread *Result = 0;
                    for(debug_thread *Thread = DebugState->FirstThread;
                        Thread;
                        Thread = Thread->Next)
                    {
                        if(Thread->ID == ThreadID)
                        {
                            Result = Thread;
                            break;
                        }
                    }

                    if(!Result)
                    {
                        Result = PushStruct(&DebugState->CollateArena, debug_thread);
                        Result->ID = ThreadID;
                        Result->LaneIndex = DebugState->FrameBarLaneCount++;
                        Result->FirstOpenBlock = 0;
                        Result->Next = DebugState->FirstThread;
                        DebugState->FirstThread = Result;
                    }

                    return(Result);
                }



27:29
so everytime we see a DebugEvent_BeginBlock, we will create a open_debug_block;

-   notice that we use the DebugState->FirstFreeBlock. so if we can recycle one, we will use it.
    otherwise, we get a new one from our memory. 

    once we recyled one, we have the "DebugState->FirstFreeBlock" point to the next one 
    that way we update our "DebugState->FirstFreeBlock" pointer to point to the correct block 

                DebugState->FirstFreeBlock = DebugBlock->NextFree;

-   we also populate the DebugBlock with the right values 

                DebugBlock->OpeningEvent = Event;
                DebugBlock->Parent = Thread->FirstOpenBlock;

    the parent is the most recent/interior OpenBlock

    the idea of our linked list implemented stack is 


        BEGIN foo();

            BEGIN foo2();



            END foo2();

        END foo();


1.  so when we get to BEGIN foo(); we have

                               
                             _______
    debug_thread            |       |
     FirstOpenBlock ------> | foo();|
                            |_______|
                                |
                                |
                                v

                              NULL

2.  then when we get to BEGIN foo2();

                                
                                
                             _______
                            |       |
    debug_thread            |foo2();|
    FirstOpenBlock ------>  |_______|
                                |
                                |
                                v
                             _______
                            |       |
                            | foo();|
                            |_______|
                                |
                                |
                                v
                              NULL


so you can think of it that we are always adding to the head of this linked list. Just like a stack 
debug_thread->FirstOpenBlock points to the top node. First In Last out.

[I think this is my first time seeing a linked-list implemented stack, which is why it took me a while 
    to understand what was going on]


    which is why we do we call "Thread->FirstOpenBlock = DebugBlock" to update the stack top node

                DebugBlock->StartingFrameIndex = FrameIndex;
                DebugBlock->OpeningEvent = Event;
                DebugBlock->Parent = Thread->FirstOpenBlock;
    ------->    Thread->FirstOpenBlock = DebugBlock;


-   notice that we also record the FrameIndex. 

                DebugBlock->StartingFrameIndex = FrameIndex;

    at the very top, our assign FrameIndex to be 

                u32 FrameIndex = DebugState->FrameCount - 1;


    recall from section 6:51, DebugState->FrameCount is incremented as we visit FRAME_MARKERS in our debug_events.
    but we do a DebugState->FrameCount++ there.

                CurrentFrame = DebugState->Frames + DebugState->FrameCount++;

    hence the DebugState->FrameCount - 1 here.


-   full code below 

                u32 FrameIndex = DebugState->FrameCount - 1;
                debug_thread *Thread = GetDebugThread(DebugState, Event->ThreadID);
                u64 RelativeClock = Event->Clock - CurrentFrame->BeginClock;
                if(Event->Type == DebugEvent_BeginBlock)
                {
                    open_debug_block *DebugBlock = DebugState->FirstFreeBlock;
                    if(DebugBlock)
                    {
                        DebugState->FirstFreeBlock = DebugBlock->NextFree;
                    }
                    else
                    {
                        DebugBlock = PushStruct(&DebugState->CollateArena, open_debug_block);
                    }

                    DebugBlock->StartingFrameIndex = FrameIndex;
                    DebugBlock->OpeningEvent = Event;
                    DebugBlock->Parent = Thread->FirstOpenBlock;
                    Thread->FirstOpenBlock = DebugBlock;
                    DebugBlock->NextFree = 0;
                }
                else if(Event->Type == DebugEvent_EndBlock)
                {
                    ...
                    ...
                }
                else
                {
                    Assert(!"Invalid event type");
                }


28:31
in the DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd); 
we set all the thread id and FirstFreeBlock to null every frame. 

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {

                    if(DebugState)
                    {
                        
                        ...
                        ...

                        DebugState->FirstThread = 0;
                        DebugState->FirstFreeBlock = 0;
                    }
                }



30:50
then in the DebugEvent_EndBlock, we want to find the matching DebugBlock and remove it from our stack

we are assuming the matching BEGIN(); block is the top one of our stack. 

of course there can be edge cases 
it is also possible that an BEGIN(); block happened many many frames ago. For example, assume we now store 
the past 63 frames of debug data. if in any case, your BEGIN(); block happened beyond 63 frames ago,
then this END(); block will never find its matching BEGIN(); block 


-   so we do a check to see if they are indeed matching blocks 

                if((OpeningEvent->ThreadID == Event->ThreadID) &&
                   (OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex) &&
                   (OpeningEvent->TranslationUnit == Event->TranslationUnit)) {}


    we the top node of our stack doesnt match, then we knokw that this debug_event ended, which its begin(); is long gone.
    and how we choose to treat this is up to us.


-   then the second check we do is the to see if the open_debug_block frame count 

                if(MatchingBlock->StartingFrameIndex == FrameIndex) {}

    this is to check if the BEGIN(); and END(); happened on the same frame. 
    
    recall the graph from day 183


            ||                               ||
            ||   |_____________|             ||
            ||           |______________|    ||
            ||                  |__|         ||
                                    |______________________|


        FRAME_MARKER                    FRAME_MARKER



    if you have a block that spans two frames, then the MatchingBlock->StartingFrameIndex and endBlock FrameIndex
    wont equal, and we will have to do two halves of a span

    we will treat this case later.


-   if we are in the same frame, matching BEGIN();, END(); block case, we can then go ahead add a debug_frame_region.
    
    which we create a Region 

                    debug_frame_region *Region = AddRegion(DebugState, CurrentFrame);
                    Region->LaneIndex = Thread->LaneIndex;
                    Region->MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
                    Region->MaxT = (r32)(Event->Clock - CurrentFrame->BeginClock);


-   once we found or try to found a block, we get rid of it from our stack 

                    Thread->FirstOpenBlock->NextFree = DebugState->FirstFreeBlock;
                    DebugState->FirstFreeBlock = Thread->FirstOpenBlock;
                    Thread->FirstOpenBlock = MatchingBlock->Parent;

-   full code below:

                else if(Event->Type == DebugEvent_EndBlock)
                {
                    if(Thread->FirstOpenBlock)
                    {
                        open_debug_block *MatchingBlock = Thread->FirstOpenBlock;
                        debug_event *OpeningEvent = MatchingBlock->OpeningEvent;
                        if((OpeningEvent->ThreadID == Event->ThreadID) &&
                           (OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex) &&
                           (OpeningEvent->TranslationUnit == Event->TranslationUnit))
                        {
                            if(MatchingBlock->StartingFrameIndex == FrameIndex)
                            {
                                if(Thread->FirstOpenBlock->Parent == 0)
                                {
                                    debug_frame_region *Region = AddRegion(DebugState, CurrentFrame);
                                    Region->LaneIndex = Thread->LaneIndex;
                                    Region->MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
                                    Region->MaxT = (r32)(Event->Clock - CurrentFrame->BeginClock);
                                }
                            }
                            else
                            {
                                // TODO(casey): Record all frames in between and begin/end spans!
                            }

                            Thread->FirstOpenBlock->NextFree = DebugState->FirstFreeBlock;
                            DebugState->FirstFreeBlock = Thread->FirstOpenBlock;
                            Thread->FirstOpenBlock = MatchingBlock->Parent;
                        }
                        else
                        {
                            // TODO(casey): Record span that goes to the beginning of the frame series?
                        }
                    }
                }



41:18
Casey might want to consider only looking functions that are one level deep in the stack

just as a "Proof of concept", we add an if check, this way we are only adding regions for the top level blocks.


                if(Thread->FirstOpenBlock->Parent == 0)
                {
                    debug_frame_region *Region = AddRegion(DebugState, CurrentFrame);
                    Region->LaneIndex = Thread->LaneIndex;
                    Region->MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
                    Region->MaxT = (r32)(Event->Clock - CurrentFrame->BeginClock);
                }



47:16
in the AddRegion(); function, we allocate a region, (although the memory is already pre allocated);
and we return it. 

                debug_frame_region *
                AddRegion(debug_state *DebugState, debug_frame *CurrentFrame)
                {
                    Assert(CurrentFrame->RegionCount < MAX_REGIONS_PER_FRAME);
                    debug_frame_region *Result = CurrentFrame->Regions + CurrentFrame->RegionCount++;

                    return(Result);
                }

Q/A
1:08:44
what are the pros and cons of doing debug information real time vs logging them and doing them after the fact. 

1.  speed. it takes of up significant time out of a frame 

2.  Accessibility on the developer work station. a typical sitaution on a professional game development is that 
    i may not be running the game on the same machine that I am actively developing on. 

    so the example, if i am developing this game for playstation 4, the play station dev kit, may not have a mouse 
    or keyboard hooked up to it, and so if you have a way of just sending those debug events over the network, so that 
    they can come off of the target platform and onto your dev machine, where you can view them in a nice little application, 

    thats really nice. 

3.  historical analysis. So if we just keeping the debug data in memory, and not dumping them to disc. Then you can 
    look and compare to another run to examine the behaviours in more detail


