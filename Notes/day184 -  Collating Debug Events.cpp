Handmade Hero Day 184 - Collating Debug Events

Summary:
discussed the ways how we want to visualize our debug data
concluded that we want two ways, one is the hierarchical view
the other the top slowest functions list view

realized that to visaulize multi-threaded, we also want lanes within a bar to represent the threads 

gave the debug_state temporary_memory so that it can use as scratch space to for collating debug data

inside the CollateDebugRecords(); function, we iterated through our past 63 frames of debug data 
and generate debug_frame_region so that our view can render these debug_frame_region as bars


Keyword:
debug, profiling


3:37
Casey dicusses the fews ways we can visualize our debug data
assume we have a stacked bar chart

assume A, B, C, D and E are the five top level functions, 
and then we can click to open up D, we see G among the few functions, we dont see G as a guy who takes up a lot of time s


but when we then we click to oppne up A, we see G again, and we dont see G as the function who takes up most of the time.

but accumulatively, G is taking lots of time.

             ___________
            |           |
            |     E     |        ___________
            |___________|       |           |  
            |     D     |------>|           |      
            |___________|       |     F     |      
            |           |       |           |       
            |     C     |       |___________|      
            |___________|       |           |       
            |     B     |       |     G     |      
            |___________|       |           |      
            |           |       |___________|       
            |     A     |       |           |       
            |           |       |     H     |       
            |___________|       |___________|
                |
                |                ___________
                |               |     L     |  
                --------------->|___________|  
                                |           |  
                                |     G     |  
                                |___________|  
                                |           |  
                                |           |  
                                |     M     |  
                                |           |  
                                |           |  
                                |___________|  
                                |           |  
                                |     N     | 
                                |___________| 

so we kind of need 2 ways of viewing things. 
so we need hierarchically who is taking most of the time 

but we also need to know how much the time a function is taking in general within a frame.
without have having to drill down every single one of A, B, C, D and E and mentally try to add up how much that is.


so we need two ways of viewing this.
1.  hierarchically view
2.  Top Slow people
    where we dont care about draw graph. just top slow functions 





11:18
Casey start tackling the  DEBUGOverlay(); function to render the bar graphs 

-   so first we go through our frame debug data stored in our debug memory storage 
    recall DebugState is in our debug memory storage

-   then we go through our Regions to render the region as a rect 

                handmade_debug.cpp

                internal void DEBUGOverlay(game_memory *Memory)
                {
                    ...
                    ...

                    for(u32 FrameIndex = 0; FrameIndex < DebugState->FrameCount; ++FrameIndex)
                    {
                        debug_frame *Frame = DebugState->Frames + FrameIndex;

                        for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex)
                        {
                            ...
                            ...
                        }                
                    }
                    
                    PushRect(RenderGroup, V3(ChartLeft + 0.5f*ChartWidth, ChartMinY + ChartHeight, 0.0f),
                             V2(ChartWidth, 1.0f), V4(1, 1, 1, 1));

                }


14:05
Casey also mentions that if we are multi-threaded, and we want to do this visualization
we need multiple trakcs inside a bar 

                 thread1         thread3 
                         thread2
                 _______________________
                |       |       |       |
                |   A   |       | Idle  |
                |_______|   E   |       |
                |       |       |_______|
                |       |       |       |
                | Idle  |_______|       |
                |       |       |       |
                |_______|       |   G   |
                |       |   D   |       |
                |   B   |       |       |
                |       |       |       |
                |       |       |       |
                |       |       |       |
                |       |       |       |
                |       |       |       |
                |       |       |       |
                |_______|_______|_______|


so visualizing this is pretty tricky


16:16
So Casey plans to render the bars depending on the lanes 
we will assume that we have something called the debug_frame_region.
we assume the debug_frame_region stores its LaneIndex.

so essentially the Frame->Regions, is just an array of debug_frame_region, whose knows about its own LaneIndex 

visually it will look like this 


                 _______________________
                |       |       |       |
                |   1   |       |   6   |
                |_______|   4   |       |
                |       |       |_______|
                |       |       |       |
                |   2   |_______|       |
                |       |       |       |
                |_______|       |   7   |
                |       |   5   |       |
                |   3   |       |       |
                |       |       |       |
                |       |       |       |
                |       |       |       |
                |       |       |       |
                |       |       |       |
                |       |       |       |
                |_______|_______|_______|

                      _________________________________________________
    Frame->Regions = | region 1 | region 2 | region 3 |.....| region 7 |
                     |__________|__________|__________|_____|__________|

    and when we render it, we will render their bars according to their lanes. 


-   full code below:



                internal void DEBUGOverlay(game_memory *Memory)
                {
                    ...
                    ...

                    r32 LaneWidth = 8.0f;
                    u32 LaneCount = DebugState->FrameBarLaneCount;
                    r32 BarWidth = LaneWidth*LaneCount;


                    for(u32 FrameIndex = 0; FrameIndex < DebugState->FrameCount; ++FrameIndex)
                    {
                        debug_frame *Frame = DebugState->Frames + FrameIndex;
                        ...
                        ...

                        for(u32 RegionIndex = 0;
                            RegionIndex < Frame->RegionCount;
                            ++RegionIndex)
                        {
                            debug_frame_region *Region = Frame->Regions + RegionIndex;

                            ...
                            ...

                            PushRect(RenderGroup, V3(StackX + 0.5f*LaneWidth + LaneWidth*Region->LaneIndex,
                                                     0.5f*(ThisMinY + ThisMaxY), 0.0f),
                                     V2(LaneWidth, ThisMaxY - ThisMinY), V4(Color, 1));
                        }                
                    }
                    
                    PushRect(RenderGroup, V3(ChartLeft + 0.5f*ChartWidth, ChartMinY + ChartHeight, 0.0f),
                             V2(ChartWidth, 1.0f), V4(1, 1, 1, 1));

                }



23:51
in struct debug_state, we start to define the debug_frame_region

as a first draft, we have the following

                handmade_debug.h

                struct debug_frame_region
                {
                    u32 LaneIndex;
                    r32 MinT;
                    r32 MaxT;
                };

                struct debug_frame
                {
                   
                    u32 RegionCount;
                    debug_frame_region *Regions;
                };

                struct debug_state
                {
                    u32 FrameBarLaneCount;
                    u32 FrameCount;
                    r32 FrameBarScale;

                    debug_frame *Frames;
                };



29:59
then we want to start to consider how do we actually collate our data 

to collate our data, it will need memory, so we put some memory related variables in the debug_state struct


                handmade_debug.h

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
                };


31:05
in the DEBUG_GAME_FRAME_END(DEBUG_GAME_FRAME_END); function, we initalize all stuff for debug_state

also we call BeginTemporaryMemory(); here.
the idea is that everyframe, we will rewrite all the debug memory everytime. 


                handmade_debug.cpp

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    ...
                    ...

                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState)
                    {
                        if(!DebugState->Initialized)
                        {
                            InitializeArena(&DebugState->CollateArena, Memory->DebugStorageSize - sizeof(debug_state),
                                            DebugState + 1);
                            DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);            
                        }

                        EndTemporaryMemory(DebugState->CollateTemp);            
                        DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);            

                        CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
                    }

                    return(GlobalDebugTable);
                }

the reason why we call BeginTemporaryMemory(); and EndTemporaryMemory(); this way is cuz 
after we CollateDebugRecords(); we need our collated data to last till the we call DEBUGOverlay();
so we can only call EndTemporaryMemory(); when we are done printing/rendering in DEBUGOverlay();

hence we call EndTemporaryMemory(); here.      



34:50
now we go into CollateDebugRecords(); to collate our debug data.

-   notice that we pass in the InvalidEventArrayIndex.

    recall that we intend to store debug information for the past 64 frames. But one of them, is one where
    we are currently writing into, so we will only collate the other 63 frames. 

    so when the CollateDebugRecords(); function takes in the DebugState, it needs to know which entries in the 
    array is the one people are writing into.


visaully it looks like this
                            
                            most 
                            recent    invalid

                                |       |
                                |       |
                                V       V
     _______________________________________________________________________
    |       |       |       |       |       |       |       |       |       |
    |       |       |       |       |   x   |       |       |       |       |
    |_______|_______|_______|_______|_______|_______|_______|_______|_______|

                                            ---------------------------------
    ------------------------------->
        collate all the other frames debug data



-   notice how we iterate through our EventArray, we also do the wrapping inside the loop


                if(EventArrayIndex == MAX_DEBUG_FRAME_COUNT)
                {
                    EventArrayIndex = 0;
                }

                if(EventArrayIndex == InvalidEventArrayIndex)
                {
                    break;
                }


-   full code below:

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {
                    DebugState->FrameBarLaneCount = 0;
                    DebugState->FrameCount = 0;
                    DebugState->FrameBarScale = 0.0f;

                    debug_frame *CurrentFrame = 0;    
                    for(u32 EventArrayIndex = InvalidEventArrayIndex + 1; ; ++EventArrayIndex)
                    {
                        if(EventArrayIndex == MAX_DEBUG_FRAME_COUNT)
                        {
                            EventArrayIndex = 0;
                        }

                        if(EventArrayIndex == InvalidEventArrayIndex)
                        {
                            break;
                        }

                        for(u32 EventIndex = 0; EventIndex < MAX_DEBUG_EVENT_COUNT; ++EventIndex)
                        {
                            ...
                            ...
                        }
                    }                
                }


44:28
then we examine each individual Events we will process them differently depending the EventType

Casey mentions that these events are mostly in order, from earliest to latest.
on any given thread, we know they will be earliest to latest. But when multi-threading is taken into account 
we dont necessarily know that that is true. But for the most part they will be. Casey mentions that 
he just doesnt know how serialized __rdtsc(); is in that sense.


-   the idea is that we have this debug_frame* CurrentFrame. We are marking debug_frame timings 
                    
                    CurrentFrame->BeginClock(); and CurrentFrame->EndClock();

    whenever we see a Frame Marker.

    imagine the case below, assume thread0 is the main thread

    GlobalDebugTable->Events[EventArrayIndex]


                                           current frame - 63                         
                                                                                      
     _______________________|_________________________________________________________
    | thread1   | thread2   | thread0       | thread0       | thread0    |            |
    | EndBlock  | EndBlock  | Frame Marker  | BeginBlock    | BeginBlock |  ......    |
    |___________|___________|_______________|_______________|____________|____________|




                                           current frame - 62                         
                                                                                      
     ___________|_________________________________________________________
    | thread1   | thread0       | thread0       | thread0    |            |
    | EndBlock  | Frame Marker  | BeginBlock    | BeginBlock |  ......    |
    |___________|_______________|_______________|____________|____________|


    imagine we were already on frame (current frame - 63);. then we start reading frame (current frame - 62);
    debug data. we will only officaly start the new frame once we get the DebugEvent_FrameMarker, and we also 
    conclude the previous frame. 

    this is also why we have.

                        if(Event->Type == DebugEvent_FrameMarker)
                        {
                            if(CurrentFrame)
                            {
                                CurrentFrame->EndClock = Event->Clock;
                            }
                        }
    
    this writes down the EndClock timing for the previous frame.


-   another thing is that we also want to throw away all the data until we see a frame marker. 
    so lets say we start at the very beginning, which is current frame - 63.

    assume since we are doing multi-threading, we got a few EndBlock markers before the very first Frame Marker
    arrives, we want to throw these out, cuz its hard for us to analyze them



-   full code below:

                handmade_debug.cpp      


                debug_frame *CurrentFrame = 0; 

                for (all 63 frames that we will visit)
                {

                    for(u32 EventIndex = 0; EventIndex < MAX_DEBUG_EVENT_COUNT; ++EventIndex)
                    {
                        debug_event *Event = GlobalDebugTable->Events[EventArrayIndex] + EventIndex;            
                        debug_record *Source = (GlobalDebugTable->Records[Event->TranslationUnit] +
                                                Event->DebugRecordIndex);

                        if(Event->Type == DebugEvent_FrameMarker)
                        {
                            if(CurrentFrame)
                            {
                                CurrentFrame->EndClock = Event->Clock;
                            }

                            CurrentFrame = DebugState->Frames + DebugState->FrameCount++;
                            CurrentFrame->BeginClock = Event->Clock;
                            CurrentFrame->EndClock = 0;
                            CurrentFrame->RegionCount = 0;
                        }
                        else if(CurrentFrame)
                        {
                            u64 RelativeClock = Event->Clock - CurrentFrame->BeginClock;
                            u32 LaneIndex = GetLaneFromThreadIndex(DebugState, Event->ThreadIndex);
                            if(Event->Type == DebugEvent_BeginBlock)
                            {
                            }
                            else if(Event->Type == DebugEvent_EndBlock)
                            {

                            }
                            else
                            {
                                Assert(!"Invalid event type");
                            }
                        }
                    }
                }

we will finish this in the next episode.



