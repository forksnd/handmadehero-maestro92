Handmade Hero Day 200 - Debug Element Layout

Summary:

a lot of code clean up through compression-oriented programming.

Keyword:
UI, rendering



8:35
beginning to clean up the rendering code.
so currently in the DEBUGDrawMainMenu(); we are just going through all the debug_variable_reference 
and we are drawing their bounds.

Casey created a layout struct, and bundled all the rendering related information in there

                handmade_debug.cpp

                struct layout
                {
                    debug_state *DebugState;
                    v2 MouseP;
                    v2 At;
                    int Depth;
                    real32 LineAdvance;
                    r32 SpacingY;
                };





9:02
so now, at the top of each debug_variable ui logic code, we will use layout to pass through ui data.
this way our code is cleaner

                internal void
                DEBUGDrawMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    for(debug_variable_hierarchy *Hierarchy = DebugState->HierarchySentinel.Next;
                        Hierarchy != &DebugState->HierarchySentinel;
                        Hierarchy = Hierarchy->Next)
                    {
                        layout Layout = {};
                        Layout.DebugState = DebugState;
                        Layout.MouseP = MouseP;
                        Layout.At = Hierarchy->UIP;
                        Layout.LineAdvance = DebugState->FontScale*GetLineAdvanceFor(DebugState->DebugFontInfo);
                        Layout.SpacingY = 4.0f;
                        
                        ...
                        ...
                   }
                }






25:06
Casey changing the rendering pattern to be 
    
                BeginElementRectangle();
                MakeElementSizable(&Element);
                DefaultInteraction(&Element, TearInteraction);
                EndElement(&Element);


-   the MakeElementSizable is just adding a Sizable box.
    we add this for DebugVariableType_CounterThreadList and DebugVariableType_BitmapDisplay

    for regular debug_variable, we dont have a Sizable box for them. 


-   full code below

                internal void
                DEBUGDrawMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {
                    for(debug_variable_hierarchy *Hierarchy = DebugState->HierarchySentinel.Next;
                        Hierarchy != &DebugState->HierarchySentinel;
                        Hierarchy = Hierarchy->Next)
                    {
                        layout Layout = {};
                        ...
                        ...

                        debug_variable_reference *Ref = Hierarchy->Group->Var->Group.FirstChild;
                        while(Ref)
                        {
                            debug_variable *Var = Ref->Var;
                            ...
                            ...

                            switch(Var->Type)
                            {
                                case DebugVariableType_CounterThreadList:
                                {
                                    layout_element Element = BeginElementRectangle(&Layout, &Var->Profile.Dimension);
                                    MakeElementSizable(&Element);
                                    DefaultInteraction(&Element, ItemInteraction);
                                    EndElement(&Element);

                                    DrawProfileIn(DebugState, Element.Bounds, MouseP);
                                } break;

                                case DebugVariableType_BitmapDisplay:
                                {
                                    ...
                                    ...

                                    layout_element Element = BeginElementRectangle(&Layout, &Var->BitmapDisplay.Dim);
                                    MakeElementSizable(&Element);
                                    DefaultInteraction(&Element, TearInteraction);
                                    EndElement(&Element);

                                    PushRect(DebugState->RenderGroup, Element.Bounds, 0.0f, V4(0, 0, 0, 1.0f));
                                    PushBitmap(DebugState->RenderGroup, Var->BitmapDisplay.ID, BitmapScale,
                                               V3(GetMinCorner(Element.Bounds), 0.0f), V4(1, 1, 1, 1), 0.0f);
                                } break;
                                
                                default:
                                {
                                    ...
                                    DEBUGVariableToText(...);

                                    ...
                                    
                                    layout_element Element = BeginElementRectangle(&Layout, &Dim);
                                    DefaultInteraction(&Element, ItemInteraction);
                                    EndElement(&Element);

                                    DEBUGTextOutAt(V2(GetMinCorner(Element.Bounds).x,
                                                      GetMaxCorner(Element.Bounds).y - DebugState->FontScale*GetStartingBaselineY(DebugState->DebugFontInfo)),
                                                   Text, ItemColor);
                                } break;
                            }








35:04
Casey created the layout_element struct 

                struct layout_element
                {
                    // NOTE(casey): Storage;
                    layout *Layout;
                    v2 *Dim;
                    v2 *Size;
                    debug_interaction Interaction;

                    // NOTE(casey): Out
                    rectangle2 Bounds;
                };

this pretty much represents a UI element. 



34:00
Caseu starting to write the functions 


                handmade_debug.cpp

                inline layout_element BeginElementRectangle(layout *Layout, v2 *Dim)
                {
                    layout_element Element = {};

                    Element.Layout = Layout;
                    Element.Dim = Dim;

                    return(Element);
                }


                inline void
                MakeElementSizable(layout_element *Element)
                {
                    Element->Size = Element->Dim;
                }

                inline void
                DefaultInteraction(layout_element *Element, debug_interaction Interaction)
                {
                    Element->Interaction = Interaction;
                }


37:23
then we finally work on the EndElement(); function 


-   we first do some math for calculate the InteriorBounds and the TotalBounds
    this is mainly for the ResizeBox 

                    v2 TotalMinCorner = V2(Layout->At.x + Layout->Depth*2.0f*Layout->LineAdvance,
                                           Layout->At.y - TotalDim.y);
                    v2 TotalMaxCorner = TotalMinCorner + TotalDim;

                    v2 InteriorMinCorner = TotalMinCorner + Frame;
                    v2 InteriorMaxCorner = InteriorMinCorner + *Element->Dim;

                    rectangle2 TotalBounds = RectMinMax(TotalMinCorner, TotalMaxCorner);
                    Element->Bounds = RectMinMax(InteriorMinCorner, InteriorMaxCorner);


so imagine you have the layout like below. The TotalBounds is the outter rectangle2
the InteriorBounds is the bounds that excludees the resize box.

                     _________________________
                    |                         |
                    |    .................    |     
                    |    .               .    |     
                    |    .               .    |  <--------TotalBounds    
                    |    .InteriorBounds .    |      
                    |    .               .    |      
                    |    .               .    |      
                    |    .               .    |      
                    |    .               .    |      
                    |    .               .    |      
                    |    .................____|      
                    |                    |    |  <------- resize box     
                    |____________________|____|

the numbers here are mainly used to render the resize box, as well as the borders


as you can see later, when we render the resize box, we do 

                    rectangle2 SizeBox = RectMinMax(V2(InteriorMaxCorner.x, TotalMinCorner.y),
                                                    V2(TotalMaxCorner.x, InteriorMinCorner.y));



recall the x and y axis, as you can see, we use the InteriorBounds and TotalBounds to indicate min and max corner of the resize box


    ^
    |                _________________________
    |               |                         |
    |               |    .................    |      
    |               |    .               .    |      
    |               |    .               .    |      
    |               |    .InteriorBounds .    |      
    |               |    .               .    |      
    |               |    .               .    |      
    |               |    .               .    |      
    |               |    .               .    |      
    |               |    .               .    |      
    |               |    .................____| <-------------  ( TotalMaxCorner.x, InteriorMinCorner.y )
    |               |                    |    |      
    |               |____________________|____|  
    |   
    |                                    ^
    |                                    | 
    |
    |                  ( InteriorMaxCorner.x, TotalMinCorner.y )
    |                                
    |
    ------------------------------------------------------>



-   then we draw the border of our layout_element, in which we just draw four rectangles 
    as mentioned above, we use the InteriorBounds and TotalBounds to draw the borders

                    PushRect(DebugState->RenderGroup, RectMinMax(V2(TotalMinCorner.x, InteriorMinCorner.y),
                                                                 V2(InteriorMinCorner.x, InteriorMaxCorner.y)), 0.0f, V4(0, 0, 0, 1));
                    PushRect(DebugState->RenderGroup, RectMinMax(V2(InteriorMaxCorner.x, InteriorMinCorner.y),
                                                                 V2(TotalMaxCorner.x, InteriorMaxCorner.y)), 0.0f, V4(0, 0, 0, 1));
                    PushRect(DebugState->RenderGroup, RectMinMax(V2(InteriorMinCorner.x, TotalMinCorner.y),
                                                                 V2(InteriorMaxCorner.x, InteriorMinCorner.y)), 0.0f, V4(0, 0, 0, 1));
                    PushRect(DebugState->RenderGroup, RectMinMax(V2(InteriorMinCorner.x, InteriorMaxCorner.y),
                                                                 V2(InteriorMaxCorner.x, TotalMaxCorner.y)), 0.0f, V4(0, 0, 0, 1));

-   lastly we do y advancement in our layout 

                    r32 SpacingY = Layout->SpacingY;
                    if(0)
                    {
                        SpacingY = 0.0f;
                    }
                    Layout->At.y = GetMinCorner(TotalBounds).y - SpacingY;

-   full code below 

                handmade_debug.cpp

                inline void EndElement(layout_element *Element)
                {
                    layout *Layout = Element->Layout;
                    debug_state *DebugState = Layout->DebugState;
                    
                    r32 SizeHandlePixels = 4.0f;
                    
                    v2 Frame = {0, 0};
                    if(Element->Size)
                    {
                        Frame.x = SizeHandlePixels;
                        Frame.y = SizeHandlePixels;
                    }
                    
                    v2 TotalDim = *Element->Dim + 2.0f*Frame;

                    v2 TotalMinCorner = V2(Layout->At.x + Layout->Depth*2.0f*Layout->LineAdvance,
                                           Layout->At.y - TotalDim.y);
                    v2 TotalMaxCorner = TotalMinCorner + TotalDim;

                    v2 InteriorMinCorner = TotalMinCorner + Frame;
                    v2 InteriorMaxCorner = InteriorMinCorner + *Element->Dim;

                    rectangle2 TotalBounds = RectMinMax(TotalMinCorner, TotalMaxCorner);
                    Element->Bounds = RectMinMax(InteriorMinCorner, InteriorMaxCorner);

                    if(Element->Interaction.Type && IsInRectangle(Element->Bounds, Layout->MouseP))
                    {
                        DebugState->NextHotInteraction = Element->Interaction;
                    }

                    if(Element->Size)
                    {
                        PushRect(DebugState->RenderGroup, RectMinMax(V2(TotalMinCorner.x, InteriorMinCorner.y),
                                                                     V2(InteriorMinCorner.x, InteriorMaxCorner.y)), 0.0f, V4(0, 0, 0, 1));
                        PushRect(DebugState->RenderGroup, RectMinMax(V2(InteriorMaxCorner.x, InteriorMinCorner.y),
                                                                     V2(TotalMaxCorner.x, InteriorMaxCorner.y)), 0.0f, V4(0, 0, 0, 1));
                        PushRect(DebugState->RenderGroup, RectMinMax(V2(InteriorMinCorner.x, TotalMinCorner.y),
                                                                     V2(InteriorMaxCorner.x, InteriorMinCorner.y)), 0.0f, V4(0, 0, 0, 1));
                        PushRect(DebugState->RenderGroup, RectMinMax(V2(InteriorMinCorner.x, InteriorMaxCorner.y),
                                                                     V2(InteriorMaxCorner.x, TotalMaxCorner.y)), 0.0f, V4(0, 0, 0, 1));

                        debug_interaction SizeInteraction = {};
                        SizeInteraction.Type = DebugInteraction_Resize;
                        SizeInteraction.P = Element->Size;

                        rectangle2 SizeBox = RectMinMax(V2(InteriorMaxCorner.x, TotalMinCorner.y),
                                                        V2(TotalMaxCorner.x, InteriorMinCorner.y));
                        PushRect(DebugState->RenderGroup, SizeBox, 0.0f,
                                 (InteractionIsHot(DebugState, SizeInteraction) ? V4(1, 1, 0, 1) : V4(1, 1, 1, 1)));
                        if(IsInRectangle(SizeBox, Layout->MouseP))
                        {
                            DebugState->NextHotInteraction = SizeInteraction;
                        }
                    }
                    
                    r32 SpacingY = Layout->SpacingY;
                    if(0)
                    {
                        SpacingY = 0.0f;
                    }
                    Layout->At.y = GetMinCorner(TotalBounds).y - SpacingY;
                }



1:04:03
Casey also mentioned that this UI system is quite similar to what he had on the witness 
which he has a blog about
https://caseymuratori.com/blog_0015
