Handmade Hero Day 150 - Loading Assets from the Asset File

Summary:
more work on the game side loading code to load the .hha asset file in AllocateGameAssets();

finish the version that our game side code loads in the entire .hha file. Game Runs fine on it.

previously, our streaming in asset code was reading in bitmaps from bitmap files, reading audio from wav files 
Casey replaced it with reading in from our game_assets struct.

mentions the possibility that we may have multiple .hha files, so started the loading code 
that loads multiple .hha file and merges all of these .hha file into one single giant game side game_assets struct

mentioned why use pointers vs local variables in the q/A

mentioned why Casey dislikes the C runtime library

Keyword:
Asset, Asset file, Asset loading, API design


2:08
Casey started by finishing up reading the asset array and asset type array. 
Recall in day 149, we only wrote the for loop to read the tags array 

previously we had 
                internal game_assets* AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    ...
                    ...

                    .........................................................
                    ........... initializing TagRange array .................
                    .........................................................

                    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile("test.hha");    
                    if(ReadResult.ContentsSize != 0)
                    {
                        hha_header *Header = (hha_header *)ReadResult.Contents;
                        .........................................................
                        .................. Reading Header Data ..................
                        .........................................................


                        // TODO(casey): Decide what will be flat-loaded and what won't be!

                        hha_tag *HHATags = (hha_tag *)((u8 *)ReadResult.Contents + Header->Tags);
                        
                        for(u32 TagIndex = 0; TagIndex < Assets->TagCount; ++TagIndex)
                        {
                            hha_tag *Source = HHATags + TagIndex;
                            asset_tag *Dest = Assets->Tags + TagIndex;

                            Dest->ID = Source->ID;
                            Dest->Value = Source->Value;
                        }
                        ...
                        ...
                #if 0
                        for()
                        {
                        }

                        for()
                        {
                        }
                #endif



2:24 24:45
Casey initially wanted to write two for loops, but he did make a change in the game side struct game_assets.
he made it so that the tags array and asset array is directly stored with "hha_tag *Tags;" and "hha_asset *Assets;"


                struct game_assets
                {
                    ...
                    ...

                    uint32 TagCount;
                    hha_tag *Tags;

                    uint32 AssetCount;
                    hha_asset *Assets;
                    asset_slot *Slots;
                    
                    asset_type AssetTypes[Asset_Count];
                    ...
                    ...
                };


and with that he got rid of the gameside struct asset_tag definition


-   with this change, we can just directly store the "hha_tag *Tags;" array and the "hha_asset* HHAAssets" array 
    without doing our for loops. We were doing the for loop to convert an hha_tag to a game side tag 
    or a hha_asset to a game side asset.
    (explained in detail in day149);

    kind of like 
                for (each hha_tag in tags)
                {
                    game_side_game_asset.tagsArray[i] = convertToGameSide(hha_tag);
                }


                for (each hha_asset in HHAAssets)
                {
                    game_side_game_asset.AssetsArray[i] = convertToGameSide(hha_asset);
                }


    now we dont have to do that

                Assets->AssetCount = Header->AssetCount;
                Assets->Assets = (hha_asset *)((u8 *)ReadResult.Contents + Header->Assets);
                ...


                Assets->TagCount = Header->TagCount;
                Assets->Tags = (hha_tag *)((u8 *)ReadResult.Contents + Header->Tags);

    literally, Casey is dong "Flat Loading". (a term he mentioned in day 149 q&a)


-   we still ahve to conver the assetTypes array to game side 

                hha_asset_type *HHAAssetTypes = (hha_asset_type *)((u8 *)ReadResult.Contents + Header->AssetTypes);

                for(u32 Index = 0; Index < Header->AssetTypeCount; ++Index)
                {
                    ................................................................
                    ....... converting an AssetType to GameSide Asset_Type .........
                    ................................................................
                }


-   full code below 

    
                hha_header *Header = (hha_header *)ReadResult.Contents;
                
                Assets->AssetCount = Header->AssetCount;
                Assets->Assets = (hha_asset *)((u8 *)ReadResult.Contents + Header->Assets);
                Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);

                Assets->TagCount = Header->TagCount;
                Assets->Tags = (hha_tag *)((u8 *)ReadResult.Contents + Header->Tags);

                hha_asset_type *HHAAssetTypes = (hha_asset_type *)((u8 *)ReadResult.Contents + Header->AssetTypes);

                for(u32 Index = 0; Index < Header->AssetTypeCount; ++Index)
                {
                    hha_asset_type *Source = HHAAssetTypes + Index;

                    if(Source->TypeID < Asset_Count)
                    {
                        asset_type *Dest = Assets->AssetTypes + Source->TypeID;
                        // TODO(casey): Support merging!
                        Assert(Dest->FirstAssetIndex == 0);
                        Assert(Dest->OnePastLastAssetIndex == 0);
                        Dest->FirstAssetIndex = Source->FirstAssetIndex;
                        Dest->OnePastLastAssetIndex = Source->OnePastLastAssetIndex;
                    }
                }

                Assets->HHAContents = (u8 *)ReadResult.Contents;




11:52
temporarily, Casey added a field in game_assets

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



12:21
Casey now tackling the AssetTypes array.



                for(u32 Index = 0; Index < Header->AssetTypeCount; ++Index)
                {
                    hha_asset_type *Source = HHAAssetTypes + Index;

                    if(Source->TypeID < Asset_Count)
                    {
                        asset_type *Dest = Assets->AssetTypes + Source->TypeID;
                        // TODO(casey): Support merging!
                        Assert(Dest->FirstAssetIndex == 0);
                        Assert(Dest->OnePastLastAssetIndex == 0);
                        Dest->FirstAssetIndex = Source->FirstAssetIndex;
                        Dest->OnePastLastAssetIndex = Source->OnePastLastAssetIndex;
                    }
                }


15:06
so previously we had 

                handmade_asset.cpp

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork)
                {
                    load_bitmap_work *Work = (load_bitmap_work *)Data;

                    asset_bitmap_info *Info = &Work->Assets->Assets[Work->ID.Value].Bitmap;
                    *Work->Bitmap = DEBUGLoadBMP(Info->FileName, Info->AlignPercentage);

                    CompletePreviousWritesBeforeFutureWrites;
                    
                    Work->Assets->Slots[Work->ID.Value].Bitmap = Work->Bitmap;
                    Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;
                    
                    EndTaskWithMemory(Work->Task);
                }

the line 
                *Work->Bitmap = DEBUGLoadBMP(Info->FileName, Info->AlignPercentage);

is where we used load the bitmaps from the bitmap file.
now that we have our asset system, we want to replace that with getting the bitmaps from our asset system.

notice that when we load the bitmap memory, we are doing
                
                Bitmap->Memory = Work->Assets->HHAContents + HHAAsset->DataOffset;

note that this is a hack, cuz this assumes we loaded in our assets entirely.
the whole point of the asset system is only load in things that we need.

-   full code below:                

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



19:11
Casey does the same for loading sound 

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork)
                {
                    load_sound_work *Work = (load_sound_work *)Data;

                    asset_sound_info *Info = &Work->Assets->Assets[Work->ID.Value].Sound;
                    *Work->Sound = DEBUGLoadWAV(Info->FileName, Info->FirstSampleIndex, Info->SampleCount);

                    CompletePreviousWritesBeforeFutureWrites;
                    
                    Work->Assets->Slots[Work->ID.Value].Sound = Work->Sound;
                    Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;
                    
                    EndTaskWithMemory(Work->Task);
                }



23:27
Casey mentioned that currently we are loading the entire asset file at startup.
we want to stream our assets. So what we want to do is only load parts of the asset file that we need at any time 
instead of loading in the whole thing.  

this involve some work with our platform player cuz currently the only thing we put in in the platform layer was
just the ability to load an entire file at once.

meaning we want to change this line 


                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {


                    ...
                    ...

                    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile("test.hha");

                    ...
                    ...
                }


30:16
so the goal here is that 
imagine in our final version, we have multiple asset file. Why do we have multiple asset file you asked?
1.  we have a 32 bit memory limit, so we had to break up our giant 16GB asset file into multiple chunks
2.  maybe we had to patch something. we have a small tiny patch file that patches the main asset file 
3.  we shipped a DLC 

so we might have multiple asset file and we want to load and merge them.

we will have a list of files to load from the operating system 

So Casey proceeds to demonstrate the API design for this system
Casey starts off by writing the usage code. The same thing that vinh taught me 


33:37
Casey first introduces the concept of multiple asset files in the game_assets struct
-   first he defines asset_file struct 

                struct asset_file
                {
                //    platform_file_handle Handle;

                    // TODO(casey): If we ever do thread stacks, AssetTypeArray
                    // doesn't actually need to be kept here probably.
                    hha_header Header;
                    hha_asset_type *AssetTypeArray;

                    u32 TagBase;
                };

-   he also adds an array of asset_file in struct game_assets

                struct game_assets
                {
                    // TODO(casey): Not thrilled about this back-pointer
                    ...
                    ...

                    u32 FileCount;
                    asset_file *Files;

                    ...
                    ...
                };



Casey starting to writes the main structure 
-   you can see that we created an array of asset_file before we read all the files

                Assets->FileCount = FileGroup.FileCount;
                Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);

-   then for every file we iterate, we store its information in our our asset_file_s struct 
    first we store the handle
                
                asset_file *File = Assets->Files + FileIndex;

                File->Handle = PlatformOpenFile(FileGroup, FileIndex);


full code below:

                platform_file_group FileGroup = PlatformGetAllFilesOfTypeBegin("hha");
                Assets->FileCount = FileGroup.FileCount;
                Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
                for(u32 FileIndex = 0;
                    FileIndex < Assets->FileCount;
                    ++FileIndex)
                {
                    asset_file *File = Assets->Files + FileIndex;

                    File->Handle = PlatformOpenFile(FileGroup, FileIndex);

                    .........................................................
                    .................. Process each file ....................
                    .........................................................
                }







37:30
In terms of error checking for the file, the first draft Casey wrote was like below:

-   we may get the handle, we may not get the handle 
    so Casey adds the first if check, 

-   going down this approach, we need to then check whether "PlatformReadDataFromFile" succeeded or not 
    for example the header wasnt there or the file was corrupted or what not
    so we have to add the 2nd if check.

                asset_file *File = Assets->Files + FileIndex;

                File->Handle = PlatformOpenFile(FileGroup, FileIndex);
                if(PlatformFileHandleIsValid(File->Handle))
                {
                    if(PlatformReadDataFromFile(File->Handle, 0, sizeof(Header), &File->Header))
                    {

                    }

                }


which resulted in Casey saying, what he would rather do is not do that, and say we just do the following:
just do all the operation on the file. And at the end, we do a final check.

this way, it doesnt matter if the error came from "PlatformOpenFile" or "PlatformReadDataFromFile".
all we care is that we didnt get back a header.

Casey did added in the comments 
                "// TODO(casey): Eventually, have some way of notifying users of bogus files?"


-   after getting the handle, 
    we also read in the Header and store it in our asset_file 

                PlatformReadDataFromFile(File->Handle, 0, sizeof(Header), &File->Header);

    so now, we have stored our Handle and the header in our asset_file struct.

-   full code below

                File->Handle = PlatformOpenFile(FileGroup, FileIndex);
                PlatformReadDataFromFile(File->Handle, 0, sizeof(Header), &File->Header);

                ...
                ...

                if(Header->MagicValue != HHA_MAGIC_VALUE)
                {
                    PlatformFileError(File->Handle, "HHA file has an invalid magic value.");
                }
            
                if(Header->Version > HHA_VERSION)
                {
                    PlatformFileError(File->Handle, "HHA file is of a later version.");
                }
            
                if(PlatformNoFileErrors(File->Handle))
                {
                    ...
                }
                else
                {
                    // TODO(casey): Eventually, have some way of notifying users of bogus files?
                    InvalidCodePath;
                }


41:03
moving on, if we gotten a valid header, we would want to sum up, the total counts of all the arrays 
for example, we want to merge the total count of Assets array, Tags array and AssetTypes array


                if(PlatformNoFileErrors(File->Handle))
                {
                    Assets->TagCount += Header->TagCount;
                    Assets->AssetCount += Header->AssetCount;
                }
                else
                {
                    // TODO(casey): Eventually, have some way of notifying users of bogus files?
                    InvalidCodePath;
                }





47:44
Casey enclosed his for loop in "PlatformGetAllFilesOfTypeBegin("hha")"; and "PlatformGetAllFilesOfTypeEnd(FileGroup);"
he claims that the OS may require resources to read the files. So once we opened files, we want to return the resources
back to the OS. 

                platform_file_group FileGroup = PlatformGetAllFilesOfTypeBegin("hha");
                Assets->FileCount = FileGroup.FileCount;
                Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
                for(u32 FileIndex = 0;
                    FileIndex < Assets->FileCount;
                    ++FileIndex)
                {
                    asset_file *File = Assets->Files + FileIndex;

                    File->Handle = PlatformOpenFile(FileGroup, FileIndex);

                    .........................................................
                    .................. Process each file ....................
                    .........................................................
                }
                PlatformGetAllFilesOfTypeEnd(FileGroup);





46:43
Next up, Casey writes the code that merges our game_assets

-   just like the handle and header, we store the AssetTypeArray as we through files 

                File->Handle = PlatformOpenFile(FileGroup, FileIndex);
                PlatformReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);
                File->AssetTypeArray = (hha_asset_type *)PushSize(Arena, AssetTypeArraySize);       <-------------- allocating 
                PlatformReadDataFromFile(File->Handle, File->Header.AssetTypes,                     <-------------- reading in
                                         AssetTypeArraySize, File->AssetTypeArray);

-   full code below:

                {
                    platform_file_group FileGroup = PlatformGetAllFilesOfTypeBegin("hha");
                    Assets->FileCount = FileGroup.FileCount;
                    Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
                    for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
                    {
                        asset_file *File = Assets->Files + FileIndex;

                        u32 AssetTypeArraySize = File->Header.AssetTypeCount*sizeof(hha_asset_type);

                        ZeroStruct(File->Header);
                        File->Handle = PlatformOpenFile(FileGroup, FileIndex);
                        PlatformReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);
                        File->AssetTypeArray = (hha_asset_type *)PushSize(Arena, AssetTypeArraySize);
                        PlatformReadDataFromFile(File->Handle, File->Header.AssetTypes,
                                                 AssetTypeArraySize, File->AssetTypeArray);

                        ..................................................
                        ............ Checking for MagicValue .............
                        ..................................................


                        if(PlatformNoFileErrors(File->Handle))
                        {
                            Assets->TagCount += Header->TagCount;
                            Assets->AssetCount += Header->AssetCount;
                        }
                        else
                        {
                            // TODO(casey): Eventually, have some way of notifying users of bogus files?
                            InvalidCodePath;
                        }
                    }
                    PlatformGetAllFilesOfTypeEnd(FileGroup);
                }





-   after our first for loop, we start to allocate memory for our merged game_assets
    the Assets->AssetCount, Assets->AssetCount and Assets->TagCount are all total counts.

                Assets->Assets = PushArray(Arena, Assets->AssetCount, hha_asset);
                Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
                Assets->Tags = PushArray(Arena, Assets->TagCount, hha_tag);
                
    
-   about merging the AssetTypes array, since we dont care about performance, and we care about organizing by AssetType  
    we will just loop through all the AssetType Ids

    for every AssetType Id, I go through all the asset_files. I look to see which files has 
    asssts of that type. Notice we are looping through our own asset_file array. We have populate the assetTypes array 
    in the first for loop.

    Casey didnt finish it. This will continue in the next episode.
        
                platform_file_group FileGroup = PlatformGetAllFilesOfTypeBegin("hha");
                ..............................................
                ......... section above ......................
                ..............................................
                PlatformGetAllFilesOfTypeEnd(FileGroup);

                Assets->Assets = PushArray(Arena, Assets->AssetCount, hha_asset);
                Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
                Assets->Tags = PushArray(Arena, Assets->TagCount, hha_tag);
                

                u32 AssetCount = 0;
                u32 TagCount = 0;
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
                                
                                if(SourceType->TypeID == AssetTypeID)
                                {
                                    PlatformReadDataFromFile();
                                    AssetCount += ;
                                }
                            }
                        }
                    }

                    DestType->OnePastLastAssetIndex = AssetCount;
                }










1:01:33
someone in the Q/A asked, when you declare a variable, when do you want to declare a pointer?

1.  when copying it is too expensive, you want to make it a pointer

Casey proceeds to show us the assembly code of calling a function by pointer vs calling a function by value 
calling a function by value, which copies your parameters has to do the work of copying your parameters 
on to the stack. 

2.  2nd reason is that if you want to modify the thing you passed in.


1:12:08
Casey explaining why he doesnt use the C runtime library
1.  He doesnt trust it cuz hes has had lots of bad experience with it.
    for example, you can call a math library function and it does a ton of things that it doesnt need to do. 

2.  he doesnt like their API.

3.  unreliability of linking

