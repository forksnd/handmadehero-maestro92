Handmade Hero Day 291 - Hopping Monstar and Occupying Trees

Summary:
added hopping logic to Monstars.
Made Trees and Walls occupying Traversables. Notice that this design will also support destructable walls in the future.


Keyword:
code clean up




adding a monstar that hops as well



24:08
Casey adding a standard_room struct
                

                struct standard_room
                {
                    world_position P[17][9];
                    traversable_reference Ground[17][9];
                };


Casey also added this AddStandardRoom

                internal standard_room AddStandardRoom(game_mode_world *WorldMode, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ,
                    random_series *Series)
                {
                    standard_room Result = {};
                    
                    for(s32 OffsetY = -4; OffsetY <= 4; ++OffsetY)
                    {
                        for(s32 OffsetX = -8; OffsetX <= 8; ++OffsetX)
                        {
                            world_position P = ChunkPositionFromTilePosition(
                                WorldMode->World, AbsTileX + OffsetX, AbsTileY + OffsetY, AbsTileZ);

                 //           P.Offset_.z = 0.25f*(r32)(OffsetX + OffsetY);

                            traversable_reference StandingOn = {};
                            if((OffsetX == 2) && (OffsetY == 2))
                            {
                                entity *Entity = BeginGroundedEntity(WorldMode, WorldMode->FloorCollision);
                                StandingOn.Entity.Index = Entity->ID;
                                Entity->TraversableCount = 1;
                                Entity->Traversables[0].P = V3(0, 0, 0);
                                Entity->Traversables[0].Occupier = 0;
                                EndEntity(WorldMode, Entity, P);
                            }
                            else
                            {
                                entity *Entity = BeginGroundedEntity(WorldMode, WorldMode->FloorCollision);
                                StandingOn.Entity.Index = Entity->ID;
                                Entity->TraversableCount = 1;
                                Entity->Traversables[0].P = V3(0, 0, 0);
                                Entity->Traversables[0].Occupier = 0;
                                EndEntity(WorldMode, Entity, P);
                            }
                    
                            Result.P[OffsetX + 8][OffsetY + 4] = P;
                            Result.Ground[OffsetX + 8][OffsetY + 4] = StandingOn;
                        }
                    }

                    return(Result);
                }



30:19
Casey made all the walls occupying a Traversables


                internal void AddWall(game_mode_world *WorldMode, world_position P, traversable_reference StandingOn)
                {
                    entity *Entity = BeginGroundedEntity(WorldMode, WorldMode->WallCollision);
                    AddFlags(Entity, EntityFlag_Collides);

                    AddPiece(Entity, Asset_Tree, 2.5f, V3(0, 0, 0), V4(1, 1, 1, 1));
                    
    ----------->    Entity->Occupying = StandingOn;
                    
                    EndEntity(WorldMode, Entity, P);
                }


31:17
Casey demonstarted that now you can hopping through Trees. This is because currently, we dont have a notion of how far you can hop


32:24
one thing Casey did is to add back the collision volume on the head.
Casey does mention that hes not sure if thats the right solution. But it does work



lots of debugging


45:00 
Casey removed the move_spec struct cuz it was causing lots of trouble.



51:47
refactored the movement and dragging 


Q/A
1:00:04
Casey mentioned about the multithreaded texture downloads back then, which was supposed to work. But they dont.
Eventually we have to stop doing it, because it doesnt work. 



