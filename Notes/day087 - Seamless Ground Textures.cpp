Handmade Hero Day 087 - Seamless Ground Textures

Summary:
making ground textures seamless. Did so by splat an entire 3 x 3 ground chunks, and only take the middle one

reusing ground chunks for rendering ground 

explained why arent our blitting scheme are not sub pixel accurate now in the Q/A

Keyword:
rendering ground chunks 



10:56
explaining how we generate ground textures now


11:55
solution:
splat an entire 3x3 and only take the middle part

so every time we call FillGroundChunk, the "ground_buffer* GroundBuffer" that got passed in is just the center one of these 3 x 3

				 _______________________			
				|		|		|		|
				|		|		|		|
				|		|		|   	|	
				|_______|_______|_______|
				|		|		|		|	
				|		|	A	| 		|
				|		|		| 		|
				|_______|_______|_______|
				|		|		|		|	
				|		|		| 		|
				|		|		| 		|
				|_______|_______|_______|



when we render, we would render to the Bitmap that is representing chunk A. 

let say one of the splats rendered to chunk A_s Bitmap is in the following situation.
where half of it is out side. 

the DrawBitmap(); takes care of that. Bounds checking is done inside that function. 

			 _______		
			|	 ___|___________
			|	|	|			|	
			|	|	|			|	
			|___|___|			|	
				|				|	
				|				|	
				|		 		|
				|				|
				|_______________|





full code below:

				internal void
				FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
				{
				    loaded_bitmap Buffer = TranState->GroundBitmapTemplate;
				    Buffer.Memory = GroundBuffer->Memory;

				    GroundBuffer->P = *ChunkP;

				    real32 Width = (real32)Buffer.Width;
				    real32 Height = (real32)Buffer.Height;

				    for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
				    {
				        for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
				        {
				        	...
				        	...
				           
				            random_series Series = RandomSeed(139*ChunkX + 593*ChunkY + 329*ChunkZ);

				            v2 Center = V2(ChunkOffsetX*Width, -ChunkOffsetY*Height);

				            for(uint32 GrassIndex = 0; GrassIndex < 100; ++GrassIndex)
				            {
				            	...
				            	...

				                DrawBitmap(&Buffer, Stamp, P.X, P.Y);
				            }
				        }
				    }

				    ...
				    ...

				}



30:08
starts to work on evicting memory of old ground chunks


30:35
reduced the GroundChunkBufferCount temporary so that we immediately run out of new ground chunks



31:55
instead of implementing a LRU scheme, we dont quite know how our engine will look in the end. So putting those fancy scheme
may be premature. So Casey is just gonna brute force it to evict the one that is furthest from the camera

-	so the three conditions we have here is 
				
				if(AreInSameChunk(World, &GroundBuffer->P, &ChunkCenterP))

	if we are in the same chunk and a valid one, then we use this current one. 

-	then in the "else if(IsValid(GroundBuffer->P))" case, we reuse the one that is furthest away


-	otherwise in the "else" case, it means this is an empty one, so we just use it right away.
we do that by setting it to Real32Maximum


				handmade.cpp

		        for(int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ)
		        {
		            for(int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
		            {
		                for(int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
		                {
		                    {
		                    	...
		                    	...

		                        // TODO(casey): This is super inefficient fix it!
		                        real32 FurthestBufferLengthSq = 0.0f;
		                        ground_buffer *FurthestBuffer = 0;
		                        for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; ++GroundBufferIndex)
		                        {
		                            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
		                            if(AreInSameChunk(World, &GroundBuffer->P, &ChunkCenterP))
		                            {
		                                FurthestBuffer = 0;
		                                break;
		                            }
		                            else if(IsValid(GroundBuffer->P))
		                            {
		                                v3 RelP = Subtract(World, &GroundBuffer->P, &GameState->CameraP);
		                                real32 BufferLengthSq = LengthSq(RelP.XY);
		                                if(FurthestBufferLengthSq < BufferLengthSq)
		                                {
		                                    FurthestBufferLengthSq = BufferLengthSq;
		                                    FurthestBuffer = GroundBuffer;
		                                }
		                            }
		                            else
		                            {
		                                FurthestBufferLengthSq = Real32Maximum;
		                                FurthestBuffer = GroundBuffer;
		                            }
		                        }

		                        if(FurthestBuffer)
		                        {
		                            FillGroundChunk(TranState, GameState, FurthestBuffer, &ChunkCenterP);
		                        }

		#if 0
		                        DrawRectangleOutline(DrawBuffer, ScreenP - 0.5f*ScreenDim, ScreenP + 0.5f*ScreenDim, V3(1.0f, 1.0f, 0.0f));
		#endif
		                        
		                    }
		                }
		            }
		        }
		    }


35:12
defines the Real32Maximum.
In this case the actual maximum value is defined by the <float.h> C compiler
<float.h> is like synthetic header that the compiler is required to provide for the platform

if you want integer limits, then you #include <limits.h>

				handmade_platform.h

				#define Real32Maximum



42:07
notice there_s some visual defect on the ground. that is becuz on the edges of our ground chunks 
we always have splats from 2 layers of ground overlapping each other. The grass splats from one chunk 
is being overwritten by the ground splats of the other chunk


the fix is that we split the "FillGroundChunk" code into two loops. We first render all the ground, grass patch splats,
then we do all the tuft splats.

Note
Grass patch splats are the green regions on the ground
Tuft splats are the little small grass that comes out of the ground

				internal void
				FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
				{
				    loaded_bitmap Buffer = TranState->GroundBitmapTemplate;
				    Buffer.Memory = GroundBuffer->Memory;

				    GroundBuffer->P = *ChunkP;

				    real32 Width = (real32)Buffer.Width;
				    real32 Height = (real32)Buffer.Height;

				    for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
				    {
				        for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
				        {
				        	...
				        	...

				            for(uint32 GrassIndex = 0; GrassIndex < 100; ++GrassIndex)
				            {
				            	...
				            	...
				            	render grass patches

				                DrawBitmap(&Buffer, Stamp, P.X, P.Y);
				            }
				        }
				    }

				    for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
				    {
				        for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
				        {
				        	...
				        	...

				            for(uint32 GrassIndex = 0; GrassIndex < 50; ++GrassIndex)
				            {

				            	...
				            	...
				            	render tuft splats

				                DrawBitmap(&Buffer, Stamp, P.X, P.Y);
				            }
				        }
				    }
				}



45:47
added some code where whenever the game code is reloaded, we flush out the ground splats 


			    if(Input->ExecutableReloaded)
			    {
			        for(uint32 GroundBufferIndex = 0;
			            GroundBufferIndex < TranState->GroundBufferCount;
			            ++GroundBufferIndex)
			        {
			            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
			            GroundBuffer->P = NullPosition();            
			        }        
			    }
			    
Casey demonstrated how this is useful with the live code editing feature

58:08
why are the trees wiggling?

since we do not have a renderer, we are not subpixel accurate. The reason why the trees are wiggling is becuz
they are snapping to integer coordinates. They are floating point positioned, but they are snapping to integer coordinates

so when one round up to the next integer coordiante, but the other one hasnt yet, you see their distance changes a bit.

In our renderer, we will fix this issue, cuz we will be sub pixel accurate. you will get a nice smooth interpolation there 




1:07:47
now most machines have 2 or 4 cores, so you want to off load some work to the background


