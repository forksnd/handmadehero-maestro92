Handmade Hero Day 082 - Caching Composited Bitmaps
  
Summary:

wrote an api for ourrandom number generator (which is currently essentially a giant array of random numbers)
replaced all the places where we directly get the numbers from the array with
api calls from handmade_random.h

rendered the ground with a bunch of random rocks, stones, turfs.

Optimized rendering ground. Pre-rendered the ground into a bitmap. Then every frame, blit the drawBuffer with this pre-rendered ground bitmap.

Talked about pass by reference vs pass by value in Q/A

Keyword: 
random number generator api, Bitmap Rendering



7:31
mentions the idea of seed in random numbers generation

and of course with the concept of seed, random numbers really pseudo random, not fully random



8:10
currently the our random number generator is just a giant array.
so the seed in our random number generator is just gonna be the starting index of the giant array. 

with this structure of having a giant array, the control you have is pretty much the starting index,
or step size 



9:21
Casey says his experience with random numbers is that you never want people to just grab random numbers
you want to give people to have a handle, that they use to get random numbers

some times you want truly random, such as particle effects 
other times, you want controlled randomness that is deterministic




10:21 
starting the api for our random number generator

				struct random_series
				{
				    uint32 Index;
				};




13:10
wrote two function. 
RandomUnilateral is for [0 ~ 1]

RandomBilateral is for [-1 ~ 1]


				handmade_random.h

				inline real32 RandomUnilateral(random_series *Series)
				{
				    real32 Divisor = 1.0f / (real32)MaxRandomNumber;
				    real32 Result = Divisor*(real32)RandomNextUInt32(Series);

				    return(Result);
				}

				inline real32 RandomBilateral(random_series *Series)
				{
				    real32 Result = 2.0f*RandomUnilateral(Series) - 1.0f;

				    return(Result);
				}






17:44

as said earlier, our seed is just gonna be the starting index

				inline random_series RandomSeed(uint32 Value)
				{
				    random_series Series;

				    Series.Index = (Value % ArrayCount(RandomNumberTable));

				    return(Series);
				}





11:08 
so when we see our random number generator in action, we have it like below

and then we go on to use series for later use 

				internal void
				DrawTestGround(game_state *GameState, loaded_bitmap *Buffer)
				{
				    // TODO(casey): Make random number generation more systemic
				    random_series Series = RandomSeed(1234);

				    ...
				    ...

				}



20:04
the first draft we had for RandomUnilateral was as below

				handmade_random.h

				inline real32 RandomUnilateral(random_series *Series)
				{
					real32 Result = (real32)NextRandomUInt32(Series)/(real32)MaxRandomNumber;
				    return(Result);
				}


the new version is 
CPU are usually better at multiplying then dividing. 
(Not sure why so, since we are still doing one division.. but that is what Casey said)

				handmade_random.h

				inline real32 RandomUnilateral(random_series *Series)
				{
				    real32 Divisor = 1.0f / (real32)MaxRandomNumber;
				    real32 Result = Divisor*(real32)RandomNextUInt32(Series);

				    return(Result);
				}






22:41
wrote the RandomBetween(); function

the first draft we had for RandomUnilateral was as below

				inline real32 RandomBetween(random_series *Series, real32 Min, real32 Max)
				{
					real32 Range = Max - Min;
					real32 Result = Min + RandomUnilateral(Series) * Range

					return(Result);
				}


then Casey shortened it to below:

				inline real32 RandomBetween(random_series *Series, real32 Min, real32 Max)
				{
				    real32 Result = Lerp(Min, RandomUnilateral(Series), Max);

				    return(Result);
				}



33:57
wrote the RandomBetween for intergers
making ((Max + 1) - Min), makes it include Max

				inline int32 RandomBetween(random_series *Series, int32 Min, int32 Max)
				{
				    int32 Result = Min + (int32)(RandomNextUInt32(Series)%((Max + 1) - Min));

				    return(Result);
				}







46:14
start to do a massive clean up with loaded_bitmap

-	notice that in bitmap images, 

according to the link below:
https://docs.microsoft.com/en-us/windows/desktop/direct3d9/width-vs--pitch

	"the pitch is Pitch is the distance, in bytes, between two memory addresses that represent the 
	beginning of one bitmap line and the beginning of the next bitmap line."

so we have 
				
				Result.Pitch = -Result.Width*BITMAP_BYTES_PER_PIXEL;

the negative is becuz we are doing some upside down thing, so we are going backwards (not 100% sure on this one)

also with out memory, since we are going backwards, we have memory pointing at the top.

			    Result.Memory = (uint8 *)Result.Memory - Result.Pitch*(Result.Height - 1);





full code below:

				internal loaded_bitmap
				DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName)
				{
				    loaded_bitmap Result = {};
				    
				    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);    
				    if(ReadResult.ContentsSize != 0)
				    {
				        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
				        uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
				        Result.Memory = Pixels;
				        Result.Width = Header->Width;
				        Result.Height = Header->Height;

				        ...
				        ...

				        uint32 *SourceDest = Pixels;
				        for(int32 Y = 0; Y < Header->Height; ++Y)
				        {
				            for(int32 X = 0; X < Header->Width; ++X)
				            {
				                uint32 C = *SourceDest;

				                *SourceDest++ = ... get its proper rgb value ...;
				            }
				        }
				    }

				    Result.Pitch = -Result.Width*BITMAP_BYTES_PER_PIXEL;
				    Result.Memory = (uint8 *)Result.Memory - Result.Pitch*(Result.Height - 1);
				    
				    return(Result);
				}





54:15
optimize rendering the background by predrawing all the ground and storing it into a bitmap.

so in the game_state struct, we store a GroundBuffer bitmap;

				struct game_state
				{
   
					...
					...

				    loaded_bitmap GroundBuffer;
				};


then in initialization. We draw our ground into this bitmap.


				handmade.cpp

				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{
					...
					...

				    if(!Memory->IsInitialized)
				    {
				     

				    	...
				    	...        

				    	GameState->GroundBuffer = MakeEmptyBitmap(&GameState->WorldArena, 512, 512);
				        DrawTestGround(GameState, &GameState->GroundBuffer);
					}
				
				}





55:04

then when we do our rendering to screen each frame, when we render our ground, we just blit it with our pre drawn bit loaded_bitmap


-	recall that the function declaration is defined in handmade_platform.h

				#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, 
															game_input *Input, game_offscreen_buffer *Buffer);

	so the argument "Buffer" passed in is an game_offscreen_buffer, in which the the win32 layer will use it to display the screen.
	so here we are just populating pixel values in it.

-	The DrawRectangle call makes the entire screen gray. The last three arguments indicates the RGB.

-	as you can see, we call the DrawBitmap(DrawBuffer, &GameState->GroundBuffer, 0, 0)); function, and we pass in DrawBuffer and GroundBuffer
	to blit it. this way, we do not have to loop through all the grass, stone, turf every frame to render since everything is already pre-rendered


				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{
					...
					...

				    if(!Memory->IsInitialized)
				    {
				     

				    	...
				    	...        

				    	GameState->GroundBuffer = MakeEmptyBitmap(&GameState->WorldArena, 512, 512);
				        DrawTestGround(GameState, &GameState->GroundBuffer);
					}
				

					...
					...

				    loaded_bitmap DrawBuffer_ = {};
				    loaded_bitmap *DrawBuffer = &DrawBuffer_;
				    DrawBuffer->Width = Buffer->Width;
				    DrawBuffer->Height = Buffer->Height;
				    DrawBuffer->Pitch = Buffer->Pitch;
				    DrawBuffer->Memory = Buffer->Memory;
				    
				    DrawRectangle(DrawBuffer, V2(0.0f, 0.0f), V2((real32)DrawBuffer->Width, (real32)DrawBuffer->Height), 0.5f, 0.5f, 0.5f);
				    // TODO(casey): Draw this at center
				    DrawBitmap(DrawBuffer, &GameState->GroundBuffer, 0, 0);
				
				    ...
				    ...

				}


1:01:21
noticed the bottom middle of the screen, there is a notch when we render the ground. 
That is the result of not bitmaps touching those pixels. When we are finding random screen locations for all the stone, rocks or turfs bitmaps,
not bitmaps covered that area

so see the picture below, imagine we have three pictures of stone in random locations below. 
Areas B are touched by our bitmaps, having proper alpha values. Area A will have alpha 0

				 _______________________________
				|								|
				|	A		 _______			|
				|			|		|			|
				|	 ______	|	B	|			|
				|	| B    ||_______|			|
				|	|______|		 _______	|
				|					|	B	|	|
				|					|_______|	|
				|_______________________________|
										

1:05:58
the concept is kind of like "RenderTexture" in OpenGL



1:17:29
when is it okay to pass larger structs by value or instead of reference

inline functions, you can pass whatever you want, most of time. The optimizer / compiler should be good enough to 
expand those inline functions, look at them, and figure out what needs to be done and do the right thing

in other situations, keep in mind that on a 64 bit machine, a pointer is 8 bytes long. So any structure that is in the neighborhood 
of 8 bytes long. let say 8 bytes, 16 bytes or even 32 bytes, so if you pass a pointer to it, you are not saving much

when you call a function on x64 target, bascially what will happen is that the compiler will push the arguments to the stack. 
if it cant fit them on the register according to the ABI (application binary interface)


Side note: 
https://en.wikipedia.org/wiki/X86_calling_conventions
Calling conventions, type representations, and name mangling are all part of what is known as an application binary interface (ABI).


so it will try to stuff a bunch of stuff on the registers. If it cant fit, then it will be placed on the stack. 
so the idea is that if you are passing stuff to a function that will not exceed 64 bytes ish. Like if you are passing 64 bytes worth of stuff,
most of that is going onto the register anyways. So it does not really matter.

its tough to say. Casey says it is not something he thinks about too hard. 

Obviously, do not pass 4k objects to the stack. Anything that is in the neighborhood, really should not be a big deal.

anywhere that you really care about it, take a look at what the compiler is doing and make sure it is doing what you want.


