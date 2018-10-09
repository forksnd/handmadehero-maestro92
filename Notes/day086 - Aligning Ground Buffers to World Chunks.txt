Handmade Hero Day 086 - Aligning Ground Buffers to World Chunks

Summary:

got rid of the concept of tiles in sim chunks

continuing the ground chunks concept from day085
Calculated Camera coverage, and initalize all the ground chunks touched by this Camera coverage 
the initalization of the ground chunks is done by calling FillGroundChunk(); where we define its splat patterns


Keyword:
rendering ground chunks 




7:37
for every place the camera can see, we need a ground chunk there
and this function will produce the ground chunks needed


26:32
wrote the code to render the GroundChunks
-	notice what we are doing here is that we are getting the camera center 
	and we define a bound around the camera 

	we get the screen width and screen height, then we create a rectangle3

-	we then map our screen rectangle3 to chunks space. Essentially we go through all the chunks that our camera touches

-	in the first iteration, Casey is just rendering the outline of each chunk


				handmade.cpp		    

			    extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{   
					...
					...


				    v2 ScreenCenter = {0.5f*(real32)DrawBuffer->Width,
				                       0.5f*(real32)DrawBuffer->Height};

				    real32 ScreenWidthInMeters = DrawBuffer->Width*PixelsToMeters;
				    real32 ScreenHeightInMeters = DrawBuffer->Height*PixelsToMeters;
				    rectangle3 CameraBoundsInMeters = RectCenterDim(V3(0, 0, 0),
				                                                    V3(ScreenWidthInMeters, ScreenHeightInMeters, 0.0f));

				    {
				        world_position MinChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMinCorner(CameraBoundsInMeters));
				        world_position MaxChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMaxCorner(CameraBoundsInMeters));

				        for(int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ)
				        {
				            for(int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
				            {
				                for(int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
				                {
				                    {
				                        world_position ChunkCenterP = CenteredChunkPoint(ChunkX, ChunkY, ChunkZ);
				                        v3 RelP = Subtract(World, &ChunkCenterP, &GameState->CameraP);
				                        v2 ScreenP = {ScreenCenter.X + MetersToPixels*RelP.X,
				                                      ScreenCenter.Y - MetersToPixels*RelP.Y};
				                        v2 ScreenDim = MetersToPixels*World->ChunkDimInMeters.XY;

				                        ...
				                        ...

				                        DrawRectangleOutline(DrawBuffer, ScreenP - 0.5f*ScreenDim, ScreenP + 0.5f*ScreenDim, V3(1.0f, 1.0f, 0.0f));
				                    }
				                }
				            }
				        }
				    }

				    ...
				    ...

				}









59:28
so previously we are rendering ground outlines, then we change it so that we allocate the chunks 
if we need it while rendering. 

-	notice that we are doing a linear search in our transient memory to find out if we have this ground chunk.
	Casey says it is incredibly inefficient so this wont be in the final code

-	if we have an EmptyBuffer, then we call FillGroundChunk on the spot

-	of course with this code, we would eventually run out space on the transient memory since we are 
	not evicting any chunks.

                bool32 Found = false;
                ground_buffer *EmptyBuffer = 0;
                for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; ++GroundBufferIndex)
                {
                    ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
                    if(AreInSameChunk(World, &GroundBuffer->P, &ChunkCenterP))
                    {
                        Found = true;
                        break;
                    }
                    else if(!IsValid(GroundBuffer->P))
                    {
                        EmptyBuffer = GroundBuffer;
                    }
                }

                if(!Found && EmptyBuffer)
                {
                    FillGroundChunk(TranState, GameState, EmptyBuffer, &ChunkCenterP);
                }



-	full code below 

				handmade.cpp		    

			    extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
				{   
					...
					...


				    v2 ScreenCenter = {0.5f*(real32)DrawBuffer->Width,
				                       0.5f*(real32)DrawBuffer->Height};

				    real32 ScreenWidthInMeters = DrawBuffer->Width*PixelsToMeters;
				    real32 ScreenHeightInMeters = DrawBuffer->Height*PixelsToMeters;
				    rectangle3 CameraBoundsInMeters = RectCenterDim(V3(0, 0, 0),
				                                                    V3(ScreenWidthInMeters, ScreenHeightInMeters, 0.0f));

				    {
				        world_position MinChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMinCorner(CameraBoundsInMeters));
				        world_position MaxChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMaxCorner(CameraBoundsInMeters));

				        for(int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ)
				        {
				            for(int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
				            {
				                for(int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
				                {
				                    {
				                        world_position ChunkCenterP = CenteredChunkPoint(ChunkX, ChunkY, ChunkZ);
				                        v3 RelP = Subtract(World, &ChunkCenterP, &GameState->CameraP);
				                        v2 ScreenP = {ScreenCenter.X + MetersToPixels*RelP.X,
				                                      ScreenCenter.Y - MetersToPixels*RelP.Y};
				                        v2 ScreenDim = MetersToPixels*World->ChunkDimInMeters.XY;


				                        // TODO(casey): This is super inefficient fix it tomorrrow!
				                        bool32 Found = false;
				                        ground_buffer *EmptyBuffer = 0;
				                        for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; ++GroundBufferIndex)
				                        {
				                            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
				                            if(AreInSameChunk(World, &GroundBuffer->P, &ChunkCenterP))
				                            {
				                                Found = true;
				                                break;
				                            }
				                            else if(!IsValid(GroundBuffer->P))
				                            {
				                                EmptyBuffer = GroundBuffer;
				                            }
				                        }

				                        if(!Found && EmptyBuffer)
				                        {
				                            FillGroundChunk(TranState, GameState, EmptyBuffer, &ChunkCenterP);
				                        }
				                        
				                        DrawRectangleOutline(DrawBuffer, ScreenP - 0.5f*ScreenDim, ScreenP + 0.5f*ScreenDim, V3(1.0f, 1.0f, 0.0f));
				                    }
				                }
				            }
				        }
				    }

				    ...
				    ...

				}




1:20:51

the problem with deferred shading is that you have to do alot to get transparency to work well

