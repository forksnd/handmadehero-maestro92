Handmade Hero Day 135 - Typed Asset Arrays
Summary:

more work on the "bitmap family based" asset structure that was started on day 134
mentioned the use case of size_t in the Q/A

Keyword:
asset system

6:30
we will be working with an asset .pak file. That asset .pak file will already be able to preprosses the 
asset in anyway that we want. The assumption is that there is going to be some art pipeline at the head end of this thing.
things come out of it, and they get packed into the .pak file. During the packing process, they will setup those header information
such as (alignPercentage, width, height);



21:58
Casey working more on the asset system. Continuing on the day 134, we properly allocated the memory 
for the few arrays that we have.

recall that Assets->Assets is the global array, so the Asset->AssetCount is the sum of SoundCount and BitmapCount.

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);
                    SubArena(&Assets->Arena, Arena, Size);
                    Assets->TranState = TranState;

                    Assets->BitmapCount = 256*Asset_Count;
                    Assets->BitmapInfos = PushArray(Arena, Assets->BitmapCount, asset_bitmap_info);
                    Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);

                    Assets->SoundCount = 1;
                    Assets->Sounds = PushArray(Arena, Assets->SoundCount, asset_slot);

                    Assets->AssetCount = Assets->SoundCount + Assets->BitmapCount;
                    Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

                    ...
                    ...
                }



21:16
Casey working on the structure to initialize AssetTypes.
Recall that AssetTypes are families of bitmaps. 

-   the Asset->DEBUGUsedBitmapCount and DEBUGUsedAssetCount are just temporary helper variables.
    not thing important.

-   anytime we want to start initializing a AssetTypes, we enclose the AddBitmapAsset calls with
    BeginAssetType(); and EndAssetType();

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    ...
                    ...

                    Assets->DEBUGUsedBitmapCount = 1;
                    Assets->DEBUGUsedAssetCount = 1;

                    BeginAssetType(Assets, Asset_Shadow);
                    AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", V2(0.5f, 0.156682029f));
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Tree);
                    AddBitmapAsset(Assets, "test2/tree00.bmp", V2(0.493827164f, 0.295652181f));
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Sword);
                    AddBitmapAsset(Assets, "test2/rock03.bmp", V2(0.5f, 0.65625f));
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Grass);
                    AddBitmapAsset(Assets, "test2/grass00.bmp");
                    AddBitmapAsset(Assets, "test2/grass01.bmp");
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Tuft);
                    AddBitmapAsset(Assets, "test2/tuft00.bmp");
                    AddBitmapAsset(Assets, "test2/tuft01.bmp");
                    AddBitmapAsset(Assets, "test2/tuft02.bmp");
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Stone);
                    AddBitmapAsset(Assets, "test2/ground00.bmp");
                    AddBitmapAsset(Assets, "test2/ground01.bmp");
                    AddBitmapAsset(Assets, "test2/ground02.bmp");
                    AddBitmapAsset(Assets, "test2/ground03.bmp");
                    EndAssetType(Assets);
                        
                    ...
                    ...
                    
                    return(Assets);
                }



25:12
as you can see here, this is exactly just filling the in the structure that we described in day 134:
Asset->Assets is the global array. AssetTypes, which represents families of bitmaps, will index into Asset->Assets.

should be pretty straight foward

-   we added the Assert(Assets->DEBUGAssetType == 0); at the beginning of the BeginAssetType(); function.
Corresponding to that, inside the EndAssetType(); function, we have Assets->DEBUGAssetType = 0;
So these are just some bits of sanity check, 

                internal void
                BeginAssetType(game_assets *Assets, asset_type_id TypeID)
                {
                    Assert(Assets->DEBUGAssetType == 0);
                    Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
                    Assets->DEBUGAssetType->FirstAssetIndex = Assets->DEBUGUsedAssetCount;
                    Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
                }

                internal void
                AddBitmapAsset(game_assets *Assets, char *FileName, v2 AlignPercentage = V2(0.5f, 0.5f))
                {
                    Assert(Assets->DEBUGAssetType);
                    asset *Asset = Assets->Assets + Assets->DEBUGAssetType->OnePastLastAssetIndex++;
                    Asset->FirstTagIndex = 0;
                    Asset->OnePastLastTagIndex = 0;
                    Asset->SlotID = DEBUGAddBitmapInfo(Assets, FileName, AlignPercentage).Value;
                }

                internal void
                EndAssetType(game_assets *Assets)
                {
                    Assert(Assets->DEBUGAssetType);
                    Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
                    Assets->DEBUGAssetType = 0;
                }



 



35:09
with our asset system refactored, we will make use of it in the FillGroundChunk code. 

essentially, we get a random bitmap_id from the RandomAssetFrom(); function.


                internal void
                FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
                {    
                    task_with_memory *Task = BeginTaskWithMemory(TranState);
                    if(Task)
                    {
                        fill_ground_chunk_work *Work = PushStruct(&Task->Arena, fill_ground_chunk_work);
                        
                        ...
                        ...                        

                        for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
                        {
                            for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
                            {
                                ...
                                ...

                                for(uint32 GrassIndex = 0; GrassIndex < 100; ++GrassIndex)
                                {
                                    bitmap_id Stamp = RandomAssetFrom(TranState->Assets,
                                                                      RandomChoice(&Series, 2) ? Asset_Grass : Asset_Stone,
                                                                      &Series);
                                
                                    v2 P = Center + Hadamard(HalfDim, V2(RandomBilateral(&Series), RandomBilateral(&Series)));
                                    PushBitmap(RenderGroup, Stamp, 2.0f, V3(P, 0.0f), Color);
                                }
                            }
                        }

                        ...
                        ...
                    }


1:05:53
Someone asked why use size_t over uint32_t or int32_t in the Q/A?

size_t when you dont know if you are on a 32 bit or 64 bit platform and you want to talk about how big something is in memory.
if I talk about the size that something takes in memory, how would I know whether I need a 32 bit or 64 bit integer to talk about it 
if i dont know what platform I am on. so size_t is something that we can always rely on to hold any memory sized thing.



1:15:33
Casey talking about API.
he opened up this tool called Dependency Walker.


