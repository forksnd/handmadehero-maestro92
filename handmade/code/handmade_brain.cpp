/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

inline void
ExecuteBrain(game_state *GameState, game_mode_world *WorldMode, game_input *Input, 
             sim_region *SimRegion, brain *Brain, r32 dt)
{
    switch(Brain->Type)
    {
        case Type_brain_hero:
        {
            brain_hero *Parts = &Brain->Hero;
            entity *Head = Parts->Head;
            entity *Body = Parts->Body;

            u32 ControllerIndex = Brain->ID.Value - ReservedBrainID_FirstHero;
            game_controller_input *Controller = GetController(Input, ControllerIndex);
            controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;
            
            v2 dSword = {};
            r32 dZ = 0.0f;
            b32 Exited = false;
            b32 DebugSpawn = false;
            
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
                    DebugSpawn = true;
                }
            }
            
            if(Head && WasPressed(Controller->Start))
            {
                entity *ClosestHero = 0;
                real32 ClosestHeroDSq = Square(10.0f); // NOTE(casey): Ten meter maximum search!
                entity *TestEntity = SimRegion->Entities;
                for(uint32 TestEntityIndex = 0;
                    TestEntityIndex < SimRegion->EntityCount;
                    ++TestEntityIndex, ++TestEntity)
                {
                    if((TestEntity->BrainID.Value != Head->BrainID.Value) &&
                       TestEntity->BrainID.Value)
                    {            
                        real32 TestDSq = LengthSq(TestEntity->P - Head->P);            
                        if(ClosestHeroDSq > TestDSq)
                        {
                            ClosestHero = TestEntity;
                            ClosestHeroDSq = TestDSq;
                        }
                    }
                }
                
                if(ClosestHero)
                {
                    brain_id OldBrainID = Head->BrainID;
                    brain_slot OldBrainSlot = Head->BrainSlot;
                    Head->BrainID = ClosestHero->BrainID;
                    Head->BrainSlot = ClosestHero->BrainSlot;
                    ClosestHero->BrainID = OldBrainID;
                    ClosestHero->BrainSlot = OldBrainSlot;
                }
            }
            
            dSword = {};
            if(Controller->ActionUp.EndedDown)
            {
                dSword = V2(0.0f, 1.0f);
            }
            if(Controller->ActionDown.EndedDown)
            {
                dSword = V2(0.0f, -1.0f);
            }
            if(Controller->ActionLeft.EndedDown)
            {
                dSword = V2(-1.0f, 0.0f);
            }
            if(Controller->ActionRight.EndedDown)
            {
                dSword = V2(1.0f, 0.0f);
            }

            if(WasPressed(Controller->Back))
            {
                Exited = true;
            }

            if(Head)
            {
                // TODO(casey): Change to using the acceleration vector
                if((dSword.x == 0.0f) && (dSword.y == 0.0f))
                {
                    // NOTE(casey): Leave FacingDirection whatever it was
                }
                else
                {
                    Head->FacingDirection = ATan2(dSword.y, dSword.x);
                }
            }

            traversable_reference Traversable;
            if(Head && GetClosestTraversable(SimRegion, Head->P, &Traversable))
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

                v3 ddP = V3(ConHero->ddP, 0);
                real32 ddPLength = LengthSq(ddP);
                if(ddPLength > 1.0f)
                {
                    ddP *= (1.0f / SquareRoot(ddPLength));
                }
                r32 MovementSpeed = 30.0f;
                r32 Drag = 8.0f;
                ddP *= MovementSpeed;

                b32 TimerIsUp = (ConHero->RecenterTimer == 0.0f);
                b32 NoPush = (LengthSq(ConHero->ddP) < 0.1f);
                r32 Cp = NoPush ? 300.0f : 25.0f;
                b32 Recenter[3] = {};
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
                        Recenter[E] = true;
                        ddP.E[E] = Cp*(ClosestP.E[E] - Head->P.E[E]) - 30.0f*Head->dP.E[E];
                    }
                    else
                    {
                        // TODO(casey): ODE here!
                        ddP.E[E] += -Drag*Head->dP.E[E];
                    }
                }

                ConHero->RecenterTimer = ClampAboveZero(ConHero->RecenterTimer - dt);
                
                Head->ddP = ddP;
            }

            if(Body)
            {
                Body->FacingDirection = Head->FacingDirection;
                Body->dP = V3(0, 0, 0);

                if(Body->MovementMode == MovementMode_Planted)
                {
                    Body->P = GetSimSpaceTraversable(Body->Occupying).P;
                    
                    if(Head)
                    {
                        r32 HeadDistance = 0.0f;
                        HeadDistance = Length(Head->P - Body->P);
                        
                        r32 MaxHeadDistance = 0.5f;
                        r32 tHeadDistance = Clamp01MapToRange(0.0f, HeadDistance, MaxHeadDistance);
                        Body->ddtBob = -20.0f*tHeadDistance;
                    }
                }

                v3 HeadDelta = {};
                if(Head)
                {
                    HeadDelta = Head->P - Body->P;
                }
                Body->FloorDisplace = (0.25f*HeadDelta).xy;
                Body->YAxis = V2(0, 1) + 0.5f*HeadDelta.xy;
            }

            if(Exited)
            {
                DeleteEntity(SimRegion, Head);
                DeleteEntity(SimRegion, Body);
                ConHero->BrainID.Value = 0;
            }
        } break;

        case Type_brain_snake:
        {
        } break;
        
        case Type_brain_familiar:
        {
            brain_familiar *Parts = &Brain->Familiar;
            entity *Head = Parts->Head;
            
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
                    if(TestEntity->BrainSlot.Type == Type_brain_hero)
                    {            
                        real32 TestDSq = LengthSq(TestEntity->P - Head->P);            
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
                Head->ddP = OneOverLength*(ClosestHero->P - Head->P);
            }
        } break;
        
        case Type_brain_floaty_thing_for_now:
        {
            // TODO(casey): Think about what this stuff actually should mean,
            // or does mean, or will mean?
            //Entity->P.z += 0.05f*Cos(Entity->tBob);
            //Entity->tBob += dt;
        } break;
        
        case Type_brain_monstar:
        {
            brain_monstar *Parts = &Brain->Monstar;
            entity *Body = Parts->Body;
            if(Body)
            {
                v3 Delta = {RandomBilateral(&WorldMode->GameEntropy),
                    RandomBilateral(&WorldMode->GameEntropy),
                    0.0f};
                traversable_reference Traversable;
                if(GetClosestTraversable(SimRegion, Body->P + Delta, &Traversable))
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
            }
        } break;
        
        InvalidDefaultCase;
    }
}