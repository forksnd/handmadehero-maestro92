Handmade Hero Day 260 - Implementing Drill-down in the Profiler

Summary:
implemented drill down view in the profiler. 

added a recursive call of DrawProfileBars(); to draw all the profile regions

added a new DebugInteraction type: DebugInteraction_SetProfileGraphRoot, to support selecting a profile node.


Keyword:
debug system, profiler 



20:08
So Casey plans to implement the drill down in the Profiler
Casey added a recursive DrawProfileBars(); call that 


                internal void DrawProfileIn(debug_state *DebugState, debug_id GraphID, rectangle2 ProfileRect, v2 MouseP,
                    debug_stored_event *RootEvent)
                {
                    DebugState->MouseTextStackY = 10.0f;
                    
                    debug_profile_node *RootNode = &RootEvent->ProfileNode;
                    object_transform NoTransform = DefaultFlatTransform();
                    PushRect(&DebugState->RenderGroup, DebugState->BackingTransform, ProfileRect, 0.0f, V4(0, 0, 0, 0.25f));

                    u32 LaneCount = DebugState->FrameBarLaneCount;
                    r32 LaneHeight = 0.0f;
                    if(LaneCount > 0)
                    {
                        LaneHeight = GetDim(ProfileRect).y / (r32)LaneCount;
                    }            
                    
                    DrawProfileBars(DebugState, GraphID, ProfileRect, MouseP, RootNode, LaneHeight, LaneHeight);
                }

notice that at the end of this function, we call DrawProfileBars again. So its recursive.
and every level we go down, we call LaneHeight/2

                internal void DrawProfileBars(debug_state *DebugState, debug_id GraphID, rectangle2 ProfileRect, v2 MouseP,
                                debug_profile_node *RootNode, r32 LaneStride, r32 LaneHeight)
                {
                    ...
                    ...

                    for(debug_stored_event *StoredEvent = RootNode->FirstChild;
                        StoredEvent;
                        StoredEvent = StoredEvent->ProfileNode.NextSameParent)
                    {
                        ...
                        ...

                        DrawProfileBars(DebugState, GraphID, RegionRect, MouseP, Node, 0, LaneHeight/2);
                    }
                }


29:40
Casey wants to add the pause functionality profiler. but he claims that he implement it in the next episode

30:21
instead he said he will implementing clicking into a profile node and have it fill in the entire profile region 



46:48
Casey added a new type of debug_interaction_type: DebugInteraction_SetProfileGraphRoot
this is for clicking into a PorfileGraphRoot and populating the entire profile graph with it.

                
                handmade_debug.h 

                enum debug_interaction_type
                {
                    DebugInteraction_None,

                    DebugInteraction_NOP,

                    DebugInteraction_AutoModifyVariable,

                    DebugInteraction_ToggleValue,
                    DebugInteraction_DragValue,
                    DebugInteraction_TearValue,

                    DebugInteraction_Resize,
                    DebugInteraction_Move,

                    DebugInteraction_Select,

                    DebugInteraction_ToggleExpansion,
                    
    ----------->    DebugInteraction_SetProfileGraphRoot,
                };


47:43
and of course we added the case for DebugInteraction_SetProfileGraphRoot in DEBUGEndInteract(); function

                handmade_debug.cpp

                internal void DEBUGEndInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
                {
                    switch(DebugState->Interaction.Type)
                    {
                        case DebugInteraction_ToggleExpansion:
                        {
                            ...
                            ...
                        } break;
                        
                        case DebugInteraction_SetProfileGraphRoot:
                        {
                            debug_view *View = GetOrCreateDebugViewFor(DebugState, DebugState->Interaction.ID);
                            View->ProfileGraph.GUID = DebugState->Interaction.Element->GUID;
                        } break;

                        case DebugInteraction_ToggleValue:
                        {
                            ...
                            ...
                        } break;
                    }
                }



51:15
then in the DrawProfileBars(); function. we added the logic for ZoomInteraction.

                internal void DrawProfileBars(debug_state *DebugState, debug_id GraphID, rectangle2 ProfileRect, v2 MouseP,
                                debug_profile_node *RootNode, r32 LaneStride, r32 LaneHeight)
                {
                    ...
                    ...

                    for(debug_stored_event *StoredEvent = RootNode->FirstChild;
                        StoredEvent;
                        StoredEvent = StoredEvent->ProfileNode.NextSameParent)
                    {
                        ...
                        ...

                        PushRectOutline(&DebugState->RenderGroup, DebugState->UITransform, RegionRect,
                            0.0f, V4(Color, 1), 2.0f);
                        
                        if(IsInRectangle(RegionRect, MouseP))
                        {
                            char TextBuffer[256];
                            _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                "%s: %10ucy",
                                Element->GUID, Node->Duration);
                            DEBUGTextOutAt(MouseP + V2(0.0f, DebugState->MouseTextStackY), TextBuffer);
                            DebugState->MouseTextStackY -= GetLineAdvance(DebugState);

    ------------------->    debug_interaction ZoomInteraction = {};
                            ZoomInteraction.ID = GraphID;
                            ZoomInteraction.Type = DebugInteraction_SetProfileGraphRoot;
                            ZoomInteraction.Element = Element;
                            DebugState->NextHotInteraction = ZoomInteraction;
                        }
                    
                        DrawProfileBars(DebugState, GraphID, RegionRect, MouseP, Node, 0, LaneHeight/2);
                    }
                }



52:54
so in the DebugType_ThreadIntervalGraph, we draw the proper "RootNode"

if we have selected on a RootNode, we draw that one 
                RootNode = Frame->RootProfileNode;


-   full code below:
                internal void DEBUGDrawElement(layout *Layout, debug_tree *Tree, debug_element *Element, debug_id DebugID)
                {
                    ...
                    ...

                    if(StoredEvent)
                    {
                        ...
                        ...
                        switch(Event->Type)
                        {
                            case DebugType_bitmap_id:
                            {
                                ...
                                ...
                            } break;

                            case DebugType_ThreadIntervalGraph:
                            {
                                layout_element Element = BeginElementRectangle(Layout, &View->ProfileGraph.Block.Dim);
                                MakeElementSizable(&Element);
                                //                DefaultInteraction(&Element, ItemInteraction);
                                EndElement(&Element);
                                
                                debug_stored_event *RootNode = 0;
                                
                                // TODO(casey): Need to figure out how we're going to get specific frames
                                // less slowly than linear search?
                                debug_frame *Frame = DebugState->MostRecentFrame;
                                if(Frame)
                                {
                                    debug_element *ViewingElement = GetElementFromGUID(DebugState, View->ProfileGraph.GUID);
                                    if(ViewingElement)
                                    {
                                        for(debug_stored_event *Search = ViewingElement->OldestEvent;
                                            Search;
                                            Search = Search->Next)
                                        {
                                            if(Search->FrameIndex == Frame->FrameIndex)
                                            {
                                                RootNode = Search;
                                            }
                                        }
                                    }
                                        
                                    if(!RootNode)
                                    {
                                        RootNode = Frame->RootProfileNode;
                                    }
                                }
                                
                                if(RootNode)
                                {
                                    DrawProfileIn(DebugState, DebugID, Element.Bounds, Layout->MouseP, RootNode);
                                }
                            } break;


