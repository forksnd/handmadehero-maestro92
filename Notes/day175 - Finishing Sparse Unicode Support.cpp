Handmade Hero Day 175 - Finishing Sparse Unicode Support

Summary:
added code to load the fonts unicode tables in the game side asset system
made it so that the gameside asset system uses a sparse [unicode, index] table as well

tested rendering of japanese characters, which works

Keyword:
font, unicode, asset.


7:27
Casey mentioned that we want to reserve space for a null glyph

                test_asset_builder.cpp
                                
                internal loaded_font * LoadFont(char *FileName, char *FontName)
                {    
                    loaded_font *Font = (loaded_font *)malloc(sizeof(loaded_font));

                    ........................................................
                    ............... initializing loaded_font ...............
                    ........................................................


                    // NOTE(casey): Reserve space for the null glyph
                    Font->GlyphCount = 1;
                    Font->Glyphs[0].UnicodeCodePoint = 0;
                    Font->Glyphs[0].BitmapID.Value = 0;

                    return font;
                }




15:22
Casey mentioned that the asset packer has ouputed a dense glyph table.
the game code needs to unpack that dense glyph table

the game code also needs a table that maps unicode codepoint to our glyphs

recall from day 174, the asset packer ouputs 

            loaded_font.Glyphs        
             ___________________
            |       |           |         
            |_______|___________|       
            |  65   | BitmapId  |
            |_______|___________|
            |0x5c0f | BitmapId  |
            |_______|___________|
            |       |           |
            |_______|___________|

we need a table that maps unicode code points to indices into this table

16:48
recall in day 174, we googled the largest unicode codepoint being 0x10FFFF.
So if we were to do the same thing in the asset packer, which is to give the game code also a sparse unicode to indices array
that will be 0x10FFFF, 1MB look up table, which is not fabulous


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



17:11
says that we will still start with the brute force 1MB look up table, but we still want to optimize


25:39
so we first add all the table variables to he game side loaded_font struct

                handmade_asset.h

                struct loaded_font
                {
                    hha_font_glyph *Glyphs;
                    r32 *HorizontalAdvance;
                    u32 BitmapIDOffset;
                    u16 *UnicodeMap;        
                };

18:48
then in the game code LoadFont(); function, we need to load the tables
-   first we extract the size of the dense [codepoint, glyph] table 
    as well as the kerning table 

                u32 HorizontalAdvanceSize = sizeof(r32)*Info->GlyphCount*Info->GlyphCount;
                u32 GlyphsSize = Info->GlyphCount*sizeof(hha_font_glyph);

-   we also get define the "UnicodeMapSize" variable

                u32 UnicodeMapSize = sizeof(u16)*Info->OnePastHighestCodepoint;


-   after we have gathered all the sizes of all the data, we allocate memory for all of the tables 

                Asset->Header = AcquireAssetMemory(Assets, SizeTotal, ID.Value);

-   then eventually, we intializing the tables

                Font->Glyphs = (hha_font_glyph *)(Asset->Header + 1);
                Font->HorizontalAdvance = (r32 *)((u8 *)Font->Glyphs + GlyphsSize);
                Font->UnicodeMap = (u16 *)((u8 *)Font->HorizontalAdvance + HorizontalAdvanceSize);

-   full codebelow:

                handmade_asset.cpp

                internal void LoadFont(game_assets *Assets, font_id ID, b32 Immediate)
                {
                    // TODO(casey): Merge all this boilerplate!!!!  Same between LoadBitmap, LoadSound, and LoadFont
                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value)
                    {
                        ...
                        ...

                        asset *Asset = Assets->Assets + ID.Value;
                        hha_font *Info = &Asset->HHA.Font;

                        u32 HorizontalAdvanceSize = sizeof(r32)*Info->GlyphCount*Info->GlyphCount;
                        u32 GlyphsSize = Info->GlyphCount*sizeof(hha_font_glyph);
                        u32 UnicodeMapSize = sizeof(u16)*Info->OnePastHighestCodepoint;
                        u32 SizeData = GlyphsSize + HorizontalAdvanceSize;
                        u32 SizeTotal = SizeData + sizeof(asset_memory_header) + UnicodeMapSize;

                        Asset->Header = AcquireAssetMemory(Assets, SizeTotal, ID.Value);

                        loaded_font *Font = &Asset->Header->Font;
                        Font->BitmapIDOffset = GetFile(Assets, Asset->FileIndex)->FontBitmapIDOffset;
                        Font->Glyphs = (hha_font_glyph *)(Asset->Header + 1);
                        Font->HorizontalAdvance = (r32 *)((u8 *)Font->Glyphs + GlyphsSize);
                        Font->UnicodeMap = (u16 *)((u8 *)Font->HorizontalAdvance + HorizontalAdvanceSize);

                        ZeroSize(UnicodeMapSize, Font->UnicodeMap);

                        ...
                        ...

                    }    
                }




26:20
Then Casey added code to build the UnicodeMap after the asset font gets loaded

So Casey added the concept of FinalizeOperation after Loading assets in the asset system

                handmade_asset.cpp

                enum finalize_asset_operation
                {
                    FinalizeAsset_None,
                    FinalizeAsset_Font,
                };
                struct load_asset_work
                {
                    ...
                    ...

                    finalize_asset_operation FinalizeOperation;
                    u32 FinalState;
                };


then in the LoadAssetWorkDirectly(); function, we add this switch statement.
Here we build our UnicodeMap for the font

essentially we are just building the left table


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


-   full code below:

                handmade_asset.cpp

                internal void LoadAssetWorkDirectly(load_asset_work *Work)
                {
                    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
                    if(PlatformNoFileErrors(Work->Handle))
                    {
                        switch(Work->FinalizeOperation)
                        {
                            case FinalizeAsset_None:
                            {
                                // NOTE(casey): Nothing to do.
                            } break;

                            case FinalizeAsset_Font:
                            {
                                loaded_font *Font = &Work->Asset->Header->Font;
                                hha_font *HHA = &Work->Asset->HHA.Font;
                                for(u32 GlyphIndex = 1;
                                    GlyphIndex < HHA->GlyphCount;
                                    ++GlyphIndex)
                                {
                                    hha_font_glyph *Glyph = Font->Glyphs + GlyphIndex;

                                    Assert(Glyph->UnicodeCodePoint < HHA->OnePastHighestCodepoint);
                                    Assert((u32)(u16)GlyphIndex == GlyphIndex);
                                    Font->UnicodeMap[Glyph->UnicodeCodePoint] = (u16)GlyphIndex;
                                }
                            } break;
                        }
                    }

                    ...
                    ...
                }



32:49
Casey added the ZeroArray #define

                handmade.h

                #define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
    -------->   #define ZeroArray(Count, Pointer) ZeroSize(Count*sizeof((Pointer)[0]), Pointer)
                inline void
                ZeroSize(memory_index Size, void *Ptr)
                {
                    // TODO(casey): Check this guy for performance
                    uint8 *Byte = (uint8 *)Ptr;
                    while(Size--)
                    {
                        *Byte++ = 0;
                    }
                }




35:37
then Casey fixed up all the function that accesses the codepoints and glyphs 

                handmade_asset.cpp

                inline u32 GetGlyphFromCodePoint(hha_font *Info, loaded_font *Font, u32 CodePoint)
                {
                    u32 Result = 0;
                    if(CodePoint < Info->OnePastHighestCodepoint)
                    {
                        Result = Font->UnicodeMap[CodePoint];
                        Assert(Result < Info->GlyphCount);
                    }

                    return(Result);
                }

                internal r32 GetHorizontalAdvanceForPair(hha_font *Info, loaded_font *Font, u32 DesiredPrevCodePoint, u32 DesiredCodePoint)
                {
                    u32 PrevGlyph = GetGlyphFromCodePoint(Info, Font, DesiredPrevCodePoint);
                    u32 Glyph = GetGlyphFromCodePoint(Info, Font, DesiredCodePoint);
                    
                    r32 Result = Font->HorizontalAdvance[PrevGlyph*Info->GlyphCount + Glyph];
                        
                    return(Result);
                }

                internal bitmap_id GetBitmapForGlyph(game_assets *Assets, hha_font *Info, loaded_font *Font, u32 DesiredCodePoint)
                {
                    u32 Glyph = GetGlyphFromCodePoint(Info, Font, DesiredCodePoint);    
                    bitmap_id Result = Font->Glyphs[Glyph].BitmapID;
                    Result.Value += Font->BitmapIDOffset;
                    
                    return(Result);
                }



43:34
after getting the text to render after the unicode tables refactor,
Casey will now test some japanese characters, 

                handmade.cpp

                internal void OverlayCycleCounters(game_memory *Memory)
                {
                    char *NameTable[] =
                    {
                        "GameUpdateAndRender",
                        "RenderGroupToOutput",
                        "DrawRectangleSlowly",
                        "ProcessPixel",
                        "DrawRectangleQuickly",
                    };
    ----------->    DEBUGTextLine("\\5C0F\\8033\\6728\\514E");  
                    DEBUGTextLine("111111");
                    DEBUGTextLine("999999");

                    ...
                }

51:09
Surely enought, we got the 小耳木兎 to render!
however, it is not kerned properly



1:12:20
someone in the Q/A Asked
Why do you use the GlyphIndexFromCodePoint table instead of an array mostly null pointers to achieve sparseness?

Thats becuz the array of pointers would have been 4 times larger.
a pointer is 8 bytes
a 16 bit index is 2 bytes, 
when you are taking about a table that has 38000 entries, that saves a lot of space.

