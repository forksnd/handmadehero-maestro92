Handmade Hero Day 174 - Adding Sparse Unicode Support

Summary:
add glyph and spacing support for the ' ' character.

devised a plan to add unicode support
plans to have two arrays. one compact array that has codepoint-bitmapID key value storage 
the other is 64 k long, which contains unicode to compact array index mapping

Keyword:
font, unicode, asset.



2:42
Casey adding characterAsset for ' '.
this way when we have ' ', it will actually be space

                test_asset_builder.cpp 


                internal void WriteFonts(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    loaded_font *DebugFont = LoadFont("c:/Windows/Fonts/arial.ttf", "Arial");
                //        AddCharacterAsset(Assets, "c:/Windows/Fonts/cour.ttf", "Courier New", Character);

                    BeginAssetType(Assets, Asset_FontGlyph);
                    AddCharacterAsset(Assets, DebugFont, ' ');      <---------
                    for(u32 Character = '!';
                        Character <= '~';
                        ++Character)
                    {
                        AddCharacterAsset(Assets, DebugFont, Character);
                    }

                    ...
                    ...
                }

7:56
Casey realizing that when we deal with the ' ' character glyph, our current LoadGlyphBitmap(); function wont really work for it.

recall in LoadGlyphBitmap(); we are doing the following. we find the min and max pixel that contains content.
but for the ' ' character. This wont really work as the MinX and MaxX wont give you anything valid. 

                test_asset_builder.cpp

                internal loaded_bitmap LoadGlyphBitmap(loaded_font *Font, u32 CodePoint, hha_asset *Asset)
                {
                    ...
                    ...

                    s32 MinX = 10000;
                    s32 MinY = 10000;
                    s32 MaxX = -10000;
                    s32 MaxY = -10000;

                    u32 *Row = (u32 *)GlobalFontBits + (MAX_FONT_HEIGHT - 1)*MAX_FONT_WIDTH;
                    for(s32 Y = 0; Y < BoundHeight; ++Y)
                    {
                        u32 *Pixel = Row;
                        for(s32 X = 0; X < BoundWidth; ++X)
                        {
                #if 0
                            COLORREF RefPixel = GetPixel(GlobalFontDeviceContext, X, Y);
                            Assert(RefPixel == *Pixel);
                #endif
                            if(*Pixel != 0)
                            {
                                if(MinX > X)
                                {
                                    MinX = X;                    
                                }

                                if(MinY > Y)
                                {
                                    MinY = Y;                    
                                }
                                
                                if(MaxX < X)
                                {
                                    MaxX = X;                    
                                }

                                if(MaxY < Y)
                                {
                                    MaxY = Y;                    
                                }
                            }

                            ++Pixel;
                        }
                        Row -= MAX_FONT_WIDTH;
                    }

                    r32 KerningChange = 0;
                    if(MinX <= MaxX)
                    {
                        ...
                        ...



                    }
                }



8:42
Casey just moved the kerning table intialization outside of the "if(MinX <= MaxX)" condition


                    test_asset_builder.cpp

                    internal loaded_bitmap LoadGlyphBitmap(loaded_font *Font, u32 CodePoint, hha_asset *Asset)
                    {
                        ...
                        ...

                        r32 KerningChange = 0;
                        if(MinX <= MaxX)
                        {
                            ...
                            ...
                            KerningChange = (r32)(MinX - PreStepX);
                        }


                        INT ThisWidth;
                        GetCharWidth32W(GlobalFontDeviceContext, CodePoint, CodePoint, &ThisWidth);
                        r32 CharAdvance = (r32)ThisWidth;
                            
                        for(u32 OtherGlyphIndex = 0;
                            OtherGlyphIndex < Font->MaxGlyphCount;
                            ++OtherGlyphIndex)
                        {
                            Font->HorizontalAdvance[GlyphIndex*Font->MaxGlyphCount + OtherGlyphIndex] += CharAdvance - KerningChange;
                            if(OtherGlyphIndex != 0)
                            {
                                Font->HorizontalAdvance[OtherGlyphIndex*Font->MaxGlyphCount + GlyphIndex] += KerningChange;
                            }
                        }
                    }





11:29
Casey now is starting to add support for other language
he started with the word "owl" in japanese 小耳木兎

Casey first check support whether the font in windows does have glyphs for these japanese characters
and it appears that Arial does have glyphs for these for characters

Casey verified it by first choosing an obscure font and the four character glyphs does become unrecognizable characters
and he choose it back to "arial" and there you go, it displays it properly.


12:28
Casey proceed to look up the unicodes for these 

小   0x5C0F
耳   0x8033
木   0x6728
兎   0x514E

the problem here is that the span range is pretty large, so just using a direct look up table is gonna require too much memory.
especially when we are doing a 2D kerning table. that is too much space


16:58
so what do we do?
brute force approach, is just having a flat array. This will work well, if your codepoints are all contiguous

but once we have something like 

小   0x5C0F
耳   0x8033
木   0x6728
兎   0x514E

there are giant gaps between codepoints, our array is no longer compact.


The most naive solution, if we dont care about performance, is to 
store the codepoint and the char as key-value pairs, and we do a linear search of the entire table anytime we want a char 
             _______________
            |   0   |   A   |
            |_______|_______|
            |   1   |   B   |
            |_______|_______|
            |   2   |   C   |
            |_______|_______|
            |   3   |   D   |
            |_______|_______|
            |   4   |   E   |
            |_______|_______|

we may also do a binary search sometimes, althought binary search is unpredicatble cache wise.

19:19
this brute force solution may be the choice if this is a game that allows you to enter text.
cuz if this is a game that allows the player to enter text, we have to be able to convert it into something we recognize.


but for handmade hero, where we create the strings, one option is to pre-process the strings.
so if we have 

小   0x5C0F
耳   0x8033
木   0x6728
兎   0x514E

and lets say we want to map the unicode into compact index

小   0x5C0F  ---->   0
耳   0x8033  ---->   1
木   0x6728  ---->   2
兎   0x514E  ---->   3

then we can go through the mapping table and output the compact index.

for our game, this seems like a better solution, cuz this way, we wont have to do anything expensive at runtime to look up.
and the tables are always maximally tiny.

The cost, you cant generate new strings at run time.

however, even that is not necessarly true.
we could also ship some kind of compressed table


21:33
screw everything what we just said...

Casey proposes a new solution
we will just have one 64k entry table. Which is 128 k 
in there we just have a mapping of the codepoint to glyph index (compact index);

the 64k will be enough to contain all unicode characters

            64 k x 2 = 128 k unicode, bimap_id table
             __________________
            |   0   | bitmapId |
            |_______|__________|
            |   1   | bitmapId |
            |_______|__________| 
            |   2   | bitmapId |
            |_______|__________| 
            |   3   | bitmapId |
            |_______|__________| 
            |   4   | bitmapId |
            |_______|__________| 
                       


25:40 35:00
essentially the plan is to have two tables 
in the asset packer(only);, we allocate one big old table, that is 64k, whatever that is out to the max codepoint

    loaded_font.GlyphIndexFromCodePoint                
                                             loaded_font.Glyphs
    index                                   
                 _______                     ___________________
                |       |                   |       |           |         
                |_______|                   |_______|___________|       
                |       |                   |  65   | BitmapId  |
                |_______|   --------->      |_______|___________|
    A, 65       |  1    |                   |0x5c0f | BitmapId  |
                |_______|                   |_______|___________|
        ...     |       |                   |       |           |
        ...     |_______|                   |_______|___________|
    小, 0x5c0f  |  2    |
                |_______|
                |       |
                |_______|

we will store the index to unicode characters to our compact [codepoint, Glyphs] table



23:34
Casey proceeds to implement this scheme
so in handmade_file_format.h, we added the hha_font_glyph struct 

                handmade_file_format.h

                struct hha_font_glyph
                {
                    u32 UnicodeCodePoint;
                    bitmap_id BitmapID;
                };


and in hha_font, we add that table " hha_font_glyph CodePoints[GlyphCount]; "
it is currently commented out cuz, this will offically done in the next episode. 

                handmade_file_format.h

                struct hha_font
                {
                    u32 GlyphCount;
                    r32 AscenderHeight;
                    r32 DescenderHeight;    
                    r32 ExternalLeading;
                    /* NOTE(casey): Data is:

                       hha_font_glyph CodePoints[GlyphCount];   <----------
                       r32 HorizontalAdvance[GlyphCount][GlyphCount];
                    */
                };


so then whenever we want to use a font, we will load characters in, 
and then we will decompress it into this CodePoints table 
that allows us to do direct look up.





25:40 35:00
so in the loaded_font struct, we add a few varables for our tables 

-   "hha_font_glyph *Glyphs;" is our [codepoint, GlyphId] table 
        
    "u32 MaxGlyphCount";
    "u32 GlyphCount;"        is the size of the tables 
   
-   we also added 

                u32 MinCodePoint;
                u32 MaxCodePoint

    to get a sense of the range that the codepoints are being mapped to 

                test_asset_builder.cpp

                struct loaded_font
                {
                    ...
                    ...

                    hha_font_glyph *Glyphs;
                    r32 *HorizontalAdvance;

                    u32 MinCodePoint;
                    u32 MaxCodePoint;
                    
                    u32 MaxGlyphCount;
                    u32 GlyphCount;

                    u32 *GlyphIndexFromCodePoint;
                };





29:27
so in the AddCharacterAsset(); we start to populate our unicode->glyph table 

-   we first call AddAsset(); and with Asset.ID, that gives us the bitmap Id 

-   we then populate the codepoint->glyph table in the font

                hha_font_glyph *Glyph = Font->Glyphs + GlyphIndex;
                Glyph->UnicodeCodePoint = Codepoint;
                Glyph->BitmapID = Result;

-   we also store the index from the codepoint 
                
                Font->GlyphIndexFromCodePoint[Codepoint] = GlyphIndex;


-   full code below:

                test_asset_builder.cpp

                internal bitmap_id AddCharacterAsset(game_assets *Assets, loaded_font *Font, u32 Codepoint)
                {
                    added_asset Asset = AddAsset(Assets);
                    ...
                    ...
                    bitmap_id Result = {Asset.ID};


                    Assert(Font->GlyphCount < Font->MaxGlyphCount);
                    u32 GlyphIndex = Font->GlyphCount++;
                    hha_font_glyph *Glyph = Font->Glyphs + GlyphIndex;
                    Glyph->UnicodeCodePoint = Codepoint;
                    Glyph->BitmapID = Result;
                    Font->GlyphIndexFromCodePoint[Codepoint] = GlyphIndex;

                    return(Result);
                }




30:15
Casey also fixes the WriteFonts(); function

as you can see, this is essentially passing in the unicode to intialize the CharacterAssets.
the API is quite clean

                test_asset_builder.cpp

                internal void WriteFonts(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    loaded_font *DebugFont = LoadFont("c:/Windows/Fonts/arial.ttf", "Arial");

                    BeginAssetType(Assets, Asset_FontGlyph);
                    AddCharacterAsset(Assets, DebugFont, ' ');
                    for(u32 Character = '!';
                        Character <= '~';
                        ++Character)
                    {
                        AddCharacterAsset(Assets, DebugFont, Character);
                    }

                    // NOTE(casey): Kanji OWL!!!!!!!
                    AddCharacterAsset(Assets, DebugFont, 0x5c0f);
                    AddCharacterAsset(Assets, DebugFont, 0x8033);
                    AddCharacterAsset(Assets, DebugFont, 0x6728);
                    AddCharacterAsset(Assets, DebugFont, 0x514e);

                    EndAssetType(Assets);
                    
                    // TODO(casey): This is kinda janky, because it means you have to get this
                    // order right always!
                    BeginAssetType(Assets, Asset_Font);
                    AddFontAsset(Assets, DebugFont);
                    EndAssetType(Assets);

                    WriteHHA(Assets, "testfonts.hha");
                }





32:34
then in the LoadFont(); we have to initalize our tables 

-   we define the max size of our [codepoint, Glyph] table, which is 

                Font->MaxGlyphCount = 5000;

-   then we allocate memory for the table 

                Font->Glyphs = (hha_font_glyph *)malloc(sizeof(hha_font_glyph)*Font->MaxGlyphCount);

-   also the kerning table 
                size_t HorizontalAdvanceSize = sizeof(r32)*Font->MaxGlyphCount*Font->MaxGlyphCount;
                Font->HorizontalAdvance = (r32 *)malloc(HorizontalAdvanceSize);


-   we also allocate the codepoint -> GlyphIndex table

                u32 GlyphIndexFromCodePointSize = ONE_PAST_MAX_FONT_CODEPOINT*sizeof(loaded_font);
                Font->GlyphIndexFromCodePoint = (u32 *)malloc(GlyphIndexFromCodePointSize);
                memset(Font->GlyphIndexFromCodePoint, 0, GlyphIndexFromCodePointSize);

    at 39:28, we define a #define for ONE_PAST_MAX_FONT_CODEPOINT

                #define ONE_PAST_MAX_FONT_CODEPOINT (0x10FFFF + 1)

    we got the 0x10FFFF after Casey googled "What is the highest unicode codepoint", so thats the number we are using. 


-   full code below

                test_asset_builder.cpp

                internal loaded_font * LoadFont(char *FileName, char *FontName)
                {    
                    loaded_font *Font = (loaded_font *)malloc(sizeof(loaded_font));
                    
                    ..........................................................
                    ............... Creating Font from windows ...............
                    ..........................................................

                    Font->MinCodePoint = INT_MAX;
                    Font->MaxCodePoint = 0;
                    
                    // NOTE(casey): 5k characters should be more than enough for _anybody_! 
                    Font->MaxGlyphCount = 5000;
                    Font->GlyphCount = 0;

                    u32 GlyphIndexFromCodePointSize = ONE_PAST_MAX_FONT_CODEPOINT*sizeof(loaded_font);
                    Font->GlyphIndexFromCodePoint = (u32 *)malloc(GlyphIndexFromCodePointSize);
                    memset(Font->GlyphIndexFromCodePoint, 0, GlyphIndexFromCodePointSize);
                    
                    Font->Glyphs = (hha_font_glyph *)malloc(sizeof(hha_font_glyph)*Font->MaxGlyphCount);
                    size_t HorizontalAdvanceSize = sizeof(r32)*Font->MaxGlyphCount*Font->MaxGlyphCount;
                    Font->HorizontalAdvance = (r32 *)malloc(HorizontalAdvanceSize);
                    memset(Font->HorizontalAdvance, 0, HorizontalAdvanceSize);

                    return(Font);
                }




44:41
Casey had to move the KerningTable intialization out of the LoadFont(); becuz we dont have a mapping.
This has to be moved after all the LoadGlyphBitmap(); has been called

Previously, we had the following code.
the 
                "Font->HorizontalAdvance[Pair->wFirst*Font->CodePointCount + Pair->wSecond]"

is being indexed through unicode 
but we really want the compact index, so we can only initalize our kerning table after 
our GlyphIndexFromCodePoint table is done.
                
                test_asset_builder.cpp

                internal loaded_font *
                LoadFont(char *FileName, char *FontName, u32 CodePointCount)
                {
                    ...
                    ...

                    DWORD KerningPairCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
                    KERNINGPAIR *KerningPairs = (KERNINGPAIR *)malloc(KerningPairCount*sizeof(KERNINGPAIR));
                    GetKerningPairsW(GlobalFontDeviceContext, KerningPairCount, KerningPairs);
                    for(DWORD KerningPairIndex = 0; KerningPairIndex < KerningPairCount; ++KerningPairIndex)
                    {
                        KERNINGPAIR *Pair = KerningPairs + KerningPairIndex;
                        if((Pair->wFirst < Font->CodePointCount) &&
                           (Pair->wSecond < Font->CodePointCount))
                        {
    --------------->        Font->HorizontalAdvance[Pair->wFirst*Font->CodePointCount + Pair->wSecond] += (r32)Pair->iKernAmount;
                        }
                    }

                    free(KerningPairs);

                    return(Font);
                }


now we have the following:

-   essentially, the only change is indexing into our kerning table  

                u32 First = Font->GlyphIndexFromCodePoint[Pair->wFirst];
                u32 Second = Font->GlyphIndexFromCodePoint[Pair->wSecond];
                Font->HorizontalAdvance[First*Font->MaxGlyphCount + Second] += (r32)Pair->iKernAmount;

-   full code below:

                internal void
                FinalizeFontKerning(loaded_font *Font)
                {
                    SelectObject(GlobalFontDeviceContext, Font->Win32Handle);

                    DWORD KerningPairCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
                    KERNINGPAIR *KerningPairs = (KERNINGPAIR *)malloc(KerningPairCount*sizeof(KERNINGPAIR));
                    GetKerningPairsW(GlobalFontDeviceContext, KerningPairCount, KerningPairs);
                    for(DWORD KerningPairIndex = 0; KerningPairIndex < KerningPairCount; ++KerningPairIndex)
                    {
                        KERNINGPAIR *Pair = KerningPairs + KerningPairIndex;
                        if((Pair->wFirst < ONE_PAST_MAX_FONT_CODEPOINT) &&
                           (Pair->wSecond < ONE_PAST_MAX_FONT_CODEPOINT))
                        {
                            u32 First = Font->GlyphIndexFromCodePoint[Pair->wFirst];
                            u32 Second = Font->GlyphIndexFromCodePoint[Pair->wSecond];
                            Font->HorizontalAdvance[First*Font->MaxGlyphCount + Second] += (r32)Pair->iKernAmount;
                        }
                    }    

                    free(KerningPairs);
                }







