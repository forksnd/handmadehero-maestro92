Handmade Hero Day 266 - Adding a Top Clocks Profile View

Summary:
adding lots of profiler features (kind of like Unity_s profiler);
such as sorting by cycles, %percentage times, number of calls.

Keyword:
debug system, profiler, UI, Sorting


4:20 
Casey wants to add a top view: a view that will sort functions by how time consuming they are.



7:39
Casey added the case DebugType_TopClocksList, in the DEBUGDrawElement. notice we want all three : 
DebugType_TopClocksList, DebugType_FrameBarGraph and DebugType_ThreadIntervalGraph to be switchable 
so we just put them all together 


                internal void DEBUGDrawElement(layout *Layout, debug_tree *Tree, debug_element *Element, debug_id DebugID,
                                 u32 FrameOrdinal)
                {
                    ...
                    ...

                    case DebugType_ThreadIntervalGraph:
                    case DebugType_FrameBarGraph:
                    case DebugType_TopClocksList:
                    {
                        debug_view_profile_graph *Graph = &View->ProfileGraph;

                        BeginRow(Layout);
                        ActionButton(Layout, "Root", SetPointerInteraction(DebugID, (void **)&Graph->GUID, 0));
                        BooleanButton(Layout, "Threads", (Element->Type == DebugType_ThreadIntervalGraph),
                            SetUInt32Interaction(DebugID, (u32 *)&Element->Type, DebugType_ThreadIntervalGraph));
                        BooleanButton(Layout, "Frames", (Element->Type == DebugType_FrameBarGraph),
                            SetUInt32Interaction(DebugID, (u32 *)&Element->Type, DebugType_FrameBarGraph));
                        BooleanButton(Layout, "Clocks", (Element->Type == DebugType_TopClocksList),
                            SetUInt32Interaction(DebugID, (u32 *)&Element->Type, DebugType_TopClocksList));
                        EndRow(Layout);

                        ...
                        ...

                        switch(Element->Type)
                        {
                            case DebugType_ThreadIntervalGraph:
                            {
                                DrawProfileIn(DebugState, DebugID, LayEl.Bounds, Layout->MouseP, ViewingElement);
                            } break;

                            case DebugType_FrameBarGraph:
                            {
                                DrawFrameBars(DebugState, DebugID, LayEl.Bounds, Layout->MouseP, ViewingElement);
                            } break;
                            
                            case DebugType_TopClocksList:
                            {
                                DrawTopClocksList(DebugState, DebugID, LayEl.Bounds, Layout->MouseP, ViewingElement);
                            } break;
                        }
                    } break;

                    ...
                    ...
                }




8:52
so now Casey wants to revisit the sorting code we initially wrote, and Casey wants to reuse this for sorting 
our profile blocks by their time spent




recall that we initially have this 

                handmade_render_group.h

                struct tile_sort_entry
                {
                    r32 SortKey;
                    u32 PushBufferOffset;
                };




we use the SortKey to compare each entry, whether one entry should go infront another or not
and the PushBufferOffset to indicate its place in the sort buffer. 

so we can make this more generic, phrasing "SortKey" a floating number and "PushBufferOffset" into an index, that way we can reuse this 
                


10:56
Casey created the handmade_sort.h file, for all sorting utility code 
and Casey moved all sorting related code to handmade_sort.cpp


                handmade_sort.cpp 

                struct sort_entry
                {
                    r32 SortKey;
                    u32 Index;
                };


                internal void MergeSort(u32 Count, sort_entry *First, sort_entry *Temp)
                {
                    ...
                    ...
                }

                internal void BubbleSort(u32 Count, sort_entry *First, sort_entry *Temp)
                {
                    ...
                    ...
                }

                ...
                ...




16:12
Casey adds the DrawTopClocksList(); function 

the plan is to go through all the debug elements that are profile times debug elements 

for now we are just gonna print them, we wont sort them yet 


so what we are doing is that in the for loop, we are going through all the blocks of this type, that occured in this frame.

this includes events that started in this frame, but ended in another frame 

so we tally up the TotalClock by this type of event on this frame 

                u64 TotalClock = 0;
                for(debug_stored_event *Event = Element->Frames[DebugState->ViewingFrameOrdinal].OldestEvent;
                    Event;
                    Event = Event->Next)
                {
                    TotalClock += Event->ProfileNode.Duration;
                }


-   full code below:

                handmade_debug.cpp 

                internal void DrawTopClocksList(debug_state *DebugState, debug_id GraphID, rectangle2 ProfileRect, v2 MouseP,
                    debug_element *RootElement)
                {
                    for(debug_variable_link *Link = DebugState->ProfileGroup->Sentinel.Next;
                        Link != &DebugState->ProfileGroup->Sentinel;
                        Link = Link->Next)
                    {

                        debug_element *Element = Link->Element;

                        u64 TotalClock = 0;
                        for(debug_stored_event *Event = Element->Frames[DebugState->ViewingFrameOrdinal].OldestEvent;
                            Event;
                            Event = Event->Next)
                        {
                            TotalClock += Event->ProfileNode.Duration;
                        }

                        ...
                        ...
                    }


37:56
Casey notices that in the Input Processing, moving the mouse takes a lot of cycles




45:26
Then Casey wants to sort it based on totalClocks 

                handmade_debug.cpp 

                internal void DrawTopClocksList(debug_state *DebugState, debug_id GraphID, rectangle2 ProfileRect, v2 MouseP,
                    debug_element *RootElement)
                {
                    temporary_memory Temp = BeginTemporaryMemory(&DebugState->DebugArena);

                    u32 LinkCount = 0;
                    for(debug_variable_link *Link = DebugState->ProfileGroup->Sentinel.Next;
                        Link != &DebugState->ProfileGroup->Sentinel;
                        Link = Link->Next)
                    {
                        ++LinkCount;
                    }
                    
                    debug_clock_entry *Entries = PushArray(Temp.Arena, LinkCount, debug_clock_entry, NoClear());
                    sort_entry *SortA = PushArray(Temp.Arena, LinkCount, sort_entry, NoClear());
                    sort_entry *SortB = PushArray(Temp.Arena, LinkCount, sort_entry, NoClear());
                        
                    r64 TotalTime = 0.0f;
                    v2 At = V2(ProfileRect.Min.x, ProfileRect.Max.y - GetBaseline(DebugState));
                    u32 Index = 0;
                    for(debug_variable_link *Link = DebugState->ProfileGroup->Sentinel.Next;
                        Link != &DebugState->ProfileGroup->Sentinel;
                        Link = Link->Next, ++Index)
                    {
                        Assert(!Link->Children);
                        
                        debug_clock_entry *Entry = Entries + Index;
                        sort_entry *Sort = SortA + Index;

                        Entry->Element = Link->Element;
                        debug_element *Element = Entry->Element;

                        BeginDebugStatistic(&Entry->Stats);
                        for(debug_stored_event *Event = Element->Frames[DebugState->ViewingFrameOrdinal].OldestEvent;
                            Event;
                            Event = Event->Next)
                        {
                            AccumDebugStatistic(&Entry->Stats, (r64)Event->ProfileNode.Duration);
                        }
                        EndDebugStatistic(&Entry->Stats);
                        TotalTime += Entry->Stats.Sum;
                        
                        Sort->SortKey = -(r32)Entry->Stats.Sum;
                        Sort->Index = Index;
                    }
                    
                    RadixSort(LinkCount, SortA, SortB);
                    
                    r64 PC = 0.0f;
                    if(TotalTime > 0)
                    {
                        PC = 100.0f / TotalTime;
                    }
                    
                    for(Index = 0;
                        Index < LinkCount;
                        ++Index)
                    {
                        debug_clock_entry *Entry = Entries + SortA[Index].Index;
                        debug_statistic *Stats = &Entry->Stats;
                        debug_element *Element = Entry->Element;
                        
                        char TextBuffer[256];
                        _snprintf_s(TextBuffer, sizeof(TextBuffer),
                            "%10ucy %02.02f%% %4d %s",
                            (u32)Stats->Sum,
                            (PC*Stats->Sum),
                            Stats->Count,
                            Element->GUID + Element->NameStartsAt);
                        TextOutAt(DebugState, At, TextBuffer);
                        At.y -= GetLineAdvance(DebugState);    
                    }
                    
                    EndTemporaryMemory(Temp);
                }



58:14
Casey pointed out that we have duplicated entries for DEBUGStart and DEBUGEnd, but not other code blocks. 
everytime we do a code reload, the guid for a particular entry will change, because its no longer the same code 
anymore. 

so for the DEBUGStart and DEBUGEnd, this is all because we created different GUID markers 


1:11:04
turns out its just windows taking its sweet time in PeekMessage();
