Handmade Hero Day 280 - Cleaned Up Streaming Entity Simulation

Summary:
mostly just fixing bugs from day 279

changed the stored positions in entity struct to local positions relative to the worldchunk
instead of absolute positions

deleted all the #ifndef #end pairing in all of the .h files since we dont need because we are only building one translation unit

mentioned the difference between #ifndef and #pragma once 

Keyword:
code clean up, entity system




39:43
Casey showing how to delete entities in this entity system 

Casey added a new entity flag 

                handmade_entity.h

                enum entity_flags
                {
                    EntityFlag_Collides = (1 << 0),
                    EntityFlag_Moveable = (1 << 1),
    ----------->    EntityFlag_Deleted = (1 << 2),
                };

then if the EntityFlag_Deleted is set, we just dont stream it out.


40:59
so in the EndSim();, if an entity is deleted, we just dont pack it into a worldchunk

                handmade_sim_region.cpp

                internal void EndSim(sim_region *Region, game_mode_world *WorldMode)
                {
                    TIMED_FUNCTION();

                    // TODO(casey): Maybe don't take a game state here, low entities should be stored
                    // in the world??

                    world *World = WorldMode->World;
                    
                    entity *Entity = Region->Entities;
                    for(uint32 EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity)
                    {
                        if(!(Entity->Flags & EntityFlag_Deleted))
                        {
                            ...
                            ...

                            StoreEntityReference(&Entity->Head);
                            PackEntityIntoWorld(World, Entity, EntityP);
                            ...
                            ...
                        }
                    }

                    ...
                }

41:31
then we add a DeleteEntity(); function 

                handmade_sim_region.cpp

                inline void DeleteEntity(sim_region *Region, entity *Entity)
                {
                    Entity->Flags |= EntityFlag_Deleted;
                }



44:29
Casey mentioned that currently when we pack entity data 

                handmade_entity.h

                struct entity
                {
                    // NOTE(casey): These are only for the sim region
                    world_chunk *OldChunk;
    ----------->    world_position ChunkP;
                    

                    entity_id ID;
                    b32 Updatable;

                    //

                    entity_movement_mode MovementMode;
    ----------->    r32 tMovement;
    ----------->    v3 MovementFrom;
    ----------->    v3 MovementTo;

                    // TODO(casey): Generation index so we know how "up to date" this entity is.
                };


so previously in entity, we are storing lots of absolute values 


Essentially the entity no longer needs to rmeember which chunk it is in since its always in there. we just have to store the offset
inside the chunk



50:47
so in the AddEntity(); function, where we are unpacking entityes from world chunk to sim region,
we just do all the proper position conversion 

                internal void AddEntity(game_mode_world *WorldMode, sim_region *SimRegion, entity *Source, v3 ChunkDelta)
                {
                    entity_id ID = Source->ID;
                    
                    entity_hash *Entry = GetHashFromID(SimRegion, ID);
                    Assert(Entry->Ptr == 0);
                    
                    if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
                    {
                        ..
                        ...

    --------------->    Dest->ID = ID;
    --------------->    Dest->P += ChunkDelta;
    --------------->    Dest->MovementFrom += ChunkDelta;
    --------------->    Dest->MovementTo += ChunkDelta;

                        Dest->Updatable = EntityOverlapsRectangle(Dest->P, Dest->Collision->TotalVolume, SimRegion->UpdatableBounds);
                    }            
                    else
                    {
                        InvalidCodePath;
                    }
                }


51:40
we do the opposite in EndSim(); to fix the positions

                handmade_sim_region.cpp

                internal void EndSim(sim_region *Region, game_mode_world *WorldMode)
                {
                    TIMED_FUNCTION();

                    // TODO(casey): Maybe don't take a game state here, low entities should be stored
                    // in the world??

                    world *World = WorldMode->World;
                    
                    entity *Entity = Region->Entities;
                    for(uint32 EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity)
                    {
                        if(!(Entity->Flags & EntityFlag_Deleted))
                        {
                            ...
                            ...

                            Entity->P += ChunkDelta;
                            Entity->MovementFrom += ChunkDelta;
                            Entity->MovementTo += ChunkDelta;
                            StoreEntityReference(&Entity->Head);
                            PackEntityIntoWorld(World, Entity, EntityP);
                        }
                    }
                }



Q/A
1:01:55
Casey added a note about the 


                #if !defined xxxxxxxx
                #define xxxxxxx


                #endif

these are only necessarily when you are trying to build multiple translation unit 
so we dont actually need them. 


Casey proceeds to remove all #if !defined (or #ifndef, these two has no difference); from all of its .h files 




1:06:04
someone asked why not do "pragma once" instead of "#ifndef"?

since "pragma once" is slower. The reason is because is a few things.

visual studio compiler is fxxking slow, especially when their older compilers used be faster, and the newer ones 
gets slower and slower. so in 1999, Casey says he profiled build times very extensively back then, and the fastest things to do is to 
have reduntant include guards 

so when casey used to do multi file builds, hes pattern is 

                foo.h 

                #if !defined(FOO)
                HEADER 
                #define FOO 
                #endif


                foo.cpp
                #if !defined(FOO)
                #include "foo.h"
                #endif 

and Casey mentioned that he timed those, and those were faster than #pragma once, so thats what he does 


then when Casey left Rad Game Tools. His old boss asked Casey whether they still need the reduntant include 
guards in the builds, can we get rid of them. Casey says, he doesnt know, you will have to profile them.

So Casey profiled it again in 2007 ~ 2008, and #pragma once is still slower. 



so the fastest thing to do is what we are doing now, which is one translation unit, so we dont have to worry about #pragma once.

So the reason not to use #pragma once is that, if you havent actually verified #pragma once is actually faster than #ifndef on your compiler 
then #pragma once is usually slower 


