Handmade Hero Day 167 - Finishing Win32 Font Glyph Extraction

Summary:
getting anti-aliasing fixed in windows font

got rid of the GetPixel(); OS call when extracting windows font


Keyword:
Asset, font


2:06
Casey added the logic that we only want to be doing spin locking if we are in Immediate mode. 


                internal void LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Immediate)
                {
                    asset *Asset = Assets->Assets + ID.Value;        
                    if(ID.Value)
                    {
                        if(AtomicCompareExchangeUInt32((uint32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) == AssetState_Unloaded)
                        {
                            task_with_memory *Task = 0;

                            if(!Immediate)
                            {
                                Task = BeginTaskWithMemory(Assets->TranState);
                            }
                        
                            if(Immediate || Task)        
                            {
                                ................................................
                                .............. Load Asset ......................
                                ................................................
                            }
                            else
                            {
                                Asset->State = AssetState_Unloaded;
                            }
                        }
                        else if(Immediate)      <---------------------------
                        {
                            // TODO(casey): Do we want to have a more coherent story here
                            // for what happens when two force-load people hit the load
                            // at the same time?
                            asset_state volatile *State = (asset_state volatile *)&Asset->State;
                            while(*State == AssetState_Queued) {}
                        }
                    }    
                }





2:30
we are going back to our font extractions and font rendering


3:13
with used fonts from windows, we never finished doing anti-aliasing.
if you look closely now, its not anti-aliased


3:46
someone was hypothesizing that perhaps we are get anti-aliasing cuz we are getting monochrome bitmaps.
so Caey tested that theory and that didnt work. 

at 4:30
Casey ran the game and it is definitely not anti-aliased. 


5:20
Casey trying to look up how to setup a bitmap that Casey can have more control over
So Casey trying to mess with Windows API to fix the anti-aliasing;


7:42
Casey wants to setup a bitmap himself 
Casey wants to be able to access the bits himself. 
previously we were calling GetPixel(); to access the pixel

GetPixel(); is super slow cuz its a OS call. Even if this the asset loader,
there is no reason of being uncessarily slow


                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    ...
                    ...

                    if(MinX <= MaxX)
                    {
                        ...
                        ...

                        Result.Width = Width;
                        Result.Height = Height;
                        Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
                        Result.Memory = malloc(Height*Result.Pitch);
                        Result.Free = Result.Memory;

                        u8 *DestRow = (u8 *)Result.Memory + (Height - 1)*Result.Pitch;
                        for(s32 Y = MinY; Y <= MaxY; ++Y)
                        {
                            u32 *Dest = (u32 *)DestRow;
                            for(s32 X = MinX; X <= MaxX; ++X)
                            {
                                COLORREF Pixel = GetPixel(DeviceContext, X, Y); <--------------------------- the GetPixel(); call
                                u8 Gray = (u8)(Pixel & 0xFF);
                                u8 Alpha = 0xFF;
                                *Dest++ = ((Alpha << 24) |
                                           (Gray << 16) |
                                           (Gray <<  8) |
                                           (Gray <<  0));
                            }
                        
                            DestRow -= Result.Pitch;
                        }
                    }

                    ...
                }



8:21
Caesy proceeds to call the CreateDibSection(); in the LoadGlyphBitmap(); function

this is mostly looking at the MSDN specs

                test_asset_builder.cpp

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

                    int MaxWidth = 1024;
                    int MaxHeight = 1024;
                    static VOID *Bits = 0;
                    static HDC DeviceContext = 0;
                    if(!DeviceContext)
                    {
                        AddFontResourceExA(FileName, FR_PRIVATE, 0);
                        int Height = 128; // TODO(casey): Figure out how to specify pixels properly here
                        HFONT Font = CreateFontA(Height, 0, 0, 0,
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
                        
                        DeviceContext = CreateCompatibleDC(GetDC(0));

                        BITMAPINFO Info = {};
                        Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
                        Info.bmiHeader.biWidth = MaxWidth;
                        Info.bmiHeader.biHeight = MaxHeight;
                        Info.bmiHeader.biPlanes = 1;
                        Info.bmiHeader.biBitCount = 32;
                        Info.bmiHeader.biCompression = BI_RGB;
                        Info.bmiHeader.biSizeImage = 0;
                        Info.bmiHeader.biXPelsPerMeter = 0;
                        Info.bmiHeader.biYPelsPerMeter = 0;
                        Info.bmiHeader.biClrUsed = 0;
                        Info.bmiHeader.biClrImportant = 0;
                        HBITMAP Bitmap = CreateDIBSection(DeviceContext, &Info, DIB_RGB_COLORS, &Bits, 0, 0);
                        SelectObject(DeviceContext, Bitmap);
                        SelectObject(DeviceContext, Font);
                        SetBkColor(DeviceContext, RGB(0, 0, 0));

                        TEXTMETRIC TextMetric;
                        GetTextMetrics(DeviceContext, &TextMetric);
                    }


13:39
After this initalization code, Casey ran the code, and it seems like it solved the anti-aliased.

its not anti-aliased cuz its no longer jenky



15:27
then Casey aims to edit the part to read in the pixels without using GetPixel();




34:12
Casey realizes that the bitmap we get from CreateDIBSection(); is a bottom-up top-down situation

-   so Casey added the one line to make the pointer go to the end

                u32 *Row = (u32 *)Bits + (MaxHeight - 1)*MaxWidth;

-   full code below:

                test_asset_builder.cpp

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

                #if USE_FONTS_FROM_WINDOWS
                    int MaxWidth = 1024;
                    int MaxHeight = 1024;
                    static VOID *Bits = 0;
                    static HDC DeviceContext = 0;
                    if(!DeviceContext)
                    {
                        ...............................................
                        ................ Initilization ................
                        ...............................................
                    }

                    ...
                    ...

                    s32 MinX = 10000;
                    s32 MinY = 10000;
                    s32 MaxX = -10000;
                    s32 MaxY = -10000;

                    u32 *Row = (u32 *)Bits + (MaxHeight - 1)*MaxWidth;      <---------------
                    for(s32 Y = 0; Y < Height; ++Y)
                    {
                        u32 *Pixel = Row;
                        for(s32 X = 0; X < Width; ++X)
                        {
                #if 0
                            COLORREF RefPixel = GetPixel(DeviceContext, X, Y);
                            Assert(RefPixel == *Pixel);
                #endif
                            if(*Pixel != 0)
                            {
                                if(MinX > X)  MinX = X;                    
                                if(MinY > Y)  MinY = Y;                                             
                                if(MaxX < X)  MaxX = X;                    
                                if(MaxY < Y)  MaxY = Y;                    
                            }

                            ++Pixel;
                        }
                        Row -= MaxWidth;
                    }


36:42
then in the last part we proceed to read in pixel information from the Bits pointer 

since Bits, the bitmap we get from windows, is bottom-up, so we start to read in from the bottom.

-   we do this row by row
    we first set the SourceRow at the start of the bottom row 

                u32 *SourceRow = (u32 *)Bits + (MaxHeight - 1 - MinY)*MaxWidth;
    

Bits  --------->    .......####.......
                    .......####.......
                    .......####.......
                    .......####.......
                    .......####.......
                    .......####.......
                    .......####.......
                    .......####.......
                    ##################
SourceRow ------>   ##################

-   once we have our row assigned, we iterate our source pixels pixel by pixel
                    
                    for(s32 Y = MinY; Y <= MaxY; ++Y)
                    {
                        u32 *Source = (u32 *)SourceRow + MinX;
                        ...    
                        for(s32 X = MinX; X <= MaxX; ++X)
                        {
                            ...
                            ...
                            ++Source;
                        }
                    
                        ...
                        SourceRow -= MaxWidth;
                    }

    once we finish reading one row, we set the SourceRow up one row by doing 

                    SourceRow -= MaxWidth;


-   full code blow

                test_asset_builder.cpp

                if(MinX <= MaxX)
                {     
                    Width = (MaxX - MinX) + 1;
                    Height = (MaxY - MinY) + 1;
                    
                    Result.Width = Width ;
                    Result.Height = Height;
                    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
                    Result.Memory = malloc(Result.Height*Result.Pitch);
                    Result.Free = Result.Memory;

                    u8 *DestRow = (u8 *)Result.Memory + (Result.Height - 1)*Result.Pitch;
                    u32 *SourceRow = (u32 *)Bits + (MaxHeight - 1 - MinY)*MaxWidth;
                    for(s32 Y = MinY; Y <= MaxY; ++Y)
                    {
                        u32 *Source = (u32 *)SourceRow + MinX;
                        u32 *Dest = (u32 *)DestRow;
                        for(s32 X = MinX; X <= MaxX; ++X)
                        {
            #if 0
                            COLORREF Pixel = GetPixel(DeviceContext, X, Y);
                            Assert(Pixel == *Source);
            #else
                            u32 Pixel = *Source;
            #endif
                            u8 Gray = (u8)(Pixel & 0xFF);
                            u8 Alpha = Gray;
                            *Dest++ = ((Alpha << 24) | (Gray << 16) | (Gray <<  8) |  (Gray <<  0));
                            ++Source;
                        }
                    
                        DestRow -= Result.Pitch;
                        SourceRow -= MaxWidth;
                    }
                }



43:27
to add one pixel wide apron, 
our previous way wasnt working, so Casey figured out a new way

-   we first set the width and height to be Width + 2 and Height + 2

-   then we clear the entire the bitmap
                
                memset(Result.Memory, 0, Result.Height*Result.Pitch);

-   then we want to write to our Result.Memory at the correct position 
                
                u8 *DestRow = (u8 *)Result.Memory + (Result.Height - 1 - 1)*Result.Pitch; 


  
                    ....................
                    ........####........
                    ........####........
                    ........####........
                    ........####........
                    ........####........
                    ........####........
                    ........####........
                    ........####........
                    .##################.
DestRow ------>     .##################.
                    ....................

we are doing 
                
                u8 *DestRow = (u8 *)Result.Memory + (Result.Height - 1 - 1)*Result.Pitch; 

cuz we want to set it to the 2nd to last row 

if we do 
                u8 *DestRow = (u8 *)Result.Memory + (Result.Height - 1)*Result.Pitch; 

it will set it to the last row 


-   also note that 

                for(s32 Y = MinY; Y <= MaxY; ++Y)
                {
                    ...
                    u32 *Dest = (u32 *)DestRow + 1;
                    for(s32 X = MinX; X <= MaxX; ++X)
                    {
                        ...
                    }
                }

    the DestRow + 1 is done cuz 

                    ....................
                    ........####........
                    ........####........
                    ........####........
                    ........####........
                    ........####........
                    ........####........
                    ........####........
                    ........####........
                    .##################.
DestRow --------->  .##################.
                    ....................                     
                     ^
                     |
                     |

                    Dest       

-   full code below:

                test_asset_builder.cpp

                if(MinX <= MaxX)
                {     
                    Width = (MaxX - MinX) + 1;
                    Height = (MaxY - MinY) + 1;
                    
                    Result.Width = Width + 2;   <---------------
                    Result.Height = Height + 2; <---------------
                    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
                    Result.Memory = malloc(Result.Height*Result.Pitch);
                    Result.Free = Result.Memory;

                    memset(Result.Memory, 0, Result.Height*Result.Pitch);

                    u8 *DestRow = (u8 *)Result.Memory + (Result.Height - 1 - 1)*Result.Pitch;   <---------
                    u32 *SourceRow = (u32 *)Bits + (MaxHeight - 1 - MinY)*MaxWidth;
                    for(s32 Y = MinY; Y <= MaxY; ++Y)
                    {
                        u32 *Source = (u32 *)SourceRow + MinX;
                        u32 *Dest = (u32 *)DestRow + 1;
                        for(s32 X = MinX; X <= MaxX; ++X)
                        {
                            u32 Pixel = *Source;
                            u8 Gray = (u8)(Pixel & 0xFF);
                            u8 Alpha = Gray;
                            *Dest++ = ((Alpha << 24) | (Gray << 16) | (Gray <<  8) |  (Gray <<  0));
                            ++Source;
                        }
                    
                        DestRow -= Result.Pitch;
                        SourceRow -= MaxWidth;
                    }
                }