Handmade Hero Day 241 - OpenGL VSync and sRGB Extensions

Summary:

explained how vsync works and how to do vsync in Windows OpenGL.

mentioned that vsync in windows opengl is achieved through the function wglSwapInterval();
however this function is not supported in the windows_s opengl32.lib becuz its one of the extension functions
so we have to do wglGetProcAddress(); to get a function pointer  

made some OpenGL settings to have OpenGL enable sRGB FrameBuffer and sRGB internal texture format to handle gamma correctly 

briefly mentioned what EXT and ARB means in OpenGL function calls

explained how SwapBuffers(); works under the hood and how it interacts with drivers 

Keyword:
vsync, windows, OpenGL


4:16
Casey pointed out in Win32DisplayBufferInWindow();

                internal void Win32DisplayBufferInWindow(platform_work_queue *RenderQueue, game_render_commands *Commands,
                                           HDC DeviceContext, s32 WindowWidth, s32 WindowHeight)
                {
                    SortEntries(Commands);

                    ...
                    ...
                }

sorting with temporary memory seems faster than sorting it in place, so Casey would like to provide SortMemory for it
since we can easily afford the space, we will just give it temporary space. 

                
5:10
then in SortEntries();, it will take in some memory to sort
recall that we have moved the sort function to the handmade_render.cpp, the utility file for the renderer system                

                handmade_render.cpp

                internal void SortEntries(game_render_commands *Commands, void *SortMemory)
                {
                    u32 Count = Commands->PushBufferElementCount;
                    tile_sort_entry *Entries = (tile_sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);

                //    BubbleSort(Count, Entries, SortMemory);
                //    MergeSort(Count, Entries, SortMemory);
                    RadixSort(Count, Entries, (tile_sort_entry *)SortMemory);
                    
                #if HANDMADE_SLOW
                    if(Count)
                    {
                        for(u32 Index = 0;
                            Index < (Count - 1);
                            ++Index)
                        {
                            tile_sort_entry *EntryA = Entries + Index;
                            tile_sort_entry *EntryB = EntryA + 1;

                            Assert(EntryA->SortKey <= EntryB->SortKey);
                        }
                    }
                #endif
                }


17:08
so in the Win32DisplayBufferInWindow(); function we want to make sure we pass in to Win32DisplayBufferInWindow();
enough memory.

so we initialize 1 mb of SortMemory at initalization,
then during the game loop, we check constantly to see if we have enough memory for sorting

                win32_handmade.cpp

                int WinMain()
                {
                    ...
                    ...
                    umm CurrentSortMemorySize = Megabytes(1);
                    void *SortMemory = Win32AllocateMemory(CurrentSortMemorySize);

                    ...
                    ...

                    while(GlobalRunning)
                    {
                        ...
                        ...


                        umm NeededSortMemorySize = RenderCommands.PushBufferElementCount * sizeof(tile_sort_entry);
                        if(CurrentSortMemorySize < NeededSortMemorySize)
                        {
                            Win32DeallocateMemory(SortMemory);
                            CurrentSortMemorySize = NeededSortMemorySize;
                            SortMemory = Win32AllocateMemory(CurrentSortMemorySize);
                        }
                        
                        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                        HDC DeviceContext = GetDC(Window);
                        Win32DisplayBufferInWindow(&HighPriorityQueue, &RenderCommands, DeviceContext,
                                                   Dimension.Width, Dimension.Height,
                                                   SortMemory);

                    }
                }


26:16

corrected the allocation for the game_render_commands




                win32_handmade.cpp

                int WinMain()
                {
                    ...
                    ...
                    umm CurrentSortMemorySize = Megabytes(1);
                    void *SortMemory = Win32AllocateMemory(CurrentSortMemorySize);

                    // TODO(casey): Decide what our pushbuffer size is!
                    u32 PushBufferSize = Megabytes(4);
                    void *PushBuffer = Win32AllocateMemory(PushBufferSize);

                    ...
                    ...

                    while(GlobalRunning)
                    {
                        ...
                        ...

                        game_render_commands RenderCommands = RenderCommandStruct(
                            PushBufferSize, PushBuffer,
                            GlobalBackbuffer.Width,
                            GlobalBackbuffer.Height);
                        
                        game_offscreen_buffer Buffer = {};
                        Buffer.Memory = GlobalBackbuffer.Memory;
                        Buffer.Width = GlobalBackbuffer.Width; 
                        Buffer.Height = GlobalBackbuffer.Height;
                        Buffer.Pitch = GlobalBackbuffer.Pitch;

                        Game.UpdateAndRender(&GameMemory, NewInput, &RenderCommands);

                        ...
                        ...
                    }
                }



29:44
Casey made a slight change in the rendering path that decides between using hardware or software rendering:
essentially, Casey made Renderer_UseSoftware into a debug flag

with our current architecture, we can just change the flag at runtime.

                internal void Win32DisplayBufferInWindow(platform_work_queue *RenderQueue, game_render_commands *Commands,
                                           HDC DeviceContext, s32 WindowWidth, s32 WindowHeight, void *SortMemory)
                {
                    SortEntries(Commands, SortMemory);

                    DEBUG_IF(Renderer_UseSoftware)
                    {
                        loaded_bitmap OutputTarget;
                        OutputTarget.Memory = GlobalBackbuffer.Memory;
                        OutputTarget.Width = GlobalBackbuffer.Width;
                        OutputTarget.Height = GlobalBackbuffer.Height;
                        OutputTarget.Pitch = GlobalBackbuffer.Pitch;

                        SoftwareRenderCommands(RenderQueue, Commands, &OutputTarget);        

                        b32 DisplayViaHardware = true;        
                        if(DisplayViaHardware)
                        {
                            OpenGLDisplayBitmap(GlobalBackbuffer.Width, GlobalBackbuffer.Height, GlobalBackbuffer.Memory,
                                                GlobalBackbuffer.Pitch, WindowWidth, WindowHeight);
                            SwapBuffers(DeviceContext);
                        }
                        else
                        {
                            // TODO(casey): Centering / black bars?
                    
                            if((WindowWidth >= GlobalBackbuffer.Width*2) &&
                               (WindowHeight >= GlobalBackbuffer.Height*2))
                            {
                                StretchDIBits(DeviceContext,
                                              0, 0, 2*GlobalBackbuffer.Width, 2*GlobalBackbuffer.Height,
                                              0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height,
                                              GlobalBackbuffer.Memory,
                                              &GlobalBackbuffer.Info,
                                              DIB_RGB_COLORS, SRCCOPY);
                            }
                            else
                            {

                                int OffsetX = 0;
                                int OffsetY = 0;

                                StretchDIBits(DeviceContext,
                                              OffsetX, OffsetY, GlobalBackbuffer.Width, GlobalBackbuffer.Height,
                                              0, 0, GlobalBackbuffer.Width, GlobalBackbuffer.Height,
                                              GlobalBackbuffer.Memory,
                                              &GlobalBackbuffer.Info,
                                              DIB_RGB_COLORS, SRCCOPY);
                            }
                        }
                    }
                    else
                    {
                        OpenGLRenderCommands(Commands, WindowWidth, WindowHeight);        
                        SwapBuffers(DeviceContext);
                    }
                }


34:05
So Casey now explaining how vsync work

vsync is a on old school concept where on a CRT monitor 


                CRT
         _______________________
        |                       |
        |                       |
        |                       |
        |                       |
        |                       |
        |                       |
        |_______________________|


essentially you have an electron beam on the back of the tv, that will be scanning across 
screen in a zig-zag pattern and lighting up the phosphorous that was coated on the front glass plate

and when the elctron hit the phosphorous, it will emit a color, and thats how you get television


                CRT
         _______________________
        |-----------------------|     essentially scanning it line by line in a z pattern
        |-----------------------|     so top left of line 1, then start aganin from the left at line 2, all the way to the bottom
        |-----------------------|
        |-----------------------|
        |-----------------------|
        |-----------------------|
        |_______________________|


the reason why there is this technology called vsync is becuz, as your scanning electron gets to the bottom,
this electron beamer will have to go from the bottom right to the top left

and during that period, where it goes from bottom right to the top left is the synchronization period, where you would start the next frame.
that is called vertical sync or vsync.

so that phrase became known as "in your current system, waiting for the right time to update the frame, such as the machine that is displaying it,
either an LCD or CRT, doesnt end up displaying half of the old frame, and half of the new frame."

what happens is that, in the middle of updating, and you change the frame right in the middle, then you would end up with some portion of the 
frame from the old one and some portion of the frame from the new one. 

and that manifests itself as a tear, or "tearing"

for example you may see the bottom half moving ahead of the upper half

         _______________________
        |                       |
        |      ######           |
        |      ######           |
        |-----------------------|
        |       ######          |
        |       ######          |
        |       ######          |
        |                       |
        |_______________________|

the bottom half is from the current frame and the upper half is from the next frame

so this is what vsync means. We certainly want our game to prevent tearing, and we also want our game to have steady timing,
so our timing is synced with the video properly.



36:48
what we can do is to ask OpenGL for it. there is a function called 

    wglSwapIntervalEXT(X);

the x argment is: how many frames do you want to wait before we swap. 

so when we call SwapBuffers(); to display our fraem in GL, if we set the "X" argument to zero, 
            
            wglSwapIntervalEXT(0);

https://www.khronos.org/registry/OpenGL/extensions/EXT/WGL_EXT_swap_control.txt

that means, as soon as we SwapBuffers();, but it on the screen. I dont care if you are in the middle of displaying a frame,
just do it. 


if the value is 1, 
        
            wglSwapIntervalEXT(1);

that means wait for the next vertical vtrace
if the value is 2, then wait for 2 vertical vtraces, which means every other frame. 


37:58
its important to know that the wglSwapIntervalEXT(0); call is just a request. For all we know, we may not even be on a card
that supports vsync. vsync is not a gurantneed. For shipping the game, we may also have to have logic to tries to look and 
see "oh looks like we are not getting any vsync, since our frame rate is 2 milliseconds a frame". so may have to manually insert 
some delay [dont we already do that?]

so wglSwapIntervalEXT(); is a request, and on most users machine, if they dont disable it on the control panel, its a request that will work,
and give a reliable vsync. so its a good thing to use.

39:10
recommends that for lots of the regular opengl call, you can refer to 
http://docs.gl/
for specs 


39:43
so the way that OpenGL works on windows is that, there is the "wgl" stuff and the "gl" stuff 
"wgl" stuff are the windows binding for OpenGL stuff
"gl" are the actual opengl calls 

so this wglSwapIntervalEXT(); is an Windows operation 
what this means is that we have to call through windows. The same call wont work on OSX 

Casey believes that this wglSwapIntervalEXT(); call is added after the version of openGL that ships on windows. 

so if you just add wglSwapInterval(); in our code, it wont compile, giving you the "identifier not found" error


in contrast wglCreateContext(); its just there, it came with windows. So if you just drop that in the code,
it will compile. 

so the solution is that there is this extension mechanism inside wgl to get new function, that you can then call. 


41:28
so what we want to do is that, when we init our OpenGL, what we want to do is to get any of these extensions that we might need 

these extensions works exactly like all the other functions that we had to use GetProcAddress(); for getting win32 functions.
only we had to use wglGetProcAddress();

essentially if there is a function that we need that wasnt in the base services that we are linking with (opengl32.lib, in this case); 
then we need to use wglGetProcAddress();

so when we call GetProcAddress(); the idea is that we are asking windows, I know there is a function named this, do you support it, 
if you do, please return a pointer to it, so I can use it 


-   one thing to note here is that 
                
                typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
                global_variable wgl_swap_interval_ext *wglSwapInterval;

    we made a typedef in our win32_handmade.cpp file 

    if you dont understnad what we are doing, refer to day 154 in the Q/A section
    essentially, we are making "wgl_swap_interval_ext" as a function pointer  


-   full code below:

                internal void Win32InitOpenGL(HWND Window)
                {
                    ...
                    ...
                    
                    HGLRC OpenGLRC = wglCreateContext(WindowDC);
                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                        wglSwapInterval = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
                        if(wglSwapInterval)
                        {
                            wglSwapInterval(1);
                        }
                    }
                    else
                    {
                        InvalidCodePath;
                        // TODO(casey): Diagnostic
                    }
                    ReleaseDC(Window, WindowDC);
                }


45:00
now when we run the game, we are running at locked speed of 60 frames per second. 

so just to compare the difference of not having OpenGL controling the vsync,
Casey set it be 0

                wglSwapInterval(0);

and you can see we are running at 2.76 ms for every frame 


45:55
we are not quite done yet for a number of reasons. 
meaning, we dont know how long wglSwapInterval(); actually is. maybe we are on a 120 Hz display, in which 
it wont be 60 frames per second, it will be 120 frames per second 

the other thing that is worth noting is that the way the OpenGL extension method technically works is that, calling wglGetProcAddress();
is not techinically sufficient. 

what we are suppose to do is to check for the existence of extensions by checking a string. 

so according to the https://people.freedesktop.org/~marcheu/extensions/EXT/wgl_swap_control.html


it says:
Dependencies on WGL_EXT_extensions_string

    Because there is no way to extend wgl, these calls are defined in
    the ICD and can be called by obtaining the address with
    wglGetProcAddress.  Because this extension is a WGL extension, it
    is not included in the GL_EXTENSIONS string.  Its existence can be
    determined with the WGL_EXT_extensions_string extension.

    For historical reasons, GL_EXT_SWAP_CONTROL is also included in the
    GL_EXTENSIONS string as return from glGetString.


so techinically, we we are suppose to do is to ask the drive for the "WGL_EXT_swap_control"
so if this string exist, then we can use it, if its not there, its not there


the "WGL_EXT_swap_control" is found, if you scroll to the top of that page.


                Name

                    EXT_swap_control

                Name Strings

    ----------->    WGL_EXT_swap_control

                Version

                    Date: 9/23/1999   Revision: 1.5



apparently Casey himeself prefers to check it with the wglGetProcAddress(); way, 

                wglSwapInterval = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
                if(wglSwapInterval)
                {
                    ... success ...
                }
                else
                {
                    ... fail ...
                }
                      
but its not technically correct 
but Casey doesnt care


48:48
there is another type of extension that we want becuz right now, our hardware rendering is actually wrong. 

our software rendering path is more correct than our hardware rendering path.
the reason is becuz our hardware path doesnt understand we are using sRGB at the moment 

[if you dont remember what sRGB is, refer to day 94]. We are actually using an sRGB encoding for our textures. 
[not quite sRGB, we are using a squared mapping]

so we would like to have the OpenGL to do the gamma correction for us. 






50:01
Casey proceeds to look up the specs for GL_ARB_framebuffer_sRGB 
https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_framebuffer_sRGB.txt

so basically what happens is that when you create a pixel format,
like what we are doing in the Win32InitOpenGL(); function 


                internal void Win32InitOpenGL(HWND Window)
                {
                    HDC WindowDC = GetDC(Window);

                    // TODO(casey): Hey Raymond Chen - what's the deal here?
                    // Is cColorBits ACTUALLY supposed to exclude the alpha bits, like MSDN says, or not?
                    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
                    DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
                    DesiredPixelFormat.nVersion = 1;
                    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
                #if HANDMADE_STREAMING
                    // NOTE(casey): PFD_DOUBLEBUFFER appears to prevent OBS from reliably streaming the window
                    DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW;
                #else
                    DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
                #endif
                    DesiredPixelFormat.cColorBits = 32;
                    DesiredPixelFormat.cAlphaBits = 8;
                    DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

                    int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
                    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
                    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
                    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
                        
                    ...
                    ...
                }

basically, when we create our FrameBuffer, we can make it to be sRGB if we want it to be. 
there is quite a few things we can set in OpenGL to have it handle gamma correctly. 



53:36
recall in our software renderer, what we did is we squared our destination values, as well as the source colors 




                void DrawRectangleQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                                     loaded_bitmap *Texture, real32 PixelsToMeters,
                                     rectangle2i ClipRect)
                {


                    for(int Y = MinY; Y < MaxY; ++Y)
                    {
                        ...
                        uint32 *Pixel = (uint32 *)Row;
                        for(int XI = MinX; XI < MaxX; XI += 4)
                        { 
                            ...
                            ...


                            __m128i TexelArb = _mm_and_si128(SampleA, MaskFF00FF);
                            __m128i TexelAag = _mm_and_si128(_mm_srli_epi32(SampleA, 8), MaskFF00FF);
                            TexelArb = _mm_mullo_epi16(TexelArb, TexelArb);
                            __m128 TexelAa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelAag, 16));
                            TexelAag = _mm_mullo_epi16(TexelAag, TexelAag);

                            __m128i TexelBrb = _mm_and_si128(SampleB, MaskFF00FF);
                            __m128i TexelBag = _mm_and_si128(_mm_srli_epi32(SampleB, 8), MaskFF00FF);
                            TexelBrb = _mm_mullo_epi16(TexelBrb, TexelBrb);
                            __m128 TexelBa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBag, 16));
                            TexelBag = _mm_mullo_epi16(TexelBag, TexelBag);

                            __m128i TexelCrb = _mm_and_si128(SampleC, MaskFF00FF);
                            __m128i TexelCag = _mm_and_si128(_mm_srli_epi32(SampleC, 8), MaskFF00FF);
                            TexelCrb = _mm_mullo_epi16(TexelCrb, TexelCrb);
                            __m128 TexelCa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCag, 16));
                            TexelCag = _mm_mullo_epi16(TexelCag, TexelCag);

                            __m128i TexelDrb = _mm_and_si128(SampleD, MaskFF00FF);
                            __m128i TexelDag = _mm_and_si128(_mm_srli_epi32(SampleD, 8), MaskFF00FF);
                            TexelDrb = _mm_mullo_epi16(TexelDrb, TexelDrb);
                            __m128 TexelDa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDag, 16));
                            TexelDag = _mm_mullo_epi16(TexelDag, TexelDag);

                            ...
                            ...
                            ...

                            // NOTE(casey): Go from sRGB to "linear" brightness space
                            Destr = mmSquare(Destr);
                            Destg = mmSquare(Destg);
                            Destb = mmSquare(Destb);



                            ...
                            ...
                            Blendedr = _mm_mul_ps(Blendedr, _mm_rsqrt_ps(Blendedr));
                            Blendedg = _mm_mul_ps(Blendedg, _mm_rsqrt_ps(Blendedg));
                            Blendedb = _mm_mul_ps(Blendedb, _mm_rsqrt_ps(Blendedb));
                        }
                    }
                }

54:49
so we need to tell OpenGL to do the exact same thing, change the textures to linear space, and then change the result back to 
sRGB space before giving it to the monitor

so all that has to happen for that, we have to tell OpenGL about the texture format

recall previously, we had GL_RGBA8 as the internal format for the textures 

                global_variable u32 TextureBindCount = 0;
                internal void OpenGLRenderCommands(game_render_commands *Commands, s32 WindowWidth, s32 WindowHeight)
                {    
                    ...
                    ... 

                    tile_sort_entry *Entry = SortEntries;
                    for(u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
                    {
                        render_group_entry_header *Header = (render_group_entry_header *)
                            (Commands->PushBufferBase + Entry->PushBufferOffset);
                        
                        void *Data = (uint8 *)Header + sizeof(*Header);
                        switch(Header->Type)
                        {
                            ...
                            ...

                            case RenderGroupEntryType_render_entry_bitmap:
                            {
                                ...
                                ...
                                Entry->Bitmap->Handle = ++TextureBindCount;
                                glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);

        ------------------->    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                             GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);
                
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                            
                                
                                OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                            } break;

                            ...
                            ...

                            InvalidDefaultCase;
                        }
                    }
                }

what we need to do is to just submit the sRGB version of it. 


56:46
so if you go to 
https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_sRGB.txt

and look at  -- Section 3.8.1, Texture Image Specification:

and you can see the internal formats listed 

and one of the formats listed is "SRGB8_ALPHA8_EXT"

so what we want is we set the default internal format to be GL_RGBA8.
then if we detect that the extensions exists, we set it to be SRGB8_ALPHA8_EXT
this way our textures will always be sampled properly. 


                GLuint OpenGLDefaultInternalTextureFormat = GL_RGBA8;
                
                if(...extensions exists...)
                {
                    OpenGLDefaultInternalTextureFormat = SRGB8_ALPHA8_EXT;
                }



58:21
so in our initalization function, we set our OpenGLDefaultInternalTextureFormat accordingly 
as you can see, we first set it to GL_RGBA8, then we check if the extension is available.
if so, we set it to GL_SRGB8_ALPHA8


another thing is that for gamma correction to work, we need to tell two things to use sRGB
1.  textures (which we mentioned above);
2.  the framebuffer

so we also have to enable the sRGB on the FrameBuffer

[note that Casey didnt finish the code here, so he left the OpenGLExtensionIsAvailable(); function commented out.
    he will finish it in the next episode]



-   so about GL_FRAMEBUFFER_SRGB and GL_SRGB8_ALPHA8
they werent defined [didnt understand why the arent defined]

so Casey went into the gl extension header file 
https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h

and found the values himself

notice that SRGB8_ALPHA8_EXT got supercedded by GL_SRGB8_ALPHA8


the way these things work is that "ARB" stands for "architecture review board"
for more info [refer to my OpenGL Extension google doc]


-   full code below:


                internal void Win32InitOpenGL(HWND Window)
                {
                    ...
                    ...
                    
                    HGLRC OpenGLRC = wglCreateContext(WindowDC);
                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                #define GL_FRAMEBUFFER_SRGB               0x8DB9
                #define GL_SRGB8_ALPHA8                   0x8C43
                        OpenGLDefaultInternalTextureFormat = GL_RGBA8;
                        // TODO(casey): Actually check for extensions!
                //        if(OpenGLExtensionIsAvailable())
                        {
                            OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
                        }

                //        if(OpenGLExtensionIsAvailable())
                        {
                            glEnable(GL_FRAMEBUFFER_SRGB);
                        }

                        wglSwapInterval = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
                        if(wglSwapInterval)
                        {
                            wglSwapInterval(1);
                        }
                    }
                    else
                    {
                        InvalidCodePath;
                        // TODO(casey): Diagnostic
                    }
                    ReleaseDC(Window, WindowDC);
                }



and in our rendering function, we pass the OpenGLDefaultInternalTextureFormat argument to the glTexImage2D(); function

                global_variable u32 TextureBindCount = 0;
                internal void OpenGLRenderCommands(game_render_commands *Commands, s32 WindowWidth, s32 WindowHeight)
                {    
                    ...
                    ... 

                    tile_sort_entry *Entry = SortEntries;
                    for(u32 SortEntryIndex = 0; SortEntryIndex < SortEntryCount; ++SortEntryIndex, ++Entry)
                    {
                        render_group_entry_header *Header = (render_group_entry_header *)
                            (Commands->PushBufferBase + Entry->PushBufferOffset);
                        
                        void *Data = (uint8 *)Header + sizeof(*Header);
                        switch(Header->Type)
                        {
                            ...
                            ...

                            case RenderGroupEntryType_render_entry_bitmap:
                            {
                                ...
                                ...
                                Entry->Bitmap->Handle = ++TextureBindCount;
                                glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);

        ------------------->    glTexImage2D(GL_TEXTURE_2D, 0, OpenGLDefaultInternalTextureFormat, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                             GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);
                
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
                                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                            
                                
                                OpenGLRectangle(Entry->P, MaxP, Entry->Color);
                            } break;

                            ...
                            ...

                            InvalidDefaultCase;
                        }
                    }
                }





Q/A
1:10:06
Casey clarifying some ideas about vsync

so when we run our game we have 

1.  game logic 
2.  SwapBuffers

3.  game logic 
4.  SwapBuffers
...

SwapBuffers(); is the thing that tells OpenGL we are done with the frame. 
people are under the misconception that SwapBuffers(); means wait. 

its not true. SwapBuffers(); just means we are done. it doenst mean we stop. 
its just telling the driver, you can kick off a frame. 

so what actually happens, we execute step1 to 4 

whenever we call SwapBuffers();, that kicks off the graphics card to let it know it can start rendering


so recall that if we dont do vsync, each frame only takes 2 ms 
so whenever we call SwapBuffers, its like OpenGL will queue up a render command.

1.  game logic 
2.  SwapBuffers ------->  

3.  game logic 
4.  SwapBuffers ------->

5.  game logic 
6.  SwapBuffers ------->
...

so depending on how the graphics card decides to set himself up, either double or triple buffering. 
(the more buffer, the commands it can take);

when it gets to a point that OpenGL drivers cant queue up anymore commands, thats when it will wait. 

SwapBuffers doesnt acutally block your thread until the driver decides that it doesnt want anymore info from you

the driver, if it wanted to, could decide quadriple buffers, then it becomes 4 frames ahead all of the time
[currently we are not sleeping, we commented out that code]



so what you are seeing when you report the frame time in our Debug UI, 
has nothing to do to waiting for a frame to be displayed. 

what it is is waiting for the driver to tell us when it wants more information. 
and the reason why it comes out to 16 ms second is becuz when we enable vsync, 
the timings for the driver requesting information from us comes out to roughly 16 ms
essentially we arent in any danger of missing frames



1:15:34
is there a reason to make the interval in wglSwapInterval(x); larger than 1?

yes, and we will probalby do it ourselves. the reason is becuz lets say we are on a machine, and we find that we are 
actually unable to hit the frame rate of the monitor

recall in day 18


                frame rate 

                    |___________|___________|___________|___________|___________|___________|
                    1           2           3           4           5           6           7


                monitor frame rate 

                    |_______|_______|_______|_______|_______|_______|_______|_______|_______|_______|
                    a       b       c       d       e       g 

                so 
                b will display game state 1, 
                c will display game state 2, 
                d displays game state 3
                e displays game state 3 again 
                g displays game state 4.

                so to the user, it will wait for two monitor frame rate to get a new game state, which is why 
                it will look like it skipped a frame. 


                we want the monitor to update roughly the same frame as our game does.

                Thats the goal
                we can either do the exact rate of your monitor frame rate, 
                or any subdivision of it. Let say your monitor is at 120 hz, u can do it at 60 fps

                Hz is just cycles per second


so we can do a subdivision of it if we can hit the monitor refresh rate.

you dont want openGL to be too ahead becuz otherwise, you have a huge input lag. so you have to only have it 
buffer 1 or 2 frames.


1:17:23
how would you allow the game to run at a non 60 fps rate and not speed up or slow down the animation speed?

our game already runs off a fixed update tick. so as long as you are passing in the fixed update tick, you are good 




1:19:52
what are the trade offs of storing textures on disc in the same linear color space that we are using?

u dont really want to store textures in a linear color space becuz u waste a lot of bits. the human doesnt percieve 
dark tones the same way it percieves light tones. Light tones needs less data then dark tones to represent properly.

thats why we have sRGB in the first place. so we want to store them non-linearly. 


1:21:20
with wglSwapInterval(); do we still have to do sleep at the end of our frame?
could you also comment on glFinish and glFlush and how are they used?

so wglSwapInterval(); doesnt actually gurantee us the vsync, so we dont have to sleep at the end of the frame 
if wglSwapInterval(); is working. 

but if its not, we will have to. it will do a sleep there for us effectively, so we dont have to if wglSwapInterval(); 
is sleeping.

glFinish(); and glFlush(); is not relevant for us. if you want to force all rendering to finish, you can call glFlush(); ?
one of them is a more stringent gurantee than the other. 

glFinish(); is the full completion function. Dont comback until you are done.  
glFlush(); is a just a way of saying, kick it off. 

1:25:24
how do you check if the vsync is active?

you just gotta check monitor refresh if you can, and see how long it takes and make sure its similar.

