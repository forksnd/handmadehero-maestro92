Handmade Hero Day 270 - Making Traversable Points

Summary:
discussed for the movement system, Casey will now make every grid cell as an entity,
he accepts the trade off of more entities, and more processing time for entities for flexiblility
of units interacting and reacting with each other 

wants to add the concept of grid cell elevations 

changed the movement system in the game from free movement within a room to traversables points on grid cells

Keyword:
movement system, Collision detection


5:22
Casey got rid of the ground chunks. Ground chunks are no longer a thing 

5:35
Casey mentioned that we probably wont be rendering in the background task anymore,
so removing ground chunks allows us to do that. 


8:58
Casey describing that he wants to add the concept of grid squares into the game. 


10:31
Casey made the architecture decision that we will make an entity for each grid cell, and that entity 
will have information about that tile. 

this approach
Pros:
makes it more flexible to have entities interact and react with each other. Everything will be simulated,
including the ground. 

Cons:
more entities than otherwise, as grid tiles can be represented easier. More memory, possibly more time spent
in simulating all entities. 

so Casey wants to the flexiblility, so hes just gonna accept the inefficiency that grid cells as entities brings.
we want everything to work everywhere, essentially



15:15
so for our movement system, so your dude is always in one cell or another, it cant be in between cells 


so Casey changed the entity type of EntityType_Space to EntityType_Floor

                handmade_sim_region.h

                enum entity_type
                {
                    EntityType_Null,
                    
                    EntityType_Space,
                    
                    EntityType_Hero,
                    EntityType_Wall,
                    EntityType_Familiar,
                    EntityType_Monstar,
                    EntityType_Sword,
                    EntityType_Stairwell,
                };


now we have:
Casey does mention that EntityType_Floor is probably not a good name, cuz sometimes you would have grass or stones
which are also standable

                enum entity_type
                {
                    EntityType_Null,
                    
                    EntityType_Hero,
                    EntityType_Wall,
    ----------->    EntityType_Floor,
                    EntityType_Familiar,
                    EntityType_Monstar,
                    EntityType_Sword,
                    EntityType_Stairwell,
                };


18:05
then in the AddStarndardRoom function, we just go through a for loop and add a bunch of floor entities 

                internal void AddStandardRoom(game_mode_world *WorldMode, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
                {
                    for(s32 OffsetY = -4; OffsetY <= 4; ++OffsetY)
                    {
                        for(s32 OffsetX = -8; OffsetX <= 8; ++OffsetX)
                        {
                            world_position P = ChunkPositionFromTilePosition(
                                WorldMode->World, AbsTileX + OffsetX, AbsTileY + OffsetY, AbsTileZ);
                            add_low_entity_result Entity = AddGroundedEntity(WorldMode, EntityType_Floor, P,
                                WorldMode->FloorCollision);
                        }
                    }
                }

we creating a centered room, hence the OffsetX and OffsetY 
            
            OffsetX = -4                Offset = 4
                 _____________________________
                |                             |
                |                             |
                |                             |
                |                             |
                |                             |
                |                             |
                |                             |
                |_____________________________|
                



26:12
Casey wants to add the concept of elevation for each ground cells, for instance if the ground cell next to you is 
too high, and you cant go to it. 


29:22
Casey also made the game design decision of traversables entities having specific points that characters can stand on.

Traversable entities will have points that characters can stand on, and when you are moving, you go to that point,
and when you want to go to the next one, it looks to see in that direction, if there is one for you to go to.

so characters arent moving through empty space, but more about moving between points. So for each entity,
there are only a few specific points that you can actually stand on

[you can think of this like civ or sid meiers pirates siege mode, 
where units go from hex cell to hex cell, instead of a freely miving world,
or kind of like chess pieces on a chess board]

31:32
so in the sim_entity_collision_volume_group, we added the concept of traversables points 

                handmade_sim_region.h

                struct sim_entity_traversable_point
                {
                    v3 P;
                };

                struct sim_entity_collision_volume_group
                {
                    sim_entity_collision_volume TotalVolume;

                    // TODO(casey): VolumeCount is always expected to be greater than 0 if the entity
                    // has any volume... in the future, this could be compressed if necessary to say
                    // that the VolumeCount can be 0 if the TotalVolume should be used as the only
                    // collision volume for the entity.
                    u32 VolumeCount;
                    sim_entity_collision_volume *Volumes; 
                    
                    u32 TraversableCount;
    ----------->    sim_entity_traversable_point *Traversables;
                };



Q/A
59:00
someone ask how will stairwells work?

you will still be able to go up or down, but you cant change direction midway



1:01:54
Casey explaining his decision to go with gridcells as entities

since computers nowadays are fast, and we arent doing a 3D game, just a 2D one, we will use the CPU that we got 
on entities simulation.

