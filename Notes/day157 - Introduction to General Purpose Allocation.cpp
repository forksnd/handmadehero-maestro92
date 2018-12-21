Handmade Hero Day 157 - Introduction to General Purpose Allocation

Summary:
summarized that our games memory footprint exceeds our max memory
so the asset memory management can be sumarzied as a caching problem / virtual memory problem

talked about the problem of fragmentation

mentioned fixed size allocator and variable szied allocator approaches to solve defragmentation in our game 

fixed size allocator methods include conforming all sprites to the same size 
or using mega-texture 

variable size allocator methods include moving memory around to compact empty space
or 
merging memory spaces as spaces get freed

changed the loaded_bitmap struct to make it occupy smaller storage. 

mentioned the potential problem we have currently in our code base with calling free assets 

briefly explained what RAII is in the Q/A

explained the difference between megatexture vs texture atlas in Q/A

Keyword:
Asset memory management, memory fragmentation, rendering memory


0:19
Casey mentioned that we would like to tackle, perhaps, the only hard memory management problem
we will have on handmade hero. 



1:09
on the whole, you can model most things as very simple stacks. You write it once, and you really
never have to think about memory again.

There are two places we actually have to think about memory. One of is this memory management for the asset system

this problem is really about how to make optimal use of fixed amount of storage space. Its really a caching problem. 


3:40
there are a couple of things you would consider when talking about memory
1.  how much. Quantity. How much memory do you need to run. whats your footprint

2.  When you need memory. this is the concept of Alloc/Free. Where do these happen in the code. 
    How we track how much we need at anytime is a big problem to consider.


10:10
for our asset memory management, point 2 is not very relevant. we only care about the point 1. 
we have a shortage of memory. 


assume, we have 3 terra bytes of assets, but we only have 4 GB of ram                                                     
    
          Assets                    RAM
         ___________
        |           |
        |           |            ___________
        |           |           |           |
        |           |           |           |
        |           |           |   4 GB    |
        |   3 TB    |           |           |
        |           |           |           |
        |           |           |           |
        |           |           |___________|
        |           |
        |           |
        |           |
        |           |
        |___________|

for us, the problem of "when do you need memory" is always. Always at all times, we would prefer to have assets in memory. 
The problem is though, we cant fit it all in our memory.

so essentially, we have a caching problem.


13:22
this can also be thought of as a VM prbolem, a virtual memory problem. 
what we want to do is to take out slices from the asset and put them into our memory 

          Assets                    RAM
         ___________
        |           |
        |           |            ___________
        |___________|           |           |
        |           |---------> |           |
        |___________|           |   4 GB    |
        |           |           |           |
        |   3 TB    |           |           |
        |           |    -----> |           |
        |           |   /       |___________|
        |           |  /
        |___________| /
        |___________|/
        |           |
        |___________|

then when our RAM fills up, we need to evict assets temporarily to put something else in.




16:40

Casey mentioned another challenge we have to solve 
assume our ram looks like below, and lets say we freed A. It leaves a hole in the memory

and only something equall or smaller than A can fit in there.
reminds me of 391 lecture notes 

           RAM                   
         ___________                         ___________
        |           |                       |           |
        |     A     | <------ freed A       |   hole    |        
        |-----------|                       |-----------|
        |           |                       |           |   
        |     B     |                       |     B     |             
        |           |                       |           |           
        |-----------|                       |-----------|            
        |     C     |                       |     C     |           
        |-----------|                       |-----------|           
        |     D     |                       |     D     |  
        |           |                       |           |  
        |-----------|                       |-----------|  
        |     E     |                       |     E     |  
        |___________|                       |___________|  

if you put something even smaller, you get fragmentation. So you end up in a case, it soo small that probably nothing can go in.
         ___________
        |    a      |
        |-----------| <-------- this hole is almost usable
        |-----------| <--------
        |           | 
        |     B     |
        |           |
        |-----------|
        |     C     |
        |-----------|
        |     D     |
        |           |
        |-----------|
        |     E     |
        |___________|

fragmentation is exactly the problem that you get when the size of your memory is very constrained.
its essentially the problem will you try to maximually fill the space that you got. 


20:36
fixed size allocators are really great. If you can turn a memory management problem into a fixed size allocation problem 
is very fantastic, becuz that will allow you to do perfect managment of the memory. 

recall the freelist system that we have implemented in our game, thats essentially a classic example of "fixed size allocators" 

we want to handle the problem where the assets are not the same size.

1.  make it into a fixed size allocation problem. For example, you flat out decided that all sprites are 256 x 256.
    of course the downsize is that it doesnt make sense for us to have all bitmaps conform to this dimensions


2.  we make things into tiles 

    bitmaps are composed of 256 x 256 tiles. So 256 x 256 is the smallest unit of our bitmap assets.
    are other assets are either 512 x 512 or 1024 x 1024 or so on...

    but for that you will still get wastages. 

    consider the example, assume we have a 512 x 512 sprite, and our sprite only occupies 300 x 300
    the you got 212 x 212 of wastage.

                    256             256
                 _______________________________
                |###############|####           |
                |###############|####           |
                |###############|####           |
          256   |###############|####           |
                |###############|####           |
                |###############|####           |
                |_______________|_______________|
                |###############|####           |
                |###############|####           |
                |###############|####           |
          256   |###############|####           |
                |               |               |
                |               |               |
                |_______________|_______________|

    so there are ways for us to mitigate that. one way to do that is to texture atlas
    so all the sprites are packed into a giant sprite sheet, not by hand, but by an automated process

    this sprite sheet are split into tiles.


                        256             256            256
                 _______________________________________________
                |###############|####           |               |
                |###############|####           |               |
                |###############|####           |               |
          256   |###############|####         @@|@@@@           |
                |###############|####         @@|@@@@           |
                |###############|####         @@|@@@@           |
                |_______________|_______________|_______________|
                |###############|####           |               |
                |###############|####      %%%%%|%%%%           |
                |###############|####      %%%%%|%%%%%%%%       |
          256   |###############|####      %%%%%|%%%%%%%%%%%    |
                |               |            %%%|%%%%%%%%%%%    |
                |$$$$$$$$$$$$$$$|$$$$$$$$     %%|%%%%%%%%%%%    |
                |_______________|_______________|_______________|
                |$$$$$$$$$$$$$$$|$$$$$$$$       |               |
                |$$$$$$$$$$$$$$$|$$$$$$$$   ####|####           |
                |$$$$$$$$$$$$$$$|$$$$$$$$   ####|####           |
          256   |$$$$$$$$$$$$$$$|$$$$$$$$   ####|####           |
                |               |           ####|####           |
                |               |           ####|####           |
                |_______________|_______________|_______________|

    then when we say we want to load a sprite, we just have to figure out which set of tiles it needs, and we load these tiles in.
    all of these tiles are 256 x 256

    this is very similar to a mega-texture scheme

    there are merits, but there are also concerns around this approach

    the biggest concern about this approach is what do you do about mipmapping. Mipmapping is hard to do in a scheme like this 
    in a 2D game, its possible that we can get away with it.

    in a 2D game, you know the mips a head of time, and you dont really have to trilinear interpolations or angular stuff, where the mip 
    levels will change across a single polygon. So its quite plausible to do a scheme like this 

    Nevertheless, this approach still doesnt completely solve the problem of your assets being different sizes.
    So even though, our actual pagable asset that you are working with is uniform sizes, what it doesnt do is fix the problem of asset
    tile list.

    we still have the problem that if asset A is 512 x 512, its got 4 tiles.
    if B is 256 x 256, that has 1 tile

    so you still have a variable size element in your assets


3.  Option3: we do a variable allocator, one that can allocate things of variable sizes.




let say we have our 4 GB memory here

                4 GB
             ___________
            |     A     |
            |           | 
            |-----------| 
            |           | 
            |     B     |
            |           |
            |           |
            |-----------|
            |           |
            |     C     |
            |           |
            |           |
            |           |
            |___________|

and we free up B and page in D, which is smaller.
and when we get fragmentation, we have two choices to cope with this. 

                4 GB
             ___________
            |     A     |
            |           | 
            |-----------| 
            |           | 
            |     D     |
            |           |
            |-----------| <------- unsuable parts cuz its too small
            |-----------|
            |           |
            |     C     |
            |           |
            |           |
            |           |
            |___________|


1.  becuz our asset system is already indirected. cuz the asset system points into these memory portions,
people always reference assets through the asset table, we can do some kind of compaction. For example,
we can move C up to close up that space. 

so we can move stuff around to defragment


2.  the other way is the more standard way, the thing that you would normally do in a "variable Allocator"
you would merge freed memory if possible. Essentially condence on free.

For example, if C were to get freed, we can merge the (B-D); part with C 

so instead of thinking you have two feed memory segment, B-D and C,
now the two are combined to one

                4 GB
             ___________
            |     A     |
            |           | 
            |-----------| 
            |           | 
            |     D     |
            |           |
            |-----------| 
            |           |
            |           |
            |   Freed   |
            |           |
            |           |
            |           |
            |___________|

30:00
Casey opted to go with the standard approach, cuz its good education.
at the end of the day this is also what malloc and free does, new and delete does


34:35
Casey will go on to implement this memory thing.
first thing he wants to try is to find a way to manually have the asset system run out of memory.


34:56
so recall in our game initalization code, we give each system a memory arena 
so to manually "Run out of memory", Casey just give the Asset System very little memory. 

so previously we were giving 

                TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(64), TranState); 

now we are just giving 2 Megabytes

                TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(2), TranState); 


-   code below:
                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    ...
                    ...

                    // NOTE(casey): Transient initialization
                    Assert(sizeof(transient_state) <= Memory->TransientStorageSize);    
                    transient_state *TranState = (transient_state *)Memory->TransientStorage;
                    if(!TranState->IsInitialized)
                    {
                        InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                                        (uint8 *)Memory->TransientStorage + sizeof(transient_state));
                            
                        ...
                        ...

                        TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(2), TranState);     <-------  low memory
                    }

                    ...
                    ...
                }




41:28
Casey first starting to carefully examine the memory usage of the asset system.

recall, currently the main asset usage is in asset_slot, where it either points to a loaded_bitmap or a loaded_sound

                struct asset_slot
                {
                    asset_state State;
                    union
                    {
                        loaded_bitmap* Bitmap;
                        loaded_sound* Sound;
                    };
                };


to remove the indirection, Casey got rid of the pointer, and just directly stored the fields 


                struct asset_slot
                {
                    asset_state State;
                    union
                    {
                        loaded_bitmap Bitmap;
                        loaded_sound Sound;
                    };
                };


then inside the loaded_bitmap, he carefully examine the usage 

previously, we had 

                struct loaded_bitmap
                {
                    v2 AlignPercentage;
                    real32 WidthOverHeight;
                    
                    int32 Width;
                    int32 Height;
                    int32 Pitch;
                    void *Memory;
                };


here s16 is Casey_s way of writing signed int 16. He did a typedef int16 s16

so in terms of dimensions, if we try to store it with a signed int 8, that only gives you bitmaps of 256 x 256
which is a bit too small.

but if we use signed int 16, that should be more than enough for any bitmap dimensions we have.

                struct loaded_bitmap
                {
                    void *Memory;
                    v2 AlignPercentage;
                    r32 WidthOverHeight;    
                    s16 Width;
                    s16 Height;
                    // TODO(casey): Get rid of pitch!
                    s16 Pitch;    
                };

Casey also mentioned that we plan to get rid of the Pitch in the future.
if we can get rid of the pitch, that gives us 4 32-bit stuff, and the pointer (either 32 bit or 64 bit, 
depending on the architecture or what your application is compiled in);






45:17
in the LoadBitmap(); code, when we initialize the Bitmap, we call SafeTruncateToUInt16(); for the Bitmap->Width and 
Bitmap->Height;

                Bitmap->Width = SafeTruncateToUInt16(Info->Dim[0]);
                Bitmap->Height = SafeTruncateToUInt16(Info->Dim[1]);

-   code below:
                internal void
                LoadBitmap(game_assets *Assets, bitmap_id ID)
                {
                    asset_slot *Slot = Assets->Slots + ID.Value;        
                    if(ID.Value &&
                       (AtomicCompareExchangeUInt32((uint32 *)&Slot->State, AssetState_Queued, AssetState_Unloaded) ==
                        AssetState_Unloaded))
                    {
                        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
                        if(Task)        
                        {
                            asset *Asset = Assets->Assets + ID.Value;
                            hha_bitmap *Info = &Asset->HHA.Bitmap;
                            loaded_bitmap *Bitmap = &Slot->Bitmap;
                            
                            Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
                            Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
                            Bitmap->Width = SafeTruncateToUInt16(Info->Dim[0]);
                            Bitmap->Height = SafeTruncateToUInt16(Info->Dim[1]);
                            Bitmap->Pitch = 4*SafeTruncateToUInt16(Info->Dim[0]);
                            u32 MemorySize = Bitmap->Pitch*Bitmap->Height;
                            Bitmap->Memory = PushSize(&Assets->Arena, MemorySize);

                            ...
                            ...

                            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
                        }
                        else
                        {
                            Slot->State = AssetState_Unloaded;
                        }
                    }    
                }




52:53
Casey mentioned that now we need to know a way to whether we are reaching the limits we allocated for our asset system.

so currently we request new asset in the LoadBitmap(); call. 

                handmade_asset.cpp

                internal void
                LoadBitmap(game_assets *Assets, bitmap_id ID)
                {
                    asset_slot *Slot = Assets->Slots + ID.Value;        
                    if(ID.Value &&
                       (AtomicCompareExchangeUInt32((uint32 *)&Slot->State, AssetState_Queued, AssetState_Unloaded) ==
                        AssetState_Unloaded))
                    {
                        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
                        if(Task)        
                        {
                            asset *Asset = Assets->Assets + ID.Value;
                            hha_bitmap *Info = &Asset->HHA.Bitmap;
                            loaded_bitmap *Bitmap = &Slot->Bitmap;
                            
                            Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
                            Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
                            Bitmap->Width = SafeTruncateToUInt16(Info->Dim[0]);
                            Bitmap->Height = SafeTruncateToUInt16(Info->Dim[1]);
                            Bitmap->Pitch = 4*SafeTruncateToUInt16(Info->Dim[0]);
                            u32 MemorySize = Bitmap->Pitch*Bitmap->Height;
                            Bitmap->Memory = PushSize(&Assets->Arena, MemorySize);

                            ...
                            ...

                            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
                        }
                        else
                        {
                            Slot->State = AssetState_Unloaded;
                        }
                    }    
                }


however, if we are reaching the memory limit in the asset system, we cant call "evict old asset" in this LoadBitmap();
call. This is becuz of the context that we are call LoadBitmap(); from 


if you check out the stack trace, you can see that in the rendering set up code,
we loop through all the entities and we call PushBitmap(); on each of them.                

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    // TODO(casey): Move this out into handmade_entity.cpp!
                    for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
                    {

                        switch(Entity->Type)
                        {
                            case EntityType_Hero:
                            {
                                // TODO(casey): Z!!!
                                real32 HeroSizeC = 2.5f;
                                PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                                PushBitmap(RenderGroup, HeroBitmaps.Torso, HeroSizeC*1.2f, V3(0, 0, 0));                                
                            } break;

                            ...
                            ...

                            case EntityType_Wall:
                            {
                                PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Tree), 2.5f, V3(0, 0, 0));
                            } break;

                            ...
                            ...
                        }
                    }
                }


                handmade_render_group.cpp

                inline void
                PushBitmap(render_group *Group, bitmap_id ID, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
                {
                    loaded_bitmap *Bitmap = GetBitmap(Group->Assets, ID);
                    if(Bitmap)
                    {
                        PushBitmap(Group, Bitmap, Height, Offset, Color);
                    }
                    else
                    {
                        LoadBitmap(Group->Assets, ID);
                        ++Group->MissingResourceCount;
                    }
                }

so now you see that, its not right to call "evict old asset" in LoadBitmap(); cuz we risk evicting bitmaps that we just setup 
in the current frame. 

55:34
What we rather is to let introduce a controlled time, when bitmaps are marked as free, so that we dont ever free them in the middle 
of an operation. 
there are 2 ways we can do that.
1.  when we call LoadBitmap(); we can put them onto a special list. we dont actually load them immediately

    we deter them to be loaded later, once we finished all the PushBitmap(); calls at the end of a frame, we call LoadBitmap();
    on everyone entry in the speical list. That way we dont risk evicting bitmaps in the same frame.


                                         ___________
                                        |     A     |
         __________                     |           | 
        |    A     |                    |-----------| 
        |----------|                    |           | 
        |    B     |                    |     D     |
        |----------| ---------------->  |           |
        |    C     |                    |-----------| 
        |----------|                    |           |
        |    D     |                    |           |
        |----------|                    |           |
        |__________|                    |           |
                                        |           |
                                        |   Freed   |
                                        |           |
                                        |           |
                                        |           |
                                        |___________|

    

2.  the other is that we can kick of tasks immediately, using space we pre-freed.
    so every frame, we always keep at least 16 MB freed. (16 is arbituarily chosen, could be some other number);
    During the frame, we will have tasks that will write to the 16 MB, 
    then after the frame, we will free back up to 16 MB.


57:14
Casey prefers the 2nd method, cuz he prefers to kick off LoadBitmap(); calls immediately.



1:01:23
someone asked what does new and delete do?

you can think of in terms of 4 steps 
1.  Getting the memory      
2.  initializing the memory

3.  deinitializing the memory 
4.  freeing the memory 

its essentially malloc and free with the contructor/destructor being called

you may ask why is deinitializng the memory necessarily
consider the case 

                struct Foo
                {
                    Bar* bar;
                    File file;
                };

where you have a nested pointer, you have to clean up Bar*, or close the file handle.
and you have to those in the "deinitializing the memory" step

so the time to use new and delete vs malloc and free is very simple: do you want the compiler 
to call the constructor or destructor?



1:19:37
someone in the Q/A asked what is Resource Acquisition Is Initalization. (RAII)?

its a modern programming trend. It something that doesnt really have any meaning out side of OOP
an object is only considered intialized once it has acquired all of its resources. 

objects should exists if it has only partially acquired its resources.


RAII is kind of stupid. not recommended.



1:29:52
what is the difference between megat texture and texture atlas?

a texture atlas is basically a same thing. They are two different flavors of a similar thing.
megatexture usually includes the 3D pieces of it.


1:31:15
when thinking about asset allocation in terms of maximum amount of pixel that can be displayed on screen be worth it?
For instance, with your 16 MB safety buffer, you tailor the asset loading to load assets that are resolution proportional 
to the distance to the camera, and operate under the assumption that only a fixed number of things can physically occupy 
the high, meidum and low resolution space.

Yes, that is what we do in Molly rocket. This is not necessary the scheme for handmade hero, cuz handmade hero we dont do 
much zooming. This will be a good scheme if you zoom a lot. 