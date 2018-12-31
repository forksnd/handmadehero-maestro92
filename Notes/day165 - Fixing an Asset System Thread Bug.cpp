Handmade Hero Day 165 - Fixing an Asset System Thread Bug

Summary:
Casey attempting to solve a problem in our asset system 
the problem is that we are at risk of freeing used assets

Casey attempted to make LRU list in the GetBitmap(); function thread safe. [but i dont think it works]
Casey got rid of the AssetState_Lock mechanism, which was previously used to protect assets 
in the background thread from getting evicted in the main thread.

started a generationId solution to protect assets set up in the current frame from being evicted. 

Keyword:
Asset, multithreading


4:51
Casey talks about the bug he introduced into our asset system.

in our AcquireAssetMemory(); function, we are calling evict/free memory on demand 

and Casey says we are at risk of freeing things that we set up for the current frame 

you might ask that we have the AssetState_Locked flag protects us from it?
but we were only setting assets in the background thread in the AssetState_Locked.
assets on the main thread are not locked. 


                handmade_asset.cpp

                internal void * AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    void *Result = 0;

                    asset_memory_block *Block = FindBlockForSize(Assets, Size);
                    for(;;)
                    {
                        if(Block && (Size <= Block->Size))
                        {
                            Block->Flags |= AssetMemory_Used;
                            
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
                            for(asset_memory_header *Header = Assets->LoadedAssetSentinel.Prev;
                                Header != &Assets->LoadedAssetSentinel;
                                Header = Header->Prev)
                            {
                                asset *Asset = Assets->Assets + Header->AssetIndex;
                                if(Asset->State >= AssetState_Loaded)
                                {
                                    u32 AssetIndex = Header->AssetIndex;
                                    asset *Asset = Assets->Assets + AssetIndex;
                    
                                    Assert(Asset->State == AssetState_Loaded);

                                    RemoveAssetHeaderFromList(Header);
                                    
                                    Block = (asset_memory_block *)Asset->Header - 1;
                                    Block->Flags &= ~AssetMemory_Used;

                                    if(MergeIfPossible(Assets, Block->Prev, Block))
                                    {
                                        Block = Block->Prev;
                                    }

                                    MergeIfPossible(Assets, Block, Block->Next);

                                    Asset->State = AssetState_Unloaded;
                                    Asset->Header = 0;    
                                    break;
                                }
                            }
                        }
                    }
                    
                    return(Result);
                }



8:05
Casey asks the question, is there a way for us to mark assets definitely as being used or not used in some specific way.
Also Casey doesnt like the "locked" mechanism we have in our game. 


10:11
Casey wants to start brainstorm how to implement in-flight asset projection

in-flight means, some system is in the process of using it. if someone else comes in and messes with it, thats not good. 

if we think about our usage pattern of our assets 
so we have multiple render groups that would use Asset

    Ground Tile 
     _______________
    |               |
    |   render      |
    |     group     |
    |               |                    _______________
    |_______________|  -------------->  |               |
                                        |               |
                                        |   Asset       |
                                        |               |
    Frame              -------------->  |_______________|
     _______________
    |               |
    |  render       |
    |     group     |
    |               |
    |_______________|
    

and they both might be this asset. When the render group starts rendering, they have the asset, when they are done,
they no longer need the asset. 

so there is an interval that the asset is being used by a render group 

so you can sort of see an "acquire" by render group and "release" by render group pattern to it.

kind of like a "reference counting" semantics to it. 

but its different from garbage collection in that we dont get rid of an asset if no one is using it, only
if it has to make way for other assets. 


13:20
obviously we could introduce literal acquire and release semantics, if we wanted to.
we could have an integer on an asset to keep track of its usage count. 

anytime a render group uses it, we increase the number. and 
decreases it if a render group is done with it. 

Here are some problems, 
-   needs to be atomic for multiple threading purposes.
    but this can be solved using atomic increment and atomic decrement. 

-   we dont really know when we are done with them.
    right now we go through all the render groups and we do all the rendering. Acquire is easy, but we dont 
    ever figure out when we release our assets. 

Casey doesnt like this approach cuz its too much complexity for too little gain.
the risk right now is that we will see garbage on screen if we are low in memory. Which will rarely rarely happen.


16:08
Casey is proposing the "ID sweep" approach.
or also called "generation index"

instead of talking about assets as having increment and decrement in them,

we can talk about the render groups being a series and then using that series to lock our assets. 

for example, everytime we create a render group, we give it an id. These id generation are all atomic, so all render group ids 
are unique
     _______________
    |               |
    | render group  |
    |               |
    |   id: 0       |                     
    |_______________|  
                                      
     _______________
    |               |
    | render group  |
    |               |
    |   id: 1       |
    |_______________|
    
everytime we use an asset, instead of incrementing the asset usage count, we could write in the generation index. 

so if the render group uses an asset, it tries to write its id. the asset will keep the latest/largest id. 


if we go down this approach, the asset has the generation index, 
what that means is that until the render group with that number finishes, and all prior render groups finishes, we can not 
gurantee that the asset can be evicted.

so wait until then to evict.

so then what we need to do is retire those render groups in a way that lets the assets know if you are finished. 


20:06
one thing we need to watch out for is to watch out for wrapping. 

so max number for a 32 bit unsigned int is 2^32 = 4294967296

we are running at 60 frames per second
60 seconds in a minute
60 minutes in an hour 
24 hours in a day 

2^32 / 60 / 60 / 60 / 24 = 828.504 days

if you generate ground chunks in the back, so that is 4, 5 render groups in a frame
828.504 / 5 = 165.7

so we can run our game half a year without wrapping.

so not gonna worry about that. 



23:14
the trick part, 
imagine the following scenario.
     _______________
    |               |
    | render group  |
    |               |
    |   id: 3       |                     
    |_______________|  
                                      
     _______________
    |               |
    | render group  |
    |               |
    |   id: 4       |
    |_______________|

     _______________
    |               |
    | render group  |
    |               |
    |   id: 5       |
    |_______________|


render group 5 might complete first.
then 3, finally 4.

so when we ask for the asset completion status, we cant just write 5 to the asset, cuz 3 is still happening.

if 5 finishes, we cant write 5 yet, cuz 3 is still not done. 

so what we really need is the "last known thing that was completed" [TCP???]

so in this case, that will be 2.
if the next render group that is attempt to write in is not the expected one, (+1 of the last known completed id);
then we cant write in it.

[this looks like how TCP implements reliability]

so the assets need to store the received completion ids.
and thats a bit tricky.





27:29
Casey exmaining our current code base and thinking about how to implement this.
The part where the render groups requesting assets happens in the GetBitmap(); and GetSound(); function
                
recall these two functions are only called in the main thread. The background thread doesnt call this.

                handmade_asset.h 

                inline loaded_bitmap *GetBitmap(game_assets *Assets, bitmap_id ID, b32 MustBeLocked)
                {
                    Assert(ID.Value <= Assets->AssetCount);
                    asset *Asset = Assets->Assets + ID.Value;
                    
                    loaded_bitmap *Result = 0;
                    if(GetState(Asset) >= AssetState_Loaded)
                    {
                        Assert(!MustBeLocked || IsLocked(Asset));
                        CompletePreviousReadsBeforeFutureReads;
                        Result = &Asset->Header->Bitmap;
                        MoveHeaderToFront(Assets, Asset);
                    }    

                    return(Result);
                }

                inline loaded_sound *GetSound(game_assets *Assets, sound_id ID)
                {
                    Assert(ID.Value <= Assets->AssetCount);
                    asset *Asset = Assets->Assets + ID.Value;
                    
                    loaded_sound *Result = 0;
                    if(GetState(Asset) >= AssetState_Loaded)
                    {
                        CompletePreviousReadsBeforeFutureReads;
                        Result = &Asset->Header->Sound;
                        MoveHeaderToFront(Assets, Asset);
                    }
                    
                    return(Result);
                }





32:00
Casey is hesitant to go down this approach cuz he begins to see complexity.


Casey mentioned that if we are doing the generation index approach, the doubly linked list will be redundant.
so he is considering changing it into a min heap.

33:41
Casey mentioning that our LRU list will touched by by multiple threads.
[however, currently in our code, only the main thread is touching the LRU list????]

33:53 
Casey once again mentioned that you can take a lock on the LRU list, but every thread who is doing rendering 
and calling GetBitmap(); gets stalled.

[we are only calling GetBitmap(); in the main thread? threads (non-main threads) dont actually call 
    GetBitmap();]


36:47
Casey addressing the problem that if we have multiple threads using the LRU list, there could be dire
consequences. so we need locks around it


37:00
Casey mentioned that we need some way of reliably keeping our LRU list sorted without the restriction
that only one thread can access it.
[again, currently only the main thread uses it. Does Casey plan to have multiple threads touchig it]?

39:10
Casey considering the possbility of a "lock free heap"



48:05
Casey mentioned that would it be better just to take a lock 
quickly do the move and release the lock, becuz uncontended locks are mostly free.



49:00
Casey editing the GetAsset(); function to make multithreading friendly.
-   he puts the main function in a for(;;) loop to keep trying to acquire the asset. 
    the idea is that a thread will try to acquire the asset until it becomes available to him.

-   once the asset is not in AssetState_Operating mode, then the thread gets access to it. 
    In fact we exepct it to be in the AssetState_Loaded sate.

    this is done by calling AtomicCompareExchangeUInt32(&Asset->State, AssetState_Operating, State) == AssetState_Loaded);

-   once one thread has access to it, it will attempt to mark the GenerationID [havent done this part], 

    then afterwards, we set it back to the original state 

                Asset->State = State;

-   full code below:

                inline asset_memory_header *GetAsset(game_assets *Assets, u32 ID)
                {
                    Assert(ID <= Assets->AssetCount);
                    asset *Asset = Assets->Assets + ID;
                    
                    asset_memory_header *Result = 0;
                    for(;;)
                    {
                        u32 State = Asset->State;
                        if(State == AssetState_Loaded)
                        {
                            if(AtomicCompareExchangeUInt32(&Asset->State, AssetState_Operating, State) == AssetState_Loaded)
                            {
                                Result = Asset->Header;
                                MoveHeaderToFront(Assets, Asset);

                #if 0
                                if(Asset->Header->GenerationID < GenerationID)
                                {
                                    Asset->Header->GenerationID = GenerationID;
                                }
                #endif
                                
                                CompletePreviousWritesBeforeFutureWrites;

                                Asset->State = AssetState_Loaded;

                                break;
                            }
                        }
                        else if(State != AssetState_Operating)
                        {
                            break;
                        }
                    }    

                    return(Result);
                }

[i dont see how this locking code protects the situation where two threads 
    calling MoveHeaderToFront on two different nodes.
i think we dont c any bugs cuz GetBitmap(); is currently only being called on the main thread]



52:20
Casey added the GenerationId to the asset_memory_header struct

                handmade_asset.h

                struct asset_memory_header
                {
                    asset_memory_header *Next;
                    asset_memory_header *Prev;
                    
                    u32 AssetIndex;
                    u32 TotalSize;
                    u32 GenerationID;       <-----------
                    union
                    {
                        loaded_bitmap Bitmap;
                        loaded_sound Sound;
                    };
                };



59:50
Casey got rid of all the lock logic in the asset system.



1:05:06
Casey summarized what he did today. 

two things 

Casey wanted to use the GenerationID scheme to mark when an asset is used. This way 
we can protect assets from being evicted.

Casey also wanted to replace the locking mechanism, which is what we previously used to to make sure that 
a thread that was using rendered assets doesnt have to worry that it gets evicted by the main thread. 

But that way implies that assets are loaded forever. which Casey doesnt like. 

So what Casey replaced the locking mechanism with is atomically changing the asset state to AssetState_Operating, 

[dont you want to lock on the LRU list instead of just the asset?, its possible that two threads could try to move a node
to the head of the LRU list at the same time?]

