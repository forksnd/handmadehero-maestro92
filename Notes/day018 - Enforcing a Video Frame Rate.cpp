Handmade Hero Day 018 - Enforcing a Video Frame Rate

Summary:

mentioned how syncing your game frame rate with Monitor frame rate is crucial to the user.

talks about the flaws in variable refresh rate monitor. Which it forces your game state to always be wrong.

talked about the difference between QueryPerformanceCounter and __rdtsc.
emphasized that QueryPerformanceCounter is used for game logic 
__rdtsc can only be used for non-game logic tasks such as profiling

wrote the two functions used for timing Win32GetWallClock(); and Win32GetSecondsElapsed();

wrote code that enfources the game to flip at the desired frame rate.
first using the CPU melting method. Basically a while loop in a spin lock.
then the sleep method.
talked about the problems of sleep granularity due to OS scheduler.

attempted and mentioned the timeBeginPeriod as a possible solution

explains why in this current code, we are doing the wait/sleep logic before the render/flip in the Q/A

Keyword:
Frame rate, variable refresh rate monitor, QueryPerformanceCounter, __rdtsc



10:25
fixed duration


basically normal monitors from today have some frame rate that they update the image at. 
So whatever that monitor frame rate is, that is also the frame rate that we want our game to be updating at.

we want to synchronize with your monitor frame rate. If we do something other than your monitor frame rate,
we will be off beat with the monitor.

To the user, it will look like we skipped a frame 

For example 


frame rate 

    |___________|___________|___________|___________|___________|___________|
    1           2           3           4           5           6           7


monitor frame rate 

    |_______|_______|_______|_______|_______|_______|_______|_______|_______|_______|
    a       b       c       d       e       g 

so 
b will display game state 1, 
c will display game state 2, 
d displays game state 3
e displays game state 3 again 
g displays game state 4.

so to the user, it will wait for two monitor frame rate to get a new game state, which is why 
it will look like it skipped a frame. 



we want the monitor to update roughly the same frame as our game does.

Thats the goal


we can either do the exact rate of your monitor frame rate, 
or any subdivision of it. Let say your monitor is at 120 hz, u can do it at 60 fps

Hz is just cycles per second





13:23
Casey mentions that there is a lot of talk about variable refresh rate monitors. These are basically monitors
that can update at whatevery frequency we want. 

Casey absolutely hates it, becuz it forces you to always be wrong about how you display your animation. 

Imagine how the computations works in one of those variable frame rate monitors. 

  world                                  world 
  state 0                                state 1

    |...physics....|....rendering..........|                                             |
    |______________________________________|_____________________________________________|
    a     30 ms                            b             45 ms                           c  
    
for example lets say your variable frame a and frame b is 30 ms apart. 

that means the physics system has to update with fixedTick 30 ms.

But the problem is, when you run your game at world state0, it wont know about this 30 ms until world state 1

for instance, the physics wont know about how long the rendering system will take, so it wont know about the 30 ms.

perhaps it will have to look at the previous frame for a guess. Then if you look at the previous frame, it will always be wrong.
so pretty much, every frame is wrong 


[also if you are running the game both on the server and client, theres no way for the server 
    to know how your monitor variable frame rate will be]



17:55

for me whenever I do a game loop and whenever I am looking at timing stuff, I want to have a target framerate,
and we try very hard to hit that framerate

and for some reason if we dont hit that framerate, we set a new lower frame rate. 
We always try to stay fixed at whatever target framerate is.

I never want the framerate to be variable, becuz there is no way to make that work properly, 
your physics is always wrong if you do that.



19:45
the problem with the audio is that, we would like to feed the amount of audio needed for that frame. 
But if we do that, we run the risk of missing, causing frame lag.



  world                                  world 
  state 0                                state 1

    |.......audio..........................|                                      |
    |______________________________________|______________________________________|
    a                                      b                                      c  



We have some choices

1.  we always get the audio there on time. Meaning our frame rate is a hard constraint.
we basically wrote the game to never miss frame rate.

2.  always feed the audio someone further ahead. 
    there are a few way of doing it:
    -   overwrite next frame
    -   frame of lag 
    -   guard thread


Casey decided on option 1 cuz its easier and the easiest.
We target 30 hz, and we always get there. If we dont get 30 hz, we have to optimize then. 

25:01
Casey googling the MSDN and trying to figure out refresh rate of our monitor
it seems like documentations are pretty bad 

29:20
normally, this is a lot easier in OpenGL or Direct3D is becuz you set them into a mode where flip on the monitor
refresh anyway. So when you do that, all you have to do is to figure out what your refresh rate is to time how long it takes to flip
then you can roughly know what the graphics card is doing. 



32:14
After with no success, Casey proceeds to write the main structure.
He will worry about the windows API to figure out the monitor refresh rate later.

-   Casey first wrote some variables

    for the GameUpdateHz, Casey is worried that our game wont hit 60 fps when we do software rendering, 
    so we will just stay at 30 fps.

                win32_handmade.cpp

                int CALLBACK
                WinMain(HINSTANCE Instance,
                        HINSTANCE PrevInstance,
                        LPSTR CommandLine,
                        int ShowCode)
                {

                    int MonitorRefreshHz = 60;
                    int GameUpdateHz = MonitorRefreshHz / 2;
                    real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;
                    

                    ...
                    ...

                }



34:25
then Casey proceed to write the code to have our game flip at the desired framerate. 

-   you can see that have this main while(GlobalRunning) loop.
    for each frame, we do the main game logic GameUpdateAndRender and Win32FillSoundBuffer.
    then afterwards, we do the frame rate related logic.


-   so we first calculate WorkSecondsElapsed. That is the time used in the current frame. 
    this number clearly will be different from the TargetSecondsPerFrame that we defined on top

    for the CPU melting version, we just have a while loop comparing the SecondsElapsedForFrame and TargetSecondsPerFrame

-   after enough time has elapsed, we do our flip with the call 
{               
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                           Dimension.Width, Dimension.Height); 
}

-   notice how we update the variable "LastCounter"
    we first declared it before the while loop.
    
    then at the end of every frame, (after we flip), we update the LastCounter.



full code below 

                win32_handmade.cpp

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {

                    int MonitorRefreshHz = 60;
                    int GameUpdateHz = MonitorRefreshHz / 2;
                    real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;
                    
                    ...

    
                        LARGE_INTEGER LastCounter = Win32GetWallClock();

                        while(GlobalRunning)
                        {

                            ....................................................................................
                            ... GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);..............
                            ... Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);.....
                            ....................................................................................



                            LARGE_INTEGER WorkCounter = Win32GetWallClock();
                            real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                            // TODO(casey): NOT TESTED YET!  PROBABLY BUGGY!!!!!
                            real32 SecondsElapsedForFrame = WorkSecondsElapsed;

                            while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {                            
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            }

                            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                            Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                                       Dimension.Width, Dimension.Height);


                            ...
                            ...

                            LARGE_INTEGER EndCounter = Win32GetWallClock();
                            real32 MSPerFrame = 1000.0f*Win32GetSecondsElapsed(LastCounter, EndCounter);                    
                            LastCounter = EndCounter;

                            ...
                            ...

                        }
                }


39:56
Casey got the game to run, but Casey mentioned that the audio is skipping like crazy.
so that sounds you hear at this time of the video is the result of audio skipping.


40:54
Casey wrote the functions that he uese for timing.
-   in the Win32GetWallClock(); function, we used QueryPerformanceCounter();
Recall we first used QueryPerformanceCounter(); day 10. For details refer to episode 10

                inline LARGE_INTEGER
                Win32GetWallClock(void)
                {    
                    LARGE_INTEGER Result;
                    QueryPerformanceCounter(&Result);
                    return(Result);
                }

-   to understand what happens in Win32GetSecondsElapsed();, refer day day 10. it explains how QueryPerformanceFrequency 
and QueryPerformanceCounter work together to compute time.

                inline real32
                Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
                {
                    real32 Result = ((real32)(End.QuadPart - Start.QuadPart) /
                                     (real32)GlobalPerfCountFrequency);
                    return(Result);
                }




43:54
Casey mentioned that __rdtsc(); is processor specific. So You cant use __rdtsc(); for timing on a user machine. 
you can only use query performance counter for timings on a user machine. 
So our game code has to be driven off of query performance counter.  

__rdtsc is only used for profiling cuz how it behaves varies from CPU to CPU. 
so you dont want to ship a game using __rdtsc for game time.


[I will mention it again, to drive forward game code, use QueryPerformanceCounter,
for profiling, you __rdtsc

here to determine when do we want to flip, or start the next frame, that is game code, so we definitely
want to use QueryPerformanceCounter.

we use __rdtsc to print out the fps as a string. That is not game code, so we use __rdtsc there]


[from this link it says 
        "Use QueryPerformanceCounter and QueryPerformanceFrequency instead of RDTSC. 
        These APIs may make use of RDTSC, but might instead make use of a timing devices on the 
        motherboard or some other system services that provide high-quality high-resolution timing information.
         While RDTSC is much faster than QueryPerformanceCounter, since the latter is an API call, 
         it is an API that can be called several hundred times per frame without any noticeable impact. 
         (Nevertheless, developers should attempt to have their games call QueryPerformanceCounter 
            as little as possible to avoid any performance penalty.)"

https://docs.microsoft.com/en-us/windows/desktop/dxtecharts/game-timing-and-multicore-processors
]




46:42
as we got the game running with this "enforce frame rate code" running, Casey asks the questions 
how do we really know it is running at the desired frame rate? (30 fps here);

Casey mentioned ways to verify it. 
one way we could do have a physical stopwatch and have a counter to see how many frames we actually
updated in lets say 1 minute of time.

Casey didnt actually implement anyting, just briefly mentioned it.




47:35
another thing we want to check is that we didnt actually miss a frame.


                win32_handmade.cpp

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {

                    int MonitorRefreshHz = 60;
                    int GameUpdateHz = MonitorRefreshHz / 2;
                    real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;
                    
                    ...

    
                        LARGE_INTEGER LastCounter = Win32GetWallClock();
                        uint64 LastCycleCount = __rdtsc();
   
                        while(GlobalRunning)
                        {
                            ...
                            ...

                            real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                            if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                            { 
                                while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                                {                            
                                    SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                                }
                            }
                            else
                            {
                                // TODO(casey): MISSED FRAME RATE!
                                // TODO(casey): Logging
                            }

                        }


graphically it looks like this

                                                                                        
                                                           if(SecondsElapsedForFrame < TargetSecondsPerFrame)

                                                                                     |
                                                                                     |
  world                                world                                         |   
  state 0                              state 1                                       v

    |..phys..|..rendering.|....sleep....|.........phys.........|.........rendering.....|
    |___________________________________|______________________________________________|
    a             30 ms                 b                45 ms                          c  
    

    |_________________|_________________|_________________|_________________|_________________|_________________|___________|
    1                 2                 3                 4                 5                 6



at the end of frame b , we do the logic to check for if(SecondsElapsedForFrame < TargetSecondsPerFrame);
if for some reason the game logic took too long, and SecondsElapsedForFrame is 45 ms, but TargetSecondsPerFrame is 30 ms

in that case we missed monitor frame 5

what we meant by that is 
frame 3 displays b
frame 4 displays b

frame 5, is when we want to flip,
so frame 5 ideally should display c
but frame b took forever, so we missed a frame.

if a situation like that comes up, we want to print out, hence the following check

                if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                {

                }
                else
                {
                    // TODO(casey): MISSED FRAME RATE!
                    // TODO(casey): Logging
                }




48:45
Casey mentioned that the while look inside the if condition will "melt the CPU"

                real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                { 
                    while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {                            
                        SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                    }
                }

what he means by is we dont want the CPU wasting CPU cycles in that while loop and checking the performacnce counter

if you open the taks manager and look at the percernage% of your CPU usage, this is be super high 
even if you game logic does absolutely nothing.

for example, even if you have the following loop, where we took out all the code of game logic,
literraly just an empty while loop, you CPU usage shown in the windows task manager for handmade.exe will stil be at 99%
cuz of that inner while loop wasting cycles. 

                while(GlobalRunning)
                {


                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    { 
                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {                            
                            SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        }
                    }
                    else
                    {

                    }
                }


so the solution for that is to sleep();

-   as you can see, we have SleepMS = TargetSecondsPerFrame - SecondsElapsedForFrame. 
That is essentially how long we can afford to sleep 

-   ofcourse when you do that multiplication

                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
    u get a fractional value, so we will just trancate it.



full code below:

                while(GlobalRunning)
                {
                    ...
                    ...

                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    { 
                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {                            
                            DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                               SecondsElapsedForFrame));
                            if(SleepMS > 0)
                            {
                                Sleep(SleepMS);
                            }

                            SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        }
                    }
                    else
                    {

                    }
                }


51:00
there is a problem with the sleep method
The sleep function in windows has a certain granularity with how long it can put your thread to sleep. 
Essentially, if it decides to put your thread to sleep, it is going to wait for the next scheduler granularity to wake you up. 

Inside every OS, theres some code called the scheduler. The scheudler is what decides when threads run. Its pretty much the thing 
that makes your OS preemptively multitask. 

The scheduler is something that has to interrupts whats going on. 

so if the scheduler only wakes up every 15 milli seconds to check if anyone wants to wake up, and you request to sleep for 2 milli 
seocnds, you are realistically gonna sleep for 15 milli seconds.

we can fix this by calling timeBeginPeriod, 

Casey procees to explain how to use it, which I dont understand.. so im not gonna bother

apparently this thing attempts to set the OS scheduler granularity.


so Casey did something like below:

                int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
                {
                    ...
                    ...

                    // NOTE(casey): Set the Windows scheduler granularity to 1ms
                    // so that our Sleep() can be more granular.
                    UINT DesiredSchedulerMS = 1;
                    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

                    ...
                    ...
                }


-   Casey then added the an if(SleepIsGranular) condition for Sleep(SleepMS) call.
    Casey says he doesnt want to sleep if the OS is not able to sleep at the granularity that we want to.

    so allow to sleep if granularity permits, otherwise we do the CPU melting version to approach the next frame.


                while(GlobalRunning)
                {
                    ...
                    ...

                    real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {      
                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {  
                            if(SleepIsGranular)
                            {
                                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                                if(SleepMS > 0)
                                {
                                    Sleep(SleepMS);
                                }
                            }
                                              
                            SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        }
                    }
                    else
                    {
                        // TODO(casey): MISSED FRAME RATE!
                        // TODO(casey): Logging
                    }
                }


57:47
as a simple test of whether we are truly hitting 30 fps,
Casey just printed out the fps 


                LARGE_INTEGER LastCounter = Win32GetWallClock();
                uint64 LastCycleCount = __rdtsc();

                while(GlobalRunning)
                {
                    ...
                    ...

                    real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    { 
                        ................
                        ... sleep(); ...
                        ................
                    }
                    else
                    {
                        // TODO(casey): MISSED FRAME RATE!
                        // TODO(casey): Logging
                    }

                    ...
                    ...

                    uint64 EndCycleCount = __rdtsc();
                    uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    LastCycleCount = EndCycleCount;
                    
                    real64 FPS = 0.0f;
                    real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));

                    char FPSBuffer[256];
                    _snprintf_s(FPSBuffer, sizeof(FPSBuffer),
                                "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
                    OutputDebugStringA(FPSBuffer);

                }

and you can see, we roughly hit 30 fps

it looks like we are hitting our target frame rate, but we have some reasons on why we wont 
basically the things we havent done yet is we are not including the time for 


                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                           Dimension.Width, Dimension.Height);



1:02:45
someone asked in Q/A, even if you could grab the refresh rate of the monitor, how do you synchronize it?
for our prototype, we probably wont bother synchronizing with it. DirectDraw(); actually does have a thing 
that allows you to wait for the vertical blink, and we could use that if we want that. Unfortunately,
DirectDraw(); is extremely deprecated. 

so the short answer is, we probably wont. We might be one frame off, or we might potentially tearing. 

when we go do the final platform layer, we will be outputting through OpenGL or directX, those have the ability
to automatically sync and flip with the monitor. So then we wont have to worry about it, it will automatically be taken
care of by the OS. We just give a frame, it will sync. 


1:03:51
someone asked in Q/A, does it make sense to put the sleep in the while loop

in the previous draft, we have 

                if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                { 
                    while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {                            
                        DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                           SecondsElapsedForFrame));
                        if(SleepMS > 0)
                        {
                            Sleep(SleepMS);
                        }

                        SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                    }
                }
                else
                {

                }


Casey reorganized the code to

                    real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {                        
                        if(SleepIsGranular)
                        {
                            DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                               SecondsElapsedForFrame));
                            if(SleepMS > 0)
                            {
                                Sleep(SleepMS);
                            }
                        }
                        
                        real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        Assert(TestSecondsElapsedForFrame < TargetSecondsPerFrame);
                        
                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {                            
                            SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
                                                                            Win32GetWallClock());
                        }
                    }

but he mentioned that either one is fine. 

Casey did mention that, it does make sense to take the Sleep(); outside of the while loop.
cuz in our original code, after the first sleep, presumably, there is nothing no time left, 
the second time in our while loop, the sleepMS will be 0.

It doesnt rally matter where you put it. Either one is fine.




1:07:12
someone asked about the pros and cons of update the game state more frequently then they render to the screen within one frame?

Casey says: no there is no benefit,

however there are reasons to subdivide some aspects of the update into more granular updats then the rendering.
The example of that is physics meeds to update at 1/20 th of a second, bit your rendering is at 30 frames 

the simplest way to do this is just to run the physics in a for i loop.
for example updating the physics 4 times for every one update of the game.



1:10:17
Casey once again talking about the issue with the following code 

                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                           Dimension.Width, Dimension.Height);

if we aim to sync with the monitor, we also have to consider the time of these two functions
so ideally, we want to to sleep for TargetSecondsPerFrame - SecondsElapsedForFrame minus the time for these two functions. 

but the thing is 

                LARGE_INTEGER EndCounter = Win32GetWallClock();
                real32 MSPerFrame = 1000.0f*Win32GetSecondsElapsed(LastCounter, EndCounter);                    
                LastCounter = EndCounter;

is done afterwards, so we are actually accounting for the time to do these two functions. So not sure 
we need to change anything. 

[i didnt quite understand this part.... to me this is still incorrect]


we added an assert 
                Assert(TestSecondsElapsedForFrame < TargetSecondsPerFrame);
                     
to make sure we didnt sleep pass the frametime.






1:16:55
Someone asks in the Q/A, wouldnt you want to refresh the framebuffer before this frame rate logic,
so the monitor could grab the when it refreshes?

Yes, but you have to remember, we dont know when the monitor is refreshing, so all we are doing here is 
we are just creating a loop that will keep our game running at 30 frames per second, so all of our timing decisions 
are correct. Later when we do a Win32 layer that outputs through one of the modern graphics API such as OpenGL, or direct3D,
then we will have a different strategy, which is to pushing the frame down, then waiting it for the flip to happen at the right time.

over here, we dont know when the flip is happening, so we cant actually do anything about that. Theres just no way
what we rather do is to wait the right amount of time before we flip, so our flip is always flipping at a fixed rate.

our structure is 
    
            while (running)
            {
                game logic 

                wait/sleep 

                render/flip
            }


recall our game logic is variable time. The render/flip will roughtly take a constant amount of time. 
so what we are doing is the wait/sleep part is to make up for the game logic varaible time amount, 
that way render/flip always happen at fixed rate.


