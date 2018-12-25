Handmade Hero Day 161 - Finishing the General Purpose Allocator

Summary:

implemented merging of asset_memory_block in our memory allocator 

mentioned the concerns with our current general memory allocator

Changed a error handling in our file system

addresses a design decision in reading in files in our file system.

made some slight changes the decouple the file system and the game code 

explained why distance field for fonts are awful in the Q/A

explained the difference between mono spaced fonts and kerned fonts in Q/A

Keyword:
Asset, Memory, file system


3:49
now that we are keep track of memory management through the freeblock nodes.
we can now get rid of the TargetMemoryUsed and TotalMemoryUsed fields in the game_assets struct.

and we also remove the EvictAssetAsNecessary(); function




7:28
Casey mentioned that the flashing is probably from us not implementing the merging of free blocks yet


7:38
Casey added a small optimization in the handmade_asset.cpp code 
recall from day 160, we had the following code:

in side our for(;;) loop, we call FindBlockForSize(); every frame.
and if we cant find a valid block, we Evict an Asset 

                handmade_asset.cpp

                internal void * AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    void *Result = 0;

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
                                .....................................................
                                ................... Splitting a Block................
                                .....................................................
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
                }


Casey added that we dont have to call FindBlockForSize(); every iteration. Whenever we evict an asset,
that asset_s original block is the only possibility that we could fit a block in. Cuz we looked at the blocks 
before and found nothing. Now after evicting, the only possibility is the block from this newly evicted asset

want to find space for 

        DDDDDDDDDDD
        DDDDDDDDDDD
        DDDDDDDDDDD

no space in memory.

             ___________
            |AAAAAAAAAAA|
            |AAAAAAAAAAA|
            |AAAAAAAAAAA|
            |           |                         
            |BBBBBBBBBBB|
            |BBBBBBBBBBB|
            |BBBBBBBBBBB|
            |           |
            |           |                         
            |CCCCCCCCCCC|
            |CCCCCCCCCCC|
            |CCCCCCCCCCC|


so we evict A, 

             ___________
            |           | <----- A_s original block is only possible block that might fit in  
            |           |           (Not saying its a gurantee, might fit in. If we cant fit in, we have to keep on evicting)
            |-----------|
            |           |                         
            |BBBBBBBBBBB|
            |BBBBBBBBBBB|
            |BBBBBBBBBBB|
            |           |
            |           |                         
            |CCCCCCCCCCC|
            |CCCCCCCCCCC|
            |CCCCCCCCCCC|



so now we have this following structure


-   notice that right after the "Removing Asset" logic, we assign block to the original Asset_s 

                Block = (asset_memory_block *)Asset->Header - 1;    <------- we get 

-   full code below:

                handmade_asset.cpp

                internal void *
                AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    void *Result = 0;

                    asset_memory_block *Block = FindBlockForSize(Assets, Size);
                    for(;;)
                    {
                        if(Block)
                        {
                            Block->Flags |= AssetMemory_Used;
                            
                            Result = (u8 *)(Block + 1);

                            .............................................................
                            ............ Using the block and Splitting ..................
                            .............................................................
                            
                            break;
                        }
                        else
                        {
                            for(asset_memory_header *Header = Assets->LoadedAssetSentinel.Prev;
                                Header != &Assets->LoadedAssetSentinel;
                                Header = Header->Prev)
                            {
                                asset *Asset = Assets->Assets + Header->AssetIndex;
                                if(GetState(Asset) >= AssetState_Loaded)
                                {
                                    ..............................................
                                    ............ Removing Asset ..................
                                    ..............................................

                                    
                                    Block = (asset_memory_block *)Asset->Header - 1;    <------- we get 
                                    Block->Flags &= ~AssetMemory_Used;

                                    ...
                                    ...

 
                                    break;
                                }
                            }
                        }
                    }
                    
                    return(Result);
                }


8:27
-   after we free the block, we have to observe either side to see if it can be merged with either side.

                Block = (asset_memory_block *)Asset->Header - 1;
                Block->Flags &= ~AssetMemory_Used;

                if(MergeIfPossible(Assets, Block->Prev, Block))
                {
                    Block = Block->Prev;
                }

                MergeIfPossible(Assets, Block, Block->Next);



10:19
Casey implementing MergeIfPossible();
-   the checks on     
                if((First != &Assets->MemorySentinel) &&
                 (Second != &Assets->MemorySentinel))

    and 
                if(!(First->Flags & AssetMemory_Used) &&
                   !(Second->Flags & AssetMemory_Used))

    is pretty straight forward.

-   then we need to check if the two blocks are contiguous.
    if we only had one big block of memory, they always be contiguous, and we could just merge them.
    but we want to support in case of 32-bit windows. So the possibility that the Asset system has been given multiple 
    separate chunks of memory to manage that may be contiguous in virtual address space. 

    so we dont want to assume these two asset_memory_block of memory actually have adjacency. 

    so we want to make sure the "Size" line up
    essentially we need to check if the math is right

                struct asset_memory_block
                {
                    asset_memory_block *Prev;
                    asset_memory_block *Next;
                    u64 Flags;
                    memory_index Size;  <-----------------
                };


    so we first compute the ExpectedSecond, which is pretty straight-forward.

                u8* ExpectedSecond = (u8 *)First + sizeof(asset_memory_block) + First->Size;
                if((u8 *)Second == ExpectedSecond)
                {
                    ...
                    ...
                }

-   after all the validiation, we merge. Simple doubly linked list operations

                Second->Next->Prev = Second->Prev;
                Second->Prev->Next = Second->Next;
                
                First->Size += sizeof(asset_memory_block) + Second->Size;


notice the the size calculation includes space for the second header 


                 ___________
                |  first    |
                |___________|  
                |           |
                |           |
                |           |   first.size
                |           |
                |           |
                |___________|
                |  second   |   header
                |___________|
                |           |
                |           |   second.size 
                |           |
                |           |
                |           |
                |___________|  

    there we included sizeof(asset_memory_block); in the calculation


-   full code below:


                handmade_asset.cpp
                                
                internal b32 MergeIfPossible(game_assets *Assets, asset_memory_block *First, asset_memory_block *Second)
                {
                    b32 Result = false;
                    
                    if((First != &Assets->MemorySentinel) &&
                       (Second != &Assets->MemorySentinel))
                    {
                        if(!(First->Flags & AssetMemory_Used) &&
                           !(Second->Flags & AssetMemory_Used))
                        {
                            u8 *ExpectedSecond = (u8 *)First + sizeof(asset_memory_block) + First->Size;
                            if((u8 *)Second == ExpectedSecond)
                            {
                                Second->Next->Prev = Second->Prev;
                                Second->Prev->Next = Second->Next;
                                
                                First->Size += sizeof(asset_memory_block) + Second->Size;

                                Result = true;
                            }
                        }
                    }

                    return(Result);
                }



16:05
Casey added an important check.
we still need to make sure the Block is big enough even after merging.

previously we had 

                if(Block)

now we add a size check 

                if(Block && (Size <= Block->Size)) 

-   full code below:

                internal void *
                AcquireAssetMemory(game_assets *Assets, memory_index Size)
                {
                    void *Result = 0;

                    asset_memory_block *Block = FindBlockForSize(Assets, Size);
                    for(;;)
                    {
                        if(Block && (Size <= Block->Size))  <-------------------------  new check
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
                                if(GetState(Asset) >= AssetState_Loaded)
                                {
                                    u32 AssetIndex = Header->AssetIndex;
                                    asset *Asset = Assets->Assets + AssetIndex;
                    
                                    Assert(GetState(Asset) == AssetState_Loaded);
                                    Assert(!IsLocked(Asset));

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



18:18
just to recap, the asset system only call PushSize(); once at initalization


                handmade_asset.cpp

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);

                    ...
                    ...

                    InsertBlock(&Assets->MemorySentinel, Size, PushSize(Arena, Size));  <----------------

                    ...
                    ...
                }

and recall the memory used in the asset system is passed in by us 

                handmade_asset.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    Assert(sizeof(transient_state) <= Memory->TransientStorageSize);    
                    transient_state *TranState = (transient_state *)Memory->TransientStorage;
                    if(!TranState->IsInitialized)
                    {
                        InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                                        (uint8 *)Memory->TransientStorage + sizeof(transient_state));
                        
                        ...
                        ...


                        TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(16), TranState);    <----------------

                        ...
                        ...

                    }


19:12
Casey asked the question: what are the problems in this system


Casey starts by summarizing once again how our scheme works, which is essentially a doubly linked list routine 

                                 _______________
                                |               |
                                |      A        |
                                |               |-------- 
                                |_______________|       |
                                |###############|       |
                                |###############| <------ 
                                |###############|         
                                |###############| -------
                                |###############|       |
                                                        |
                                 _______________        |
                                |               |       |
                                |      B        | <------  
                                |               |                                
                                |               |--------
                                |_______________|       |
                                |###############| <------  
                                |###############|       
                                |###############| -------
                                |###############|       |                         
                                |               |       |
                                |               |       |
                                |      C        | <------ 
                                |               |
                                |_______________|

the problem that Casey forsee is the length of the linked list.

this linked list could potentially get very long

let say we have 4000 assets, that means we potentially have 8000 nodes/links

so assume in one frame, we have to load 5 assets, we could potentially loop through 8000 * 5 nodes


what we probably want to do is to implement a faster search model ontop of our doubly linked list 
[mhmmmm, reminds me of that leetcode LRU cache problem]

the doubly linked list is great for merging. Merging is constant time and very fast.

whats not fast is the iteration

                
                handmade_asset.cpp

                internal asset_memory_block *
                FindBlockForSize(game_assets *Assets, memory_index Size)
                {
                    asset_memory_block *Result = 0;

                    // TODO(casey): This probably will need to be accelerated in the
                    // future as the resident asset count grows.
                    
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


Casey mentioned that he is going to hold off on that. Since we only have 30, 40 assets, we may not see
any significant impovement. So we will wait till when we have more assets.

but down the line, this is something we should be aware of.




26:00
Casey mentioned a problem in the file system.

in the "File->Handle = Platform.OpenNextFile(&FileGroup);", since this is allocation based operation,
its possible for Platform.OpenNextFile(); to fail completely.
the platform layer cant gurantee that you get a handle back. 


                handmade_asset.cpp

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    
                    ...
                    ...

                    platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin(PlatformFileType_AssetFile);
                    Assets->FileCount = FileGroup.FileCount;
                    Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
                    for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
                    {
                        asset_file *File = Assets->Files + FileIndex;

                        File->TagBase = Assets->TagCount;

                        ZeroStruct(File->Header);
        ------------>   File->Handle = Platform.OpenNextFile(FileGroup);
                        Platform.ReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);
                        
                        u32 AssetTypeArraySize = File->Header.AssetTypeCount*sizeof(hha_asset_type);
                        File->AssetTypeArray = (hha_asset_type *)PushSize(Arena, AssetTypeArraySize);
                        Platform.ReadDataFromFile(File->Handle, File->Header.AssetTypes,
                                                  AssetTypeArraySize, File->AssetTypeArray);
                        
                        if(File->Header.MagicValue != HHA_MAGIC_VALUE)
                        {
                            Platform.FileError(File->Handle, "HHA file has an invalid magic value.");
                        }
                    
                        if(File->Header.Version > HHA_VERSION)
                        {
                            Platform.FileError(File->Handle, "HHA file is of a later version.");
                        }
        
                        
                        ...
                        ...
                    }

                    ...
                    ...

                }


and if you think about it, if File->Handle = Platform.OpenNextFile(FileGroup);
returns null properly, all the subsecquent actions

                Platform.ReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);
                
                u32 AssetTypeArraySize = File->Header.AssetTypeCount*sizeof(hha_asset_type);
                File->AssetTypeArray = (hha_asset_type *)PushSize(Arena, AssetTypeArraySize);
                Platform.ReadDataFromFile(File->Handle, File->Header.AssetTypes,
                                          AssetTypeArraySize, File->AssetTypeArray);

wont be valid.



recall in our Platform.OpenNextFile(); which we implemented in the win32_handemade.cpp file,
It does a VirtualAlloc. And then we return a win32_platform_file_handle.

                win32_handemade.cpp

                internal PLATFORM_OPEN_FILE(Win32OpenNextFile);
                {
                    ...
                    ...

                    if(Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE)
                    {    
                        // TODO(casey): If we want, someday, make an actual arena used by Win32
                        Result = (win32_platform_file_handle *)VirtualAlloc(       <---------------------------- 
                            0, sizeof(win32_platform_file_handle),
                            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

                        ...
                        ...
                    }
                    
                    return((platform_file_handle *)Result);
                }


the win32_platform_file_handle struct contains a platform_file_handle struct

                win32_handemade.cpp

                struct win32_platform_file_handle
                {
                    platform_file_handle H;
                    HANDLE Win32Handle;
                };



                handmade_platform.h

                typedef struct platform_file_handle
                {
                    b32 NoErrors;
                } platform_file_handle;



so graphically it looks like this 


    game code                                               win32 platform 


    calls PLATFORM_OPEN_FILE();         
                                ------------------------>
                                                            calls Win32OpenNextFile();
                                                            {
                                                                creates win32_platform_file_handle

                                                                win32_platform_file_handle.h = new platform_file_handle
                                                                
                                                                the win32_platform_file_handle contains the 
                                                                platform_file_handle

                                                                return win32_platform_file_handle.h
                                                            }
                                    returns the 
                                    platform_file_handle
                                    to the game code

    reads the file             <------------------------
    and checks error 
    on the platform_file_handle



but if you think about it, when the win32 platform calls Win32OpenNextFile(); 
it could fail, so that it will fail to return a platform_file_handle to the game side 

and that will mess shit up in the PLATFORM_OPEN_FILE(); game side code.


-   currently the code is like this:

                win32_handemade.cpp

                internal PLATFORM_OPEN_FILE(Win32OpenNextFile)
                {
                    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup;
                    win32_platform_file_handle *Result = 0;             <---------------------- creates win32_platform_file_handle here         

                    if(Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE)
                    {    
                        // TODO(casey): If we want, someday, make an actual arena used by Win32
                        Result = (win32_platform_file_handle *)VirtualAlloc(
                            0, sizeof(win32_platform_file_handle),
                            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

                        if(Result)
                        {
                            char *FileName = Win32FileGroup->FindData.cFileName;
                            Result->Win32Handle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
                            Result->H.NoErrors = (Result->Win32Handle != INVALID_HANDLE_VALUE);
                        }
                        
                        if(!FindNextFileA(Win32FileGroup->FindHandle, &Win32FileGroup->FindData))
                        {
                            FindClose(Win32FileGroup->FindHandle);
                            Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
                        }
                    }
                    
                    return((platform_file_handle *)Result);
                }




27:10
so what Casey wants to do is to change the behaviour of how we process a file handle slightly


Casey redefined the platform_file_handle definition

                win32_handemade.cpp

                struct win32_platform_file_handle
                {
                    HANDLE Win32Handle;
                };



                handmade_platform.h

                typedef struct platform_file_handle
                {
                    b32 NoErrors;
                    void *Platform;
                } platform_file_handle;


now that in Win32OpenNextFile(); we create platform_file_handle
and the platform_file_handle.Platform will store the win32_platform_file_handle.


    game code                                               win32 platform 


    calls PLATFORM_OPEN_FILE();         
                                ------------------------>
                                                            calls Win32OpenNextFile()
                                                            {
                                                                creates platform_file_handle

                                                                platform_file_handle.Platform = new win32_platform_file_handle.

                                                                return platform_file_handle
                                                            }
                                    returns the 
                                    platform_file_handle
                                    to the game code

    reads the file             <------------------------
    and checks error 
    on the platform_file_handle



this way the platform_file_handle will always be valid when it returns to the game code.

and we can always assume platform_file_handle.NoErrors will be there. 



-   full code below:

                internal PLATFORM_OPEN_FILE(Win32OpenNextFile)
                {
                    ...
                    platform_file_handle Result = {};           <---------------------- creates platform_file_handle here

                    if(Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE)
                    {    
                        // TODO(casey): If we want, someday, make an actual arena used by Win32
                        win32_platform_file_handle *Win32Handle = (win32_platform_file_handle *)VirtualAlloc(
                            0, sizeof(win32_platform_file_handle),
                            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
                        Result.Platform = Win32Handle;          <---------------------- assigning the platform_file_handle.Platform 

                        if(Win32Handle)
                        {
                            wchar_t *FileName = Win32FileGroup->FindData.cFileName;
                            Win32Handle->Win32Handle = CreateFileW(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
                            Result.NoErrors = (Win32Handle->Win32Handle != INVALID_HANDLE_VALUE);   
                        }
                        
                        if(!FindNextFileW(Win32FileGroup->FindHandle, &Win32FileGroup->FindData))
                        {
                            FindClose(Win32FileGroup->FindHandle);
                            Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
                        }
                    }
                    
                    return(Result);         <----------- returning it
                }






36:00
Casey addresses another thing in the code 

which is that the Win32GetAllFilesOfTypeBegin(); cant handle unicode file names 


this is becuz we are calling FindFirstFileA(); and functions
recall there are FindFirstFileA() and FindFirstFileW(); versions of the FindFirstFile function.

                internal PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
                {
                    ...
                    ...

                    WIN32_FIND_DATAW FindData;
                    HANDLE FindHandle = FindFirstFileA(WildCard, &FindData);
                    while(FindHandle != INVALID_HANDLE_VALUE)
                    {
                        ++Result.FileCount;
                        
                        if(!FindNextFileW(FindHandle, &FindData))
                        {
                            break;
                        }
                    }
                    FindClose(FindHandle);

                    Win32FileGroup->FindHandle = FindFirstFileA(WildCard, &Win32FileGroup->FindData);
                    
                    return(Result);
                }


Casey initially used the FindFirstFileA(); function. and that was by design.
you may notice that Casey never expose the file names to the game code. that was a deliberate decision.

The reason why Casey made that decision is exactly the issue.  

We dont know what Operating system this is gonna run on, we dont know what conventions it has,
we dont know if its gonna use UTF-8, UTF-16, UTF-32 or path separator.

anything could happen. So when people architecture this, they pass those file names into the game code.
and that is a horrible horrible idea. This cuz this requires the game code layer to be aware of the complexity of filenames 

the game code shouldnt ever worry about it. 

thats why when Casey designed it, its is blind towards the filenames. 
and the game side gets a list of files, it indexes it using indicies.


42:15
Casey goes on to refactor the internal PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin);
function to work with UTF-16


47:25
after the refactoring, Casey mentioned that if you want to go down this route where the game code has no notion 
of file names. We are currently assume that the idea of an extension is the same on every platform, and maybe that is not true. 

so if we want to go one step further, we can define the set of file types that we know and care about.

                
                handmade_platform.h

                typedef enum platform_file_type
                {
                    PlatformFileType_AssetFile,
                    PlatformFileType_SavedGameFile,
                    
                    PlatformFileType_Count,
                } platform_file_type;
                    

with this change, the game code is further removed from filenames 

previously we had 

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);

                    ...
                    ...

                    // NOTE(casey): This code was written using Snuffleupagus-Oriented Programming (SOP)
                    {
                        platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin("hha");                  

                        ...
                        ...
                    }
                }




now we have 

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);

                    ...
                    ...

                    // NOTE(casey): This code was written using Snuffleupagus-Oriented Programming (SOP)
                    {
                        platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin(PlatformFileType_AssetFile);                    

                        ...
                        ...
                    }
                }

50:04
then in the win32 layer, we read the files depending on the enums passed in.

                win32_handmade.cpp

                internal PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
                {
                    platform_file_group Result = {};
                    
                    // TODO(casey): If we want, someday, make an actual arena used by Win32
                    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)VirtualAlloc(
                        0, sizeof(win32_platform_file_group),
                        MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
                    Result.Platform = Win32FileGroup;

                    wchar_t *WildCard = L"*.*";
                    switch(Type)
                    {
                        case PlatformFileType_AssetFile:
                        {
                            WildCard = L"*.hha";
                        } break;

                        case PlatformFileType_SavedGameFile:
                        {
                            WildCard = L"*.hhs";
                        } break;

                        InvalidDefaultCase;
                    }
                    
                    ....................................................
                    ........... open the file ..........................
                    ....................................................
                }


this way no file related strings are passed into the game code.


52:24
Casey mentioned that we will be doing some pre caching to remove some flickering.


1:12:47
someone in the Q/A asked about distant field font.

distance field dont really work for font. 
that is becuz the fonts will always be rounded. 

for example if your glyph looks like 

    ###############################################
    ###############################################
    ###############################################
    ###############################################
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############

with distance field font will have rounded edges.


     #############################################
    ###############################################
    ###############################################
     #############################################
                   ###############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                    #############
                     ###########


so distance field is awful for fonts.


1:17:12
someone asked the difference between mono space fonts vs font kerning in Q/A

assume you have two front 

you draw the first character A at this location

        A   B

then we have to figure out the starting location for our 2nd character.

the difference is that 
for mono spaced fonts we, would draw B at
    x + c   c is some constant 

for kerned fonts/.proportional fonts, we would draw B at 
    x + f(prev char, this char);

its just a simple look up table, that takes the two glyphs and tells it how far to move. 

it is literally replacing c with a table look up.