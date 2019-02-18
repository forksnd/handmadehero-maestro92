#if !defined(HANDMADE_CUTSCENE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

/*
  NOTE(casey):

  - Snow for shot 2 (and 1?)
  - Closed door in shot 1
  - Handedness of glove?
  
 */

enum scene_layer_flags
{
    SceneLayerFlag_AtInfinty = 0x1,
    SceneLayerFlag_CounterCameraX = 0x2,
    SceneLayerFlag_CounterCameraY = 0x4,
    SceneLayerFlag_Transient = 0x8,
    SceneLayerFlag_Floaty = 0x10,
};

struct scene_layer
{
    v3 P;
    r32 Height;
    u32 Flags;
    v2 Param;
};

struct layered_scene
{
    asset_type_id AssetType;
    u32 ShotIndex;
    u32 LayerCount;
    scene_layer *Layers;

    r32 Duration;
    v3 CameraStart;
    v3 CameraEnd;
};

// NOTE(casey): Thank you to Ron Gilbert and friends for
// introducing the word "cutscene" into the game development
// vocabulary.
struct playing_cutscene
{
    u32 SceneCount;
    layered_scene *Scenes;
    r32 t;
};

#define HANDMADE_CUTSCENE_H
#endif
