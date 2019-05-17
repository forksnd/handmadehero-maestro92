Handmade Hero Day 286 - Starting to Decouple Entity Behavior

Summary:
pointed out that we want to move away from entityType specific logic 
added an array of entity references to struct entity.

add code to handle the array of entity reference when packing and unpacking entities

Keyword:
entity system 


4:05
right now what you see us doing is that we basicallly special case everything by the entity type.
this design is not a bad thing. There are plenty of games whose entity system works exactly that way. 
So thats not a problem. 

if you have a game where each entity is so unique, and its very different from every other entity, thats
totaly the resonable solution.

in this game, we want more of a continuous feel.

6:30
So Casey now wants to examine the code and see what do we need to do to make the code more 
entityType agnoistic. meaning we want to remove code that is "if entityType == xxx". 

this way when we want to add behavior for other kinds of entities, adding them would be trivial.

9:27
right now all of our code is 

				if (entityType == xxxx)
				{
					logic
				}
				else if (entityType == yyyy)
				{
                    logic
				}

and that is very C++, object oriented like code. This entityType field is like saying 
this entity is a particular instance of a class of entity, which prevents us from making generic, 
shared logic code 



19:45
Casey wants to organize the code where one entity is paired with another, and the entity has a certain
behaviour towards that other entity to whom I am paired.

for example currently in our entity struct we have 

                handmade_entity.h

                struct entity
                {

                    ...
                    ...
                    entity_reference Head;

                };


Casey also mentioned that we are gonna now start to actually finalize the proper entity struct 
so Casey divided the entity struct into "finalized" code and "prototype" code

as you can see, Casey added that one line in the middle 

                handmade_entity.h

                struct entity
                {
                    entity_id ID;
                    b32 Updatable;

                    //
                    // NOTE(casey):
                    //
                    
                    
                    //
    ----------->    // NOTE(casey): Everything below here is NOT worked out yet
                    //

                    entity_type Type;
                    u32 Flags;

                    v3 P;
                    v3 dP;

                    r32 DistanceLimit;

                    entity_collision_volume_group *Collision;

                    r32 FacingDirection;
                    r32 tBob;
                    r32 dtBob;
                  
                    ...
                    ...
                };





21:29
for the structure of pairing, Casey introduced a notion of paired entities 
also Casey mentioned that we want to be able to be paired to multiple entities,
so he made it into an array

                handmade_entity.h

                struct entity
                {
                    entity_id ID;
                    b32 Updatable;

                    //
                    // NOTE(casey):
                    //

    ----------->    u32 PairedEntityCount;
                    entity_reference *PairedEntities;

                    ...
                    ...
                };


37:41
Casey encounter a problem of how to pack and unpack 

                u32 PairedEntityCount;
                entity_reference *PairedEntities;

in the entity struct


we either pack and unpack the array properly, using the only amount of space that is necessary
for example if PairedEntities is [6], then we just give it that much memory

or we can get around it by just doing 

                u32 PairedEntityCount;
                entity_reference *PairedEntities[32];

but this will be too bloated. the amount of data throughput may be too much.
so we are going to only pay for the storage that we need 
so we are packing just the number of entities that is there.


38:35
the tricker thing here is that, what is our scheme to grow these arrays over time.
commonly people do resize the array by double the size. 

this is essentially a dynamically growing array. 


for the PairedEntities list, its unlikely you will have too many of those all that often, so what 
we will do is to just increase blocks of 4.


[the packing and unpacking is just serialization and deserializations]

45:36
Now Casey is debating how do we actually orgianize the entity_reference code.
so for the, we need to atleast keep track of Index. 

                handmade_entity.h

                union entity_reference
                {
                    entity *Ptr;
                    entity_id Index;
                }

but if we have the entity* Ptr, it can be handy





48:00
Casey reorganized it into 

                handmade_entity.h

                enum entity_relationship
                {
                    Relationship_None,
                    
                    Relationship_Paired,
                };
                struct stored_entity_reference
                {
                    entity_id Index;
                    entity_relationship Relationship;
                };
                struct entity_reference
                {
                    entity *Ptr;
                    stored_entity_reference Stored;
                };



52:24
Casey added the entity reference related functions 

                handmade_entity.h

                inline b32 ReferenceIsValid(entity_reference A)
                {
                    b32 Result = (A.Stored.Index.Value != 0); 
                    return(Result);
                }
                inline b32 ReferencesAreEqual(entity_reference A, entity_reference B)
                {
                    b32 Result = ((A.Ptr == B.Ptr) &&
                                  (A.Stored.Index.Value == B.Stored.Index.Value) &&
                                  (A.Stored.Relationship == B.Stored.Relationship));
                    return(Result);
                }

57:20
now in the PackEntityIntoChunk(); function, Casey handle packing the entity_reference array 
                
first we modify the packSize to handle the size of the PairedEntityCount

also we added the PackEntityReferenceArray(); function

                handmade_world.cpp 

                internal void PackEntityIntoChunk(world *World, entity *Source, world_chunk *Chunk)
                {
    ----------->    u32 PackSize = (sizeof(*Source) + Source->PairedEntityCount*sizeof(stored_entity_reference));

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
                    ++Block->EntityCount;

                    entity *DestE = (entity *)Dest;
                    *DestE = *Source;
                    PackTraversableReference(&DestE->Occupying);
                    PackTraversableReference(&DestE->CameFrom);
    ----------->    PackEntityReferenceArray(Source->PairedEntityCount, Source->PairedEntities, 
                                             (stored_entity_reference *)Source + 1);
                }




59:18
we look at how this function works:

                handmade_world.cpp

                inline void PackEntityReferenceArray(u32 Count, entity_reference *Source, stored_entity_reference *Dest)
                {
                    for(u32 Index = 0; Index < Count; ++Index)
                    {
                        Dest->Index.Value = 0;
                        Dest->Relationship = Relationship_None;
                        
                        if(Source->Ptr == 0)
                        {
                            // TODO(casey): Need the hash table to check if we should keep this!
                        }
                        else
                        {
                            Dest->Index = Source->Ptr->ID;
                            Dest->Relationship = Source->Stored.Relationship;
                        }
                        
                        ++Dest;
                        ++Source;
                    }
                }

Q/A
1:09:02
Someone mentioned Scott Meyers. Someone said that Scott Meyers you want to store data of the same type together 
so they end up on the same cache line.

Casey immediately said, dont listen to a Scott Meyers talk.
THen Casey said that statement is false, because it depends on how you are processing the data. you want to keep data 
that is accessed together, right next to each other in memory.
if you are accessing A, B and C together and they are different types, you still want them to be together.


