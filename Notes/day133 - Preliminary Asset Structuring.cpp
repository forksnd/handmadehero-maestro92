Handmade Hero Day 133 - Preliminary Asset Structuring
Summary:

discussed plans of working with a memory budget for assets. 
mentioned plans of evicting assets 

mentions the problem of assets being evicted in the middle of rendering. 

introduced the concept of locked assets which cant be evicted to solve the above problem.

introduced the concept of asset header and asset meta info

Keyword:
Asset, memory


4:05
Casey mentioned the problem in the following function 

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
                {
                    load_asset_work *Work = (load_asset_work *)Data;

                    // TODO(casey): Get rid of this thread thing when I load through a queue instead of the debug call.
                    thread_context *Thread = 0;
                    *Work->Bitmap = DEBUGLoadBMP(Thread, Work->Assets->ReadEntireFile, Work->FileName);
                    // , AlignX, TopDownAlignY);

                    // TODO(casey): Fence!
                    
                    Work->Assets->Bitmaps[Work->ID] = Work->Bitmap;

                    EndTaskWithMemory(Work->Task);
                }


the DEBUGLoadBMP call doesnt gurantee a bounded memory footprint
basically what will happen is that, as we run the game, everytime we call DEBUGLoadBMP();, it will load more memory into the system.
So we will use more and more memory until we run out of virtual paging in the OS, then the game will flat out stop working.




5:12
to solve this, Casey wants to introduce the concept of "one in, one out"

so we want to estimate how much memory we want the art assets to take. 

for example assume lets say it will take up 2GB of memory 

                  2 GB
                 ___________
                |           |
                |           |
                |           |
                |           |
                |           |
                |           |
                |           |
                |           |
                |           |
                |___________|


when we encounter an asset that we havent loaded but need, what we like to do is to take 
something from the existing loaded asset. we could pick the Least recently used (LSU); or other heurstics,
and then we would like to evict that. 

then we load in the new one that we need.

with this approach, we always know that we wont run out of memory and crash. 



7:47
the other thing we want to get to is that we dont want to load the same art asset twice. 




15:39
to solve the not load the same art asset twice problem, we just do some critical section in our LoadAsset function;
                
this is essentially saying, we want to compare &Assets->Bitmaps[ID].State with AssetState_Unloaded, and set it to 
AssetState_Queued

then if the return value is AssetState_Unloaded, we go ahead and load our art.
this happens becuz when the first thread comes in, it will return the AssetState_Unloaded value.
when subsequent threads come in, the value of the state would have been changed to AssetState_Queued already.
so only one thread will execute inside this if statement


                internal void
                LoadAsset(game_assets *Assets, game_asset_id ID)
                {
                    if(AtomicCompareExchangeUInt32((uint32 *)&Assets->Bitmaps[ID].State, AssetState_Unloaded, AssetState_Queued) ==
                       AssetState_Unloaded)
                    { 
                        ...
                        ...
                    }
                }



16:43
we will have to define AtomicCompareExchangeUInt32 based on the platform

                handmade_intrinsics.h

                #if COMPILER_MSVC
                #define CompletePreviousWritesBeforeFutureWrites _WriteBarrier();
                inline uint32 AtomicCompareExchangeUInt32(uint32 volatile *Value, uint32 Expected, uint32 New)
                {
                    uint32 Result = _InterlockedCompareExchange((long *)Value, Expected, New);

                    return(Result);
                }
                #else
                // TODO(casey): Need GCC/LLVM equivalents!
                #endif



22:00
made a sanity check in the FillGroundChunk(); function.
we want to make sure that when we kick off the work, all the bitmaps in the RenderGroup are actually valid
so we added the AllResourcesPresent(); function

                handmade.cpp

                internal void
                FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
                {
                    task_with_memory *Task = BeginTaskWithMemory(TranState);
                    if(Task)
                    {
                        fill_ground_chunk_work *Work = PushStruct(&Task->Arena, fill_ground_chunk_work);
                        
                        ...
                        ...

                        for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
                        {
                            for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
                            {
                                ...
                                ...
                                PushBitmap(RenderGroup, Stamp, 0.1f, V3(P, 0.0f));                                
                            }
                        }

                        if(AllResourcesPresent(RenderGroup))
                        {
                            GroundBuffer->P = *ChunkP;

                            Work->RenderGroup = RenderGroup;
                            Work->Buffer = Buffer;
                            Work->Task = Task;

                            PlatformAddEntry(TranState->LowPriorityQueue, FillGroundChunkWork, Work);            
                        }
                    }
                }



24:05
here we write the AllResourcesPresent function;
as you can see, we are keeping track of a counter for MissingResourceCount in render_group;

                handmade_render_group.cpp

                inline bool32
                AllResourcesPresent(render_group *Group)
                {
                    bool32 Result = (Group->MissingResourceCount == 0);

                    return(Result);
                }


23:15
we added the MissingResourceCount counter in render_group

                struct render_group
                {
                    ...
                    ...
                    uint32 MissingResourceCount;
                };


23:44
in the PushBitmap(); function, whenever we resort to loading the asset, we increase the RenderGroup MissingResourceCount, 

                inline void
                PushBitmap(render_group *Group, game_asset_id ID, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
                {
                    loaded_bitmap *Bitmap = GetBitmap(Group->Assets, ID);
                    if(Bitmap)
                    {
                        PushBitmap(Group, Bitmap, Height, Offset, Color);
                    }
                    else
                    {
                        LoadAsset(Group->Assets, ID);
                        ++Group->MissingResourceCount;
                    }
                }


[I guess Casey didnt finish this part up cuz we dont currently decrement MissingResourceCount anywhere...]



24:28
there is still a problem: resource eviction. We dont want resource eviction happen while rendering. 

one thing we can make with the main renderer is that during the time when stuff is being rendered, 
we gurantee no assets will be evicted. 

we can just make resource evicting happen at a known time, (a time that is different from rendering time)

howwever, with ground chunks, the ground chunks are happening on a separate thread. 


solutions:
1.  cross your fingers. Depending on how you implement your Least Recently used scheme, you always mark your ground 
chunks bitmap "hot", that way it is never evicted.
2.  temporary lock these resources in. everyone someone uses it in the renderer, we lock these assets. For example, 
we can keep track of the state of these assets. when we render, we change it to a "used" state. Then when we are done,
we clean up the "used" state.
3.  make ground chunk asset un-evictable

Casey says that it is probably the correct way, the only catch is that it invovles reiterating the buffer.


37:16
Casey attempted solution 2, find it too annoying, straight up used solution 3.
Pretty much he introduced the concept of un-evictable/locked resources

Casey added the AssetState_Locked state 

                enum asset_state
                {
                    AssetState_Unloaded,
                    AssetState_Queued,
                    AssetState_Loaded,
                    AssetState_Locked,
                };

and the AssetState_Locked cant be streamed out.

this also gives us the flexibility of marking certain asset locked if we choose to do so 


43:14
Casey starting to introduce the concept of asset header or asset meta files
essentially we want a struct that describes certain bitmaps.                

the idea is that we want some asset information that might help with game logic 

                struct asset_tag
                {
                    uint32 ID;
                    real32 Value;
                };

                struct asset_bitmap_info
                {
                    v2 AlignPercentage;
                    real32 WidthOverHeight;
                    int32 Width;
                    int32 Height;

                    uint32 FirstTagIndex;
                    uint32 OnePastLastTagIndex;    
                };
                
                struct asset_group
                {
                    uint32 FirstTagIndex;
                    uint32 OnePastLastTagIndex;    
                };


For example, when we do the ground chunk construction, right now we are completely picking random splats.
If we have a decision vector to choose the "best" one, that will be great. 

the decision vector will have to consider "tags" or whatnot


53:54
Casey wrote a sample function of how the asset_tag can help us.


1:16:23
someone the Q/A ask why are we implementing virtual memory ontop of OSs virtual memory?

the reason is becuz relying on OSs virtual memory is very tenuous. you have no idea what it is going to do,
you dont know when the OS is very page in, and which thread will hit the virtual memory. 


1:18:28
its more power efficient to use a GPU, on laptops if we go through the GPU, it wont burn through the battery nearly as much 

