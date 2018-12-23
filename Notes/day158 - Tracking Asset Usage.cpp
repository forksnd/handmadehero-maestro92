Handmade Hero Day 158 - Tracking Asset Usage

Summary:
First implemented allocation functions using the OS new allocate and free functions.
In this case, we are using the windows VirtualAlloc and VirtFree. 

added the fields in the game_assets to keep track of the memory usage in our asset system

wrote the EvictAssetsAsNecessary(); function at the end of a frame. 

implemented a Least Recently Used scheme for evicting assets 

the Least Recently Used scheme is done using a doubly linked list. 

talked about cache and linked list in the Q/A

Keyword:
Assets, Memory, Least Recently Used

4:51
Casey first gonna change the code so that it is doing allocation from the OS, and then released the memory back to the OS,
everytime we want to load an asset.

on a 64-bit OS, this is probably something you can just ship. Casey doesnt suspect you will run into many problems.
VirtualAlloc should be reasonably fast on Windows.

so to do that Casey added two functions in the platform layer




                handmade_platform.h

                #define PLATFORM_ALLOCATE_MEMORY(name) void *name(memory_index Size)
                typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

                #define PLATFORM_DEALLOCATE_MEMORY(name) void name(void *Memory)
                typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

                ...
                ...

                typedef struct platform_api
                {
                    platform_add_entry *AddEntry;
                    platform_complete_all_work *CompleteAllWork;

                    platform_get_all_files_of_type_begin *GetAllFilesOfTypeBegin;
                    platform_get_all_files_of_type_end *GetAllFilesOfTypeEnd;
                    platform_open_next_file *OpenNextFile;
                    platform_read_data_from_file *ReadDataFromFile;
                    platform_file_error *FileError;

                    platform_allocate_memory *AllocateMemory;           <------------ 
                    platform_deallocate_memory *DeallocateMemory;       <------------
                    
                    debug_platform_free_file_memory *DEBUGFreeFileMemory;
                    debug_platform_read_entire_file *DEBUGReadEntireFile;
                    debug_platform_write_entire_file *DEBUGWriteEntireFile;
                } platform_api;


you can see that PLATFORM_ALLOCATE_MEMORY function just returns a void*, essentially it returns a memory pointer 


8:22
Casey go on to implement the functions in win32_handmade.cpp


                win32_handmade.cpp

                PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
                {
                    void *Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

                    return(Result);
                }

                PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
                {
                    if(Memory)
                    {
                        VirtualFree(Memory, 0, MEM_RELEASE);
                    }
                }



11:57 
now in our LoadBitmap(); call, we need to get enough of memory that we need
the only thing we now know is the requested MemorySize.  

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
                            
                            ...
                            ...
                            Bitmap->Memory = PushSize(&Assets->Arena, MemorySize);       <--------------
                            
                            ...
                            ...
                        }
                        else
                        {
                            Slot->State = AssetState_Unloaded;
                        }
                    }    
                }


13:40
inside game_assets, we are gonna start tracking how much memory we are using. 

so Casey first defined a few variables 


                struct game_assets
                {
                    // TODO(casey): Not thrilled about this back-pointer
                    struct transient_state *TranState;
                    memory_arena Arena;

                    u64 TargetMemoryUsed;
                    u64 TotalMemoryUsed;
                    
                    ...
                    ...
                };




13:58
when we start up, the initalize it to be 0

                handmade_asset.cpp

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);
                    SubArena(&Assets->Arena, Arena, Size);
                    Assets->TranState = TranState;
                    Assets->TotalMemoryUsed = 0;

                    ...
                    ...
                }


then everytime we load somehing, we count towards the Assets->TotalMemoryUsed.

                handmade_asset.cpp

                inline void* AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    void *Result = Platform.AllocateMemory(Size);
                    if(Result)
                    {
                        Assets->TotalMemoryUsed += Size;
                    }
                    return(Result);
                }



so in the LoadBitmap(); call, instead of just straight up calling the PushSize(); function, 
we call this AcquireAssetMemory(); function

                handmade_asset.cpp

                internal void LoadBitmap(game_assets *Assets, bitmap_id ID)
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
                            
                            ...
                            ...
                            Bitmap->Memory = AcquireAssetMemory(Assets, Size.Total); // PushSize(&Assets->Arena, MemorySize);

                            ...
                            ...
                        }
                        else
                        {
                            Slot->State = AssetState_Unloaded;
                        }
                    }    
                }



15:18
then Casey proceeds to write the DeallocateMemory(); function


                handmade_asset.cpp

                inline void
                ReleaseAssetMemory(game_assets *Assets, memory_index Size, void *Memory)
                {
                    if(Memory)
                    {
                        Assets->TotalMemoryUsed -= Size;
                    }
                    Platform.DeallocateMemory(Memory);
                }

16:14 
since we are now not threaded, we dont have to worry about doing an atomic increment or decrement 
for "Assets->TotalMemoryUsed -= Size;"


                handmade_asset.cpp

                inline void* AcquireAssetMemory(game_assets *Assets, memory_index Size)
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


19:30
now we would want to think about how to free memory. 
as mentioned in day 157, we want to free memory at a control point during a frame, where we know we are not at risk
of accidentally freeing assets that we set up previously in our current frame. (this is explained in great detail in day 157);

so what we can do is that at the end of a frame, we call this function EvictAssetsAsNecessary(TranState->Assets);

                #if HANDMADE_INTERNAL
                game_memory *DebugGlobalMemory;
                #endif
                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    Platform = Memory->PlatformAPI;

                    ....................................................
                    .......... all the logic in a frame ................
                    ....................................................


                    // TODO(casey): Make sure we hoist the camera update out to a place where the renderer
                    // can know about the location of the camera at the end of the frame so there isn't
                    // a frame of lag in camera updating compared to the hero.
                    EndSim(SimRegion, GameState);
                    EndTemporaryMemory(SimMemory);
                    EndTemporaryMemory(RenderMemory);

                    EvictAssetsAsNecessary(TranState->Assets);
                    
                    CheckArena(&GameState->WorldArena);
                    CheckArena(&TranState->TranArena);

                    END_TIMED_BLOCK(GameUpdateAndRender);
                }




20:28
Casey writing the EvictAssetsAsNecessary(); function

so we are doing this here, cuz we dont want to make all the evicting logic be super multi-threading locking 

what EvictAsset will do is that it will sit in a loop to decrease the TotalMemoryUsed.

the scheme to free asset is to do least recently used. (Casey doesnt actually write the GetLeastRecentlyUsedAsset(); until later,
this is just him writing usage code)


                handmade_asset.cpp

                internal void EvictAssetsAsNecessary(game_assets *Assets)
                {
                    while(Assets->TotalMemoryUsed > Assets->TargetMemoryUsed)
                    {
                        u32 SlotIndex *Asset = GetLeastRecentlyUsedAsset(Assets);
                        if(SlotIndex)
                        {
                            EvictAsset(Assets, Asset);
                        }
                        else
                        {
                            InvalidCodePath;
                            break;
                        }
                    }
                }




22:39
Then Casey writes the EvictAsset(); function
-   note that we only want to evict asset that are in the AssetState_Loaded state 
hence we added the 

                Assert(GetState(Slot) == AssetState_Loaded);

-   we release the memory by calling 


                ReleaseAssetMemory(Assets, Size.Total, Memory);


-   at the end, we transition the state to be AssetState_Unloaded.

                Slot->State = AssetState_Unloaded;


-   full code below

                internal void EvictAsset(game_assets *Assets, asset_memory_header *Header)
                {
                    u32 SlotIndex = Header->SlotIndex;
                    
                    asset_slot *Slot = Assets->Slots + SlotIndex;
                    Assert(GetState(Slot) == AssetState_Loaded);

                    asset_memory_size Size = GetSizeOfAsset(Assets, GetType(Slot), SlotIndex);
                    void *Memory = 0;
                    if(GetType(Slot) == AssetState_Sound)
                    {
                        Memory = Slot->Sound.Samples[0];
                    }
                    else
                    {
                        Assert(GetType(Slot) == AssetState_Bitmap);
                        Memory = Slot->Bitmap.Memory;
                    }

                    RemoveAssetHeaderFromList(Header);
                    ReleaseAssetMemory(Assets, Size.Total, Memory);
                    
                    Slot->State = AssetState_Unloaded;
                }



31:03
to differentiate between Sound and bitmap assets, Casey added the asset_state flags 

                handmade_asset.h

                enum asset_state
                {
                    AssetState_Unloaded,
                    AssetState_Queued,
                    AssetState_Loaded,
                    AssetState_Locked,
                    AssetState_StateMask = 0xFFF,

                    AssetState_Sound = 0x1000,
                    AssetState_Bitmap = 0x2000,
                    AssetState_TypeMask = 0xF000,
                };



43:03
notice the RemoveAssetHeaderFromList(Header); function. That is a part of the algorithm for the GetLeastRecentlyUsedAsset(); system
Essentially, we need something in our system that lets us know what is our least recently used asset is.

                handmade_asset.cpp

                internal void EvictAssetsAsNecessary(game_assets *Assets)
                {
                    while(Assets->TotalMemoryUsed > Assets->TargetMemoryUsed)
                    {
                        u32 SlotIndex *Asset = GetLeastRecentlyUsedAsset(Assets);
                        if(SlotIndex)
                        {
                            EvictAsset(Assets, Asset);
                        }
                        else
                        {
                            InvalidCodePath;
                            break;
                        }
                    }
                }


Casey first writes a doubly linked list. Which is not a particularly good way of doing this. 


44:11
Casey did mention that, another way to approach the GetLeastRecentlyUsedAsset(); algorithm, you can think of it as a min-heap problem:
you can attach a number to assets and everytime you want LRU, you get your min. 


45:37
Casey starting to write the doubly linked list implementation
Do note that these asset doubly link list nodes also take up memory


                handmade_asset.h

                struct asset_memory_header
                {
                    asset_memory_header *Next;
                    asset_memory_header *Prev;
                    u32 SlotIndex;
                    u32 Reserved;
                };



Then we add a head node in our game_assets struct.
Note that we are using the safeguard method. valid data nodes are starts on the node after this one.

                handmade_asset.h

                struct game_assets
                {
                    ...
                    ...

                    asset_memory_header LoadedAssetSentinel;
                
                    ...
                    ...
                };



everytime an asset is used, we will move it to the front of our list, which means our tail is always the Least Recently Used asset. 

-   notice that we are calling AddAssetHeaderToList();

    we didnt want call this AddAssetHeaderToList(); function in the LoadAssetWork(); cuz we want to avoid 
    calling AddAssetHeaderToList(); in a multi-threading context.

                handmade_asset.cpp

                internal void LoadBitmap(game_assets *Assets, bitmap_id ID)
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

                            asset_memory_size Size = GetSizeOfAsset(Assets, AssetState_Bitmap, ID.Value);
                            Bitmap->Pitch = SafeTruncateToInt16(Size.Section);
                            Bitmap->Memory = AcquireAssetMemory(Assets, Size.Total); // PushSize(&Assets->Arena, MemorySize);

                            ...
                            ...

                            AddAssetHeaderToList(Assets, ID.Value, Bitmap->Memory, Size);

                            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
                        }
                        else
                        {
                            Slot->State = AssetState_Unloaded;
                        }
                    }    
                }




47:50
Casey writes the GetSizeOfAsset(); function
notice the asset_memory_size struct. 

Casey introduced this struct cuz we do lots of memory calculation. And Casey proceeds to replace most memory calculation in the codebase 
with asset_memory_size.

what we are doing here is the size of the data and the size of the asset_memory_header, 
which is the doubly linked list note that we defined

the data calculation is 

                u32 Width = SafeTruncateToUInt16(Info->Dim[0]);
                u32 Height = SafeTruncateToUInt16(Info->Dim[1]);
                Result.Section = 4*Width;
                Result.Data = Height*Result.Section;

the total sum is 

                Result.Total = Result.Data + sizeof(asset_memory_header);

-   full code below:

                handmade_asset..cpp

                asset_memory_size GetSizeOfAsset(game_assets *Assets, u32 Type, u32 SlotIndex)
                {
                    asset *Asset = Assets->Assets + SlotIndex;

                    asset_memory_size Result = {};

                    if(Type == AssetState_Sound)
                    {
                        hha_sound *Info = &Asset->HHA.Sound;

                        Result.Section = Info->SampleCount*sizeof(int16);
                        Result.Data = Info->ChannelCount*Result.Section;
                    }
                    else
                    {
                        Assert(Type == AssetState_Bitmap);
                        
                        hha_bitmap *Info = &Asset->HHA.Bitmap;

                        u32 Width = SafeTruncateToUInt16(Info->Dim[0]);
                        u32 Height = SafeTruncateToUInt16(Info->Dim[1]);
                        Result.Section = 4*Width;
                        Result.Data = Height*Result.Section;
                    }

                    Result.Total = Result.Data + sizeof(asset_memory_header);
                    
                    return(Result);
                }




50:46
Casey writing the AddAssetHeaderToList();


                handmade_asset.cpp

                internal void
                AddAssetHeaderToList(game_assets *Assets, u32 SlotIndex, void *Memory, asset_memory_size Size)
                {
                    asset_memory_header *Header = (asset_memory_header *)((u8 *)Memory + Size.Data);

                    asset_memory_header *Sentinel = &Assets->LoadedAssetSentinel;

                    Header->SlotIndex = SlotIndex;
                    Header->Prev = Sentinel;
                    Header->Next = Sentinel->Next;

                    Header->Next->Prev = Header;
                    Header->Prev->Next = Header;
                }



57:44
Casey writing the RemoveAssetHeaderFromList(); as well

                handmade_asset.cpp

                internal void
                RemoveAssetHeaderFromList(asset_memory_header *Header)
                {
                    Header->Prev->Next = Header->Next;
                    Header->Next->Prev = Header->Prev;

                    Header->Next = Header->Prev = 0;
                }
                   


1:02:00
Interestly, Casey had the doubly linked list safeguard node to point to itself instead of null

that way when we code, we never had to do extra if checks to see if the tail->next points to null or not.

                handmade_asset.cpp

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);
                    SubArena(&Assets->Arena, Arena, Size);
                    Assets->TranState = TranState;
                    Assets->TotalMemoryUsed = 0;
                    Assets->TargetMemoryUsed = Size;

                    Assets->LoadedAssetSentinel.Next = 
                        Assets->LoadedAssetSentinel.Prev =
                        &Assets->LoadedAssetSentinel;

                    ...
                    ...
                }

1:10:16
Someone mentioned in the Q/A, isnt the linked list the thing not to do when you care about cache at all?

Yes, basically if this is something that is critical to performance and this is something that you will be 
heavily trafficking, you wont do it. 
In our situation, we dont know if we will be heavily trafficking


there are ways to alleviate the cache miss. For example, instead of just allocateing the linked list nodes,
we can pre-allocate a block of memory, and allocate nodes from there. That way its all cache friendly. 

you dont really care about whether your code is cache friendly, if you dont even know if that code is running 
very often. 

I wouldnt discourage people from using linked list. If you find out that code is slow, then replace the linked list 
with a faster data structure. 





1:12:36
to make a more cache friend linked list.
the traditional way is 

         ___________                 ___________
        |           |               |           |
        |  asset    |        -----> |  asset    |
        |    data   |       /       |    data   |
        |___________|      /        |___________|
        |   link    | -----         |   link    |
        |___________|               |___________|



                struct asset_memory_header
                {
                    // asset data
                    u32 SlotIndex;
                    u32 Reserved;

                    // link
                    asset_memory_header *Next;
                    asset_memory_header *Prev;

                };

what you will do to make this more cache friendly, is to grab all the links and put them in one buffer 

         _______
        | link  |
        |_______|
        | link  |
        |_______|
        | link  |
        |_______|
        | link  |
        |_______|
        | link  |
        |_______|
        | link  |
        |_______|



