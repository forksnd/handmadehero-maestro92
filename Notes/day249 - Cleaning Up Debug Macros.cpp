Handmade Hero Day 249 - Cleaning Up Debug Macros

Summary:
added the DEBUG_PROFILE for adding profiler view onto the debug menu view 
Cleaned up debug macros, such as DEBUG_VALUE, TIMED_BLOCK_

Keyword:
Debug System



2:03
Casey wants to add back the profiler functionality
so Casey added the DEBUG_PROFILE #define 



                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...                 

                    DEBUG_BEGIN_DATA_BLOCK(Game, DEBUG_POINTER_ID(Memory));
                    DEBUG_VALUE(Global_Renderer_Camera_UseDebug);
                    DEBUG_VALUE(Global_Renderer_Camera_DebugDistance);
                    DEBUG_VALUE(Global_Renderer_Camera_RoomBased);
                    DEBUG_VALUE(Global_GroundChunks_Checkerboards);
                    DEBUG_VALUE(Global_GroundChunks_RecomputeOnEXEChange);
                    DEBUG_VALUE(Global_Renderer_TestWeirdDrawBufferSize);
                    DEBUG_VALUE(Global_GroundChunks_Outlines);
                    DEBUG_VALUE(Global_AI_Familiar_FollowsHero); 
                    DEBUG_VALUE(Global_Particles_Test); 
                    DEBUG_VALUE(Global_Particles_ShowGrid);
                    DEBUG_VALUE(Global_Simulation_UseSpaceOutlines);

                    DEBUG_BEGIN_DATA_BLOCK(Game, DEBUG_POINTER_ID(Memory));
    ----------->    DEBUG_PROFILE(GameUpdateAndRender);
                    DEBUG_END_DATA_BLOCK();


                    DEBUG_END_DATA_BLOCK();

                    ...
                    ...
                }





3:28
so Casey added the #define for DEBUG_PROFILE below:

                handmade_debug_interface.h

                #define DEBUG_DATA_BLOCK(Name) debug_data_block DataBlock__(Name)

                #define DEBUG_VALUE(Value)  \
                     { \
                         RecordDebugEvent(DebugType_Unknown, #Value);                              \
                         DEBUGValueSetEventData(Event, Value);                          \
                     } 

                #define DEBUG_PROFILE(FunctionName) \
                     { \
                         RecordDebugEvent(DebugType_CounterFunctionList, #FunctionName);                   \
                     } 

18:06
Casey once again mentions how he doesnt like the DEBUG_POINTER_ID
We still need to generate an unique id for debug records, so that we can uniquely identify these DEBUG records

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...                 

                    DEBUG_BEGIN_DATA_BLOCK(Game, DEBUG_POINTER_ID(Memory));
                    DEBUG_VALUE(Global_Renderer_Camera_UseDebug);
                    DEBUG_VALUE(Global_Renderer_Camera_DebugDistance);
                    DEBUG_VALUE(Global_Renderer_Camera_RoomBased);
                    DEBUG_VALUE(Global_GroundChunks_Checkerboards);
                    DEBUG_VALUE(Global_GroundChunks_RecomputeOnEXEChange);
                    DEBUG_VALUE(Global_Renderer_TestWeirdDrawBufferSize);
                    DEBUG_VALUE(Global_GroundChunks_Outlines);
                    DEBUG_VALUE(Global_AI_Familiar_FollowsHero); 
                    DEBUG_VALUE(Global_Particles_Test); 
                    DEBUG_VALUE(Global_Particles_ShowGrid);
                    DEBUG_VALUE(Global_Simulation_UseSpaceOutlines);

                    DEBUG_BEGIN_DATA_BLOCK(Game, DEBUG_POINTER_ID(Memory));
                    DEBUG_PROFILE(GameUpdateAndRender);
                    DEBUG_END_DATA_BLOCK();


                    DEBUG_END_DATA_BLOCK();

                    ...
                    ...
                }

so we can either convert the "Global_Renderer_Camera_UseDebug" argument into strings (assuming all the names we are giving to
all fo the debug events will be unique); or we use some kind of GUID 

The reason why Casey doesnt really like the string idea is because we may give multiple debug blocks that all have the same name.

if we do the string method, we just hash the string to look up the id. 

so we we went with the string approach.


Casey also did more refactoring where we got rid of the DATA_END_DATA_BLOCK();
right now the DATA_END_DATA_BLOCK are just implicit 


                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    Platform = Memory->PlatformAPI;    
                    
                #if HANDMADE_INTERNAL
                    DebugGlobalMemory = Memory;
                    
                    {DEBUG_DATA_BLOCK("Renderer");
                        DEBUG_VALUE(Global_Renderer_TestWeirdDrawBufferSize);
                        {DEBUG_DATA_BLOCK("Renderer/Camera");
                            DEBUG_VALUE(Global_Renderer_Camera_UseDebug);
                            DEBUG_VALUE(Global_Renderer_Camera_DebugDistance);
                            DEBUG_VALUE(Global_Renderer_Camera_RoomBased);
                        }
                    }
                    {DEBUG_DATA_BLOCK("GroundChunks");
                        DEBUG_VALUE(Global_GroundChunks_Checkerboards);
                        DEBUG_VALUE(Global_GroundChunks_RecomputeOnEXEChange);
                        DEBUG_VALUE(Global_GroundChunks_Outlines);
                    }
                    {DEBUG_DATA_BLOCK("AI/Familiar");
                        DEBUG_VALUE(Global_AI_Familiar_FollowsHero); 
                    }
                    {DEBUG_DATA_BLOCK("Particles");
                        DEBUG_VALUE(Global_Particles_Test); 
                        DEBUG_VALUE(Global_Particles_ShowGrid);
                    }
                    {DEBUG_DATA_BLOCK("Simulation");
                        DEBUG_VALUE(Global_Simulation_UseSpaceOutlines);
                    }
                    {DEBUG_DATA_BLOCK("Profile");
                        DEBUG_PROFILE(GameUpdateAndRender);
                    }
                    ...
                    ...
                }

32:44
for the past 20 minutes, Casey wrestled with organizing his RecordDebugEvent code.
Casey mentioned that this is why he loves metaprogramming so much because you never had to deal with this 
ridiculousness in metaprogramming. All of this work is completely unecessarily.

C++ doest give good support for marking up your code. 



40:09
Casey cleaned up a bunch of Debug Macros 
                
                handmade_debug_interface.h 

                #define TIMED_BLOCK__(BlockName, Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, BlockName, ## __VA_ARGS__)
                #define TIMED_BLOCK_(BlockName, Number, ...) TIMED_BLOCK__(BlockName, Number, ## __VA_ARGS__)
                #define TIMED_BLOCK(BlockName, ...) TIMED_BLOCK_(#BlockName, __LINE__, ## __VA_ARGS__)
                #define TIMED_FUNCTION(...) TIMED_BLOCK_((char *)__FUNCTION__, __LINE__, ## __VA_ARGS__)

                #define BEGIN_BLOCK_(Counter, FileNameInit, LineNumberInit, BlockNameInit)          \
                    {RecordDebugEvent(DebugType_BeginBlock, BlockNameInit);}
                #define END_BLOCK_(Counter) \
                    { \
                        RecordDebugEvent(DebugType_EndBlock, "End Block"); \
                    }
                    
                #define BEGIN_BLOCK(Name) \
                    int Counter_##Name = __COUNTER__;                       \
                    BEGIN_BLOCK_(Counter_##Name, __FILE__, __LINE__, #Name);

                #define END_BLOCK(Name) \
                    END_BLOCK_(Counter_##Name);
                    

now we have. Notice we got rid of the idea of counters. that was just some residual code from one of our earlier 
experimental code.


                handmade_debug_interface.h 

                #define TIMED_BLOCK__(GUID, Number, ...) timed_block TimedBlock_##Number(GUID, ## __VA_ARGS__)
                #define TIMED_BLOCK_(GUID, Number, ...) TIMED_BLOCK__(GUID, Number, ## __VA_ARGS__)
                #define TIMED_BLOCK(Name, ...) TIMED_BLOCK_(DEBUG_NAME(Name), ## __VA_ARGS__)
                #define TIMED_FUNCTION(...) TIMED_BLOCK_(DEBUG_NAME(__FUNCTION__), ## __VA_ARGS__)

                #define BEGIN_BLOCK_(GUID) {RecordDebugEvent(DebugType_BeginBlock, GUID);}
                #define END_BLOCK_(GUID) {RecordDebugEvent(DebugType_EndBlock, GUID);}

                #define BEGIN_BLOCK(Name) BEGIN_BLOCK_(DEBUG_NAME(Name))
                #define END_BLOCK() END_BLOCK_(DEBUG_NAME("END_BLOCK_"))



42:01
also noticed that our TIMED_BLOCK__ has cleaned up signficantly.
previously we had:

                handmade_debug_interface.h

                struct timed_block
                {
                    int Counter;
                    
                    timed_block(int CounterInit, char *FileName, int LineNumber, char *BlockName, u32 HitCountInit = 1)
                    {
                        // TODO(casey): Record the hit count value here?
                        Counter = CounterInit;
                        BEGIN_BLOCK_(Counter, FileName, LineNumber, BlockName);
                    }
                    
                    ~timed_block()
                    {
                        END_BLOCK_(Counter);
                    }
                };

now we just have:

we have the UniqueFileCouterString__ to create unique GUIDs, then we can just simplify our timed_block calls.

                handmade_debug_interface.h

                struct timed_block
                {
                    timed_block(char *GUID, u32 HitCountInit = 1)
                    {
                        BEGIN_BLOCK_(GUID);
                        // TODO(casey): Record the hit count value here?
                    }
                    
                    ~timed_block()
                    {
                        END_BLOCK();
                    }
                };


52:19
while debugging one of his macros, casey outputted the preprocessor output to a file to debug 
so the command Casey used was 

    cl -DHANDM_MADE_INTERNAL=1 /EP handmade.cpp > preprocess.tmp


cl is the microsoft compiler. /EP is the preprocessor options

and surely enought Casey was able to debug his macros.


57:01
Casey also mentions that this is another benefit of the single compilation unit build. 
if you want to preprocess a file, you can preprocess a whole thing