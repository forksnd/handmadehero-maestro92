Handmade Hero Day 132 - Asset Streaming
Summary:

changed the api so that all the game logic accesses bitmaps through game_asset_id instead 
directly touching the loaded_bitmap.

make the decision so that if we first try to load a bitmap it is not there, we just dont render it.
but we load it so that it will be there next time. So we got our asset loading deferred

made the load bitmap part run in the background.

Keyword:
Asset streaming, multithreading




4:35
that the game code and the assets have to agree on the name of something.


6:00
all you have to do to have a reasonable asset streaming system is to you need to make sure 
the names are always valid 

if you are referring to your assets through ways that is crashable, then it is not stable.
for example a pointer to memory is not a good way to refer to an asset. 





12:14
Casey starting to clean up the asset code.
Previously all of our assets are in the game_state struct

                handmade.h

                struct game_state
                {
                    ...
                    ...

                    loaded_bitmap Grass[2];
                    loaded_bitmap Stone[4];
                    loaded_bitmap Tuft[3];
                    
                    loaded_bitmap Backdrop;
                    loaded_bitmap Shadow;
                    hero_bitmaps HeroBitmaps[4];

                    loaded_bitmap Tree;
                    loaded_bitmap Sword;
                    loaded_bitmap Stairwell;

                    ...
                    ...
                };

and manually load them in the initialization code 


                if(!Memory->IsInitialized)
                {   
                    GameState->Grass[0] =
                        DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/grass00.bmp");
                    GameState->Grass[1] =
                        DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/grass01.bmp");

                    GameState->Tuft[0] =
                        DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/tuft00.bmp");
                    GameState->Tuft[1] =
                        DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/tuft01.bmp");
                    GameState->Tuft[2] =
                        DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/tuft02.bmp");

                    ...
                    ...
                }



Casey now putting everything into a new game_assets struct. 
if you look at the game_state struct, we essentially took all the stuff that were loaded from disk into this struct.
everything else in the game_state struct is not from disk.


                struct game_assets
                {

                    loaded_bitmap Grass[2];
                    loaded_bitmap Stone[4];
                    loaded_bitmap Tuft[3];
                    
                    loaded_bitmap Backdrop;
                    loaded_bitmap Shadow;
                    hero_bitmaps HeroBitmaps[4];

                    loaded_bitmap Tree;
                    loaded_bitmap Sword;
                    loaded_bitmap Stairwell;
                };


14:37
Casey re-organizing the game_assets struct 
we have the arrayed assets, and we have the structured assets.
Recall that hero_bitmaps has three parts:

the Backdrop, Shadow and stuff has no structures in them, they are just plain vanilla bitmaps.

                struct hero_bitmaps
                {
                    loaded_bitmap Head;
                    loaded_bitmap Cape;
                    loaded_bitmap Torso;
                };


                struct game_assets
                {
                    loaded_bitmap Backdrop;
                    loaded_bitmap Shadow;
                    
                    loaded_bitmap Tree;
                    loaded_bitmap Sword;
                    loaded_bitmap Stairwell;

                    // arrayed assets 
                    loaded_bitmap Grass[2];
                    loaded_bitmap Stone[4];
                    loaded_bitmap Tuft[3];
                    
                    // structured assets 
                    hero_bitmaps HeroBitmaps[4];
                };



15:40
the fundamental problem with our current structure for the purpose of asset streaming is that 

1.  the entire loaded_bitmap data has to be loaded for others to refer to it.
2.  anyone who is gonna refer to Backdrop is gonna refer directly to it. (they will directly access
    this piece of bitmap memory)

17:30
we would prefer people to refer to them in an more indirect way or a more abstract way. 
So instead of the assets API being pointers and bitmaps, Casey wants to the assets API to be id based. 
so Casey created the enums 

                enum game_asset_id
                {
                    GAI_Backdrop,
                    GAI_Shadow,
                    GAI_Tree,
                    GAI_Sword,
                    GAI_Stairwell,

                    GAI_Count,
                };

GAI is game_asset_id 



18:40
and Casey wrote the GetBitmap(); function. This will be the API that people will use.

                struct game_assets
                {
                    // TODO(casey): Not thrilled about this back-pointer
                    struct transient_state *TranState;
                    memory_arena Arena;
                    debug_platform_read_entire_file *ReadEntireFile;
                    
                    loaded_bitmap *Bitmaps[GAI_Count];

                    // NOTE(casey): Array'd assets
                    loaded_bitmap Grass[2];
                    loaded_bitmap Stone[4];
                    loaded_bitmap Tuft[3];

                    // NOTE(casey): Structured assets
                    hero_bitmaps HeroBitmaps[4];
                };
                inline loaded_bitmap *GetBitmap(game_assets *Assets, game_asset_id ID)
                {
                    loaded_bitmap *Result = Assets->Bitmaps[ID];

                    return(Result);
                }


Casey proceeds to replace all code where we are accessing the loaded_bitmap through this GetBitmap(); call.


26:58
moved the game_assets struct from game_state to transient_state

                struct transient_state
                {
                    ...
                    ...

                    game_assets Assets;
                };


30:19
Casey wrote the game_asset_id version of the PushBitmap(); function 

                inline void
                PushBitmap(render_group *Group, game_asset_id ID, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
                {
                    loaded_bitmap *Bitmap = GetBitmap(Group->Assets, ID);
                    if(Bitmap)
                    {
                        PushBitmap(Group, Bitmap, Height, Offset, Color);
                    }
                }


Casey explains that if the bitmap isnt there, we just dont render it. 
your game should be set up so that the background streaming is always ahead. But if streaming ever falls behind,
I would rather have the frame rate not take a hit. 
there are other ways to mitigate it, for example, have a fallback bitmap.


40:04
with this structure, we dont have to load our assets until somebody actually asks for them



48:31
we initalize the memory for assets




                if(!TranState->IsInitialized)
                {
                    InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                                    (uint8 *)Memory->TransientStorage + sizeof(transient_state));

                    SubArena(&TranState->Assets.Arena, &TranState->TranArena, Megabytes(64));
                    TranState->Assets.ReadEntireFile = Memory->DEBUGPlatformReadEntireFile;
                    TranState->Assets.TranState = TranState;

                    ...
                    ...
                }



49:52
Casey changed it so that if the bitmap is not loaded, we load the bitmap, so that it will be there next time.

                inline void
                PushBitmap(render_group *Group, game_asset_id ID, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
                {
                    loaded_bitmap *Bitmap = GetBitmap(Group->Assets, ID);
                    if(Bitmap)
                    {
                        PushBitmap(Group, Bitmap, Height, Offset, Color);
                    }
                    else
                    {
                        LoadAsset(Group->Assets, ID);
                    }
                }





50:46
Casey proceed to make the asset loading run in the background 

the pattern is exactly the same as our other multithreading stuff,
we have the job data and job description, and we give the jobs to the LowPriorityQueue, which the threads will take from.


                struct load_asset_work
                {
                    game_assets *Assets;
                    char *FileName;
                    game_asset_id ID;
                    task_with_memory *Task;
                    loaded_bitmap *Bitmap;
                };

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
                {
                    load_asset_work *Work = (load_asset_work *)Data;

                    // TODO(casey): Get rid of this thread thing when I load through a queue instead of the debug call.
                    thread_context *Thread = 0;
                    *Work->Bitmap = DEBUGLoadBMP(Thread, Work->Assets->ReadEntireFile, Work->FileName);
                    // , AlignX, TopDownAlignY);

                    // TODO(casey): Fence!
                    
                    Work->Assets->Bitmaps[Work->ID] = Work->Bitmap;

                    EndTaskWithMemory(Work->Task);
                }

this way, anytime we call LoadAsset, it is done in the background.

                internal void
                LoadAsset(game_assets *Assets, game_asset_id ID)
                {
                    task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
                    if(Task)        
                    {
                        // TODO(casey): Get rid of this thread thing when I load through a queue instead of the debug call.
                        debug_platform_read_entire_file *ReadEntireFile = Assets->ReadEntireFile;

                        load_asset_work *Work = PushStruct(&Task->Arena, load_asset_work);

                        Work->Assets = Assets;
                        Work->ID = ID;
                        Work->FileName = "";
                        Work->Task = Task;
                        Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);

                        PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);

                        thread_context *Thread = 0;
                        switch(ID)
                        {
                            case GAI_Backdrop:
                            {
                                Work->FileName = "test/test_background.bmp";
                            } break;

                            case GAI_Shadow:
                            {
                                Work->FileName = "test/test_hero_shadow.bmp";
                                // 72, 182
                            } break;

                            case GAI_Tree:
                            {
                                Work->FileName = "test2/tree00.bmp";
                                // 40, 80
                            } break;

                            case GAI_Stairwell:
                            {            
                                Work->FileName = "test2/rock02.bmp";
                            } break;

                            case GAI_Sword:
                            {
                                Work->FileName = "test2/rock03.bmp";
                                // 29, 10
                            } break;
                        }
                    }
                }


1:05:03
in this episode, we got deferred loading working. But there are still other things we want to work on 
For example, how do we want assets get evicted? 
If we want to run the game in a fixed amount of memory, and our memory budget does not allow us to have all of our 
assets in memory at once. 
so we want to be able to evict assets smartly

also we want to tackle arrayed assets and structured assets later. 


1:06:44
Casey mentioned his plan of handling double loading. For example, if an asset is already queued to load,
then another thread comes in to load the bitmap, we want to avoid that. 

the approach he plans to take is to record asset states 

                handmade.h

                enum asset_state
                {
                    AssetState_Unloaded,
                    AssetState_Queued,
                    AssetState_Loaded,
                };
                struct asset_handle
                {
                    asset_state State;
                    loaded_bitmap *Bitmap;
                };


