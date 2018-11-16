Handmade Hero Day 125 - Abstracting the Work Queue

Summary:
made a genearlized work_queue class
this work_queue is used in the multi-threading test code and designed for the renderer 
TiledRenderGroupToOutput function
(not yet completed for the renderer)

mentioned that our work_queue is a lock free queue in the Q/A

Someone in the Q/A asked how expensive it is to spawn a thread?
mentioned that spawning a thread is actually very expensive, so the approach is to 
spawn a bunch of threads at startup, and be read to do work. That way, we dont ever pay the cost 
to spawn threads. 

mentioned that we initially spawned 8 threads for the WinMain(); for the test code. That code is likely to 
persist: we will just spawn our threads at startup.

kind of like allocating memory at startup

Keyword:
multiple-threading, renderer


2:19
recall from day124, we had the following code 

				int CALLBACK
				WinMain(...)
				{
				    win32_state Win32State = {};

				    win32_thread_info ThreadInfo[8];

				    uint32 InitialCount = 0;
				    uint32 ThreadCount = ArrayCount(ThreadInfo);
				    HANDLE SemaphoreHandle = CreateSemaphoreEx(0, InitialCount, ThreadCount,
				                                               0, 0, SEMAPHORE_ALL_ACCESS);

				    for(uint32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
				    {
				    	...
				    	...
				        
				        DWORD ThreadID;
				        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Info, 0, &ThreadID);
				        CloseHandle(ThreadHandle);
				    }

			        PushString(SemaphoreHandle, "String A0");
    				PushString(SemaphoreHandle, "String A1");

    				...
    				...

    			    // TODO(casey): Turn this into something waitable!
				    while(EntryCount != EntryCompletionCount);

				    ...
				    ...
				}

notice in the last line, we are put in an empty while loop(); Instead we would prefer to put the main thread to sleep.


3:27
Casey says instead of putting the main thread to sleep, it can just do some work it self.

so Casey did a bit of refactoring of the code 

				win32_handmade.cpp

				inline void
				DoWorkerWork(int LogicalThreadIndex)
				{
					bool DidSomeWork = false;

					if(NextEntryTodo < EntryCount)
					{
						.. do work ...
						DidSomeWork = true;
					}

					return DidSomeWork;
				}



so for our ThreadProc(); it is essentially that if the DoWorkerWork(); did not do anywork,
we go to sleep.

				DWORD WINAPI ThreadProc()
				{
					for(;;)
					{
						if(!DoWorkerWork())
						{
							WaitForSingleObjectEx();
						}
					}
				}

and now in the MainThread also calls DoWorkerWork(); itself
now, with this structure, our mainthread will just spinlock a little bit. this way the main thread is doing work 
with all its children thread 

				int CALLBACK WinMain(...)
				{
					...
					...

				    Queue.SemaphoreHandle = CreateSemaphoreEx(...);

				    for(uint32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
				    {
				    	...
				    	...

				        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Info, 0, &ThreadID);
				        CloseHandle(ThreadHandle);
				    }

				    PushString(&Queue, "String A0");
				    PushString(&Queue, "String A1");
				    ...
				    ...

				    work_queue_entry Entry = {};
				    while(EntryCount != EntryCompletionCount)
				    {
			            DoWorkerWork(7);
				    }
				   
				   	...
				   	...
				}




7:40
so now that we have fix this code. The next step is that, we want to make it into a format 
that the renderer can make use.

Essentially writing a generalized work_queue class(); that the renderer can use.



12:13 
Casey beginning to change the above code into a single producer, multiple consumer work_queue

so we eventually created this work_queue class, that the threads will get work from. 

				struct work_queue_entry_storage
				{
				    void *UserPointer;
				};

				struct work_queue
				{
				    uint32 volatile EntryCompletionCount;
				    uint32 volatile NextEntryToDo;
				    uint32 volatile EntryCount;
				    HANDLE SemaphoreHandle;

				    work_queue_entry_storage Entries[256];
				};

				struct work_queue_entry
				{
				    void *Data;
				    bool32 IsValid;
				};



Casey also made a bunch APIs for this work_queue function;

for example:

				internal bool32 QueueWorkStillInProgress(work_queue* Queue)
				{
					bool32 Result = (Queue->EntryCount != Queue->EntryCompletionCount);
					return (Result);
				}


				internal void
				AddWorkQueueEntry(work_queue *Queue, void *Pointer)
				{
					...
					...
				}


				internal work_queue_entry
				CompleteAndGetNextWorkQueueEntry(work_queue *Queue, work_queue_entry Completed)
				{
					...
					...
				}


just straight forward API design.




40:36

notice that work_queue now stores a buffer of work_queue_entry_storage, instead of just work_queue_entry.

Casey says his design of the work_queue is that the queue has a buffer that points to entries of anytype
we want. Which is why that instead of 


				struct work_queue
				{
				    uint32 volatile EntryCompletionCount;
				    uint32 volatile NextEntryToDo;
				    uint32 volatile EntryCount;
				    HANDLE SemaphoreHandle;

					work_queue_entry Entries[256];
				};


we have 

				struct work_queue_entry_storage
				{
				    void *UserPointer;
				};

				struct work_queue
				{
				    uint32 volatile EntryCompletionCount;
				    uint32 volatile NextEntryToDo;
				    uint32 volatile EntryCount;
				    HANDLE SemaphoreHandle;

					work_queue_entry_storage Entries[256];
				};

				struct work_queue_entry
				{
				    void *Data;
				    bool32 IsValid;
				};


the idea is that work_queue_entry_storage.UserPointer contains all the job information.




Lets walk through our example. 
in our code, the job information is just the string value.


so when we put jobs onto the queue by calling PushString();, we just pass the string to our work queue.

				internal void
				PushString(work_queue *Queue, char *String)
				{
				    AddWorkQueueEntry(Queue, String);    
				}


and in the AddQueueEntry function, we store the string in UserPointer. By doing that, we are literraly 
storing the our "job information" into UserPointer. Again, our job information here is just the strings.

				internal void
				AddWorkQueueEntry(work_queue *Queue, void *Pointer)
				{
					...
				    Queue->Entries[Queue->EntryCount].UserPointer = Pointer;
				    ...
				    ...
				    ++Queue->EntryCount;
				    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
				}




Later on, when threads come take jobs from the queue, we create the work_queue_entry and we give it to the thread.
when we create the work_queue_entry for the thread, we pass the job information. 

				Result.Data = Queue->Entries[Index].UserPointer;

full code below.

				internal work_queue_entry
				CompleteAndGetNextWorkQueueEntry(work_queue *Queue, work_queue_entry Completed)
				{
				    work_queue_entry Result;
				    Result.IsValid = false;

				    if(Queue->NextEntryToDo < Queue->EntryCount)
				    {
				    	...
				        Result.Data = Queue->Entries[Index].UserPointer;
				    	...
				    	...
				    }

				    return(Result);
				}


finally when the thread actually executes the job, in our case, we print it.

				inline void
				DoWorkerWork(work_queue_entry Entry, int LogicalThreadIndex)
				{
				    Assert(Entry.IsValid);

				    char Buffer[256];
				    wsprintf(Buffer, "Thread %u: %s\n", LogicalThreadIndex, (char *)Entry.Data);
				    OutputDebugStringA(Buffer);
				}




I would prefer to call it 


				struct work_queue_entry
				{
				    void *JobInforation;
				};

				struct work_queue
				{
				    uint32 volatile EntryCompletionCount;
				    uint32 volatile NextEntryToDo;
				    uint32 volatile EntryCount;
				    HANDLE SemaphoreHandle;

					work_queue_entry Entries[256];
				};

				struct thread_job
				{
				    void *JobInformation;
				    bool32 IsValid;
				};




graphically we have,


		struct work_queue
		{


			Entries
			 ________________
			|                | 
			| JobInforation -|------->  job information
			|________________|
			|                | 
			| JobInforation -|------->  job information
			|________________|
			|                | 
			| JobInforation -|------->  job information
			|________________|
			|                | 
			| JobInforation -|------->  job information
			|________________|
                   ...
                   ...
                   ...

		};



then we create thread_job, we give it its job information


			thread_job
			{
				...
				JobInformation = job information 
				...
			}

and gives this to a thread, let the thread do shit.











47:21
When Casey was designing the work_queue, he wrote a bit of usage code to help guide him the API design. 

recall in handmade_render_group.cpp so we wrote a TiledRenderGroupToOutput(); function in preparation 
to make the renderer renderer code to multi-threading, 

				handmade_render_group.cpp

				internal void TiledRenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
				{
				    int TileCountX = 4;
				    int TileCountY = 4;

				    // TODO(casey): Make sure that allocator allocates enough space so we can round these?
				    // TODO(casey): Round to 4??
				    int TileWidth = OutputTarget->Width / TileCountX;
				    int TileHeight = OutputTarget->Height / TileCountY;    
				    for(int TileY = 0; TileY < TileCountY; ++TileY)
				    {
				        for(int TileX = 0; TileX < TileCountX; ++TileX)
				        {
				        	... getting ClipRect ...

				            RenderGroupToOutput(RenderGroup, OutputTarget, ClipRect, true);
				            RenderGroupToOutput(RenderGroup, OutputTarget, ClipRect, false);
				        }
				    }
				}



so now he changed it to 

as you can see, the structure is very similar to the test code we wrote in win32_handmade.cpp

				struct tile_render_work
				{
				    render_group *RenderGroup;
				    loaded_bitmap *OutputTarget;
				    rectangle2i ClipRect;
				};


				internal void
				DoTiledRenderWork(void *Data)
				{
				    tile_render_work *Work = (tile_render_work *)Data;

				    RenderGroupToOutput(Work->RenderGroup, Work->OutputTarget, Work->ClipRect, true);
				    RenderGroupToOutput(Work->RenderGroup, Work->OutputTarget, Work->ClipRect, false);
				}


in the TiledRenderGroupToOutput(); function, we create our work queue.

				tile_render_work WorkArray[TileCountX*TileCountY];

-   we first go through the tiles to create our job,
-   then we dispatch threads to do the jobs. 

obviously this is just test code. Casey will complete the multi-threading refactor for the renderer 
in the next few episodes.

notice that the first argument passed in is the platform_work_queue* RenderQueue. Currently that is commented out.

				internal void TiledRenderGroupToOutput(//platform_work_queue *RenderQueue,
				                         render_group *RenderGroup, loaded_bitmap *OutputTarget)
				{
				    int const TileCountX = 4;
				    int const TileCountY = 4;
				    tile_render_work WorkArray[TileCountX*TileCountY];

				    // TODO(casey): Make sure that allocator allocates enough space so we can round these?
				    // TODO(casey): Round to 4??
				    int TileWidth = OutputTarget->Width / TileCountX;
				    int TileHeight = OutputTarget->Height / TileCountY;

				    int WorkCount = 0;
				    for(int TileY = 0; TileY < TileCountY; ++TileY)
				    {
				        for(int TileX = 0; TileX < TileCountX; ++TileX)
				        {
				            tile_render_work *Work = WorkArray + WorkCount++;

				            rectangle2i ClipRect;
				            ClipRect.MinX = TileX*TileWidth + 4;
				            ClipRect.MaxX = ClipRect.MinX + TileWidth - 4;
				            ClipRect.MinY = TileY*TileHeight + 4;
				            ClipRect.MaxY = ClipRect.MinY + TileHeight - 4;

				            Work->RenderGroup = RenderGroup;
				            Work->OutputTarget = OutputTarget;
				            Work->ClipRect = ClipRect;

	                        // RenderQueue->AddEntry(RenderQueue, DoTiledRenderWork, Work);
				        }
				    }

					//    RenderQueue->CompleteAllWork(RenderQueue);

				    for(int WorkIndex = 0; WorkIndex < WorkCount; ++WorkIndex)
				    {
				        tile_render_work *Work = WorkArray + WorkIndex;
				        DoTiledRenderWork(Work);
				    }
				}

58:05
when we are rendering, we want all of our hourse power going into rendering, becuz we have to get that out in 
a certain amount of time. But when we are not rendering, we have the luxury of having our threads doing other things. 

Casey plans to have two work queues. one for background stuff that doesnt have to finish in a fast time, the other for 
computation stuff that has to be done right away.




1:00:39
someone in the Q/A Asking about volatile and memory barrier again.

Casey explaning it in a more detailed way.

Consider code in C++, x64 and processor. 

assume in C++, we have 


				C++ code:

				entry->data = data;
				++entryCount;

for us we want a strict ordering of these two lines. line 1 happens before line 2

when c++ gets compiled into x64 assembly code.
Nevertheless, for the compiler, these two lines seems like completly independent code. 
there are no dependencies between the two.

so it may very make it 

				x64 assembly code:

				add entrycount, 1
				mov entryData, data


so just by the act of compiling, our code broke 
so we put a compiler fence, which is what volatile does. 


then when the assembly code gest to the processor, the problem we see is out of order execution. 
The processor has a bunch of factors that may change the order in which things occur.

so that is where we insert the memory fence instruction

				mov entryData, data

				Memory Fense instruction: __mm_fense / __mm_sfence / __mm_lfense 
				(full fense, store fence, load fence)

				add entrycount, 1
				


1:08:59
Casey says apparently the work_queue we wrote this episode is a lock free queue.
Casey will make it into a circular buffer next episode.



1:09:34
what is the cycle cost to spawn a thread, and what is the minimal amount of cycles to work in two threads to gain speed?

in our case, we have gotten rid of the cost to spawn threads.
spawning threads is actually very expensive, at the scales that we work at. So what we want to do is that we dont ever 
want to spawn a thread. What we want to have is a bunch threads already spawned, ready to do work. 

and if there is a lot of work to be done, the threads should never go to sleep.

that is actually why, Casey wrote the code at the beginning of WinMain();

although we initailly started this with test code, we will atually spawn 8 threds at startup, and never spawn again. 

				win32_handmade.cpp

				int CALLBACK WinMain(...)
				{
				    win32_state Win32State = {};

				    win32_thread_info ThreadInfo[8];

				    uint32 InitialCount = 0;
				    uint32 ThreadCount = ArrayCount(ThreadInfo);
				    HANDLE SemaphoreHandle = CreateSemaphoreEx(0,
				                                               InitialCount,
				                                               ThreadCount,
				                                               0, 0, SEMAPHORE_ALL_ACCESS);
				    for(uint32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
				    {
				        win32_thread_info *Info = ThreadInfo + ThreadIndex;
				        Info->SemaphoreHandle = SemaphoreHandle;
				        Info->LogicalThreadIndex = ThreadIndex;
				        
				        DWORD ThreadID;
				        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Info, 0, &ThreadID);
				        CloseHandle(ThreadHandle);
				    }



1:14:32 
someone in the Q/A asks, does volatile clear the assembly registers, by pushing them into the stack, 
and restore by popping?  
(assembly regisers are just the register you see all the time: eax, ebx, ecx, .....)

Yes. That is exactly what it does.
Although its usually not on the stack, usually volatile is in global memory somewhere. so its on the heap. 


1:17:06
much like memory managment, creating a thread is just like allocating memory, is very expensive at the operating system 
level. Its expensive in terms of game ticks.


1:18:34
someone asked, is it possible for work queue entry to spawn another work queue entry?
not yet. currently we are in the single producer, multiple consumer architecture.
To support entry spawning another entry, we need multiple producer, multiple architecture.
this queue system is more complicated and Casey doesnt plan to do it unless we have to do it.

