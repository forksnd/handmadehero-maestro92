Handmade Hero Day 119 - Counting Intrinsics

Summary:
Wrote #defines to count the amount of intrisincs used in our DrawRectangleQuickly function
counted the total number of cycles used (intrinsics count * their throughput);
the throughput cycles count are taken from the intel intrinsics guide 

compared the measured throughput with the theoretical throughput.

explained that some instructions are overlap, and how the processors issues them,
which causes the difference between measured and theoretical

someone in Q/A mentioned the 
https://commons.wikimedia.org/wiki/File:Intel_Nehalem_arch.svg

to examine the Nahalem processor diagram to see if how many ports and units it has

Keyword:
intrinsics, processors


5:25
although we did a good job optimizing, going from 350 to 50,
we dont know how long it should take to fill this up





20:53

defines a bunch of macros that will replace the intrinsics that we are calling.
this way we can get a count of all the intrinsics calls that we are calling

				struct counts
				{
				    int mm_add_ps;
				    int mm_sub_ps;
				    int mm_mul_ps;
				    int mm_castps_si128;
				    int mm_and_ps;
				    int mm_or_si128;
				    int mm_cmpge_ps;
				    int mm_cmple_ps;
				    int mm_min_ps;
				    int mm_max_ps;
				    int mm_cvttps_epi32;
				    int mm_cvtps_epi32;
				    int mm_cvtepi32_ps;
				    int mm_and_si128;
				    int mm_andnot_si128;
				    int mm_srli_epi32;
				    int mm_slli_epi32;
				    int mm_sqrt_ps;
				};


				#if COUNT_CYCLES
				            counts Counts = {};
				#define _mm_add_ps(a, b) ++Counts.mm_add_ps; a; b
				#define _mm_sub_ps(a, b) ++Counts.mm_sub_ps; a; b
				#define _mm_mul_ps(a, b) ++Counts.mm_mul_ps; a; b
				#define _mm_castps_si128(a) ++Counts.mm_castps_si128; a
				#define _mm_and_ps(a, b) ++Counts.mm_and_ps; a; b
				#define _mm_or_si128(a, b) ++Counts.mm_or_si128; a; b
				#define _mm_cmpge_ps(a, b) ++Counts.mm_cmpge_ps; a; b
				#define _mm_cmple_ps(a, b) ++Counts.mm_cmple_ps; a; b
				#define _mm_min_ps(a, b) ++Counts.mm_min_ps; a; b
				#define _mm_max_ps(a, b) ++Counts.mm_max_ps; a; b
				#define _mm_cvttps_epi32(a) ++Counts.mm_cvttps_epi32; a
				#define _mm_cvtps_epi32(a) ++Counts.mm_cvtps_epi32; a
				#define _mm_cvtepi32_ps(a) ++Counts.mm_cvtepi32_ps; a
				#define _mm_and_si128(a, b) ++Counts.mm_and_si128; a; b
				#define _mm_andnot_si128(a, b) ++Counts.mm_andnot_si128; a; b
				#define _mm_srli_epi32(a, b) ++Counts.mm_srli_epi32; a
				#define _mm_slli_epi32(a, b) ++Counts.mm_slli_epi32; a
				#define _mm_sqrt_ps(a) ++Counts.mm_sqrt_ps; a
				#undef mmSquare
				#define mmSquare(a) ++Counts.mm_mul_ps; a
				#define __m128 int
				#define __m128i int

				#define _mm_loadu_si128(a) 0
				#define _mm_storeu_si128(a, b)
				#endif

You may be confused about the pound define, but lets see what happens

{
           		__m128 U = _mm_add_ps(_mm_mul_ps(PixelPx, nXAxisx_4x), _mm_mul_ps(PixelPy, nXAxisy_4x));
}

becomes 

{
				++Counts.mm_add_ps;

				(++Counts.mm_mul_ps; 
	 			PixelPx; 
	 			nXAxisx_4x 
	 			
	 			,

	 			++Counts.mm_mul_ps; 
	 			PixelPx; 
	 			nXAxisx_4x)

				;
}

basically, we just get it to compile


31:49
we get the number of operations needed for 4 pixels
Casey shows it in the debugger.
and Casey verifies it by counting it manually



34:51
then Casey writes code to actually count the number of cycles

essentially Casey wrote down all the throughput cycle value from the intel intrinsics guide 
and counted them here.

			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			    	...
			    	...

			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {     

			        	...
			        	...


			#if COUNT_CYCLES
			#undef _mm_add_ps
			            
			            real32 Third = 1.0f / 3.0f;

			            real32 Total = 0.0f;
			#define Sum(L, A) (L*(real32)A); Total += (L*(real32)A)
			            real32 mm_add_ps = Sum(1, Counts.mm_add_ps);
			            real32 mm_sub_ps = Sum(1, Counts.mm_sub_ps);
			            real32 mm_mul_ps = Sum(1, Counts.mm_mul_ps);
			            real32 mm_and_ps = Sum(Third, Counts.mm_and_ps);
			            real32 mm_cmpge_ps = Sum(1, Counts.mm_cmpge_ps);
			            real32 mm_cmple_ps = Sum(1, Counts.mm_cmple_ps);
			            real32 mm_min_ps = Sum(1, Counts.mm_min_ps);
			            real32 mm_max_ps = Sum(1, Counts.mm_max_ps);
			            real32 mm_castps_si128 = Sum(0, 0);
			            real32 mm_or_si128 = Sum(Third, Counts.mm_or_si128);
			            real32 mm_cvttps_epi32 = Sum(1, Counts.mm_cvttps_epi32);
			            real32 mm_cvtps_epi32 = Sum(1, Counts.mm_cvtps_epi32);
			            real32 mm_cvtepi32_ps = Sum(1, Counts.mm_cvtepi32_ps);
			            real32 mm_and_si128 = Sum(Third, Counts.mm_and_si128);
			            real32 mm_andnot_si128 = Sum(Third, Counts.mm_andnot_si128);
			            real32 mm_srli_epi32 = Sum(1, Counts.mm_srli_epi32);
			            real32 mm_slli_epi32 = Sum(1, Counts.mm_slli_epi32);
			            real32 mm_sqrt_ps = Sum(16, Counts.mm_sqrt_ps);
			#endif
			     
			            PixelPx = _mm_add_ps(PixelPx, Four_4x);            
			            Pixel += 4;
			        }
			        
				    Row += Buffer->Pitch;				    
				}


43:25
our measured throughput is slightly lower than our theoretical maximum would be if all of these 
instructions are inssued back to back.

Casey says the only explanation is that some of these instructions are issued in parrallel then. 
so we are issuing mulitple instructions per cycle


44:33
Casey says this total is basically what the total would be there was only one unit executing all these instructions
and they perfectly overlap.

and we can see that our measured throughput is slightly below that.


45:11
what this means is that we are actually getting a lot of pairing in here.
In order to count the pairing, we would have to know how many of these instructions we can issue at any given time.


45:40
Casey claims he doesnt know how many units this processor has, so he proceeds to google it and find out about it.
What we really want to know through the number of units is that how many instructions we can issue at once.

Casey is using a Nahalem processor


48:11
From a random link,
Casey found out that there are 2 FP units that we can issue to.


49:21
Casey says that this intrinsics guide does not tell us what instructions can be issued with other instructions

Recal on day112, Casey mentioned that a x64 processor can issue 4 instructions at once. 
However, which intructions can be issued together Matters. It cant just be any 4 instructions.
There has to be a unit free that can accept that instruction

if there are only two multipliers on the chip, it doeesnt matter if the front end of the processor can do 4 instructions,
u can only do two multiplies. 

if we want to get our intrisincs cycle count calculation to something more accurate is that we want to see 
which of these instructions can be issued per cycle.



54:15
Casey starts to examine how expensive is the gamma correction part

we found out it is not too bad. We only went from 48 cycles to 43 cycles.


1:03:12
someone mentioned the wikimedia nahalem diagram

https://commons.wikimedia.org/wiki/File:Intel_Nehalem_arch.svg

since Caseys computer has a nahalem processor
this is the diagram of the chip. it shows how many ports and units that it has 


Casey says that the multiplier port (FP MUL); is single ported. that means it can issue one multiply 
per cycle


1:06:51
someguy in the Q/A asked, would it be worth timing just the load stores with no ALU ops
to see how much is just memory bounds?

Casey says Yes! Although we are not memory aligned yet, Casey proceeds to do it.


1:20:42
why is it that taking away the gamma correction only speed things up by 6 cycles?

THe answer is the mulplications and square roots overlap.
Once the processor kicks off a square root, it is free to do other stuff. It is not sitting around waiting 
for that square root to come back. Theoretical the gamma correction takes 16 cycles, but that is end to end.
It is all about how many free units does the processor have at any time so that they can issue stuff.

