Handmade Hero Day 228 - Waiting for Dependent Tasks on Metagame Mode Changes

Summary:

fixed the problem of when we switch from games modes to game modes, we may have background threads running certain tasks.
offered two solutions, 
1.  is to completely avoid having tasks as such, 
2.  keep track of these tasks and finish them when we do a proper game mode change 

opted to do the 2nd option
added a flag in the task_with_memory struct, and we now complete all task when we call SetGameMode();

mentioned that right now when we startup our game, it actually takes awhile. mentioned that this happened 
when we added the ClearToZero(); in our memory allocation function, PushSize();

refactored the PushSize(); to take both the memory alignment and the ClearToZero flag.


mentions that there is a need to have the concept of memory arenas flow all the way to the OS level.
Because Casey wants to do some stuff where we can have memory arenas that grow, where the OS gives us more memory.

The reason why we want to do that is becuz, first we may just want to run on the PC that way. 
PC has virutal memory, and people may want to run huge worlds. We can allow them to use all the memory on that machine 
if they want. 

mentions that while the game should run on a fixed memory limit, our debug system should not have that constraint. 
The debug system should have a separate set of memory, and that should be able to run on some 
growable thing, and ask the OS for more memory. Since its the debug system, it will not ship to a users machine. 

[but due to time constraints, Casey didnt do it in this episode]



looked at the sorting problem in rendering. mentioned that we are currently diving the scene into different render tiles 
and we assign a thread to each render tile to render it. 

Casey argues that we can do a first pass on our render_group, and have each thread keep an array of all the render_elements
that falls into its render tiles, then we sort it internally. 

Keyword:
game modes, multithreading, memory, sorting


5:48
So Casey addresses the problem we mentioned in day 227:
when we switch from games modes to game modes, we may have background threads running certain tasks.
so we have to fix that.

just to give some context information:

in the asset system, we have a streaming system where we we dispatch tasks to load assets dynamically. 
this way we dont need loading screens/pauses. But you may ask, why are those tasks not a problem, whereas 
the background ground chunk generation a problem? 

that is becuz the asset system is totally decoupled from any game modes.

so the tasks that we are trying to solve here are tasks that are directly related to data that is relevant 
to one of the game modes 


we actually have another set of tasks, the renderer tasks, which we dont care about either. These tasks 
all get spawned and stopped in the middle of one frame. They never hang around outside of a frame. 



if you recall, in our TiledRenderGroupToOutput(); function, we call Platform.CompleteAllWork(); at the end of the function
[I went back to check the videos, it doesnt seem that Casey really gave an explanation why we call
 CompleteAllWork in the rendering code]

                handmade_render_group.cpp 

                internal void TiledRenderGroupToOutput(platform_work_queue *RenderQueue,
                                         render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {
                    ...
                    ...

                    int WorkCount = 0;
                    for(int TileY = 0; TileY < TileCountY; ++TileY)
                    {
                        for(int TileX = 0; TileX < TileCountX; ++TileX)
                        {
                            tile_render_work *Work = WorkArray + WorkCount++;

                            ...
                            ...                            
                            Platform.AddEntry(RenderQueue, DoTiledRenderWork, Work);
                        }
                    }

                    Platform.CompleteAllWork(RenderQueue);
                }

so the only tasks we actually care about are tasks that accesses the game mode_s mode state, and spans across multiple frames
right now the ground chunk are the only things that are doing this


we have two choices: 
1.  we can just NOT have tasks like that 

2.  we track them in a way that we can close them when a mode is changed. 

Casey says that he doesnt know which one is the better answer cuz we dont know what will we need in the finished game. 


9:35 
So Casey reasoned that he suspects the only tasks that falls into this category might be world generation tasks 
lets say the game starts, and the player is trying to play the game, since Casey never wants to have a loading screen (something 
that will stop the players playing experience); 

so if we want to avoid loading screens, we will have to support world generation on background threads.


11:36
so recall in transient_state, we have our task_with_memory array 

                handmade.h

                struct transient_state
                {
                    bool32 IsInitialized;
                    memory_arena TranArena;    

                    task_with_memory Tasks[4];

                    game_assets *Assets;

                    uint32 GroundBufferCount;
                    ground_buffer *GroundBuffers;
                    platform_work_queue *HighPriorityQueue;
                    platform_work_queue *LowPriorityQueue;

                    uint32 EnvMapWidth;
                    uint32 EnvMapHeight;
                    // NOTE(casey): 0 is bottom, 1 is middle, 2 is top
                    environment_map EnvMaps[3];
                };

all we need to do is tag these tasks whether or not they need to be flushed out in the case the game mode changes. 


12:31
so Casey added a flag in the task_with_memory struct 

                handmade.h

                struct task_with_memory
                {
                    b32 BeingUsed;
    ----------->    b32 DependsOnGameMode;
                    memory_arena Arena;

                    temporary_memory MemoryFlush;
                };


then in the BeginTaskWIthMemory(); function we set the flag whether it DependsOnGameMode or not

                handmade.cpp

                internal task_with_memory * BeginTaskWithMemory(transient_state *TranState, b32 DependsOnGameMode)
                {
                    task_with_memory *FoundTask = 0;

                    for(uint32 TaskIndex = 0;
                        TaskIndex < ArrayCount(TranState->Tasks);
                        ++TaskIndex)
                    {
                        task_with_memory *Task = TranState->Tasks + TaskIndex;
                        if(!Task->BeingUsed)
                        {
                            FoundTask = Task;
                            Task->BeingUsed = true;
    ------------------->    Task->DependsOnGameMode = DependsOnGameMode;
                            Task->MemoryFlush = BeginTemporaryMemory(&Task->Arena);
                            break;
                        }
                    }

                    return(FoundTask);
                }


13:43
so now when we switch game modes in the SetGameMode(); function, we added logic so that we can just wait for all tasks to finish 


                handmade.cpp

                internal void SetGameMode(game_state *GameState, transient_state *TranState, game_mode GameMode)
                {
                    b32 NeedToWait = false;
                    for(u32 TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex)
                    {
                        NeedToWait = NeedToWait || TranState->Tasks[TaskIndex].DependsOnGameMode;
                    }
                    if(NeedToWait)
                    {
                        Platform.CompleteAllWork(TranState->LowPriorityQueue);
                    }
                    Clear(&GameState->ModeArena);
                    GameState->GameMode = GameMode;
                }


21:36
Casey also mentiened that when we start our game from the visual studio debugger, it takes awhile for the game to start.
This actually first happened when we added ClearToZero on pushSize();

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

what Casey suspects is that we are spending lots of time in the clearing to zero part when the game starts.

Casey wanted to validate this without using a profiler, so he tried using "debug break all" in the visual studio debugger.
and the debugger certain gave us at 

                handmade.h

                inline void ZeroSize(memory_index Size, void *Ptr)
                {
                    // TODO(casey): Check this guy for performance
                    uint8 *Byte = (uint8 *)Ptr;
                    while(Size--)
                    {
    ----------->        *Byte++ = 0;
                    }
                }


for example in asset system where we call AllocateGameAssets();


                handmade_asset.cpp

                internal game_assets * AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    ...
                    ...

                    InsertBlock(&Assets->MemorySentinel, Size, PushSize(Arena, Size, NoClear()));
                                                                ^
                    ...                                         |
                    ...                                         |
                }                                               |

we will allocate all the memory we give to the asset system, and ClearToZero will be called for all of the memory.


23:05
so Casey wants to address this problem. Casey will change the way PushSize(); to work. 
PushSize(); will not take Alignment and the clear to zero flag

so Casey added some structs to help the refactor 

                enum arena_push_flag
                {
                    ArenaFlag_ClearToZero = 0x1,
                };
                struct arena_push_params
                {
                    u32 Flags;
                    u32 Alignment;
                };


24:30
So Casey also added the DefaultArenaParams(); function 

                handmade.h

                inline arena_push_params DefaultArenaParams(void)
                {
                    arena_push_params Params;
                    Params.Flags = ArenaFlag_ClearToZero;
                    Params.Alignment = 4;
                    return(Params);
                }

and casey goes on to change all the memory related function to use arena_push_params

for example:

                handmade.h

                inline memory_index GetArenaSizeRemaining(memory_arena *Arena, arena_push_params Params = DefaultArenaParams())
                {
                    memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Params.Alignment));

                    return(Result);
                }


26:14
Casey mentioned that the reason why he bundled it like this instead of adding another parameter is because 
now we can create this custom set of convenient phrases, that is very readable, and easy for us to use

for example, we create a AlignNoClear(); function 

                handmade_audio.cpp 

                internal void OutputPlayingSounds(audio_state *AudioState,
                                    game_sound_output_buffer *SoundBuffer, game_assets *Assets,
                                    memory_arena *TempArena)
                {    
                    ...
                    ...

                    // TODO(casey): Are we sure we don't want to let _this_ do the clear?
                    __m128 *RealChannel0 = PushArray(TempArena, ChunkCount, __m128, AlignNoClear(16));
                    __m128 *RealChannel1 = PushArray(TempArena, ChunkCount, __m128, AlignNoClear(16));

                    ...
                    ...
                }


the code looks like:

                handmade.h

                inline arena_push_params
                AlignNoClear(u32 Alignment)
                {
                    arena_push_params Params = DefaultArenaParams();
                    Params.Flags &= ~ArenaFlag_ClearToZero;
                    Params.Alignment = Alignment;
                    return(Params);
                }



29:18
Casey also changed the Align(); function 


                inline arena_push_params Align(u32 Alignment, b32 Clear)
                {
                    arena_push_params Params = DefaultArenaParams();
                    if(Clear)
                    {
                        Params.Flags |= ArenaFlag_ClearToZero;
                    }
                    else
                    {
                        Params.Flags &= ~ArenaFlag_ClearToZero;
                    }
                    Params.Alignment = Alignment;
                    return(Params);
                }


38:15 
Casey wants to go ahead and start making the concept of memory arenas flow all the way to the OS level.
Because Casey wants to do some stuff where we can have memory arenas that grow, where the OS gives us more memory.

The reason why we want to do that is becuz, first we may just want to run on the PC that way. 
PC has virutal memory, and people may want to run huge worlds. We can allow them to use all the memory on that machine 
if they want. 

even we previously mentioned that we want the game to run on a fixed memory limit, our debug system should never have to 
run throught that limitation. the debug system should have a separate set of memory, and that should be able to run on some 
growable thing, and ask the OS for more memory. Since its the debug system, it will not ship to a users machine. 

[but due to time constraints, Casey didnt do it in this episode]





51:00
Casey now looking at sorting the entries in the render_group

so we need to be able to compare two elements and swap them, 
and we typically want to do them pretty "random access"

recall previously, we were accessing each entry by advincing the memory sizes 
so Casey wonders if that is a good idea, or if our design for each entry can be the same size. 


                internal void sRenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget,
                                    rectangle2i ClipRect, bool Even)
                {
                    ...

                    for(uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize; )
                    {
                        render_group_entry_header *Header = (render_group_entry_header *) (RenderGroup->PushBufferBase + BaseAddress);
                        BaseAddress += sizeof(*Header);

                        ...
                        switch(Header->Type)
                        {
                            case RenderGroupEntryType_render_entry_clear:
                            {
                                render_entry_clear *Entry = (render_entry_clear *)Data;

                                ...
                                ...

            --------------->    BaseAddress += sizeof(*Entry);
                            } break;

                            case RenderGroupEntryType_render_entry_bitmap:
                            {
                                render_entry_bitmap *Entry = (render_entry_bitmap *)Data;

                                ...
                                ...

            --------------->    BaseAddress += sizeof(*Entry);
                            } break;

                            ...
                            ...

                            InvalidDefaultCase;
                        }
                    }
                }


if we have them in the same size, then we can just sort them in place  

however, we may not super care, because it may be the case where we pre-call everything to see what falls inside each tile,
and then only have each thread only sort the context of each bucket. 
Recall how our renderer works is that we break the scene into each tile, we have a thread for each tile 

using the graph from day 121

         ______________________________________
        |         AAA|AAA        B|BBB         |
        |   0     AAA|AAA     1  B|BBB     2   |
        |            |           B|BBB         |
        |            |            |            |
        |            |            |            |
        |____________|____________|____________|
        |            |            |            |
        |   3        |        4   |        5   |
        |            |    CCCCC   |            |
        |            |    CCCCC   |            |
        |   DDDDDDDDD|DDD         |            |
        |____________|____________|____________|


each thread gets a tile in our scene and renders it 


so what we might be able to do is to do a first pass, build an array for each thread, and sort it internally 
and Casey decided to go down this approach.


53:18
so we first store how many elements are in the render_group, intead of just haveing memory sizes "u32 PushBufferSize"


                struct render_group
                {
                    ...
                    ...

                    u32 MaxPushBufferSize;
                    u32 PushBufferSize;
                    u8 *PushBufferBase;
    ----------->    u32 PushBufferElementCount;

                    uint32 MissingResourceCount;
                    b32 RendersInBackground;

                    ...
                };


so then when we prepare our tile_render_work


                handmade_render_group.cpp

                struct tile_render_work
                {
                    render_group *RenderGroup;
                    loaded_bitmap *OutputTarget;
                    rectangle2i ClipRect;

                    tile_sort_entry *SortSpace;
                };

we can give it space for sorting. 
the tile_sort_entry looks like below:

                struct tile_sort_entry
                {
                    r32 SortKey;
                    u32 PushBufferOffset;
                };



56:00
so when we actually dispatch the thread to do the rendering work, we create that space 

                handmade_render_group.cpp

                internal void RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget, memory_arena *TempArena)
                {
                    ...
                    ...

                    tile_render_work Work;
                    Work.RenderGroup = RenderGroup;
                    Work.OutputTarget = OutputTarget;
                    Work.ClipRect = ClipRect;
    ----------->    Work.SortSpace = PushArray(TempArena, RenderGroup->PushBufferElementCount, tile_sort_entry);

                    DoTiledRenderWork(0, &Work);

                    EndTemporaryMemory(Temp);
                }


then also in the TiledRenderGroupToOutput(); function, we also give it SortSpace

                handmade_render_group.cpp

                internal void TiledRenderGroupToOutput(platform_work_queue *RenderQueue,
                                         render_group *RenderGroup, loaded_bitmap *OutputTarget,
                                         memory_arena *TempArena)
                {
                    TIMED_FUNCTION();

                    temporary_memory Temp = BeginTemporaryMemory(TempArena);

                    Assert(RenderGroup->InsideRender);

                    ...
                    ...

                    int WorkCount = 0;
                    for(int TileY = 0; mTileY < TileCountY; ++TileY)
                    {
                        for(int TileX = 0; TileX < TileCountX; ++TileX)
                        {
                            ...
                            ...

                            Work->RenderGroup = RenderGroup;
                            Work->OutputTarget = OutputTarget;
                            Work->ClipRect = ClipRect;
        --------------->    Work->SortSpace = PushArray(TempArena, RenderGroup->PushBufferElementCount, tile_sort_entry);

                            Platform.AddEntry(RenderQueue, DoTiledRenderWork, Work);

                        }
                    }

                    Platform.CompleteAllWork(RenderQueue);

                    EndTemporaryMemory(Temp);
                }


Q/A
1:02:05
someone asked can you briefly explained how the explain how the expanding arenas will be implemented?

the only thing you do, is in the PushSize_(); function, instead of the assert, when we run out of memory, 
you just call the operating system for memory. 


                inline void * PushSize_(memory_arena *Arena, memory_index SizeInit, arena_push_params Params = DefaultArenaParams())
                {
                    memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Params);
                    
    ----------->    Assert((Arena->Used + Size) <= Arena->Size);

                    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Params.Alignment);
                    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
                    Arena->Used += Size;

                    Assert(Size >= SizeInit);

                    if(Params.Flags & ArenaFlag_ClearToZero)
                    {
                        ZeroSize(SizeInit, Result);
                    }
                    
                    return(Result);
                }
