Handmade Hero Day 065 - Finishing the Simulation Region Change

Summary:
Continuing day 063, 064

refactered the code where the we parses the controller input and updates the player

Moved all the Update code to the "handmade_entity.h", "handmade_entity.cpp" files 

talks about what is the transient used for, the usage and philosphy behind it

used transient memory in BeginSim code

Keyword: player input control, Entity, transient memory



7:15
refactered the code where the we parses the controller input and updates the player

previously the code looks like this 

				handmade.cpp

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{  

					...
					...

				    for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
				    {

				        game_controller_input *Controller = GetController(Input, ControllerIndex);

				        ...
				        ...

	                 	if(Controller->MoveUp.EndedDown)
		                {
		                    ddP.Y = 1.0f;
		                }
		                if(Controller->MoveDown.EndedDown)
		                {
		                    ddP.Y = -1.0f;
		                }
		                if(Controller->MoveLeft.EndedDown)
		                {
		                    ddP.X = -1.0f;
		                }
		                if(Controller->MoveRight.EndedDown)
		                {
		                    ddP.X = 1.0f;
		                }

            			...
            			...

            			MoveEntity(GameState, ControllingEntity, Input->dtForFrame, &MoveSpec, ddP);

            			...
            			...
					}
				}


the reasons to move the simulation code from the parsing the controller input code is becuz
1.	We do not actually know how the controller parsing is gonna work in the future
	For example, multiple controller update the same player,
	or we are in the menu screen instead of being actually in the game, and we need MoveDown.EndedDown to do something else
	instead of changing the entity velocity

2.	we want to put the entity movement update simulation code with all the other simulation code


so the way we changed it to is that, we introduced a new struct 

				struct controlled_hero
				{
				    uint32 EntityIndex;
				    
				    // NOTE(casey): These are the controller requests for simulation
				    v2 ddP;
				    v2 dSword;
				    real32 dZ;
				};


this stores all the input that was accumulated on this frame. so in the new controller parsing code, we have

				handmade.cpp

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{  

					...
					...

				    for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
				    {
				        game_controller_input *Controller = GetController(Input, ControllerIndex);
				        controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;

				        ...
				        ...

			            ConHero->ddP = {};

		                // NOTE(casey): Use digital movement tuning
		                if(Controller->MoveUp.EndedDown)
		                {
		                    ConHero->ddP.Y = 1.0f;
		                }
		                if(Controller->MoveDown.EndedDown)
		                {
		                    ConHero->ddP.Y = -1.0f;
		                }
		                if(Controller->MoveLeft.EndedDown)
		                {
		                    ConHero->ddP.X = -1.0f;
		                }
		                if(Controller->MoveRight.EndedDown)
		                {
		                    ConHero->ddP.X = 1.0f;
		                }
				    }				        
				}

and also, we moved the movement update simulation code to the simulation region part.
In there we readin controlled_hero, and its accumulated input to update the user
note that Casey will make more changes afterwards



				...
				...


			    for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex, ++Entity)
			    {
			    	...
			    	...
			        
			        switch(Entity->Type)
			        {
			            case EntityType_Hero:
			            {
			            	...

			                for(uint32 ControlIndex = 0; ControlIndex < ArrayCount(GameState->ControlledHeroes); ++ControlIndex)
			                {
			                    controlled_hero *ConHero = GameState->ControlledHeroes + ControlIndex;

			                    if(Entity->StorageIndex == ConHero->EntityIndex)
			                    {
			                    	...
			                    	...

			                        MoveEntity(SimRegion, Entity, Input->dtForFrame, &MoveSpec, ConHero->ddP);

			                        ...
			                        ...
			                    }
			                }

			                ...
			                ...

			            } break;

			            case EntityType_Wall:
			            {
			            	...
			            } break;

			            case EntityType_Sword:
			            {
			            	...
			            } break;

			            ...
			            ...
			        }



25:55
moved UpdateMonstar, UpdateFamilar, UpdateSword to handmade_entity.cpp






32:05
start to talk about what we want to use the transient memory for.

transient memory is pretty much used only for calculations. We will be diving the transient memory to multiple chunks. 
anyone who needs some memory for calculations, borrows a chunk, and uses it. Once it finishes using it, it returns it.


33:05
the reason why having this transient memory is so important is becuz people tend to write code with a spider web of pointers

for example, you can have an entity class that poitns to multiple stuff

				class Entity
				{
					SimSate* simState;

					int* pointerB;
				};


				class SimState
				{
					C*	c;
				};


so when entity gets destoryed, you have to destroy memory for everything. Entity has to tell Simstate, which then destroys C, etc ...

people do this regardless if Entity is permenant or not


we want to use transient memory like a scratch pad.

once we hare done, we just scracth everything. So there is no allocation cost and not deallocation cost.

allocation is just increasing an integer cost. deallocation is just no cost

U leak no memory and there is no cost. So this is a much better way to use memory



35:21 
the only time you need the spider web of pointers is that you need functionality where you just want to delete a specific pointer and deallocate
its memory. The mass majority of system do not need that functionality.






36:40
				
the first time we will use transient memory. 
and we do not need to clean it up.
				

				handmade.cpp

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{  
					...
					...

				    memory_arena SimArena;
				    InitializeArena(&SimArena, Memory->TransientStorageSize, Memory->TransientStorage);
				    sim_region *SimRegion = BeginSim(&SimArena, GameState, GameState->World,
				                                     GameState->CameraP, CameraBounds);



				    ...


				}




37:54
changed the 


				handmade.h

				internal void
				InitializeArena(memory_arena *Arena, memory_index Size, uint8* Base)
				{
				    Arena->Size = Size;
				    Arena->Base = Base;
				    Arena->Used = 0;
				}

to

				internal void
				InitializeArena(memory_arena *Arena, memory_index Size, void* Base)
				{
				    Arena->Size = Size;
				    Arena->Base = (uint8 *)Base;
				    Arena->Used = 0;
				}

that way it can pass in any memory you want






40:33
in the BeginSim function called,
we have to clear the hastable
				

				internal sim_region *
				BeginSim(memory_arena *SimArena, game_state *GameState, world *World, world_position Origin, rectangle2 Bounds)
				{
				    // TODO(casey): If entities were stored in the world, we wouldn't need the game state here!

				    // TODO(casey): IMPORTANT(casey): NOTION OF ACTIVE vs. INACTIVE ENTITIES FOR THE APRON!
				    
				    sim_region *SimRegion = PushStruct(SimArena, sim_region);
				    ZeroStruct(SimRegion->Hash);

				    ...
				    ...
				}

as you can see we defined a ZeroStruct function
again this is "Brute force", which is why he added "Check thihs guy for performance"
my intuition is to use memcpy? 


				handmade.h

				#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
				inline void
				ZeroSize(memory_index Size, void *Ptr)
				{
				    // TODO(casey): Check this guy for performance
				    uint8 *Byte = (uint8 *)Ptr;
				    while(Size--)
				    {
				        *Byte++ = 0;
				    }
				}




1:09:03
regarding high and low entity. would the following scenario work?
using one entity struct, everyone sits in one array. 
Use high frequency updating if he is within camera bounds. 
use low frequency updating if he is outside camera bounds.


What he is asking is, if sim region can point directly into the entity array and update them there.
The answer is we can do that, but there is a reason why Casey does not want to do that that we have not
gotten to yet.

Basically, he does not want the thing "entity struct" we store all the time, to be the "entity struct" that we operate on
here we are storing "low_entity"

				
				struct game_state
				{

					...
					...
					iomt32 LowEntityCount;
					low_entity LowEntities[100000];
					...
					...
				};

and we are operating on 

				struct sim_entity
				{
					...
					...
				};

the reasons will be revealed later.

if your entities are very simple, u can totaly do that.
but if your entities are complicated, you can not do that.

nothing wrong with the proposed solution, but it will not work for what Casey wants to do



1:17:08
all simulation in sim region takese place in floating points coordinates. all relative to the center of simregion

in BeginSim
-	entity copied into sim 
-	position converted to float 
-	References changed to pointers

in EndSim
-	Entity copied to state
-	position translated to int + float 
-	References changed to indicies


We do cuz this way we can stay in float without losing precision, and still have giant worlds
and we can have giant entity counts. We can even compress the LowEntities array.
the size of LowEntities is not a concern, since we are doing high frequency update in sim entities



1:20:15

the transient memory does not laste more than a frame