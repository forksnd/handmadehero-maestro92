Handmade Hero Day 173 - Precise Font Alignment

Summary:

made rendering text aligned to the top left corner of the screen.

reversed engineered windows kerning table as much as possible.

digging through horrible windows documentation to find out how to get alignment correct

explained the difference between "spacing" and "kerning" in typography

Keyword:
font, rendering font, kerning table 

3:45
Casey proceeds to polish some of the alignment for text.

Casey first attempts to fix the starting AtY at the very beginning.
This way our top line starts at the right position

                handmade.cpp

                internal void DEBUGReset(game_assets *Assets, u32 Width, u32 Height)
                {
                    asset_vector MatchVector = {};
                    asset_vector WeightVector = {};
                    FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);

                    FontScale = 1.0f;
                    Orthographic(DEBUGRenderGroup, Width, Height, 1.0f);
                    LeftEdge = -0.5f*Width;

                    hha_font *Info = GetFontInfo(Assets, FontID);
                    AtY = FontScale*GetLineAdvanceFor(Info, Font);
                }




4:47
Casey noted that


                AtY = 0.5f*Height - FontScale*GetLineAdvanceFor(Info, Font);

we dont want to rely on the font for getting things line LineAdvance. We dont want to rely on it being a Load time thing.
Recall We only get the Font asset after loading it.
we want it to be a known thing, specifically we can do Resets anytime we want.

so he refactored the function to GetLineAdvanceFor(Info);



7:16
Casey realizes that AtY = 0 will be at the center, so he subtract of of screen height so it is at the top left corner

                handmade.cpp

                internal void DEBUGReset(game_assets *Assets, u32 Width, u32 Height)
                {
                    ...
                    ...
                    AtY = 0.5f*Height - FontScale*GetStartingBaselineY(Info);
                }

9:01
at this point, we are rendering text of different sizes in the same line.
Casey brings up a point where the kerning has to be broken up across two letters.
cuz if the letters are of different sizes, the kerning has to change


if we have a small letter and a big letter


                  #
                #####
                  #
              C   ##  
    
char scale   0.2  1.0

Whose kerning do we use? The kerning value is the same, its just different character scale.
so for the kerning value that we get back, do we interpret it at 0.2 scale or 1.0 scale?

if we interpret at 0.2, it will be to little, the giant 't' will overlap with the tiny c

as a matter of fact, this may be an impossible problem.
For example, if we have 'u' and 'T' below, the regular kerning will have the horizontal bar of T overlap with 'u'


                ttttttttttttttttt 
                        tt 
           uu   uu      tt
           uu   uu      tt
           uuuuuuuu     tt

but if you shrink the scale of 'T', it will have to make way for the horizontal bar of T, hence 


           uu   uu  ttttttttttttttt
           uu   uu        tt
           uuuuuuuu       tt

so Casey decided that we are not allowed to change character scale for adjacent characters 

with the information we get from windows, we dont have a solution 

you can still do it for characters that are spaced apart 
so if you do something like 

            "AAAAA bb"

we can still do that.


12:27
Casey explaining why do we have giant space at the front of the first character of every line.

this is becuz our spacing table is not initialized to know what happens when you have a letter that follows nothing 
(codePoint 0, the 0th glyph);


by default, it will use a generic width. 
we will like to setup our kerning table to say, we dont advance at all if our previous character is the 0th glyph

previously, we have something like this, 
whenever we render the current letter, we update AtX by the AdvanceX depending on the the PrevCodePoint.
everytime you write the first character, PrevCodePoint will be 0. 

so if PrevCodePoint is 0, it will just use the default spacing value, becuz we initalized our kerning table that way.

                handmade.cpp

                internal void DEBUGTextLine(char *String)
                {    
                    if(DEBUGRenderGroup)
                    {
                        ...
                        ...

                        if(Font)
                        {
            
            ------------->  u32 PrevCodePoint = 0;
                            for(char *At = String; *At; )
                            {
                                u32 CodePoint = *At;
            -------------->     r32 AdvanceX = CharScale*GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);
                                AtX += AdvanceX;
                            
                                if(CodePoint != ' ')
                                {
                                    ...
                                    ...

                                    PushBitmap(RenderGroup, BitmapID, CharScale*(r32)Info->Dim[1], V3(AtX, AtY, 0), Color);
                                }

                                PrevCodePoint = CodePoint;
                            
                                ++At;
                            }

                            AtY -= GetLineAdvanceFor(Info, Font)*FontScale;
                        }
                    }
                }

14:04
So Casey modified the kerning table to fix that


15:29
after fixing the kerning table, we still some very tiny spacing.


19:28
Casey making the text to align the top boundary
We fixed the GetLineAdvanceFor(); and we also formalized the GetStartingBaselineY(); function

-   notice that GetStartingBaselineY(); we return Info->AscenderHeight. 

                handmade_asset.cpp

                internal r32 GetLineAdvanceFor(hha_font *Info)
                {
                    r32 Result = Info->AscenderHeight + Info->DescenderHeight + Info->ExternalLeading;

                    return(Result);
                }

                internal r32 GetStartingBaselineY(hha_font *Info)
                {
                    r32 Result = Info->AscenderHeight;

                    return(Result);
                }

For the GetLineAdvanceFor(); consider the graph below:

        -----------------------------------------------------
        |                                                   |
        |                                                   |
        |               ###                                 |
        |               ###                                 |
        |------------#########------------------------------|
        |               ###                                 |
        |               ###                                 |
        |               ###                                 |
        |                ###                                | 
        |------------------###------------------------------|       ---  
        |                                                   |        .
        |                                                   |   DescenderHeight
        |                                                   |        .
        |---------------------------------------------------|       ---
                                                                ExternalLeading
                                                                     .
        |---------------------------------------------------|       ---
        |                                           ##      |        .   
        |                                           ##      |        .
        |               ###                         ##      |    AscenderHeight
        |               ###                         ##      |        .
        |------------#########-----##---------##----##------|        .
        |               ###         ##       ##     ##      |        .
        |               ###          ##     ##      ##      |        .
        |               ###           ##   ##       ##      |        .
        |                ###           ## ##        ##      |        .
        |------------------###----------###---------##------|       ---
        |                               ##                  |
        |                              ##                   |
        |                             ##                    |
        |                                                   |
        -----------------------------------------------------


essentially we are just summing the all three values



22:10
Casey explaining why that even after writing the GetLineAdvanceFor(); and GetStartingBaselineY(); function, 
we still see spacing on the top, and why having the space may be considered "correct"

mainly becuz the fonts itself needs to reserver spacing that would have occupy that space 
for example, in chinese, 声调 would have occupied that space 
lets say the 'o' has the third tone, thats what the space is reserved for 

                 ##    ##
                  ##  ##
                    ## 


                 ########
                ##      ##
                ##      ##
                ##      ##
                 ########

23:56
Casey coming back to the tiny spacing of the left edge. that is becuz we had the ABC spacings 
that is just the abcA spacing 

Since we want to align exactly on the left, what Casey wants to do is to bake the spacing back to the kerning.

so previously our structure of our code is, in our LoadFont(); function, we initalize our KerningTable from windows.

now that we want to get rid of the abcA width and bake it into the KerningTable, what we need to do is to that,
as we iterate each character, we update the KerningTable.
This happens in the LoadGlyphBitmap();

essentially something like this:


                LoadFont();     ------->        intialize the KerningTable with windows

                ...
                ...

                foreach(character)
                {
                    LoadGlyphBitmap();      ------>     make adjustments in the KerningTable
                }



the amount of change we want to make is 

                r32 KerningChange = (r32)(MinX - PreStepX);

that is the abcA spacing. 


recall previously, we had                     

                Asset->Bitmap.AlignPercentage[0] = (1.0f - (MinX - PreStepX)) / (r32)Result.Width;
                Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;


-   now as we iterate the current character, we add the "KerningChange" to the table

                r32 KerningChange = (r32)(MinX - PreStepX);
                for(u32 OtherCodePointIndex = 0; OtherCodePointIndex < Font->CodePointCount; ++OtherCodePointIndex)
                {   
                    Font->HorizontalAdvance[OtherCodePointIndex*Font->CodePointCount + CodePoint] += KerningChange;
                }

-   full code below:

                test_asset_builder.cpp 

                internal loaded_bitmap LoadGlyphBitmap(loaded_font *Font, u32 CodePoint, hha_asset *Asset)
                {
                    ...
                    ...

                    if(MinX <= MaxX)
                    {
                        ...
                        ...

                        r32 KerningChange = (r32)(MinX - PreStepX);
                        for(u32 OtherCodePointIndex = 0;
                            OtherCodePointIndex < Font->CodePointCount;
                            ++OtherCodePointIndex)
                        {   
                            Font->HorizontalAdvance[OtherCodePointIndex*Font->CodePointCount + CodePoint] += KerningChange;
                        }
                        
        ---------->     Asset->Bitmap.AlignPercentage[0] = (1.0f) / (r32)Result.Width;
                        Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;
                    }




32:20
since we are putting the abcA width to the kerning table, we get rid of abcA Width in the default value 


so previously we had 

                r32 W = (r32)This->abcA + (r32)This->abcB + (r32)This->abcC;

now we just have 

                r32 W = (r32)This->abcB + (r32)This->abcC;

                internal loaded_font *
                LoadFont(char *FileName, char *FontName, u32 CodePointCount)
                {

                    ...
                    ...

                    GetCharABCWidthsW(GlobalFontDeviceContext, 0, (Font->CodePointCount - 1), ABCs);
                    for(u32 CodePointIndex = 0;
                        CodePointIndex < Font->CodePointCount;
                        ++CodePointIndex)
                    {
                        ABC *This = ABCs + CodePointIndex;
                        r32 W = (r32)This->abcB + (r32)This->abcC;      <-----------
                        for(u32 OtherCodePointIndex = 0;
                            OtherCodePointIndex < Font->CodePointCount;
                            ++OtherCodePointIndex)
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
                }



33:51
With all these trial and error, our kerning alignment still seems off 



37:41
Casey refactored the code a bit. 
First in our LoadFont(); we no longer initalize the table with the "default" value

we just memset the whole thing with 0 value 

as you can see the first double for loop is gone. 
                
the 2nd double for loop is left unchanged.
                
                test_asset_builder.cpp

                internal loaded_font * LoadFont(char *FileName, char *FontName, u32 CodePointCount)
                {
                    ...
                    ...

                    Font->CodePointCount = CodePointCount;
                    Font->BitmapIDs = (bitmap_id *)malloc(sizeof(bitmap_id)*CodePointCount);
                    size_t HorizontalAdvanceSize = sizeof(r32)*CodePointCount*CodePointCount;
                    Font->HorizontalAdvance = (r32 *)malloc(HorizontalAdvanceSize);
                    memset(Font->HorizontalAdvance, 0, HorizontalAdvanceSize);

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
                }


then inside the LoadGlyphBitmap(); we add "default" the abcWidth
                
                r32 KerningChange = (r32)(ThisABC.abcA + ThisABC.abcB + ThisABC.abcC);

this way, we can look at all the data in one function, instead of spread across both LoadFont(); and LoadGlyphBitmap();

-   full code below:

                test_asset_builder.cpp

                internal loaded_bitmap LoadGlyphBitmap(loaded_font *Font, u32 CodePoint, hha_asset *Asset)
                {

                    ...
                    ...

                    ABC ThisABC;
                    GetCharABCWidthsW(GlobalFontDeviceContext, CodePoint, CodePoint, &ThisABC);

                    for(u32 OtherCodePointIndex = 0;
                        OtherCodePointIndex < Font->CodePointCount;
                        ++OtherCodePointIndex)
                    {
                        r32 KerningChange = (r32)(ThisABC.abcA + ThisABC.abcB + ThisABC.abcC);
                        Font->HorizontalAdvance[CodePoint*Font->CodePointCount + OtherCodePointIndex] += KerningChange;
                    }
                    
                    Asset->Bitmap.AlignPercentage[0] = (1.0f) / (r32)Result.Width;
                    Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;
                }

so the KerningTable value is essentially still 
                
                (r32)(ThisABC.abcA + ThisABC.abcB + ThisABC.abcC) + (r32)Pair->iKernAmount; 
                |                                               |                          |
                |-------- in LoadGlyphBitmap() -----------------|---- in LoadFont ---------|

39:35
yet at this point, the alignment is still off.

45:13
Casey realizes that if we want to keep the characters to be "flushed aglined" (meaning aligned exactly on the left)
but also have the proper abcA width,
then we need to put the abcA width when you render it as the "Next" character. 


Take the example below, we have the abcWidth defined below:


                   ###
                  #####
                 ##   ##
                #########
               ##       ##
              ##         ##
             ##           ##
            ##             ##
         |-|-----------------|-|
         |a|      b          |c|



since we want to render flushed aligned, so we get rid of the abcA width. We want to include the abcA width 
information inside the kerning table

               ###
              #####
             ##   ##
            #########
           ##       ##
          ##         ##
         ##           ##
        ##             ##  
       |-----------------|-|
       |        b        |c|


Then lets say we have a second character "F"

               ###          ############
              #####         ##
             ##   ##        ##
            #########       ##########
           ##       ##      ##
          ##         ##     ## 
         ##           ##    ##
        ##             ##   ##
       |-----------------|-|------------|-|
       |          b      |c|     b      |c|

as you can see, with F, if we dont have the abcA width, it looks to tightly packed. what we really want is 

               ###            ############
              #####           ##
             ##   ##          ##
            #########         ##########
           ##       ##        ##
          ##         ##       ## 
         ##           ##      ##
        ##             ##     ##
       |-----------------|-|-|------------|-|
       |          b      |c|a|     b      |c|


so whenever you render 'F' as the next character, we still want its F.abcA width.
so the full formula is 

        kernTableValue = A.abcB + A.abcC + iKernAmount + F.abcA



-   we do that in our code 
        
                r32 CharAdvance = (r32)(ThisABC.abcB + ThisABC.abcC)
                Font->HorizontalAdvance[CodePoint*Font->CodePointCount + OtherCodePointIndex] += CharAdvance;
                Font->HorizontalAdvance[OtherCodePointIndex*Font->CodePointCount + CodePoint] += KerningChange;


    so when we call LoadGlyphBitmap(); on 'A',
    Font->HorizontalAdvance['A', 'F'] will first have "A.abcB + A.abcC + iKernAmount"

    then when we get to LoadGlyphBitmap(); on 'F'
    Font->HorizontalAdvance['A', 'F'] will first have "A.abcB + A.abcC + iKernAmount + F.abcA" 


recall in the rendering code, we are doing pre-advancement. 

    
                handmade.cpp 

                internal void DEBUGTextLine(char *String)
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
                }


    so when we first render 'A', AdvanceX = 0, AtX is 0 becuz PrevCodePoint = 0
    we render 'A' at position 0.

    then when we get to 'F'
    AdvanceX is A.abcB + A.abcC + iKernAmount + F.abcA
    AtX = A.abcB + A.abcC + iKernAmount + F.abcA

    we finally render 'F' at right position. 


-   see code below:

                test_asset_builder.cpp 

                internal loaded_bitmap LoadGlyphBitmap(loaded_font *Font, u32 CodePoint, hha_asset *Asset)
                {
                    ...
                    ...

                    if(MinX <= MaxX)
                    {
                        ...
                        ...

                        ABC ThisABC;
                        GetCharABCWidthsW(GlobalFontDeviceContext, CodePoint, CodePoint, &ThisABC);

                        r32 KerningChange = (r32)(MinX - PreStepX);
                        for(u32 OtherCodePointIndex = 0;
                            OtherCodePointIndex < Font->CodePointCount;
                            ++OtherCodePointIndex)
                        {
                            r32 CharAdvance = (r32)(ThisABC.abcB + ThisABC.abcC)
                            Font->HorizontalAdvance[CodePoint*Font->CodePointCount + OtherCodePointIndex] += CharAdvance;
                            Font->HorizontalAdvance[OtherCodePointIndex*Font->CodePointCount + CodePoint] += KerningChange;
                        }
                        
                        Asset->Bitmap.AlignPercentage[0] = (1.0f) / (r32)Result.Width;
                        Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;
                    }                    
                }

[I dont understand why KerningChange is not ThisABC.abcA, but MinX - PreStepX]


47:20
with this change, Casey finally got the spacing to look much better.


48:14
Casey made another change 

Casey made the CharAdvance to be 

                r32 CharAdvance = (r32)(ThisABC.abcA + ThisABC.abcB + ThisABC.abcC)

the full abc width, then we subtract the KerningChange.
                
                Font->HorizontalAdvance[CodePoint*Font->CodePointCount + OtherCodePointIndex] += CharAdvance - KerningChange;

-   full code below:

                test_asset_builder.cpp 

                internal loaded_bitmap LoadGlyphBitmap(loaded_font *Font, u32 CodePoint, hha_asset *Asset)
                {
                    ...
                    ...

                    if(MinX <= MaxX)
                    {
                        ...
                        ...

                        ABC ThisABC;
                        GetCharABCWidthsW(GlobalFontDeviceContext, CodePoint, CodePoint, &ThisABC);

                        r32 KerningChange = (r32)(MinX - PreStepX);
                        for(u32 OtherCodePointIndex = 0;
                            OtherCodePointIndex < Font->CodePointCount;
                            ++OtherCodePointIndex)
                        {
                            r32 CharAdvance = (r32)(ThisABC.abcA + ThisABC.abcB + ThisABC.abcC)
                            Font->HorizontalAdvance[CodePoint*Font->CodePointCount + OtherCodePointIndex] += CharAdvance - KerningChange;
                            Font->HorizontalAdvance[OtherCodePointIndex*Font->CodePointCount + CodePoint] += KerningChange;
                        }
                        
                        Asset->Bitmap.AlignPercentage[0] = (1.0f) / (r32)Result.Width;
                        Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;
                    }                    
                }


[again, i am confused about the difference between thisABC.abcA and MinX - PreStepX]


50:54
Casey added a final check for 
                
                if(OtherCodePointIndex != 0)

-   full code below:

                r32 KerningChange = (r32)(MinX - PreStepX);
                for(u32 OtherCodePointIndex = 0;
                    OtherCodePointIndex < Font->CodePointCount;
                    ++OtherCodePointIndex)
                {
                    Font->HorizontalAdvance[CodePoint*Font->CodePointCount + OtherCodePointIndex] += CharAdvance - KerningChange;
                    if(OtherCodePointIndex != 0)
                    {
                        Font->HorizontalAdvance[OtherCodePointIndex*Font->CodePointCount + CodePoint] += KerningChange;
                    }
                }
                
                Asset->Bitmap.AlignPercentage[0] = (1.0f) / (r32)Result.Width;
                Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;


51:38
Casey considering kerning supporting another language

Casey wants to allow support for other people whose native language is not representable by roman alphabet
to be able to translate the words in handmade hero

Casey using the example of 东京


京 codepoint is U+4eac
http://unicode.scarfboy.com/?s=U%2B4eac

4eac in hex is the code, which is 20140, which is really high
recall how we did our kerning, which is a flat array lookup


currently we are only support the ASCII character, which is 256 characters.
so our kerning table size is 256 x 256 x 4 (if you do them as floats) = 262144 in bytes

if you want to support 京, then we need a kerning table of 20140 x 20140 * 4.... Too large 


1:00:08
Fonts are more demanding than other things 
if you think about it, bitmaps usually dont have that much alignment requirements.
regular bitmaps has no concept of baseline, no kerning. Fonts has all of these complexity


1:04:39
someone recommended the msdn uniscribe glossary
https://docs.microsoft.com/en-us/windows/desktop/intl/uniscribe-glossary

Casey says what we really need is the "advance width", which is not explained in the link above.


1:06:31
Casey found another link 
https://docs.microsoft.com/en-us/windows/desktop/gdi/character-widths


which says 
        "The GetCharWidth32 function returns the advance width as an integer value."

so instead of the ABC width, Casey is just gonna use GetCharWidth32();

                {
                #if 0
                        ABC ThisABC;
                        GetCharABCWidthsW(GlobalFontDeviceContext, CodePoint, CodePoint, &ThisABC);
                        r32 CharAdvance = (r32)(ThisABC.abcA + ThisABC.abcB + ThisABC.abcC);
                #else
                        INT ThisWidth;
                        GetCharWidth32W(GlobalFontDeviceContext, CodePoint, CodePoint, &ThisWidth);
                        r32 CharAdvance = (r32)ThisWidth;
                #endif

                        r32 KerningChange = (r32)(MinX - PreStepX);
                        for(u32 OtherCodePointIndex = 0;
                            OtherCodePointIndex < Font->CodePointCount;
                            ++OtherCodePointIndex)
                        {
                            Font->HorizontalAdvance[CodePoint*Font->CodePointCount + OtherCodePointIndex] += CharAdvance - KerningChange;
                            if(OtherCodePointIndex != 0)
                            {
                                Font->HorizontalAdvance[OtherCodePointIndex*Font->CodePointCount + CodePoint] += KerningChange;
                            }
                        }
                        
                        Asset->Bitmap.AlignPercentage[0] = (1.0f) / (r32)Result.Width;
                        Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;
                    }
                }

and it looks a lot better.


1:14:58
how do you render to the GPU

you use a programming API, that the GPU manufacturers ships drivers for, such as OpenGL



1:16:57
Casey explaning the difference between spacing vs kerning in Q/A

in some languages all characters have equal spacing
for example, it doesnt matter what characters are next to each other 

        一二 

all characters will advance the same "character width"

kerning is pair wise 
if you can determine the spacing of the characters without the pairs of characters, you will not use the term "kerning". 


essentially, there are pairwise spacings, and non-pairwise spacings. 
some languages are determined entiresly by one character 

kerning is a 2D thing.
