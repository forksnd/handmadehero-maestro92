#if !defined(HANDMADE_WORLD_MODE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct game_mode_world;

struct low_entity
{
    // TODO(casey): It's kind of busted that P's can be invalid here,
    // AND we store whether they would be invalid in the flags field...
    // Can we do something better here?
    world_position P;
    sim_entity Sim;
};

struct pairwise_collision_rule
{
    bool32 CanCollide;
    uint32 StorageIndexA;
    uint32 StorageIndexB;
    
    pairwise_collision_rule *NextInHash;
};
struct game_state;
internal void AddCollisionRule(game_mode_world *WorldMode, uint32 StorageIndexA, uint32 StorageIndexB, bool32 ShouldCollide);
internal void ClearCollisionRulesFor(game_mode_world *WorldMode, uint32 StorageIndex);

struct particle_cel
{
    real32 Density;
    v3 VelocityTimesDensity;
};
struct particle
{
    bitmap_id BitmapID;
    v3 P;
    v3 dP;
    v3 ddP;
    v4 Color;
    v4 dColor;
};

struct game_mode_world
{
    world *World;
    real32 TypicalFloorHeight;
    
    // TODO(casey): Should we allow split-screen?
    uint32 CameraFollowingEntityIndex;
    world_position CameraP;
    world_position LastCameraP;

    // TODO(casey): Change the name to "stored entity"
    uint32 LowEntityCount;
    low_entity LowEntities[100000];

    // TODO(casey): Must be power of two
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;

    sim_entity_collision_volume_group *NullCollision;
    sim_entity_collision_volume_group *SwordCollision;
    sim_entity_collision_volume_group *StairCollision;
    sim_entity_collision_volume_group *PlayerCollision;
    sim_entity_collision_volume_group *MonstarCollision;
    sim_entity_collision_volume_group *FamiliarCollision;
    sim_entity_collision_volume_group *WallCollision;
    sim_entity_collision_volume_group *StandardRoomCollision;

    real32 Time;

    random_series EffectsEntropy; // NOTE(casey): This is entropy that doesn't affect the gameplay
    real32 tSine;

#define PARTICLE_CEL_DIM 32
    u32 NextParticle;
    particle Particles[256];
    
    particle_cel ParticleCels[PARTICLE_CEL_DIM][PARTICLE_CEL_DIM];
};

inline low_entity *
GetLowEntity(game_mode_world *WorldMode, uint32 Index)
{
    low_entity *Result = 0;
    
    if((Index > 0) && (Index < WorldMode->LowEntityCount))
    {
        Result = WorldMode->LowEntities + Index;
    }

    return(Result);
}

internal void PlayWorld(game_state *GameState, transient_state *TranState);

#define HANDMADE_WORLD_MODE_H
#endif
