Handmade Hero Day 160 - Basic General Purpose Allocation

Summary:
replaced the logic of us asking the OS paging table for memory with our own memory allocator in AcquireAssetMemory();
and ReleaseAssetMemory();

Realized there is no need to call EvictAssetsAsNecessary(); in the main thread. Since our assets are all protected 
by the AssetState_Lock flag.
Removed the call EvictAssetsAsNecessary(); call in our main thread.
changed the code that EvictAssetsAsNecessary(); is called when AcquireAssetMemory();

used a doubly linked list scheme to manage our own memory. 


Keyword:
Asset, Memory 



3:28
Currently we are calling allocate and Deallocate from the OS page table.
we are asking from the OS, a new set of pages everytime we want an asset.
and then releaseing the pages back to the OS everytime we got rid of an asset. 


                handmade_asset.cpp

                inline void *
                AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    void *Result = Platform.AllocateMemory(Size);
                    if(Result)
                    {
                        Assets->TotalMemoryUsed += Size;
                    }
                    return(Result);
                }

                inline void
                ReleaseAssetMemory(game_assets *Assets, memory_index Size, void *Memory)
                {
                    if(Memory)
                    {
                        Assets->TotalMemoryUsed -= Size;
                    }
                    Platform.DeallocateMemory(Memory);
                }

the goal is to replace these two functions ourselves.

you noticed that these are completely general, the assets can be allocated or freed at anytime.
there is no stack like structure to it. Most of the things in a game are happy to use a stack.
which is why we have been able to get away with not having a General Purpose Allocator anywhere else.

essentially by structuring things so that they operate on stacks, that removes the problem of dealing with 
any lifetime issues, and also removes the problem of dealing with placements (where to place things in memory);, cuz its just FILO.


9:20
you can see that memory placement and fragmentation is a whole domain of interesting problems.
you can choose to spend months if this is your interest


10:50
you can imagine that right at the beginning, we have this gigantic empty piece of memory. 

     _______         
    |       |       
    | 1 GB  |
    |       |       
    |       |       
    |       |       
    |       |       
    |       |       
    |       |       
    |       |       
    |       |       
    |       |       
    |       |              
    |       |       
    |_______|       
    

On 32 bit windows, its possible that it cant find 1 GB of contiguous memory. 
So you may get handed a few free regions 

     _______         _______
    |       |       |       |
    | 64 mg |       | 64 mg |
    |       |       |       |
    |_______|       |_______|
     _______         _______
    |       |       |       |
    | 64 mg |       | 64 mg |
    |       |       |       |
    |_______|       |_______|
     _______         _______
    |       |       |       |
    | 64 mg |       | 64 mg |
    |       |       |       |
    |_______|       |_______|

14:02
in some sense, when we are doing allocation, we dont even have to think about regions that are filled. 
we just have to think about regions that are free. 

so in some sense, you can think of them as disjoint regions 
     _______         
    |#######|           
    |#######|                _______
    |#######|               |       |
    |       |               |   A   |
    |   A   |               |_______|
    |       |                   |
    |#######|                   v
    |#######|                _______
    |   B   |               |   B   |
    |       |               |_______|
    |#######|                   |
    |       |                   v
    |   C   |                _______
    |_______|               |       |
                            |   C   |
                            |_______|

this would work fine with allocation. we just iterate through each region and see if we can put it in there.




then lets say you free the region between A and B, then node A and node B are no longer separate nodes
they actually become one bigger node.

     _______                    
    |#######|                
    |#######|                     _______
    |#######|                    |       |
    |       |                    |   A   |
    |   A   |                    |_______|
    |       |                        |
    |#######| <----- region          v
    |#######|                     _______
    |   B   |                    |   B   |
    |       |                    |_______|
    |#######|                        |
    |       |                        v
    |   C   |                     _______
    |_______|                    |       |
                                 |   C   |
                                 |_______|

so even though when we are allocating, we dont actually care about filled space, we still need someway of knowing
there are filled space. Cuz we need to know where the filled space are in relation to the free space.


16:09
other things we know about or memory 
this is an asset system, so basically we dont have small assets. pretty much we dont have to worry about people 
allocating 1 byte, or 16 bytes.
People will be allocating like 64 kb.

which means we can assume that our free space nodes can usually fit in quite some space. 
Pretty much they are at least some 32 kb or 64 kb in space.



16:55
so if we just want to do this in some very inefficient fashion, 

we can structure it in a way where every free block of memory can hold some information about what comes before 
and what comes after, kind of like a doubly linked list




                                 _______________
                                |     prev      |
                                |###############|
                                |###############|
                                |###############|
                                |     next      |
                                |_______________|
                                       | 
     _______                           |
    |#######|                          v
    |#######|                    _______________
    |#######|                   |     prev      |
    |       |                   |               |
    |   A   |                   |               |
    |       |                   |      A        |
    |#######|                   |               |
    |#######|                   |     next      |
    |   B   |                   |_______________|
    |       |                          |
    |#######|                          |
    |       |                          v
    |   C   |                    _______________
    |_______|                   |     prev      |
                                |###############|
                                |###############|
                                |###############|
                                |###############|
                                |     next      |
                                |_______________|
                                       |
                                       |
                                       v
                                 _______________ 
                                |     prev      |
                                |               |
                                |      B        |
                                |     next      |
                                |_______________|
                                       |          
                                       |
                                       v
                                 _______________
                                |###############|
                                |###############|
                                |     next      |
                                |_______________|
                                       |
                                       |
                                       v

                                 _______________ 
                                |     prev      |
                                |               |
                                |      C        |
                                |     next      |
                                |_______________|

just some info that will help us with any potential merging operation.
also, whats nice is that we are putting these "info headers" in these free blocks, so we dont have to allocate
outside memory for the array of "info headers"

meaning, we dont need to borrow memory from the "Permenant Memory" or "Transient Memory" to store these info headers.
They just live inside the free chunks.  

19:02
lets say we have the following structure 

one way we can do is that when we want to allocate something, we just walk all the free blocks.
presumably, the one we want to put it in, is the smallest one that can fit your needs                 
                                 _______________
                                |               |
                                |      A        |
                                |               |-------- check if we can fit in here
                                |_______________|       |
                                |###############|       |
                                |###############| <-----| skip
                                |###############|         
                                |###############| -------
                                |###############|       |
                                                        |
                                 _______________        |
                                |               |       |
                                |      B        | <------ check if we can fit in here 
                                |               |                                
                                |               |--------
                                |_______________|       |
                                |###############| <------  skip
                                |###############|       
                                |###############| -------
                                |###############|       |                         
                                |               |       |
                                |               |       |
                                |      C        | <------ check if we can fit in here
                                |               |
                                |_______________|


20:19
one of the interesting thing we know about asset system, is that when we load our asset,
we know all the sizes of all of the assets.

so we can even plot a histogram of sizes 

        ^
        |
        |
        |
        |
        |           ##
        |           ##
        |   ##      ##
        |   ##      ##      ##
        |___##______##______##_________
            
            4k      8k      16k

so we can actually see the probability of us hitting an asset of a certain size range.
so if we know there are 4000 8k assets, and 200 16k assets, we would know that if we have free chunks
of 9k lying around, we wont be concerned cuz the chances of us hitting an 8k asset allocation is fairly high.


24:49
Casey makes a remark that we may not need to have EvictAssetsAsNecessary(); pumped in the Tick(); function

the assets used in our background chunks thread are always locked, so they are never put on to the doubly linked list nodes.
so they are never at risk of being evicted.

for assets being loaded on the main thread, they are always protecte by our AssetState_Lock flag.
Even for regular assets (ones whose final state are not locked), we put them on the doubly linked list. 
We would give these assets the AssetState_Lock flag when they are being queued or loaded, but their final state
would just be AssetState_Loaded.

recall our function


                internal void
                LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Locked)
                {
                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value &&
                       (AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
                        AssetState_Unloaded))
                    {
                        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
                        if(Task)        
                        {
                            asset *Asset = Assets->Assets + ID.Value;
                            hha_bitmap *Info = &Asset->HHA.Bitmap;

                            ...
                            ...

                            Work->FinalState = (AssetState_Loaded) | (Locked ? AssetState_Lock : 0);

                            Asset->State |= AssetState_Lock;

                            if(!Locked)
                            {
                                AddAssetHeaderToList(Assets, ID.Value, Size);
                            }

                            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
                        }
                        else
                        {
                            Asset->State = AssetState_Unloaded;
                        }
                    }    
                }

in the EvictAssetsAsNecessary(); you can see that we only call EvictAsset(); if it is loaded.
so you can see that Assets are always protected by the AssetState_Lock flag.               

                handmade_asset.cpp

                internal void
                EvictAssetsAsNecessary(game_assets *Assets)
                {
                    while(Assets->TotalMemoryUsed > Assets->TargetMemoryUsed)
                    {
                        asset_memory_header *Header = Assets->LoadedAssetSentinel.Prev;
                        if(Header != &Assets->LoadedAssetSentinel)
                        {
                            asset *Asset = Assets->Assets + Header->AssetIndex;
                            if(GetState(Asset) >= AssetState_Loaded)
                            {
                                EvictAsset(Assets, Header);
                            }
                        }
                        else
                        {
                            InvalidCodePath;
                            break;
                        }
                    }
                }


and for this reason, Casey realized that we dont have to call EvictAssetsAsNecessary(); every tick.

So now we can evict assets whenever we request memory. Recall we initially put EvictAssetsAsNecessary(); in the tick();
cuz we were doing the "reserve 16 MG of memory every frame" approach. Now we are just going to Evict on demand (which is 
when we call the AcquireAssetMemory(); function);

So Casey got rid of the EvictAssetsAsNecessary(); in the main loop,

and added it in the AcquireAssetMemory(); call;



                internal void *
                AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    // NOTE(casey): This is the platform memory path
                    EvictAssetsAsNecessary(Assets);
                    void* Result = Platform.AllocateMemory(Size);                    
                    if(Result)
                    {
                        Assets->TotalMemoryUsed += Size;
                    }
                    return(Result);
                }


28:59
Casey starting to change the AcquireAssetMemory(); function. So we start to allocate memory out of our own memory, instead 
of OS_s page tables.

previously, all of our memory allocations were taken from our memory arenas. Here we dont really want to do it as an arena, cuz 
this is not like a stack


so Casey straight up removed the "memory_arena Arena:" in the game_assets struct.

                handmade_asset.h

                struct game_assets
                {
                    // TODO(casey): Not thrilled about this back-pointer
                    struct transient_state *TranState;
                    memory_arena Arena;     <--------- removed 

                    u64 TargetMemoryUsed;
                    u64 TotalMemoryUsed;

                    ...
                    ...

                };

instead of using arena, Casey is defined a new asset_memory_block.

Casey will do this change step by step. 
as a first implementation, the asset_memory_block is just gonna model after what the memory arena did 

                struct asset_memory_block
                {
                    u32 TotalSize;
                    u32 UsedSize;                    
                };

and then we give the game_assets, a "asset_memory_block* FirstBlock;" variable
                struct game_assets
                {
                    // TODO(casey): Not thrilled about this back-pointer
                    struct transient_state *TranState;

                    asset_memory_block* FirstBlock;
                    
                    u64 TargetMemoryUsed;
                    u64 TotalMemoryUsed;
                    asset_memory_header LoadedAssetSentinel;

                    ...
                    ...
                };


and we staright up grab all the memory from the arena to the Asset->FirstBlock

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);
                    Assets->FirstBlock = (asset_memory_block*)PushSize(Arena, Size);
                    Assets->FirstBlock.TotalSize = Size - sizeof(asset_memory_block);
                    Assets->FirstBlock.UsedSize = 0


                    Assets->TranState = TranState;
                    Assets->TotalMemoryUsed = 0;
                    Assets->TargetMemoryUsed = Size;

                    ...
                    ...
                }


graphically, it looks like this 

             _______________    
            |               |
            |   FirstBlock  |
            |_______________|       all the memory we got from the Memory Arena
            |               |
            |               |
            |   memory      |
            |  we can give  |
            | to the assets |
            |   system      |
            |_______________|

which is why we had
                Assets->FirstBlock.TotalSize = Size - sizeof(asset_memory_block);
                Assets->FirstBlock.UsedSize = 0

in the Assets->FirstBlock.TotalSize, we exclude the size we allocated for the FirstBlock.




32:17
then in our AcquireAssetMemory(); function, we will get memory from our memory blocks

if you noticed, with this implementation, the Assert will always trigger. Becuz we will
run out of asset space.




-   you may notice the we had the EvictAssetsAsNecessary(); call on top.

                void EvictAssetsAsNecessary()
                {
                    ...
                    ReleaseAssetMemory();
                    ...
                }

                inline void ReleaseAssetMemory(game_assets *Assets, memory_index Size, void *Memory)
                {
                    if(Memory)
                    {
                        Assets->TotalMemoryUsed -= Size;
                    }
                    Platform.DeallocateMemory(Memory);
                }

    at that timestamp of the video, the function is still DeallocatingMemory from the Platform layer,
    but not from our assets->FirstBlock memory. 
    [im actually slightly quite unsure about what will the EvictAssetsAsNecessary(); call do over here]


-   you can see that what we are doing here is we just keep on allocating memory from our FirstBlock

    Frame 1
             _______________    
            |               |
            |   FirstBlock  |
            |_______________|       
            |###############|
            |###############|
            |               |
            |               |
            |               |
            |               |
            |_______________|

    Frame 2

    void* Result = (u8*)(Block + 1) + Block->UsedSize;
             _______________    
            |               |
            |   FirstBlock  |
            |_______________|       
            |###############|
            |###############|
            |###############|
            |###############|
            |###############|
            |               |
            |_______________|


    Frame 3

    void* Result = (u8*)(Block + 1) + Block->UsedSize;
             _______________    
            |               |
            |   FirstBlock  |
            |_______________|       
            |###############|
            |###############|
            |###############|
            |###############|
            |###############|
            |###############|  
            |###############|

    then the Assert gets triggered!




-   full code below:
                handmade_asset.h

                internal void * AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    // NOTE(casey): This is the platform memory path
                    EvictAssetsAsNecessary(Assets);

                    #if 0
                        // NOTE(casey): This is the platform memory path
                        Result = Platform.AllocateMemory(Size);
                    #else
                        asset_memory_block* Block = Assets->FirstBlock;
                        Assert((Block->UsedSize + Size) < Block->TotalSize);
                        void* Result = (u8*)(Block + 1) + Block->UsedSize;
                        Block->UsedSize += (u32)Size;

                    #endif 

                    if(Result)
                    {
                        Assets->TotalMemoryUsed += Size;
                    }
                    return(Result);
                }




36:14
then continuing on the step by step migration to our own memory allocation
this assert pretty much is telling us when we are overflowing our memory, 
so instead of an assert, we replace it with an "if statement ", and we put the EvictAssetsAsNecessary(); call there.


                handmade_asset.h

                internal void * AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {


                    #if 0
                        // NOTE(casey): This is the platform memory path
                        Result = Platform.AllocateMemory(Size);
                    #else
                        asset_memory_block* Block = Assets->FirstBlock;
                        if((Block->UsedSize + Size) < Block->TotalSize)
                        {
                            // NOTE(casey): This is the platform memory path
                            EvictAssetsAsNecessary(Assets);
                        }

                        Assert((Block->UsedSize + Size))
                        void* Result = (u8*)(Block + 1) + Block->UsedSize;
                        Block->UsedSize += (u32)Size;

                    #endif 

                    if(Result)
                    {
                        Assets->TotalMemoryUsed += Size;
                    }
                    return(Result);
                }


38:40
Casey starting to consider how do we actually free memory. 
Casey added a enum asset_memory_block_flags, and changed the definition of the asset_memory_block.

notice that asset_memory_block got Prev and Next. so you can see we are going down the doubly linked list route


                enum asset_memory_block_flags
                {
                    AssetMemory_Used = 0x1,
                };
                struct asset_memory_block
                {
                    asset_memory_block *Prev;
                    asset_memory_block *Next;
                    u64 Flags;
                    memory_index Size;
                };

                struct game_assets
                {
                    // TODO(casey): Not thrilled about this back-pointer
                    struct transient_state *TranState;

                    asset_memory_block MemorySentinel;
                    
                    u64 TargetMemoryUsed;
                    u64 TotalMemoryUsed;

                    ...
                    ...
                };


so inside the AllocateGameAssets(); function, we initialize our MemorySentinel node.


                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);

                    Assets->MemorySentinel.Flags = 0;
                    Assets->MemorySentinel.Size = 0;
                    Assets->MemorySentinel.Prev = &Assets->MemorySentinel;
                    Assets->MemorySentinel.Next = &Assets->MemorySentinel;

                    InsertBlock(&Assets->MemorySentinel, Size, PushSize(Arena, Size));
                    
                    ...
                    ...
                }


also we call InsertBlock(); and we intialize our first memory block.
you can see that for block size, we always do 
                
                Block->Size = Size - sizeof(asset_memory_block); 

which we exclude the size of our block header 

                internal asset_memory_block *
                InsertBlock(asset_memory_block *Prev, u64 Size, void *Memory)
                {
                    Assert(Size > sizeof(asset_memory_block));
                    asset_memory_block *Block = (asset_memory_block *)Memory;
                    Block->Flags = 0;
                    Block->Size = Size - sizeof(asset_memory_block);
                    Block->Prev = Prev;
                    Block->Next = Prev->Next;
                    Block->Prev->Next = Block;
                    Block->Next->Prev = Block;
                    return(Block);
                }


visually we have. That first bllock will have the entire memory from the memory arena

    struct game_assets
    {                                                            _______________
        ...                                                     |               |
        asset_memory_block MemorySentinel;      -------->       |  first block  |
                                                                |_______________|
        ...                                                     |               |
        ...                                                     |               |
                                                                |               |
                                                                |               |
    };                                                          |               |
                                                                |               |
                                                                |               |
                                                                |               |
                                                                |               |
                                                                |               |
                                                                |               |
                                                                |_______________|



45:58
so for our 2nd draft our AcquireAssetMemory(); function, we will just loop through memory blocks 
to see if there is a block that satisfy our memory needs. 


-   you can see that our code is in a for(;;) loop.
    every iteration, we try to find a valid block. If we cant do so, we Evict an asset.

    we keep the loop running until we evicted enough blocks to find a valid block 

-   full code below:

                handmade_asset.cpp

                internal void * AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    void *Result = 0;
                #if 0
                    // NOTE(casey): This is the platform memory path
                    EvictAssetsAsNecessary(Assets);
                    Result = Platform.AllocateMemory(Size);
                #else
                    for(;;)
                    {
                        asset_memory_block *Block = FindBlockForSize(Assets, Size);
                        if(Block)
                        {
                            .....................................................
                            ....... Use that Block and Split that block .........
                            .....................................................
                            
                            break;
                        }
                        else
                        {
                            .....................................................
                            ................... Evict an Asset ..................
                            .....................................................
                        }
                    }
                #endif
                    
                    if(Result)
                    {
                        Assets->TotalMemoryUsed += Size;
                    }
                    return(Result);
                }






48:08
for the Evict an Asset logic, 
as a brute force approach, we just do the same as what we did in the EvictAssetsAsNecessary(); function 

we just Evict the first asset that is loaded.

                handmade_asset.cpp

                internal void * AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    void *Result = 0;
                #if 0
                    // NOTE(casey): This is the platform memory path
                    EvictAssetsAsNecessary(Assets);
                    Result = Platform.AllocateMemory(Size);
                #else
                    for(;;)
                    {
                        asset_memory_block *Block = FindBlockForSize(Assets, Size);
                        if(Block)
                        {
                            .....................................................
                            ....... Use that Block and Split that block .........
                            .....................................................
                            
                            break;
                        }
                        else
                        {
                            for(asset_memory_header *Header = Assets->LoadedAssetSentinel.Prev;   <-----------------
                                Header != &Assets->LoadedAssetSentinel;
                                Header = Header->Prev)
                            {
                                asset *Asset = Assets->Assets + Header->AssetIndex;
                                if(GetState(Asset) >= AssetState_Loaded)
                                {
                                    EvictAsset(Assets, Header);
                                    // TODO(casey): Actually do this, instead of just saying you're going to do it!
                //                    Block = EvictAsset(Assets, Header);
                                    break;
                                }
                            }
                        }
                    }
                #endif
                    
                    if(Result)
                    {
                        Assets->TotalMemoryUsed += Size;
                    }
                    return(Result);
                }


50:16
Casey writes the FindBlockForSize(); function

the logic is pretty straight forward. if the block is not used, and the size qualifies
then we return that block.

                handmade_asset.h

                internal asset_memory_block* FindBlockForSize(game_assets *Assets, memory_index Size)
                {
                    asset_memory_block *Result = 0;
                    
                    // TODO(casey): Best match block!
                    for(asset_memory_block *Block = Assets->MemorySentinel.Next;
                        Block != &Assets->MemorySentinel;
                        Block = Block->Next)
                    {
                        if(!(Block->Flags & AssetMemory_Used))
                        {
                            if(Block->Size >= Size)
                            {
                                Result = Block;
                                break;
                            }
                        }
                    }

                    return(Result);
                }


52:25
Casey coming back to do the logic for splitting the block 
what we want to do is that if the amount of space that was left over after splitting was going to be useful(above a certain threshold);
then we would split the block. Otherwise we wouldnt split it. we will just use the whole block


-   we first set this block to be used 

                Block->Flags |= AssetMemory_Used;

-   the memory pointer that we are actually returning is 

                Result = (u8 *)(Block + 1);

    recall that Block + 1 in pointer arithmatic is Block + sizeof(asset_memory_block);
    cuz the memory starts after our asset_memory_block.

-   then we check if we want to split, depending on the remaining size would be useful.

                memory_index RemainingSize = Block->Size - Size;
                memory_index BlockSplitThreshold = 4096; // TODO(casey): Set this based on the smallest asset?
                if(RemainingSize > BlockSplitThreshold)
                {
                    ...
                }

-   if we do split the size, we create a new block:

                if(RemainingSize > BlockSplitThreshold)
                {
                    Block->Size -= RemainingSize;
                    InsertBlock(Block, RemainingSize, (u8 *)Result + Size);
                }

-   full code below:

                handmade_asset.cpp

                internal void * AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    void *Result = 0;
                #if 0
                    // NOTE(casey): This is the platform memory path
                    EvictAssetsAsNecessary(Assets);
                    Result = Platform.AllocateMemory(Size);
                #else
                    for(;;)
                    {
                        asset_memory_block *Block = FindBlockForSize(Assets, Size);
                        if(Block)
                        {
                            Block->Flags |= AssetMemory_Used;
                            
                            Assert(Size <= Block->Size);
                            Result = (u8 *)(Block + 1);

                            memory_index RemainingSize = Block->Size - Size;
                            memory_index BlockSplitThreshold = 4096; // TODO(casey): Set this based on the smallest asset?
                            if(RemainingSize > BlockSplitThreshold)
                            {
                                Block->Size -= RemainingSize;
                                InsertBlock(Block, RemainingSize, (u8 *)Result + Size);
                            }
                            else
                            {
                                // TODO(casey): Actually record the unused portion of the memory
                                // in a block so that we can do the merge on blocks when neighbors
                                // are freed.
                            }
                            
                            break;
                        }
                        else
                        {
                            .....................................................
                            ................... Evict an Asset ..................
                            .....................................................
                        }
                    }
                #endif
                    
                    if(Result)
                    {
                        Assets->TotalMemoryUsed += Size;
                    }
                    return(Result);
                }





56:40
Then Casey writes the ReleaseAssetMemory(); function

-   notice how we retrieve the asset_memory_block from memory,
    we know that in our memory structure 

             _______________    
            | asset_memory  |
            | _block        |
            |_______________|       
            |               |
            |               |
            |               |
            |               |
            |               |
            |               |
            |_______________|
     
    the block sits ontop of the actual memory, so we just do 

                asset_memory_block *Block = (asset_memory_block *)Memory - 1;

    and then we just set the usedFlag off 

                Block->Flags &= ~AssetMemory_Used;

-   Casey mentioned that he will do the merging in the next episode.

-   full code below:

                handmade_asset.cpp

                inline void
                ReleaseAssetMemory(game_assets *Assets, memory_index Size, void *Memory)
                {
                    if(Memory)
                    {
                        Assets->TotalMemoryUsed -= Size;
                    }

                #if 0
                    // NOTE(casey): This is the platform memory path
                    Platform.DeallocateMemory(Memory);
                #else
                    asset_memory_block *Block = (asset_memory_block *)Memory - 1;
                    Block->Flags &= ~AssetMemory_Used;
                    // TODO(casey): Merge!
                #endif
                }





1:06:53
if this were a commercial product, will it make sense to use malloc instead our own memory allocator?

if this were a commercial product, i might have gone ahead do the asset tiling, and making sure that I never
have to do asset tiling. Might have. Most of the time, I make sure my allocation just goes through 
fixed block allocation. 

I certainly never call malloc. 
