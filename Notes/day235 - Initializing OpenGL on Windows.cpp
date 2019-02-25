Handmade Hero Day 235 - Initializing OpenGL on Windows

Summary:
explained that the libraries that we are currently linking in our build.batch file, user32.lib, gdi32.lib and 
winmm.lib are import libraries and how are we currently using each one of them

describes OpenGL history

wrote OpenGL set up code 

Q/A
explained how dynamically linked functions work and why we call GetProcAddress();

Keyword:
OpenGL




6:37
Casey went over how we are doing the rendering right now 

essentialy in the win32 layer, we get this window from the CreateWindowExA(); function,
and then we start doing blits to it, and thats how we are doing our rendering  

                win32_handmade.cpp

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {
                    ...
                    ....

                    WNDCLASSA WindowClass = {};
                        
                    Win32ResizeDIBSection(&GlobalBackbuffer, 960, 540);

                    ...
                    ...
                    
                    if(RegisterClassA(&WindowClass))
                    {
    --------------->    HWND Window =
                        CreateWindowExA(
                            0, // WS_EX_TOPMOST|WS_EX_LAYERED,
                            WindowClass.lpszClassName,
                            "Handmade Hero",
                            ...
                            ...);

                        if(Window)
                        {
                            ToggleFullscreen(Window);
                            ...
                            ...
                        }
                    }
                }



7:05
so inorder to use OpenGL, we need to be able to setup our window in a way that tells windows we are going to use 
3D hardware. 

recall in our build.batch file 

                @echo off

                ...
                ...
                set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

we are linking to all these libraries.
These arent the libraries that you are used to. You are normally used to libraries where you can call functions
that are inside the libraries

The libraries we are linking are different. They are called import libraries. What they do is that they allow us to call through to
the windows operating system services that we need

so user32.lib gives us access such as CreateWindowExA(); so we added that there 
gdi32.lib has the graphics functions such as GetDeviceCaps(); and stuff.
winmm.lib is needed for us to set the clock and timer resolution

notice that none of these three libraries connects us to the part of the operating system that talk to the 3D graphics hardware 

there is no way for us to hardcode this. Windows is the guy that talks to the 3D graphics hardware, we have to go through windows 
to access it.

so what we need to do is to add another import library that gives us access to it. so we add 


                @echo off

                ...
                ...
                set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib
                                                                                                    
                                                                                                    ^
                                                                                                    |
                                                                                                    |



9:53
so now we have to include the openGL header 

#include <xinput.h>
#include <dsound.h>
#include <gl/gl.h>

what that does it that it brings in all the openGL functions that we might need. so now we can talk to OpenGL. 

there is actually more than one way we can call it 
if you recall how we call xinput and direct sound is that we are doing late binding.

instead of linking to these import libraries directly, we manually call GetProcAddress(); 

                win32_handmade.cpp

                internal void Win32LoadXInput(void)    
                {
                    // TODO(casey): Test this on Windows 8
                    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
                    if(!XInputLibrary)
                    {
                        // TODO(casey): Diagnostic
                        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
                    }
                    
                    if(!XInputLibrary)
                    {
                        // TODO(casey): Diagnostic
                        XInputLibrary = LoadLibraryA("xinput1_3.dll");
                    }
                    
                    if(XInputLibrary)
                    {
                        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
                        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}

                        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
                        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}
                    }
                    else
                    {
                        // TODO(casey): Diagnostic
                    }
                }


the reason why we do that is becuz, for xinput, it may not be on the machine. So if we were to cold link to xinput as a 
library, like a .lib, then when our executable loads if xinput.h isnt on that machine, we would fail to load, and we dont want to do that 

OpenGL has the nice property of being shipped on windows since the beginning (we are talking about Windows NT 3.51); the oldest 
branch for this strain of windows. We are on the NT branch, we arent on the windows 95, 98 branch.

Essentially all the way back to Windows NT 3.51, it had opengl support on it.

So its totaly safe to link with OpenGL and use the basic OpenGL functions.


11:48
however, you cant gurantee that all the openGL functions in the most moden openGL will be there becuz that depends on how new
the graphics card is and how new the drivers are. 
it doesnt mean you can call any openGL functions becuz there are a lot of extensions to OpenGL 

there is OpenGL 1.0, 2.0, 4.0, 4.3 and so on


13:27
so when we initialized direct sound, we had to load the library dsound.dll
as mentioned above, this is something we didnt have to do for OpenGL. its safe to assume OpenGL is on the machine,
so its okay to hard bind to it. So when our executable gets loaded and looks for the Operationg system services,
OpenGL library will always be there, its just a matter of which version it is.
                
                win32_handmade.cpp

                internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
                {
                    // NOTE(casey): Load the library
                    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
                    if(DSoundLibrary)
                    {
                        // NOTE(casey): Get a DirectSound object! - cooperative
                        direct_sound_create *DirectSoundCreate = (direct_sound_create *)
                            GetProcAddress(DSoundLibrary, "DirectSoundCreate");

                            ...
                            ...
                    }
                }





15:23
So Casey gave a little lecture on how OpenGL works on Windows 

OpenGL is a Graphics API. It was an attempt to make a public spec out of something called IrisGL (or something something "GL"); 
made by a compnay called Silicon Graphics.

Silicon Graphics dont really exists anymore. what they were is that way before anyone were even talking about the concept of 
that a personal computer would have 3D graphics acceleration, Silicon Graphics, a company that specializes in building 
hard accelerated graphics.

so what they did is that they built high end, very very expensive work stations that had 3D accelerated hardware in them. 

in those days, just filling a triangle with a solid color was amzazing.
and as time goes on, they did more and more stuff, such as texture mapping and stuff. 

so then people start to think that the future is gonna be 3D on the PC, so people start to leave Silicon Graphics and start to make 
add-on cards (essentially graphics cards);. It was getting cheap enough to just to make a add-on cards, instead of making dedicated 
work stations by Silicon Graphics

and that gave rise to NVIDIA and 3DFX. 


so what OpenGL was Silicon Graphics Graphics API. They had a graphics api that you use to talk to the Silicon Graphics hardware 
at their dedicated work stations, it was initially called GL.

so what happened was that they wanted to make a more open standard, so people would program to this graphics language.
So GL is a way to talk to the Graphics hardware.

the functionality back then was draw some lines, some triangles, and some texture mapping. that was it. 
there was absolutely no concept of shaders. 


19:43
what happened to OpenGL is that it bifurcate (divide into two branches or forks); into two halves.
so when GL becaome OpenGL, and they created a commitee to create and maintain a standard, obviously Silicon Graphics 
are not the only machines running GL. Which means OpenGL has two halves to it 

so we have the Platform specific part, and the platform independent part 

                             OpenGL
        
            Platform Specific       Platform independent
        ____________________________________________________
                                |
                                |
            not specified       |   glVertex3f();
                                |
                                |   glClear();
                                |
                                |



so when you are talking about OpenGL, you are mostly referring to the Platform independent part. 
for instance, glVertex3f(); glClear(); 

the platform independent part is all the functions that you are used to seeing.

the platform specific part is how you get OpenGL up and running on your platform.
and the reason why this cant be automated is becuz who knows what you are trying to do with it.

        how many windows do you have in your app that needs OpenGL support?

        does this platform even have the concept of a window?

so what the commitee decided to do is to just not specify it.
so when you mention OpenGL, you are never referring to the Platform Specific part, bucuz the commitee never bothered
to specifiy it. Its just left up to the implementation on any given platform to decide how you want to implement it. 



22:04
so the non-platform specific part is what we want to do today. the way that it works on windows, they have a set of functions 
that are prefixed with "wgl", which stands for "windows gl"



23:26
recall from all of our windows programming, we have a thing called DC, device context.
a deviec context is like a notion in windows of the state of graphics at that time.

and we also have things like "HDC" which are handles to DC

OpenGL have an equivalent concept, which is an OpenGL RC, a rendering Context.

the rendering context sits ontop of a HDC

once you have your OpenGL Rendering Context, you can issue your OpenGL calls 

25:07
there is a caveat. In windows, lets say I want to clear something,
i would call the FillRect(); function 


                int FillRect(
                  HDC        hDC,
                  const RECT *lprc,
                  HBRUSH     hbr
                );

https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-fillrect

as you can see, it would take an hDC as a parameter, this way it will know which window you are clearing 
for example, if you have two windows on your screen, hdc will allows us to pass in which window you want to 
FillRect for 

                         ___________________
                        |                   |
                        |                   |
         _______________|                   |
        |               |                   |
        |               |                   |
        |               |                   |
        |               |___________________|
        |_______________|


however, if you look at glClear();

                void glClear(   GLbitfield mask);


https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glClear.xml

it doesnt take in any window handle.

so how do we know which window you are clearing?

the idea is that the whole state of your GL is implicit to your thread 

so each thread, implicit to it, has an OpenGL RC.
so anytime a thread issues an openGL call, it must previously establish which OpenGL RC it is calling about.

so the call to make which OpenGL RC is for your thread is wglMakeCurrent();
https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/nf-wingdi-wglmakecurrent


28:07 
so what we need to do is we create one of those OpenGL Rendering context.
its basically setting up a tiered system: we have windows DC idea, and now we are putting Opengl RC on top of it 


14:58
so initalizing OpenGL on windows is soo jenky, that Casey gave a warning that he will spend tons of time on it 
so Casey started the initalization code 

28:58
so we first start with a function called 

                HGLRC wglCreateContext(
                  HDC Arg1
                );

https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/nf-wingdi-wglcreatecontext

HGLRC refers to windows openGL rendering context handle. A handle to a GLRC

and then we call wglMakeCurrent();


                win32_handmade.cpp

                internal void Win32InitOpenGL(HWND Window)
                {
                    HDC WindowDC = GetDC(Window);

                    ...
                    ...
                    
                    HGLRC OpenGLRC = wglCreateContext(WindowDC);
                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                        // NOTE(casey): Success!!!
                    }
                    else
                    {
                        InvalidCodePath;
                        // TODO(casey): Diagnostic
                    }
                    ReleaseDC(Window, WindowDC);
                }



33:00
so to test OpenGL functionality, Casey commented out our original rendering code and put in some OpenGL rendering code 

-   the glClearColor(); establishes waht color will be used for clearing the screen.

-   one of the things about OpenGL is that its not that low level of a graphcs API.
    its not like mantle. 

    so when we then call glClear(); it doesnt matter what format the backbuffer is that you are clearing,
    it automatically convert the color down to whatever format that is, and fill it with that.


    here, we are clearing GL_COLOR_BUFFER_BIT. the GL_ACCUM_BUFFER_BIT is something so old that you dont even use nowadays.

-   so the thing is that when you call glClear(); it is doing something similar to what we do:
    it is drawing to an off-screen buffer. So its going to do all these operation on some memory that we cant actually see. 

    and the way you actually display that buffer to the window is to call SwapBuffers();

-   there is also this notion in OpenGL of "what region of your screen you are drawing into". and that is done with the function
    glViewport(); 


-   full code below:

                win32_handmade.cpp

                internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                                           HDC DeviceContext, int WindowWidth, int WindowHeight)
                {
                #if 0
                    ..................................................................
                    ............. our original StretchDIBits(); call .................
                    ..................................................................
                #endif
                    glViewport(0, 0, WindowWidth, WindowHeight);
                    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    SwapBuffers(DeviceContext);
                }




37:29
however, with these OpenGL call, it is still not enough. we still wont see our pink screen 
which we set with glClearColor(1.0f, 0.0f, 1.0f, 0.0f);

the reason why it didnt work is becuz for reasons that nowadays Casey would that are very legacy, 
in the old days, there was a lot more optimizations around graphics,  

particularly on the format of what was onscreen and what was stored. THis is becuz memory was very expensive,
and we didnt have infinity of it like we do now. Memory bandwidth is very expensive. we couldnt just go around 
and do 32 bit color all the time. 

and so in the old days, you used to have stuff like having windows running in a palettized graphics mode.
like windows could run in 8 bit color mode where there was a color mode, and each pixel is only 8 bits of data,
and the pixel would go look up its color in the color table. 

and when you were hooking this up in the old days, it was important to tell windows before you actually try to do OpenGL 
rendering to a window, what kind of display you are trying to accomplish. what kind of pixels you want to store in the back buffer 
of openGL, and what kind of pixels you want to display on the screen. 
so the thing we need to do is to call SetPixelFormat(); function 

https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/nf-wingdi-setpixelformat



so if you look at the SetPixelFormat(); function, you see 

                BOOL SetPixelFormat(
                  HDC                         hdc,
                  int                         format,
                  const PIXELFORMATDESCRIPTOR *ppfd
                );

you can see it takes the PIXELFORMATDESCRIPTOR as an argument

and if you look at the definition of PIXELFORMATDESCRIPTOR
https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/ns-wingdi-tagpixelformatdescriptor

you can see how verbose it is.

the catch is that you cant actually set to anything you want.
this is a way for you to set to one of the formats that that OpenGL and gdi currently supports 

so you have to imagine this as a negotiation process. 
we have something we want, which is a 32 bit rgba buffer, 
on the otherside, we have openGL, the graphics library, and the graphics card that it is running on 
can only support certain ways of dealing with pixels. (becuz tis graphics hardware, it is not completely arbituary);

so we dont actually fully create a PIXELFORMATDESCRIPTOR,
what we do is find one PIXELFORMATDESCRIPTOR, and in order to find one, we have to use some other functions 

42:25
so some of the functions you would use includes 

                ChoosePixelFormat();
https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/nf-wingdi-choosepixelformat

so what this will do is to ask the operating system to try to find a pixel format it has that most
closely matches the format that we pass in. 

so we will actually create a PIXELFORMATDESCRIPTOR, and we will fill out some of the information,
then we will ask the OS to find a good one for us. 

-   so now below, we will create PIXELFORMATDESCRIPTOR.and will filled out some of the information

-   then we will call ChoosePixelFormat(); that will return an integer that is the closest match 
    to our pixel format, found by the OS 

-   we then define another PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    we pass it into DescribePixelFormat(); which windows will fill up the relevant information 

-   notice that in the cColorBits and cAlphaBits, we set it to 32 and 8
    Casey wasnt sure if cColorBits includes cAlphaBits. So Casey initially had 24, but the SuggestedPixelFormat
    has 32, so Casey changed it to 32

-   full code below:

                win32_handmade.cpp

                internal void Win32InitOpenGL(HWND Window)
                {
                    HDC WindowDC = GetDC(Window);

                    // TODO(casey): Hey Raymond Chen - what's the deal here?
                    // Is cColorBits ACTUALLY supposed to exclude the alpha bits, like MSDN says, or not?
                    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
                    DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
                    DesiredPixelFormat.nVersion = 1;
                    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
                    DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
                    DesiredPixelFormat.cColorBits = 32;
                    DesiredPixelFormat.cAlphaBits = 8;
                    DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

                    int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
                    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
                    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
                    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
                    
                    HGLRC OpenGLRC = wglCreateContext(WindowDC);
                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                        // NOTE(casey): Success!!!
                    }
                    else
                    {
                        InvalidCodePath;
                        // TODO(casey): Diagnostic
                    }
                    ReleaseDC(Window, WindowDC);
                }

55:48
so now we can access OpenGL. There is one caveat, which is that if you want to access >= OpenGL 3.0 
you have to do an extended OpenGL Context creation call
more will be on this later.


Q/A
58:24
the CPU is packaging up a buffer of commands, and those commands are transferred to the GPU over the PCI Bus, where
it executes the commands, within a reasonable amount of slop

so the Clearing does not happen on the CPU. it happens to the GPU. 


1:00:17
Will we need function pointers from OpenGL?

so just putting our bitmaps on the screen wont require any OpenGL. but if we want to take advantage of OpenGL 4.0 stuff,
we would need function pointers 



1:01:18
since we now have a z buffer from OpenGL, do we need to do our own z sorting? 

absolutely. for two reasons,
1.  for transparent entities. if you have to sort if you want transparent things to work properly.
2.  it will be faster, cuz we can do front-to-back, and we can do an early z to get speed improvement from that.

also you save a bunch of bandwidth if you dont use the oepnGL z-buffer, since it doesnt have to read or write to it. 



1:04:33
is there a difference between when calling a function from the OpenG32.lib vs the on the graphics driver dll?

they are the same thing.
so when you see .lib, it could mean one of two things 
1.  import library 
2.  code library 

a code library has just a bunch of code. 

openGL32.lib is a import library. It has no code. what it has is a bunch of bind points to the Operating System.
So when you link to OpenGL32.lib, what you are actually calling through to is just the OpenGL ICD that is loaded at that time.

that is what it will link to you. 

so it could either go straight to the driver (if it wants to do that); or go through the OS, than to the driver 

sometimes it needs to first go through the OS to do some checking or whatever it wants to do.



1:09:00
so in your code, there two different things that can happen 

                myFunction(__);
                CreateWindow(__);

myFunction(); is something I created myself  
CreateWindow(); is in the Operating System, 

so when our executable gets compiled, myFunction(); goes away. 
it becomes 
                call (relative address myFunction(); was compiled at);


but that is not what happens with CreateWindow(); 
it does become 

                call (address);

but that call to an address goes to an address is dynamically resolved. 
[its pretty much things explained in the linker and loader book]


GetProcAddress(); is just us "dynamic loading" the function address manually.
the reason why we do this manually is becuz we dont know if it exists or not.
its possible that Xinput.lib doesnt exist. 

if it doenst exist, our game wont run. but we dont want that. we want our game to still run 
if xinput doesnt exist.

so any library that Casey thinks that has a chance of not being present, Casey will do a GetProcAddress(); call 