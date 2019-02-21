Handmade Hero Day 229 - Sorting Render Elements

Summary:
mentioned the concept of sort buffer, used in sorting algorithms

added our tile_sort_entry table at the end of our PushBuffer. 

         _________________   <------------ our push buffer
        | render element  |
        |_________________|
        | render element  |
        |_________________|
        |                 |
        |                 |
        |                 |
        |                 |
        | key      index  |
        |_________________| 
        |        |        |
        |________|________|
        |        |        |
        |________|________| 
        |        |        |
        |________|________|  <----------- our sort buffer 

implement a very easy sorting metric, using the z and y position. 
mentioned that since our game is in 2.5 d world, there is no perfect formula. 
as a first draft, implemented bubble sort for the render elements 

Keyword:
sorting, rendering, bubble sort



13:23
So Casey mentioned a thing about sorting 

currently we want to sort the render entries in our tile_render_work struct 

so if we were to sort this, we might have to copy

for instance, lets say for one of the steps, we sorted from the Push Buffer on the left to the Push Buffer on the right

        Push Buffer
         ___________             ___________
        |     A     |           |     B     |               
        |___________|           |___________|
        |     C     |           |     A     |
        |___________|           |___________|  
        |     B     |  ------>  |     C     |
        |___________|           |___________|
        |           |           |           |
        |___________|           |___________|
        |           |           |           |
        |___________|           |___________|

so this may involve copying push buffer entries around, and that may be a very expensive operation.
so what we may want to do is to only do the copying at the final step


so what we have to do, and how sorting typically works is that we have a sort buffer 
and it has the minimal amount of information that is needed to execute the sort properly

typically what that is is two things 

                Key         Index 
             _______________________ 
            |           |           |
            |___________|___________|
            |           |           |
            |___________|___________|
            |           |           |
            |___________|___________|
            |           |           |
            |___________|___________|
            |           |           |
            |___________|___________|


Key is the thing we are comparing to sort 
Index is just index in our array, something that allows us to access our original data.

so Index is just the offset into our push buffer
that will correspond to the "PushBufferOffset" variable in our tile_sort_entry struct 

the "SortKey" obviously corresponds to the Key in the sort buffer

                handmade_render_group.h

                struct tile_sort_entry
                {
                    r32 SortKey;
                    u32 PushBufferOffset;
                };


so what we will do is to produce this sort buffer. 
our tile_sort_entry is 64 bit

                Key         Index 
             _______________________ 
            |    32     |    32     |
            |___________|___________|
            |           |           |
            |___________|___________|
            |           |           |
            |___________|___________|
            |           |           |
            |___________|___________|
            |           |           |
            |___________|___________|


15:29
so in our case, the SortKey would have been easy if we just take the z value. however,
we have this 2.5 D nonsense, so we have to figure something out.

so we have a z value, 

[in game 2, we just use the formula of 

+x, -y is more towards the camera 
-x, y is away from the camera]



so for use we want a formula that mixes z and y 

    y

    ^
    |    _______________________
    |   |                       |
    |   |                       |
    |   |                       |
    |   |                       |
    |   |_______________________| 
    |
    ------------------------------>
                    x

in 2.5 d land, there is no perfect answer



21:17
so what we have now is we iterate the our render_entry inside the render_group in the sorted order 

so previously we are iterating through the PushBuffer by the size 

                internal void
                RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget, rectangle2i ClipRect)
                {
                    ...
                    
                    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize; )
                    {
                        render_group_entry_header *Header = (render_group_entry_header *)
                            (RenderGroup->PushBufferBase + BaseAddress);
                        BaseAddress += sizeof(*Header);
                        
                        ...
                        ...   

                    }
                }

now we have we will iterate by the sorted order. 
we will be accessing specific entries by doing 

                render_group_entry_header *Header = (render_group_entry_header *)
                    (RenderGroup->PushBufferBase + Entry->PushBufferOffset);

So we will be jumping around.

                handmade_render_group.cpp

                internal void RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget, rectangle2i ClipRect)
                {
                    ...

                    u32 SortEntryCount = RenderGroup->PushBufferElementCount;
                    tile_sort_entry *SortEntries = (tile_sort_entry *)(RenderGroup->PushBufferBase + RenderGroup->SortEntryAt);
                    
                    real32 NullPixelsToMeters = 1.0f;

                    tile_sort_entry *Entry = SortEntries;
                    for(u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
                    {
                        render_group_entry_header *Header = (render_group_entry_header *)
                            (RenderGroup->PushBufferBase + Entry->PushBufferOffset);
                        
                        void *Data = (uint8 *)Header + sizeof(*Header);
    
                        ...
                        ...
                    }
                }


30:20
so what Casey is gonna do is to have two stacks

assume we have our PushBuffer stack, 

         _________________   <------------ our push buffer, with all of our render elements 
        | render element  |
        |_________________|
        | render element  |
        |_________________|
        |                 |
        |                 |
        |                 |
        |                 |
        | key      index  |
        |_________________| 
        |        |        |
        |________|________|
        |        |        |
        |________|________| 
        |        |        |
        |________|________|  <----------- our sort buffer 


so our sort buffer will grow upwards
and our render element buffer will grow downwards.

we will be out of space if these two stacks hit each other.



29:17
So we added a field in the render_group struct. That is the starting point where the sort buffer entries
will be pushed at 

                struct render_group
                {
                    ...
                    ...

                    u32 MaxPushBufferSize;
                    u32 PushBufferSize;
                    u8 *PushBufferBase;
                    
                    u32 PushBufferElementCount;
    ----------->    u32 SortEntryAt;

                    uint32 MissingResourceCount;
                    b32 RendersInBackground;

                    b32 InsideRender;
                };


then when we allocate a RenderGroup, we just set SortEntryAt to the end of our PushBuffer

    
                internal render_group * AllocateRenderGroup(game_assets *Assets, memory_arena *Arena, uint32 MaxPushBufferSize,
                                    b32 RendersInBackground)
                {
                    render_group *Result = PushStruct(Arena, render_group);

                    if(MaxPushBufferSize == 0)
                    {
                        // TODO(casey): Safe cast from memory_uint to uint32?
                        MaxPushBufferSize = (uint32)GetArenaSizeRemaining(Arena);
                    }
                    Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize, NoClear());
    ----------->    Result->SortEntryAt = MaxPushBufferSize;

                    Result->MaxPushBufferSize = MaxPushBufferSize;
                    Result->PushBufferSize = 0;
                    Result->PushBufferElementCount = 0;
                    
                    ...
                    ...

                    return(Result);
                }





30:57
then in the PushRenderElement_(); call we check if our two stacks will collide or not 
pretty much we need to check for the size for the new render element, as well as 
the tile_sort_entry.


visually, it looks like:


         _________________   
        | render element  |
        |_________________|
        | render element  |
        |_________________|  <------------ Group->PushBufferSize
        |                 | 
        |                 |
        |                 |
        |                 |
        | key      index  |
        |_________________|  <----------- Group->SortEntryAt
        |        |        |
        |________|________|
        |        |        |
        |________|________| 
        |        |        |
        |________|________|  


-   full code below:

                #define PushRenderElement(Group, type, SortKey) (type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type, SortKey)
                inline void *
                PushRenderElement_(render_group *Group, uint32 Size, render_group_entry_type Type, r32 SortKey)
                {

                    ...
                    ...

                    Size += sizeof(render_group_entry_header);
                    
    ----------->    if((Group->PushBufferSize + Size) < (Group->SortEntryAt - sizeof(tile_sort_entry)))
                    {
                        render_group_entry_header *Header = (render_group_entry_header *)(Group->PushBufferBase + Group->PushBufferSize);
                        Header->Type = Type;
                        Result = (uint8 *)Header + sizeof(*Header);

                        Group->SortEntryAt -= sizeof(tile_sort_entry);
                        tile_sort_entry *Entry = (tile_sort_entry *)(Group->PushBufferBase + Group->SortEntryAt);
                        Entry->SortKey = SortKey;
                        Entry->PushBufferOffset = Group->PushBufferSize;

                        Group->PushBufferSize += Size;
                        ++Group->PushBufferElementCount;
                    }
                    else
                    {
                        InvalidCodePath;
                    }

                    return(Result);
                }



33:48
so now when we call functions like PushBitmap(); in which we will call PushRenderElement(); inside,
we have to pass a sort key in there 

                handmade_render_group.cpp

                inline void PushBitmap(render_group *Group, loaded_bitmap *Bitmap, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1), r32 CAlign = 1.0f)
                {
                    used_bitmap_dim Dim = GetBitmapDim(Group, Bitmap, Height, Offset, CAlign);
                    if(Dim.Basis.Valid)
                    {
    --------------->    render_entry_bitmap *Entry = PushRenderElement(Group, render_entry_bitmap, Dim.Basis.SortKey);
                        if(Entry)
                        {
                            Entry->Bitmap = Bitmap;
                            Entry->P = Dim.Basis.P;
                            Entry->Color = Group->GlobalAlpha*Color;
                            Entry->Size = Dim.Basis.Scale*Dim.Size;
                        }
                    }
                }

35:10
Casey had it so that our bitmap_dim.Basis will have the SortKey

                handmade_render_group.h

                struct entity_basis_p_result
                {
                    v2 P;
                    r32 Scale;
                    b32 Valid;
    ----------->    r32 SortKey;
                };


so the formula we have is just 




                inline entity_basis_p_result GetRenderEntityBasisP(render_transform *Transform, v3 OriginalP)
                {

                    entity_basis_p_result Result = {};

                    v3 P = V3(OriginalP.xy, 0.0f) + Transform->OffsetP;

                    if(Transform->Orthographic)
                    {
                        ...
                        ...
                    }
                    else
                    {
                        ...
                        ...
                    }

    ----------->    Result.SortKey = 4096.0f*P.z - P.y;
                    
                    return(Result);
                }


recall that our y axis goes up, so we want the SortKey value to be higher as it gets closer to us 

    y

    ^
    |    _______________________
    |   |                       |
    |   |                       |
    |   |                       |
    |   |                       |
    |   |_______________________| 
    |
    ------------------------------>
                    x


the lower the y value, the closer it is to the camera

42:35
now in both RenderGroupToOutput(); and TiledRenderGroupToOutput(); function, we implement the sort

                handmade_render_group.cpp

                internal void RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget, memory_arena *TempArena)
                {
                    TIMED_FUNCTION();

                    // TODO(casey): Don't do this twice?
    ----------->    SortEntries(RenderGroup);

                    ...
                    ...
                }



so as a brute force approach, we will just implement bubble sort

which is n^2
                handmade_render_group.cpp

                internal void SortEntries(render_group *RenderGroup)
                {
                    u32 Count = RenderGroup->PushBufferElementCount;
                    tile_sort_entry *Entries = (tile_sort_entry *)(RenderGroup->PushBufferBase + RenderGroup->SortEntryAt);
                    
                    // TODO(casey): This is not a fast way to sort!!!!
                    for(u32 Outer = 0; Outer < Count; ++Outer)
                    {
                        // TODO(casey): Early out!
                        for(u32 Inner = 0; Inner < (Count - 1); ++Inner)
                        {
                            tile_sort_entry *EntryA = Entries + Inner;
                            tile_sort_entry *EntryB = EntryA + 1;

                            if(EntryA->SortKey > EntryB->SortKey)
                            {
                                tile_sort_entry Swap = *EntryB;
                                *EntryB = *EntryA;
                                *EntryA = Swap;
                            }
                        }
                    }
                }


Q/A
59:12
Casey_s tendency is to implement a N(logN); sort algorithm. 
the C standard library often does N^2 algorithms. N^2 is usually faster in practice.
Casey says he doesnt care if it is better in practice, he is more concerned about whether it will ever produce a bad case.

1:00:41
the reason why N^2 is faster in practice in most cases is becuz usually when we talk about the run time of a algorithm,
often times, you have N^2 as the worst case. If the worst case almost never occurs in practice, then what you care about is the 
expected case/common case, and the total cost for any individual iteration of the loop. 

so what ends up happening lots of times is that the algorithms which has the lowest worst case complexity arent necessarily 
fastest cuz worst case rarely gets hit, or the size isnt big enough so that the constant factors (the cost to run a single iteration
of the loop); is so expenvie, so it ends up costing more. If you never sort enough items to have that scale to come into play.


1:02:28
why cant you do binary insertion when you call PushElement();? that way the PushBuffer is always sorted?

You can, but that may not always be a good idea. The reason becuz if you do that, you are constantly pulling in everything 
in the render group into the cache

essentially we are comparing "sort after" vs "sort during"

so when you are doing the "sort oafter" method, you will first just push items into the array in order
         _________________   
      1 | render element  |  
        |_________________|
      2 | render element  |  
        |_________________|
      3 | render element  |  
        |_________________|
      4 | render element  |  
        |_________________|
      5 | render element  | 
        |_________________|
      6 | render element  | 
        |_________________|
      7 | render element  | 
        |_________________|
      8 | render element  | 
        |_________________|

assume our cache line can hold 3 lements 

lets say when you pushed element 1, 2, 3, you will be done with that cache line. which means you will never touched that cache line
again until you do your final sort. So that is very cache friendly

same thing for 4, 5, 6 

if you sort during you add elements, it can never be better, becuz you are constantly touching all sort of the cache line. 

so one of the nice things about the "sort after" method is that its more cache friendly

so you need to be careful with this. you may think you are saving work, but you may be doing more work. There is no reason
not to consider it, but you have to measure it.
