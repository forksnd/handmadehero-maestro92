Handmade Hero Day 256 - XBox Controller Stalls and Fixing GL Blit Gamma

Summary:
explained how swap_buffers works underneath, where the CPU and GPU has a one frame delay.

pointed out in our profiling results that we are spending 2 million cycles in Input processing,
which is extremely unusual. Turns out its a microsoft bug with xbox controller

also fixed a small bug with software renderer + openGL rendering pass 

Keyword:
profiling 


5:19
so just to recap how our profiling works 
in our GAME_UPDATE_AND_RENDER function, the TIMED_FUNCTION will give us a DebugType_BeginBlock event 



                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    Platform = Memory->PlatformAPI;    
                    
                    ...
                    ...

                    TIMED_FUNCTION();

                    ...
                    ...
    --------->  }



then the closing bracket gives us the DebugType_EndBlock debug event. 


5:40
in the DebugType_BeginBlock, what we are doing is that we look to see if we already have 
an open profile block on this thread.

its very important for us to do it per thread since we may have multiple threads executing at once.

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray)
                {  
                    ...
                    ...

                    case DebugType_BeginBlock:
                    {
                        ++DebugState->CollationFrame->ProfileBlockCount;
                        debug_element *Element = 
                            GetElementFromEvent(DebugState, Event, DebugState->ProfileGroup, false);
                         
                        debug_stored_event *ParentEvent = DebugState->CollationFrame->RootProfileNode;
                        u64 ClockBasis = DebugState->CollationFrame->BeginClock;
    --------------->    if(Thread->FirstOpenCodeBlock)
                        {
                            ParentEvent = Thread->FirstOpenCodeBlock->Node;
                            ClockBasis = Thread->FirstOpenCodeBlock->OpeningEvent->Clock;
                        }
                        else if(!ParentEvent)
                        {
                            ...
                            ...
                        }
                    }
                }


7:58
the DebugState->CollationFrame->RootProfileNode is the root profiling node for the whole frame  






15:37
Casey mentioned how he does the coloring for the profiling view

the idea is that debug_element has a "char* GUID", a pointer to its GUID,
we literatlly just convert that pointer value to an integer and mod it by the length of the color array

                handmade_debug.cpp

                internal void DrawProfileIn(debug_state *DebugState, rectangle2 ProfileRect, v2 MouseP,
                    debug_stored_event *RootEvent)
                {

                    ...
                    ...

                    v3 Colors[] =
                    {
                        {1, 0, 0},
                        {0, 1, 0},
                        {0, 0, 1},
                        {1, 1, 0},
                        {0, 1, 1},
                        {1, 0, 1},
                        {1, 0.5f, 0},
                        {1, 0, 0.5f},
                        {0.5f, 1, 0},
                        {0, 1, 0.5f},
                        {0.5f, 0, 1},
                        {0, 0.5f, 1},
                    };

                    for(debug_stored_event *StoredEvent = RootNode->FirstChild;
                        StoredEvent;
                        StoredEvent = StoredEvent->ProfileNode.NextSameParent)
                    {
    --------------->    debug_element *Element = Node->Element;
                        ...
                        ...

                        v3 Color = Colors[PointerToU32(Element->GUID)%ArrayCount(Colors)];

                        ...
                        ...
                    }


the problem is that, if we look at the distribution of PointerToU32(Element->GUID)%ArrayCount(Colors);
we wont get very even distribution.

the reason is because a pointer has nothing int he low bits 

a pointer is usually gonna be aligned to a 4 byte boundary or 16 byte (if you have sse sized things, depending on how the 
    allocator is going);


since we either have 4 byte alinged or 16 byte.

so the values are gonna be 
                0x00000000
                0x00000004
                0x00000008
                0x0000000C
                0x00000010
                0x00000014

assuming you have an array of 12 colors, 
                0 mod 12 = 0
                4 mod 12 = 4
                8 mod 12 = 8
                C mod 12 = 0
                ...
                ...

so thye PointerToU32(Element->GUID)%ArrayCount(Colors) will only give you the 0, 4, 8 indices 




so Casey changed it to 11 colors, which is a prime number 

                0 mod 11 = 0
                4 mod 11 = 4
                8 mod 11 = 8
                C mod 11 = 1
                ...
                ...

and you can see, we got more colors 



18:17
Casey analyzing our profile results.
the big green rect is us waiting for our vertical refresh 

also noticed the pink rect, which is our our input processing. turns out our input processing is taking a while
(more on this later);


29:00
Casey showing the profile results during running the game (instead of running just the cut scenes);

Casey mentioned that during the frame display time, we dont actually know how much of that time is the graphics card 
actually doing work, and how much of that is waiting for v-sync



so visually it looks like 


     _______________________________________________________________
    |       |                  |          |                         |
    |       |                  |          |  Frame                  |
    |       |                  |          |   Display               |
    |       |                  |          |                         |
    |_______|__________________|__________|_________________________|
                                          ^
                                          |

                                      swap buffer       

so at the beginning of the Frame Display section, is where we called swap buffer
its sort of wrong to think about it has a sequential linear thing, where the graphics card draws the frame 
then waits 
     ________________________________________________________________
    |       |                  |          |                          |
    |       |                  |          |  Frame                   |
    |       |                  |          |   Display                |
    |       |                  |          |                          |
    |_______|__________________|__________|__________________________|
                                          |            |             |
                                          |            |             |
                                          |-- draws ---|--- wait ----|


                           

what actually happens is that imagine you have frame 4, 5 and 6

                        
                frame 5                                              frame 6
                                                |                                             |
                                                |                                             |
     ___________________________________________|  ___________________________________________|   
    | |   |          |                          | | |   |          |                          |
    | |   |          |  Frame                   | | |   |          |  Frame                   |
    | |   |          |   Display                | | |   |          |   Display                |
    | |   |          |                          | | |   |          |                          |
    |_|___|__________|__________________________| |_|___|__________|__________________________|
                                                |                                             |
                                                |                                             |
                                                |                                             |

                     ------------------------------------------------------------------------>



the Frame Display section in frame 5, where we are waiting for frames is actually waiting for frame 4 to display 

then when we kick off a swap buffer, that is actaully kicking off a frame, to be displayed on frame 6
so the graphics card may be one frame a head of you, if it wants to be, sometimes maybe 2 frames ahead of you.



32:25 
so dont assume that when you call swap buffers, it will draw and then wait to display frame 5 at the end of frame 5.

            frame 5

     ___________________________________________
    | |   |          |                          |
    | |   |          |  Frame                   |
    | |   |          |   Display                |
    | |   |          |                          |
    |_|___|__________|__________________________|
                     |            |             |
                     |            |             |
                     |-- draws ---|--- wait ----|



what may happen is that, when you call swap buffers, spends the time drawing frame 5 across the two frames 
displays frame 4, the previous frame, in middle of drawing frame 5, 

and then waits for you to submit the next swap buffers (the one from frame 6);

then eventually display frame 5 and the end of frame 6


                frame 5                                              frame 6
                                                |                                             |
                                                |                                             |
     ___________________________________________|  ___________________________________________|   
    | |   |          |                          | | |   |          |                          |
    | |   |          |  Frame                   | | |   |          |  Frame                   |
    | |   |          |   Display                | | |   |          |   Display                |
    | |   |          |                          | | |   |          |                          |
    |_|___|__________|__________________________| |_|___|__________|__________________________|
                     |                                       |                                |
                     |                                       |                                |
                     |---------------- draws ----------------|------------- wait -------------|


essentially, the CPU and GPU are not two things that operate in lock step.
its two processors that can operate asynchronously. And then they may decide to stagger their operations 


so you can see that we are displaying frame 5 at the end of frame 6. So we are adding a frame of latency, which means
each frame gets more time to do work at both the CPU and the GPU with out reducing the frame rate. 





34:53
Casey mentioned that right now we are only displaying the top level profiling results.
and the first thing we noticed is that, Input Processing is taking us 2 million cycles

41:00
turns out that the xbox controller is taking 2 million cycles to poll. 


                        DWORD MaxControllerCount = XUSER_MAX_COUNT;
                        if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                        {
                            MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
                        }

                        for (DWORD ControllerIndex = 0;
                             ControllerIndex < MaxControllerCount;
                             ++ControllerIndex)
                        {
                            DWORD OurControllerIndex = ControllerIndex + 1;
                            game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
                            game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

                            XINPUT_STATE ControllerState;
                            if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                            {
                                ...
                                ...
                                ...

                                real32 Threshold = 0.5f;
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageX < -Threshold) ? 1 : 0,
                                    &OldController->MoveLeft, 1,
                                    &NewController->MoveLeft);
                                ...
                                ...

                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->ActionDown, XINPUT_GAMEPAD_A,
                                                                &NewController->ActionDown);
                                Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                &OldController->ActionRight, XINPUT_GAMEPAD_B,
                                                                &NewController->ActionRight);
                                ...
                                ...
                            }
                            else
                            {
                                // NOTE(casey): The controller is not available
                                NewController->IsConnected = false;
                            }
                        }



the question is of course, why.....


what is funny is that we are spending 2 million cycles to not get xbox controller input
since we dont even have an xbox controller plugged in. 


42:20
apparently this is a well known bug with xbox controller. no idea why we never fixed it. 
when you call XInputGetstate, if you have an xbox controller plugged in, it will operate efficiently. 

but if there isnt one plugged, in, it will take tons of time just to tell you we dont have an xbox controller plugged in..


43:30
there is various ways that you can work around this bug. 
one thing you can do is that is a simple fix, but not perfect, is to only poll 1 xbox controller per frame, if its unplugged.

so for all the xbox controller that are plugged in, we poll every frame. For the ones that arent plugged in,
we poll only one every frame. 

and that is what we do. 



51:13
Casey went on to fix another bug where when we do our software rendering pass, but blit it through OpenGL it looks weird

so in our OpenGLDisplayBitmap function, our frame buffer is now SRGB, but the texture we are submitting is linear RGB 
                                                 
                                                |           
                                                |
                                                v
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0,
                             GL_BGRA_EXT, GL_UNSIGNED_BYTE, Memory);


-   full code below:
                handmade_opengl.cpp

                inline void
                OpenGLDisplayBitmap(s32 Width, s32 Height, void *Memory, int Pitch,
                                    s32 WindowWidth, s32 WindowHeight)
                {
                    Assert(Pitch == (Width*4));
                    glViewport(0, 0, Width, Height);

                    glBindTexture(GL_TEXTURE_2D, 1);

                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0,
                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Memory);

                    ...
                    ...
                }


so casey fixed it by doing changing the format the texture to GL_SRGB8_ALPHA8


                inline void
                OpenGLDisplayBitmap(s32 Width, s32 Height, void *Memory, int Pitch,
                    s32 WindowWidth, s32 WindowHeight, GLuint BlitTexture)
                {
                                                        |
                                                        |
                                                        v
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, Width, Height, 0,
                                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Memory);
                    ...
                    ...
                }