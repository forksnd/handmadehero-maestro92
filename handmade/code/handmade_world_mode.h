/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct game_mode_world;

struct pairwise_collision_rule
{
    bool32 CanCollide;
    uint32 IDA;
    uint32 IDB;
    
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
    entity_id CameraFollowingEntityIndex;
    world_position CameraP;
    world_position LastCameraP;

    // TODO(casey): Must be power of two
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;

    entity_collision_volume_group *NullCollision;
    entity_collision_volume_group *StairCollision;
    entity_collision_volume_group *HeroHeadCollision;
    entity_collision_volume_group *HeroBodyCollision;
    entity_collision_volume_group *MonstarCollision;
    entity_collision_volume_group *FamiliarCollision;
    entity_collision_volume_group *WallCollision;
    entity_collision_volume_group *FloorCollision;

    real32 Time;

    random_series EffectsEntropy; // NOTE(casey): This is entropy that doesn't affect the gameplay
    real32 tSine;

#define PARTICLE_CEL_DIM 32
    u32 NextParticle;
    particle Particles[256];

    u32 CreationBufferIndex;
    entity CreationBuffer[4];
    u32 LastUsedEntityStorageIndex; // TODO(casey): Worry about this wrapping - free list for IDs?
    
    particle_cel ParticleCels[PARTICLE_CEL_DIM][PARTICLE_CEL_DIM];
};

internal void PlayWorld(game_state *GameState, transient_state *TranState);
