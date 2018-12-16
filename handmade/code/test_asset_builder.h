#if !defined(TEST_ASSET_BUILDER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include "handmade_platform.h"
#include "handmade_asset_type_id.h"
#include "handmade_file_formats.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"

struct bitmap_id
{
    uint32 Value;
};

struct sound_id
{
    uint32 Value;
};

enum asset_type
{
    AssetType_Sound,
    AssetType_Bitmap,
};

struct asset_source
{
    asset_type Type;
    char *FileName;
    u32 FirstSampleIndex;
};

#define VERY_LARGE_NUMBER 4096

struct game_assets
{
    u32 TagCount;
    hha_tag Tags[VERY_LARGE_NUMBER];

    u32 AssetTypeCount;
    hha_asset_type AssetTypes[Asset_Count];

    u32 AssetCount;
    asset_source AssetSources[VERY_LARGE_NUMBER];
    hha_asset Assets[VERY_LARGE_NUMBER];

    hha_asset_type *DEBUGAssetType;
    u32 AssetIndex;
};

#define TEST_ASSET_BUILDER_H
#endif
