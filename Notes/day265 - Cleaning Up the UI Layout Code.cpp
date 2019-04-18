Handmade Hero Day 265 - Cleaning Up the UI Layout Code

Summary:
changed the debug_interaction struct to be more generic

created the handmade_debug_ui.h file, moved all the ui code, ui layout code there.

explained how we are implementing lambdas in our ui system

Keyword:
debug system, profiler, UI



5:56
Casey added a "Pause" button in the FrameSlider 

                case DebugType_FrameSlider:
                {
                    ...
                    ...

                    BooleanButton(Layout, "Pause", DebugState->Paused,
                        SetUInt32Interaction(DebugID, (u32 *)&DebugState->Paused, !DebugState->Paused));

                    DrawFrameSlider(DebugState, DebugID, LayEl.Bounds, Layout->MouseP, Element);
                } break;



notice in the BooleanButton function, we are passing a debug_interaction,

                internal void BooleanButton(layout *Layout, char *Name, b32 Highlight, debug_interaction Interaction)
                {
                    BasicTextElement(Layout, Name, Interaction, 
                        Highlight ? V4(1, 1, 1, 1) : V4(0.5f, 0.5f, 0.5f, 1.0f), V4(1, 1, 1, 1),
                        4.0f, V4(0.0f, 0.5f, 1.0f, 1.0f));
                }



the debug_interaction here is produced by the SetUInt32Interaction function 
                
                handmad_debug_ui.cpp

                inline debug_interaction SetUInt32Interaction(debug_id DebugID, u32 *Target, u32 Value)
                {
                    debug_interaction Result = {};
                    Result.ID = DebugID;
                    Result.Type = DebugInteraction_SetUInt32;
                    Result.Target = Target;
                    Result.UInt32 = Value;

                    return(Result);
                }

notice in the in the SetUInt32Interaction(); function, it takes in a u32* Target.
essentially, it takes in a target address that it can write data into 


so Casey aims to make the UI a little more generalized so that we can reduce repeated Code
Casey added the 

                enum debug_interaction_type
                {
                    DebugInteraction_None,

                    ...
                    ...

                    DebugInteraction_ToggleExpansion,

    ------------>   DebugInteraction_SetUInt32,
                    DebugInteraction_SetPointer,
                };


11:08
with this, Casey changed the structure in debug_interaction
where Casey added the "void* Target" field

                handmad_debug_ui.h

                struct debug_interaction
                {
                    debug_id ID;
                    debug_interaction_type Type;

    ----------->    void *Target;
                    union
                    {
                        void *Generic;
                        void *Pointer;
                        u32 UInt32;
                        debug_tree *Tree;
                        debug_variable_link *Link;
                        debug_type DebugType;
                        v2 *P;
                        debug_element *Element;
                    };
                };


this way we have a very generic debug_interaction that we can use for anything 



18:29
Casey also added the SetPointerInteraction(); function 
this essentially will be changing the value for pointers

                handmade_debug.cpp

                inline debug_interaction SetPointerInteraction(debug_id DebugID, void **Target, void *Value)
                {
                    debug_interaction Result = {};
                    Result.ID = DebugID;
                    Result.Type = DebugInteraction_SetPointer;
                    Result.Target = Target;
                    Result.Pointer = Value;

                    return(Result);
                }


with these two InteractionType, we were able to get rid of 

                DebugInteraction_SetProfileGraphRoot,
                DebugInteraction_SetViewFrameOrdinal,
                
these two types 



27:58
Casey mentioned that currently in handmade_debug.h, half of it is UI, the other half is collation code ,
so Casey split it into a separate file.

so Casey created the handmade_debug_ui.h, and Casey moved a bunch of stuff there.





54:48
Casey added the BasicTextElement, which will render a BackdropColor if specified.

    
                handmade_debug_ui.cpp

                internal v2 BasicTextElement(layout *Layout, char *Text, debug_interaction ItemInteraction,
                                 v4 ItemColor = V4(0.8f, 0.8f, 0.8f, 1), v4 HotColor = V4(1, 1, 1, 1),
                                 r32 Border = 0.0f, v4 BackdropColor = V4(0, 0, 0, 0))
                {
                    debug_state *DebugState = Layout->DebugState;

                    rectangle2 TextBounds = GetTextSize(DebugState, Text);
                    v2 Dim = {GetDim(TextBounds).x + 2.0f*Border, Layout->LineAdvance + 2.0f*Border};
                    
                    layout_element Element = BeginElementRectangle(Layout, &Dim);
                    DefaultInteraction(&Element, ItemInteraction);
                    EndElement(&Element);

                    b32 IsHot = InteractionIsHot(Layout->DebugState, ItemInteraction);

                    TextOutAt(DebugState, V2(GetMinCorner(Element.Bounds).x + Border,
                                GetMaxCorner(Element.Bounds).y - Border - 
                                DebugState->FontScale*GetStartingBaselineY(DebugState->DebugFontInfo)),
                            Text, IsHot ? HotColor : ItemColor);
                    if(BackdropColor.w > 0.0f)
                    {
                        PushRect(&DebugState->RenderGroup, 
                                 DebugState->BackingTransform, Element.Bounds, 0.0f, BackdropColor);
                    }
                    
                    return(Dim);
                }

Q/A 

1:03:45
Can you explain how your implementation of lambda works? (this is referring to Casey implementing function calls for the buttons);


things that would be very easy to do in languages that support full closure, is very difficult in C++. 
So you can either use what C++ has for lambdas now.

with lambdas, you can get a pointer back to its parent_s stack. 


so in our implementation of our lambdas, lets say DrawFrameSlider(); 


                internal void DrawFrameSlider(debug_state *DebugState, debug_id SliderID, rectangle2 TotalRect, v2 MouseP,
                                debug_element *RootElement)
                {
                    u32 FrameCount = ArrayCount(RootElement->Frames);
                    if(FrameCount > 0)
                    {
                        object_transform NoTransform = DefaultFlatTransform();
                        PushRect(&DebugState->RenderGroup, DebugState->BackingTransform, TotalRect, 0.0f, V4(0, 0, 0, 0.25f));
                        ...
                        ...

                        for(u32 FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
                        {
                            rectangle2 RegionRect = RectMinMax(V2(AtX, ThisMinY), V2(AtX + BarWidth, ThisMaxY));

                            ...
                            ...

        --------------->    if(IsInRectangle(RegionRect, MouseP))
                            {
                                char TextBuffer[256];
                                _snprintf_s(TextBuffer, sizeof(TextBuffer), "%u", FrameIndex);
                                TextOutAt(DebugState, MouseP + V2(0.0f, 10.0f), TextBuffer);

                                DebugState->NextHotInteraction = 
                                    SetUInt32Interaction(SliderID, &DebugState->ViewingFrameOrdinal, FrameIndex);
                            }

                            AtX += BarWidth;
                        }
                    }
                }


we do a check to see if the mouse is clicking onto an Element

                if(IsInRectangle(RegionRect, MouseP))
                {
                    ...
                    ...

                    DebugState->NextHotInteraction = 
                        SetUInt32Interaction(SliderID, &DebugState->ViewingFrameOrdinal, FrameIndex);
                }

and we want to do an interaction that takes place if your mouse clicks into it. 
the most straight forward way to implement this in a immiedate mose user interface design 
is to do the operation right there 

                if(IsInRectangle(RegionRect, MouseP))
                {
                    (... do your shit, whatever you want to do ...);
    ------------>   DebugState=ViewingFrameOrdinal = FrameIndex 
                }


this is a bit tricky because it makes it difficult to do certian more complicated operations 

so in our case, we just made a structure that can remember what we wanted to do, so we can do it later 
when we actually go process the UI 

                DebugState->NextHotInteraction = 
                    SetUInt32Interaction(SliderID, &DebugState->ViewingFrameOrdinal, FrameIndex);

so this is our poor man_s implementation of closure 
