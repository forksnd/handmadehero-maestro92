Handmade Hero Day 258 - Fixing Profiling Across Code Reloads

Summary:
reorgnaized the game code dll reload code and the DEBUGFrameEnd code so that the profiler works during 
debug code reloading.

Keyword:
debug system, profiler 


1:42
Casey summarizing the bug we had from day 257.
what happens is that if we change the game code dll somewhat significatnly, not like twiddle small piece of thing. 
Rather we have to if out a block of code, so that the code gets rearranged a little bit.

we can get to a point where the profiler no longer works. 


4:23
as you already know, our game is multithreaded. what that means is when we are collating the debug log,
we have logs that are filled with debug events from mulitple frames.
as we are collating the debug logs, we also have to know which thread the debug events came from, and we have to make 
sure the debug begin and end blocks pair up properly.


6:19
so currently the problem is that during the reload, we never get an ending block to match the open block 
around the reload 

so in our code, we have this DebugCollation block which opens. Then we will collate all the debug data,
then we call the END_BLOCK


                while(GlobalRunning)
                {

                    ...
                    ...

                    BEGIN_BLOCK("Debug Collation");

                    if(Game.DEBUGFrameEnd)
                    {
                        Game.DEBUGFrameEnd(&GameMemory, NewInput, &RenderCommands);
                    }
                    else
                    {
                        // NOTE(casey): If for some reason the game didn't load,
                        // make sure we clear the debug event array so it doesn't
                        // pile up on itself.
                        GlobalDebugTable_.EventArrayIndex_EventIndex = 0;
                    }

                    END_BLOCK();

                    ...
                    ...
                }


then on the next frame, if we potentially do a reload. and something during the process,
our begin and end blocks got messed up

                while(GlobalRunning)
                {

                    //
                    //
                    //

                    BEGIN_BLOCK("Executable Refresh");
                    NewInput->dtForFrame = TargetSecondsPerFrame;

                    GameMemory.ExecutableReloaded = false;                    
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32CompleteAllWork(&HighPriorityQueue);
                        Win32CompleteAllWork(&LowPriorityQueue);
                        
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                 TempGameCodeDLLFullPath,
                                                 GameCodeLockFullPath);
                        GameMemory.ExecutableReloaded = true;
                    }
                    END_BLOCK();

                    ...
                    ...




                    BEGIN_BLOCK("Debug Collation");

                    if(Game.DEBUGFrameEnd)
                    {
                        Game.DEBUGFrameEnd(&GameMemory, NewInput, &RenderCommands);
                    }
                    else
                    {
                        // NOTE(casey): If for some reason the game didn't load,
                        // make sure we clear the debug event array so it doesn't
                        // pile up on itself.
                        GlobalDebugTable_.EventArrayIndex_EventIndex = 0;
                    }

                    END_BLOCK();

                    ...
                    ...
                }

10:48
Casey mentioned the executabe reload has need to be on an empty debug buffer. So the executabe code and 
debug code has to work togeteher

11:14
so what Casey plans to do is to remember whether we have to do a reload, then do the reload later. 




12:24 
So Casey moved the reloading down with the DEBUGFrameEnd(); function

14:59
the idea is that we collate all the debug events that we want to collate, then we reload the executabe

                while(GlobalRunning)
                {

                    ...
                    ...

                    BEGIN_BLOCK("Debug Collation");

                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    b32 ExecutableNeedsToBeReloaded = 
                        (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0);

                    GameMemory.ExecutableReloaded = false;
                    if(ExecutableNeedsToBeReloaded)
                    {
                        Win32CompleteAllWork(&HighPriorityQueue);
                        Win32CompleteAllWork(&LowPriorityQueue);
                     }
                    
                    if(Game.DEBUGFrameEnd)
                    {
                        Game.DEBUGFrameEnd(&GameMemory, NewInput, &RenderCommands);
                    }
                    
                    if(ExecutableNeedsToBeReloaded)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                            TempGameCodeDLLFullPath,
                            GameCodeLockFullPath);
                        GameMemory.ExecutableReloaded = true;
                    }
                   
                    END_BLOCK();
                }


16:20
apparently in the above code, our game didnt reload properly. in the line 

                    Win32UnloadGameCode(&Game);
                    Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                        TempGameCodeDLLFullPath,
                        GameCodeLockFullPath);
                    GameMemory.ExecutableReloaded = true;


we cant load our new gamecode DLL through the line "Win32LoadGameCode" immediately because our compiler 
is not done compiling it yet. So Casey added the following for loop, to try to load it multiple times

                   
                    Win32UnloadGameCode(&Game);
                    for(u32 LoadTryIndex = 0;
                        !Game.IsValid && (LoadTryIndex < 100);
                        ++LoadTryIndex)
                    {
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                            TempGameCodeDLLFullPath,
                            GameCodeLockFullPath);
                        Sleep(100);
                    }
                    
                    GameMemory.ExecutableReloaded = true;
                    DEBUGSetEventRecording(Game.IsValid);




20:12 
Casey reorganized it further, so now we have 



                while(GlobalRunning)
                {

                    ...
                    ...

                    BEGIN_BLOCK("Debug Collation");

                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    b32 ExecutableNeedsToBeReloaded = 
                        (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0);

                    GameMemory.ExecutableReloaded = false;
                    if(ExecutableNeedsToBeReloaded)
                    {
                        Win32CompleteAllWork(&HighPriorityQueue);
                        Win32CompleteAllWork(&LowPriorityQueue);
                     }
                    
                    if(Game.DEBUGFrameEnd)
                    {
                        Game.DEBUGFrameEnd(&GameMemory, NewInput, &RenderCommands);
                    }
                    else
                    {
                        GlobalDebugTable_.EventArrayIndex_EventIndex = 0;
                    }

                    if(ExecutableNeedsToBeReloaded)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                            TempGameCodeDLLFullPath,
                            GameCodeLockFullPath);
                        Sleep(100);

                        GameMemory.ExecutableReloaded = true;
                    }


                    END_BLOCK();
                }



20:57
Casey once again questions the purpose of 
                
                GlobalDebugTable_.EventArrayIndex_EventIndex = 0;

we got rid of that line 

below we have the final code 

                while(GlobalRunning)
                {

                    ...
                    ...

                    BEGIN_BLOCK("Debug Collation");

                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    b32 ExecutableNeedsToBeReloaded = 
                        (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0);

                    GameMemory.ExecutableReloaded = false;
                    if(ExecutableNeedsToBeReloaded)
                    {
                        Win32CompleteAllWork(&HighPriorityQueue);
                        Win32CompleteAllWork(&LowPriorityQueue);
                    }
                    
                    if(Game.DEBUGFrameEnd)
                    {
                        Game.DEBUGFrameEnd(&GameMemory, NewInput, &RenderCommands);
                    }
                    
                    if(ExecutableNeedsToBeReloaded)
                    {
                        Win32UnloadGameCode(&Game);
                        for(u32 LoadTryIndex = 0;
                            !Game.IsValid && (LoadTryIndex < 100);
                            ++LoadTryIndex)
                        {
                            Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                TempGameCodeDLLFullPath,
                                GameCodeLockFullPath);
                            Sleep(100);
                        }
                        
                        GameMemory.ExecutableReloaded = true;
                    }

                    
                    END_BLOCK();
                }



35:07
Notice that Casey also added the SetEventRecording Flag. This way, during the reloading
we dont actually write into our GlobalDebugTable. Our GlobalDebugTable remains empty during the reloading 


                while(GlobalRunning)
                {

                    ...
                    ...

                    BEGIN_BLOCK("Debug Collation");

                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    b32 ExecutableNeedsToBeReloaded = 
                        (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0);

                    GameMemory.ExecutableReloaded = false;
                    if(ExecutableNeedsToBeReloaded)
                    {
                        Win32CompleteAllWork(&HighPriorityQueue);
                        Win32CompleteAllWork(&LowPriorityQueue);
    ----------->        DEBUGSetEventRecording(false);
                    }
                    
                    if(Game.DEBUGFrameEnd)
                    {
                        Game.DEBUGFrameEnd(&GameMemory, NewInput, &RenderCommands);
                    }
                    
                    if(ExecutableNeedsToBeReloaded)
                    {
                        Win32UnloadGameCode(&Game);
                        for(u32 LoadTryIndex = 0;
                            !Game.IsValid && (LoadTryIndex < 100);
                            ++LoadTryIndex)
                        {
                            Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                TempGameCodeDLLFullPath,
                                GameCodeLockFullPath);
                            Sleep(100);
                        }
                        
                        GameMemory.ExecutableReloaded = true;
    ----------->        DEBUGSetEventRecording(Game.IsValid);
                    }

                    
                    END_BLOCK();
                }
