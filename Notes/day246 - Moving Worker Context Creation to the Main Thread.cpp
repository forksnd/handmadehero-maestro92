Handmade Hero Day 246 - Moving Worker Context Creation to the Main Thread

Summary:
moved openGL context creation for the background threads to the main thread, then pass it down to background threads. 
did this because some people had some NVIDA driver bugs

mentioned the importance of API designed. mentioned one of his API blog post

Keyword:
OpenGL initialization

0:49
Casey mentioned that we will fix a driver bug today 

1:44
apparently from people who posted on forums, if we test our current code on NVIDA using multiple threads
for downloading textures was not working properly on there.

what they said is that for some reason, if you create all of the openGL context you will use on the main thread,
and then pass them down to the other threads, it works fine.

but if you create the context on the separate threads, it doesnt work. 

Casey says he doesnt know why and he has sent the source code NVIDA. we are still awaiting for an explanation. 


3:11
in general, with PC compatibility, you dont have the luxury of ever really knowing why things dont work a lot of the time
you simply have to accept that if someone runs into a bug on PC, and they almost always do, it has to do with hardware
that is outside of your control, you just have to find a way to work around it. In this case, we just have to create context 
on the main thread and pass it to the background threads.
this way our code will be compliant with the nvidia drivers


4:16
so currently our code looks like:
                
                win32_handmade.cpp 

                internal void Win32CreateOpenGLContextForWorkerThread(void)
                {
                    if(wglCreateContextAttribsARB)
                    {
                        HDC WindowDC = GlobalDC;
                        HGLRC ShareContext = GlobalOpenGLRC;
                        HGLRC ModernGLRC = wglCreateContextAttribsARB(WindowDC, ShareContext, Win32OpenGLAttribs);
                        if(ModernGLRC)
                        {
                            if(wglMakeCurrent(WindowDC, ModernGLRC))
                            {
                                // TODO(casey): Fatal error?
                            }
                            else
                            {
                                Assert(!"Unable to create texture download context");
                            }
                        }
                    }
                }

so the problem is that wglCreateContextAttribsARB(WindowDC, ShareContext, Win32OpenGLAttribs); 
can not be called other than on the main thread. 



5:13
so recall, currently when we start up our threads, we are calling ThreadProc();
and you can see currently, the thread gets some of its infromation from platform_work_queue pointer 
however, the platform_work_queue is shared among all threads. what we need is some way of passing information
that is unique to a thread.


                win32_handmade.cpp

                DWORD WINAPI ThreadProc(LPVOID lpParameter)
                {
    ----------->    platform_work_queue *Queue = (platform_work_queue *)lpParameter;

                    u32 TestThreadID = GetThreadID();
                    Assert(TestThreadID == GetCurrentThreadId());

                    if(Queue->NeedsOpenGL)
                    {
                        Win32CreateOpenGLContextForWorkerThread();
                    }

                    for(;;)
                    {
                        if(Win32DoNextWorkQueueEntry(Queue))
                        {
                            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
                        }
                    }
                }

6:06
so from a previous episode, we somehow left in the following struct 

                win32_handmade.cpp

                struct win32_thread_startup
                {
                    HWND Window;
                    HGLRC OpenGLRC;
                    platform_work_queue *Queue;
                };


now we changed it to the following.
we get all the threads information it needs from the win32_thread_startup

                DWORD WINAPI ThreadProc(LPVOID lpParameter)
                {
                    win32_thread_startup *Thread = (win32_thread_startup *)lpParameter;
                    platform_work_queue *Queue = Thread->Queue;

                    u32 TestThreadID = GetThreadID();
                    Assert(TestThreadID == GetCurrentThreadId());

                    if(Thread->OpenGLRC)
                    {
                        wglMakeCurrent(Thread->OpenGLDC, Thread->OpenGLRC);
                    }

                    for(;;)
                    {
                        if(Win32DoNextWorkQueueEntry(Queue))
                        {
                            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
                        }
                    }

                //    return(0);
                }


18:44
so Casey mentioned that among the IntAttribs, we pass in WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB
Casey was concerned that whether it is legal to pass this argument to the driver if the driver doest support it

                internal void
                Win32SetPixelFormat(HDC WindowDC)
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
                            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
                            0,
                        };
                        
                        float FloatAttribList[] = {0};
                        
                        wglChoosePixelFormatARB(WindowDC, IntAttribList, FloatAttribList, 1, 
                            &SuggestedPixelFormatIndex, &ExtendedPick);
                    }
                    
                    ...
                    ...
                }

so Casey went into the specs and check 
https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
the speec says 

"    An error is generated if <piAttributes> contains an invalid
    attribute, if <iPixelFormat> is not a positive integer or is larger
    than the number of pixel formats, if <iLayerPlane> doesn't refer to
    an existing layer plane, or if <hdc> is invalid."


that suggests that we cant put that argument in if the driver doesnt support it 


21:35
So Casey mentions that there is a difference between OpenGL extensions and wgl extensions 
so if we were to search for WGL_EXT_framebuffer_sRGB, we need to look for wg extensions 

25:07
so Casey proceed to write the code to check for the extensions for it 



                internal void Win32LoadWGLExtensions(void)
                {
                    ...
                    ...

                    if(RegisterClassA(&WindowClass))
                    {
                        ...
                        ...

                        HDC WindowDC = GetDC(Window);
                        Win32SetPixelFormat(WindowDC);
                        HGLRC OpenGLRC = wglCreateContext(WindowDC);
                        if(wglMakeCurrent(WindowDC, OpenGLRC))        
                        {
                            wglChoosePixelFormatARB =  (wgl_choose_pixel_format_arb *)wglGetProcAddress("wglChoosePixelFormatARB");
                            wglCreateContextAttribsARB = (wgl_create_context_attribs_arb *)wglGetProcAddress("wglCreateContextAttribsARB");
                            wglSwapIntervalEXT = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
                            wglGetExtensionsStringEXT = (wgl_get_extensions_string_ext *)wglGetProcAddress("wglGetExtensionsStringEXT");
                        
                            if(wglGetExtensionsStringEXT)
                            {
                                char *Extensions = (char *)wglGetExtensionsStringEXT();
                                char *At = Extensions;
                                while(*At)
                                {
                                    while(IsWhitespace(*At)) {++At;}
                                    char *End = At;
                                    while(*End && !IsWhitespace(*End)) {++End;}

                                    umm Count = End - At;        

                                    if(0) {}
                                    else if(StringsAreEqual(Count, At, "WGL_EXT_framebuffer_sRGB")) {OpenGLSupportsSRGBFramebuffer = true;}

                                    At = End;
                                }
                            }

                            ...
                            ...
                        }
                    }


54:53
Casey mentioned one of his blog post about API design
https://caseymuratori.com/blog_0024

Q/A

1:10:17
someone asked selection buffer vs ray picking vs color picking for mouse picking

Casey prefers ray picking