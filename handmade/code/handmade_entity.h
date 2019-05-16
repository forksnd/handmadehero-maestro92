/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct entity;

enum entity_type
{
    EntityType_Null,
    
    EntityType_HeroBody,
    EntityType_HeroHead,
    
    EntityType_Wall,
    EntityType_Floor,
    EntityType_FloatyThingForNow,
    EntityType_Familiar,
    EntityType_Monstar,
    EntityType_Stairwell,
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point
{
    // TODO(casey): Bake this down into one variable
    uint8 Flags;
    uint8 FilledAmount;
};

struct entity_id
{
    u32 Value;
};
union entity_reference
{
    entity *Ptr;
    entity_id Index;
};

struct traversable_reference
{
    entity_reference Entity;
    u32 Index;
};
inline b32 IsEqual(traversable_reference A, traversable_reference B)
{
    b32 Result = ((A.Entity.Ptr == B.Entity.Ptr) &&
                  (A.Entity.Index.Value == B.Entity.Index.Value) &&
                  (A.Index == B.Index));
    
    return(Result);
}

enum entity_flags
{
    EntityFlag_Collides = (1 << 0),
    EntityFlag_Moveable = (1 << 1),
    EntityFlag_Deleted = (1 << 2),
};

struct entity_collision_volume
{
    v3 OffsetP;
    v3 Dim;
};

struct entity_collision_volume_group
{
    entity_collision_volume TotalVolume;

    // TODO(casey): VolumeCount is always expected to be greater than 0 if the entity
    // has any volume... in the future, this could be compressed if necessary to say
    // that the VolumeCount can be 0 if the TotalVolume should be used as the only
    // collision volume for the entity.
    u32 VolumeCount;
    entity_collision_volume *Volumes; 
};

struct entity_traversable_point
{
    v3 P;
    entity *Occupier;
};

enum entity_movement_mode
{
    MovementMode_Planted,
    MovementMode_Hopping,
};
struct entity
{
    entity_id ID;
    b32 Updatable;

    //

    entity_type Type;
    u32 Flags;

    v3 P;
    v3 dP;

    r32 DistanceLimit;

    entity_collision_volume_group *Collision;

    r32 FacingDirection;
    r32 tBob;
    r32 dtBob;

    s32 dAbsTileZ;

    // TODO(casey): Should hitpoints themselves be entities?
    u32 HitPointMax;
    hit_point HitPoint[16];

    entity_reference Head;

    // TODO(casey): Only for stairwells!
    v2 WalkableDim;
    r32 WalkableHeight;

    entity_movement_mode MovementMode;
    r32 tMovement;
    traversable_reference Occupying;
    traversable_reference CameFrom;

    v2 XAxis;
    v2 YAxis;

    v2 FloorDisplace;
    
    u32 TraversableCount;
    entity_traversable_point Traversables[16];

    // TODO(casey): Generation index so we know how "up to date" this entity is.
};

#define InvalidP V3(100000.0f, 100000.0f, 100000.0f)

inline bool32
IsSet(entity *Entity, uint32 Flag)
{
    bool32 Result = Entity->Flags & Flag;

    return(Result);
}

inline void
AddFlags(entity *Entity, uint32 Flag)
{
    Entity->Flags |= Flag;
}

inline void
ClearFlags(entity *Entity, uint32 Flag)
{
    Entity->Flags &= ~Flag;
}

inline void
MakeEntitySpatial(entity *Entity, v3 P, v3 dP)
{
    Entity->P = P;
    Entity->dP = dP;
}

inline v3
GetEntityGroundPoint(entity *Entity, v3 ForEntityP)
{
    v3 Result = ForEntityP;

    return(Result);
}

inline v3
GetEntityGroundPoint(entity *Entity)
{
    v3 Result = GetEntityGroundPoint(Entity, Entity->P);

    return(Result);
}

inline real32
GetStairGround(entity *Entity, v3 AtGroundPoint)
{
    Assert(Entity->Type == EntityType_Stairwell);
    
    rectangle2 RegionRect = RectCenterDim(Entity->P.xy, Entity->WalkableDim);
    v2 Bary = Clamp01(GetBarycentric(RegionRect, AtGroundPoint.xy));
    real32 Result = Entity->P.z + Bary.y*Entity->WalkableHeight;

    return(Result);
}
