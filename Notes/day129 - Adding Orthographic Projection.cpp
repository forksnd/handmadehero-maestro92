Handmade Hero Day 129 - Adding Orthographic Projection
Summary:
Added Orthographic projection for rendering ground chunks.

made the other entities render in Perspective mode.

discussed the difference between Tiled GPU and non-Tiled GPU in Q/A

Keyword:
renderer, Orthographic Projection


1:43
plans to add Orthographic projection for rendering ground chunks. 

3:09
right now our ground chunks are incorrect again. That is becuz is that 
we are rendering all the ground chunks in Perspective Projection

what we used to do is that we draw neighboring tiles. so if a splat is on the border,
you can capture that in A. This was all done in Orthographic Projection.

             ______________
            |    |    |    |
            |____|____|____|
            |    | A  |    |
            |____|____|____|
            |    |    |    |
            |____|____|____|



17:49
Casey changing the FillGroundChunk(); function to render in Orthographic(); mode.


                handmade.cpp

                internal void FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
                {
                    ...
                    ...
                    
                    // TODO(casey): Decide what our pushbuffer size is!
                    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4));
                    Orthographic(RenderGroup, Buffer->Width, Buffer->Height, Buffer->Width / Width);
                    Clear(RenderGroup, V4(1.0f, 0.0f, 1.0f, 1.0f));
                    
                    for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
                    {
                        for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
                        {
                            ...
                            ...
                        }
                    }

                    ...
                    ...
                }


19:40
in other places we render in Perspective mode

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    ...
                    ...
                    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels, 0.6f, 9.0f);

                    ...
                    ...

                }







21:16
adding the Perspective and Orthographic function in handmade_render_group.cpp

                inline void
                Perspective(render_group *RenderGroup, int32 PixelWidth, int32 PixelHeight,
                            real32 MetersToPixels, real32 FocalLength, real32 DistanceAboveTarget)
                {
                    // TODO(casey): Need to adjust this based on buffer size
                    real32 PixelsToMeters = SafeRatio1(1.0f, MetersToPixels);

                    RenderGroup->MonitorHalfDimInMeters = {0.5f*PixelWidth*PixelsToMeters,
                                                           0.5f*PixelHeight*PixelsToMeters};
                    
                    RenderGroup->Transform.MetersToPixels = MetersToPixels;
                    RenderGroup->Transform.FocalLength =  FocalLength; // NOTE(casey): Meters the person is sitting from their monitor
                    RenderGroup->Transform.DistanceAboveTarget = DistanceAboveTarget;
                    RenderGroup->Transform.ScreenCenter = V2(0.5f*PixelWidth, 0.5f*PixelHeight);

                    RenderGroup->Transform.Orthographic = false;
                }

                inline void
                Orthographic(render_group *RenderGroup, int32 PixelWidth, int32 PixelHeight, real32 MetersToPixels)
                {
                    real32 PixelsToMeters = SafeRatio1(1.0f, MetersToPixels);
                    RenderGroup->MonitorHalfDimInMeters = {0.5f*PixelWidth*PixelsToMeters,
                                                           0.5f*PixelHeight*PixelsToMeters};
                    
                    RenderGroup->Transform.MetersToPixels = MetersToPixels;
                    RenderGroup->Transform.FocalLength =  1.0f; // NOTE(casey): Meters the person is sitting from their monitor
                    RenderGroup->Transform.DistanceAboveTarget = 1.0f;
                    RenderGroup->Transform.ScreenCenter = V2(0.5f*PixelWidth, 0.5f*PixelHeight);

                    RenderGroup->Transform.Orthographic = true;
                }








31:37
in the GetRenderEntityBasisP(); function, we calculate the ground point depending if we are Orthographic or not 
              
                handmade_render_group.cpp

                inline entity_basis_p_result GetRenderEntityBasisP(render_transform *Transform, v3 OriginalP)
                {
                    entity_basis_p_result Result = {};

                    v3 P = V3(OriginalP.xy, 0.0f) + Transform->OffsetP;

                    if(Transform->Orthographic)
                    {
                        Result.P = Transform->ScreenCenter + Transform->MetersToPixels*P.xy;
                        Result.Scale = Transform->MetersToPixels;
                        Result.Valid = true;
                    }
                    else
                    {
                        real32 OffsetZ = 0.0f;
                    
                        real32 DistanceAboveTarget = Transform->DistanceAboveTarget;

                        ...
                        ...

                    
                        real32 DistanceToPZ = (DistanceAboveTarget - P.z);
                        real32 NearClipPlane = 0.2f;
                    
                        v3 RawXY = V3(P.xy, 1.0f);

                        if(DistanceToPZ > NearClipPlane)
                        {
                            v3 ProjectedXY = (1.0f / DistanceToPZ) * Transform->FocalLength*RawXY;        
                            Result.Scale = Transform->MetersToPixels*ProjectedXY.z;
                            Result.P = Transform->ScreenCenter + Transform->MetersToPixels*ProjectedXY.xy + V2(0.0f, Result.Scale*OffsetZ);
                            Result.Valid = true;
                        }
                    }
                    
                    return(Result);
                }


55:27
got it to render, but the seems are incorrect. However Casey had to go to the Q/A



1:06:35
There are two different kinds of GPU, Tiled and non-Tiled

we are doing SIMD, so we are doing 4 wide 

                 _______________
                |   |   |   |   |
                |___|___|___|___|


GPU are usually at least 16 wide 

                 _______________
                |   |   |   |   |
                |___|___|___|___|
                |   |   |   |   |
                |___|___|___|___|
                |   |   |   |   |
                |___|___|___|___|
                |   |   |   |   |
                |___|___|___|___|



THe non-Tiled architecture, Casey claims hes not familiar
pretty much it rasterizes a triangle on screen. 

THe Tiled architecture, is very similar to what we do. 

The Tiled GPU are usually on Mobile and the non-Tiled are usually on desktop
