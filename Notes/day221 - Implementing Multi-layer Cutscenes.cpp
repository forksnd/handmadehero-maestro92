Handmade Hero Day 221 - Implementing Multi-layer Cutscenes

Summary:
loaded some cut scenes art, which are contained in intro_art.hha 
added the Asset_OpeningCutscene as asset_type_id 
added Tag_ShotIndex and Tag_LayerIndex as asset_tag_id

rendered cut scenes in perspective mode. gave different layers their own z depth, which creates that parallax effect.

Keyword:
cut scene, asset system


4:44
Casey added two asset_tag in asset_tag_id.
also added the Asset_OpeningCutscene in asset_type_id

                handmade_file_formats.h

                enum asset_tag_id
                {
                    ...
                    ...

                    Tag_ShotIndex,
                    Tag_LayerIndex,
                    
                    Tag_Count,
                };

                enum asset_type_id
                {
                    Asset_None,

                    ...
                    ...

                    Asset_OpeningCutscene,
                    
                    Asset_Count,
                };



6:40
Casey explaining how the cut scene art is structured

like any Cutscenes, there are a series of shots 


      shot 1           shot 2           shot 3              shot N
     ___________     ___________     ___________          ___________
    |           |   |           |   |           |        |           |
    |           |   |           |   |           |        |           |
    |           |   |           |   |           |  ...   |           |
    |           |   |           |   |           |        |           |
    |___________|   |___________|   |___________|        |___________|


each shot we have multiple layers 


       layer 1         layer 2         layer 3
     ___________     ___________     ___________ 
    |           |   |           |   |           |
    |           |   |           |   |           |
    |           |   |           |   |           |
    |           |   |           |   |           |
    |___________|   |___________|   |___________|


the layer 1 can have some background hills 
layer 2 can have the character or waht not
layer 3 can have something

this way we can achieve some parallax effect. we can do some zooming which we can have some depth to the image
This way it will be a lot more interesting as oppose to just sticking a bitmap onto the screen. 


8:30
so all the cut scene asset will have Asset_OpeningCutscene asset_type_id
also, they will be tagged with two tags: Tag_ShotIndex and Tag_LayerIndex.

recall that we have our asset matching algorithm. This way we can pull out any shot and layer.


9:23 
Casey reiterated that we do all of our asset loading on demand.
the intro_art.hha is 462,867 KB. (which is half of GB);

so we arent actually loading 500 MB in an instance. We are only loading header of the .hha file.
this way our asset system knows about the asset. 



10:37
recall in our initalization code, we gave our asset system 16 megabyes of memory 

                handmade.cpp
                
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

                        TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(16), TranState);
                 
                        ...
                        ...
                    }
                    ...
                    ...
                }

that is way too low for our Cutscenes.

so if we are at 1080 p, that equates to a 1920 x 1080 screen. if we have 4 bytes per pixel, then we have 

so each layer is at least this size: 1920 x 1080 x 4 = 8,294,400
if the layer has some ability to pan, it could be even bigger
so each layer, at minimum is 8 MB. 

so with our current 16 MB of memory allocated to the asset system, we wouldnt even be able to load one cut scene. probably 
cant even load one single frame if it has 3 layers 


so Casey gave it 256 MB


                handmade.cpp
                
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

                        TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(256), TranState);
                 
                        ...
                        ...
                    }
                    ...
                    ...
                }




13:00
so off to do some cut scene coding.
Casey added the handmade_cutscene.cpp

so the first thing we want to do is to just take the first shot and render all the layers 


-   we have 8 layers and layer 0 is the null layer (I suppose);? 
    so casey put "for(u32 LayerIndex = 1; LayerIndex <= 8; ++LayerIndex)"

                handmade_cutscene.cpp

                internal void RenderCutscene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
                {
                    ...............................................................
                    ....... Camera Persepctive parameters initalization ...........
                    ...............................................................

                    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels, FocalLength, DistanceAboveGround);

                    asset_vector MatchVector = {};
                    asset_vector WeightVector = {};
                    WeightVector.E[Tag_ShotIndex] = 10.0f;
                    WeightVector.E[Tag_LayerIndex] = 1.0f;

                    int ShotIndex = 1;
                    MatchVector.E[Tag_ShotIndex] = (r32)ShotIndex;
                       
                    for(u32 LayerIndex = 1; LayerIndex <= 8; ++LayerIndex)
                    {
                        MatchVector.E[Tag_LayerIndex] = (r32)LayerIndex;        
                        bitmap_id LayerImage = GetBestMatchBitmapFrom(Assets, Asset_OpeningCutscene, &MatchVector, &WeightVector);
                        PushBitmap(RenderGroup, LayerImage, Placement.w, V3(0, 0, 0));
                    }
                }






30:56
Casey will now do placement and movement

each of these layers is something we want to position in space. What we want to do is to simulate the fact that we have some perspective 
in our scene.  

to give you an idea, imagine we have layer A and layer B

                  \     _________A__________     /
                   \                            /
                    \                          /
                     \                        /
                      \                      /
                       \                    /
                        \                  /
                         \  _____B______  /
                          \              /
                           \            /
                            \          /
                             \        /
                              \      /
                               \    /
                                eye

if our eye moves forward, we want layer B to become larger than at a rate that is larger then layer A

so what we can do is that we can give each layer a z-depth



first thing we want to do is to give some notion of a camera movement overtime

that is actually the whole point of having all these layers. Having all these layers allows us to play with the depth 
of the final look. Otherwise, we can just put it in one bitmap. 


32:29
so in game_state, we added a timer that we can drive our Cutscene time from.


                struct game_state
                {
                    bool32 IsInitialized;

                    ...
                    ...

                    r32 tCutScene;
                };



then in the RenderCutscene(); function, we pass the Cutscene variable in.
so just as a temporary measure, we will slowly zoom the camera in 


this is done by changing the DistanceAboveGround value 

                r32 tStart = 0.0f;
                r32 tEnd = 5.0f;
                r32 tNormal = Clamp01MapToRange(tStart, tCutScene, tEnd);

                v3 CameraStart = {0.0f, 0.0f, 0.0f};
                v3 CameraEnd = {-4.0f, -2.0f, 0.0f};
                v3 CameraOffset = Lerp(CameraStart, tNormal, CameraEnd);
                real32 DistanceAboveGround = 10.0f - tNormal*5.0f;

so tNormal is value from 0 to 1 that tells us where we are in our cut scene.
so if tCutScene is greather than 5.0f, tNormal will just be clamped at 1.


-   so we also have to manully specify each layer_s placement 
    notice that it is a v4. the 4th value is the "scaling" factor, meaning the height of the bitmap
    more on that layer

                v4 LayerPlacement[] =
                {
                    {0.0f, 0.0f, DistanceAboveGround - 200.0f, 300.0f}, // NOTE(casey): Sky background
                    {0.0f, 0.0f, -170.0f, 300.0f}, // NOTE(casey): Weird sky light
                    {0.0f, 0.0f, -100.0f, 40.0f}, // NOTE(casey): Backmost row of trees
                    {0.0f, 10.0f, -70.0f, 80.0f}, // NOTE(casey): Middle hills and trees
                    {0.0f, 0.0f, -50.0f, 70.0f}, // NOTE(casey): Front hills and trees 
                    {30.0f, 0.0f, -30.0f, 50.0f}, // NOTE(casey): Right side tree and fence
                    {0.0f, -2.0f, -20.0f, 40.0f}, // NOTE(casey): 7
                    {2.0f, -1.0f, -5.0f, 25.0f}, // NOTE(casey): 8
                };


-   full code below:                


                internal void RenderCutscene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, r32 tCutScene)
                {
                    // TODO(casey): Unify this stuff?
                    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
                    real32 MetersToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;

                    real32 FocalLength = 0.6f;

                    r32 tStart = 0.0f;
                    r32 tEnd = 5.0f;

                    r32 tNormal = Clamp01MapToRange(tStart, tCutScene, tEnd);

                    v3 CameraStart = {0.0f, 0.0f, 0.0f};
                    v3 CameraEnd = {-4.0f, -2.0f, 0.0f};
                    v3 CameraOffset = Lerp(CameraStart, tNormal, CameraEnd);
                    real32 DistanceAboveGround = 10.0f - tNormal*5.0f;
                    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels, FocalLength, DistanceAboveGround);

                    asset_vector MatchVector = {};
                    asset_vector WeightVector = {};
                    WeightVector.E[Tag_ShotIndex] = 10.0f;
                    WeightVector.E[Tag_LayerIndex] = 1.0f;

                    int ShotIndex = 1;
                    MatchVector.E[Tag_ShotIndex] = (r32)ShotIndex;
                    v4 LayerPlacement[] =
                    {
                        {0.0f, 0.0f, DistanceAboveGround - 200.0f, 300.0f}, // NOTE(casey): Sky background
                        {0.0f, 0.0f, -170.0f, 300.0f}, // NOTE(casey): Weird sky light
                        {0.0f, 0.0f, -100.0f, 40.0f}, // NOTE(casey): Backmost row of trees
                        {0.0f, 10.0f, -70.0f, 80.0f}, // NOTE(casey): Middle hills and trees
                        {0.0f, 0.0f, -50.0f, 70.0f}, // NOTE(casey): Front hills and trees 
                        {30.0f, 0.0f, -30.0f, 50.0f}, // NOTE(casey): Right side tree and fence
                        {0.0f, -2.0f, -20.0f, 40.0f}, // NOTE(casey): 7
                        {2.0f, -1.0f, -5.0f, 25.0f}, // NOTE(casey): 8
                    };
                        
                    for(u32 LayerIndex = 1; LayerIndex <= 8; ++LayerIndex)
                    {
                        v4 Placement = LayerPlacement[LayerIndex - 1];
                        RenderGroup->Transform.OffsetP = Placement.xyz - CameraOffset;
                        MatchVector.E[Tag_LayerIndex] = (r32)LayerIndex;        
                        bitmap_id LayerImage = GetBestMatchBitmapFrom(Assets, Asset_OpeningCutscene, &MatchVector, &WeightVector);
                        PushBitmap(RenderGroup, LayerImage, Placement.w, V3(0, 0, 0));
                    }
                }





37:00
in the game loop, we just increase dt for the timer.

                handmade.cpp 

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    RenderCutscene(TranState->Assets, RenderGroup, DrawBuffer, GameState->tCutScene);
                    GameState->tCutScene += Input->dtForFrame;
                }



44:49 
so our first layer is the sky background, Casey wants it so that the sky background doesnt move at all.
in real life, the sky is so far away. any amount of human motions, the sky wont move at all. 

notice the z value we give for the first layer is 

                DistanceAboveGround - 200.0f

this way, the layer is placed at some distance away from the camera. It if is just at DistanceAboveGround,
that means it will be sitting on the camera. 

then Casey changes the height of the bitmap till it fills up our entire screen. 



                v4 LayerPlacement[] =
                {
                    {0.0f, 0.0f, DistanceAboveGround - 200.0f, 300.0f}, // NOTE(casey): Sky background
                    {0.0f, 0.0f, -170.0f, 300.0f}, // NOTE(casey): Weird sky light
                    {0.0f, 0.0f, -100.0f, 40.0f}, // NOTE(casey): Backmost row of trees
                    {0.0f, 10.0f, -70.0f, 80.0f}, // NOTE(casey): Middle hills and trees
                    {0.0f, 0.0f, -50.0f, 70.0f}, // NOTE(casey): Front hills and trees 
                    {30.0f, 0.0f, -30.0f, 50.0f}, // NOTE(casey): Right side tree and fence
                    {0.0f, -2.0f, -20.0f, 40.0f}, // NOTE(casey): 7
                    {2.0f, -1.0f, -5.0f, 25.0f}, // NOTE(casey): 8
                };

