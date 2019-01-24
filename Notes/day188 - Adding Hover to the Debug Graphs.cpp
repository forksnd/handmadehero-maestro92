Handmade Hero Day 188 - Adding Hover to the Debug Graphs

Summary:
reduced the number of stored debug events from 64 frames to 8 frames, which dramatically increased performance

displayed the function as text when mouse hovers ontop of the debug_frame_region

passed the game input into DEBUGOverlay(); function.

converted game input from platform coordinates to game code UI coordinates.

discussed the two ways of how to map platform coordinates to game code UI coordinates

added a pause button to pause the data collation.

Keyword:
Game input, debug, profiling, UI

1:20
Casey says he notices that the game gets a lots lower as the game goes. He says that this makes
sense cuz the number of events that gets piled up in 64 frames worth of data becomes a lot

so the first thing Casey will do is to set the number of debug frames down to a lower number.

2:29
so in handmade_platform.h, casey changed the number of frames of debug data to 8

                handmade_platform.h

                #define MAX_DEBUG_EVENT_ARRAY_COUNT 8

6:20
Casey mentions that he wants to be able to hover the mouse over one of the bars, then print out what the name of the function is.
the first thing we need to do is to get the mouse information to the debug system



7:37
currently in DEBUGOverlay();, we are only passing the game_memory, now we also want to pass in game_input

                handmade_debug.cpp

                internal void DEBUGOverlay(game_memory *Memory, game_input *Input)
                {

                    ...
                    ...
                }




8:09
so previously in DEBUGOverlay(); we were rendering all the debug_frame_region


                handmade_debug.cpp

                internal void DEBUGOverlay(game_memory *Memory, game_input *Input)
                {

                    ...
                    ...


                    for(u32 FrameIndex = 0; FrameIndex < MaxFrame; ++FrameIndex)
                    {
                        debug_frame *Frame = DebugState->Frames + DebugState->FrameCount - (FrameIndex + 1);

                        ...
                        ...

                        for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex)
                        {
                            debug_frame_region *Region = Frame->Regions + RegionIndex;

                            ...
                            ...

                            PushRect(RenderGroup, V3(StackX + 0.5f*LaneWidth + LaneWidth*Region->LaneIndex,
                                                     0.5f*(ThisMinY + ThisMaxY), 0.0f),
                                     V2(LaneWidth, ThisMaxY - ThisMinY), V4(Color, 1));
                        }                
                    }
                }


now that we want a hover, we will just test if the mouse position is intersecting with the region 


                handmade_debug.cpp

                internal void DEBUGOverlay(game_memory *Memory, game_input *Input)
                {

                    ...
                    ...

                    v2 MouseP = V2(Input->MouseX, Input->MouseY);

                    for(u32 FrameIndex = 0; FrameIndex < MaxFrame; ++FrameIndex)
                    {
                        debug_frame *Frame = DebugState->Frames + DebugState->FrameCount - (FrameIndex + 1);

                        ...
                        ...

                        for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex)
                        {
                            debug_frame_region *Region = Frame->Regions + RegionIndex;

                            ...
                            ...

                            PushRect(RenderGroup, V3(StackX + 0.5f*LaneWidth + LaneWidth*Region->LaneIndex,
                                                     0.5f*(ThisMinY + ThisMaxY), 0.0f),
                                     V2(LaneWidth, ThisMaxY - ThisMinY), V4(Color, 1));


                            if(IsInRectangle(RegionRect, MouseP))
                            {
                                debug_record *Record = Region->Record;
                                char TextBuffer[256];
                                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                            "%32s: %10ucy [%s(%d)]",
                                            Record->BlockName,
                                            Region->CycleCount,
                                            Record->FileName,
                                            Record->LineNumber);
                                DEBUGTextLine(TextBuffer);
                            }

                        }                
                    }
                }

12:01
we make the debug_frame_region store what the debug_record it corresponds to 


                handmade_debug.cpp

                struct debug_frame_region
                {
                    debug_record *Record;
                    u64 CycleCount;
                    ...
                    ...
                };



18:32
when debugging, Casey mentioned that the Mouse position is not in the same space as our rendering space.



27:04
Casey mentioned that cross platform wise, we dont know how those mouse coordinates sysmte is gonna be.
basically what happens is that all the platform layer has to emulate the way windows does their mouse. 
so it makes more sense for the platform layer to transform their input to the game code coordinate system.
In handmade hero, we are using a centered, Y is up coordinate system 



28:42
so in win32_handmade.cpp, when we get the input from the platform, we just do the coordinate transformation 
right away.

                int WinMain()
                {

                    while(GlobalRunning)
                    {
                        ...
                        ...
                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        NewInput->MouseX = (-0.5f*(r32)GlobalBackbuffer.Width + 0.5f) + (r32)MouseP.x;
                        NewInput->MouseY = (0.5f*(r32)GlobalBackbuffer.Height - 0.5f) - (r32)MouseP.y;
                        NewInput->MouseZ = 0; // TODO(casey): Support mousewheel?
                        ...
                    }
                }

at 30:30 Casey does mention that he thinks that this transformation is off by 1 pixel. the reason he says that is becuz 

we havent really formally defined our center 
recall that our screen resolution is 1920 in width.
so 1920 / 2 = 960 pixels 


the idea is that where do you want windows mouse_x = 0 and windows mouse_x = 1919 map to. 

Option1:
pixel are anchored on the left edge 
so windows mouse_x = 0 would map to -959.5
windows mouse_x = 1919 would map to +959.5
                                    
                                      
              -960                      0                     +960    
                |                       |                       |                   
                v                       v                       v
                 _______________________________________________
                |       |       |       |       |       |       |
                |       |       |       |       |       |       |
                |_______|_______|_______|_______|_______|_______|
                |       |       |       |       |       |       |
                |       |       |       |       |       |       |
                |_______|_______|_______|_______|_______|_______|
                |       |       |       |       |       |       |
                |       |       |       |       |       |       |
                |_______|_______|_______|_______|_______|_______|
                |       |       |       |       |       |       |
                |       |       |       |       |       |       |
                |_______|_______|_______|_______|_______|_______|



Option2:
pixel are anchored on the center
so windows mouse_x = 0 would map to -960
windows mouse_x = 1919 would map to +959

                  -960                      0             +959    
                    |                       |               |                   
                    v                       v               v
                 _______________________________________________
                |       |       |       |       |       |       |
                |       |       |       |       |       |       |
                |_______|_______|_______|_______|_______|_______|
                |       |       |       |       |       |       |
                |       |       |       |       |       |       |
                |_______|_______|_______|_______|_______|_______|
                |       |       |       |       |       |       |
                |       |       |       |       |       |       |
                |_______|_______|_______|_______|_______|_______|
                |       |       |       |       |       |       |
                |       |       |       |       |       |       |
                |_______|_______|_______|_______|_______|_______|

Casey claims that hes not a graphics programmer, so he doesnt know which one to do.

so Casey went with Option1, cuz he likes the symmetry

                NewInput->MouseX = (-0.5f*(r32)GlobalBackbuffer.Width + 0.5f) + (r32)MouseP.x;
                NewInput->MouseY = (0.5f*(r32)GlobalBackbuffer.Height - 0.5f) - (r32)MouseP.y;

[we had this problem in game2 with our map grid coordinates as well, in prototype, we did option2,
    in production, we did option1. I think I like Option1 better]



38:44
Casey added a PausedFlag which will pause the collation. so in terms of gathering debug_data, it will pause there.
The game will still run.


                handmade_debug.h

                struct debug_state
                {
                    ...
                    b32 Paused;
                    
                    ...
                    ...                    
                };



then in DEBUG_GAME_FRAME_END(); we add the logic to only Collate data when we are pausded

                handmade_debug.cpp

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    ...
                    ...

                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState)
                    {
                        ...
                        ...

                        if(!DebugState->Paused)
                        {
                            EndTemporaryMemory(DebugState->CollateTemp);            
                            DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);

                            DebugState->FirstThread = 0;
                            DebugState->FirstFreeBlock = 0;

                            CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
                        }        
                    }

                    return(GlobalDebugTable);
                }

41:26
so to make the game inputs also platform friendly, we defined some enums for game_input

                handmade_platform.h

                enum game_input_mouse_button
                {
                    PlatformMouseButton_Left,
                    PlatformMouseButton_Middle,
                    PlatformMouseButton_Right,
                    PlatformMouseButton_Extended0,
                    PlatformMouseButton_Extended1,

                    PlatformMouseButton_Count,
                };
                typedef struct game_input
                {
    ------------>   game_button_state MouseButtons[PlatformMouseButton_Count];
                    r32 MouseX, MouseY, MouseZ;

                    b32 ExecutableReloaded;
                    r32 dtForFrame;

                    game_controller_input Controllers[5];
                } game_input;


and in the win32_handmade layer, we make the game_input use these enums

also notice that at 48:18, Casey realizes that our input never gets cleared in the platform layer
so we added that

                while(GlobalRunning)
                {   
                    ...
                    ...
                
                    DWORD WinButtonID[PlatformMouseButton_Count] =
                    {
                        VK_LBUTTON,
                        VK_MBUTTON,
                        VK_RBUTTON,
                        VK_XBUTTON1,
                        VK_XBUTTON2,
                    };

                    for(u32 ButtonIndex = 0; ButtonIndex < PlatformMouseButton_Count; ++ButtonIndex)
                    {
                        NewInput->MouseButtons[ButtonIndex] = OldInput->MouseButtons[ButtonIndex];
                        NewInput->MouseButtons[ButtonIndex].HalfTransitionCount = 0;
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[ButtonIndex],
                                                    GetKeyState(WinButtonID[ButtonIndex]) & (1 << 15));
                    }

                }


41:56
so in DEBUGOverlay(); Casey added that whenver we hit the mouse right button, we pause the debug rendering 


                internal void
                DEBUGOverlay(game_memory *Memory, game_input *Input)
                {
                    ...

                    v2 MouseP = V2(Input->MouseX, Input->MouseY);
                    if(WasPressed(Input->MouseButtons[PlatformMouseButton_Right]))
                    {
                        DebugState->Paused = !DebugState->Paused;
                    }

                    ...
                    ...
                }



43:06
Casey added to the platform layer whether some button went down or not 

recall a half transition is either down to up or up to down 

so if the HalfTransitionCount is above 1, that means it must have gone through at least one up to down transition
so it was pressed.

or if the HalfTransitionCount is exactly1, and the button state_s final state is EndedDown, that means it is pressed 

so we return true in these two scenarios


                handmade_platform.h
                
                inline b32 WasPressed(game_button_state State)
                {
                    b32 Result = ((State.HalfTransitionCount > 1) ||
                                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));

                    return(Result);
                }



