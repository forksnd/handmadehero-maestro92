Handmade Hero Day 122 - Introduction to Multithreading

Summary:
talked about what process and threads are.

talked about how hypthreads work (again);

talked about what logical processors are. 

talked about how can "out of order processor" and "in order processor" benefit from hyperthreads

talked about what Preemptive multitasking OS and Cooperative multitasking OS are,
compared the two and mentioned the pros and cons of each. 

showed that the reason why we dont just write single threaded program is becuz we are not 
taking advantage of all the processing power if we dont go multithreading

mentioned that heat is the reason why the hardware guys invented SIMD and multithreading

Casey showing us how to use the CreateThread api in windows

Keyword:
Multithreading, process, threads, core, hyperthreads, OS




3:09
Casey introducing some of the terminologies mentioned in the Operating system land

Process
Thread
Fiber

Generally what these words refer to is the degree of logical separation that an operation system does
between lines of execution


so in handmade hero we have code which looks like assembly code 
and there is this notion of where in the code we are, which is tracked by the instruction pointer

I believe on x86 is called eip?

                Code 

     IP  --->   asm inst
                asm inst
                asm inst

and we also have registers, TLB, address table and stuff.

The idea is that there is this notion of CPU state.
There are a bunch of information that the CPU needs to know in order for him to execute an assembly language instruction.

so on our computer we are always running multiple applications. And each application has its own virtual address space. 
that is what process is.

The process is the state of a memory layout. The memory layout is controlled on the process level. 
when you are running this particular application, your process knows about the state of its memory, TLB and address space.
The memory, The TLB and the address space is per process.

The registers, the assembly instructions is not per process, it is actually done at a level below process (namely threads);

A process may want to have multiple simultaenous code execution states, which is what we call threads.
In a process you have multiple threads. 

A thread knows what instruction it is executing, what the register values it should have.
A thread has the a state of the registers in the processor.



Process keeps track of the memory map, and it has a collection of threads
Each threads stores the state of registers 


Handmade hero is a process, inside this process there are many threads that runs. 
Right there we have only written one of those threads. The main execution thread. There are other threads that are running
that the operating system starts that we have no control over. For example Direct Sounds starts some threads. The fact that 
we are using Direct Sounds at all starts some threads in our process space. 


In addition to process and threads, we have also have Fiber, which we wont really talk about, becuz they are not super necessary.


9:39
so why do we care about process and threads at all? Why cant we just write everything in one thread?

The reason is becuz, as we have shown before, you can see the number of logical processors. most modern machines 
have 4 cores, each with 2 hyperthreads, giving you 8 logical processors.  

Notice that if you open the taks manager and open the "Performance" page, it will list the Number of logical processors on your machine

so each one of these logical processor is capable of running a thread simultaenously. 
So for our handmaro game, if Casey_s computer has 16 logical processors on the CPU, we have only written work for one. 


11:52
we want to increase the number of threads we have available to the processor to run. So that it can fill up logical processors,
with those threads and do more work. 


[lets say our computer only has one core, we can create multiple threads. 
  however, only one thread will be running on that core at any given time.
  what that means is that the OS will be constantly switching threads in and out for the OS to run.
  so when the OS switch threads, it is effectively saving all the thread0_s register state and memory to somewhere in memory, 
  then loading in thread1_s register state and memory]



12:14
there is this concept of Core and hyperthreads

a core is a set of units that can do work. It has 
-   instruction decoding
-   arithmatic logic units
-   memory units 

and a core can do multiplies, looking up addresses and so on. 



In the past you used to have one thread to per-logical core.

if you have a processor that has two cores. The OS will then assign one thread to each core. And they can run simultaenously.

               ___________  ___________ 
              |           ||           |
              |   core A  ||  core B   |
              |           ||           |
              |           ||           |       
              |___________||___________|

However if a core is a hyperthreading core, Then the OS assigns more than threads to the core. Depends on the hyperthreading count.
Sometimes a core can handle two hyperthreads, some handle 4 hyperthreads. It pretty much depends on how many slots the core has to store registers 
information 

               ___________  ___________ 
              |           ||           |
              |  hyper    ||  hyper    |  thread C
              |  core A   ||  core B   |  thread B 
              |           ||           |       
              |___________||___________|

Taking just core B. On any given clock cycle, it looks at both threads C and B, and sees if either of these two threads has instructions 
that can go into ports that I have available. 

You can think of it that it was free for the core to switch between these threads. 
This is much more efficient than the old style where the OS has to swtich between threads. 




18:28
there is typically more threads running then the logical processing units that you have 

In Caseys machine, it has 16 slots, but the number of threads running are probably in the 100s or 1000s 


If you look at the task manager in the processes page, there are more processes running than the logical processers.
Each of the processes in the list has threads. 

so the real picture for the OS is that we have a giant list of threads, but it only has 16 logical processors. (on Caseys machine);


    thread 0
    thread 1
    thread 2                   
    thread 3                   16 slots
    
    ...
    ...

    thread N-1
    thread N


There is difference between physical cores and logical cores.
with hyperthreading, each of hyperthread is consiered a logical core. 
Recall On wikipedia, it is said that 

                "For each processor core that is physcially present, the Operating system addresses two 
                virtual (logical) cores and shares the workload between them when possible."

                "Architecturally, a processor with Hyperthreading consists of two logical processors per core,
                each of which has its own processor architectural state.

                Unlike a traditional dual-processor configuraiton that uses two separate physical processors, the logical
                processors in a hyper-threaded core share the execution resources.
                "

I also wrote this in day 121

having a second logical core from a physical core is not as good as having a separate physical core becuz having two 
logical cores in one physical core still has to share the units (ALU, memory units);, L1 Cache. Those are shared among 
logical cores. 

with two logical cores from a hyperthreading core, you can still run two threads, but just not as fast as if you have two 
physical cores. But the hope is you do run somewhere that fast. Like 1.4 times vs 2 times, or maybe 1.8 times. Thats the goal.
It is relative cheap to add hyperthreads to a core, as oppose to a completely new physical core.


so you can imagine in Intel, where they would look at how under utlized the units(ALU, FPU); are, and they would see
how many hyperthreads does it take to fully unitlize the units(ALU, FPU..);. If it takes two hyperthreads to have a high 
utlizied rate, then thats a good number. 



22:32
what you typically see between "out of order processors" vs "in order processors" is that "out of order processors" has that 
big window of operations that it can grab instructions from. What that means is that the degree it needs hyperthreading is 
drastically reduced, becuz it is able to use things from the same thread much more efficiently than an "in order processor", 
"in order processors" stalls a lot more on the same code. So "in order processors" tend to benefit a lot more from hyperthreads,
so you might see 4 hyperthreads on an "in order processor" being common, while seeing 2 hyperthreads on an "out of order processor" 
being the norm. 



23:28
What is the OS role in this topic?
There is this thing called the Interrupt. (ECE 391 stuff...)

one of the interrupt handler OS installed might be the scheduling timer in this case. 
it tells the processors, every millisecond or whatever, stop whatever you were running, 
run my interrupt handler code.

The scheduler interrupt handler will see whether it wants to change which thread is running on this core. 


For example, 
one of the scheduler interrupt handler code might say 
SAVE all register information to memory somewhere,
LOAD in someone elses state stored somewhere in memory, and run that code 


This is called "Preemption". Operating Systems that work this way are preemtpive multitasking operating systems 

A preemtpive multitasking operating system is one that Preempts the execution of a thread, without that thread 
giving the OS permission. 

(preempt means 1. prevent, 
               2. do something in advanced)

this is how modern preemptive operating system works. 



There are other styles, such as cooperative multitasking OS. Those OS are ones that do not have the interrupt.
Every thread, when written, has to call the yield(); function. this yield(); function will call the OS 
scheduler code. 
The cooperative multitasking OS are actually much more efficient. becuz they never have these interrupts popping up ever, 
but they are much less reliable for the user. For example, if one the code that is running on the core is behaving badly, 
and it never calls yeild();, you can never switch whats running on that core. 

Thats why that most modern OS does the preemptive multitasking style. It no longer requires the user programmers to be good 
citizens. Also it also gets much more even load balacning, becuz it doesn matter how the user code is written. The OS, at 
predefined itervals, will interrupt and switch to someone else if necessary. Essentially it can spread the work more evenly. 



32:16
Casey ran Handmade hero with Task Manager open.
Currently handmade hero is using 3% of the processing power. 
OBS is using 11%.

So the goal for handmade hero is to use 90%. 


On Caseys machine, it has 16 logical processors. so 1/16 is = 0.625. which is 6% of the CPU usage. 

If we do not increase the number of threads we use in handmade hero, this 6% is the maxim we will ever reach in handmade hero,
if we are running in just single thread. Even if we fully max it out, with gigantic rendering work, all these work will still only be on 
one logical processor.

Recall that when we had rendering ground chunks on, our game was running dead slow. But our CPU usage is still low. 

34:51
Casey proves this by turning the DrawRectangleQuickly off, and turning DrawRectangleSlowly back on. 
And in the task manager, we are seeing exactly 6% of CPU usage. which is exactly what we calculated.



35:42
that is the whole point of multi-threading. For modern processors, if you dont do multithreading, only staying on single threaded code,
you are not taking advantage all the CPU power.

With 16 logical cores, you could potentially get 16 times the performance. Of course you wont get 16, cuz 16 logical cores,
some of them are physica, some of them or hyperthreads logical cores. But 10 x, totaly plausible. 



36:53
you may wonder why did we make processors multi-threading in the first place? Why not make the processor execute single-threaded code 
very fast?
The answer is heat. 

There was an architecture called the P6, the one after the pentium. It was initially designed to run super fast,
something like 8 Ghz. What they found is that when it got to 4 Ghz, the chip melted. Apparently, to this day,
they never solved this problem. THe heat is just flat out not solvable at the moment. 


so what they quickly found is that, they cant increase the clock rate anymore.
so they went with other approaches
1.  wide execution, SIMD 
2.  More execution, essentially what threads are for

that is why optimizations today, widely boils down to these two things. At the end of the day, this is where all the processor horse 
power goes. Getting wider, and getting more execution at the same time. 


41:14
memory, synchronizations issue tends to be the hardest part about multithreading.


43:40
Casey proceeds to show us how to create threads in windows.

it is using the CreateThread(); function in windows

https://docs.microsoft.com/en-us/windows/desktop/api/processthreadsapi/nf-processthreadsapi-createthread


47:39
in the CreateThread(); function, one of the arguments is dwCreationFlags.
on their msdn site, you can see that you can either pass in 0, CREATE_SUSPENDED or STACK_SIZE_PARAM_IS_A_RESERVATION;
if you pass in 0, the thread gets executed rigth away.
if you pass CREATE_SUSPENDED, your thread doesnt run until ResumeThread is called.



56:33
Casey steppping through what happens when we close handmade hero 

stepping through in the visual studio debugger brings us to the crt0dat.c file 
and we arrive at a function called 
                
                crt0dat.c

                void __cdecl __crtExitProcess(int status)
                {

                    ...
                    ...
                    ExitProcess(status);
                }

This ExitProcess(); call is what actually kills our other threads

from the specs on the msdn site 
https://docs.microsoft.com/en-us/windows/desktop/api/processthreadsapi/nf-processthreadsapi-exitprocess

the first item it says 
                "All of the threads in the process, except the calling thread, 
                terminate their execution without receiving a DLL_THREAD_DETACH notification."

it terminates all the threads


If the ExitProcess(); were not called by the C runtime library, then if we created a thread that is 
running in a while(true) loop, it will just kept on running.

The idea that Casey brings this up is becuz he plans to build without the c runtime library at all.
So when we get to that, we will have to call ExitProcess(); to kill the threads 



58:20
threads share all the same memory.

unfortunately, the compiler doesnt know about threading. 
for example, currently Caseys code looks like this.

and you can see in our sub thread, we are reading from GlobalRunning.

                DWORD WINAPI
                ThreadProc(LPVOID lpParameter)
                {
                    ...
                    while(GlobalRunning)
                    {
                        ...
                    }
                    ...
                }


                int CALLBACK
                WinMain(...)
                {

                    while (GlobalRunning)
                    {
                        ...
                        ...

                        GlobalRunning = false;
                    }

                }

Casey says doing this may or maynot work, depends on whether the compiler tries to reload the 
GlobalRunning value or not. Recall the optimizing compiler might think that the value of "GlobalRunning"
never changes, so I dont actually have to reload from memory everytime. Not realizing some other thread 
is actually changing it. Theres things like mark up such as volatile and stuff that we might need to use.




1:03:34
the there a reason why you would create a separate process to render graphics in our game?
the only reason why you would create a separate process in a game is if you are worried about security.



1:08:27
jonathon blow has created 10000 threads on his compiler demo. How is taht possible?
a thread is just a cpu state. so you can create as many as you want. Its only limited by the storage space that 
the OS will allow you to use for storing thread state.