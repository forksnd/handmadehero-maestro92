Handmade Hero Day 148 - Writing the Asset File Header

Summary:
wrote the code that outputs the header for the asset file in test_asset_builder.cpp

created test_asset_builder.h. moved all the declaration there.

wrote the struct for the header
introduced the concept of MagicValue and Version number in the header 

discussed the problems with the streaming write paradigm

discussed the pros and cons of asserts in game dev vs writing unit tests

claims never use C++ exceptions. Just use asserts 

Keyword:
Asset, Asset file, Asset file header, Asset system, asset memory management


2:27
spent the first 10 minutes fixing a but on the detecting an audio sample has finished playing

12:43
Assert is something I really do recommend. Assert catches bugs!

16:20
what we want to transition to is a single big file, 

24:09
added an test_asset_builder.h file for convenience

copied a lot of the structs definition to the .h file 

                struct bitmap_id
                {
                    ...
                };

                struct sound_id
                {
                    ...
                };

                struct asset_bitmap_info
                {
                    ...
                };

                struct asset_sound_info
                {
                    ...
                    ...
                };

                struct asset
                {
                    ...
                    ...
                };

                #define VERY_LARGE_NUMBER 4096

                struct game_assets
                {
                    ...
                    ...
                };



26:12
what we want to do now is to go through all these assets and write out a giant file which has the directory information we need,
which is all the meta data information, such as asset_bitmap_info and asset_sound_info

then we want to write all the physical data, the actual bitmaps and audio sample data. all into one file.



27:03
first thing Casey did is defined a handmade_file_formats.h. 
this is a thing that will define the file format that we will use. 

27:52
recall in C, when you define a structure, introduces padding to make sure things are aligned. And the padding can be different 
depending on the circunstances.

we dont want that padding obviously. so we have to resort to #pragma pack again 


                handmade_file_formats.h

                #pragma pack(push, 1)

                ...
                ...

                #pragma pop(push, 1)


we want to enclose our definition for our file format structures inside a #pragma push and pop, 
so that we know the exact format that we specified in these structs is actually what will be written to disc



28:45
we said that our asset file will have a .hha extension. so Casey first defined an hha_header struct 



                #define HHA_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))

                struct hha_header
                {
                #define HHA_MAGIC_VALUE HHA_CODE('h','h','a','f')
                    u32 MagicValue;

                #define HHA_VERSION 0
                    u32 Version;
                    ...
                    ...
                };


-   this is gonna be something that starts the file, so that if you were to open one of these files, you could test to see
    whether it is a handmade hero file or not

    This is a very common thing in file formats, and we have seen it already in the file formats we have talked about so far. 

    for example in day 137 and day 138, when you open a .wav file, 
    you are always suppose to see WAVE_ChunkID_RIFF, one of the values that we predefine 

                enum
                {
                    WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
                    WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
                    WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
                    WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
                };

                struct WAVE_header
                {
                    uint32 RIFFID;
                    uint32 Size;
                    uint32 WAVEID;
                };

                internal loaded_sound
                DEBUGLoadWAV(char *FileName)
                {
                    loaded_sound Result = {};
                    
                    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);    
                    if(ReadResult.ContentsSize != 0)
                    {
                        WAVE_header *Header = (WAVE_header *)ReadResult.Contents;
                        Assert(Header->RIFFID == WAVE_ChunkID_RIFF);
                        Assert(Header->WAVEID == WAVE_ChunkID_WAVE);
                    }

                    return(Result);
                }

    its a way of error checking


    like the example of a RIFFID being the frist thing in a .wave file, these are called Magic values.
    They have no real reason of being there, other than to indicate that something is such and such file

    so casey put in a magic value 


    Notice Casey just use the method from the .wave file format definitions. having ASCII letters. 
    hhaf = handmade hero asset file


-   some people like to put a version number in there. so we put one in. 




33:56
after the magic value and the version number, we need someway to define where our data is. 

-   what usually happens when load in an asset file is that, we first get the MagicValue and Version.
    These two are just there to verify that the thing we are about to load, we have the ability load it. 

    for example, sometimes the magicvalue may be the correct 'hhaf' value, 
    but the Version is outdated. So we need both to be correct for us to load it. 

-   the next thing in the file is about where in the file will we look for data
    
    recall that we have assets and tags. so the next information we want to put in our header file is 
    where are the assets and where are the tags. and how many of them are there.


                 _______________________
                |   Header              |
                |       MagicValue      |
                |       Version         |
                |                       |
                |       Info on types   |
                |       and tag table   |
                |                       |
                |                       |
                |                       |
                |                       |
                |                       |
                |_______________________|


-   36:30
    we first defined the number of entries in our Tags, assets and AssetTypes table 

    then we define offsets of the tables inside the file 

                #pragma pack(push, 1)
                struct hha_header
                {
                #define HHA_MAGIC_VALUE HHA_CODE('h','h','a','f')
                    u32 MagicValue;

                #define HHA_VERSION 0
                    u32 Version;

                    u32 TagCount;
                    u32 AssetTypeCount;
                    u32 AssetCount;

                    u64 Tags; // hha_tag[TagCount]
                    u64 AssetTypes; // hha_asset_type[AssetTypeCount]
                    u64 Assets; // hha_asset[AssetCount]
                };

    the definition of our hha_header is very similar to our game_assets struct

                struct game_assets
                {
                    u32 TagCount;
                    hha_tag Tags[VERY_LARGE_NUMBER];

                    u32 AssetTypeCount;
                    hha_asset_type AssetTypes[Asset_Count];

                    u32 AssetCount;
                    asset Assets[VERY_LARGE_NUMBER];

                    hha_asset_type *DEBUGAssetType;
                    asset *DEBUGAsset;
                };


39:49
then we go define what a tag looks like on disc 
for now these are mostly gonna be cut and paste
                
                handmade_file_formats.h                         handmade_asset_h

                struct hha_tag                                  struct asset_tag
                {                                               {
                    u32 ID;                                         uint32 ID; // NOTE(casey): Tag ID       
                    r32 Value;                                      real32 Value;
                };                                              };
                 


for hha_asset_type, we will also store TypeID                   

                struct hha_asset_type                           struct asset_type
                {                                               {
                    u32 TypeID;                                     uint32 FirstAssetIndex;
                    u32 FirstAssetIndex;                            uint32 OnePastLastAssetIndex;
                    u32 OnePastLastAssetIndex;                  };
                };



currently in the game code, the asset_type array index is its TypeID
so its kind of implicit.

        asset_type AssetTypes[Asset_Count];                         asset *Assets;
                                                                     
                 ___________                                    
                | Asset_    |   FirstAssetIndex = 0                 backdrop0
                |  Backdrop |   OnePastLastAssetIndex = 5           backdrop1
                |___________|                                       backdrop2
                | Asset_    |   FirstAssetIndex = 5                 backdrop3
                |  Shadow   |   OnePastLastAssetIndex = 9           backdrop4
                |___________|                                       shadow0
                | Asset_    |   FirstAssetIndex = 9                 shadow1
                |  Tree     |   OnePastLastAssetIndex = 15          shadow2
                |___________|                                       shadow3
                |           |   ...                                 tree0
                |  ...      |                                       tree1
                |___________|                                       tree2
                |           |                                       tree3
                |  ...      |                                       tree4
                |___________|                                       tree5
                |           |                                       ...
                |           |                                       ...
                |___________|


but when we write to disc, we want to write out the TypeID more explicitly.


41:48
for our hha_asset, its very similar as well. 
                

                struct hha_asset                                    struct asset
                {                                                   {
                    u64 DataOffset;                                     uint32 FirstTagIndex;                            
                    u32 FirstTagIndex;                                  uint32 OnePastLastTagIndex;
                    u32 OnePastLastTagIndex;
                    union                                               union
                    {                                                   {
                        hha_bitmap Bitmap;                                  asset_bitmap_info Bitmap;
                        hha_sound Sound;                                    asset_sound_info Sound;
                    };                                                  };
                };                                                  };


            
                    
then Casey added these two 

                struct asset_bitmap_info
                {
                    char *FileName;
                    r32 AlignPercentage[2];
                };

                struct asset_sound_info
                {
                    char *FileName;
                    u32 FirstSampleIndex;
                    u32 SampleCount;
                    sound_id NextIDToPlay;
                };







45:14

Casey writing the code to output the file 


-   notice what we did to define the position offsets for Header.Tags, Header.AssetTypes and Header.Assets

                Header.Tags = sizeof(Header)

    the Tags array comes right after the header. so we give the number of bytes of the Header
    the AssetTypes array comes after the tags array 

                u32 TagArraySize = Header.TagCount*sizeof(hha_tag);
                ...

                Header.AssetTypes = Header.Tags + TagArraySize;

    then we do the same with Assets array. 




                Out = fopen("test.hha", "wb");
                if(Out)
                {
                    hha_header Header = {};
                    Header.MagicValue = HHA_MAGIC_VALUE;
                    Header.Version = HHA_VERSION;
                    Header.TagCount = Assets->TagCount;
                    Header.AssetTypeCount = Asset_Count; // TODO(casey): Do we really want to do this?  Sparseness!
                    Header.AssetCount = Assets->AssetCount;

                    u32 TagArraySize = Header.TagCount*sizeof(hha_tag);
                    u32 AssetTypeArraySize = Header.AssetTypeCount*sizeof(hha_asset_type);
                    u32 AssetArraySize = Header.AssetCount*sizeof(hha_asset);
                    
                    Header.Tags = sizeof(Header);
                    Header.AssetTypes = Header.Tags + TagArraySize;
                    Header.Assets = Header.AssetTypes + AssetTypeArraySize;

                    fwrite(&Header, sizeof(Header), 1, Out);
                    fwrite(Assets->Tags, TagArraySize, 1, Out);
                    fwrite(Assets->AssetTypes, AssetTypeArraySize, 1, Out);
            //        fwrite(AssetArray, AssetArraySize, 1, Out);
                    
                    fclose(Out);
                }
                else
                {
                    printf("ERROR: Couldn't open file :(\n");
                }


49:08
Casey mentioning how fwrite works. 

the way that the c runtime works is that it keeps the position of where you are in the file.
so you start out writing at the zeroth byte. After you write the header, you point to the byte immediately after the header.
kind of like the streaming write paradigm.

apparently Casey absolutely loathes the concept of streaming write.

the concept of file handles have a position is just bad. 



1:05:55
Casey showing us our output test.hha file in the emacs hex editor.


1:08:10
someone asks in the Q/A: why is a position in a stream a bad idea?
its a hidden state, and error prone.

consider the following API 

Method 1 
    Write A 
    Write B 

Method 2 
    Write Location A, A 
    Write Location B, B 

Method 1 only works in single threaded, very simplistic use cases
Method 2 always works, no matter what you want to do. 


For example 
lets say, you run the command in order 
    
    Write A 
    Write B

which gives you A then B.

but lets now consider if you want to queue up or "stack" up your writes.

so then when you add the Write A command, then Write B command. 
then Write B will pop up first. Then B gets written first 
    


then also consider the multithreaded cases
lets say you use method 1 to emulate method 2 

            thread 1
                Seek(locationA);
                Write(A);

            thread 2
                Seek(locationB);
                Write(B);

lets say aftering running Seek(); in thread 1, it gets interrupted. 
then Write(A); is no longer valid. 

so its always very critical that the seeks get bundled together with the writes.



if you want a streaming behaviour. you can always write a streaming api in the layer above. 
but at the low level, it make sense to use method2


1:21:09
what is the main difference between using asserts in your code and writing a separate program to test your results?

so asserts are the preferred method typically in game dev, becuz most of the time it is difficult or impossible to write 
actual inclusive tests, pretty tests that cover all the things that you want to do. So of the time in game dev 
you will use asserts, cuz writing a separate test code is infeasible.

Asserts are a lot more valuable cuz they catch things in run time, in real scenarios.


examples of places where I ve written test code in the past: memory allocators, database queries, math functions 


1:34:01
Casey claims he doesnt know a single good programmer who thinks exception is a good idea. 
C++ exceptions are something you should never use. 
They are categorically bad. 

