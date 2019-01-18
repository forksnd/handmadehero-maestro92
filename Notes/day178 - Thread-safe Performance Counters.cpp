Handmade Hero Day 178 - Thread-safe Performance Counters

Summary:
added a different font for print debug stuff. Used the tag matching system for fonts in different places in the game.

Combined timed_block.CycleCount and timed_block.HitCount into one u64, and used atomic operations on it.

made reads to the debug values atomic as well.

explained the difference between interlock add and mutex in the Q/A

Keyword:
debug system, multi-threading, mutex, 


4:29
working add a different font for dispalying debug information. Casey plans to use the asset_tag system for it 
so Casey added a font type amont the tags enums

                handmade_file_formats.h

                enum asset_font_type
                {
                    FontType_Default = 0,
                    FontType_Debug = 10,
                };

                enum asset_tag_id
                {
                    Tag_Smoothness,
                    Tag_Flatness,
                    Tag_FacingDirection, // NOTE(casey): Angle in radians off of due right
                    Tag_UnicodeCodepoint,
    ----------->    Tag_FontType, // NOTE(casey): 0 - Default Game Font, 10 - Debug Font?   
                    
                    Tag_Count,
                };




5:48
in the WriteFonts(); function we load two different fonts,
as you can see the Debug font has a much smaller PixelHeight

                internal void WriteFonts(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    loaded_font *Fonts[] =
                    {
                        LoadFont("c:/Windows/Fonts/arial.ttf", "Arial", 128),
                        LoadFont("c:/Windows/Fonts/LiberationMono-Regular.ttf", "Liberation Mono", 20),
                    };

                    ...
                    ...
                    
                    WriteHHA(Assets, "testfonts.hha");
                }



26:49
after a lot of debugging, we finally got it to render

28:29
Casey starting to spam TIMED_BLOCK everywhere to test the debug logs

as a side note, this also demonstarted what Casey wanted out of a debug system
simple and fast 



31:54
so Casey spamed the TIMED_BLOCK everywhere, but there are some problems
1.  any timing that encloses the rendering are totaly wrong, becuz the block 
    that is timing them is open when it actually gets printed 

    for instance, if you look at OverlayCycleCounters(); that calls DEBUGTextLine();

    if we put a TIMED_BLOCK in DEBUGTextLine(); which is inside the OverlayCycleCounters(); function
    in which we are printing our debug numbers, that messes up the timing numbers.




    so we have to delay our timings for one frame, and print our values from the previous frame.
    so that we can have those blocks be closed before we try to go access them.

2.  this is not Thread-safe. if two threads open or close the blocks together, it can have garbage values.





35:19
Casey aiming to fix the open block, closed block issue 

so we want to make sure every TIMED_BLOCK is closed by the time we get to the OverlayCycleCounters(); function.

so what we need to do is that at the end of a frame, we do a copy of that memory, so we make sure that copied memory is not being used.


36:22
Casey changed his mind. he wants to tackle the "Thread Safe Performance Counters " problem 
first cuz hes more interested in that problem



37:11
Recall how Casey implemented the timed_block timings. 

                handmade_debug.h

                struct timed_block
                {
                    debug_record *Record;
                    
                    timed_block(int Counter, char *FileName, int LineNumber, char *FunctionName, int HitCount = 1)
                    {
                        // TODO(casey): Thread safety
                        ...
                        ...
                        Record->CycleCount -= __rdtsc();
                        Record->HitCount += HitCount;
                    }
                    
                    ~timed_block()
                    {
                        Record->CycleCount += __rdtsc();
                    }
                };


notice that he first subtracted __rdtsc(); in the constructor, then added __rdtsc(); in the destructor.
which means he first subtracted the startTime, and added the endTime 

the math is just endTime - startTime. we are just doing this math in different order:

                T0 - startTime = T1;
                T1 + endTime = T2;


what that means is that if another thread were to come in and read the Record->CycleCount value, it will be garbage.

so we are relying on the fact that between these two operations, any thread that comes in to read Record->CycleCount,
will get the T2 value instead of T1

                T0 - startTime = T1;
                T1 + endTime = T2;




39:25
so the fix is very simple, we just store the startTime.
                
                handmade_debug.h

                struct timed_block
                {
                    ...
                    u64 StartCycles;
                    ...
                    
                    timed_block(int Counter, char *FileName, int LineNumber, char *FunctionName, u32 HitCountInit = 1)
                    {
                        ...
                        ...
                        Record->HitCount += HitCount;                        
                        StartCycles = __rdtsc();                        
                    }
                    
                    ~timed_block()
                    {
                        Record->CycleCount += (__rdtsc() - StartCycles);
                    }
                };



40:22
as a follow up, theres another problem: two threads could come in and modify "Record->HitCount" or "Record->CycleCount"
at the sametime.

we dont quite carea bout the FileName, LineNumber or FunctionName, since these, even when multiple threads are contending
will always overwrite it with correct values. 

                handmade_debug.h

                struct timed_block
                {
                    ...
                    u64 StartCycles;
                    ...
                    
                    timed_block(int Counter, char *FileName, int LineNumber, char *FunctionName, u32 HitCountInit = 1)
                    {
                        ...
                        ...
                        Record->FileName = FileName;
                        Record->LineNumber = LineNumber;
                        Record->FunctionName = FunctionName;

                        Record->HitCount += HitCount;                        
                        StartCycles = __rdtsc();                        
                    }
                    
                    ~timed_block()
                    {
                        Record->CycleCount += (__rdtsc() - StartCycles);
                    }
                };


so the hit count and CycleCount, we want to change it atomically.

so what we want to do here is to do a lock increment of the hit count, so i can accumulate.


41:13
so Casey made the following change 


                handmade_debug.h

                struct timed_block
                {
                    ...
                    u64 StartCycles;
                    ...
                    
                    timed_block(int Counter, char *FileName, int LineNumber, char *FunctionName, u32 HitCountInit = 1)
                    {
                        ...
                        ...
                        Record->FileName = FileName;
                        Record->LineNumber = LineNumber;
                        Record->FunctionName = FunctionName;

                        AtomicAddU32(&debug->HitCount, HitCount);                      
                        StartCycles = __rdtsc();                        
                    }
                    
                    ~timed_block()
                    {
                        u32 DeltaTime = (__rdtsc() - StartCycles)
                        AtomicAddU32(&Record->CycleCount , DeltaTime);
                        AtomicAddU32(&Record->HitCount , HitCount);
                    }
                };



so Casey added the atomic increment back in

                                



42:32
Casey now adding the AtomicAddU32(); in the handmade_instrinsics.h function 

                handmade_instrinsics.h

                inline u64 AtomicAddU32(u32 volatile *Value, u32 Addend)
                {
                    // NOTE(casey): Returns the original value _prior_ to adding
                    u32 Result = _InterlockedExchangeAdd32((__int32 *)Value, Addend);

                    return(Result);
                }





48:11
so now what we want to do is to make sure OverlayCycleCounters is atomic as well.

Casey also made an optimization.
                
currently, we have 

                handmade_debug.h

                struct debug_record
                {
                    char* FileName;
                    char* FunctionName;

                    u32 LineNumber;
                    u32 Reserved;

                    u32 CycleCount;
                    u32 HitCount; 
                };

Casey wants to combine CycleCount and HitCount into one u64, so they are updated atomically together.

So Casey changed it to 

                handmade_debug.h

                struct debug_record
                {
                    char* FileName;
                    char* FunctionName;

                    u32 LineNumber;
                    u32 Reserved;

                    u64 HitCount_CycleCount; 
                };


so back to our timed_block struct, what happens is that we have 

-   in the Delta, the lower 32 bit is just 
                (__rdtsc() - StartCycles)

    the upper half 32 bit is 

                (u64)HitCount << 32;

    this way we have CycleCount and HitCount all combined into one 64 bit number 


-   full code below:

                handmade_debug.h

                struct timed_block
                {
                    debug_record* Record;
                    u64 StartCycles;
                    u32 HitCount;
                    
                    timed_block(int Counter, char *FileName, int LineNumber, char *FunctionName, u32 HitCountInit = 1)
                    {
                        HitCount = HitCountInit;
                        ...
                        ...
                 
                        StartCycles = __rdtsc();                        
                    }
                    
                    ~timed_block()
                    {
                        u64 Delta = (__rdtsc() - StartCycles) | ((u64)HitCount << 32);
                        AtomicAddU64(&Record->HitCount_CycleCount, Delta);
                    }
                };


50:12
so then instead of AtomicAddU32();, its actually AtomicAddU64();

                handmade_instrinsics.h

                inline u64 AtomicAddU64(u64 volatile *Value, u64 Addend)
                {
                    // NOTE(casey): Returns the original value _prior_ to adding
                    u64 Result = _InterlockedExchangeAdd64((__int64 *)Value, Addend);

                    return(Result);
                }



51:16
so now when we print it out the debug values in OverlayCycleCounters();, it will look like 

-   should be straight-forward. We are just extracting the top and bottom 32 bits to get 
    HitCount and CycleCount

-   notice when we read the value, we want to read it atomically as well, and we are clearing the value as well
    
    which is why we are calling 

                u64 HitCount_CycleCount = AtomicExchangeU64(&Counter->HitCount_CycleCount, 0);

    to read the values, as well as clearing it to 0.

-   full code below

                handmade.cpp

                internal void OutputDebugRecords(u32 CounterCount, debug_record *Counters)
                {
                    for(u32 CounterIndex = 0; CounterIndex < CounterCount; ++CounterIndex)
                    {
                        debug_record *Counter = Counters + CounterIndex;

                        u64 HitCount_CycleCount = AtomicExchangeU64(&Counter->HitCount_CycleCount, 0);
                        u32 HitCount = (u32)(HitCount_CycleCount >> 32);
                        u32 CycleCount = (u32)(HitCount_CycleCount & 0xFFFFFFFF);
                        
                        if(HitCount)
                        {
                #if 1
                            char TextBuffer[256];
                            _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                        "%s(%d): %ucy %uh %ucy/h",
                                        Counter->FunctionName,
                                        Counter->LineNumber,
                                        CycleCount,
                                        HitCount,
                                        CycleCount / HitCount);
                            DEBUGTextLine(TextBuffer);
                #endif
                        }
                    }
                }


53:13
Casey adding the AtomicExchangeU64(); in handmade_instrinsics.h

                handmade_instrinsics.h

                inline u64 AtomicExchangeU64(u64 volatile *Value, u64 New)
                {
                    u64 Result = _InterlockedExchange64((__int64 *)Value, New);

                    return(Result);
                }



54:56
currently our system has very low overhead, the only atomic operatio in TIMED_BLOCK is in the destructor


                handmade_debug.h

                struct timed_block
                {
                    debug_record* Record;
                    u64 StartCycles;
                    u32 HitCount;
                    
                    timed_block(int Counter, char *FileName, int LineNumber, char *FunctionName, u32 HitCountInit = 1)
                    {
                        HitCount = HitCountInit;
                        ...
                        ...
                 
                        StartCycles = __rdtsc();                        
                    }
                    
                    ~timed_block()
                    {
                        u64 Delta = (__rdtsc() - StartCycles) | ((u64)HitCount << 32);
        ------------->  AtomicAddU64(&Record->HitCount_CycleCount, Delta);
                    }
                };

and this atomic operation is not particularly expensive. 



55:33
Casey goes on to also print the debug_records from the handmade_optimized.cpp translation unit


59:50
now since we are thread safe, Casey starts to put the TIMED_BLOCK(); in the job threads. 


1:03:10
someone in Q/A asked if Casey aligned the debug logs in columns

                internal void
                OutputDebugRecords(u32 CounterCount, debug_record *Counters)
                {
                    for(u32 CounterIndex = 0;
                        CounterIndex < CounterCount;
                        ++CounterIndex)
                    {
                        debug_record *Counter = Counters + CounterIndex;

                        u64 HitCount_CycleCount = AtomicExchangeU64(&Counter->HitCount_CycleCount, 0);
                        u32 HitCount = (u32)(HitCount_CycleCount >> 32);
                        u32 CycleCount = (u32)(HitCount_CycleCount & 0xFFFFFFFF);
                        
                        if(HitCount)
                        {
                #if 1
                            char TextBuffer[256];
                            _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                        "%32s(%4d): %10ucy %8uh %10ucy/h",
                                        Counter->FunctionName,
                                        Counter->LineNumber,
                                        CycleCount,
                                        HitCount,
                                        CycleCount / HitCount);
                            DEBUGTextLine(TextBuffer);
                #endif
                        }
                    }
                }


1:09:44
someone in the Q/A mentioned the cycle count is changing frame, wouldnt better to average them?

Casey says this will be addressed later on in terms of debug visualization



1:10:11
have you considered thread local storage instead of atomic operations for every thread?


Casey asks why do you want to do that? cuz local storage is only available on certain platforms.
If you put them in local storage, you have to naser the question, how do you get the out of local storage, 
who is the thread that does that


1:14:44
Someone asked to explain the difference between mutex and interlocks

a lot of times those terms are used imprecisely.

inside the processor, we have cores, and inside the core, we have caches


                Core0                       Core1

                L1 Cache                    L1 Cache


                 ________________________________________
                |                                       |
                |           Shared Memory               |
                |                                       |
                |_______________________________________|

                 ________________________________________
                |                                       |
                |             Memory                    |
                |                                       |
                |                                       |
                |    a                                  |
                |                                       |
                |_______________________________________|


the only thing that happens on the atomic add operation, is that a particular core will annouce that 
he now needs to own 'a' address. So now Core0 will have exclusive access on it. This is done through all the MESI stuff,
which Casey mentioned previously 

A mutex
is not anything like that. A mutex has arbiturary performance penalty to it that depends on the locking situation.
The mutex is programming concept built ontop of the atomic add. What a mutex does is that it I have two pieces of code 
that needs to be in critical section


the mutex uses the atomic operation on the processor to set a lock.
It sets a lock to 0 when no one uses it, and it sets it to 1 if someone has it 

so its likely that its doing a AtomicCompareExchange on lock



so a mutex may have unbounded amount of time that has to go by, and you have to have loops to check.
Mutex also may involve the operating system, if you want to put the thread to sleep, if the mutex is taken.

The only time that the mutex is comparable to AtomicCompareExchange is when the lock is uncontested.



1:21:37
Can you still time multiple section within a function?

Yes, you can, you just have to wrap them in brackets.


1:23:05
How do threads wait on lock free structures?
a "lock free" structure is a misnomer (misnomer means wrong name);

the correct term is "wait free"
https://cs.brown.edu/~mph/Herlihy91/p124-herlihy.pdf

Casey says this is the bible of multi-threading

