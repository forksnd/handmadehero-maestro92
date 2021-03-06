Handmade Hero Day 085 - Transient Ground Buffers

Summary:
talks about new approach for the scrolling ground buffer
mentions that will want to do a tile based approach so we do not waste bandwidth

discusses wanting to put the ground buffer inside transient memory.

talks about the three main ways to partition memory
fixed
pseudo fixed
free for all

mentions how we want to use transient memory for this ground buffers 
and the simregions(which we were previously using the transient memory for)

Keyword:
Transient Memory, memory partition


3:23
mentions that the approach he suggested in day084 is kind of dumb


				Frame1
				Ground Buffer
				 _______________________
				|		|				|
				| 		|    A 			|
				|		|		    	|	
				|	 	|		_______	| 
				|		|	   |	   ||	
				|		|	   |screen ||
				|		|	   |	   || 
				|		|	   |_______||
				|		|				|
				|_______|_______________|


in the old approach, when the screen recenters, we have to copy region A back into this ground buffer.

				Frame2
				Ground Buffer
				 _______________________
				|				|		|
				|				|		|
				|			A	|   B	|	
				|	 _______	|		|
				|	|		|	|		|	
				|	|screen	|	| 		|
				|	|		| 	|		|
				|	|_______|	|		|
				|				|		|
				|_______________|_______|


Casey points out that copying Region A back to the buffer is a huge waste of bandwidth becuz 
eventually, we have to copy from this ground buffer to the windows screen. 

This is what he meant by 

				since we are already spending the bandwidth copy from the buffer to the screen anyway



3:49
Casey suggests new approach:

we break the buffer into the tiles 

				 _______________________			
				|		|		|		|
				|		|		|		|
				|		|		|   	|	
				|_______|_______|_______|
				|		|		|		|	
				|		|		| 		|
				|		|		| 		|
				|_______|_______|_______|
				|		|		|		|	
				|		|		| 		|
				|		|		| 		|
				|_______|_______|_______|


of course we store it as an array
				 _______			
				|		|
				|		|
				|		|	
				|_______|
				|		|	
				|		|
				|		|
				|_______|
				|		|	
				|		|
				|		|
				|_______|
				|		|	
				|		|
				|		|
				|_______|
				|		|	
				|		|
				|		|
				|_______|
				|		|	
				|		|
				|		|
				|_______|


then we just blit them directly onto the windows screen

so if our resolution is 1920 x 1080,

and our tiles are 256 x 256

1920 / 256 = 7.5
1080 / 256 = 4.2

we would 8 x 5 = 40 tiles cover the screen

so if we want to determine the dimensions of our buffer
we want something substantially more than 40, so maybe we can do 64 (this number can change)

so we can have our buffer to be 256 x 256 x 64 


5:31
so this new approach is that we just make an array of 256 x 256 x 64

this approach has no blitting, ever. This way we are only doing the copy to the screen
which we have to do anyway.

we keep track of each tiles world position. Then when we move the screen around, 
we evict the least recently used one



8:30
The idea with Transient memory is that it doesnt matter if it does not persist from
frame to frame 

if it does get preserved from frame to frame, Maybe thats good, maybe that saves us work.
but if it doesnt, no big deal. For example, if we were to reload a game, the stuff in the transient
can literally not be stored at all. We can trade CPU time to regenerate it.







24:10

mentioned a few algorithms on how do we want to throw away old tile chunks when we scroll

introduces the notion of LRU, Least recently used

it is not necessarily the best indicator. We can possibiliy use which ever one 
is furthest away from the character

But there are also other cases if the character teleports. So the best heuristic can depend.



34:26
we need to understand our memory partition 

there are three options

option 1, the 100% fixed layout
we always have a fixed amount of memory, assume we just have 2GB of memory
and you just plan out where the memory for each system is going to live in

				2GB of memroy 
				 _______			
				|		|
				|		|
				|		|	
				|_______|
				|		|	
				|		|	<--- renderer stuff lives here, hard bounds
				|		|
				|_______|
				|		|	
				|		|	<--- world stuff lives here, hard bounds
				|		|	
				|_______|



Pros:
always works 100% predictably

Cons:
not flexibile, for example if theres a scenario, the renderer system or one particular system 
can use more memory, then it cant do it 




Option 2, pseudo fixed
you still start with a total fixed amount of 2GB, but you do not specify where things are going to live in
you just pile stuff on and on and on, and see where the usage amount

				2GB of memory
				 _______			
				|		|
				|		|
				|		|	<--- put all the renderer stuff here
				|		|		 and just so happens that we used this much
				|		|		 not restricted by any bounds
				|_______|	
				|		|
				|		|
				|		|	
				|		|	
				|		|	
				|_______|

Cons:
if you do not know the bounds, you might just run over your 2GB limit. So it is a bit unpredictable in that sense
and "What do you do when that happens"



Option 3, free for all
you start off by allocating some memory, and then you start partitining it 

				no amount specified
				 _______			
				|		|
				|		|
				|		|	
				|_______|
				|		|	
				|		|	<--- renderer stuff lives here
				|		|
				|_______|
				|		|	
				|		|	<--- world stuff lives here
				|		|	
				|_______|

when you run out, you just go back and ask for more 



then you just chain these blocks together
				 _______			
				|		|
				|		|
				|		|	
				|_______|
				|		|	
				|		|	
				|		|
				|_______|
				|		|	
				|		|	
				|		|	____
				|_______|		|
								|
								|
				 _______		|	
				|		|	<---
				|		|
				|		|	
				|_______|
				|		|	
				|		|	
				|		|
				|_______|
				|		|	
				|		|	
				|		|	____
				|_______|		|
								|
								|
				 _______		|	
				|		|	<---
				|		|
				|		|	
				|_______|
				|		|	
				|		|	
				|		|
				|_______|
				|		|	
				|		|	
				|		|	
				|_______|


So there is no finite bound on them. If you want to allocate 18 GB, keep on going!




37:55

option1 is usually the best way in terms of robustness, becuz you can 100% guarantee 
everyone operates in their system

but not necessarily best for flexibility




40:12
we want to make a boundary between the things that persist from frame to frame in the 
transient arena. For example our ground buffers do not get erased from frame to frame.
But they COULD get erased, and if that happens. it is not a big deal.

Preferably, we do not want them to get erased

so becuz of this, we are making a separation between transient_state and game_state

previously we have our TransientArena and our ground Buffers stored in the game_state 

				struct game_state
				{
					memory_arena TransientArena;
					memory_arena WorldArena;
					world* World;

					...
					...

				    uint32 GroundBufferCount;
				    loaded_bitmap GroundBitmapTemplate;
				    ground_buffer *GroundBuffers;
				};



now that we want to separate the two, we defined a transient_state struct 
and we moved ground related stuff to the transient_state struct


				struct transient_state
				{
				    bool32 IsInitialized;
				    memory_arena TranArena;
				    uint32 GroundBufferCount;
				    loaded_bitmap GroundBitmapTemplate;
				    ground_buffer *GroundBuffers;
				};


				struct game_state
				{
					memory_arena WorldArena;
					world* World;

					...
					...
				};


42:26
so for our transient memory, Casey plans to do this.
Recall currently we use the transient memory for two parts:

1.	This ground buffer that we used for rendering our ground

2.	in BeginSim, we use transient memory to do simulation
	we allocate 
	we move all the entities in a sim region from the WorldSpace to sim space and we simulate each entity

				handmade.cpp

			    temporary_memory SimMemory = BeginTemporaryMemory(&TranState->TranArena);
			    sim_region *SimRegion = BeginSim(&TranState->TranArena, GameState, GameState->World,
			                                     GameState->CameraP, CameraBounds, Input->dtForFrame);
			    ...
				...



			    handmade_sim_region.cpp

			    internal sim_region *
				BeginSim(memory_arena *SimArena, game_state *GameState, world *World, world_position Origin, rectangle3 Bounds, real32 dt)
				{
				    ...
				    ...

			        SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);
	    
			        ...
			        ...

			        simulate our entities

			        ...
			    }


so the way we want to use our memory is 

				 _______			
				|		|
				|		|	<--- ground buffers stuff 
				|		|	
				|_______|	<--- start
				|		|	
				|		|	<--- sim region stuff 
				|		|
				|		|	
				|		|	
				|		|	
				|		|	
				|_______|

so ground buffers stuff is fixed, so we put it at the top
for sim region stuff, we want it to start after the ground buffers. 
everytime it beginsSim Region, it will use as much as it needs 
then at EndSim, we bungie back to the "start"





42:44
we separate the initialization of PermanentStorage and transient storage

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{    

					...
    				Assert(sizeof(game_state) <= Memory->PermanentStorageSize);   
				    game_state *GameState = (game_state *)Memory->PermanentStorage;
				    if(!Memory->IsInitialized)
				    {
				    	...
				    	...
				    	Memory->IsInitialized = true;
				    }

				    Assert(sizeof(transient_state) <= Memory->TransientStorageSize);    
				    transient_state *TranState = (transient_state *)Memory->TransientStorage;
				    if(!TranState->IsInitialized)
				    {

				    	TranState->IsInitialized = true;
				    }

				    ...
				    ...

				}




16:01 45:04
we begin to initalizing the groundBufferTile array. 

Note that GroundBufferWidth is really GroundBufferTilePixelWidth, since we are doing 128 of 256 x 256
so the name is slightly misleading


-	defines the ground_buffer class. This represents the tiles in our array.


				struct ground_buffer
				{
				    // NOTE(casey): An invalid P tells us that this ground_buffer has not been filled
				    world_position P; // NOTE(casey): This is the center of the bitmap
				    void *Memory;
				};


full code below, as you can see, we are just allocating 128 of 256 x 256 bitmaps

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{    

				    Assert(sizeof(transient_state) <= Memory->TransientStorageSize);    
				    transient_state *TranState = (transient_state *)Memory->TransientStorageSize;
				    if(!TranState->IsInitialized)
				    {

				    	...
				    	...

				        uint32 GroundBufferWidth = 256;
				        uint32 GroundBufferHeight = 256;
				        TranState->GroundBufferCount = 128;
				        TranState->GroundBuffers = PushArray(&TranState->TranArena, TranState->GroundBufferCount, ground_buffer);
				        for(uint32 GroundBufferIndex = 0;
				            GroundBufferIndex < TranState->GroundBufferCount;
				            ++GroundBufferIndex)
				        {
				            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
				            TranState->GroundBitmapTemplate = MakeEmptyBitmap(&TranState->TranArena, GroundBufferWidth, GroundBufferHeight, false);
				            GroundBuffer->Memory = TranState->GroundBitmapTemplate.Memory;
				            GroundBuffer->P = NullPosition();
				        }

				    	TranState->IsInitialized = true;
				    }







17:22 
in the rendering code, we just go through our entire array, and we render our ground buffer tiles 

-	the way we verify if a groundTileBuffer is valid is checking 

				IsValid(GroundBuffer->P)

those ones who have not filled with a particular world position value.


full code below:
				handmade.cpp 

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{    
					...
					...

				    if(!Memory->IsInitialized)
				    {
				    	...
				    }

				    if(!TranState->IsInitialized)
				    {
				    	...
				    }

				    ...
				    ...

				    for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; ++GroundBufferIndex)
				    {
				        ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
				        if(IsValid(GroundBuffer->P))
				        {
				            loaded_bitmap Bitmap = TranState->GroundBitmapTemplate;
				            Bitmap.Memory = GroundBuffer->Memory;
				            v3 Delta = GameState->MetersToPixels*Subtract(GameState->World, &GroundBuffer->P, &GameState->CameraP);        
				            v2 Ground = {ScreenCenterX + Delta.X - 0.5f*(real32)Bitmap.Width,
				                         ScreenCenterY - Delta.Y - 0.5f*(real32)Bitmap.Height};            
				            DrawBitmap(DrawBuffer, &Bitmap, Ground.X, Ground.Y);
				        }
				    }




46:48
recall that we want transient memory for simRegion to start after ground buffers, and we want to do the bungie
jump back at EndSim();

so we do the following.
				

-	notice we do a check of the memory at the end 


				handmade.cpp


				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{    

					...
					...

				    temporary_memory SimMemory = BeginTemporaryMemory(&TranState->TranArena);
	    			sim_region *SimRegion = BeginSim(&TranState->TranArena, GameState, GameState->World,
	                                     GameState->CameraP, CameraBounds, Input->dtForFrame);

	    			...
	    			...

				    EndSim(SimRegion, GameState);
				    EndTemporaryMemory(SimMemory);

				    CheckArena(&GameState->WorldArena);
				    CheckArena(&TranState->TranArena);

				}


48:52
below we show the BeginTemporaryMemory(); and EndTemporaryMemory() functions 

in BeginTemporaryMemory(); we store the boundaries. notice the memory_arena has this variable
TempCount. we increment it in BeginTemporaryMemory(); and decrement it in EndTemporaryMemory();

				handmade.h

				struct temporary_memory
				{
				    memory_arena *Arena;
				    memory_index Used;
				};

				...
				...

				inline temporary_memory
				BeginTemporaryMemory(memory_arena *Arena)
				{
				    temporary_memory Result;

				    Result.Arena = Arena;
				    Result.Used = Arena->Used;

				    ++Arena->TempCount;

				    return(Result);
				}


then in EndTemporaryMemory(); we bungie back to the boundary we stored in BeginTemporaryMemory();


				inline void
				EndTemporaryMemory(temporary_memory TempMem)
				{
				    memory_arena *Arena = TempMem.Arena;
				    Assert(Arena->Used >= TempMem.Used);
				    Arena->Used = TempMem.Used;
				    Assert(Arena->TempCount > 0);
				    --Arena->TempCount;
				}

				inline void
				CheckArena(memory_arena *Arena)
				{
				    Assert(Arena->TempCount == 0);
				}




1:01:08
explains how does memory work, and what he meant by "Push" and "Pop" for memory storage 

memory is a big line!

				12 GB
				 _______		
				|		|
				|		|	
				|		|	
				|		|	
				|		|	
				|		|	 
				|		|
				|		|	
				|		|	
				|		|	
				|		|	
				|_______|


if your computer has 12 GB of memory, that means your physical memory starts from byte0
and goe sto byte 12 GB

but now all OS uses virtual memory. so what we see is virtual memory

				virtual memory
				 _______		
				|		|
				|		|	
				|		|	
				|_______|	x
				|		|	
				|		|	 
				|		|
				|_______|	x + 1 GB
				|		|	
				|		|	
				|		|	
				|_______|


pretty much how it works is that, if you want to allcate memory, you can have access to 
memory starting at byte x, going to byte x + 1 GB (assuming 1 GB is the amount you want )

and that is mapped to somewhere in physical memory. So we do not really deal with physical memory.
we only see the virtual memory space 




1:03:58

so how our games memory works is that we just asked the OS for a huge chunk

				 _______	<--- start of our transient memory	
				|		|
				|		|	
				|		|	
				|		|	
				|		|	
				|		|	 transient storage size 
				|		|
				|		|	
				|		|	
				|		|	
				|		|	
				|_______|	



and when in our game we are asking for memory it is kind of like pushing on to a stack


				 _______	<--- start of our transient memory	
				|_______|
				|		|	
				|		|	<--- a call to PushStruct or PushArray
				|		|	
				|_______|	
				|		|	<--- a call to PushStruct or PushArray
				|_______|
				|		|		
				|		|	|
				|		|	|	as we do more calls to memory, the memory usage goes down
				|		|	|	
				|_______|	V



The restriction here is that you have to do this in order. You cant free something in the middle of your stack
one of the reasons why people lean on calls to "new", "malloc", or "allocates" is that these are out of order


1:08:17
Caseys solution to the "out of order" problem with this push and pop arthictecture is the "free list". Once things
are allocated, they arent really ever freed unless they are purely temporary.

Casey find that more efficient, and he never runs into those fragmentation problems







