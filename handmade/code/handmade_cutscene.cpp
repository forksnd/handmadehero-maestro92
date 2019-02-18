/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "handmade_cutscene.h"

internal void
RenderLayeredScene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer,
                   layered_scene *Scene, r32 tNormal)
{
    // TODO(casey): Unify this stuff?
    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
    real32 MetersToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;
    real32 FocalLength = 0.6f;

    v3 CameraStart = Scene->CameraStart;
    v3 CameraEnd = Scene->CameraEnd;
    v3 CameraOffset = Lerp(CameraStart, tNormal, CameraEnd);
    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels, FocalLength, 0.0f);

    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    WeightVector.E[Tag_ShotIndex] = 10.0f;
    WeightVector.E[Tag_LayerIndex] = 1.0f;
    
    MatchVector.E[Tag_ShotIndex] = (r32)Scene->ShotIndex;
        
    for(u32 LayerIndex = 1;
        LayerIndex <= Scene->LayerCount;
        ++LayerIndex)
    {
        scene_layer Layer = Scene->Layers[LayerIndex - 1];
        b32 Active = true;
        if(Layer.Flags & SceneLayerFlag_Transient)
        {
            Active = ((tNormal >= Layer.Param.x) &&
                      (tNormal < Layer.Param.y));
        }

        if(Active)
        {
            v3 P = Layer.P;
            if(Layer.Flags & SceneLayerFlag_AtInfinty)
            {
                P.z += CameraOffset.z;
            }

            if(Layer.Flags & SceneLayerFlag_Floaty)
            {
                P.y += Layer.Param.x*Sin(Layer.Param.y*tNormal);
            }
            
            if(Layer.Flags & SceneLayerFlag_CounterCameraX)
            {
                RenderGroup->Transform.OffsetP.x = P.x + CameraOffset.x;
            }
            else
            {
                RenderGroup->Transform.OffsetP.x = P.x - CameraOffset.x;
            }

            if(Layer.Flags & SceneLayerFlag_CounterCameraY)
            {
                RenderGroup->Transform.OffsetP.y = P.y + CameraOffset.y;
            }
            else
            {
                RenderGroup->Transform.OffsetP.y = P.y - CameraOffset.y;
            }

            RenderGroup->Transform.OffsetP.z = P.z - CameraOffset.z;
        
            MatchVector.E[Tag_LayerIndex] = (r32)LayerIndex;        
            bitmap_id LayerImage = GetBestMatchBitmapFrom(Assets, Scene->AssetType, &MatchVector, &WeightVector);
            PushBitmap(RenderGroup, LayerImage, Layer.Height, V3(0, 0, 0));
        }
    }
}

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

#if 0
    // NOTE(casey): Shot 3
    {
        Scene.AssetType = Asset_OpeningCutscene;
        Scene.ShotIndex = 3;
        Scene.LayerCount = 4;
        scene_layer Layers[] =
        {
            {{0.0f, 0.0f, -30.0f}, 100.0f, SceneLayerFlag_AtInfinty}, // NOTE(casey): Sky
            {{0.0f, 0.0f, -20.0f}, 45.0f, SceneLayerFlag_CounterCameraY}, // NOTE(casey): Wall and window
            {{0.0f, -2.0f, -4.0f}, 15.0f, SceneLayerFlag_CounterCameraY}, // NOTE(casey): Icicles
            {{0.0f, 0.35f, -0.5f}, 1.0f}, // NOTE(casey): Icicles
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.5f, 0.0f};
        Scene.CameraEnd = {0.0f, 6.5f, -1.5f};
    }

    // NOTE(casey): Shot 4
    {
        r32 ShotChangeTime = 0.5f;
        Scene.AssetType = Asset_OpeningCutscene;
        Scene.ShotIndex = 4;
        Scene.LayerCount = 5;
        scene_layer Layers[] =
        {
            {{0.0f, 0.0f, -4.0f}, 6.0f},
            {{-1.2f, -0.2f, -4.0f}, 4.0f, SceneLayerFlag_Transient, {0.0f, ShotChangeTime}},
            {{-1.2f, -0.2f, -4.0f}, 4.0f, SceneLayerFlag_Transient, {ShotChangeTime, 1.0f}},
            {{2.25f, -1.5f, -3.0f}, 2.0f},
            {{0.0f, 0.35f, -1.0f}, 1.0f},
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {0.0f, 0.0f, -0.5f};
    }

    // NOTE(casey): Shot 5
    {
        r32 ShotChangeTime = 0.5f;
        Scene.AssetType = Asset_OpeningCutscene;
        Scene.ShotIndex = 5;
        Scene.LayerCount = 5;
        scene_layer Layers[] =
        {
            {{0.0f, 0.0f, -20.0f}, 30.0f},
            {{0.0f, 0.0f, -5.0f}, 8.0f, SceneLayerFlag_Transient, {0.0f, ShotChangeTime}},
            {{0.0f, 0.0f, -5.0f}, 8.0f, SceneLayerFlag_Transient, {ShotChangeTime, 1.0f}},
            {{0.0f, 0.0f, -3.0f}, 4.0f, SceneLayerFlag_Transient, {ShotChangeTime, 1.0f}},
            {{0.0f, 0.0f, -2.0f}, 3.0f, SceneLayerFlag_Transient, {ShotChangeTime, 1.0f}},
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {0.0f, 0.5f, -1.0f};
    }

    // NOTE(casey): Shot 6
    {
        Scene.AssetType = Asset_OpeningCutscene;
        Scene.ShotIndex = 6;
        Scene.LayerCount = 6;
        scene_layer Layers[] =
        {
            {{0.0f, 0.0f, -8.0f}, 12.0f},
            {{0.0f, 0.0f, -5.0f}, 8.0f},
            {{1.0f, -1.0f, -3.0f}, 3.0f},
            {{0.85f, -0.95f, -3.0f}, 0.5f},
            {{-2.0f, -1.0f, -2.5f}, 2.0f},
            {{0.2f, 0.5f, -1.0f}, 1.0f},
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {-0.5f, 0.5f, -1.0f};
    }

    // NOTE(casey): Shot 7
    {
        Scene.AssetType = Asset_OpeningCutscene;
        Scene.ShotIndex = 7;
        Scene.LayerCount = 2;
        scene_layer Layers[] =
        {
            {{-0.5f, 0.0f, -8.0f}, 12.0f, SceneLayerFlag_CounterCameraX},
            {{-1.0f, 0.0f, -4.0f}, 6.0f},
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {2.0f, 0.0f, 0.0f};
    }

    // NOTE(casey): Shot 8
    {
        Scene.AssetType = Asset_OpeningCutscene;
        Scene.ShotIndex = 8;
        Scene.LayerCount = 4;
        scene_layer Layers[] =
        {
            {{0.0f, 0.0f, -8.0f}, 12.0f},
            {{0.0f, -1.0f, -5.0f}, 4.0f, SceneLayerFlag_Floaty, {0.05f, 15.0f}},
            {{3.0f, -1.5f, -3.0f}, 2.0f},
            {{0.0f, 0.0f, -1.5f}, 2.5f},
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {0.0f, -0.5f, -1.0f};
    }

    // NOTE(casey): Shot 9
    {
        Scene.AssetType = Asset_OpeningCutscene;
        Scene.ShotIndex = 9;
        Scene.LayerCount = 4;
        scene_layer Layers[] =
        {
            {{0.0f, 0.0f, -8.0f}, 12.0f},
            {{0.0f, 0.25f, -3.0f}, 4.0f},
            {{1.0f, 0.0f, -2.0f}, 3.0f},
            {{1.0f, 0.1f, -1.0f}, 2.0f},
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {-0.75f, -0.5f, -1.0f};
    }
#endif
    RenderLayeredScene(Assets, RenderGroup, DrawBuffer, &Scene, tNormal);
}
