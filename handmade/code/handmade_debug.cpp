/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

// TODO(casey): Stop using stdio!
#include <stdio.h>
#include <stdlib.h>

#include "handmade_debug.h"
#include "handmade_debug_ui.cpp"

struct debug_parsed_name
{
    u32 HashValue;
    u32 FileNameCount;
    u32 NameStartsAt;
    u32 LineNumber;

    u32 NameLength;
    char *Name;
};
inline debug_parsed_name
DebugParseName(char *GUID)
{
    debug_parsed_name Result = {};

    u32 PipeCount = 0;
    char *Scan = GUID;
    for(;
        *Scan;
        ++Scan)
    {
        if(*Scan == '|')
        {
            if(PipeCount == 0)
            {
                Result.FileNameCount = (u32)(Scan - GUID);
                Result.LineNumber = atoi(Scan + 1);
            }
            else if(PipeCount == 1)
            {

            }
            else
            {
                Result.NameStartsAt = (u32)(Scan - GUID + 1);
            }

            ++PipeCount;
        }

        // TODO(casey): Better hash function
        Result.HashValue = 65599*Result.HashValue + *Scan;
    }

    Result.NameLength = (u32)(Scan - GUID) - Result.NameStartsAt;
    Result.Name = GUID + Result.NameStartsAt;

    return(Result);
}

inline debug_element *
GetElementFromGUID(debug_state *DebugState, u32 Index, char *GUID)
{
    debug_element *Result = 0;

    for(debug_element *Chain = DebugState->ElementHash[Index];
        Chain;
        Chain = Chain->NextInHash)
    {
        if(StringsAreEqual(Chain->GUID, GUID))
        {
            Result = Chain;
            break;
        }
    }

    return(Result);
}

inline debug_element *
GetElementFromGUID(debug_state *DebugState, char *GUID)
{
    debug_element *Result = 0;

    if(GUID)
    {
        debug_parsed_name ParsedName = DebugParseName(GUID);
        u32 Index = (ParsedName.HashValue % ArrayCount(DebugState->ElementHash));

        Result = GetElementFromGUID(DebugState, Index, GUID);
    }

    return(Result);
}

inline debug_id
DebugIDFromLink(debug_tree *Tree, debug_variable_link *Link)
{
    debug_id Result = {};

    Result.Value[0] = Tree;
    Result.Value[1] = Link;

    return(Result);
}

inline debug_id
DebugIDFromGUID(debug_tree *Tree, char *GUID)
{
    debug_id Result = {};

    Result.Value[0] = Tree;
    Result.Value[1] = GUID;

    return(Result);
}

inline debug_state *
DEBUGGetState(game_memory *Memory)
{
    debug_state *DebugState = 0;
    if(Memory)
    {
        DebugState = (debug_state *)Memory->DebugStorage;
        if(!DebugState->Initialized)
        {
            DebugState = 0;
        }
    }

    return(DebugState);
}

inline debug_state *
DEBUGGetState(void)
{
    debug_state *Result = DEBUGGetState(DebugGlobalMemory);

    return(Result);
}

internal debug_tree *
AddTree(debug_state *DebugState, debug_variable_group *Group, v2 AtP)
{
    debug_tree *Tree = PushStruct(&DebugState->DebugArena, debug_tree);

    Tree->UIP = AtP;
    Tree->Group = Group;

    DLIST_INSERT(&DebugState->TreeSentinel, Tree);

    return(Tree);
}

inline void
BeginDebugStatistic(debug_statistic *Stat)
{
    Stat->Min = Real32Maximum;
    Stat->Max = -Real32Maximum;
    Stat->Avg = 0.0f;
    Stat->Count = 0;
}

inline void
AccumDebugStatistic(debug_statistic *Stat, r64 Value)
{
    ++Stat->Count;

    if(Stat->Min > Value)
    {
        Stat->Min = Value;
    }

    if(Stat->Max < Value)
    {
        Stat->Max = Value;
    }

    Stat->Avg += Value;
}

inline void
EndDebugStatistic(debug_statistic *Stat)
{
    if(Stat->Count)
    {
        Stat->Avg /= (r64)Stat->Count;
    }
    else
    {
        Stat->Min = 0.0f;
        Stat->Max = 0.0f;
    }
}

internal memory_index
DEBUGEventToText(char *Buffer, char *End, debug_event *Event, u32 Flags)
{
    char *At = Buffer;
    char *Name = Event->GUID;

    if(Flags & DEBUGVarToText_AddDebugUI)
    {
        At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                          "#define DEBUGUI_");
    }

    if(Flags & DEBUGVarToText_AddName)
    {
        char *UseName = Name;
        if(!(Flags & DEBUGVarToText_ShowEntireGUID))
        {
            for(char *Scan = Name;
                *Scan;
                ++Scan)
            {
                if((Scan[0] == '|') &&
                   (Scan[1] != 0))
                {
                    UseName = Scan + 1;
                }
            }
        }

        At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                          "%s%s ", UseName, (Flags & DEBUGVarToText_Colon) ? ":" : "");
    }

    if(Flags & DEBUGVarToText_AddValue)
    {
        switch(Event->Type)
        {
            case DebugType_r32:                
            {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                    "%f", Event->Value_r32);
                if(Flags & DEBUGVarToText_FloatSuffix)
                {
                    *At++ = 'f';
                }
            } break;

            case DebugType_b32:                
            {
                if(Flags & DEBUGVarToText_PrettyBools)
                {
                    At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                        "%s",
                        Event->Value_b32 ? "true" : "false");
                }
                else
                {
                    At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                        "%d", Event->Value_b32);
                }
            } break;

            case DebugType_s32:
            {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                    "%d", Event->Value_s32);
            } break;

            case DebugType_u32:   
            {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                    "%u", Event->Value_u32);
            } break;

            case DebugType_v2:
            {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                    "V2(%f, %f)",
                    Event->Value_v2.x, Event->Value_v2.y);
            } break;

            case DebugType_v3:
            {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                    "V3(%f, %f, %f)",
                    Event->Value_v3.x, Event->Value_v3.y, Event->Value_v3.z);
            } break;

            case DebugType_v4:
            {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                    "V4(%f, %f, %f, %f)",
                    Event->Value_v4.x, Event->Value_v4.y,
                    Event->Value_v4.z, Event->Value_v4.w);
            } break;

            case DebugType_rectangle2:
            {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                    "Rect2(%f, %f -> %f, %f)",
                    Event->Value_rectangle2.Min.x,
                    Event->Value_rectangle2.Min.y,
                    Event->Value_rectangle2.Max.x,
                    Event->Value_rectangle2.Max.y);
            } break;

            case DebugType_rectangle3:
            {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                    "Rect2(%f, %f, %f -> %f, %f, %f)",
                    Event->Value_rectangle3.Min.x,
                    Event->Value_rectangle3.Min.y,
                    Event->Value_rectangle3.Min.z,
                    Event->Value_rectangle3.Max.x,
                    Event->Value_rectangle3.Max.y,
                    Event->Value_rectangle3.Max.z);
            } break;

            case DebugType_bitmap_id:
            {
            } break;

            default:
            {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                    "UNHANDLED: %s", Event->GUID);
            } break;
        }
    }

    if(Flags & DEBUGVarToText_LineFeedEnd)
    {
        *At++ = '\n';
    }

    if(Flags & DEBUGVarToText_NullTerminator)
    {
        *At++ = 0;
    }

    return(At - Buffer);
}

struct debug_variable_iterator
{
    debug_variable_link *Link;
    debug_variable_link *Sentinel;
};

internal debug_view *
GetOrCreateDebugViewFor(debug_state *DebugState, debug_id ID)
{
    // TODO(casey): Better hash function
    u32 HashIndex = ((U32FromPointer(ID.Value[0]) >> 2) + (U32FromPointer(ID.Value[1]) >> 2)) % ArrayCount(DebugState->ViewHash);
    debug_view **HashSlot = DebugState->ViewHash + HashIndex;

    debug_view *Result = 0;
    for(debug_view *Search = *HashSlot;
        Search;
        Search = Search->NextInHash)
    {
        if(DebugIDsAreEqual(Search->ID, ID))
        {
            Result = Search;
            break;
        }
    }

    if(!Result)
    {
        Result = PushStruct(&DebugState->DebugArena, debug_view);
        Result->ID = ID;
        Result->Type = DebugViewType_Unknown;
        Result->NextInHash = *HashSlot;
        *HashSlot = Result;
    }

    return(Result);
}

inline debug_interaction
ElementInteraction(debug_state *DebugState, debug_id DebugID, debug_interaction_type Type, debug_element *Element)
{    
    debug_interaction ItemInteraction = {};
    ItemInteraction.ID = DebugID;    
    ItemInteraction.Type = Type;
    ItemInteraction.Element = Element;

    return(ItemInteraction);
}

inline debug_interaction
DebugIDInteraction(debug_interaction_type Type, debug_id ID)
{
    debug_interaction ItemInteraction = {};
    ItemInteraction.ID = ID;
    ItemInteraction.Type = Type;

    return(ItemInteraction);
}

inline debug_interaction
DebugLinkInteraction(debug_interaction_type Type, debug_variable_link *Link)
{
    debug_interaction ItemInteraction = {};
    ItemInteraction.Link = Link;
    ItemInteraction.Type = Type;

    return(ItemInteraction);
}

internal b32
IsSelected(debug_state *DebugState, debug_id ID)
{
    b32 Result = false;

    for(u32 Index = 0;
        Index < DebugState->SelectedIDCount;
        ++Index)
    {
        if(DebugIDsAreEqual(ID, DebugState->SelectedID[Index]))
        {
            Result = true;
            break;
        }
    }

    return(Result);
}

internal void
ClearSelection(debug_state *DebugState)
{
    DebugState->SelectedIDCount = 0;
}

internal void
AddToSelection(debug_state *DebugState, debug_id ID)
{
    if((DebugState->SelectedIDCount < ArrayCount(DebugState->SelectedID)) &&
       !IsSelected(DebugState, ID))
    {
        DebugState->SelectedID[DebugState->SelectedIDCount++] = ID;
    }
}

internal void
DEBUG_HIT(debug_id ID, r32 ZValue)
{
    debug_state *DebugState = DEBUGGetState();
    if(DebugState)
    {
        DebugState->NextHotInteraction = DebugIDInteraction(DebugInteraction_Select, ID);
    }
}

internal b32
DEBUG_HIGHLIGHTED(debug_id ID, v4 *Color)
{
    b32 Result = false;

    debug_state *DebugState = DEBUGGetState();
    if(DebugState)
    {
        if(IsSelected(DebugState, ID))
        {
            *Color = V4(0, 1, 1, 1);
            Result = true;
        }

        if(DebugIDsAreEqual(DebugState->HotInteraction.ID, ID))
        {
            *Color = V4(1, 1, 0, 1);
            Result = true;
        }
    }

    return(Result);
}

internal b32
DEBUG_REQUESTED(debug_id ID)
{
    b32 Result = false;

    debug_state *DebugState = DEBUGGetState();
    if(DebugState)
    {
        Result = IsSelected(DebugState, ID)
            || DebugIDsAreEqual(DebugState->HotInteraction.ID, ID);
    }

    return(Result);
}

internal u64
GetTotalClocks(debug_element_frame *Frame)
{
    u64 Result = 0;
    for(debug_stored_event *Event = Frame->OldestEvent;
        Event;
        Event = Event->Next)
    {
        Result += Event->ProfileNode.Duration;
    }
    return(Result);
}

global_variable v3 DebugColorTable[] =
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
    //    {0, 0.5f, 1},
};

internal void
DrawProfileBars(debug_state *DebugState, debug_id GraphID, rectangle2 ProfileRect, v2 MouseP,
                debug_profile_node *RootNode, r32 LaneStride, r32 LaneHeight)
{
    r32 FrameSpan = (r32)(RootNode->Duration);
    r32 PixelSpan = GetDim(ProfileRect).x;

    r32 Scale = 0.0f;
    if(FrameSpan > 0)
    {
        Scale = PixelSpan / FrameSpan;
    }

    for(debug_stored_event *StoredEvent = RootNode->FirstChild;
        StoredEvent;
        StoredEvent = StoredEvent->ProfileNode.NextSameParent)
    {
        debug_profile_node *Node = &StoredEvent->ProfileNode;
        debug_element *Element = Node->Element;
        Assert(Element);

        v3 Color = DebugColorTable[U32FromPointer(Element->GUID)%ArrayCount(DebugColorTable)];
        r32 ThisMinX = ProfileRect.Min.x + Scale*(r32)(Node->ParentRelativeClock);
        r32 ThisMaxX = ThisMinX + Scale*(r32)(Node->Duration);

        u32 LaneIndex = Node->ThreadOrdinal;
        r32 LaneY = ProfileRect.Max.y - LaneStride*LaneIndex;
        rectangle2 RegionRect = RectMinMax(V2(ThisMinX, LaneY - LaneHeight),
                                           V2(ThisMaxX, LaneY));

        PushRectOutline(&DebugState->RenderGroup, DebugState->UITransform, RegionRect,
            0.0f, V4(Color, 1), 2.0f);

        // TODO(casey): Pull this out so all profilers share it.
        if(IsInRectangle(RegionRect, MouseP))
        {
            char TextBuffer[256];
            _snprintf_s(TextBuffer, sizeof(TextBuffer),
                "%s: %10ucy",
                Element->GUID, Node->Duration);
            TextOutAt(DebugState, MouseP + V2(0.0f, DebugState->MouseTextStackY), TextBuffer);
            DebugState->MouseTextStackY -= GetLineAdvance(DebugState);

            // TODO(casey): It would be better to generate a graph+element debug ID here!
            debug_view *View = GetOrCreateDebugViewFor(DebugState, GraphID);
            DebugState->NextHotInteraction = 
                SetPointerInteraction(GraphID, (void **)&View->ProfileGraph.GUID, Element->GUID);
        }

//        DrawProfileBars(DebugState, GraphID, RegionRect, MouseP, Node, 0, LaneHeight/2);
    }
}

internal void
DrawProfileIn(debug_state *DebugState, debug_id GraphID, rectangle2 ProfileRect, v2 MouseP,
    debug_element *RootElement)
{
    DebugState->MouseTextStackY = 10.0f;
    object_transform NoTransform = DefaultFlatTransform();
    PushRect(&DebugState->RenderGroup, DebugState->BackingTransform, ProfileRect, 0.0f, V4(0, 0, 0, 0.25f));

    u32 LaneCount = DebugState->FrameBarLaneCount;
    r32 LaneHeight = 0.0f;
    if(LaneCount > 0)
    {
        LaneHeight = GetDim(ProfileRect).y / (r32)LaneCount;
    }            

    debug_element_frame *RootFrame = RootElement->Frames + DebugState->ViewingFrameOrdinal;
    r32 NextX = ProfileRect.Min.x;
    u64 TotalClock = GetTotalClocks(RootFrame);
    u64 RelativeClock = 0;
    for(debug_stored_event *Event = RootFrame->OldestEvent;
        Event;
        Event = Event->Next)
    {
        debug_profile_node *Node = &Event->ProfileNode;
        rectangle2 EventRect = ProfileRect;

        RelativeClock += Node->Duration;
        r32 t = (r32)((r64)RelativeClock / (r64)TotalClock);
        EventRect.Min.x = NextX;
        EventRect.Max.x = (1.0f - t)*ProfileRect.Min.x + t*ProfileRect.Max.x;
        NextX = EventRect.Max.x;

        DrawProfileBars(DebugState, GraphID, EventRect, MouseP, Node, LaneHeight, LaneHeight);
    }
}

internal void
DrawFrameBars(debug_state *DebugState, debug_id GraphID, rectangle2 ProfileRect, v2 MouseP,
              debug_element *RootElement)
{
    u32 FrameCount = ArrayCount(RootElement->Frames);
    if(FrameCount > 0)
    {
        DebugState->MouseTextStackY = 10.0f;

        object_transform NoTransform = DefaultFlatTransform();
        PushRect(&DebugState->RenderGroup, DebugState->BackingTransform, ProfileRect, 0.0f, V4(0, 0, 0, 0.25f));

        r32 BarWidth = (GetDim(ProfileRect).x / (r32)FrameCount);
        r32 AtX = ProfileRect.Min.x;
        for(u32 FrameIndex = 0;
            FrameIndex < FrameCount;
            ++FrameIndex)
        {
            debug_stored_event *RootEvent = RootElement->Frames[FrameIndex].MostRecentEvent;
            if(RootEvent)
            {
                debug_profile_node *RootNode = &RootEvent->ProfileNode;
                r32 FrameSpan = (r32)(RootNode->Duration);
                r32 PixelSpan = GetDim(ProfileRect).y;
                r32 Scale = 0.0f;
                if(FrameSpan > 0)
                {
                    Scale = PixelSpan / FrameSpan;
                }

                for(debug_stored_event *StoredEvent = RootNode->FirstChild;
                    StoredEvent;
                    StoredEvent = StoredEvent->ProfileNode.NextSameParent)
                {
                    debug_profile_node *Node = &StoredEvent->ProfileNode;
                    debug_element *Element = Node->Element;
                    Assert(Element);

                    v3 Color = DebugColorTable[U32FromPointer(Element->GUID)%ArrayCount(DebugColorTable)];
                    r32 ThisMinY = ProfileRect.Min.y + Scale*(r32)(Node->ParentRelativeClock);
                    r32 ThisMaxY = ThisMinY + Scale*(r32)(Node->Duration);

                    rectangle2 RegionRect = RectMinMax(V2(AtX, ThisMinY), V2(AtX + BarWidth, ThisMaxY));

                    PushRectOutline(&DebugState->RenderGroup, DebugState->UITransform, RegionRect,
                        0.0f, V4(Color, 1), 2.0f);

                    if(IsInRectangle(RegionRect, MouseP))
                    {
                        char TextBuffer[256];
                        _snprintf_s(TextBuffer, sizeof(TextBuffer),
                            "%s: %10ucy",
                            Element->GUID, Node->Duration);
                        TextOutAt(DebugState, MouseP + V2(0.0f, DebugState->MouseTextStackY), TextBuffer);
                        DebugState->MouseTextStackY -= GetLineAdvance(DebugState);

                        debug_view *View = GetOrCreateDebugViewFor(DebugState, GraphID);
                        DebugState->NextHotInteraction = 
                            SetPointerInteraction(GraphID, (void **)&View->ProfileGraph.GUID, Element->GUID);
                    }
                }

                AtX += BarWidth;
            }
        }
    }
}

internal void
DrawFrameSlider(debug_state *DebugState, debug_id SliderID, rectangle2 TotalRect, v2 MouseP,
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
                TextOutAt(DebugState, MouseP + V2(0.0f, 10.0f), TextBuffer);

                DebugState->NextHotInteraction = 
                    SetUInt32Interaction(SliderID, &DebugState->ViewingFrameOrdinal, FrameIndex);
            }

            AtX += BarWidth;
        }
    }
}

internal void
DEBUGDrawElement(layout *Layout, debug_tree *Tree, debug_element *Element, debug_id DebugID,
                 u32 FrameOrdinal)
{
    object_transform NoTransform = DefaultFlatTransform();

    debug_state *DebugState = Layout->DebugState;
    render_group *RenderGroup = &DebugState->RenderGroup;

    debug_interaction ItemInteraction =
        ElementInteraction(DebugState, DebugID, DebugInteraction_AutoModifyVariable, Element);

    b32 IsHot = InteractionIsHot(DebugState, ItemInteraction);
    v4 ItemColor = IsHot ? V4(1, 1, 0, 1) : V4(1, 1, 1, 1);

    debug_stored_event *OldestStoredEvent = 
        Element->Frames[DebugState->ViewingFrameOrdinal].OldestEvent;

    debug_view *View = GetOrCreateDebugViewFor(DebugState, DebugID);
    switch(Element->Type)
    {
        case DebugType_bitmap_id:
        {
            debug_event *Event = OldestStoredEvent ? &OldestStoredEvent->Event : 0;
            loaded_bitmap *Bitmap = 0;
            r32 BitmapScale = View->InlineBlock.Dim.y;
            if(Event)
            {
                Bitmap = GetBitmap(RenderGroup->Assets, Event->Value_bitmap_id, RenderGroup->GenerationID);
                if(Bitmap)
                {
                    used_bitmap_dim Dim = GetBitmapDim(RenderGroup, NoTransform, Bitmap, BitmapScale, V3(0.0f, 0.0f, 0.0f), 1.0f);
                    View->InlineBlock.Dim.x = Dim.Size.x;
                }
            }

            layout_element LayEl = BeginElementRectangle(Layout, &View->InlineBlock.Dim);
            MakeElementSizable(&LayEl);
            DefaultInteraction(&LayEl, ItemInteraction);
            EndElement(&LayEl);
            PushRect(&DebugState->RenderGroup, NoTransform, LayEl.Bounds, 0.0f, V4(0, 0, 0, 1.0f));

            if(Bitmap)
            {
                PushBitmap(&DebugState->RenderGroup, NoTransform, Event->Value_bitmap_id, BitmapScale,
                    V3(GetMinCorner(LayEl.Bounds), 0.0f), V4(1, 1, 1, 1), 0.0f);
            }
        } break;

        case DebugType_ThreadIntervalGraph:
        case DebugType_FrameBarGraph:
        {
            debug_view_profile_graph *Graph = &View->ProfileGraph;

            BeginRow(Layout);
            ActionButton(Layout, "Root", SetPointerInteraction(DebugID, (void **)&Graph->GUID, 0));
            BooleanButton(Layout, "Threads", (Element->Type == DebugType_ThreadIntervalGraph),
                SetUInt32Interaction(DebugID, (u32 *)&Element->Type, DebugType_ThreadIntervalGraph));
            BooleanButton(Layout, "Frames", (Element->Type == DebugType_FrameBarGraph),
                SetUInt32Interaction(DebugID, (u32 *)&Element->Type, DebugType_FrameBarGraph));
            EndRow(Layout);

            layout_element LayEl = BeginElementRectangle(Layout, &Graph->Block.Dim);
            if((Graph->Block.Dim.x == 0) && (Graph->Block.Dim.y == 0))
            {
                Graph->Block.Dim.x = 1400;
                Graph->Block.Dim.y = 480;
            }



            MakeElementSizable(&LayEl);
            //                DefaultInteraction(&LayEl, ItemInteraction);
            EndElement(&LayEl);

            debug_stored_event *RootNode = 0;

            u32 ViewingFrameOrdinal = DebugState->ViewingFrameOrdinal;
            debug_element *ViewingElement = GetElementFromGUID(DebugState, View->ProfileGraph.GUID);
            if(!ViewingElement)
            {
                ViewingElement = DebugState->RootProfileElement;
            }

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
            }
        } break;

        case DebugType_FrameSlider:
        {
            v2 Dim = {1400, 32};
            layout_element LayEl = BeginElementRectangle(Layout, &Dim);
            EndElement(&LayEl);

            BeginRow(Layout);
            BooleanButton(Layout, "Pause", DebugState->Paused,
                SetUInt32Interaction(DebugID, (u32 *)&DebugState->Paused, !DebugState->Paused));
            ActionButton(Layout, "Oldest", 
                SetUInt32Interaction(DebugID, &DebugState->ViewingFrameOrdinal,
                    DebugState->OldestFrameOrdinal));
            ActionButton(Layout, "Most Recent", 
                SetUInt32Interaction(DebugID, &DebugState->ViewingFrameOrdinal,
                    DebugState->MostRecentFrameOrdinal));
            EndRow(Layout);

            DrawFrameSlider(DebugState, DebugID, LayEl.Bounds, Layout->MouseP, Element);
        } break;

        case DebugType_LastFrameInfo:
        {
            debug_frame *MostRecentFrame = DebugState->Frames + DebugState->ViewingFrameOrdinal;
            char Text[256];
            _snprintf_s(Text, sizeof(Text),
                "Viewing frame time: %.02fms %de %dp %dd",
                MostRecentFrame->WallSecondsElapsed * 1000.0f,
                MostRecentFrame->StoredEventCount,
                MostRecentFrame->ProfileBlockCount,
                MostRecentFrame->DataBlockCount);

            BasicTextElement(Layout, Text, ItemInteraction);
        } break;

        case DebugType_DebugMemoryInfo:
        {
            char Text[256];
            _snprintf_s(Text, sizeof(Text),
                "Per-frame arena space remaining: %ukb",
                (u32)(GetArenaSizeRemaining(&DebugState->PerFrameArena, AlignNoClear(1)) / 1024));

            BasicTextElement(Layout, Text, ItemInteraction);
        } break;

        default:
        {
            debug_event NullEvent = {};
            NullEvent.GUID = Element->GUID;
            NullEvent.Type = (u8)Element->Type;

            debug_event *Event = OldestStoredEvent ? &OldestStoredEvent->Event : &NullEvent;
            char Text[256];
            DEBUGEventToText(Text, Text + sizeof(Text), Event,
                DEBUGVarToText_AddName|
                    DEBUGVarToText_AddValue|
                    DEBUGVarToText_NullTerminator|
                    DEBUGVarToText_Colon|
                    DEBUGVarToText_PrettyBools);

            BasicTextElement(Layout, Text, ItemInteraction);
        } break;
    }
}

internal void
DEBUGDrawMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
{
    object_transform NoTransform = DefaultFlatTransform();

    u32 FrameOrdinal = DebugState->ViewingFrameOrdinal;
    for(debug_tree *Tree = DebugState->TreeSentinel.Next;
        Tree != &DebugState->TreeSentinel;
        Tree = Tree->Next)
    {
        layout Layout = BeginLayout(DebugState, MouseP, Tree->UIP);
        
        u32 Depth = 0;
        debug_variable_iterator Stack[DEBUG_MAX_VARIABLE_STACK_DEPTH];

        debug_variable_group *Group = Tree->Group;
        if(Group)
        {
            Stack[Depth].Link = Group->Sentinel.Next;
            Stack[Depth].Sentinel = &Group->Sentinel;
            ++Depth;
            while(Depth > 0)
            {
                debug_variable_iterator *Iter = Stack + (Depth - 1);
                if(Iter->Link == Iter->Sentinel)
                {
                    --Depth;
                }
                else
                {
                    Layout.Depth = Depth;

                    debug_variable_link *Link = Iter->Link;
                    Iter->Link = Iter->Link->Next;

                    if(Link->Children)
                    {
                        debug_id ID = DebugIDFromLink(Tree, Link);
                        debug_view *View = GetOrCreateDebugViewFor(DebugState, ID);
                        debug_interaction ItemInteraction = DebugIDInteraction(DebugInteraction_ToggleExpansion, ID);
                        if(DebugState->AltUI)
                        {
                            ItemInteraction = DebugLinkInteraction(DebugInteraction_TearValue, Link);
                        }

                        char *Text = Link->Children->Name;

                        rectangle2 TextBounds = GetTextSize(DebugState, Text);
                        v2 Dim = {GetDim(TextBounds).x, Layout.LineAdvance};

                        layout_element Element = BeginElementRectangle(&Layout, &Dim);
                        DefaultInteraction(&Element, ItemInteraction);
                        EndElement(&Element);

                        b32 IsHot = InteractionIsHot(DebugState, ItemInteraction);
                        v4 ItemColor = IsHot ? V4(1, 1, 0, 1) : V4(1, 1, 1, 1);

                        TextOutAt(DebugState, V2(GetMinCorner(Element.Bounds).x,
                                          GetMaxCorner(Element.Bounds).y - DebugState->FontScale*GetStartingBaselineY(DebugState->DebugFontInfo)),
                                       Text, ItemColor);

                        if(View->Collapsible.ExpandedAlways)
                        {
                            Iter = Stack + Depth;
                            Iter->Link = Link->Children->Sentinel.Next;
                            Iter->Sentinel = &Link->Children->Sentinel;
                            ++Depth;
                        }
                    }
                    else
                    {
                        debug_id DebugID = DebugIDFromLink(Tree, Link);
                        DEBUGDrawElement(&Layout, Tree, Link->Element, DebugID, FrameOrdinal);
                    }
                }
            }    
        }

        DebugState->AtY = Layout.At.y;

        if(1)
        {
            debug_interaction MoveInteraction = {};
            MoveInteraction.Type = DebugInteraction_Move;
            MoveInteraction.P = &Tree->UIP;

            rectangle2 MoveBox = RectCenterHalfDim(Tree->UIP - V2(4.0f, 4.0f), V2(4.0f, 4.0f));
            PushRect(RenderGroup, NoTransform, MoveBox, 0.0f,
                     InteractionIsHot(DebugState, MoveInteraction) ? V4(1, 1, 0, 1) : V4(1, 1, 1, 1));

            if(IsInRectangle(MoveBox, MouseP))
            {
                DebugState->NextHotInteraction = MoveInteraction;
            }
        }
        
        EndLayout(&Layout);
    }

#if 0
    u32 NewHotMenuIndex = ArrayCount(DebugVariableList);
    r32 BestDistanceSq = Real32Maximum;

    r32 MenuRadius = 400.0f;
    r32 AngleStep = Tau32 / (r32)ArrayCount(DebugVariableList);
    for(u32 MenuItemIndex = 0;
        MenuItemIndex < ArrayCount(DebugVariableList);
        ++MenuItemIndex)
    {
        debug_variable *Var = DebugVariableList + MenuItemIndex;
        char *Text = Var->Name;

        v4 ItemColor = Var->Value ? V4(1, 1, 1, 1) : V4(0.5f, 0.5f, 0.5f, 1);
        if(MenuItemIndex == DebugState->HotMenuIndex)
        {
            ItemColor = V4(1, 1, 0, 1);
        }

        r32 Angle = (r32)MenuItemIndex*AngleStep;
        v2 TextP = DebugState->MenuP + MenuRadius*Arm2(Angle);

        r32 ThisDistanceSq = LengthSq(TextP - MouseP);
        if(BestDistanceSq > ThisDistanceSq)
        {
            NewHotMenuIndex = MenuItemIndex;
            BestDistanceSq = ThisDistanceSq;
        }

        rectangle2 TextBounds = DEBUGGetTextSize(DebugState, Text);        
        DEBUGTextOutAt(TextP - 0.5f*GetDim(TextBounds), Text, ItemColor);
    }

    if(LengthSq(MouseP - DebugState->MenuP) > Square(MenuRadius))
    {
        DebugState->HotMenuIndex = NewHotMenuIndex;
    }
    else
    {
        DebugState->HotMenuIndex = ArrayCount(DebugVariableList);
    }
#endif
}

internal void
DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
{
    u32 FrameOrdinal = DebugState->MostRecentFrameOrdinal;
    if(DebugState->HotInteraction.Type)
    {
        if(DebugState->HotInteraction.Type == DebugInteraction_AutoModifyVariable)
        {
            switch(DebugState->HotInteraction.Element->Frames[FrameOrdinal].MostRecentEvent->Event.Type)
            {
                case DebugType_b32:
                {
                    DebugState->HotInteraction.Type = DebugInteraction_ToggleValue;
                } break;

                case DebugType_r32:
                {
                    DebugState->HotInteraction.Type = DebugInteraction_DragValue;
                } break;

                case DebugType_OpenDataBlock:
                {
                    DebugState->HotInteraction.Type = DebugInteraction_ToggleValue;
                } break;
            }
        }

        switch(DebugState->HotInteraction.Type)
        {
            case DebugInteraction_TearValue:
            {
                debug_variable_group *RootGroup = CloneVariableGroup(DebugState, DebugState->HotInteraction.Link);
                debug_tree *Tree = AddTree(DebugState, RootGroup, MouseP);
                DebugState->HotInteraction.Type = DebugInteraction_Move;
                DebugState->HotInteraction.P = &Tree->UIP;
            } break;

            case DebugInteraction_Select:
            {
                if(!Input->ShiftDown)
                {
                    ClearSelection(DebugState);
                }
                AddToSelection(DebugState, DebugState->HotInteraction.ID);
            } break;                
        }

        DebugState->Interaction = DebugState->HotInteraction;
    }
    else
    {
        DebugState->Interaction.Type = DebugInteraction_NOP;
    }
}

internal debug_element *
GetElementFromEvent(debug_state *DebugState, debug_event *Event, debug_variable_group *Parent = 0,
                    b32 CreateHierarchy = true);
void
DEBUGMarkEditedEvent(debug_state *DebugState, debug_event *Event)
{
    if(Event)
    {
        GlobalDebugTable->EditEvent = *Event;
        GlobalDebugTable->EditEvent.GUID = 
            GetElementFromEvent(DebugState, Event)->OriginalGUID;
    }
}

internal void
DEBUGEndInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
{
    u32 FrameOrdinal = DebugState->MostRecentFrameOrdinal;
    switch(DebugState->Interaction.Type)
    {
        case DebugInteraction_ToggleExpansion:
        {
            debug_view *View = GetOrCreateDebugViewFor(DebugState, DebugState->Interaction.ID);
            View->Collapsible.ExpandedAlways = !View->Collapsible.ExpandedAlways;
        } break;
        
        case DebugInteraction_SetUInt32:
        {
            *(u32 *)DebugState->Interaction.Target = DebugState->Interaction.UInt32;
        } break;
        
        case DebugInteraction_SetPointer:
        {
            *(void **)DebugState->Interaction.Target = DebugState->Interaction.Pointer;
        } break;

        case DebugInteraction_ToggleValue:
        {
            debug_event *Event = &DebugState->Interaction.Element->Frames[FrameOrdinal].
                MostRecentEvent->Event;
            Assert(Event);
            switch(Event->Type)
            {
                case DebugType_b32:
                {
                    Event->Value_b32 = !Event->Value_b32;
                } break;
            }
            DEBUGMarkEditedEvent(DebugState, Event);
        } break;
    }

    DebugState->Interaction.Type = DebugInteraction_None;
    DebugState->Interaction.Generic = 0;
}

internal void
DEBUGInteract(debug_state *DebugState, game_input *Input, v2 MouseP)
{
    v2 dMouseP = MouseP - DebugState->LastMouseP;

/*
    if(Input->MouseButtons[PlatformMouseButton_Right].EndedDown)
    {
        if(Input->MouseButtons[PlatformMouseButton_Right].HalfTransitionCount > 0)
        {
            DebugState->MenuP = MouseP;
        }            
        DrawDebugMainMenu(DebugState, RenderGroup, MouseP);
    }
    else if(Input->MouseButtons[PlatformMouseButton_Right].HalfTransitionCount > 0)
*/
    if(DebugState->Interaction.Type)
    {
        u32 FrameOrdinal = DebugState->MostRecentFrameOrdinal;
        debug_tree *Tree = DebugState->Interaction.Tree;
        v2 *P = DebugState->Interaction.P;

        // NOTE(casey): Mouse move interaction
        switch(DebugState->Interaction.Type)
        {
            case DebugInteraction_DragValue:
            {
                debug_event *Event = DebugState->Interaction.Element ? 
                    &DebugState->Interaction.Element->Frames[FrameOrdinal].MostRecentEvent->Event : 0;
                switch(Event->Type)
                {
                    case DebugType_r32:
                    {
                        Event->Value_r32 += 0.1f*dMouseP.y;
                    } break;
                }
                DEBUGMarkEditedEvent(DebugState, Event);
            } break;

            case DebugInteraction_Resize:
            {
                *P += V2(dMouseP.x, -dMouseP.y);
                P->x = Maximum(P->x, 10.0f);
                P->y = Maximum(P->y, 10.0f);
            } break;

            case DebugInteraction_Move:
            {
                *P += V2(dMouseP.x, dMouseP.y);
            } break;
        }

        // NOTE(casey): Click interaction
        for(u32 TransitionIndex = Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
            TransitionIndex > 1;
            --TransitionIndex)
        {
            DEBUGEndInteract(DebugState, Input, MouseP);
            DEBUGBeginInteract(DebugState, Input, MouseP);
        }

        if(!Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
        {
            DEBUGEndInteract(DebugState, Input, MouseP);
        }
    }
    else
    {
        DebugState->HotInteraction = DebugState->NextHotInteraction;

        for(u32 TransitionIndex = Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
            TransitionIndex > 1;
            --TransitionIndex)
        {
            DEBUGBeginInteract(DebugState, Input, MouseP);
            DEBUGEndInteract(DebugState, Input, MouseP);
        }

        if(Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
        {
            DEBUGBeginInteract(DebugState, Input, MouseP);
        }
    }

    DebugState->LastMouseP = MouseP;
}

inline u32
GetLaneFromThreadIndex(debug_state *DebugState, u32 ThreadIndex)
{
    u32 Result = 0;

    // TODO(casey): Implement thread ID lookup.

    return(Result);
}

internal debug_thread *
GetDebugThread(debug_state *DebugState, u32 ThreadID)
{
    debug_thread *Result = 0;
    for(debug_thread *Thread = DebugState->FirstThread;
        Thread;
        Thread = Thread->Next)
    {
        if(Thread->ID == ThreadID)
        {
            Result = Thread;
            break;
        }
    }

    if(!Result)
    {
        FREELIST_ALLOCATE(Result, DebugState->FirstFreeThread, PushStruct(&DebugState->DebugArena, debug_thread));

        Result->ID = ThreadID;
        Result->LaneIndex = DebugState->FrameBarLaneCount++;
        Result->FirstOpenCodeBlock = 0;
        Result->FirstOpenDataBlock = 0;
        Result->Next = DebugState->FirstThread;
        DebugState->FirstThread = Result;
    }

    return(Result);
}

#if 0
debug_frame_region *
AddRegion(debug_state *DebugState, debug_frame *CurrentFrame)
{
    Assert(CurrentFrame->RegionCount < MAX_REGIONS_PER_FRAME);
    debug_frame_region *Result = CurrentFrame->Regions + CurrentFrame->RegionCount++;

    return(Result);
}
#endif

internal void
FreeVariableGroup(debug_state *DebugState, debug_variable_group *Group)
{
    Assert(!"Not implemented");
}

internal debug_variable_link *
AddElementToGroup(debug_state *DebugState, debug_variable_group *Parent, debug_element *Element)
{
    debug_variable_link *Link = PushStruct(&DebugState->DebugArena, debug_variable_link);

    DLIST_INSERT_AS_LAST(&Parent->Sentinel, Link);
    Link->Children = 0;
    Link->Element = Element;

    return(Link);
}

internal debug_variable_link *
AddGroupToGroup(debug_state *DebugState, debug_variable_group *Parent, debug_variable_group *Group)
{
    debug_variable_link *Link = PushStruct(&DebugState->DebugArena, debug_variable_link);

    DLIST_INSERT_AS_LAST(&Parent->Sentinel, Link);
    Link->Children = Group;
    Link->Element = 0;

    return(Link);
}

internal debug_variable_group *
CreateVariableGroup(debug_state *DebugState, u32 NameLength, char *Name)
{
    debug_variable_group *Group = PushStruct(&DebugState->DebugArena, debug_variable_group);    
    DLIST_INIT(&Group->Sentinel);

    Group->Name = PushAndNullTerminate(&DebugState->DebugArena, NameLength, Name);

    return(Group);
}

internal debug_variable_link *
CloneVariableLink(debug_state *DebugState, debug_variable_group *DestGroup, debug_variable_link *Source)
{
    debug_variable_link *Dest = AddElementToGroup(DebugState, DestGroup, Source->Element);
    if(Source->Children)
    {
        Dest->Children = PushStruct(&DebugState->DebugArena, debug_variable_group);    
        DLIST_INIT(&Dest->Children->Sentinel);
        Dest->Children->Name = Source->Children->Name;
        for(debug_variable_link *Child = Source->Children->Sentinel.Next;
            Child != &Source->Children->Sentinel;
            Child = Child->Next)
        {
            CloneVariableLink(DebugState, Dest->Children, Child);
        }
    }

    return(Dest);
}    

internal debug_variable_group *
CloneVariableGroup(debug_state *DebugState, debug_variable_link *Source)
{
    char *Name = PushString(&DebugState->DebugArena, "Cloned");
    debug_variable_group *Result = CreateVariableGroup(DebugState, StringLength(Name), Name);
    CloneVariableLink(DebugState, Result, Source);
    return(Result);
}

internal debug_variable_group *
GetOrCreateGroupWithName(debug_state *DebugState, debug_variable_group *Parent, u32 NameLength, char *Name)
{
    debug_variable_group *Result = 0;
    for(debug_variable_link *Link = Parent->Sentinel.Next;
        Link != &Parent->Sentinel;
        Link = Link->Next)
    {
        if(Link->Children && StringsAreEqual(NameLength, Name, Link->Children->Name))
        {
            Result = Link->Children;
        }
    }

    if(!Result)
    {
        Result = CreateVariableGroup(DebugState, NameLength, Name);
        AddGroupToGroup(DebugState, Parent, Result);
    }

    return(Result);
}

internal debug_variable_group *
GetGroupForHierarchicalName(debug_state *DebugState, debug_variable_group *Parent, char *Name, b32 CreateTerminal)
{
    debug_variable_group *Result = Parent;

    char *FirstSeparator = 0;
    char *Scan = Name;
    for(;
        *Scan;
        ++Scan)
    {
        if(*Scan == '/')
        {
            FirstSeparator = Scan;
            break;
        }
    }

    if(FirstSeparator || CreateTerminal)
    {
        u32 NameLength = 0;
        if(FirstSeparator)
        {
            NameLength = (u32)(FirstSeparator - Name);
        }
        else
        {
            NameLength = (u32)(Scan - Name);
        }

        Result = GetOrCreateGroupWithName(DebugState, Parent, NameLength, Name);
        if(FirstSeparator)
        {
            Result = GetGroupForHierarchicalName(DebugState, Result, FirstSeparator + 1, CreateTerminal);
        }
    }

    return(Result);
}

inline open_debug_block *
AllocateOpenDebugBlock(debug_state *DebugState, debug_element *Element,
                       u32 FrameIndex, debug_event *Event,
                       open_debug_block **FirstOpenBlock)
{
    open_debug_block *Result = 0;
    FREELIST_ALLOCATE(Result, DebugState->FirstFreeBlock, PushStruct(&DebugState->DebugArena, open_debug_block));

    Result->StartingFrameIndex = FrameIndex;
    Result->BeginClock = Event->Clock;
    Result->Element = Element;
    Result->NextFree = 0;

    Result->Parent = *FirstOpenBlock;
    *FirstOpenBlock = Result;

    return(Result);
}

inline void
DeallocateOpenDebugBlock(debug_state *DebugState, open_debug_block **FirstOpenBlock)
{
    open_debug_block *FreeBlock = *FirstOpenBlock;
    *FirstOpenBlock = FreeBlock->Parent;

    FreeBlock->NextFree = DebugState->FirstFreeBlock;
    DebugState->FirstFreeBlock = FreeBlock;                            
}

inline b32
EventsMatch(debug_event A, debug_event B)
{
    // TODO(casey): Have counters for blocks?
    b32 Result = (A.ThreadID == B.ThreadID);

    return(Result);
}

internal void
FreeFrame(debug_state *DebugState, u32 FrameOrdinal)
{
    Assert(FrameOrdinal < DEBUG_FRAME_COUNT);

    u32 FreedEventCount = 0;

    for(u32 ElementHashIndex = 0;
        ElementHashIndex < ArrayCount(DebugState->ElementHash);
        ++ElementHashIndex)
    {
        for(debug_element *Element = DebugState->ElementHash[ElementHashIndex];
            Element;
            Element = Element->NextInHash)
        {
            debug_element_frame *ElementFrame = Element->Frames + FrameOrdinal;
            while(ElementFrame->OldestEvent)
            {
                debug_stored_event *FreeEvent = ElementFrame->OldestEvent;
                ElementFrame->OldestEvent = FreeEvent->Next;
                FREELIST_DEALLOCATE(FreeEvent, DebugState->FirstFreeStoredEvent);
                ++FreedEventCount;
            }
            ZeroStruct(*ElementFrame);
        }
    }

    debug_frame *Frame = DebugState->Frames + FrameOrdinal;
    Assert(Frame->StoredEventCount == FreedEventCount);

    ZeroStruct(*Frame);
}

internal void
InitFrame(debug_state *DebugState, u64 BeginClock, debug_frame *Result)
{
    Result->FrameIndex = DebugState->TotalFrameCount++;
    Result->FrameBarScale = 1.0f;
    Result->BeginClock = BeginClock;
}

inline void
IncrementFrameOrdinal(u32 *Ordinal)
{
    *Ordinal = (*Ordinal+1) % DEBUG_FRAME_COUNT;
}

internal void
FreeOldestFrame(debug_state *DebugState)
{
    FreeFrame(DebugState, DebugState->OldestFrameOrdinal);

    if(DebugState->OldestFrameOrdinal == DebugState->MostRecentFrameOrdinal)
    {
        IncrementFrameOrdinal(&DebugState->MostRecentFrameOrdinal);
    }
    IncrementFrameOrdinal(&DebugState->OldestFrameOrdinal);
}

inline debug_frame *
GetCollationFrame(debug_state *DebugState)
{
    debug_frame *Result = DebugState->Frames + DebugState->CollationFrameOrdinal;

    return(Result);
}

internal debug_stored_event *
StoreEvent(debug_state *DebugState, debug_element *Element, debug_event *Event)
{
    debug_stored_event *Result = 0;
    while(!Result)
    {
        Result = DebugState->FirstFreeStoredEvent;
        if(Result)
        {
            DebugState->FirstFreeStoredEvent = Result->NextFree;
        }
        else
        {
            if(ArenaHasRoomFor(&DebugState->PerFrameArena, sizeof(debug_stored_event)))
            {
                Result = PushStruct(&DebugState->PerFrameArena, debug_stored_event);
            }
            else
            {
                FreeOldestFrame(DebugState);
            }
        }
    }

    debug_frame *CollationFrame = GetCollationFrame(DebugState);

    Result->Next = 0;
    Result->FrameIndex = CollationFrame->FrameIndex;
    Result->Event = *Event;

    ++CollationFrame->StoredEventCount;

    debug_element_frame *Frame = Element->Frames + DebugState->CollationFrameOrdinal;
    if(Frame->MostRecentEvent)
    {
        Frame->MostRecentEvent = Frame->MostRecentEvent->Next = Result;
    }
    else
    {
        Frame->OldestEvent = Frame->MostRecentEvent = Result;
    }

    return(Result);
}

internal debug_element *
GetElementFromEvent(debug_state *DebugState, debug_event *Event, debug_variable_group *Parent,
                    b32 CreateHierarchy)
{
    Assert(Event->GUID);

    if(!Parent)
    {
        Parent = DebugState->RootGroup;
    }

    debug_parsed_name ParsedName = DebugParseName(Event->GUID);
    u32 Index = (ParsedName.HashValue % ArrayCount(DebugState->ElementHash));

    debug_element *Result = GetElementFromGUID(DebugState, Index, Event->GUID);
    if(!Result)
    {
        Result = PushStruct(&DebugState->DebugArena, debug_element);

        Result->OriginalGUID = Event->GUID;
        Result->GUID = PushString(&DebugState->DebugArena, Event->GUID);
        Result->FileNameCount = ParsedName.FileNameCount;
        Result->LineNumber = ParsedName.LineNumber;
        Result->NameStartsAt = ParsedName.NameStartsAt;
        Result->Type = (debug_type)Event->Type;

        Result->NextInHash = DebugState->ElementHash[Index];
        DebugState->ElementHash[Index] = Result;

        debug_variable_group *ParentGroup = Parent;
        if(CreateHierarchy)
        {
            ParentGroup = GetGroupForHierarchicalName(DebugState, Parent, GetName(Result), false);
            AddElementToGroup(DebugState, ParentGroup, Result);
        }
    }

    return(Result);
}

internal void
CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray)
{    
    for(u32 EventIndex = 0;
        EventIndex < EventCount;
        ++EventIndex)
    {
        debug_event *Event = EventArray + EventIndex;
        if(Event->Type == DebugType_FrameMarker)
        {
            debug_frame *CollationFrame = GetCollationFrame(DebugState);

            CollationFrame->EndClock = Event->Clock;
            if(CollationFrame->RootProfileNode)
            {
                CollationFrame->RootProfileNode->ProfileNode.Duration =
                    (CollationFrame->EndClock - CollationFrame->BeginClock);
            }

            CollationFrame->WallSecondsElapsed = Event->Value_r32;

            r32 ClockRange = (r32)(CollationFrame->EndClock - CollationFrame->BeginClock);
            ++DebugState->TotalFrameCount;

            if(DebugState->Paused)
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
        }
        else 
        {
            debug_frame *CollationFrame = GetCollationFrame(DebugState);

            Assert(CollationFrame);

            u32 FrameIndex = DebugState->TotalFrameCount - 1;
            debug_thread *Thread = GetDebugThread(DebugState, Event->ThreadID);
            u64 RelativeClock = Event->Clock - CollationFrame->BeginClock;

            debug_variable_group *DefaultParentGroup = DebugState->RootGroup;
            if(Thread->FirstOpenDataBlock)
            {
                DefaultParentGroup = Thread->FirstOpenDataBlock->Group;
            }

            switch(Event->Type)
            {
                case DebugType_BeginBlock:
                {
                    ++CollationFrame->ProfileBlockCount;
                    debug_element *Element = 
                        GetElementFromEvent(DebugState, Event, DebugState->ProfileGroup, false);

                    debug_stored_event *ParentEvent = CollationFrame->RootProfileNode;
                    u64 ClockBasis = CollationFrame->BeginClock;
                    if(Thread->FirstOpenCodeBlock)
                    {
                        ParentEvent = Thread->FirstOpenCodeBlock->Node;
                        ClockBasis = Thread->FirstOpenCodeBlock->BeginClock;
                    }
                    else if(!ParentEvent)
                    {
                        debug_event NullEvent = {};
                        ParentEvent = StoreEvent(DebugState, DebugState->RootProfileElement, &NullEvent);
                        debug_profile_node *Node = &ParentEvent->ProfileNode;
                        Node->Element = 0;
                        Node->FirstChild = 0;
                        Node->NextSameParent = 0;
                        Node->ParentRelativeClock = 0;
                        Node->Duration = 0;
                        Node->ThreadOrdinal = 0;
                        Node->CoreIndex = 0;

                        ClockBasis = CollationFrame->BeginClock;
                        CollationFrame->RootProfileNode = ParentEvent;
                    }

                    debug_stored_event *StoredEvent = StoreEvent(DebugState, Element, Event);
                    debug_profile_node *Node = &StoredEvent->ProfileNode;
                    Node->Element = Element;
                    Node->FirstChild = 0;
                    Node->ParentRelativeClock = Event->Clock - ClockBasis;
                    Node->Duration = 0;
                    Node->ThreadOrdinal = (u16)Thread->LaneIndex;
                    Node->CoreIndex = Event->CoreIndex;

                    Node->NextSameParent = ParentEvent->ProfileNode.FirstChild;
                    ParentEvent->ProfileNode.FirstChild = StoredEvent;

                    open_debug_block *DebugBlock = AllocateOpenDebugBlock(
                        DebugState, Element, FrameIndex, Event, 
                        &Thread->FirstOpenCodeBlock);
                    DebugBlock->Node = StoredEvent;
                } break;

                case DebugType_EndBlock:
                {
                    if(Thread->FirstOpenCodeBlock)
                    {
                        open_debug_block *MatchingBlock = Thread->FirstOpenCodeBlock;
                        Assert(Thread->ID == Event->ThreadID);

                        debug_profile_node *Node = &MatchingBlock->Node->ProfileNode;
                        Node->Duration = Event->Clock - MatchingBlock->BeginClock;

                        DeallocateOpenDebugBlock(DebugState, &Thread->FirstOpenCodeBlock);
                    }
                } break;

                case DebugType_OpenDataBlock:
                {
                    ++CollationFrame->DataBlockCount;
                    open_debug_block *DebugBlock = AllocateOpenDebugBlock(
                        DebugState, 0, FrameIndex, Event, &Thread->FirstOpenDataBlock);

                    debug_parsed_name ParsedName = DebugParseName(Event->GUID);
                    DebugBlock->Group =
                        GetGroupForHierarchicalName(DebugState, DefaultParentGroup, ParsedName.Name, true);
                } break;

                case DebugType_CloseDataBlock:
                {
                    if(Thread->FirstOpenDataBlock)
                    {
                        open_debug_block *MatchingBlock = Thread->FirstOpenDataBlock;
                        Assert(Thread->ID == Event->ThreadID);
                        DeallocateOpenDebugBlock(DebugState, &Thread->FirstOpenDataBlock);
                    }
                } break;

                default:
                {
                    debug_element *Element = GetElementFromEvent(DebugState, Event, DefaultParentGroup, true);
                    Element->OriginalGUID = Event->GUID;
                    StoreEvent(DebugState, Element, Event);
                } break;
            }
        }
    }
}

internal void
DEBUGStart(debug_state *DebugState, game_render_commands *Commands, game_assets *Assets, u32 MainGenerationID, u32 Width, u32 Height)
{
    TIMED_FUNCTION();

    if(!DebugState->Initialized)
    {
        DebugState->FrameBarLaneCount = 0;
        DebugState->FirstThread = 0;
        DebugState->FirstFreeThread = 0;
        DebugState->FirstFreeBlock = 0;

        DebugState->TotalFrameCount = 0;    
        DebugState->MostRecentFrameOrdinal = 0;
        DebugState->CollationFrameOrdinal = 1;
        DebugState->OldestFrameOrdinal = 0;


        DebugState->TreeSentinel.Next = &DebugState->TreeSentinel;
        DebugState->TreeSentinel.Prev = &DebugState->TreeSentinel;
        DebugState->TreeSentinel.Group = 0;

        memory_index TotalMemorySize = DebugGlobalMemory->DebugStorageSize - sizeof(debug_state);
        InitializeArena(&DebugState->DebugArena, TotalMemorySize, DebugState + 1);
#if 1
        SubArena(&DebugState->PerFrameArena, &DebugState->DebugArena, (TotalMemorySize / 2));
#else
        // NOTE(casey): This is the stress-testing case to make sure the memory
        // recycling works.
        SubArena(&DebugState->PerFrameArena, &DebugState->DebugArena, 8*1024*1024);
#endif

        DebugState->RootGroup = CreateVariableGroup(DebugState, 4, "Root");
        DebugState->ProfileGroup = CreateVariableGroup(DebugState, 7, "Profile");

#if 0
        debug_variable_definition_context Context = {};
        Context.State = DebugState;
        Context.Arena = &DebugState->DebugArena;
        Context.GroupStack[0] = 0;

        DebugState->RootGroup = DEBUGBeginVariableGroup(&Context, "Root");
        DEBUGBeginVariableGroup(&Context, "Debugging");

        DEBUGCreateVariables(&Context);
        DEBUGBeginVariableGroup(&Context, "Profile");
        DEBUGBeginVariableGroup(&Context, "By Thread");
        DEBUGAddVariable(&Context, DebugType_CounterThreadList, "");
        DEBUGEndVariableGroup(&Context);
        DEBUGBeginVariableGroup(&Context, "By Function");
        DEBUGAddVariable(&Context, DebugType_CounterThreadList, "");
        DEBUGEndVariableGroup(&Context);
        DEBUGEndVariableGroup(&Context);

        asset_vector MatchVector = {};
        MatchVector.E[Tag_FacingDirection] = 0.0f;
        asset_vector WeightVector = {};
        WeightVector.E[Tag_FacingDirection] = 1.0f;
        bitmap_id ID = GetBestMatchBitmapFrom(Assets, Asset_Head, &MatchVector, &WeightVector);

        DEBUGAddVariable(&Context, "Test Bitmap", ID);

        DEBUGEndVariableGroup(&Context);
        DEBUGEndVariableGroup(&Context);
        Assert(Context.GroupDepth == 0);
#endif

        debug_event RootProfileEvent = {};
        RootProfileEvent.GUID = DEBUG_NAME("RootProfile");
        DebugState->RootProfileElement = GetElementFromEvent(DebugState, &RootProfileEvent, 0, false);

        DebugState->Paused = false;
        DebugState->ScopeToRecord = 0;

        DebugState->Initialized = true;

        AddTree(DebugState, DebugState->RootGroup, V2(-0.5f*Width, 0.5f*Height));
    }

    DebugState->RenderGroup = BeginRenderGroup(Assets, Commands, MainGenerationID, false);

    DebugState->DebugFont = PushFont(&DebugState->RenderGroup, DebugState->FontID);
    DebugState->DebugFontInfo = GetFontInfo(DebugState->RenderGroup.Assets, DebugState->FontID);

    DebugState->GlobalWidth = (r32)Width;
    DebugState->GlobalHeight = (r32)Height;

    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    MatchVector.E[Tag_FontType] = (r32)FontType_Debug;
    WeightVector.E[Tag_FontType] = 1.0f;
    DebugState->FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);

    DebugState->FontScale = 1.0f;
    Orthographic(&DebugState->RenderGroup, Width, Height, 1.0f);
    DebugState->LeftEdge = -0.5f*Width;
    DebugState->RightEdge = 0.5f*Width;

    DebugState->AtY = 0.5f*Height;

    DebugState->TextTransform = DefaultFlatTransform();
    DebugState->ShadowTransform = DefaultFlatTransform();
    DebugState->UITransform = DefaultFlatTransform();
    DebugState->BackingTransform = DefaultFlatTransform();

    DebugState->BackingTransform.SortBias = 100000.0f;
    DebugState->ShadowTransform.SortBias = 200000.0f;
    DebugState->UITransform.SortBias = 300000.0f;
    DebugState->TextTransform.SortBias = 400000.0f;

    if(!DebugState->Paused)
    {
        DebugState->ViewingFrameOrdinal = DebugState->MostRecentFrameOrdinal;
    }
}

internal void
DEBUGEnd(debug_state *DebugState, game_input *Input)
{
    TIMED_FUNCTION();

    render_group *RenderGroup = &DebugState->RenderGroup;

    debug_event *HotEvent = 0;

    DebugState->AltUI = Input->MouseButtons[PlatformMouseButton_Right].EndedDown;
    v2 MouseP = Unproject(RenderGroup, DefaultFlatTransform(), V2(Input->MouseX, Input->MouseY)).xy;
    DEBUGDrawMainMenu(DebugState, RenderGroup, MouseP);
    DEBUGInteract(DebugState, Input, MouseP);

    loaded_font *Font = DebugState->DebugFont;
    hha_font *Info = DebugState->DebugFontInfo;
    if(Font)
    {
#if 0
        for(u32 CounterIndex = 0;
            CounterIndex < DebugState->CounterCount;
            ++CounterIndex)
        {
            debug_counter_state *Counter = DebugState->CounterStates + CounterIndex;


            debug_statistic HitCount, CycleCount, CycleOverHit;
            BeginDebugStatistic(&HitCount);
            BeginDebugStatistic(&CycleCount);
            BeginDebugStatistic(&CycleOverHit);
            for(u32 SnapshotIndex = 0;
                SnapshotIndex < DEBUG_SNAPSHOT_COUNT;
                ++SnapshotIndex)
            {
                AccumDebugStatistic(&HitCount, Counter->Snapshots[SnapshotIndex].HitCount);
                AccumDebugStatistic(&CycleCount, (u32)Counter->Snapshots[SnapshotIndex].CycleCount);

                r64 HOC = 0.0f;
                if(Counter->Snapshots[SnapshotIndex].HitCount)
                {
                    HOC = ((r64)Counter->Snapshots[SnapshotIndex].CycleCount /
                           (r64)Counter->Snapshots[SnapshotIndex].HitCount);
                }
                AccumDebugStatistic(&CycleOverHit, HOC);
            }
            EndDebugStatistic(&HitCount);
            EndDebugStatistic(&CycleCount);
            EndDebugStatistic(&CycleOverHit);

            if(Counter->BlockName)
            {
                if(CycleCount.Max > 0.0f)
                {
                    r32 BarWidth = 4.0f;
                    r32 ChartLeft = 0.0f;
                    r32 ChartMinY = AtY;
                    r32 ChartHeight = Info->AscenderHeight*FontScale;
                    r32 Scale = 1.0f / (r32)CycleCount.Max;
                    for(u32 SnapshotIndex = 0;
                        SnapshotIndex < DEBUG_SNAPSHOT_COUNT;
                        ++SnapshotIndex)
                    {
                        r32 ThisProportion = Scale*(r32)Counter->Snapshots[SnapshotIndex].CycleCount;
                        r32 ThisHeight = ChartHeight*ThisProportion;
                        PushRect(RenderGroup, V3(ChartLeft + BarWidth*(r32)SnapshotIndex + 0.5f*BarWidth, ChartMinY + 0.5f*ThisHeight, 0.0f), V2(BarWidth, ThisHeight), V4(ThisProportion, 1, 0.0f, 1));
                    }
                }

#if 1
                char TextBuffer[256];
                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                            "%32s(%4d): %10ucy %8uh %10ucy/h",
                            Counter->BlockName,
                            Counter->LineNumber,
                            (u32)CycleCount.Avg,
                            (u32)HitCount.Avg,
                            (u32)CycleOverHit.Avg);
                DEBUGTextLine(TextBuffer);
#endif
            }
        }
#endif
    }

    if(WasPressed(Input->MouseButtons[PlatformMouseButton_Left]))
    {
        if(HotEvent)
        {
            DebugState->ScopeToRecord = HotEvent->GUID;
        }
        else
        {
            DebugState->ScopeToRecord = 0;
        }
    }

    EndRenderGroup(&DebugState->RenderGroup);

    // NOTE(casey): Clear the UI state for the next frame
    ZeroStruct(DebugState->NextHotInteraction);
}

extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
{   
    ZeroStruct(GlobalDebugTable->EditEvent);

    GlobalDebugTable->CurrentEventArrayIndex = !GlobalDebugTable->CurrentEventArrayIndex;    
    u64 ArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugTable->EventArrayIndex_EventIndex,
                                                  (u64)GlobalDebugTable->CurrentEventArrayIndex << 32);

    u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
    Assert(EventArrayIndex <= 1);
    u32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;

    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
    if(DebugState)
    {
        game_assets *Assets = DEBUGGetGameAssets(Memory);

        DEBUGStart(DebugState, RenderCommands, Assets, DEBUGGetMainGenerationID(Memory), RenderCommands->Width, RenderCommands->Height);
        CollateDebugRecords(DebugState, EventCount, GlobalDebugTable->Events[EventArrayIndex]);
        DEBUGEnd(DebugState, Input);
    }
}
