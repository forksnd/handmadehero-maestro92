/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct move_spec
{
    bool32 UnitMaxAccelVector;
    real32 Speed;
    real32 Drag;
};

struct entity_hash
{
    entity *Ptr;
    entity_id Index;
};

struct sim_region
{
    // TODO(casey): Need a hash table here to map stored entity indices
    // to sim entities!
    
    world *World;
    r32 MaxEntityRadius;
    r32 MaxEntityVelocity;

    world_position Origin;
    rectangle3 Bounds;
    rectangle3 UpdatableBounds;
    
    u32 MaxEntityCount;
    u32 EntityCount;
    entity *Entities;
    
    // TODO(casey): Do I really want a hash for this??
    // NOTE(casey): Must be a power of two!
    entity_hash Hash[4096];
};
