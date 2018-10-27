Handmade Hero Day 113 - Simple Performance Counters

Summary:

wrote the debug_cycle_counter struct to profile the program
found out that DrawRectangleSlowly is just too slow 

analyzed how badly are we overdrawing, and it turns out we arent

analyzed how expensive the TestPixel + FillPixel portion is 

explained the difference between new and malloc in Q/A

Keyword:
profiling



9:18
recall when we were doing the platform layer
we had this function 
-	__rdtsc(); read time stamp counter, this is actually an intrinsic



				int CALLBACK
				WinMain(HINSTANCE Instance,
				        HINSTANCE PrevInstance,
				        LPSTR CommandLine,
				        int ShowCode)
				{

					...
					...


	                uint64 LastCycleCount = __rdtsc();
	                while(GlobalRunning)
	                {
	                	...
	                	...

                        uint64 EndCycleCount = __rdtsc();
                        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;                  
	                }
				}





10:51 
so the simplest thing we can do is just to do this same code and put it at GAME_UPDATE_AND_RENDER(GameUpdateAndRender);


				extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{
					record cycle count 




					record cycle count 
				}




12:34
starting to define the things needed for the measureing game cycles 




				typedef struct debug_cycle_counter
				{    
				    uint64 CycleCount;
				    ...
				} debug_cycle_counter;




13:08
in our game_memory struct, we give ourselves some counters
-	notice the #if HANDMADE_INTERNAL

-	the reason why we are storing it as an array in game_memory is becuz currently in our handmade.cpp layer 
	we cant print anything. also we dont have any font rendering support. So the only way we can know the numbers is to 
	pass it back the win32 layer and print those out.

				typedef struct game_memory
				{
					...
					...

				#if HANDMADE_INTERNAL
				    debug_cycle_counter Counters[DebugCycleCounter_Count];
				#endif
				} game_memory;


14:22 
for people on other platforms, we may not have __rdtsc(); so we will make it so that only on MSC_VER 
we use rdtsc();

				extern struct game_memory *DebugGlobalMemory;
				#if _MSC_VER
				#define BEGIN_TIMED_BLOCK(ID) uint64 StartCycleCount##ID = __rdtsc();
				#define END_TIMED_BLOCK(ID) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += __rdtsc() - StartCycleCount##ID; ++DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount;
				#else
				#define BEGIN_TIMED_BLOCK(ID) 
				#define END_TIMED_BLOCK(ID) 
				#endif


-	## is the token-pasting operator
https://msdn.microsoft.com/en-us/library/09dwwt6y.aspx


- 	so StartCycleCount##1, becomes StartCycleCount1
StartCycleCount##9, becomes StartCycleCount9

doing this cuz maybe in one code snippet, you want more than one StartCycleCount count 

for example 

				void foo()
				{
					BEGIN_TIMED_BLOCK(1)
					...
					...
					END_TIMED_BLOCK(1)


					BEGIN_TIMED_BLOCK(2)
					...
					...
					END_TIMED_BLOCK(2)
				}

otherwise you will be declaring uint64 StartCycleCount twice 



-	flattening the END_TIMED_BLOCK(); code out 

				#define END_TIMED_BLOCK(ID) 
				DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += __rdtsc() - StartCycleCount##ID; 
				++DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount;
							

18:06
instead of passing random ids to BEGIN_TIMED_BLOCK and END_TIMED_BLOCK

we make enums.
				enum
				{
				    DebugCycleCounter_GameUpdateAndRender,
				    DebugCycleCounter_RenderGroupToOutput,
				    DebugCycleCounter_DrawRectangleSlowly,
				    DebugCycleCounter_TestPixel,
				    DebugCycleCounter_FillPixel,
				    DebugCycleCounter_Count,
				};

so now Casey designed it so that 

				void foo()
				{
					BEGIN_TIMED_BLOCK(DebugCycleCounter_GameUpdateAndRender)
					...
					...
					END_TIMED_BLOCK(DebugCycleCounter_GameUpdateAndRender)


					BEGIN_TIMED_BLOCK(DebugCycleCounter_DrawRectangleSlowly)
					...
					...
					END_TIMED_BLOCK(DebugCycleCounter_DrawRectangleSlowly)
				}







21:32
in our game loop now, we call the HandleDebugCycleCounters(); function

				int CALLBACK
				WinMain(HINSTANCE Instance,
				        HINSTANCE PrevInstance,
				        LPSTR CommandLine,
				        int ShowCode)
				{


					...
					...

                    if(Game.UpdateAndRender)
                    {
                        Game.UpdateAndRender(&Thread, &GameMemory, NewInput, &Buffer);
                        HandleDebugCycleCounters(&GameMemory);
                    }


				}




22:10
and print the numbers out, and we clear them

				win32_handmade.cpp

				internal void
				HandleDebugCycleCounters(game_memory *Memory)
				{
				#if HANDMADE_INTERNAL
				    OutputDebugStringA("DEBUG CYCLE COUNTS:\n");
				    for(int CounterIndex = 0;
				        CounterIndex < ArrayCount(Memory->Counters);
				        ++CounterIndex)
				    {
				        debug_cycle_counter *Counter = Memory->Counters + CounterIndex;

				        if(Counter->HitCount)
				        {
				            char TextBuffer[256];
				            _snprintf_s(TextBuffer, sizeof(TextBuffer),
				                        "  %d: %I64ucy %uh %I64ucy/h\n",
				                        CounterIndex,
				                        Counter->CycleCount,
				                        Counter->HitCount,
				                        Counter->CycleCount / Counter->HitCount);
				            OutputDebugStringA(TextBuffer);
				            Counter->HitCount = 0;
				            Counter->CycleCount = 0;
				        }
				    }
				#endif
				}


26:35
printing out cycle counts in visual studio debug
so currently we are using 310,812,445 cycles

as we previously calcualted we needed to be 107,000,000 cycles per frame


29:90
starts putting blocks every to profiler the program

				BEGIN_TIMED_BLOCK(DebugCycleCounter_GameUpdateAndRender);
				...
				...
				END_TIMED_BLOCK(DebugCycleCounter_GameUpdateAndRender);





35:28
so Casey has put a few timed blocks in 


				    GameUpdateAndRender,
				    {
						BEGIN_TIMED_BLOCK(DebugCycleCounter_GameUpdateAndRender);
						RenderGroupToOutput
						END_TIMED_BLOCK(DebugCycleCounter_GameUpdateAndRender);
				    }


					RenderGroupToOutput
					{
						BEGIN_TIMED_BLOCK(DebugCycleCounter_RenderGroupToOutput);
						RenderGroupToOutput
						END_TIMED_BLOCK(DebugCycleCounter_RenderGroupToOutput);
					}


					DrawRectangleSlowly
					{
						BEGIN_TIMED_BLOCK(DebugCycleCounter_DrawRectangleSlowly);
						
						...
						...

						END_TIMED_BLOCK(DebugCycleCounter_DrawRectangleSlowly);
					}

and we found out the following numbers
					
				DEBUG CYCLE COUNTS 
					0:	316775452		DebugCycleCounter_GameUpdateAndRender
					1:	316048411		DebugCycleCounter_RenderGroupToOutput
					2:	313919964		DebugCycleCounter_DrawRectangleSlowly

as you can see that DebugCycleCounter_DrawRectangleSlowly is taking the bulk of the time 
but Casey says we dont know if that is b ecuz DebugCycleCounter_DrawRectangleSlowly is slow 
or if DebugCycleCounter_DrawRectangleSlowly is being called too many times 

which is why he added a HitCount variable in the debug_cycle_counter struct


				typedef struct debug_cycle_counter
				{    
				    uint64 CycleCount;
				    uint32 HitCount;
				} debug_cycle_counter;

and in the END_TIMED_BLOCK #define, we increment the hitCount

				#define BEGIN_TIMED_BLOCK(ID) uint64 StartCycleCount##ID = __rdtsc();
				#define END_TIMED_BLOCK(ID) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += __rdtsc() - StartCycleCount##ID; ++DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount;



37:01 
we added more pirntouts in the HandleDebugCycleCounters function

the number of cycles, and the cycles per hit 

%d: %I64ucy: [unsigned integer 64] "cy"			cycles
%uh: [unsigned int] "h"  						hit 
%I64ucy/h: [unsigned integer 64] "cy/h"			cycles per hit 

				win32_handmade.cpp

				internal void
				HandleDebugCycleCounters(game_memory *Memory)
				{
				#if HANDMADE_INTERNAL
				    OutputDebugStringA("DEBUG CYCLE COUNTS:\n");
				    for(int CounterIndex = 0;
				        CounterIndex < ArrayCount(Memory->Counters);
				        ++CounterIndex)
				    {
				        debug_cycle_counter *Counter = Memory->Counters + CounterIndex;

				        if(Counter->HitCount)
				        {
				            char TextBuffer[256];
				            _snprintf_s(TextBuffer, sizeof(TextBuffer),
				                        "  %d: %I64ucy %uh %I64ucy/h\n",
				                        CounterIndex,
				                        Counter->CycleCount,
				                        Counter->HitCount,
				                        Counter->CycleCount / Counter->HitCount);
				            OutputDebugStringA(TextBuffer);
				            Counter->HitCount = 0;
				            Counter->CycleCount = 0;
				        }
				    }
				#endif
				}


38:40
and we found out that it is not becuz it is being called too many times,
but becuz our DrawRectangleSlowly is just too slow, 






39:36
aims to add another tool to see how many pixels we are actually filling 

also added two more 

			    DebugCycleCounter_TestPixel,
			    DebugCycleCounter_FillPixel,

to see the pixel we are testing vs the pixel we are filling

				internal void
				DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
				                    loaded_bitmap *Texture, loaded_bitmap *NormalMap,
				                    environment_map *Top,
				                    environment_map *Middle,
				                    environment_map *Bottom,
				                    real32 PixelsToMeters)
				{
				    BEGIN_TIMED_BLOCK(DrawRectangleSlowly);

					...
					...

				    for(int Y = YMin; Y <= YMax; ++Y)
				    {
				        uint32 *Pixel = (uint32 *)Row; for(int X = XMin;  X <= XMax; ++X)
				        {
				            BEGIN_TIMED_BLOCK(TestPixel);

				            ...
				            ...

				            if((Edge0 < 0) && (Edge1 < 0) && (Edge2 < 0) && (Edge3 < 0))
				            {
				                BEGIN_TIMED_BLOCK(FillPixel);
								
								...
								...

				                END_TIMED_BLOCK(FillPixel);
				            }
							
				            ...
							...

				            END_TIMED_BLOCK(TestPixel);
				        }				        
				    }

				    END_TIMED_BLOCK(DrawRectangleSlowly);
				}




42:56
so we are testing 854662 pixels and we are filling 833006 pixels

854,662	-> tested 
our screen is 960 x 540 = 518,400 pixels

apparently according to Casey, as he compared these two numbers, we are overdrawing that bad...






47:16
so currently for the DebugCycleCounter_TestPixel case, we are doing 

				341728226 cy 854552h 399cy/h

that means we are spending 400 cycles per pixel.

Is that good or bad?


49:10
Casey mentioned that these are ballpark timings, rdtsc does not tell you exactly how fast something is 


52:29
Casey super handwavely, demonstarted how to estimate how expensive a piece a code is 

tallyed up all the number of operations done in a piece of code 

so in the analysis
Casey shows that we only need 200 cycles in total, and if we were to do 4 wide 
we only need 50 cycles 

50 vs 385 cycles

so we definitely need a lot of work to do 




1:02:19
new vs malloc in C++

pretty much the difference is whether you need C++ features. 

malloc allocates memory 

new is malloc + calling the constructor of whatever type you are instantiating 

same with delete and free 

if you are using destructors, you have to use delete 




