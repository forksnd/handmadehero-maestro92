Handmade Hero Day 010 - QueryPerformanceCounter and RDTSC


Summary:
mentions the difference between __rdtsc and QueryPerformanceCounter on windows.


Keyword:



https://software.intel.com/sites/default/files/managed/a4/60/325383-sdm-vol-2abcd.pdf

3:47
__rdtsc
a way of getting "a thing" inside the CPU that increments by one every time the CPU retires a clock cycle

Example:
if your computer has a 3.2 GHz Processor, that means 3.2 billion cycles per second. 
For each one of those cycle, this counter increments by one

the RDTSC counter lets you access the value in this counter

so if you time it at the beginning of your code,
then time it at the end of your code
you can get a pretty good idea of how many cycles it has elapsed

HOWEVER, it might not be accurate,
cuz you wont know if your OS woke up in the middle of your code, swap out your code, and ran some other code, and then swap your code back in

so you have to be aware that you are effectively timing "Everything" that the processor did

this is all dependent on the OS
some OS, actually try to save and restore the RDTSC, depending on how you access it

also, you might different results different time, depending on if something is cached this time, but not cached the 2nd time...

so it is not foulproof. but a quite useful thing that give u indication of your timing


QueryPerformanceCounter
https://msdn.microsoft.com/en-us/library/windows/desktop/ms644904(v=vs.85).aspx
this is u asking windows, to the best of your knowledge, what the worlds real time now in high resolution, 

        "Retrieves the current value of the performance counter, 
        which is a high resolution (<1us) time stamp that can be used for time-interval measurements."

resolution is actually (1<us);



[From the following link, it says 
                
        "QueryPerformanceCounter returns the number of 'ticks' since the computer was rebooted.
        QueryPerformanceFrequency returns the number of 'ticks' in a second

        to measure the duration of a frame (or anything else), you will call QueryPerformanceCounter at the beginining
        of the frame and save the value so that you can compare it to the returned at the end of a frame());
        "
https://www.gamedev.net/forums/topic/339194-can-somebody-please-explain-queryperformancecounter/
]
    

[The following link also provides very good informatin about QueryPerformanceCounter 
http://www.songho.ca/misc/timer/timer.html
]



17:25
long long is essentially a 64 integer

DWORD is 32 bit
LONG is 32 bit


51:55
__rdtsc(); is cheaper than QueryPerformanceCounter



53:40
the reason why I am casting it to int32, is becuz, printf %d is a uniform way of printing. printing 64 bit integer using a printf 
is only really recent


54:30
if you have fps * megaCyclesPerFrame, your value should roughly equal the processor cycle count listed on the specs


1:12:24
they do the printf in doubles anyway


1:22:23
doubles are slower than float, so dont use it unless u have to, usually half as fast


1:23:23
wait for the vertical v-trace



agner fog table

1:39:10
tempaltes are not worth the pain


1:41:45

float vs double being twice as fast 
MULPS (real32); - 5 cycles to complete, 1 cycle throughput
MULPD (real64); - 5 cycles to complete, 1 cycle throughput

if you execute a MULPS vs MULPD, its pretty much the same
but MULPS is twice as fast becuz these instructions are the same width.
they operate on 128-bit woth of data

128/32, means you can pack 4 real32s into one register
128/64, means you can pack 2 real64s into one register

So the idea is that if you have 3 or more math related operations
float is twice as fast as doubles

128 refers to Streaming SIMD Extensions,
SSE has this 128-bit registers 

