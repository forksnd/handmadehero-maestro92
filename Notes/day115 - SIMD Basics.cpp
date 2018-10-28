Handmade Hero Day 115 - SIMD Basics

Summary:
talked about the basics of SIMD on x64

changed the DrawRectangleHopefullyQuickly(); function to set up for SIMD 
he did not finish the optimization. Will complete tomorrow.

mentioned optimization for SIMD vs optimization for cache miss in Q/A

mentioned that SSE2 is very standard in Q/A 
according to the Steam hardware survey, everyone uses SSE2

talked about how bad compilers are at emitting SIMD instructions
mentioned a link about Auto-Vectorization in LLVM
https://llvm.org/docs/Vectorizers.html

Keyword:
SIMD



2:00
SIMD on x64
there are SIMD on ARM processors, called Neon

single instruction, multiple data

for every instruction that the processor decodes, it will do it on more than once piece of data at once 


B = A + A  ------->   ADD    B  A A


		in SIMD, we see 

		B0   A0   A0		<--- lane
		B1 = A1 + A1	
		B2 	 A2	  A2
		B3	 A3	  A3

this example is 4 wide 



6:00
there are different kinds of those. 
there are SSE / Neon		these are 4-wide 
SSE is an x64 thing 
Neon is an Arm thing 


recently Intel introduced AVX, which is 8 wide 

and they also introduced AVX512, which is 16 wide 




7:55
the simplest SIMD thing you can do is to change 


			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        uint32 *Pixel = (uint32 *)Row;
			        for(int X = XMin; X <= XMax; X++)
			        {
			        	test pixel[X,Y]

to

			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        uint32 *Pixel = (uint32 *)Row;
			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {
			        	test pixel[XI,y]
			        	test pixel[XI+1,y]
			        	test pixel[XI+2,y]
			        	test pixel[XI+3,y]



which instead of doing this one thing one pixel at a time, we can do it 4 pixels at a time




9:27
our frame buffer is currently structured in the following way 

	pointer -> rgba rgba rgba rgba rgba rgba .....

				A    B    C    D    E    F 


which represents an image that looks like 

	A B C D E F 
	G H I J K L 
	L M N O P Q 


just pixel rgba values pacted together

probably this is not the most efficient way to organize the pixels that we are operating on 

in fact this is often not the way the GPUS organize frame buffer 

ofthen times they store it in a "swizzel" format. (is this the morten order Z way?)

since "Long - slizors" are very uncommon in games, the common case are well shaped quads

so they will often rearrange memory, so four contiguous memory entries forms a block


	pointer -> rgba rgba rgba rgba rgba rgba .....

				A    B    G    H    C    D 

	A B C D E F 
	G H I J K L 
	L M N O P Q 


A B G H are together 


13:03
this is even more important, for example when the Avx 512 that is 16 wide. you will burn even more if 
you do the straightforward row by row storage 



14:18
"SOA" vs "AOS"

struct of array vs array of struct

what you want for SIMD is structure of array
Array of structure is what C makes easy


pretty much the same argument for ECS, for performance, you want structure of array 
AOS is garbage for performance





19:03
take an example, lets say one of your general purpose register is 64 bit 

				mov A, [memory]
				ADD A, A


				64 bits 
				 __________________________________________
				|					  |		float		   |
				|_____________________|____________________|

lets say you load a 32 bit float from memory to a registers, it will just occupy the register

registers designed for SIMD are just longer. 128 bits 

instead of just a simple ADD command, it will have an extra note indicating the operation 
for example ADD4F can indicate 4 wide floating 

which says it treats the registers as if there are 4 wide floating points in it, and add them separately 

				mov A, [memory]
				ADD4F A, A


				128 bits 
				 ____________________________________________________________________________________
				|					  |		               |          float     |                    |
				|_____________________|____________________|____________________|____________________|


or something like ADD8S

which is like 8 signed integers, each being 16 bit 

				128 bits 
				 _______________________________________________________________________________________
				|	1   	|	2	  |		3   |     4     |   5     |    6      |   7    |   8        |
				|___________|_________|_________|___________|_________|___________|________|____________|

that allows the processor more to do the operation most efficiently

22:05
most of the time it is exactly the same as regular intructions 
so to do SIMD, it is more about setting it up, such as the	"mov A, [memory]" operation 

how do I fill up the 128 bits, and how do I write them out later 



this is why SOA vs AOS is pretty important on intel is becuz they do not support strided loading 

on the NEON architecture, you can actually issue a load instruction like load every fourth float 
into my SIMD register

so if you have the structure 
	RGBA RGBA RGBA RGBA ...

and you want to load that into your register, bascially you just specify the stride of your operation


but on SSE (intels SIMD register), there is no such stuff


24:07
so if you have your data in the SOA structure, you have to shuffle things around and do a bunch of reorganization

so for Intel, SOA is more favored










26:17
mentioned the intel intrinsic guide again

https://software.intel.com/sites/landingpage/IntrinsicsGuide/


26:43
if you target SSE2 processor, that covers bascially all the processors anyone runs games on right now for intel
for example 

				__m128i _mm_add_epi16 (__m128i a, __m128i b)			paddw

the paddw is the actual instruction, the left part is the intrinsics


29:06
the way you do one of those is that 

				internal void DrawRectangleHopefullyQuickly()
				{
					BEGIN_TIMED_BLOCK();

					__m128 Value = _mm_set1_ps(1.0f);


					...
					..
				}

this instruction means, take the value 1.0f, replicated to all four lanes in SIMD 





29:44
the way you do one of those is that 

				internal void DrawRectangleHopefullyQuickly()
				{
					BEGIN_TIMED_BLOCK();

					__m128 ValueA = _mm_set1_ps(1.0f);
					__m128 ValueB = _mm_set1_ps(2.0f);
					__m128 Sum = _mm_add_ps(ValueA, ValueB);
					
					...
					...
				}

this instruction means, take the value 1.0f, replicated to all four lanes in SIMD 


31:26
showing us the debug window 
the SIMD registers are 
				XMM0
				XMM1
				XMM2
				XMM3
				XMM4
				XMM5
				XMM6
				XMM7

the first eight here are originally introduced with SSE

they later added 8 more registers to the processor
				
				XMM8
				XMM9
				XMM10
				XMM11
				XMM12
				XMM13
				XMM14
				XMM15

the values in XMM00, XMM01, XMM02, XMM03 maps to XMM0



36:18
there are also other instructions such as 

				__m128 ValueA = _mm_set_ps(1.0f, 2.0f, 3.0f, 4.0f);


39:41
but usually when people talk about SIMD, they are talking about how to organize your data instead of 
talking about which instrinsics to use 


42:32
starting to do the actual batches of 4 pixels change 


			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        uint32 *Pixel = (uint32 *)Row;
			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {
			     		


			        }

			    }


46:47
Casey points out something that we are overwriting our boundaries a bit 

that is becuz when we are on the last bit of each row, we are still writing 4 pixels.
so the fix is to clamp it at Buffer->Width - 3, Buffer->Height - 3


			    int WidthMax = (Buffer->Width - 3);
			    int HeightMax = (Buffer->Height - 3);
			    




49:39
Casey says he usually just write the code direcly SIMD


50:45
Casey wrote it in three parts 
loading from memory
shading
writing to memory
so we got three for loops


            bool ShouldFill[4];
            
            for(int I = 0; I < 4; ++I)
            {
                ShouldFill[I] = ((U >= 0.0f) &&
                                 (U <= 1.0f) &&
                                 (V >= 0.0f) &&
                                 (V <= 1.0f));
                if(ShouldFill[I])
                {
                	...
                }
            }
            

            for(int I = 0; I < 4; ++I)
            { 
				compute the shading shading 
            }

            
            // actually writing to memory
            for(int I = 0; I < 4; ++I)
            {
                if(ShouldFill[I])
                {
                    // NOTE(casey): Repack
                    *(Pixel + I) = (((uint32)(Blendeda[I] + 0.5f) << 24) |
                                    ((uint32)(Blendedr[I] + 0.5f) << 16) |
                                    ((uint32)(Blendedg[I] + 0.5f) << 8) |
                                    ((uint32)(Blendedb[I] + 0.5f) << 0));
                }
            }



51:56
Casey says: i know this looks ugly, but you dont have to do any of this stuff 
he is just doing this to show us something 

if you actually program in SIMD, it will look nothing like this 



1:12:40

how often do you optimize for cache miss vs optmize for SIMD? I have the impression that cache miss
are the most important thing?

They are and they arent. basically what it boils down to is what does the structure of your data look like 

cache miss tends to about chasing pointers, and other places where you cant predict where and waht 
is the next piece of the memory that you need 

in this piece of code, we know exactly where our memory is gonna be, so cache miss is less of an issue.
so in this case  where we are filling a large block of known pixels in a contiguous order, doing things SIMD 
plays a bigger role 

in other case cache miss will typically be a larger concern in other cases




1:18:41

mentioned this link:
https://www.itu.dk/~sestoft/bachelor/IEEE754_article.pdf

for really really understanding floating points 


also recommended two books by Forman Acton

Real Computing made really
https://www.amazon.com/Real-Computing-Made-Engineering-Calculations/dp/0486442217

Numerical Methods that Work (Spectrum)
https://www.amazon.com/Numerical-Methods-that-Work-Spectrum/dp/0883854503


1:21:53
mentioned that SSE2 is very standard

mentioned that on Steam hardware survey
https://store.steampowered.com/hwsurvey/Steam-Hardware-Software-Survey-Welcome-to-Steam

Everyone uses SSE2 and SSE3


1:30:40
how efficient are compilers at emitting SIMD instructions?

They are not at all good at it. Unfortunately.

Casey mentioned a link about Auto-Vectorization in LLVM
https://llvm.org/docs/Vectorizers.html

if you havent structure your code to work in SIMD, then the compiler cant do much to help you anyway

typically Casey finds that compiler not that helpful in doing SIMD.


1:34:47
how does SIMD and parallel processing work together?
Each processor can do SIMD themselves. Core are separarate

SIMD works on registers. Since each CPU has their SIMD registers, they will work separately.
