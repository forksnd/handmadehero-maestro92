Handmade Hero Day 222 - Laying Out Cutscenes

Summary:
defined the layered_scene struct and the scene_layer struct
defined all the camera movement and depth configurations for every layer of every scene

Keyword:
cutscene

4:33
starting to clean up the code from day 221 a bit 


defined the layered_scene struct and the scene_layer struct

notice that it contains an array of scene_layer.

                handmade_cutscene.h

                struct layered_scene
                {
                    asset_type_id AssetType;
                    u32 ShotIndex;
                    u32 LayerCount;
                    scene_layer *Layers;

                    v3 CameraStart;
                    v3 CameraEnd;
                };

then in the RenderCutScene(); function, we just cleaned up the code by defining a scene and rendering it

                internal void
                RenderCutscene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer,
                               r32 tCutScene)
                {
                    r32 tStart = 0.0f;
                    r32 tEnd = 20.0f;

                    r32 tNormal = Clamp01MapToRange(tStart, tCutScene, tEnd);
                    
                    layered_scene Scene;

                    // NOTE(casey): Shot 1
                    {
                        Scene.AssetType = Asset_OpeningCutscene;
                        Scene.ShotIndex = 1;
                        Scene.LayerCount = 8;
                        scene_layer Layers[] =
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
                        Scene.Layers = Layers;
                        Scene.CameraStart = {0.0f, 0.0f, 10.0f};
                        Scene.CameraEnd = {-4.0f, -2.0f, 5.0f};
                    }


                    ...
                    ...
                    RenderLayeredScene(Assets, RenderGroup, DrawBuffer, &Scene, tNormal);
                }



8:29
So Casey wants to give the notion that certain layer in our scene is special.
for example, our skyline background scene is special since its positioned at infinity.

so Casey created the scene_layer struct. 

                handmade_cutscene.cpp

                struct scene_layer
                {
                    v3 P;
                    r32 Height;
                    u32 Flags;
                    v2 Param;
                };


as you can see in the code above, we defining an array of scene_layer for a layered_scene


                scene_layer Layers[] =
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
                Scene.Layers = Layers;


9:19
Casey proceeds to add the scene_layer_flags enum


                enum scene_layer_flags
                {
                    SceneLayerFlag_AtInfinty = 0x1,
                    SceneLayerFlag_CounterCameraX = 0x2,
                    SceneLayerFlag_CounterCameraY = 0x4,
                    SceneLayerFlag_Transient = 0x8,
                    SceneLayerFlag_Floaty = 0x10,
                };



13:40
then Casey starts to defining all the scenes manually:

                handmade_cutscene.cpp

                internal void RenderCutscene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, r32 tCutScene)
                {
                    r32 tStart = 0.0f;
                    r32 tEnd = 20.0f;

                    r32 tNormal = Clamp01MapToRange(tStart, tCutScene, tEnd);
                    
                    layered_scene Scene;

                    // NOTE(casey): Shot 1
                    {
                        Scene.AssetType = Asset_OpeningCutscene;
                        Scene.ShotIndex = 1;
                        Scene.LayerCount = 8;
                        scene_layer Layers[] =
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
                        Scene.Layers = Layers;
                        Scene.CameraStart = {0.0f, 0.0f, 10.0f};
                        Scene.CameraEnd = {-4.0f, -2.0f, 5.0f};
                    }

                    // NOTE(casey): Shot 2
                    {
                        Scene.AssetType = Asset_OpeningCutscene;
                        Scene.ShotIndex = 2;
                        Scene.LayerCount = 3;
                        scene_layer Layers[] =
                        {
                            {{2.0f, -1.0f, -22.0f}, 30.0f}, // NOTE(casey): Hero and tree
                            {{0.0f, 0.0f, -14.0f}, 22.0f}, // NOTE(casey): Wall and window
                            {{0.0f, 2.0f, -8.0f}, 10.0f}, // NOTE(casey): Icicles
                        };
                        Scene.Layers = Layers;
                        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
                        Scene.CameraEnd = {0.5f, -0.5f, -1.0f};
                    }

                    ...
                    ...
                }




51:41 
Casey added the concept of a scene_layer_flags, SceneLayerFlag_Transient
which will make scene_layer appear only for a certain time.

in the scene_laye struct, it contains the "v2 Param" parameter, which will define the time that a certain scene_layer will exist for 


                struct scene_layer
                {
                    v3 P;
                    r32 Height;
                    u32 Flags;
                    v2 Param;
                };



53:28
in the RenderLayeredScene(); notice we check if a certain SceneLayerFlag_Transient layer becomes active 
by checking time 


                Active = ((tNormal >= Layer.Param.x) &&
                          (tNormal < Layer.Param.y));

it is intential that we are doing a >=, then a <


this is useful when we do the shot at 56:33 (check video at that timestamp);
where the demon opens the door

-   full code below:
                
                handmade_cutscene.cpp

                internal void RenderLayeredScene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer,
                                   layered_scene *Scene, r32 tNormal)
                {
                    // TODO(casey): Unify this stuff?
                    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
                    real32 MetersToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;
                    real32 FocalLength = 0.6f;

                    ...
                    ...
                        
                    for(u32 LayerIndex = 1; LayerIndex <= Scene->LayerCount; ++LayerIndex)
                    {
                        scene_layer Layer = Scene->Layers[LayerIndex - 1];
                        b32 Active = true;
                        if(Layer.Flags & SceneLayerFlag_Transient)
                        {
                            Active = ((tNormal >= Layer.Param.x) &&
                                      (tNormal < Layer.Param.y));
                        }

                    }

                    ...
                    ...
                }



1:18:50
Casye also added a SceneLayerFlag_Floaty flag 


                if(Layer.Flags & SceneLayerFlag_Floaty)
                {
                    P.y += Layer.Param.x*Sin(Layer.Param.y*tNormal);
                }
                
which adjusts its y position in a sine wave trajectory.                
