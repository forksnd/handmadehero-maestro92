#if !defined(HANDMADE_ASSET_TYPE_ID_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

enum asset_tag_id
{
    Tag_Smoothness,
    Tag_Flatness,
    Tag_FacingDirection, // NOTE(casey): Angle in radians off of due right
    
    Tag_Count,
};

enum asset_type_id
{
    Asset_None,

    //
    // NOTE(casey): Bitmaps!
    //
    
    Asset_Shadow,
    Asset_Tree,
    Asset_Sword,
//    Asset_Stairwell,
    Asset_Rock,

    Asset_Grass,
    Asset_Tuft,
    Asset_Stone,

    Asset_Head,
    Asset_Cape,
    Asset_Torso,

    //
    // NOTE(casey): Sounds!
    //

    Asset_Bloop,
    Asset_Crack,
    Asset_Drop,
    Asset_Glide,
    Asset_Music,
    Asset_Puhp,
    
    //
    //
    //
    
    Asset_Count,
};

#define HANDMADE_ASSET_TYPE_ID_H
#endif
