Handmade Hero Day 279 - Finishing World Chunk Entity Storage

Summary:
following up on day 278. made it so that we delete world chunks on demand when we call BeginSim();
and we recreate them when we call EndSim(); to pack entities back to world chunks

[note that this wont be slow, cuz we are just using world chunks from the free list]

Keyword:
entity system




1:30
Casey coming back to discuss the comparison between the OOP Component approach vs the ECS system approach 


    Component                               Act/React 
    "AOS"                                   "SOA"

------------------------------------------------------
                            |
                            |       
(what we do in dominations) |   Entity Component system (game2);
                            |
                            |
                        

Casey does mention that a lot of times if certain things require simulation, you can have a hybrid of these two.
We did this in prototype, where physics is SOA style. 


6:54
so recall in yesterday_s episode, in BeginLowEntity function, we are putting things in a temporary buffer

                Handmade_world_mode.h

                struct game_mode_world
                {
                    ...
                    ...


                    b32 CreationBufferLocked; // TODO(casey): Remove this eventually, just for catching bugs?
                    low_entity CreationBuffer;
    ----------->    u32 LastUsedEntityStorageIndex; // TODO(casey): Worry about this wrapping - free list for IDs?
                    
                    particle_cel ParticleCels[PARTICLE_CEL_DIM][PARTICLE_CEL_DIM];
                };


7:55
handmade_sim_region.cpp 

so recall in the BeginSim function, we call AddEntity from the chunks 

Casey mentioned that this sim is meant to be a stream in stream out system. 
the goal is to pull everything out of the world. We are not even gonna leave anything in the world chunks. 
also we arent even gonna leave a backing store 

we gonna pull everything out of world chunks, sim it, put things back in. 

                handmade_sim_region.cpp

                internal sim_region * BeginSim(memory_arena *SimArena, game_mode_world *WorldMode, world *World, world_position Origin, rectangle3 Bounds, real32 dt)
                {
                    ...

                    for(int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ)
                    {
                        for(int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
                        {
                            for(int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
                            {
                                world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, ChunkZ);
                                if(Chunk)
                                {
                                    for(world_entity_block *Block = &Chunk->FirstBlock; Block; Block = Block->Next)
                                    {
                                        for(uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex)
                                        {                        
                                            low_entity *Low = (low_entity *)Block->EntityData + EntityIndex;
                                            if(!IsSet(&Low->Sim, EntityFlag_Nonspatial))
                                            {
                                                v3 SimSpaceP = GetSimSpaceP(SimRegion, Low);
                                                if(EntityOverlapsRectangle(SimSpaceP, Low->Sim.Collision->TotalVolume, SimRegion->Bounds))
                                                {
                                                    AddEntity(WorldMode, SimRegion, Low->Sim.StorageIndex, Low, &SimSpaceP);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    return(SimRegion);
                }


12:01
Casey introducing the concept of "Transactional Memory".
its a simple concept dressed up as a very facy term.
from wikipedia: https://en.wikipedia.org/wiki/Software_transactional_memory

we see that Transactional memory is a concurrency control mechanism analogous to database transactions for controlling 
access to shared memory in concurrent computing



If i want to do something multithreaded, lets say we have thread A and thread B doing something on the Target 


       Target
     ___________
    |           |       thread A 
    |           |
    |           |
    |           |       thread B
    |           |
    |___________|

if thread B starts before thread A finishes, then we would have a corrupt state for Target.




so as you can see, what we are doing here is that we are completely getting rid of the world chunks 
when we are done with them 

                internal sim_region * BeginSim(memory_arena *SimArena, game_mode_world *WorldMode, world *World, world_position Origin, rectangle3 Bounds, real32 dt)
                {

                    for(int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ)
                    {
                        for(int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
                        {
                            for(int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
                            {
                                world_chunk *Chunk = RemoveWorldChunk(World, ChunkX, ChunkY, ChunkZ);
                                if(Chunk)
                                {
                                    world_entity_block *Block = Chunk->FirstBlock;
                                    while(Block)
                                    {
                                        for(uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex)
                                        {                        
                                            entity *Low = (entity *)Block->EntityData + EntityIndex;
                                            if(!IsSet(Low, EntityFlag_Nonspatial))
                                            {
                                                v3 SimSpaceP = GetSimSpaceP(SimRegion, Low);
                                                if(EntityOverlapsRectangle(SimSpaceP, Low->Collision->TotalVolume, SimRegion->Bounds))
                                                {
                                                    AddEntity(WorldMode, SimRegion, Low->StorageIndex, Low, &SimSpaceP);
                                                }
                                            }
                                        }
                                        
                                        world_entity_block *NextBlock = Block->Next;
                                        AddBlockToFreeList(World, Block);
                                        Block = NextBlock;
                                    }
                                    
        ------------------------>   AddChunkToFreeList(World, Chunk);
                                }
                            }
                        }
                    }
                    
                    return(SimRegion);
                }


then when EndSim();, we recreat them 

                handmade_sim_region.cpp

                internal void EndSim(sim_region *Region, game_mode_world *WorldMode)
                {
                    world *World = WorldMode->World;
                    
                    entity *Entity = Region->Entities;
                    for(uint32 EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity)
                    {
                        world_position ChunkP = MapIntoChunkSpace(World, Region->Origin, Entity->P);
                        StoreEntityReference(&Entity->Head);

                        ...
                        ...
                        
                        PackEntityIntoWorld(World, Entity, ChunkP);
                    }
                }


42:26
Casey now writing the PackEntityIntoWorld(); function


-   so we first check if we have enough room to store EntityData 
    if we have exceeded, we need to make a new chunk.

-   then we copy the Block->EntityData into world_entity_block.EntityData.


-   full code below:

                handmade_world.cpp

                internal void PackEntityIntoChunk(world *World, entity *Source, world_chunk *Chunk)
                {
                    u32 PackSize = sizeof(Source);

                    if(!Chunk->FirstBlock || !HasRoomFor(Chunk->FirstBlock, PackSize))
                    {
                        if(!World->FirstFreeBlock)
                        {
                            World->FirstFreeBlock = PushStruct(&World->Arena, world_entity_block);
                            World->FirstFreeBlock->Next = 0;
                        }
                        
                        Chunk->FirstBlock = World->FirstFreeBlock;
                        World->FirstFreeBlock = Chunk->FirstBlock->Next;

                        ClearWorldEntityBlock(Chunk->FirstBlock);
                    }
                    
                    world_entity_block *Block = Chunk->FirstBlock;

                    Assert(HasRoomFor(Block, PackSize));
                    u8 *Dest = (Block->EntityData + Block->EntityDataSize);
                    Block->EntityDataSize += PackSize;
                    
                    *(entity *)Dest = *Source;
                }

                internal void PackEntityIntoWorld(world *World, entity *Source, world_position At)
                {
                    world_chunk *Chunk = GetWorldChunk(World, At.ChunkX, At.ChunkY, At.ChunkZ, &World->Arena);
                    PackEntityIntoChunk(World, Source, Chunk);
                }


58:26
when writing the function for AddBlockToFreeList(); and AddChunkToFreeList();
Casey mentioned that if you have metaprogramming in the language you are using, these are trivial.

