Handmade Hero Day 226 - Handling Multiple Metagame Modes

Summary:
organized the code so that we have different game modes: the game mode, the cutscene mode, and the title screen mode

organized memory arena for different game modes.

implemented game mode transitions. [not fully completed, will be finished in tomorrow_s episode]


Keyword:
game modes



6:20
so Casey mentioned that we can now switch between the game mode and the attract mode (the mode where we are playing all the scenes);.

and when we return to the attract mode, we dont want to resume where we left off playing the cutscenes. we want to start over




11:38
So Casey added the game_mode enum 

                handmade.h 

                enum game_mode
                {
                    GameMode_TitleScreen,
                    GameMode_CutScene,
                    GameMode_World,
                };


12:59
then in our game_state, we just added our game_mode variable 
you might also see that we have a union in the game_state as well. 
each of the game_mode will want its own storage, or data. so we created three struct 
for all the different game modes to store the relevant data


                handmade.h

                struct game_state
                {
                    bool32 IsInitialized;

                    ...
                    ...

                    game_mode GameMode;
                    union
                    {
                        game_mode_title_screen *TitleScreen;
                        game_mode_cutscene *CutScene;
                        game_mode_world *WorldMode;
                    };
                };



and we defined the others. the playing_cutscene is renamed to game_mode_cutscene

                struct game_mode_cutscene
                {
                    u32 SceneCount;
                    layered_scene *Scenes;
                    r32 t;
                };

                struct game_mode_title_screen
                {
                    r32 t;
                };




and we also pulled all of the things that pertains to our game world from game_state to game_mode_world

                handmade_world.h

                struct game_mode_world
                {
                    world *World;

                    ...
                    ...

                    real32 Time;

                    random_series EffectsEntropy; // NOTE(casey): This is entropy that doesn't affect the gameplay
                    real32 tSine;

                #define PARTICLE_CEL_DIM 32
                    u32 NextParticle;
                    particle Particles[256];
                    
                    particle_cel ParticleCels[PARTICLE_CEL_DIM][PARTICLE_CEL_DIM];
                };





16:50
then now in the GAME_UPDATE_AND_RENDER(); function we add the logic depending on whatever mode we are in.

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    // TODO(casey): Decide what our pushbuffer size is!
                    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), false);
                    BeginRender(RenderGroup);

                    switch(GameState->GameMode)
                    {
                        case GameMode_TitleScreen:
                        {
                            UpdateAndRenderTitleScreen(TranState->Assets, RenderGroup, DrawBuffer,
                                                       GameState->TitleScreen);
                        } break;

                        case GameMode_CutScene:
                        {
                            UpdateAndRenderCutScene(TranState->Assets, RenderGroup, DrawBuffer,
                                                    Input, GameState->CutScene);
                        } break;

                        case GameMode_World:
                        {
                            UpdateAndRenderWorld(GameState->WorldMode, TranState, Input, RenderGroup, DrawBuffer);
                        } break;

                        InvalidDefaultCase;
                    }

                    ...
                    ...
                }



26:17
Casey made another function that will be responsible for changing what game mode we are in 

we first clear out the previous mode, so we just clear the memory_arena 

                handmade.cpp

                internal void SetGameMode(game_state *GameState, game_mode GameMode)
                {
                    Clear(&GameState->ModeArena);
                    GameState->GameMode = GameMode;
                }


recall that we renamed the memory_arena from WorldArena to ModeArena. 
we will be reusing this arena for every game mode. 

                handmade.h

                struct game_state
                {
                    bool32 IsInitialized;

    ------------>   memory_arena ModeArena;
                    memory_arena AudioArena; // TODO(casey): Move this into the audio system proper!

                    ...
                    ...

                    game_mode GameMode;
                    union
                    {
                        game_mode_title_screen *TitleScreen;
                        game_mode_cutscene *CutScene;
                        game_mode_world *WorldMode;
                    };
                };



28:13 
Casey mentioned that clearing an Arena is different from EndTemporaryMemory();
for us, clearing will be the same as initalizing 

                handmade.h

                inline void Clear(memory_arena *Arena)
                {
                    InitializeArena(Arena, Arena->Size, Arena->Base);
                }

