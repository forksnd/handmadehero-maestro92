Handmade Hero Day 181 - Log-based Performance Counters

Summary:

mentions that windows does have a function to get a threadID();

examines the assembly code for GetCurrentThreadId(); to see how expensive this function is 

briefly mentioned what the gs register is and how it is used for thread local storage 

refactored the debug system definitions. Moved debug_record and debug_event definitions from handmade_debug.h code to handmade_platform.h
so that both the win32 platform and the game code side can use a uniform memory for their debug_records and debug_events

created the debug_table struct and defined an extern GlobalDebugTable debug_table; for this purpose.

also made the debug_table flexible enough so that we can add as many TranslationUnit as we can, and anyone 
who includes handmade_platform.h will have access to this global debug table for their debug memory.

Keyword:
debug system. Profiling



2:20
one thing we like to solve with our new Log-based system that we are working on is 
how long did a function take exclusive of its children

for example currently in the TIMED_BLOCK inside GameUpdateAndRender(); that TIMED_BLOCK also includes the 
time in Overlay Debug.

jus like unity_s profiler, we want to be able show exclusive timings



6:58
one of our current limitations is best said by the comments:

                "currently no attempt is made to ensure that the final debug records being written to the 
                event array acctually complete their outpout prior to the swap of the event array index."

what he meant by this is that we currently have two GlobalDebugEventArrays.
As these debug_event gets streamed out into our debug system, we want to be able to do this continuously,
for as many frames as possible. so we dont want to place any restrictions on it. 

As a result, when someone goes to gather debug_events information, there is no way for us look at the data there 
to verify that all the writes to that GlobalDebugEventArray are done, and we dont want to place a mutex on it,
cuz the whole point of our timing blocks is to make them as fast as possible 

that is why, since we dont want a mutex, we are doing this with a double buffer. 

                extern u64 Global_DebugEventArrayIndex_DebugEventIndex;
                extern debug_event GlobalDebugEventArray[2][MAX_DEBUG_EVENT_COUNT];



there will be the rare case, which Casey thinks is extremely unlikely, where a thread got its debug_event array index from the 
AtomicAdd(); operation, (assuming index 200);. had not written to it yet, and in that period of time, got preempted.

and it stayed preempted for the entire time for the main thread to swap to the new buffer, and parse the whole debug buffer,
which includes index 200, the main thread will read garbage value there.

                handmade_debug.h

                inline void RecordDebugEvent(int RecordIndex, debug_event_type EventType)
                {
                    u64 ArrayIndex_EventIndex = AtomicAddU64(&Global_DebugEventArrayIndex_DebugEventIndex, 1);
                    u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;      
                                <----------------------------------------------------- preempted         
                    Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);                         
                    debug_event *Event = GlobalDebugEventArray[ArrayIndex_EventIndex >> 32] + EventIndex; 
                    Event->Clock = __rdtsc();                                           
                    Event->ThreadIndex = 0;                                             
                    Event->CoreIndex = 0;                                               
                    Event->DebugRecordIndex = (u16)RecordIndex;                         
                    Event->DebugRecordArrayIndex = DebugRecordArrayIndexConstant;       
                    Event->Type = (u8)EventType;
                }


Casey thinks the chances of that happening is close to zero. 
[Casey mentioins that the reason why he thinks is very slim is becuz the size of 
    MAX_DEBUG_EVENT_COUNT is extremely large. Why would a large size prevent this from happening?]

Casey mentioned that if there were a lot less debug events, this will happening every once in awhile, but we are 
talking about 16 * 65536]

Casey does mention that we might not fix this
Becuz we want to keep time recording light weight, so its probably not worth it to fix it if it happens 



10:44
from day 181, we created the RecordDebugEvent(); but we didnt populate the CoreIndex and ThreadIndex.

                handmade_debug.h

                inline void RecordDebugEvent(int RecordIndex, debug_event_type EventType)
                {
                    ...
                    ...
                    Event->Clock = __rdtsc();                                           
    ----------->    Event->ThreadIndex = 0;                                             
    ----------->    Event->CoreIndex = 0;                                               
                    ...
                    ...
                }

CoreIndex, Casey planed to accomplish it with __rdtscp(); but he couldnt figure it out.
The CoreIndex is a nice to have, but not a core part. we can leave it as 0, and not care about.

ThreadIndex is absolutely crucial, becuz some of our threads call the same function. 
we cannot assume, that any given functions begin and end are happening on the same thread.

for example 

                Thread1                             Thread2 

                function1.TIMED_BLOCK.Begin
                                                    function1.TIMED_BLOCK.Begin


                                                    
                function1.TIMED_BLOCK.End
                                                    function1.TIMED_BLOCK.End

there is just no way to know unless we have this ThreadIndex


so we need to know how all of our TIMED_BLOCK information line up in a thread sense 

so for instance, if we just have on thread running, and we got 

                function1.TIMED_BLOCK.Begin();
                function1.TIMED_BLOCK.Begin();
                function1.TIMED_BLOCK.Begin();
                function1.TIMED_BLOCK.End();
                function1.TIMED_BLOCK.End();
                function1.TIMED_BLOCK.End();

then we know that the first Begin(); goes with the last End();

but in the 2 thread case above , that assumption is no longer true.

we would have
                function1.TIMED_BLOCK.Begin();
                function1.TIMED_BLOCK.Begin();
                function1.TIMED_BLOCK.End();
                function1.TIMED_BLOCK.End();

but the first Begin(); and the last End(); did not came from the same thread. 
this is why we need the threadIdx.



13:34
we have two options to solve this problem

option1, we figure out a unique number for the thread

Casey likes this approach cuz that way we can examine specific threads behaviouir.
lets say we just want to examine thread 23_s behaviour, we will have the ability to filter out all other threads
and just examine thread 23_s activity

our game is not thread crazy though. 

option2, 
since we know the begin and end blocks are paired, we can introduce another counter (lets say a u32); that gets atomically incremented 
everytime when a TIMED_BLOCK gets called and create a debug_event.
So instead of thread index, it will be unique id for each debug_event.


15:45
in win32 there is a way to get the threadId with a function call.
Remember function call not something we really want to do in a timer block. 


GetCurrentThreadId();
https://docs.microsoft.com/en-us/windows/desktop/api/processthreadsapi/nf-processthreadsapi-getcurrentthreadid


16:17
so Casey just to get a sense how expensive this function is, examines the assembly of this function 

                call    qword ptr [__imp_GetCurrentThreadID()]
                mov     dword ptr [ThreadID], eax 


goes through a dynamically linked jump table

                mov     rax, qword ptr gs:[30h]
                mov     eax, dword ptr [rax+48h]


as you can see, it is just referencing off of gs memory
so getting the threadID(); is really just doing addressing into gs memory

what that implies is that using windows to get the ThreadId(); is just 2 insruction,
which we access thread local storage

19:28
Casey explains what he means by thread local storage.

Most OS these days, when a thread begin, will set aside some memory that is unique for the thread.
and it sets up some known segment addressing, some predefined stuff in the memory table, basically. 

Proceeds to show it in the Visual Studio debugger by opening the "Register" tab, and he right clicks to 
show the "CPU Segments"

OS used to do Memory segmentation, where it based memory offset from addresses stored in the segment registers.
but now we mostly do paging 

so essentially OS nowadays can do memory addressing 2 ways. One the normal way, the other basing off of segment. 
memory segmentation is only used for special cases now. 


each thread can have the segment registers set to a different value. That effectively makes the thread to have thread local storage.



23:48
What Casey is wondering, at the end of the day, we dont really want to do a function call. Casey wonders if 
there is some kind of intrinsics that he can do to get the threadId



30:29 
So Casey found an intrinsics

__readgsqword

https://docs.microsoft.com/en-us/cpp/intrinsics/readgsbyte-readgsdword-readgsqword-readgsword?view=vs-2017

and he tested it with a small snippet to see if the value we got back using this intrinsics is the same as the value 
we got back from GetCurrentThreadId();
-   the 0x30 and 0x48 are values we got from viewing the disassembly code

                DWORD ThreadId = GetCurrentThreadId();
                u8* ThreadLocalStorage = (u8 *)__readgsqword(0x30);
                u32 ThreadID_2 = *(32 *)(ThreadLocalStorage + 0x48);



31:34
Casey Proceeds to put that in the RecordDebugEvent(); function


                handmade_debug.h

                inline void RecordDebugEvent(int RecordIndex, debug_event_type EventType)
                {

                    ...
                    ...                                     
    ----------->    Event->ThreadIndex = (u16)GetThreadID();
                    Event->CoreIndex = 0;                                               
                    ...
                    ...
                }



32:28
Casey proceeds to add an intrinsics function file 
of course we only want to do this when we are in windows. hence the #if COMPILER_MSVC 


                handmade_platform.h

                #if COMPILER_MSVC

                inline u32 GetThreadID(void)
                {
                    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
                    u32 ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);

                    return(ThreadID);
                }

                #else 

                #endif





35:57
Casey plans to improve the visualization tomorrw. He will use the rest of the time on 
cleaning up the debug code 

first he wants to get rid of the debug_frame_end_info. The only reason he was doing it is that there was no way 
to get these debug counters to work on the platform side of things. 

Casey wants to figure out a way to unify the platform debug code and regular debug code, such that we can use the counter
on both sides of the program. 

so the goal is move certain things from the handmade_debug.h to handmade_platform.h so that the platform layer can also use it 



we moved the Atomic operations, struct debug_frame_timestamp, struct debug_frame_end_info, struct debug_record,
struct timed_block and a bunch intrinsics all to handmade_platform.h



38:31
Casey also got rid of  some of the variables in timed_block.
As you can see, we are no longer recording the counter since we do that in debugEvents. 

as for the HitCount, Casey added a "TODO(casey): Record the hit count value here?" comment 


                handmade_platform.h

                struct timed_block
                {
                    int Counter;
                    
                    timed_block(int CounterInit, char *FileName, int LineNumber, char *FunctionName, u32 HitCountInit = 1)
                    {
                        // TODO(casey): Record the hit count value here?
                        
                        Counter = CounterInit;
                        debug_record *Record = GlobalDebugTable.Records[TRANSLATION_UNIT_INDEX] + Counter;
                        Record->FileName = FileName;
                        Record->LineNumber = LineNumber;
                        Record->FunctionName = FunctionName;

                        RecordDebugEvent(Counter, DebugEvent_BeginBlock);
                    }
                    
                    ~timed_block()
                    {
                        RecordDebugEvent(Counter, DebugEvent_EndBlock);
                    }
                };




40:31
the next problme is that for the RecordDebugEvent(); and timed_block(); function how do we find memory for these 
debug events and debug records to write in?

recall previously, we had global arrays in the game side. We need to refactor it so that both game side, and platform side 
can use it. 


                #define MAX_DEBUG_EVENT_COUNT (16*65536)
                extern u64 Global_DebugEventArrayIndex_DebugEventIndex;
                extern debug_event GlobalDebugEventArray[2][MAX_DEBUG_EVENT_COUNT];

                inline void RecordDebugEvent(int RecordIndex, debug_event_type EventType)
                {
                    ...
                    ...                       
    ------------>   debug_event *Event = GlobalDebugEventArray[ArrayIndex_EventIndex >> 32] + EventIndex; 
                    ...
                    ...
                }




                struct timed_block
                {
                    int Counter;
                    
                    timed_block(int CounterInit, char *FileName, int LineNumber, char *FunctionName, u32 HitCountInit = 1)
                    {
                        // TODO(casey): Record the hit count value here?
                        
                        Counter = CounterInit;
    --------------->    debug_record *Record = GlobalDebugTable.Records[TRANSLATION_UNIT_INDEX] + Counter;
                        Record->FileName = FileName;
                        Record->LineNumber = LineNumber;
                        Record->FunctionName = FunctionName;

                        RecordDebugEvent(Counter, DebugEvent_BeginBlock);
                    }
                    
                    ~timed_block()
                    {
                        RecordDebugEvent(Counter, DebugEvent_EndBlock);
                    }
                };


41:37
Casey proposes that they are always stored on the platform layer side. when the dll is loaded, we can make it so that 
these global values are intialized at startup time. 

Casey also wonders if dll bindings can patch the address of the global variables just fine

Casey start look up __declspec();


45:36
Casey defines a struct in handmade_platform.h. This way anyone can use this global debug table
regardless which side and which translation unit you are on. you can see that it has the memory for the debug_event and 
debug_record. This way anyone who uses it, will be writing their debug events/debug records into this debug table.

                handmade_platform.h

                #define MAX_DEBUG_TRANSLATION_UNITS 3
                #define MAX_DEBUG_EVENT_COUNT (16*65536)
                #define MAX_DEBUG_RECORD_COUNT (65536)
                struct debug_table
                {
                // TODO(casey): No attempt is currently made to ensure that the final
                // debug records being written to the event array actually complete
                // their output prior to the swap of the event array index.
                    u32 CurrentEventArrayIndex;
                    u64 volatile EventArrayIndex_EventIndex;
                    debug_event Events[2][MAX_DEBUG_EVENT_COUNT];
                    debug_record Records[MAX_DEBUG_TRANSLATION_UNITS][MAX_DEBUG_RECORD_COUNT];
                };

                extern debug_table GlobalDebugTable;

and we define an extern debug_table 


notice that we have the variable MAX_DEBUG_TRANSLATION_UNITS

                the debug_record[0] is for the handmade.cpp translation unit 
                the debug_record[1] is for the handmade_optimized.cpp translation unit 
                the debug_record[2] is for the win32_handmade.cpp translation unit 

the index that each of these TranslationUnit uses is defined in the build.bat file 


                cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=1 -O2 -I..\iaca-win64\ -c ..\handmade\code\handmade_optimized.cpp -Fohandmade_optimized.obj -LD
                cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=0 -I..\iaca-win64\ ..\handmade\code\handmade.cpp handmade_optimized.obj -Fmhandmade.map -LD /link -incremental:no -opt:ref -PDB:handmade_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender -EXPORT:DEBUGGameFrameEnd
                del lock.tmp
                cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=2 ..\handmade\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags%


in which we define this TRANSLATION_UNIT_INDEX variable



51:22
while in our global debug table, we have defined the MAX_DEBUG_RECORD_COUNT, but each TranslationUnit may not actually have that many 
TIMED_BLOCKs put it. the number of actual DebugRecords it will use is still defined in __COUNTER__

so in the handmade_debug.h 

we define the following:

                #define DebugRecords_Main_Count __COUNTER__

                debug_table GlobalDebugTable;


the idea is that we have MAX_DEBUG_RECORD_COUNT amount of memory, but when we iterate through our debug records array,
we will only iterate till DebugRecords_Main_Count

we also have the debug_table GlobalDebugTable. That is just referencing the the globa debug table defined in the 
handmade_platform.h




53:40
Casey renamed the debug_event struct DebugRecordArrayIndex to TranslationUnit

                handmade_platform.h 

                struct debug_event
                {
                    u64 Clock;
                    u16 ThreadIndex;
                    u16 CoreIndex;
                    u16 DebugRecordIndex;
                    u8 TranslationUnit;     <------------
                    u8 Type;
                };


                inline void RecordDebugEvent(int RecordIndex, debug_event_type EventType)
                {
                    u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable.EventArrayIndex_EventIndex, 1);
                    u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;                
                    Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);                         
                    debug_event *Event = GlobalDebugTable.Events[ArrayIndex_EventIndex >> 32] + EventIndex; 
                    Event->Clock = __rdtsc();                                           
                    Event->ThreadIndex = (u16)GetThreadID();
                    Event->CoreIndex = 0;                                               
                    Event->DebugRecordIndex = (u16)RecordIndex;                         
                    Event->TranslationUnit = TRANSLATION_UNIT_INDEX;       
                    Event->Type = (u8)EventType;
                }


55:55
in the CollateDebugRecords function, now we refactor it so that it will be gathering debug_records and 
debug events data from both the gameside and the win32 platform side side.


-   first we have the 

                DebugState->CounterCount = (DebugRecords_Main_Count +
                                                DebugRecords_Optimized_Count +
                                                DebugRecords_Platform_Count);
    
    the  #define DebugRecords_Platform_Count 0
    is just left there for the moment 

the rest of the code is just changed to accomodate that we are now reading and writing to a global debug_table

-   full code below

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *Events)
                {
                #define DebugRecords_Platform_Count 0
                    DebugState->CounterCount = (DebugRecords_Main_Count +
                                                DebugRecords_Optimized_Count +
                                                DebugRecords_Platform_Count);

                    for(u32 CounterIndex = 0; CounterIndex < DebugState->CounterCount; ++CounterIndex)
                    {
                        debug_counter_state *Dest = DebugState->CounterStates + CounterIndex;
                        Dest->Snapshots[DebugState->SnapshotIndex].HitCount = 0;
                        Dest->Snapshots[DebugState->SnapshotIndex].CycleCount = 0;
                    }

                    debug_counter_state *CounterArray[3] =
                    {
                        DebugState->CounterStates,
                        DebugState->CounterStates + DebugRecords_Main_Count,
                        DebugState->CounterStates + DebugRecords_Main_Count + DebugRecords_Optimized_Count,
                    };

                    for(u32 EventIndex = 0; EventIndex < EventCount; ++EventIndex)
                    {
                        debug_event *Event = Events + EventIndex;

                        debug_counter_state *Dest = CounterArray[Event->TranslationUnit] + Event->DebugRecordIndex;

                        debug_record *Source = GlobalDebugTable.Records[Event->TranslationUnit] + Event->DebugRecordIndex;
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

