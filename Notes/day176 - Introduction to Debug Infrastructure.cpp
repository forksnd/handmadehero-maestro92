Handmade Hero Day 176 - Introduction to Debug Infrastructure

Summary:
discusses his plans and requirements for the debug service for handmade hero.

listed requirements such as 

-   Tuning / Fiddling
-   performance counter log
-   frame recording / Jump
-   memory consumption
-   diagraming

Emphasizes the importances of having all of these to be a part of a centralized log system 

Casey replaced the previous BEGIN_TIMED_BLOCK(); and END_TIMED_BLOCK(); pattern with the timed_block struct class,
which takes advantage of C++ concept of constructor and destructor 

Keyword:
Debug Services, Debug Infrastructure



 

5:42
Debug services are core part of any professional quality game development 

These usually take the form debug console that can have console commands, debug overlays, graphs

The goal is to make bugs more visible



12:28
typically a difficult bug is where the manifestation and the cause of the bug are separated, where
the two are not immeidatley corrolated


13:21
typically one of the most useful/common debugging tool is a log

this log should be "a way of recording information, such that later on we can query it, or display it in a useful fashion"

assume we have a frame spike 

Frames 
        16 ms           16 ms           35 ms           16 ms
     _______________________________________________________________
    |               |               |               |               |
    |               |               |               |               |
    |               |               |               |               |
    |_______________|_______________|_______________|_______________|

usually by the time we realize that this frame took too long, its already to late to prevent it.
so the approach is usually record some useful information that happened at that frame in the form of a log



18:44
so recall we have our looped live code editing,
what we can do is that we set a start break point at the beginning of that 35 ms frame, then an end break point at the end of that frame
then we replay that frame.

the only thing annoying is that, assume this bug happens 10 minutes into the gameplay, so we wouldnt want to replay the game 
10 minutes all the time

so one thing we can consider doing is to set "checkpoints"

so one thing we can do is every 30 seconds, we can add a "checkpoints" which we take a snapshot of the memory.
That way we can always jump back to the earlier 30 second check point mark


21:25
features that we plan to have
-   Tuning / Fiddling
-   performance counter log
-   frame recording / Jump
-   memory consumption
-   diagraming



26:06
we dont really want to have a lot of separate systems. we dont want the memory debug thing as a separate system,
or the performance counter as a separate system. We want everything to go through the log. The reason why is becuz 
for game programming bug, alot of the subtle bugs, they happen across alot of time and space. The easy bugs are the localized bugs.




27:15
Lets take an example. we want the diagram to be apart of the log system
assume we have the following code. 

                V3 a, b and c 
                a = b + c;

we suspect there are some problems in this piece of code. 
What I dont want to do is to change the structure of this code. 
I dont want to have to modify our code to add the diagram.

we dont want to pass in some debug services 

                for( --------------- )
                {
                    V3 a, b and c 
                    a = b + c 
                }


what I want to be able to straight ahead mark up this function 
    

what I want is just to add things in to get my diagrams 
                
                V3 a, b and c 
                a = b + c;

                Draw(a);
                Draw(b);
                Draw(c);

furthermore, I want to be able to leave that in without a lot of cost, easy to turn it on or off 
so the philosphy is always to "ADD" debug code, easy to turn on and off.






33:01
Casey also mentioned Tuning / Fiddling.

this is more about surfacing bugs and being able to inspect them.

Right now we have looped live code editing, our need for tuning and fiddling is substantially less.
so we may just not need this.

if you are a game that doesnt have looped live code editing (which is most games);, what tuning / fiddling means is about getting 
stuff on the screen that lets you tweat values. 

for example, if we want to test different movement speed, you will have a slider in game to adjust the movement speed values.

however, our looped live code editing is much better than the UI version that Casey mentioned



38:27
Casey emphasized repeated that his previous implemenation of DebugCycleCounter is incredibly lousy 

                handmade_platform.h

                // TODO(casey): Give these things names soon!
                enum
                {
                    /* 0 */ DebugCycleCounter_GameUpdateAndRender,
                    /* 1 */ DebugCycleCounter_RenderGroupToOutput,
                    /* 2 */ DebugCycleCounter_DrawRectangleSlowly,
                    /* 3 */ DebugCycleCounter_ProcessPixel,
                    /* 4 */ DebugCycleCounter_DrawRectangleQuickly,
                    DebugCycleCounter_Count,
                };
                typedef struct debug_cycle_counter
                {    
                    uint64 CycleCount;
                    uint32 HitCount;
                } debug_cycle_counter;


Casey wants to show the contrast when he starts to implement the real debug system.


38:42
consider the example in our renderer, which we have a bunch of cycle counter blocks 

                handmade_render_group.cpp

                internal void RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget,
                                    rectangle2i ClipRect, bool Even)
                {
                    BEGIN_TIMED_BLOCK(RenderGroupToOutput);

                    ...
                    ...

                    END_TIMED_BLOCK(RenderGroupToOutput);
                }

Casey pointed out that although this scheme is pretty simple, there is still a cost.
cuz the programmer has to add a new enum if they want to measure something else.
What Casey prefer is just something more agnostic

                handmade_render_group.cpp

                internal void RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget,
                                    rectangle2i ClipRect, bool Even)
                {
                    TIMED_BLOCK(RenderGroupToOutput);

                    ...
                    ...

                    TIMED_BLOCK(RenderGroupToOutput);
                }

something exteremely simple, one line of code, no need to add new enums, and thats it.



41:45
Casey mentioned using the concept of constructor and destructor to help with his implementation of begin and end timed blocks 

43:17
Casey added a handmade_debug.h file 



previously our patter is 
                
                {
                    BEGIN_TIMED_BLOCK_(ID);

                    // code you want to measure

                    END_TIMED_BLOCK(ID);
                }

we want to replace that with something that just has 

                {
                    TIMED_BLOCK(ID);

                    // code you want to measure

                }



So to achieve this, we need something that gets called at the beginning of the block, 
and at the end of the block 


so Casey takes advantage of the C++ constructor and destructor pattern.

as a side note, Casey says that C++ tries to formalize the concept of initialization and shutdown of pieces of data. 
which is kind of a ridiculous idea, becuz data doesnt have that concept inherently.


coming back to the code. Casey implemented the following 

                handmade_platform.h

                #define TIMED_BLOCK(ID) timed_block TimedBlock##ID(DebugCycleCounter_##ID);

                struct timed_block
                {
                    uint64 StartCycleCount;
                    u32 ID;
                    
                    timed_block(u32 IDInit)
                    {
                        ID = IDInit;
                        BEGIN_TIMED_BLOCK_(StartCycleCount);
                    }
                    
                    ~timed_block()
                    {
                        END_TIMED_BLOCK_(StartCycleCount, ID);
                    }
                };


1:03:24
someone asked in the Q/A, is there a way to know the overhead when enabling the debug?

Casey says, the only way to know the overhead is compiling it out. 
Currently we can use the HANDMADE_SLOW or HANDMADE_INTERNAL compile flag,
so have some #define controls 



1:06:38
someone asked, would you consider functional programming for game programming. 

Casey says, we actually do it all the time. The important thing is you dont want completely functional programming. 
But often times, you want small parts of your program to be written functionally. Things that dont need to access alot of 
state, you want them to be written functionally, becuz it eliminates the possbility of error based on state you cant easily 
observe. 

writing a whole game functinally is not a good idea. Functional programming style is a tool that you use to accomplish something. 
when that task lines up well with the style, its very good. When it doesnt line up well, its poor.
