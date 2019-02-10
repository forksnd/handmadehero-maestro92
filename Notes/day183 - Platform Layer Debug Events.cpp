Handmade Hero Day 183 - Platform Layer Debug Events

Summary:
brings back the traditional TIMED_BLOCK BEGIN(); and TIMED_BLOCK END(); style of profiling
also made the distinction of TIMED_FUNCTION();, TIMED_BLOCK(); and BEGIN(); and END(); to profile code.

tackled the problem of can the platform layer have access to the GlobalDebugTable memory.

Considered both options:
have the platform layer owns the debug_table memory, pass it into the game code.
have the game code own the debug_table memory, pass it back to the platform layer.

Curently went with the game code onwing the debug_table memory, passing it back to the platform layer approach.

addresses the issue that as we are doing multi-threading operations, we may have TIMED_BLOCK(); that span 
multiple frames. 

with our previous system, we may miss them or have incorrect timings. 

solved this issue by collecting multiple frames of debug data at the expense of more memory 

argues that since this is debug code, it wont run on users machine, so more memory for instrumentation is fine.

introduced the concept of FRAME_MARKER, which will help us set Frame boundaries after we collect multiple frames of data.


Keyword:
debug, profiling


5:00
Casey now putting TIMED_BLOCK(); in the win32 platform layer.
that is excatly why we did what we did in day 182, this way the platform layer can use TIMED_BLOCK(); as well
and we have unified debug system anywhere.

                win32_handmade.cpp 

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {  

                    while(GlobalRunning)
                    {
                        TIMED_BLOCK();

                        ...
                        ...
                    }
                }


5:15
as he plans to put TIMED_BLOCK(); everywhere, 
Casey realizes he wants to bring back the traditional BEGIN and END block pair. Imagine the case where you defined a variable 
within a block, but you want it to use it through the frame logic. 
Here If i defined an interger a, we want to be able to use it throughout the frame 


                win32_handmade.cpp 

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {  

                    while(GlobalRunning)
                    {
                        
                        {
                            TIMED_BLOCK();

                            int a = ...;
                        }



                        {
                            TIMED_BLOCK();
                            ..........................................
                            ......... do something with a ............
                            ..........................................
                        }
                    }
                }


-   full code below                


                #define BEGIN_BLOCK_(Counter, FileNameInit, LineNumberInit, BlockNameInit)          \
                    {debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
                    Record->FileName = FileNameInit;                                        \
                    Record->LineNumber = LineNumberInit;                                    \
                    Record->BlockName = BlockNameInit;                                   \
                    RecordDebugEvent(Counter, DebugEvent_BeginBlock);}
                #define END_BLOCK_(Counter) \
                    RecordDebugEvent(Counter, DebugEvent_EndBlock);
                    
                #define BEGIN_BLOCK(Name) \
                    int Counter_##Name = __COUNTER__;                       \
                    BEGIN_BLOCK_(Counter_##Name, __FILE__, __LINE__, #Name);

                #define END_BLOCK(Name) \
                    END_BLOCK_(Counter_##Name);


we also refactored the timed_block struct to use the same BEGIN_BLOCK and END_BLOCK

                win32_handmade.cpp
                    
                struct timed_block
                {
                    int Counter;
                    
                    timed_block(int CounterInit, char *FileName, int LineNumber, char *BlockName, u32 HitCountInit = 1)
                    {
                        // TODO(casey): Record the hit count value here?
                        Counter = CounterInit;
                        BEGIN_BLOCK_(Counter, FileName, LineNumber, BlockName);
                    }
                    
                    ~timed_block()
                    {
                        END_BLOCK_(Counter);
                    }
                };



11:12
then in our WinMain(); we do the following, we just put BEGIN_BLOCK and END_BLOCK everywhere

                int WinMain()
                {



                    while(GlobalRunning)
                    {
                        
                        BEGIN_BLOCK(ExecutableRefresh);
                        ...............................................
                        ......... Input Processing Logic ..............
                        ...............................................
                        END_BLOCK(ExecutableRefresh);



                        BEGIN_BLOCK(InputProcessing);
                        ...............................................
                        ......... Input Processing Logic ..............
                        ...............................................
                        END_BLOCK(InputProcessing);

                        ...
                        ...
                    }
                } 

20:06
Casey make the distinction of TIMED_BLOCK and TIMED_FUNCTION. so now if you want to time the whole function 
you put TIMED_FUNCTION at the top of your function.

so now there are three ways for you to profile some code 
TIMED_FUNCTION, TIMED_BLOCK or put TIMED_BLOCK or put BEGIN_BLOCK(); and END_BLOCK(); anywhere. 

so now we have
                handmade_platform.h

                #define TIMED_BLOCK__(BlockName, Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, BlockName, ## __VA_ARGS__)
                #define TIMED_BLOCK_(BlockName, Number, ...) TIMED_BLOCK__(BlockName, Number, ## __VA_ARGS__)
                #define TIMED_BLOCK(BlockName, ...) TIMED_BLOCK_(#BlockName, __LINE__, ## __VA_ARGS__)
                #define TIMED_FUNCTION(...) TIMED_BLOCK_(__FUNCTION__, __LINE__, ## __VA_ARGS__)

                #define BEGIN_BLOCK_(Counter, FileNameInit, LineNumberInit, BlockNameInit)          \
                    {debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
                    Record->FileName = FileNameInit;                                        \
                    Record->LineNumber = LineNumberInit;                                    \
                    Record->BlockName = BlockNameInit;                                   \
                    RecordDebugEvent(Counter, DebugEvent_BeginBlock);}
                #define END_BLOCK_(Counter) \
                    RecordDebugEvent(Counter, DebugEvent_EndBlock);
                    
                #define BEGIN_BLOCK(Name) \
                    int Counter_##Name = __COUNTER__;                       \
                    BEGIN_BLOCK_(Counter_##Name, __FILE__, __LINE__, #Name);

                #define END_BLOCK(Name) \
                    END_BLOCK_(Counter_##Name);


also debug_record now has BlockName instead of FunctionName

                handmade_platform.h

                struct debug_record
                {
                    char *FileName;
    ----------->    char *BlockName;
                    
                    u32 LineNumber;
                    u32 Reserved;

                    u64 HitCount_CycleCount;
                };




28:57
now what Casey needs to do is that we dont actually have the GlobalDebugTable when we are on the win32_handmade.cpp yet 
so when we load the .dll, we need someway of grabbing the global debug table from the system. 


right now the GlobalDebugTable is in the handmade_platform.h 

                handmade_platform.h 

                extern debug_table GlobalDebugTable;

we need some way of pointing to it. 
the problem is that Casey didnt make this GlobalDebugTable into a pointer. If it was a pointer, we can just point to it.

this is a bit unfortunate cuz that means we have some indirection instead of a constant location. 





35:55
so Casey considering that, when we load the game code, we can switch which debug_table we are writing into.

we first define a GlobalDebugTable in the platform layer 

                win32_handmade.cpp 

                global_variable debug_table GlobalDebugTable_;
                debug_table *GlobalDebugTable = &GlobalDebugTable_;


then, during the frame, Casey wants this table to point to the correct location.


                while(GlobalRunning)
                {
                    FRAME_MARKER();

                    ...
                    ...

                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32CompleteAllWork(&HighPriorityQueue);
                        Win32CompleteAllWork(&LowPriorityQueue);

                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                 TempGameCodeDLLFullPath,
                                                 GameCodeLockFullPath);
                        NewInput->ExecutableReloaded = true;
                    }

                    ...
                    ...
                }

so at the end of a frame, we will assign the GlobalDebugTable with what the game code returns. 

-   notice that we are also clearning our local GlobalDebugTable_. This way in case we unload our game code 
    and we switch back to our local one, it doesnt overflow.

                while(GlobalRunning)
                {
                    ...
                    ...

                    if(Game.DEBUGFrameEnd)
                    {
                        GlobalDebugTable = Game.DEBUGFrameEnd(&GameMemory);
                        // TODO(casey): Move this to a global variable so that
                        // there can be timers below this one?
                        GlobalDebugTable->RecordCount[TRANSLATION_UNIT_INDEX] = __COUNTER__;
                    }
                    GlobalDebugTable_.EventArrayIndex_EventIndex = 0;
                }


and Casey made it so that the DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd); function returns the address of GlobalDebugTable;

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    GlobalDebugTable->RecordCount[0] = DebugRecords_Main_Count;
                    GlobalDebugTable->RecordCount[1] = DebugRecords_Optimized_Count;

                    ...
                    ...

                    return(GlobalDebugTable);
                }






Casey also mentions that it will seem alot more sane if the debug_table lived on the platform layer side 
and the executable writes into that.  
there are also reasons why this approach wont work, for example the strings dont map correctly.

When the dll gets unloaded, all the strings all become invalid.
[didnt understand this part]



37:53
so when we turn unload our game code,
we will have the GlobalDebugTable point back to our own GlobalDebugTable_



                while(GlobalRunning)
                {
                    FRAME_MARKER();

                    //
                    //
                    //
                    
                    BEGIN_BLOCK(ExecutableRefresh);
                    NewInput->dtForFrame = TargetSecondsPerFrame;

                    NewInput->ExecutableReloaded = false;                    
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32CompleteAllWork(&HighPriorityQueue);
                        Win32CompleteAllWork(&LowPriorityQueue);

    --------------->    GlobalDebugTable = &GlobalDebugTable_;
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                 TempGameCodeDLLFullPath,
                                                 GameCodeLockFullPath);
                        NewInput->ExecutableReloaded = true;
                    }
                    
                    ...
                    ...
                }




41:15
Now to print/render the TIMED_BLOCK(); from the platform layer, we want to the CollateDebugRecords();
start to graph DebugRecords from the platform layer as well 

-   we added an extra for loop where we tally up the Total DebugRecords count from all translationUnits 

-   full code below:
                handmade_debug.cpp 

                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *Events)
                {
                    debug_counter_state *CounterArray[MAX_DEBUG_TRANSLATION_UNITS];
                    debug_counter_state *CurrentCounter = DebugState->CounterStates;
                    u32 TotalRecordCount = 0;
                    for(u32 UnitIndex = 0; UnitIndex < MAX_DEBUG_TRANSLATION_UNITS; ++UnitIndex)
                    {
                        CounterArray[UnitIndex] = CurrentCounter;
                        TotalRecordCount += GlobalDebugTable->RecordCount[UnitIndex];

                        CurrentCounter += GlobalDebugTable->RecordCount[UnitIndex];
                    }
                    DebugState->CounterCount = TotalRecordCount;


                    for(u32 CounterIndex = 0; CounterIndex < DebugState->CounterCount; ++CounterIndex)
                    {
                        .........................................................
                        ........... clearing the debug_counter_state ............
                        .........................................................
                    }

                    for(u32 EventIndex = 0; EventIndex < EventCount; ++EventIndex)
                    {
                        ........................................................................
                        ........... Copying debug event data to debug_counter_state ............
                        ........................................................................
                    }
                }




46:43
Casey mentioned that it seems that the __COUNTER__ technique is a little bit more trouble than its worth 
becuz we do have more than one translation unit, it ends up not being as clean as Casey likes. 

we didnt do the dynamic code loading, we can the windowed optimization where we just have one translation unit. 
it would have been a lot nicer. 

its okay though, cuz everytime you find a new technique, you dont know how effective it is going to be. 
you dont know until you try it.



49:27 55:34
Now that the platform layer and the game code are using the same debug system, Casey moves on to do other things.
the first thing he wants to do is to introduce this concept of Frame latch, so we can tell where the frame boundary 
actually occur. 

visually, you can think of it as below, you have the FRAME_MARKER, and you have BEGIN_BLOCK and END_BLOCK pairs 

some might even span across Frame boundaries: namely multi-threading work

            ||                               ||
            ||   |_____________|             ||
            ||           |______________|    ||
            ||                  |__|         ||
                                    |______________________|


        FRAME_MARKER                    FRAME_MARKER


so the idea is that we kind of want keep the log rolling, so that we can view operations that span across frame boundaries

meaning, currently, we are gathering numbers one frame at a time, and we give to debug memory storage

so what Casey proposes is to write a lot longer than a frame, so we get many frames of data. 

we just need a lot of memory for this operation. 




so Casey added a new type of debug_event_type: DebugEvent_FrameMarker

                handmade_platform.h 

                enum debug_event_type
                {
                    DebugEvent_FrameMarker,
                    DebugEvent_BeginBlock,
                    DebugEvent_EndBlock,
                };

this is a gonna special marker 


51:58
So Casey added this FRAME_MARKER function;

                handmade_platform.h 

                #define FRAME_MARKER() \
                     { \
                     int Counter = __COUNTER__; \
                     RecordDebugEvent(Counter, DebugEvent_FrameMarker); \
                     debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
                     Record->FileName = __FILE__;                                        \
                     Record->LineNumber = __LINE__;                                    \
                     Record->BlockName = "Frame Marker";                                   \
                } 



and we start to use it in our code 

                win32_handmade.cpp 

                    while(GlobalRunning)
                    {
                        FRAME_MARKER();

                        ...
                        ...
                    }


59:12 
so in our struct debug_table 

for our debug_event Array, we previously had 
                
                debug_event Events[2][MAX_DEBUG_EVENT_COUNT];

which we 2 debug_event arrays cuz we do the pingpong buffer.
now that we want to gather more data, Casey changed it to 64, which means we have 63 frames worth of 
debug_event to look back at 

it is writing to the 64 frame, and it can look back at 63 frames.

                handmade_platform.h

                struct debug_table
                {
                // TODO(casey): No attempt is currently made to ensure that the final
                // debug records being written to the event array actually complete
                // their output prior to the swap of the event array index.
                    u32 CurrentEventArrayIndex;
                    u64 volatile EventArrayIndex_EventIndex;
                    debug_event Events[64][MAX_DEBUG_EVENT_COUNT];
                    debug_record Records[MAX_DEBUG_TRANSLATION_UNITS][MAX_DEBUG_RECORD_COUNT];
                };





59:41
so when we get to CurrentEventArrayIndex, we will just do a ++CurrentEventArrayIndex.
and we do a check if to see if it reaches the array count, we can set it to 0.

recall previously, we are doing the poingpong buffer, so we are just toggling it 

                GlobalDebugTable.CurrentEventArrayIndex = !GlobalDebugTable.CurrentEventArrayIndex;


-   full code below:

                handmade_debug.cpp

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    GlobalDebugTable->RecordCount[0] = DebugRecords_Main_Count;
                    GlobalDebugTable->RecordCount[1] = DebugRecords_Optimized_Count;
                    
    ----------->    ++GlobalDebugTable->CurrentEventArrayIndex;
                    if(GlobalDebugTable->CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable->Events))
                    {
                        GlobalDebugTable->CurrentEventArrayIndex = 0;
                    }
                    
                    u64 ArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugTable->EventArrayIndex_EventIndex,
                                                                  (u64)GlobalDebugTable->CurrentEventArrayIndex << 32);

                    u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
                    u32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;

                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState)
                    {
                        DebugState->CounterCount = 0;
                        CollateDebugRecords(DebugState, EventCount, GlobalDebugTable->Events[EventArrayIndex]);
                        
                        ++DebugState->SnapshotIndex;
                        if(DebugState->SnapshotIndex >= DEBUG_SNAPSHOT_COUNT)
                        {
                            DebugState->SnapshotIndex = 0;
                        }
                    }

                    return(GlobalDebugTable);
                }



1:00:58
Casey feels like this is the right way do go about, to take a huge memory footprint to record the data.
Again, this is not something we will run on the end user_s machine cuz this is debug stuff. 

so we will use a bunch of memory.

for code that is meant for instrumentation, you usually dont really care about memory.

now that we have this tech, what this means is that, we can look up however many frames we want, and we can find the frame boundary.
This way we can capture timings that span across frames.

previously when we are taking debug_records and debug_events one frame a time, if a TIMED_BLOCK span across frames,
we might miss these edge cases (or have incorrect timings);.

so this allows us timings that exist out side of a single frame.



Q/A:
1:04:35
when do you think its a good idea to use someone elses library vs implementing your own?

when you think its gonna be as good as you implemented. Thats why he uses STB libraries, he thinks 
Sean Barret_s code is gonna be like his code

