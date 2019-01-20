#if !defined(HANDMADE_DEBUG_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct debug_counter_snapshot
{
    u32 HitCount;
    u64 CycleCount;
};

#define DEBUG_SNAPSHOT_COUNT 120
struct debug_counter_state
{
    char *FileName;
    char *BlockName;
    
    u32 LineNumber;
    
    debug_counter_snapshot Snapshots[DEBUG_SNAPSHOT_COUNT];
};

struct debug_state
{
    u32 SnapshotIndex;
    u32 CounterCount;
    debug_counter_state CounterStates[512];
};

// TODO(casey): Fix this for looped live code editing
struct render_group;
struct game_assets;
global_variable render_group *DEBUGRenderGroup;

internal void DEBUGReset(game_assets *Assets, u32 Width, u32 Height);
internal void DEBUGOverlay(game_memory *Memory);

#define HANDMADE_DEBUG_H
#endif
