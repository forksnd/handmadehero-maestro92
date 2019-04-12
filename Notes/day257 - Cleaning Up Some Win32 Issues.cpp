Handmade Hero Day 257 - Cleaning Up Some Win32 Issues

Summary:
added the HANDMADE_STREAMING environment variable so that on other machines, we will use Double buffer in OpenGL
only on Casey_s machine, we dont because we need to support OBS streaming

removed the fader

moved the debug_table to the win32_platform layer from the game code dll
so that its memory doesnt change during dll reloading. 


Keyword:
debug system, profiler 

1:00
mentioned that we previously, to make OBS streaming work, we put in this "Hack", where we turn off 
double buffer. The solution he wants is that he only wants WGL_DOUBLE_BUFFER_ARB off on his machine,
but on on everyone else_s machine

                win32_handmade.cpp

                internal void Win32SetPixelFormat(HDC WindowDC)
                {
                    int SuggestedPixelFormatIndex = 0;
                    GLuint ExtendedPick = 0;
                    if(wglChoosePixelFormatARB)
                    {
                        int IntAttribList[] =
                        {
                            ...
                            ...
                #if HANDMADE_STREAMING
                            WGL_DOUBLE_BUFFER_ARB, GL_FALSE,
                #else
                            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                #endif
                            ...
                            ...
                        };
                    }
                }



So casey added a line to see if an "windows environment" variable is defined in batch

                build.bat

                IF "%HANDMADE_STREAMING%"=="" goto nostreaming
                set CommonCompilerFlags=%CommonCompilerFlags% -DHANDMADE_STREAMING=1
                :nostreaming

when Casey meant "windows environment variable", he literally meant windows environment variable, where he went to 
"My Computer", "System Properties", "Environment Variables" and added the HANDMADE_STREAMING the environment variable 


19:20
Casey mentioned that he doesnt plan to support the fader (fading into the game from desktop to first cut scene on game startup); in the future.
he claims that the fading is a bad idea, because you are taking something that is often very nasty, which is initalizing the 3D card, 
and we are now adding something else that can go wrong on top of it. So ideally, the fader should just get removed. 

trying to have the fadeout over the desktop, and fading into the game is a lot of complexities, and you have to deal with that on every
platform. So Casey doesent want to deal with it. 

so Casey removed it 


37:05
Casey fixed the debug string being messed up after reloading

 
44:38
So Casey mentioned that our debug_table 

                handmade_debug_interface.h 

                struct debug_table
                {
                    ...
                    ...
                };

                extern debug_table* GlobalDebugTable;

and is declared static, also it lives in the game code dll. which means whenever we reload the dll, its memory position 
will ge relocated.

so what we need to do is to move this to the platform layer. (in our case, we want to put it in win32)
so you would want the win32_platform layer to pass it to the game code

so Casey added in the win32_handmade.cpp

                win32_handmade.cpp 


                #if HANDMADE_INTERNAL
                global_variable debug_table GlobalDebugTable_;
                debug_table *GlobalDebugTable = &GlobalDebugTable_;
                #endif



49:40
so we put it in the game memory, and the win32_platform layer passes the DebugTable to the 
game code dll
                typedef struct game_memory
                {
                    uint64 PermanentStorageSize;
                    void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

                    ...
                    ...

                #if HANDMADE_INTERNAL
    ------------>   struct debug_table *DebugTable;
                #endif

                    platform_work_queue *HighPriorityQueue;
                    platform_work_queue *LowPriorityQueue;

                    b32 ExecutableReloaded;
                    platform_api PlatformAPI;
                } game_memory;


