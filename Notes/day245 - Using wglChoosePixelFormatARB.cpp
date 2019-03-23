Handmade Hero Day 245 - Using wglChoosePixelFormatARB

Summary:
trying to get sRGB mode to work on the destination framebuffer by calling the 
extended wglChoosePixelFormatARB(); function.


Keyword:
OpenGL initalization


1:30
Casey talking about what Pixel Buffer objects(PB); are.

PBO is something that Casey doesnt use very often. They are kind of old, they have been around for a very time.
they are not a new feature, but they are still potentially a relevant feature. 

so here is the model, so the box on the left is all CPU memory. they are both main memory in the machine.
main memory in the machine is not necessarily all the same. the reason is because certain parts of the memory in the machine, 
may or may not be accessible to the GPU over the PCI bus.

for example, lets say when the GPU is trying to read some textures out of our main system memory and bring it down to the card, 
so when the GPU wants to access some memory that is in the main system memory,  it has to use physical address instead of virtual address
what that means while its grabbing texture, it cant be something that is being vritually addressed somewhere by the CPU 
in a way that can be moved around. 

One thing about Virtual memory is that we dont even know our memory is resident. the OS could just take our memroy and page it out to disk.
and then only when we go to access it again and cause a page fault, then it will bring it back. 

or if it chooses not to do that, it can choose to rearrange the memory in physical space if it wanted to do, and swap where our virtual
pages were pointed to in virtual space. 


        ----------------------------------------------
        |                                            |
        |    CPU Memory              GPU Visible     |               GPU Memory        
        |                              Memory        |
        |    ___________              ___________    |               ___________                                              
        |   |           |            |           |   |              |           |                                            
        |   |           |            |           |   |              |           |                                    
        |   |           |            |           |   |              |           |                                              
        |   |           |            |           |   |              |           |                                         
        |   |           |            |           |   |  PCI bus     |           |                                         
        |   |           |            | _________ |   |==============| _________ |                                          
        |   |           |            ||    B    ||   |==============||    B    ||                                    
        |   |           |            ||_________||   |              ||_________||                                           
        |   |___________|            |           |   |              |           |                                    
        |                            |           |   |              |           |                                    
        |                            |           |   |              |           |                                    
        |                            |           |   |              |           |                                    
        |                            |           |   |              |           |                                    
        |                            |___________|   |              |___________|                                    
        |--------------------------------------------|


so what that means is that during the transfer of the texture, the whole time, the texture has to live in 
memory that is visible to the GPU, so it has to be in physical memory, and it cant be paged out to disk.
so it has to be locked down. the OS has to know that it is in the middle of a transfer, so it can not decide 
to evict that page or pages of memory, from physical memory into virtual memory that is stored on disk, because 
that will ruin the transfer and the GPU will get garbage. 

7:05
so what does that mean to us in practice?

it means if we have a call like glTexImage(ptr);, and we pass a virtual addressing pointer of texture memory into that function, 
and that texture memory is allocated by us. It might have non of the requirements that the GPU requires. we have not told 
the OS to lock it down, it is not necessarily in the range of memory that is accessible to the GPU, if there are range restrictions 
on what part of the physical memory it can access. (for example, it may allow the low 4 gb of the full 16 gb of memory);

or even maybe the gpu memory access has to be aligned. WHo knows.

none of that true with our own pointer of texture memory. so what the driver has to do first is copy it from 
the memory where we loaded it, into some region of driver controled GPU-visible CPU memory, that is set up for the GPU to read it
and then it copies it accross the PCI bus. 

        ----------------------------------------------
        |                                            |
        |    CPU Memory              GPU Visible     |               GPU Memory        
        |                              Memory        |
        |    ___________              ___________    |               ___________                                              
        |   |           |            |           |   |              |           |                                            
        |   |           |            |           |   |              |           |                                    
        |   |           |            |           |   |              |           |                                              
        |   |           |            |           |   |              |           |                                         
        |   | _________ |            |           |   |  PCI bus     |           |                                         
        |   ||    B    ||            | _________ |   |==============| _________ |                                          
        |   ||_________|| =========> ||    B    ||   |==============||    B    ||                                    
        |   |           |            ||_________||   |              ||_________||                                           
        |   |___________|            |           |   |              |           |                                    
        |                            |           |   |              |           |                                    
        |                            |           |   |              |           |                                    
        |                            |           |   |              |           |                                    
        |                            |           |   |              |           |                                    
        |                            |___________|   |              |___________|                                    
        |--------------------------------------------|


so when one of the people who asked about wheter texture transfer being optimized by PBO,
Casey says he believes that PBO doesnt help you do texture transfers to the GPU at all. 

they are completely unrelated and would not optimize GPU transfer at all. 

but what they would optimize is the copy from CPU memory to driver controled GPU-visible CPU memory.
So if you find that you need to further optimize your texture transfers, and you find that CPU memory bandwidth
has became a bottleneck for you, then that absolutely that FBO can help you. 

of course, that depends on the card, the driver, the vendor, the OS. whether that PBO is better is not for certain. 


14:27
now back to cleaning up our openGL initalization code in setting sRGB mode on the destination framebuffer. 
so just to recap, we were trying to specify that we want sRGB on our destination framebuffer.
In order to do that, you cant use the old OpenGL way which is using the ChoosePixelFormat(); function 


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

    --------------->    SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
                    }

this ChoosePixelFormat(); way goes through GDI instead of OpenGL. 
(GDI is like an openGL, its an API. its the original Windows 2D graphics interface);


so what we have to do is to call this extended wglChoosePixelFormatARB(); as what we did in last episode.

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
    ----------->    wglChoosePixelFormatARB(WindowDC, IntAttribList, FloatAttribList, 1, 
                        &SuggestedPixelFormatIndex, &ExtendedPick);
                }

this allows us to specify a lot more different things 




15:04
so one thing about our current code in Win32InitOpenGL();
wglGetProcAddress(); doesnt work until you have a openGL context. so we need to move it down behind the 
wglCreateContext(); and wglMakeCurrent(); call. 

                internal HGLRC Win32InitOpenGL(HDC WindowDC)
                {
                    int SuggestedPixelFormatIndex = 0;
                    GLuint ExtendedPick = 0;

                    ...
                    ...
                    wgl_choose_pixel_format_arb *wglChoosePixelFormatARB = 
                        (wgl_choose_pixel_format_arb *)wglGetProcAddress("wglChoosePixelFormatARB");
                    if(wglChoosePixelFormatARB)
                    {
                        ...
                        ...
                        float FloatAttribList[] = {0};
                        wglChoosePixelFormatARB(WindowDC, IntAttribList, FloatAttribList, 1, 
                            &SuggestedPixelFormatIndex, &ExtendedPick);
                    }

                    if(ExtendedPick == 0)
                    {
                        ...
                        ...

                        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
                    }

                    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
                    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
                    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);

                    HGLRC OpenGLRC = wglCreateContext(WindowDC);
                    if(wglMakeCurrent(WindowDC, OpenGLRC))        
                    {
                        ................................................
                        ......... wglCreateOpenGLContext ...............
                        ................................................
                    }
                    else
                    {
                        InvalidCodePath;
                        // TODO(casey): Diagnostic
                    }

                    return(OpenGLRC);
                }


however the thing is that when we move it down, we would have already set the pixel format and we 
would have already called wglMakeCurrent


29:08
4coder, the editor Casey was using crashed. 

so Casey mentioned that when an application crahes, if its an alpha version (early development of a product);, 
its very helpful if you can take a moment, since you are programmer, to report it since if they dont have any 
automated crash reporting mechanism 



34:50
so Casey settled with the following structure 

we first try to load the glExtension, and see if we get aa function pointer 

                internal void
                Win32LoadWGLExtensions(void)
                {

                    if(RegisterClassA(&WindowClass))
                    {
                        ...
                        ...

                        HDC WindowDC = GetDC(Window);
                        Win32SetPixelFormat(WindowDC);
                        HGLRC OpenGLRC = wglCreateContext(WindowDC);
                        if(wglMakeCurrent(WindowDC, OpenGLRC))        
                        {
                            wglChoosePixelFormatARB = 
                                (wgl_choose_pixel_format_arb *)wglGetProcAddress("wglChoosePixelFormatARB");

                                ...
                                ...
                        }
                    }
                }



then if we got a valid function pointer, inside Win32SetPixelFormat, we actually choose 
our pixel format 


                internal void Win32SetPixelFormat(HDC WindowDC)
                {
                    int SuggestedPixelFormatIndex = 0;
                    GLuint ExtendedPick = 0;
                    if(wglChoosePixelFormatARB)
                    {
                        int IntAttribList[] =
                        {
                            ...
                            ...
                        };
                        
                        float FloatAttribList[] = {0};
                        
                        wglChoosePixelFormatARB(WindowDC, IntAttribList, FloatAttribList, 1, 
                            &SuggestedPixelFormatIndex, &ExtendedPick);
                    }
                    
                    if(!ExtendedPick)
                    {
                        ...
                        ...
                        
                        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
                    }

                    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
                    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
                    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
                }



52:22
so now we as we have gotten gl extension logic all cleaned up, we now like to add the sRGB IntAttribList 
to our pixel format 
so we want to enable WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB

                internal void Win32SetPixelFormat(HDC WindowDC)
                {
                    int SuggestedPixelFormatIndex = 0;
                    GLuint ExtendedPick = 0;
                    if(wglChoosePixelFormatARB)
                    {
                        int IntAttribList[] =
                        {
                            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
                            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                            WGL_DOUBLE_BUFFER_ARB, GL_FALSE,
                            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
    ------------------->    WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
                            0,
                        };
                        
                        float FloatAttribList[] = {0};
                        
                        wglChoosePixelFormatARB(WindowDC, IntAttribList, FloatAttribList, 1, 
                            &SuggestedPixelFormatIndex, &ExtendedPick);
                    }
                    ...
                    ...
                }









54:55
So Casey got it to work, but on the stream, its completely black on the stream.
this is because we are passing double buffer equals true. and it is siliy smooth on his machine. 

so if Casey wants to stream it, we have to put a false on the WGL_DOUBLE_BUFFER_ARB.

                internal void Win32SetPixelFormat(HDC WindowDC)
                {
                    int SuggestedPixelFormatIndex = 0;
                    GLuint ExtendedPick = 0;
                    if(wglChoosePixelFormatARB)
                    {
                        int IntAttribList[] =
                        {
                            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
                            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                #if HANDMADE_STREAMING
                            WGL_DOUBLE_BUFFER_ARB, GL_FALSE,
                #else
                            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                #endif
                            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
    ------------------->    WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
                            0,
                        };
                        
                        float FloatAttribList[] = {0};
                        
                        wglChoosePixelFormatARB(WindowDC, IntAttribList, FloatAttribList, 1, 
                            &SuggestedPixelFormatIndex, &ExtendedPick);
                    }
                    ...
                    ...
                }


1:05:45
is the GPU doing pre-multipled alpha?

this is the wrong way to think about it if that makes sense.
the GPU doesnt know whether if it s doing pre pre-multipled alpha or not. Its just whether you set your textures that way.
and then you set your blend mode whatever mode that will work out. 

so for us when we set our blend mode, we did glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); since we are doing pre-multipled alpha 


                internal void OpenGLRenderCommands(game_render_commands *Commands, s32 WindowWidth, s32 WindowHeight)
                {    
                    glViewport(0, 0, Commands->Width, Commands->Height);

                    glEnable(GL_TEXTURE_2D);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                    ...
                    ...
                }

whereas if we dont do pre-multipled mode, we would have 

                
                glBlendFunc(GL_SOURCE_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

recall that this the arguments is the multiplier. Since we are currently in pre-multipled mode, we already have 
the alpha component, so we just set GL_ONE to be the multiplier.

other than that, nothing else in OpenGL needs ot know about it, because pre-multipled alpha is just you do to textures.
everything you do on the GPU, it doest know whether its pre-multipled or not. 

when it does such as blending or bilinear blending, its just doing to the values. it doesnt know if its pre-multipled or not.
only when it gets to the last stage of blending, then the blend func comes into play. 


1:09:07
could we do sRGB in shaders?

yes and no. yes since shaders are very programmable these days, but it will be very inefficient. 
the cards has a color look up table in it that will do the sRGB conversions for you properly, or they got 
little circuit designed to just do this. Hardware people had it figured it out. 

whereas we do the sRGB conversion, either we have to upload a texture and use the texture as a lookup texture 
to do the sRGB, or we have to write the code to do it, which is gonna be more expensive. 

so basically there really isnt a case for you to do it unless you have a good reason. 
