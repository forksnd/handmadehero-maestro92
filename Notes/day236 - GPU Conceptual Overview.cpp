Handmade Hero Day 236 - GPU Conceptual Overview

Summary:
explained how GPU works. almost everything in this episode is important.

explained how GPU and CPU are different and how their memory are different.

explained how GPU and CPU executes code differently.

explained how shaders get run on GPU.

Q/A
explained the difference between programming graphics on PC vs console 

Keyword:
GPU, CPU 



6:26
the world we live in nowadays, we have cell phones and laptops, and almost all of them have a GPU in it.

7:01
So Casey gonna give a little background on how CPU and GPU works together 

so recall CPU, has the main memory, usually in 16 GB

                                         16 GB
             ___________              ___________
            |           |____________|           |
            |  CPU i7   |=== bus ====|           |
            |           |____________|           |
            |___________|            |  Main     |
                                     |  Memory   |
                                     |           |
                                     |           |
                                     |           |
                                     |___________|


then you also have the CPU, lets say its i7. 
the CPU and Main Memory is connected by a bus

the bus allows the cpu chip to access main memory and to put things send things and get things.

and there are some io ports and stuff 

there are of course other components, but we dont really care.
The CPU and memory are the main thing we care about right now.

so with this current diagram, there is nothing putting things onto the screen. 


imagine we have our screen on the side 
                                                                                        
                                                                                        screen
                                                                                 ___________________________
                                         16 GB                                  |                           |
             ___________              ___________                               |                           |
            |           |____________|           |                              |        ######             |
            |  CPU i7   |=== bus ====|           |                              |        ######             |
            |           |____________|           |                              |        ######             |
            |___________|            |  Main     |                              |                           |
                                     |  Memory   |                              |                           |
                                     |           |                              |___________________________|
                                     |           |
                                     |           |
                                     |___________|


u may wonder how are we drawing things to the screen,
so essentially we have the GPU doing the job.
so we gave the GPU chip and GPU memory

The GPU memory is often times 1 GB. its alot smaller. its almost always smaller than the CPU memory



                                                                                                           screen
                                                                                                      ___________________________
                                         16 GB                                                       |                           |
             ___________              ___________         ___________        _______________         |                           |
            |           |____________|           |       |           |______|               |        |        ######             |
            |  CPU i7   |=== bus ====|  Main     |       |  GPU      | bus  |    GPU        |========|        ######             |
            |           |____________|  Memory   |       |           |______|  Memory       |  HDMI  |        ######             |
            |___________|            |           |       |___________|      |               | cable  |                           |
                                     |           |                          |               |        |                           |
                                     |  #####    |                          |  #####        |        |___________________________|
                                     |  #####----|--------------------------|->#####        |
                                     |  #####    |     transfering          |  #####        |               
                                     |___________|       bitmap data        |_______________|


the reason why this has to happen is becuz, the HDMI cable is connected to the GPU memory.
so if we were to display something on the screen, that data has to live in the GPU memory. 


9:17
so what happens is that at some point, windows running on the CPU, has been moving our bitmap from CPU memory 
to the GPU memory. Then the GPU has been transfering that data over the HDML cable to the screen.

There is actually hardware on the GPU that is designed to read out from the GPU memory and encoded into a HDMI signal


10:30
so in our code, the transfering of bitmaps from CPU memory to GPU is being done at 

                win32_handmade.cpp
                
                internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                                           HDC DeviceContext, int WindowWidth, int WindowHeight)
                {

                    // TODO(casey): Centering / black bars?
                    
                    if((WindowWidth >= Buffer->Width*2) && (WindowHeight >= Buffer->Height*2))
                    {
                        StretchDIBits(...);
                    }
                    else
                    {

                        int OffsetX = 0;
                        int OffsetY = 0;
                    
                        // NOTE(casey): For prototyping purposes, we're going to always blit
                        // 1-to-1 pixels to make sure we don't introduce artifacts with
                        // stretching while we are learning to code the renderer!
    --------------->    StretchDIBits(...);
                    }

                }


10:59
on some computers, this doesnt actually have to happen. 
if we were in a situation where we have a CPU and GPU combo, like an integrated GPU on the intel chips 

and they have a shared memory architecture, that means the bitmap we drew could get direclty scanned out 
by that HDMI cable 


                                            Memory
         _______________                 _______________
        |               |               |               |
        |   CPU         |               |               |
        |               |               |               |
        |   GPU         |               |               |
        |_______________|               |               |
                                        |               |
                                        |    #####      | HDMI Cable
                                        |    #####------|------------>
                                        |    #####      |
                                        |               |
                                        |_______________|


so its not always the case that the transfer has to happen, it only happens if the GPU memory is separate.
so the picture drawn above (not this one, the one with separate GPU and CPU); is a typical Desktop setup.

this setup is a typicall mobile setup

so optimizing for the desktop setup is different from optimizing for the mobile setup
the desktop setup is the "worst case" scenario since it is the most disconnected setup.

so when you write your code, you have to design your code around the "worst case" setup 



13:22
so for our case we have, 

so the System RAM and GPU Ram are connected by the PCI bus. That is the slot that you plug 
your GPU card into

                         Intel                          NVIDIA

                          System RAM                    GPU Ram
     ______________      _______________             _______________      ______________  
    |              |    |               |           |               |    |              |
    |   CPU        |    |               |           |               |    |   GPU        |
    |              |====|               |           |               |====|              |
    |              |    |               |           |               |    |              |
    |______________|    |               |  PCI bus  |               |    |______________|
                        |               |===========|               |
                        |               |           |               |
                        |               |           |               |
                        |               |           |               |
                        |               |           |               |
                        |_______________|           |_______________|
                        
so there is obviously a speed hit when you transfer data from CPU RAM to GPU Ram.
the speed hit is not about bandwidth, its rarely about how fast you transfer data,
its more about latency: meaning the time you start to the time you finish 

so there is a lot of "latency" in the part of the PCI bus transfer 


16:05
so Casey beginning to explain another question. Why do we need a GPU?
why cant we have the CPU do the GPU stuff?

CPU and GPU are designed to do different tasks, with different trade offs.
They are getting closer as time goes on, but they started out as very different purposes, very far apart


Casey drew a graph 
on one end, we have a CPU at year 1999, and the other end we have a GPU at year 1999

            CPU 1999                         PU 2030                              GPU 1999
                |______________________________|____________________________________|

                ---------------------->                    <-------------------------                 


Casey made wrote PU to stand for "Processing Unit"

essentially, what has happened over the years is that the CPU and GPU has moved towards each other 



17:35
so a long time ago, a CPU is something that mostly operated SISD, single instruction single data, (as oppose to SIMD);

meanwhile a GPU is things that very parallel and very predictable operations 
such as filling pixels and texture mapping. 

so imagine you have a texture and you have your pixels 
            
often times, the texture memory is read only, and your pixels is write only, very very rarely you would read,
maybe except for texture blending 


            Read only
            Texture                                 Write Only
             ___________________                    ___________________
            |    |    |    |    |                  |    |    |    |    |
            |____|____|____|____|                  |____|____|____|____|
            |    |    |    |    |                  |    |    |    |    |
            |____|____|____|____|                  |____|____|____|____|
            |    |    |    |    |                  |    |    |    |    |
            |____|____|____|____|                  |____|____|____|____|
            |    |    |    |    |                  |    |    |    |    |
            |____|____|____|____|                  |____|____|____|____|

so its very dedicated and designed to do operations for reading textures and filling pixels



20:40
so the CPU and GPU are very different back then.

as the GPU become more general purpose, and the CPU start to encouter heat barriers, these two have 
moved much closer together. 



               CPU                CPU                    GPU                       GPU 
               1999               2016       PU 2030     2016                      1999
                |__________________|___________|__________|_________________________|

                ---------------------->                    <-------------------------                 

CPU 2016 either have a 8, or 16 wide SIMD unit (128 bits or 256 bits register units);

a huge amount of general purpose tech were added to GPU
and a huge amount of parallel tech were added to CPU 

so you start to see the two converge to a point where both could do both 

so we know alot about how CPU works, but we dont know how exactly GPU architecture works becuz NVIDIA wont tell us.



22:58
so now what a GPU looks like is no longer just something that is reading from textures
and writing to pixels.

now a GPU is a giant ALU. (this is getting inpu ECE 483 stuff);


so recall when we did SIMD stuff, we can issue a SIMD command that does 

    _mm_mul_ps (a, b);

and these were 128 bits, so they were 4 wide. 


GPU is just a giant ALU that is 16 wide or 32 wide 
so they basically take in a giant number of floats 
     _______________________________________________________________________________
    |___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|

and they do some arithmetic operation on it 


so if you are interested, you can go online and check NVIDA GPU core counts 

for example, this NVIDIA gtx titan 
https://www.nvidia.com/gtx-700-graphics-cards/gtx-titan-black/

and you can see from the specs, there are we have 2880 Cuda cores .

*As a side note, Casey mentioned:
so you might wonder if my CPU only has 4 or 8 cores, this gtx titan GPU has 2880 cuda core must be a beast?

as you expect anything with marketing, the number listed can be misleading.

                Number of Cuda cores = Number of GPU cores * width of ALU * Number of ALUS

so if the ALU is 16 wide, and lets say you have 4 ALUs per core.



so if you do the math again:
        
        2880 = number of GPU cores * 16 * 4

        number of GPU cores = 2880 / 16 / 4
                            = 45

so you really only got 45 cores. 



anyways back to the main topic. The cores on the GPU is a lot more simplified, so they dont 
have nearly as complicated instruction set. They dont have necessarily the same caching technology

29:06
the way that the GPU core works is a lot like our own software rasterizer in handmade hero 

recall we are doing 4 pixels at a time 

                 _______________________________
                |       |       |       |       |
                |       |       |       |       |
                |_______|_______|_______|_______|

so you may wonder do they do 16 wide?
the answer is no. since triangles are rarely long in one direction.

so you often triangles like 


                ################
                ##############
                ############
                ##########
                ########
                ######
                ####
                ##

and rarely like 

                ################################
                ###############
                ###

so instead of doing 16 wide,
they just added a vertical component to it 

                 _______________________________
                |       |       |       |       |
                |   A   |   B   |   C   |   D   |
                |_______|_______|_______|_______|
                |       |       |       |       |
                |   E   |   F   |   G   |   H   |
                |_______|_______|_______|_______|
                |       |       |       |       |
                |   I   |   J   |   K   |   L   |
                |_______|_______|_______|_______|
                |       |       |       |       |
                |   M   |   N   |   O   |   P   |
                |_______|_______|_______|_______|


so when they do their 16 wide multiple operation 

            gpu_mul_(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p);

the layout of the data is like what we drawn above


so the triangle it rasterizes probably only touches some of the pixels, so they mask


                 _______________________________
                |       |       |       |       |
                |   A   | ###   |   C   |   D   |
                |_______|#####__|_______|_______|
                |      #|#######|       |       |
                |   E ##|########   G   |   H   |
                |_______|#########______|_______|
                |       |   #######     |       |
                |   I   |   J  #####    |   L   |
                |_______|_______|####___|_______|
                |       |       |  ###  |       |
                |   M   |   N   |    ## |   P   |
                |_______|_______|_______|_______|

just like what we did
recall when we did our 4 wide operations, and we didnt want to write out to the last pixel 

                 _______________________________
                |       |       |       |#######|
                |       |       |       |#######|
                |_______|_______|_______|#######|
                
so the way we wrote our rasterizer is very very similar to how GPU works internally.



31:48
so these GPU cores are designed to take Shader code and execute them 16 or 32 wide.
that is what GPU cores are trying to do.

that is why a "shader" is not quite the same as "CPU code" 

you have to remember that an intel chip is higly optimized for executing straight line CPU code
that has lots of branches, jumps, and random memory access.

GPUs are not set up to work that way. they are made to run much more coherent code. 
so one of the reason why you cant just take CPU code and shove it into a GPU and expect it 
to get faster is becuz the degree that the GPU will give you faster performance is entirely on parallelism

and its dependent on two kinds of parallelism
1.  the fact that there are more cores on GPU than CPU 
2.  and utilzing the fact that the ALU is much bigger. 

what that means is that you have to be able to do 10 way parallel, or go 16 wide 
otherwise its likely that running on the GPU will be slower since the GPU runs at a slower clock rate,
with worst support for arbituary operations.


33:03
with that masking part in mind, lets talk about how those "if" statement works 

lets take an example 
                
                y = 5;
                if (x)
                {
                    y += 7;
                }
                else
                {

                }

when we run this on the CPU, what we expect is, we would load y and x from memory to registers, and 
we would do a test/compare instruction, and we do a jmp instruction based on the result of the comparison


we are used to thinking of "if" conditions in this way, but this is not how it works in the world of wide instructions.

in the GPU world, all branches are always taken. so what happens, when you get an "if" statement in shader code,
what that compiles to is not an "if" statement, but rather both branches, with a mask 

so in shader code when you have the exact same code 

                y = 5;
                if (x)
                {
                    y += 7;
                }
                else
                {

                }

what will happen is that, in your 16 wide lane, it will first do a test to see which slot in your 16 wide lane will do which branch. 
then it will produce a mask based on the result 

so it will compute the result from the "if" branch, and then compute the result from the "else" branch, then "and" the result together
to produce the correct composite result 

so essentially, in GPU land, there isnt a concept of a branch not taken. They are all taken


36:47
how does this work with loops?

Same thing, they loop until the very last guy no longer tests for the loop.


37:39
you will also often see things like "warp". 
a warp is just 16 or 32 slots in a 16 wide operation doing work together, which will take all the branches together.


39:58
so on the GPU there is also some special purpose hardware in there. for example, there is usually a memory unit that specializes in
fetching textures. There are lots of complexities in the GPU that tries to make the GPU run fast. 


40:55
so the next thing we need to know is how do we get the GPU to actually start doing the stuff we tell it to do.

there is this concept that "memory is mapped". what that means is that some portion of the system ram will be mapped, 
such that it lines up with a portion of the GPU ram




                         Intel                          NVIDIA

                          System RAM                    GPU Ram
     ______________      _______________             _______________      ______________  
    |              |    |  ___________  |           |  ___________  |    |              |
    |   CPU        |    | |           | |           | |           | |    |   GPU        |
    |              |====| |           |-|-----------|>|           | |====|              |
    |              |    | |___________| |           | |___________| |    |              |
    |______________|    |               |  PCI bus  |               |    |______________|
                        |               |===========|               |
                        |               |           |               |
                        |               |           |               |
                        |               |           |               |
                        |               |           |               |
                        |_______________|           |_______________|
                        

what will happen is that the CPU will say, I would like to put some data in the GPU RAM.

so it will first write into the system ram that was initially mapped, and that memory will get transferred
over the PCI bus to essentially fill in on the GPU side.

that is the concept of mapped memory. When two devices want to pass and share memory, so they can map it, so that it is mapped 
it both spaces. 

as you can might image, this goes both ways. the GPU could write into that memory and the CPU could receive it. 

42:20
however, nowadays, you can imagine that the graphics work load is so heavy.
so nowadays GPU have this thing "called DMA Transfer controls" that they call it as "Copy Engines"

which are part of the GPU chip, that literrally do nothing but takes instructions from you about where to grab stuff.
and they are just sitting on the chip grabbing stuff from CPU memory to GPU memory. 

Casey is unsure about whether this Copy Engine requires the memory to be mapped. its possible that as long as its memory 
that the Copy Engine can See and access, we are good. 

actually nowadays, the concept of "mapped memory" is less about the synchronization issue between the 
CPU Ram and GPU Ram sharing some memory on both sides. Today the problem is more about transfering data between the two sides.

when the DMA is reading from the CPU memory, the CPU needs to lock that part of memory down until DMA says he doesnt need it anymore.



46:10
so we also need to learn about how to tell the GPU how to do anything at all. 
that invovles the concept called "Push Buffer". You will notice that in our software renderer code, we also have something called 
the "Push Buffer". And we actually codded it this deliberately becuz that is modeled after exactly how the GPU works. 

what basically happens is that you get some part of your CPU, you lock it down, and into there, you put some data on it (essentially
data or commands for the GPU);  and the GPU is instructed to read from that part of your memory, and execute what it says. 

so the model of execution what you have to think of is:

let say we have handmade hero in our memory, and we have OpenGL32.dll, system drivers code mapped to our address space.
The CPU is executing instructions from our handmade hero executable. 


                Memory
             ___________________
            |                   |
    CPU ----|---> Handmade      |----
            |       Hero        |    |       
            |                   |    |
            |                   |    |
            |  ##############   |    | 
            |  ##############   |    |
        ____|___________________|__  |
            |                   |    |
            |    OpenGL32.dll   |<----
            |                   |
            |___________________|
            

so we will call out to functions in OpenGL32.dll. 
so what happens is that code in OpenGL32.dll (code that we cant see unless you work for NVIDIA); 
is that it will prepare some push buffer in our CPU memory. 

and then at some point (this is entirely up to the driver); it will take a "ring transition" 
[ring transitions is essentially priviledge level transitions. there is R3, R2, R1, R0 and R-1]

to go into the actual driver code. so we are mostly likely on the application code, so we will go from ring X (something 
higher, that doesnt have much priviledge); to Ring 0.

inside the ring 0 code, it will run code to kickoff our push buffer. 

50:45
In summary, the mental model we should have is that, CPU has stuff in his memory space that he is building up with calls 
to OpenGL. We will call the OpenGL, which will execute driver code to do this. 
We cant build the commands manually becuz we dont know the format. its different for different graphis card.

so we gotta call OpenGL or Direct3D or mantle, to construct our "command buffer" or "push buffer".

once we have constructed them, we do a ring transition to execute into kernel code 


52:07
so when we call our OpenGL calls, they are somewhat high level. Cuz they are card agnostic.  
the driver will take care of converting these somewhat highlevel calls into "push buffers"

Q/A
1:10:37
so the big difference between programming on console vs on pc in terms of how you deal with the graphics card is not about 
how OpenGL is so much different. The big different, you can count on certain things. for example:

    you know that a certain chip takes this long to do certain things, 
    I know this memory is shared between the GPU and the CPU, 
    I know the CPU can directly call the GPU to do certain operation.  

you can include these things into the code and use them. However, when you write code for PC, you cant do it. when you 
write code that way, it wouldnt run abstractly on all PC platforms. That is the price you pay to write code that runs on a 
heterogeneous computing environment.

but nowadays, the differences are very small sometimes. Today the playstation are becoming like living room PC.
the "console specific" optimizations you do during the playstation2 days vs playstation4 is night and day.

