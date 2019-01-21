Handmade Hero Day 187 - Fixing an Event Recording Bug

Summary:
fixed the bug from day 185. From the help from the forums, turns out its becuz the compiler and linker
is compiling some versions of the RecordDebugEvent(); out cuz it was marked as an inline function for different 
translation units. 

the fix was it change RecordDebugEvent(); to a macro.

added a HANDMADE_PROFILE flag so we can compile the TIMED_RECORDS out 

moved the FRAME_MARKER debugEvent to the end of a frame, we passed the frame time measured 
by QueryPerformanceCounter to the debug system.

measured and found out that debug data collation is taking a lot of time.

starting to display frame time in the top right corner  

Keyword:
profiling, debug system 


3:02
so in summary of the bug we have had from day 185. We get a pretty valid profile happening in the first few frames.
our debug data bar charts seems to render correctly. Then after that initial wave, we dont see any more debug data.

in day 186, we tried to track down the bug, and what it was is that in the CollateDebugRecords(); function, 
we were keeping track of BEGIN blocks and END blocks 

and after that initial wave, it seems that we get a bunch of BEGIN blocks with no matching END blocks.





8:15
what the online forums was saying that the we were tripped up by our translation unit index.

Casey mentions that he was trying this new technique of using the __COUNTER__ system to create a unique id
for each of our debug locations

and also we were giving each translation unit a translation unit index


9:50 
so Casey initially made RecordDebugEvent(); a macro, but later switched it into an inline function.
cuz its easier to step through in the debugger (its hard to step through a macro);

                handmade_platform.h

                inline void RecordDebugEvent(int RecordIndex, debug_event_type EventType)
                {
                    u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1);
                    u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;                
                    Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);                         
                    debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex; 
                    Event->Clock = __rdtsc();
                    u32 ThreadID = GetThreadID();
                    Event->ThreadID = (u16)ThreadID;
                    Event->CoreIndex = 0;                                               
                    Event->DebugRecordIndex = (u16)RecordIndex;                         
                    Event->TranslationUnit = TRANSLATION_UNIT_INDEX;       
                    Event->Type = (u8)EventType;
                }


apparently theres a problem with that, which is that if it was an inline function,
if we are not building it with inlining on everywhere, the compiler is free to not inline it.

if the compiler decides not to inline this function, what happens is that, there will be a version of it 
generated in every translation unit that calls this. 

in our case, we have 3 translation unit, and all of them calls this RecordDebugEvent(); function. So we have 
3 version of it. However, when it comes time to link, the linker is only gonna link one of them. cuz thats how the linker 
work. 

so essentially, our code is only every using one of the translation unit.


13:05
to test that theory, Casey added an Asset(); in the CollateDebugRecords();. We want to see if we were only seeing one 
translation unit ever. We have TranslationUnit 0, 1, 2. 

                Asset(Event->TranslationUnit != 0);

                Asset(Event->TranslationUnit != 1);

                Asset(Event->TranslationUnit != 2);


15:01
so Casey wants to see if we compile handmade_optimized.cpp in debug mode, whether do we reproduce the bug.
and we dont repo in debug mode. 


16:15
So Casey went inside DrawRectangleQuickly(); to debug the TIMED_BLOCK to see what is happening. 



18:05
Casey also noticed that while debugging the RecordDebugEvent(); we are in a function, meaning this function did not get inlined.
so it is highly possible that multiple versions of CollateDebugRecords(); got collapsed


19:09
Casey sees that in DrawRectangleQuickly(); it is getting translation unit index 0, which is incorrect.
DrawRectangleQuickly(); is in the handmade_optimized.cpp translation unit, and in the build.bat script, we gave its
TranslationUnitIndex to be 1.



19:35
the trivial fix for this is making RecordDebugEvent(); a macro

and that surely does fix the problem


21:11
Casey would like to mention that, using the __COUNTER__ is probably not worth it.
as mentioned, it was a technique he was trying, he was already seeing lots of annoying things about it.
with this bug, it was the last straw for him to abondon this techinique here.

he just thinks this is no reliable solution. 

we dont have to fix it for now, 

so Casey added a TODO 
                "
                // TODO(casey): I would like to switch away from the translation unit indexing
                // and just go to a more standard one-time hash table because the complexity
                // seems to be causing problems
                "



24:21
back to visualizing our debug data.
first thing Casey wants to do is that if the regions cant be seen, we wont produce that region



26:45
Casey wants some way to be able to compile out the TIMED_BLOCK code

so Casey added a HANDMADE_PROFILE flag 

                handmade_platform.h

----------->    #if HANDMADE_PROFILE

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
                    
                struct timed_block
                {
                    ...
                    ...
                };

----------->    #else

                #define TIMED_BLOCK(BlockName, ...) 
                #define TIMED_FUNCTION(...) 
                #define BEGIN_BLOCK(Name)
                #define END_BLOCK(Name)

                #endif

so if HANDMADE_PROFILE is not defined, the TIMED_BLOCK, TIMED_FUNCTION, BEGIN_BLOCK and END_BLOCK just expands to nothing.





29:29
So with HANDMADE_PROFILE turned on, Casey mentioned that our game is running extremely slow.
but our profiling debug data doesnt suppor that.

recall that we draw that white line, which is the target frame rate of 30 seconds. the rendered bars only
just exceed that white line. so either we are rendering the debug data incorrectly, or something is up. 

[or we are just taking that long collating our data? 
The time that takes to Collate the data is not shown in the debug results]




31:36
so one thing that Casey did to show the framerate
On the win32 layer, Casey is thinking about passing the Frame Elapsed time; to the debug layer. 
then the debug system can validate its __rdtsc(); measurements against that wall clock time.


Casey also mentioned that another thing we can do is to record the __rdtsc(); total for the whole block,
and we store that in the FRAME_MARKER debug events


33:07 
so in the debug_event, for the FRAME_MARKER, it doesnt care about the ThreadID and CoreIndex, we put an union 


                handmade_platform.h

                struct threadid_coreindex
                {
                    u16 ThreadID;
                    u16 CoreIndex;
                };
                struct debug_event
                {
                    u64 Clock;
                    union
                    {
                        threadid_coreindex TC;
                        r32 SecondsElapsed;
                    };
                    u16 DebugRecordIndex;
                    u8 TranslationUnit;
                    u8 Type;
                };


this way, the BEGIN and END debug events would use the TC. the FRAME_MARKER debug_event uses the SecondsElapsed.

note that Win32GetWallClock(); just uses the QueryPerformanceCounter.

also we have put the FRAME_MARKER at the end of our frame.

                WinMain()
                {
                    LARGE_INTEGER LastCounter = Win32GetWallClock();
                    while(GlobalRunning)
                    {
                        ...
                        ...

                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        FRAME_MARKER(Win32GetSecondsElapsed(LastCounter, EndCounter));
                        LastCounter = EndCounter;

                    }
                }


41:19
Casey changing the FRAME_MARKER(); function



                handmade_platform.h

                #define FRAME_MARKER(SecondsElapsedInit) \
                     { \
                     int Counter = __COUNTER__; \
                     RecordDebugEventCommon(Counter, DebugEvent_FrameMarker); \
                     Event->SecondsElapsed = SecondsElapsedInit; \
                     debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
                     Record->FileName = __FILE__;                                        \
                     Record->LineNumber = __LINE__;                                    \
                     Record->BlockName = "Frame Marker";                                   \
                } 


to make the RecordDebugEvent work with both FRAME_MARKER and BEGIN_BLOCK(); and END_BLOCK();
Casey defined a RecordDebugEventCommon(); function, 

                #define RecordDebugEventCommon(RecordIndex, EventType) \
                        u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
                        u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;            \
                        Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);                     \
                        debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex; \
                        Event->Clock = __rdtsc();                       \
                        Event->DebugRecordIndex = (u16)RecordIndex;                     \
                        Event->TranslationUnit = TRANSLATION_UNIT_INDEX;                \
                        Event->Type = (u8)EventType;                                    

                #define RecordDebugEvent(RecordIndex, EventType)        \
                    {                                                   \
                        RecordDebugEventCommon(RecordIndex, EventType); \
                        Event->TC.CoreIndex = 0;                                           \
                        Event->TC.ThreadID = (u16)GetThreadID();           \
                    }

                #define FRAME_MARKER(SecondsElapsedInit) \
                     { \
                     int Counter = __COUNTER__; \
                     RecordDebugEventCommon(Counter, DebugEvent_FrameMarker); \
                     Event->SecondsElapsed = SecondsElapsedInit; \
                     debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
                     Record->FileName = __FILE__;                                        \
                     Record->LineNumber = __LINE__;                                    \
                     Record->BlockName = "Frame Marker";                                   \
                } 

this way FRAME_MARKER(); and BEGIN_BLOCK(); and END_BLOCK(); shares as much stuff as possible, but still does its specific code.




45:02
Casey added a "r32 WallSecondsElapsed;" variable

                handmade_debug.h

                struct debug_frame
                {
                    u64 BeginClock;
                    u64 EndClock;
                    r32 WallSecondsElapsed;
                   
                    u32 RegionCount;
                    debug_frame_region *Regions;
                };



45:25
with us changing the FRAME_MARKER at the end of a frame, we have to change the way we take timings for debug_frames 
in CollateDebugRecords();

so what that means is that, everytime we see a FRAME_MARKER

-   we calculate the timings for the debug_frame, essentially we mark the EndClock and WallSecondsElapsed.

                CurrentFrame->EndClock = Event->Clock;
                CurrentFrame->WallSecondsElapsed = Event->SecondsElapsed;

-   then we also initailize the debug_frame for the next frame, particularly, we write the BeginClock();

                CurrentFrame = DebugState->Frames + DebugState->FrameCount++;
                CurrentFrame->BeginClock = Event->Clock;

-   full code below:

                handmade_debug.cpp

                void CollateDebugRecords()
                {
                    ...
                    ...

                    if(Event->Type == DebugEvent_FrameMarker)
                    {
                        if(CurrentFrame)
                        {
                            CurrentFrame->EndClock = Event->Clock;
                            CurrentFrame->WallSecondsElapsed = Event->SecondsElapsed;

                            r32 ClockRange = (r32)(CurrentFrame->EndClock - CurrentFrame->BeginClock);
                        }

                        CurrentFrame = DebugState->Frames + DebugState->FrameCount++;
                        CurrentFrame->BeginClock = Event->Clock;
                        CurrentFrame->EndClock = 0;
                        CurrentFrame->RegionCount = 0;
                        CurrentFrame->Regions = PushArray(&DebugState->CollateArena, MAX_REGIONS_PER_FRAME, debug_frame_region);
                        CurrentFrame->WallSecondsElapsed = 0.0f;
                    }
                }






50:08
Casey added the BEGIN_BLOCK and END_BLOCK for the Debug Data Collation step. This way we would get timings 
for Debug Data Collation as well.
                
                void WinMain()
                {
                    while(GlobalRunning)
                    {
                        ...
                        ...

                        BEGIN_BLOCK(DebugCollation);

                        if(Game.DEBUGFrameEnd)
                        {
                            GlobalDebugTable = Game.DEBUGFrameEnd(&GameMemory);
                        }
                        GlobalDebugTable_.EventArrayIndex_EventIndex = 0;

                        END_BLOCK(DebugCollation);


                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        FRAME_MARKER(Win32GetSecondsElapsed(LastCounter, EndCounter));
                        LastCounter = EndCounter;

                        if(GlobalDebugTable)
                        {
                            // TODO(casey): Move this to a global variable so that
                            // there can be timers below this one?
                            GlobalDebugTable->RecordCount[TRANSLATION_UNIT_INDEX] = __COUNTER__;
                        }
                    }

                }

50:27
so now we have something that is more representative of the frame rate.



53:44
Casey printing out the last frame time in the top left corner
and we are hovering around 250 ms per frame

Casey also turned off the rendering all the chart,
and we get print our the last frame time, and it hovers around 90 ms.
We can conclude that collating takes a long time right now

we print the frame time by just printing the WallSecondsElapsed of the most recent frame 

                DebugState->Frames[DebugState->FrameCount - 1].WallSecondsElapsed

-   full code below:

                handmade.cpp

                internal void DEBUGOverlay(game_memory *Memory)
                {
                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState && DEBUGRenderGroup)
                    {
                        render_group *RenderGroup = DEBUGRenderGroup;

                        // TODO(casey): Layout / cached font info / etc. for real debug display
                        loaded_font *Font = PushFont(RenderGroup, FontID);
                        if(Font)
                        {
                            ...
                            ...

                            if(DebugState->FrameCount)
                            {
                                char TextBuffer[256];
                                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                            "Last frame time: %.02fms",
        ----------->                        DebugState->Frames[DebugState->FrameCount - 1].WallSecondsElapsed * 1000.0f);
                                DEBUGTextLine(TextBuffer);
                            }
                        }
                    }
                }