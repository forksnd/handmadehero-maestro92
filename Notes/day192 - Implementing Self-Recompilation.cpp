Handmade Hero Day 192 - Implementing Self-Recompilation

Summary:
created a "handmade_config.h" file, where we define a bunch of #defines for debugs. 

made it so that we can modify #define values in the "handmade_config.h" through the game ui,
and it will recompile the game executable programmatically.

Keyword:
build process, UI, debug system 


4:20
Casey plans to look at the debug code. He mentioned that there are plenty of times where we #if 0 things out,

So Casey wants to make the debug functionality a more fundamentally accessible thing 



5:22
Casey_s dream scenario is that he click the right mouse button, and you get access to all the debugging tech.

Casey wants to think about what is a good way to make the debug functionality turn on and off, and he wants to reorganize that code into
the debug system.


6:30
the most simple example of a debgy functionality is our the camera, there was this thing we can pull back our camera for debugging 


so previously in our GetRenderEntityBasisP(); function, we had the following #if 0

                handmade_render_group.cpp

                inline entity_basis_p_result GetRenderEntityBasisP(render_transform *Transform, v3 OriginalP)
                {
                    ...
                    ...

                        real32 DistanceAboveTarget = Transform->DistanceAboveTarget;
                #if 0
                        // TODO(casey): How do we want to control the debug camera?
                        if(1)
                        {
                            DistanceAboveTarget += 50.0f;
                        }
                #endif
                    
                    ...
                    ...

                    return(Result);
                }


so if Casey change it to #if 1, it will just bring the camera out, everything else is exactly the same



9:43
while Casey is talking about this debug camera stuff, we have encountered a bug where in the CollateDebugData(); function 
since we do incremental processing, since we do a code reload, we lose the pointer of debug memory.



14:20
so in the DEBUG_GAME_FRAME_END();, we add a check to see if our Executable is reloaded or not.
if so, we restart the Collation

                handmade_debug.cpp 

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    ...
                    ...

                    debug_state *DebugState = DEBUGGetState(Memory);
                    if(DebugState)
                    {        
        ------------>   if(Memory->ExecutableReloaded)
                        {
                            RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
                        }

                        if(!DebugState->Paused)
                        {
                            if(DebugState->FrameCount >= MAX_DEBUG_EVENT_ARRAY_COUNT*4)
                            {
                                RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
                            }
                            CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
                        }        
                    }

                    return(GlobalDebugTable);
                }


15:40
Back to our debug camera, Casey mentioned that while this debug functionality of pulling the camera works,
one problem he has is that, he doesnt want to have to find that #if 0 in the code and manually change it. 

[this is just like in our situation, when you want to a designer to come in and play around things, you wont expect him 
to go inside the code and change that #if value, instead you will put it in a config file.]




16:43
so what Casey did is that, he changed the #if define value name to DEBUGUI_UseDebugCamera

                handmade_render_group.cpp

                inline entity_basis_p_result GetRenderEntityBasisP(render_transform *Transform, v3 OriginalP)
                {
                    ...
                    ...

                        real32 DistanceAboveTarget = Transform->DistanceAboveTarget;
                #if DEBUGUI_UseDebugCamera
                        // TODO(casey): How do we want to control the debug camera?
                        if(1)
                        {
                            DistanceAboveTarget += 50.0f;
                        }
                #endif
                    
                    ...
                    ...

                    return(Result);
                }



So Casey added the definition of DEBUGUI_UseDebugCamera in a completely new handmade_config.h file.


                handmade_config.h

                #define DEBUGUI_UseDebugCamera 0 // b32


also, he then added this file into the platform layer 


                handmade_platform.h 

                #include "handmade_config.h"

                ...
                ...

what Casey wants to do is, when he goes into handmade_config.h and change the value DEBUGUI_UseDebugCamera
lets say he changes it from 0 to 1, he wants the build process to automatically recompile the code.
so thats what we will attempt to work on today. 



21:13
as a matter of fact, What Casey wanted is even more.

Imagine the case where the user is messing with the Debug ui radial menu, and one of the option is zoom out debug camera,

so the user selects that "zoom out debug camera" option, then the game zooms out and starts using the debug camera.

that will require the game code to edit the DEBUGUI_UseDebugCamera value in the handmade_config.h file, 
and then automatically recompile to load the new executable 




22:26
so in the handmade_debug.cpp file, Casey adds the WriteHandmadeCondig function

this function uses the platform layer to write then entire file.

-                   int TempSize = _snprintf_s(Temp, sizeof(Temp), "#define DEBUGUI_UseDebugCamera %d // b32\n", UseDebugCamera);

    calcualtes the size of the string 



-   full code below:

                handmade_debug.cpp

                internal void WriteHandmadeConfig(debug_state *DebugState, b32 UseDebugCamera)
                {
                    char Temp[4096];
                    int TempSize = _snprintf_s(Temp, sizeof(Temp), "#define DEBUGUI_UseDebugCamera %d // b32\n",
                                               UseDebugCamera);
                    Platform.DEBUGWriteEntireFile("../code/handmade_config.h", TempSize, Temp);

                    ...
                    ...
                }


[notice that the part where we programmically recompile the file will come later]


27:28
so Casey wanted it so that everytime the user does a gesture, it will rewrite the config file 

[so its kind of like in game2, where we changing local settings, like skipping FTUE, or changing language]

so in the DEBUGEnd(); function case, where we do mouse right clicks, and we pause or turn the ProfileOn flag, 
we would write to our Config file. [in the future, they may be an option of choosing the debug camera. the more 
important thing here is that everytime we do a gesture, we Rewrite the cofig file and recompile]



                handmade_debug.cpp

                internal void DEBUGEnd(game_input *Input, loaded_bitmap *DrawBuffer)
                {
                    ...

                    debug_state *DebugState = DEBUGGetState();
                    if(DebugState)
                    {
                        ...
                        ...

                        if(Input->MouseButtons[PlatformMouseButton_Right].EndedDown)
                        {
                            ...
                            ...
                        }
                        else if(Input->MouseButtons[PlatformMouseButton_Right].HalfTransitionCount > 0)
                        {
                            DrawDebugMainMenu(DebugState, RenderGroup, MouseP);
                            switch(DebugState->HotMenuIndex)
                            {
                                case 0:
                                {
                                    DebugState->ProfileOn = !DebugState->ProfileOn;
                                } break;

                                case 1:
                                {
                                    DebugState->Paused = !DebugState->Paused;
                                } break;
                            }

        -------------->     WriteHandmadeConfig(DebugState, !DEBUGUI_UseDebugCamera);
                        }


now whenver we do a gesture, our values in the handmade_config.h will get changed. 
but we still have to manully recompile our executable


28:43
so to make the code recompile, we have the platform layer to execute system commands.

                handmade_debug.cpp

                internal void WriteHandmadeConfig(debug_state *DebugState, b32 UseDebugCamera)
                {
                    char Temp[4096];
                    int TempSize = _snprintf_s(Temp, sizeof(Temp), "#define DEBUGUI_UseDebugCamera %d // b32\n",
                                               UseDebugCamera);
                    Platform.DEBUGWriteEntireFile("../code/handmade_config.h", TempSize, Temp);

                    if(!DebugState->Compiling)
                    {
                        Platform.DEBUGExecuteSystemCommand("..\\code", "c:\\windows\\system32\\cmd.exe", "/C build.bat");
                    }
                }

so the cmd is the command line interpreter

so when we are actually in the command line, the command to run our build.bat script is 

                c:\windows\System32\cmd.exe /c build.bat 

in the ../code directory

/c is just one of the options                


30:39
so the question is, can we do this? 
if regular programs can exectube system commands, of course we can do this. 

so in handmade_platform.h, we add our DEBUGExecuteSystemCommand(); function 

                handmade_platform.h 

                #define DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(name) debug_executing_process name(char *Path, char *Command, char *CommandLine)
                typedef DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(debug_platform_execute_system_command);

32:21
then in the win32 layer, we write our execute system command function

while Casey is googling, he did mention that the winapi ShellExecute(); if not what we want. 
cuz ShellExecute(); goes through the shell, and thats more for opening a file, for example, the way windows explorer opening a file.

-   the "Command" variable here is  
                
                "c:\\windows\\system32\\cmd.exe"


-   the "CommandLine" variable is 

                "/C build.bat"

    so we are creating cmd.exe process that will run our command line 


-   full code below:                

                win32_handmade.cpp

                DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(DEBUGExecuteSystemCommand)
                {
                    debug_executing_process Result = {};
                    
                    STARTUPINFO StartupInfo = {};
                    StartupInfo.cb = sizeof(StartupInfo);
                    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
                    StartupInfo.wShowWindow = SW_HIDE;
                    
                    PROCESS_INFORMATION ProcessInfo = {};    
                    if(CreateProcess(Command,
                                     CommandLine,
                                     0,
                                     0,
                                     FALSE,
                                     0,
                                     0,
                                     Path,
                                     &StartupInfo,
                                     &ProcessInfo))
                    {
                        // TODO
                    }
                    return(Result);
                }

39:23 
so when Casey first wrote the execute system command function, it didnt work.
So Casey printed out the error code to help him debug

-   the SW_HIDE option is done to hide the window that originially pops out.

                win32_handmade.cpp

                DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(DEBUGExecuteSystemCommand)
                {

                    STARTUPINFO StartupInfo = {};
                    StartupInfo.cb = sizeof(StartupInfo);
                    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
                    StartupInfo.wShowWindow = SW_HIDE;
                    
                    PROCESS_INFORMATION ProcessInfo = {};    
                    if(CreateProcess(Command,
                                     CommandLine,
                                     0,
                                     0,
                                     FALSE,
                                     0,
                                     0,
                                     Path,
                                     &StartupInfo,
                                     &ProcessInfo))
                    {

                    }
                    else
                    {
    -------------->     DWORD ErrorCode = GetLastError();
                    }

                    return(Result);
                }

so visual studio has this useful Error look up widnow you can use to directly look up 
what each error means 

go to "Tools->Error Lookup", then you can just put in your error number and see whats wrong.



46:54
now that it works, Casey mentioned that since we dont show the window of us recompiling 
we need to know if we got some compile error. 

so when we call CreateProcess(); we will get back a PROCESS_INFORMATION ProcessInfo variable. 
So what Casey want is to somehow get the state of the process. The process here refers to the cmd.exe process.

                

49:57
so the first thing Casey tried is the WaitForSingleObject(); winapi                

so Casey first tried to call this function with the INFINITE parameter. The idea is that we will have the game 
locked up, until the compiling is done.

                win32_handmade.cpp

                DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(DEBUGExecuteSystemCommand)
                {

                    STARTUPINFO StartupInfo = {};
                    StartupInfo.cb = sizeof(StartupInfo);
                    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
                    StartupInfo.wShowWindow = SW_HIDE;
                    
                    PROCESS_INFORMATION ProcessInfo = {};    
                    if(CreateProcess(Command,
                                     CommandLine,
                                     0,
                                     0,
                                     FALSE,
                                     0,
                                     0,
                                     Path,
                                     &StartupInfo,
                                     &ProcessInfo))
                    {
                        WaitForSingleObject(ProcessInfo.hProcess, INFINITE)
                    }   
                    else
                    {
    -------------->     DWORD ErrorCode = GetLastError();
                    }

                    return(Result);
                }




then according to the MSDN specs, Casey is picturing a structure like below:

                win32_handmade.cpp

                DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(DEBUGExecuteSystemCommand)
                {

                    STARTUPINFO StartupInfo = {};
                    StartupInfo.cb = sizeof(StartupInfo);
                    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
                    StartupInfo.wShowWindow = SW_HIDE;
                    
                    PROCESS_INFORMATION ProcessInfo = {};    
                    if(CreateProcess(Command,
                                     CommandLine,
                                     0,
                                     0,
                                     FALSE,
                                     0,
                                     0,
                                     Path,
                                     &StartupInfo,
                                     &ProcessInfo))
                    {
                        if(WaitForSingleObject(ProcessInfo.hProcess, 0) == WAIT_OBJECT_0)
                        {

                        }
                    }   
                    else
                    {
    -------------->     DWORD ErrorCode = GetLastError();
                    }

                    return(Result);
                }



51:02
so what Casey decided is that he will give the debug system a way of managing the process. That way
the debug system can know the status of the compile 

so Casey defined the following two struct 

                handmade_platform.h

                typedef struct debug_executing_process
                {
                    u64 OSHandle;
                } debug_executing_process;
                    
                typedef struct debug_process_state
                {
                    b32 StartedSuccessfully;
                    b32 IsRunning;
                    s32 ReturnCode;
                } debug_process_state;


53:26
Casey also added this platform api where the game code can query about the debug_process state


                #define DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(name) debug_executing_process name(char *Path, char *Command, char *CommandLine)
                typedef DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(debug_platform_execute_system_command);

                // TODO(casey): Do we want a formal release mechanism here?
                #define DEBUG_PLATFORM_GET_PROCESS_STATE(name) debug_process_state name(debug_executing_process Process)
                typedef DEBUG_PLATFORM_GET_PROCESS_STATE(debug_platform_get_process_state);


so the idea is that, when the game code runs the DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(); command from the platform layer,
it will get a debug_executing_process.

then the game code can also query the status of this debug_executing_process by calling DEBUG_PLATFORM_GET_PROCESS_STATE();
which it will get a debug_process_state back 


55:27
so then finally in our DEBUGExecuteSystemCommand function, we have the following:
in which, we are just returning the debug_executing_process with the OSHandle populated 

                win32_handmade.cpp

                DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(DEBUGExecuteSystemCommand)
                {
                    debug_executing_process Result = {};
                    
                    STARTUPINFO StartupInfo = {};
                    StartupInfo.cb = sizeof(StartupInfo);
                    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
                    StartupInfo.wShowWindow = SW_HIDE;
                    
                    PROCESS_INFORMATION ProcessInfo = {};    
                    if(CreateProcess(Command,
                                     CommandLine,
                                     0,
                                     0,
                                     FALSE,
                                     0,
                                     0,
                                     Path,
                                     &StartupInfo,
                                     &ProcessInfo))
                    {
                        Assert(sizeof(Result.OSHandle) >= sizeof(ProcessInfo.hProcess));
    --------------->    *(HANDLE *)&Result.OSHandle = ProcessInfo.hProcess;
                    }
                    else
                    {
                        DWORD ErrorCode = GetLastError();
                        *(HANDLE *)&Result.OSHandle = INVALID_HANDLE_VALUE;
                    }

                    return(Result);
                }






56:31
then we write the DEBUG_PLATFORM_GET_PROCESS_STATE(); function 
as you can see, we moved WaitForSingleObject(); function into here

-   so from the MSDN spec, we know that if the WaitForSingleObject(); returns WAIT_OBJECT_0,
    that means the the process is no longer running. Then we just return the os handle to windows 


-   Casey did had concerns about this API, whether "Do we want a formal release mechnaism here"


                win32_handmade.cpp

                DEBUG_PLATFORM_GET_PROCESS_STATE(DEBUGGetProcessState)
                {
                    debug_process_state Result = {};

                    HANDLE hProcess = *(HANDLE *)&Process.OSHandle;
                    if(hProcess != INVALID_HANDLE_VALUE)
                    {
                        Result.StartedSuccessfully = true;

                        if(WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0)
                        {
                            DWORD ReturnCode = 0;
                            GetExitCodeProcess(hProcess, &ReturnCode);
                            Result.ReturnCode = ReturnCode;
                            CloseHandle(hProcess);
                        }
                        else
                        {
                            Result.IsRunning = true;
                        }
                    }

                    return(Result);
                }





1:00:35
then in our debug_state, we can add the debug_executing_process variable for the cmd.exe which will be compiling our executable 


                handmade_debug.h

                struct debug_state
                {
                    ...
                    ...

                    b32 Compiling;
                    debug_executing_process Compiler;
                    
                    ...
                    ...
                };





1:01:27
then when we actually run our WriteHandmadeConfig(); function, we will set the DebugState->Compiling flag to true and 
DebugState->Compiler to what we get back from the platform layer .

                handmade_debug.cpp

                internal void WriteHandmadeConfig(debug_state *DebugState, b32 UseDebugCamera)
                {
                    char Temp[4096];
                    int TempSize = _snprintf_s(Temp, sizeof(Temp), "#define DEBUGUI_UseDebugCamera %d // b32\n",
                                               UseDebugCamera);
                    Platform.DEBUGWriteEntireFile("../code/handmade_config.h", TempSize, Temp);

                    if(!DebugState->Compiling)
                    {
                        DebugState->Compiling = true;
                        DebugState->Compiler = Platform.DEBUGExecuteSystemCommand("..\\code", "c:\\windows\\system32\\cmd.exe", "/C build.bat");
                    }
                }




1:02:15
lastly in our DEBUGEnd(); function, we just check every frame and see if DebugState->Compiling is still 
compiling or not. if so, we just render "COMPILING" on to our screen by calling DEBUGTextLine("COMPILING");

                handmade_debug.cpp

                internal void DEBUGEnd(game_input *Input, loaded_bitmap *DrawBuffer)
                {
                    TIMED_FUNCTION();
                    
                    debug_state *DebugState = DEBUGGetState();
                    if(DebugState)
                    {
                        ...
                        ...

                        if(DebugState->Compiling)
                        {
                            debug_process_state State = Platform.DEBUGGetProcessState(DebugState->Compiler);
                            if(State.IsRunning)
                            {
                                DEBUGTextLine("COMPILING");
                            }
                            else
                            {
                                DebugState->Compiling = false;
                            }
                        }

                        ...
                        ...
                    }
                }

Q/A:
1:09:49
why not just use a boolean instead of recompiling a #define?

cuz we wont have performance overhead. For example, we can put tons of debug code anywhere in our game 
including our profiling blocks. So we can turn the profiling blocks dynamically through the games UI. 



