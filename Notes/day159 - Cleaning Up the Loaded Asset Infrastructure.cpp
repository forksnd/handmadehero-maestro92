Handmade Hero Day 159 - Cleaning Up the Loaded Asset Infrastructure

Summary:

redesigned the concepto of AssetState_Locked in our asset system. Making it a flag, rather than a state.

cleaned the Loaded Asset Infrastructure, collapses the asset array and asset_slot array into one array.
Also, entries of the asset array points into our LRU doubly linked list nodes. 

reduced the size of the memory usage for the asset tables

in the Q/A mentioned the drawback of this doubly linked-list implementaion for Least Recently Used asset management.


Keyword:
Assets, Memory, Least Recently Used


2:24
recall in day 158, we added the AssetState_Locked flag.


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

The plan is that, assets are int AssetState_Locked state, we cant evict them

so the plan is that we want to set the background tasks to be in AssetState_Locked state.

AssetState_Locked, our definition is that some background tasks are using it, and its not safe to evict it.


5:32
so in the LoadBitmap(); function, we pass in a b32 Locked flag to indicate whether 
this asset, after loading, should be in an AssetState_Locked state.
    
                Work->FinalState = (AssetState_Loaded) | (Locked ? AssetState_Lock : 0);        <------------


-   the AssetState_Loaded means indiciates whether if an asset is done loading. 
                
    notice we also added a check 

                if(!Locked)
                {
                    AddAssetHeaderToList(Assets, ID.Value, Size);
                }

-   we are doing this cuz we only want to add assets to our doubly linkes list if we are okay with evicting them 
    at some point. So if its a locked asset, we just never add them to our doubly linked list. 

-   full code below

                handmade_asset.cpp

                internal void LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Locked)
                {
                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value &&
                       (AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
                        AssetState_Unloaded))
                    {
                        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
                        if(Task)        
                        {
                            ...
                            ...
                            ...
                            ...
                            

                            load_asset_work *Work = PushStruct(&Task->Arena, load_asset_work);
                            ...
                            ...
                            Work->FinalState = (AssetState_Bitmap) | (Locked ? AssetState_Lock : AssetState_Loaded);              

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



9:33
Casey adding the "b32 AssetsShouldBeLocked;" field in the render_group struct

                handmade_render_group.h

                struct render_group
                {
                    ...
                    ...

                    b32 AssetsShouldBeLocked;
                };


7:58
we also modify the PushBitmap(); function to pass in the Locked flag.

                handmade_render_group.cpp

                inline void
                PushBitmap(render_group *Group, bitmap_id ID, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
                {
                    loaded_bitmap *Bitmap = GetBitmap(Group->Assets, ID, Group->AssetsShouldBeLocked);
                    if(Bitmap)
                    {
                        PushBitmap(Group, Bitmap, Height, Offset, Color);
                    }
                    else
                    {
                        LoadBitmap(Group->Assets, ID, Group->AssetsShouldBeLocked);
                        ++Group->MissingResourceCount;
                    }
                }



10:16 
then we modify our GetBitmap(); call. 

notice that we added an Assert();

                Assert(!MustBeLocked || IsLocked(Asset));

the idea is that when you have an asset that is already loaded, it should either be in the AssetState_Loaded or AssetState_Locked
so if you pass in the MustBeLocked flag, and you found out that your asset is not locked, we want to assert that then. 

-   code below
                handmade_asset.h

                inline loaded_bitmap *GetBitmap(game_assets *Assets, bitmap_id ID, b32 MustBeLocked)
                {
                    Assert(ID.Value <= Assets->AssetCount);
                    asset *Asset = Assets->Assets + ID.Value;
                    
                    loaded_bitmap *Result = 0;
                    if(GetState(Asset) >= AssetState_Loaded)
                    {
                        Assert(!MustBeLocked || IsLocked(Asset));   <-------
                        CompletePreviousReadsBeforeFutureReads;
                        Result = &Asset->Header->Bitmap;
                    }    

                    return(Result);
                }


13:34
Casey mentioned that in EvictAssetsAsNecessary(game_assets* Assets); 
it is possible to call Evict on an asset, thats not yet loaded 

our original code is below:

                handmade_asset.cpp

                internal void
                EvictAssetsAsNecessary(game_assets *Assets)
                {
                    while(Assets->TotalMemoryUsed > Assets->TargetMemoryUsed)
                    {
                        asset_memory_header *Asset = Assets->LoadedAssetSentinel.Prev;
                        if(Asset != &Assets->LoadedAssetSentinel)
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

so what we need to do is to actually iterate through asset, until we find one that is loaded


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


17:01
Casey now finishing up the part he didnt to do in the Least Recently used Asset Eviction scheme,
which is that everytime you used ans asset, you move it to the front of our doubly linked list

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
                        MoveHeaderToFront();        <--------------------
                    }    

                    return(Result);
                }


the MoveHeaderToFront(); function is pretty forward. 
                handmade_asset.cpp

                internal void
                MoveHeaderToFront(game_assets *Assets, asset *Asset)
                {
                    if(!IsLocked(Asset))
                    {
                        asset_memory_header *Header = Asset->Header;
                    
                        RemoveAssetHeaderFromList(Header);
                        InsertAssetHeaderAtFront(Assets, Header);
                    }
                }


30:28
Upont more thinking, Casey redeisgned the concept of AssetState_Locked in our asset system

AssetState_Locked is really a flag not a state.

so Casey refactored the asset_state enums. Now AssetState_Lock is a flag that everyone can have on it. 

                handmade_asset.h

                enum asset_state
                {
                    AssetState_Unloaded,
                    AssetState_Queued,
                    AssetState_Loaded,
                    AssetState_StateMask = 0xFFF,

                    AssetState_Lock = 0x10000,
                };

[I would make it more explicit, rather than making it a part of a asset_state enums]


31:36
Casey added the function

                handmade_asset.h                

                inline b32
                IsLocked(asset *Asset)
                {
                    b32 Result = (Asset->State & AssetState_Lock);
                    return(Result);
                }


so Casey mentioned that an Asset could either be in AssetState_Unloaded, AssetState_Queued or AssetState_Loaded.
and an asset could be AssetState_Lock regardless of its Unloaded, Queued or Loaded state.

its not like an asset could only be locked after it is loaded.




33:30
and in our LoadBitmap(); function, we edit how we set our AssetState_Lock flag.

-   we first set the FinalState of the asset to have a AssetState_Lock flag after the asset gets loaded 

                Work->FinalState = (AssetState_Loaded) | (Locked ? AssetState_Lock : 0);

-   then as soon as we start loading the bitmap, the asset gets the AssetState_Lock flag.

                 Asset->State |= AssetState_Lock;

and as soon as 

                handmade_asset.cpp

                internal void LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Locked)
                {
                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value &&
                       (AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
                        AssetState_Unloaded))
                    {
                        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
                        if(Task)        
                        {
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


41:11
Casey making a change in the asset tables

Casey combines asset_slot and asset, and got rid of the asset_slot struct.

                struct asset_memory_header
                {
                    asset_memory_header *Next;
                    asset_memory_header *Prev;
                    
                    u32 AssetIndex;
                    u32 TotalSize;
                    union
                    {
                        loaded_bitmap Bitmap;
                        loaded_sound Sound;
                    };
                };

                struct asset
                {
                    u32 State;
                    asset_memory_header *Header;

                    hha_asset HHA;
                    u32 FileIndex;
                };

in terms of the asset tables, 

the AssetTypes, Assets and Tags array are still the same 

asset_type AssetTypes[Asset_Count];                         
                                            asset *Assets;                  asset_tag *Tags;          
                 ___________                                    
                | Asset_    |               hero0   -------------------->   Tag_FacingDirection: up  
                |  hero     |               hero1   -----------             Tag_Height: 180
                |___________|               hero2              \            Tag_Weight: 155 lbs
                | Asset_    |               hero3               \           Tag_Gender: male
                |  Shadow   |               shadow0              ------>    Tag_FacingDirection: left
                |___________|               shadow1                         Tag_Height: 180
                | Asset_    |  --------->   shadow2                         Tag_Weight: 155 lbs
                |  Tree     |               shadow3                         Tag_Gender: male
                |___________|               shadow4                         ...
                |           |               tree0   -------------------->   Tag_Color: green                          
                |  ...      |               tree1                           Tag_Type: oak
                |___________|               tree2   -------------------->   Tag_Color: yellow             
                |           |               tree3                           Tag_Type: oak
                |  ...      |               tree4
                |___________|               tree5
                |           |               ...
                |           |               ...
                |___________|

The difference is inside the Assets array.

we used to have two arrays, the assets array and the asset_slots array

                asset *Assets;                  asset_slot *Slots;

                hero0                           loaded_bitmap0                
                hero1                           loaded_bitmap1 
                hero2                           loaded_bitmap2  
                hero3                           loaded_bitmap3 
                shadow0                         loaded_bitmap4   
                shadow1                         loaded_bitmap5
                shadow2
                shadow3                         ...
                shadow4                         ...
                tree0                           ...
                tree1                           ...
                tree2                           ...
                tree3                           ...
                tree4                           ...
                tree5                 
                ...                   
                ...
                sound0                          loaded_sound0
                sound1                          loaded_sound1


assuming we have 100,000 assets, the memory we need for this is sizeof(asset) + sizeof(Slots);
(this is excluding the memory for bitmaps and sounds, just the structs fo it )


now we got rid of asset_slots, and moved some of the information to the asset_memory_header.
this way sizeof(asset) is reduced and the number of asset_memory_header is proportional to the number of asses 
we load in our game. 


                asset *Assets;                  our doubly linked list

                hero0                            ____________________
                hero1                           | asset data         |
                hero2                           | loaded_bitmap her0 |
    100,000     hero3                           |____________________|                                               
    of          shadow0                             |
    assets      shadow1                             |
                shadow2                             V                        
                                                 _______________________        if we only 1000 of loaded assets
                                                | asset data            |       we only have 1000 asset_memory_header in our memory
                ...                             | loaded_bitmap shadow0 |
                ...                             |_______________________|                                              
                ...                                 |
                                                    |
                tree0                               V
                tree1                            ____________________
                tree2                           | asset data         |
                                                | loaded_bitmap her2 |
                                                |____________________|                                                
                                                    |
                                                    |
                                                    V
                                  
so our memory usage for our asset tables is reduced.


For example, in our LoadBitmap(); function, when we try to load the bitmap, we literally just get the memory 
for our asset_memory_header on the spot. 

                Asset->Header = (asset_memory_header *)AcquireAssetMemory(Assets, Size.Total);   <--------- Header

-   full code below:            
                
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

                            asset_memory_size Size = {};
                            u32 Width = Info->Dim[0];
                            u32 Height = Info->Dim[1];
                            Size.Section = 4*Width;
                            Size.Data = Height*Size.Section;
        getting memory      Size.Total = Size.Data + sizeof(asset_memory_header);
 for the header on demand
      -------------->       Asset->Header = (asset_memory_header *)AcquireAssetMemory(Assets, Size.Total);   <--------------

                            ...
                            ...

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




1:03:00
the linked list way of implementing the LRU scheme seems clean enough. whats the draw back?

Its just doing too much work. whats the minimal amount of work to know if something is LRU?
Just an integer. For example, we can just use an integer to record that this asset was used 
on a certain or not. 

so if you think about the cost of updating one integer, which is 4 byte value (or 8 bytes);
vs 
the cost of moving a node to the front. 
so first we have to remove our current node. which includes editing the "next" pointer and the "prev" pointer 
of nodes on its front and back side.

we add it to the front, which means it has rewrite its own "next" and "prev" pointer 
then it has to change the "next" and "prev" pointer of its new neighbors.

so we are talking about changing 6, 8 bytes values, everytime we touch the resource.

and all of the nodes are scattered in memory, so you may pay for the cost of a cache line miss 
so its just a lot of extra work 

Again, we may not care, since this may be prove to be critical to our performance.
its possible that we just dont hit bitmaps that often to make it matter.



1:17:26
Casey was previously concerned about the size of the audio file 

previously, we had an asset array and a asset_slot array

the size of this asset is the total number of assets 
if we had 100,000 assets in the game, then the size of memory needed was sizeof(asset) + sizeof(asset_slot) * 100,000

so if sizeof(asset) + sizeof(asset_slot) = 128 bytes, we would need 128 * 100,000 bytes of memory, which is 12 megabytes


previously, we had 

                struct asset_slot
                {
                    u32 State;
                    union
                    {
                        loaded_bitmap Bitmap;
                        loaded_sound Sound;
                    };
                };

                struct asset
                {
                    hha_asset HHA;
                    u32 FileIndex;
                };


and the table looks like this, which there is a 1 to 1 mapping

                asset *Assets;                  asset_slot *Slots;

                hero0                           loaded_bitmap0                
                hero1                           loaded_bitmap1 
                hero2                           loaded_bitmap2  
                hero3                           loaded_bitmap3 
                shadow0                         loaded_bitmap4   
                shadow1                         loaded_bitmap5
                shadow2
                shadow3                         ...
                shadow4                         ...
                tree0                           ...
                tree1                           ...
                tree2                           ...
                tree3                           ...
                tree4                           ...
                tree5                 
                ...                   
                ...
                sound0                          loaded_sound0
                sound1                          loaded_sound1
                                
                
now we have changed it, so that we got rid of asset_slot, 
and we put as much info as possible into the asset_memory_header.

this way sizeof(asset); is minimal, and memory we need for all of our asset_memory_header is only proportional
to the number of assets we actually loaded in our game


                struct asset_memory_header
                {
                    asset_memory_header *Next;
                    asset_memory_header *Prev;
                    
                    u32 AssetIndex;
                    u32 TotalSize;
                    union
                    {
                        loaded_bitmap Bitmap;
                        loaded_sound Sound;
                    };
                };

                struct asset
                {
                    u32 State;
                    asset_memory_header *Header;

                    hha_asset HHA;
                    u32 FileIndex;
                };



1:21:18
is the heap and the stack just different sections of the RAM?

Yes.

stack is memory set aside as scratch space for a thread of execution.

heap is memory set aside for dynamic allocation.

the Heap is not really a thing. The heap is just utility code that runs on top of the system memory manager, which is the page table.
which calls VirtualAlloc and VirtualFree.

a heap is just a utility operator that sits ontop of the OS page table

The stack, is just a reserved memory space, that is contiguous.


1:24:41
the OS lives in physical memory space

the physical memory space just has a bunch of 4k pages 

                 ___________
                |           |       
                |   4k      |
                |           |       
                |___________|
                |           |
                |   4k      |
                |           |
                |___________|
                |           |
                |   4k      |
                |           |
                |___________|
                |           |
                |   4k      |
                |           |
                |___________|
                |           |
                |   4k      |
                |           |
                |___________|
                |           |
                |   4k      |
                |           |
                |___________|

the OS has a page table, so if someone wants 16k, the OS just gives him four 4k pages.


                