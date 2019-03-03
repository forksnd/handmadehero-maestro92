Handmade Hero Day 240 - Moving the Renderer into a Third Tier

Summary:
went on and made the architecture change that was mentioned in day 239
where we now have three tiers in our game architecture, platform code, game code, and renderer code

created a utility file for renderers, handmade_render.cpp. mentioned that this is platform-non specific code
for the renderer tier operations.  

mentioned complications with the generation ID

changed it so that now the game code takes in a game_render_commands struct. which will store all the render elements.
the win32 platform side allocates memory for the game_render_commands, which will store all of the render elements.

the RenderGroup in the game code no longer allocates memory.

Keyword:
renderer, game architecture, openGL, memory


0:51
followed up on what was mentioned in day 239
Casey wants to move forward with this structure 
     ___________________________                       _______________                                 _______________________
    | win32 layer               |  Render buffer      |               |                               |                       |
    |    GameUpdateAndRender    |-------------------->|   Game code   |------------------------------>|     win32 layer       |
    |___________________________|                     |               |   passes the filled           |_______________________|
                                                      |   fills the   |   render buffer back to win32       |
                                                      |    bitmap     |                                     |
                                                      |_______________|                                     |
                                                                                                       _____v___________________
                                                                                                      |                         |
                                                                                                      |   rendering layer       |
                                                                                                      |_________________________|


3:06
Casey mentioned that right now we essentially have two render groups. one that renders the game, one that renders the debug UI


4:23
so Casey first take the game_offscreen_buffer off the GAME_UPDATE_AND_RENDER(); function
recall, we previously had:

                Handmade_platform.h

                #define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
                typedef GAME_UPDATE_AND_RENDER(game_update_and_render);


now we have, so essentially we pass in a game_render_commands

                Handmade_platform.h

                typedef struct game_render_commands
                {
                    u32 Width;
                    u32 Height;
                    
                    u32 MaxPushBufferSize;
                    u32 PushBufferSize;
                    u8 *PushBufferBase;
                    
                    u32 PushBufferElementCount;
                    u32 SortEntryAt;
                } game_render_commands;

                #define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, game_render_commands *RenderCommands)
                typedef GAME_UPDATE_AND_RENDER(game_update_and_render);


11:15
so in the win32 layer, we allocate memory for RenderCOmmands, and then we pass it to the GameUpdateAndRender function.
[Casey left the game at a uncompiled state, so I am assume the line

                void *PushBuffer = VirtualAlloc;

is meant to be 

                void *PushBuffer = VirtualAlloc(PushBufferSize);

essentially, we are asking windows for a 4 MB of memory, and VirtualAlloc is returning us a pointer to that memory
]

                while(GlobalRunning)
                {
                    // TODO(casey): Decide what our pushbuffer size is!
                    u32 PushBufferSize = Megabytes(4);
                    void *PushBuffer = VirtualAlloc;
                    game_render_commands RenderCommands = RenderCommandStruct(
                        PushBufferSize, PushBuffer,
                        GlobalBackbuffer.Width,
                        GlobalBackbuffer.Height);
                    
                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackbuffer.Memory;
                    Buffer.Width = GlobalBackbuffer.Width; 
                    Buffer.Height = GlobalBackbuffer.Height;
                    Buffer.Pitch = GlobalBackbuffer.Pitch;
                    if(!GlobalPause)
                    {
                        ...
                        ...

                        if(Game.UpdateAndRender)
                        {
                            Game.UpdateAndRender(&GameMemory, NewInput, &RenderCommands);
                            if(NewInput->QuitRequested)
                            {
                                BeginFadeToDesktop(&Fader);
                            }
                        }
                    }
                }


so for the RenderCommandStruct function, that is essentially a #define 

                handmade_platform.h
                #define RenderCommandStruct(MaxPushBufferSize, PushBuffer, Width, Height) \
                {
                    Width, Height, MaxPushBufferSize, 0, (u8 *)PushBuffer, 0, MaxPushBufferSize
                };


to expand this #define out, it is essentially creating a game_render_commands. nothing special

                game_render_commands RenderCommands = 
                {
                    GlobalBackbuffer.Width, GlobalBackbuffer.Height, PushBufferSize, 0, (u8 *)PushBuffer, 0, PushBufferSize
                };



Recall that our RenderGroup used to look like 

                struct render_group
                {
                    ...
                    ...

                    u32 MaxPushBufferSize;
                    u32 PushBufferSize;
                    u8 *PushBufferBase;
                    
                    u32 PushBufferElementCount;
                    u32 SortEntryAt;

                    uint32 MissingResourceCount;
                    b32 RendersInBackground;

                    b32 InsideRender;
                };

now we are moving all the memory allocation stuff to game_render_commands


so now the render_group looks like 

                struct render_group
                {
                    struct game_assets *Assets; 
                    real32 GlobalAlpha;

                    v2 MonitorHalfDimInMeters;
                    
                    camera_transform CameraTransform;

                    uint32 MissingResourceCount;
                    b32 RendersInBackground;

                    u32 GenerationID;
                    game_render_commands *Commands;
                };




14:00
following the refactor above, recall previously, our AllocateRenderGroup(); function looks like:

                handmade_render_group.cpp

                internal render_group * AllocateRenderGroup(game_assets *Assets, memory_arena *Arena, uint32 MaxPushBufferSize, b32 RendersInBackground)
                {
                    render_group *Result = PushStruct(Arena, render_group);

                    if(MaxPushBufferSize == 0)
                    {
                        // TODO(casey): Safe cast from memory_uint to uint32?
                        MaxPushBufferSize = (uint32)GetArenaSizeRemaining(Arena);
                    }
                    Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize, NoClear());
                    Result->MaxPushBufferSize = MaxPushBufferSize;
                    
                    Result->Assets = Assets;
                    Result->RendersInBackground = RendersInBackground;
                    Result->InsideRender = false;

                    ClearRenderValues(Result);
                    
                    return(Result);
                }

in which we were allocate memory for the render_group from our memory_arena. 
this will now come from our game_render_commands.


53:01
so now in our main game loop, you can see that (we renamed AllocateRenderGroup to BeingRenderGroup);
in which we will create a RenderGroup on the program stack (not from our memory arena);

[recall previously, we created RenderGroup from our memory arena becuz the RenderGroup has to store all the render elements 
taking a picture from day 88

                 ___________________________            
                |                           |
                |       Render_Group        |   
                |___________________________|
                |                           |
                |   entity_visible_piece    |
                |                           |   
                |               bitmap -----|-----> somewhere in Transient memory
                |___________________________|
                |                           |
                |   entity_visible_piece    |
                |                           |   
                |               bitmap -----|-----> somewhere in Transient memory
                |___________________________|
                |                           |
                |   entity_visible_piece    |
                |                           |   
                |               bitmap -----|-----> somewhere in Transient memory
                |___________________________|
                |                           |
                |                           |
                |                           |
                |                           |
                |                           |
                |                           |
                |                           |
                |                           |
                |___________________________|
                |                           |
                |       DefaultBasis        |
                |___________________________|


the RenderGroup needs some memory for all of its render elements.
now all the responsibilities all goes to game_render_commands, so we now just create a render_group from the program stack memory.

again, our memory_arena is meant to replace the "new" allocator from the C runtime library. 
we intially had to allocate memory for the array of render elements, now that memory will come from the win32 layer.]



and then we pass it into UpdateAndRenderWorld(); whatever game mode we are in to create our render elements. 

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    ...
                    ...

                    // TODO(casey): Decide what our pushbuffer size is!
                    render_group RenderGroup_ = BeginRenderGroup(TranState->Assets, RenderCommands, TranState->MainGenerationID, false);
                    render_group *RenderGroup = &RenderGroup_;

                    b32 Rerun = false;
                    do
                    {
                        switch(GameState->GameMode)
                        {
                            ...
                            ...

                            case GameMode_World:
                            {
                                Rerun = UpdateAndRenderWorld(GameState, GameState->WorldMode, TranState, Input, RenderGroup, &DrawBuffer);
                            } break;

                            InvalidDefaultCase;
                        }
                    } while(Rerun);

                    EndRenderGroup(RenderGroup);
                }



our BeginRenderGroup now looks like:
as you can see, render_group now is just a utility thing, we dont keep this guy around anymore.

                inline render_group BeginRenderGroup(game_assets *Assets, game_render_commands *Commands,
                                 u32 GenerationID, b32 RendersInBackground)
                {
                    render_group Result = {};
                    
                    Result.Assets = Assets;
                    Result.RendersInBackground = RendersInBackground;
                    Result.GlobalAlpha = 1.0f;
                    Result.MissingResourceCount = 0;
                    Result.GenerationID = GenerationID;
                    Result.Commands = Commands;
                    
                    return(Result);
                }




55:33
then in UpdateAndRenderWorld();, we will create Render Elements for our render_group and our game_render_commands.

so Casey made it so that PushRenderElement_ now pushes stuff onto the game_render_commands. (the RenderGroup stores a pointer to it);


                handmade_render_group.cpp

                inline void * PushRenderElement_(render_group *Group, uint32 Size, render_group_entry_type Type, r32 SortKey)
                {
    ----------->    game_render_commands *Commands = Group->Commands;
                    
                    void *Result = 0;

                    Size += sizeof(render_group_entry_header);
                    
                    if((Commands->PushBufferSize + Size) < (Commands->SortEntryAt - sizeof(tile_sort_entry)))
                    {
                        render_group_entry_header *Header = (render_group_entry_header *)(Commands->PushBufferBase + Commands->PushBufferSize);
                        Header->Type = Type;
                        Result = (uint8 *)Header + sizeof(*Header);

                        Commands->SortEntryAt -= sizeof(tile_sort_entry);
                        tile_sort_entry *Entry = (tile_sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
                        Entry->SortKey = SortKey;
                        Entry->PushBufferOffset = Commands->PushBufferSize;

                        Commands->PushBufferSize += Size;
                        ++Commands->PushBufferElementCount;
                    }
                    else
                    {
                        InvalidCodePath;
                    }

                    return(Result);
                }






19:01
so now in the win32 layer, we now change the function so that it takes a render buffer 


                while(GlobalRunning)
                {

                                                    |
                                                    |
                                                    V
                    Win32DisplayBufferInWindow(&RenderCommands, DeviceContext,
                               Dimension.Width, Dimension.Height);

                    ...
                    ...
                }









19:36
then in the Win32DisplayBufferInWindow, we make this function to do the actual rendering 

so now you can see, our game_s architecture is 3 tiers,
we have the platform code, the game code, then the renderer 
all three tiers are decoupled.

so if you would want to do directX, you would just add another if statement 

the "InHardware" flag is completely using OpenGL 
the "DisplayViaHardware" flag is to compute the bits through software renderer, then display through OpenGL.


                internal void Win32DisplayBufferInWindow(platform_work_queue *RenderQueue, game_render_commands *Commands,
                                           HDC DeviceContext, s32 WindowWidth, s32 WindowHeight)
                {
                    SortEntries(Commands);

                    b32 InHardware = true;
                    b32 DisplayViaHardware = true;
                    if(InHardware)
                    {
                        RenderToOpenGL(Commands, WindowWidth, WindowHeight);        
                        SwapBuffers(DeviceContext);
                    }
                    else
                    {
                        TiledRenderGroupToOutput(RenderQueue, Commands, OutputTarget);        

                        if(DisplayViaHardware)
                        {
                            DisplayBitmapViaOpenGL();
                            SwapBuffers(DeviceContext);
                        }
                        else
                        {
                            // TODO(casey): Centering / black bars?
                    
                            if((WindowWidth >= Buffer->Width*2) &&
                               (WindowHeight >= Buffer->Height*2))
                            {
                                StretchDIBits(DeviceContext,
                                              0, 0, 2*Buffer->Width, 2*Buffer->Height,
                                              0, 0, Buffer->Width, Buffer->Height,
                                              Buffer->Memory,
                                              &Buffer->Info,
                                              DIB_RGB_COLORS, SRCCOPY);
                            }
                            else
                            {

                                int OffsetX = 0;
                                int OffsetY = 0;

                                // NOTE(casey): For prototyping purposes, we're going to always blit
                                // 1-to-1 pixels to make sure we don't introduce artifacts with
                                // stretching while we are learning to code the renderer!
                                StretchDIBits(DeviceContext,
                                              OffsetX, OffsetY, Buffer->Width, Buffer->Height,
                                              0, 0, Buffer->Width, Buffer->Height,
                                              Buffer->Memory,
                                              &Buffer->Info,
                                              DIB_RGB_COLORS, SRCCOPY);
                            }
                        }
                    }
                }

32:04
Casey mentioned that in the Win32DisplayBufferInWindow(); function, this SortEntries();
is somewhat tricky.
it is a platform non-specific code, but in the renderer tier operation.

so Casey added a handmade_render.cpp file, this is pretty much a utility file for renderer functions 
so Casey moved all the renderer sorting things to handmade_render.cpp


36:05
also notice in the Win32DisplayBufferInWindow(); function, when we call TiledRenderGroupToOutput(); 
we just directly pass the threads in. That means in terms of multi-threading rendering, we could possibly
completely keep the threads out of the game code, and entirely in the win32 side. 


37:47
the way we are structing this code is that
Basically anything in the game code that is not involved with building up the rendering push buffers, needs 
to come out and be placed in the handmade_render.cpp

since we are moving the renderer out of the game code, but the render group it self is not coming out of the game code.


41:55
so previously we had BeginRender(); and EndRender(); which we mainly used for 
RenderGroup Id Generations 

recall, we had:
                handmade_render_group.cpp

                internal void BeginRender(render_group *Group)
                {
                    IGNORED_TIMED_FUNCTION();

                    if(Group)
                    {
                        Assert(!Group->InsideRender);
                        Group->InsideRender = true;

                        Group->GenerationID = BeginGeneration(Group->Assets);
                    }
                }

                internal void EndRender(render_group *Group)
                {
                    IGNORED_TIMED_FUNCTION();

                    if(Group)
                    {
                        Assert(Group->InsideRender);
                        Group->InsideRender = false;
                        
                        EndGeneration(Group->Assets, Group->GenerationID);

                        ClearRenderValues(Group);
                   }
                }

so Casey mentioned that we can just get rid of the BeginRender(); and EndRender(); function.
we can just have AllocateRenderGroup(); and DeallocateRenderGroup(); instead. 

Casey mentioned that he doesnt know if the Generation ID for multithreaded asset loading protection
is something we will stick with in the long run, but we need to find some way to maintain the protection 

51:01
Casey is considering removing Generation ID completely.

[so the generation ID is left in a state thats somewhat messy, Casey will come back and address it]




