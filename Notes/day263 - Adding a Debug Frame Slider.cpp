Handmade Hero Day 263 - Adding a Debug Frame Slider

Summary:
added the DebugType_FrameSlider ui element. Which gives us control which frame we want to view in the debugger view
also when we select a frame, we wont overwrite backstore frames 

Keyword:
debug system, profiler, UI


1:40
Casey mentioned that in terms of the ui debugger, 
Casey wants to be able to pause the profiler and go back to previous frames 


16:49
Casey mentioned that previously we were always viewing the MostRecentFrame

                handmade_debug.h

                struct debug_state
                {
                    ...
                    ...

    ----------->    u32 MostRecentFrameOrdinal;
                    u32 CollationFrameOrdinal;
                    u32 OldestFrameOrdinal;
                    debug_frame Frames[DEBUG_FRAME_COUNT];

                    ...
                };

Casey will add a viewingFrameOrdinal 

                handmade_debug.h

                struct debug_state
                {
                    ...
                    ...

                    u32 ViewingFrameOrdinal;

                    u32 MostRecentFrameOrdinal;
                    u32 CollationFrameOrdinal;
                    u32 OldestFrameOrdinal;
                    debug_frame Frames[DEBUG_FRAME_COUNT];

                    ...
                };



17:29
Casey added the DebugType_FrameSlider





21:22 
added the casey for the DebugType_FrameSlider in the 

                handmade_debug.cpp

                internal void DEBUGDrawElement(layout *Layout, debug_tree *Tree, debug_element *Element, debug_id DebugID,
                                 u32 FrameOrdinal)
                {


                    ...

                    case DebugType_FrameSlider:
                    {
                        v2 Dim = {1800, 32};
                        layout_element LayoutElement = BeginElementRectangle(Layout, &Dim);
                        EndElement(&LayoutElement);
                       
                        DrawFrameSlider(DebugState, DebugID, LayoutElement.Bounds, Layout->MouseP, Element);
                    } break;
                    


                }



31:43
Casey mentioned that if we have so much debug events that we ran out of memory,
the green and red frames would start to diverge. 



36:62
As Casey demonstrated at this point, there is now a gap between the green and the red
and that gap are the only valid frames 


38:09
Casey then added the DrawFrameSlider(); function 

                handmade_debug.cpp

                internal void DrawFrameSlider(debug_state *DebugState, debug_id SliderID, rectangle2 TotalRect, v2 MouseP,
                                debug_element *RootElement)
                {
                    u32 FrameCount = ArrayCount(RootElement->Frames);
                    if(FrameCount > 0)
                    {
                        object_transform NoTransform = DefaultFlatTransform();
                        PushRect(&DebugState->RenderGroup, DebugState->BackingTransform, TotalRect, 0.0f, V4(0, 0, 0, 0.25f));
                        
                        r32 BarWidth = (GetDim(TotalRect).x / (r32)FrameCount);
                        r32 AtX = TotalRect.Min.x;
                        r32 ThisMinY = TotalRect.Min.y;
                        r32 ThisMaxY = TotalRect.Max.y;
                        for(u32 FrameIndex = 0;
                            FrameIndex < FrameCount;
                            ++FrameIndex)
                        {
                            rectangle2 RegionRect = RectMinMax(V2(AtX, ThisMinY), V2(AtX + BarWidth, ThisMaxY));
                            
                            v4 HiColor = V4(1, 1, 1, 1);
                            b32 Highlight = false;
                            if(FrameIndex == DebugState->ViewingFrameOrdinal)
                            {
                                HiColor = V4(1, 1, 0, 1);
                                Highlight = true;
                            }
                            
                            if(FrameIndex == DebugState->MostRecentFrameOrdinal)
                            {
                                HiColor = V4(0, 1, 0, 1);
                                Highlight = true;
                            }
                            
                            if(FrameIndex == DebugState->CollationFrameOrdinal)
                            {
                                HiColor = V4(1, 0, 0, 1);
                                Highlight = true;
                            }
                            
                            if(FrameIndex == DebugState->OldestFrameOrdinal)
                            {
                                HiColor = V4(0, 0.5f, 0, 1);
                                Highlight = true;
                            }

                            if(Highlight)
                            {
                                PushRect(&DebugState->RenderGroup, DebugState->UITransform, RegionRect,
                                    0.0f, HiColor);
                            }
                            PushRectOutline(&DebugState->RenderGroup, DebugState->UITransform, RegionRect,
                                1.0f, V4(0.5f,0.5f,0.5f, 1), 2.0f);
                            
                            if(IsInRectangle(RegionRect, MouseP))
                            {
                                char TextBuffer[256];
                                _snprintf_s(TextBuffer, sizeof(TextBuffer), "%u", FrameIndex);
                                DEBUGTextOutAt(MouseP + V2(0.0f, 10.0f), TextBuffer);
                                
                                debug_interaction Interaction = {};
                                Interaction.ID = SliderID;
                                Interaction.Type = DebugInteraction_SetViewFrameOrdinal;
                                Interaction.UInt32 = FrameIndex;
                                DebugState->NextHotInteraction = Interaction;
                            }

                            AtX += BarWidth;
                        }
                    }
                }



39:04
Casey added the DebugInteraction_SetViewFrameOrdinal debug_interaction_type


56:53
So Casey made it so that when we select a frame, we pause the collating
that way we can view the profiler result of a specific frame

                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray)
                {    
                    for(u32 EventIndex = 0; EventIndex < EventCount; ++EventIndex)
                    {
                        debug_event *Event = EventArray + EventIndex;
                        if(Event->Type == DebugType_FrameMarker)
                        {
                            ...
                            ...

                            r32 ClockRange = (r32)(CollationFrame->EndClock - CollationFrame->BeginClock);
                            ++DebugState->TotalFrameCount;
                            
        --------------->    if(DebugState->Paused)
                            {
                                FreeFrame(DebugState, DebugState->CollationFrameOrdinal);
                            }
                            else
                            {
                                DebugState->MostRecentFrameOrdinal = DebugState->CollationFrameOrdinal;
                                IncrementFrameOrdinal(&DebugState->CollationFrameOrdinal);
                                if(DebugState->CollationFrameOrdinal == DebugState->OldestFrameOrdinal)
                                {
                                    FreeOldestFrame(DebugState);
                                }
                                CollationFrame = GetCollationFrame(DebugState);
                            }
                            InitFrame(DebugState, Event->Clock, CollationFrame);

                            ...
                            ...
                        }
                    }
                }


