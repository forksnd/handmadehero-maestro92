Handmade Hero Day 294 - Adding the Glove

Summary:
wrote the logic for the character attacking

essentially maintaining a state machine for the character, added the states for MovementMode_AttackSwipe


Keyword:
game code, game logic



10:46
added a glove slot in the brain system

                hanmade_brain.h

                struct brain_hero
                {
                    entity *Head;
                    entity *Body;
    --------->      entity *Glove;
                };





11:15
addded a collision for the glove

                handmade_world_mode.cpp

                internal void PlayWorld(game_state *GameState, transient_state *TranState)
                {
                    ...
                    ...

                    WorldMode->NullCollision = MakeNullCollision(WorldMode);
                    WorldMode->StairCollision = MakeSimpleGroundedCollision(WorldMode,
                                                                            TileSideInMeters,
                                                                            2.0f*TileSideInMeters,
                                                                            1.1f*TileDepthInMeters);
                    WorldMode->HeroHeadCollision = MakeSimpleGroundedCollision(WorldMode, 1.0f, 0.5f, 0.5f, 0.7f);
                    WorldMode->HeroBodyCollision = WorldMode->NullCollision; // MakeSimpleGroundedCollision(WorldMode, 1.0f, 0.5f, 0.6f);
    ----------->    WorldMode->HeroGloveCollision = WorldMode->NullCollision;

                    ...
                    ...
                }


23:11
added a new movement mode 

                handmade_entity.h

                enum entity_movement_mode
                {
                    MovementMode_Planted,
                    MovementMode_Hopping,
                    MovementMode_Floating,
                    
    ----------->    MovementMode_AngleOffset,
                    MovementMode_AngleAttackSwipe,
                };





24:39
added the logic for the new movement mode when the character attacks


                internal b32 UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, transient_state *TranState,
                                     game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
                {
                    
                    ...
                    ...

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



26:43 
Casey explaining how he wants the swiping motion to work 




55:39
Casee added the Sin01(); function to simulate the motion of the sword 

                handmade_math.h

                inline r32 Sin01(r32 t)
                {
                    r32 Result = Sin(Pi32*t);
                    
                    return(Result);
                }


Q/A
1:10:44
why not have an artist animate the glove swipe motion?

becuz we dont have an animation artist. and even if we do, we dont always want to leave everything to animation because 
animation is not parametric. They dont have any logic in it.
we will have to make a trade off.

for something as crucial as glove swipe or character hopping, maybe us programmers want to have more control over it 
