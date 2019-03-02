Handmade Hero Day 166 - Adding Locks to the Asset Operations

Summary:
corrected the locking mechanism implemented in day 165.

Finished up the generationID scheme that Casey started in day 165. 
[although his implementation in this episode doesnt protect against one of the scenario he mentioned 
in day 165]

moved much of the work from FillGroudChunk to FillGroundChunkWork so that it runs in the background thread
and it wont stall on the main thread.

Casey stresses that he hasnt debugged any of this implementation

Keyword:
multithreaded, Asset


2:19
Casey ADMITS that in the GetAsset(); function, the AtomicCompareExchangeUInt32(); is insufficient 
to protect against multiple threads accessing the LRU list.

                handmade_asset.h

                internal void MoveHeaderToFront(game_assets *Assets, asset *Asset);
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
        --------------->    if(AtomicCompareExchangeUInt32(&Asset->State, AssetState_Operating, State) == State) 
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

                                Asset->State = State;

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





3:01
so what we want to do is to isolate the stuff that ever works with that linked list. 
so we want to make sure anyone who wants to work with the linked list needs the lock.
so we kind of just want to add the concept of the lock here.


4:06
when we are doing AcquireAssetMemory(); one thing we need to do is we pretty much, no two people are calling 
AcquireAssetMemory(); the same time as well.

Essentially Casey says the two choices we have is that 
limit these important parts to only happen on the main thread.

or accepting the fact that we will have a lock if we want multiple threads to touch the Asset system. 
[Previously, we were only touching the sensitive parts on the main thread.]

Casey says he doesnt know what is the best answer without trying to do some profiling.



6:17
Casey examines the LoadBitmap(); function
the important functions are BeginTaskWithMemory();,
AcquireAssetMemory();
AddAssetHeaderToList();
                 
                internal void
                LoadBitmap(game_assets *Assets, bitmap_id ID)
                {
                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value &&
                       (AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
                        AssetState_Unloaded))
                    {
                        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);    <--------
                        if(Task)        
                        {
                            ...
                            ...

                            Asset->Header = (asset_memory_header *)AcquireAssetMemory(Assets, Size.Total);

                            ...
                            ...

                            AddAssetHeaderToList(Assets, ID.Value, Size);

                            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
                        }
                        else
                        {
                            Asset->State = AssetState_Unloaded;
                        }
                    }    
                }

and he realizes the BeginTaskWithMemory(); is not thread safe either.
Casey made the decisions it doesnt make sense for a background thread to spawn another background thread. 

so the smarter thing to do is not have to make BeginTaskWithMemory(); multithreaded.



8:55
Casey looking at the usage pattern of PushBitmap(); and GetBitmap();
if this rendergroup from the mainthread, then go ahead and spawn a background thread

[Casey! we are already only calling this in the main thread..
I suppose the goal here is to make GetBitmap(); multithreaded safe.]


so Casey modified the GetBitmap(); function
-   if we are rendering in the background, then we dont want to spawn a background thread to load it.

                handmade_render_group.cpp

                inline void
                PushBitmap(render_group *Group, bitmap_id ID, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
                {
                    loaded_bitmap *Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
                    if(Group->RendersInBackground && !Bitmap)
                    {
                        LoadBitmap(Group->Assets, ID, true);
                        Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
                    }
                    
                    if(Bitmap)
                    {
                        PushBitmap(Group, Bitmap, Height, Offset, Color);
                    }
                    else
                    {
                        Assert(!Group->RendersInBackground);
                        LoadBitmap(Group->Assets, ID, false);
                        ++Group->MissingResourceCount;
                    }
                }





11:26
Caey editing LoadBitmap(); as well.

Casey passes an "Immediate" flag, to indicate whether we want to load it ondemad or load something in the backround 

you can see that the initalization of a Task 

                Task = BeginTaskWithMemory(); 

is a background task, so if you dont have the "Immediate" flag, we dont call this line.

which is also why that, we have the branching of 

                if(Task)
                {
                    load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work);
                    *TaskWork = Work;
                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                }
                else
                {
                    LoadAssetWorkDirectly(&Work);
                }


-   full code below:

                handmade_asset.cpp

                internal void LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Immediate)
                {
                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value)
                    {
                        if(AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
                           AssetState_Unloaded)
                        {
                            task_with_memory *Task = 0;

                            if(!Immediate)
                            {
                                Task = BeginTaskWithMemory(Assets->TranState);
                            }
                        
                            if(Immediate || Task)        
                            {
                                ...
                                ...

                                Asset->Header = AcquireAssetMemory(Assets, Size.Total, ID.Value);
    
                                ...
                                ...

                                AddAssetHeaderToList(Assets, ID.Value, Size);

                                if(Task)
                                {
                                    load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work);
                                    *TaskWork = Work;
                                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                                }
                                else
                                {
                                    LoadAssetWorkDirectly(&Work);
                                }
                            }
                            else
                            {
                                Asset->State = AssetState_Unloaded;
                            }
                        }
                        else
                        {
                            // TODO(casey): Do we want to have a more coherent story here
                            // for what happens when two force-load people hit the load
                            // at the same time?
                            asset_state volatile *State = (asset_state volatile *)&Asset->State;
                            while(Asset->State == AssetState_Queued) {}
                        }
                    }    
                }


13:12
Casey writing the function LoadAssetWorkDirectly();

                handmade_asset.cpp

                internal void LoadAssetWorkDirectly(load_asset_work *Work)
                {
                    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
                    
                    CompletePreviousWritesBeforeFutureWrites;

                    if(!PlatformNoFileErrors(Work->Handle))
                    {
                        ZeroSize(Work->Size, Work->Destination);
                    }

                    Work->Asset->State = Work->FinalState;
                }


14:18
Casey refactoring the LoadAssetWork(); background task to call LoadAssetWorkDirectly();

                handmade_asset.cpp

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
                {
                    load_asset_work *Work = (load_asset_work *)Data;

                    LoadAssetWorkDirectly(Work);

                    EndTaskWithMemory(Work->Task);
                }





17:53
Casey mention he doesnt know how much people carea bout making LoadSound multithreaded, cuz no one does that.



18:18
Coming back to LoadBitmap(); if we look at the current version of this function. 
What would be nice is to take the lock and do both of these two functions together

Casey suspects that we can just put AcquireAssetMemory(); inside BeginTaskWithMemory();


                handmade_asset.cpp

                internal void LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Immediate)
                {
                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value)
                    {
                        if(AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
                           AssetState_Unloaded)
                        {
                            task_with_memory *Task = 0;

                            if(!Immediate)
                            {
                                Task = BeginTaskWithMemory(Assets->TranState);
                            }
                        
                            if(Immediate || Task)        
                            {
                                ...
                                ...

                                Asset->Header = AcquireAssetMemory(Assets, Size.Total, ID.Value);   <--------------------
    
                                ...
                                ...

                                AddAssetHeaderToList(Assets, ID.Value, Size);   <---------------------------

                                if(Task)
                                {
                                    load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work);
                                    *TaskWork = Work;
                                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                                }
                                else
                                {
                                    LoadAssetWorkDirectly(&Work);
                                }
                            }
                            else
                            {
                                Asset->State = AssetState_Unloaded;
                            }
                        }
                        else
                        {
                            // TODO(casey): Do we want to have a more coherent story here
                            // for what happens when two force-load people hit the load
                            // at the same time?
                            asset_state volatile *State = (asset_state volatile *)&Asset->State;
                            while(Asset->State == AssetState_Queued) {}
                        }
                    }    
                }



20:00
Casey putting AddAssetHeaderToList(); inside AcquireAssetMemory(); 

Casey got rid of the AddAssetHeaderToList(); funciton and replace it with InsertAssetHeaderAtFront();

                handmade_asset.cpp

                internal asset_memory_header* AcquireAssetMemory(game_assets *Assets, u32 Size, u32 AssetIndex)
                {
                    asset_memory_header *Result = 0;
                    
                    asset_memory_block *Block = FindBlockForSize(Assets, Size);
                    for(;;)
                    {
                        if(Block && (Size <= Block->Size))
                        {
                            ..........................................................
                            ........ using and spliting the memory block .............
                            ..........................................................
                            
                            break;
                        }
                        else
                        {
                            ..........................................................
                            ................... evicting an asset ....................
                            ..........................................................
                        }
                    }

                    if(Result)
                    {
                        Result->AssetIndex = AssetIndex;
                        Result->TotalSize = Size;
                        InsertAssetHeaderAtFront(Assets, Result);
                    }

                    return(Result);
                }


23:38
Casey now considering the locking needed for this function
AcquireAssetMemory(); is a very large function with substantial amount of work.

compare to the other function that modifies our LRU list is the GetAsset(); function

                handmade_asset.h
                
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
                            if(AtomicCompareExchangeUInt32(&Asset->State, AssetState_Operating, State) == State)
                            {
                                Result = Asset->Header;
       ----------------->       MoveHeaderToFront(Assets, Asset);


                                if(Asset->Header->GenerationID < GenerationID)
                                {
                                    Asset->Header->GenerationID = GenerationID;
                                }

                                
                                CompletePreviousWritesBeforeFutureWrites;

                                Asset->State = State;

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


The MoveHeaderToFront(); is very light-weight

                handmade_asset.cpp

                internal void
                MoveHeaderToFront(game_assets *Assets, asset *Asset)
                {
                    asset_memory_header *Header = Asset->Header;
                    
                    RemoveAssetHeaderFromList(Header);
                    InsertAssetHeaderAtFront(Assets, Header);
                }

the RemoveAssetHeaderFromList(); and InsertAssetHeaderAtFront(); is just simply modifying asset node pointers. 









25:23
with that in mind thought, for the moment we will just do with a regular lock in the AcquireAssetMemory(); 

                handmade_asset.cpp

                internal asset_memory_header* AcquireAssetMemory(game_assets *Assets, u32 Size, u32 AssetIndex)
                {
                    asset_memory_header *Result = 0;
                    
                    BeginAssetLock(Assets);     <--------------------------
                
                    asset_memory_block *Block = FindBlockForSize(Assets, Size);
                    for(;;)
                    {
                        if(Block && (Size <= Block->Size))
                        {
                            ..........................................................
                            ........ using and spliting the memory block .............
                            ..........................................................
                            
                            break;
                        }
                        else
                        {
                            ..........................................................
                            ................... evicting an asset ....................
                            ..........................................................
                        }
                    }

                    if(Result)
                    {
                        Result->AssetIndex = AssetIndex;
                        Result->TotalSize = Size;
                        InsertAssetHeaderAtFront(Assets, Result);
                    }

                    EndAssetLock(Assets);       <----------------------------

                    return(Result);
                }


we also add it in the GetAsset(); function

                handmade_asset.h

                inline asset_memory_header *GetAsset(game_assets *Assets, u32 ID, u32 GenerationID)
                {
                    Assert(ID <= Assets->AssetCount);
                    asset *Asset = Assets->Assets + ID;
                    
                    asset_memory_header *Result = 0;

                    BeginAssetLock(Assets);     <-----------------------

                    if(Asset->State == AssetState_Loaded)
                    {        
                        Result = Asset->Header;
                        RemoveAssetHeaderFromList(Result);
                        InsertAssetHeaderAtFront(Assets, Result);

                        if(Asset->Header->GenerationID < GenerationID)
                        {
                            Asset->Header->GenerationID = GenerationID;
                        }

                        CompletePreviousWritesBeforeFutureWrites;
                    }

                    EndAssetLock(Assets);       <----------------------
                    
                    return(Result);
                }



32:10
Casey starts writing the BeginAssetLock(); and EndAssetLock(); function

                handmade_asset.h

                inline void
                BeginAssetLock(game_assets *Assets)
                {
                    for(;;)
                    {
                        if(AtomicCompareExchangeUInt32(&Assets->OperationLock, 1, 0) == 0)
                        {
                            break;
                        }
                    }
                }

                inline void
                EndAssetLock(game_assets *Assets)
                {
                    CompletePreviousWritesBeforeFutureWrites;
                    Assets->OperationLock = 0;
                }

you can see that the AtomicCompareExchangeUInt32(); function is operationg on the variable Assets->OperationLock.

                struct game_assets
                {
                    ...
                    ...

                    u32 OperationLock;

                    ...
                    ...
                };

so we added the lock variable in the game_assets struct.

the OperationLock always should be set to 0. when you want to use it, you set it to 1.

34:00
some might ask, normally if you have a thread, you want to put the thread to sleep if you cant get it. 
we are not gonna do that here cuz the threads wont wait that long to get the locks. we dont have that many threads, 
so you kind of just want threads being in a spin lock behaviour, instead of putting them to sleep



35:19
Casey now added the "b32 RendersInBackground;" variable in render_group.

                handmade_render_group.h
                
                struct render_group
                {
                    ...
                    ...

                    uint32 MissingResourceCount;
                    b32 RendersInBackground;        <----------
                };

and when we allocate render groups for fill ground chunks, we will pass in the RendersInBackground flag

                handmade_render_group.cpp

                internal render_group * AllocateRenderGroup(game_assets *Assets, memory_arena *Arena, uint32 MaxPushBufferSize,
                                    b32 RendersInBackground)
                {
                    render_group *Result = PushStruct(Arena, render_group);

                    ...
                    ...

                    Result->MissingResourceCount = 0;
                    Result->RendersInBackground = RendersInBackground;
                    
                    return(Result);
                }


37:46
Casey mentioned that there is no way to really verify unless you do deeper investigation. Casey doesnt 
really want to investigate now. But Casey just wanted to put in something plausible 



38:30
Casey will now tackle the GenerationID scheme 

38:55
Casey got rid of the AssetState_Operating flag. Recall that was Casey_s attempt to implement the lock for the LRU list.
now we are doing a proper lock with 

                AtomicCompareExchangeUInt32(&Assets->OperationLock, 1, 0) == 0;

in day 165 we added the generationID in the asset_memory_header struct

                handmade_asset.h

                struct asset_memory_header
                {
                    asset_memory_header *Next;
                    asset_memory_header *Prev;
                    
                    u32 AssetIndex;
                    u32 TotalSize;
                    u32 GenerationID;   <---------------------
                    union
                    {
                        loaded_bitmap Bitmap;
                        loaded_sound Sound;
                    };
                };



39:50
now in the GetAsset(); function whenever the render_group requests for an asset, it will need to set its 
generationID



41:46
so when we create a render_group, we will need to create its generationID

                handmade_render_group.cpp

                internal render_group * AllocateRenderGroup(game_assets *Assets, memory_arena *Arena, uint32 MaxPushBufferSize,
                                    b32 RendersInBackground)
                {
                    render_group *Result = PushStruct(Arena, render_group);

                    ...
                    ...

                    Result->GenerationID = BeginGeneration(Assets);

                    ...
                    ...

                    return(Result);
                }





42:41
Casey writing the BeginGeneration(); function 
                
                handmade_asset.cpp

                inline u32 BeginGeneration(game_assets *Assets)
                {
                    u32 Result = NextGenerationID++;
                    
                    return(Result);
                }

we add the GenerationID fields in the assets struct 

                struct game_assets
                {
                    u32 NextGenerationID;
                    
                    ...
                    ...
                };


this is the Naive solution, Casey mentioned that if BeginGeneration(); were to be touched by multiple threads,
this wont work.

So we have two ways. one way is to do atomic increment. 
which is to call AtomicCompareExchangeUInt32(); in a loop. 

But there is an instrinsics that can already do that already 

                handmade_intrinsics.h

                inline uint32 AtomicAdd(uint32 volatile* Value, uint32 Addend)
                {
                    uint32 Result = _InterlockedExchangeAdd((long *)Value, Addend) + Addend;

                    return(Result);
                }


notice that for uint32 Result, we have to add + Addend cuz in the specs it says _InterlockedExchangeAdd(); returns 
the initial value.


                handmade_asset.h

                inline u32 NewGenerationID(game_assets* Assets)
                {
                    u32 Result = AtomicAdd(&Assets->NextGenerationID, 1);

                    return (Result);
                }






51:08
now that rendergroups all have generationIDs, we have to make sure that we dont evict assets that are in flight.
meaning we dont evict assets that are currently used in this frame

so the simplest way to do this is to keep an list of assets that are "in flight". 

we go to the evict asset logic

-   in the part where we iterate through our LRU list, and we check whether we can evict an asset, 
    we add an extra check in the if condition

                if((Asset->State >= AssetState_Loaded) &&
                   (GenerationHasCompleted(Assets, Asset->Header->GenerationID)))

-   full code below:


                handmade_asset.cpp

                internal asset_memory_header* AcquireAssetMemory(game_assets *Assets, u32 Size, u32 AssetIndex)
                {
                    asset_memory_header *Result = 0;

                    BeginAssetLock(Assets);
                    
                    asset_memory_block *Block = FindBlockForSize(Assets, Size);
                    for(;;)
                    {
                        if(Block && (Size <= Block->Size))
                        {
                            .........................................................
                            .......... using and splitting memory block .............
                            .........................................................
                        }
                        else
                        {
                            for(asset_memory_header *Header = Assets->LoadedAssetSentinel.Prev;
                                Header != &Assets->LoadedAssetSentinel;
                                Header = Header->Prev)
                            {
                                asset *Asset = Assets->Assets + Header->AssetIndex;
                                if((Asset->State >= AssetState_Loaded) &&
                                   (GenerationHasCompleted(Assets, Asset->Header->GenerationID)))
                                {
                                    ..............................................
                                    ........ evicting single asset ...............
                                    ..............................................                                    
                                }
                            }
                        }
                    }

                    ...
                    ...

                    EndAssetLock(Assets);
                    
                    return(Result);
                }


52:38

Casey adding the InFlightGenerations buffer

                handmade_asset.h

                struct game_assets
                {
                    ...
                    ...

                    u32 InFlightGenerationCount;
                    u32 InFlightGenerations[16];
                };


Casey also says that this InFlightGenerations and InFlightGenerationCount wants to be protected as well.
[he will do that later]





53:47
in the AllocateGameAssets(); function, we initalize the the Ids to zeros.

                handmade_asset.cpp

                internal game_assets * AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);

                    Assets->NextGenerationID = 0;
                    Assets->InFlightGenerationCount = 0;    

                    ...
                    ...
                }



54:20
Recall Casey mentioned that when we generate ID for rendergroups, the generationId list needs to be protected.
So Casey modified the BeginGeneration(); and he got rid of the atomic add, 

so Casey added the Asset Locks in the function as we modify the generationId list. 

                handmade_asset.h

                inline u32 BeginGeneration(game_assets *Assets)
                {
                    BeginAssetLock(Assets);

                    Assert(Assets->InFlightGenerationCount < ArrayCount(Assets->InFlightGenerations));
                    u32 Result = Assets->NextGenerationID++;
                    Assets->InFlightGenerations[Assets->InFlightGenerationCount++] = Result;

                    EndAssetLock(Assets);
                    
                    return(Result);
                }





56:20
Casey writing the GenerationHasCompleted(); function. Since this function is only ever called inside the lock,
so we wont put locks around it. 

so if we find the CheckId is in the list, 

                handmade_asset.h

                internal b32 GenerationHasCompleted(game_assets *Assets, u32 CheckID)
                {
                    b32 Result = true;
                    
                    for(u32 Index = 0; Index < Assets->InFlightGenerationCount; ++Index)
                    {
                        if(Assets->InFlightGenerations[Index] == CheckID)
                        {
                            Result = false;
                            break;
                        }
                    }

                    return(Result);
                }


[this doesnt check against the case mentioned in day 165, right?
     _______________
    |               |
    | render group  |
    |               |
    |   id: 3       |                     
    |_______________|  
                                      
     _______________
    |               |
    | render group  |           used asset GenerationId = 5
    |               |
    |   id: 4       |
    |_______________|

     _______________
    |               |
    | render group  |
    |               |
    |   id: 5       |
    |_______________|


render group 5 might complete first. then 3, finally 4. 

so if 5 finishes, that means this asset is 5,
and GenerationHasCompleted(); will return true. So this asset is good to be evicted.

but 3 and 4 are still using it.]






57:46
we also have this EndGeneration();

the deletion is just doing the swap with end technique

                handmade_asset.h

                inline void EndGeneration(game_assets *Assets, u32 GenerationID)
                {
                    BeginAssetLock(Assets);

                    for(u32 Index = 0; Index < Assets->InFlightGenerationCount; ++Index)
                    {
                        if(Assets->InFlightGenerations[Index] == GenerationID)
                        {
                            Assets->InFlightGenerations[Index] = Assets->InFlightGenerations[--Assets->InFlightGenerationCount];
                            break;
                        }
                    }    

                    EndAssetLock(Assets);    
                }






59:03
now the question is to we have to figure where to get rid of the rendergroup. this way we can call 
the EndGeneration();


                internal void FinishRenderGroup(render_group *Group)
                {
                    if(Group)
                    {
                        EndGeneration(Group->Assets, Group->GenerationID);
                    }
                }


in the FillGroundChunkWork(); function we call FinishRenderGroup();

                internal void FillGroundChunkWork()
                {
                    ...
                    ...

                    Assert(AllResourcesPresent(RenderGroup));

                    RenderGroupToOutput(RenderGroup, Buffer);       <------------------------
                    FinishRenderGroup(RenderGroup);

                    EndTaskWithMemory(Work->Task);
                }



also in the main thread, we would call 

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...
    
                    TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer);    
  -------------->   FinishRenderGroup(RenderGroup); 

                    // TODO(casey): Make sure we hoist the camera update out to a place where the renderer
                    // can know about the location of the camera at the end of the frame so there isn't
                    // a frame of lag in camera updating compared to the hero.
                    EndSim(SimRegion, GameState);
                    EndTemporaryMemory(SimMemory);
                    EndTemporaryMemory(RenderMemory);
                    
                    CheckArena(&GameState->WorldArena);
                    CheckArena(&TranState->TranArena);

                    END_TIMED_BLOCK(GameUpdateAndRender);
                }


1:03:43
Casey stresses that only in theory, he implemented assets in-flight bug
this is more of a to-be-continued thing 



1:17:20
someone in the Q/A Asked 
dont you need to move all ground chunk work in the separate thread, at the moment it seems like only the 
RenderGroupToOutput is being done in the background task.

1:21:20
then he followed up with a question:
now it seems like you will be stalling on the LoadBitmap(); function in the main thread. 

This make sense cuz the FillGroundChunk(); is called in the main thread, and we will be stalling when we load the asset there

so what is happening is that we create a RenderGroup that has the RendersInBackground flag set to true 

                handmade.cpp

                interval void FillGroudChunk()
                {
                    fill_ground_chunk_work *Work = (fill_ground_chunk_work *)Data;
                            
                    ...
                    ...

                    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &Task->Arena, 0, true);  <-------------- we pass a true flag here

                    ...

                        for(uint32 GrassIndex = 0; GrassIndex < 100; ++GrassIndex)
                        {
                            ...
                            ...
                            PushBitmap(RenderGroup, Stamp, 2.0f, V3(P, 0.0f), Color);
                        }

                }

then in the FillGroundChunk(); function we will be calling PushBitmap(); 


Recall in the PushBitmap(); function, if the Group->RendersInBackground has the flag on
we will call LoadBitmap(); immediately. 

                handmade_render_group.cpp

                inline void PushBitmap(render_group *Group, bitmap_id ID, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
                {
                    loaded_bitmap *Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
                    if(Group->RendersInBackground && !Bitmap)
                    {
                        LoadBitmap(Group->Assets, ID, true);
                        Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
                    }
                    
                    ...
                    ...
                }

that will stall up the main thread. cuz LoadBItmap with the Immediate flag is an expensive function.
we dont want to do that in the main thread. that is what we meant by stalling up the main thread. 

So casey proceeds to move everything in the FillGroudChunk into FillGroundChunkWork, which is done in the background thread.


so our FillGroundChunk looks like below:

                handmade.cpp

                internal void FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
                {    
                    task_with_memory *Task = BeginTaskWithMemory(TranState);
                    if(Task)
                    {
                        fill_ground_chunk_work *Work = PushStruct(&Task->Arena, fill_ground_chunk_work);
                        Work->Task = Task;
                        Work->TranState = TranState;
                        Work->GameState = GameState;
                        Work->GroundBuffer = GroundBuffer;
                        Work->ChunkP = *ChunkP;
                        GroundBuffer->P = *ChunkP;
                        Platform.AddEntry(TranState->LowPriorityQueue, FillGroundChunkWork, Work);            
                    }
                }


and the FillGroundChunkWork function now looks like below:

which has everything.

                handmade.cpp

                internal PLATFORM_WORK_QUEUE_CALLBACK(FillGroundChunkWork)
                {
                    fill_ground_chunk_work *Work = (fill_ground_chunk_work *)Data;
                            
                    ...
                    ...

                    render_group *RenderGroup = AllocateRenderGroup(Work->TranState->Assets, &Work->Task->Arena, 0, true);

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

                    Assert(AllResourcesPresent(RenderGroup));

                    RenderGroupToOutput(RenderGroup, Buffer);
                    FinishRenderGroup(RenderGroup);

                    EndTaskWithMemory(Work->Task);
                }




1:31:12
one big change that Casey made is that in LoadBitmap(); he added a spin lock 
cuz multiple thread calling LoadBitmap(); threads are not guaranteed to get it immediately.
so while the asset is queued, I wait to load it

-   note that 
                asset_state volatile *State = (asset_state volatile *)&Asset->State;

we have the volatile keyword 


                handmade_asset.cpp

                internal void LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Immediate)
                {
                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value)
                    {
                        if(AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) == AssetState_Unloaded)
                        {
                            ...........................................................
                            ................ loading the bitmap asset .................
                            ...........................................................
                        }
                        else
                        {
                            // TODO(casey): Do we want to have a more coherent story here
                            // for what happens when two force-load people hit the load
                            // at the same time?
                            asset_state volatile *State = (asset_state volatile *)&Asset->State;
                            while(State == AssetState_Queued) {}     <----------------------------------
                        }
                    }    
                }

