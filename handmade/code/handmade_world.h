#if !defined(HANDMADE_WORLD_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

// TODO(casey): Replace this with a v3 once we get to v3
struct world_difference
{
    v2 dXY;
    real32 dZ;
};

struct world_position
{
    // TODO(casey): Puzzler!  How can we get rid of abstile* here,
    // and still allow references to entities to be able to figure
    // out _where they are_ (or rather, which world_chunk they are
    // in?)
    
    int32 ChunkX;
    int32 ChunkY;
    int32 ChunkZ;

    // NOTE(casey): These are the offsets from the chunk center
    v2 Offset_;
};

// TODO(casey): Could make this just tile_chunk and then allow multiple tile chunks per X/Y/Z
struct world_entity_block
{
    uint32 EntityCount;
    uint32 LowEntityIndex[16];
    world_entity_block *Next;
};

struct world_chunk
{
    int32 ChunkX;
    int32 ChunkY;
    int32 ChunkZ;

    // TODO(casey): Profile this and determine if a pointer would be better here!
    world_entity_block FirstBlock;
    
    world_chunk *NextInHash;
};

struct world
{
    real32 TileSideInMeters;
    real32 ChunkSideInMeters;

    world_entity_block *FirstFree;

    // TODO(casey): WorldChunkHash should probably switch to pointers IF
    // tile entity blocks continue to be stored en masse directly in the tile chunk!
    // NOTE(casey): A the moment, this must be a power of two!
    world_chunk ChunkHash[4096];
};

#define HANDMADE_WORLD_H
#endif
