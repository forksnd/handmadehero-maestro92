Handmade Hero Day 177 - Automatic Performance Counters

Summary:
mentioned that our debug cycle counter from day 176 is not "simple/free" enough cuz it requires us to add 
enums everytime we want to profile a block of code. 

mentioned that we can use preprocessor directives to help us get rid of enums 

introduced some C preprocessor directives: __FILE__, __FUNCTION__, __LINE__ and __COUNTER__

aims to use __FILE__, __FUNCTION__, __LINE__ and __COUNTER__ to replace the enums,
which tells us where our timed block is, and it stores the debug values 

ran into the problem of needing to map __FILE__, __FUNCTION__, __LINE__ and __COUNTER__
into an index that we can use to access into our debug_record arrays.

thought of a trick of using __COUNTER__ to accomplish it.

defined the debug_record struct, we now store debug_record into an array.

Keyword:
debug system, preprocessor directives, profiling


3:03
recall in day 176, we replaced the BEGIN_TIMED_BLOCK(); and END_TIMED_BLOCK(); with the timed_block struct class,
but Casey mentioned that this is still no where close to the level of ease of use that he wants 

Casey says one criteria is that he wants to the profiling code to be as free as possible. 

that is not currently the case. Assume we want to measure the DrawBitmap(); function

                handmade_render_group.cpp

                internal void DrawBitmap()
                {
                    TIMED_BLOCK(DrawBitmap);
                    ...
                    ...
                }

this will give you a compiler error of "DebugCycleCounter_DrawBitmap" undeclared identifier.
Recall that the #define we have is 

                handmade_platform.h

                #define END_TIMED_BLOCK_(StartCycleCount, ID) DebugGlobalMemory->Counters[ID].CycleCount += __rdtsc() - StartCycleCount; ++DebugGlobalMemory->Counters[ID].HitCount;
                #define END_TIMED_BLOCK(ID) END_TIMED_BLOCK_(StartCycleCount##ID, DebugCycleCounter_##ID)

which is parsing the enum DebugCycleCounter_##ID. 

the "DebugCycleCounter_DrawBitmap" enum needs to correspond to one of the enums that was defined

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


and thats too much work.


4:40
think of an example like Assert();
you put them anywhere you want, and be done with it. That what we want our block timing to end up like

7:50
Casey is gonna start to get rid of our IDs
turns out in C, there is a relative easy way that we can get some information about where this information is 
in such a way that we dont have to introduce any additional enums


8:30
if we wanted to, at any point of a C program, we can use a set of preprocessor directives 

here are a few 

                __FILE__;
                __FUNCTION__;
                __LINE__;


to use them, its exactly what you imagined 

                char* FileName = __FILE__;      

this you can get the filename of the file thats currently being compiled 


                char* FunctionName = __FUNCTION__;
same thing with the function
        

                int LineNumber = __LINE__;
same with with the line number


10:13
what the compiler is doing here is that it is going to embed these string into our code, when it compiles it. 
so its not done at runtime. It is not going off and asking some oracle what the filename is.

so all of these are done by the compiler, and we dont really have to worry much. 


11:12
there is this other directives that Casey is unsure whether it is allowed in all C compilers. 
                
                __COUNTER__

https://gcc.gnu.org/onlinedocs/gcc-7.2.0/cpp/Common-Predefined-Macros.html

                

11:47
assuming msvc supports this directive, we can do the following 

                int Counter0 = __COUNTER__;                
                int Counter1 = __COUNTER__;                
                int Counter2 = __COUNTER__;                
                int Counter3 = __COUNTER__;                
                int Counter4 = __COUNTER__;                
                int Counter5 = __COUNTER__;

so what it does is that, everytime __COUNTER__ is called, it increments.
so its essentially the number of times __COUNTER__ has appeared in this file
so the values on these ints will be 

                Counter0 = 0;
                Counter1 = 1;
                Counter2 = 2;
                Counter3 = 3;
                Counter4 = 4;
                Counter5 = 5;


there really isnt much of a point of doing this other than the situation below:

assume you have multiple things on the same line, __LINE__ will return true for all of them.
so you would want to use __COUNTER__ to distinguish between all of them 

this is not something you would typically do too much in your actual production C++ code, but it is something 
you will absolutely use all the time in the debug augmentation of that C++ code. 


13:18
Casey will proceed with an example: 

lets say we want to just say "TIMED_BLOCK", and we dont want to know anything else about it.
we just want it to time our block and display it for us later on when we are curious
                
                handmade_render_group.cpp

                internal void DrawBitmap()
                {
                    TIMED_BLOCK
                    ...
                    ...
                }


currently we have the following 

                handmade_debug.h

                #define TIMED_BLOCK(ID) timed_block TimedBlock##ID(DebugCycleCounter_##ID);

                struct timed_block
                {
                    ...
                    ...
                };




so now we refactor it to following:

-   we want it to be TIMED_BLOCK, not TIMED_BLOCK(ID);, so we got rid of the (ID);

-   previously, we used DebugCycleCounter_##ID to help us locate where this TIMED_BLOCK is 
    for instance, if had the DebugCycleCounter_DrawRectangleQuickly enum, and we had 

    TIMED_BLOCK(DebugCycleCounter_DrawRectangleQuickly);, that most likely means that it is in the 
    DebugCycleCounter_DrawRectangleQuickly(); function.

    so in our new system, we want to be able to give each TIMED_BLOCK the context of its location 
    
    this is where __FILE__, __LINE__, __FUNCTION__ comes into play. We pass it into the timed_block constructor.



-   full code below:

                handmade_debug.h

                #define TIMED_BLOCK timed_block TimedBlock_##__LINE__(__FILE__, __LINE__, __FUNCTION__);

                struct timed_block
                {
                    uint64 StartCycleCount;
                    u32 ID;

                    timed_block(char* FileName, int LineNumber, char* FunctionName)
                    {
                        ...
                    }
                };

16:13
so we assume that no one is going to write 

                internal void DrawBitmap()
                {
                    TIMED_BLOCK; TIMED_BLOCK; TIMED_BLOCK; TIMED_BLOCK;
                    ...
                    ...
                }


but even if they do, we can add in the __COUNTER__, to give each timed_block unique locations

                handmade_debug.h

                #define TIMED_BLOCK timed_block TimedBlock_##__LINE__(__FILE__, __LINE__, __FUNCTION__, __COUNTER__);

                struct timed_block
                {
                    uint64 StartCycleCount;
                    u32 ID;

                    timed_block(char* FileName, int LineNumber, char* FunctionName, int counter)
                };



16:53
so now we have given eahc timed_block unique locations identifiers, but we still have one problem 
cuz we would have to map __FILE__, __LINE__, __FUNCTION__, __COUNTER__ down to an id.

recall previously, we were using the enums to index into our debug_cycle_counter array

                #define END_TIMED_BLOCK_(StartCycleCount, ID) 
       --------------->         DebugGlobalMemory->Counters[ID].CycleCount += __rdtsc() - StartCycleCount; 
                                ++DebugGlobalMemory->Counters[ID].HitCount;
                                [this is all one line, i just separated into multiple lines for clarity]

                typedef struct game_memory
                {
                    ...
                    ...
                    debug_cycle_counter Counters[DebugCycleCounter_Count];

                } game_memory;



so the next step is to somehow translate __FILE__, __LINE__, __FUNCTION__, __COUNTER__ into an index.

[note, although once we have new debug system ready, we wont be using this debug_cycle_counter array,
we will still have a different array. so we still face the problme of converting all this  __FILE__, __LINE__, __FUNCTION__, __COUNTER__
information to an array index]


so some of the solutions that come to mind are like create a table, and do a linear search 

so if we were to go down that path, our code will look something like 

                handmade_debug.h

                #define TIMED_BLOCK timed_block TimedBlock_##__LINE__(__FILE__, __LINE__, __FUNCTION__, __COUNTER__);

                struct timed_block
                {
                    uint64 StartCycleCount;
                    u32 ID;

                    timed_block(char* FileName, int LineNumber, char* FunctionName, int counter)
                    {
                        ID = FindIDFromFileNameLineNumber(FileName, LineNumber);
                        BEGIN_TIMED_BLOCK_(StartCycleCount);
                    }

                    ...
                };


however this may not be soo useful if FindIDFromFileNameLineNumber(); takes too long to traversals in its search. 


19:01
it may be, could potentially be, that this is still the best approach. which Casey will mention later 
but Casey wants to try another approach first. 
                

19:23
here goes Caseys other method.
what Casey prefers to do instead, is to store the FileName, LineNumber and then not have to find it until later when we go 
print out this information. 

if you imagine we have a timer_record struct

                handmade_debug.h

                struct timer_record
                {
                    u64 Timestamp;
                    u32 ThreadIndex;
                    u32 LineNumber;
                    char* FileName;
                    char* FunctionName;
                };

then everytime we call timed_block(); and ~timed_block();
we can just record the timestamp and write out one of those strucutres 

something like:

                    timed_block(char* FileName, int LineNumber, char* FunctionName, int counter)
                    {
                        timer_record.Timestamp = ... record the timestamp ...
                    }


                    ~timed_block()
                    {
                        timer_record.Timestamp = ... record the timestamp ...
                    }

the only pollution that this method does is writing a timer_record struct out. 
you can even do a non-temperal store so you dont pollute the cache. 


28:44
so the reason why Casey brings this method up, is that if you plan to planning to call this a reasonable number of times, 
(lets say a million times).
then you are adding several megabytes of write bandwidth, that could affect your performance as well. 

it is worth noting that if you look at timer_record, you can also trim it down so its more cache friendly.


                handmade_debug.h

                struct timer_record
                {
                    u64 Timestamp;
                    char* FileName;
                    char* FunctionName;
                    u16 ThreadIndex;
                    u16 LineNumber;
                };




22:32
so assume our timer_record struct is like this: 

                handmade_debug.h

                struct timer_record
                {
                    u64 Timestamp;
                    char* FileName;
                    char* FunctionName;
                    u32 ThreadIndex;
                    u32 LineNumber;
                };


this is 32 bytes.
[why does Casey assume "char* FileName" and "char* FunctionName" to be 8 bytes each?]

so everytime you do a 32 byte write on the entry and exit. So thats about 64 byte write.

then you would collate later on. 

collate: collect and combine (texts, information, or sets of figures); in proper order.



Casey mention that he hasnt written a debug system in a while, and he hasnt tested much on modern machines.
so he doesnt know which way he would prefer. 




23:31
Casey says what he does know is that when the number of timed blocks gets low enough, we would prefer the log version

23:47
Casey decided to do the log version, cuz its more interesting to him, it also has some nice properties.


24:14
by the way, its worth noting that all of this design and work is compensation for the fact that C++ is not a good language. 

one thing it cant do in a particularly easy way is aggregate this data out. what you rather do, is to have some way 
of generating a set of indicies (just like our previous enums);, 

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

and these indicies would indicate where our timed blocks occur,
and you would want to be able to bake that number into our TIMED_BLOCKs.


24:48
Casey claims that hes having a thought, becuz we are doing a single compilation unit, does that mean 
we can just use __COUNTER__, to actually do that list collapse?


if you are normally compiling in the way that most C++ projects compiler, where you have one file per class:
        
for example:        apple.h, apple.cpp ---> apple.o
                    banna.h, banna.cpp ---> banna.o
                    ...

you wont be able to do this. but if you compile everything as one file, __COUNTER__ actually will give a unit number to any place
that a timed_block occurs. (Recall __COUNTER__ starts the counter per file).

So we might be able to cheese out on this.


27:21
Casey is going to test it out whether this approach could work or not. 
so if Casey passes the __COUNTER__ as the first thing 

                #define TIMED_BLOCK__(Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__)

                timed_block(int Counter, char *FileName, int LineNumber, char *FunctionName, int HitCount = 1)
                {
                    ...
                    ...
                }

                ~timed_block()
                {

                }

so Casey put a TIMED_BLOCK in DEBUGReset(); function and in AllocateGameAssets();

                handmade.cpp 

                DEBUGReset()
                {
                    TIMED_BLOCK
                    ...
                    ...
                }

                internal game_assets* AllocateGameAssets();
                {
                    TIMED_BLOCK
                    ...
                    ...
                }

and these are in totally separate files, Casey wants to see if we end up with the values of 0 and 1 for our counters. 

and it seems to prove Caseys theory.

29:53
we have two translation units in our build.bat file

handmade_optimized.cpp and handmade.cpp


so if we go to handmade.cpp, and put a __COUNTER__ at the end,

that you give us the number of TIMED_BLOCK we have put in handmade.cpp

that means if you define something like 

                handmade.cpp 

                ...
                ...

                debug_record Main_DebugRecords[__COUNTER__];

will give me an array thats big enough to hold all the debug records.

Example:

                foo.cpp

                void foo()
                {
                    TIMED_BLOCK
                }

                void bar()
                {
                    TIMED_BLOCK
                }

                int myArray[__COUNTER__];

if I have a file like this, I will have an array thats int myArray[2]. 
same thing what Casey is saying here. 



furthermore, Casey can make a separate one in the other translation unit 

                handmade_optimized.cpp 

                ...
                ...

                debug_record Optimized_DebugRecords[__COUNTER__];


now we have to arrays to hold both DebugRecords from both translation units. 




31:54
so if we go into handmade_debug.h
we can just use the Counter, index it into the array and overwrite its values 

                handmade_debug.h

                struct timed_block
                {
                    timed_block(int Counter, char* FileName, int LineNumber, char* FunctionName)
                    {
                        debug_record* Record = ... access our debug record array ...
                    }

                };




36:28
so the goal here is that for each compilation unit, Casey wants to define an debug_record array. 
in previous section, we wanted to define a "debug_record Main_DebugRecords[__COUNTER__];" in handmade.cpp 
and "debug_record Optimized_DebugRecords[__COUNTER__];" in handmade_optimized.cpp

to make it easier, Casey added some preprocessing trick in the build.bat file 
Casey predefined a -DDebugRecordArray macro. 

                build.bat

                cl %CommonCompilerFlags% -DDebugRecordArray=DebugRecords_Optimized -O2 -I..\iaca-win64\ -c ..\handmade\code\handmade_optimized.cpp -Fohandmade_optimized.obj -LD
                cl %CommonCompilerFlags% -DDebugRecordArray=DebugRecords_Main -I..\iaca-win64\ ..\handmade\code\handmade.cpp handmade_optimized.obj -Fmhandmade.map -LD /link -incremental:no -opt:ref -PDB:handmade_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender


this way, when we wrote  
                
                handmade.cpp
    
                ...
                ...

                debug_record DebugRecordArray[__COUNTER__];


after compiling, it becomes 


                handmade.cpp
    
                ...
                ...

                debug_record DebugRecords_Main[__COUNTER__];


and as a declaration in handmade_debug.h 

-   again, the DebugRecordArray[] will interpreted as DebugRecords_Main when we compile handmade.cpp,
    then DebugRecords_Optimized when we compile handmade_optimized.cpp


                handmade_debug.h

                debug_record DebugRecordArray[];

                struct timed_block
                {
                    timed_block(int Counter, char* FileName, int LineNumber, char* FunctionName)
                    {
                        debug_record* Record = ... access our debug record array ...
                    }

                };



38:13
now Casey officially writes the debug_record struct 

                handmade_debug.h

                struct debug_record
                {
                    u64 CycleCount;

                    char *FileName;
                    char *FunctionName;
                    
                    u32 LineNumber;
                    u32 HitCount;
                };



38:23
then in constructor and destructor
we just write into the debug_records array 

-   the line below access the entry in the array.

                Record = DebugRecordArray + Counter;

-   then we start to populate the values of the entry

                Record->FileName = FileName;
                Record->LineNumber = LineNumber;
                Record->FunctionName = FunctionName;
                Record->CycleCount -= __rdtsc();
                Record->HitCount += HitCount;

-   full code below:

                handmade_debug.h

                debug_record DebugRecordArray[];

                struct timed_block
                {
                    debug_record *Record;
                    
                    timed_block(int Counter, char *FileName, int LineNumber, char *FunctionName, int HitCount = 1)
                    {
                        // TODO(casey): Thread safety
                        Record = DebugRecordArray + Counter;
                        Record->FileName = FileName;
                        Record->LineNumber = LineNumber;
                        Record->FunctionName = FunctionName;
                        Record->CycleCount -= __rdtsc();
                        Record->HitCount += HitCount;
                    }
                    
                    ~timed_block()
                    {
                        Record->CycleCount += __rdtsc();
                    }
                };



41:27
so now Casey removed the debug_cycle_counter code 


43:13
Casey starting to add different TIMED_BLOCK macros to support different calls 

                handmade_debug.h

                #define TIMED_BLOCK__(Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)
                #define TIMED_BLOCK_(Number, ...) TIMED_BLOCK__(Number, ## __VA_ARGS__)
                #define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ## __VA_ARGS__)


51:20
Casey testing the performance counter from our new debug system by testing with the OverlayCycleCounters();

-   as you can see, now we are interating through the DebugRecords_Main array.

                handmade.cpp 

                internal void OverlayCycleCounters(game_memory *Memory)
                {
                //    DEBUGTextLine("\\5C0F\\8033\\6728\\514E");
                //    DEBUGTextLine("111111");
                //    DEBUGTextLine("999999");
                #if HANDMADE_INTERNAL
                    DEBUGTextLine("\\#900DEBUG \\#090CYCLE \\#990\\^5COUNTS:");
                    for(int CounterIndex = 0;
    --------------->    CounterIndex < ArrayCount(DebugRecords_Main);
                        ++CounterIndex)
                    {
                        debug_record *Counter = DebugRecords_Main + CounterIndex;

                        if(Counter->HitCount)
                        {
                #if 1
                            char TextBuffer[256];
                            _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                        "%s: %I64ucy %uh %I64ucy/h",
                                        Counter->FunctionName,
                                        Counter->CycleCount,
                                        Counter->HitCount,
                                        Counter->CycleCount / Counter->HitCount);
                            DEBUGTextLine(TextBuffer);
                            Counter->HitCount = 0;
                            Counter->CycleCount = 0;
                #endif
                        }
                    }
                #endif
                //    DEBUGTextLine("AVA WA Ta");
                }


52:09
with this, we effectively can put debug times throughout our code, and its effectively free. 
the only work that is being done is 
    
                handmade_debug.h

                timed_block(int Counter, char *FileName, int LineNumber, char *FunctionName, int HitCount = 1)
                {
                    // TODO(casey): Thread safety
                    Record = DebugRecordArray + Counter;
                    Record->FileName = FileName;
                    Record->LineNumber = LineNumber;
                    Record->FunctionName = FunctionName;
                    Record->CycleCount -= __rdtsc();
                    Record->HitCount += HitCount;
                }



55:19
someone in Q/A mentioning his way of doing timed_block without relying on constructor/destructor

                #define USING(obj, data)    for(u8 done = obj.BeginUsing(data); !done; done = obj.EndUsing(data))

Casey says it seems to introduce as many variables as the timed_block way. so he doesnt see any benefit to it.


56:34
why does the array need to be different between builds?

thats cuz we want to print out both arrays 



1:04:49
what other preprocessor directives are useful other than __FILE__, __FUNCTION__, __LINE__ and __COUNTER__?

I dont know many that are useful other than these for debugging.
there are other ones useful for other ones.


1:14:05
Casey gave a rant on public setters and getters in C++;