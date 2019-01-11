Handmade Hero Day 164 - Asset Processing with Windows Fonts

Summary:
extracted font bitmaps from windows. the same task as day 163 without using STB stb_truetype

Keyword:
Font, windows API


0:31
Casey claims that todays topic will be doing day 163 tasks without the STB library,
using Windows Fonts

4:03
every operating system, no matter what you are running on, you will have a way to extract fonts out of that OS.

so we will extract fonts out of Windows.


4:20
windows has a bunch of calls for drawing fonts. 
such as the TextOut(); function
https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/nf-wingdi-textouta


8:53
so at the top of the test_asset_builder.cpp, Casey added a #define 

                test_asset_builder.cpp

                #include "test_asset_builder.h"

                #define USE_FONTS_FROM_WINDOWS 1

                #if USE_FONTS_FROM_WINDOWS
                #include <windows.h>
                #else
                #define STB_TRUETYPE_IMPLEMENTATION
                #include "stb_truetype.h"
                #endif

this way we can choose whether we want to use the library in our asset generator.


9:16
then in our LoadGlyphBitmap(); we write the logic path for using fonts from windows


                test_asset_builder.cpp 

                internal loaded_bitmap
                LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    #if USE_FONTS_FROM_WINDOWS

                        yet to be written

                    #else
                        ......................................................
                        ........ loading GlyphBitmap fron stb_truetype .......
                        ......................................................                        
                    #endif
                }



10:01
we start off with our TextOut function. Windows always expect UTF-16

the msdn documentation says 

    "The TextOut function writes a character string at the specified location, 
    using the currently selected font, background color, and text color."
    https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/nf-wingdi-textouta


the TextOut(); function takes a DeviceContext, so we try to create the DeviceContext


-   notice that we made DeviceContext a static variable.
    this is so that we dont want to create the device context and bitmap every time we call LoadGlyphBitmap();

-   most of this is windows API calls. Not much to say.

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

                    static HDC DeviceContext = 0;
                    if(!DeviceContext)
                    {
                        DeviceContext = CreateCompatibleDC(0);
                    }

                    wchar_t CheesePoint = (wchar_t)Codepoint;
                    TextOutW(DeviceContext, 0, 0, &CheesePoint, 1);
                    ...
                    ...
                }


11:57
then we want to call CreateCompatibleBitmap();
and we want to select that in by calling SelectObject();
its kind of like "set the active bitmap"

-   we just gave it 1024 x 1024, this way we will always have plenty of room for any character that we are making

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

                    static HDC DeviceContext = 0;
                    if(!DeviceContext)
                    {
                        DeviceContext = CreateCompatibleDC(0);
                        HBITMAP Bitmap = CreateCompatibleBitmap(DeviceContext, 1024, 1024);
                        SelectObject(DeviceContext, Bitmap);
                    }

                    wchar_t CheesePoint = (wchar_t)Codepoint;
                    TextOutW(DeviceContext, 0, 0, &CheesePoint, 1);
                    ...
                    ...
                }


15:59
then the thing we need to consider how much space did that text occupy
we also call the GetTextMetrics(); function. that tells us the metrics on a specific font,
such as how high or wide it is.

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

                    static HDC DeviceContext = 0;
                    if(!DeviceContext)
                    {
                        DeviceContext = CreateCompatibleDC(0);
                        HBITMAP Bitmap = CreateCompatibleBitmap(DeviceContext, 1024, 1024);
                        SelectObject(DeviceContext, Bitmap);

                        TEXTMETRIC TextMetric;
                        GetTextMetrics(DeviceContext, &TextMetric);
                    }

                    wchar_t CheesePoint = (wchar_t)Codepoint;
                    TextOutW(DeviceContext, 0, 0, &CheesePoint, 1);
                    ...
                    ...
                }



16:52
the next thing to consider is how much space did that text take up
Then we want the GetTextentPoint32(); function
https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/nf-wingdi-gettextextentpoint32a

"The GetTextExtentPoint32 function computes the width and height of the specified string of text."

what this will do is that, when we do our text out, we want to know how big it is gonna be

the function will populate the Size field.
now we would know the region that the text occupy after we do our TextOut();


                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

                    static HDC DeviceContext = 0;
                    if(!DeviceContext)
                    {
                        DeviceContext = CreateCompatibleDC(0);
                        HBITMAP Bitmap = CreateCompatibleBitmap(DeviceContext, 1024, 1024);
                        SelectObject(DeviceContext, Bitmap);

                        TEXTMETRIC TextMetric;
                        GetTextMetrics(DeviceContext, &TextMetric);
                    }

                    wchar_t CheesePoint = (wchar_t)Codepoint;

                    SIZE size;
                    GetTextentPoint32(DeviceContext, &CheesePoint, 1, &Size);
                    int Width = Size.cx;
                    int Height = Size.cy

                    TextOutW(DeviceContext, 0, 0, &CheesePoint, 1);
                    ...
                    ...
                }



18:24
one thing we have to do is to clear the background.

we do by calling PatBlt();, one of the windows api

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

                    static HDC DeviceContext = 0;
                    if(!DeviceContext)
                    {
                        DeviceContext = CreateCompatibleDC(0);
                        HBITMAP Bitmap = CreateCompatibleBitmap(DeviceContext, 1024, 1024);
                        SelectObject(DeviceContext, Bitmap);

                        TEXTMETRIC TextMetric;
                        GetTextMetrics(DeviceContext, &TextMetric);
                    }

                    wchar_t CheesePoint = (wchar_t)Codepoint;

                    SIZE size;
                    GetTextentPoint32(DeviceContext, &CheesePoint, 1, &Size);
                    int Width = Size.cx;
                    int Height = Size.cy

                    PatBlt(DeviceContext, 0, 0, Width, Height, BLACKNESS);
                    TextOutW(DeviceContext, 0, 0, &CheesePoint, 1);
                    ...
                    ...
                }





20:04
we set the text color by calling 

                SetTextColor(DeviceContext, RGB(255, 255, 255));

which is also a windows call

        

27:26
when you draw text in windows There is this SetBkColor();

                SetBkColor();

that you have to call to set the color of background of your font bitmap in widnows
https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/nf-wingdi-setbkcolor



21:58
then just like in stb_truetype, we want to extract the font.

Casey doesnt matter if its upside down 

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    ...................................................
                    ......... DeviceContext, TextOutW .................
                    ...................................................


                    Result.Width = Width + 2;
                    Result.Height = Height + 2;
                    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
                    Result.Memory = malloc(Result.Height*Result.Pitch);
                    Result.Free = Result.Memory;

                    memset(Result.Memory, 0, Result.Height*Result.Pitch);

                    u8 *DestRow = (u8 *)Result.Memory + (Result.Height - 1 - 1)*Result.Pitch;
                    for(s32 Y = MinY; Y <= MaxY; ++Y)
                    {
                        for(s32 X = MinX; X <= MaxX; ++X)
                        {

                            COLORREF Pixel = GetPixel(GlobalFontDeviceContext, X, Y);
                            u8 Alpha = (u8)(Pixel & 0xFF);

                            *Dest++ = ((Alpha << 24) |
                                       (Alpha << 16) |
                                       (Alpha << 8) |
                                       (Alpha << 0));

                        }
                    
                        DestRow -= Result.Pitch;
                    }
                }


35:10
we call AddFontResourceExA(); function, this will get the font into windows        
essentially we are telling windows to load this font.

-   the FileName we pass in is gonna be the ".ttf" file.
    for example, "arial.ttf"

                test_asset_builder.cpp

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

                    static HDC DeviceContext = 0;
                    if(!DeviceContext)
                    {
                        AddFontResourceExA(FileName, FR_PRIVATE, 0);
                        ...
                        ...
                    }

                }





37:56
Now we want get the font out.
we do this by calling CreateFontA();

-   we just say for now Height = 128

-   width is 0, cuz we dont want any stretching 

                test_asset_builder.cpp

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

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
                        ...
                        ...
                    }

                }






45:05
after creating the font, now we want to tell the device Context to use this font

we would call the SelectObject(); function.


                test_asset_builder.cpp

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

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
        
                        DeviceContext = CreateCompatibleDC(0);
                        HBITMAP Bitmap = CreateCompatibleBitmap(DeviceContext, 1024, 1024);
                        SelectObject(DeviceContext, Bitmap);
        ------------>   SelectObject(DeviceContext, Font);
                        SetBkColor(DeviceContext, RGB(0, 0, 0));

                        TEXTMETRIC TextMetric;
                        GetTextMetrics(DeviceContext, &TextMetric);
                    }

                    ...
                    ...
                }


-   full code below:
    
                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {
                    loaded_bitmap Result = {};    

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
                        
                        DeviceContext = CreateCompatibleDC(0);
                        HBITMAP Bitmap = CreateCompatibleBitmap(DeviceContext, 1024, 1024);
                        SelectObject(DeviceContext, Bitmap);
                        SelectObject(DeviceContext, Font);
                        SetBkColor(DeviceContext, RGB(0, 0, 0));

                        TEXTMETRIC TextMetric;
                        GetTextMetrics(DeviceContext, &TextMetric);
                    }

                    ...
                    ...
                }



46:40
Casey thinks the text render in windows is smaller and thinner.

Casey suspects that when we call GetTextExtentPoint32(); point for a letter, it is not giving us a tight rectangle.
the bitmaps has a layer of empty padding,

the stb_truetype library is giving us a tight rectangle enclosing the letter




49:20
Casey proved his assumption by rendering the alpha in black. and as shown the windows implementation does have empty space 
at the top and bottom of the bitmap

this shows that GetTextExtentPoint32(); is not giving us a tight rectangle.


52:50
Casey couldnt find a way to tell windows to get rid of the empty space padding

so he will just have to do it himself. He will scan the bitmap to get rid of the empty space.

we can just scan the entire 1024 x 1024 bitmap, but we are also calling GetTextExtentPoint32(); to bounds the search area.
this way we only have to go from 0 ~ Height and 0 ~ Width, instead of 0 ~ 1024 


                test_asset_builder.cpp

                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {

                    SIZE Size;
                    GetTextExtentPoint32W(DeviceContext, &CheesePoint, 1, &Size);

                    int Width = Size.cx;
                    int Height = Size.cy;

                    ...
                    ...                    

                    s32 MinX = 10000;
                    s32 MinY = 10000;
                    s32 MaxX = -10000;
                    s32 MaxY = -10000;
                    for(s32 Y = 0; Y < Height; ++Y)
                    {
                        for(s32 X = 0; X < Width; ++X)
                        {
                            COLORREF Pixel = GetPixel(DeviceContext, X, Y);
                            if(Pixel != 0)
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
                        }
                    }

                    .........................................................
                    .............. getting bitmap data ......................
                    .........................................................
                }





57:44
Casey mentioned that we want that one pixel border in our bitmaps. (this is mentioned when we implemented the renderer)

so we expanded our bitmap by one border


                internal loaded_bitmap LoadGlyphBitmap(char *FileName, char *FontName, u32 Codepoint)
                {

                    ...
                    ...

                   if(MinX <= MaxX)
                    {
                        --MinX;         <---------------------- this part
                        --MinY;
                        ++MaxX;
                        ++MaxY;
                        
                        Width = (MaxX - MinX) + 1;
                        Height = (MaxY - MinY) + 1;
                        
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
                                COLORREF Pixel = GetPixel(DeviceContext, X, Y);
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

                }



1:00:45
what is a backing bitmap?

a device context has the notion of having a bitmap set in it. That is the bitmap to which it draw. Here in windows,
we didnt really create one ourselves manually. we just said called Windows to create one that will be comppatible 
with the windows device context. 


1:15:11
will passing CLEAR_TYPE_QUALITY instead of ANTIALIASED_QUALITY into the CreateFontA(); windows function make a difference?

CLEAR_TYPE_QUALITY is not what we want. that actually plays with the color channels.

an LCD panel actually look like 

         _______________________
        |       |       |       |
        |       |       |       |
        |       |       |       |
        |  Red  | Green | Blue  |
        |       |       |       |
        |       |       |       |
        |       |       |       |
        |_______|_______|_______|

it has a bar of red, a bar of green and a bar of blue. 

what happens is is that if you have two pixels together 

         _______________________________________________
        |       |       |       |       |       |       |
        |       |       |       |       |       |       |
        |       |       |       |       |       |       |
        |  Red  | Green | Blue  |  Red  | Green | Blue  |
        |       |       |       |       |       |       |
        |       |       |       |       |       |       |
        |       |       |       |       |       |       |
        |_______|_______|_______|_______|_______|_______|

you have some subpixel precision in the color channels.

so if you want to render a font that is aligned like this


                        |                               |
                        |                               |
                        |                               |
         _______________|_______________________________|
        |       |       |       |       |       |       |
        |       |       |       |       |       |       |
        |       |       |       |       |       |       |
        |  Red  | Green | Blue  |  Red  | Green | Blue  |
        |       |       |       |       |       |       |
        |       |       |       |       |       |       |
        |       |       |       |       |       |       |
        |_______|_______|_______|_______|_______|_______|
                        |                               |
                        |                               |
                        |                               |

so you can actually fill a blue fringe, 
this is what clear type does, and that will be awful. 

we dont want any color fringing. 
