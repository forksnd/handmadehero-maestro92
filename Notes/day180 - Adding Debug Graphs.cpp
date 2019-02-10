Handmade Hero Day 180 - Adding Debug Graphs

Summary:
renders bar graph for CycleCount, HitCount and CycleOverHitCount timings

changed the structure of debug_frame_end_info struct to be more platform friendly and flexible

renders stacked bar graph for debug_frame_end_info snapshots 

Keyword:
rendering, debug, profiling


5:28
moved alot of the debug related functions to handmade_debug.cpp


8:25
So Casey mentioned that, now we have all this debug data, Casey plans to push all these debug data 
through the renderer and drawn them as bars. 

This way we can visualize the cost of a particular function (Timed by our TIMED_BLOCK); over time


13:17
Casey renders a bar for our debug data.

-   notice that we are using the DEBUGRenderGroup

                if(DebugState && DEBUGRenderGroup)
                {
                    ...
                }

-   the height of the bar is determined by its ratio with max 

    for example, lets just take a look at how we render CycleCount.

    for a single TIMED_BLOCK, we go through all the Snapshots, and we accumulate the debug data.
    then the scale is gonna be 1/CycleCount.Max.

    finally, we go through each snapshot, and render each snapshot_s CycleCount as a bar. 

-   we render the bars by calling 

                PushRect(RenderGroup, 
                    V3(ChartLeft + BarWidth*(r32)SnapshotIndex + 0.5f*BarWidth, 
                        ChartMinY + 0.5f*ThisHeight, 0.0f), 
                    V2(BarWidth, ThisHeight), 
                    V4(ThisProportion, 1, 0.0f, 1));


-   full code below:

                handmade_debug.cpp

                internal void OverlayCycleCounters(game_memory *Memory)
                {
                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState && DEBUGRenderGroup)
                    {
                        ...
                        ...

                        for(u32 CounterIndex = 0; CounterIndex < DebugState->CounterCount; ++CounterIndex)
                        {
                            debug_counter_state *Counter = DebugState->CounterStates + CounterIndex;

                            debug_statistic HitCount, CycleCount, CycleOverHit;
                            ...
                            BeginDebugStatistic(&CycleCount);
                            ...
                            for(u32 SnapshotIndex = 0; SnapshotIndex < DEBUG_SNAPSHOT_COUNT; ++SnapshotIndex)
                            {
                                ...
                                AccumDebugStatistic(&CycleCount, Counter->Snapshots[SnapshotIndex].CycleCount);
                                ...
                                ...
                            }
                            ...
                            EndDebugStatistic(&CycleCount);
                            ...


                            if(CycleCount.Max > 0.0f)
                            {
                                ...
                                ...

                                r32 Scale = 1.0f / (r32)CycleCount.Max;

                                for(u32 SnapshotIndex = 0; SnapshotIndex < DEBUG_SNAPSHOT_COUNT; ++SnapshotIndex)
                                {
                                    r32 ThisProportion = Scale*(r32)Counter->Snapshots[SnapshotIndex].CycleCount;
                                    r32 ThisHeight = ChartHeight*ThisProportion;
                                    PushRect(RenderGroup, V3(ChartLeft + BarWidth*(r32)SnapshotIndex + 0.5f*BarWidth, ChartMinY + 0.5f*ThisHeight, 0.0f), V2(BarWidth, ThisHeight), V4(ThisProportion, 1, 0.0f, 1));
                                }

                                ......................................................
                                ................... print debug data .................
                                ......................................................
                            }
                        }
                    }
                }


18:12
notice that when we render the Bars when we call the PushRect(); function

-   ThisProportion is a number between 0 and 1 
    
    so with the color V4(ThisProportion, 1, 0.0f, 1);, we will have yellow when it is near the max, and green 
    when it is near 0


                r32 ThisProportion = Scale*(r32)Counter->Snapshots[SnapshotIndex].CycleCount;

                PushRect(..., ..., ..., V4(ThisProportion, 1, 0.0f, 1));



21:46
so now Casey got a basic graph rendered
But Casey does mention a few problems:

these charts, they change their scale. That is becuz they are using the maximum value for the past 
128 frames (or whatever the length of our snapshots is);
what that means is, in the event that we see some performance spike, such as the OS taking some time 
at that particular moment, that will reset the scale for that 128 frames.

but as soon as that performance spike falls out of the 128 frames window, the scale will jump back up again.
so the way we are currently graphing is somewhat jenky.


however, it is not very obvious how you want to graph and visualize your debug data

you can do something fancy, such as throwing out outliers. you also cant compare the graphics with each other
for instance, there is no way of know if function1 is taking more time than function2



26:04
Casey also mentions that currently we have a bunch of nulls. Those nulls are anywhere we dont actually hit 
the performance counter. 
meaning lets say I put a TIMED_BLOCK inside a function 

                void function2()
                {
                    TIMED_BLOCK;
                    
                    ...
                    ...
                }

and that function2 never gets called. thats what happens.

so to filter out those, Casey just added an if check for the FunctionName.               

this way we only display functions that we have called at least one time, so the information is properly filled. 



27:58
now Casey wants to make a stacked graph. something that shows timings over time in a more proportional way. 

recall that we made the debug_frame_end_info struct 

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


what Casey wants to do is to save those as well, as a separate set of snapshots. 
so Casey creates another array of snapshots for the debug_frame_end_infos


                handmade_debug.h

                struct debug_state
                {
                    u32 SnapshotIndex;
                    u32 CounterCount;
                    debug_counter_state CounterStates[512];
    ----------->    debug_frame_end_info FrameEndInfos[DEBUG_SNAPSHOT_COUNT];   
                };

this will store a snapshot, one for every frame.


33:15
in our DEBUGOverlay(); (we renamed OverlayCycleCounters to DEBUGRenderGroup); function 
we also want to render a chart fo the FrameEndInfos informations.


34:13
Casey mentions that for our debug_frame_end_info, if we think about what we have to do for platform porting, 
this is kind of specific to the windows platform. 

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

the order that these steps go is very much specific to the win32 platform layer. Thats not necessarily what we want.
for instance, on linux, we may not want to do InputProcessed before GameUpdated.

so a better thing to do here, is to have have the debug_frame_end_info more like 

                handmade_platform.h

                struct debug_frame_timestamp
                {
                    char *Name;
                    r32 Seconds;
                };

                struct debug_frame_end_info
                {
                    u32 TimestampCount;
                    debug_frame_timestamp Timestamps[64];
                };




37:43
then in the DEBUGOverlay(); we also wanto visualize data on our debug_frame_end_infos
here, Casey says he wants to show absolute time.

-   Casey determined that the total ChartHeight 300 pixels tall 

-   for the total chart height, since we want to render at 30 frames a second, which is our target frame rate.
    so 0.03333f seconds a frame

    if a frame takes longer than 0.03333f seconds, it will stand out

    so Scale = 1.0f / 0.03333f. This is just like in section 13:17, where we did "r32 Scale = 1.0f / (r32)CycleCount.Max;"
    the scale is 1.0f / max_value of your y axis. Here the max value of our y axis is 0.03333f seconds.

-   we also make a stacked bars. 

                internal void DEBUGOverlay(game_memory *Memory)
                {
                    .................................................................
                    ........... printing CycleCount and HitCount ...................
                    ................................................................

                    ...
                    r32 ChartHeight = 300.0f;
                    ...
                    ...
                    r32 Scale = 1.0f / 0.03333f;

                    ..................................................................
                    ........ iterating the debug_frame_end_info snapshot .............
                    ..................................................................
                    
                }



42:10
then we iterate through our debug_frame_end_info snapshots to do our visualization

-   notice that we have this PrevTimestampSeconds variable. Recall for our timings in debug_frame_end_info,
    it was accumulative timings 

                handmade_platform.h

                struct debug_frame_end_info
                {
                    u32 TimestampCount;
                    debug_frame_timestamp Timestamps[64];
                };

    the way we used it is 

                void WinMain()
                {

                    while(GlobalRunning)
                    {
                        portion0;
                        record debug_frame_end_info.Timestamps[0];

                        portion1;
                        record debug_frame_end_info.Timestamps[1];

                        portion2;
                        record debug_frame_end_info.Timestamps[2];

                        ...
                        ...
                    }

                }

    so for timings of each section, we have to subtract the previous Timestamp

    so timings for portion1 = debug_frame_end_info.Timestamps[1] - debug_frame_end_info.Timestamps[0];

    timings for portion2 is 
    debug_frame_end_info.Timestamps[2] - debug_frame_end_info.Timestamps[1];


    so as we interate our snapshot, we do the following

                debug_frame_timestamp *Timestamp = Info->Timestamps + TimestampIndex;
                r32 ThisSecondsElapsed = Timestamp->Seconds - PrevTimestampSeconds;
                PrevTimestampSeconds = Timestamp->Seconds;


-   53:12, we also draw a line that represents the max value of our chart 

                PushRect(RenderGroup, V3(ChartLeft + 0.5f*ChartWidth, ChartMinY + ChartHeight, 0.0f),
                             V2(ChartWidth, 1.0f), V4(1, 1, 1, 1));
    we draw it in white.

-   we add a list of colors. As we render our stacked bar, we just cycle through the colors 

                v3 Color = Colors[TimestampIndex%ArrayCount(Colors)];

-   full code below

                internal void DEBUGOverlay(game_memory *Memory)
                {
                    .................................................................
                    ........... printing CycleCount and HitCount ...................
                    ................................................................

                    ...
                    r32 ChartHeight = 300.0f;
                    ...
                    ...
                    r32 Scale = 1.0f / 0.03333f;

                    v3 Colors[] =
                    {
                        {1, 0, 0},
                        {0, 1, 0},
                        {0, 0, 1},
                        {1, 1, 0},
                        {0, 1, 1},
                        {1, 0, 1},
                        {1, 0.5f, 0},
                        {1, 0, 0.5f},
                        {0.5f, 1, 0},
                        {0, 1, 0.5f},
                        {0.5f, 0, 1},
                        {0, 0.5f, 1},
                    };


                    for(u32 SnapshotIndex = 0; SnapshotIndex < DEBUG_SNAPSHOT_COUNT; ++SnapshotIndex)
                    {
                        debug_frame_end_info *Info = DebugState->FrameEndInfos + SnapshotIndex;
                        r32 StackY = ChartMinY;
                        r32 PrevTimestampSeconds = 0.0f;
                        for(u32 TimestampIndex = 0; TimestampIndex < Info->TimestampCount; ++TimestampIndex)
                        {
                            debug_frame_timestamp *Timestamp = Info->Timestamps + TimestampIndex;
                            r32 ThisSecondsElapsed = Timestamp->Seconds - PrevTimestampSeconds;
                            PrevTimestampSeconds = Timestamp->Seconds;

                            v3 Color = Colors[TimestampIndex%ArrayCount(Colors)];
                            r32 ThisProportion = Scale*ThisSecondsElapsed;
                            r32 ThisHeight = ChartHeight*ThisProportion;
                            PushRect(RenderGroup, V3(ChartLeft + BarSpacing*(r32)SnapshotIndex + 0.5f*BarWidth,
                                                     StackY + 0.5f*ThisHeight, 0.0f),
                                     V2(BarWidth, ThisHeight), V4(Color, 1));
                            StackY += ThisHeight;
                        }                
                    }

                    PushRect(RenderGroup, V3(ChartLeft + 0.5f*ChartWidth, ChartMinY + ChartHeight, 0.0f),
                             V2(ChartWidth, 1.0f), V4(1, 1, 1, 1));
                }



visually, what we did is this, pretty much we render stacked timings for the past DEBUG_SNAPSHOT_COUNT frames.

        |                        ___________
        |                       |           |        ___________
        |                       |           |       |           |
        |    ___________        |  portion2 |       |  portion2 |
        |   |           |       |           |       |___________|
        |   | portion2  |       |___________|       |           |
        |   |___________|       |           |       |           |
        |   | portion1  |       |  portion1 |       |  portion1 |
        |   |___________|       |           |       |           |
        |   |           |       |___________|       |___________|
        |   | portion0  |       |           |       |           |           ...
        |   |           |       |  portion0 |       |  portion0 |
        |___|___________|_______|___________|_______|___________|___________________________________

            current frame                           current frame + 2           ...
            - DEBUG_SNAPSHOT_COUNT                  - DEBUG_SNAPSHOT_COUNT      


                                current frame + 1
                                - DEBUG_SNAPSHOT_COUNT



46:24
Casey writes the Win32RecordTimestamp(); function 

                win32_handmade.cpp

                inline void Win32RecordTimestamp(debug_frame_end_info *Info, char *Name, r32 Seconds)
                {
                    Assert(Info->TimestampCount < ArrayCount(Info->Timestamps));
                    
                    debug_frame_timestamp *Timestamp = Info->Timestamps + Info->TimestampCount++;
                    Timestamp->Name = Name;
                    Timestamp->Seconds = Seconds;
                }


48:15
and we call these in our frame to record timings, nothing special

                win32_handmade.cpp 

                WinMain()
                {
                    while(GlobalRunning)
                    {
                        Win32RecordTimestamp(&FrameEndInfo, "ExecutableReady", 
                                             Win32GetSecondsElapsed(LastCounter, Win32GetWallClock()));
                        ...
                        ...

                        Win32RecordTimestamp(&FrameEndInfo, "InputProcessed",
                                            Win32GetSecondsElapsed(LastCounter, Win32GetWallClock()));

                        ...
                        ...
                        Win32RecordTimestamp(&FrameEndInfo, "GameUpdated",
                                            Win32GetSecondsElapsed(LastCounter, Win32GetWallClock()));

                        ...
                        ...
                    }   
                }


1:03:10
someone asked in the Q/A, how do we track how long it takes a frame to work and the render engine to work 
if rendering debug is part of that work.


right now we dont have a way of eliminating the debug part of the work 
we can figure it out to a certain extent 

                
recall the structure of our debug system is 

                
                WinMain()
                {

                    while(GlobalRunning)
                    {
                        ...
                        ...

                        GameUpdateAndRender()
                        {
                            ...
                            ...
                            DEBUGOverlay();
                            {
                                ......................................................................
                                .......... renders all of bars and prints out our numbers ............
                                ......................................................................
                            }
                        }

                        ...
                        ...

                        DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                        {                           
                            ......................................................................
                            .......... Copies debug data from Debug_Records array ................
                            .......... to our Debug Memory storage ...............................
                            ......................................................................
                        }
                    }

                }


so Casey just put a TIMED_BLOCK in the DEBUGOverlay(); function
and it turns out, DEBUGOverlay is taking a significant time



1:09:39
Someone asked what can we optimize about the way we draw our rects 
(since its taking so long for the debug system to render all these bar charts);

currently we only optimized our texture filing, but we wrote an optimized version for 
the DrawRectangle(); function. its not even 4 pixels at a time, not in SIMD.

                handmade_render_group.cpp

                internal void
                DrawRectangle(loaded_bitmap *Buffer, v2 vMin, v2 vMax, v4 Color, rectangle2i ClipRect, bool32 Even)
                {
                    TIMED_BLOCK();

                    ...
                    ...

                    uint8 *Row = ((uint8 *)Buffer->Memory +
                                  FillRect.MinX*BITMAP_BYTES_PER_PIXEL +
                                  FillRect.MinY*Buffer->Pitch);
      
                    for(int Y = FillRect.MinY; Y < FillRect.MaxY; Y += 2)
                    {
                        uint32 *Pixel = (uint32 *)Row;
                        for(int X = FillRect.MinX; X < FillRect.MaxX; ++X)
                        {            
                            *Pixel++ = Color32;
                        }
                        
                        Row += 2*Buffer->Pitch;
                    }
                }


1:10:45
someone asked, becuz you are using rendering on multiple threads, is it saying your are spending more cycles in 
DrawRectangle(); and DrawRectangleQuickly(); than GameUpdateAndRender();


Yes. GameUpdateAndRender(); is doing one frame worth of rendering, waitng until all the other render threads on done. 
the other render threads will actually sum up the total amount of time they all spend on their separate cores.

so if we are running on 4 cores, that timings will the sum of all 4 timings for each individual core. 

so if you have 8 cores, we are gonna get the sum of all the cycles spent on all 8 cores.

which is why we are seeing more cycle counts on DrawRectangle(); and DrawRectangleQuickly();


