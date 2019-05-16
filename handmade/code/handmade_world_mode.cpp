/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

internal entity *
BeginLowEntity(game_mode_world *WorldMode, entity_type Type)
{
    Assert(WorldMode->CreationBufferIndex < ArrayCount(WorldMode->CreationBuffer));
    entity *EntityLow = WorldMode->CreationBuffer + WorldMode->CreationBufferIndex++;
    // TODO(casey): Worry about this taking awhile once the entities are large (sparse clear?)
    ZeroStruct(*EntityLow);

    EntityLow->ID.Value = ++WorldMode->LastUsedEntityStorageIndex;
    EntityLow->Type = Type;
    EntityLow->Collision = WorldMode->NullCollision;

    return(EntityLow);
}

internal void
EndEntity(game_mode_world *WorldMode, entity *EntityLow, world_position P)
{
    --WorldMode->CreationBufferIndex;
    Assert(EntityLow == (WorldMode->CreationBuffer + WorldMode->CreationBufferIndex));
    EntityLow->P = P.Offset_;
    PackEntityIntoWorld(WorldMode->World, EntityLow, P);
}

internal entity *
BeginGroundedEntity(game_mode_world *WorldMode, entity_type Type,
    entity_collision_volume_group *Collision)
{
    entity *Entity = BeginLowEntity(WorldMode, Type);
    Entity->Collision = Collision;

    return(Entity);
}

inline world_position
ChunkPositionFromTilePosition(world *World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ,
                              v3 AdditionalOffset = V3(0, 0, 0))
{
    world_position BasePos = {};

    real32 TileSideInMeters = 1.4f;
    real32 TileDepthInMeters = 3.0f;

    v3 TileDim = V3(TileSideInMeters, TileSideInMeters, TileDepthInMeters);
    v3 Offset = Hadamard(TileDim, V3((real32)AbsTileX, (real32)AbsTileY, (real32)AbsTileZ));
    world_position Result = MapIntoChunkSpace(World, BasePos, AdditionalOffset + Offset);

    Assert(IsCanonical(World, Result.Offset_));

    return(Result);
}

internal void
AddStandardRoom(game_mode_world *WorldMode, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ,
    random_series *Series)
{
    for(s32 OffsetY = -4;
        OffsetY <= 4;
        ++OffsetY)
    {
        for(s32 OffsetX = -8;
            OffsetX <= 8;
            ++OffsetX)
        {
            world_position P = ChunkPositionFromTilePosition(
                WorldMode->World, AbsTileX + OffsetX, AbsTileY + OffsetY, AbsTileZ);

 //           P.Offset_.z = 0.25f*(r32)(OffsetX + OffsetY);
            
            if((OffsetX == 2) && (OffsetY == 2))
            {
                entity *Entity = BeginGroundedEntity(WorldMode, EntityType_FloatyThingForNow,
                    WorldMode->FloorCollision);
                Entity->TraversableCount = 1;
                Entity->Traversables[0].P = V3(0, 0, 0);
                Entity->Traversables[0].Occupier = 0;
                EndEntity(WorldMode, Entity, P);
            }
            else
            {
                entity *Entity = BeginGroundedEntity(WorldMode, EntityType_Floor,
                    WorldMode->FloorCollision);
                Entity->TraversableCount = 1;
                Entity->Traversables[0].P = V3(0, 0, 0);
                Entity->Traversables[0].Occupier = 0;
                EndEntity(WorldMode, Entity, P);
            }
        }
    }
}

internal void
AddWall(game_mode_world *WorldMode, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(WorldMode->World, AbsTileX, AbsTileY, AbsTileZ);
    entity *Entity = BeginGroundedEntity(WorldMode, EntityType_Wall,
        WorldMode->WallCollision);
    AddFlags(Entity, EntityFlag_Collides);
    EndEntity(WorldMode, Entity, P);
}

internal void
AddStair(game_mode_world *WorldMode, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(WorldMode->World, AbsTileX, AbsTileY, AbsTileZ);
    entity *Entity = BeginGroundedEntity(WorldMode, EntityType_Stairwell,
        WorldMode->StairCollision);
    AddFlags(Entity, EntityFlag_Collides);
    Entity->WalkableDim = Entity->Collision->TotalVolume.Dim.xy;
    Entity->WalkableHeight = WorldMode->TypicalFloorHeight;
    EndEntity(WorldMode, Entity, P);
}

internal void
InitHitPoints(entity *EntityLow, uint32 HitPointCount)
{
    Assert(HitPointCount <= ArrayCount(EntityLow->HitPoint));
    EntityLow->HitPointMax = HitPointCount;
    for(uint32 HitPointIndex = 0;
        HitPointIndex < EntityLow->HitPointMax;
        ++HitPointIndex)
    {
        hit_point *HitPoint = EntityLow->HitPoint + HitPointIndex;
        HitPoint->Flags = 0;
        HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
    }
}

internal entity_id
AddPlayer(game_mode_world *WorldMode, sim_region *SimRegion, traversable_reference StandingOn)
{
    world_position P = MapIntoChunkSpace(SimRegion->World, SimRegion->Origin, 
                                         GetSimSpaceTraversable(StandingOn).P);

    entity *Body = BeginGroundedEntity(WorldMode, EntityType_HeroBody,
        WorldMode->HeroBodyCollision);
    AddFlags(Body, EntityFlag_Collides|EntityFlag_Moveable);

    entity *Head = BeginGroundedEntity(WorldMode, EntityType_HeroHead,
        WorldMode->HeroHeadCollision);
    AddFlags(Head, EntityFlag_Collides|EntityFlag_Moveable);

    InitHitPoints(Body, 3);

    // TODO(casey): We will probably need a creation-time system for
    // guaranteeing now overlapping occupation.
    Body->Occupying = StandingOn;
    
    Body->Head.Ptr = Head;
    Head->Head.Ptr = Body;

    if(WorldMode->CameraFollowingEntityIndex.Value == 0)
    {
        WorldMode->CameraFollowingEntityIndex = Head->ID;
    }

    entity_id Result = Head->ID;

    EndEntity(WorldMode, Head, P);
    EndEntity(WorldMode, Body, P);

    return(Result);
}

internal void
AddMonstar(game_mode_world *WorldMode, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(WorldMode->World, AbsTileX, AbsTileY, AbsTileZ);
    entity *Entity = BeginGroundedEntity(WorldMode, EntityType_Monstar, 
                                                     WorldMode->MonstarCollision);
    AddFlags(Entity, EntityFlag_Collides|EntityFlag_Moveable);

    InitHitPoints(Entity, 3);

    EndEntity(WorldMode, Entity, P);
}

internal void
AddFamiliar(game_mode_world *WorldMode, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(WorldMode->World, AbsTileX, AbsTileY, AbsTileZ);
    entity *Entity = BeginGroundedEntity(WorldMode, EntityType_Familiar, 
                                                     WorldMode->FamiliarCollision);
    AddFlags(Entity, EntityFlag_Collides|EntityFlag_Moveable);

    EndEntity(WorldMode, Entity, P);
}

internal void
DrawHitpoints(entity *Entity, render_group *PieceGroup, object_transform Transform)
{
    if(Entity->HitPointMax >= 1)
    {
        v2 HealthDim = {0.2f, 0.2f};
        real32 SpacingX = 1.5f*HealthDim.x;
        v2 HitP = {-0.5f*(Entity->HitPointMax - 1)*SpacingX, -0.25f};
        v2 dHitP = {SpacingX, 0.0f};
        for(uint32 HealthIndex = 0;
            HealthIndex < Entity->HitPointMax;
            ++HealthIndex)
        {
            hit_point *HitPoint = Entity->HitPoint + HealthIndex;
            v4 Color = {1.0f, 0.0f, 0.0f, 1.0f};
            if(HitPoint->FilledAmount == 0)
            {
                Color = V4(0.2f, 0.2f, 0.2f, 1.0f);
            }

            PushRect(PieceGroup, Transform, V3(HitP, 0), HealthDim, Color);
            HitP += dHitP;
        }
    }
}

internal void
ClearCollisionRulesFor(game_mode_world *WorldMode, uint32 ID)
{
    // TODO(casey): Need to make a better data structure that allows
    // removal of collision rules without searching the entire table
    // NOTE(casey): One way to make removal easy would be to always
    // add _both_ orders of the pairs of storage indices to the
    // hash table, so no matter which position the entity is in,
    // you can always find it.  Then, when you do your first pass
    // through for removal, you just remember the original top
    // of the free list, and when you're done, do a pass through all
    // the new things on the free list, and remove the reverse of
    // those pairs.
    for(uint32 HashBucket = 0;
        HashBucket < ArrayCount(WorldMode->CollisionRuleHash);
        ++HashBucket)
    {
        for(pairwise_collision_rule **Rule = &WorldMode->CollisionRuleHash[HashBucket];
            *Rule;
            )
        {
            if(((*Rule)->IDA == ID) ||
               ((*Rule)->IDB == ID))
            {
                pairwise_collision_rule *RemovedRule = *Rule;
                *Rule = (*Rule)->NextInHash;

                RemovedRule->NextInHash = WorldMode->FirstFreeCollisionRule;
                WorldMode->FirstFreeCollisionRule = RemovedRule;
            }
            else
            {
                Rule = &(*Rule)->NextInHash;
            }
        }
    }
}

internal void
AddCollisionRule(game_mode_world *WorldMode, uint32 IDA, uint32 IDB, bool32 CanCollide)
{
    // TODO(casey): Collapse this with ShouldCollide
    if(IDA > IDB)
    {
        uint32 Temp = IDA;
        IDA = IDB;
        IDB = Temp;
    }

    // TODO(casey): BETTER HASH FUNCTION
    pairwise_collision_rule *Found = 0;
    uint32 HashBucket = IDA & (ArrayCount(WorldMode->CollisionRuleHash) - 1);
    for(pairwise_collision_rule *Rule = WorldMode->CollisionRuleHash[HashBucket];
        Rule;
        Rule = Rule->NextInHash)
    {
        if((Rule->IDA == IDA) &&
           (Rule->IDB == IDB))
        {
            Found = Rule;
            break;
        }
    }

    if(!Found)
    {
        Found = WorldMode->FirstFreeCollisionRule;
        if(Found)
        {
            WorldMode->FirstFreeCollisionRule = Found->NextInHash;
        }
        else
        {
            Found = PushStruct(&WorldMode->World->Arena, pairwise_collision_rule);
        }

        Found->NextInHash = WorldMode->CollisionRuleHash[HashBucket];
        WorldMode->CollisionRuleHash[HashBucket] = Found;
    }

    if(Found)
    {
        Found->IDA = IDA;
        Found->IDB = IDB;
        Found->CanCollide = CanCollide;
    }
}

entity_collision_volume_group *
MakeSimpleGroundedCollision(game_mode_world *WorldMode, real32 DimX, real32 DimY, real32 DimZ,
    real32 OffsetZ = 0.0f)
{
    // TODO(casey): NOT WORLD ARENA!  Change to using the fundamental types arena, etc.
    entity_collision_volume_group *Group = PushStruct(&WorldMode->World->Arena, entity_collision_volume_group);
    Group->VolumeCount = 1;
    Group->Volumes = PushArray(&WorldMode->World->Arena, Group->VolumeCount, entity_collision_volume);
    Group->TotalVolume.OffsetP = V3(0, 0, 0.5f*DimZ + OffsetZ);
    Group->TotalVolume.Dim = V3(DimX, DimY, DimZ);
    Group->Volumes[0] = Group->TotalVolume;

    return(Group);
}

entity_collision_volume_group *
MakeSimpleFloorCollision(game_mode_world *WorldMode, real32 DimX, real32 DimY, real32 DimZ)
{
    // TODO(casey): NOT WORLD ARENA!  Change to using the fundamental types arena, etc.
    entity_collision_volume_group *Group = PushStruct(&WorldMode->World->Arena, entity_collision_volume_group);
    Group->VolumeCount = 0;
    Group->TotalVolume.OffsetP = V3(0, 0, 0);
    Group->TotalVolume.Dim = V3(DimX, DimY, DimZ);
    
#if 0
    Group->VolumeCount = 1;
    Group->Volumes = PushArray(&WorldMode->World->Arena, Group->VolumeCount, entity_collision_volume);
    Group->TotalVolume.OffsetP = V3(0, 0, 0.5f*DimZ);
    Group->TotalVolume.Dim = V3(DimX, DimY, DimZ);
    Group->Volumes[0] = Group->TotalVolume;
#endif

    return(Group);
}

entity_collision_volume_group *
MakeNullCollision(game_mode_world *WorldMode)
{
    // TODO(casey): NOT WORLD ARENA!  Change to using the fundamental types arena, etc.
    entity_collision_volume_group *Group = PushStruct(&WorldMode->World->Arena, entity_collision_volume_group);
    Group->VolumeCount = 0;
    Group->Volumes = 0;
    Group->TotalVolume.OffsetP = V3(0, 0, 0);
    // TODO(casey): Should this be negative?
    Group->TotalVolume.Dim = V3(0, 0, 0);

    return(Group);
}

internal void
PlayWorld(game_state *GameState, transient_state *TranState)
{
    SetGameMode(GameState, TranState, GameMode_World);

    game_mode_world *WorldMode = PushStruct(&GameState->ModeArena, game_mode_world);

    uint32 TilesPerWidth = 17;
    uint32 TilesPerHeight = 9;

    WorldMode->EffectsEntropy = RandomSeed(1234);
    WorldMode->TypicalFloorHeight = 3.0f;

    // TODO(casey): Remove this!
    real32 PixelsToMeters = 1.0f / 42.0f;
    v3 WorldChunkDimInMeters = {PixelsToMeters*(real32)GroundBufferWidth,
                                PixelsToMeters*(real32)GroundBufferHeight,
                                WorldMode->TypicalFloorHeight};
    WorldMode->World = CreateWorld(WorldChunkDimInMeters, &GameState->ModeArena);

    real32 TileSideInMeters = 1.4f;
    real32 TileDepthInMeters = WorldMode->TypicalFloorHeight;

    WorldMode->NullCollision = MakeNullCollision(WorldMode);
    WorldMode->StairCollision = MakeSimpleGroundedCollision(WorldMode,
                                                            TileSideInMeters,
                                                            2.0f*TileSideInMeters,
                                                            1.1f*TileDepthInMeters);
    WorldMode->HeroHeadCollision = MakeSimpleGroundedCollision(WorldMode, 1.0f, 0.5f, 0.5f, 0.7f);
    WorldMode->HeroBodyCollision = MakeSimpleGroundedCollision(WorldMode, 1.0f, 0.5f, 0.6f);
    WorldMode->MonstarCollision = MakeSimpleGroundedCollision(WorldMode, 1.0f, 0.5f, 0.5f);
    WorldMode->FamiliarCollision = MakeSimpleGroundedCollision(WorldMode, 1.0f, 0.5f, 0.5f);
    WorldMode->WallCollision = MakeSimpleGroundedCollision(WorldMode,
                                                           TileSideInMeters,
                                                           TileSideInMeters,
                                                           TileDepthInMeters);
    WorldMode->FloorCollision = MakeSimpleFloorCollision(WorldMode,
        TileSideInMeters,
        TileSideInMeters,
        TileDepthInMeters);

    random_series Series = RandomSeed(1234);

    uint32 ScreenBaseX = 0;
    uint32 ScreenBaseY = 0;
    uint32 ScreenBaseZ = 0;
    uint32 ScreenX = ScreenBaseX;
    uint32 ScreenY = ScreenBaseY;
    uint32 AbsTileZ = ScreenBaseZ;

    // TODO(casey): Replace all this with real world generation!
    bool32 DoorLeft = false;
    bool32 DoorRight = false;
    bool32 DoorTop = false;
    bool32 DoorBottom = false;
    bool32 DoorUp = false;
    bool32 DoorDown = false;
    for(uint32 ScreenIndex = 0;
        ScreenIndex < 1;
        ++ScreenIndex)
    {
#if 0
        uint32 DoorDirection = RandomChoice(&Series, (DoorUp || DoorDown) ? 2 : 4);
#else
        uint32 DoorDirection = RandomChoice(&Series, 2);
#endif

//            DoorDirection = 3;

        bool32 CreatedZDoor = false;
        if(DoorDirection == 3)
        {                
            CreatedZDoor = true;
            DoorDown = true;
        }
        else if(DoorDirection == 2)
        {
            CreatedZDoor = true;
            DoorUp = true;
        }
        else if(DoorDirection == 1)
        {
            DoorRight = true;
        }
        else
        {
            DoorTop = true;
        }

        AddStandardRoom(WorldMode,
                        ScreenX*TilesPerWidth + TilesPerWidth/2,
                        ScreenY*TilesPerHeight + TilesPerHeight/2,
                        AbsTileZ, &Series);
                    
        for(uint32 TileY = 0;
            TileY < TilesPerHeight;
            ++TileY)
        {
            for(uint32 TileX = 0;
                TileX < TilesPerWidth;
                ++TileX)
            {
                uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
                uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;

                bool32 ShouldBeDoor = false;
                if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
                {
                    ShouldBeDoor = true;
                }

                if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
                {
                    ShouldBeDoor = true;
                }

                if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
                {
                    ShouldBeDoor = true;
                }

                if((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
                {
                    ShouldBeDoor = true;
                }

                if(TileX == 14)
                {
                    AddWall(WorldMode, AbsTileX, AbsTileY, AbsTileZ);
                }
                
                if(ShouldBeDoor)
                {
                    AddWall(WorldMode, AbsTileX, AbsTileY, AbsTileZ);
                }
                else if(CreatedZDoor)
                {
                    if(((AbsTileZ % 2) && (TileX == 10) && (TileY == 5)) ||
                       (!(AbsTileZ % 2) && (TileX == 4) && (TileY == 5)))
                    {
                        AddStair(WorldMode, AbsTileX, AbsTileY, DoorDown ? AbsTileZ - 1 : AbsTileZ);
                    }
                }
            }
        }

        DoorLeft = DoorRight;
        DoorBottom = DoorTop;

        if(CreatedZDoor)
        {
            DoorDown = !DoorDown;
            DoorUp = !DoorUp;
        }
        else
        {
            DoorUp = false;
            DoorDown = false;
        }

        DoorRight = false;
        DoorTop = false;

        if(DoorDirection == 3)
        {
            AbsTileZ -= 1;                
        }
        else if(DoorDirection == 2)
        {
            AbsTileZ += 1;
        }
        else if(DoorDirection == 1)
        {
            ScreenX += 1;
        }
        else
        {
            ScreenY += 1;
        }
    }

#if 0
    while(WorldMode->LowEntityCount < (ArrayCount(WorldMode->LowEntities) - 16))
    {
        uint32 Coordinate = 1024 + WorldMode->LowEntityCount;
        AddWall(WorldMode, Coordinate, Coordinate, Coordinate);
    }
#endif

    world_position NewCameraP = {};
    uint32 CameraTileX = ScreenBaseX*TilesPerWidth + 17/2;
    uint32 CameraTileY = ScreenBaseY*TilesPerHeight + 9/2;
    uint32 CameraTileZ = ScreenBaseZ;
    NewCameraP = ChunkPositionFromTilePosition(WorldMode->World,
                                               CameraTileX,
                                               CameraTileY,
                                               CameraTileZ);
    WorldMode->CameraP = NewCameraP;

    AddMonstar(WorldMode, CameraTileX - 3, CameraTileY + 2, CameraTileZ);
    for(u32 FamiliarIndex = 0;
        FamiliarIndex < 1;
        ++FamiliarIndex)
    {
        int32 FamiliarOffsetX = RandomBetween(&Series, -7, 7);
        int32 FamiliarOffsetY = RandomBetween(&Series, -3, -1);
        if((FamiliarOffsetX != 0) ||
           (FamiliarOffsetY != 0))
        {
            AddFamiliar(WorldMode, CameraTileX + FamiliarOffsetX, CameraTileY + FamiliarOffsetY,
                        CameraTileZ);
        }
    }

    GameState->WorldMode = WorldMode;
}

internal b32
UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, transient_state *TranState,
                     game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
{
    TIMED_FUNCTION();

    b32 Result = false;

    world *World = WorldMode->World;

    v2 MouseP = {Input->MouseX, Input->MouseY};

    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
    real32 MetersToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;

    real32 FocalLength = 0.6f;
    real32 DistanceAboveGround = 9.0f;
    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels, FocalLength, DistanceAboveGround);

    Clear(RenderGroup, V4(0.25f, 0.25f, 0.25f, 0.0f));

    v2 ScreenCenter = {0.5f*(real32)DrawBuffer->Width,
                       0.5f*(real32)DrawBuffer->Height};

    rectangle2 ScreenBounds = GetCameraRectangleAtTarget(RenderGroup);
    rectangle3 CameraBoundsInMeters = RectMinMax(V3(ScreenBounds.Min, 0.0f), V3(ScreenBounds.Max, 0.0f));
    CameraBoundsInMeters.Min.z = -3.0f*WorldMode->TypicalFloorHeight;
    CameraBoundsInMeters.Max.z =  1.0f*WorldMode->TypicalFloorHeight;    

    real32 FadeTopEndZ = 0.75f*WorldMode->TypicalFloorHeight;
    real32 FadeTopStartZ = 0.5f*WorldMode->TypicalFloorHeight;
    real32 FadeBottomStartZ = -2.0f*WorldMode->TypicalFloorHeight;
    real32 FadeBottomEndZ = -2.25f*WorldMode->TypicalFloorHeight;

    // TODO(casey): How big do we actually want to expand here?
    // TODO(casey): Do we want to simulate upper floors, etc.?
    v3 SimBoundsExpansion = {15.0f, 15.0f, 15.0f};
    rectangle3 SimBounds = AddRadiusTo(CameraBoundsInMeters, SimBoundsExpansion);
    temporary_memory SimMemory = BeginTemporaryMemory(&TranState->TranArena);
    world_position SimCenterP = WorldMode->CameraP;
    sim_region *SimRegion = BeginSim(&TranState->TranArena, WorldMode, WorldMode->World,
                                     SimCenterP, SimBounds, Input->dtForFrame);

    v3 CameraP = Subtract(World, &WorldMode->CameraP, &SimCenterP) + WorldMode->CameraOffset;

    object_transform WorldTransform = DefaultUprightTransform();
    WorldTransform.OffsetP -= CameraP;

    PushRectOutline(RenderGroup, WorldTransform, V3(0.0f, 0.0f, 0.0f), GetDim(ScreenBounds), V4(1.0f, 1.0f, 0.0f, 1));
//    PushRectOutline(RenderGroup, V3(0.0f, 0.0f, 0.0f), GetDim(CameraBoundsInMeters).xy, V4(1.0f, 1.0f, 1.0f, 1));
    PushRectOutline(RenderGroup, WorldTransform, V3(0.0f, 0.0f, 0.0f), GetDim(SimBounds).xy, V4(0.0f, 1.0f, 1.0f, 1));
    PushRectOutline(RenderGroup, WorldTransform, V3(0.0f, 0.0f, 0.0f), GetDim(SimRegion->Bounds).xy, V4(1.0f, 0.0f, 1.0f, 1));


    b32 HeroesExist = false;
    b32 QuitRequested = false;
    for(u32 ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;
        if(ConHero->EntityIndex.Value == 0)
        {
            if(WasPressed(Controller->Back))
            {
                QuitRequested = true;
            }
            else if(WasPressed(Controller->Start))
            {
                *ConHero = {};
                traversable_reference Traversable;
                if(GetClosestTraversable(SimRegion, CameraP, &Traversable))
                {
                    HeroesExist = true;
                    ConHero->EntityIndex = AddPlayer(WorldMode, SimRegion, Traversable);
                }
                else
                {
                    // TODO(casey): GameUI that tells you there's no safe place...
                    // maybe keep trying on subsequent frames?
                }
            }
        }
        else //if(ConHero->EntityIndex.Value)
        {
            HeroesExist = true;
            
            ConHero->dZ = 0.0f;
            ConHero->dSword = {};

            if(Controller->IsAnalog)
            {
                // NOTE(casey): Use analog movement tuning
                ConHero->ddP = V2(Controller->StickAverageX, Controller->StickAverageY);
            }
            else
            {
                // NOTE(casey): Use digital movement tuning
                r32 Recenter = 0.5f;
                if(WasPressed(Controller->MoveUp))
                {
                    ConHero->ddP.x = 0.0f;
                    ConHero->ddP.y = 1.0f;
                    ConHero->RecenterTimer = Recenter;
                }
                if(WasPressed(Controller->MoveDown))
                {
                    ConHero->ddP.x = 0.0f;
                    ConHero->ddP.y = -1.0f;
                    ConHero->RecenterTimer = Recenter;
                }
                if(WasPressed(Controller->MoveLeft))
                {
                    ConHero->ddP.x = -1.0f;
                    ConHero->ddP.y = 0.0f;
                    ConHero->RecenterTimer = Recenter;
                }
                if(WasPressed(Controller->MoveRight))
                {
                    ConHero->ddP.x = 1.0f;
                    ConHero->ddP.y = 0.0f;
                    ConHero->RecenterTimer = Recenter;
                }

                if(!IsDown(Controller->MoveLeft) &&
                   !IsDown(Controller->MoveRight))
                {
                    ConHero->ddP.x = 0.0f;
                    if(IsDown(Controller->MoveUp))
                    {
                        ConHero->ddP.y = 1.0f;
                    }
                    if(IsDown(Controller->MoveDown))
                    {
                        ConHero->ddP.y = -1.0f;
                    }
                } 

                if(!IsDown(Controller->MoveUp) &&
                   !IsDown(Controller->MoveDown))
                {
                    ConHero->ddP.y = 0.0f;
                    if(IsDown(Controller->MoveLeft))
                    {
                        ConHero->ddP.x = -1.0f;
                    }
                    if(IsDown(Controller->MoveRight))
                    {
                        ConHero->ddP.x = 1.0f;
                    }
                }
                
                if(WasPressed(Controller->Start))
                {
                    ConHero->DebugSpawn = true;
                }
            }

#if 0
            if(Controller->Start.EndedDown)
            {
                ConHero->dZ = 3.0f;
            }
#endif

            ConHero->dSword = {};
            if(Controller->ActionUp.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 10.0f, V2(1.0f, 1.0f));
                ConHero->dSword = V2(0.0f, 1.0f);
            }
            if(Controller->ActionDown.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 10.0f, V2(0.0f, 0.0f));
                ConHero->dSword = V2(0.0f, -1.0f);
            }
            if(Controller->ActionLeft.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 5.0f, V2(1.0f, 0.0f));
                ConHero->dSword = V2(-1.0f, 0.0f);
            }
            if(Controller->ActionRight.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 5.0f, V2(0.0f, 1.0f));
                ConHero->dSword = V2(1.0f, 0.0f);
            }

            if(WasPressed(Controller->Back))
            {
                ConHero->Exited = true;
            }
        }
    }

    // TODO(casey): Move this out into handmade_entity.cpp!
    {
        TIMED_BLOCK("EntityRender");

        for(uint32 EntityIndex = 0;
            EntityIndex < SimRegion->EntityCount;
            ++EntityIndex)
        {
            entity *Entity = SimRegion->Entities + EntityIndex;

            // TODO(casey): Set this at construction
            Entity->XAxis = V2(1, 0);
            Entity->YAxis = V2(0, 1);

            // TODO(casey): We don't really have a way to unique-ify these :(
            debug_id EntityDebugID = DEBUG_POINTER_ID((void *)Entity->ID.Value);
            if(DEBUG_REQUESTED(EntityDebugID))
            {
                DEBUG_BEGIN_DATA_BLOCK("Simulation/Entity");
            }

            if(Entity->Updatable)
            {
                real32 dt = Input->dtForFrame;

                // TODO(casey): This is incorrect, should be computed after update!!!!
                real32 ShadowAlpha = 1.0f - 0.5f*Entity->P.z;
                if(ShadowAlpha < 0)
                {
                    ShadowAlpha = 0.0f;
                }

                move_spec MoveSpec = DefaultMoveSpec();
                v3 ddP = {};

                // TODO(casey): Probably indicates we want to separate update and render
            // for entities sometime soon?
                v3 CameraRelativeGroundP = GetEntityGroundPoint(Entity) - CameraP;
                RenderGroup->GlobalAlpha = 1.0f;
                if(CameraRelativeGroundP.z > FadeTopStartZ)
                {
                    RenderGroup->GlobalAlpha = Clamp01MapToRange(FadeTopEndZ, CameraRelativeGroundP.z, FadeTopStartZ);
                }
                else if(CameraRelativeGroundP.z < FadeBottomStartZ)
                {
                    RenderGroup->GlobalAlpha = Clamp01MapToRange(FadeBottomEndZ, CameraRelativeGroundP.z, FadeBottomStartZ);
                }

                //
            // NOTE(casey): Pre-physics entity work
            //
                hero_bitmap_ids HeroBitmaps = {};
                asset_vector MatchVector = {};
                MatchVector.E[Tag_FacingDirection] = Entity->FacingDirection;
                asset_vector WeightVector = {};
                WeightVector.E[Tag_FacingDirection] = 1.0f;
                HeroBitmaps.Head = GetBestMatchBitmapFrom(TranState->Assets, Asset_Head, &MatchVector, &WeightVector);
                HeroBitmaps.Cape = GetBestMatchBitmapFrom(TranState->Assets, Asset_Cape, &MatchVector, &WeightVector);
                HeroBitmaps.Torso = GetBestMatchBitmapFrom(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector);
                switch(Entity->Type)
                {
                    case EntityType_HeroHead:
                    {
                        DEBUG_VALUE(Entity->P);
                        // TODO(casey): Now that we have some real usage examples, let's solidify
                        // the positioning system!
                        controlled_hero ConHero_ = {};
                        ConHero_.ddP.x = 1.0f;
                        
                        controlled_hero *ConHero = &ConHero_;
                        for(uint32 ControlIndex = 0;
                            ControlIndex < ArrayCount(GameState->ControlledHeroes);
                            ++ControlIndex)
                        {
                            controlled_hero *TestHero = 
                                GameState->ControlledHeroes + ControlIndex;
                            if(Entity->ID.Value == TestHero->EntityIndex.Value)
                            {
                                ConHero = TestHero;
                                
                                if(ConHero->DebugSpawn)
                                {
                                    traversable_reference Traversable;
                                    if(GetClosestTraversable(SimRegion, Entity->P, &Traversable, 
                                                             TraversableSearch_Unoccupied))
                                    {
                                        AddPlayer(WorldMode, SimRegion, Traversable);
                                    }
                                    else
                                    {
                                        // TODO(casey): GameUI that tells you there's no safe place...
                                        // maybe keep trying on subsequent frames?
                                    }
                                    
                                    ConHero->DebugSpawn = false;
                                }
                            }
                        }
                        
                        ConHero->RecenterTimer = ClampAboveZero(ConHero->RecenterTimer - dt);

                        entity *Head = Entity;
                        entity *Body = Head->Head.Ptr;

                        if(ConHero->dZ != 0.0f)
                        {
                            Entity->dP.z = ConHero->dZ;
                        }

                        MoveSpec.UnitMaxAccelVector = true;
                        MoveSpec.Speed = 30.0f;
                        MoveSpec.Drag = 8.0f;

                        ddP = V3(ConHero->ddP, 0);

                        // TODO(casey): Change to using the acceleration vector
                        if((ConHero->dSword.x == 0.0f) && (ConHero->dSword.y == 0.0f))
                        {
                            // NOTE(casey): Leave FacingDirection whatever it was
                        }
                        else
                        {
                            Entity->FacingDirection = ATan2(ConHero->dSword.y, ConHero->dSword.x);
                        }

                        traversable_reference Traversable;
                        if(GetClosestTraversable(SimRegion, Head->P, &Traversable))
                        {
                            if(Body)
                            {
                                if(Body->MovementMode == MovementMode_Planted)
                                {
                                    if(!IsEqual(Traversable, Body->Occupying))
                                    {
                                        Body->CameFrom = Body->Occupying;
                                        if(TransactionalOccupy(Body, &Body->Occupying, Traversable))
                                        {
                                            Body->tMovement = 0.0f;
                                            Body->MovementMode = MovementMode_Hopping;
                                        }
                                    }
                                }
                            }

                            v3 ClosestP = GetSimSpaceTraversable(Traversable).P;

                            b32 TimerIsUp = (ConHero->RecenterTimer == 0.0f);
                            b32 NoPush = (LengthSq(ddP) < 0.1f);
                            r32 Cp = NoPush ? 300.0f : 25.0f;
                            v3 ddP2 = {};
                            for(u32 E = 0;
                                E < 3;
                                ++E)
                            {
#if 1
                                if(NoPush || (TimerIsUp && (Square(ddP.E[E]) < 0.1f)))
#else
                                if(NoPush)
#endif
                                {
                                    ddP2.E[E] = Cp*(ClosestP.E[E] - Entity->P.E[E]) - 30.0f*Entity->dP.E[E];
                                }
                            }
                            Entity->dP += dt*ddP2;
                        }

                        if(Body)
                        {
                            Body->FacingDirection = Head->FacingDirection;
                            // Body->XAxis = Perp(Body->YAxis);
                        }

                        if(ConHero->Exited)
                        {
                            ConHero->Exited = false;
                            DeleteEntity(SimRegion, Entity);
                            ConHero->EntityIndex.Value = 0;
                        }
                    } break;

                    case EntityType_HeroBody:
                    {
                        DEBUG_VALUE(Entity->P);
                        entity *Head = Entity->Head.Ptr;
                        entity *Body = Entity;
                        
                        Entity->dP = V3(0, 0, 0);
                        
                        if(Entity->MovementMode == MovementMode_Planted)
                        {
                            Entity->P = GetSimSpaceTraversable(Entity->Occupying).P;
                        }

                        v3 HeadDelta = {};
                        if(Head)
                        {
                            HeadDelta = Head->P - Body->P;
                        }
                        Body->FloorDisplace = (0.25f*HeadDelta).xy;
                        Body->YAxis = V2(0, 1) + 0.5f*HeadDelta.xy;
                        
                        r32 ddtBob = 0.0f;
                        switch(Entity->MovementMode)
                        {
                            case MovementMode_Planted:
                            {
                                if(Head)
                                {
                                    r32 HeadDistance = Length(HeadDelta);
                                    r32 MaxHeadDistance = 0.5f;
                                    r32 tHeadDistance = Clamp01MapToRange(0.0f, HeadDistance, MaxHeadDistance);
                                    ddtBob = -20.0f*tHeadDistance;
                                }
                            } break;
                            
                            case MovementMode_Hopping:
                            {
                                v3 MovementTo = GetSimSpaceTraversable(Entity->Occupying).P;
                                v3 MovementFrom = GetSimSpaceTraversable(Entity->CameFrom).P;
                                
                                r32 tJump = 0.1f;
                                r32 tThrust = 0.2f;
                                r32 tLand = 0.9f;
                                
                                if(Entity->tMovement < tThrust)
                                {
                                    ddtBob = 30.0f;
                                }    

                                if(Entity->tMovement < tLand)
                                {
                                    r32 t = Clamp01MapToRange(tJump, Entity->tMovement, tLand);
                                    v3 a = V3(0, -2.0f, 0.0f);
                                    v3 b = (MovementTo - MovementFrom) - a;
                                    Entity->P = a*t*t + b*t + MovementFrom;
                                }

                                if(Entity->tMovement >= 1.0f)
                                {
                                    Entity->P = MovementTo;
                                    Entity->CameFrom = Entity->Occupying;
                                    Entity->MovementMode = MovementMode_Planted;
                                    Entity->dtBob = -2.0f;
                                }

                                Entity->tMovement += 4.0f*dt;
                                if(Entity->tMovement > 1.0f) 
                                {
                                    Entity->tMovement = 1.0f;
                                }
                            } break;
                        }

                        r32 Cp = 100.0f;
                        r32 Cv = 10.0f;
                        ddtBob += Cp*(0.0f - Entity->tBob) + Cv*(0.0f - Entity->dtBob);
                        Entity->tBob += ddtBob*dt*dt + Entity->dtBob*dt;
                        Entity->dtBob += ddtBob*dt;
                    } break;

                    case EntityType_FloatyThingForNow:
                    {
                        Entity->P.z += 0.05f*Cos(Entity->tBob);
                        Entity->tBob += dt;
                    } break;
                    
                    case EntityType_Familiar:
                    {
                        entity *ClosestHero = 0;
                        real32 ClosestHeroDSq = Square(10.0f); // NOTE(casey): Ten meter maximum search!

                        if(Global_AI_Familiar_FollowsHero)
                        {
                            // TODO(casey): Make spatial queries easy for things!
                            entity *TestEntity = SimRegion->Entities;
                            for(uint32 TestEntityIndex = 0;
                                TestEntityIndex < SimRegion->EntityCount;
                                ++TestEntityIndex, ++TestEntity)
                            {
                                if(TestEntity->Type == EntityType_HeroBody)
                                {            
                                    real32 TestDSq = LengthSq(TestEntity->P - Entity->P);            
                                    if(ClosestHeroDSq > TestDSq)
                                    {
                                        ClosestHero = TestEntity;
                                        ClosestHeroDSq = TestDSq;
                                    }
                                }
                            }
                        }

                        if(ClosestHero && (ClosestHeroDSq > Square(3.0f)))
                        {
                            real32 Acceleration = 0.5f;
                            real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
                            ddP = OneOverLength*(ClosestHero->P - Entity->P);
                        }

                        MoveSpec.UnitMaxAccelVector = true;
                        MoveSpec.Speed = 50.0f;
                        MoveSpec.Drag = 8.0f;
                    } break;
                }

                if(IsSet(Entity, EntityFlag_Moveable))
                {
                    MoveEntity(WorldMode, SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
                }

                object_transform EntityTransform = DefaultUprightTransform();
                EntityTransform.OffsetP = GetEntityGroundPoint(Entity) - CameraP;

                //
            // NOTE(casey): Post-physics entity work
            //            
                real32 HeroSizeC = 3.0f;
                switch(Entity->Type)
                {
                    case EntityType_HeroBody:
                    {
                        v4 Color = {1, 1, 1, 1};
                        v2 XAxis = Entity->XAxis;
                        v2 YAxis = Entity->YAxis;
                        v3 Offset = V3(Entity->FloorDisplace, 0.0f);
                        PushBitmap(RenderGroup, EntityTransform, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                        PushBitmap(RenderGroup, EntityTransform, HeroBitmaps.Torso, HeroSizeC*1.2f, V3(0, 0.0f, -0.002f), Color, 1.0f, XAxis, YAxis);
                        PushBitmap(RenderGroup, EntityTransform, HeroBitmaps.Cape, HeroSizeC*1.2f, Offset + V3(0, Entity->tBob - 0.1f, -0.001f), Color, 1.0f, XAxis, YAxis);
                        DrawHitpoints(Entity, RenderGroup, EntityTransform);
                    } break;

                    case EntityType_HeroHead:
                    {                    
                        // TODO(casey): Z!!!
                        PushBitmap(RenderGroup, EntityTransform, HeroBitmaps.Head, HeroSizeC*1.2f, V3(0, -0.7f, 0));
                    } break;

                    case EntityType_Wall:
                    {
                        bitmap_id BID = GetFirstBitmapFrom(TranState->Assets, Asset_Tree);
                        if(DEBUG_REQUESTED(EntityDebugID))
                        {
                            DEBUG_NAMED_VALUE(BID);
                        }

                        PushBitmap(RenderGroup, EntityTransform, BID, 2.5f, V3(0, 0, 0));
                    } break;

                    case EntityType_Stairwell:
                    {
                        PushRect(RenderGroup, EntityTransform, V3(0, 0, 0), Entity->WalkableDim, V4(1, 0.5f, 0, 1));
                        PushRect(RenderGroup, EntityTransform, V3(0, 0, Entity->WalkableHeight), Entity->WalkableDim, V4(1, 1, 0, 1));
                    } break;

                    case EntityType_Familiar:
                    {
                        bitmap_id BID = HeroBitmaps.Head;
                        if(DEBUG_REQUESTED(EntityDebugID))
                        {
                            DEBUG_NAMED_VALUE(BID);
                        }

                        Entity->tBob += dt;
                        if(Entity->tBob > Tau32)
                        {
                            Entity->tBob -= Tau32;
                        }
                        real32 BobSin = Sin(2.0f*Entity->tBob);
                        PushBitmap(RenderGroup, EntityTransform, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 2.5f, V3(0, 0, 0), V4(1, 1, 1, (0.5f*ShadowAlpha) + 0.2f*BobSin));
                        PushBitmap(RenderGroup, EntityTransform, BID, 2.5f, V3(0, 0, 0.25f*BobSin));
                    } break;

                    case EntityType_Monstar:
                    {
                        DEBUG_VALUE(CameraRelativeGroundP);
                        PushBitmap(RenderGroup, EntityTransform, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 4.5f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                        PushBitmap(RenderGroup, EntityTransform, HeroBitmaps.Torso, 4.5f, V3(0, 0, 0));

                        DrawHitpoints(Entity, RenderGroup, EntityTransform);
                    } break;

                    case EntityType_Floor:
                    case EntityType_FloatyThingForNow:
                    {
                        for(uint32 VolumeIndex = 0;
                            VolumeIndex < Entity->Collision->VolumeCount;
                            ++VolumeIndex)
                        {
                            entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        
                            PushRectOutline(RenderGroup, EntityTransform, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, V4(0, 0.5f, 1.0f, 1));
                        }

                        for(uint32 TraversableIndex = 0;
                            TraversableIndex < Entity->TraversableCount;
                            ++TraversableIndex)
                        {
                            entity_traversable_point *Traversable = 
                                Entity->Traversables + TraversableIndex;                        
                            PushRect(RenderGroup, EntityTransform, Traversable->P, V2(0.1f, 0.1f), 
                                     Traversable->Occupier ? V4(0.0f, 0.5f, 1.0f, 1) : V4(1.0, 0.5f, 0.0f, 1));
                        }
                    } break;

                    default:
                    {
                        // InvalidCodePath;
                    } break;
                }

                if(DEBUG_UI_ENABLED)
                {
                    debug_id EntityDebugID = DEBUG_POINTER_ID((void *)Entity->ID.Value);

                    for(uint32 VolumeIndex = 0;
                        VolumeIndex < Entity->Collision->VolumeCount;
                        ++VolumeIndex)
                    {
                        entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        

                        v3 LocalMouseP = Unproject(RenderGroup, EntityTransform, MouseP);

                        if((LocalMouseP.x > -0.5f*Volume->Dim.x) && (LocalMouseP.x < 0.5f*Volume->Dim.x) &&
                                (LocalMouseP.y > -0.5f*Volume->Dim.y) && (LocalMouseP.y < 0.5f*Volume->Dim.y))
                        {
                            DEBUG_HIT(EntityDebugID, LocalMouseP.z);
                        }

                        v4 OutlineColor;
                        if(DEBUG_HIGHLIGHTED(EntityDebugID, &OutlineColor))
                        {
                            PushRectOutline(RenderGroup, EntityTransform, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, OutlineColor, 0.05f);
                        }
                    }
                }

                if(DEBUG_REQUESTED(EntityDebugID))
                {
                    DEBUG_VALUE(Entity->ID.Value);
                    DEBUG_VALUE(Entity->Updatable);
                    DEBUG_VALUE(Entity->Type);
                    DEBUG_VALUE(Entity->P);
                    DEBUG_VALUE(Entity->dP);
                    DEBUG_VALUE(Entity->DistanceLimit);
                    DEBUG_VALUE(Entity->FacingDirection);
                    DEBUG_VALUE(Entity->tBob);
                    DEBUG_VALUE(Entity->dAbsTileZ);
                    DEBUG_VALUE(Entity->HitPointMax);
#if 0
                    DEBUG_BEGIN_ARRAY(Entity->HitPoint);                    
                    for(u32 HitPointIndex = 0;
                        HitPointIndex < Entity->HitPointMax;
                        ++HitPointIndex)
                    {
                        DEBUG_VALUE(Entity->HitPoint[HitPointIndex]);
                    }
                    DEBUG_END_ARRAY();
                    DEBUG_VALUE(Entity->Sword);
#endif
                    DEBUG_VALUE(Entity->WalkableDim);
                    DEBUG_VALUE(Entity->WalkableHeight);

                    DEBUG_END_DATA_BLOCK("Simulation/Entity");
                }
            }
        }
    }

    RenderGroup->GlobalAlpha = 1.0f;

#if 0
    WorldMode->Time += Input->dtForFrame;

    v3 MapColor[] =
        {
            {1, 0, 0},
            {0, 1, 0},
            {0, 0, 1},
        };
    for(uint32 MapIndex = 0;
        MapIndex < ArrayCount(TranState->EnvMaps);
        ++MapIndex)
    {
        environment_map *Map = TranState->EnvMaps + MapIndex;
        loaded_bitmap *LOD = Map->LOD + 0;
        bool32 RowCheckerOn = false;
        int32 CheckerWidth = 16;
        int32 CheckerHeight = 16;
        rectangle2i ClipRect = {0, 0, LOD->Width, LOD->Height};
        for(int32 Y = 0;
            Y < LOD->Height;
            Y += CheckerHeight)
        {
            bool32 CheckerOn = RowCheckerOn;
            for(int32 X = 0;
                X < LOD->Width;
                X += CheckerWidth)
            {
                v4 Color = CheckerOn ? V4(MapColor[MapIndex], 1.0f) : V4(0, 0, 0, 1);
                v2 MinP = V2i(X, Y);
                v2 MaxP = MinP + V2i(CheckerWidth, CheckerHeight);
                DrawRectangle(LOD, MinP, MaxP, Color, ClipRect, true);
                DrawRectangle(LOD, MinP, MaxP, Color, ClipRect, false);
                CheckerOn = !CheckerOn;
            }
            RowCheckerOn = !RowCheckerOn;
        }
    }
    TranState->EnvMaps[0].Pz = -1.5f;
    TranState->EnvMaps[1].Pz = 0.0f;
    TranState->EnvMaps[2].Pz = 1.5f;

    DrawBitmap(TranState->EnvMaps[0].LOD + 0,
               &TranState->GroundBuffers[TranState->GroundBufferCount - 1].Bitmap,
               125.0f, 50.0f, 1.0f);


//    Angle = 0.0f;

    // TODO(casey): Let's add a perp operator!!!
    v2 Origin = ScreenCenter;

    real32 Angle = 0.1f*WorldMode->Time;
#if 1
    v2 Disp = {100.0f*Cos(5.0f*Angle),
               100.0f*Sin(3.0f*Angle)};
#else
    v2 Disp = {};
#endif

#if 1
    v2 XAxis = 100.0f*V2(Cos(10.0f*Angle), Sin(10.0f*Angle));
    v2 YAxis = Perp(XAxis);
#else
    v2 XAxis = {100.0f, 0};
    v2 YAxis = {0, 100.0f};
#endif
    uint32 PIndex = 0;
    real32 CAngle = 5.0f*Angle;
#if 0
    v4 Color = V4(0.5f+0.5f*Sin(CAngle),
                  0.5f+0.5f*Sin(2.9f*CAngle),
                  0.5f+0.5f*Cos(9.9f*CAngle),
                  0.5f+0.5f*Sin(10.0f*CAngle));
#else
    v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f);
#endif
    CoordinateSystem(RenderGroup, Disp + Origin - 0.5f*XAxis - 0.5f*YAxis, XAxis, YAxis,
                     Color,
                     &WorldMode->TestDiffuse,
                     &WorldMode->TestNormal,
                     TranState->EnvMaps + 2,
                     TranState->EnvMaps + 1,
                     TranState->EnvMaps + 0);
    v2 MapP = {0.0f, 0.0f};
    for(uint32 MapIndex = 0;
        MapIndex < ArrayCount(TranState->EnvMaps);
        ++MapIndex)
    {
        environment_map *Map = TranState->EnvMaps + MapIndex;
        loaded_bitmap *LOD = Map->LOD + 0;

        XAxis = 0.5f * V2((real32)LOD->Width, 0.0f);
        YAxis = 0.5f * V2(0.0f, (real32)LOD->Height);

        CoordinateSystem(RenderGroup, MapP, XAxis, YAxis, V4(1.0f, 1.0f, 1.0f, 1.0f), LOD, 0, 0, 0, 0);
        MapP += YAxis + V2(0.0f, 6.0f);
    }
#endif

    Orthographic(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, 1.0f);

    PushRectOutline(RenderGroup, DefaultFlatTransform(), V3(MouseP, 0.0f), V2(2.0f, 2.0f));

    // TODO(casey): Make sure we hoist the camera update out to a place where the renderer
    // can know about the location of the camera at the end of the frame so there isn't
    // a frame of lag in camera updating compared to the hero.
    EndSim(SimRegion, WorldMode);
    EndTemporaryMemory(SimMemory);

    if(!HeroesExist)
    {
        PlayTitleScreen(GameState, TranState);
    }

    return(Result);
}

#if 0

                        if(Global_Particles_Test)
                        {
                            for(u32 ParticleSpawnIndex = 0;
                                ParticleSpawnIndex < 3;
                                ++ParticleSpawnIndex)
                            {
                                particle *Particle = WorldMode->Particles + WorldMode->NextParticle++;
                                if(WorldMode->NextParticle >= ArrayCount(WorldMode->Particles))
                                {
                                    WorldMode->NextParticle = 0;
                                }

                                Particle->P = V3(RandomBetween(&WorldMode->EffectsEntropy, -0.05f, 0.05f), 0, 0);
                                Particle->dP = V3(RandomBetween(&WorldMode->EffectsEntropy, -0.01f, 0.01f), 7.0f*RandomBetween(&WorldMode->EffectsEntropy, 0.7f, 1.0f), 0.0f);
                                Particle->ddP = V3(0.0f, -9.8f, 0.0f);
                                Particle->Color = V4(RandomBetween(&WorldMode->EffectsEntropy, 0.75f, 1.0f),
                                    RandomBetween(&WorldMode->EffectsEntropy, 0.75f, 1.0f),
                                    RandomBetween(&WorldMode->EffectsEntropy, 0.75f, 1.0f),
                                    1.0f);
                                Particle->dColor = V4(0, 0, 0, -0.25f);

                                asset_vector MatchVector = {};
                                asset_vector WeightVector = {};
                                char Nothings[] = "NOTHINGS";
                                MatchVector.E[Tag_UnicodeCodepoint] = (r32)Nothings[RandomChoice(&WorldMode->EffectsEntropy, ArrayCount(Nothings) - 1)];
                                WeightVector.E[Tag_UnicodeCodepoint] = 1.0f;

                                Particle->BitmapID = HeroBitmaps.Head; //GetBestMatchBitmapFrom(TranState->Assets, Asset_Font,
                            //   &MatchVector, &WeightVector);

//                        Particle->BitmapID = GetRandomBitmapFrom(TranState->Assets, Asset_Font, &WorldMode->EffectsEntropy);
                            }

                            // NOTE(casey): Particle system test
                            ZeroStruct(WorldMode->ParticleCels);

                            r32 GridScale = 0.25f;
                            r32 InvGridScale = 1.0f / GridScale;
                            v3 GridOrigin = {-0.5f*GridScale*PARTICLE_CEL_DIM, 0.0f, 0.0f};
                            for(u32 ParticleIndex = 0;
                                ParticleIndex < ArrayCount(WorldMode->Particles);
                                ++ParticleIndex)
                            {
                                particle *Particle = WorldMode->Particles + ParticleIndex;

                                v3 P = InvGridScale*(Particle->P - GridOrigin);

                                s32 X = TruncateReal32ToInt32(P.x);
                                s32 Y = TruncateReal32ToInt32(P.y);

                                if(X < 0) {X = 0;}
                                if(X > (PARTICLE_CEL_DIM - 1)) {X = (PARTICLE_CEL_DIM - 1);}
                                if(Y < 0) {Y = 0;}
                                if(Y > (PARTICLE_CEL_DIM - 1)) {Y = (PARTICLE_CEL_DIM - 1);}

                                particle_cel *Cel = &WorldMode->ParticleCels[Y][X];
                                real32 Density = Particle->Color.a;
                                Cel->Density += Density;
                                Cel->VelocityTimesDensity += Density*Particle->dP;
                            }

                            if(Global_Particles_ShowGrid)
                            {
                                for(u32 Y = 0;
                                    Y < PARTICLE_CEL_DIM;
                                    ++Y)
                                {
                                    for(u32 X = 0;
                                        X < PARTICLE_CEL_DIM;
                                        ++X)
                                    {
                                        particle_cel *Cel = &WorldMode->ParticleCels[Y][X];
                                        real32 Alpha = Clamp01(0.1f*Cel->Density);
                                        PushRect(RenderGroup, EntityTransform, GridScale*V3((r32)X, (r32)Y, 0) + GridOrigin, GridScale*V2(1.0f, 1.0f),
                                            V4(Alpha, Alpha, Alpha, 1.0f));
                                    }
                                }
                            }

                            for(u32 ParticleIndex = 0;
                                ParticleIndex < ArrayCount(WorldMode->Particles);
                                ++ParticleIndex)
                            {
                                particle *Particle = WorldMode->Particles + ParticleIndex;                        

                                v3 P = InvGridScale*(Particle->P - GridOrigin);

                                s32 X = TruncateReal32ToInt32(P.x);
                                s32 Y = TruncateReal32ToInt32(P.y);

                                if(X < 1) {X = 1;}
                                if(X > (PARTICLE_CEL_DIM - 2)) {X = (PARTICLE_CEL_DIM - 2);}
                                if(Y < 1) {Y = 1;}
                                if(Y > (PARTICLE_CEL_DIM - 2)) {Y = (PARTICLE_CEL_DIM - 2);}

                                particle_cel *CelCenter = &WorldMode->ParticleCels[Y][X];
                                particle_cel *CelLeft = &WorldMode->ParticleCels[Y][X - 1];
                                particle_cel *CelRight = &WorldMode->ParticleCels[Y][X + 1];
                                particle_cel *CelDown = &WorldMode->ParticleCels[Y - 1][X];
                                particle_cel *CelUp = &WorldMode->ParticleCels[Y + 1][X];

                                v3 Dispersion = {};
                                real32 Dc = 1.0f;
                                Dispersion += Dc*(CelCenter->Density - CelLeft->Density)*V3(-1.0f, 0.0f, 0.0f);
                                Dispersion += Dc*(CelCenter->Density - CelRight->Density)*V3(1.0f, 0.0f, 0.0f);
                                Dispersion += Dc*(CelCenter->Density - CelDown->Density)*V3(0.0f, -1.0f, 0.0f);
                                Dispersion += Dc*(CelCenter->Density - CelUp->Density)*V3(0.0f, 1.0f, 0.0f);

                                v3 ddP = Particle->ddP + Dispersion;

                                // NOTE(casey): Simulate the particle forward in time
                                Particle->P += (0.5f*Square(Input->dtForFrame)*Input->dtForFrame*ddP +
                                        Input->dtForFrame*Particle->dP);
                                Particle->dP += Input->dtForFrame*ddP;
                                Particle->Color += Input->dtForFrame*Particle->dColor;

                                if(Particle->P.y < 0.0f)
                                {
                                    r32 CoefficientOfRestitution = 0.3f;
                                    r32 CoefficientOfFriction = 0.7f;
                                    Particle->P.y = -Particle->P.y;
                                    Particle->dP.y = -CoefficientOfRestitution*Particle->dP.y;
                                    Particle->dP.x = CoefficientOfFriction*Particle->dP.x;
                                }

                                // TODO(casey): Shouldn't we just clamp colors in the renderer??
                                v4 Color;
                                Color.r = Clamp01(Particle->Color.r);
                                Color.g = Clamp01(Particle->Color.g);
                                Color.b = Clamp01(Particle->Color.b);
                                Color.a = Clamp01(Particle->Color.a);

                                if(Color.a > 0.9f)
                                {
                                    Color.a = 0.9f*Clamp01MapToRange(1.0f, Color.a, 0.9f);
                                }

                                // NOTE(casey): Render the particle
                                PushBitmap(RenderGroup, EntityTransform, Particle->BitmapID, 1.0f, Particle->P, Color);
                            }
                        }
#endif