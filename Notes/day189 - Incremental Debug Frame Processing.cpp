Handmade Hero Day 189 - Incremental Debug Frame Processing

Summary:
changed it so that we do incremental debug frame processing. 

when we collate our debug data, we remember which frame is the one we last processed.
then as new frames comes in, we just collate the new one, instead of restarting from the begnning like what 
we did in starting from day 184

added the functionality of examining further down into a timed blocks by hovering over a bar, and left clicking into it.

explain why we see some time differences between time, attributed to OS swap out time.

Keyword:
debug syste, profiling, rendering, UI



4:48
changed the rendering to be bar chart rendering to be horizontal
since when we print out the debug details information, we print out horizontal texts,
this way both debug charts and debug text are aligned horizontally





10:17
Casey mentioned that why is the "Frame Rate Wait" routine is taking so much time. 
essentially the following routine

                
                int WinMain()
                {
                
                    while(GlobalRunning)
                    {

                        ...
                        ...

                        BEGIN_BLOCK(FramerateWait);

                        if(!GlobalPause)
                        {
                            LARGE_INTEGER WorkCounter = Win32GetWallClock();
                            real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                            // TODO(casey): NOT TESTED YET!  PROBABLY BUGGY!!!!!
                            real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                            if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {                        
                                if(SleepIsGranular)
                                {
                                    DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                                       SecondsElapsedForFrame));
                                    if(SleepMS > 0)
                                    {
                                        Sleep(SleepMS);
                                    }
                                }
                            
                                real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
                                                                                           Win32GetWallClock());
                                if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                                {
                                    // TODO(casey): LOG MISSED SLEEP HERE
                                }
                            
                                while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                                {                            
                                    SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
                                                                                    Win32GetWallClock());
                                }
                            }
                            else
                            {
                                // TODO(casey): MISSED FRAME RATE!
                                // TODO(casey): Logging
                            }
                        }

                        END_BLOCK(FramerateWait);
                    }

                }

Casey thinks this part is too jenky.
Casey essentially disablled the sleep portion, so we are leaving this part off until we have v-blank support.




16:32
so what we will work on today is Incremental Debug Frame Processing.
recall our debug_state looks like this right now 


                handmade_debug.h

                struct debug_state
                {
                    b32 Initialized;
                    b32 Paused;
                    
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


the idea is that we want our collation to be a more permenant thing that runs forwards. the way we would do that is that 
for "debug_frame *Frames;" and "debug_thread *FirstThread;", we wont reset them every frame. Instead is that it restarts itself
everytime we askes it to. Right now what we are doing is that, we have our memory arena, and we start collating things. And every frame,
we clear memory, we restart, and we start collating again. 

what we could do instead, keep collating. The only time to restart, is when we tell it to restart. 





17:48
recall that our logic to CollateDebugRecords(); is as follow:

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

                        if(!DebugState->Paused)
                        {
                            EndTemporaryMemory(DebugState->CollateTemp);            
                            DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);

                            DebugState->FirstThread = 0;
                            DebugState->FirstFreeBlock = 0;

                            CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
                        }        
                    }

                    return(GlobalDebugTable);
                }


-   so for every frame, we no longer will be doing BeginTemporaryMemory(); and EndTemporaryMemory();
    we will only restart when we run out of frame storage.



                internal void RestartCollation()
                {
                    EndTemporaryMemory(DebugState->CollateTemp);            
                    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);

                    DebugState->FirstThread = 0;
                    DebugState->FirstFreeBlock = 0;
                }



20:14
so now what we do is that we remember the last frame count we just processed, and start there.

so in DebugState, we added a CollationArrayIndex variable to remember where we are 

                
                handmade_debug.cpp

                struct debug_state
                {
                    ...
                    ...

                    u32 CollationArrayIndex;

                    ...
                    ...
                };




so in the RestartCollation, we have the CollationArrayIndex to be at InvalidEventArrayIndex + 1.
the InvalidEventArrayIndex is the one we will be writing in, so InvalidEventArrayIndex + 1 is the oldest one. 
so in RestartCollation();, we are literally restarting from the oldest frame and processing it till 
the InvalidEventArrayIndex - 1;

                
                internal void RestartCollation(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {
                    EndTemporaryMemory(DebugState->CollateTemp);            
                    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);

                    DebugState->FirstThread = 0;
                    DebugState->FirstFreeBlock = 0;

                    DebugState->Frames = PushArray(&DebugState->CollateArena, MAX_DEBUG_EVENT_ARRAY_COUNT*4, debug_frame);
                    DebugState->FrameBarLaneCount = 0;
                    DebugState->FrameCount = 0;    
                    DebugState->FrameBarScale = 1.0f / 60000000.0f;

    ------------>   DebugState->CollationArrayIndex = InvalidEventArrayIndex + 1;
                    DebugState->CollationFrame = 0;
                }



graphically it looks like this:
so the invalid one, the one that everyone is writing in this frame, 
invalid + 1, is the oldest one. In RestartCollation, we are starting from InvalidEventArrayIndex + 1, wrapping around
and processing it till we meet up with InvalidEventArrayIndex itself 

                            most 
                            recent   invalid  oldest

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




21:07
then in CollateDebugRecords(); recall previously we had 

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {
                    ...
                    ...

                    for(u32 EventArrayIndex = InvalidEventArrayIndex + 1;
                        ;
                        ++EventArrayIndex)
                    {
                        ...
                        ...
                    }
                }

now we have, we will be iterating ++DebugState->CollationArrayIndex itself.

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {
                    for(;
                        ;
                        ++DebugState->CollationArrayIndex)
                    {
                        ...
                        ...

                        u32 EventArrayIndex = DebugState->CollationArrayIndex;
                        if(EventArrayIndex == InvalidEventArrayIndex)
                        {
                            break;
                        }
                    }
                }

so when we get till EventArrayIndex == InvalidEventArrayIndex, that is when we stop. Just like what we drew up there 
in the graph. 



23:54
so just to do this refactor little by little, we will restart if the frame count has reached our frame cout buffer limit


                handmade_debug.cpp 

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    ...
                    ...

                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState)
                    {
                        ...
                        ...

                        if(!DebugState->Paused)
                        {
                            if(DebugState->FrameCount > MAX_DEBUG_EVENT_ARRAY_COUNT)
                            {
                                RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
                            }

                            CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
                        }        
                    }

                    return(GlobalDebugTable);
                }


24:58
then in the ColllateDebugRecords(); recall that we were processing all the debug_events, 
and we were generating debug_frame based on all the FRAME_MARKERS we were getting.

now we want to remember the last debug_frame. So we want to store the most recent debug_frame 

                handmade_debug.cpp 

                struct debug_state
                {
                    ...
                    ...

                    u32 CollationArrayIndex;
    ----------->    debug_frame *CollationFrame;
                    ...
                };

and we just replace anywhere taht was using CurrentFrame with CollationFrame.



25:56
so with this refactor, it has dramatically decreased the time, but just not as much as Casey would like.
we are still spending too much time in the CollationData phase 


turns out we were not ever initailizing debug_state ever. Once we got the intialized flag set to true 
at game initialization, we are good. 




37:31
so right Casey would like to work on expanding the bar when we "hover" or click onto a bar.
That way we can have more information about who is taking the time within that TIMED_BLOCK

42:54
so in the DEBUGOverlay, as the user left clocks onto a bar, 
we will remember the TIMED_BLOCK that the user are interested in side DebugState->ScopeToRecord.

recall that each TIMED_BLOCK is represented by a debug_record. so we are just storing the debug_record.

                handmade_debug.cpp

                internal void DEBUGOverlay(game_memory *Memory, game_input *Input)
                {

                    ...
                    ...

                    if(WasPressed(Input->MouseButtons[PlatformMouseButton_Left]))
                    {
                        if(HotRecord)
                        {
                            DebugState->ScopeToRecord = HotRecord;
                        }
                        else
                        {
                            DebugState->ScopeToRecord = 0;
                        }
                        RefreshCollation(DebugState);
                    }
                }


and obviously for that, we create a debug_record stored in DebugState

                handmade_debug.h

                struct debug_state
                {
                    ...
                    ...

                    debug_record *ScopeToRecord;

                    ...
                    ...
                };



37:31
Recall from day 189, in CollateDebugRecords(); we are only showing the top level Regions.
the condition we were using to see if a BEGIN_BLOCK and END_BLOCK block is a top level block is:

                if(Thread->FirstOpenBlock->Parent == 0)

then if this condition is met, we call AddRegion(); which then the renderer will display all these regions on screen.
                               
-   full code below:

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {
                    ...
                    ...

                    for(u32 EventArrayIndex = InvalidEventArrayIndex + 1; ; ++EventArrayIndex)
                    {
                        ...
                        ...

                        for(u32 EventIndex = 0; EventIndex < GlobalDebugTable->EventCount[EventArrayIndex]; ++EventIndex)
                        {
                            ...
                            if(Event->Type == DebugEvent_BeginBlock)
                            {
                                ...
                                ...
                            }
                            else if(Event->Type == DebugEvent_EndBlock)
                            {                    
                                ...
                                ...
        ---------------->       if(Thread->FirstOpenBlock->Parent == 0)
                                {
                                    ...
                                    ...

                 ------------->     debug_frame_region *Region = AddRegion(DebugState, CurrentFrame);
                                    Region->Record = Source;
                                    ...
                                    ...
                                }
                            }
                        }
                    }
                }



so for us to render all the blocks within a block.
all we have to do is to
1.  grab the parent of our current TIMED_BLOCK
2.  check if the our parent TIMED_BLOCK matches the TIMED_BLOCK that the user is interested. 

or course debug_record is essentially a TIMED_BLOCK, so here we will just compare debug_records

so we will compare parent_s debug_record == DebugState.ScopeToRecord, the one we just stored by left cliking. 


visually, it looks like 

        BEGIN_BLOCK (debug_record1);                                        debug_state.ScopeToRecord
            

            BEGIN_BLOCK (debug_record2);    


            END_BLOCK (debug_record2);      <------- current block 


        END_BLOCK (debug_record1);


so if we are processing an END_BLOCK (at where current block is pointing at), 
we grab our parent, which is BEGIN_BLOCK(debug_record1) and END_BLOCK(debug_record1);
the debug_record that represents our parent is debug_record1.

we check if debug_record1 matches with debug_state.ScopeToRecord, which is the one the user wants to examine.

if they are the same, we call AddRegion, and the renderer will render. 




38:43
so to do that, we will store the debug_record that represents this BEGIN_BLOCK and END_BLOCK pair.

                handmade_debug.h

                struct open_debug_block
                {
                    u32 StartingFrameIndex;
    ----------->    debug_record *Source;
                    debug_event *OpeningEvent;
                    open_debug_block *Parent;

                    open_debug_block *NextFree;
                };



40:26
then in the CollateDebugRecords(); function, we add the conditino to see if our parent is the debug_record 
that the user is interested in.

                else if(Event->Type == DebugEvent_EndBlock)
                {        
                    .................................................................
                    .......... check to see if it a MatchingBlock ...................
                    .................................................................


    ----------->    if(GetRecordFrom(MatchingBlock->Parent) == DebugState->ScopeToRecord)
                    {
                        ...
                        ...
                    }
                }


41:07
the GetRecordFrom(); is literraly just returning the debug_record that created the blocks 

                inline debug_record *
                GetRecordFrom(open_debug_block *Block)
                {
                    debug_record *Result = Block ? Block->Source : 0;

                    return(Result);
                }

visually 

        BEGIN_BLOCK (debug_record1);                              
            
            BEGIN_BLOCK (debug_record2);   

            END_BLOCK (debug_record2);      

        END_BLOCK (debug_record1);


the first BEGIN_BLOCK and END_BLOCK will return debug_record1, the 2nd BEGIN_BLOCK and END_BLOCK will return debug_record2




45:05

remember in section 42:54 
so of course usually the work flow is the user pauses the collation,
user left clicks onto a bar since he wants to investigate further.

however, we have to restart the collation after the DebugState->ScopeToRecord. 
so we have to call RefreshCollation(); after we left-click on the the region 

also notice in this logic, if we click outside of any bar, we just set DebugState->ScopeToRecord = 0;
so we just go back to the state we are rendering top level TIMED_BLOCK regions


                handmade_debug.cpp

                internal void DEBUGOverlay(game_memory *Memory, game_input *Input)
                {

                    ...
                    ...

                    if(WasPressed(Input->MouseButtons[PlatformMouseButton_Left]))
                    {
                        if(HotRecord)
                        {
                            DebugState->ScopeToRecord = HotRecord;
                        }
                        else
                        {
                            DebugState->ScopeToRecord = 0;
                        }
        ------------>   RefreshCollation(DebugState);
                    }
                }




and we have the RefreshCollation function, which is just restart the collation, and CollateDebugRecords again.
this way the renderer will render the expanded bar chart.

                internal void
                RefreshCollation(debug_state *DebugState)
                {   
                    RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
                    CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
                }

49:13
now Casey got it to work where we can now expand and examine each block 



54:59
So casey made the debug_record text information render at where the mouse is.
and that reveals a sorting problem, where the text label is sometimes above or below the bars



recommended a site that explains 
Point-In-Polygon Algorithm — Determining Whether A Point Is Inside A Complex Polygon
http://alienryderflex.com/polygon/

[i did that for my zillow clone]


Q/A

1:09:20
Casey explaining why the bars for the child threads are not left aligned with the main threads
mainly becuz that is where the renderer kicks off, the renderer invokes child threads 

we havent multi-threaded other things until we get to the renderer, which is why we 
see a giant blank at the front of each frame 


1:12:53
we still seem to see a lot of fluctuations of frame time between frames?

For Casey, he thinks this is actaully pretty consistent timing 
we are probably losing some time to the system, especially since Casey is running OBS, this is actaully 
pretty consistent timings. You do have to remember that this is 16ms of time.

Consider that memory effects, being what they are, there are so much unpredictability 
in modern day execution, even if nobody is stealing anytime from you, even at every frame, you are at a different 
prcoessr state is enough to have your frames have different times 

one guess is that we may be seeing is the OS swapping us out, and swapping us back in, hence the time difference

For example, if you look at FrameDisplay, that is just mostly windows calling blit itself. 
that all windows OS. Swap out time is not necessarily the OS taking a longer time to do something for you. 





1:17:17
everyone once in a while, we notice that we miss a frame, is there a way to get around that reliably?

Not on windows. There are things we will be doing to minimize it more, such as not going to stretch blit. 
so if we move to OpenGL to display frames, that will help us not miss frames, cuz that is supposed to be going through a pipe 
that offers ontime delivery, unfortunately even that wont guarantee. Windows is not even a soft real time operating system,
let a lone hard real time OS. As a result, on windows you will miss frames on windows, full stop. 

windows OS can sometimes just swap things, for example, decides to render the title on your visual studio, hence making you 
miss your frame 

