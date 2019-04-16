Handmade Hero Day 262 - Drawing Multi-frame Profile Graphs

Summary:
more experimental code with how to organize and store debug_element and debug_frame

[I feel like all these debug experimental code will only make sense to me once i make one.
    none of these code is registering in my mind]


Keyword:
debug system, profiler 

5:58
now if you look at the way our debug system is architectued, inside the debug element itself 
we have the concept of frames 


                struct debug_element
                {
                    char *OriginalGUID; // NOTE(casey): Can never be printed!  Might point into unloaded DLL.
                    char *GUID;
                    u32 FileNameCount;
                    u32 LineNumber;
                    u32 NameStartsAt;
                    
                    b32 ValueWasEdited;
                    
    ----------->    debug_element_frame Frames[DEBUG_FRAME_COUNT];
                    
                    debug_element *NextInHash;
                };


which means our debug_frame struct is now kind of redundant. If you look at the struct, we really only need 
"u64 BeginClock", "u64 EndClock" and "r32 WallSecondsElapsed"            

                struct debug_frame
                {
                    // IMPORTANT(casey): This actually gets freed as a set in FreeFrame!
                    u64 BeginClock;
                    u64 EndClock;
                    r32 WallSecondsElapsed;

                    r32 FrameBarScale;

                    u32 FrameIndex;
                    
                    u32 StoredEventCount;
                    u32 ProfileBlockCount;
                    u32 DataBlockCount;
                    
                    debug_stored_event *RootProfileNode;
                };


27:05
so the current structure is below:

                debug_element --> RootProfile Element 

                     _____________________________
                    |     |     |     |     |     |
                    |_____|_____|_____|_____|_____|
                       |
                       |
                       v
                      ___      ___      ___      ___ 
                     |   |--->|   |--->|   |--->|   |
                     |___|    |___|    |___|    |___|


so the debug_element stores all the frames

each frames contains all the debug_events 


[I feel like all these debug experimental code will only make sense to me once i make one.
    none of these code is registering in my mind]


Q/A
1:01:27
people have mentioned concerns about the slowness of the debug view.

Casey says he thinks that the slowness is not rendering related,
its more about how we are setting up draw calls. 

Currently we dont have any ways of pushing down a huge set of rectangles down to the renderer 
in a nice coherent chunk.

So what is happening for every triangle is that it is creating an entry in the render_group, 
and it is being sent down to OpenGL. Again, this is done for every triangle. So the way we are doing 
right now is very inefficient. 

way to improve: make a special path to draw lots of triangles.
we can setup a special vertex buffer, and we write all of our rectangles directly into there, and issue it as one big batch.



1:09:20
generally speaking, if you have a lot of physic relevant code in your game, with lots of fancy physics code going on in it,
typically what you want to do is to lock it down to a predictable time step, cuz physics could be very time step sensitive,
and you typically dont want it to run at greater than or less then the standard testing frame rate. 

so typically what I have seen games do if they are very physics relevant is that they always run the physics at their given 
timestep (lets say 120 frames per second); So they will always do that man updates of the physics.

then when they render, they will just render whatever the most recent one is, or interpolation between the two most recent ones. 

A lot of times, games dont want to have variable physics frame rates, the reason is because physics is very sensitive to 
the framerate, and you dont want the game to change that much 

for handmade hero, where we dont have lots of lots of complex physical interactions between 3D rigid bodies and things like that, 
you can typically make the game feel rougly the same within a pretty wide tolerance, from 15 frames second to 120 frames per second.  

the behaviour of the game in general, will be roughly consistent, but just using basic techiniques. but that is not true for of complex 
physics usually. they tend to diverge quite a bit. 
