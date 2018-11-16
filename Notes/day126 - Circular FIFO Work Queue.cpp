Handmade Hero Day 126 - Circular FIFO Work Queue

Summary:
solved the concurrency bug we had in CompleteAndGetNextWorkQueueEntry();, where 
we are not protecting the critical section.

hugely polished and hugely refactored the work queue system.
made work_queue system cross platform.

declared the work queue in the handmade_platform.cpp file 
defined it in the platform specific files. (win32_handmade.cpp); in this case

demoed the the work_queue system in the tiled rendering code, as well as the test sinppet code. 

made the work_queue circular fifo 

fixed a few multithreading bugs

did some profiling, and found out that with multithreading, 
we are hitting full screen 1920 x 1080 at 60 frames per second, sub-pixel, in the software renderer

someone mentioned in the Q/A whether threads and processes are paused when you are at a break point in one of the thread
Casey showed a few tricks to help with that.

Keyword:
multithreading, work queue



1:34
apparently there is a huge bug in the CompleteAndGetNextWorkQueueEntry(); function


                internal work_queue_entry
                CompleteAndGetNextWorkQueueEntry(work_queue *Queue, work_queue_entry Completed)
                {
                    ...
                    ...

                    if(Queue->NextEntryToDo < Queue->EntryCount)
                    {
                        uint32 Index = InterlockedIncrement((LONG volatile *)&Queue->NextEntryToDo) - 1;
                        Result.Data = Queue->Entries[Index].UserPointer;
                        Result.IsValid = true;
                        _ReadBarrier();
                    }

                    return(Result);
                }

the problem is that multiple threads  coud get inside the 

                if(Queue->NextEntryToDo < Queue->EntryCount)
                {
                    ...
                }

condition. Queue->NextEntryToDo and Queue->EntryCount are not in critical section.
this means that multiple threads will be calling the InterlockedIncrement(); function.




instead, Casey opt to use InterlockedCompareExchange();
https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/nf-wdm-interlockedcompareexchange

            

                internal work_queue_entry
                CompleteAndGetNextWorkQueueEntry(work_queue *Queue, work_queue_entry Completed)
                {
                    ...
                    ...

                    uint32 OriginalNextEntryTodo = Queue->NextEntryToDo;                    
                    if(Queue->NextEntryToDo < Queue->EntryCount)
                    {
                        uint32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToDo,
                                                                    OriginalNextEntryTodo + 1,
                                                                    OriginalNextEntryTodo);
                        if(Index == OriginalNextEntryTodo)
                        {
                            Result.Data = Queue->Entries[Index].UserPointer;
                            Result.IsValid = true;
                            _ReadBarrier();
                        }
                    }

                    return(Result);
                }

pretty much the idea of this function is that, still multiple threads will call the InterlockedCompareExchange();

so assume both threadA and threadB gest inside the "if(Queue->NextEntryToDo < Queue->EntryCount)"


....... assume both of them equal 1
                uint32 OriginalNextEntryTodo = Queue->NextEntryToDo; 

threadA:        uint32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToDo,
                                                            OriginalNextEntryTodo + 1,
                                                            OriginalNextEntryTodo);

....... Queue->NextEntryToDo becomes 2


then when threadB calls this line 

....... assume both of them equal 1 when thread B enters inside here
                uint32 OriginalNextEntryTodo = Queue->NextEntryToDo; 

...... threadA calls it first, and Queue->NextEntryToDo becomes 2

                uint32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToDo,  
                                                            OriginalNextEntryTodo + 1,
                                                            OriginalNextEntryTodo);

Index will return 2 here, becuz thats the "original" value of Queue->NextEntryToDo.
And the "if(Index == OriginalNextEntryTodo)" will fail.

this is how Casey uses the InterlockedCompareExchange function to enforce critical section.


6:05
there is an edge case here, becuz we could fail even if there is work to do.








9:00
Casey finishing up the work queue api design. Also Casey made the work_queue to be fifo.

The concept of fifo is pretty much the same as I what I did on my github, which is to implement a circular queue with a list.


I will describe the complete design 
-    So in the game_memory struct, we added this array of platform_work_queue.
it is designed so that regardless of platform, win32, linux or what not, anyone can use it.
our game will have a platform_work_queue that anyone can use on any platform.                



                handmade_platform.h

                struct platform_work_queue;

                ...

                typedef struct game_memory
                {
                    ...
                    ...
                    platform_work_queue* RenderQueue;
                    ...
                    ...
                };



-    and of course, anyone will want to have a way to add entries, complete work, or anything to manage the work queue. 
This is becuz the multithreading api will be different depending on the platform. 

so Casey added some functions that people can use 

                handmade_platform.h

                struct platform_work_queue;

                typedef void platform_add_entry(platform_work_queue *Queue, void *Data);
                typedef void platform_complete_all_work(platform_work_queue *Queue);

                ...
                ...

                typedef struct game_memory
                {
                    ...
                    ...

                    platform_work_queue* RenderQueue;

                    platform_add_entry *PlatformAddEntry;
                    platform_complete_all_work *PlatformCompleteAllWork;

                    ...
                    ...
                };


-    then finally we come to realize that we need a way to describe the job information as well when we 
call the platform_add_entry();

the flow chart and API that we designed it to be is that:

1.    we add an entry to the work_queue by calling platform_add_entry();
    to do that, we have to pass the job description information 

2.    then we call platform_complete_all_work(); all the threads take work from the work_queue and complete them.



we are making a change to the "job description" part. IN the previous two episodes, The only job description informaiton 
is just a string, therefore platform_add_entry only has void* Data as an argument. 

now we are making job description is to contain 2 things 

thread_job_description
1. what_job_this_is        platform_work_queue_callback.
2. job data,            void* Data


hence we are adding this callback to the platform_add_entry(); function

                handmade_platform.h

                struct platform_work_queue;
                #define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue *Queue, void *Data)
                typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);
                    
                typedef void platform_add_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
                typedef void platform_complete_all_work(platform_work_queue *Queue);
                typedef struct game_memory

                ...
                ...

                typedef struct game_memory
                {
                    ...
                    ...

                    platform_work_queue* RenderQueue;

                    platform_add_entry *PlatformAddEntry;
                    platform_complete_all_work *PlatformCompleteAllWork;

                    ...
                    ...
                };



25:54
Casey explaining how the PLATFORM_WORK_QUEUE_CALLBACK macro works 

so initially, he had a compiler error  where 
                
                typedef void platform_work_queue_callback(platform_work_queue* Queue, void* Data);
                PlatformAddEntry(RenderQueue, DoTiledRenderWork, Work);



and the compiler error is, 
            "platform_add_entry : cannot convert arugment 2 from overloaded-function" to 
                'platform_work_queuecallback (__cdecl *)'

still dont quite get how does this macro work.








as mentioned before, different platforms has different api.
So even for the platform_work_queue, it will be platform specific.

we merely declared 
                handmade_platform.h

                struct platform_work_queue;
in handmade_platform.h 



but the actual definition we still put it in win32_handmade.cpp
notice that the platform_work_queue is a circular fifo array.
More details later.

                struct platform_work_queue_entry
                {
                    platform_work_queue_callback *Callback;
                    void *Data;
                };

                struct platform_work_queue
                {
                    uint32 volatile CompletionGoal;
                    uint32 volatile CompletionCount;
                    
                    uint32 volatile NextEntryToWrite;
                    uint32 volatile NextEntryToRead;
                    HANDLE SemaphoreHandle;

                    platform_work_queue_entry Entries[256];
                };


As a side note, notice in platform_work_queue_entry, its got the callback and the data. 
The two, it describes the job that the thread has to do. 




Overall, graphically, our design looks like below 

any platform code will define the functions, then platform-independent code will just call these functions agnostically

                win32 platform
             ________________________________
            |                                |
            |    game_memory                  |
            |                                |
            |    PlatformAddEntry --------------->  win32_PlatformAddEntry function
            |    PlatformCompleteAllWork -------->  win32_PlatformCompleteAllWork function
            |                                |
            |________________________________|



                handmade.cpp
             __________________________________
            |                                   |
            |                                   |
            |     renderer                       |
            |                                  | 
            |     PlatformAddEntry();          |
            |     PlatformCompleteAllWork();   |
            |                                   |
            |__________________________________|





Just as we explained above, in our renderer, we do exactly that: we call these two functions 

-    notice as we loop through the tiles, we are calling PlatformAddEntry(); to add entries to the work_queue.
    we are passing a function, for the "what job this is" part, and then the "Work" variable, for the "Job Data" part
    these two gives the full information of a job description.

-    finally we call PlatformCompleteAllWork();


                handmade_render_group.cpp

                internal void
                TiledRenderGroupToOutput(platform_work_queue *RenderQueue,
                                         render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {
                    ...

                    tile_render_work WorkArray[TileCountX*TileCountY];

                    ...

                    int WorkCount = 0;
                    for(int TileY = 0; TileY < TileCountY; ++TileY)
                    {
                        for(int TileX = 0; TileX < TileCountX; ++TileX)
                        {
                            tile_render_work *Work = WorkArray + WorkCount++;

                            // TODO(casey): Buffers with overflow!!!
                            rectangle2i ClipRect;
                            ClipRect.MinX = TileX*TileWidth + 4;
                            ClipRect.MaxX = ClipRect.MinX + TileWidth - 4;
                            ClipRect.MinY = TileY*TileHeight + 4;
                            ClipRect.MaxY = ClipRect.MinY + TileHeight - 4;

                            Work->RenderGroup = RenderGroup;
                            Work->OutputTarget = OutputTarget;
                            Work->ClipRect = ClipRect;
                #if 1
                            // NOTE(casey): This is the multi-threaded path
                            PlatformAddEntry(RenderQueue, DoTiledRenderWork, Work);
                #else
                            // NOTE(casey): This is the single-threaded path
                            DoTiledRenderWork(RenderQueue, Work);
                #endif
                        }
                    }

                    PlatformCompleteAllWork(RenderQueue);
                }



and surely enough, we define these two functions in the platform code.
in win32_handmade.cpp, we do so:

This is also a good point to review how the threads once takes jobs cuz Casey did another refactor.

                
so when the user code jobs to the queue, we call this function
whenever we finish submitting a job, we want to wake a thread up,

the last 5 lives do so 

                ++Queue->CompletionGoal;
                _WriteBarrier();
                _mm_sfence();
                Queue->NextEntryToWrite = NewNextEntryToWrite;
                ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);

full code below:
                win32_handmade.cpp

                internal void
                Win32AddEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data)
                {
                    // TODO(casey): Switch to InterlockedCompareExchange eventually
                    // so that any thread can add?
                    uint32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
                    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
                    platform_work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
                    Entry->Callback = Callback;
                    Entry->Data = Data;
                    ++Queue->CompletionGoal;
                    _WriteBarrier();
                    _mm_sfence();
                    Queue->NextEntryToWrite = NewNextEntryToWrite;
                    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
                }
                

-    35:55
Casey mentioned that for the time being, there is no place where we reset the queue. 
so as a temporary measure, we just reset it once by setting everything to 0 in Win32CompleteAllWork();

-    39:06
also apparently, Casey said the last two lines, Queue->CompletionGoal = 0; and CompletionCount = 0; are not thread safe

                internal void
                Win32CompleteAllWork(platform_work_queue *Queue)
                {
                    while(Queue->CompletionGoal != Queue->CompletionCount)
                    {
                        Win32DoNextWorkQueueEntry(Queue);
                    }

                    Queue->CompletionGoal = 0;
                    Queue->CompletionCount = 0;
                }


-    now the threads are in an while loop. if it doesnt have work to do, its gets put into sleep.

                DWORD WINAPI ThreadProc(LPVOID lpParameter)
                {
                    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

                    for(;;)
                    {
                        if(Win32DoNextWorkQueueEntry(ThreadInfo->Queue))
                        {
                            WaitForSingleObjectEx(ThreadInfo->Queue->SemaphoreHandle, INFINITE, FALSE);
                        }
                    }
                }


-    this is where the threads jobs from the platform_work_queue.
    this is just the same part that Casey fixed at the beginning of the episode.

    once we get inside the if(Index == OriginalNextEntryToRead) statement,
    we call Entry.Callback(); and we InterlockedIncrement(); the CompleteCount.

                internal bool32
                Win32DoNextWorkQueueEntry(platform_work_queue *Queue)
                {
                    bool32 WeShouldSleep = false;

                    uint32 OriginalNextEntryToRead = Queue->NextEntryToRead;
                    uint32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
                    if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
                    {
                        uint32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                                                  NewNextEntryToRead,
                                                                  OriginalNextEntryToRead);
                        if(Index == OriginalNextEntryToRead)
                        {        
                            platform_work_queue_entry Entry = Queue->Entries[Index];
                            Entry.Callback(Queue, Entry.Data);
                            InterlockedIncrement((LONG volatile *)&Queue->CompletionCount);
                        }
                    }
                    else
                    {
                        WeShouldSleep = true;
                    }

                    return(WeShouldSleep);
                }



22:22
Casey has previously mentioned that sometimes we may want our threads to look for jobs from two queues,
so you can imagine us refactoring the code to be 

                win32_handmade.cpp

                DWORD WINAPI ThreadProc(LPVOID lpParameter)
                {
                    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

                    for(;;)
                    {
                        if(Win32DoNextWorkQueueEntry(ThreadInfo->Queue) ||
                            Win32DoNextWorkQueueEntry(ThreadInfo->OtherQueue) )
                        {
                            WaitForSingleObjectEx(ThreadInfo->Queue->SemaphoreHandle, INFINITE, FALSE);
                        }
                    }
                }

He didnt leave this code in.





28:17
as we startup, we assign the PlatformAddEntry and PlatformCompleteAllWork functions 


                win32_handmade.cpp



                int CALLBACK
                WinMain(HINSTANCE Instance,
                        HINSTANCE PrevInstance,
                        LPSTR CommandLine,
                        int ShowCode)
                {
                    ...
                    ...

                    game_memory GameMemory = {};
                    GameMemory.PermanentStorageSize = Megabytes(256);
                    GameMemory.TransientStorageSize = Gigabytes(1);
                    GameMemory.HighPriorityQueue = &Queue;
                    GameMemory.PlatformAddEntry = Win32AddEntry;
                    GameMemory.PlatformCompleteAllWork = Win32CompleteAllWork;

                    ...
                    ...
                }


and as we call GameUpdateAndRender, we pass it to the platform_independent layer.

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    BEGIN_TIMED_BLOCK(GameUpdateAndRender)

                    ...
                    ...

                    game_state *GameState = (game_state *)Memory->PermanentStorage;
                    if(!Memory->IsInitialized)
                    {
                        PlatformAddEntry = Memory->PlatformAddEntry;
                        PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;
                        
                        ...
                        ...

                    }
                }





With this work queue design, you can now see that the flexibility of it.

in win32_handmade.cpp 

                
                int CALLBACK WinMain(...)
                {
                    ...
                    ...
                    win32_thread_info ThreadInfo[7];

                    platform_work_queue Queue = {};
                    
                    Queue.SemaphoreHandle = CreateSemaphoreEx(...);
                    
                    for(uint32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
                    {
                        ...
                        ...

                        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Info, 0, &ThreadID);
                        CloseHandle(ThreadHandle);
                    }

                    Win32AddEntry(&Queue, DoWorkerWork, "String A0");
                    Win32AddEntry(&Queue, DoWorkerWork, "String A1");
                    Win32AddEntry(&Queue, DoWorkerWork, "String A2");

                    ...
                    ...

                    Win32CompleteAllWork(&Queue);

                    ...
                    ...
                }

                                
                internal PLATFORM_WORK_QUEUE_CALLBACK(DoWorkerWork)
                {
                    char Buffer[256];
                    wsprintf(Buffer, "Thread %u: %s\n", GetCurrentThreadId(), (char *)Data);
                    OutputDebugStringA(Buffer);
                }

then in the renderer, we do also use our api design 

                handmade_render_group.cpp

                internal void
                TiledRenderGroupToOutput(platform_work_queue *RenderQueue,
                                         render_group *RenderGroup, loaded_bitmap *OutputTarget)
                {
                    ...

                    tile_render_work WorkArray[TileCountX*TileCountY];

                    ...

                    int WorkCount = 0;
                    for(int TileY = 0; TileY < TileCountY; ++TileY)
                    {
                        for(int TileX = 0; TileX < TileCountX; ++TileX)
                        {
                            tile_render_work *Work = WorkArray + WorkCount++;

                            // TODO(casey): Buffers with overflow!!!
                            rectangle2i ClipRect;
                            ClipRect.MinX = TileX*TileWidth + 4;
                            ClipRect.MaxX = ClipRect.MinX + TileWidth - 4;
                            ClipRect.MinY = TileY*TileHeight + 4;
                            ClipRect.MaxY = ClipRect.MinY + TileHeight - 4;

                            Work->RenderGroup = RenderGroup;
                            Work->OutputTarget = OutputTarget;
                            Work->ClipRect = ClipRect;
                #if 1
                            // NOTE(casey): This is the multi-threaded path
                            PlatformAddEntry(RenderQueue, DoTiledRenderWork, Work);
                #else
                            // NOTE(casey): This is the single-threaded path
                            DoTiledRenderWork(RenderQueue, Work);
                #endif
                        }
                    }

                    PlatformCompleteAllWork(RenderQueue);
                }


                internal PLATFORM_WORK_QUEUE_CALLBACK(DoTiledRenderWork)
                {
                    tile_render_work *Work = (tile_render_work *)Data;

                    RenderGroupToOutput(Work->RenderGroup, Work->OutputTarget, Work->ClipRect, true);
                    RenderGroupToOutput(Work->RenderGroup, Work->OutputTarget, Work->ClipRect, false);
                }





Recall that this line of 

                internal PLATFORM_WORK_QUEUE_CALLBACK(DoTiledRenderWork);

is effectively defining a DoTiledRenderWork function becuz of this macro,

                handmade_platform.h

                #define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue *Queue, void *Data)
                typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

so we really get 

                internal void DoTiledRenderWork(platform_work_queue, void* Data);








38:42
Casey making the work_queue into a circular buffer.
notice that if this work_queue is for multithreading, it is crucial to have all the volatile keywords
for all the variables.

                struct platform_work_queue
                {
                    uint32 volatile CompletionGoal;
                    uint32 volatile CompletionCount;
                    
                    uint32 volatile NextEntryToWrite;
                    uint32 volatile NextEntryToRead;
                    HANDLE SemaphoreHandle;

                    platform_work_queue_entry Entries[256];
                };

my implementation is 
https://github.com/maestro92/FIFOList/blob/master/FIFOList/FIFOList.cpp

I use two variables, m_front and m_back.

NextEntryToRead is the equivlanet of m_front in my implementation
NextEntryToWrite is the equivlanet of m_back in my implementation.

in my implementation, I am constantly writting at index m_back




the wrap around is done in here

                internal void
                Win32AddEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data)
                {
                    // TODO(casey): Switch to InterlockedCompareExchange eventually
                    // so that any thread can add?
                    uint32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);

                    ...
                    ...

                    Queue->NextEntryToWrite = NewNextEntryToWrite;

                }
                


58:32
one of the multithreading bug Casey encoutered is in the below code 

initially he had                 

                internal bool32 Win32DoNextWorkQueueEntry(platform_work_queue *Queue)
                {
                    ...

                    uint32 NewNextEntryToRead = (Queue->NextEntryToRead + 1) % ArrayCount(Queue->Entries);
                    uint32 OriginalNextEntryToRead = Queue->NextEntryToRead;
                    if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
                    {
                        uint32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                                                  NewNextEntryToRead,
                                                                  OriginalNextEntryToRead);
                        if(Index == OriginalNextEntryToRead)
                        {        
                            ...
                            ...
                        }
                    }

                    ...
                    ...
                }

then after debugging a race condition, he fixed it with 
the idea is that we only want to read the values of Queue->NextEntryToRead once.

in the version above, reading it twice could give you two different values. 

                internal bool32 Win32DoNextWorkQueueEntry(platform_work_queue *Queue)
                {
                    ...

           ------>  uint32 OriginalNextEntryToRead = Queue->NextEntryToRead;
           ------>  uint32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
                    if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
                    {
                        uint32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                                                  NewNextEntryToRead,
                                                                  OriginalNextEntryToRead);
                        if(Index == OriginalNextEntryToRead)
                        {        
                            ...
                            ...
                        }
                    }

                    ...
                    ...
                }


59:23
got the TiledRenderGroupToOutput(); function to work, and now the code is running in multi-threading mode.
Casey opened the task-manager and showed that handmade_heros CPU usage is around 16%


1:00:51
Casey asks the question, can we crank it up to 1920 x 1080 now?

recall that making the it 1920 x 1080 is making the win32_offscreen_buffer 1920 x 1080 in dimensions.
which means we are populating a lot more pixels.

See day 040 how toggle full screen works 

Sees some slight lag

Casey increased the thread count, it is at playable speed.



1:02:58
since the work load is done completely in order, what if one work load takes very long, while the other threads
wrap around in the queue, such that a new work load overwrites the slot with the one that is still running. 


That is definitely a problem. There are a lot of different ways to approach this.
The way Casey approaches it is that, I shouldnt be chewing that much work. 

in our case, our threads renders a fixed number of tiles, and it needs to have all of our tiles completed before it moves on. 
so we can just very easily construct one high priority queue that always have enough space to render our tiles, and thats never 
gonna be a problem.  

for background threads, we can have a separate queue, and we just dont start new asset loads if our queue is looking like its 
getting filled 



1:04:52
apparently the previous attempt was in debug build.
so Casey turned to optimized build and ran 1920 x 1080, and it indeed ran in full speed.



1:12:04
with multithreading, we are hitting full screen 1920 x 1080 at 60 frames per second, sub-pixel, in the software renderer



1:13:05
someone in the Q/A asked, 

instead of asserting when the queue overflow when adding an entry, will it be better just wait for an entry to be read, when 
there is no space to write?

Casey says, he doesnt really like to do if it can be avoided becuz that means it might stall the game, in the middle of a 
frame. Casey prefers constant time insertion.



1:13:47
when stepping through multi-threaded code, does all the other threads pause as well?

Casey says you can actually set that?

Casey showed us the toggle for "Break all processes when one process breaks" under 
Options

Debugging
    General

turns out its Processes you can break.

another option is that you can choose to "Freeze" or "thaw" threads in the Threads debugging window 
