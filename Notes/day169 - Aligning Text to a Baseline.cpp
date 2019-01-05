Handmade Hero Day 169 - Aligning Text to a Baseline

Summary:
made all the font rendering respect their natual character size.
made all the font rendering aligned properly with its baseline

Keyword:
graphics, font



4:58
the reason why fonts size are not rendered properly is becuz, we are literally have 

                r32 CharScale = FontScale;

where FontScale is set to 20.

                internal void DEBUGTextLine(char *String)
                {    
                    if(DEBUGRenderGroup)
                    {
                        render_group *RenderGroup = DEBUGRenderGroup;
                    
                        asset_vector MatchVector = {};
                        asset_vector WeightVector = {};
                        WeightVector.E[Tag_UnicodeCodepoint] = 1.0f;

                        r32 CharScale = FontScale;
                        v4 Color = V4(1, 1, 1, 1);
                        r32 AtX = LeftEdge;
                        for(char *At = String; *At; )
                        {           
                            if(*At != ' ')
                            {
                                MatchVector.E[Tag_UnicodeCodepoint] = *At;
                                // TODO(casey): This is too slow for text, at the moment!
                                bitmap_id BitmapID = GetBestMatchBitmapFrom(RenderGroup->Assets, Asset_Font, &MatchVector, &WeightVector);
                                PushBitmap(RenderGroup, BitmapID, CharScale, V3(AtX, AtY, 0), Color);
                            }
                            AtX += CharScale;

                            ++At;
                        
                        }

                        AtY -= 1.2f*FontScale;
                    }
                }

being able to set scale is good in general in our renderer. but for fonts we want them to be a special case.
we want the fonts to be proportional to the scale that they were.


6:36
the dimension format of the fonts asset is actually stored in hha_bitmap struct
which is in hha_asset

                handmade_asset.h

                struct hha_bitmap
                {
                    u32 Dim[2];     <--------------
                    r32 AlignPercentage[2];
                };

                struct hha_asset
                {
                    u64 DataOffset;
                    u32 FirstTagIndex;
                    u32 OnePastLastTagIndex;
                    union
                    {
                        hha_bitmap Bitmap;      <----------
                        hha_sound Sound;
                    };
                };

so all we have to do is to use that dimension in DEBUGTextLine();


7:30
added the GetBitmapInfo(); function

                handmade_asset.h

                inline hha_bitmap* GetBitmapInfo(game_assets *Assets, bitmap_id ID)
                {
                    Assert(ID.Value <= Assets->AssetCount);
                    hha_bitmap *Result = &Assets->Assets[ID.Value].HHA.Bitmap;

                    return(Result);
                }



7:50
now in DEBUGTextLine(); we get our dimension from the bitmap info.

-   notice what we did is that, we have 

                CharScale*(r32)(Info->Dim[0] + 2);

and 
                CharScale*(r32)Info->Dim[1]

so Info->Dim is whatever the dimensions of the font naturally should have been.
then, we multiple it with CharScale.

-   we advance by the size of the character 
                
                CharDim = CharScale*(r32)(Info->Dim[0] + 2);

                ...
                ...

            AtX += CharDim;

-   full code below:

                internal void DEBUGTextLine(char *String)
                {    
                    if(DEBUGRenderGroup)
                    {
                        render_group *RenderGroup = DEBUGRenderGroup;
                    
                        asset_vector MatchVector = {};
                        asset_vector WeightVector = {};
                        WeightVector.E[Tag_UnicodeCodepoint] = 1.0f;

                        r32 CharScale = FontScale;
                        v4 Color = V4(1, 1, 1, 1);
                        r32 AtX = LeftEdge;
                        for(char *At = String; *At; )
                        {

                            ...
                            ...

                            r32 CharDim = CharScale*10.0f;
                            if(*At != ' ')
                            {
                                MatchVector.E[Tag_UnicodeCodepoint] = *At;
                                // TODO(casey): This is too slow for text, at the moment!
                                bitmap_id BitmapID = GetBestMatchBitmapFrom(RenderGroup->Assets, Asset_Font, &MatchVector, &WeightVector);
           ------------->       hha_bitmap *Info = GetBitmapInfo(RenderGroup->Assets, BitmapID);

                                CharDim = CharScale*(r32)(Info->Dim[0] + 2);
                                PushBitmap(RenderGroup, BitmapID, CharScale*(r32)Info->Dim[1], V3(AtX, AtY, 0), Color);
                            }
                            AtX += CharDim;

                            ++At;
                        }

                        AtY -= 1.2f*80.0f*FontScale;
                    }
                }




11:27
now the characters are at right size relative to each other. The Capital letters are taller than the lower case letters


11:41
the other problem we have at this point is that the alignment we have for our characters is at their center.
so you can see the alignment is of characters is off.

most visibly, is that all chacaters are aligned at a center horizontal line. 



13:04
Casey pointed out another problem, which is we are advancing the write position in correctly for each character .

so currently, we are advancing the write position of the current size of the character 

for example, assume we have character 'T', 'i' and 'r' 


                #################
                        #                #
                        #                               
                        #                #              # ######
                        #                #              ##    ###
                        #                #              #
                        #                #              #
                        #                #              #

                |                |     |   |
                |------ w -------|     |w2 | 

since we are aligned at the center, so our write position for each character is calculated from the center
of each text. 

                #################
                        #                #
                        #
                        #                #   # ######
                        #                #   ##    ###
                        #                #   #
                        #                #   #
                        #                #   #

                        |                |
                        |------ w -------|

                                         |   |
                                         |w2-|

and you can see that the distance between 'T' and 'i' is awfully far away while, 'i' and 'r' is much closer.    

essentially, we are advancing the write position incorrectly. 



15:50
the first thing we want to do is to start aligning the text properly

for vertically alignment, recall in day 162, we introduced the concept of "baseline"


        -----------------------------------------------------
                                                    ##
                                                    ##   
                        ###                         ##
                        ###                         ##
        -------------#########-----##---------##----##-------
                        ###         ##       ##     ##
                        ###          ##     ##      ##
                        ###           ##   ##       ##
                         ###           ## ##        ##
        -------------------###----------###---------##-------   <--- baseline
                                        ##
                                       ##
                                      ##

        -----------------------------------------------------


this is the line that most characters "sits" on




17:20
Casey proposes that we want our y alignment of our fonts to be placed on where it sits on the baseline

so taking the example above, the 'C' point is where we want the y alignment to be 

        -----------------------------------------------------
                                                    ##
                                                    ##   
                        ###                         ##
                        ###                         ##
        -------------#########-----##---------##----##-------
                        ###         ##       ##     ##
                        ###          ##     ##      ##
                        ###           ##   ##       ##
                         ###           ## ##        ##
        -------------------#C#----------#C#---------#C-------   <--- baseline
                                        ## 
                                       ##
                                      ##

        -----------------------------------------------------


18:52
currently in our code, we actually have the flexibility to set the alignment already.

recall in our hha_bitmap struct, we have the AlignPercentage struct. 


                handmade_asset.h

                struct hha_bitmap
                {
                    u32 Dim[2];     
                    r32 AlignPercentage[2];     <--------
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
                    };
                };


currently we dont set that in our asset_generator, we just leave it as the default (0.5, 0.5);

Casey proceeds to set it properly in the asset generator 



20:46
Casey does mention that since we have both the truetype library path and the windows path
we will have to maintain both paths

22:11
for the windows path, Casey looked up the specs on text metrics
https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/ns-wingdi-tagtextmetrica

as you can see, the TEXTMETRICA structure has both the "tmAscent" and "tmDescent" filed


        ___________
            |
            |
            |   ascent
            |
            |
            |
            |_______
     height |
            |   descent
            |
            |
        ____|_______

so to figure where the baseline is, we can just have ascent/height if we start from the top
or descent / height if we start from the bottom. so really (1 - descent / height);
meaning this is over specified.

both "tmAscent" and "tmDescent" in pixels.



23:33
Casey starting to get the TextMetric in the LoadGlyphBitmap(); function

-   Casey noted that if we extracting multiple fonts, our static variables wont work 

                static VOID *Bits = 0;
                static HDC DeviceContext = 0;
                static TEXTMETRIC TextMetric;   

-   at the end, we set the Asset->Bitmap.AlignPercentage.
    setting the AlignPercentage y is a bit tricky. As you can see, we are using the descent

    so windows give us a giant bitmap. 

                 _______________________
                |                       |
                |                       |
                |                       |
                |    ##############     |
                |          ##           |
                |          ##           |
                |          ##           |
                |          ##           |
                |          ##           |
           -----|----------##-----------|------ baseline
                |                       |
                |                       |   tmDescent
                |                       |
                |_______________________|


but recall, we do a font extraction. Recall that we do have a one pixel apron
                     ________________ 
                    | ############## |
                    |       ##       |
                    |       ##       |
                    |       ##       |
                    |       ##       |
                    |       ##       |
                    |       ##       |
                    |________________|


so the thing to note that the tmDescent is in the space of the original giant bitmap, not our own extraction region

so for Asset->Bitmap.AlignPercentage[1]; we cant just do 1/tmDescent 
that will set the baseline to be 

                     ________________ 
                    | ############## |
                    |       ##       |
                    |       ##       |
              ------|-------##-------|-----------   baseline
                    |       ##       |
                    |       ##       |
                    |       ##       |
                    |________________|


whereas we really want the baseline to be at 

                     ________________ 
                    | ############## |
                    |       ##       |
                    |       ##       |
                    |       ##       |
                    |       ##       |
                    |       ##       |
                ----|-------##-------|-----------   baseline
                    |________________|

so we have to set the baseline with respect to our own extraction region


the math we do is below:

                Asset->Bitmap.AlignPercentage[1] = (1.0f + (TextMetric.tmDescent - MinY)) / (r32)Result.Height;

1.0 is for the apron of the extraction region 


                 _______________________
                |                       |
                |                       |
     maxY  -----|-----------------------|
                |    ##############     |
                |          ##           |
                |          ##           |
                |          ##           |
                |          ##           |
                |          ##           |
                |----------##-----------|----------------------- 
     minY  -----|-----------------------|                descent   
                |                       |
                |_______________________|----------------------


(TextMetric.tmDescent - MinY) is to calculate the offset of baseline in our extraction region. For characters such as 
'a', 'T', ones that literally sit on the base line, this may be 0.





-   full code below:
                internal loaded_bitmap
                LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint, hha_asset *Asset)
                {
                    loaded_bitmap Result = {};    

                    int MaxWidth = 1024;
                    int MaxHeight = 1024;
                    // TODO(casey): These won't work for extracting multiple fonts!
                    // The font has to be part of the spec when we call LoadGlyphBitmap!
                    static VOID *Bits = 0;
                    static HDC DeviceContext = 0;
                    static TEXTMETRIC TextMetric;   <--------------
                    if(!DeviceContext)
                    {
                        .............................................................
                        .......... intalizing DeviceContext and bitmap ..............
                        .............................................................

                        GetTextMetrics(DeviceContext, &TextMetric);
                    }

                    .............................................................
                    ................ calculating the bounds .....................
                    .............................................................

                    if(MinX <= MaxX)
                    {
                        ...
                        ...

                        Asset->Bitmap.AlignPercentage[0] = 1.0f / (r32)Result.Width;
                        Asset->Bitmap.AlignPercentage[1] = (1.0f + (TextMetric.tmDescent - MinY)) / (r32)Result.Height;
                    }

31:50
at this point of the video, when we use the
                
                Asset->Bitmap.AlignPercentage[1] = (1.0f + (TextMetric.tmDescent - MinY)) / (r32)Result.Height;

formula, we see that our text are aligned top, not aligned bottom




46:16
Casey realizing that there are some upside down issue with the way we are extracting text.



                 _______________________ --------------------------
                |                       |
                |                       |
     maxY  -----|-----------------------|                   
                |    ##############     |
                |          ##           |
                |          ##           |                      H_extraction
                |          ##           |
                |          ##           |
                |          ##           |
                |----------##-----------|------- 
     minY  -----|-----------------------|      descent   
                |                       |
                |_______________________|---------------------------






                1 + (MaxY - (H_extraction - Descent))
                _____________________________________

                            H_bitmap



-   H_extraction is entire extraction region

-   H_bitmap is just H_extraction + 2





hence we have the code 

                Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - TextMetric.tmDescent))) / (r32)Result.Height;

[to be honest, i didnt understand how this formula works... he explained it another time in Q/A
    and I still didnt understand it... Doesnt really matter that much though. Im sure I can figure it out]



56:06
Casey adding the gamma when we do text extraction

this is just exactly the same as what we did for our renderer.

                test_asset_builder.cpp 

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint, hha_asset *Asset)
                {
                    ...
                    ...

                    if(MinX <= MaxX)
                    {
                        ...
                        ...

                        u8 *DestRow = (u8 *)Result.Memory + (Result.Height - 1 - 1)*Result.Pitch;
                        u32 *SourceRow = (u32 *)Bits + (MaxHeight - 1 - MinY)*MaxWidth;
                        for(s32 Y = MinY; Y <= MaxY; ++Y)
                        {
                            u32 *Source = (u32 *)SourceRow + MinX;
                            u32 *Dest = (u32 *)DestRow + 1;
                            for(s32 X = MinX; X <= MaxX; ++X)
                            {
                                u32 Pixel = *Source;

                                r32 Gray = (r32)(Pixel & 0xFF);        <-------------------
                                v4 Texel = {255.0f, 255.0f, 255.0f, Gray};
                                Texel = SRGB255ToLinear1(Texel);
                                Texel.rgb *= Texel.a;
                                Texel = Linear1ToSRGB255(Texel);
                                
                                *Dest++ = (((uint32)(Texel.a + 0.5f) << 24) |
                                           ((uint32)(Texel.r + 0.5f) << 16) |
                                           ((uint32)(Texel.g + 0.5f) << 8) |
                                           ((uint32)(Texel.b + 0.5f) << 0));

                                
                                ++Source;
                            }
                        
                            DestRow -= Result.Pitch;
                            SourceRow -= MaxWidth;
                        }

                        Asset->Bitmap.AlignPercentage[0] = 1.0f / (r32)Result.Width;
                        Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - TextMetric.tmDescent))) / (r32)Result.Height;
                    }
                }


58:59
now you can see that we are not seeing black fringes around the text
we get a nice, clean edge



1:00:45
someone asked if it is possible to do this in STB_truetype library in Q/A?

Casey said you can just use the stbtt_GetFontVMetrics(); function to get the same infromation