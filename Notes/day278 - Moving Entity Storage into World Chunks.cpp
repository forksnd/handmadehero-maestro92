Handmade Hero Day 278 - Moving Entity Storage into World Chunks

Summary:
starting to implement the entity system mentioned in day 277
[not completely]

Keyword:
entity system


4:30
so currently the structure of our entity is like 


                struct game_mode_world
                {
                    ...
                    ...

                    // TODO(casey): Change the name to "stored entity"
                    uint32 LowEntityCount;
                    low_entity LowEntities[100000];

                    ...
                };

what we were doing is to have 


                                          Low
     _______________________             _______
    |                       |           |       |
    |                       |           |       |       
    |                       | ----->    |       |   ------>  Simulation
    |                       |           |       |
    |                       |           |       |
    |_______________________|           |_______|




so what Casey wants to try to do is to completely get rid of low entity storage 

there is only the sim region. They use the giant sparse entity concept introduced in day 277.

and then when you go write out the entities, they are stored directly on the world chunks 


9:24
so the first thing we want to do is to obliterate low entities array entirely


13:25


so the first thing we are gonna is to 
[1 << 16] is 64k


                handmade_world.h

                struct world_entity_block
                {
                    u32 EntityCount;
                    u32 LowEntityIndex[16];
                    world_entity_block *Next;
                    
                    u32 EntityDataSize;
                    u8 EntityData[1 << 16];
                };





15:45
So Casey immediately caught a problem with the world creation in which we ran out of memory for our memory arena 
                
                handmade_world.h

                struct world
                {
                    v3 ChunkDimInMeters;

                    world_entity_block *FirstFree;

                    // TODO(casey): WorldChunkHash should probably switch to pointers IF
                    // tile entity blocks continue to be stored en masse directly in the tile chunk!
                    // NOTE(casey): A the moment, this must be a power of two!
    ----------->    world_chunk ChunkHash[4096];
                 
                    memory_arena Arena;
                };

currently we just have a static array of 4096;
so Casey changed it to 

                handmade_world.h

                struct world
                {
                    v3 ChunkDimInMeters;

                    world_entity_block *FirstFree;

                    // TODO(casey): WorldChunkHash should probably switch to pointers IF
                    // tile entity blocks continue to be stored en masse directly in the tile chunk!
                    // NOTE(casey): A the moment, this must be a power of two!
    ----------->    world_chunk* ChunkHash[4096];
                 
                    memory_arena Arena;
                };



26:48
so now we come to the table of low entities 

                handmade_world_mode.h

                struct game_mode_world
                {
                    ...
                    ...

                    // TODO(casey): Change the name to "stored entity"
                    uint32 LowEntityCount;
                    low_entity LowEntities[100000];

                    ...
                    ...
                };


47:51
so Casey now wrote the new pattern


                internal low_entity * BeginLowEntity(game_mode_world *WorldMode, entity_type Type, world_position P)
                {
                    Assert(!WorldMode->CreationBufferLocked);
                    WorldMode->CreationBufferLocked = true;
                    
                    low_entity *EntityLow = &WorldMode->CreationBuffer;
                    EntityLow->Sim.StorageIndex.Value = ++WorldMode->LastUsedEntityStorageIndex;

                    // TODO(casey): Worry about this taking awhile once the entities are large (sparse clear?)
                    ZeroStruct(*EntityLow);
                    
                    EntityLow->Sim.Type = Type;
                    EntityLow->Sim.Collision = WorldMode->NullCollision;
                    EntityLow->P = P;
                    
                    return(EntityLow);
                }

                internal void PackEntityIntoChunk(world *World, low_entity *Entity)
                {
                    // TODO(casey): NotImplemented
                }

you can see that in EndEntity, we call the PackEntityIntoChunk(); function

                internal void EndEntity(game_mode_world *WorldMode, low_entity *EntityLow)
                {
                    Assert(WorldMode->CreationBufferLocked);
                    WorldMode->CreationBufferLocked = false;
                    
                    PackEntityIntoChunk(WorldMode->World, EntityLow);
                }
