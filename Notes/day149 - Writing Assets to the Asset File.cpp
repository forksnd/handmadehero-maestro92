Handmade Hero Day 149 - Writing Assets to the Asset File

Summary:
wrote out the assets array and asset_slots array to the asset file

started the game side loading code to load the .hha asset file in AllocateGameAssets();

mentioned the relationship between compression oriented programming and Mike Acton_s data driven design.

Casey mentioned on the importance of writing portable game code.

Keyword:
Asset, Asset file, Asset file header, Asset system, asset memory management


2:52
if you look at the way we wrote out the arrays (Assets-Tags or Assets->Assets-Types); 

we pretty much have them in memory exactly as they would be in the file. Files and memory are like the same thing.
File is just a permenantly stored version of memory. Files are just bytes, nothing magical going on in there.

so when we do 
                fwrite(Assets->Tags, TagArraySize, 1, Out);

we are essentially saying, take this block of memory, put it into the drive. 


                if(Out)
                {
                    ...
                    ...

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


3:31

Recall in our game_assets struct, we have 4 main arrays 

                struct game_assets
                {
                    ...
                    ...

                    asset_tag *Tags;

                    ...
                    asset *Assets;
                    asset_slot *Slots;
                    
                    ...
                    asset_type AssetTypes[Asset_Count];
                };




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

the asset* assets array also has a 1 to 1 mapping to the actual asset 


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
                




our .hha file will then look like below
              
         ___________________________________________________________________________________________________________________________
        |       struct hha_header                                                                                                   |
        |       {                                                                                                                   |
        |           u32 MagicValue;                                                                                                 |
        |           u32 Version;                                            fwrite(&Header, sizeof(Header), 1, Out);                |    
        |                                                                                                                           |
        |           u32 TagCount;                                                                                                   |
        |           u32 AssetTypeCount;                                                                                             |
        |           u32 AssetCount;                                                                                                 |
        |                                                                                                                           |
        |           u64 Tags; // hha_tag[TagCount]                                                                                  |
        |           u64 AssetTypes; // hha_asset_type[AssetTypeCount]                                                               |
        |           u64 Assets; // hha_asset[AssetCount]                                                                            |
        |       };                                                                                                                  |
        |                                                                                                                           |
        |        _______________________________________                                                                            |
        |       |   hha_tag Tags[VERY_LARGE_NUMBER];    |                                                                           |
        |       |                                       |                                                                           |
        |       |   Tag_FacingDirection: up             |                   fwrite(Assets->Tags, TagArraySize, 1, Out);             |
        |       |   Tag_Height: 180                     |                                                                           |
        |       |   Tag_Weight: 155 lbs                 |                                                                           |
        |       |   Tag_Gender: male                    |                                                                           |
        |       |   Tag_FacingDirection: left           |                                                                           |
        |       |   Tag_Height: 180                     |                                                                           |
        |       |   Tag_Weight: 155 lbs                 |                                                                           |
        |       |   Tag_Gender: mal                     |                                                                           |
        |       |   ...                                 |                                                                           |
        |       |   Tag_Color: green                    |                                                                           |
        |       |   Tag_Type: oak                       |                                                                           |
        |       |   Tag_Color: yellow                   |                                                                           |
        |       |   Tag_Type: oak                       |                                                                           |
        |       |_______________________________________|                                                                           |
        |                                                                                                                           |
        |        ___________________________________________                                                                        |
        |       |   hha_asset_type AssetTypes[Asset_Count]; |                                                                       |
        |       |    ___________                            |               fwrite(Assets->AssetTypes, AssetTypeArraySize, 1, Out); |
        |       |   | Asset_    |                           |                                                                       |
        |       |   |  hero     |                           |                                                                       |
        |       |   |___________|                           |                                                                       |
        |       |   | Asset_    |                           |                                                                       |
        |       |   |  Shadow   |                           |                                                                       |
        |       |   |___________|                           |                                                                       |
        |       |   | Asset_    |                           |                                                                       |
        |       |   |  Tree     |                           |                                                                       |
        |       |   |___________|                           |                                                                       |
        |       |   |           |                           |                                                                       |
        |       |   |  ...      |                           |                                                                       |
        |       |   |___________|                           |                                                                       |
        |       |___________________________________________|                                                                       |

        |        ___________________________                                                                                        |
        |       |                           |                                                                                       |
        |       |   asset *Assets;          |                                                                                       |
        |       |                           |                                                                                       |
        |       |   hero0                   |                   fwrite(Assets->Assets, AssetArraySize, 1, Out);                     |
        |       |   hero1                   |                                                                                       |
        |       |   hero2                   |                                                                                       |
        |       |   hero3                   |                                                                                       |
        |       |   shadow0                 |                                                                                       |
        |       |   shadow1                 |                                                                                       |
        |       |   shadow2                 |                                                                                       |
        |       |   shadow3                 |                                                                                       |
        |       |   shadow4                 |                                                                                       |
        |       |   tree0                   |                                                                                       |
        |       |   tree1                   |                                                                                       |
        |       |   tree2                   |                                                                                       |
        |       |   tree3                   |                                                                                       |
        |       |   tree4                   |                                                                                       |
        |       |   tree5                   |                                                                                       |
        |       |   ...                     |                                                                                       |
        |       |   ...                     |                                                                                       |
        |       |                           |                                                                                       |
        |       |   sound0                  |                                                                                       |
        |       |   sound1                  |                                                                                       |
        |       |___________________________|                                                                                       |
        |                                                                                                                           |
        |                                                                                                                           |
        |        ___________________________________                                                                                |
        |       |   asset_slot *Slots;              |       for(Header.AssetCount)                                                  |
        |       |                                   |       {                                                                       |
        |       |    loaded_bitmap0                 |                                                                               |
        |       |    loaded_bitmap1                 |           fwrite(WAV.Samples[ChannelIndex], Dest->Sound.SampleCount*sizeof(s16), 1, Out);
        |       |    loaded_bitmap2                 |              or                                                               |
        |       |    loaded_bitmap3                 |           fwrite(Bitmap.Memory, Bitmap.Width*Bitmap.Height*4, 1, Out);        |
        |       |    loaded_bitmap4                 |       }                                                                       |
        |       |    loaded_bitmap5                 |                                                                               |
        |       |    ...                            |                                                                               |
        |       |    ...                            |                                                                               |
        |       |    loaded_sound0                  |                                                                               |
        |       |    loaded_sound1                  |                                                                               |
        |       |___________________________________|                                                                               |
        |                                                                                                                           |
        |                                                                                                                           |
        |___________________________________________________________________________________________________________________________|


        if you noticed the order of this will match the struct game_assets in handmade_asset.h 

        struct game_assets
        {
            // TODO(casey): Not thrilled about this back-pointer
            struct transient_state *TranState;
            memory_arena Arena;

            real32 TagRange[Tag_Count];

            uint32 TagCount;
            asset_tag *Tags;

            uint32 AssetCount;
            asset *Assets;
            asset_slot *Slots;
            
            asset_type AssetTypes[Asset_Count];
        };




in episode 148, 
we have written the "header", "hha_tag Tags[VERY_LARGE_NUMBER];" and "hha_asset_type AssetTypes[Asset_Count];" into the file

today, we will tackle the "asset *Assets;" array and the "asset_slot *Slots;"  array

the thing with writing the "asset *Assets;" array is that 

recall 
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

we need to fill in the DataOffset field. We dont know this beforehand.     

so we will have to populate that field as we go 




-   what fseek(); allows you to move the file position to a different position without writing anything
    so what we are doing here is that we are first skipping writing the "asset *Assets;", and 
    we are first going to iterate and write the  "asset_slot *Slots;"  array to the file.
    as we iterate, we will populate the DataOffset field.

    we are doing the skipping by calling 

                fseek(Out, AssetArraySize, SEEK_CUR);

    in the following link,
    http://www.cplusplus.com/reference/cstdio/fseek/

    you can see that there are different ways of calling fseek();   
    you can pass in SEEK_SET, SEEK_CUR or SEEK_END. For more details, refer to the specs


-   then after we are done iterating through the asset_slots, we comeback and finish up writing the "asset *Assets;" array

-   full code below

                test_asset_builder.cpp

                int main(int ArgCount, char **Args)
                {
                    .....................................................
                    ........ Creating game_assets Assets; ...............
                    .....................................................

                    ...
                    ...

                    fwrite(&Header, sizeof(Header), 1, Out);
                    fwrite(Assets->Tags, TagArraySize, 1, Out);
                    fwrite(Assets->AssetTypes, AssetTypeArraySize, 1, Out);
                    fseek(Out, AssetArraySize, SEEK_CUR);

                    for(...... i in Header.AssetCount ......)
                    {
                        asset_source *Source = Assets->AssetSources + AssetIndex;
                        hha_asset *Dest = Assets->Assets + AssetIndex;

                        Dest->DataOffset = ftell(Out);      ,----------------- populating the DataOffset field

                        if(Source->Type == AssetType_Sound)
                        {
                            loaded_sound WAV = LoadWAV(Source->FileName,
                                                       Source->FirstSampleIndex,
                                                       Dest->Sound.SampleCount);
                            
                            Dest->Sound.SampleCount = WAV.SampleCount;
                            Dest->Sound.ChannelCount = WAV.ChannelCount;

                            for(u32 ChannelIndex = 0;
                                ChannelIndex < WAV.ChannelCount;
                                ++ChannelIndex)
                            {
                                fwrite(WAV.Samples[ChannelIndex], Dest->Sound.SampleCount*sizeof(s16), 1, Out);
                            }

                            free(WAV.Free);
                        }
                        else
                        {
                            Assert(Source->Type == AssetType_Bitmap);

                            loaded_bitmap Bitmap = LoadBMP(Source->FileName);

                            Dest->Bitmap.Dim[0] = Bitmap.Width;
                            Dest->Bitmap.Dim[1] = Bitmap.Height;

                            Assert((Bitmap.Width*4) == Bitmap.Pitch);
                            fwrite(Bitmap.Memory, Bitmap.Width*Bitmap.Height*4, 1, Out);
                            
                            free(Bitmap.Free);
                        }
                    }

                    fseek(Out, (u32)Header.Assets, SEEK_SET);
                    fwrite(Assets->Assets, AssetArraySize, 1, Out);
                    fclose(Out);
                }


38:37
notice in the function above we have the LoadWAV(); and LoadBMP(); function
in these two functions, we had to call the ReadEntireFile(); function. 

                internal loaded_sound
                LoadWAV(char *FileName, u32 SectionFirstSampleIndex, u32 SectionSampleCount)
                {
                    loaded_sound Result = {};
                    
                    entire_file ReadResult = ReadEntireFile(FileName);    
                    if(ReadResult.ContentsSize != 0)
                    {
                        Result.Free = ReadResult.Contents;
                        ...
                        ...
                    }
                }


                internal loaded_bitmap
                LoadBMP(char *FileName)
                {
                    loaded_bitmap Result = {};
                    
                    entire_file ReadResult = ReadEntireFile(FileName);
                    if(ReadResult.ContentsSize != 0)
                    {
                        Result.Free = ReadResult.Contents;
                        ...
                        ...
                    }
                }

Casey went on to implement the ReadEntireFile(); function

-   notice how we get the ContentSize.
    we just called the c runtime library function fseek.
    the SEEK_END and SEEK_SET are ways to call the fseek(); function. (as mentioned above);

    we essentially call fseek(..., SEEK_END); to get us straight to the end of the file.
    we record the file position, store it in Result.ContentsSize.
    then we set our file position back to the start again.

-   once we know the content size, we malloc a block of memory and give it to Result.Contents.


                fseek(In, 0, SEEK_END);
                Result.ContentsSize = ftell(In);
                fseek(In, 0, SEEK_SET);

    Casey says, this feels hacky, but he doesnt know a better way to do it with the C runtime library.

-   full code below

                struct entire_file
                {
                    u32 ContentsSize;
                    void *Contents;
                };

                entire_file ReadEntireFile(char *FileName)
                {
                    entire_file Result = {};

                    FILE *In = fopen(FileName, "rb");
                    if(In)
                    {
                        fseek(In, 0, SEEK_END);
                        Result.ContentsSize = ftell(In);
                        fseek(In, 0, SEEK_SET);
                        
                        Result.Contents = malloc(Result.ContentsSize);
                        fread(Result.Contents, Result.ContentsSize, 1, In);
                        fclose(In);
                    }
                    else
                    {
                        printf("ERROR: Cannot open file %s.\n", FileName);
                    }
                    
                    return(Result);
                }

38:59
also notice in the LoadWAV(...); and LoadBMP(); file, we gave the memory to both loaded_sound.Free or loaded_bitmap.Free


                internal loaded_sound
                LoadWAV(char *FileName, u32 SectionFirstSampleIndex, u32 SectionSampleCount)
                {
                    loaded_sound Result = {};
                    
                    entire_file ReadResult = ReadEntireFile(FileName);    
                    if(ReadResult.ContentsSize != 0)
                    {
                        Result.Free = ReadResult.Contents; <-----------------------
                        ...
                        ...
                    }
                }

what we did is, we added a placeholder in loaded_bitmap and loaded_sound 

                struct loaded_sound
                {
                    uint32 SampleCount; // NOTE(casey): This is the sample count divided by 8
                    uint32 ChannelCount;
                    int16 *Samples[2];

                    void *Free;
                };


what happens is that after we finish writing to disc, we free the memory 


        for(u32 AssetIndex = 1; AssetIndex < Header.AssetCount; ++AssetIndex)
        {
            ...
            ...

            if(Source->Type == AssetType_Sound)
            {
                loaded_sound WAV = LoadWAV(...);
                ...
                ...

                for(... each channel ...)
                {
                    fwrite(WAV.Samples[ChannelIndex], Dest->Sound.SampleCount*sizeof(s16), 1, Out);
                }

                free(WAV.Free);
            }
            else
            {
                ...
                loaded_bitmap Bitmap = LoadBMP(...);

                ...
                ...

                fwrite(Bitmap.Memory, Bitmap.Width*Bitmap.Height*4, 1, Out);
                
                free(Bitmap.Free);
            }
        }



48:43
Casey starts cleaning up the code




53:28
Casey starting to work on the game code to load in the asset 

-   notice that we are reading in the .hha asset file  
                
                debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile("test.hha");    
                

-   we first check the MagicValue and Version number

                hha_header *Header = (hha_header *)ReadResult.Contents;
                Assert(Header->MagicValue == HHA_MAGIC_VALUE);
                Assert(Header->Version == HHA_VERSION);
                
-   recall that the file contains 3 arrays, the tags array, the asset array and the asset_slot array.
    we only wrote the for loop for the tags array. we will do the other two array in the next episode


                handmade_asset.cpp

                internal game_assets* AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);
                    SubArena(&Assets->Arena, Arena, Size);
                    Assets->TranState = TranState;

                    for(uint32 TagType = 0; TagType < Tag_Count; ++TagType)
                    {
                        Assets->TagRange[TagType] = 1000000.0f;
                    }
                    Assets->TagRange[Tag_FacingDirection] = Tau32;

                    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile("test.hha");    
                    if(ReadResult.ContentsSize != 0)
                    {
                        hha_header *Header = (hha_header *)ReadResult.Contents;
                        Assert(Header->MagicValue == HHA_MAGIC_VALUE);
                        Assert(Header->Version == HHA_VERSION);
                        
                        Assets->AssetCount = Header->AssetCount;
                        Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
                        Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);

                        Assets->TagCount = Header->TagCount;
                        Assets->Tags = PushArray(Arena, Assets->TagCount, asset_tag);

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

                    }
                }





1:18:40
someone asked the relationship between compression oriented programming and Mike Acton_s data driven design.

Mike Acton is saying the first thing you should do when you write code, is to write a thing that transforms 
data in a way that it is suppose to work.

you got data, you move from one place to another and change it. Thats what code does.
Your code is changing data in place or in route to some place. 

so orient the code around that. Thats what we do in handmade hero and thats what we do.


But data oriented design doesnt really speak to what happens when you want to instill some high level structure.
thats what compression oriented programming is about. you see some common things, then you take them out 
and try to organize them in a reusable way 


1:21:19
wont the game have to wait until all the assets have been decompressed to start?

a simplied version of the giant graph above.

when we fetch a particular bitmap that we want, we will only decompress that specific one. 
we wouldnt compress the entire "Bitmaps" block to decompress all the bitmaps, just to access one of them.

we will setup the compression in a way to allows us to do that random access

                 _______________
                |               |
                |   Tags        |
                |_______________|
                |               |
                |  Asset Types  |
                |_______________|
                |               |
                |  Assets       |
                |_______________|
                |               |
                |   Bitmaps     |
                |               |
                | Bitmap0  <----|-----------     
                | Bitmap1       |
                | ...           |
                | ...           |
                |               |
                |               |
                |_______________|
                |               |
                |   Sounds      |
                |               |
                |_______________|
                


1:24:18
Casey made a remark on since 96% of the gamer audience runs on windows "why bother porting the game to linux or osx at all"?

writing portable code is a good skill to have, and its good to learn. You know if windows will b the 96% forever. 
you also dont know if you want to run on a console. maybe you got a deal with sony and you want to ship on PS4.
you need to have experience of writing proper code that can be easily shipped to more than one platform, cuz in game development 
you are often expected to ship on atleast 2 platforms that has to work well.

