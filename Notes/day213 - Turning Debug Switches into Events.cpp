Handmade Hero Day 213 - Turning Debug Switches into Events

Summary:
Added #defines for debug swithces in the handmade_config.h files.

followed up on the test we did in day 203 where having the debug switches as #defines 
vs as bool doesnt give you performance benefits. Also having them as debug switches requires recompiling 
all the time. So Casey converted them as booleans.

added a #define so that when we are in debug mode, we will create a debug_event for debug switches 

explained why in the Q/A we are doing GetKeyState(); for "shift", "alt" and "control" instead of tracking all the 
WM_KEYUP and WM_KEYDOWN events for it 

explained why we nested levels of C preprocessors macros when we want to stringize a macro with it

Keyword:
debug system, keyboard input




5:39
in day 212, we mentioned that we want to support multi-select but never fully implemented in
here, we added code to check if the shift, alt and control keys are down in the platform layer 

                win32_handmade.cpp 

                int WinMain()
                {
                    ...
                    ...

                    if(!GlobalPause)
                    {
                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        NewInput->MouseX = (r32)MouseP.x;
                        NewInput->MouseY = (r32)((GlobalBackbuffer.Height - 1) - MouseP.y);
                        NewInput->MouseZ = 0; // TODO(casey): Support mousewheel?

    ----------->        NewInput->ShiftDown = (GetKeyState(VK_SHIFT) & (1 << 15));
                        NewInput->AltDown = (GetKeyState(VK_MENU) & (1 << 15));
                        NewInput->ControlDown = (GetKeyState(VK_CONTROL) & (1 << 15));
                    }
                }



8:26
then now in the DebugInteraction_Select Case, if the shift key is down, we will add it to the list of selected debug_events 


                internal void DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP, b32 AltUI)
                {
                    if(DebugState->HotInteraction.Type)
                    {
                        ...
                        ...

                        switch(DebugState->HotInteraction.Type)
                        {
                            case DebugInteraction_TearValue:
                            {
                                ...
                                ...
                            } break;

                            case DebugInteraction_Select:
                            {
    ----------------->          if(!Input->ShiftDown)
                                {
                                    ClearSelection(DebugState);
                                }
                                AddToSelection(DebugState, DebugState->HotInteraction.ID);
                            } break;                
                        }

                        DebugState->Interaction = DebugState->HotInteraction;
                    }
                    else
                    {
                        DebugState->Interaction.Type = DebugInteraction_NOP;
                    }
                }


12:36
Casey coming back to address how to integrate debug switches in our handmade_config.h into our current debug system
since we dont have struct debug_variables anymore, and we only have debug_events 



14:34
so Casey mentioned that when we someone asked in day 193

"why have all these debug switches as #defines instead of just a boolean"
and Casey mentioned that if statement can be expensive 

and we did some testing and found out that it barely hurt performance at all.

so previously we had 
                
                handmade_render_group.cpp

                inline v3 SampleEnvironmentMap(v2 ScreenSpaceUV, v3 SampleDirection, real32 Roughness, environment_map *Map,
                                     real32 DistanceFromMapInZ)
                {
                    ...
                    ...

                #if DEBUGUI_ShowLightingSamples
                    uint8 *TexelPtr = ((uint8 *)LOD->Memory) + Y*LOD->Pitch + X*sizeof(uint32);
                    *(uint32 *)TexelPtr = 0xFFFFFFFF;
                #endif

                    ...

                    return(Result);
                }


Casey mentions that as long as its a constant boolean, so that it is a perfectly predicted branch, it wont really hurt us 
Also Casey mentioned that previous with our #define debug switches, it was taking too long to compile (too long for Casey);

so Casey would prefer to have something like:

                handmade_render_group.cpp

                inline v3 SampleEnvironmentMap(v2 ScreenSpaceUV, v3 SampleDirection, real32 Roughness, environment_map *Map,
                                     real32 DistanceFromMapInZ)
                {
                    ...
                    ...

                    if(DEBUGUI_ShowLightingSamples)
                    {
                        uint8 *TexelPtr = ((uint8 *)LOD->Memory) + Y*LOD->Pitch + X*sizeof(uint32);
                        *(uint32 *)TexelPtr = 0xFFFFFFFF;
                    }

                    ...

                    return(Result);
                }


15:59
Casey has realized that since there is no need to go with the compilation route (long compile time and it doesnt give us 
any performance advantage);

so what Casey proposes to do is to just 

                handmade_render_group.cpp

                inline v3 SampleEnvironmentMap(v2 ScreenSpaceUV, v3 SampleDirection, real32 Roughness, environment_map *Map,
                                     real32 DistanceFromMapInZ)
                {
                    ...
                    ...

                    DEBUG_IF(ShowLightingSamples)
                    {
                        uint8 *TexelPtr = ((uint8 *)LOD->Memory) + Y*LOD->Pitch + X*sizeof(uint32);
                        *(uint32 *)TexelPtr = 0xFFFFFFFF;
                    }

                    ...

                    return(Result);
                }




Casey also mentioned that he didnt like the fact that he had to type out the hierarchy 

                handmade_debug_variables.h

                internal void DEBUGCreateVariables(debug_variable_definition_context *Context)
                {
                // TODO(casey): Parameterize the fountain?
                    
                #define DEBUG_VARIABLE_LISTING(Name) DEBUGAddVariable(Context, #Name, DEBUGUI_##Name)

                    DEBUGBeginVariableGroup(Context, "Entities");
                    DEBUG_VARIABLE_LISTING(DrawEntityOutlines);
                    DEBUGEndVariableGroup(Context);

                    DEBUGBeginVariableGroup(Context, "Group Chunks");
                    DEBUG_VARIABLE_LISTING(GroundChunkOutlines);
                    DEBUG_VARIABLE_LISTING(GroundChunkCheckerboards);
                    DEBUG_VARIABLE_LISTING(RecomputeGroundChunksOnEXEChange);
                    DEBUGEndVariableGroup(Context);

                    DEBUGBeginVariableGroup(Context, "Particles");
                    DEBUG_VARIABLE_LISTING(ParticleTest);
                    DEBUG_VARIABLE_LISTING(ParticleGrid);
                    DEBUGEndVariableGroup(Context);
                    
                    DEBUGBeginVariableGroup(Context, "Renderer");
                    {        
                        DEBUG_VARIABLE_LISTING(TestWeirdDrawBufferSize);
                        DEBUG_VARIABLE_LISTING(ShowLightingSamples);

                        DEBUGBeginVariableGroup(Context, "Camera");
                        {
                            DEBUG_VARIABLE_LISTING(UseDebugCamera);
                            DEBUG_VARIABLE_LISTING(DebugCameraDistance);
                            DEBUG_VARIABLE_LISTING(UseRoomBasedCamera);
                        }
                        DEBUGEndVariableGroup(Context);

                        DEBUGEndVariableGroup(Context);
                    }

                    DEBUG_VARIABLE_LISTING(FamiliarFollowsHero);
                    DEBUG_VARIABLE_LISTING(UseSpaceOutlines);
                    DEBUG_VARIABLE_LISTING(FauxV4);

                #undef DEBUG_VARIABLE_LISTING
                }



17:58
Casey also mentioned that there is a specific reason why he did 

                handmade_render_group.cpp
                
                inline v3 SampleEnvironmentMap(v2 ScreenSpaceUV, v3 SampleDirection, real32 Roughness, environment_map *Map,
                                     real32 DistanceFromMapInZ)
                {
                    ...
                    ...

                    DEBUG_IF(ShowLightingSamples)
                    {
                        uint8 *TexelPtr = ((uint8 *)LOD->Memory) + Y*LOD->Pitch + X*sizeof(uint32);
                        *(uint32 *)TexelPtr = 0xFFFFFFFF;
                    }

                    ...

                    return(Result);
                }

instead of 

                inline v3 SampleEnvironmentMap(v2 ScreenSpaceUV, v3 SampleDirection, real32 Roughness, environment_map *Map,
                                     real32 DistanceFromMapInZ)
                {
                    ...
                    ...

                    if(DEBUG_IF(ShowLightingSamples))
                    {
                        uint8 *TexelPtr = ((uint8 *)LOD->Memory) + Y*LOD->Pitch + X*sizeof(uint32);
                        *(uint32 *)TexelPtr = 0xFFFFFFFF;
                    }

                    ...

                    return(Result);
                }


if we want DEBUG_IF(); to be a light weight operation, it needs to be something 
where the debug system can always access the variable easily.

so here if you can see DEBUG_IF__(); what we are doing is that we are creating a debug_event 
through the InitializeDebugValue(); (which we havent implemented);

THe InitializeDebugValue(); will take in the #define variable that you are interested
for example: DebugValue_ShowLightingSamples


                #define DEBUG_IF(Name)

                
                #define DEBUG_IF__(Name, Path)                                       \
                local_persist debug_event DebugValue##Path = InitializeDebugValue(DebugType_b32, &DebugValue##Variable, #Path); \
                if(DebugValue##Path.Value_b32)

                #define DEBUG_IF_(Path) DEBUG_IF__(Path)
                #define DEBUG_IF(Path) DEBUG_IF_(Path)

do note that Casey didnt finish this part. This will be finalized in tomorrow_s episode


25:23
Casey then wrote the usage code for DEBUG_VARIABLE();
                
                handmade_render_group.cpp

                inline entity_basis_p_result GetRenderEntityBasisP(render_transform *Transform, v3 OriginalP)
                {
                    ...
                    ...

                    if(Transform->Orthographic)
                    {
                        ...
                    }
                    else
                    {
                        real32 OffsetZ = 0.0f;
                    
                        real32 DistanceAboveTarget = Transform->DistanceAboveTarget;

                        DEBUG_IF(Renderer_Camera_UseDebug)
                        {
                            DEBUG_VARIABLE(r32, Renderer_Camera, DebugDistance);
                            
                            DistanceAboveTarget += DebugDistance;
                        }
                    
                        ...
                        ...
                    }
                    
                    return(Result);
                }


28:49
Now Casey writing the #define for DEBUG_VARIABLE

                #define DEBUG_VARIABLE__(type, Path, Variable) \
                local_persist debug_event DebugValue##Variable = InitializeDebugValue(DebugType_##type, &DebugValue##Variable, Path "_" #Variable); \
                type Variable = DebugValue##Variable.Value_##type;

                #define DEBUG_VARIABLE_(type, Path, Variable) DEBUG_VARIABLE__(type, Path, Variable)
                #define DEBUG_VARIABLE(type, Path, Variable) DEBUG_VARIABLE_(type, Path, Variable)



50:27
straight up deleted handmade_debug_variables.h


53:48
Casey made it so that its also easy to compile out these debug 
meaning when we dont have debugging mode turned on. We dont want to create debug_events
for these debug_switches, and code should still flow well. 
we will have these if conditions use default values


                #if 0

                #define DEBUG_IF__(Name, Path)                                       \
                local_persist debug_event DebugValue##Path = InitializeDebugValue(DebugType_b32, &DebugValue##Variable, #Path); \
                if(DebugValue##Path.Value_b32)

                #define DEBUG_VARIABLE__(type, Path, Variable) \
                local_persist debug_event DebugValue##Variable = InitializeDebugValue(DebugType_##type, &DebugValue##Variable, Path "_" #Variable); \
                type Variable = DebugValue##Variable.Value_##type;

                #else

                #define DEBUG_IF__(Path) if(GlobalConstants_##Path)
                #define DEBUG_VARIABLE__(type, Path, Variable) type Variable = GlobalConstants_##Path##_##Variable;

                #endif


1:03:51
right now our #defines debug switches look like this 

                handmade_config.h 

                #define GlobalConstants_Renderer_ShowLightingSamples 0
                #define GlobalConstants_Renderer_Camera_UseDebug 0
                #define GlobalConstants_Renderer_Camera_DebugDistance 25.0f
                #define GlobalConstants_Renderer_Camera_RoomBased 0
                #define GlobalConstants_GroundChunks_Checkerboards 0
                #define GlobalConstants_GroundChunks_RecomputeOnEXEChange 1
                #define GlobalConstants_Renderer_TestWeirdDrawBufferSize 0
                #define GlobalConstants_GroundChunks_Outlines 0
                #define GlobalConstants_AI_Familiar_FollowsHero 0
                #define GlobalConstants_Particles_Test 0
                #define GlobalConstants_Particles_ShowGrid 0
                #define GlobalConstants_Simulation_UseSpaceOutlines 0


Q/A 
1:05:58
For the "shift", "Alt" and "control" key, why use 
    
                NewInput->ShiftDown = (GetKeyState(VK_SHIFT) & (1 << 15));

instead of like mouse keys where we record all the IsDown and IsUp events so we can keep track of halfTransitions?



                win32_handmade.cpp

                case WM_KEYDOWN:
                case WM_KEYUP:
                {

                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }
                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    }
                }

                ...
                ...

                internal void Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
                {
                    if(NewState->EndedDown != IsDown)
                    {
                        NewState->EndedDown = IsDown;
                        ++NewState->HalfTransitionCount;
                    }
                }



Casey:
the answer is becuz "shift", "Alt" and "control" are modifier keys.
the WM_KEYDOWN and WM_KEYUP are nice if you want to detect exactly when keys are up or down.
imagine u want a notification exactly when the keys changes state 
this allows you to get directly to the keys when the key changes.


for "shift", "alt" and "control" here, we dont actually care for the exact moment if these three keys went up or down.
all we care is when we send the mouse inputs to the UI input logic, at that exact moment, if "shift" key is down or not.

there are actually two kinds: there is GetKeyState(); and GetAsyncKeyState();

GetAsyncKeyState(); tells you the state as windows knows it at the time of the call 
GetKeyState(); tells you what your program think it was if all you have been doing is looking 
at the key up and key down messages. 

so actually there is really no difference between you tracking key up and key down yourself for "shift", "alt" and "control"
since you are not doing anything when they are pushed (meaning like, we not doing like a onShiftButton Clicked call);
they are not a trigger key, as we are just checking the state to help us multi select

so for our purposes, GetKeyState(); is perfect. it does exactly what we want to do. 



1:09:56
I dont quite get why you need to do nested #defines

mostly becuz the paste operate paste the macro arguments directly


so Casey demo this with the following code snippet. 

Assume we have the following #defines. 

                #define ThreeTime__(Param) #Param
                #define ThreeTime_(Param) ThreeTime__(Param)
                #define ThreeTime(Param) THreeTime_(Param)


                #define TwoTime_(Param) #Param
                #define TwoTime(Param) TwoTime_(Param)


                #define OneTime(Param) #Param

then we have this code

                char* OneTimeResult = OneTime(Foo);
                char* TwoTimeResult = TwoTime(Foo);
                char* ThreeTimeResult = ThreeTime(Foo);

the result is three strings exactly the same 

                Foo 
                Foo 
                Foo


now lets say we want to Stringizing the line number into a string 


                #define ThreeTime__(Param) #Param
                #define ThreeTime_(Param) ThreeTime__(Param)
                #define ThreeTime(Param) THreeTime_(__LINE__)


                #define TwoTime_(Param) #Param
                #define TwoTime(Param) TwoTime_(__LINE__)


                #define OneTime(Param) #__LINE__

the OneTime will actually give you a compiler error, cuz __LINE__ itself is a macro

            TwoTimeResult gives you: "__LINE__"
            ThreeTimeResults gives you the : "697"


this is also mentioned in here 
https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html

                "if you want to stringize the result of expansion of a macro argument, you have to use two levels of macros"