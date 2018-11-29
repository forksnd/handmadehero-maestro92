Handmade Hero Day 134 - Mapping Assets to Bitmaps
Summary:

mentioned the plans for the structure of the asset system, added handemade_asset.h and .cpp

started the "bitmap family based" asset structure. 

someone asked about "how do you procedurally generate character movement?" in the Q/A
Casey mentioned David Rosen_s GDC talk
https://www.youtube.com/watch?v=LNidsMesxSE

Keyword:
asset system


3:55
Casey describing how he wants the game to generate trees and tree sprites.
essentially he wants a decision vector to determine what kind of tree to be generated (depending 
on terrain, tree height, etc etc);



4:27
Casey describes his ideal user case:
as a user, he will make a request to the assets sytem for a tree with certain properties,
the user doesnt have to care about how many trees the assets sytem has. 

with this design, we get more flexiblity: without having to change game code, we can upgrade the set of 
visual representations 

Casey proceeds to polish up the asset system




10:37 34:41
first Casey changed asset_id into asset_type_id. The idea behind asset_type_id is that
asset_type will represent a family of bitmaps.

For example, we declared this Asset_Tree asset type. Then we will load in all kinds of tree bitmaps. 

The user wont have to care specific bitmaps, the usage code will be such that the user will be requesting a type of 
asset (for example a type of tree), then the asset system will return a specific bitmap

                enum asset_type_id
                {
                    Asset_None,
                    
                    Asset_Backdrop,
                    Asset_Shadow,
                    Asset_Tree,
                    Asset_Sword,
                    Asset_Stairwell,
                    Asset_Rock,
                    
                    Asset_Count,
                };


[Personally, I would rename Asset_Count to Asset_Type_Count]




11:28 47:48
Casey polishing the game_assets struct
as you can see, Casey introduced a couple of arrays 


                struct game_assets
                {
                    ...
                    ...

                    uint32_t BitmapCount;
                    asset_slot *Bitmaps;

                    uint32_t SoundCount;
                    asset_slot *Sounds;

                    uint32_t TagCount;
                    asset_tag *Tags;

                    uint32_t AssetCount;
                    asset *Assets;
                    
                    asset_type AssetTypes[Asset_Count];

                    ...
                    ...
                };


-   the main idea is below:
    
    the "asset* Assets" is the global array of all assets. 


-   the "asset_type AssetTypes[Asset_Count];" like previously mentioned represents families of bitmaps.

    recall that asset_type structure will be as below:

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






-   we then have the "asset_slot *Bitmaps" and "asset_slot *Sounds" 

                struct asset_slot
                {
                    asset_state State;
                    loaded_bitmap *Bitmap;
                };

    these two contains actual asset data. 


-   the "asset* Assets" global array of all assets, will actually use the SlotID to fetch the actual asset data.

                struct asset
                {
                    ...
                    ...
                    uint32 SlotID;
                };


so graphically, if you are going from Asset_Type to actualy bitmap, there are two levels of indirection


    asset_type AssetTypes[Asset_Count];                         
                                            asset *Assets;                  asset_slot *Bitmaps;             
                 ___________                                    
                | Asset_    |               backdrop0                       bitmap0  
                |  Backdrop |               backdrop1                       bitmap1
                |___________|               backdrop2                       bitmap2
                | Asset_    |               backdrop3                       bitmap3
                |  Shadow   |               backdrop4                       bitmap4
                |___________|               shadow0                         bitmap5
                | Asset_    |  --------->   shadow1         --------->      bitmap6
                |  Tree     |               shadow2                         ...
                |___________|               shadow3                         ...
                |           |               tree0
                |  ...      |               tree1
                |___________|               tree2
                |           |               tree3
                |  ...      |               tree4
                |___________|               tree5
                |           |               ...
                |           |               ...
                |___________|









21:33

here in the transtate, we load the GameAssets

                    // NOTE(casey): Transient initialization
                
                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    if(!TranState->IsInitialized)
                    {
                        InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                                        (uint8 *)Memory->TransientStorage + sizeof(transient_state));

                        ...
                        ...

                        TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(64), TranState);

                        ...
                    }

                }




23:14 38:00
Casey writing the AllocateGameAssets(); function 
notice the function takes in the memory_index Size argument. That is the memory budget that the 
asset system will have. 
                
notice that we get the memory off of the Transtate memory arena, and we did a SubArena call.

-   we allocated memory for all the arrays in the game_assets struct.

                handmade_asset.cpp

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);
                    SubArena(&Assets->Arena, Arena, Size);
                    Assets->TranState = TranState;

                    Assets->BitmapCount = Asset_Count;
                    Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);

                    Assets->SoundCount = 1;
                    Assets->Sounds = PushArray(Arena, Assets->SoundCount, asset_slot);

                    Assets->TagCount = 0;
                    Assets->Tags = 0;

                    Assets->AssetCount = Assets->BitmapCount;
                    Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

                    for(uint32_t AssetID = 0; AssetID < Asset_Count; ++AssetID)
                    {
                        asset_type *Type = Assets->AssetTypes + AssetID;
                        Type->FirstAssetIndex = AssetID;
                        Type->OnePastLastAssetIndex = AssetID + 1;

                        asset *Asset = Assets->Assets + Type->FirstAssetIndex;
                        Asset->FirstTagIndex = 0;
                        Asset->OnePastLastTagIndex = 0;
                        Asset->SlotID = Type->FirstAssetIndex;
                    }

                    ...
                    ...
                }












27:02
previously, we only had one LoadAsset() function;. Casey mentions that in our game, we will have 
both bitmaps and sound audio, hence we now have 
            
                internal void LoadBitmap(game_assets* Assets, uint32 ID); and 
                internal void LoadSound(game_assets* Assets, uint32 ID);
                
furthermore, notice that both functions takes in an integer ID, Casey defined two new structs to minimize 
the chance of user accidentally making the mistake of passing in the wrong ID


                struct bitmap_id
                {
                    uint32 Value;
                };

                struct audio_id
                {
                    uint32 Value;
                };

                internal void LoadBitmap(game_assets *Assets, bitmap_id ID);
                internal void LoadSound(game_assets *Assets, audio_id ID);

this way you have to explicitly pass in bitmap_id or audio_id





32:04
the LoadBitmap(); is essentially our previous LoadAsset(); function

-   notice we changed load_assetwork to load_bitmap_work. Pretty straight-forward.

-   the manual loading of file names will be removed in the future.

                handmade_asset.cpp

                internal void LoadBitmap(game_assets *Assets, bitmap_id ID)
                {
                    if(ID.Value && (AtomicCompareExchangeUInt32((uint32 *)&Assets->Bitmaps[ID.Value].State, AssetState_Unloaded, AssetState_Queued) ==
                        AssetState_Unloaded))
                    {    
                        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
                        if(Task)        
                        {
                            load_bitmap_work *Work = PushStruct(&Task->Arena, load_bitmap_work);

                            ...
                            ...

                            switch(ID.Value)
                            {
                                case Asset_Backdrop:
                                {
                                    Work->FileName = "test/test_background.bmp";
                                } break;

                                case Asset_Shadow:
                                {
                                    Work->FileName = "test/test_hero_shadow.bmp";
                                    ...
                                    ...
                                } break;

                                case Asset_Tree:
                                {
                                    Work->FileName = "test2/tree00.bmp";
                                    ...
                                } break;

                                case Asset_Stairwell:
                                {            
                                    Work->FileName = "test2/rock02.bmp";
                                } break;

                                case Asset_Sword:
                                {
                                    Work->FileName = "test2/rock03.bmp";
                                    ...
                                    ...
                                } break;
                            }

                            PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadBitmapWork, Work);
                        }
                    }    
                }







35:15
as previously mentioned, tale the example of bitmaps, the user will be interacting with Asset_types.
so here when we render the EntityType_Hero, Casey just passes the Asset_Shadow to the GetFirstBitmapID(); function

Note that the GetFirstBitmapID(); is just for porting purposes. Casey was just in hurry to get the game to compile.
Nevertheless, this shows how we want the usage code to be. The User will be mostly interacting with Asset_type

                handmade.cpp

                switch(Entity->Type)
                {
                    case EntityType_Hero:
                    {
                        // TODO(casey): Z!!!
                        real32 HeroSizeC = 2.5f;
                        PushBitmap(RenderGroup, GetFirstBitmapID(TranState->Assets, Asset_Shadow), HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                        PushBitmap(RenderGroup, &HeroBitmaps->Torso, HeroSizeC*1.2f, V3(0, 0, 0));
                        PushBitmap(RenderGroup, &HeroBitmaps->Cape, HeroSizeC*1.2f, V3(0, 0, 0));
                        PushBitmap(RenderGroup, &HeroBitmaps->Head, HeroSizeC*1.2f, V3(0, 0, 0));
                        DrawHitpoints(Entity, RenderGroup);
                    } break;

                    ...
                    ...
                }
                    

51:57
the GetFirstBitmapID(); is below. Refer to section 11:28 47:48
the table describes what we are doing here. The "asset_type *Type" gives you all the bitmap members inside the global asset array.
we are just going to take the first one. 

the if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex) check is just to see if this family actually has any valid members. 
if there arent any valid members, we just return empty bitmap_id.


                handmade_asset.cpp

                internal bitmap_id GetFirstBitmapID(game_assets *Assets, asset_type_id TypeID)
                {
                    bitmap_id Result = {};

                    asset_type *Type = Assets->AssetTypes + TypeID;
                    if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
                    {
                        asset *Asset = Assets->Assets + Type->FirstAssetIndex;
                        Result.Value = Asset->SlotID;
                    }

                    return(Result);
                }


1:11:50
how do you procedurally generate character movement?
Casey mentioned David Rosen_s talk

