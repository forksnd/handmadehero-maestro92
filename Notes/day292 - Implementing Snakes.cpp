Handmade Hero Day 292 - Implementing Snakes

Summary:
implemented a snake entity

Keyword:
gameplay code 


since this is game play code, I didnt really pay much attention.





added an AddSnakeSegment(); function


                handmade_world_mode.cpp

                internal void AddSnakeSegment(game_mode_world *WorldMode, world_position P, traversable_reference StandingOn,
                                brain_id BrainID, u32 SegmentIndex)
                {
                    entity *Entity = BeginGroundedEntity(WorldMode, WorldMode->MonstarCollision);
                    AddFlags(Entity, EntityFlag_Collides);

                    Entity->BrainSlot = IndexedBrainSlotFor(brain_snake, Segments, SegmentIndex);
                    Entity->BrainID = BrainID;
                    Entity->Occupying = StandingOn;

                    InitHitPoints(Entity, 3);

                    AddPiece(Entity, Asset_Shadow, 1.5f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                    AddPiece(Entity, SegmentIndex ? Asset_Torso : Asset_Head, 1.5f, V3(0, 0, 0), V4(1, 1, 1, 1));

                    EndEntity(WorldMode, Entity, P);
                }

                

and we call this at startup

                brain_id SnakeBrainID = AddBrain(WorldMode);
                for(u32 SegmentIndex = 0;
                    SegmentIndex < 12;
                    ++SegmentIndex)
                {
                    u32 X = 2 + SegmentIndex;
                    AddSnakeSegment(WorldMode, Room.P[X][2], Room.Ground[X][2], SnakeBrainID, SegmentIndex);
                }


33:54
added the case for Type_brain_snake:


                case Type_brain_snake:
                {
                    brain_snake *Parts = &Brain->Snake;
                    
                    entity *Head = Parts->Segments[0];
                    if(Head)
                    {
                        v3 Delta = {RandomBilateral(&WorldMode->GameEntropy),
                                    RandomBilateral(&WorldMode->GameEntropy),
                                    0.0f};
                        traversable_reference Traversable;
                        if(GetClosestTraversable(SimRegion, Head->P + Delta, &Traversable))
                        {
                            if(Head->MovementMode == MovementMode_Planted)
                            {
                                if(!IsEqual(Traversable, Head->Occupying))
                                {
                                    traversable_reference LastOccupying = Head->Occupying;
                                    Head->CameFrom = Head->Occupying;
                                    if(TransactionalOccupy(Head, &Head->Occupying, Traversable))
                                    {
                                        Head->tMovement = 0.0f;
                                        Head->MovementMode = MovementMode_Hopping;
                                        
                                        for(u32 SegmentIndex = 1;
                                            SegmentIndex < ArrayCount(Parts->Segments);
                                            ++SegmentIndex)
                                        {
                                            entity *Segment = Parts->Segments[SegmentIndex];
                                            if(Segment)
                                            {
                                                Segment->CameFrom  = Segment->Occupying;
                                                TransactionalOccupy(Segment, &Segment->Occupying, LastOccupying);
                                                LastOccupying = Segment->CameFrom;
                                                
                                                Segment->tMovement = 0.0f;
                                                Segment->MovementMode = MovementMode_Hopping;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } break;
                