Handmade Hero Day 223 - Playing Multiple Cutscenes

Summary:
put in code play the scenes back to back in sequence.

explained why we currently see imperfect transitions between scene switches. it has to do with our asset system cache.
mentioned that we might have to prefetch the bitmaps

Keyword:
cutscene




32:02
Casey defined a ton more cutscenes, now we want to somehow play them back to back.
as you can see, the cutscenes are now very data driven

so Casey put all of the scene_layer into an array

                global_variable scene_layer IntroLayers1[] =
                {
                    {{0.0f, 0.0f, -200.0f}, 300.0f, SceneLayerFlag_AtInfinty}, // NOTE(casey): Sky background
                    {{0.0f, 0.0f, -170.0f}, 300.0f}, // NOTE(casey): Weird sky light
                    {{0.0f, 0.0f, -100.0f}, 40.0f}, // NOTE(casey): Backmost row of trees
                    {{0.0f, 10.0f, -70.0f}, 80.0f}, // NOTE(casey): Middle hills and trees
                    {{0.0f, 0.0f, -50.0f}, 70.0f}, // NOTE(casey): Front hills and trees 
                    {{30.0f, 0.0f, -30.0f}, 50.0f}, // NOTE(casey): Right side tree and fence
                    {{0.0f, -2.0f, -20.0f}, 40.0f}, // NOTE(casey): 7
                    {{2.0f, -1.0f, -5.0f}, 25.0f}, // NOTE(casey): 8
                };

                global_variable scene_layer IntroLayers2[] =
                {
                    {{2.0f, -1.0f, -22.0f}, 32.0f}, // NOTE(casey): Hero and tree
                    {{0.0f, 0.0f, -14.0f}, 22.0f}, // NOTE(casey): Wall and window
                    {{0.0f, 2.0f, -8.0f}, 10.0f}, // NOTE(casey): Icicles
                };

                ...
                ...
                ...

                global_variable scene_layer IntroLayers10[] =
                {
                    {{-15.0f, 25.0f, -100.0f}, 130.0f, SceneLayerFlag_AtInfinty},
                    {{0.0f, 0.0f, -10.0f}, 22.0f},
                    {{-0.8f, -0.2f, -3.0f}, 4.5f},
                    {{0.0f, 0.0f, -2.0f}, 4.5f},
                    {{0.0f, -0.25f, -1.0f}, 1.5f},
                    {{0.2f, 0.2f, -0.5f}, 1.0f},
                };

                global_variable scene_layer IntroLayers11[] =
                {
                    {{0.0f, 0.0f, -100.0f}, 150.0f, SceneLayerFlag_AtInfinty},
                    {{0.0f, 10.0f, -40.0f}, 40.0f},
                    {{0.0f, 3.2f, -20.0f}, 23.0f},
                    {{0.25f, 0.9f, -10.0f}, 13.5f},
                    {{-0.5f, 0.625f, -5.0f}, 7.0f},
                    {{0.0f, 0.1f, -2.5f}, 3.9f},
                    {{-0.3f, -0.15f, -1.0f}, 1.2f},
                };


then also Casey made a macro do defined the layered_scene. 

                #define INTRO_SHOT(Index) Asset_OpeningCutscene, Index, ArrayCount(IntroLayers##Index), IntroLayers##Index
                global_variable layered_scene IntroCutscene[] =
                {
                    {},
                    {INTRO_SHOT(1), 20.0f, {0.0f, 0.0f, 10.0f}, {-4.0f, -2.0f, 5.0f}},
                    {INTRO_SHOT(2), 20.0f, {0.0f, 0.0f, 0.0f}, {0.5f, -0.5f, -1.0f}},
                    {INTRO_SHOT(3), 20.0f, {0.0f, 0.5f, 0.0f}, {0.0f, 6.5f, -1.5f}},
                    {INTRO_SHOT(4), 20.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -0.5f}},
                    {INTRO_SHOT(5), 20.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.5f, -1.0f}},
                    {INTRO_SHOT(6), 20.0f, {0.0f, 0.0f, 0.0f}, {-0.5f, 0.5f, -1.0f}},
                    {INTRO_SHOT(7), 20.0f, {0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}},
                    {INTRO_SHOT(8), 20.0f, {0.0f, 0.0f, 0.0f}, {0.0f, -0.5f, -1.0f}},
                    {INTRO_SHOT(9), 20.0f, {0.0f, 0.0f, 0.0f}, {-0.75f, -0.5f, -1.0f}},
                    {INTRO_SHOT(10), 20.0f, {0.0f, 0.0f, 0.0f}, {-0.1f, 0.05f, -0.5f}},
                    {INTRO_SHOT(11), 20.0f, {0.0f, 0.0f, 0.0f}, {0.6f, 0.5f, -2.0f}},
                };



40:33
now to figure out how to play them in sequence,
we will add another parameter of how long each shot will last

                handmade_cutscene.h

                struct layered_scene
                {
                    asset_type_id AssetType;
                    u32 ShotIndex;
                    u32 LayerCount;
                    scene_layer *Layers;

    ----------->    r32 Duration;
                    v3 CameraStart;
                    v3 CameraEnd;
                };



42:24
for now we dont have many cutscenes, so we will just linearly search and find out where tCutScene fall 

so essentially a brute force approach. Everytime we call RenderCutscene, 
we do a for loop to check where if tCutScene falls within tStart and tEnd.

the PrettyStupid flag is just a boolean to indicate whether we have played a layered_scene.
so if we have found a layered_scene where tCutScene falls under, we set the PrettyStupid to true.

then at the end, if we havent found any scene that tCutScene falls under, meaning we have finished playing all the scenes,
we just set the tCutScene to 0.

we can also use this information later to give the game code a signal that we have finished playing all the scenes, and its time
to render the actual game


                internal void RenderCutscene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, r32 *tCutScene)
                {
                    b32 PrettyStupid = false;
                    
                    r32 tBase = 0.0f;
                    for(u32 ShotIndex = 1; ShotIndex < ArrayCount(IntroCutscene); ++ShotIndex)
                    {
                        layered_scene *Scene = IntroCutscene + ShotIndex;
                        r32 tStart = tBase;
                        r32 tEnd = tStart + Scene->Duration;

                        if((*tCutScene >= tStart) && (*tCutScene < tEnd))
                        {
                            r32 tNormal = Clamp01MapToRange(tStart, *tCutScene, tEnd);
                            RenderLayeredScene(Assets, RenderGroup, DrawBuffer, &IntroCutscene[ShotIndex], tNormal);
                            PrettyStupid = true;
                        }

                        tBase = tEnd;
                    }

                    if(!PrettyStupid)
                    {
                        *tCutScene = 0.0f;
                    }
                }


Q/A
someone mentioned that the transitions from scene to scene isnt prefect. why is that?

that has to do with our asset cache. recall we do a LRU scheme in our asset system. when our scene switch, that is the first time 
that our system getting asked to display those bitmaps. But if the asset system doesnt have it in its cache, it will have to load it 
on the spot. so what you see is that period of paging in the bitmaps