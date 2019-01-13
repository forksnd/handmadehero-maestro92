Handmade Hero Day 172 - Extracting Kerning Tables from Windows

Summary:

wrote code to load font in game code 
loading font asset dynamically on demand, just like all other assets (bitmap, sound)

extracted kerning table from windows 

loaded LineAdvance from font

fixed some clipping issue when rendering characters

Keyword:
kerning, fonts 



4:04
Casey making a separate WriteFonts() function in the asset packer 


                test_asset_builder.cpp

                internal void WriteFonts(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    loaded_font *DebugFont = LoadFont("c:/Windows/Fonts/arial.ttf", "Arial", ('~' + 1));

                    BeginAssetType(Assets, Asset_Font);
                    AddFontAsset(Assets, DebugFont);
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_FontGlyph);
                    for(u32 Character = '!';
                        Character <= '~';
                        ++Character)
                    {
                        DebugFont->BitmapIDs[Character] = AddCharacterAsset(Assets, DebugFont, Character);
                    }
                    EndAssetType(Assets);

                    WriteHHA(Assets, "testfonts.hha");
                }

and we write it 

                int main(int ArgCount, char **Args)
                {
                    InitializeFontDC();
                    
                    WriteFonts();
                    WriteNonHero();
                    WriteHero();
                    WriteSounds();
                }




14:32
moved the kerning table generation to LoadFont();


                internal loaded_font * LoadFont(char *FileName, char *FontName, u32 CodePointCount)
                {
                    .................................................................
                    ............. Loading and Initailizing the Font .................
                    .................................................................

                    ABC *ABCs = (ABC *)malloc(sizeof(ABC)*Font->CodePointCount);
                    GetCharABCWidthsW(GlobalFontDeviceContext, 0, (Font->CodePointCount - 1), ABCs);
                    for(u32 CodePointIndex = 0; CodePointIndex < Font->CodePointCount; ++CodePointIndex)
                    {
                        ABC *This = ABCs + CodePointIndex;
                        r32 W = (r32)This->abcA + (r32)This->abcB + (r32)This->abcC;
                        for(u32 OtherCodePointIndex = 0; OtherCodePointIndex < Font->CodePointCount; ++OtherCodePointIndex)
                        {
                            Font->HorizontalAdvance[CodePointIndex*Font->CodePointCount + OtherCodePointIndex] = (r32)W;
                        }
                    }
                    free(ABCs);

                    ...
                    ...
                }


23:48
now we call the function to actually load the font asset in run time. 

recall in DEBUGTextLine();, we call GetFont();

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

                        loaded_font *Font = GetFont(RenderGroup, FontID);
                        if(Font)
                        {
                            ...
                            ...
                        }
                    }
                    ...
                    ...
                }



just like when the renderer gets bitmaps, it loads bitmaps on demand, we will want to do the same thing 

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

                        loaded_font *Font = PushFont(RenderGroup, FontID);
                        if(Font)
                        {
                            ...
                            ...
                        }
                    }
                    ...
                    ...
                }



23:53
so we wrote the PushFont(); function

                handmade_render_group.cpp

                inline loaded_font * PushFont(render_group *Group, font_id ID)
                {
                    loaded_font *Font = GetFont(Group->Assets, ID, Group->GenerationID);    
                    if(Font)
                    {
                        // NOTE(casey): Nothing to do
                    }
                    else
                    {
                        Assert(!Group->RendersInBackground);
                        LoadFont(Group->Assets, ID, false);
                        ++Group->MissingResourceCount;
                    }

                    return(Font);
                }

just as a reference, I have included PushBitmap(); here. So you can see the pattern is exactly the same  

                inline void PushBitmap(render_group *Group, bitmap_id ID, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
                {
                    loaded_bitmap *Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
                    if(Group->RendersInBackground && !Bitmap)
                    {
                        LoadBitmap(Group->Assets, ID, true);
                        Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
                    }
                    
                    if(Bitmap)
                    {
                        PushBitmap(Group, Bitmap, Height, Offset, Color);
                    }
                    else
                    {
                        Assert(!Group->RendersInBackground);
                        LoadBitmap(Group->Assets, ID, false);
                        ++Group->MissingResourceCount;
                    }
                }


25:44
we also write the LoadFont(); and PrefetchFont(); function

                handmade_render_group.cpp

                internal void LoadFont(game_assets *Assets, font_id ID, b32 Immediate);
                inline void PrefetchFont(game_assets *Assets, font_id ID) {LoadFont(Assets, ID, false);}



27:13
Casey getting the game to finally render some text

31:11
Casey starting to work on the kerning table from windows.
these are just windows API calls.


-   these three lines loads the kerning table from windows 

                DWORD KerningPairCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
                KERNINGPAIR *KerningPairs = (KERNINGPAIR *)malloc(KerningPairCount*sizeof(KERNINGPAIR));
                GetKerningPairsW(GlobalFontDeviceContext, KerningPairCount, KerningPairs);


    we read the kerning pair and we copy the value to our own kerning table 

-   recall in day 162, the kerning table stores delta values. Every pair gets a "default" value,
    and our kerning table value has a delta value that deviates from that "default" value. 

    so we need to store the default value in our table 

    this is also the reason why we are doing the math with "+=" sign here

                Font->HorizontalAdvance[Pair->wFirst*Font->CodePointCount + Pair->wSecond] += (r32)Pair->iKernAmount;

    according to windows documentation, iKernAmount is usually negative. Cuz kerning usually makes characters pack more tightly.

-   Casey gets the default advance between characters at 36:11
    it uses the GetCharWidth32(); function 

-   full code below 

                test_asset_builder.cpp 

                internal loaded_font * LoadFont(char *FileName, char *FontName, u32 CodePointCount)
                {

                    ...
                    ...

                    for(u32 CodePointIndex = 0; CodePointIndex < Font->CodePointCount; ++CodePointIndex)
                    {
                        for(u32 OtherCodePointIndex = 0; OtherCodePointIndex < Font->CodePointCount; ++OtherCodePointIndex)
                        {
                            Font->HorizontalAdvance[CodePointIndex*Font->CodePointCount + OtherCodePointIndex] = (r32)W;
                        }
                    }
                    free(ABCs);



                    DWORD KerningPairCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
                    KERNINGPAIR *KerningPairs = (KERNINGPAIR *)malloc(KerningPairCount*sizeof(KERNINGPAIR));
                    GetKerningPairsW(GlobalFontDeviceContext, KerningPairCount, KerningPairs);
                    for(DWORD KerningPairIndex = 0;
                        KerningPairIndex < KerningPairCount;
                        ++KerningPairIndex)
                    {
                        KERNINGPAIR *Pair = KerningPairs + KerningPairIndex;
                        if((Pair->wFirst < Font->CodePointCount) &&
                           (Pair->wSecond < Font->CodePointCount))
                        {
                            Font->HorizontalAdvance[Pair->wFirst*Font->CodePointCount + Pair->wSecond] += (r32)Pair->iKernAmount;
                        }
                    }

                    free(KerningPairs);
                }



35:47
now that we have loaded the kerning table from windows, we need to go back to our first double for loop 

we just need to get the default horizontal advance value for each character.

This is done with GetCharABCWidthsW();

the way we use this function is that we Initialize an ABC struct.
Then we call GetCharABCWidthsW(); to populate our ABC variable


the variables ABC struct, to represent them graphically, is 
https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/ns-wingdi-_abc

                typedef struct _ABC {
                  int  abcA;
                  UINT abcB;
                  int  abcC;
                } ABC, *PABC, *NPABC, *LPABC;

abcA: 
The A spacing of the character. 
The A spacing is the distance to add to the current position before drawing the character glyph.

abcB:
The B spacing of the character. 
The B spacing is the width of the drawn portion of the character glyph.

abcC:
The C spacing of the character. 
The C spacing is the distance to add to the current position to provide white space to the right of the character glyph.
                 _______________________
                |                       |
                |                       |
                |                       |
                |    ##############     |
                |          ##           |
                |    |     ##     |     |
                |          ##           |
                |    |     ##     |     |
                |          ##           |
                |    |     ##     |     |                
                |                       |
                |    |            |     |   
                |                       |
                |____|____________|_____|

                abcA     abcB       abcC 


so what we want to is to just add all three together 

-   also note that we call 

                free(ABCs);

    at the end. that frees the memory that we use 


-   the main purpose of this double for loop is main to setup for 2nd double for loop for the kerning table 
            
    lets say when we write 'a'

    for our first loop, we just do 

        a, b = a_default_spacing        b, a = b_default_spacing
        a, c = a_default_spacing        b, c = b_default_spacing
        a, d = a_default_spacing        b, d = b_default_spacing
        a, e = a_default_spacing        b, e = b_default_spacing
        ...                             ...
        ...                             ...


    then we update this table with the kerning table information

        a, b = (a_default_spacing + kern(a,b))                  b, a = (b_default_spacing + kern(b,a)) 
        a, c = (a_default_spacing + kern(a,c))                  b, c = (b_default_spacing + kern(b,c)) 
        a, d = (a_default_spacing + kern(a,d))                  b, d = (b_default_spacing + kern(b,d)) 
        a, e = (a_default_spacing + kern(a,e))                  b, e = (b_default_spacing + kern(b,e)) 
        ...                                                     ...
        ...                                                     ...

-   full code below:

                ABC *ABCs = (ABC *)malloc(sizeof(ABC)*Font->CodePointCount);
                GetCharABCWidthsW(GlobalFontDeviceContext, 0, (Font->CodePointCount - 1), ABCs);
                for(u32 CodePointIndex = 0; CodePointIndex < Font->CodePointCount; ++CodePointIndex)
                {
                    ABC *This = ABCs + CodePointIndex;
                    r32 W = (r32)This->abcA + (r32)This->abcB + (r32)This->abcC;
                    for(u32 OtherCodePointIndex = 0; OtherCodePointIndex < Font->CodePointCount; ++OtherCodePointIndex)
                    {
                        Font->HorizontalAdvance[CodePointIndex*Font->CodePointCount + OtherCodePointIndex] = (r32)W;
                    }
                }
                free(ABCs);

    
                DWORD KerningPairCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
                KERNINGPAIR *KerningPairs = (KERNINGPAIR *)malloc(KerningPairCount*sizeof(KERNINGPAIR));
                GetKerningPairsW(GlobalFontDeviceContext, KerningPairCount, KerningPairs);
                for(DWORD KerningPairIndex = 0;
                    KerningPairIndex < KerningPairCount;
                    ++KerningPairIndex)
                {
                    KERNINGPAIR *Pair = KerningPairs + KerningPairIndex;
                    if((Pair->wFirst < Font->CodePointCount) &&
                       (Pair->wSecond < Font->CodePointCount))
                    {
                        Font->HorizontalAdvance[Pair->wFirst*Font->CodePointCount + Pair->wSecond] += (r32)Pair->iKernAmount;
                    }
                }

                free(KerningPairs);



41:30
Casey wanted to find out more about windows API, so he actually went to look up the Microsoft windows SDK 
to find out about it. 

he looked up the following path on his computer:

C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Include;



46:14
Casey got some text to finally render with somewhat decent spacing, but its not entirely correct at this point
first of all there is a giant spacing at the beginning of the first character

what Casey will do now is that we are currently drawing the bitmap right at the position, but we need to include the
abcA spacing 



currently, recall that we find the bounds of our text, so we just have Asset->Bitmap.AlignPercentage[0] = 0,
we will be drawing at 

                 _______________________
                |                       |
                |                       |
                |                       |
                |    ##############     |
                |          ##           |
                |    |     ##     |     |
                |          ##           |
                |    |     ##     |     |
                |          ##           |
                |    |     ##     |     |                
                |                       |
                |    |            |     |   
                |                       |
                |____|____________|_____|

                abcA     abcB       abcC 

                     ^
                     |
                     |

but we want to have abcA spacing at the front. If you consider it as an x-axis,
abcA starts at -abcA
abcB starts at 0, 



                 _______________________
                |                       |
                |                       |
                |                       |
                |    ##############     |
                |          ##           |
                |    |     ##     |     |
                |          ##           |
                |    |     ##     |     |
                |          ##           |
                |    |     ##     |     |                
                |                       |
                |    |            |     |   
                |                       |
                |____|____________|_____|

                abcA     abcB       abcC 

                ^    ^
                |    |
    |           |    |
    |            
    |         -abcA   0
    ---------------------------------------->

therefore, we need the AlignPercentage to be at -abcA

            Asset->Bitmap.AlignPercentage[0] = (1.0f - ThisABC.abcA) / (r32)Result.Width;

this way it will be drawn at the desired x location



so in the code we have 

                test_asset_builder.cpp

                internal loaded_bitmap LoadGlyphBitmap(loaded_font *Font, u32 CodePoint, hha_asset *Asset)
                {

                    ...
                    ...


    ----------->    Asset->Bitmap.AlignPercentage[0] = (1.0f - ThisABC.abcA) / (r32)Result.Width;
                    Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;
                }




55:52 
Casey voices another concern
right now the abcA defines that spacing in the front of the drawn portion.

                 _______________________
                |                       |
                |                       |
                |                       |
                |    | ########## |     |
                |          ##           |
                |    | |   ##   | |     |
                |          ##           |
                |    | |   ##   | |     |
                |          ##           |
                |    | |   ##   | |     |                
                |                       |
                |    | |        | |     |   
                |                       |
                |____|_|________|_|_____|

                abcA     abcB       abcC 

there is also the possibility that the abcB portion also has spacing. But recall that we are doing 
a bounding area search, so with our current system, when we render it, we wont be rendering the front and back spacing 
within the abcB area



58:10

with that notion inmind, Casey modifed the code by just using MinX. 
MinX is minX that has pixel content.


                Asset->Bitmap.AlignPercentage[0] = (1.0f - MinX) / (r32)Result.Width;
                Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;


59:33 
At this point of video, we can notice that the front leg of 'A' is clipped 

-   recall from day 164, we have this BoundWidth and BoundHeight becuz we want to put a bound on the search area for our 
    1024 x 1024 bitmap.

    and we initially set that area from the size dimensions given from GetTextExtentPoint32W();

-   at 1:16:16 in Q/A, Casey also explains that he suspects windows is drawing the bitmap clipped cuz it doesnt have enough space.
    so what we need to do is to move it over to the middle of the bitmap, so it can draw the left side 

-   So Casey added the "PreStepX" variable to the BoundWidth and BoundHeight 
    and we call TextOutW with PreStepX
                
                TextOutW(GlobalFontDeviceContext, PreStepX, 0, &CheesePoint, 1);

-   at 1:19:14
    we also notice that the 'V' is also clipped 

    initially Casey only had 

                int BoundWidth = Size.cx + PreStepX;

    which only fixed the clipping on 'A'
    Casey make the BoundWidth to be 

                int BoundWidth = Size.cx + 2 * PreStepX;

    and that fixed the clipping on 'V'


-   full code below:

                internal loaded_bitmap LoadGlyphBitmap(loaded_font *Font, u32 CodePoint, hha_asset *Asset)
                {
                    loaded_bitmap Result = {};    

                    ...
                    ...

                    SIZE Size;
                    GetTextExtentPoint32W(GlobalFontDeviceContext, &CheesePoint, 1, &Size);

                    int PreStepX = 128;
                    
                    int BoundWidth = Size.cx + 2*PreStepX;
                    if(BoundWidth > MAX_FONT_WIDTH)
                    {
                        BoundWidth = MAX_FONT_WIDTH;
                    }
                    int BoundHeight = Size.cy;
                    if(BoundHeight > MAX_FONT_HEIGHT)
                    {
                        BoundHeight = MAX_FONT_HEIGHT;
                    }

                    TextOutW(GlobalFontDeviceContext, PreStepX, 0, &CheesePoint, 1);

                    ...
                    ...

                    Asset->Bitmap.AlignPercentage[0] = (1.0f - (MinX - PreStepX)) / (r32)Result.Width;
                    Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;
                }


1:18:18
what also contributed to the clipping bug was setting the bitmap to be full white.
when casey turn it to black, it works again


                memset(GlobalFontBits, 0xFF, MAX_FONT_WIDTH*MAX_FONT_HEIGHT*sizeof(u32));

    
-   now we have 

                memset(GlobalFontBits, 0x00, MAX_FONT_WIDTH*MAX_FONT_HEIGHT*sizeof(u32));
    