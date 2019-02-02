Handmade Hero Day 201 - Isolating the Debug Code

Summary:
separated the game code and debug system
made it so that the debug system will render DEBUGOverlay(); it self so that game code and debug system are decoupled

fixed the problem where after copying a debug_variable hierarchy, the two will still share the same expand/contract state.

this is solved by having separate concepts of debug_variable data and debug_variable view.

refactered the debug_variable struct and added the debug_view struct


Keyword:
debug system


2:36
encountered a bug.
Casey found out that this is becuz the executable is getting reloaded in between the times we collate and display.
so our debug_record memory can sometimes be invalid if that happens


so currently our code looks like this 


                win32_handmade.cpp
                           
                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {      


                    while(GlobalRunning)
                    {
                     


                        if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                        {
                            Win32CompleteAllWork(&HighPriorityQueue);
                            Win32CompleteAllWork(&LowPriorityQueue);

                            GlobalDebugTable = &GlobalDebugTable_;
                            Win32UnloadGameCode(&Game);
                            Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                     TempGameCodeDLLFullPath,
                                                     GameCodeLockFullPath);
                            GameMemory.ExecutableReloaded = true;
                        }


                        ...
                        ...

                        if(Game.UpdateAndRender)
                        {
                            Game.UpdateAndRender(&GameMemory, NewInput, &Buffer);
                        }

                        ...
                        ...

                        BEGIN_BLOCK(DebugCollation);

                        if(Game.DEBUGFrameEnd)
                        {
                            GlobalDebugTable = Game.DEBUGFrameEnd(&GameMemory);
                        }
                        GlobalDebugTable_.EventArrayIndex_EventIndex = 0;

                        END_BLOCK(DebugCollation);

                    }
                }


so currently our flow is:
1.  on frame N, we update our debug records data in Game.DEBUGFrameEnd();
2.  on frame N + 1, we change the executable if that happens, 
3.  on the same frame (frame N + 1);, then we would call Game.UpdateAndRender(); to render our debug overlays.

so if the executable got swapped, the logic inside Game.UpdateAndRender(); where it read our debug_record data to render 
our debug stuff will be screwed



5:23
So what Casey wants to do is to move the DEBUG rendering logic inside the debug system. (inside DEBUGFrameEnd()); this way 
the debug system will do its own rendering

Casey didnt like how we were doing debug related stuff in 2 different places:
-   debug rendering happens inside GameUpdateAndRender();
-   debug data collation happens in DEBUGFrameEnd();



6:04
so initially we had the DEBUGFrameEnd(); code below the Win32DisplayBufferInWindow(); function.
Now Casey wants to move it ontop of it. This way when DEBUGFrameEnd(); does its own rendering,
the windows code can blit the buffer to the screen as well.


                win32_handmade.cpp
                           
                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {      


                    while(GlobalRunning)
                    {
                        ...
                        ...

                        BEGIN_BLOCK(FrameDisplay);
                        
                        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                        HDC DeviceContext = GetDC(Window);
                        Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
                        ReleaseDC(Window, DeviceContext);

                        ...
                        ...

                        END_BLOCK(FrameDisplay);



                        BEGIN_BLOCK(DebugCollation);

                        if(Game.DEBUGFrameEnd)
                        {
                            GlobalDebugTable = Game.DEBUGFrameEnd(&GameMemory);
                        }
                        GlobalDebugTable_.EventArrayIndex_EventIndex = 0;

                        END_BLOCK(DebugCollation);
                    }
                }

so now we have:

                win32_handmade.cpp
                           
                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {      
                    while(GlobalRunning)
                    {
                        ...
                        ...

                        BEGIN_BLOCK(DebugCollation);

                        if(Game.DEBUGFrameEnd)
                        {
                            GlobalDebugTable = Game.DEBUGFrameEnd(&GameMemory);
                        }
                        GlobalDebugTable_.EventArrayIndex_EventIndex = 0;

                        END_BLOCK(DebugCollation);



                        BEGIN_BLOCK(FrameDisplay);
                        
                        ...
                        Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
                        ReleaseDC(Window, DeviceContext);

                        ...

                        END_BLOCK(FrameDisplay);

                    }
                }



6:49
so the goal here is that we want the DEBUGOverlay to happen in our debug system.

Casey made it so that the DEBUGFrameEnd(); takes the GameMemory, and ScreenBuffer as arguments as well
this way the debug system will render to the screen himself, instead of having the GameUpdateAndRender(); function
to render our debug data.

This organization of code, will have the debug system is completely isolated from the GameUpdateAndRender();


                win32_handmade.cpp
                           
                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {      
                    while(GlobalRunning)
                    {
                        ...
                        ...


                        BEGIN_BLOCK(DebugCollation);

                        if(Game.DEBUGFrameEnd)
                        {
                            GlobalDebugTable = Game.DEBUGFrameEnd(&GameMemory, NewInput, &Buffer);
                        }
                        GlobalDebugTable_.EventArrayIndex_EventIndex = 0;

                        END_BLOCK(DebugCollation);  

                        ...
                    }
                }


There is one problem with it, which Casey will get to in a second


7:45
so as a first thing, Casey will get rid of the DEBUGStart(); and DEBUGEnd(); call in the GameUpdateAndRender(); 
and moved them to the DEBUGGameFrameEnd(); function 

this way GameUpdateAndRender(); wont have any debug related code
                
                handmade_debug.cpp

                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    GlobalDebugTable->RecordCount[0] = DebugRecords_Main_Count;
                    GlobalDebugTable->RecordCount[1] = DebugRecords_Optimized_Count;
                    
                    ++GlobalDebugTable->CurrentEventArrayIndex;
                    if(GlobalDebugTable->CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable->Events))
                    {
                        GlobalDebugTable->CurrentEventArrayIndex = 0;
                    }
                    
                    u64 ArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugTable->EventArrayIndex_EventIndex,
                                                                  (u64)GlobalDebugTable->CurrentEventArrayIndex << 32);

                    u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
                    u32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;
                    GlobalDebugTable->EventCount[EventArrayIndex] = EventCount;

                    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
                    if(DebugState)
                    {
                        game_assets *Assets = DEBUGGetGameAssets(Memory);

    --------------->    DEBUGStart(DebugState, Assets, Buffer->Width, Buffer->Height);

                        if(Memory->ExecutableReloaded)
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
                                
                        loaded_bitmap DrawBuffer = {};
                        DrawBuffer.Width = Buffer->Width;
                        DrawBuffer.Height = Buffer->Height;
                        DrawBuffer.Pitch = Buffer->Pitch;
                        DrawBuffer.Memory = Buffer->Memory;
    --------------->    DEBUGEnd(DebugState, Input, &DrawBuffer);
                    }

                    return(GlobalDebugTable);
                }



11:16
Since Casey wants the debug system to take care of rendering it self, 
Casey made it so that the debug system can access the assets. Now the debug system calls into the game instead of the other way around.
whereas previously, we have the game calling into the debug system 




12:05
so now Casey wrote the DEBUGGetGameAssets(); function 

                handmade.cpp

                internal game_assets * DEBUGGetGameAssets(game_memory *Memory)
                {
                    game_assets *Assets = 0;
                    
                    transient_state *TranState = (transient_state *)Memory->TransientStorage;
                    if(TranState->IsInitialized)
                    {
                        Assets = TranState->Assets;
                    }

                    return(Assets);
                }



17:20
now we removed the bug where the executable can get swapped between debug data collation and debug data rendering


20:04
now that our debug system is completely decoupled
Casey made it so that the debug code is compiled out if the HANDMADE_INTERNAL flag is false 

                handmade.cpp 

                #if HANDMADE_INTERNAL
                #include "handmade_debug.cpp"
                #else
                extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
                {
                    return(0);
                }
                #endif



27:40
Casey now wants to bring back the radial menu. 
so the idea Casey has in mind is that when the user right clicks, the radial menu shows up, and you can do a series 
of actions, such as pausing the profile view. 


29:05
so Casey wants to address the problem where when you duplicate a debug_variable, they will share the same state 


recall in day 198, we mentioned this problem 

     _______________________________________________________________________________
    |                                                                               |
    |   Particles: (expanded)                                                       |
    |       ParticleTest:   True                        Particles: (contract)       |
    |       ParticleGrid:   true                                                    |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |                                                                               |
    |_______________________________________________________________________________|

where lets say the debug_variable ui element on the right is copied from the left, these two UI elements will 
now share the same expanded, contract state. 

so this is happening becuz both the left and right debug_variable_reference 
points to the same debug_variable, which contains debug_variable_group that has the expanded flag


                                                         
    (Left) Particles.                        _______________________________                    (right) Particles
    debug_variable_reference   ---------->  |                               |   <-------------   debug_variable_reference
                                            |   debug_variable              |
                                            |   {                           |
                                            |       debug_variable_group    |
                                            |       expanded = true         |
                                            |   }                           |
                                            |_______________________________|

    

so essentially, we have the debug_variable, which is the data,

the debug_variable data should know about its hierarchy, but it should not know about whether its UI is expanded or not.

the expanded flag should be a part of the its view

so essentially Casey wants to now refactor the code so that there is a separation between debug_variable data and debug_variable view

33:26
so Casey refactored the debug_variable data. 

so we have the debug_tree struct which replaces the original debug_variable_hiearachy class 


                struct debug_tree
                {
                    v2 UIP;
                    debug_variable *Group;

                    debug_tree *Next;
                    debug_tree *Prev;
                };

then in side debug_variable, we only kept the data related part.
for example, in debug_bitmap_display, we only kept bitmap_id. the Dim and alpha variables is really only a concept of rendering.
so we removed it out of the debug_bitmap_display struct. 

                struct debug_variable
                {
                    debug_variable_type Type;
                    char *Name;

                    union
                    {
                        b32 Bool32;
                        s32 Int32;
                        u32 UInt32;
                        r32 Real32;
                        v2 Vector2;
                        v3 Vector3;
                        v4 Vector4;
                        debug_profile_settings Profile;
                        debug_bitmap_display BitmapDisplay;
                        debug_variable_array VarArray;
                    };
                };

                struct debug_profile_settings
                {
                    int Placeholder;
                };

                struct debug_bitmap_display
                {
                    bitmap_id ID;
                };

                struct debug_variable_array
                {
                    u32 Count;
                    debug_variable *Vars;
                };


so essentially our structure now looks this 

                debug_state
                {

                    debug_tree TreeSentinel;

                       
                         _______________________               _______________________               _______________________
                        |   debug_tree          |             |   debug_tree          |             |   debug_tree          |                                           
                        |   {                   |             |   {                   |             |   {                   |
                        |       debug_variable  |  -------->  |       debug_variable  |  -------->  |       debug_variable  |  
                        |       hierarchy       |  <--------  |       hierarchy       |  <--------  |       hierarchy       |
                        |   }                   |             |   }                   |             |   }                   |      
                        |_______________________|             |_______________________|             |_______________________|

                }


[i initially had the question what is the point of debug_tree?

so the conclusion that I eventually have is that debug_tree are just linked list nodes here.
in debug_state, we choose to store all of our debug_variable as linked list.
so what we did is we created a debug_tree, which is just linked list nodes, and we stored the debug_variable inside there.


then I asked myself, are there ways to remove debug_tree? since 
it feels rather redundant, and debug_variable already has the concept of a hierarchy?

                struct debug_variable
                {
                    ...
                    ...
                    debug_variable_array VarArray;

                };


one thing about is that we are doing random tear offs and copies, so we want to be able to add and remove hierarchy easily.
and since debug_variable stores a fixed size array of debug_variable_array here, having a linked list is easier to manage.


another thing is that if we want to remove debug_tree struct, but still have debug_state storing us as a linked list,
that means the debug_variable struct needs to be something like:

                struct debug_variable
                {
                    ...
                    ...
                    debug_variable_array VarArray;

                    debug_variable *Next;
                    debug_variable *Prev;
                };


This might be wasteful. Currently we really only need to use Next and Prev pointer when we iterate the linked list at the top level. 
for example, when we render, we will do something like :


                debug_variable* cur = null 
                while(true)
                {
                    ...............................
                    ....... Render cur ............
                    ...............................

                    cur = cur->next;
                }


then when we tear off one debug_variable_hiearachy, then copy it, we will add it to our linked list 

                debug_state.TreeSentinel.tail->next = new debug_variable.


so really, we only need the linkedList operations at the top level. If we add it inside debug_variable, it becomes wasteful.
which is why we created the concept of debug_tree. You can think of debug_tree as debug_variable on the top level.

so if you do make it to be 

                struct debug_variable
                {
                    ...
                    ...
                    debug_variable_array VarArray;

                    debug_variable *Next;
                    debug_variable *Prev;
                };

that means you can add debug_variable to any level that you want.
but here, we really only need to add debug_variable to the top level, so we made a separate debug_tree class.

also I think Casey wants to make the concept that: I have a list of trees more explicit.]





45:02 
Casey createing the debug_view struct 

[i would call it debug_variable_view, this is the same pattern we do in game2]

                struct debug_view_inline_block
                {
                    v2 Dim;
                };

                struct debug_view_collapsible
                {
                    b32 ExpandedAlways;
                    b32 ExpandedAltView;
                };

                enum debug_view_type
                {
                    DebugViewType_Basic,
                    DebugViewType_InlineBlock,
                    DebugViewType_Collapsible,
                };

                struct debug_view
                {
                    debug_tree *Tree;
                    debug_variable *Var;
                    debug_view *NextInHash;
                    
                    debug_view_type Type;
                    union
                    {
                        debug_view_inline_block InlineBlock;
                        debug_view_collapsible Collapsible;
                    };
                };




then in the debug_state, we store a hashtable of debug_view*
which is essentially an array.

                struct debug_state
                {
                    ...
                    ...

                    debug_variable *RootGroup;
                    debug_view* ViewHash[4096];
                    ...
                };



Casey will finish this up next episode

