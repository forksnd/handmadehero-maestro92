Handmade Hero Day 151 - New Platform File API

Summary:
wrote the code to merge the tag arrays and assetTypes arrays from multiple files.

start to define all the platform file apis.

Refactored and cleaned up some of the asset streaming code.

Keyword:
Asset, Asset file, Asset loading, API design

9:48
Casey mentioned that we need to be able to rebase the tag indices.


so we have our global tags array in the game_assets 

                Assets->Tags = PushArray(Arena, Assets->TagCount, hha_tag);

and the assets indexes into the tags array.

                struct hha_asset
                {
                    u64 DataOffset;
                    u32 FirstTagIndex;
                    u32 OnePastLastTagIndex;
                    union
                    {
                        hha_bitmap Bitmap;
                        hha_sound Sound;
                    };
                };

recall the graph 


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



the FirstTagIndex and OnePastLastTagIndex are absolute indices 

so if we have two files, and we want to merge the two, the first asset array indicies will be correct 
but the 2nd asset array wont 

file1           
                asset *Assets;                  asset_tag *Tags;          
                                   
                hero0   -------------------->   Tag_FacingDirection: up  
                hero1   -----------             Tag_Height: 180
                hero2              \            Tag_Weight: 155 lbs
                hero3               \           Tag_Gender: male
                shadow0              ------>    Tag_FacingDirection: left
                shadow1                         Tag_Height: 180
                shadow2                         Tag_Weight: 155 lbs
                shadow3                         Tag_Gender: male
                shadow4                         ...
                tree0   -------------------->   Tag_Color: green                          
                tree1                           Tag_Type: oak
                tree2   -------------------->   Tag_Color: yellow             
                tree3                           Tag_Type: oak
                tree4
                tree5
                ...
                ...

file2           
                asset *Assets;                  asset_tag *Tags;          
                                   
                car0   -------------------->    Tag_FacingDirection: up  
                car1   -----------              Tag_Length: 180
                car2              \             Tag_Width: 155 lbs
                water0             \            Tag_Model: Toyota
                water1              ------>     Tag_FacingDirection: left
                water2                          ...
                                                ...
                                                Tag_WaterColor: blue
                                                Tag_Thickness: shallow




hero0 wll have 
    FirstTagIndex = 0
    OnePastLastTagIndex = 5


car0 will also have 
    FirstTagIndex = 0
    OnePastLastTagIndex = 5

but we need everything in file2 to have an offset of file1 Tags.size()


car0 will also have 
    FirstTagIndex = file.Tags.size() + 0
    OnePastLastTagIndex = file.Tags.size() + 5


something like this 


                asset *Assets;                  asset_tag *Tags;          
                                   
                hero0   -------------------->   Tag_FacingDirection: up  
                hero1   -----------             Tag_Height: 180
                hero2              \            Tag_Weight: 155 lbs
                hero3               \           Tag_Gender: male
                shadow0              ------>    Tag_FacingDirection: left
                shadow1                         Tag_Height: 180
                shadow2                         Tag_Weight: 155 lbs
                shadow3                         Tag_Gender: male
                shadow4                         ...
                tree0   -------------------->   Tag_Color: green                          
                tree1                           Tag_Type: oak
                tree2   -------------------->   Tag_Color: yellow             
                tree3                           Tag_Type: oak
                tree4                           ...
                tree5                           ...
                car0   -------------------->    Tag_FacingDirection: up  
                car1   -----------              Tag_Length: 180
                car2              \             Tag_Width: 155 lbs
                water0             \            Tag_Model: Toyota
                water1              ------>     Tag_FacingDirection: left
                water2                          ...
                                                ...
                                                Tag_WaterColor: blue
                                                Tag_Thickness: shallow



12:29
so here Casey is writing the code to load the tags 
you can see that as we are going through each asset_file, we are directly loadin the tag array
into our game_asset global tags array

                Platform.ReadDataFromFile(File->Handle, File->Header.Tags,
                                      TagArraySize, Assets->Tags + File->TagBase);

you can see, that we are directly loading into "Assets->Tags + File->TagBase"


-   code below
                // NOTE(casey): Allocate all metadata space
                Assets->Assets = PushArray(Arena, Assets->AssetCount, hha_asset);
                Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
                Assets->Tags = PushArray(Arena, Assets->TagCount, hha_tag);

                // NOTE(casey): Load tags
                for(u32 FileIndex = 0;
                    FileIndex < Assets->FileCount;
                    ++FileIndex)
                {
                    asset_file *File = Assets->Files + FileIndex;
                    if(PlatformNoFileErrors(File->Handle))
                    {
                        u32 TagArraySize = sizeof(hha_tag)*File->Header.TagCount;
                        Platform.ReadDataFromFile(File->Handle, File->Header.Tags,
                                                  TagArraySize, Assets->Tags + File->TagBase);
                    }
                }



of course we also have to assign the proper File->TagBase in the first for loop.

                platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin("hha");
                Assets->FileCount = FileGroup.FileCount;
                Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
                for(u32 FileIndex = 0;
                    FileIndex < Assets->FileCount;
                    ++FileIndex)
                {
                    asset_file *File = Assets->Files + FileIndex;

                    File->TagBase = Assets->TagCount;           ,----------------------------

                    ...
                    ...
                }


                        // NOTE(casey): Allocate all metadata space
                Assets->Assets = PushArray(Arena, Assets->AssetCount, hha_asset);
                Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
                Assets->Tags = PushArray(Arena, Assets->TagCount, hha_tag);

                // NOTE(casey): Load tags
                for(u32 FileIndex = 0;
                    FileIndex < Assets->FileCount;
                    ++FileIndex)
                {
                    asset_file *File = Assets->Files + FileIndex;
                    if(PlatformNoFileErrors(File->Handle))
                    {
                        u32 TagArraySize = sizeof(hha_tag)*File->Header.TagCount;
                        Platform.ReadDataFromFile(File->Handle, File->Header.Tags,
                                                  TagArraySize, Assets->Tags + File->TagBase);
                    }
                }


                u32 AssetCount = 0;
                for(u32 DestTypeID = 0; DestTypeID < Asset_Count; ++DestTypeID)
                {
                    ...
                    ...
                }




15:25
Casey finishiing up the for loop that merges our assetTypes array.

so thats something we have to fix. 





-   for the first for loop

                for(u32 DestTypeID = 0; DestTypeID < Asset_Count; ++DestTypeID)

    if you dont remember, Asset_Count is defined in the enums 

                enum asset_type_id
                {
                    Asset_None,
                    ...
                    ...
                    Asset_Shadow,
                    Asset_Tree,
                    Asset_Sword,
                    Asset_Rock,

                    ...
                    ...
                    Asset_Count,        <------------
                };

    so we are literally looping through all the asset_types 



-   again, just like how we are merging the tags array, we merge our Assets array 


                u32 AssetCountForType = (SourceType->OnePastLastAssetIndex -
                                         SourceType->FirstAssetIndex);
                Platform.ReadDataFromFile(File->Handle,
                                         File->Header.Assets +
                                         SourceType->FirstAssetIndex*sizeof(hha_asset),
                                         AssetCountForType*sizeof(hha_asset),
                                         Assets->Assets + AssetCount);


-   just like what we mention previously, we have to edit the TagIndex    

                for(u32 AssetIndex = AssetCount;
                    AssetIndex < (AssetCount + AssetCountForType);
                    ++AssetIndex)
                {
                    hha_asset *Asset = Assets->Assets + AssetIndex;
                    Asset->FirstTagIndex += File->TagBase;
                    Asset->OnePastLastTagIndex += File->TagBase;
                }


    the assets are indexing into our global Tag array, 
    so we have to edit "FirstTagIndex" and "OnePastLastTagIndex"

                Asset->FirstTagIndex += File->TagBase;
                Asset->OnePastLastTagIndex += File->TagBase;



-   full code below


                platform_file_group FileGroup = PlatformGetAllFilesOfTypeBegin("hha");
                ..............................................
                ......... section above ......................
                ..............................................
                PlatformGetAllFilesOfTypeEnd(FileGroup);

                Assets->Assets = PushArray(Arena, Assets->AssetCount, hha_asset);
                Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
                Assets->Tags = PushArray(Arena, Assets->TagCount, hha_tag);
                
                // TODO(casey): Excersize for the reader - how would you do this in a way
                // that scaled gracefully to hundreds of asset pack files?  (or more!)
                u32 AssetCount = 0;
                for(u32 DestTypeID = 0; DestTypeID < Asset_Count; ++DestTypeID)
                {
                    asset_type *DestType = Assets->AssetTypes + DestTypeID;
                    DestType->FirstAssetIndex = AssetCount;
                    
                    for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
                    {
                        asset_file *File = Assets->Files + FileIndex;
                        if(PlatformNoFileErrors(File->Handle))
                        {
                            for(u32 SourceIndex = 0; SourceIndex < File->Header.AssetTypeCount; ++SourceIndex)
                            {
                                hha_asset_type *SourceType = File->AssetTypeArray + SourceIndex;
                                
                                if(SourceType->TypeID == DestTypeID)
                                {
                                    u32 AssetCountForType = (SourceType->OnePastLastAssetIndex -
                                                             SourceType->FirstAssetIndex);
                                    Platform.ReadDataFromFile(File->Handle,
                                                             File->Header.Assets +
                                                             SourceType->FirstAssetIndex*sizeof(hha_asset),
                                                             AssetCountForType*sizeof(hha_asset),
                                                             Assets->Assets + AssetCount);
                                    for(u32 AssetIndex = AssetCount;
                                        AssetIndex < (AssetCount + AssetCountForType);
                                        ++AssetIndex)
                                    {
                                        hha_asset *Asset = Assets->Assets + AssetIndex;
                                        Asset->FirstTagIndex += File->TagBase;
                                        Asset->OnePastLastTagIndex += File->TagBase;
                                    }
                                    AssetCount += AssetCountForType;
                                    Assert(AssetCount < Assets->AssetCount);
                                }
                            }
                        }
                    }

                    DestType->OnePastLastAssetIndex = AssetCount;
                }




20:23 
Casey will tacket actually writing the Platform API 
such as the 

                Platform.GetAllFilesOfTypeBegin("hha");

                Platform.ReadDataFromFile();

                PlatformNoFileErrors(File->Handle);

that we used previously.


22:18
Casey first defined the following in the handmade_platform.h file 

                handmade_platform.h

                typedef struct platform_file_handle
                {
                    b32 HasErrors;
                } platform_file_handle;
                    
                typedef struct platform_file_group
                {
                    u32 FileCount;
                    void *Data;
                } platform_file_group;

                #define PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(name) platform_file_group name(char *Type)
                typedef PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(platform_get_all_files_of_type_begin);

                #define PLATFORM_GET_ALL_FILE_OF_TYPE_END(name) void name(platform_file_group FileGroup)
                typedef PLATFORM_GET_ALL_FILE_OF_TYPE_END(platform_get_all_files_of_type_end);

                #define PLATFORM_OPEN_FILE(name) platform_file_handle *name(platform_file_group FileGroup, u32 FileIndex)
                typedef PLATFORM_OPEN_FILE(platform_open_file);

                #define PLATFORM_READ_DATA_FROM_FILE(name) void name(platform_file_handle *Source, u64 Offset, u64 Size, void *Dest)
                typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);

                #define PLATFORM_FILE_ERROR(name) void name(platform_file_handle *Handle, char *Message)
                typedef PLATFORM_FILE_ERROR(platform_file_error);

                #define PlatformNoFileErrors(Handle) (!(Handle)->HasErrors)



32:24
Casey created a new platform_api struct that to hold all the api calls.

                handmade_platform.h

                typedef struct platform_api
                {
                    platform_add_entry *AddEntry;
                    platform_complete_all_work *CompleteAllWork;

                    platform_get_all_files_of_type_begin *GetAllFilesOfTypeBegin;
                    platform_get_all_files_of_type_end *GetAllFilesOfTypeEnd;
                    platform_open_file *OpenFile;
                    platform_read_data_from_file *ReadDataFromFile;
                    platform_file_error *FileError;
                    
                    debug_platform_free_file_memory *DEBUGFreeFileMemory;
                    debug_platform_read_entire_file *DEBUGReadEntireFile;
                    debug_platform_write_entire_file *DEBUGWriteEntireFile;
                } platform_api;


and he created a global variable in the Handmade.h 


                handmade.h 

                ...
                ...

                global_variable platform_api Platform;

                ...
                ...



                handmade_asset.h

                struct asset_file
                {
                    platform_file_handle *Handle;

                    // TODO(casey): If we ever do thread stacks, AssetTypeArray
                    // doesn't actually need to be kept here probably.
                    hha_header Header;
                    hha_asset_type *AssetTypeArray;

                    u32 TagBase;
                };



37:05
the in win32_handmade.cpp, Casey wired up all the functions 

                win32_handmade.cpp

                int CALLBACK
                WinMain(HINSTANCE Instance,
                        HINSTANCE PrevInstance,
                        LPSTR CommandLine,
                        int ShowCode)
                {
                    ...
                    ...

                    game_memory GameMemory = {};
                    GameMemory.PermanentStorageSize = Megabytes(256);
                    GameMemory.TransientStorageSize = Gigabytes(1);
                    GameMemory.HighPriorityQueue = &HighPriorityQueue;
                    GameMemory.LowPriorityQueue = &LowPriorityQueue;
                    GameMemory.PlatformAPI.AddEntry = Win32AddEntry;
                    GameMemory.PlatformAPI.CompleteAllWork = Win32CompleteAllWork;
                    
                    GameMemory.PlatformAPI.GetAllFilesOfTypeBegin = Win32GetAllFilesOfTypeBegin;
                    GameMemory.PlatformAPI.GetAllFilesOfTypeEnd = Win32GetAllFilesOfTypeEnd;
                    GameMemory.PlatformAPI.OpenFile = Win32OpenFile;
                    GameMemory.PlatformAPI.ReadDataFromFile = Win32ReadDataFromFile;
                    GameMemory.PlatformAPI.FileError = Win32FileError;

                    GameMemory.PlatformAPI.DEBUGFreeFileMemory = DEBUGPlatformFreeFileMemory;
                    GameMemory.PlatformAPI.DEBUGReadEntireFile = DEBUGPlatformReadEntireFile;
                    GameMemory.PlatformAPI.DEBUGWriteEntireFile = DEBUGPlatformWriteEntireFile;

                    ...
                    ...
                }


39:18
of course Casey went on to declare all these functions. 


                win32_handmade.cpp

                internal PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
                {
                    platform_file_group FileGroup = {};

                    return(FileGroup);
                }

                internal PLATFORM_GET_ALL_FILE_OF_TYPE_END(Win32GetAllFilesOfTypeEnd)
                {
                }
                    
                internal PLATFORM_OPEN_FILE(Win32OpenFile)
                {
                    return(0);
                }

                internal PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile)
                {
                }

                internal PLATFORM_FILE_ERROR(Win32FileError)
                {
                }



41:47
recall in day 150, we did the following as a hack.

                struct game_assets
                {
                    ...
                    ...

                    u8 *HHAContents;
                };


we store the entire contents in the HHAContents becuz we are gonna pull stuff out of it in the future
after the initialization stage. 

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    ...
                    ...

                    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile("test.hha");
                    if(ReadResult.ContentsSize != 0)
                    {
                        hha_header *Header = (hha_header *)ReadResult.Contents;
                        
                        ...
                        ...

                        Assets->HHAContents = (u8 *)ReadResult.Contents;
                    }

                    ...
                    ...
                }

and when we are streaming in, we just have the Bitmap memory point to the Work->Assets->HHAContents + HHAAsset->DataOffset;

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork)
                {
                    load_bitmap_work *Work = (load_bitmap_work *)Data;

                    hha_asset *HHAAsset = &Work->Assets->Assets[Work->ID.Value];
                    hha_bitmap *Info = &HHAAsset->Bitmap;
                    loaded_bitmap *Bitmap = Work->Bitmap;

                    Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
                    Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
                    Bitmap->Width = Info->Dim[0];
                    Bitmap->Height = Info->Dim[1];
                    Bitmap->Pitch = 4*Info->Dim[0];
                    Bitmap->Memory = Work->Assets->HHAContents + HHAAsset->DataOffset;
                    
                    CompletePreviousWritesBeforeFutureWrites;
                    
                    Work->Assets->Slots[Work->ID.Value].Bitmap = Work->Bitmap;
                    Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;
                    
                    EndTaskWithMemory(Work->Task);
                }

this aint good, cuz that assumes we loaded in all of our assets entirely at startup           
the whole point of our streaming system is we load in stuff that we need


so when we load a Bitmap, it will need Bitmap memory.      

                Bitmap->Memory = Work->Assets->HHAContents + HHAAsset->DataOffset;

so we will want it to be some Bitmap memory thats reserved for us.
so when we were in LoadBitmap(); the function that starts a job:

what we want to do is to push on space for the eventual bitmap. 

previously, we had 

                internal void
                LoadBitmap(game_assets *Assets, bitmap_id ID)
                {
                    if(ID.Value &&
                       (AtomicCompareExchangeUInt32((uint32 *)&Assets->Slots[ID.Value].State, AssetState_Queued, AssetState_Unloaded) ==
                        AssetState_Unloaded))
                    {    
                        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
                        if(Task)        
                        {
                            load_bitmap_work *Work = PushStruct(&Task->Arena, load_bitmap_work);

                            Work->Assets = Assets;
                            Work->ID = ID;
                            Work->Task = Task;
                            Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
                            Work->FinalState = AssetState_Loaded;

                            PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadBitmapWork, Work);
                        }
                        else
                        {
                            Assets->Slots[ID.Value].State = AssetState_Unloaded;
                        }
                    }    
                }


now we want to reserve memory for the Bitmap in the LoadBitmap(); function






42:22
another thing about 
                
                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork);

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork);

-   noticed in these two functions, we were doing lots of bitmap and sound specific work:
    we initially put these things here cuz we didnt know what the width or height of a bitmap was 
    we had to load the bitmap file, read its header, to know it. 

    now that we have an assest system, we will know all of these meta data after intialization.
    it is only the bitmap memory or audio memory that we dont have.

    so we can actually pull out these bitmap / audio specfic work out, 
    which means the LoadBitmapWork and LoadSoundWork can be collapsed into one, cuz we dont have to do 
    any asset specific work. We are just loading memory.

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork)
                {
                    ...
                    ...

                    Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
                    Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
                    Bitmap->Width = Info->Dim[0];
                    Bitmap->Height = Info->Dim[1];
                    Bitmap->Pitch = 4*Info->Dim[0];
                    Bitmap->Memory = Work->Assets->HHAContents + HHAAsset->DataOffset;
                    
                    ...
                    ...

                    EndTaskWithMemory(Work->Task);
                }


                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork)
                {
                    load_sound_work *Work = (load_sound_work *)Data;

                    hha_asset *HHAAsset = &Work->Assets->Assets[Work->ID.Value];
                    hha_sound *Info = &HHAAsset->Sound;
                    loaded_sound *Sound = Work->Sound;

                    Sound->SampleCount = Info->SampleCount;
                    Sound->ChannelCount = Info->ChannelCount;
                    Assert(Sound->ChannelCount < ArrayCount(Sound->Samples));
                    u64 SampleDataOffset = HHAAsset->DataOffset;
                    for(u32 ChannelIndex = 0;
                        ChannelIndex < Sound->ChannelCount;
                        ++ChannelIndex)
                    {
                        Sound->Samples[ChannelIndex] = (int16 *)(Work->Assets->HHAContents + SampleDataOffset);
                        SampleDataOffset += Sound->SampleCount*sizeof(int16);
                    }

                    CompletePreviousWritesBeforeFutureWrites;
                    
                    Work->Assets->Slots[Work->ID.Value].Sound = Work->Sound;
                    Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;
                    
                    EndTaskWithMemory(Work->Task);
                }








45:48
so continuing down this appoach, Casey defined a struct load_asset_work

the key things it has are 
-   an offset into the asset file  
-   the size of memory to load 
-   the destination to assing to 

                struct load_asset_work
                {
                    task_with_memory *Task;
                    asset_slot *Slot;

                    platform_file_handle *Handle;
                    u64 Offset;
                    u64 Size;
                    void *Destination;

                    asset_state FinalState;
                };



and routine to of loading an asset_work looks like:

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
                {
                    load_asset_work *Work = (load_asset_work *)Data;

                    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
                    
                    CompletePreviousWritesBeforeFutureWrites;

                    if(PlatformNoFileErrors(Work->Handle))
                    {
                        Work->Slot->State = Work->FinalState;
                    }
                    
                    EndTaskWithMemory(Work->Task);
                }





51:14
and Casey proceeds to change the LoadBitmap code to use the load_asset_work instead of LoadBitmapWork
noticed we also moved the bitmap specific code in here 

                
                Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
                Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
                Bitmap->Width = Info->Dim[0];
                Bitmap->Height = Info->Dim[1];
                Bitmap->Pitch = 4*Info->Dim[0];
                u32 MemorySize = Bitmap->Pitch*Bitmap->Height;
                Bitmap->Memory = PushSize(&Assets->Arena, MemorySize);

-   full code below 

                internal void
                LoadBitmap(game_assets *Assets, bitmap_id ID)
                {
                    if(ID.Value &&
                       (AtomicCompareExchangeUInt32((uint32 *)&Assets->Slots[ID.Value].State, AssetState_Queued, AssetState_Unloaded) ==
                        AssetState_Unloaded))
                    {    
                        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
                        if(Task)        
                        {
                            hha_asset *HHAAsset = Assets->Assets + ID.Value;
                            hha_bitmap *Info = &HHAAsset->Bitmap;
                            loaded_bitmap *Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
                            
                            Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
                            Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
                            Bitmap->Width = Info->Dim[0];
                            Bitmap->Height = Info->Dim[1];
                            Bitmap->Pitch = 4*Info->Dim[0];
                            u32 MemorySize = Bitmap->Pitch*Bitmap->Height;
                            Bitmap->Memory = PushSize(&Assets->Arena, MemorySize);

                            load_asset_work *Work = PushStruct(&Task->Arena, load_asset_work);
                            Work->Task = Task;
                            Work->Slot = Assets->Slots + ID.Value;
                            Work->Handle = 0;
                            Work->Offset = HHAAsset->DataOffset;
                            Work->Size = MemorySize;
                            Work->Destination = Bitmap->Memory;
                            Work->FinalState = AssetState_Loaded;
                            Work->Slot->Bitmap = Bitmap;

                            Bitmap->Memory = Assets->HHAContents + HHAAsset->DataOffset;

                            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
                        }
                        else
                        {
                            Assets->Slots[ID.Value].State = AssetState_Unloaded;
                        }
                    }    
                }
