Handmade Hero Day 127 - Aligning Rendering Memory

Summary:

talked about that _mm_sfence(); is not needed. 
Described that _mm_sfence(); is only needed if you are writing to "Write-Combining memory".
Proceeds to explain what is "Write-Combining memory."

removed the code where we were shrinking the clipRect by 4 in width and height

mentioned in our multithreading context, all the alignment problems we face as we are doing 
pixels in batches of four in our tiles

showed us that we will have to do memory aligned writes and reads and also clip the front and back 
when rendering 

showed all the code and little details for making the code do memory aligned reads and writes. 

[Personally im slightly confused. He mentioned that we cant do unaligned memory reads and write becuz 
it might loads in extra pixels or write extra pixels. And when two cores read or write the same pixels,
we have no control over what vales gets written

In the aligned memory read/write version that he showed, do we not face the same "write" race condition? 
even though we are using a mask, are not we not just writing back the pixels with its "original values"?]



Keyword:
multithreading, rendering, memory aligned read/write

3:27
Casey showing that althought we got multithreading to work, there are borders on the tiles
that are not rendered correctly.


4:32
Casey said that his colleages at rad gaming tools confirmed that on x64 _mm_sfence(); are not necessary

                internal void
                Win32AddEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data)
                {
                    ...
                    ....

                    _WriteBarrier();
                    _mm_sfence();

                    ...
                }



5:53
The reason why _mm_sfence(); is not necessary is becuz of "Write-Combining memory"

on x64 there is no need to worry about writing ordering. Becuz writes are always ordered.
so whatever order it is the code, (NOTE after whatever the compiler outputs), it will be in that order.

recall that _WriteBarrier(); is for the compiler, so that is still needed.
_mm_sfence is for the processor.



7:32
the reason why _mm_sfence(); is even there since the writes are always in order is becuz, there are actually
many memory types. Not all memory is treated the same on the processor level. 

Usually the memory that the user obtains are usually follows strict write ordering.

There this other type of memory called "Write-Combining memory"

what that is that the hardware (such as GPU); has some memory on the GPU card

and the CPU has some memory mapped to the GPU memory

                                       CPU Memory                        GPU
                                     _______________                     _______________
        CPU                         |               |      _______      | GPU Memory    |
           --------------------->   |               |  <--|_______|-->  |_______________|
                                    |_______________|      PCI Bus      |               |
                                    |               |                   |               |
                                    |               |                   |               |
                                    |               |                   |               |
                                    |_______________|                   |_______________|


so whenever the CPU writes to some CPU Memory, the PCI will syc the CPU Memory and GPU Memory

what can happen sometimes is that if the CPU Memory has special constraints, such as a the one mentioned above 
where particular part of memory is mapped to GPU, then it has to write out cache lines in a specially considered way. 

(write out cache line here means, flushing the cache line out to memory or syncing the cache with the memory)

and complex stuff happens, cuz the CPU has to talk the PCI Bus or what not.

The gist of it is that CPU writes are much slower. so what it will do is try to combine writese to this memory, so that 
it does not have to flush the cache line out serially. 


Example:
assume we have cache line A and cache line B 

and assume the CPU writes to cache line 
        _______________________
    A  |_______1_________3_____|
    
        _______________________
    B  |____2__________________|

with Write-Combining memory, the CPU might wait for the cache line to fill up, and then flush them out to memory.

here the CPU might just flush cache line B before cache line A. In that situation value 2 will get into memory 
before 1 and 3.

so this is what _mm_sfence(); is for. This is only for "Write-Combining memory". Essentially,
the "Write-Combining memroy" may reorder writes for you, and we dont want that.

so in handmade hero, currently, we are not dealing with that kind of memory, and we dont have to worry about _mm_sfence();






13:14
Casey addresses one of the problem that was causing our tiles to not render correctly.

in the TiledRenderGroupToOutput(); function, we are shrinking the ClipRect(); by 4


                handmade_render_group.cpp

                internal void TiledRenderGroupToOutput(platform_work_queue *RenderQueue,
                                         render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {                    
                    ...
                    ...

                    for(int TileY = 0; TileY < TileCountY; ++TileY)
                    {
                        for(int TileX = 0; TileX < TileCountX; ++TileX)
                        {

                            ...

                            // TODO(casey): Buffers with overflow!!!
                            rectangle2i ClipRect;
                            ClipRect.MinX = TileX*TileWidth + 4;
                            ClipRect.MaxX = ClipRect.MinX + TileWidth - 4;
                            ClipRect.MinY = TileY*TileHeight + 4;
                            ClipRect.MaxY = ClipRect.MinY + TileHeight - 4;

                            ... create work to render inside the ClipRect ...
                            ...
                        }
                    }

                    PlatformCompleteAllWork(RenderQueue);
                }

13:48
we did the shrinking in the first place becuz in our rendering code, we are processing four pixels at time
(all that SIMD stuff);

so if our bitmap is not a multiple of 4, things get ugly. as you can see, if we get a bucket of 3, its gets
tricky.
                
                #### #### #### ###                
                ### #### #### ####

Casey says we are just gonna make sure that our bitmaps are always multiples of 4

the second thing is that we are storing and loading memory unaligned 


                internal void DrawRectangleQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                                     loaded_bitmap *Texture, real32 PixelsToMeters,
                                     rectangle2i ClipRect, bool32 Even)
                {
                    ...

                    for(int Y = MinY; Y < MaxY; Y += 2)
                    {
                        for(int XI = MinX; XI < MaxX; XI += 4)
                        { 
                            __m128i OriginalDest = _mm_loadu_si128((__m128i *)Pixel);

                            ...
                            ...

                            _mm_storeu_si128((__m128i *)Pixel, MaskedOut);

                        }
                    }
               }


16:56
even if our bitmap is multiples of 4, we also have to ensure that our tiles have to be multiples of 4
 
let say the our bitmap is multiples 4 (# being our pixels). but our tile boundaries doesnt end on the 4.
so we have 2 pixels in tile A, and 2 pixels tile B.
                
                 A         B
 
                       .   
                       .
                #### ##.## #### #### ####
                       .
                       .
                0123 45 67



the problem we may see is that our tiles could be done by two different threads. 

assume core 0 gets tile A and core 1 gets tile B

when we do the unaligned load with _mm_loadu_si128((__m128i *)Pixel);

                internal void
                DrawRectangleQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                                     loaded_bitmap *Texture, real32 PixelsToMeters,
                                     rectangle2i ClipRect, bool32 Even)
                {


                    for(int Y = MinY; Y < MaxY; Y += 2)
                    {
                        for(int XI = MinX; XI < MaxX; XI += 4)
                        { 
                            __m128i OriginalDest = _mm_loadu_si128((__m128i *)Pixel);

                            ...
                            ...

                            _mm_storeu_si128((__m128i *)Pixel, MaskedOut);

                        }
                    }
               }


what might happen is that, when core 0 gets the pixel 4 and 5, it might load pixels 4 5 6 7

then when core 1 comes, it may load 4 5 6 7 as well. 
they both will process it, and now they both will write back to it. if that happens, we get a race condition,
where one will overwrite the other. 




21:04
so the first thing we will do is get aligned tiles in TiledRenderGroupToOutput();



23:34
we have to make sure the TiledRenderGroupToOutput(); code handle any resolution the user specifies.
the user may have, let say 967

                            967
                 _______________________
                |                       |
                |                       |
                |                       |
                |                       |
                |                       |
                |_______________________|


recall in the TiledRenderGroupToOutput() function; we are doing a 4 x 4 tile count.

                internal void
                TiledRenderGroupToOutput(platform_work_queue *RenderQueue,
                                         render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {
                    int const TileCountX = 4;
                    int const TileCountY = 4;
              
                    ...
                    ...

                    int WorkCount = 0;
                    for(int TileY = 0; TileY < TileCountY; ++TileY)
                    {
                        for(int TileX = 0; TileX < TileCountX; ++TileX)
                        {
                            ...
                            ...
                        }
                    }
                }


so we try to get the smallest multiple of 4 that contains 967
(967 + 3) / 4 = 242.5

so our actual tile with will be 242.


then for each tile, we have to process it in batches of four pixels. 
242 / 4 = 60.5 bathces.

rounding it up, it will have to be 61 four pixel batches. 




so if we have 4 tiles, each tile has 61 four pixel batches,

so our plan is the first 3 tile is 61 x 4 = 244 pixels wide 
the last tile is 967 - 732 = 235 pixels wide

                                967 pixels 
                 ___________________________________________
                |  61 x 4  |  61 x 4  |  61 x 4  |  235     |
                |__________|__________|__________|__________|
                .                                .          .
                .............. 732 ................. 235 .... 
                .                                .          .

when we do the last tile, we will still be doing in batches of 4. so the the actual tile size is 

(235 + 3) / 4 = 59.5

so we will need a 59 * 4 = 236 tile.

we just need to make sure the buffer for the last tile supports 236 pixels 

in summary, the things we need take care of
-   each of the tile boundary happens on a 4 pixel boundary 
-   the file tile has enough space to overwrite 



29:57
Casey starting to write code 

-   first thing we need to do is to make sure that the output memory is already aligned to a four pixel boundary

    recall that in loaded_bitmap we have Memory 

                struct loaded_bitmap
                {
                    v2 AlignPercentage;
                    real32 WidthOverHeight;
                    
                    int32 Width;
                    int32 Height;
                    int32 Pitch;
                    void *Memory;
                };

    so in the TiledRenderGroupToOutput(); we add a line of assert to do that. 4 pixels is 16 bytes.
    so the and mask is 15. We are just looking at the bottom 15 bits.


                internal void TiledRenderGroupToOutput(platform_work_queue *RenderQueue,
                                         render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {
                    ...
                    ...

                    Assert(((uintptr)OutputTarget->Memory & 15) == 0);   
   
                }




31:21
next thing is that we need to makre sure TileWidth is multiples of 4.
We dont have to worry about TileHeight, cuz we only process batches 4 horizontally, not vertically.

and we just do some math calculation to get that value.

                internal void TiledRenderGroupToOutput(platform_work_queue *RenderQueue,
                                         render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {
                    ...
                    ...

                    Assert(((uintptr)OutputTarget->Memory & 15) == 0);   
                    int TileWidth = OutputTarget->Width / TileCountX;
                    int TileHeight = OutputTarget->Height / TileCountY;

                    TileWidth = ((TileWidth + 3) / 4) * 4;

                    ...
                    ...

                }





35:01
Then in the loop, we set the proper tile width for each tile.
so inside the for loop, we clamp it by the OutputTarget->Width.


                internal void TiledRenderGroupToOutput(platform_work_queue *RenderQueue,
                                         render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {

                    ...
                    ...

                    for(int TileY = 0; TileY < TileCountY; ++TileY)
                    {
                        for(int TileX = 0; TileX < TileCountX; ++TileX)
                        {
                            tile_render_work *Work = WorkArray + WorkCount++;

                            rectangle2i ClipRect;
                            ClipRect.MinX = TileX*TileWidth;
                            ClipRect.MaxX = ClipRect.MinX + TileWidth;
                            ClipRect.MinY = TileY*TileHeight;
                            ClipRect.MaxY = ClipRect.MinY + TileHeight;

                            if(TileX == (TileCountX - 1))
                            {
                                ClipRect.MaxX = OutputTarget->Width;
                            }
                            if(TileY == (TileCountY - 1))
                            {
                                ClipRect.MaxY = OutputTarget->Height;
                            }

                            ...
                            ...

                        }
                    }

                    ...
                    ...
                }







37:00 
Casey mentioned a problem we have to solve 

            A                                    B                                    
            
             __________                        _____________
            |          |                      |             |
            |          |                      |             |
            |    ______|______________________|_______      |
            | ##|# ####|                      |## ####|     |
            |___|______|                      |_______|_____|           
                |                                     |     
                |                                     |     
                |                                     |     
                |                                     |     
                |                                     |     
                |                                     |     
                |                                     |     
                |                                     |     
                |                                     |
                |_____________________________________|


Recall that whenever we render a bitmap, if it is not a multple of 4
we align in the back, and make the first few pixel render with a special mask,
kind of like th case below.

                ## #### #### ####

in situation B, that is fine,
but in situation A, we are rendering into out of bounds area, so we are not handling clipping in the front.


one solution is to allocate extra memory in the beginning. However, this presents a problem for us 
which is that, if we have a bitmap that ends on a tile boundary.

so if we have a bitmap that is 6 pixels long, the beginning of till will end on a tile boundary.


                 A         B
 
                       .   
                       .
                #### ##.## ####
                       .
                       .
                     01 23 4567


so the ultimate solution is that we do have to read and write aligned, and we Essentially have to mask 
both our front and our back. 





42:35
so now within the DrawRectangleQuickly(); code, we make our changes. Previously we had:
where the key thing we are doing is
1.  pushing FillRect.MinX forward so that it is at a multple of 4
2.  making the appropriate mask 

                handmade_render_group.cpp

                internal void
                DrawRectangleQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                                     loaded_bitmap *Texture, real32 PixelsToMeters,
                                     rectangle2i ClipRect, bool32 Even)
                {
                    ...
                    ...

                    FillRect = Intersect(ClipRect, FillRect);

                    ...
                    ...

                    if(HasArea(FillRect))
                    {
                        __m128i StartupClipMask = _mm_set1_epi8(-1);
                        int FillWidth = FillRect.MaxX - FillRect.MinX;
                        int FillWidthAlign = FillWidth & 3;
                        if(FillWidthAlign > 0)
                        {
                            int Adjustment = (4 - FillWidthAlign);
                            // TODO(casey): This is stupid.
                            switch(Adjustment)
                            {
                                case 1: {StartupClipMask = _mm_slli_si128(StartupClipMask, 1*4);} break;
                                case 2: {StartupClipMask = _mm_slli_si128(StartupClipMask, 2*4);} break;
                                case 3: {StartupClipMask = _mm_slli_si128(StartupClipMask, 3*4);} break;
                            }
                            FillWidth += Adjustment;
                            FillRect.MinX = FillRect.MaxX - FillWidth;
                        }
                 
                        ...
                        ...   
                    }
                }


now we also have to make the whole blip at the nearest boundary.
this will make us have a mask at the start and the end.

imagine the boundaries are like below 

                         ______________
                        |# #### #### ##|
                        |# #### #### ##|
                        |# #### #### ##|
                        |# #### #### ##|
                        ^              ^
                        |...FillRect...|
                    
                         ______________
                     ###|# #### #### ##|##
                     ###|# #### #### ##|##
                     ###|# #### #### ##|##
                     ###|# #### #### ##|##
                    ^                     ^
                    |.....FillRect........|
                    
                



now we have the following code 

-   recall from day 121, the mask layout is (actualy number of bits is not accurate);

    128 bits 

    pixel 3    pixel 2    pixel 1     pixel 0
    0000000000 0000000000 00000000000 000000000


-   so for StartClipMask we do shift left, so if you have to shift 1 pixel, 

    128 bits 

    pixel 3    pixel 2    pixel 1     pixel 0
    1111111111 1111111111 11111111111 00000000000


-   for EndClipMask, we do shift right 

    pixel 3    pixel 2    pixel 1     pixel 0
    0000000000 0000000000 00000000000 11111111111


-   previously we had a switch statement, here we just replace it with an array 


full code below 

                handmade_render_group.cpp

                internal void
                DrawRectangleQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                                     loaded_bitmap *Texture, real32 PixelsToMeters,
                                     rectangle2i ClipRect, bool32 Even)
                {

                    FillRect = Intersect(ClipRect, FillRect);

                    ...
                    ...

                    if(HasArea(FillRect))
                    {
                        __m128i StartClipMask = _mm_set1_epi8(-1);
                        __m128i EndClipMask = _mm_set1_epi8(-1);

                        __m128i StartClipMasks[] =
                        {
                            _mm_slli_si128(StartClipMask, 0*4),
                            _mm_slli_si128(StartClipMask, 1*4),
                            _mm_slli_si128(StartClipMask, 2*4),
                            _mm_slli_si128(StartClipMask, 3*4),            
                        };

                        __m128i EndClipMasks[] =
                        {
                            _mm_srli_si128(EndClipMask, 0*4),
                            _mm_srli_si128(EndClipMask, 3*4),
                            _mm_srli_si128(EndClipMask, 2*4),
                            _mm_srli_si128(EndClipMask, 1*4),            
                        };
                        
                        if(FillRect.MinX & 3)
                        {
                            StartClipMask = StartClipMasks[FillRect.MinX & 3];
                            FillRect.MinX = FillRect.MinX & ~3;
                        }

                        if(FillRect.MaxX & 3)
                        {
                            EndClipMask = EndClipMasks[FillRect.MaxX & 3];
                            FillRect.MaxX = (FillRect.MaxX & ~3) + 4;
                        }
                          
                    }

                }


49:24
at the end of every row, we have to set the appropriate if we need to put on the end mask.

we have XI + 8 becuz 
XI + 4 is for our next iteration

XI + 4 + 4 < MaxX is to see if, in our next iteration, whether our XI will be out of boundsd, hence 
being our last iteration 

                for(int Y = MinY; Y < MaxY; Y += 2)
                {
                    ...
                    ...

                    uint32 *Pixel = (uint32 *)Row;
                    for(int XI = MinX; XI < MaxX; XI += 4)
                    {
                        ...
                        ...

                        if((XI + 8) < MaxX)
                        {
                            ClipMask = _mm_set1_epi8(-1);
                        }
                        else
                        {
                            ClipMask = EndClipMask;
                        }
                    }
                }






50:53
Casey proceeds to check if we are aligned.
he does that by replacing _mm_storeu_si128 with _mm_store_si128, and 
                          _mm_loadu_si128 with _mm_load_si128


in the intrinsics guide, the specs for _mm_storeu_si128 says:
"Store 128-bits of integer data from a into memory. mem_addr does not need to be aligned on any particular boundary."


for specs for _mm_store_si128 says:
"Store 128-bits of integer data from a into memory. 
mem_addr must be aligned on a 16-byte boundary or a general-protection exception may be generated."

and we can see that our game doesnt crash.

https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=_mm_storeu_si128&expand=5619


what this means is that we have to ensure the destination buffer will always allows us 
to overwrite it by that much 








52:00
Casey wants to make sure that the platform layer is allocating memory aligned.

currently, the place we are allocating framebuffer is in the Win32ResizeDIBSection(); function

                win32_handmade.cpp

                internal void
                Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
                {
                    ...
                    ...

                    Buffer->Width = Width;
                    Buffer->Height = Height;

                    int BytesPerPixel = 4;
                    Buffer->BytesPerPixel = BytesPerPixel;

                    ...
                    ...

                    // NOTE(casey): Thank you to Chris Hecker of Spy Party fame
                    // for clarifying the deal with StretchDIBits and BitBlt!
                    // No more DC for us.
                    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
                    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
                    Buffer->Pitch = Width*BytesPerPixel;
                }


as you can see we dont do anything that guarantees the 16 byte alignment.
VirtualAlloc will always returns us memory that starts on a page boundary, which by definition will 
be aligned to 4k. 

For our pitch here, we have no guarantees that it will be aligned to 4 pixel boundaries.
(Pitch the number of bytes for each row.)

so we make a slight change. we align the Pitch calculation, rounding it to the next largest 16

{
                Buffer->Pitch = Align16(Width*BytesPerPixel);
}


the Align16(); function is in the handmade_platform.h file 


                handmade_platform.h 

                #define Align16(Value) ((Value + 15) & ~15)


~ has the effect of "flipping" bits


then we allocate memory based on the pitch. 

                Buffer->Pitch*Buffer->Height


full code below:
                {
                    ...
                    ...

                    Buffer->Pitch = Align16(Width*BytesPerPixel);
                    int BitmapMemorySize = (Buffer->Pitch*Buffer->Height);
                    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

                }




