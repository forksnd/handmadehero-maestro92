Handmade Hero Day 193 - Run-time Setting of Compile-time Variables

Summary:
followed up on day 192, went through our code, added a bunch more DEBUGUI_xxxxx #defines 
that we can toggle on and off through the UI.

added a collision body radisu to the radial menu


struggles to give an example that results in significant performance boost 
of where all this dynamic toggleing of #define is better than just having booleans. 

Keyword:
build process, UI, debug system 




4:06
So just like in day 192, our code have been littered with #if 0.
Casey begings to go through all of them, giving them a #define variable name 
and migrated them to the handmade_config.h

                handmade_config.h

                #define DEBUGUI_UseDebugCamera 0
                #define DEBUGUI_GroundChunkOutlines 0
                #define DEBUGUI_ParticleTest 0
                #define DEBUGUI_ParticleGrid 0
                #define DEBUGUI_UseSpaceOutlines 0
                #define DEBUGUI_GroundChunkCheckerboards 1
                #define DEBUGUI_RecomputeGroundChunksOnEXEChange 0
                #define DEBUGUI_TestWeirdDrawBufferSize 1
                #define DEBUGUI_FamiliarFollowsHero 1
                #define DEBUGUI_ShowLightingSamples 0
                #define DEBUGUI_UseRoomBasedCamera 0



then in WriteHandmadeConfig(); recall in day 192, we were only doing it for one DebugVariable.
now we hae to do it to all of them.


29:27
notice that Casey prefixed them with DEBUGUI, thats cuz if we ever want to find them, it will be easy for us to find them. 

                

31:02
so then Casey made a list that describes all of our debug variables. 

                handmade_debug.cpp

                enum debug_variable_type
                {
                    DebugVariableType_Boolean,
                };

                struct debug_variable
                {
                    debug_variable_type Type;
                    char *Name;
                    b32 Value;
                };



32:02
Then Casey added a new file to for the DebugVariableList


                handmade_config_variables.h

                #if !defined(HANDMADE_DEBUG_VARIABLES_H)

                #define DEBUG_VARIABLE_LISTING(Name) DebugVariableType_Boolean, #Name, Name

                // TODO(casey): Add _distance_ for the debug camera, so we have a float example
                // TODO(casey): Parameterize the fountain?
                debug_variable DebugVariableList[] =
                {
                    DEBUG_VARIABLE_LISTING(DEBUGUI_UseDebugCamera),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_GroundChunkOutlines),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_ParticleTest),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_ParticleGrid),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_UseSpaceOutlines),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_GroundChunkCheckerboards),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_RecomputeGroundChunksOnEXEChange),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_TestWeirdDrawBufferSize),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_FamiliarFollowsHero),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_ShowLightingSamples),
                    DEBUG_VARIABLE_LISTING(DEBUGUI_UseRoomBasedCamera),
                };

                #define HANDMADE_DEBUG_VARIABLES_H
                #endif



33:40
then in our WriteHandmadeConfig, we just iterate through our DebugVariableList to output it into our 
handmade_config.h file 


                handmade_debug.cpp

                internal void WriteHandmadeConfig(debug_state *DebugState)
                {
                    char Temp[4096];
                    char *At = Temp;
                    char *End = Temp + sizeof(Temp);
                    for(u32 DebugVariableIndex = 0;
                        DebugVariableIndex < ArrayCount(DebugVariableList);
                        ++DebugVariableIndex)
                    {
                        debug_variable *Var = DebugVariableList + DebugVariableIndex;
                        At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At),
                                          "#define %s %d\n", Var->Name, Var->Value);
                    }    
                    Platform.DEBUGWriteEntireFile("../code/handmade_config.h", (u32)(At - Temp), Temp);

                    if(!DebugState->Compiling)
                    {
                        DebugState->Compiling = true;
                        DebugState->Compiler = Platform.DEBUGExecuteSystemCommand("..\\code", "c:\\windows\\system32\\cmd.exe", "/C build.bat");
                    }
                }



41:01
so in the DrawDebugMainMenu(); previously we just hard coded some menu items, now we will just have the radial menu 
render from the DebugVariableList, and we will be toggling them 


                handmade_debug.cpp 

                internal void
                DrawDebugMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {  
                    ...
                    ...

                    for(u32 MenuItemIndex = 0;
                        MenuItemIndex < ArrayCount(DebugVariableList);
                        ++MenuItemIndex)
                    {
                        debug_variable *Var = DebugVariableList + MenuItemIndex;
                        char *Text = Var->Name;
                        
                        ...
                        ...

                        DEBUGTextOutAt(TextP - 0.5f*GetDim(TextBounds), Text, ItemColor);
                    }

                    ...
                    ...
                }





43:45
its not quite as well as having these booleans directly in the program, it does have to do the recompilation,
but its vastly more powerful cuz we actually just insert things that are extremely costly, and turn them on and off.

and we dont have to put in branches to might affect performance. 



48:36
Casey adding the concept of not selecting a Radial Menu item 
Currently, there is no way not to select something, as it just compares it by distance.
it just selects the one that is it closest to. 

so Casey kind of just giving the Radial Menu a collider body radius, to see if 
if the point of hte mouse is inside that MenuRadius

                handmade_debug.cpp

                internal void DrawDebugMainMenu(debug_state *DebugState, render_group *RenderGroup, v2 MouseP)
                {  
                    u32 NewHotMenuIndex = ArrayCount(DebugVariableList);
                    r32 BestDistanceSq = Real32Maximum;
                    
                    r32 MenuRadius = 400.0f;
                    r32 AngleStep = Tau32 / (r32)ArrayCount(DebugVariableList);
                    for(u32 MenuItemIndex = 0; MenuItemIndex < ArrayCount(DebugVariableList); ++MenuItemIndex)
                    {
                        ...
                        ...

                        r32 ThisDistanceSq = LengthSq(TextP - MouseP);
                        if(BestDistanceSq > ThisDistanceSq)
                        {
                            NewHotMenuIndex = MenuItemIndex;
                            BestDistanceSq = ThisDistanceSq;
                        }

                        ...
                    }

                    if(LengthSq(MouseP - DebugState->MenuP) > Square(MenuRadius))
                    {
                        DebugState->HotMenuIndex = NewHotMenuIndex;
                    }
                    else
                    {
                        DebugState->HotMenuIndex = ArrayCount(DebugVariableList);
                    }
                }



52:15 
Casey mentioned that the only thing hes not too satisfied is the compilation time is a bit slow for his standards





Q/A 

54:44
someone mentioned that he doest understand why a single if statement can be expensive.
Meaning why do all of this instead of just using a boolean.


so what Casey tried is that in the DrawRectangleQuickly() function, which is a very function with 
very high hit rate. (we come to here for every pixel); we added an if condition

                handmade_optimized.cpp

                void DrawRectangleQuickly()
                {

                    for(int Y = MinY; Y < MaxY; Y += 2)
                    {
                        ...
                        ...
                        for(int XI = MinX; XI < MaxX; XI += 4)
                        { 

                            if(a boolean)
                            {
                               do some shit  
                            }

                        }
                    }
                }

and he compared it with. 

                handmade_optimized.cpp

                void DrawRectangleQuickly()
                {

                    for(int Y = MinY; Y < MaxY; Y += 2)
                    {
                        ...
                        ...
                        for(int XI = MinX; XI < MaxX; XI += 4)
                        { 

                           do some shit      

                        }
                    }
                }

but it turns out there wasnt much performance drop.
what Casey was worried that if in some high performance sensitive code, if you put a branch.
it might be expensive.            
