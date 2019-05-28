Handmade Hero Day 288 - Finishing Brains

Summary:
more work on the controller template based entity system that was started in day 287 

Keyword:
entity system 


12:35
in the Brain struct (as mentioned in day 287, brain is entity controller);
we are adding a list of entities that this controller controls 


                handmade_entity.h



                struct brain_hero_parts
                {
                    entity *Head;
                    entity *Body;
                };

                struct brain
                {
                    brain_id ID;
                    brain_type Type;
                    
                    union
                    {
                        brain_hero_parts Hero;
                        entity *Array[16];
                    };
                };






14:55
recall that we previously added the brain_slot in the entity struct 
that tells us where to access itself in the brain 

                struct entity
                {
                    entity_id ID;
                    b32 Updatable;

                    //
                    // NOTE(casey): Things we've thought about
                    //
                    
                    
                    //
                    // NOTE(casey): Everything below here is NOT worked out yet
                    //

                    brain_type BrainType;
    ----------->    brain_slot BrainSlot;
                    brain_id BrainID;
                    ...
                };

for example, for an entity to access himself through the brain, it can do 


                entity ent = brain->Array[ent->BrainSlot];



17:10
so now in handmade_world_mode.cpp, we go through the logic of all 
brains [recall brains are controllers in our game] and do logic on them 

take for example, for our Brain_Hero brain, 
we access the Head entity and Body entity as follow:

                brain_hero_parts *Parts = &Brain->Hero;
                entity *Head = Parts->Head;
                entity *Body = Parts->Body;

-   full code below:


                handmade_world_mode.cpp 

                internal b32 UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, transient_state *TranState,
                                     game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
                {

                    ...
                    ...

                    for(u32 BrainIndex = 0;
                        BrainIndex < SimRegion->BrainCount;
                        ++BrainIndex)
                    {
                        brain *Brain = SimRegion->Brains + BrainIndex;
                        switch(Brain->Type)
                        {
                            case Brain_Hero:
                            {
                                // TODO(casey): Check that they're not deleted when we do?
                                brain_hero_parts *Parts = &Brain->Hero;
                                entity *Head = Parts->Head;
                                entity *Body = Parts->Body;
                                
                                u32 ControllerIndex = Brain->ID.Value - ReservedBrainID_FirstHero;
                                game_controller_input *Controller = GetController(Input, ControllerIndex);
                                controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;

                                ...
                                ...
                            }
                        }
                    }

                    ...
                }



19:06
So now Casey wants to address a problem in our current code 
he now wants to be able to put accelerations into objects. 
so currently we can have our brain to assign accelerations into objects, and later when the physics kicks in, we do the ticking there. 


there are certain scenarios we have to be careful of.

for examples:
lets say one brain controls for entities 
and all of them wants to move in the x = 1 direction
         _______   _______   _______   _______  
        |       | |       | |       | |       | 
        |   --->| |   --->| |   --->| |   --->| 
        |_______| |_______| |_______| |_______| 

            A         B         C         D

because they are right next to each other, if I happen to move A first, its gonna hit B.
and it wont go the full amount it was gonna go. so you can end up with all these fractious micro bumping 


so you have to assign accelerations to all these parts and simulate them together. so you want to do as much simulataneous movement 
as possible 



22:00
so now in our Brain_Hero code (hero controller code); we want to assign acceleration to our entities 


                for(u32 BrainIndex = 0; BrainIndex < SimRegion->BrainCount; ++BrainIndex)
                {
                    brain *Brain = SimRegion->Brains + BrainIndex;
                    switch(Brain->Type)
                    {
                        case Brain_Hero:
                        {
                            // TODO(casey): Check that they're not deleted when we do?
                            brain_hero_parts *Parts = &Brain->Hero;
                            entity *Head = Parts->Head;
                            entity *Body = Parts->Body;
                                                        
                            ...
                            ...

                            if(Head)
                            {
                                ...
                                ...
                            }

                            if(Body)
                            {
                                ...
                                ...
                            }
                        }
                    }
                }


34:48
Casey made it so that the brainId corresponds to the controller Index

so in the handmade_entity.h, Casey added these reserved_brain_id

                handmade_entity.h 

                enum reserved_brain_id
                {
                    ReservedBrainID_FirstHero = 1,
                    ReservedBrainID_LastHero = (ReservedBrainID_FirstHero + MAX_CONTROLLER_COUNT - 1),
                    
                    ReservedBrainID_FirstFree,
                };



then in the main logic, for heros, we get the Controller index based on its brainId 
handmade_world_mode.cpp

                handmade_world_mode.cpp 

                internal b32 UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, transient_state *TranState,
                                     game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
                {
                    ...
                    ...

                    for(u32 BrainIndex = 0; BrainIndex < SimRegion->BrainCount; ++BrainIndex)
                    {
                        brain *Brain = SimRegion->Brains + BrainIndex;
                        switch(Brain->Type)
                        {
                            case Brain_Hero:
                            {
                                // TODO(casey): Check that they're not deleted when we do?
                                brain_hero_parts *Parts = &Brain->Hero;
                                entity *Head = Parts->Head;
                                entity *Body = Parts->Body;
                                
            --------------->    u32 ControllerIndex = Brain->ID.Value - ReservedBrainID_FirstHero;
                                game_controller_input *Controller = GetController(Input, ControllerIndex);
                                controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;

                                ...
                                ...
                            }
                        }
                    }

                    ...
                    ...
                }


56:17
Casey also added the BrainSlotFlor function

recall in day 287, we had 

                Handmade.cpp

                internal void AddPlayer(game_mode_world *WorldMode, sim_region *SimRegion, traversable_reference StandingOn,
                          brain_id BrainID)
                {
                    ...
                    ...

                    entity *Body = BeginGroundedEntity(WorldMode, EntityType_HeroBody,
                        WorldMode->HeroBodyCollision);
                    AddFlags(Body, EntityFlag_Collides|EntityFlag_Moveable);

                    entity *Head = BeginGroundedEntity(WorldMode, EntityType_HeroHead,
                        WorldMode->HeroHeadCollision);
                    AddFlags(Head, EntityFlag_Collides|EntityFlag_Moveable);

                    InitHitPoints(Body, 3);

                    // TODO(casey): We will probably need a creation-time system for
                    // guaranteeing now overlapping occupation.
                    Body->Occupying = StandingOn;

                    brain_id BrainID = AddBrain(WorldMode);
                    Body->BrainType = Brain_Hero;
                    Body->BrainSlot.Index = 0;
                    Body->BrainID = BrainID;
                    Head->BrainType = Brain_Hero;
                    Head->BrainSlot.Index = 1;
                    Head->BrainID = BrainID;

                    ...
                    ...
                }


and Casey changed the AddPlayer(); function
                
                handmade_world_mode.cpp

                internal void AddPlayer(game_mode_world *WorldMode, sim_region *SimRegion, traversable_reference StandingOn, brain_id BrainID)
                {
                    ...
                    ...

                    // TODO(casey): We will probably need a creation-time system for
                    // guaranteeing now overlapping occupation.
                    Body->Occupying = StandingOn;

                    Body->BrainType = Brain_Hero;
                    Body->BrainSlot = BrainSlotFor(brain_hero_parts, Body);
                    Body->BrainID = BrainID;
                    Head->BrainType = Brain_Hero;
                    Head->BrainSlot = BrainSlotFor(brain_hero_parts, Head);
                    Head->BrainID = BrainID;

                    ...
                }

now Casey added a BrainSlotFlor function 

                #define BrainSlotFor(type, Member) BrainSlotFor_(&(((type *)0)->Member) - (entity **)0)
                inline brain_slot
                BrainSlotFor_(u32 PackValue)
                {
                    brain_slot Result = {PackValue};
                    
                    return(Result);
                }

what casey is doing is that, recall that for the brain_hero_parts struct, we have:

                handmade_entity.h

                struct brain_hero_parts
                {
                    entity *Head;
                    entity *Body;
                };

which is just a list of entity pointer.

here we have "Body" and "Hero" in the brain_hero_parts struct.
if we figure out the byte offset within the struct, then we can get the index for "Body" and "Hero" reliably.

so the term "&(((type *)0)->Member)", translates to the address of "brain_hero_parts->Body" or "brain_hero_parts->Hero".


we actually have done this once in the past:

                (type *)0; 

translates to 

                (brain_hero_parts*)0;

which is casting address 0 into a brain_hero_parts struct.

then we do 
                
                &(((type *)0)->Member);

depending if you are 32bit or 64 bit, pointers are either 4 bytes or 8 bytes, then 
if we access the memory address of "brain_hero_parts->Body"
this gives us either 4 or 8

then we do the math, which is to divide by the size of entity pointers 


                BrainSlotFor_(&(((type *)0)->Member) / sizeof(entity);


or like what Casey did, which is calculating how many (entity **); is from address 0 to address Member

                BrainSlotFor_(&(((type *)0)->Member) - (entity **)0);

[kind of like a C way of doing reflections];




Q/A

1:11:11
someone mentioned that, why not define enums for Head and body, so that we have:
                
                entity *Head = Brain->Slots[BrainSlot_Head];
                entity *Body = Brain->Slots[BrainSlot_Body];
                
instead of 

                entity *Head = Parts->Head;
                entity *Body = Parts->Body;
                

he said both are equally fine, and he said, maybe the enums is the right way to do                 

