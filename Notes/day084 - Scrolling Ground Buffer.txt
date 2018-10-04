Handmade Hero Day 084 - Scrolling Ground Buffer

Summary:

fixed rendering of ground so that the ground is rendered to the proper location in the world.
started the plan to implement a scrolling ground buffer. (havent finished implementing it in this episode)
that way when entities moves out of the screen, the groud buffer is recentered to the middle of the screen.

talked about the difference between reference and pointer in the Q/A

Keyword: 
rendering




first 30 minutes 
talked about ideas of generating ground


31:31
so we need 


if this is the screen, we want the screen to fill the ground, we can just use width x height
				 _______________
				|		     	|
				|	Z = 0     	|
				|			 	|		
				|			 	|
				|			 	|
				|_______________|


but now if imagine the screen recedes into the distance by a little bit. For example
the character walks up the stairs, things in the lower level shrinks a bit. (things further a way shrinks a bit)

				 _______________
				|	Z = 1     	|
				|	 _______   	|
				|	| Z = 0	| 	|		
				|	|_______| 	|
				|			 	|
				|_______________|

so now, the Z = 1 level needs to fill the screen.

so for us, we need to find out the Z scaling factor of the Z top level and the Z bottom level 







33:00
we modify the Ground Buffer 

-	we also defined the GroundOverscan. that is used for situations where when we scroll to the left or right 
	we do not have to update quite as often

				 _______________________
				|						|
				|	 _______________ 	|
				|	|	Z = 1     	|	|
				|	|			 	|	|
				|	|				|	|	
				|	|			 	|	|
				|	|			 	|	|
				|	|_______________|	|
				|						|
				|_______________________|


-	for now we just define the ground buffer dimensions as below:

				uint32 GroundBufferWidth = RoundReal32ToInt32(GroundOverscan*ScreenWidth);
			    uint32 GroundBufferHeight = RoundReal32ToInt32(GroundOverscan*ScreenHeight);


full code below

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{    
				   	...
				   	...

				    if(!Memory->IsInitialized)
				    {
				       	...
				       	...


				        real32 ScreenWidth = (real32)Buffer->Width;
				        real32 ScreenHeight = (real32)Buffer->Height;
				        real32 MaximumZScale = 0.5f;
				        real32 GroundOverscan = 1.5f;
				        uint32 GroundBufferWidth = RoundReal32ToInt32(GroundOverscan*ScreenWidth);
				        uint32 GroundBufferHeight = RoundReal32ToInt32(GroundOverscan*ScreenHeight);
				        GameState->GroundBuffer = MakeEmptyBitmap(&GameState->WorldArena, GroundBufferWidth, GroundBufferHeight);
				        GameState->GroundBufferP = GameState->CameraP;
				        DrawGroundChunk(GameState, &GameState->GroundBuffer, &GameState->GroundBufferP);

				        Memory->IsInitialized = true;
				    }




36:14
So when our camera follows our character, we have to constantly recenter our buffer 


				 _______________________
				|						|
				|	buffer				|
				|				    	|	
				|	 _______	 		|
				|	|		|			|	
				|	|screen	|	 		|
				|	|		| ----->	|
				|	|_______|			|
				|						|
				|_______________________|




when our screen is almost moving out of our buffer, we need to recenter it 
				 _______________________
				|						|
				|	buffer				|
				|				    	|	
				|	 			_______	| 
				|			   |	   ||	
				|			   |screen ||
				|			   |	   || ----->
				|			   |_______||
				|						|
				|_______________________|


37:19 
when we recenter, we need to copy region A that was in frame1 to frame2, then fill up region B in frame2

				Frame1
				 _______________________
				|		|				|
				|	buffer    A 		|
				|		|		    	|	
				|	 	|		_______	| 
				|		|	   |	   ||	
				|		|	   |screen ||
				|		|	   |	   || 
				|		|	   |_______||
				|		|				|
				|_______|_______________|



				Frame2
				 _______________________
				|				|		|
				|	buffer		|		|
				|			A	|   B	|	
				|	 _______	|		|
				|	|		|	|		|	
				|	|screen	|	| 		|
				|	|		| 	|		|
				|	|_______|	|		|
				|				|		|
				|_______________|_______|


37:39
since this is not multi-threaded for now, so we will experience a hiccup when this happens cuz we composite very slow.
but that is acceptable for now.



38:18
so we need to store the position of our ground buffer relative to our camera 
so we added a "GroundBufferP" at game_state

				struct game_state
				{
					...
					...
				    
				    world_position GroundBufferP;
				    loaded_bitmap GroundBuffer;
				};


the idea is that initially, we have GroundBufferP equal to the cameraP.
then whenever cameraP walks too far away, we have GroundBufferP snap in





42:44 
so every frame, we render our ground with the proper positioned ground, 
with proper conversion of world to screen transformation, you see the ground locked to the world.
(I really didnt take the time to study the math. You just have to take the time to sort out world-to-pixel, pixel-to-world transformations)

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{ 

					...
					...

				    real32 ScreenCenterX = 0.5f*(real32)DrawBuffer->Width;
				    real32 ScreenCenterY = 0.5f*(real32)DrawBuffer->Height;

				    v2 Ground = {ScreenCenterX - 0.5f*(real32)GameState->GroundBuffer.Width,
				                 ScreenCenterY - 0.5f*(real32)GameState->GroundBuffer.Height};
				    v3 Delta = Subtract(GameState->World, &GameState->GroundBufferP, &GameState->CameraP);
				    Delta.Y = -Delta.Y;
				    Ground += GameState->MetersToPixels*Delta.XY;
				    DrawBitmap(DrawBuffer, &GameState->GroundBuffer, Ground.X, Ground.Y);
				}



45:50
so when we walk from room1 to room2, we want there to be ground.

back to the problem mentioned above. We want to be able to fill region B. And it has to be deterministic


				Frame2
				 _______________________
				|				|		|
				|	buffer		|		|
				|			A	|   B	|	
				|	 _______	|		|
				|	|		|	|		|	
				|	|screen	|	| 		|
				|	|		| 	|		|
				|	|_______|	|		|
				|				|		|
				|_______________|_______|



Caseys approach is to generate ground splats based on world position. 

we split the world into chunks, 
Use the world position as a seed. 
then generate the spltas for that chunk

				 _______________________
				|	|			|		|
				|___|___________|_______|
				|	|			|   	|	
				|	| 			|		|
				|	|	seed	|  seed |	
				|	|  			| 		|
				|	|		 	|		|
				|___|___________|_______|	
				|	|			|		|
				|___|___________|_______|


51:34
you can see the code below. Casey puts a temporary seed for the random number generator

				handmade.cpp

				internal void
				DrawGroundChunk(game_state *GameState, loaded_bitmap *Buffer, world_position *ChunkP)
				{
				    // TODO(casey): Make random number generation more systemic
				    // TODO(casey): Look into wang hashing or some other spatial seed generation "thing"!
				    random_series Series = RandomSeed(139*ChunkP->ChunkX + 593*ChunkP->ChunkY + 329*ChunkP->ChunkZ);

				    ...
				    ...

				    draw a bunch of grass

				    ...
				    ...

				    draw a bunch of stones

				}



56:20
when you are developing, you almost never want to see your game in full screen, the reason for that is becuz
you may have drawing bug off the region that you intended

in the future, Casey will even push the rendered region just the middle as oppose to now which is anchored in the 
top left corner




1:05:13
you mentioned off loading ground drawing to a separate thread to prevent hitches? why so?

Casey says his intentions is to off load it to different threads so that it can be drawn over multiple frames.

ususally there are lots of CPU power sitting around and not doing anything. So we can have cores doing work asynchornously




1:10:34
difference between pointer and reference in C++

THey are the same, they are just syntatically different.

				int X;
				int *ptrX = &X;

				int& ptrX = X;


what a reference is essentially a pointer that does not need to be dereferenced in order to be used.
so its just saying to the compiler, I want you to treat this like a pointer, but pretend that I dereferenced a pointer that you have

its just another name for X

if you have something like 

				struct giant 
				{
					int Foo[4096];
				};

there is no way you will fit that in a register

				giant X;
				giant *PtrX = &X;
				giant &RefX = X;


so when you do either of the two 

				void Foo(giant *Bar)
				{
					Bar->Z[36] = 3;
				}

				void Foo(giant &Bar)
				{
					Bar->Z[36] = 3;
				}

these two might look different, but in terms of what the compiler is going to generate, they will generate
exactly the same code for these two foos.


So it will really just end up being a pointer when the compiler uses it 
So it is really about syntactically how you want to treat this thing.


if there are some crazy C++ features that you care about, then you might care about references.
otherwise you never need to use them. Pointers do the same thing.


