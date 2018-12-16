#if !defined(HANDMADE_ASSET_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct loaded_sound
{
    uint32 SampleCount; // NOTE(casey): This is the sample count divided by 8
    uint32 ChannelCount;
    int16 *Samples[2];
};

enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    AssetState_Locked,
};
struct asset_slot
{
    asset_state State;
    union
    {
        loaded_bitmap *Bitmap;
        loaded_sound *Sound;
    };
};

struct asset_vector
{
    real32 E[Tag_Count];
};

struct asset_type
{
    uint32 FirstAssetIndex;
    uint32 OnePastLastAssetIndex;
};

struct asset_file
{
    platform_file_handle *Handle;

    // TODO(casey): If we ever do thread stacks, AssetTypeArray
    // doesn't actually need to be kept here probably.
    hha_header Header;
    hha_asset_type *AssetTypeArray;

    u32 TagBase;
};

struct game_assets
{
    // TODO(casey): Not thrilled about this back-pointer
    struct transient_state *TranState;
    memory_arena Arena;

    real32 TagRange[Tag_Count];

    u32 FileCount;
    asset_file *Files;
    
    uint32 TagCount;
    hha_tag *Tags;

    uint32 AssetCount;
    hha_asset *Assets;
    asset_slot *Slots;
    
    asset_type AssetTypes[Asset_Count];

    u8 *HHAContents;

#if 0
    // NOTE(casey): Structured assets
//    hero_bitmaps HeroBitmaps[4];

    // TODO(casey): These should go away once we actually load a asset pack file
    uint32 DEBUGUsedAssetCount;
    uint32 DEBUGUsedTagCount;
    asset_type *DEBUGAssetType;
    asset *DEBUGAsset;
#endif
};

inline loaded_bitmap *GetBitmap(game_assets *Assets, bitmap_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    asset_slot *Slot = Assets->Slots + ID.Value;
    loaded_bitmap *Result = (Slot->State >= AssetState_Loaded) ? Slot->Bitmap : 0;

    return(Result);
}

inline loaded_sound *GetSound(game_assets *Assets, sound_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    asset_slot *Slot = Assets->Slots + ID.Value;
    loaded_sound *Result = (Slot->State >= AssetState_Loaded) ? Slot->Sound : 0;

    return(Result);
}

inline hha_sound *
GetSoundInfo(game_assets *Assets, sound_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    hha_sound *Result = &Assets->Assets[ID.Value].Sound;

    return(Result);
}

inline bool32
IsValid(bitmap_id ID)
{
    bool32 Result = (ID.Value != 0);

    return(Result);
}

inline bool32
IsValid(sound_id ID)
{
    bool32 Result = (ID.Value != 0);

    return(Result);
}

internal void LoadBitmap(game_assets *Assets, bitmap_id ID);
inline void PrefetchBitmap(game_assets *Assets, bitmap_id ID) {LoadBitmap(Assets, ID);}
internal void LoadSound(game_assets *Assets, sound_id ID);
inline void PrefetchSound(game_assets *Assets, sound_id ID) {LoadSound(Assets, ID);}

#define HANDMADE_ASSET_H
#endif
