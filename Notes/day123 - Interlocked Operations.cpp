Handmade Hero Day 123 - Interlocked Operations

Summary:

introduced the concept of concurrency

talked about two x64 concurrency functions
locked exchange and interlocked compare exchange

did a small demo of the things we need to look out for in multithreading.
especially mentioned that if there is a variable being read by multiple threads. We need to 
let the compiler know to prevent it from optimizing it out.

in Q/A, mentioned that Mutexes is just Operating System Primitives that are built off of interlocked primitives in the x64 processor.

compared single producer, single consumer approach vs single producer, multiple consumer approach

Keyword:
Concurrency, multithreading, synchronization, mutexes

3:36
Processes / Thread are not the same thing.

Processes have different memory space. For two processes to share data, they have to do special things,
such as setting up a special pipe in the OS betwen them, or ask specifically ask the OS the share specific memory ranges. 


Threads inside a process all share the memory space.
so threads can all see the same data.


9:04
Case talking about order dependency in multithreading programming.
introduces the concept of concurrency in multithreading programming


17:30
Casey wrote an example of concurrency issue 

thread 1
				if (ptr)
				{
					work = ptr;
					ptr = 0;
					render(ptr);
				}


thread 2
				if (ptr)
				{
					work = ptr;
					ptr = 0;
					render(ptr);
				}



18:28
C actually does not really give you anyway to create a primitive that is thread safe




19:08
so for multithreading purporses, x64 provides a few instructions that helps with concurrency

for example, one of the example is 
				
				locked increment



or another example:

				locked exchange

which replaces a location in memory with a new value. And I would like to ensure that no one else will be performing
the same operation at the same time

this operation will exactly solve the race condition we have at 17:30. Essentially we are replacing the value in memory pointed by ptr with 0.
and we want to make sure we are in critical section when we do that. 

22:25
so to modify the code above, we would have 

				work = interlocked exchange(ptr, 0);
				if (work)
				{
					do(work);
				}

24:22
one of the more power functions is called 
				
				interlocked compare exchange

the interlocked compare exchange pretty much does what interlocked exchange does, but it allows you to do it predicated (dont know what that means);.
in most circumstances, where you dont super care about the absolute maximum performance of the interlocked point, 

(Note the interlocked exchange are usually on the order of 100 cycles, which is pretty fast)

interloced compare exchange allows you to pretty much write any old crappy, janky code that you want, and still work. 


the interlocked compare exchange is very similar to interlocked exchange. It takes a value that you are expecting to see in pointer. If that is not the value 
in the pointer, it does not do the exchange.  



31:00
for reads and writes in multithreading. 
Reads for a multithreading context is fine. 


32:03
the processor actually thinks about things in terms of cache lines

The cache, as it turns out, is the primary place where synchronization happens in the processor.

meaning when the processors cores think about coordinating things among themselves, they are really think about, who owns which cache lines. 


depending on the processor, the following situation may happen.  

assume you have the following date in memory 

		a0 b0
	 
then core 1 and core 2 loads a0 b0 into their cache lines .

core 1 makes the change a1 b0
core 2 makes the change a0 b1 

then when they commit back to memory, one will just overwrite the other.

Casey claims on x64, from his recollection, this is not something you have to care about. it guarantees writes are always visible to other processors. 
(Casey claims that he could be wrong);


37:15
regardless of Casey behing right or wrong about x64, even if you are on such an architecture where you dont have to worry about the correctness of synchronization writes,
you should still consider that whenever two processes contends for a piece of memory, they still have to synchronize somewhere. So synchronization is not free. 



41:06
Casey proceeds to write some multithreading code for demo purposes



53:32
Casey highlighted one line that needs to be in critical section.
not only that two threads could see the same value 
the compiler doesnt know that multiple threads could write this value. 
if the compiler is not made aware of the fact that a shared value could be changed by other threads, it may just optimize it out. 

so we need to use a c keyword to make the compiler be aware of it. 




1:02:39
Casey offering a graphical explanation of why we are seeing race conditions in the ThreadProc code 

				win32_handmade.cpp

				DWORD WINAPI
				ThreadProc(LPVOID lpParameter)
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

				//    return(0);
				}


assuming we have two cores, each running a thread, and we have NextEntryToDo, out in memory somewhere
both cores first loads the NextEntryToDo into their register  

                core 0              core 1                           memory 
                                                              
                register = 1        register = 1  <-------------     NextEntryToDo = 1

then it increments it, and writes back to memory 

	            register = 2        register = 2  -------------->    NextEntryToDo = 2

which is obviously wrong ..



1:05:30
What are mutexes in multithreading?

Mutexes is just Operating System Primitives that are built off of interlocked primitives in the x64 processor.

in other words, built off of interlocked instructions on the x64 processor




1:09:35
Someone in the Q/A asks, which is better? a job scheduler where each thread can get a job, or separate queues for each thread

Bascially it depends on the work load.

Take the rendering as an example. We may say that why have a queue at all? why not just have each thread do one row of work?
that may be a perfect valid stratgy. 

Casey says he does want to show the work queue version just for educational purposes, and it is more general purpose


but even with the queue-free, each row per thread case, there is a catch, becuz each pixel may take a different amount of time. 


This essentially leads to the single producer, single consumer vs the single producer, multiple consumer multithreading approach.

Single producer, single consumer 
                            ________
    A B C D E F G   --->      C B A   --->  thread 1
    H I J K L M N           ________
    O P Q R S T U           ________
    V W X Y Z 1 2   --->      D E F   --->  thread 2
    3 4 5 6 7 8 9           ________
                            ________
                              G H I   --->  thread 3
                            ________



single producer, multiple consumer 

                           _____________________
    A B C D E F G   --->         G F E D C B A     --->   thread 1  
    H I J K L M N          _____________________   --->   thread 2
    O P Q R S T U                                  --->   thread 3 
    V W X Y Z 1 2
    3 4 5 6 7 8 9



the reason why the single producer, multiple producer is often preferred is becuz 

we want to distribute the work evenly. 

Imagine A takes 10 ms, and every other taks is 1 ms, 
then in the single/singel case, thread1 will get tasks A B C, totaling 10 ms + 2 ms = 12 ms, 

while thread2 and thread3 will only have 3 ms worth of work. 



So when you know that you dont have too many work units, lets say around 100 work units, its usually 
not worth the extra time savings from doing locked increments in the single producer, single consumer case. 
(we dont have to do critical section in the single/single case, as there is only one queue for each thread)


so when you know that you have a small number of work units, its almost always better to go with the single producer, multiple consumer 
approach. Mainly cuz you get a more even spreading of work





1:17:52
the network card cant produce packets fast enough for an x64 core to choke on. an x64 processor can read much faster than that.

almost always in network packet receipt case, its gonna be the inbound link thats the slow part, not you dequeing it.

However, that is not true for the code of handling incoming network packets. that may be slower than the inbound link. At that point 
you need to make sure you handle them on multiple threads. 




1:19:42
what are the advantages of having a queue of jobs, instead of creating a thread every time you add a job?

we are not queueing for overlap, we are queuing for performance. 
There are totaly two different reasons why you want to thread 

one is that you are trying to overlap work. 


Overlap:
The goal here is not performance, the goal is simply to have 30 things happen simultaenously, and not have one stall completely
while they happen. For example, loading some asset in the background. We dont care how fast that is, we just want it to happen in the background.


Performance:
this means we are trying get as many cycles out of this processor as possible.
so if this computer has 16 hypercores, and we create 30 threads, then we are forcing the operating system to do our job queue for us.
all 30 threads will go into the OS scheduler, as if they were a job queue. 
The OS will have to save regsiters, do interrupts, switching around and so on.

if we were to create 30 works units on our queue, all we need to dequeue them is interlocked exchange. We know we can totaly beat this crazy work
that the OS is doing for the scheduling our threads. 

