Handmade Hero Day 224 - Prefetching Cutscene Layers

Summary:
prefetched the cutscenes assets. added a black screen as the "loading screen" to prefetch the first scene

Keyword:
cutscene


7:12
followed up the day 223 Q/A where someone asked about why there are imperfect transitions between scene switches 

what we could do is to just render the cutscene twice 

recall that whenever we render something, and we build up the bitmap requests, that is actually when the asset system 
load in the assets for the bitmaps. So what we can do is to render the cutscene twice, one time for prefetching (but we dont actually render);
the other time to actually render a scene

-   notice for the first call for RenderCutsceneAtTime();, we pass in 0 as the RenderGroup. So this is the prefetching call, we pass in an empty 
    RenderGroup and we throw it away. This way we are prefetching the assets, but we dont actually render

                handmade_cutscene.cpp

                internal b32 RenderCutscene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, playing_cutscene *CutScene)
                {
                    RenderCutsceneAtTime(Assets, 0, DrawBuffer, CutScene, CutScene->t + CUTSCENE_WARMUP_SECONDS);    
                    b32 CutsceneComplete = RenderCutsceneAtTime(Assets, RenderGroup, DrawBuffer, CutScene, CutScene->t);
                    if(!CutsceneComplete)
                    {
                        CutScene->t = 0.0f;
                    }
                    return(CutsceneComplete);
                }


then we just add logic in the RenderCutsceneAtTime(); for prefetching 

                handmade_cutscene.cpp

                internal void RenderLayeredScene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, layered_scene *Scene, r32 tNormal)
                {
                    ...
                    ...

                    if(RenderGroup)
                    {
                        Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetersToPixels, FocalLength, 0.0f);
                    }

                    ...
                    ...

                    for(u32 LayerIndex = 1; LayerIndex <= Scene->LayerCount; ++LayerIndex)
                    {
                        ...
                        ...

                        if(Active)
                        {
                            MatchVector.E[Tag_LayerIndex] = (r32)LayerIndex;        
                            bitmap_id LayerImage = GetBestMatchBitmapFrom(Assets, Scene->AssetType, &MatchVector, &WeightVector);

                            if(RenderGroup)
                            {
                                ...
                                ...

                                RenderGroup->Transform.OffsetP.z = P.z - CameraOffset.z;
                        
                                PushBitmap(RenderGroup, LayerImage, Layer.Height, V3(0, 0, 0));
                            }
                            else
                            {
                                PrefetchBitmap(Assets, LayerImage);
                            }
                        }
                    }
                }




12:00
now that we got rid of most of the glitch during transitions, but we still a glitch when we show the first scene 
(so when the game starts up, also when we transition from the last scene to the first scene when we were playing on loop);


13:40
so What Casey do is to put a fake Loading screen, lets say a complete black screen, to preload the first cutscene.

so we added just a WARPUP cut scene

                #define INTRO_SHOT(Index) Asset_OpeningCutscene, Index, ArrayCount(IntroLayers##Index), IntroLayers##Index
                global_variable layered_scene IntroCutscene[] =
                {
    ----------->    {Asset_None, 0, 0, 0, CUTSCENE_WARMUP_SECONDS},
                    {INTRO_SHOT(1), 20.0f, {0.0f, 0.0f, 10.0f}, {-4.0f, -2.0f, 5.0f}},
                    {INTRO_SHOT(2), 20.0f, {0.0f, 0.0f, 0.0f}, {0.5f, -0.5f, -1.0f}},
                    {INTRO_SHOT(3), 20.0f, {0.0f, 0.5f, 0.0f}, {0.0f, 6.5f, -1.5f}},
                    {INTRO_SHOT(4), 20.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -0.5f}},
                    {INTRO_SHOT(5), 20.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.5f, -1.0f}},
                    {INTRO_SHOT(6), 20.0f, {0.0f, 0.0f, 0.0f}, {-0.5f, 0.5f, -1.0f}},
                    {INTRO_SHOT(7), 20.0f, {0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}},
                    {INTRO_SHOT(8), 20.0f, {0.0f, 0.0f, 0.0f}, {0.0f, -0.5f, -1.0f}},
                    {INTRO_SHOT(9), 20.0f, {0.0f, 0.0f, 0.0f}, {-0.75f, -0.5f, -1.0f}},
                    {INTRO_SHOT(10), 20.0f, {0.0f, 0.0f, 0.0f}, {-0.1f, 0.05f, -0.5f}},
                    {INTRO_SHOT(11), 20.0f, {0.0f, 0.0f, 0.0f}, {0.6f, 0.5f, -2.0f}},
                };


17:46
so now when we startup and load the black screen, we still see that blinking white flash
what Casey did is that when we pass in layer 0, which is our black loading screen, we will just clear the screen 

as you can see, when the LayerCount == 0, we just call Clear();

                internal void RenderLayeredScene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, layered_scene *Scene, r32 tNormal)
                {
                    ...
                    ...
                    
                    MatchVector.E[Tag_ShotIndex] = (r32)Scene->ShotIndex;

                    if(Scene->LayerCount == 0)
                    {
                        Clear(RenderGroup, V4(0.0f, 0.0f, 0.0f, 0.0f));
                    }

                    ...
                    ...
                }




19:51
although we clear the render group, we still see a white flicker at startup. 
First Casey suspects that in our win32 layer, we arent initalizing our offscreen buffer to 0.

                win32_handmade.cpp

                internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
                {
                    // TODO(casey): Bulletproof this.
                    // Maybe don't free first, free after, then free first if that fails.

                    if(Buffer->Memory)
                    {
                        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
                    }

                    Buffer->Width = Width;
                    Buffer->Height = Height;

                    int BytesPerPixel = 4;
                    Buffer->BytesPerPixel = BytesPerPixel;

                    // NOTE(casey): When the biHeight field is negative, this is the clue to
                    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
                    // the first three bytes of the image are the color for the top left pixel
                    // in the bitmap, not the bottom left!
                    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
                    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
                    Buffer->Info.bmiHeader.biHeight = Buffer->Height;
                    Buffer->Info.bmiHeader.biPlanes = 1;
                    Buffer->Info.bmiHeader.biBitCount = 32;
                    Buffer->Info.bmiHeader.biCompression = BI_RGB;

                    // NOTE(casey): Thank you to Chris Hecker of Spy Party fame
                    // for clarifying the deal with StretchDIBits and BitBlt!
                    // No more DC for us.
                    Buffer->Pitch = Align16(Width*BytesPerPixel);
                    int BitmapMemorySize = (Buffer->Pitch*Buffer->Height);
                    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
                    // NOTE(casey): VirtualAlloc should _only_ have given us back
                    // zero'd memory which is all black, so we don't need to clear it.
                }


25:56
so Casey intialized a windows black brush that fixes the white flash 

[I suppose that is just windows knowledge]

                win32_handmade.cpp

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {

                    ...
                    ...

                    WNDCLASSA WindowClass = {};
                    
                    Win32ResizeDIBSection(&GlobalBackbuffer, 1920, 1080);
                    
                    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
                    WindowClass.lpfnWndProc = Win32MainWindowCallback;
                    WindowClass.hInstance = Instance;
                    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    ------------>   WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

                    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

                    ...
                    ...
                }




34:12
Casey mentioned that when we switch from cutscenes to the game play, our game assets are not loaded yet, so we want to prefetch 
our game assets in the future




36:08
Casey made it so that if you hit the delete button, that gets rid of the main character

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    if(WasPressed(Controller->Back))
                    {
                        DeleteLowEntity(GameState, ConHero->EntityIndex);
                        ConHero->EntityIndex = 0;
                    }
                }


                internal void DeleteLowEntity(game_state *GameState, int Index)
                {
                    // TODO(casey): Actually delete;
                }





44:29
Casey wants to give the game a more concrete notion of cutscenes.
So Casey defined this playing_cutscene struct

                handmade_cutscene.h

                struct playing_cutscene
                {
                    u32 SceneCount;
                    layered_scene *Scenes;
                    r32 t;
                };



so during game initalization, we intialize the playing_cutscene.

                handmade.cpp

                internal playing_cutscene
                MakeIntroCutscene(void)
                {
                    playing_cutscene Result = {};

                    Result.SceneCount = ArrayCount(IntroCutscene);
                    Result.Scenes = IntroCutscene;

                    return(Result);
                }


                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    Platform = Memory->PlatformAPI;    

                    ...
                    ...

                    if(!GameState->IsInitialized)
                    {                
                        ...
                        ...
                        
                        
                        GameState->CurrentCutscene = MakeIntroCutscene();
                        
                        GameState->IsInitialized = true;
                    }
                }



49:27
we then also add the playing_cutscene in the game_state 

                handmade.h

                struct game_state
                {
                    ...
                    ...

                    playing_cutscene CurrentCutscene;
                };
