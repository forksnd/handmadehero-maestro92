Handmade Hero Day 227 - Switching Between Metagame Modes

Summary:
more work on changing game modes

now we can change between title screen, CutScene attract mode and game mode

mentioned a problem that when we change modes, there are background threads still running.
so the proper solution is to finish all the background tasks before we do a proper game mode shut down and transition.

Casey temporarily commented out the background ground chunk generation threads
proper fix will be implemented next episode

Keyword:
game modes


12:43
So Casey added this CheckForMetaInput(); function that will switch between game modes depending on the keyboard button pressed action

                handmade_cutscene.cpp

                internal b32 CheckForMetaInput(game_state *GameState, game_input *Input)
                {
                    b32 Result = false;
                    for(u32 ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
                    {
                        game_controller_input *Controller = GetController(Input, ControllerIndex);
                        if(WasPressed(Controller->Back))
                        {
                            Input->QuitRequested = true;
                            break;
                        }
                        else if(WasPressed(Controller->Start))
                        {
                            PlayWorld(GameState);
                            Result = true;
                            break;
                        }
                    }

                    return(Result);
                }




17:05 
Casey made it so that when we are in other modes, we will handle the input 

                handmade_cutscene.cpp

                internal b32 UpdateAndRenderCutScene(game_state *GameState, game_assets *Assets, render_group *RenderGroup,
                                        loaded_bitmap *DrawBuffer, game_input *Input, game_mode_cutscene *CutScene)
                {
                    b32 Result = CheckForMetaInput(GameState, Input);
                    if(!Result)
                    {        
                        RenderCutsceneAtTime(Assets, 0, DrawBuffer, CutScene, CutScene->t + CUTSCENE_WARMUP_SECONDS);    
                        b32 CutsceneStillRunning = RenderCutsceneAtTime(Assets, RenderGroup, DrawBuffer, CutScene, CutScene->t);
                        if(CutsceneStillRunning)
                        {
                            CutScene->t += Input->dtForFrame;
                        }
                        else
                        {
                            PlayTitleScreen(GameState);
                        }    
                    }

                    return(Result);
                }

                internal b32 UpdateAndRenderTitleScreen(game_state *GameState, game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer,
                                           game_input *Input, game_mode_title_screen *TitleScreen)
                {
                    b32 Result = CheckForMetaInput(GameState, Input);
                    if(!Result)
                    {
                        Clear(RenderGroup, V4(1.0f, 0.25f, 0.25f, 0.0f));
                        if(TitleScreen->t > 10.0f)
                        {
                            PlayIntroCutscene(GameState);
                        }
                        else
                        {
                            TitleScreen->t += Input->dtForFrame;
                        }
                    }

                    return(Result);
                }



19:03 
then in the main loop, we setup the logic to be like the following. As you can see, if UpdateAndRenderTitleScreen();
detects an input to change the game mode, Rerun will set to true and we will just rerun this while loop 

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    b32 Rerun = false;
                    do
                    {
                        switch(GameState->GameMode)
                        {
                            case GameMode_TitleScreen:
                            {
                                Rerun = UpdateAndRenderTitleScreen(GameState, TranState->Assets, RenderGroup, DrawBuffer,
                                                                   Input, GameState->TitleScreen);
                            } break;

                            case GameMode_CutScene:
                            {
                                Rerun = UpdateAndRenderCutScene(GameState, TranState->Assets, RenderGroup, DrawBuffer,
                                                                Input, GameState->CutScene);
                            } break;

                            case GameMode_World:
                            {
                                Rerun = UpdateAndRenderWorld(GameState, GameState->WorldMode, TranState, Input, RenderGroup, DrawBuffer);
                            } break;

                            InvalidDefaultCase;
                        }
                    } while(Rerun);

                    ...
                    ...
                }



27:55
Casey than encouters the issue that when you do mode switching, lets say from world mode to cutscene mode,
we may have background threads generating ground chunks. 

so any threaded work for any mode needs to get 

So Casey temporarily commented out the fill ground chunk on threads code.
He will fix this in the next episode





40:46
Casey mentioned that we have to clear our stuff to world when we start up as we change modes.
so what casey opted to do is that our PushStruct(); function should just clear stuff to 0

so whenever we do PushSize_(); also call ZeroSize();


                handmade.h
    
                inline void * PushSize_(memory_arena *Arena, memory_index SizeInit, memory_index Alignment = DEFAULT_MEMORY_ALIGNMENT)
                {
                    memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Alignment);
                    
                    Assert((Arena->Used + Size) <= Arena->Size);

                    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
                    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
                    Arena->Used += Size;

                    Assert(Size >= SizeInit);

    ------------>   ZeroSize(SizeInit, Result);
                    
                    return(Result);
                }


46:09
So Casey mentioned that we would have to make sure, when we shut down a mode, we dont shut down until we can close down 
all of its tasks, including the threaded tasks.



50:50
wrote code to UpdateAndRenderTitleScreen(); 

                handmade_cutscene.cpp

                internal b32 UpdateAndRenderTitleScreen(game_state *GameState, game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer,
                                           game_input *Input, game_mode_title_screen *TitleScreen)
                {
                    b32 Result = CheckForMetaInput(GameState, Input);
                    if(!Result)
                    {
                        Clear(RenderGroup, V4(1.0f, 0.25f, 0.25f, 0.0f));
                        if(TitleScreen->t > 10.0f)
                        {
                            PlayIntroCutscene(GameState);
                        }
                        else
                        {
                            TitleScreen->t += Input->dtForFrame;
                        }
                    }

                    return(Result);
                }