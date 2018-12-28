Handmade Hero Day 163 - Asset Processing with STB TrueType

Summary:
integrated the STB TrueType font library into the handmade hero codebase.
demonstrated how to use the library.

praises the simplicity of STB TrueType library

added it to test_asset_builder.cpp, asset processor now loads fonts.

Keyword:
Font, STB TrueType library


1:39
Casey says if he was asked what library he would use if he could use one,
he said the STB libraries.

they are libraries written by Sean Barret, they are designed with game programming in mind.
they are written in a very conveinient way to integrate into your product. 

So Casey will show us how to use STB TrueType into your game.



3:47
so to use STB libraries, you would go to the github page 

https://github.com/nothings/stb

and you can actually see abunch of STB libraries

several of these libraries are pretty darn useful. like STB_image, is a quick way 
to add images. 

most of the time it only reads in the part that the game programmers care about. 


and since we are doing font stuff, we want to look at stb_truetype.h



5:36
so the structure of these libraries is very straightforwad. there is one .h file. 
the way they work, inside the .h file, there is actually a #define that specifies if you want the implementation
or the header. 

so its just two files in one: its both the .h and .cpp warpped up in one. 


if you look inside stb_truetype.h, the #define is STB_TRUETYPE_IMPLEMENTATION


                stb_truetype.h

                #if 0
                #define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
                #include "stb_truetype.h"
                
                ...
                ...                

                #endif


so if you dont define STB_TRUETYPE_IMPLEMENTATION 
it compiles all the .c code away, so its just the .h portion


10:03
Casey showing that if you want to use this file, all you do is 
add the following two lines into your file. 

                #define STB_TRUETYPE_IMPLEMENTATION
                #include "stb_truetype.h"


13:58
Casey mentioned that the .h file also contains all the documentation.
and Casey is following the "Complete program (this compiles): get a single bitmap, print as ASCII art" tutorial 

Casey dmonstarting how to use the library by writing a piece of code to render a single bitmap
 


37:04
Casey getting it to render the 'N' bitmaps in the test code snippet.
but either upside down or left to right.

Casey happens to know and the STB TrueType specified that the bitmaps assumes top bottom.


41:01
Casey moved the test cde to the test_asset_builder.cpp where library code are allowed.



42:28 
Casey moved everything from handmade_asset_type_id.h to handmade_file_formats.h 
and he deleted the handmade_asset_type_id.h file. 



43:13
Casey starting to incorporate font into our asset system. 

first Casey added the Asset_Font in the asset_type_id

                handmade_file_formats.h

                enum asset_type_id
                {
                    Asset_None,

                    Asset_Shadow,
                    Asset_Tree,
                    Asset_Sword,

                    ...
                    ...

                    Asset_Font,     <---------

                    ..
                    ...

                    Asset_Count,
                };





44:11
then in test_asset_builder.cpp, Casey will now add the assets for font

                test_asset_builder.cpp

                internal void
                WriteNonHero(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    BeginAssetType(Assets, Asset_Shadow);
                    AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", 0.5f, 0.156682029f);
                    EndAssetType(Assets);

                    ...
                    ...

                    BeginAssetType(Assets, Asset_Font);     <------------------------
                    for(u32 Character = 'A';
                        Character <= 'Z';
                        ++Character)
                    {
                        AddCharacterAsset(Assets, "c:/Windows/Fonts/arial.ttf", Character);
                        AddTag(Assets, Tag_UnicodeCodepoint, (r32)Character);
                    }
                    EndAssetType(Assets);
                    
                    WriteHHA(Assets, "test2.hha");
                }


45:36
notice in the above code, we are also calling the AddTag(); function.
Casey wants the fonts to also use the tag matching system. So Casey added the Tag_UnicodeCodepoint for the characters. 

                handmade_file_formats.h

                enum asset_tag_id
                {
                    Tag_Smoothness,
                    Tag_Flatness,
                    Tag_FacingDirection, // NOTE(casey): Angle in radians off of due right
                    Tag_UnicodeCodepoint,   <--------------------
                    
                    Tag_Count,
                };


Casey did note that what hes doing now is a horrible idea. He plans to refactor this in the future. 



54:58
Casey writing the AddCharacterAsset(); function. This is essentially exactly the same as AddBitmapAsset();
the only difference is where it gets its loaded bitmap from. 

                test_asset_builder.cpp 

                internal bitmap_id
                AddCharacterAsset(game_assets *Assets, char *FontFile, u32 Codepoint, r32 AlignPercentageX = 0.5f, r32 AlignPercentageY = 0.5f)
                {
                    Assert(Assets->DEBUGAssetType);
                    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

                    bitmap_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
                    asset_source *Source = Assets->AssetSources + Result.Value;
                    hha_asset *HHA = Assets->Assets + Result.Value;
                    HHA->FirstTagIndex = Assets->TagCount;
                    HHA->OnePastLastTagIndex = HHA->FirstTagIndex;
                    HHA->Bitmap.AlignPercentage[0] = AlignPercentageX;
                    HHA->Bitmap.AlignPercentage[1] = AlignPercentageY;

                    Source->Type = AssetType_Font;
                    Source->FileName = FontFile;
                    Source->Codepoint = Codepoint;

                    Assets->AssetIndex = Result.Value;

                    return(Result);
                }




55:32
Then in the WriteHHA(); we add the path for loading GlyphBitmap.
the logic is almost exactly the same as loading bitmaps.

thats why we added the if path for bitmaps 

                if(Source->Type == AssetType_Font)
                {
                    Bitmap = LoadGlyphBitmap(Source->FileName, Source->Codepoint);
                }
                else
                {
                    Assert(Source->Type == AssetType_Bitmap);
                    Bitmap = LoadBMP(Source->FileName);
                }

-   full code below 

                internal void
                WriteHHA(game_assets *Assets, char *FileName)
                {
                    FILE *Out = fopen(FileName, "wb");
                    if(Out)
                    {

                        .................................................................
                        .................... initalizing HHA Header .....................
                        .................................................................

                        for(u32 AssetIndex = 1; AssetIndex < Header.AssetCount; ++AssetIndex)
                        {
                            asset_source *Source = Assets->AssetSources + AssetIndex;
                            hha_asset *Dest = Assets->Assets + AssetIndex;

                            Dest->DataOffset = ftell(Out);

                            if(Source->Type == AssetType_Sound)
                            {
                                ...............................................
                                .................. Load Sound .................
                                ...............................................
                            }
                            else
                            {
                                loaded_bitmap Bitmap;
     --------------------->     if(Source->Type == AssetType_Font)
                                {
                                    Bitmap = LoadGlyphBitmap(Source->FileName, Source->Codepoint);
                                }
                                else
                                {
                                    Assert(Source->Type == AssetType_Bitmap);
                                    Bitmap = LoadBMP(Source->FileName);
                                }

                                Dest->Bitmap.Dim[0] = Bitmap.Width;
                                Dest->Bitmap.Dim[1] = Bitmap.Height;

                                Assert((Bitmap.Width*4) == Bitmap.Pitch);
                                fwrite(Bitmap.Memory, Bitmap.Width*Bitmap.Height*4, 1, Out);
                                
                                free(Bitmap.Free);
                            }
                        }
                        fseek(Out, (u32)Header.Assets, SEEK_SET);
                        fwrite(Assets->Assets, AssetArraySize, 1, Out);
                        
                        fclose(Out);
                    }
                    else
                    {
                        printf("ERROR: Couldn't open file :(\n");
                    }
                }


46:27
As you can see we Casey modifying the test code snippet to load the GlyphBitmap.
most of it is just learning to use the STB font library APIs. nothing special.

-   we get the bitmap data from stb, and we store it in MonoBitmap.
    then we store in loaded_bitmap Result_s memory, Result.Memory.

                test_asset_builder.cpp

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

                    entire_file TTFFile = ReadEntireFile(FileName);
                    if(TTFFile.ContentsSize != 0)
                    {
                        stbtt_fontinfo Font;
                        stbtt_InitFont(&Font, (u8 *)TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8 *)TTFFile.Contents, 0));
                    
                        int Width, Height, XOffset, YOffset;
                        u8 *MonoBitmap = stbtt_GetCodepointBitmap(&Font, 0, stbtt_ScaleForPixelHeight(&Font, 128.0f),
                                                                  Codepoint, &Width, &Height, &XOffset, &YOffset);

                        Result.Width = Width;
                        Result.Height = Height;
                        Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
                        Result.Memory = malloc(Height*Result.Pitch);
                        Result.Free = Result.Memory;

                        u8 *Source = MonoBitmap;
                        u8 *DestRow = (u8 *)Result.Memory + (Height - 1)*Result.Pitch;
                        for(s32 Y = 0; Y < Height; ++Y)
                        {
                            u32 *Dest = (u32 *)DestRow;
                            for(s32 X = 0; X < Width; ++X)
                            {
                                u8 Alpha = *Source++;
                                *Dest++ = ((Alpha << 24) |
                                           (Alpha << 16) |
                                           (Alpha <<  8) |
                                           (Alpha <<  0));
                            }
                        
                            DestRow -= Result.Pitch;
                        }

                        stbtt_FreeBitmap(MonoBitmap, 0);
                        free(TTFFile.Contents);
                    }
                    
                    return(Result);
                }


1:02:37
Casey demonstrating the usage of the tag matching system.
in the the particle system where we were spawning heads, Casey made it so that only letters from "NOTHINGS" can 
be spawned.

                handmade.cpp

                    for(u32 ParticleSpawnIndex = 0;
                        ParticleSpawnIndex < 3;
                        ++ParticleSpawnIndex)
                    {
                        particle *Particle = GameState->Particles + GameState->NextParticle++;
                        if(GameState->NextParticle >= ArrayCount(GameState->Particles))
                        {
                            GameState->NextParticle = 0;
                        }

                        Particle->P = V3(RandomBetween(&GameState->EffectsEntropy, -0.05f, 0.05f), 0, 0);
                        Particle->dP = V3(RandomBetween(&GameState->EffectsEntropy, -0.01f, 0.01f), 7.0f*RandomBetween(&GameState->EffectsEntropy, 0.7f, 1.0f), 0.0f);
                        Particle->ddP = V3(0.0f, -9.8f, 0.0f);
                        Particle->Color = V4(RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                             RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                             RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                             1.0f);
                        Particle->dColor = V4(0, 0, 0, -0.25f);

                        asset_vector MatchVector = {};
                        asset_vector WeightVector = {};
                        char Nothings[] = "NOTHINGS";
                        MatchVector.E[Tag_UnicodeCodepoint] = (r32)Nothings[RandomChoice(&GameState->EffectsEntropy, ArrayCount(Nothings) - 1)];
                        WeightVector.E[Tag_UnicodeCodepoint] = 1.0f;

                        Particle->BitmapID = GetBestMatchBitmapFrom(TranState->Assets, Asset_Font,
                                                                    &MatchVector, &WeightVector);
                        ...
                        ...
                    }

this could be slow, but it does work. What is happening is that the GetBestMatchBitmapFrom(); function will loop through 
all the Asset_Font assets, and pulling one out based on tags matching. 

when the MatchVector.E ='N', the asset 'N' glyph bitmap asset will have the best match, hence returning the 'N' glyph bitmap asset 



1:10:09
Casey mentions that his experience with build programs such as (C make); is only useful becuz libraries and programs 
are constructed poorly. If you write libraries and programs properly, you can build for even for a very large codebase,
by just doing a file build. 

That doesnt necessarily means thats what you want to do, but that implies build tools is extraneous

