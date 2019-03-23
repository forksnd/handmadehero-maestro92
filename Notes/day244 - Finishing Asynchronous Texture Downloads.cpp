Handmade Hero Day 244 - Finishing Asynchronous Texture Downloads

Summary:
followed up on day 243. In day 243, we implemented texture downloads. Today we will debug yesterday_s implementation.

mentioned that if we are doing texture downloads on multiple threads, OpenGL doesnt do anything for synchronization 
between the background thread and the main thread. Casey argues that we are fine with that. 

went through initilization code to set up sRGB for the framebuffer.

Keyword:
debugging OpenGL. OpenGL initalization


1:37
so in day 243, we implemented texture downloads. our code compiles, but our screen doesnt render anything 
properly. so today is about debugging. 

anytime that you are trying to debug graphics drivers on windows, its always a nightmare.
the thihng is that you cant actually step into their code and see what is going on. 
so you have to treat it as a black box. sometimes you can get errors back, so that may help.


2:50
we know that if we want to ship a game, we need to use a GPU. However, the tradeoff is, once you decided to use
the GPU, there is the driver that is in the way, and all the graphics card specific stuff in the way. 
and so you kind of have to treat it like a black box.  



5:48
so Casey went into the debugger, and step through the openGL initialization code where we create the context, 

                win32_handmade.cpp

                internal void Win32InitOpenGL(HWND Window)
                {
                    HDC WindowDC = GetDC(Window);

                    ...
                    ...

                    HGLRC OpenGLRC = wglCreateContext(WindowDC);
                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                        b32 ModernContext = false;

                        wglCreateContextAttribsARB =
                            (wgl_create_context_attribts_arb *)wglGetProcAddress("wglCreateContextAttribsARB");
                        if(wglCreateContextAttribsARB)
                        {
                            // NOTE(casey): This is a modern version of OpenGL
                            HGLRC ShareContext = 0;
        --------------->    HGLRC ModernGLRC = wglCreateContextAttribsARB(WindowDC, ShareContext, Win32OpenGLAttribs);
                            if(ModernGLRC)
                            {
                                if(wglMakeCurrent(WindowDC, ModernGLRC))
                                {
                                    ModernContext = true;
                                    wglDeleteContext(OpenGLRC);
                                    OpenGLRC = ModernGLRC;
                                    GlobalOpenGLRC = OpenGLRC;
                                }
                            }
                        }
                    }
                }

Casey showed that just to do a openGL context, there was a noticable delay. It was a like a 2 second delay.

                HGLRC ModernGLRC = wglCreateContextAttribsARB(WindowDC, ShareContext, Win32OpenGLAttribs);

the whole startup time for our game was instant before. now, all we did was to ask hardware acceleration to get initalized,
and our startup time gets extremely slow, like 10X worst. 

but there is nothing you can do. If you want to talk to the hardware, you have to talk to the driver. 



15:50
Casey opened up the "Threads" tab in the visual studio debugger. Since we have multiplte threads calling 
Win32CreateOpenGLContextForWorkerThread, Casey demonstrated how to look at only one thread_s routine.

essentially Casey right clicked and selected the "Freeze" option on the threads that he doesnt want to interfere our 
debugging process 


26:47
Casey mentioned that there is a very important thing underlying what is going on that you may not be aware of, but probably should be.
notice that we are doing our texture downloads in a separate background thread.


so when that happens, there is no synchronization between our code in the "FinalizeAsset_Bitmap" and the rendering.
as far as opengl is concerned.

                handmade_asset.cpp

                internal void LoadAssetWorkDirectly(load_asset_work *Work)
                {
                    TIMED_FUNCTION();

                    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
                    if(PlatformNoFileErrors(Work->Handle))
                    {
                        switch(Work->FinalizeOperation)
                        {
                            case FinalizeAsset_None:
                            {
                                ...
                            } break;

                            case FinalizeAsset_Font:
                            {
                                ...
                            } break;

                            case FinalizeAsset_Bitmap:
                            {
                                loaded_bitmap *Bitmap = &Work->Asset->Header->Bitmap;
                                Bitmap->TextureHandle = 
                                    Platform.AllocateTexture(Bitmap->Width, Bitmap->Height, Bitmap->Memory);
                            } break;
                        }
                    }

                    ...
                    ...
                }


meaning, when we are submitting this texture, it is totaly a separate thing from when we do our drawing. It will 
not attempt to serialize these operations for us at all. what that means, there is nothing preventing OpenGL from trying to render 
with a texture that is not yet finished downloading. So that can happen. It other words, lets say we started texture downloading, and we 
dont finish it by the time we need it. so we issued a render command, and the textures arent ready yet. if these two operations are 
on the same thread (imagine we are just using a single threaded openGL); OpenGL will wait the texture downloading finishes, until 
it starts rendering. so it wouldnt render until it finishes downloading. 

since we are multithreaded and we have two separate context, only openGL operations that are on the same context are serialized. so it 
doesnt have any need to serialize the download of that texture with the usage of the texture from another thread. 

Casey says he doesnt really remember what the specs says. It might be allowed to serialize them. it could decide to do it, but not 
required. what that means is that, we may just draw garbage on the screen, if the texture hasnt gotten there in time. 

Casey says he would prefer to leave it this way, and just try to make it so that our code is always gives textures downloads enough time. 
The only thing that could happen, which is rendering the wrong thing, or pause the phrame, creating a hiccup. neither of those is good. 

Casey says he prefers to have a little glitch (rendering the wrong thing, or not rendering anything); than have a frame rate hiccup. 

ideally, we want neither. 



32:40
now that Casey have finished debugging, and our game renders fine with multithreaded texture downloads, 
so now the only thing we are missing now, is we dont have sRGB support on the framebuffer (gemma correction);
we are enabling sRGB writes to the framebuffer

so previously, we did this:

                handmade_opengl.cpp

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
    ----------->        glEnable(GL_FRAMEBUFFER_SRGB);
                    }
                }       


so remember when we wrote the software rasterizer, there were two places where we did gamma correct. 
one place is in the textures. we load in the textures, we convert it from sRGB to linear space.

the other place was the framebuffer. we convert framebuffer pixels from sRGB to linear. 
the degree that this is gonna matter visually is very minimal. its only gonna matter for translucent things. 


in 3D, however, gamma correction becomes much more important. so if you want to do 3D some day, you have to understand this. 



35:50
so what we want to do here is to find a way to tell OpenGL is that 
so here we have enabled sRGB blending

                if(Info.GL_EXT_framebuffer_sRGB)
                {
                    glEnable(GL_FRAMEBUFFER_SRGB);
                }

unfortunately, we havent told that the framebuffer that we are drawing to on the window should be sRGB. 


36:13
you may ask, why are there two separate enables calls for the framebuffer_srgb. The reason is because OpenGL is a little bit 
wonky this way. OpenGL tends to set state on both the objects that it exists in the system, and the system itself. 

so glEnable(GL_FRAMEBUFFER_SRGB); is setting the system. but not the actual framebuffer.

so if you have a texture, there are two ways you can set how that texture is sampled. One way is on the texture, 
another way is on the actual shading code. And these two typically actually combine to produce what is actually sampled. 

now in our case, it may seem odd for the framebuffer to have two ways of setting it. 
one on the framebuffer itself, the other on GL shading enables (on the system). 
But the reason for that, in OpenGL, you can create multiple framebuffers. 
For example, we can create back buffers to draw to as temporaries as a part of a more complicated render pass. 

and so what they did when they designed OpenGL is that they enabled the ability to set some of those framebuffers as 
sRGB, some of them as linear. and when you bound a framebuffer to draw to, 
if GL_FRAMEBUFFER_SRGB is on (meaning if the system has SRGB enabled);, 
then if the actual framebuffer is SRGB as well, then finally, it will use SRGB. if that framebuffer doesnt have SRGB on,
but system has sRGB on, it wont render sRGB. 


38:21
so now in our initalization code we want to get our framebuffer and set it to sRGB mode. 
so to do that, we need to get another "extension situation"

51:33
so casey made it so that if the game doesnt support sRGB (cuz this is only supported in the extended versions);, 
we dont really care. if it does support it, then we set it 


                internal HGLRC Win32InitOpenGL(HDC WindowDC)
                {
                    int SuggestedPixelFormatIndex = 0;
                    GLuint ExtendedPick = 0;

                    // TODO(casey): This needs to happen after we create the initial OpenGL context,
                    // BUT how do we do that given that the DC needs to be in the correct format
                    // first?  Do we just wglMakeCurrent back to zero and _then_ re set the pixel format?
                    // Or what???
                    wgl_choose_pixel_format_arb *wglChoosePixelFormatARB = 
                        (wgl_choose_pixel_format_arb *)wglGetProcAddress("wglChoosePixelFormatARB");
                    if(wglChoosePixelFormatARB)
                    {
                        int IntAttribList[] =
                        {
                            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
                            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                            0,
                        };
                        float FloatAttribList[] = {0};
                        wglChoosePixelFormatARB(WindowDC, IntAttribList, FloatAttribList, 1, 
                            &SuggestedPixelFormatIndex, &ExtendedPick);
                    }

                    if(ExtendedPick == 0)
                    {
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

                        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
                    }



55:10
so notice the list of IntAttribList

                int IntAttribList[] =
                {
                    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                    WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
                    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                    WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                    0,
                };

these are all things that we want to specify

Casey literally went though the specs for 

                BOOL wglGetPixelFormatAttribivARB(HDC hdc,
                                                  int iPixelFormat,
                                                  int iLayerPlane,
                                                  UINT nAttributes,
                                                  const int *piAttributes,
                                                  int *piValues);

and examined which IntAttribList we would need. 

https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
