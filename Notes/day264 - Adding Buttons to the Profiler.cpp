Handmade Hero Day 264 - Adding Buttons to the Profiler

Summary:
Added buttons for our UI rendering system

Keyword:
debug system, profiler, UI

Casey is thinking about adding a back button in the profiler graph once he is done selecting a frame. 


23:11
Casey adding code to add the Buttons


                case DebugType_FrameBarGraph:
                {
                    debug_view_profile_graph *Graph = &View->ProfileGraph;

                    debug_interaction ZoomRootInteraction = {};
                    ZoomRootInteraction.ID = DebugID;
                    ZoomRootInteraction.Type = DebugInteraction_SetProfileGraphRoot;
                    ZoomRootInteraction.Element = 0;
                    
                    BeginRow(Layout);
                    ActionButton(Layout, "Root", ZoomRootInteraction);
                    BooleanButton(Layout, "Threads", (Element->Type == DebugType_ThreadIntervalGraph),
                        SetElementTypeInteraction(DebugID, Element, DebugType_ThreadIntervalGraph));
                    BooleanButton(Layout, "Frames", (Element->Type == DebugType_FrameBarGraph),
                        SetElementTypeInteraction(DebugID, Element, DebugType_FrameBarGraph));
                    EndRow(Layout);
                 
                    ...
                    ...
                }


28:56
Casey made a function, that sets the interaction type for ui elements 

                handmade_debug.cpp

                inline debug_interaction SetElementTypeInteraction(debug_id DebugID, debug_element *Element, debug_type Type)
                {
                    debug_interaction Result = {};
                    Result.ID = DebugID;
                    Result.Type = DebugInteraction_SetElementType;
                    Result.Element = Element;
                    Result.DebugType = Type;
                    
                    return(Result);
                }


32:20
Casey wrote the other functions 

                internal void BeginRow(layout *Layout)
                {
                    ++Layout->NoLineFeed;
                }

                internal void ActionButton(layout *Layout, char *Name, debug_interaction Interaction)
                {
                    BasicTextElement(Layout, Name, Interaction);
                }

                internal void BooleanButton(layout *Layout, char *Name, b32 Highlight, debug_interaction Interaction)
                {
                    BasicTextElement(Layout, Name, Interaction, Highlight ? V4(1, 1, 1, 1) : V4(0.5f, 0.5f, 0.5f, 1.0f));
                }

                internal void EndRow(layout *Layout)
                {
                    Assert(Layout->NoLineFeed > 0);
                    --Layout->NoLineFeed;
                    
                    AdvanceElement(Layout, RectMinMax(Layout->At, Layout->At));
                }
