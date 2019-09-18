/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

/* NOTE(casey):

   1) Everywhere outside the renderer, Y _always_ goes upward, X to the right.

   2) All bitmaps including the render target are assumed to be bottom-up
      (meaning that the first row pointer points to the bottom-most row
       when viewed on screen).

   3) It is mandatory that all inputs to the renderer are in world
      coordinates ("meters"), NOT pixels.  If for some reason something
      absolutely has to be specified in pixels, that will be explicitly
      marked in the API, but this should occur exceedingly sparingly.

   4) Z is a special coordinate because it is broken up into discrete slices,
      and the renderer actually understands these slices.  Z slices are what
      control the _scaling_ of things, whereas Z offsets inside a slice are
      what control Y offsetting.

   5) All color values specified to the renderer as V4's are in
      NON-premulitplied alpha.

*/

struct loaded_bitmap
{
    void *Memory;
    v2 AlignPercentage;
    r32 WidthOverHeight;    
    s32 Width;
    s32 Height;
    // TODO(casey): Get rid of pitch!
    s32 Pitch;
    void *TextureHandle;
};

struct environment_map
{    
    loaded_bitmap LOD[4];
    real32 Pz;
};

enum render_group_entry_type
{
    RenderGroupEntryType_render_entry_clear,
    RenderGroupEntryType_render_entry_bitmap,
    RenderGroupEntryType_render_entry_rectangle,
    RenderGroupEntryType_render_entry_cliprect,
    RenderGroupEntryType_render_entry_coordinate_system,
};
struct render_group_entry_header // TODO(casey): Don't store type here, store in sort index?
{
    u16 Type;
    u16 ClipRectIndex;
    
#if HANDMADE_SLOW
    u32 DebugTag;
#endif
};

struct clip_rect_fx
{
    v4 tColor;
    v4 Color;
};

struct render_entry_cliprect
{
    render_entry_cliprect *Next;
    rectangle2i Rect;
    clip_rect_fx FX;
};

struct render_entry_clear
{
    v4 PremulColor;
};

struct render_entry_saturation
{
    real32 Level;
};

struct render_entry_bitmap
{
    loaded_bitmap *Bitmap;

    v4 PremulColor;
    v2 P;

    // NOTE(casey): X and Y axes are already scaled by the dimension.
    v2 XAxis;
    v2 YAxis;
};

struct render_entry_rectangle
{
    v4 PremulColor;
    v2 P;
    v2 Dim;
};

// NOTE(casey): This is only for test:
// {
struct render_entry_coordinate_system
{
    v2 Origin;
    v2 XAxis;
    v2 YAxis;
    v4 Color;
    loaded_bitmap *Texture;
    loaded_bitmap *NormalMap;

//    real32 PixelsToMeters; // TODO(casey): Need to store this for lighting!

    environment_map *Top;
    environment_map *Middle;
    environment_map *Bottom;
};
// }

struct object_transform
{
    // TODO(casey): Move this out to its own thang
    b32 Upright;
    v3 OffsetP;
    r32 Scale;
    r32 SortBias;
};

struct camera_transform
{
    b32 Orthographic;

    // NOTE(casey): Camera parameters
    r32 MetersToPixels; // NOTE(casey): This translates meters _on the monitor_ into pixels _on the monitor_
    v2 ScreenCenter;

    r32 FocalLength;
    r32 DistanceAboveTarget;
};

struct render_group
{
    struct game_assets *Assets; 

    v4 tGlobalColor;
    v4 GlobalColor;

#if HANDMADE_SLOW
    u32 DebugTag;
#endif
    v2 MonitorHalfDimInMeters;

    camera_transform CameraTransform;

    uint32 MissingResourceCount;
    b32 RendersInBackground;

    u32 CurrentClipRectIndex;

    u32 GenerationID;
    game_render_commands *Commands;
};

struct entity_basis_p_result
{
    v2 P;
    r32 Scale;
    b32 Valid;
    r32 SortKey;
};

struct used_bitmap_dim
{
    entity_basis_p_result Basis;
    v2 Size;
    v2 Align;
    v3 P;
};

void DrawRectangleQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                          loaded_bitmap *Texture, real32 PixelsToMeters,
                          rectangle2i ClipRect);

inline object_transform
DefaultUprightTransform(void)
{
    object_transform Result = {};

    Result.Upright = true;
    Result.Scale = 1.0f;

    return(Result);
}

inline object_transform
DefaultFlatTransform(void)
{
    object_transform Result = {};

    Result.Scale = 1.0f;

    return(Result);
}

struct transient_clip_rect
{
    transient_clip_rect(render_group *RenderGroupInit, u32 NewClipRectIndex)
    {
        RenderGroup = RenderGroupInit;
        OldClipRect = RenderGroup->CurrentClipRectIndex;
        RenderGroup->CurrentClipRectIndex = NewClipRectIndex;
    }
    
    transient_clip_rect(render_group *RenderGroupInit)
    {
        RenderGroup = RenderGroupInit;
        OldClipRect = RenderGroup->CurrentClipRectIndex;
    }

    ~transient_clip_rect(void)
    {
        RenderGroup->CurrentClipRectIndex = OldClipRect;
    }
    
    render_group *RenderGroup;
    u32 OldClipRect;
};
