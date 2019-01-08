Handmade Hero Day 170 - Defining Font Metadata

Summary:

talked about what kerning is again

made the distinction between Asset_Font and Asset_FontGlyph in our game engine 

made accessing font Glyph bitmaps a direct table look up instead of through the asset matching algorithm.

started to add support to load fonts as an asset in our game. (previously we were only loading font glyphs as assets in our game)

loading fonts also sets us up for having kerning in the near future

Keyword:
kerning, Asset, Font asset,




8:55
like what Casey mentioned in day 162. Kerning is literrally advancing write position based on the pair of
characters from the table 

-   we add a "PrevCodePoint" variable to remember what the previous code point is

-   instead of advancing by font dimensions (like what we had in day 169);, we have 

                r32 AdvanceX = CharScale*GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);


-   full code below:

                handmade.cpp 

                internal void DEBUGTextLine(char *String)
                {    
                    if(DEBUGRenderGroup)
                    {
                        ...
                        ...


                        u32 PrevCodePoint = 0;  <------------------
                        r32 CharScale = FontScale;
                        v4 Color = V4(1, 1, 1, 1);
                        r32 AtX = LeftEdge;
                        for(char *At = String; *At; )
                        {
                            ...
                            ...

                            u32 CodePoint = *At;
                            r32 AdvanceX = CharScale*GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);
                            AtX += AdvanceX;
                        
                            if(CodePoint != ' ')
                            {
                                bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, CodePoint);
                                hha_bitmap *Info = GetBitmapInfo(RenderGroup->Assets, BitmapID);

                                PushBitmap(RenderGroup, BitmapID, CharScale*(r32)Info->Dim[1], V3(AtX, AtY, 0), Color);
                            }

                            PrevCodePoint = CodePoint;
                        
                            ++At;
                        
                        }

                        AtY -= 1.2f*80.0f*FontScale;
                    
                    }
                }



14:00
the actual info on how much to advance/kerning depends on the font. 
so our asset system kind of need to know about different fonts. 

So Casey wants to be able to get a font from the asset system.


16:08
Casey also considering the asset system to do matching where we put it the characteristics of a font

and the matching can spit out font based on the matching heursitics



17:54 32:08
so Casey added the concept of loaded_font in our asset system 

-   the "bitmap_id *CodePoints;"
    is just a table of our codePoints 

-   the "r32 *HorizontalAdvance"
    HorizontalAdvance is a 2D table. Recall kerning is about pair wise character distances

                handmade_asset.h

                struct loaded_font
                {
                    bitmap_id *CodePoints;
                    r32 *HorizontalAdvance;
                };

                struct asset_memory_header
                {
                    asset_memory_header *Next;
                    asset_memory_header *Prev;
                    
                    u32 AssetIndex;
                    u32 TotalSize;
                    u32 GenerationID;
                    union
                    {
                        loaded_bitmap Bitmap;
                        loaded_sound Sound;
                        loaded_font Font;
                    };
                };



18:09
then casey proceed to change the DEBUGTextLine(); function
now here, we access our font info, and we use the font info to do the kerning and spacing

-   so assuming that our game will load different fonts for different languages, our GetBestMatchFontFrom();
function will spit out the font we want based on the matching formula

                font_id FontID = GetBestMatchFontFrom(RenderGroup->Assets, Asset_Font, &MatchVector, &WeightVector);

                loaded_font *Font = GetFont(RenderGroup->Assets, FontID, RenderGroup->GenerationID);


-   previously we were getting individal character bitmap through the matching algorithm.
    Casey didnt like that cuz its rather slow.
    now it the bitmap will be coming from a direct look up inside the font. so we have 

                bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, CodePoint);

-   now that we have the font, we can offically advance both x and y depending on the font 
                
                r32 AdvanceX = CharScale*GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);
    and

                AtY -= GetLineAdvanceFor(Info, Font)*FontScale;


-   full code below:
                handmade.cpp

                internal void DEBUGTextLine(char *String)
                {    
                    if(DEBUGRenderGroup)
                    {
                        render_group *RenderGroup = DEBUGRenderGroup;
                    
                        asset_vector MatchVector = {};
                        asset_vector WeightVector = {};
                        font_id FontID = GetBestMatchFontFrom(RenderGroup->Assets, Asset_Font,
                                                              &MatchVector, &WeightVector);

                        loaded_font *Font = GetFont(RenderGroup->Assets, FontID, RenderGroup->GenerationID);
                        if(Font)
                        {
                            hha_font *Info = GetFontInfo(RenderGroup->Assets, FontID);
                            
                            u32 PrevCodePoint = 0;
                            r32 CharScale = FontScale;
                            v4 Color = V4(1, 1, 1, 1);
                            r32 AtX = LeftEdge;
                            for(char *At = String; *At; )
                            { 
                                u32 CodePoint = *At;
                                r32 AdvanceX = CharScale*GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);
                                AtX += AdvanceX;
                            
                                if(CodePoint != ' ')
                                {
                                    bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, CodePoint);
                                    hha_bitmap *Info = GetBitmapInfo(RenderGroup->Assets, BitmapID);

                                    PushBitmap(RenderGroup, BitmapID, CharScale*(r32)Info->Dim[1], V3(AtX, AtY, 0), Color);
                                }

                                PrevCodePoint = CodePoint;
                            
                                ++At;                            
                            }

                            AtY -= GetLineAdvanceFor(Info, Font)*FontScale;
                        }
                    }
                }

22:54
Casey added the struct font_id, just like the bitmap_id and sound_id

                handmade_file_formats.h

                struct bitmap_id
                {
                    u32 Value;
                };

                struct sound_id
                {
                    u32 Value;
                };

                struct font_id
                {
                    u32 Value;
                };



as well as the GetFont(); function

                handmade_asset.h

                inline loaded_font* GetFont(game_assets *Assets, font_id ID, u32 GenerationID)
                {
                    asset_memory_header *Header = GetAsset(Assets, ID.Value, GenerationID);

                    loaded_font *Result = Header ? &Header->Font : 0;

                    return(Result);
                }


25:11
Casey writing all the key functions mentioned above

                handmade_asset.cpp

                internal font_id GetBestMatchFontFrom(game_assets *Assets, asset_type_id TypeID, asset_vector *MatchVector, asset_vector *WeightVector)
                {
                    font_id Result = {GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector)};
                    return(Result);
                }


-   recall that the Font->HorizontalAdvance is a 2D table. 

                internal r32 GetHorizontalAdvanceForPair(hha_font *Info, loaded_font *Font, u32 DesiredPrevCodePoint, u32 DesiredCodePoint)
                {
                    u32 PrevCodePoint = GetClampedCodePoint(Info, DesiredPrevCodePoint);
                    u32 CodePoint = GetClampedCodePoint(Info, DesiredCodePoint);
                    
                    r32 Result = Font->HorizontalAdvance[PrevCodePoint*Info->CodePointCount + CodePoint];
                        
                    return(Result);
                }


-   this is just a straight up look up into our table

                internal bitmap_id GetBitmapForGlyph(game_assets *Assets, hha_font *Info, loaded_font *Font, u32 DesiredCodePoint)
                {
                    u32 CodePoint = GetClampedCodePoint(Info, DesiredCodePoint);    
                    bitmap_id Result = Font->CodePoints[CodePoint];
                    
                    return(Result);
                }

                internal r32 GetLineAdvanceFor(hha_font *Info, loaded_font *Font)
                {
                    r32 Result = Info->LineAdvance;

                    return(Result);
                }


38:33
so now, we will work on definiing font metadata in our asset system 

Casey wants to introduce the difference between Asset_Front and Asset_FrontGlyph


                handmade_file_formats

                enum asset_type_id
                {
                    Asset_None,

                    ...
                    ...

                    Asset_Font,
                    Asset_FontGlyph,

                }

one is for actual characters, the other is for Font 
we really wont be using Asset_FontGlyph to look things up cuz we are doing the directly look up with our tables.
but Casey woud like to have it defined in the asset_type_id, so its explicitly clear that Font and Asset_FrontGlyph have
different definitions in our game. 



40:00
now we define the hha_font metadata

                handmade_file_formats.h

                struct hha_font
                {
                    u32 CodePointCount;
                    r32 LineAdvance;
                    /* NOTE(casey): Data is:

                       bitmap_id CodePoints[CodePointCount];
                       r32 HorizontalAdvance[CodePointCount];
                    */
                };


                struct hha_asset
                {
                    u64 DataOffset;
                    u32 FirstTagIndex;
                    u32 OnePastLastTagIndex;
                    union
                    {
                        hha_bitmap Bitmap;
                        hha_sound Sound;
                        hha_font Font;
                    };
                };

46:58
then we work out how do we LoadFonts 
the code in here is pretty straight forward.
most of this is very similar to LoadBitmap(); or LoadSound(); function 

                handmade_asset.cpp

                internal void LoadFont(game_assets *Assets, bitmap_id ID, b32 Immediate)
                {
                    // TODO(casey): Merge all this boilerplate!!!!  Same between LoadBitmap, LoadSound, and LoadFont
                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value)
                    {
                        if(AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) == AssetState_Unloaded)
                        {
                            ...
                            ...

                            if(Immediate || Task)        
                            {
                                asset *Asset = Assets->Assets + ID.Value;
                                hha_font *Info = &Asset->HHA.Font;

                                u32 HorizontalAdvanceSize = sizeof(r32)*Info->CodePointCount*Info->CodePointCount;
                                u32 CodePointsSize = Info->CodePointCount*sizeof(bitmap_id);
                                u32 SizeData = CodePointsSize + HorizontalAdvanceSize;
                                u32 SizeTotal = SizeData + sizeof(asset_memory_header);

                                Asset->Header = AcquireAssetMemory(Assets, SizeTotal, ID.Value);

                                loaded_font *Font = &Asset->Header->Font;            
                                Font->CodePoints = (bitmap_id *)(Asset->Header + 1);
                                Font->HorizontalAdvance = (r32 *)((u8 *)Font->CodePoints + CodePointsSize);

                                load_asset_work Work;
                                Work.Task = Task;
                                Work.Asset = Assets->Assets + ID.Value;
                                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                                Work.Offset = Asset->HHA.DataOffset;
                                Work.Size = SizeData;
                                Work.Destination = Font->CodePoints;
                                Work.FinalState = AssetState_Loaded;            
                                if(Task)
                                {
                                    load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work);
                                    *TaskWork = Work;
                                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                                }
                                else
                                {
                                    LoadAssetWorkDirectly(&Work);
                                }
                            }
                            else
                            {
                                Asset->State = AssetState_Unloaded;
                            }
                        }
                        else if(Immediate)
                        {
                            ...
                            ...
                        }
                    }    
                }


