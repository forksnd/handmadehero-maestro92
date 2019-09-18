/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

inline move_spec
DefaultMoveSpec(void)
{
    move_spec Result;

    Result.UnitMaxAccelVector = false;
    Result.Speed = 1.0f;
    Result.Drag = 0.0f;

    return(Result);
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

inline s32
ConvertToLayerRelative(game_mode_world *WorldMode, r32 *Z)
{
    s32 RelativeIndex = 0;
    RecanonicalizeCoord(WorldMode->TypicalFloorHeight, &RelativeIndex, Z);
    return(RelativeIndex);
}

internal void
UpdateAndRenderEntities(game_mode_world *WorldMode, sim_region *SimRegion, render_group *RenderGroup, v3 CameraP,
                        loaded_bitmap *DrawBuffer, v4 BackgroundColor, r32 dt, transient_state *TranState, v2 MouseP)
{
    TIMED_FUNCTION();

    real32 FadeTopEndZ = 0.75f*WorldMode->TypicalFloorHeight;
    real32 FadeTopStartZ = 0.5f*WorldMode->TypicalFloorHeight;
    real32 FadeBottomStartZ = -1.0f*WorldMode->TypicalFloorHeight;
    real32 FadeBottomEndZ = -4.0f*WorldMode->TypicalFloorHeight;

#define MinimumLevelIndex -4
#define MaximumLevelIndex 1
    u32 ClipRectIndex[(MaximumLevelIndex - MinimumLevelIndex + 1)];
    for(u32 LevelIndex = 0;
        LevelIndex < ArrayCount(ClipRectIndex);
        ++LevelIndex)
    {
        // TODO(casey): Probably indicates we want to separate update and render
        // for entities sometime soon?
        s32 RelativeLayerIndex = MinimumLevelIndex + LevelIndex;
        r32 CameraRelativeGroundZ = SimRegion->Origin.Offset_.z + (r32)RelativeLayerIndex*WorldMode->TypicalFloorHeight;
        
        clip_rect_fx FX = {};
        if(CameraRelativeGroundZ > FadeTopStartZ)
        {
            RenderGroup->CurrentClipRectIndex = ClipRectIndex[0];

            r32 t = Clamp01MapToRange(FadeTopStartZ, CameraRelativeGroundZ, FadeTopEndZ);
            FX.tColor = V4(0, 0, 0, t);
        }
        else if(CameraRelativeGroundZ < FadeBottomStartZ)
        {
            RenderGroup->CurrentClipRectIndex = ClipRectIndex[1];

            r32 t = Clamp01MapToRange(FadeBottomStartZ, CameraRelativeGroundZ, FadeBottomEndZ);
            FX.tColor = V4(t, t, t, 0.0f);
            FX.Color = BackgroundColor;
        }
        else
        {   
            RenderGroup->CurrentClipRectIndex = ClipRectIndex[2];
        }
        
        ClipRectIndex[LevelIndex] = PushClipRect(RenderGroup, 0, 0, DrawBuffer->Width, DrawBuffer->Height, FX);
    }

    transient_clip_rect Rect(RenderGroup);
    for(uint32 EntityIndex = 0;
        EntityIndex < SimRegion->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = SimRegion->Entities + EntityIndex;
        
        // TODO(casey): We don't really have a way to unique-ify these :(
        debug_id EntityDebugID = DEBUG_POINTER_ID((void *)Entity->ID.Value);
        if(DEBUG_REQUESTED(EntityDebugID))
        {
            DEBUG_BEGIN_DATA_BLOCK("Simulation/Entity");
        }

        if(Entity->Updatable)
        {

            //
            // NOTE(casey): "Physics"
            //

            BEGIN_BLOCK("EntityPhysics");
            
            switch(Entity->MovementMode)
            {
                case MovementMode_Planted:
                {
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
                        Entity->ddtBob = 30.0f;
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

                case MovementMode_AngleAttackSwipe:
                {
                    if(Entity->tMovement < 1.0f)
                    {
                        Entity->AngleCurrent = Lerp(Entity->AngleStart, 
                                                    Entity->tMovement,
                                                    Entity->AngleTarget);

                        Entity->AngleCurrentDistance = Lerp(Entity->AngleBaseDistance, 
                                                            Triangle01(Entity->tMovement),
                                                            Entity->AngleSwipeDistance);
                    }
                    else
                    {
                        Entity->MovementMode = MovementMode_AngleOffset;
                        Entity->AngleCurrent = Entity->AngleTarget;
                        Entity->AngleCurrentDistance = Entity->AngleBaseDistance;
                    }

                    Entity->tMovement += 10.0f*dt;
                    if(Entity->tMovement > 1.0f)
                    {
                        Entity->tMovement = 1.0f;
                    }
                }
                case MovementMode_AngleOffset:
                {
                    v2 Arm = Entity->AngleCurrentDistance*Arm2(Entity->AngleCurrent + Entity->FacingDirection);
                    Entity->P = Entity->AngleBase + V3(Arm.x, Arm.y + 0.5f, 0.0f);
                } break;
            }

            r32 Cp = 100.0f;
            r32 Cv = 10.0f;
            Entity->ddtBob += Cp*(0.0f - Entity->tBob) + Cv*(0.0f - Entity->dtBob);
            Entity->tBob += Entity->ddtBob*dt*dt + Entity->dtBob*dt;
            Entity->dtBob += Entity->ddtBob*dt;

            if((LengthSq(Entity->dP) > 0) || (LengthSq(Entity->ddP) > 0))
            {
                MoveEntity(WorldMode, SimRegion, Entity, dt, Entity->ddP);
            }

            END_BLOCK();

            object_transform EntityTransform = DefaultUprightTransform();
            EntityTransform.OffsetP = GetEntityGroundPoint(Entity) - CameraP;
            s32 RelativeLayer = ConvertToLayerRelative(WorldMode, &EntityTransform.OffsetP.z);
            
            if((RelativeLayer >= MinimumLevelIndex) &&
               (RelativeLayer <= MaximumLevelIndex))
            {
                RenderGroup->CurrentClipRectIndex = ClipRectIndex[RelativeLayer - MinimumLevelIndex];
                
                //
                // NOTE(casey): Rendering
                //            
                asset_vector MatchVector = {};
                MatchVector.E[Tag_FacingDirection] = Entity->FacingDirection;
                asset_vector WeightVector = {};
                WeightVector.E[Tag_FacingDirection] = 1.0f;

                for(u32 PieceIndex = 0;
                    PieceIndex < Entity->PieceCount;
                    ++PieceIndex)
                {
                    entity_visible_piece *Piece = Entity->Pieces + PieceIndex;
                    bitmap_id BitmapID = 
                        GetBestMatchBitmapFrom(TranState->Assets, 
                                               Piece->AssetType, &MatchVector, &WeightVector);

                    v2 XAxis = {1, 0};
                    v2 YAxis = {0, 1};
                    if(Piece->Flags & PieceMove_AxesDeform)
                    {
                        XAxis = Entity->XAxis;
                        YAxis = Entity->YAxis;
                    }

                    r32 tBob = 0.0f;
                    v3 Offset = {};
                    if(Piece->Flags & PieceMove_BobOffset)
                    {
                        tBob = Entity->tBob;
                        Offset = V3(Entity->FloorDisplace, 0.0f);
                        Offset.y += Entity->tBob;
                    }

                    PushBitmap(RenderGroup, EntityTransform, BitmapID, Piece->Height, Offset + Piece->Offset, Piece->Color, 1.0f, XAxis, YAxis);
                }

                DrawHitpoints(Entity, RenderGroup, EntityTransform);

                EntityTransform.Upright = false;
                for(uint32 VolumeIndex = 0;
                    VolumeIndex < Entity->Collision->VolumeCount;
                    ++VolumeIndex)
                {
                    entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        
                    PushRectOutline(RenderGroup, EntityTransform, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, V4(0, 0.5f, 1.0f, 1));
                }

                BEGIN_BLOCK("TraversableRendering");
                for(uint32 TraversableIndex = 0;
                    TraversableIndex < Entity->TraversableCount;
                    ++TraversableIndex)
                {
                    entity_traversable_point *Traversable = 
                        Entity->Traversables + TraversableIndex;                        
                    PushRect(RenderGroup, EntityTransform, Traversable->P, V2(1.2f, 1.2f), 
                             Traversable->Occupier ? V4(1.0, 0.5f, 0.0f, 1) : V4(0.05f, 0.25f, 0.05f, 1));
                    PushRectOutline(RenderGroup, EntityTransform, Traversable->P, V2(1.2f, 1.2f),
                                    V4(0, 0, 0, 1));
                }
                END_BLOCK();

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
            }

            if(DEBUG_REQUESTED(EntityDebugID))
            {
                DEBUG_VALUE(Entity->ID.Value);
                DEBUG_VALUE(Entity->Updatable);
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
    END_BLOCK();
}