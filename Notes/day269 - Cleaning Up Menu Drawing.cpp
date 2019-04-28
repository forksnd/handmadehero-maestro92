Handmade Hero Day 269 - Cleaning Up Menu Drawing

Summary:
made tooltip code more general, made is so that any ui debug element can add tooltips
made tooptip text not restrained to clipping. 


Keyword:
UI, Code cleanup


25:09
Casey trying to make the tooltip/hover tip more general so that any ui debug element can use it.
currently only the when you hover ontop of a profile block that the tooltip is showing some text 

so Casey moved abunch of ui from handmade_debug.cpp to handmade_debug_ui.cpp



35:58
Casey added the AddTooltip function

                handmade_debug_ui.cpp

                internal void AddTooltip(debug_state *DebugState, char *Text)
                {                    
                    layout *Layout = &DebugState->MouseTextLayout;
                    
                    rectangle2 TextBounds = GetTextSize(DebugState, Text);
                    v2 Dim = {GetDim(TextBounds).x, Layout->LineAdvance};
                    
                    layout_element Element = BeginElementRectangle(Layout, &Dim);
                    EndElement(&Element);
                    
                    TextOutAt(DebugState, V2(GetMinCorner(Element.Bounds).x,
                                GetMaxCorner(Element.Bounds).y - 
                                DebugState->FontScale*GetStartingBaselineY(DebugState->DebugFontInfo)),
                            Text, V4(1, 1, 1, 1), 10000.0f);
                }



41:07
Casey now making the tooptip not clip by the debug menu RenderGroup panel
casey made it so that tooltip text will never be clipped. How clipping is covered in day 267


                handmade_debug_ui.cpp

                internal void AddTooltip(debug_state *DebugState, char *Text)
                {
    ----------->    render_group *RenderGroup = &DebugState->RenderGroup;
                    u32 OldClipRect = RenderGroup->CurrentClipRectIndex;
                    RenderGroup->CurrentClipRectIndex = DebugState->DefaultClipRect;
                    
                    layout *Layout = &DebugState->MouseTextLayout;
                    
                    rectangle2 TextBounds = GetTextSize(DebugState, Text);
                    v2 Dim = {GetDim(TextBounds).x, Layout->LineAdvance};
                    
                    layout_element Element = BeginElementRectangle(Layout, &Dim);
                    EndElement(&Element);
                    
                    TextOutAt(DebugState, V2(GetMinCorner(Element.Bounds).x,
                                GetMaxCorner(Element.Bounds).y - 
                                DebugState->FontScale*GetStartingBaselineY(DebugState->DebugFontInfo)),
                            Text, V4(1, 1, 1, 1), 10000.0f);
                        
    ----------->    RenderGroup->CurrentClipRectIndex = OldClipRect;
                }


nothing interesting
