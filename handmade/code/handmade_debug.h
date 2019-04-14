#if !defined(HANDMADE_DEBUG_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#define DEBUG_MAX_VARIABLE_STACK_DEPTH 64

enum debug_variable_to_text_flag
{
    DEBUGVarToText_AddDebugUI = 0x1,
    DEBUGVarToText_AddName = 0x2,
    DEBUGVarToText_FloatSuffix = 0x4,
    DEBUGVarToText_LineFeedEnd = 0x8,
    DEBUGVarToText_NullTerminator = 0x10,
    DEBUGVarToText_Colon = 0x20,
    DEBUGVarToText_PrettyBools = 0x40,
    DEBUGVarToText_ShowEntireGUID = 0x80,
    DEBUGVarToText_AddValue = 0x100,
};

struct debug_tree;

struct debug_view_inline_block
{
    v2 Dim;
};

struct debug_view_profile_graph
{
    debug_view_inline_block Block;
    char *GUID;
};

struct debug_view_collapsible
{
    b32 ExpandedAlways;
    b32 ExpandedAltView;
};

enum debug_view_type
{
    DebugViewType_Unknown,

    DebugViewType_Basic,
    DebugViewType_InlineBlock,
    DebugViewType_Collapsible,
};

struct debug_view
{
    debug_id ID;
    debug_view *NextInHash;

    debug_view_type Type;
    union
    {
        debug_view_inline_block InlineBlock;
        debug_view_profile_graph ProfileGraph;
        debug_view_collapsible Collapsible;
    };
};

struct debug_profile_node
{
    struct debug_element *Element;
    struct debug_stored_event *FirstChild;
    struct debug_stored_event *NextSameParent;
    u32 ParentRelativeClock;
    u32 Duration;
    u32 AggregateCount;
    u16 ThreadOrdinal;
    u16 CoreIndex;
};

struct debug_stored_event
{
    union
    {
        debug_stored_event *Next;
        debug_stored_event *NextFree;
    };

    u32 FrameIndex;
    
    union
    {
        debug_event Event;
        debug_profile_node ProfileNode;
    };
};

struct debug_string
{
    u32 Length;
    char *Value;
};

struct debug_element
{
    char *OriginalGUID; // NOTE(casey): Can never be printed!  Might point into unloaded DLL.
    char *GUID;
    u32 FileNameCount;
    u32 LineNumber;
    u32 NameStartsAt;
    
    b32 ValueWasEdited;
    
    debug_element *NextInHash;

    debug_stored_event *OldestEvent;
    debug_stored_event *MostRecentEvent;
};
inline char *GetName(debug_element *Element) {char *Result = Element->GUID + Element->NameStartsAt; return(Result);}
inline debug_string GetFileName(debug_element *Element) {debug_string Result = {Element->FileNameCount, Element->GUID}; return(Result);}

struct debug_variable_group;
struct debug_variable_link
{
    debug_variable_link *Next;
    debug_variable_link *Prev;

    debug_variable_group *Children;
    debug_element *Element;
};

struct debug_tree
{
    v2 UIP;
    debug_variable_group *Group;

    debug_tree *Next;
    debug_tree *Prev;
};

struct debug_variable_group
{
    char *Name;
    debug_variable_link Sentinel;
};

struct render_group;
struct game_assets;
struct loaded_bitmap;
struct loaded_font;
struct hha_font;

enum debug_text_op
{
    DEBUGTextOp_DrawText,
    DEBUGTextOp_SizeText,
};

struct debug_counter_snapshot
{
    u32 HitCount;
    u64 CycleCount;
};

struct debug_counter_state
{
    char *FileName;
    char *BlockName;

    u32 LineNumber;
};

struct debug_frame_region
{
    // TODO(casey): Do we want to copy these out in their entirety?
    debug_event *Event;
    u64 CycleCount;
    u16 LaneIndex;
    u16 ColorIndex;
    r32 MinT;
    r32 MaxT;
};

#define MAX_REGIONS_PER_FRAME 2*4096
struct debug_frame
{
    // IMPORTANT(casey): This actually gets freed as a set in FreeFrame!

    union
    {
        debug_frame *Next;
        debug_frame *NextFree;
    };

    u64 BeginClock;
    u64 EndClock;
    r32 WallSecondsElapsed;

    r32 FrameBarScale;

    u32 FrameIndex;
    
    u32 StoredEventCount;
    u32 ProfileBlockCount;
    u32 DataBlockCount;
    
    debug_stored_event *RootProfileNode;
};

struct open_debug_block
{
    union
    {
        open_debug_block *Parent;
        open_debug_block *NextFree;
    };

    u32 StartingFrameIndex;
    debug_element *Element;
    u64 BeginClock;
    debug_stored_event *Node;

    // NOTE(casey): Only for data blocks?  Probably!
    debug_variable_group *Group;    
};

struct debug_thread
{
    union
    {
        debug_thread *Next;
        debug_thread *NextFree;
    };

    u32 ID;
    u32 LaneIndex;
    open_debug_block *FirstOpenCodeBlock;
    open_debug_block *FirstOpenDataBlock;
};

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
    
    DebugInteraction_SetProfileGraphRoot,
};

struct debug_interaction
{
    debug_id ID;
    debug_interaction_type Type;
    union
    {
        void *Generic;
        debug_element *Element;
        debug_tree *Tree;
        debug_variable_link *Link;
        v2 *P;
    };
};

struct debug_state
{
    b32 Initialized;

    memory_arena DebugArena;
    memory_arena PerFrameArena;

    render_group RenderGroup;
    loaded_font *DebugFont;
    hha_font *DebugFontInfo;
    
    object_transform TextTransform;
    object_transform ShadowTransform;
    object_transform UITransform;
    object_transform BackingTransform;

    v2 MenuP;
    b32 MenuActive;

    u32 SelectedIDCount;
    debug_id SelectedID[64];
    
    debug_element *ElementHash[1024];
    debug_view *ViewHash[4096];
    debug_variable_group *RootGroup;
    debug_variable_group *ProfileGroup;
    debug_tree TreeSentinel;

    v2 LastMouseP;
    b32 AltUI;
    debug_interaction Interaction;
    debug_interaction HotInteraction;
    debug_interaction NextHotInteraction;
    b32 Paused;

    r32 LeftEdge;
    r32 RightEdge;
    r32 AtY;
    r32 FontScale;
    font_id FontID;
    r32 GlobalWidth;
    r32 GlobalHeight;
    
    r32 MouseTextStackY;

    char *ScopeToRecord;

    u32 TotalFrameCount;
    u32 FrameCount;
    debug_frame *OldestFrame;
    debug_frame *MostRecentFrame;

    debug_frame *CollationFrame;

    u32 FrameBarLaneCount;
    debug_thread *FirstThread;
    debug_thread *FirstFreeThread;
    open_debug_block *FirstFreeBlock;

    // NOTE(casey): Per-frame storage management
    debug_stored_event *FirstFreeStoredEvent;
    debug_frame *FirstFreeFrame;
};

internal debug_variable_group *CreateVariableGroup(debug_state *DebugState, u32 NameLength, char *Name);
internal debug_variable_group *CloneVariableGroup(debug_state *DebugState, debug_variable_link *First);

#define HANDMADE_DEBUG_H
#endif
