Handmade Hero Day 225 - Fading In and Out from the Windows Desktop

Summary:
created a state machine to implement fading in and fading out from the windows desktop
created a second window to implement the fading by chaging its alpha value 

mentioned that we created a second window intentionally instead of fading the main window 
because if we set the Layered flag on the main window, the performance decreases dramatically.

implemented fading between scenes as well

Keyword:
cutscene


1:35
Casey finished up the FadeOut(); function 

                win32_handmade.cpp

                int CALLBACK WinMain()
                {
                    FadeOut(Instance);

                    ...
                    ...
                }



2:29
Casey mentioned that there are multiple ways of doing this. The way that he will do it here is to 
create a full screen window that is transparent but black. We then gradually increase its opacity until it blocks the background.

so as you can see below, we have a for loop which we go from alpha 0 to alpha 255, which will block our screen
and everytime we display a black screen, we sleep for a bit. 

                win32_handmade.cpp

                internal void FadeOut(HINSTANCE Instance)
                {
                    WNDCLASSA WindowClass = {};

                    ...
                    ...

                    if(RegisterClassA(&WindowClass))
                    {
                        HWND Window = CreateWindowExA(...);

                        if(Window)
                        {
                            ToggleFullScreen(Window);

                            ShowWindow(Window, SW_SHOW);

                            for(u32 AlphaLevel = 0; AlphaLevel <= 255; ++AlphaLevel)
                            {
                                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), (BYTE)AlphaLevel, LWA_ALPHA);
                                Sleep(16);
                            }
                        }
                    }
                }


5:20
one problem with this approach is that the sleep(); call is not a reasonable way to do timing. its not guranteed amount of time 
lets say we want to do 60 frames per second, we cant just put sleep(16); milli-seconds.

also it will different from machine to machine. 

so the idea is that we want to call this routine in our main clock instead of at right at the beginning of the WinMain(); function. 







8:45
So Casey refactored it into the following function:

-   IsWindowVisible(); is a windows api

                win32_handmade.cpp

                internal void SetFadeAlpha(HWND Window, r32 Alpha)
                {
                    BYTE WindowsAlpha = (BYTE)(Alpha*255.0f);
                    if(Alpha == 0)
                    {
                        if(IsWindowVisible(Window))
                        {
                            ShowWindow(Window, SW_HIDE);
                        }
                    }
                    else
                    {
                        SetLayeredWindowAttributes(Window, RGB(0, 0, 0), WindowsAlpha, LWA_ALPHA);
                        if(!IsWindowVisible(Window))
                        {
                            ShowWindow(Window, SW_SHOW);
                        }
                    }
                }





10:11
our initial approach is to have a FadeAlpha variable, and the FadeAlpha has to reach 1.0 before anything else begins.



                win32_handmade.cpp

                int CALLBACK WinMain()
                {
                    ...
                    ...

                    r32 FadeAlpha = 0.0f;

                    while(GlobalRunning)
                    {

                        if(FadeAlpha < 0.0f)
                        {
                            FadeAlpha = 0.0f;
                        }
                        else if (FadeAlpha > 1.0f)
                        {
                            FadeAlpha = 1.0f;
                        }
                        

                        if(FadeAlpha == 1.0f)
                        {
                            ShowWindow(Window, SW_SHOW);
                            SetFadeAlpha(FadeWindow, 0.0f);
                        }
                        else
                        {
                            FadeAlpha += NewInput->dtForFrame;
                            SetFadeAlpha(FadeWindow, FadeAlpha);
                        }

                        ...
                        ...

                    }
                }

14:52
so now we have another problem. As we finish the fade, we see a brief flash in the change over period 





39:11
Casey essentially went on to implement a state machine to implement Fading in and fading out of the state machine 

first Casey implemented a handle for this fading effect

                win32_handmade.h

                enum win32_fader_state
                {
                    Win32Fade_FadingIn,
                    Win32Fade_WaitingForShow,
                    Win32Fade_Inactive,
                    Win32Fade_FadingGame,
                    Win32Fade_FadingOut,
                    Win32Fade_WaitingForClose,
                };
                struct win32_fader
                {
                    win32_fader_state State;
                    HWND Window;    
                    r32 Alpha;
                };




in the main game loop, we call the UpdateFade(); function 

                while(GlobalRunning)
                {
                    //
                    //
                    //
                    
                    BEGIN_BLOCK(ExecutableRefresh);
                    NewInput->dtForFrame = TargetSecondsPerFrame;

                    if(UpdateFade(&Fader, NewInput->dtForFrame, Window) == Win32Fade_WaitingForClose)
                    {
                        GlobalRunning = false;
                    }
                    
                }





the UpdateFade(); is where we update all the states. Should be pretty straightforward.

                win32_handmade.cpp

                internal win32_fader_state UpdateFade(win32_fader *Fader, r32 dt, HWND GameWindow)
                {
                    switch(Fader->State)
                    {
                        case Win32Fade_FadingIn:
                        {
                            if(Fader->Alpha >= 1.0f)
                            {
                                SetFadeAlpha(Fader->Window, 1.0f);
                                ShowWindow(GameWindow, SW_SHOW);
                                InvalidateRect(GameWindow, 0, TRUE);
                                UpdateWindow(GameWindow);
                                
                                Fader->State = Win32Fade_WaitingForShow;
                            }
                            else
                            {
                                SetFadeAlpha(Fader->Window, Fader->Alpha);
                                Fader->Alpha += dt;
                            }
                        } break;

                        case Win32Fade_WaitingForShow:
                        {
                            SetFadeAlpha(Fader->Window, 0.0f);
                            Fader->State = Win32Fade_Inactive;
                        } break;

                        case Win32Fade_Inactive:
                        {
                            // NOTE(casey): Nothing to do.
                        } break;

                        case Win32Fade_FadingGame:
                        {
                            if(Fader->Alpha >= 1.0f)
                            {
                                SetFadeAlpha(Fader->Window, 1.0f);
                                ShowWindow(GameWindow, SW_HIDE);                
                                Fader->State = Win32Fade_FadingOut;
                            }
                            else
                            {
                                SetFadeAlpha(Fader->Window, Fader->Alpha);
                                Fader->Alpha += dt;
                            }
                        } break;
                        
                        case Win32Fade_FadingOut:
                        {
                            Fader->Alpha -= dt;
                            if(Fader->Alpha <= 0.0f)
                            {
                                SetFadeAlpha(Fader->Window, 0.0f);
                                Fader->State = Win32Fade_WaitingForClose;
                            }
                            else
                            {
                                SetFadeAlpha(Fader->Window, Fader->Alpha);
                            }
                        } break;

                        case Win32Fade_WaitingForClose:
                        {
                            // NOTE(casey): Nothing to do.
                        } break;

                        default:
                        {
                            Assert(!"Unrecognized fader state!");
                        } break;
                    }

                    return(Fader->State);
                }




then we have these few functions. 

                internal void SetFadeAlpha(HWND Window, r32 Alpha)
                {
                    BYTE WindowsAlpha = (BYTE)(Alpha*255.0f);
                    if(Alpha == 0)
                    {
                        if(IsWindowVisible(Window))
                        {
                            ShowWindow(Window, SW_HIDE);
                        }
                    }
                    else
                    {
                        SetLayeredWindowAttributes(Window, RGB(0, 0, 0), WindowsAlpha, LWA_ALPHA);
                        if(!IsWindowVisible(Window))
                        {
                            ShowWindow(Window, SW_SHOW);
                        }
                    }
                }

                internal void InitFader(win32_fader *Fader, HINSTANCE Instance)
                {
                    WNDCLASSA WindowClass = {};

                    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
                    WindowClass.lpfnWndProc = Win32FadeWindowCallback;
                    WindowClass.hInstance = Instance;
                    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
                    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
                    WindowClass.lpszClassName = "HandmadeFadeOutWindowClass";

                    if(RegisterClassA(&WindowClass))
                    {
                        Fader->Window =
                            CreateWindowExA(
                                WS_EX_LAYERED,//|WS_EX_TOPMOST,
                                WindowClass.lpszClassName,
                                "Handmade Hero",
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                0,
                                0,
                                Instance,
                                0);
                        if(Fader->Window)
                        {
                            ToggleFullscreen(Fader->Window);
                        }
                    }
                }

                internal void BeginFadeToGame(win32_fader *Fader)
                {
                    Fader->State = Win32Fade_FadingIn;
                    Fader->Alpha = 0.0f;
                }

                internal void BeginFadeToDesktop(win32_fader *Fader)
                {
                    if(Fader->State == Win32Fade_Inactive)
                    {
                        Fader->State = Win32Fade_FadingGame;
                        Fader->Alpha = 0.0f;
                    }
                }




then whenever we have the QuitRequested triggered, we call BeginFadeToDesktop();

                if(!GlobalPause)
                {
                    ...
                    ...

                    if(Game.UpdateAndRender)
                    {
                        Game.UpdateAndRender(&GameMemory, NewInput, &Buffer);
                        if(GameMemory.QuitRequested)
                        {
                            BeginFadeToDesktop(&Fader);
                        }
                    }
                }


Q/A
1:00:26
why are we using a second window instead of fading the main window?

that is what Casey would have done a couple months ago. However, Casey was experimenting with layers and transparency and found out that 
if you set windowed layered flag on your window, your performance goes down very bad. so you dont ever want to set the layered flag 
on your window if you want to do high performance graphics. 


1:08:54
if we want to fade between in game scenes, will that still involve the windows API?

no, we are using the windows API cuz we are fading in and out of the desktop. Once we are in our own game code, we can just do all of this 
in our own renderer  


1:10:00
so if you want to fade something to black for our layered scenes, what we can do is to 

we can add a variable in the layered_scene. This will indicate how long it will take the scene to fade in

                handmade_cutscene.h

                struct layered_scene
                {
                    asset_type_id AssetType;
                    u32 ShotIndex;
                    u32 LayerCount;
                    scene_layer *Layers;

                    r32 Duration;
                    v3 CameraStart;
                    v3 CameraEnd;

    ------------>   r32 tFadeIn;
                };



so in the RenderLayeredScene function, we change the SceneFadeValue.


-   notice we set the color to be 

                v4 Color = {SceneFadeValue, SceneFadeValue, SceneFadeValue, 1.0f};

    we are not making it transparent, we are making it black. this Color is like the shading color 
    when we start SceneFadeValue at 0, we get black. once it gets to 1, it will be at full color 


-   full code below:

                handmade_cutscene.cpp

                internal void RenderLayeredScene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, layered_scene *Scene, r32 tNormal)
                {
                    // TODO(casey): Unify this stuff?
                    real32 WidthOfMonitor = 0.635f; // NOTE(casey): Horizontal measurement of monitor in meters
                    real32 MetersToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;
                    real32 FocalLength = 0.6f;

                    r32 SceneFadeValue = 1.0f;
                    if(tNormal < Scene->tFadeIn)
                    {
                        SceneFadeValue = Clamp01MapToRange(0.0f, tNormal, Scene->tFadeIn);
                    }

                    v4 Color = {SceneFadeValue, SceneFadeValue, SceneFadeValue, 1.0f};

                    ...
                    ...


                    PushBitmap(RenderGroup, LayerImage, Layer.Height, V3(0, 0, 0), Color);
                }