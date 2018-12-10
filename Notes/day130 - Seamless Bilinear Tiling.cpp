Handmade Hero Day 130 - Seamless Bilinear Tiling
Summary:
Explain what causes the seams to appear.

Explain why we would need a pixel of borders around our textures.
(as a part of the solution to remove seams);

went on to remove the seams 

Keyword:
subpixel accurate, renderering, anti-aliasing


2:25
Casey showed that there is currently seams among the tiles.

Casey says we dont actually know the source of these seams. Possibilities:
1.  source art. Its possible that they just dont have smooth alpha.
2.  Code that blits tiles to screen. 
3.  not layering in splats properly.


5:22
Casey pointed out that something unusual: it seems that in between tiles, one tiles entire alpha values 
are just one level lower than the other.


[Actually if you pause the video at 6:14], one of the grass leaf is clearly cut off..
so I suspect that its just not splatting correctly?]



10:07
Casey mentioned when thing we were not doing correctly that will definitely contribute to the the seam 

recall in the rendering function, to avoid the edge case of sampling out of bounds when doing 
bilinear filtering on edge pixels, we do the following: (in both functions);


                handmade_render_group.cpp


                DrawRectangleSlowly()
                {
                    ...
                    ...
                    if((Edge0 < 0) && (Edge1 < 0) && (Edge2 < 0) && (Edge3 < 0))
                    {
                        real32 tX = ((U*(real32)(Texture->Width - 2)));
                        real32 tY = ((V*(real32)(Texture->Height - 2)));
                        
                        ...
                        ...
                    }
                    ...
                    ...
                }


                DrawRectangleQuickly()
                {

                    ...
                    ...

                    __m128 WidthM2 = _mm_set1_ps((real32)(Texture->Width - 2));
                    __m128 HeightM2 = _mm_set1_ps((real32)(Texture->Height - 2));
    
                }


10:40 
Recall that we werent seeing these artifacts previously. 
Casey goes on to explain why do we see seams with this approach when we are rendering in tiles.

image we have two ground chunk, Tile0 and Tile1, when

a few things we need to take care of 
1.  the color values between A and B has to look smooth, since they belong to different tiles 
    same goes for C and D 

2.  also when we render, pixels on the edge, lets say a pixel value lands inside the 4 texels of A, B, C and D ,
    and you have to bilinear lerp that pixel. The problem is Texel B and D exists on the different bitmap.
    so Tile0 somehow needs to know about Tile1s bitmap


[even if we were to read it for free, we also run in race condition issues. We have to wait for B to finish drawing 
 to draw A, B has to wait for A to finish to draw itself]

                            .
                Tile0       .       Tile1
                            . 
                 _______________________
                |   |   |   |   |   |   |
                |___|___|___|___|___|___|
                |   |   | A | B |   |   |
                |___|___|___|___|___|___|
                |   |   | C | D |   |   |
                |___|___|___|___|___|___|
                            .
                            .
                            .
                            .


15:18
Casey proposes a solution: instead of doing texture mapping on a full texture, 
we can just have a extra pixel of boundaries on the textures

so if we get a 4 x 4 texuture bitmap, we interpret it as 6 x 6



                 ___________________
                | A  | A  | A  | A  |
                |____|____|____|____|
                | A  | A  | A  | A  |
                |____|____|____|____|
                | A  | A  | A  | A  |
                |____|____|____|____|
                | A  | A  | A  | A  |
                |____|____|____|____|



             _____________________________
            |    |    |    |    |    |    |
            |____|____|____|____|____|____|
            |    | A  | A  | A  | A  |    |
            |____|____|____|____|____|____|
            |    | A  | A  | A  | A  |    |
            |____|____|____|____|____|____|
            |    | A  | A  | A  | A  |    |
            |____|____|____|____|____|____|
            |    | A  | A  | A  | A  |    |
            |____|____|____|____|____|____|
            |    |    |    |    |    |    |
            |____|____|____|____|____|____|
            
then for the borders, we store the adjoining values 


16:232
This also works nicely for us to get proper anti-aliasing for our sprites, we want a one pixel border around them 
that is filled with nothing, so that it always bilinear out to 0. That way we are always fetching into the texture, 
and we dont have any hard borders.



18:40
we want to re-think our UV coordinates of pixels.
essentially we want to design how pixel UV values work in our game
for example, if the top and bottom corner get UV, 0,0 and 1,1                           


                 ___________________
                |    |    |    | 1,1|
                |____|____|____|____|
                |    |    |    |    |
                |____|____|____|____|
                |    |    |    |    |
                |____|____|____|____|
                | 0,0|    |    |    | 
                |____|____|____|____|


assume we have the texels below, which point does UV 0,0 map to, (a or b)?


                 _______________________________            
                |       |       |       |       |   
                |       |       |       |       |
                |       |  1,1  |       |       |
                |_______b_______|_______|_______|
                |       |       |       |       |   
                |   a   |       |       |       |
                |  0,0  |       |       |       |
                |_______|_______|_______|_______|


if it maps to a, pixel 0,0 will just get texel [0,0]

if it maps to b, pixel 0,0, will the an even bilinear interpolation of texel [0,0], [0,1], [1,0] and [1,1]




24:08
decides that the bottom pixel[0,0] will map to point b, the version that has an even bilinear blend of A,B,C,D 

pixels
                 ___________________
                |    |    |    | u1,|
                |____|____|____|_v1_|
                |    |    |    |    |
                |____|____|____|____|
                |    |    |    |    |
                |____|____|____|____|
                | u0,|    |    |    | 
                |_v0_|____|____|____|


Texels
             _____________________________ 
            |    |    |    |    |    | w-1|
            |____|____|____|____|____|_h-1|
            |    |    |    |    | w-2|    |
            |____|____|____|____|_h-2|____|
            |    |    |    |    |    |    |
            |____|____|____|____|____|____|
            |    |    |    |    |    |    |
            |____|____|____|____|____|____|
            | A  | B  |    |    |    |    |
            |____|____|____|____|____|____|
            | C  | D  |    |    |    |    |
            |____|____|____|____|____|____|
           
so assuming center of texel C is [0,0], and center of texel B is [1, 1]

u0, v0 will just be 0.5, 0.5, 
or [0.5/w, 0.5/h], if you care to normalize so that UV is between [0,1]


u1, v1 will just be [w-2, h-2] + [0.5, 0.5]
or [ (w-2+0.5)/w, (h-2+0.5)/h], if you care to normalize so that UV is between [0,1]

so what we will do is to just offset it by 0.5.


29:56
now we have the code. you can see tX, tY is offset with Half.

{ 
                __m128 U = _mm_add_ps(_mm_mul_ps(PixelPx, nXAxisx_4x), PynX);
                __m128 V = _mm_add_ps(_mm_mul_ps(PixelPx, nYAxisx_4x), PynY);

                ...
                ...

                U = _mm_min_ps(_mm_max_ps(U, Zero), One);
                V = _mm_min_ps(_mm_max_ps(V, Zero), One);

                // NOTE(casey): Bias texture coordinates to start
                // on the boundary between the 0,0 and 1,1 pixels.
                __m128 tX = _mm_add_ps(_mm_mul_ps(U, WidthM2), Half);
                __m128 tY = _mm_add_ps(_mm_mul_ps(V, HeightM2), Half);

}





31:43
In the FillGroundChunk function, we will shrink the orthographic size so that we have a layer of boundaries around
as you can see, for the Orthographic function call, we have Orthographic(RenderGroup, Buffer->Width, Buffer->Height, (Buffer->Width - 2) / Width);

                internal void
                FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
                {
                    ...
                    ...

                    real32 Width = GameState->World->ChunkDimInMeters.x;
                    real32 Height = GameState->World->ChunkDimInMeters.y;

                    v2 HalfDim = 0.5f*V2(Width, Height);
                    
                    // TODO(casey): Decide what our pushbuffer size is!
                    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4));
                    Orthographic(RenderGroup, Buffer->Width, Buffer->Height, (Buffer->Width - 2) / Width);

                    ...
                    ...

                    for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
                    {
                        for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
                        {
                            ...
                            ...

                        }
                    }
that seemed to fix the seam

[what confuses me is that it doesnt seem like we fill adjacent edge boundary cell colors anywhere... so no idea what happened to that]

43:17
Casey plans to fill the ground chunk on a separate thread.


45:49
we added a low priority queue in the game_memory

                typedef struct game_memory
                {
                    bool32 IsInitialized;

                    ...
                    ...

                    platform_work_queue *HighPriorityQueue;
                    platform_work_queue *LowPriorityQueue;

                    ...
                    ...

                } game_memory;


46:59
one way we can manage the threads is that we can have the threads pull off of the HighPriorityQueue
if it is not empty, then the LowPriorityQueue if the HighPriorityQueue is empty.

the problem is that if we have things that take a long time on the HighPriorityQueue, they will starve out the 
HighPriorityQueue queue potentially


49:09
now in our setup code we make two queues, 6 slots on the HighPriorityQueue and 2 slots on the LowPriorityQueue

                win32_handmade.cpp

                int CALLBACK
                WinMain(HINSTANCE Instance,
                        HINSTANCE PrevInstance,
                        LPSTR CommandLine,
                        int ShowCode)
                {
                    win32_state Win32State = {};

                    platform_work_queue HighPriorityQueue = {};
                    Win32MakeQueue(&HighPriorityQueue, 6);
                    
                    platform_work_queue LowPriorityQueue = {};
                    Win32MakeQueue(&LowPriorityQueue, 2);

                    ...
                    ...
                }


51:10
then in the FillGroundChunk function, we just let the LowPriorityQueue do the job


                internal void
                FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
                {
                    ...
                    ...

                    for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
                    {
                        for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
                        {
                            ...
                            ...
                        }
                    }
 
                    TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, Buffer);
                    ...
                }


1:06:02
how are you deciding whether something gets queued into HighPriorityQueue or LowPriorityQueue?

anything that needs to be computed this frame is in the HighPriorityQueue.
anything that can be computered across a couple of frames is gonna be in LowPriorityQueue


