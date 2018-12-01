Handmade Hero Day 136 - Tag-based Asset Retrieval
Summary:

working on the Tag system in the asset system.

while working on hero bitmap facing direction, Casey mentioned the wrap around issue in comparing angles
Casey mentioned that that is becuz some of the Tag parameters are periodic, not monotonic
periodic meaning, certain for certain paramaters, their values dont grow infinitly. 
For example angles goes from 0 to 360, then it wraps around.

Casey mentions that we will have to deal with periodic parameters in our tag system

Keyword:
asset system



2:37
what we would like to be able to do is to have the engine talk in a general way about what it is looking for 
regarding a particular asset.

For example, the engine could request a hero sprite facing a certain direction.
the asset system goes ahead and find the sprite that most closely match that direction

this gives us huge flexiblity and it decouples game logic code with asset system code.

you can add assets afterwards and not having to do any additional work in the game logic to make the game 
a little more rich.

in the above example, where the engine submits a request for a hero sprite and gives a certain direction.
you can add more facing sprites and the asset system will automatically output the best one.


it also helps development, becuz for example, even if the actual art isnt ready, you can write the api of the asset 
system to test the functionality. Then afterwards, when the art does become ready, you dont have to change any code,
and your game will have the udpated art.




5:56
so Casey mentioned once again the purpose of asset_type and asset_tag

asset_type is gonna be the big categorizer. 
if the engine wants a tree, it will get a tree.
if the engine wants a hero, it will get a hero.

asset_tag will be like properties.
kind of the 2nd degree identifier.
for example: maybe the gender of a hero, or surface of the tree.
these will be represented by asset_tag.





9:04
Casey starting to tackle how to add structured assets into our asset system 
currently the one example of a structured asset is the hero bitmap

                struct hero_bitmaps
                {
                    loaded_bitmap Head;
                    loaded_bitmap Cape;
                    loaded_bitmap Torso;                      
                };

There are two ways:
1.  the engine doest know what makes up a hero. The engine asks for a hero, then the asset system
tells the engine what parts does a hero_bitmaps has (head, cape and torso); 

2.  the engine knows the parts, and asks for the asset system for individual pieces. 

Casey says for this game, we will have the game engine decide individual parts. 





11:30

Casey reasoned that instead of making the Head a tag under the Asset_Hero type, he will just make it a separate
Asset_Head type.

that is becuz Asset_Head is not something that can be specified as something else. For example, if I request a head, but
you give me the next closest thing, a torso. That wouldnt be acceptable.

so giving me a head when I ask for a head is a hard constraint. So Casey made it as a separate Asset_Head type.


                enum asset_type_id
                {
                    Asset_None,
                    
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
                    
                    Asset_Count,
                };





12:47
for facing direction, that is a soft constraint, so Casey will make that as a tag.
for example, if I ask for a direction, you can give me a direction that is somewhat in-correct.



13:55
Casey will now try to start to implement the tag system. the goal is, after we have our tag system,
we can get rid of the hero_bitmaps struct. That way we dont have any special case, and we can just use 
the tag system to deal with all structured bitmap situations 

as a first iteration, Casey will attempt the "wide vectors" system for tags.

a wide vector is just gonna be a giant set of real32s

                struct asset_vector
                {
                    real32 E[Tag_Count];
                };









Following the discussion we did on day 134

-   apart from the bitmaps, sounds and assets array, it also has the Tags array.

                struct game_assets
                {
                    ...
                    ...

                    uint32 TagCount;
                    asset_tag *Tags;

                    ...
                    ...
                };


the tags array is just a global list of all the possible tags. 

                struct asset_tag
                {
                    uint32 ID; // NOTE(casey): Tag ID
                    real32 Value;
                };


the ID will be one of the asset_tag_id, and we will have a value for it. 

                enum asset_tag_id
                {
                    Tag_Smoothness,
                    Tag_Flatness,
                    Tag_FacingDirection, // NOTE(casey): Angle in radians off of due right
                    
                    Tag_Count,
                };



graphically, the tag system will look like below:
following the graph we draw on day 134
it will look like below:

[Note, Obviously Tag_Height, Tag_Weight, Tag_Gender are tags that I made up ]


    asset_type AssetTypes[Asset_Count];                         
                                            asset *Assets;                  asset_tag              
                 ___________                                    
                | Asset_    |               hero0   -------------------->   Tag_FacingDirection: up  
                |  hero     |               hero1   -----------             Tag_Height: 180
                |___________|               hero2              \            Tag_Weight: 155 lbs
                | Asset_    |               hero3               \           Tag_Gender: male
                |  Shadow   |               shadow0              ------>    Tag_FacingDirection: left
                |___________|               shadow1                         Tag_Height: 180
                | Asset_    |  --------->   shadow2                         Tag_Weight: 155 lbs
                |  Tree     |               shadow3                         Tag_Gender: male
                |___________|               shadow4                         ...
                |           |               tree0   -------------------->   Tag_Color: green                          
                |  ...      |               tree1                           Tag_Type: oak
                |___________|               tree2   -------------------->   Tag_Color: yellow             
                |           |               tree3                           Tag_Type: oak
                |  ...      |               tree4
                |___________|               tree5
                |           |               ...
                |           |               ...
                |___________|



recall that in the asset class, we also have FirstTagIndex and OnePastLastTagIndex

                struct asset
                {
                    uint32 FirstTagIndex;
                    uint32 OnePastLastTagIndex;
                    uint32 SlotID;
                };

SlotID points to the array in bitmap, so we dont care that right here.
FirstTagIndex and OnePastLastTagIndex will point into the "asset_tag *Tags;" array

for instance 

                hero0.FirstTagIndex = 0
                hero0.OnePastLastTagIndex = 4

                hero0.FirstTagIndex = 4
                hero0.OnePastLastTagIndex = 8

and so on. 






16:25
Casey wrote the code that will use this tag system. 

-   as you can see, we first go through the entire family of assets under the TypeID.

-   then we go through each asset member

    for each member, we compare our MatchVector with the asset member_s tag vector


                internal bitmap_id
                BestMatchAsset(game_assets *Assets, asset_type_id TypeID,
                               asset_vector *MatchVector, asset_vector *WeightVector)
                {
                    bitmap_id Result = {};

                    real32 BestDiff = Real32Maximum;
                    asset_type *Type = Assets->AssetTypes + TypeID;
                    for(uint32 AssetIndex = Type->FirstAssetIndex; AssetIndex < Type->OnePastLastAssetIndex; ++AssetIndex)
                    {
                        asset *Asset = Assets->Assets + AssetIndex;

                        real32 TotalWeightedDiff = 0.0f;
                        for(uint32 TagIndex = Asset->FirstTagIndex; TagIndex < Asset->OnePastLastTagIndex; ++TagIndex)
                        {
                            asset_tag *Tag = Assets->Tags + TagIndex;
                            real32 Difference = MatchVector->E[Tag->ID] - Tag->Value;
                            real32 Weighted = WeightVector->E[Tag->ID]*AbsoluteValue(Difference);
                            TotalWeightedDiff += Weighted;
                        }

                        if(BestDiff > TotalWeightedDiff)
                        {
                            BestDiff = TotalWeightedDiff;
                            Result.Value = Asset->SlotID;
                        }
                    }

                    return(Result);
                }


-   you might have noticed that with this tag system, it is crucial for the assets to have all the 
properties/tags defined, otherwise the MatchVector wont line up its tags properly






28:54
-   Casey proceeds to write the add Tags on some of the assets.
-   Note that later on, all of these will be built in the asset preprocessing step

                handmade_assets.cpp

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {

                    ...
                    ...

                    BeginAssetType(Assets, Asset_Head);
                    AddBitmapAsset(Assets, "test/test_hero_right_head.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleRight);
                    AddBitmapAsset(Assets, "test/test_hero_back_head.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleBack);
                    AddBitmapAsset(Assets, "test/test_hero_left_head.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleLeft);
                    AddBitmapAsset(Assets, "test/test_hero_front_head.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleFront);
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Cape);
                    AddBitmapAsset(Assets, "test/test_hero_right_cape.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleRight);
                    AddBitmapAsset(Assets, "test/test_hero_back_cape.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleBack);
                    AddBitmapAsset(Assets, "test/test_hero_left_cape.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleLeft);
                    AddBitmapAsset(Assets, "test/test_hero_front_cape.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleFront);
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Torso);
                    AddBitmapAsset(Assets, "test/test_hero_right_torso.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleRight);
                    AddBitmapAsset(Assets, "test/test_hero_back_torso.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleBack);
                    AddBitmapAsset(Assets, "test/test_hero_left_torso.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleLeft);
                    AddBitmapAsset(Assets, "test/test_hero_front_torso.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleFront);
                    EndAssetType(Assets);

                    ...
                    ...

                }




31:00
the AddTag is literally adding it to the "Asset->Tags" array.
also notice we increment the "++Assets->DEBUGAsset->OnePastLastTagIndex;" 

                internal void
                AddTag(game_assets *Assets, asset_tag_id ID, real32 Value)
                {
                    Assert(Assets->DEBUGAsset);

                    ++Assets->DEBUGAsset->OnePastLastTagIndex;
                    asset_tag *Tag = Assets->Tags + Assets->DEBUGUsedTagCount++;

                    Tag->ID = ID;
                    Tag->Value = Value;
                }




[like what I previously mentioned, one thing I see with this tag system is that, since we are doing this MatchVector thing, 
    all assets needs to have all the same tags in the same order. 

    and if we want to add an additional tag/property, lets say tag_eye_color for all hero_bitmaps,
    we will have to add it to all the hero bitmaps, and that could potentially be slightly annoying]

for instance, we were to do it in code 
                {
                    BeginAssetType(Assets, Asset_Head);
                    AddBitmapAsset(Assets, "test/test_hero_right_head.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleRight);
                    AddTag(Assets, Tag_EyeColor, Black);

                    AddBitmapAsset(Assets, "test/test_hero_back_head.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleBack);
                    AddTag(Assets, Tag_EyeColor, Black);

                    AddBitmapAsset(Assets, "test/test_hero_left_head.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleLeft);
                    AddTag(Assets, Tag_EyeColor, Black);

                    AddBitmapAsset(Assets, "test/test_hero_front_head.bmp");
                    AddTag(Assets, Tag_FacingDirection, AngleFront);
                    AddTag(Assets, Tag_EyeColor, Black);

                    EndAssetType(Assets);
                }







23:09
using the tag system, Casey refactored the code where we find the hero sprites.

                handmade.cpp

                hero_bitmap_ids HeroBitmaps = {};
                asset_vector MatchVector = {};
                MatchVector.E[Tag_FacingDirection] = Entity->FacingDirection;
                asset_vector WeightVector = {};
                WeightVector.E[Tag_FacingDirection] = 1.0f;
                HeroBitmaps.Head = BestMatchAsset(TranState->Assets, Asset_Head, &MatchVector, &WeightVector);
                HeroBitmaps.Cape = BestMatchAsset(TranState->Assets, Asset_Cape, &MatchVector, &WeightVector);
                HeroBitmaps.Torso = BestMatchAsset(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector);


                ...
                ...

                switch(Entity->Type)
                {
                    case EntityType_Hero:
                    {
                        // TODO(casey): Z!!!
                        real32 HeroSizeC = 2.5f;
                        PushBitmap(RenderGroup, GetFirstBitmapID(TranState->Assets, Asset_Shadow), HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                        PushBitmap(RenderGroup, HeroBitmaps.Torso, HeroSizeC*1.2f, V3(0, 0, 0));
                        PushBitmap(RenderGroup, HeroBitmaps.Cape, HeroSizeC*1.2f, V3(0, 0, 0));
                        PushBitmap(RenderGroup, HeroBitmaps.Head, HeroSizeC*1.2f, V3(0, 0, 0));
                        DrawHitpoints(Entity, RenderGroup);
                    } break;

                    ...
                    ...
                }


the hero_bitmap_ids is just a struct that Casey defined

                handmade.h

                struct hero_bitmap_ids
                {
                    bitmap_id Head;
                    bitmap_id Cape;
                    bitmap_id Torso;
                };



38:20
Casey talking about character facing directions

assume you have sprite facing 0, 80 and 160 in our asset system

so if you have a sprite facing 80 degrees, and you request for one that is 75 degrees,
you will get the 80 degree one just fine. 


so if you have a sprite facing 125 degrees, and you request for one that is 160 degrees,
you will get the 160 degree one just fine. 


if you are requesting for 300 degrees, and you want to get the 0 degree one 
there is the wrap around issue 

if you just use straight up math, you will get the 160 degree one, but what we really want is the 0 degree one


                    #################  0
                    ###          
                       ######          
                            ###### 300


Casey describes that this angle parameter is periodic, not monotonic.
periodic means the angle does not keep increasing, it wraps around.


what we need to do here is that, for every two angles that you are comparing
if they are 180 degrees apart, we need to add 360 to the smaller one.

example:
we are comparing 10 and 300

adding 360 to 10

so we get 370 and 300

in our Tag system, Casey mentioned that we will have to handle
 non-periodic tags matching and periodic tags matching later on.




42:00
Casey actually changing the entity FacingDirection to actual angles.
Previously we had integers representing directions.

                internal void
                MoveEntity(game_state *GameState, sim_region *SimRegion, sim_entity *Entity, real32 dt,
                           move_spec *MoveSpec, v3 ddP)
                {
                    ...
                    ...
                                        // TODO(casey): Change to using the acceleration vector
                    if((Entity->dP.x == 0.0f) && (Entity->dP.y == 0.0f))
                    {
                        // NOTE(casey): Leave FacingDirection whatever it was
                    }
                    else if(AbsoluteValue(Entity->dP.x) > AbsoluteValue(Entity->dP.y))
                    {
                        if(Entity->dP.x > 0)
                        {
                            Entity->FacingDirection = 0;
                        }
                        else
                        {
                            Entity->FacingDirection = 2;
                        }
                    }
                    else
                    {
                        if(Entity->dP.y > 0)
                        {
                            Entity->FacingDirection = 1;
                        }
                        else
                        {
                            Entity->FacingDirection = 3;
                        }
                    }
                }


to make things more flexible, we change it to actual angles

-   ATan2 return a value from [-180, 180] instead of [0, 360]

                internal void
                MoveEntity(game_state *GameState, sim_region *SimRegion, sim_entity *Entity, real32 dt,
                           move_spec *MoveSpec, v3 ddP)
                {
                    ...
                    ...

                    // TODO(casey): Change to using the acceleration vector
                    if((Entity->dP.x == 0.0f) && (Entity->dP.y == 0.0f))
                    {
                        // NOTE(casey): Leave FacingDirection whatever it was
                    }
                    else
                    {
                        Entity->FacingDirection = ATan2(Entity->dP.y, Entity->dP.x);
                    }
                }





in the asset system, we populate the sprites the proper angles. 
Tau is two two times Pi. 
Pi is a half circle, 2 * Pi is a full circle, 
which is why we are defining AngleRight = 0.0f*Tau32;, AngleBack = 0.25f*Tau32;
these are just fractions of a circle

apparently Tau equaling 2 * pi is standard in math... totaly didnt know that

                handmade_platform.h

                #define Pi32 3.14159265359f
                #define Tau32 6.28318530717958647692f

                handmade_assets.cpp

                real32 AngleRight = 0.0f*Tau32;
                real32 AngleBack = 0.25f*Tau32;
                real32 AngleLeft = 0.5f*Tau32;
                real32 AngleFront = 0.75f*Tau32;
                
                BeginAssetType(Assets, Asset_Head);
                AddBitmapAsset(Assets, "test/test_hero_right_head.bmp");
                AddTag(Assets, Tag_FacingDirection, AngleRight);
                AddBitmapAsset(Assets, "test/test_hero_back_head.bmp");
                AddTag(Assets, Tag_FacingDirection, AngleBack);
                AddBitmapAsset(Assets, "test/test_hero_left_head.bmp");
                AddTag(Assets, Tag_FacingDirection, AngleLeft);
                AddBitmapAsset(Assets, "test/test_hero_front_head.bmp");
                AddTag(Assets, Tag_FacingDirection, AngleFront);
                EndAssetType(Assets);




50:17
Note that we dont properly handle the wrap around issue in this episode. Casey will properly fix it in 
tomorrows episode.
As a sneek peek, he did Casey did 

                internal void
                MoveEntity(game_state *GameState, sim_region *SimRegion, sim_entity *Entity, real32 dt,
                           move_spec *MoveSpec, v3 ddP)
                {
                    ...
                    ...

                    // TODO(casey): Change to using the acceleration vector
                    if((Entity->dP.x == 0.0f) && (Entity->dP.y == 0.0f))
                    {
                        // NOTE(casey): Leave FacingDirection whatever it was
                    }
                    else
                    {
                        Entity->FacingDirection = ATan2(Entity->dP.y, Entity->dP.x);
                        if(Entity->FacingDirection < 0.0)
                        {
                            Entity->FacingDirection += 2.0f * Pi32;
                        }
                    }
                }





53:00
someone mentioned in the Q/A. Can all Tag matching be periodic, and we make the period very large if we dont want them to wrap?

Casey likes the idea, will have a final decision in tomorrows episode.



1:04:09
briefly mentioned the difference between atan and ATan2

(just like my atan vs atan2 article)
atan2 is a much more powerful and useful function in computer graphics, becuz it considers the sign of both y and x