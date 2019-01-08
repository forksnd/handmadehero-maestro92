Handmade Hero Day 171 - Adding Font Metadata to the Asset Builder

Summary:
worked on writing fonts and font glyphs in the asset packer 

set up to fill in the kerning table

more work on loading in the font and font glyphs in game

Keyword:
kerning, Asset, Font asset,


8:32
more work on asset packer loading font and font glyphs
                
                test_asset_builder.h

                enum asset_type
                {
                    AssetType_Sound,
                    AssetType_Bitmap,
                    AssetType_Font,
                    AssetType_FontGlyph,
                };

                struct loaded_font;
                struct asset_source_font
                {
                    loaded_font *Font;
                };

                struct asset_source_font_glyph
                {
                    loaded_font *Font;
                    u32 Codepoint;
                };

                struct asset_source_bitmap
                {
                    char *FileName;
                };

                struct asset_source_sound
                {
                    char *FileName;
                    u32 FirstSampleIndex;
                };

                struct asset_source
                {
                    asset_type Type;
                    union
                    {
                        asset_source_bitmap Bitmap;
                        asset_source_sound Sound;
                        asset_source_font Font;
                        asset_source_font_glyph Glyph;
                    };
                };



11:50
we added two different functionts to load these two assets 


                internal bitmap_id AddCharacterAsset(game_assets *Assets, loaded_font *Font, u32 Codepoint, r32 AlignPercentageX = 0.5f, r32 AlignPercentageY = 0.5f)
                {
                    added_asset Asset = AddAsset(Assets);
                    Asset.HHA->Bitmap.AlignPercentage[0] = AlignPercentageX;
                    Asset.HHA->Bitmap.AlignPercentage[1] = AlignPercentageY;
                    Asset.Source->Type = AssetType_Font;
                    Asset.Source->Glyph.Font = Font;
                    Asset.Source->Glyph.Codepoint = Codepoint;

                    bitmap_id Result = {Asset.ID};
                    return(Result);
                }


                internal font_id AddFontAsset(game_assets *Assets, loaded_font *Font)
                {
                    added_asset Asset = AddAsset(Assets);
                    Asset.HHA->Font.CodePointCount = Font->CodePointCount;
                    Asset.HHA->Font.LineAdvance = Font->LineAdvance;
                    Asset.Source->Type = AssetType_Font;
                    Asset.Source->Font.Font = Font;

                    font_id Result = {Asset.ID};
                    return(Result);
                }


13:40
previously in our LoadGlyphBitmap(); function, we had all that DeviceContext logic inside that function.


                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint, hha_asset *Asset)
                {

                    static HDC DeviceContext = 0;
                    if(!DeviceContext)
                    {
                        ............................................................
                        ............... intialzing DeviceContext ...................
                        ............................................................                        
                    }

                    ...
                    ...
                }   






15:29 27:42 37:00

now we want to explicitly load a font         

-   notice that in this function, we get the LineAdvance value 

-   we also allocate space the codepoint tables,
    which we call the malloc here

                Result->BitmapIDs = (bitmap_id *)malloc(sizeof(bitmap_id)*CodePointCount);
               
-   also we want to allocate space for our kerning table 
                Result->HorizontalAdvance = (r32 *)malloc(sizeof(r32)*CodePointCount*CodePointCount);

-   full code below:
               
                test_asset_builder.cpp

                internal loaded_font * LoadFont(char *FileName, char *FontName, u32 CodePointCount)
                {
                    loaded_font *Result = (loaded_font *)malloc(sizeof(loaded_font));
                    
                    AddFontResourceExA(FileName, FR_PRIVATE, 0);
                    int Height = 128; // TODO(casey): Figure out how to specify pixels properly here
                    Result->Win32Handle = CreateFontA(Height, 0, 0, 0,
                                                     FW_NORMAL, // NOTE(casey): Weight
                                                     FALSE, // NOTE(casey): Italic
                                                     FALSE, // NOTE(casey): Underline
                                                     FALSE, // NOTE(casey): Strikeout
                                                     DEFAULT_CHARSET, 
                                                     OUT_DEFAULT_PRECIS,
                                                     CLIP_DEFAULT_PRECIS, 
                                                     ANTIALIASED_QUALITY,
                                                     DEFAULT_PITCH|FF_DONTCARE,
                                                     FontName);

                    SelectObject(GlobalFontDeviceContext, Result->Win32Handle);
                    GetTextMetrics(GlobalFontDeviceContext, &Result->TextMetric);

                    Result->LineAdvance = (r32)Result->TextMetric.tmHeight + (r32)Result->TextMetric.tmExternalLeading;
                    Result->CodePointCount = CodePointCount;
                    Result->BitmapIDs = (bitmap_id *)malloc(sizeof(bitmap_id)*CodePointCount);
                    Result->HorizontalAdvance = (r32 *)malloc(sizeof(r32)*CodePointCount*CodePointCount);

                    return(Result);
                }





16:55 
now in our asset_packer code, we load the font and the glyphs separately



                test_asset_builder.cpp

                internal void WriteNonHero(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    ...
                    ...

                    loaded_font *DebugFont = LoadFont("c:/Windows/Fonts/arial.ttf", "Arial", ('~' + 1));
                //        AddCharacterAsset(Assets, "c:/Windows/Fonts/cour.ttf", "Courier New", Character);

                    BeginAssetType(Assets, Asset_Font);
                    AddFontAsset(Assets, DebugFont);
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_FontGlyph);
                    for(u32 Character = '!'; Character <= '~'; ++Character)
                    {
                        DebugFont->BitmapIDs[Character] = AddCharacterAsset(Assets, DebugFont, Character);
                    }
                    EndAssetType(Assets);
                    
                    WriteHHA(Assets, "test2.hha");
                }


22:03
refactored and compressed alot of the "AddAsset" code in test_asset_builder.cpp

previously, we had lots of repeated code in AddBitmapAsset(); and AddCharacterAsset(); 

Casey now added a AddAsset(); function to get rid of the repeated code. 

so now we have 


                internal added_asset
                AddAsset(game_assets *Assets)
                {
                    Assert(Assets->DEBUGAssetType);
                    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

                    u32 Index = Assets->DEBUGAssetType->OnePastLastAssetIndex++;
                    asset_source *Source = Assets->AssetSources + Index;
                    hha_asset *HHA = Assets->Assets + Index;
                    HHA->FirstTagIndex = Assets->TagCount;
                    HHA->OnePastLastTagIndex = HHA->FirstTagIndex;

                    Assets->AssetIndex = Index;

                    added_asset Result;
                    Result.ID = Index;
                    Result.HHA = HHA;
                    Result.Source = Source;
                    return(Result);
                }


and when we get to the specific asset, we write the asset specific work after calling the AddAsset(); function

                internal bitmap_id AddBitmapAsset(game_assets *Assets, char *FileName, r32 AlignPercentageX = 0.5f, r32 AlignPercentageY = 0.5f)
                {
                    added_asset Asset = AddAsset(Assets);
                    Asset.HHA->Bitmap.AlignPercentage[0] = AlignPercentageX;
                    Asset.HHA->Bitmap.AlignPercentage[1] = AlignPercentageY;
                    Asset.Source->Type = AssetType_Bitmap;
                    Asset.Source->Bitmap.FileName = FileName;

                    bitmap_id Result = {Asset.ID};
                    return(Result);
                }

                internal bitmap_id AddCharacterAsset(game_assets *Assets, loaded_font *Font, u32 Codepoint, r32 AlignPercentageX = 0.5f, r32 AlignPercentageY = 0.5f)
                {
                    added_asset Asset = AddAsset(Assets);
                    Asset.HHA->Bitmap.AlignPercentage[0] = AlignPercentageX;
                    Asset.HHA->Bitmap.AlignPercentage[1] = AlignPercentageY;
                    Asset.Source->Type = AssetType_Font;
                    Asset.Source->Glyph.Font = Font;
                    Asset.Source->Glyph.Codepoint = Codepoint;

                    bitmap_id Result = {Asset.ID};
                    return(Result);
                }

                internal sound_id AddSoundAsset(game_assets *Assets, char *FileName, u32 FirstSampleIndex = 0, u32 SampleCount = 0)
                {
                    added_asset Asset = AddAsset(Assets);
                    Asset.HHA->Sound.SampleCount = SampleCount;
                    Asset.HHA->Sound.Chain = HHASoundChain_None;
                    Asset.Source->Type = AssetType_Sound;
                    Asset.Source->Sound.FileName = FileName;
                    Asset.Source->Sound.FirstSampleIndex = FirstSampleIndex;

                    sound_id Result = {Asset.ID};
                    return(Result);
                }

                internal font_id AddFontAsset(game_assets *Assets, loaded_font *Font)
                {
                    added_asset Asset = AddAsset(Assets);
                    Asset.HHA->Font.CodePointCount = Font->CodePointCount;
                    Asset.HHA->Font.LineAdvance = Font->LineAdvance;
                    Asset.Source->Type = AssetType_Font;
                    Asset.Source->Font.Font = Font;

                    font_id Result = {Asset.ID};
                    return(Result);
                }




40:17
when we write out the asset, we also write out the Font asset

                internal void WriteHHA(game_assets *Assets, char *FileName)
                {

                    ...
                    ...

                    else if(Source->Type == AssetType_Font)
                    {
                        loaded_font *Font = Source->Font.Font;
                        u32 HorizontalAdvanceSize = sizeof(r32)*Font->CodePointCount*Font->CodePointCount;
                        u32 CodePointsSize = Font->CodePointCount*sizeof(bitmap_id);
                        fwrite(Font->BitmapIDs, CodePointsSize, 1, Out);
                        fwrite(Font->HorizontalAdvance, HorizontalAdvanceSize, 1, Out);
                    }

                }




43:31
Casey will hack the kerning table in.
we will be extracting the kerning table from windows in the next episode.

-   anytime we load one character glyph, we fill in its entries in the kerning table

                test_asset_builder.cpp

                internal loaded_bitmap
                LoadGlyphBitmap(loaded_font *Font, u32 CodePoint, hha_asset *Asset)
                {

                    ...
                    ...

                    for(u32 OtherCodePointIndex = 0;
                        OtherCodePointIndex < Font->CodePointCount;
                        ++OtherCodePointIndex)
                    {
                        Font->HorizontalAdvance[CodePoint*Font->CodePointCount + OtherCodePointIndex] = (r32)Result.Width;
                    }

                }







1:09:47

for a 1920 x 1080 frame buffer, we have 2073600 elements. (2 million);

what we care about is the bandwidth. 
for the GPU to operate on 2 million elements, it has to get all these 2 million elements from and to memory
and that costs memory bandwidth. Memory bandwidth is the most expensive thing on a graphics card. 
its what determines the performance of a graphics card,

so what ends up happening, if you can reduce memory bandwidth as much as possible, thats like 
getting a graphics card from several years in the future. 

For example,
https://en.wikipedia.org/wiki/List_of_Nvidia_graphics_processing_units

take a GPU from the GeForce 500 series 

GeForce 510         --->        14.4 GB/s   
GeForce GTX 580     --->        192.384 GB/s


then we take one from the GeForce 900 series 

GeForce GTX 980 Ti[ --->        336.5 GB/s      (which is 2 times the amount)

the idea is that, the bandwidth is just not growing that quickly. You just dont have that much bandwidth


And this is the reasoning behind many things.
Why do we have texture compression?
its not actually to fit more texture on the card memory, thats one positive benefit of it. 
the real win is less memory bandwidth pulling on the card 


1:16:57
Casey says he doesnt like to return or pass things by value if they are not less than 32 bytes.
he keeps his struct pretty small if he is passing by value 

