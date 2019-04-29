Handmade Hero Day 271 - Hybrid Tile-based Movement

Summary:
separted the hero movement into head movement and body movement.

the head movement guides the body movement, where the body movement sort of slowly snaps to the closest 
Traversable point and come to rest (kind of like a spring system);

Keyword:
movement system

19:24 
So Casey separated the head and the body, and we are now controlling the movement of head as we press the keyboard keys 


Casey made is so that when we are simulating the Hero, it will look for the closest Traversable point from the head. 

so we go through each entity, and go though the Traversable points. 

                internal b32 UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, transient_state *TranState,
                                     game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
                {

                    case EntityType_HeroBody:
                    {
                        // TODO(casey): Make spatial queries easy for things!
                        sim_entity *Head = Entity->Head.Ptr;
                        if(Head)
                        {
                            r32 ClosestDistanceSq = Square(1000.0f);
                            v3 ClosestP = Entity->P;
                            sim_entity *TestEntity = SimRegion->Entities;
                            for(uint32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity)
                            {
                                sim_entity_collision_volume_group *VolGroup = TestEntity->Collision;
                                for(u32 PIndex = 0; PIndex < VolGroup->TraversableCount; ++PIndex)
                                {
                                    sim_entity_traversable_point P = 
                                        GetSimSpaceTraversable(TestEntity, PIndex);

                                    v3 HeadToPoint = P.P - Head->P;

                                    real32 TestDSq = LengthSq(HeadToPoint);            
                                    if(ClosestDistanceSq > TestDSq)
                                    {
                                        ClosestP = P.P;
                                        ClosestDistanceSq = TestDSq;
                                    }
                                }
                            }

                            ddP = (ClosestP - Entity->P);
                            Entity->Collision = WorldMode->NullCollision;
                            MoveSpec.UnitMaxAccelVector = true;
                            MoveSpec.Speed = 100.0f;
                            MoveSpec.Drag = 10.0f;
                        }
                    } break;

                    ...
                    ...


32:34
Casey mentioned that whats nice about this movement system is that this is not based on the grid,
so even if our world doesnt have grids, but just Traversable points, it will still work 



57:30
Casey making it so that the body animates and slowly arrives, clamp, come to rest to the Traversable points, guided by the head.


Q/A 
1:01:17
How to avoid doing dynamic memory allocations


Dynamic memory allocations isnt bad. You should make sure you know what you are actually doing.
So the question is can you use a simple freelist, like what we use. 
A general purpose allocator is usually a bunch of freelists that a bunch of additional code has to run to determine which freelists
to use when you need some memory. And then it has split things up and merge things together and do all these extra work. most times, 
are about 64 bytes long, and i usually have about 1000 of them total, so I can just use a freelist and we are done 
