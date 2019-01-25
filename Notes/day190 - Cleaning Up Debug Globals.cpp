Handmade Hero Day 190 - Cleaning Up Debug Globals

Summary:
cleaning up the code 

Keyword:
debug system, code clean up


20:05
cleaned up the DEBUG Rendering and Collating code.

added this DEBUGGetState(); function, 

                handmade_debug.cpp

                inline debug_state * DEBUGGetState(game_memory *Memory)
                {
                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    Assert(DebugState->Initialized);

                    return(DebugState);
                }

                inline debug_state * DEBUGGetState(void)
                {
                    debug_state *Result = DEBUGGetState(DebugGlobalMemory);

                    return(Result);
                }


26:52
one of the things about debug assets is that, we dont necessarily want to use the assets in the debug system. 
so if the Asset system is broken, we wont see our fonts at all. so we want to decouple these two.

so often times the way you do this is that you wont have the debug system using assets at all. what you do is that 
you pack up some font into the executable itself, then you can just load out of a static array.



40:45
made a rectangle that where we will render our rectangle stuff 

                handmade_debug.h

                struct debug_state
                {
                    ...
                    ...

                    rectangle2 ProfileRect;

                    ...
                    ...
                };



nothing interesting, just a lot of code clean up.
