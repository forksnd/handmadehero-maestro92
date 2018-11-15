Handmade Hero Day 124 - Memory Barriers and Semaphores

Summary:

talked about the problems in the sample multi-threading code.
the problems include the compiler potentially moving memory writes around cuz of the compiler optimizing variables out,
the procoessor not having strong write ordering. 

fixed those issues:
by adding the volaile keyword for the compiler, and by adding memory fenses for the processor

mentioned how bad it is to have threads being in a empty while loop when there is no work. cuz it eats up CPU Usage.

introduced the concept of semaphore 

showed us a few windows Synchronization functions, particularly WaitForSingleObjectEx(); and ReleaseSemaphore();
which puts threads to sleep and wakes threads up.

mentioned how intel processors uses MESI to solve cache line concurrency

Keyword:
threads, sempahore, memory fences, intrinsics



3:32
Casey reiterating the few things that is wrong with his small multi-threading code snippet.

Recall we have the following code 
				
				win32_handmade.cpp

				struct work_queue_entry
				{
				    char *StringToPrint;
				};

				global_variable uint32 EntryCompletionCount;
				global_variable uint32 NextEntryToDo;
				global_variable uint32 EntryCount;

				internal void PushString(char *String)
				{
				    Assert(EntryCount < ArrayCount(Entries));

				    // TODO(casey): These writes are not in order!
				    work_queue_entry *Entry = Entries + EntryCount++;
				    Entry->StringToPrint = String;
				}

				...
				...

				DWORD WINAPI ThreadProc(LPVOID lpParameter)
				{
				    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

				    for(;;)
				    {
				        if(NextEntryToDo < EntryCount)
				        {
				            // TODO(casey): This line is not interlocked, so two threads could see the same value.
				            // TODO(casey): Compiler doesn't know that multiple threads could write this value!
				            int EntryIndex = NextEntryToDo++;

				            // TODO(casey): These reads are not in order!
				            work_queue_entry *Entry = Entries + EntryIndex;

				            char Buffer[256];
				            wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
				            OutputDebugStringA(Buffer);
				        }
				    }
				}



1.	first thing is that in the PushString(); function. 
	Notice that we first run the following, which we would increment EntryCount

				work_queue_entry *Entry = Entries + EntryCount++;

	then we populate the Entry->StringToPrint value;

				internal void PushString(char *String)
				{
					...

				    work_queue_entry *Entry = Entries + EntryCount++;
				    Entry->StringToPrint = String;
				}

	
	the thing is that between these two lines, one of the Threads might see that EntityCount is increased and go on and print 
	the Entry->StringToPrint value, before the Entry->StringToPrint gets properly populated 


				internal void PushString(char *String)
				{
					...

				    work_queue_entry *Entry = Entries + EntryCount++;

	                                                                void ThreadProc()
				    									            {
				                                                        if(NextEntryToDo < EntryCount)
		                                                 		        {
		                                                 		          	...
		                                  <---------------------         	...
                                         we will be printing garbage
																	            wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
																	            OutputDebugStringA(Buffer);
		                                                 		        }
				    									            }

				    Entry->StringToPrint = String;
				}


4:16 
so you might do the following:
this is still not okay

				internal void PushString(char *String)
				{
					...

				    work_queue_entry *Entry = Entries + EntryCount;
				    Entry->StringToPrint = String;

				    ++EntryCount;
				}

Even if the processor may be enforcing memory write order,  
(here the write to Entry->StringToPrint = String comes before the write to EntryCount. EntryCount++ is essentially writing 
EntryCount + 1 to EntryCount);

the compiler are under obligation to keep writes in order. So it is very possible that it will move ++EntryCount above the 
Entry->StringToPrint = String 

order of writes on a certain order means not all processors guarantee that writes that come in a certain order, will go out a certain order.


6:15
the first thing we need to do is to tell the compiler, never to move certain writes

something like 

				internal void PushString(char *String)
				{
					...

				    work_queue_entry *Entry = Entries + EntryCount;
				    Entry->StringToPrint = String;

				    ***** CompletePastWritesBeforeFutureWrites *****

				    ++EntryCount;
				}

so we want to make this to be some kind of a macro, that on whatever platform we happen to be on, we can enforce strong write order.

Depending on the platform, so we can put in the correct "strong memory write order" keyword for the compiler.   
and if we are at a platform that does not support strong ordering of writes, we can put in "memory fences"


7:44
Casey proceeds to add the macro

#define CompletePastWritesBeforeFutureWrites _WriteBarrier

_WriteBarrier is an intrinsic
https://docs.microsoft.com/en-us/cpp/intrinsics/writebarrier?view=vs-2017

				win32_handmade.cpp

				internal void PushString(char *String)
				{
					...

				    work_queue_entry *Entry = Entries + EntryCount;
				    Entry->StringToPrint = String;

				    CompletePastWritesBeforeFutureWrites

				    ++EntryCount;
				}



what this lines does is that it prevents any writes below being moved above this line.
So ++EntryCount, cant move pass the line of _WriteBarrier


Similarly, there is a version for Reads 


#define CompletePastWritesBeforeFutureReads _ReadBarrier 

Note that _WriteBarrier and _ReadBarrier isnt actually a function call. Its just a marker for the compiler


10:02
Casey vaguely recalls on that on x64, we dont have to tell the CPU to following strong write ordering.
But if we were to do that, we would have to put in memory fence

there is also an intrinsic for that 

#define CompletePastWritesBeforeFutureWrites _WriteBarrier(); _mm_sfence()
#define CompletePastWritesBeforeFutureReads _ReadBarrier(); 

so this way, we are putting both a compiler barrier and a processor barrier.
_WriteBarrier(); is strictly telling the compiler not to re-order stores, 
_mm_sfence(); is actually inserting an actual instruction in the instruction stream that the processor will see, which says 
i dont want the processor to reorder writes around this point.



14:37
so now we have our code like this 

				win32_handmade.cpp

				internal void PushString(char *String)
				{
					...

				    work_queue_entry *Entry = Entries + EntryCount;
				    Entry->StringToPrint = String;

				    CompletePastWritesBeforeFutureWrites

				    ++EntryCount;
				}

but we still have another problem that we havent addressed yet. If we actually go ahead and fixed that problem,
it might remove the need of having the line CompletePastWritesBeforeFutureWrites;








15:05

when the compiler looks at this code:

				DWORD WINAPI ThreadProc(LPVOID lpParameter)
				{
				    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

				    for(;;)
				    {
				        if(NextEntryToDo < EntryCount)
				        {
				            // TODO(casey): This line is not interlocked, so two threads could see the same value.
				            // TODO(casey): Compiler doesn't know that multiple threads could write this value!
				            int EntryIndex = NextEntryToDo++;

				            // TODO(casey): These reads are not in order!
				            work_queue_entry *Entry = Entries + EntryIndex;

				            char Buffer[256];
				            wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
				            OutputDebugStringA(Buffer);
				        }
				    }
				}

it may look at the variable EntryCount and thinks that nobody is modifying EntryCount within the ThreadProc function.
So why do we even need to do the check for "if (NextEntryToDo < EntryCount)"

So what it may do is that it may load these EntryCount into registers, or (who knows what).
Point being, the compiler may not be compelled to keep going out to memory to load in the latest value of EntryCount, 

becuz it doesnt know that theres another thread out there that is changing EntryCount. While we are executing. 
So we need to add the volatile keyword 

				global_variable uint32 volatile NextEntryToDo;
				global_variable uint32 volatile EntryCount;

this lets the compiler to know that whatever variable you added the volatile keyword to, it maybe changed without the compiler 
implicit local knowledge. So when a compiler looks at a piece of code and thinks it can determine that a particular variable cannot 
be changed, volatile is your way of telling the compiler: NO NO, someone else in the system may change this at anytime. 

So anytime you need to use it again, I need the compiler to load it back in and double check.
essentially telling the compiler, dont optimize this shit out.




16:59
recall that we said, fixing adding volatile might remove the need for CompletePastWritesBeforeFutureWrites

				win32_handmade.cpp

				internal void PushString(char *String)
				{
					...

				    work_queue_entry *Entry = Entries + EntryCount;
				    Entry->StringToPrint = String;

				    CompletePastWritesBeforeFutureWrites

				    ++EntryCount;
				}

that is becuz, if we were to declare any variable volatile

				global_variable uint32 volatile NextEntryToDo;
				global_variable uint32 volatile EntryCount;

writes to volatile automatically has a _WriteBarrier. Casey is not sure about this.

So if we declare volatile, it might inser the _WriteBarrier for us.
In this case, if we leave the CompletePastWritesBeforeFutureWrites in there, which gives us two _WriteBarrier,
that is fine, cuz its not a function call, its not inserting any instruction. Its just a markup.




18:40
Casey tackling another problem in the ThreadProc(); function

				DWORD WINAPI ThreadProc(LPVOID lpParameter)
				{
				    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

				    for(;;)
				    {
				        if(NextEntryToDo < EntryCount)
				        {
				            int EntryIndex = NextEntryToDo++;

				            work_queue_entry *Entry = Entries + EntryIndex;

				            char Buffer[256];
				            wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
				            OutputDebugStringA(Buffer);
				        }
				    }
				}

mainly NextEntryToDo is not in critical section

Casey decides to use the InterlockedIncrement() function;
the InterlockedIncrement(); is really an intrinsic that tells the compiler to use the correct instructions 
for doing the x64 locked behaviour on the proceossor.

the reason why we want the -1 is becuz, in the msdn specs, it says the function returns "the resulting incremented value"
so its like a ++a, not a++


				DWORD WINAPI ThreadProc(LPVOID lpParameter)
				{
				    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

				    for(;;)
				    {
				        if(NextEntryToDo < EntryCount)
				        {
				            int EntryIndex = InterlockedIncrement((LONG volatile *) &NextEntryToDo) - 1;

				            work_queue_entry *Entry = Entries + EntryIndex;

				            char Buffer[256];
				            wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
				            OutputDebugStringA(Buffer);
				        }
				    }
				}

22:50
Casey also wants to point out that InterlockedIncrement(); actually serves as a fence it self. its a processor fence

if we really really want to be extra extra safe, we can do 

				DWORD WINAPI ThreadProc(LPVOID lpParameter)
				{
				    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

				    for(;;)
				    {
				        if(NextEntryToDo < EntryCount)
				        {
				            int EntryIndex = InterlockedIncrement((LONG volatile *) &NextEntryToDo) - 1;
				            CompletePastWritesBeforeFutureReads
				            work_queue_entry *Entry = Entries + EntryIndex;

				            char Buffer[256];
				            wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
				            OutputDebugStringA(Buffer);
				        }
				    }
				}



26:20
Casey begins to talk about the concept of thread finishing.

Currently in our code, we have 

				global_variable uint32 volatile NextEntryToDo;
				global_variable uint32 volatile EntryCount;
				work_queue_entry Entries[256]

and NextEntryToDo indicates how jobs have been started. But we dont actually keep track of how many jobs have finished.
That is a problem becuz, lets take our renderer. If we dont know when the renderer finished rendering, we wont know 
when to move on. 

There is a distinction between how many jobs have finished and which specific jobs have finished. For our renderer, we just 
need to knkow how many jobs have finished 

				global_variable uint32 volatile EntryCompletionCount;
				global_variable uint32 volatile NextEntryToDo;
				global_variable uint32 volatile EntryCount;
				work_queue_entry Entries[256]


so our little test demo code, we can do 

				DWORD WINAPI ThreadProc(LPVOID lpParameter)
				{
				    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

				    for(;;)
				    {
				        if(NextEntryToDo < EntryCount)
				        {
				            int EntryIndex = InterlockedIncrement((LONG volatile *) &NextEntryToDo) - 1;
				            CompletePastWritesBeforeFutureReads
				            work_queue_entry *Entry = Entries + EntryIndex;

				            char Buffer[256];
				            wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
				            OutputDebugStringA(Buffer);

				            InterlockedIncrement((LONG volatile *)&EntryCompletionCount);
				        }
				    }
				}





30:59
Casey mentioned that how do we structure this code, so that when there is not work to do, or times 
where threads are waiting for some tasks to finish before moving, we want to tell the OS to put these
threads to low power mode, so that CPU is not wasting cycles.

Casey says this part is a bit janky?
Windows is not realtime operating systems. They are not even soft realtime operating systems. 






32:00
so what we want to do here is that in ThreadProc, is essentially checking if there is work to do in the work queue

				DWORD WINAPI ThreadProc(LPVOID lpParameter)
				{
				    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

				    for(;;)
				    {
				        if(NextEntryToDo < EntryCount)
				        {
				            int EntryIndex = InterlockedIncrement((LONG volatile *) &NextEntryToDo) - 1;
				            CompletePastWritesBeforeFutureReads
				            work_queue_entry *Entry = Entries + EntryIndex;

				            char Buffer[256];
				            wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
				            OutputDebugStringA(Buffer);

				            InterlockedIncrement((LONG volatile *)&EntryCompletionCount);
				        }
				    }
				}



currently, the line 
				
				if(NextEntryToDo < EntryCount)

is essentially the check we are doing to see if there is work in the queue for us to do.

what we want to do is that in the case if there isnt work to do, we want like to be put into sleep in some way.
That way we dont waste CPU cycles.

				DWORD WINAPI ThreadProc(LPVOID lpParameter)
				{
				    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

				    for(;;)
				    {
				        if(NextEntryToDo < EntryCount)
				        {
				            int EntryIndex = InterlockedIncrement((LONG volatile *) &NextEntryToDo) - 1;
				            CompletePastWritesBeforeFutureReads
				            work_queue_entry *Entry = Entries + EntryIndex;

				            char Buffer[256];
				            wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
				            OutputDebugStringA(Buffer);

				            InterlockedIncrement((LONG volatile *)&EntryCompletionCount);
				        }
				        else
				        {
				        	PUTING us to "sleep" or "low power mode"
				        }
				    }
				}

sleep is probably the wrong word. What we want to do is to tell the OS, we are done doing work right now. Mr.OS
you can suspend us right now. We will do something later, and we will tell us when to wake us up to resume


33:38
so if you look at our current code, When we initalize the the threads, all the thread should be put to sleep, until
we call the PushString();

				int CALLBACK 
				WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
				{
				    win32_state Win32State = {};

				    win32_thread_info ThreadInfo[4];
				    for(int ThreadIndex = 0;
				        ThreadIndex < ArrayCount(ThreadInfo);
				        ++ThreadIndex)
				    {
				        win32_thread_info *Info = ThreadInfo + ThreadIndex;
				        Info->LogicalThreadIndex = ThreadIndex;
				        
				        DWORD ThreadID;
				        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Info, 0, &ThreadID);
				        CloseHandle(ThreadHandle);
				    }

				    PushString("String 0");
				    PushString("String 1");
				    PushString("String 2");
				    PushString("String 3");
				
					...
				    ...
				    ...

				}



so what we want to do is to be able to wake the threads up in the PushString(); function 

				win32_handmade.cpp
				
				internal void PushString(char* String)
				{
					...
					...

					Some way to wake up our threads.
				}


34:52
the things we will use to solve this is with Semaphores.
Windows implements these.

Semaphore is a countable wait primitive.


Casey proceeds to talk about what semaphores are.



37:06
once Casey finishes introducing semaphores, he showed us a page of Synchronization functions.
https://docs.microsoft.com/en-us/windows/desktop/sync/synchronization-functions

in particular, Casey wants to use the WaitForSingleObjectEx(); function
https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-waitforsingleobjectex

if you look at the api 

				DWORD WaitForSingleObjectEx(
				  	HANDLE hHandle,
				  	DWORD  dwMilliseconds,
				  	BOOL   bAlertable
				);

it takes a hHandle, the one that is similar to the ones we get from a sempahore, and the number of milliseconds
we want to wait for, which can be INFINITE

and finally we dont care about the bAlertable 


using it in the ThreadProc(); function.

				DWORD WINAPI ThreadProc(LPVOID lpParameter)
				{
				    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

				    for(;;)
				    {
				        if(NextEntryToDo < EntryCount)
				        {
				            int EntryIndex = InterlockedIncrement((LONG volatile *) &NextEntryToDo) - 1;
				            CompletePastWritesBeforeFutureReads
				            work_queue_entry *Entry = Entries + EntryIndex;

				            char Buffer[256];
				            wsprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
				            OutputDebugStringA(Buffer);

				            InterlockedIncrement((LONG volatile *)&EntryCompletionCount);
				        }
				        else
				        {
				        	WaitForSingleObjectEx(sempahoreHandle, INFINITE, FALSE);
				        }
				    }
				}

this tells the OS to suspend me for INFINITE seconds, until the whatever handle that I passed into in the first argument "Triggers", or 
until the handle gets "signaled"

In the context of multi-threading programming, there are a lot of things that you can Wait for, so this api is very generic. 
it can wait for semaphores, but it can also wait for lots of other types of handles in the system. 


47:39
To wake semaphores up, Casey uses the ReleaseSemaphore(); function. 

https://docs.microsoft.com/en-us/windows/desktop/api/synchapi/nf-synchapi-releasesemaphore


				internal void PushString(HANDLE SemaphoreHandle, char *String)
				{
				    Assert(EntryCount < ArrayCount(Entries));

				    work_queue_entry *Entry = Entries + EntryCount;
				    Entry->StringToPrint = String;

				    CompletePastWritesBeforeFutureWrites;
				    
				    ++EntryCount;

				    ReleaseSemaphore(SemaphoreHandle, 1, 0);
				}




54:13
Casey showing us the difference between calling the WaitForSingleObjectEx(); function in ThreadProc();
function the not calling it.

He opens up Task manager and shows that entire computers CPU Usage is spiked to 72% for the one not calling WaitForSingleObjectEx();
these threads are just spinning in a empty for(;;) loop

and with the one calling WaitForSingleObjectEx(); the entire computers CPU usage is 17%.



57:31
semaphores is a very conveninent way to manage work queues. 



1:03:46
what do you plan to maintain cache line coherency between processosors? Can physical CPUs share a cache line 

Fabien was saying that System bus locks dont even happen anymore, everything goes through MESI.
x64 processosors dont need a lock to do interlocked operations


Intel chips are designed to maintain cache line coherency between processors and cores

they way they do this is through MESI
Modified Exclusive Shared Invalid


Assuming you have a super powerful machine 

      4 CPU / 4 Core
	 __________________________________
	|      L1             L1           |
	|     _________      _________     |
	|    | c0 | c1 |	| c4 | c5 |    |
	|    |____|____|    |____|____|    |
	|    | c2 | c3 |    |    |    |    |
	|    |____|____|    |____|____|    |
	|                                  |
	|     _________      _________     |
	|    |    |    |    |    |    |    |
	|    |____|____|    |____|____|    |
	|    |    |    |    |    |c15 |    |
	|    |____|____|    |____|____|    |
	|                                  |
	|__________________________________|

so we got core 0, core 1 .... core 15

Sometimes lets say core 0 got a L1 cache, and core 4 got a L1 cache,
how doees it maintain coherency between different processors.


assume we have a variable foo at memory 0x100

so if core 0 changed value of foo, and that change is only existing in c0_L1 cache
then at the same time core 4 also wants to access foo. 

coherency is about doing this with the correct value and the correct order.


So the way intel solves this problem is through the "MESI" protocol. 

there is this thing called "Snooper".
what happens when someone goes to load a value, they will mark the flags of MESI 

for example, one might be marked:
modified = true;
exclusive = true;
shared = false;
invalid = true;

so through the flags, when c4_L1 loads foo, c4_L1 cache will know if some other cache previously
did a write which turns on the "modified" flag

For example, when c0_L1 changes foo, it notifies c4_L1 cache that its value is invalid, turning on its "invalid" flag.
Then the other processors/cores "snoopes" the new value of foo from the core 0. Hence the name.    


different situations and transitiosn turns on different flags.
so if two processors read from the same memory, it is marked shared, then when something happens, it transitions into 
another state.

Nevertheless, you dont have to worry about it on x64. x64 hardware can maintain coherency on its own, without software. 

