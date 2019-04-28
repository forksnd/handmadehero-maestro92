Handmade Hero Day 267 - Adding Per-Element Clipping Rectangles

Summary:
added the Clipprect struct into our renderer system.
as we generate and assemble our render_group, we also add its ClipRect information to the render_group.

we also generate the memory required for having Clipprect into our system

showed a simple hack version of clipping the debug panel,
then went on the implement the proper approach, which is to add the concept of ClipRect into the renderer system

Keyword:
Renderer



4:05
Casey mentioned that he wants to add the equivalent of "self ms" column from Unity Profiler to our Profiler 


5:26
so in the debug_profile_node, Casey added Duration, and DurationOfChildren

                struct debug_profile_node
                {
                    struct debug_element *Element;
                    struct debug_stored_event *FirstChild;
                    struct debug_stored_event *NextSameParent;
                    u64 Duration;
    ----------->    u64 DurationOfChildren;
                    u64 ParentRelativeClock;
                    u32 Reserved;
                    u16 ThreadOrdinal;
                    u16 CoreIndex;
                };


22:12
So Casey now wants to address the problem of debug elements being drawn off the ends of a clipping region.
see video at this time stamp


22:32
the quick, hacky simple fix for clipping this is in the debug rendering, we cap the At.y layout 

so as you can see, we are just doing the clipping by line, but not at the pixel/math level 

                handmade_debug.cpp

                internal void DrawTopClocksList(debug_state *DebugState, debug_id GraphID, rectangle2 ProfileRect, v2 MouseP,
                    debug_element *RootElement)
                {
                    ...
                    ...
                    
                    v2 At = V2(ProfileRect.Min.x, ProfileRect.Max.y - GetBaseline(DebugState));
                    for(Index = 0;
                        (Index < LinkCount);
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
                        
    --------------->    if(At.y < ProfileRect.Min.y)
                        {
                            break;
                        }
                        else
                        {
                            At.y -= GetLineAdvance(DebugState);    
                        }
                    }
                    
                    EndTemporaryMemory(Temp);
                }





39:24
so Casey went on to do the proper fix.
the idea is that we define the render_entry_cliprect struct, which has a rectangle2i Rect.
This will be the region where a particular bitmap will be allowed to render in. 
Casey adding the concept of a clipped rect 

                handmade_render_group.h

                struct render_entry_cliprect
                {
                    render_entry_cliprect *Next;
                    rectangle2i Rect;
                };




So for example, in our final code, we have something like to fix the clipping in our debug profile view 
so we would define all of our ui elements 
then at the end, we define the ClipRect for DebugState->RenderGroup

                internal void DEBUGDrawElement(layout *Layout, debug_tree *Tree, debug_element *Element, debug_id DebugID,
                                 u32 FrameOrdinal)
                {
                    ...
                    render_group *RenderGroup = &DebugState->RenderGroup;

                    debug_view *View = GetOrCreateDebugViewFor(DebugState, DebugID);
                    switch(Element->Type)
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

                            PushRect(&DebugState->RenderGroup, DebugState->BackingTransform,
                                LayEl.Bounds, 0.0f, V4(0, 0, 0, 0.75f));
                            
                            u32 OldClipRect = RenderGroup->CurrentClipRectIndex;
                            RenderGroup->CurrentClipRectIndex = 
        ---------->             PushClipRect(RenderGroup, DebugState->BackingTransform, LayEl.Bounds, 0.0f);

                            ...
                            ...

                            RenderGroup->CurrentClipRectIndex = OldClipRect;

                        }
                    }
                }

this way everything in this RenderGroup will be restrained into this ClipRect
then when this current element is done with the clipRect,
we set it back to the OldClipRect


1:03:15
in game_render_commands, we added 
                
                typedef struct game_render_commands
                {
                    ...
                    ...

                    u32 ClipRectCount;
                    struct render_entry_cliprect *ClipRects;
                    
                    render_entry_cliprect *FirstRect;
                    render_entry_cliprect *LastRect;
                } game_render_commands;

the idea is that, anytime during the rendering, we can record a clipRect as we push it in the render buffer. 
we also remember the index, so we can do random access. We also want to them to fit in the cache nice and compact.

so the ClipRects is gonna be that nice and compact array that we will have. 

the idea is that will first store the render_entry_cliprect in our render commands with all the other information,
then we will have to sort of all our render entries, and that render_entry_cliprect has to be sorted with the render entries 
more on this later.



1:00:55
So in the PushClipRect Function, we see below, we push the ClipRect information ont the game_render_commands Commands Buffer 
recall that we allocate some memory for the game_render_commands render buffers. So this function just returns 
the index of inside the buffer. 

we also appeneded the render_entry_cliprect to the linked list. More on this later. 

                handmade_render_group.cpp

                inline u32 PushClipRect(render_group *Group, u32 X, u32 Y, u32 W, u32 H)
                {
                    u32 Result = 0;

                    game_render_commands *Commands = Group->Commands;
                    
                    u32 Size = sizeof(render_entry_cliprect);
                    if((Commands->PushBufferSize + Size) < (Commands->SortEntryAt - sizeof(sort_entry)))
                    {
                        render_entry_cliprect *Rect = (render_entry_cliprect *)
                            (Commands->PushBufferBase + Commands->PushBufferSize);
                        Commands->PushBufferSize += Size;
                        
                        Result = Group->Commands->ClipRectCount++;
                        
                        if(Group->Commands->LastRect)
                        {
    --------------->        Group->Commands->LastRect = Group->Commands->LastRect->Next = Rect;
                        }
                        else
                        {
    --------------->        Group->Commands->LastRect = Group->Commands->FirstRect = Rect;
                        }
                        Rect->Next = 0;
                        
                        Rect->Rect.MinX = X;
                        Rect->Rect.MinY = Y;
                        Rect->Rect.MaxX = X + W;
                        Rect->Rect.MaxY = Y + H;
                    }
                    
                    return(Result);
                }


here are some other similar functions 

                inline u32 PushClipRect(render_group *Group, object_transform ObjectTransform, v3 Offset, v2 Dim)
                {
                    u32 Result = 0;
                    
                    v3 P = (Offset - V3(0.5f*Dim, 0));
                    entity_basis_p_result Basis = GetRenderEntityBasisP(Group->CameraTransform, ObjectTransform, P);
                    if(Basis.Valid)
                    {
                        v2 P = Basis.P;
                        v2 DimB = Basis.Scale*Dim;
                        
                        Result = PushClipRect(Group, 
                            RoundReal32ToInt32(P.x), RoundReal32ToInt32(P.y),
                            RoundReal32ToInt32(DimB.x), RoundReal32ToInt32(DimB.y));
                    }
                    
                    return(Result);
                }

                inline u32 PushClipRect(render_group *Group, object_transform ObjectTransform, rectangle2 Rectangle, r32 Z)
                {
                    u32 Result = PushClipRect(Group, ObjectTransform, V3(GetCenter(Rectangle), Z), GetDim(Rectangle));
                    return(Result);
                }





55:37
for things that dont have anythign special ClipRect, we just set the ClipRect to the actual size of the bitmap
as you can see here, we just set the ClipRect to PixelWidth and PixelHeight

                inline void Perspective(render_group *RenderGroup, int32 PixelWidth, int32 PixelHeight,
                            real32 MetersToPixels, real32 FocalLength, real32 DistanceAboveTarget)
                {
                    ...
                    ...
                    
    ------------>   RenderGroup->CurrentClipRectIndex = PushClipRect(RenderGroup, 0, 0, PixelWidth, PixelHeight);
                }

                inline void Orthographic(render_group *RenderGroup, int32 PixelWidth, int32 PixelHeight, real32 MetersToPixels)
                {
                    ...
                    ...

    ------------>   RenderGroup->CurrentClipRectIndex = PushClipRect(RenderGroup, 0, 0, PixelWidth, PixelHeight);
                }







then once we actually get to the rendering, 
we call SortEntries, and we now construct the nice array for our ClipRect in the LinearizeClipRects function. 

                internal void
                Win32DisplayBufferInWindow(platform_work_queue *RenderQueue, game_render_commands *Commands,
                                           HDC DeviceContext, s32 WindowWidth, s32 WindowHeight, 
                                           void *SortMemory, void *ClipRectMemory)
                {
                    SortEntries(Commands, SortMemory);
                    LinearizeClipRects(Commands, ClipRectMemory);

                /*  TODO(casey): Do we want to check for resources like before?  Probably?      
                    if(AllResourcesPresent(RenderGroup))
                    {
                        RenderToOutput(TranState->HighPriorityQueue, RenderGroup, &DrawBuffer, &TranState->TranArena);
                    }
                */

                    if(GlobalRenderingType == Win32RenderType_RenderOpenGL_DisplayOpenGL)
                    {
                        OpenGLRenderCommands(Commands, WindowWidth, WindowHeight);        
                        SwapBuffers(DeviceContext);
                    }
                    else




1:10:52
LinearizeClipRects goes from game_render_commands->FirstRect and walks towards LastRect, 
and puts them in linear order into to ClipMemory.
then it gives it back to Commands->CLipRect. 
essentially, ClipMemory is just scratch space for us to Linearize the CLipRects

                internal void LinearizeClipRects(game_render_commands *Commands, void *ClipMemory)
                {
                    render_entry_cliprect *Out = (render_entry_cliprect *)ClipMemory;
                    for(render_entry_cliprect *Rect = Commands->FirstRect;
                        Rect;
                        Rect = Rect->Next)
                    {
                        *Out++ = *Rect;
                    }
                    Commands->ClipRects = (render_entry_cliprect *)ClipMemory;
                }

recall in section 1:00:55, in one of the PushClipRect function, as we sort, we append the ClipRects 
to Group->Commands Linked List



44:26
Ofcourse, when we get to actually processing the render commands,
in our openGL rendering path, as we go through renderer commands, we have to call glScissor();

                global_variable u32 TextureBindCount = 0;
                internal void OpenGLRenderCommands(game_render_commands *Commands, s32 WindowWidth, s32 WindowHeight)
                {    
                    ...
                    ...

                    sort_entry *Entry = SortEntries;
                    for(u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
                    {
                        render_group_entry_header *Header = (render_group_entry_header *)
                            (Commands->PushBufferBase + Entry->Index);
                        if(ClipRectIndex != Header->ClipRectIndex)
                        {
                            ClipRectIndex = Header->ClipRectIndex;
                            Assert(ClipRectIndex < Commands->ClipRectCount);
                            
                            render_entry_cliprect *Clip = Commands->ClipRects + ClipRectIndex;
                            glScissor(Clip->Rect.MinX, Clip->Rect.MinY, 
                                      Clip->Rect.MaxX - Clip->Rect.MinX, 
                                      Clip->Rect.MaxY - Clip->Rect.MinY);
                        }

                        ...
                        ...
                    }
                }



glScissor is a way for us to set the region we want to draw inside of.
By default its set to the whole window. but if we want to restrict to our clip rect, glScissor is the call.




1:48:00 
Casey drew a graph during the Q/A to explain glScissor

            Screen
         ___________________________________
        |                                   |
        |                                   |
        |         glScissor                 |
        |       _______________             |
        |      |               |            |
        |      |               |            |
        |      |_______________|            |
        |                                   |
        |                                   |
        |___________________________________|

so if you call glEnable(GL_SCISSOR_TEST);, it will use the clipping region specified in glScissor,
otherwise it will use the whole screen.



1:26:40
now you see the difference between the Clipprect vs the simple one we put at section 22:32
now the bitmaps are clipped at the pixel level, now by the "Line" level, as we seen in section 22:32


Q/A
when does clipping overdrawn areas cost more than just drawing them?

never. It can only not save you time, since everything already has to be clipped, for example, it has to be clipped 
to the screen area. so narrowing the clipping can only save you time.