Handmade Hero Day 181 - Log-based Performance Counters

Summary:

so from da 180, we have one debug system, in which we call TIMED_BLOCKS everywhere and  
we have the print outs for all the timings for each functions 

and we have a second debug system, where we collect debug_frame_end_info timings and 
we render stacked graphs at the bottom.

Casey wants to combine the two systems 

proposes the concept of debug_event logging for our debug system.

described the difference between __rdtsc(); and __rdtscp();

create the debug_event struct. 
create a debug_event at TIMED_BLOCK begin and end;

populated the array of debug_counter_state 
(the array that stores TIMED_BLOCK information for the past 128 frames in the debug memory storage);
with information at th debug_event


Keyword:
debug system. Profiling


0:42
to summarize what we have been doing for the past few days, we now have a debug system that is easy to add, 
(by putting TIMED_BLOCKS anywhere you want);. The timers are completely thread safe, they have almost zero overhead, 
in which they have extremely low impact on the actual performance of the program.

but they do cost a lot to visualize

so we have one debug system, in which we call TIMED_BLOCKS everywhere and  
we have the print outs for all the timings for each functions 

and we have a second debug system, where we collect debug_frame_end_info timings and 
we render stacked graphs at the bottom.

Casey wants to somehow combine these two debugging systems

what we dont get from the first system is any sense of who is taking up the time in our frame.
that is the advantage of our 2nd system.
our 2nd system, which renders a stacked bar can visually tell you that the blue section is taking the most time.


if you remember, for our 2nd system, the platform layer is taking all the timings in debug_frame_end_info

but for the 1st system, the game code has all the TIMED_BLOCKS


5:27
if we want to keep most of the advantages of this system. we want to avoid doing much work when the timer code is profiling, gathering timings.
that part of the code needs to be lightweight.

we can spend an arbituaray amount of time at the end of a frame. Although that may create some 
stuff that skew the performance of our program.

For example, it may cause the program to not have the same cache effect becuz at the end of our frame there is all this 
extra cache effects. 

so frame to frame there may be some sort of jenk there.

but other than that, doing a ton of work to gather profiling results is fine, as long as we keep the code 
that does the sampling relatively efficient. 

so for the most part, Casey doesnt really care if we do tons of shit at the end.
we would only mind if we have heavy-weight stuff happening at the timers when profiling.


7:31
one approach we could take to combine both visualizations is to take a log-based system. (not the math log, but the log of outputting text)
we can have a system that collect log entries after the fact. This has the nice property, that if we do log entry, 
we can stamp those down in a much more efficient fashion. 

that log entry can contain information such as the __rdtsc();, or what processor it was on.


8:18
The problem is that, for some debug_record, the number of "hitCount" could be in the thousands, 

so if our debug_record entry has a hit count of 5000, and each one of my records is 16 bytes
then we are talking about 5000 * 16 = 80,000. or 80k writes that are happening

however, at the end of the day, we dont really care cuz thats not alot of write bandwidth.
its not really polluting the cache cuz its writing that wont be read very often. You might even be able to do it in a non-temporal storage, 
where we can tell explicitly, dont pollute the cache. 


[what is non-temporal store?
https://stackoverflow.com/questions/37070/what-is-the-meaning-of-non-temporal-memory-accesses-in-x86

non-temporal SSE instructions dont follow the normal cache-coherency rules. Therefore non-temporal stores must be followed
by an SFENSE instruction in order for their results to be seen by other processors in a timely fashion

Essentially, processors provide support for non-temporal write operations. Non-temporal in this context means the data
will not be reused soon, so there is no reason to cache it. These non-temporal write operations do not read a cache line 
and then modify it; instead, the new content is directly written to memory]

but in summary, Casey doesnt know what to do.



11:36 
Casey introduces the function __rdtscp();

__rdtsc(); stands for read timestamp counter. You can think of it as reading a register/counter on the CPU
that tells you what time it is, effectively. As a the CPU executes instructions, you can see how much processor time
has elapsed.

__rdtscp(); is supposed to do two things. It does what __rdtsc does, and it also gives you back the processor ID.
https://www.felixcloutier.com/x86/rdtscp

in the link above, it says it returns a 32-bit IA32_TSC_AUX value

Casey initially thought that it is supposed to return "what core you are running on?"

Casey also went inside the intel manual to understand what __rdtscp(); does.

at the end, Casey couldnt get __rdtscp(); to work


22:07
So what Casey wants to do write down all the debug_event. we define a debug_event as whenever TIMED_BLOCKS begin block 
or a TIMED_BLOCKS end block gets called  

Casey first defines a new struct debug_event

-   recall we have to DebugRecordArray from our two compilation units.
    the DebugRecordArrayIndex indicates which array you are in.

-   we also define a u8 Type, which tells if you are BeginBlock or EndBlock. This will be explained later

                handmade_debug.h

                enum debug_event_type
                {
                    DebugEvent_BeginBlock,
                    DebugEvent_EndBlock,
                };


                struct debug_event
                {
                    u64 Clock;
                    u16 ThreadIndex;
                    u16 CoreIndex;
                    u16 DebugRecordIndex;
                    u8 DebugRecordArrayIndex;
                    u8 Type;
                };



24:59
Casey then defines the global array.

-   the u64 GlobalDebugEventIndex

    lets us know where we are writing inside the GlobalDebugEventArray

                handmade_debug.h

                extern u64 GlobalDebugEventIndex;
                extern debug_event GlobalDebugEventArray[65536];


                handmade_debug.cpp 

                u64 GlobalDebugEventIndex = 0;
                debug_event GlobalDebugEventArray[65536];


Casey mentioned that this DebugEventArray doest have to be unique. Anyone can write into it 



25:50
if we want this thing to be relatively thread safe, we will face a problem down the road 
where somebody will have to collect the data at some point, and in so doing, we need to fix the 
synchronization issue of someone writing and someone reading

so we want a pingpong buffer situation, one buffer where people write into, the other people reading
so just a double buffering scheme 


                handmade_debug.h

                #define MAX_DEBUG_EVENT_COUNT (16*65536)        
                extern u64 GlobalDebugEventIndex;
                extern debug_event GlobalDebugEventArray[2][MAX_DEBUG_EVENT_COUNT];


                handmade_debug.cpp 

                u64 GlobalDebugEventIndex = 0;
                debug_event GlobalDebugEventArray[2][MAX_DEBUG_EVENT_COUNT];



40:58
so previously Casey used a debug_event* to swap the pingpong buffer 

                handmade_debug.h

                #define MAX_DEBUG_EVENT_COUNT 65536
                extern u32 GlobalDebugEventIndex;
    -------->   extern debug_event* GlobalDebugEventArray;


                handmade_debug.cpp

    -------->   debug_event GlobalDebugEventArray[2][MAX_DEBUG_EVENT_COUNT];


to swap them, we can use a SynchronizeExchange on the pointer, but what we cant do is exchange the pointer 
and clear the GlobalDebugEventIndex at the sametime.


so Casey combined the two the GlobalDebugEventIndex and DebugEventArrayIndex into one 64 bit
now we have:



                handmade_debug.h
                
                #define MAX_DEBUG_EVENT_COUNT (16*65536)
                extern u64 Global_DebugEventArrayIndex_DebugEventIndex;
                extern debug_event GlobalDebugEventArray[2][MAX_DEBUG_EVENT_COUNT];


                handmade_debug.cpp

                u64 Global_DebugEventArrayIndex_DebugEventIndex = 0;
                debug_event GlobalDebugEventArray[2][MAX_DEBUG_EVENT_COUNT];







27:42
once we have defined our DebugEvent struct, we create one whenever TIMED_BLOCKS begin and end gets called  


                handmade_debug.cpp

                struct timed_block
                {
                    ...
                    ...

                    timed_block(int CounterInit, char *FileName, int LineNumber, char *FunctionName, u32 HitCountInit = 1)
                    {
                        Counter = CounterInit;
                        
                        ...
                        ...

                        StartCycles = __rdtsc();

                        //

                        RecordDebugEvent(Counter, DebugEvent_BeginBlock);
                    }
                    
                    ~timed_block()
                    {
                        ...
                        ...

                        RecordDebugEvent(Counter, DebugEvent_EndBlock);
                    }
                };




27:30 42:43
we now define what happens in the RecordDebugEvent(); function 
-   you can see that any thread that tries to write into this Global_DebugEventArrayIndex_DebugEventIndex(); will try to 
    grab the next index:

                AtomicAddU64(&Global_DebugEventArrayIndex_DebugEventIndex, 1);

    which makes it thread safe.
    we have the DebugEventIndex in the lower 32 bit. So calling +1 on it wont affect the DebugEventArrayIndex unless 
    DebugEventIndex is 0xFFFFFFFF which we know it wont happen.

-   just like before, we the the EventIndex and ArrayIndex using the upper 32 and bottom 32 bit mask

                u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;       
                ...
                debug_event *Event = GlobalDebugEventArray[ArrayIndex_EventIndex >> 32] + EventIndex; 


-   full code below 

                handmade_debug.cpp

                inline void
                RecordDebugEvent(int RecordIndex, debug_event_type EventType)
                {
                    u64 ArrayIndex_EventIndex = AtomicAddU64(&Global_DebugEventArrayIndex_DebugEventIndex, 1);
                    u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;                
                    Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);                         
                    debug_event *Event = GlobalDebugEventArray[ArrayIndex_EventIndex >> 32] + EventIndex; 
                    Event->Clock = __rdtsc();                                           
                    Event->ThreadIndex = 0;                                             
                    Event->CoreIndex = 0;                                               
                    Event->DebugRecordIndex = (u16)RecordIndex;                         
                    Event->DebugRecordArrayIndex = DebugRecordArrayIndexConstant;       
                    Event->Type = (u8)EventType;
                }







45:56
then in our DEBUG_GAME_FRAME_END(); function, we call this 


-   we swap the pingpong buffer at the beginning of this function.

    we initialize a GlobalCurrentEventArrayIndex variable.


    then when we toggle the buffer, we use the GlobalCurrentEventArrayIndex to help us accomplish that.

                u64 ArrayIndex_EventIndex = AtomicExchangeU64(&Global_DebugEventArrayIndex_DebugEventIndex,
                                                                  (u64)GlobalCurrentEventArrayIndex << 32);

-   as what we mentioned earlier, once we toggled the buffer0, we can start reading from one of the buffer,
    while all the threads will be writing debug events into buffer1. 


-   full code below:

                handmade_debug.cpp

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    GlobalCurrentEventArrayIndex = !GlobalCurrentEventArrayIndex;
                    u64 ArrayIndex_EventIndex = AtomicExchangeU64(&Global_DebugEventArrayIndex_DebugEventIndex,
                                                                  (u64)GlobalCurrentEventArrayIndex << 32);

                    u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
                    u32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;
                    
                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState)
                    {
                        DebugState->CounterCount = 0;
                #if 0
                        UpdateDebugRecords(DebugState, DebugRecords_Optimized_Count, DebugRecords_Optimized);
                        UpdateDebugRecords(DebugState, ArrayCount(DebugRecords_Main), DebugRecords_Main);
                #else
                        CollateDebugRecords(DebugState, EventCount, GlobalDebugEventArray[EventArrayIndex]);
                #endif        

                        DebugState->FrameEndInfos[DebugState->SnapshotIndex] = *Info;
                        
                        ++DebugState->SnapshotIndex;
                        if(DebugState->SnapshotIndex >= DEBUG_SNAPSHOT_COUNT)
                        {
                            DebugState->SnapshotIndex = 0;
                        }
                    }
                }


53:35
so at this point, we have all of the debug events, we can now do stuff with them.
Casey defines the function CollateDebugRecords();

-   first we do
        
                DebugState->CounterCount = DebugRecords_Optimized_Count + ArrayCount(DebugRecords_Main);

    recall DebugState comes from DebugMemory. So we can use that part of memory anyway we want.

                typedef struct game_memory
                {
                    ...
                    ...

                    uint64 DebugStorageSize;
                    void *DebugStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

                    platform_api PlatformAPI;
                } game_memory;


    so here, we are treating the memory at game_memory->DebugStorage pas if we have an array of DebugState that is 
    (DebugRecords_Optimized_Count + ArrayCount(DebugRecords_Main)); in length


-   remember, the purpose of DebugState->CounterStates is to store TIMED_BLOCK information over time.
    currently we just store the past 128 frames

    so first we go through all the debug_counter_state in the current frame, and we clear the HitCount and CycleCount


-    we create two array heads that will do the 2nd for loop


                    debug_counter_state *CounterArray[2] =
                    {
                        DebugState->CounterStates,
                        DebugState->CounterStates + ArrayCount(DebugRecords_Main),
                    };
                    debug_record *DebugRecords[2] =
                    {
                        DebugRecords_Main,
                        DebugRecords_Optimized,
                    };


    visually, it looks like this 

    DebugState->ConterStates                DebugState->CounterStates + ArrayCount(DebugRecords_Main),
        |                                       |
        |                                       |
        V                                       V
     _______________________________________________________________
    |       |       |       |       |       |       |       |       |
    |       |       |       |       |       |       |       |       |
    |_______|_______|_______|_______|_______|_______|_______|_______|

-   then in the second for loop, we go through our debug_events and put debug_events information to our 
    debug_counter_state
    
    we grab the debug_counter_state

                    debug_counter_state *Dest = CounterArray[Event->DebugRecordArrayIndex] + Event->DebugRecordIndex;

    we grab our debug_event

                    debug_event *Event = Events + EventIndex;

    then we populate the snapshot date 

                    if(Event->Type == DebugEvent_BeginBlock)
                    {
                        ++Dest->Snapshots[DebugState->SnapshotIndex].HitCount;
                        Dest->Snapshots[DebugState->SnapshotIndex].CycleCount -= Event->Clock;
                    }
                    else
                    {
                        Assert(Event->Type == DebugEvent_EndBlock);
                        Dest->Snapshots[DebugState->SnapshotIndex].CycleCount += Event->Clock;
                    }


-   full code below:


                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *Events)
                {
                    DebugState->CounterCount = DebugRecords_Optimized_Count + ArrayCount(DebugRecords_Main);
                    for(u32 CounterIndex = 0; CounterIndex < DebugState->CounterCount; ++CounterIndex)
                    {
                        debug_counter_state *Dest = DebugState->CounterStates + CounterIndex;
                        Dest->Snapshots[DebugState->SnapshotIndex].HitCount = 0;
                        Dest->Snapshots[DebugState->SnapshotIndex].CycleCount = 0;
                    }

                    debug_counter_state *CounterArray[2] =
                    {
                        DebugState->CounterStates,
                        DebugState->CounterStates + ArrayCount(DebugRecords_Main),
                    };
                    debug_record *DebugRecords[2] =
                    {
                        DebugRecords_Main,
                        DebugRecords_Optimized,
                    };

                    for(u32 EventIndex = 0; EventIndex < EventCount; ++EventIndex)
                    {
                        debug_event *Event = Events + EventIndex;

                        debug_counter_state *Dest = CounterArray[Event->DebugRecordArrayIndex] + Event->DebugRecordIndex;

                        debug_record *Source = DebugRecords[Event->DebugRecordArrayIndex] + Event->DebugRecordIndex;
                        Dest->FileName = Source->FileName;
                        Dest->FunctionName = Source->FunctionName;
                        Dest->LineNumber = Source->LineNumber;

                        if(Event->Type == DebugEvent_BeginBlock)
                        {
                            ++Dest->Snapshots[DebugState->SnapshotIndex].HitCount;
                            Dest->Snapshots[DebugState->SnapshotIndex].CycleCount -= Event->Clock;
                        }
                        else
                        {
                            Assert(Event->Type == DebugEvent_EndBlock);
                            Dest->Snapshots[DebugState->SnapshotIndex].CycleCount += Event->Clock;
                        }
                    }
                }


1:10:56
someone asked, whats the point of CoreId in debug_event? It seems that the threadId is the important part.

the reason is becuz that way we can see when tasks moved cores, that way we can see if we got task swapped, and if a task 
moved from core 1 to core 2. it also helps you know for cache coherency, if a task should be happening on the same core, but arent,
we want to be able to see that. 


1:12:34
how would you avoid collecting stats on debug rendering code?

Casey says he hopes this debug_event log system can solve it.


