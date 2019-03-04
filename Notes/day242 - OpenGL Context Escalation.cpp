Handmade Hero Day 242 - OpenGL Context Escalation

Summary:
showed how to get opengl information that your machines GPU supports
including the openGL version, and all the extensions calls. 

talked about the concept of Modern OpenGL 3.0 context. Mentions that in the future,
we will need to create a 2nd OpenGL Context that will be responsible for texture downloads

showed code how to create a modern opengl 3.0 context

explained why adobe came up with sRGB color space. 

Keyword:
openGL, sRGB

5:20
Casey mentioned that currently our implementation for textures are hacked together. 
you can clearly see a pause in the frames at this hour when the scene with all the hats show up 




9:43
recall in day 241, we had our the following setup for the Win32InitOpenGL(); function 

                internal void Win32InitOpenGL(HWND Window)
                {
                    ...
                    ...

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

and as you can see, we left the "if(OpenGLExtensionIsAvailable())" commented out.
Today Casey will come in and fix it up. essentially look up openGL extension properly.


10:08
so Casey first defined a struct for opengl_info in the handmade_opengl.cpp file 

this struct essentially will contain all the relevant information about our GPU card
(what extensions can we use, what version of OpenGL can we use, etc etc)

so we first added two booleans to mark whether these two Extensions are present or not 

                handmade_opengl.cpp

                struct opengl_info
                {
                    ...
                    ...

                    b32 GL_EXT_texture_sRGB;
                    b32 GL_EXT_framebuffer_sRGB;
                };



12:58
then we add a function, OpenGLGetInfo();
this will return a opengl_info, tell us all the openGL information we need



                internal opengl_info OpenGLGetInfo(b32 ModernContext)
                {
                    ...
                    ...
                }



13:49
then we can have the following structure:


                internal void Win32InitOpenGL(HWND Window)
                {
                    ...
                    ...

                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                        opengl_extensions Extensions = OpenGLGetInfo();

                        if(Extensions.GL_EXT_texture_sRGB)
                        {
                            OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
                        }

                        if(Extensions.GL_EXT_framebuffer_sRGB)
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



14:03
now we go back and actually implement the OpenGLGetInfo(); function 
so the idea is that we ask OpenGL, what extensions do you support.
so there is this glGetString(); command, that we can use 

notice in the desciption, you can pass in a parameter 

                "
                Parameters
                name
                Specifies a symbolic constant, one of GL_VENDOR, GL_RENDERER, GL_VERSION, 
                GL_SHADING_LANGUAGE_VERSION, or GL_EXTENSIONS."

so Casey figured that we might as well store the strings for any relevant strings 
so Casey added more info in the opengl_info struct


                handmade_opengl.cpp
                
                struct opengl_info
                {
                    b32 ModernContext;
                    
                    char *Vendor;
                    char *Renderer;
                    char *Version;
                    char *ShadingLanguageVersion;
                    char *Extensions;
                    
                    b32 GL_EXT_texture_sRGB;
                    b32 GL_EXT_framebuffer_sRGB;
                };

so inside the OpenGLGetInfo(); we made all the calls to glGetString();
as you can see, we are querying GL_VENDOR, GL_RENDERER, GL_VERSION, GL_SHADING_LANGUAGE_VERSION and GL_EXTENSIONS

[i am assuming this is what GLEW is doing?]

so casey initially did 


                internal opengl_info OpenGLGetInfo(b32 ModernContext)
                {
                    opengl_info Result = {};

                    Result.ModernContext = ModernContext;
                    Result.Vendor = (char *)glGetString(GL_VENDOR);
                    Result.Renderer = (char *)glGetString(GL_RENDERER);
                    Result.Version = (char *)glGetString(GL_VERSION);
  ------------->    Result.ShadingLanguageVersion = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
                    Result.Extensions = (char *)glGetString(GL_EXTENSIONS);                       
                }

and this apparently gave us a compiler error becuz GL_SHADING_LANGUAGE_VERSION is in openGL 2.0


so Casey did the following 

                internal opengl_info OpenGLGetInfo(b32 ModernContext)
                {
                    opengl_info Result = {};

                    Result.ModernContext = ModernContext;
                    Result.Vendor = (char *)glGetString(GL_VENDOR);
                    Result.Renderer = (char *)glGetString(GL_RENDERER);
                    Result.Version = (char *)glGetString(GL_VERSION);
                #if 0
                    Result.ShadingLanguageVersion = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
                #else 
                    Result.ShadingLanguageVersion = "(none)";
                #endif
                    Result.Extensions = (char *)glGetString(GL_EXTENSIONS);                       
                }



17:40
so Casey now went into the debugger and checked out what these string output 
and as you can see on Casey_s machine, we have 

        Vendor =    "ATI Technologies Inc"
        Renderer =  "ATI FirePro V8700 (FireGL)"
        Version =   "3.3.11296 Compatibility Profile"
        ShadingLanguageVersion Version = "(none)"     
        Extensions =    "GL_AMDX_debug_output GL_AMDX_version...."

for the version, what that means is that this GPU support OpenGL 3.X
so the Extensions string we got back is actually a huge ass list. of strings
it contains an entire list of strings, separated by space
you can see the entire string at 29:39, which Casey showcasses

so this list contains every extension that this particular implementation of OpenGL supports

so if you go on docs.gl
any function that has "gl3" highlighted would work on this card 




19:04
what Casey would like to do is to parse Result.Extensions,
then we would compare if any of the extension matches GL_EXT_texture_sRGB or GL_EXT_framebuffer_sRGB

if the string matcehs, we would just set the Result.GL_EXT_texture_sRGB or Result.GL_EXT_framebuffer_sRGB to true

                handmade_opengl.cpp

                internal opengl_info OpenGLGetInfo(b32 ModernContext)
                {
                    opengl_info Result = {};

                    Result.ModernContext = ModernContext;
                    Result.Vendor = (char *)glGetString(GL_VENDOR);
                    Result.Renderer = (char *)glGetString(GL_RENDERER);
                    Result.Version = (char *)glGetString(GL_VERSION);
                #if 0
                    Result.ShadingLanguageVersion = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
                #else                    
                    Result.ShadingLanguageVersion = "(none)";
                #endif
                    
                    Result.Extensions = (char *)glGetString(GL_EXTENSIONS);

                    char *At = Result.Extensions;
                    while(*At)
                    {
                        while(IsWhitespace(*At)) {++At;}
                        char *End = At;
                        while(*End && !IsWhitespace(*End)) {++End;}

                        umm Count = End - At;        
                        
                        if(0) {}
                        else if(StringsAreEqual(Count, At, "GL_EXT_texture_sRGB")) {Result.GL_EXT_texture_sRGB=true;}
                        else if(StringsAreEqual(Count, At, "GL_EXT_framebuffer_sRGB")) {Result.GL_EXT_framebuffer_sRGB=true;}

                        At = End;
                    }
                    
                    return(Result);
                }




27:49
Casey added handmade_shared.h file for all the utility functions
Casey moved all the string manipulation related functions there. 



35:25
so now Casey will look at to clean up some of the OpenGL functions 

                win32_handmade.cpp

                internal void OpenGLInit(b32 ModernContext)
                {
                    opengl_info Info = OpenGLGetInfo(ModernContext);
                        
                    OpenGLDefaultInternalTextureFormat = GL_RGBA8;
                    if(Info.GL_EXT_texture_sRGB)
                    {
                        OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
                    }

                    // TODO(casey): Need to go back and use extended version of choose pixel format
                    // to ensure that our framebuffer is marked as SRGB?
                    if(Info.GL_EXT_framebuffer_sRGB)
                    {
                        glEnable(GL_FRAMEBUFFER_SRGB);
                    }
                }       

and Casey made slight changes in Win32InitOpenGL();  

                internal void Win32InitOpenGL(HWND Window)
                {
                    ...
                    ...
                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                        OpenGLInit(ModernContext);
                        
                        wglSwapInterval = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
                        if(wglSwapInterval)
                        {
                            wglSwapInterval(1);
                        }
                    }
                    else
                    {

                    }
                }



37:35
so Casey reiterated that the OpenGL versions that shipped with microsoft is always OpenGL 1.1
microsoft never bothered to update the bindings in windows. 

so all of the OpenGL progress comes from the driver manufacturers. 
the driver writers themselves are the ones who updates OpenGL.

so when you do "#include <gl/gl.h>" on windows, you are really including bindings that is from 20 years ago
(1997 to be exact); 
everything else you want to use, you have to get by querying these extensions

so OpenGL is meant to be extended. so when people add extensions, and over time, some extensions 
are agreed upon generally useful things, and when they agreed that a bunch of these things are the "new version" 
of OpenGL, they update the core number, so it becomes OpenGL 2.0, OpenGL 2.1, ... OpenGL 3.0...

so what they did with OpenGL 3.0 (maybe 4.0); is they said, OpenGL 3.0 adds a tons of stuff mandatorily.
OpenGL 3.0 requires you do have a certain set of shading language available

OpenGL 4.0 adds a bunch of stuff.

so what ends up happening is that, we only need to query if you are OpenGL 3.0  or above. If you are, I just assume 
that you have these extensions/functionality.

so OpenGL 3.0, there is a way for us to query a "OpenGL 3.0 context". 

so querying that OpenGL 3.0 context, actually allows you the ability to create more than one context,
so you can use it on multiple threads, and still have them share texture handles 

and so what you can do with that is you can download textures on one thread, while you run the game and render on 
another thread, without these context fighting each other. 

Casey believes that the actual requirement on OpenGL, is that you are not allowed to use the same context on two threads 
at the same time. Its single-threaded. so if you want to use a context on two different threads, you have to make two 
different context. 

so if you have two context, you have to tell OpenGL that they are sharing a texture handle 


so now Casey will show us how to use OpenGL 3.0 context. 
this will help us to download textures in the background. 


42:25
so previously recall that when we were calling 
                
                HGLRC OpenGLRC = wglCreateContext(WindowDC);

so this context will return us an OpenGL 1.1 context. 

so what you can do, is that you can use one of the extended openGL function,ARB_CreateContext();
what that will do is that, it will return a OpenGL Context with the version that you request. 

specs are in the link below:
https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt



45:09
So Casey once again created a function pointer for it 

                win32_handmade.cpp

                typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
                global_variable wgl_swap_interval_ext *wglSwapInterval;

                typedef HGLRC WINAPI wgl_create_context_attribts_arb(HDC hDC, HGLRC hShareContext,
                                                                     const int *attribList);


and now in our Win32InitOpenGL(); code, we have: 
-   we first check if "wglCreateContextAttribsARB" exists.

-   if so we get a ModernGLContext by calling wglCreateContextAttribsARB();
    notice that in the specs, the wglCreateContextAttribsARB(); has the following definition

                HGLRC wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int *attribList);

    and you can see that it takes in "HGLRC hShareContext" as an argument. The ShareContext is very important 
    we dont need to use it yet, but we will need to use it later when we create a second context for texture downloads
    

-   this is also one of the reasons why Casey called this "OpenGL Context Escalation"
    in order to call wglCreateContextAttribsARB, you would have to call a regular wglCreateContext(); first 

    you cant call wglCreateContextAttribsARB(); until you create a openGL context of version 1. 


-   full code below:

                internal void Win32InitOpenGL(HWND Window)
                {
                    ...
                    ...

                    HGLRC OpenGLRC = wglCreateContext(WindowDC);
                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                        b32 ModernContext = false;
                        
                        wgl_create_context_attribts_arb *wglCreateContextAttribsARB =
                            (wgl_create_context_attribts_arb *)wglGetProcAddress("wglCreateContextAttribsARB");
                        if(wglCreateContextAttribsARB)
                        {
                            // NOTE(casey): This is a modern version of OpenGL
                            int Attribs[] =
                            {
                                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                                WGL_CONTEXT_MINOR_VERSION_ARB, 0,
                                WGL_CONTEXT_FLAGS_ARB, 0 // NOTE(casey): Enable for testing WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
                #if HANDMADE_INTERNAL
                                |WGL_CONTEXT_DEBUG_BIT_ARB
                #endif
                                ,
                                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                                0,
                            };
                                
                            HGLRC ShareContext = 0;
                            HGLRC ModernGLRC = wglCreateContextAttribsARB(WindowDC, ShareContext, Attribs);
                            if(ModernGLRC)
                            {
                                if(wglMakeCurrent(WindowDC, ModernGLRC))
                                {
                                    ModernContext = true;
                                    wglDeleteContext(OpenGLRC);
                                    OpenGLRC = ModernGLRC;
                                }
                            }
                        }
                        else
                        {
                            // NOTE(casey): This is an antiquated version of OpenGL
                        }

                        OpenGLInit(ModernContext);
                        
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



48:02
notice all the Attribs passed into the wglCreateContextAttribsARB(); function, 
Casey added all of the #define definitions to handmade_opengl.cpp

                handmade_opengl.cpp 

                #include "handmade_render_group.h"

                #define GL_FRAMEBUFFER_SRGB               0x8DB9
                #define GL_SRGB8_ALPHA8                   0x8C43

                // NOTE(casey): Windows-specific
                #define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
                #define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
                #define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
                #define WGL_CONTEXT_FLAGS_ARB                   0x2094
                #define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

                #define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
                #define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

                #define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
                #define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002


50:15
Casey mentioned that in our Attribs list, we pass in "WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB"

                int Attribs[] =
                {
                    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
                    WGL_CONTEXT_FLAGS_ARB, 0 // NOTE(casey): Enable for testing WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
    #if HANDMADE_INTERNAL
                    |WGL_CONTEXT_DEBUG_BIT_ARB
    #endif
                    ,
                    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                    0,
                };

this was needed becuz in the modern openGL, they got rid of the fixed function pipeline function calls 
everything goes through shaders. So this bit flag is just saying, dont just give me the modern opengl stuff,
give me the old stuff as well. 




54:29
another thing is that Casey went into the debugger and showed us where 
wglCreateContextAttribsARB is pointing at. 

as you can see, it is pointing at 

                atio6axx.dll!0x000006908e0e0 

which means it is pointing at the ati driver. 

when we are making these function calls, we are not going into a library. 
when you are linking with the OpenGL32.lib library, you are not linking with a library, you are just connecting 
yourself with the driver that is doing the work. 



55:57
now that we are in OpenGL 3.0, Casey believes that, all the logic in OpenGLInit(); are superfulous calls,
becuz Casey believes these are all required in OpenGL 3.0

but we will keep it for now becuz this will allow us to run on older hardware if we wanted to 
so there is no reason to get rid of them. 

                internal void OpenGLInit(b32 ModernContext)
                {
                    opengl_info Info = OpenGLGetInfo(ModernContext);
                        
                    OpenGLDefaultInternalTextureFormat = GL_RGBA8;
                    if(Info.GL_EXT_texture_sRGB)
                    {
                        OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
                    }

                    // TODO(casey): Need to go back and use extended version of choose pixel format
                    // to ensure that our framebuffer is marked as SRGB?
                    if(Info.GL_EXT_framebuffer_sRGB)
                    {
                        glEnable(GL_FRAMEBUFFER_SRGB);
                    }
                }       

58:42
Notice that Casey also added the #define for GL_SHADING_LANGUAGE_VERSION 
in the handmade_opengl.cpp file

                handmade_opengl.cpp 

                #include "handmade_render_group.h"

                #define GL_FRAMEBUFFER_SRGB               0x8DB9
                #define GL_SRGB8_ALPHA8                   0x8C43

    --------->  #define GL_SHADING_LANGUAGE_VERSION       0x8B8C

                // NOTE(casey): Windows-specific
                #define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
                #define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
                #define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
                #define WGL_CONTEXT_FLAGS_ARB                   0x2094
                #define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

                #define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
                #define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

                #define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
                #define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002



Casey found the value for GL_SHADING_LANGUAGE_VERSION in 
https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h


59:42
With all this ModernContext stuff, Casey also added a flag in opengl_info

                struct opengl_info
                {
                    b32 ModernContext;
                    
                    char *Vendor;
                    char *Renderer;
                    char *Version;
                    char *ShadingLanguageVersion;
                    char *Extensions;
                    
                    b32 GL_EXT_texture_sRGB;
                    b32 GL_EXT_framebuffer_sRGB;
                };




1:01:42
so now we are also querying the ShadingLanguageVersion, which gives us GLSL 3.3
that means this GPU supports GLSL 3.3


1:13:07
so Casey explained again why sRGB space is the way it is 
recall that sRGB is on the y = x^2.2 curve, which means more values in the dark end, less values in the bright end 


            |                                     .
            |                                    .
            |                                   .
            |                                  .
            |
            |                               .
            |
            |                          .
            |                    .
            |             .
            |    .
            |_________________________________________


so human eyes, bright places are hard to discern 



            |                                     .
            |                                    b
            |                                   a
            |                                  .
            |
            |                               .
            |
            |                          .
            |                    .
            |             .
            |    c  d
            |_________________________________________

for example, to discern between the brightness between point a and point b is very hard for human eyes
however, in the dark areas, its easy to discern 

so for us, to distinguish between point c and point d, its very easy. 

recall lets say you are using 8 bits per channel, that means 8 bits is all the resolution we got. 

what that means is, for any given encoding, encoding them linearly, either you will have way more resolution that you need 
in the bright than you want, or you dont have enough resolution to represent the darks. 

which is why Adobe came up with this curved representation 

so the solution to that is to use this curve, so as you go through that curve,
you have lots of values down in the dark region, and you have less bits to represent the bright region.


                                        |-- bright  |
                                        |  region --|
            |                                     .
            |                                    .
            |                                   .
            |                                  .
            |
            |                               .
            |
            |                          .
            |                    .
            |             .
            |    .
            |_________________________________________
            
            |-- dark   --|
            |-- region --|

essentially, you are bending the curve to give you the output resolution to regions/place that you want 

that is not saying that 8 bits is enough. As display become more high dynamic range, we probably want 10 bits or more 