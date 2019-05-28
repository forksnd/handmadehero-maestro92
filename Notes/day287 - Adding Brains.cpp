Handmade Hero Day 287 - Adding Brains

Summary:
reverting the work done on day 286 and the relationship structs 

mentions that he wants to try this new architecture he thought of for the entity system of handmade hero 
which is to add controller to each entity. 

starting to implement this entity system 

renamed controllers for entities as "brains"

Keyword:
entity system 


4:14 
Casey mentioned that yesterday we made a design where entities are paired with each other
he doesnt know if that is the right design where entities know who they are paired with
so its a mutual direction thing

he is concerned about this because he doesnt know if that is the natural way to write the code.

for example, currently we have the pairing between the Head and the body 

                _____
               |#####|
               |#####|  <----  Head 
               _______
              |       |
              |       | <---- Body
              |_______|

and we had to spread the code into two different things: a specific logic for head,
and a specific logic for body

what we really want is a head-body controller, where the code simulates both things,
and if the head or the body isnt there, we just have an if statement checking if the body is there or not

In the current structure, we are forcing every entity to simulate itself, and this is similar to the OOP path,
where we are writing code specific to an entities, whereas we should really write the logic across the entities

6:00
so what Casey rather do is to step back from this OOP stupdity, and go into again on the algorithm to get this entity
system to work across objects

and Casey thought of a way to accomplish it.

8:43
Casey mentions the idea of having "controller templates"
what Casey meant by templates is not the C++ templates, but more of an idea of what a controller can be

once example of a controller template could be the Hero template, which can have a slot for the Body and 
another slot for the Head 

    Head 
        Body 
        Head 


any entity in our system, when it is packed into the chunk, would have a "controller tag"
what that would have is a "template type" and an "instance ID"

    Controller Tag
        tempate type
        instance Id

furthermore, we add one more thing in here, which is the slot  
(we could store the slot information as index, depending our implementation);

    Controller Tag
        tempate type
        instance Id
        [slot --> index]

the template types are all the different types of controller that we might have
for example: the hero controller, the snake controller.


10:10
they also get an instance ID, which tells what group is this actually associated with 
for example, if I create a Hero, each of the pieces of that hero would have a template type of a hero controller
then the instance id is an id that all of them share 

when they get unpacked, we have a separate hash table for the instance ID. we look up into 
this hash table to see if we already have a controller at that instance id. if we do insert that entity 
into that corresponding slot 
[didnt quite understand this part]


11:17
the idea of this approach is that anytime we can just take any entity and make it into its own thing 
by assigning it a new template type and a new instance id, 




12:00
this would even work in ridiculous scenarios. If I chop off someguy_s arm off, and now 
I am carry this creature_s arm around with me. If I walk off screen and drop the arm somewhere.
then the creature walks to fetch his arm, this structure will work fine.

This struture can do all the basic require things in an entity system, and its also entirely on demand
so it can be broken and streamed apart. There is no deletion or maintenance. It is all stored per entity 
and assembled at sim time. So there is no overhead of doing it. You dont have to keep things live or worry about 
scenarios like if it thinks there is head, but there actually isnt, Because they are all assembled each time
you do a sim. 



14:10
Casey mentioned that we want to revert yesterday_s changes. We wont need array storage of entity references 

18:23
Casey adding the concept of entity_controller_type, essentially the template type

                handmade_entity.h

                enum brain_type
                {
                    Brain_Hero,
                    Brain_Snake,
                    
                    Brain_Count,
                };
                                

then we add the controller related variables to entity 

the brain_type is the type of controller template as mentioned above

                handmade_entity.h

                struct entity
                {
                    ...
                    ...

                    brain_type BrainType;
                    brain_slot BrainSlot;
                    brain_id BrainID;

                    ...
                };

and Casey also added the struct for brain_slot and brain_id 

                enum brain_type
                {
                    Brain_Hero,
                    Brain_Snake,
                    
                    Brain_Count,
                };

                struct brain_slot 
                {
                    u32 Index;
                };
                
                struct brain_id
                {
                    u32 Value;
                };


so these three
                    brain_type BrainType;
                    brain_slot BrainSlot;
                    brain_id BrainID;

are whats necessary to assemble the meta entities



22:17
Casey also mentions that this technically replaces the notion of "entity_type".
because now, the entityType will never determine any logic for the entity. M<eaning instead of the entityType
determining who your controller is and how you are drawn, each one of these operations will be determined by other things. 

it just has whatever it is controlling it self at that time. Because with our new design, thats the only thing that is unique about 
this entity. 

Only one thing controller allowed to control that entity, its either the player controller, or one of those brains.
so we are never gonna say something is a hero. We are just gonna say an entity has a hero controller on it. 




24:27
so our game will likely have lots of pairing information. For example: if Im attacking, i have to remember who my target is. 
so that is when we use entity references. 

recall in day 286 we had the following:

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

in which we wrote the serve the purpose of entity pairings.

Now with this aggregate controller design, it replacese the entity_relationship.




24:48
so now Casey adds a controller hash in the sim_region, also Casey added a brain_hash struct 


                struct entity_hash
                {
                    entity *Ptr;
                    entity_id Index; // TODO(casey): Why are we storing these in the hash??
                };

                struct brain
                {
                    brain_id ID;
                    brain_type Type;
                };
    ------->    struct brain_hash
                {
                    brain *Ptr;
                    brain_id ID; // TODO(casey): Why are we storing these in the hash??
                };

                struct sim_region
                {
                    // TODO(casey): Need a hash table here to map stored entity indices
                    // to sim entities!
                    
                    world *World;
                    r32 MaxEntityRadius;
                    r32 MaxEntityVelocity;

                    world_position Origin;
                    rectangle3 Bounds;
                    rectangle3 UpdatableBounds;
                    
                    u32 MaxEntityCount;
                    u32 EntityCount;
                    entity *Entities;
                    
                    u32 MaxBrainCount;
                    u32 BrainCount;
                    brain *Brains;
                    
                    // TODO(casey): Do I really want a hash for this??
                    // NOTE(casey): Must be a power of two!
                    entity_hash EntityHash[4096];
    ----------->    brain_hash BrainHash[256];
                };




42:20
in the AddPlayer(); function, we give the Brain entity a controller 

as you can see here, we have both an Body entity and a Head entity.
we set both the Body entity and Head entity BrainType to be Brain_Hero, meaning
we are giving both of these entities a Brain_Hero Controller.

for now the controller remembers the index of the entity (which we will change later);
so just for now the Body entity is index 0 and then the Head entity is index 1.

the entity also remembers the ControllerId (BrainId);


                internal brain_id AddPlayer(game_mode_world *WorldMode, sim_region *SimRegion, traversable_reference StandingOn)
                {
                    world_position P = MapIntoChunkSpace(SimRegion->World, SimRegion->Origin, 
                                                         GetSimSpaceTraversable(StandingOn).P);
                    ...
                    ...

                    entity *Body = BeginGroundedEntity(WorldMode, EntityType_HeroBody,
                        WorldMode->HeroBodyCollision);
                    AddFlags(Body, EntityFlag_Collides|EntityFlag_Moveable);

                    entity *Head = BeginGroundedEntity(WorldMode, EntityType_HeroHead,
                        WorldMode->HeroHeadCollision);
                    AddFlags(Head, EntityFlag_Collides|EntityFlag_Moveable);
                    
                    ...
                    ...

                    // TODO(casey): We will probably need a creation-time system for
                    // guaranteeing now overlapping occupation.
                    Body->Occupying = StandingOn;
                    
    ----------->    brain_id BrainID = AddBrain(WorldMode);
                    Body->BrainType = Brain_Hero;
                    Body->BrainSlot.Index = 0;
                    Body->BrainID = BrainID;
                    Head->BrainType = Brain_Hero;
                    Head->BrainSlot.Index = 1;
                    Head->BrainID = BrainID;

                    ...
                    ...

                    return(BrainID);
                }



46:40
Now in our main loop, Casey will have to loop through all of our controller script

                
                handmade_world_mode.cpp

                internal b32 UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, transient_state *TranState,
                                     game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
                {

                    ...
                    ...

                    b32 HeroesExist = false;
                    b32 QuitRequested = false;

                    for(u32 ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
                    {
                        game_controller_input *Controller = GetController(Input, ControllerIndex);
                        controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;
                        if(ConHero->EntityIndex.Value == 0)
                        {
                            ...
                            ...

                        }
                    }
                }




Q/A
1:05:11
Casey giving the example of a snake creature to see whether his architecture will work well or not 

