/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

inline entity_traversable_point *
GetTraversable(entity *Entity, u32 Index)
{
    entity_traversable_point *Result = 0;
    if(Entity)
    {
        Assert(Index < Entity->TraversableCount);
        Result = Entity->Traversables + Index;
    }
    return(Result);
}

inline entity_traversable_point *
GetTraversable(traversable_reference Reference)
{
    entity_traversable_point *Result = GetTraversable(Reference.Entity.Ptr, Reference.Index);
    return(Result);
}

inline entity_traversable_point
GetSimSpaceTraversable(entity *Entity, u32 Index)
{
    entity_traversable_point Result = {};
    Result.P = Entity->P;
    
    entity_traversable_point *Point = GetTraversable(Entity, Index);
    if(Point)
    {
        // TODO(casey): This wants to be rotated eventually!
        Result.P += Point->P;
        Result.Occupier = Point->Occupier;
    }
    
    return(Result);
}

inline entity_traversable_point
GetSimSpaceTraversable(traversable_reference Reference)
{
    entity_traversable_point Result = 
        GetSimSpaceTraversable(Reference.Entity.Ptr, Reference.Index);
    return(Result);
}

internal entity_hash *
GetHashFromID(sim_region *SimRegion, entity_id StorageIndex)
{
    Assert(StorageIndex.Value);
    
    entity_hash *Result = 0;
    
    uint32 HashValue = StorageIndex.Value;
    for(uint32 Offset = 0;
        Offset < ArrayCount(SimRegion->Hash);
        ++Offset)
    {
        uint32 HashMask = (ArrayCount(SimRegion->Hash) - 1);
        uint32 HashIndex = ((HashValue + Offset) & HashMask);
        entity_hash *Entry = SimRegion->Hash + HashIndex;
        if((Entry->Index.Value == 0) || (Entry->Index.Value == StorageIndex.Value))
        {
            Result = Entry;
            break;
        }
    }
    
    return(Result);
}

inline entity *
GetEntityByID(sim_region *SimRegion, entity_id ID)
{
    entity_hash *Entry = GetHashFromID(SimRegion, ID);
    entity *Result = Entry->Ptr;
    return(Result);
}

inline void
LoadEntityReference(sim_region *SimRegion, entity_reference *Ref)
{
    // TODO(casey): Load these!
#if 0
    if(Ref->Index.Value)
    {
        entity_hash *Entry = GetHashFromID(SimRegion, Ref->Index);
        Ref->Ptr = Entry ? Entry->Ptr : 0;
    }
#endif
}

inline void
LoadTraversableReference(sim_region *SimRegion, traversable_reference *Ref)
{
    LoadEntityReference(SimRegion, &Ref->Entity);
}

internal entity *
AddEntityRaw(game_mode_world *WorldMode, sim_region *SimRegion, entity_id ID, entity *Source)
{
    TIMED_FUNCTION();
    
    Assert(ID.Value);
    entity *Entity = 0;

    
    return(Entity);
}

inline bool32
EntityOverlapsRectangle(v3 P, entity_collision_volume Volume, rectangle3 Rect)
{
    rectangle3 Grown = AddRadiusTo(Rect, 0.5f*Volume.Dim);
    bool32 Result = IsInRectangle(Grown, P + Volume.OffsetP);
    return(Result);
}

internal void
AddEntity(game_mode_world *WorldMode, sim_region *SimRegion, entity *Source, v3 ChunkDelta)
{
    entity_id ID = Source->ID;
    
    entity_hash *Entry = GetHashFromID(SimRegion, ID);
    Assert(Entry->Ptr == 0);
    
    if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
    {
        entity *Dest = SimRegion->Entities + SimRegion->EntityCount++;
        
        Entry->Index = ID;
        Entry->Ptr = Dest;

        if(Source)
        {
            // TODO(casey): This should really be a decompression step, not
            // a copy!
            *Dest = *Source;
        }

        Dest->ID = ID;
        Dest->P += ChunkDelta;
        
        Dest->Updatable = EntityOverlapsRectangle(Dest->P, Dest->Collision->TotalVolume, SimRegion->UpdatableBounds);
    }            
    else
    {
        InvalidCodePath;
    }
}

internal void
ConnectEntityPointers(sim_region *SimRegion)
{
    // TODO(casey): Reenable this!
#if 0
    for(u32 EntityIndex = 0;
        EntityIndex < SimRegion->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = SimRegion->Entities + EntityIndex;
        LoadEntityReference(SimRegion, &Entity->Head);
        
        LoadTraversableReference(SimRegion, &Entity->Occupying);
        if(Entity->Occupying.Entity.Ptr)
        {
            Entity->Occupying.Entity.Ptr->Traversables[Entity->Occupying.Index].Occupier = 
                Entity;
        }
        
        LoadTraversableReference(SimRegion, &Entity->CameFrom);
    }
#endif
}

internal sim_region *
BeginSim(memory_arena *SimArena, game_mode_world *WorldMode, world *World, world_position Origin, rectangle3 Bounds, real32 dt)
{
    TIMED_FUNCTION();

    // TODO(casey): If entities were stored in the world, we wouldn't need the game state here!
    
    sim_region *SimRegion = PushStruct(SimArena, sim_region);
    
    // TODO(casey): Try to make these get enforced more rigorously
    // TODO(casey): Perhaps try using a dual system here, where we support
    // entities larger than the max entity radius by adding them multiple times
    // to the spatial partition?
    SimRegion->MaxEntityRadius = 5.0f;
    SimRegion->MaxEntityVelocity = 30.0f;
    real32 UpdateSafetyMargin = SimRegion->MaxEntityRadius + dt*SimRegion->MaxEntityVelocity;
    real32 UpdateSafetyMarginZ = 1.0f;
    
    SimRegion->World = World;
    SimRegion->Origin = Origin;
    SimRegion->UpdatableBounds = AddRadiusTo(Bounds, V3(SimRegion->MaxEntityRadius,
                                                        SimRegion->MaxEntityRadius,
                                                        0.0f));
    SimRegion->Bounds = AddRadiusTo(SimRegion->UpdatableBounds,
                                    V3(UpdateSafetyMargin, UpdateSafetyMargin, UpdateSafetyMarginZ));    

    // TODO(casey): Need to be more specific about entity counts
    SimRegion->MaxEntityCount = 4096;
    SimRegion->EntityCount = 0;
    SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, entity);
    
    world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
    world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));
    
    DEBUG_VALUE(SimRegion->Origin.ChunkX);
    DEBUG_VALUE(SimRegion->Origin.ChunkY);
    DEBUG_VALUE(SimRegion->Origin.ChunkZ);
    DEBUG_VALUE(SimRegion->Origin.Offset_);
    
    for(int32 ChunkZ = MinChunkP.ChunkZ;
        ChunkZ <= MaxChunkP.ChunkZ;
        ++ChunkZ)
    {
        for(int32 ChunkY = MinChunkP.ChunkY;
            ChunkY <= MaxChunkP.ChunkY;
            ++ChunkY)
        {
            for(int32 ChunkX = MinChunkP.ChunkX;
                ChunkX <= MaxChunkP.ChunkX;
                ++ChunkX)
            {
                world_chunk *Chunk = RemoveWorldChunk(World, ChunkX, ChunkY, ChunkZ);
                if(Chunk)
                {
                    Assert(Chunk->ChunkX == ChunkX);
                    Assert(Chunk->ChunkY == ChunkY);
                    Assert(Chunk->ChunkZ == ChunkZ);
                    world_position ChunkPosition = {ChunkX, ChunkY, ChunkZ};
                    v3 ChunkDelta = Subtract(SimRegion->World, &ChunkPosition, &SimRegion->Origin);
                    world_entity_block *Block = Chunk->FirstBlock;
                    while(Block)
                    {
                        for(uint32 EntityIndex = 0;
                            EntityIndex < Block->EntityCount;
                            ++EntityIndex)
                        {                        
                            entity *Low = (entity *)Block->EntityData + EntityIndex;
                            v3 SimSpaceP = Low->P + ChunkDelta;
                            if(EntityOverlapsRectangle(SimSpaceP, Low->Collision->TotalVolume, SimRegion->Bounds))
                            {
                                AddEntity(WorldMode, SimRegion, Low, ChunkDelta);
                            }
                        }
                        
                        world_entity_block *NextBlock = Block->Next;
                        AddBlockToFreeList(World, Block);
                        Block = NextBlock;
                    }
                    
                    AddChunkToFreeList(World, Chunk);
                }
            }
        }
    }
    
    ConnectEntityPointers(SimRegion);
    
    return(SimRegion);
}

inline void
DeleteEntity(sim_region *Region, entity *Entity)
{
    Entity->Flags |= EntityFlag_Deleted;
}

internal void
EndSim(sim_region *Region, game_mode_world *WorldMode)
{
    TIMED_FUNCTION();

    // TODO(casey): Maybe don't take a game state here, low entities should be stored
    // in the world??

    world *World = WorldMode->World;
    
    entity *Entity = Region->Entities;
    for(uint32 EntityIndex = 0;
        EntityIndex < Region->EntityCount;
        ++EntityIndex, ++Entity)
    {
        if(!(Entity->Flags & EntityFlag_Deleted))
        {
            world_position EntityP = MapIntoChunkSpace(World, Region->Origin, Entity->P);
            world_position ChunkP = EntityP;
            ChunkP.Offset_ = V3(0, 0, 0);
            
            v3 ChunkDelta = EntityP.Offset_ - Entity->P;

            // TODO(casey): Save state back to the stored entity, once high entities
            // do state decompression, etc.

            if(Entity->ID.Value == WorldMode->CameraFollowingEntityIndex.Value)
            {
                world_position NewCameraP = WorldMode->CameraP;

                v3 RoomDelta = {24.0f, 12.5f, WorldMode->TypicalFloorHeight};
                v3 hRoomDelta = 0.5f * RoomDelta;
                r32 ApronSize = 0.7f;
                r32 BounceHeight = 0.5f;
                v3 hRoomApron = {hRoomDelta.x - ApronSize, hRoomDelta.y - ApronSize, hRoomDelta.z - ApronSize};

                if(Global_Renderer_Camera_RoomBased)
                {
                    WorldMode->CameraOffset = V3(0, 0, 0);

                    v3 AppliedDelta = {};
                    for(u32 E = 0;
                        E < 3;
                        ++E)
                    {
                        if(Entity->P.E[E] > hRoomDelta.E[E])
                        {
                            AppliedDelta.E[E] = RoomDelta.E[E];
                            NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                        }
                        if(Entity->P.E[E] < -hRoomDelta.E[E])
                        {
                            AppliedDelta.E[E] = -RoomDelta.E[E];
                            NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedDelta);
                        }
                    }
                    
                    v3 EntityP = Entity->P - AppliedDelta;

                    if(EntityP.y > hRoomApron.y)
                    {
                        r32 t = Clamp01MapToRange(hRoomApron.y, EntityP.y, hRoomDelta.y);
                        WorldMode->CameraOffset = V3(0, t*hRoomDelta.y, (-(t*t)+2.0f*t)*BounceHeight);
                    }
                    if(EntityP.y < -hRoomApron.y)
                    {
                        r32 t = Clamp01MapToRange(-hRoomApron.y, EntityP.y, -hRoomDelta.y);
                        WorldMode->CameraOffset = V3(0, -t*hRoomDelta.y, (-(t*t)+2.0f*t)*BounceHeight);
                    }
                    if(EntityP.x > hRoomApron.x)
                    {
                        r32 t = Clamp01MapToRange(hRoomApron.x, EntityP.x, hRoomDelta.x);
                        WorldMode->CameraOffset = V3(t*hRoomDelta.x, 0, (-(t*t)+2.0f*t)*BounceHeight);
                    }
                    if(EntityP.x < -hRoomApron.x)
                    {
                        r32 t = Clamp01MapToRange(-hRoomApron.x, EntityP.x, -hRoomDelta.x);
                        WorldMode->CameraOffset = V3(-t*hRoomDelta.x, 0, (-(t*t)+2.0f*t)*BounceHeight);
                    }
                    if(EntityP.z > hRoomApron.z)
                    {
                        r32 t = Clamp01MapToRange(hRoomApron.z, EntityP.z, hRoomDelta.z);
                        WorldMode->CameraOffset = V3(0, 0, t*hRoomDelta.z);
                    }
                    if(EntityP.z < -hRoomApron.z)
                    {
                        r32 t = Clamp01MapToRange(-hRoomApron.z, EntityP.z, -hRoomDelta.z);
                        WorldMode->CameraOffset = V3(0, 0, -t*hRoomDelta.z);
                    }
                }
                else
                {
                    //            real32 CamZOffset = NewCameraP.Offset_.z;
                    NewCameraP = EntityP;
                    //            NewCameraP.Offset_.z = CamZOffset;
                }

                WorldMode->CameraP = NewCameraP;
            }

            v3 OldEntityP = Entity->P;
            Entity->P += ChunkDelta;
            PackEntityIntoWorld(World, Entity, ChunkP);
            
            //v3 ReverseChunkDelta = Subtract(Region->World, &ChunkP, &Region->Origin);
            //v3 TestP = Entity->P + ReverseChunkDelta;
            //Assert(OldEntityP.z == TestP.z);
        }
    }
}

struct test_wall
{
    real32 X;
    real32 RelX;
    real32 RelY;
    real32 DeltaX;
    real32 DeltaY;
    real32 MinY;
    real32 MaxY;
    v3 Normal;
};
internal bool32
TestWall(real32 WallX, real32 RelX, real32 RelY, real32 PlayerDeltaX, real32 PlayerDeltaY,
         real32 *tMin, real32 MinY, real32 MaxY)
{
    bool32 Hit = false;
    
    real32 tEpsilon = 0.001f;
    if(PlayerDeltaX != 0.0f)
    {
        real32 tResult = (WallX - RelX) / PlayerDeltaX;
        real32 Y = RelY + tResult*PlayerDeltaY;
        if((tResult >= 0.0f) && (*tMin > tResult))
        {
            if((Y >= MinY) && (Y <= MaxY))
            {
                *tMin = Maximum(0.0f, tResult - tEpsilon);
                Hit = true;
            }
        }
    }

    return(Hit);
}

internal bool32
CanCollide(game_mode_world *WorldMode, entity *A, entity *B)
{
    b32 Result = false;

    if(A != B)
    {
        if(A->ID.Value > B->ID.Value)
        {
            entity *Temp = A;
            A = B;
            B = Temp;
        }

        if(IsSet(A, EntityFlag_Collides) && IsSet(B, EntityFlag_Collides))
        {
            Result = true;
            
            // TODO(casey): BETTER HASH FUNCTION
            uint32 HashBucket = A->ID.Value & (ArrayCount(WorldMode->CollisionRuleHash) - 1);
            for(pairwise_collision_rule *Rule = WorldMode->CollisionRuleHash[HashBucket];
                Rule;
                Rule = Rule->NextInHash)
            {
                if((Rule->IDA == A->ID.Value) &&
                   (Rule->IDB == B->ID.Value))
                {
                    Result = Rule->CanCollide;
                    break;
                }
            }
        }
    }
    
    return(Result);
}

internal bool32
HandleCollision(game_mode_world *WorldMode, entity *A, entity *B)
{
    bool32 StopsOnCollision = true;
    
    if(A->Type > B->Type)
    {
        entity *Temp = A;
        A = B;
        B = Temp;
    }

    return(StopsOnCollision);
}

internal bool32
CanOverlap(game_mode_world *WorldMode, entity *Mover, entity *Region)
{
    bool32 Result = false;

    if(Mover != Region)
    {
        if(Region->Type == EntityType_Stairwell)
        {
            Result = true;
        }
    }

    return(Result);
}

internal bool32
SpeculativeCollide(entity *Mover, entity *Region, v3 TestP)
{
    TIMED_FUNCTION();

    bool32 Result = true;
    
    if(Region->Type == EntityType_Stairwell)
    {
        // TODO(casey): Needs work :)
        real32 StepHeight = 0.1f;
#if 0
        Result = ((AbsoluteValue(GetEntityGroundPoint(Mover).z - Ground) > StepHeight) ||
                  ((Bary.y > 0.1f) && (Bary.y < 0.9f)));
#endif
        v3 MoverGroundPoint = GetEntityGroundPoint(Mover, TestP);
        real32 Ground = GetStairGround(Region, MoverGroundPoint);
        Result = (AbsoluteValue(MoverGroundPoint.z - Ground) > StepHeight);
    }

    return(Result);
}

internal bool32
EntitiesOverlap(entity *Entity, entity *TestEntity, v3 Epsilon = V3(0, 0, 0))
{
    TIMED_FUNCTION();

    bool32 Result = false;
    
    for(uint32 VolumeIndex = 0;
        !Result && (VolumeIndex < Entity->Collision->VolumeCount);
        ++VolumeIndex)
    {
        entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;
                            
        for(uint32 TestVolumeIndex = 0;
            !Result && (TestVolumeIndex < TestEntity->Collision->VolumeCount);
            ++TestVolumeIndex)
        {
            entity_collision_volume *TestVolume = TestEntity->Collision->Volumes + TestVolumeIndex;
                        
            rectangle3 EntityRect = RectCenterDim(Entity->P + Volume->OffsetP, Volume->Dim + Epsilon); 
            rectangle3 TestEntityRect = RectCenterDim(TestEntity->P + TestVolume->OffsetP, TestVolume->Dim);
            Result = RectanglesIntersect(EntityRect, TestEntityRect);
        }
    }

    return(Result);
}

internal b32
TransactionalOccupy(entity *Entity, traversable_reference *DestRef, traversable_reference DesiredRef)
{
    b32 Result = false;
    
    entity_traversable_point *Desired = GetTraversable(DesiredRef);
    if(!Desired->Occupier)
    {
        entity_traversable_point *Dest = GetTraversable(*DestRef);
        if(Dest)
        {
            Dest->Occupier = 0;
        }
        *DestRef = DesiredRef;
        Desired->Occupier = Entity;
        Result = true;
    }
    
    return(Result);
}

internal void
MoveEntity(game_mode_world *WorldMode, sim_region *SimRegion, entity *Entity, real32 dt,
           move_spec *MoveSpec, v3 ddP)
{
    TIMED_FUNCTION();
    
    world *World = SimRegion->World;

    if(Entity->Type == EntityType_HeroHead)
    {
        int BreakHere = 5;
    }
    
    if(MoveSpec->UnitMaxAccelVector)
    {
        real32 ddPLength = LengthSq(ddP);
        if(ddPLength > 1.0f)
        {
            ddP *= (1.0f / SquareRoot(ddPLength));
        }
    }
    
    ddP *= MoveSpec->Speed;

    // TODO(casey): ODE here!
    v3 Drag = -MoveSpec->Drag*Entity->dP;
    Drag.z = 0.0f;
    ddP += Drag;
    
    v3 PlayerDelta = (0.5f*ddP*Square(dt) + Entity->dP*dt);
    Entity->dP = ddP*dt + Entity->dP;
    // TODO(casey): Upgrade physical motion routines to handle capping the
    // maximum velocity?
    Assert(LengthSq(Entity->dP) <= Square(SimRegion->MaxEntityVelocity));
    
    real32 DistanceRemaining = Entity->DistanceLimit;
    if(DistanceRemaining == 0.0f)
    {
        // TODO(casey): Do we want to formalize this number?
        DistanceRemaining = 10000.0f;
    }
    
    for(uint32 Iteration = 0;
        Iteration < 4;
        ++Iteration)
    {
        real32 tMin = 1.0f;
        real32 tMax = 1.0f;

        real32 PlayerDeltaLength = Length(PlayerDelta);
        // TODO(casey): What do we want to do for epsilons here?
        // Think this through for the final collision code
        if(PlayerDeltaLength > 0.0f)
        {
            if(PlayerDeltaLength > DistanceRemaining)
            {
                tMin = (DistanceRemaining / PlayerDeltaLength);
            }
        
            v3 WallNormalMin = {};
            v3 WallNormalMax = {};
            entity *HitEntityMin = 0;
            entity *HitEntityMax = 0;

            v3 DesiredPosition = Entity->P + PlayerDelta;

            // NOTE(casey): This is just an optimization to avoid enterring the
            // loop in the case where the test entity is non-spatial!
                            
            // TODO(casey): Spatial partition here!
            for(uint32 TestHighEntityIndex = 0;
                TestHighEntityIndex < SimRegion->EntityCount;
                ++TestHighEntityIndex)
            {
                entity *TestEntity = SimRegion->Entities + TestHighEntityIndex;

                // TODO(casey): Robustness!
                real32 OverlapEpsilon = 0.001f;

                if(CanCollide(WorldMode, Entity, TestEntity))
                {
                    for(uint32 VolumeIndex = 0;
                        VolumeIndex < Entity->Collision->VolumeCount;
                        ++VolumeIndex)
                    {
                        entity_collision_volume *Volume =
                            Entity->Collision->Volumes + VolumeIndex;

                        for(uint32 TestVolumeIndex = 0;
                            TestVolumeIndex < TestEntity->Collision->VolumeCount;
                            ++TestVolumeIndex)
                        {
                            entity_collision_volume *TestVolume =
                                TestEntity->Collision->Volumes + TestVolumeIndex;

                            v3 MinkowskiDiameter = {TestVolume->Dim.x + Volume->Dim.x,
                                TestVolume->Dim.y + Volume->Dim.y,
                                TestVolume->Dim.z + Volume->Dim.z};

                            v3 MinCorner = -0.5f*MinkowskiDiameter;
                            v3 MaxCorner = 0.5f*MinkowskiDiameter;

                            v3 Rel = ((Entity->P + Volume->OffsetP) -
                                    (TestEntity->P + TestVolume->OffsetP));

                            // TODO(casey): Do we want an open inclusion at the MaxCorner?
                            if((Rel.z >= MinCorner.z) && (Rel.z < MaxCorner.z))
                            {
                                test_wall Walls[] =
                                {
                                    {MinCorner.x, Rel.x, Rel.y, PlayerDelta.x, PlayerDelta.y, MinCorner.y, MaxCorner.y, V3(-1, 0, 0)},
                                    {MaxCorner.x, Rel.x, Rel.y, PlayerDelta.x, PlayerDelta.y, MinCorner.y, MaxCorner.y, V3(1, 0, 0)},
                                    {MinCorner.y, Rel.y, Rel.x, PlayerDelta.y, PlayerDelta.x, MinCorner.x, MaxCorner.x, V3(0, -1, 0)},
                                    {MaxCorner.y, Rel.y, Rel.x, PlayerDelta.y, PlayerDelta.x, MinCorner.x, MaxCorner.x, V3(0, 1, 0)},
                                };

                                real32 tMinTest = tMin;
                                bool32 HitThis = false;

                                v3 TestWallNormal = {};
                                for(uint32 WallIndex = 0;
                                    WallIndex < ArrayCount(Walls);
                                    ++WallIndex)
                                {
                                    test_wall *Wall = Walls + WallIndex;

                                    real32 tEpsilon = 0.001f;
                                    if(Wall->DeltaX != 0.0f)
                                    {
                                        real32 tResult = (Wall->X - Wall->RelX) / Wall->DeltaX;
                                        real32 Y = Wall->RelY + tResult*Wall->DeltaY;
                                        if((tResult >= 0.0f) && (tMinTest > tResult))
                                        {
                                            if((Y >= Wall->MinY) && (Y <= Wall->MaxY))
                                            {
                                                tMinTest = Maximum(0.0f, tResult - tEpsilon);
                                                TestWallNormal = Wall->Normal;
                                                HitThis = true;
                                            }
                                        }
                                    }
                                }

                                // TODO(casey): We need a concept of stepping onto vs. stepping
                                    // off of here so that we can prevent you from _leaving_
                                    // stairs instead of just preventing you from getting onto them.
                                if(HitThis)
                                {
                                    v3 TestP = Entity->P + tMinTest*PlayerDelta;
                                    if(SpeculativeCollide(Entity, TestEntity, TestP))
                                    {
                                        tMin = tMinTest;
                                        WallNormalMin = TestWallNormal;
                                        HitEntityMin = TestEntity;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            v3 WallNormal;
            entity *HitEntity;
            real32 tStop;
            if(tMin < tMax)
            {
                tStop = tMin;
                HitEntity = HitEntityMin;
                WallNormal = WallNormalMin;
            }
            else
            {
                tStop = tMax;
                HitEntity = HitEntityMax;
                WallNormal = WallNormalMax;
            }
            
            Entity->P += tStop*PlayerDelta;
            DistanceRemaining -= tStop*PlayerDeltaLength;            
            if(HitEntity)
            {
                PlayerDelta = DesiredPosition - Entity->P;                

                bool32 StopsOnCollision = HandleCollision(WorldMode, Entity, HitEntity);
                if(StopsOnCollision)
                {
                    PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;
                    Entity->dP = Entity->dP - 1*Inner(Entity->dP, WallNormal)*WallNormal;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }    
    
    if(Entity->DistanceLimit != 0.0f)
    {
        Entity->DistanceLimit = DistanceRemaining;
    }

#if 0
    // TODO(casey): Change to using the acceleration vector
    if((Entity->dP.x == 0.0f) && (Entity->dP.y == 0.0f))
    {
        // NOTE(casey): Leave FacingDirection whatever it was
    }
    else
    {
        Entity->FacingDirection = ATan2(Entity->dP.y, Entity->dP.x);
    }
#endif
}

enum traversable_search_flag
{
    TraversableSearch_Unoccupied = 0x1,
};
internal b32
GetClosestTraversable(sim_region *SimRegion, v3 FromP, traversable_reference *Result,
                      u32 Flags = 0)
{
    b32 Found = false;
    
    // TODO(casey): Make spatial queries easy for things!
    r32 ClosestDistanceSq = Square(1000.0f);
    entity *TestEntity = SimRegion->Entities;
    for(uint32 TestEntityIndex = 0;
        TestEntityIndex < SimRegion->EntityCount;
        ++TestEntityIndex, ++TestEntity)
    {
        for(u32 PIndex = 0;
            PIndex < TestEntity->TraversableCount;
            ++PIndex)
        {
            entity_traversable_point P = 
                GetSimSpaceTraversable(TestEntity, PIndex);
            if(!(Flags & TraversableSearch_Unoccupied) || (P.Occupier == 0))
            {
                v3 ToPoint = P.P - FromP;
                // TODO(casey): What should this value be??
                //ToPoint.z = ClampAboveZero(AbsoluteValue(ToPoint.z) - 1.0f);

                real32 TestDSq = LengthSq(ToPoint);            
                if(ClosestDistanceSq > TestDSq)
                {
                    // P.P;
                    Result->Entity.Ptr = TestEntity;
                    Result->Index = PIndex;
                    ClosestDistanceSq = TestDSq;
                    Found = true;
                }
            }
        }
    }

    if(!Found)
    {
        Result->Entity.Ptr = 0;
        Result->Index = 0;
    }
    
    return(Found);
}
