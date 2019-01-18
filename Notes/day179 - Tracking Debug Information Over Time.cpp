Handmade Hero Day 179 - Tracking Debug Information Over Time

Summary:
addresses the debug records and rendering TIMED_BLOCKS tangling problem.
elevated the "read from debug records" to the platform layer

added a debug_frame_end_info struct in the platform layer
took down timings for multiple operations in within the frame in the platform layer

solved the debug reocrds and rendering TIMED_BLOCKS tangling issue 
by having the debug_records array copied its data to a debug memory storage, then we print/render 
debug data from the debug memory storage.

added support for gathering statistics from debug data in the debug memory storage


Keyword:
Frames per second, debug system, memory


10:48
Casey mentioned that currently in our OverlayCycleCounters(); 




11:22
So just like what we mentioned in day 178, currently in our OverlayCycleCounters();

we are calling DEBUGTextLine();
if we put a TIMED_BLOCK in DEBUGTextLine(); that messes up the timing numbers and TIMED_BLOCKs.

Meaning currently when we go through the debugRecords and we pull the data out, and we do the reset, it is happening a part of the render
or the render is happening a part of the debug reading. (however you want to look at it, one contains the other)

so Casey wants to change that so that we store the debugRecords some other storage, 
and rendering will just rendered from that other storage.



12:42
Casey mentions that we would want this debug system to visualize some things that largely happening out side of the platform
independent code. (meaning we want to visualize platform spceific stuff);

for example, we may want windows or linux to be able to put information into the system, thats about the current frame that we are on.


so we would to elevate the action of "read from debug records" to something that the platform layer calls. 

So that we can gurantee that the platform will conduct this "read from debug records" at a time when 
the platform knows it has finished doing all of the stuff in that frame 

that way, we dont need to worry about the ordering problem of debug_read vs rendering



14:24
so in Casey proceeding to make that change in win32_handmade.cpp.
Essentially at the end of our frame, we would like to do this debug read.

so previously, we were doing something similar. Only that we were printing them out to the console

                win32_handmade.cpp

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {
                    ...
                    ...
                
                    while(GlobalRunning)
                    {

                        uint64 EndCycleCount = __rdtsc();
                        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;
                    
                        real64 FPS = 0.0f;
                        real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));

                        char FPSBuffer[256];
                        _snprintf_s(FPSBuffer, sizeof(FPSBuffer),
                                    "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
                        OutputDebugStringA(FPSBuffer);

                        ...
                        ...

                        ++DebugTimeMarkerIndex;
                        if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
                        {
                            DebugTimeMarkerIndex = 0;
                        }
                    }

                    ...
                    ...
                }


now Casey wants to have something like  


                win32_handmade.cpp

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {
                    ...
                    ...
                
                    while(GlobalRunning)
                    {
                        ...............................................
                        .......... All my work in my Frame ............
                        ...............................................


                        Game.DEBUGFrameEnd(&GameMemory, MSPerFrame, FPS, MCPF);

                        ++DebugTimeMarkerIndex;
                        if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
                        {
                            DebugTimeMarkerIndex = 0;
                        }
                    }

                    ...
                    ...
                }



16:04
Casey starting to revisit his MSPerFrame code. 
previously, the way we calculate MSPerFrame is 


                win32_handmade.cpp

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {
                    ...
                    ...
                
                    while(GlobalRunning)
                    {

                        ......................................................
                        ............. Generating our Frame........ ...........
                        ......................................................


                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        real32 MSPerFrame = 1000.0f*Win32GetSecondsElapsed(LastCounter, EndCounter);                    
                        LastCounter = EndCounter;
                

                        ......................................................
                        ....... Windows Win32DisplayBufferInWindow ...........
                        ......................................................


                        uint64 EndCycleCount = __rdtsc();
                        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;
                    
                        real64 FPS = 0.0f;
                        real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));

                        char FPSBuffer[256];
                        _snprintf_s(FPSBuffer, sizeof(FPSBuffer),
                                    "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
                        OutputDebugStringA(FPSBuffer);
                    }

                    ...
                    ...
                }

Casey mentioned that, now we want to do multiple timings within a frame. 



18:25
So Casey proceeds to write the debug_frame_end_info struct, to help with that

                handmade_platform.h

                struct debug_frame_end_info
                {
                    r32 ExecutableReady;
                    r32 InputProcessed;
                    r32 GameUpdated;
                    r32 AudioUpdated;
                    r32 FramerateWaitComplete;
                    r32 EndOfFrame;
                };


19:15
so now we initalize one of those at the beginning of a frame 
and we set the timings during the frame.

-   also notice that, once FrameEndInfo has all the right info, we pass it to our Game.DEBUGFrameEnd(); function.
    althought we havent written this function, but we are just designing our API 

                if(Game.DEBUGFrameEnd)
                {
                    Game.DEBUGFrameEnd(&GameMemory, &FrameEndInfo);
                }


-   fullcode below:
                win32_handmade.cpp

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {
                    ...
                    ...
                
                    while(GlobalRunning)
                    {
                        debug_frame_end_info FrameEndInfo = {};

                        ......................................................
                        ............. Loading our executable .................
                        .....................................................

                        FrameEndInfo.ExecutableReady = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());  



                        .......................................................
                        ............. Processing our Inputs ...................
                        .......................................................

                        FrameEndInfo.InputProcessed = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());  


                        ...............................................................
                        ............. Game Update, Frame Generation ...................
                        ...............................................................

                        FrameEndInfo.InputProcessed = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());  


                        ...............................................................
                        ................... Updating the Audio ........................
                        ...............................................................

                        FrameEndInfo.InputProcessed = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());  


                        ................................................................................
                        ..................... Waiting For Frame to Finish, .............................
                        ..................... Sleep(); till Frame time is up ...........................
                        ................................................................................

                        FrameEndInfo.FramerateWaitComplete = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());                    
                

                        ................................................................................
                        ..................... Windows Calling Win32DisplayBufferInWindow ...............
                        ................................................................................

                        FrameEndInfo.EndOfFrame = Win32GetSecondsElapsed(LastCounter, EndCounter);

                        uint64 EndCycleCount = __rdtsc();
                        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

                        if(Game.DEBUGFrameEnd)
                        {
                            Game.DEBUGFrameEnd(&GameMemory, &FrameEndInfo);
                        }
                    }
                }



27:32
Casey defining the DEBUGFrameEnd(); function 
                
                #define DEBUG_GAME_FRAME_END(name) void name(game_memory *Memory, debug_frame_end_info *Info)
                typedef DEBUG_GAME_FRAME_END(debug_game_frame_end);


28:58
of course we add it to our dispatch table 

                handmade_platform.h

                struct win32_game_code
                {
                    HMODULE GameCodeDLL;
                    FILETIME DLLLastWriteTime;

                    // IMPORTANT(casey): Either of the callbacks can be 0!  You must
                    // check before calling.
                    game_update_and_render *UpdateAndRender;
                    game_get_sound_samples *GetSoundSamples;
                    debug_game_frame_end *DEBUGFrameEnd;        <----------------
                    
                    bool32 IsValid;
                };


30:37
Then in Win32LoadGameCode();
we also load the DEBUGFrameEnd(); function


                win32_handmade.cpp

                internal win32_game_code Win32LoadGameCode(char *SourceDLLName, char *TempDLLName, char *LockFileName)
                {
                    win32_game_code Result = {};

                    ...

                    if(!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
                    {
                        ...
                        ...
                    
                        Result.GameCodeDLL = LoadLibraryA(TempDLLName);
                        if(Result.GameCodeDLL)
                        {
                            Result.UpdateAndRender = (game_update_and_render *)
                                GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
                        
                            Result.GetSoundSamples = (game_get_sound_samples *)
                                GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
                        
    ------------------->    Result.DEBUGFrameEnd = (debug_game_frame_end *)
                                GetProcAddress(Result.GameCodeDLL, "DEBUGGameFrameEnd");

                            Result.IsValid = (Result.UpdateAndRender &&
                                              Result.GetSoundSamples);
                        }
                    }

                    ...
                    ...

                    return(Result);
                }





34:04
Casey starting to define the DEBUG_GAME_FRAME_END(); function.
So the idea is that, at the end of our frame, we will call this DEBUGFrameEnd(); this will be updating our debug records.
we will be reading from the debug_record array to our debug memory storage.



                handmade.cpp

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    UpdateDebugRecords(DebugState, DebugRecords_Optimized_Count, DebugRecords_Optimized);
                    UpdateDebugRecords(DebugState, ArrayCount(DebugRecords_Main), DebugRecords_Main);
                }



then in the middle of our GameUpdateAndRender();, (so in middle of our frame);, we will be rendering our debug 
information out from our debug memory storage. Essentially printing the data out of 
debug memory storage (we mentioned this at the beginning of our stream);, instead of debug_records array.

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    OverlayCycleCounters();

                    ...
                    ...
                }




36:53
first thing we need to do is get some debug memory out of the system.

so we added some debug storage to the game_memory

                handmade_platform.h

                typedef struct game_memory
                {
                    uint64 PermanentStorageSize;
                    void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

                    uint64 TransientStorageSize;
                    void *TransientStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

                    uint64 DebugStorageSize;
    ----------->    void *DebugStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

                    platform_work_queue *HighPriorityQueue;
                    platform_work_queue *LowPriorityQueue;

                    platform_api PlatformAPI;
                } game_memory;




47:16
and of course, we have to give it memory, we gave it 64 MB of debug memory.

                int CALLBACK
                WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {
                    ...
                    ...

                    game_memory GameMemory = {};
                    ...
                    ...
                    GameMemory.DebugStorageSize = Megabytes(64);

                    ...
                    Win32State.TotalSize = (GameMemory.PermanentStorageSize +
                                            GameMemory.TransientStorageSize +
                                            GameMemory.DebugStorageSize);
                    GameMemory.DebugStorage = ((u8 *)GameMemory.TransientStorage +
                                               GameMemory.TransientStorageSize);
                    ...
                    ...
                }


38:34
then in the handmade_debug.h, we define the struct that we will store our debug_records in.

                struct debug_counter_snapshot
                {
                    u32 HitCount;
                    u32 CycleCount;
                };

                #define DEBUG_SNAPSHOT_COUNT 120
                struct debug_counter_state
                {
                    char *FileName;
                    char *FunctionName;
                    
                    u32 LineNumber;
                    
                    debug_counter_snapshot Snapshots[DEBUG_SNAPSHOT_COUNT];
                };

                struct debug_state
                {
                    u32 SnapshotIndex;
                    u32 CounterCount;
                    debug_counter_state CounterStates[512];
                };

essentially we have 

                debug_counter_state[0]  ------->    TIMED_BLOCK at GameUpdateAndRender(); 

                                                    Snapshots[0]    =   HitCount and CycleCount
                                                    Snapshots[1]    =   HitCount and CycleCount
                                                    ...
                                                    ...


                debug_counter_state[1]  ------->    TIMED_BLOCK at PushBitmap();

                                                    Snapshots[0]    =   HitCount and CycleCount
                                                    Snapshots[1]    =   HitCount and CycleCount
                                                    ...
                                                    ...
                
                debug_counter_state[2]  ------->    TIMED_BLOCK at DrawingRectangleSlowly();

                                                    Snapshots[0]    =   HitCount and CycleCount
                                                    Snapshots[1]    =   HitCount and CycleCount
                                                    ...
                                                    ...

                ...
                ...


so the debug_state contains an array of CounterStates.

each debug_counter_state describes the data from each TIMED_BLOCK over time.

that is contained in the debug_counter_state.Snapshots variable
each snapshot holds the value for the HitCount and CycleCount



45:38
in the UpdateDebugRecords(); function, we want to port data from the debug_record array to our debug memory storage
we are just reading the data from the array, and putting it in the correct snapshot.

-   as you can see, we are grabbing all the other relevant information 

                    Dest->FileName = Source->FileName;
                    Dest->FunctionName = Source->FunctionName;
                    Dest->LineNumber = Source->LineNumber;
                    Dest->Snapshots[DebugState->SnapshotIndex].HitCount = (u32)(HitCount_CycleCount >> 32);
                    Dest->Snapshots[DebugState->SnapshotIndex].CycleCount = (u32)(HitCount_CycleCount & 0xFFFFFFFF);

-   notice that we are still doing the AtomicExchangeU64(); on HitCount_CycleCount, since it is shared among threads.

-   full code below

                handmade.cpp

                internal void UpdateDebugRecords(debug_state *DebugState, u32 CounterCount, debug_record *Counters)
                {
                    for(u32 CounterIndex = 0;
                        CounterIndex < CounterCount;
                        ++CounterIndex)
                    {
                        debug_record *Source = Counters + CounterIndex;
                        debug_counter_state *Dest = DebugState->CounterStates + DebugState->CounterCount++;

                        u64 HitCount_CycleCount = AtomicExchangeU64(&Source->HitCount_CycleCount, 0);
                        Dest->FileName = Source->FileName;
                        Dest->FunctionName = Source->FunctionName;
                        Dest->LineNumber = Source->LineNumber;
                        Dest->Snapshots[DebugState->SnapshotIndex].HitCount = (u32)(HitCount_CycleCount >> 32);
                        Dest->Snapshots[DebugState->SnapshotIndex].CycleCount = (u32)(HitCount_CycleCount & 0xFFFFFFFF);
                    }
                }



41:05
then now in the OverlayCycleCounters(); just like before,
we first read from the first Snapshots, and print them out.

                handmade.cpp 

                internal void OverlayCycleCounters(game_memory *Memory)
                {
                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState)
                    {
                        for(u32 CounterIndex = 0;
                            CounterIndex < DebugState->CounterCount;
                            ++CounterIndex)
                        {
                            debug_counter_state *Counter = DebugState->CounterStates + CounterIndex;

                            u32 HitCount = Counter->Snapshots[0].HitCount;
                            u32 CycleCount = Counter->Snapshots[0].CycleCount;


                            if(HitCount)
                            {
                                .........................................
                                ........ printing them out ..............
                                .........................................
                            }

                        }
                    }
                }

and now we have recovered our debug printing functionality, only that the structure is of our debug system differently now,
and we have solved our debug system and rendering TIMED_BLOCK tangling issue



49:42
now that we are outputting our data in the first snapshot, we want to do rolling snapshot index 
in the DEBUGGameFrameEnd function, we just keep on incrementing the SnapshotIndex. once we read the end, we go back to 0

                handmade.cpp

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState)
                    {
                        DebugState->CounterCount = 0;
                        UpdateDebugRecords(DebugState, DebugRecords_Optimized_Count, DebugRecords_Optimized);
                        UpdateDebugRecords(DebugState, ArrayCount(DebugRecords_Main), DebugRecords_Main);

                        ++DebugState->SnapshotIndex;
                        if(DebugState->SnapshotIndex >= DEBUG_SNAPSHOT_COUNT)
                        {
                            DebugState->SnapshotIndex = 0;
                        }
                    }
                }


51:01
Casey coming back to fix the OverlayCycleCounters so that is not just printing from the 0th snapshot.
instead, Casey will do the brute force way to gather statistics
                
-   its literally going through all the snapshot and accumulating the numbers.
    nothing surprising.


                handmade.cpp

                internal void OverlayCycleCounters(game_memory *Memory)
                {
                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState)
                    {
                        for(u32 CounterIndex = 0; CounterIndex < DebugState->CounterCount; ++CounterIndex)
                        {
                            debug_counter_state *Counter = DebugState->CounterStates + CounterIndex;

                            debug_statistic HitCount, CycleCount, CycleOverHit;
                            BeginDebugStatistic(&HitCount);
                            BeginDebugStatistic(&CycleCount);
                            BeginDebugStatistic(&CycleOverHit);
                            for(u32 SnapshotIndex = 0; SnapshotIndex < DEBUG_SNAPSHOT_COUNT; ++SnapshotIndex)
                            {
                                AccumDebugStatistic(&HitCount, Counter->Snapshots[SnapshotIndex].HitCount);
                                AccumDebugStatistic(&CycleCount, Counter->Snapshots[SnapshotIndex].CycleCount);

                                r64 HOC = 0.0f;
                                if(Counter->Snapshots[SnapshotIndex].HitCount)
                                {
                                    HOC = ((r64)Counter->Snapshots[SnapshotIndex].CycleCount /
                                           (r64)Counter->Snapshots[SnapshotIndex].HitCount);
                                }
                                AccumDebugStatistic(&CycleOverHit, HOC);
                            }
                            EndDebugStatistic(&HitCount);
                            EndDebugStatistic(&CycleCount);
                            EndDebugStatistic(&CycleOverHit);
                            
                            if(HitCount.Max > 0.0f)
                            {
                #if 1
                                char TextBuffer[256];
                                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                            "%32s(%4d): %10ucy %8uh %10ucy/h",
                                            Counter->FunctionName,
                                            Counter->LineNumber,
                                            (u32)CycleCount.Avg,
                                            (u32)HitCount.Avg,
                                            (u32)CycleOverHit.Avg);
                                DEBUGTextLine(TextBuffer);
                #endif
                            }
                        }
                //    DEBUGTextLine("\\5C0F\\8033\\6728\\514E");
                //    DEBUGTextLine("111111");
                //    DEBUGTextLine("999999");
                //    DEBUGTextLine("AVA WA Ta");
                    }
                }


the statistics related functions are just shown below. Pretty straight forward stuff.    


                handmade.cpp

                struct debug_statistic
                {
                    r64 Min;
                    r64 Max;
                    r64 Avg;
                    u32 Count;
                };
                inline void
                BeginDebugStatistic(debug_statistic *Stat)
                {
                    Stat->Min = Real32Maximum;
                    Stat->Max = -Real32Maximum;
                    Stat->Avg = 0.0f;
                    Stat->Count = 0;
                }

                inline void
                AccumDebugStatistic(debug_statistic *Stat, r64 Value)
                {
                    ++Stat->Count;
                    
                    if(Stat->Min > Value)
                    {
                        Stat->Min = Value;
                    }

                    if(Stat->Max < Value)
                    {
                        Stat->Max = Value;
                    }

                    Stat->Avg += Value;
                }

                inline void
                EndDebugStatistic(debug_statistic *Stat)
                {
                    if(Stat->Count)
                    {
                        Stat->Avg /= (r64)Stat->Count;
                    }
                    else
                    {
                        Stat->Min = 0.0f;
                        Stat->Max = 0.0f;
                    }
                }


55:14
initially, Casey had struct debug_statistic to be 
     
                handmade.cpp

                struct debug_statistic
                {
                    u32 Min;
                    u32 Max;
                    u32 Avg;
                    u32 Count;
                };

but then casey said, we will just do these in double, becuz this is debug code, we dont care too much about its speed.
and we wont include them in the shipped build. so we will do them in doubles



1:13:30
some guy in the Q/A asked about entity composition architecture.
I have a player struct that contains a various other structs that represents what the player is.
for example, any entity that has movement struct will have the transform struct.


Casey says Composition is not particularly useful unless the compositions dont have to interact with each other.
usually the interesting parts come from if they have to interact with each other. Casey will get to that later on 
in handmade hero. 



1:14:28
for writing game engines, how much should a programmer know about how a compiler works?

it depends a lot on the game. if the game has performance implications, then knowing how the compiler works could be
important. also for portability, its important to know a few things, but much less so.

the easiest way to learn about compilers, is get a debugger, you right click, and you go to disassembly.
that tells you what the compiler did.


1:20:48
Casey giving his opinions on white board interviews
