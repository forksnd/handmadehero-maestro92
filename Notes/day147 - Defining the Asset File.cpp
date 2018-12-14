Handmade Hero Day 147 - Defining the Asset File

Summary:
discussed for the asset system, we will address two problems, asset file type and memory management
discussed what is the purpose of having asset file format

created a separate program which aims to be a asset file generator that will output asset file that he main game will read in 

copied lots of asset code to the asset file generator

cleaned up a lot of the asset table code

Keyword:
Asset, Asset file, asset memory management


3:21
we wrote the sound mixer just so that we can see what the needs might be for the Asset system

3:49
The thing we need consider now is that what does the sound mixer need from the asset system
then later on, we need to consider what does the game code want from the sound system

7:51

The asset system is kind of a special case in our system. We want it to sort of act like a virtual memory system.
kind of like the Operating System_s virtual memory system. Only with more predicatability and more control for us.

what we want is that, when we work with assets, we want to have a potentially a giant asset memory footprint, 
for example 16 GB uncompressed worth of asset. But if the user only has 2 GB or 4 GB of memory, we want to take however much 
we are giving to the asset system, and be smart about it. For example only pulling in the assets we are currently or potentially will use.

kind of like paging I suppose.

The goal is to make this asset system scalable that way. So even if at the end, our asset usage is very small, our goal is still to 
set a working set size, and no matter what this working set size is, the artists can pile in as much content as possible. 



10:09
what working on the "Asset File" means is that, now our system is a bunch of random bitmaps on the drive and 
a bunch of random .wav files on the drive, and some code that assigns ID to each.

we want to move away from this, and move on to having a unified structure on the disc, that represents all of the asset information
we actually need to process. That way we can distribute the game as one nice .pak file that has all the stuff in it, 
which makes it easy for the game to get around.


11:11
our game code thinks of asset in terms of 

-   types
-   tags

so the game code will ask for a type of object, for example Type_Hero or Type_Tree.
Then it will specify the tags, which is the sub property. For example facing direction, height tag, 
age tag, weight tag, gender tag

if the artists have put in multiple sprites with different properties and attributes to the asset system. 
for example sprites of characters ranging from 120 cm to 190 cm 

then the game code asks the asset system for a sprite that is 150 cm.
the asset system will find the closest one that match the sprite. 

this is convered in detail in day 136 and day 137.

we have structured it this way becuz we want to decouple the game code with the asset table.


                        |
        Game Code       |       Asset Table
    __________________  |  ______________________
                        |
    -   types           |       Bitmaps
                        |
    -   tags            |
                        |
                        |
                        |


The asset table is a table that has individual assets in them.
i dont want the game code to think in terms of specific bitmaps. 
we want the game code to think in terms of types and tags 

This way the game code and the asset table can update independently.
for example, if i didnt like how the character looked, which I want a older looking characterAll i have to do is to update
the asset table. i dont even have to re-compile.


18:33
Casey once again explaining our current asset system

Taken from day 134

                struct asset_type
                {
                    uint32 FirstAssetIndex;
                    uint32 OnePastLastAssetIndex;
                };

    the FirstAssetIndex and OnePastLastAssetIndex will be used to indicate all the members in your bitmap family


    Graphically, we have something below:

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




20:02
Casey mentioned a problem with this system. 
if you want to have multiple asset files, you need to find a way of managing it. 

for example. If you have the original asset in the game, and you have certain types of tree

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


then you want to ship an expansion pack, and this expansion is just all about trees.


        asset_type AssetTypes[Asset_Count];                         asset *Assets;
                                                                     
                 ___________                                    
                | Asset_    |   FirstAssetIndex = 0                 tree6
                |  Tree     |   OnePastLastAssetIndex = 1000        tree7
                |___________|                                       tree8
                |           |                                       tree9
                |           |                                       tree10
                |___________|                                       tree11
                |           |                                      tree12
                |           |                                       tree13
                |___________|                                       ...


now the problem is that there isnt a single range that ecapsulates all the trees. 


what we want to do to support expansion packs is that we want the expansion pack on disk to be a little different on disc then 
it is in memory. We probably want to "merge" on load.
so in memory all the trees will be contiguous. but on disk, it can be in different places.


28:10
most of the code we wrote in AllocateGameAssets(); is simulating building the asset file in memory

29:01
Casey plans to make a program whose only job it is is to make a dummy asset pak file, that we can start loading. 

Casey editing the build.bat to give himself an extra build target.

Casey wrote the new program, starting with the test_asset_builder.cpp file 


31:41
Casey talking about his views on using libraries.
pretty much if he wants his code to last, you doesnt use libraries. cuz if you do, you will end up 
with a maintenance nightmare. mainly cuz you will always depend on the library developers supporting new platform.
For example if windows 19 comes out, and you want to support that, but the library doesnt do so...
what are you going to do????

If its throw away/testing code, Casey will use libraries. Sometimes there are exception. For example some of the 
std libraries that are well done. 



34:53
Casey copied most of what was in the AllocateGameAssets(); function into the  test_asset_builder.cpp_s new main(); function.
the goal is that the test_asset_builder.cpp will output asset files, which our main game code will read in and use.


37:18
Casey mentioned that test_asset_builder.cpp will just open one file, write everything in there, then output it.

-   the main program pretty much defines 

                FILE *Out

    then writes to it.

-   notice he defined his file to be "test.hha". the .hha file extension stands for handmade hero asset 

-   in this episode, we are not actually writing anything to the file. This will be done in the next few episodes.

                test_asset_builder.cpp

                FILE *Out = 0;

                int
                main(int ArgCount, char **Args)
                {
                    BeginAssetType(Assets, Asset_Shadow);
                    AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", V2(0.5f, 0.156682029f));
                    EndAssetType(Assets);

                    ...
                    ...

                    BeginAssetType(Assets, Asset_Puhp);
                    AddSoundAsset(Assets, "test3/puhp_00.wav");
                    AddSoundAsset(Assets, "test3/puhp_01.wav");
                    EndAssetType(Assets);

                    Out = fopen("test.hha", "wb");
                    if(Out)
                    {
                        
                        
                        fclose(Out);
                    }

                }


44:53 
Casey pulled out the asset_type_id enums into a separate file handmade_asset_type_id.h


                enum asset_type_id
                {
                    Asset_None,

                    //
                    // NOTE(casey): Bitmaps!
                    //
                    
                    Asset_Shadow,
                    Asset_Tree,
                    Asset_Sword,
                //    Asset_Stairwell,
                    Asset_Rock,

                    Asset_Grass,
                    Asset_Tuft,
                    Asset_Stone,

                    Asset_Head,
                    Asset_Cape,
                    Asset_Torso,

                    ...
                    ...
                }

this way it can be shared among different programs. In our case, our asset file generator and our game code both need it. 



45:59
previously our asset information were all stored inside the struct game_assets 

pretty much we stored all of our asset tables and arrays in struct game_assets

                handmade_asset.h 

                struct game_assets
                {
                    // TODO(casey): Not thrilled about this back-pointer
                    ...
                    ...

                    real32 TagRange[Tag_Count];

                    uint32 TagCount;
                    asset_tag *Tags;

                    uint32 AssetCount;
                    asset *Assets;
                    asset_slot *Slots;
                    
                    asset_type AssetTypes[Asset_Count];

                    // NOTE(casey): Structured assets
                //    hero_bitmaps HeroBitmaps[4];

                    // TODO(casey): These should go away once we actually load a asset pack file
                    uint32 DEBUGUsedBitmapCount;
                    uint32 DEBUGUsedSoundCount;
                    uint32 DEBUGUsedAssetCount;
                    uint32 DEBUGUsedTagCount;
                    asset_type *DEBUGAssetType;
                    asset *DEBUGAsset;
                };



we want to copy the functionality to test_asset_builder.cpp. So we first copied all these arrays, and put it in test_asset_builder.cpp 


                test_asset_builder.cpp 

                #define VERY_LARGE_NUMBER 4096

                uint32 BitmapCount;
                uint32 SoundCount;
                uint32 TagCount;
                uint32 AssetCount;
                asset_bitmap_info BitmapInfos[VERY_LARGE_NUMBER];
                asset_sound_info SoundInfos[VERY_LARGE_NUMBER];
                asset_tag Tags[VERY_LARGE_NUMBER];
                asset Assets[VERY_LARGE_NUMBER];
                asset_type AssetTypes[Asset_Count];

                u32 DEBUGUsedBitmapCount;
                u32 DEBUGUsedSoundCount;
                u32 DEBUGUsedAssetCount;
                u32 DEBUGUsedTagCount;
                asset_type *DEBUGAssetType;
                asset *DEBUGAsset;


51:14 52:18

Casey did some refactoring on the game_assets asset system as well 

previously we have 

                struct asset
                {
                    uint32 FirstTagIndex;
                    uint32 OnePastLastTagIndex;
                    uint32 SlotID;
                };

                struct game_assets
                {
                    ...
                    ...

                    real32 TagRange[Tag_Count];
                    
                    uint32 BitmapCount;
                    asset_bitmap_info *BitmapInfos;
                    asset_slot *Bitmaps;

                    uint32 SoundCount;
                    asset_sound_info *SoundInfos;
                    asset_slot *Sounds;

                    uint32 TagCount;
                    asset_tag *Tags;

                    uint32 AssetCount;
                    asset *Assets;
                    
                    asset_type AssetTypes[Asset_Count];

                    ...
                    ...
                };


now we have 

-   Casey first got rid of "asset_slot *Bitmaps;" and "asset_slot *Sounds;."  (along with their info arrays)

                uint32 BitmapCount;
                asset_bitmap_info *BitmapInfos;
                asset_slot *Bitmaps;

                uint32 SoundCount;
                asset_sound_info *SoundInfos;
                asset_slot *Sounds;

    pretty much all of these 6 are gone. 

    which he combined it into just one array: "asset_slot *Slots;"

    So Casey removed the two separate arrays, and replaced them into a single array. 


-   Casey got rid of the SlotID field in struct asset
    
    previously we had two separate arrays for bitmaps and sounds, so SlotID was used to index into 
    two separate arrays. 

    now that we have one global asset array now, we dont need it.
    instead we have the asset point to its info struct 


                struct asset
                {
                    uint32 FirstTagIndex;
                    uint32 OnePastLastTagIndex;

                    union
                    {
                        asset_bitmap_info Bitmap;
                        asset_sound_info Sound;
                    };
                };


                struct game_assets
                {
                    ...
                    ...

                    real32 TagRange[Tag_Count];

                    uint32 TagCount;
                    asset_tag *Tags;

                    uint32 AssetCount;
                    asset *Assets;
                    asset_slot *Slots;
                    
                    asset_type AssetTypes[Asset_Count];

                    ...
                    ...
                };
