Handmade Hero Day 121 - Rendering in Tiles (Marathon);

Summary:

explains how the IACA knows how to analyze your block of code.
showed us the disassembly code of IACA_VC64_START and IACA_VC64_END
explains what actually happens behind the scenes when you put IACA_VC64_START and IACA_VC64_END blocks

Showcased a file that he had, blowtard.cpp, which is the output of a compiler he wrote for the xbox-360
xbox-360 is an in order process. 
Explained how to interpret the ouputs in blowtard.cpp.

Compared this output with the analysis report from the IACA. 
Explains the reason why his output for the xbox-360 is inorder, while ouputs from IACA is out of order,
which is literally a table of instructions costs.

Explained how to understand the port bottleneck and Critical Path in the IACA output

Mentions that, after a few episodes of optimizations, why he suspects that the bottleneck right now is the memory bandwidth.

Explained how hyperthreading work. 

Planning to make the renderer threaded, which will solve memory bandwidth bottleneck problem.

proceedes to make the renderer code threading ready.

discusses plans on how to split the renderer work among threads and hyperthreads

proceeds to change the RenderRectangleQuickly function to render on even or odd lines, as well as 
rendering into a clipped rectangle.

Added an integer version of rectangle, also the rectangle intersect and rectangle union functions in the math file.
these two functions are helpful utility functions. 
 
Explain how we are doing unaligned writes to memory.
Mentions that whether unaligned writes to memory affects performance depends on the processor
Tested on his computer and found out it doesnt improve performance on his computer

fixed a bug where there are weird edge motion when rendering to a clipped rectangle due not filling in the 
correct clipped rectangle region.

replaced all the calls of RenderGroupToOutput with TiledRenderGroupToOutput. This is to set up the multithreading
for next episode.

Keyword:
IACA, multithreading, rendering, memory, hyperthreading, cahce


2:54
Casey talking about what he found out about IACA


so the way we were using it is that we were putting the IACA_VC64_START and IACA_VC64_END
markers on the block of code you want to analyze.

These two #defines are defined in the iacaMarks.h


iacaMarks.h

#define IACA_VC64_START __writegsbyte(111,111);
#define IACA_VC64_END __writegsbyte(222,222);

as you can see they are calling the __writegsbyte function

from its msdn link below, we can see the description

"
                Write memory to a location specified by an offset relative to the beginning of the GS segment.
                https://msdn.microsoft.com/en-us/library/529bay7a.aspx
"

Casey describes how he thinks the IACA works. 

Casey proceeds to show us the disassembly code 


        IACA_VC64_START

mov         al,6FH            <------ h postfix means this is a hexadecimal number, this is really 6h
mov         byte ptr gs:[6FH],al

0x6F is 111


so what happened is that, the IACA inserted a thing that they believe it is very unlikely for the compiler to ever have produced
for any reason on its own. 

A write to the gs (general segment); is not something that the compiler will ever produce.

so during the execution, they just look through the executable, 
and they find the encoded instruction of __writegsbyte(111,111); and __writegsbyte(222,222);
that way they know which block of code to analyze.



So this way of putting markers 
#define IACA_VC64_START __writegsbyte(111,111);
#define IACA_VC64_END __writegsbyte(222,222);
means that any external tool can trivially sweep through and find without having to know anything about that executable.
For example, it doesnt need to know how to read Microsofts PDB file or how to read a map file. 
Essentially it doesnt need to know how to read some other markup to figure out where the code is
It can just use this trick.







13:17
Casey showing us the content of a blowtard.cpp (a random file on his computer)

blowtard.cpp is the output of a compiler that Casey wrote

it was a compiler that was designed to make blit code for x-box 360 of various kinds

The way that PowerPC works (PowerPC is a instruction set archtecture that came out 1992); is that,
you can issue 2 instructions every cycle, but you can only issue those 2 instructions every cycle if the resources
that they needed are not blocked. This is similar to the units on modern day processors. If the units are available, you can 
issue it.

unlike the processor we use nowadays, x-box 360 (which came out 2005) is an in-order procoessor. As you go through the 
instruction stream, the instructions are executed in order.


For example here:
                addi    r18,    r0,    16        // ex(  0)
                 add    r27,    r3,    r5        // ex(  1) pipe(1)
                 add    r11,   r27,    r5        // ex(  5) pipe(1) r27(3)
 

line1:
"ex(  0)" 
at the end of each line means which clock cycle this instruction is beying run at 

line2: 
"ex(  1) pipe(1)" means 
I cant execute this add on cycle0 becuz the pipeline for the add is full. there is only one add pipeline on the 360

line3: 
"ex(  5)" means we cant execute it until clock5
"pipe(1)" means it needs to wait on cycle for the add pipeline 
"r27(3)" it has to further wait for 3 more cycles for r27. Notice r27 is used in line2, then it gets immediately used in line 3



Casey mentioned that he used to be very good at optimizing for xbox 360 becuz he had to write this compiler
which required him to really understand how the processor worked.

The output of the IACA is much different from what he is used to.


23:27
Casey bringing up the IACA report again
he thought that the report was showing how the processor would execute the program

what the report shows has nothing to do with the order of its execution. This is just a straight table.
This is not a actual step by step read out like blowtard.cpp

it just a table saying an instruction costs this many instruction in this unit/port/slot 

for example (2nd instruction at 23:51);
the instruction "mulps xmm5, xmm4" costs 1 micro operations in port 0-DV


24:40
Casey claims that it makes sense for his tool to do a step by step analysis for the xbox 360 while this IACA
did just a table for this x64 processor 

becuz the xbox 360 is a in-order processor, and x64 is a highly out of order processor. It has a giant micro ops window,
(someone posted on the stream that the window is 196, not sure if this is correct)

notice that our total number of micro ops is 284, which means it is possible that half of our 284 micro-ops may be able to 
fit inside our 196 micro opts window, as long as the micro-opts do not serially depend on each other. So showing a step-by-step
thing is kind of impossible.

so instead, they give information such as, what port will be most heavily utilized, 

at 26:13 casey highlights it that in our program, it is port 1, which is doing 97 cycles.

this means our program, even nothing is serially depending, will take at least, at a minimum. 97 cycles. becuz it will have 
at least 97 cycles going to port1. 


27:20
explaing what CP, Critical path, in this IACA report mean
Casey says he think it is marking the instructions that has to go to the high pressure port (high pressure port is port1 in our case);

essentially all the instructiosn that is going to port1, that is a critical path

so if our bottle neck was port2, all the instructions going to port2 will have a CP mark.





31:10
Casey says at this point it seems like our bottleneck is a memory throughput problems (looking up texels and writing to pixels);
rather then our block throughput

The reason why he says that is becuz, our current block throughput is 97 cycles. 
since we are doing 4 pixels, 97 / 4 = 24.25.

so if we should be doing only 24.25 cycles, but our measured throughput is 40 cycles, that means we are probably getting some 
stalls in memory.


32:24
one thing that the analyzer does not take into account is the loop. And we do have one for loop for the texture feturing.

since this is a very small loop, Casey will manually unroll this loop and see the cycles count reported in the IACA.



33:42
for some reason, after unrolling the loop, our cycles have gone up....

Casey proceeded to re-organize the code a bit, and it got faster.... literally just reorganizing it.


40:17 
Casey proceeds to make the texture fetching part SIMD as well.

we got it down to 37 cycles/hit



42:48
now that we have unrolled the loop and made it SIMD, the IACA reports 103.00 cycles.
so this is the actual accurate cycle count, accounting for all the instructions in the block we are measuring.


47:50
Casey pulled out the the diagram for the Nehalem processor
https://commons.wikimedia.org/wiki/File:Intel_Nehalem_arch.svg

He mentioned that now with this diagram, we know which port an instruction will go to. Something that the 
Intel Intrinsics guide wont tell us. We can then use this diagram with the IACA, to see if we are doing 
some instruction on the heavy loaded port.



1:16:56
Casey again mentioning that he thinks we have a memory problem. 
He mentiones that it might be a good time to switch to multi-threading for the renderer.


Hyperthreading
the idea behind hyperthreading is very simple

Imageine we have two units

               ___________
              |           |
              |           |
              | FP ADD    |
              |           |              
              |___________|

               ___________
              |           |
              |           |
              |  Load     |
              |           |              
              |___________|

Ideally, if we issue instructions to these two units, it can finish in lets say 2 cycles.
But however, for us to issue instructions to these two units, we have to give certain inputs to these units.
there will be cases where we are waiting on for the inputs for these units, lets say we are fetching stuff from memory, 
and we get a massive stall, then we got our units sitting around doing nothing.

we call these wasted cycles on units "Latency Bubbles". They are basically bubles of time that formed becuz we are waiting for something
to put in our units.

What Hyperthreading is is that we have two streams of code, and we run both loop 0 and loop 1 at the same time, on the same processor,
on the same core 
                
                 loop 0                    loop 1
               ___________                ___________ 
              |           |              |           |
              |           |              |           |
              |           |              |           |
              |           |              |           |       
              |___________|              |___________|

This is still entirelly singlethreaded, meaning there is only one thing executing at any given time. 
But what can happen is that we can have loop 0 hitting a latency bubble, where it hits a point where loop 0 is completely
starved for memory, I cant get what I actually need to get to issue any more instructions, and the units on the processor will just 
go fallow (in farmland terminology, fallow means not cultivated for crops);

what procoessor then does is that it tasks switches to loop 1 in a very very fast way. it is swtiching in a very special way, 
where it doesnt have to register save and load from memory. Thats why they are a "Hyper" thread, its like a 
thread switches that does not go through the operating system. It doesnt have to ask the Operating system to switch.

its a way to give the processor more potential things to do.

on wikipedia it says 

                "For each processor core that is physcially present, the Operating system addresses two 
                virtual (logical) cores and shares the workload between them when possible."

                "Architecturally, a processor with Hyperthreading consists of two logical processors per core,
                each of which has its own processor architectural state.

                Unlike a traditional dual-processor configuraiton that uses two separate physical processors, the logical
                processors in a hyper-threaded core share the execution resources.
                "


so with hyper-threading, it saves us the trouble of having to optimizing for memory, since hyper-threading 
can solve the problem of units not being able to do work due to memory stalling.

Hyperthreading is the technology that makes it possible for every core in an i5 or i7 multicore processor to handle 
two separate process threads simultaneously.

https://techspirited.com/difference-between-cores-threads

1:26:09
Casey will begin making the rendered threaded. 
While he wont introduce threading this episode. He will prepare the renderer to be threaded. 

take our example, which we have a screen, and we want to render stuff.
usually our rendering works tends to be in rectangular sprites, instead of long strips 
               _________________________________
              |                         _       |
              |                        | |      |
              |                        | |      |
              |                        | |      |
              |            _______     | |      |
              |           |       |    | |      |
              |           |_______|    | |      |
              |                        | |      |
              |                        | |      |
              |                        |_|      |
              |                                 |
              |_________________________________|




when we want to split the work threading purposes, we dont want to split it in a way that puts lots of pressure 
on the cache. 

We have three fundamental pieces of memory that we are working with, which we want them to be in the cache as often as possible

0.  command buffer. thats our render group stuff
    recall that every frame, we have a huge list of render commands.
    Casey suspects that this is not as important as the other two. Becuz this is not accessed as frequently as the other two.
    The number of commands we are dealing vs the number of pixels we are touching is minimal.

1.  The FrameBuffer, the memory we are writing to, and reading from. this is a R/W thing 
2.  Textures, Read only

every once in a while, the frameBuffer might be something we texture from, so we do have in some sense a read write texture,
but we will just treat them as framebuffers, such as our ground chunks, where we write to the ground chunks first,
but we read from the ground chunks later.

so the idea is that while we want to do multithreading on all these three things, we dont want to mess up cache badly.


1:30:07
so the goal is to minimize the degree to which we load or read/write from 1 and 2 that are spatially incoherent.

For eaxmple, 
lets say we have to three steps, in which we render frameBuffer region1 and region2
we dont want to fill the cache of a region1 in step 1, then in step 2 complete wipe out the cahce with region2, 
and then in step 3 go back to region1


1:31:20
Casey decides to tackle threading for framebuffer first cuz he thinks its easier than solving threading for textures

for exmaple, we can just split the frameBuffer in half, and have two threads render left and right.

               framebuffer
               _________________________________
              |                |                |
              |                |                |
              |                |                |
              |     thread 1   |   thread 2     |
              |                |                |
              |                |                |
              |                |                |
              |                |                |
              |                |                |
              |                |                |
              |                |                |
              |________________|________________|



Casey says he would like to work in chunks of the framebuffer, that are sized appropriatedly with the cache.


1:33:06
Casey looking up the cache size on his computer.
He has a W5580 processor. The specs are 
L1 64 KB per core 
L2 256 KB per core 
L3 4 MB to 24 MB shared

in terms of how big of a square that is 
256 * 1024 is the 256 KB, 4 is cuz 4 bytes per pixel

sqrt(256 * 1024 / 4) = 256;

So basically the size for the L2 cache is a 256 x 256 square. But obviously we need space for other stuff,
such as textures and stuff.


Casey proceeds to estimate what should our chunks size be that works well with our cache.




1:37:23
so the idea is to have the renderer easily break the framebuffer into chunks, then multithreading wise,
assign different chunks to different cores

For hyperthreads, we will treat them specially, becuz Casey wants to maximize the likelihood that the textures 
hit the cache as well.

notice the L1 cache is shared among hyperthreads within a core, 
the L3 cache is shared across cores.

               framebuffer
               _________________________________
              |     |     |    |                |
              |     |     |    |                |
              |_____|_____|    |                |
              |                |                |
              |                |                |
              |                |                |
              |                |                |
              |                |                |
              |                |                |
              |                |                |
              |                |                |
              |________________|________________|

So for cahce locality for the hyperthreading, my cache space is very small. So the idea is that 
Casey will split the framebuffer chunk in a way that the pixels getting hit by the hyperthreads are very close to 
each other in terms of what they need. 

for example, if a texture is being filled at in the cunk below,
 it is very like that one hyperthread is filling pixels starting at the 1 s, while the other hyperthread is filling pixels
at the 2 s

                 ___________
    1 --------->|111111     |
    2 --------->|222222     |
                |           |            
                |           |
                |___________|

so for the hyperthreads, we might not split the pixels into rectangular chunks, but maybe interleaved scanned lines. 



1:39:55
Casey would like to setup for the renderer the ability to send a dispatch of doing the following.

render this subrect
Even lines
odd lines 


1:41:00
Casey proceeds to make the DrawRectangleQuickly function draw on even or odd lines.



1:55:17
Casey made the DrawRectangleQuickly render only a clipped small subrect. The idea is just to setting xMin, xMax, yMin, yMax.
Pretty straightforward.

Just for testing Casey hard coded some numbers here


                void DrawRectangleQuickly()
                {

                    ...
                    ...

                    int32 ClipXMin = 128;
                    int32 ClipYMin = 128;
                    int32 ClipXMax = 256;
                    int32 ClipYMax = 256;                   

                    if(XMin < ClipXMin) {XMin = ClipXMin;}
                    if(YMin < ClipYMin) {YMin = ClipYMin;}
                    if(XMax > ClipXMax) {XMax = ClipXMax;}
                    if(YMax > ClipYMax) {YMax = ClipYMax;}    

                    for(int Y = MinY; Y < MaxY; Y += 2)
                    {

                        for(int XI = MinX; XI < MaxX; XI += 4)
                        {
                            ...
                            ...
                        }
                        ...
                        ...
                    }
                }





1:58:01
for some reason, we are seeing some weird edge motion. 

1:59:15
Casey found out the problem.
recall since we are writing 4 pixels at a time. But we are not writing every 4 pixels relative to where the clip
buffer starts, but where the texture starts. So we will over render a few pixels. 




                for(int Y = MinY; Y < MaxY; Y += 2)
                {
                    ...
                    ...
                    __m128i ClipMask = StartupClipMask;

                    uint32 *Pixel = (uint32 *)Row;
                    for(int XI = MinX; XI < MaxX; XI += 4)
                    {
                        ...
                        ...
                    }

                    ...
                    ...
                }






2:10:14
Casey changed the function to pass the clipping information into the DrawRectangleQuickly(); function

for that, he defined a integer version of rectangle, and he wrote a intersect fnction that replaces the min max 
clamping we were doing 


                handmade_math.h

                struct rectangle2i
                {
                    int32 MinX, MinY;
                    int32 MaxX, MaxY;
                };


                inline rectangle2i
                Intersect(rectangle2i A, rectangle2i B)
                {
                    rectangle2i Result;
                    
                    Result.MinX = (A.MinX < B.MinX) ? B.MinX : A.MinX;
                    Result.MinY = (A.MinY < B.MinY) ? B.MinY : A.MinY;
                    Result.MaxX = (A.MaxX > B.MaxX) ? B.MaxX : A.MaxX;
                    Result.MaxY = (A.MaxY > B.MaxY) ? B.MaxY : A.MaxY;    

                    return(Result);
                }



2:13:51
Casey says if we are talking about rectangels, there are two fundamental operations we might want to do. 

1st is the enclosing rectangle, which he calls it the union. The combinatino of the two maximually.

                 ______________________ ..........
                |                      |         .  <---- the enclosing rectangle
                |                      |         .
                |            __________|_________            
                |       A   |          |         |
                |           |          |         |
                |           |          |         |
                |___________|__________|         |
                .           |               B    |
                .           |                    |
                .           |                    |
                ............|____________________|
                            



2nd is the overlap, which is the intersection.
                 ______________________ 
                |                      |
                |                      |
                |            __________|_________            
                |       A   |          |         |
                |           | inter-   |         |
                |           | section  |         |
                |___________|__________|         |
                            |               B    |
                            |                    |
                            |                    |
                            |____________________|
                            

you can see that these two operations are pretty much either taking the min or the max of each component.


hence he added the union function

                handmade_math.h

                inline rectangle2i
                Union(rectangle2i A, rectangle2i B)
                {
                    rectangle2i Result;
                    
                    Result.MinX = (A.MinX < B.MinX) ? A.MinX : B.MinX;
                    Result.MinY = (A.MinY < B.MinY) ? A.MinY : B.MinY;
                    Result.MaxX = (A.MaxX > B.MaxX) ? A.MaxX : B.MaxX;
                    Result.MaxY = (A.MaxY > B.MaxY) ? A.MaxY : B.MaxY;

                    return(Result);
                }






2:18:35
now the clipping code in the DrawRectangleQuickly(); is changed to 


                void DrawRectangleQuickly()
                {

                    ...
                    ...

                    rectangle2i ClipRect = {128, 128, 256, 256};
                    rectangle2i FillRect = {Xmin, Ymin, Xmax, YMax};

                    FillRect = Intersect(ClipRect, FillRect);

                    ...
                    ...

                }




2:51:17
Casey starting to discuss whether unaligned writes affects our performance.

currently in our registers we have our pixel values 

                128 bit wide registers                

                 ____________________________________________________________________________________
                |     px 3            |     px 2           |     px 1           |    px 0            |
                |_____________________|____________________|____________________|____________________|

        

and we are writing these out to memory. but un-aligned


so if you assume memory is below, starting from byte 0  (obviously in byte 0 is null, but assume its like this)

              byte 0                                            byte 16

                                                                 | 
                                                                 |
                                                                 v
                 ___________________________________________________________________
                | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 |    .....   | 8 | 8 | 8 | ...    
                |___|___|___|___|___|___|___|___|___|____________|___|___|___|______
                                            
                                              ^
                                              |
                                              |

                                          pointer 

and let say we have a pointer value of 7, and we write 128 bits of pixel data 


              byte 0                                            byte 16

                                                                 | 
                                                                 |
                                                                 v
                 ___________________________________________________________________
                | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 |    .....   | 8 | 8 | 8 | ...    
                |___|___|___|___|___|___|___|___|___|____________|___|___|___|______
                                            
                                              ^   x   x  x   x  x  x  x   x   x     ^
                                              |            16 bytes                 |
                                              |                                     |

                                          pointer                                  end 


notice our write is un-aligned, meaning some of our pixel data writes occur in the byte 0 ~ byte 16 bucket,
then the second half happens  in the 2nd bucket.


so anytime we write a 128 bit write, there are only two ways it can happen. 
so we are either perfectly only in one bucket. 
or we straddle two buckets.

depending on the proceossrs, like these more or less.
meaning sometimes the processor doesnt care if you writes these unaligned.
so on some processors it is extremely important
on others it might not be.

so the we have to write tests to do this 

2:56:20
the first thing we can do is to always align Xmin and Xmax, so we are always aligned.

-   Casey is using this uintptr so that it changes Row into an integer that he can manipulate, but maintain the size 
that it was initially. So on a 64 bit build, it will be 64 bit. when its on a 32 bit build, it will be 32 bit.

-   we are looking for 16 byte alignment. 16 - 1 = 011111. Masking with (16 - 1); tells you waht are the bottom bits
this will tell us however far off we are from being in alignment

Obviously this is just a test to see if moving to alignment helps or not. We wont even render the right things.

                handmade_render_group.cpp

                internal void
                DrawRectangleQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                                     loaded_bitmap *Texture, real32 PixelsToMeters)
                {

                    ...
                    ...

                    uint8 *Row = ((uint8 *)Buffer->Memory +
                                  FillRect.MinX*BITMAP_BYTES_PER_PIXEL +
                                  FillRect.MinY*Buffer->Pitch);
                    ...

                    int32 Align = (uintptr)Row & (16 - 1);
                    
                    Row -= Align;

                    ...
                    ...
                }



3:04:16
and through our test, it doesnt seem like our speed really improved. almost the same.
so its possible that the unaligness of the store and the load based on the amount of other work we have to do is just not relevant.





3:05:23
Casey starting to address the problem of always filling the real clipped region.
This is to solve the weird edge motion 


recall in our DrawRectangleQuickly(); code we have 

                void DrawRectangleQuickly()
                {

                    ...
                    ...

                    for(int Y = MinY; Y < MaxY; Y += 2)
                    {

                        for(int XI = MinX; XI < MaxX; XI += 4)
                        {
                            ...
                            ...
                        }
                        ...
                        ...
                    }
                }

we get an edge case on the last iteration. For example lets say XI = 3 and MaxX is 16

on the last iteration, where XI is 15, we will actually be filling 
XI = 15, 16, 17, 18. So three more pixels. But What we really want to do is to cap it at 16

we can either fix the alignment in the beginning (the first iteration); or at the end (the last iteration);





and assume we want to fill 5 pixels, and we are filling them in chunks of 4

                            start            end 
                 _______________________________________________________
                |   |   |   | 0 | 1 | 2 | 3 | 4 |   |   |   |   |   |   |  option 1
                |___|___|___|___|___|___|___|___|___|___|___|___|___|___|

                 _______________________________________________________
                |   |   |   | 0 | 1 | 2 | 3 | 4 |   |   |   |   |   |   |  option 2    
                |___|___|___|___|___|___|___|___|___|___|___|___|___|___| 
                                                                     

so we can either, mask out in the last iteration (the x means masking them out);

                            start            end 
                 _______________________________________________________
                |   |   |   | 0 | 0 | 0 | 0 | 1 | 1 | 1 | 1 |   |   |   |  option 1
                |___|___|___|___|___|___|___|___|_x_|_x_|_x_|___|___|___|


so we can either, mask out in the first iteration (the x means masking them out);

                            start            end 
                 _______________________________________________________
                | 0 | 0 | 0 | 0 | 1 | 1 | 1 | 1 |   |   |   |   |   |   |  option 2
                |_x_|_x_|_x_|___|___|___|___|___|___|___|___|___|___|___|



3:08:49
Casey says its usually easier to do the speical case in the beginning than to do it at the end

we can just initalize the mask to to the 0001 in the first iteration and clear it at the end of the first iteration. 
that way we dont ever have to have branching code of checking whether we are on the last iteration of the loop to do 
the special case.

So Casey prefers option2.



3:10:20
so now the code is below:

notice that the StartupClipMask is set to -1. -1 in 2 compliments is 1111111111111111111111..  all ones

-   so at first we have FillRect = Intersect(ClipRect, FillRect);
    Since we are doing batches of 4 pixels in our SIMD code, if FillRect is not a multiple of 4, 
    we have to adjust FillRect width to be multiples of 4. 

    we already just mentioned above that we will make the beginning the special case, 
    so we will make FillRect.MaxX fixed, and push FillRect.MinX forward


    Visually, is looks like like 
                    
  pixel 0                              pixel N 
                         _____________
                        |### #### ####|
                        |### #### ####|
                        |### #### ####|
                        |### #### ####|
                                           <--------   FillWidth += Adjustment;
                        ______________                 FillRect.MinX = FillRect.MaxX - FillWidth;
                       |#### #### ####|
                       |#### #### ####|
                       |#### #### ####|
                       |#### #### ####|
    
    now it is a multiple of 4                       



-   So to push FillRect.MinX forward if we need, we do some math calculations
    At first, FillWidth is the difference between Max and Min, we require that to be a multiple of 4.
    Specically the next largest multiple of 4.
    hence FillWidth += Adjustment; happens 


-   FillWidthAlign = FillWidth & 3, which gives you the remainder of FillWidth / 4
    so to be more explicit, this is FillWidth & (numPixelsInBatch - 1), where numPixelsInBatch = 4


-   we also assign the StartupClipMask accordingly.
                   
                  FillRect.MinX     MaxX
                         _____________
                        |### #### ####|
                        |### #### ####|
                        |### #### ####|
                        |### #### ####|
                  
                        ______________
                       |#### #### ####|
                       |#### #### ####|
                       |#### #### ####|
                       |#### #### ####|
             
StartupClipMask         0111 1111 1111

    but our StartupClipMask is gonna be 0111



-   note that our StartupClipMask operation. 

    in the intel Intrinsics,
    https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=_mm_slli_si128&expand=5288

    _mm_slli_si128 takes the 2nd argument as bytes. so if our adjustment is 3 pixels, 

    then our StartupClipMask has to shift left 12 bytes, which becomes (number of bits is not accurate);
    
    1111111111 0000000000 00000000000 000000000

    also note, this is shift left, not shift right becuz the later pixels are in the high bit

    pixel 3    pixel 2    pixel 1     pixel 0
    1111111111 0000000000 00000000000 000000000


                void DrawRectangleQuickly()
                {

                    ...
                    ...

                    FillRect = Intersect(ClipRect, FillRect);
                
                    ...
                    ...

                    __m128i StartupClipMask = _mm_set1_epi8(-1);
                    int FillWidth = FillRect.MaxX - FillRect.MinX;
                    int FillWidthAlign = FillWidth & 3;
                    if(FillWidthAlign > 0)
                    {
                        int Adjustment = (4 - FillWidthAlign);
                        // TODO(casey): This is stupid.
                        switch(Adjustment)
                        {
                            case 1: {StartupClipMask = _mm_slli_si128(StartupClipMask, 1*4);} break;
                            case 2: {StartupClipMask = _mm_slli_si128(StartupClipMask, 2*4);} break;
                            case 3: {StartupClipMask = _mm_slli_si128(StartupClipMask, 3*4);} break;
                        }
                        FillWidth += Adjustment;
                        FillRect.MinX = FillRect.MaxX - FillWidth;
                    }
                


                    for(int Y = MinY; Y < MaxY; Y += 2)
                    {
                        ...
                        ...

                        __m128i ClipMask = StartupClipMask;


                        for(int XI = MinX; XI < MaxX; XI += 4)
                        {
                            ...
                            ...



                            __m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero),
                                                                                       _mm_cmple_ps(U, One)),
                                                                            _mm_and_ps(_mm_cmpge_ps(V, Zero),
                                                                                       _mm_cmple_ps(V, One))));
                            WriteMask = _mm_and_si128(WriteMask, ClipMask);



                            ...
                            ...
                            ClipMask = _mm_set1_epi8(-1);
                        }
                        ...
                        ...
                    }
                }







3:39:56
For multithreading purposes, we added a new function, TiledRenderGroupToOutput(); This replaces 
the RenderGroupToOutput(); function.

                handmade_render_group.cpp

                internal void
                TiledRenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
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
                            // TODO(casey): Buffers with overflow!!!
                            rectangle2i ClipRect;

                            ClipRect.MinX = TileX*TileWidth + 4;
                            ClipRect.MaxX = ClipRect.MinX + TileWidth - 4;
                            ClipRect.MinY = TileY*TileHeight + 4;
                            ClipRect.MaxY = ClipRect.MinY + TileHeight - 4;

                            RenderGroupToOutput(RenderGroup, OutputTarget, ClipRect, true);
                            RenderGroupToOutput(RenderGroup, OutputTarget, ClipRect, false);
                        }
                    }
                }




and we replaced the calls to RenderGroupToOutput in the handmade.cpp file to that 


                internal void
                FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
                {
                    // TODO(casey): Decide what our pushbuffer size is!
                    temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);

                    // TODO(casey): Need to be able to set an orthographic display mode here!!!
                    loaded_bitmap *Buffer = &GroundBuffer->Bitmap;
                    Buffer->AlignPercentage = V2(0.5f, 0.5f);
                    Buffer->WidthOverHeight = 1.0f;
                    
                    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4),
                                                                    Buffer->Width, Buffer->Height);

                    Clear(RenderGroup, V4(1.0f, 1.0f, 0.0f, 1.0f));

                    GroundBuffer->P = *ChunkP;

                    
                    TiledRenderGroupToOutput(RenderGroup, Buffer);
                    EndTemporaryMemory(GroundMemory);
                }

and 


                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {       
                    ...
                    ...
                 
                    for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
                    {
                        ...
                        ...
                            
                        hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[Entity->FacingDirection];
                        switch(Entity->Type)
                        {
                            case EntityType_Hero:
                            {
                                ...
                                ...
                                PushBitmap(RenderGroup, &GameState->Shadow, HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                                ...
                            } break;
                            ...
                            ...
                        }                        
                    }

                    ...

                    TiledRenderGroupToOutput(RenderGroup, DrawBuffer);    

                    ...
                    ...
                }


now our multi threading renderer render stuff like below:
assume we have Tile 0 1 2 3 4 5. Core 0 gets Tile 0, Core 1 gets Tile 1... 

Every thread takes the entire RenderGroup. The RenderGroup will have A, B, C and D.

since we added the ClipRect system, Core 0 will only render portions that overlap with tile 0

so in this case, it will render render the left half of A in Tile 0. 
Core 1 will render right half of A and left edge of B in Tile 1
...
...

         ______________________________________
        |         AAA|AAA        B|BBB         |
        |   0     AAA|AAA     1  B|BBB     2   |
        |            |           B|BBB         |
        |            |            |            |
        |            |            |            |
        |____________|____________|____________|
        |            |            |            |
        |   3        |        4   |        5   |
        |            |    CCCCC   |            |
        |            |    CCCCC   |            |
        |   DDDDDDDDD|DDD         |            |
        |____________|____________|____________|







