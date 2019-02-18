Handmade Hero Day 120 - Measuring Port Usage with IACA

Summary:
mentioned that Fabian Giesen argues the profiling methods we wrote in 119 is not accurate.
Fabian suggested to use the Intel Architecture Code Analyzer(IACA); to profile actualy cycles counts instead.
Casey used the IACA tool to profile his pixel rendering code.

Got rid of all the conversions from 0~255 to 0~1 space 
refactored code so that all calculations are done in the 0~255 and 0~65025 space (255 squared);

found out removing all these mulitplications did very little improvement.

Attempted move optimizations by doing 16 bit wide, integer multiplications. Previously we were doing 
4 wide 32 bit multiplications on the registers.

turns out although our total number of micros operations went down, our cycles went up. 
This is due to the specific operations we are running in our new version.

Someone in the Q/A recommended a fix to prevent aliasing, which did indeed increased performance

for info on aliasing 

real C++ example of aliasing
https://stackoverflow.com/questions/9709261/what-is-aliasing-and-how-does-it-affect-performance





Keyword:
IACA, optimizations 


1:29

from day119, Casey mentioned that Fabian did not like the we were attributing our counts here 

recall from day 119, we have the following code:

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


Fabian said it is a not a useful number by putting a throughput number in here.
this is becuz the throughput number already includes the parallelism in the CPU

for example if the throughput for add is 1, that means there is one add unit.

for the mm_and_ps, for the skylake family or processor, it is 0.33 CPI (cycles per instructions);
that means there are 3 and units in the processors, hence the 0.33. So you can do 3 and instruction in one cycle

while both mm_add_ps and mm_sub_ps takes one instruction, they can be issued together.

So Fabian objected that it is meaning less to add all these numbers together.
For example, we are not capturing that mm_add_ps and mm_mul_ps can be completed in one cycle.



3:59
so Casey said to Fabian that
to do this right, you will have to try to simulate the CPU, and we take all the assembly (not intrinsics); that we have,
and it will figure out exactly what the chip will do to the degree that it is documented. That will tell you when there will be 
stalls like for reasons that some instructiosn cant be issued in the same cycle. 

Fabian says theres an intel tool that does that for you already.

its called the Intel® Architecture Code Analyzer




7:56
Casey teaching us how to use this Intel Architecture Code Analyzer


8:25
Casey says how it thinks it really does is that it emits code that their analyzing tool can use to look
at your binary and see whats in it.

So Casey us using IACA_START, IACA_END around the parts you wanna analyze.


9:06
now the code looks like

			    for(int Y = MinY; Y < MaxY; ++Y)
			    {
			        uint32 *Dest = (uint32 *)DestRow;
			        uint32 *Source = (uint32 *)SourceRow;
			        for(int X = MinX; X < MaxX; ++X)
			        {
			        	IACA_VC64_START

			        	...
			        	...
			        	...
			        	...

			        	IACA_VC64_END
			        }

			    }


12:12
we got our report


13:20
Uops means micro ops. Recall micro ops are the things that the processor fundamentally operates on.
a single instruction might take multiple micro ops 



18:33
with this tool, there is no need to use the #define stuff we wrote in day119.
So Casey got rid of all the #defines.


20:07
Fabian says our code does not take advantage of some math rearragement as we could.

bilinear and squaring, they dont have to be done in floating point if we dont want them to be so

furthermore, they dont have to done in 0 to 1

For example we initially have the following code:

-	we get all the texel colors needed for bilinear interpolation, 
	we then square it, which fixes the gamma 

-	then we bi-lerp it 
{
                // NOTE(casey): Convert texture from 0-255 sRGB to "linear" 0-1 brightness space
                TexelAr = mmSquare(_mm_mul_ps(Inv255_4x, TexelAr));
                TexelAg = mmSquare(_mm_mul_ps(Inv255_4x, TexelAg));
                TexelAb = mmSquare(_mm_mul_ps(Inv255_4x, TexelAb));
                TexelAa = _mm_mul_ps(Inv255_4x, TexelAa);

                do the same for 
                TexelBb, TexelBg, TexelBr, TexelBa
                TexelCb, TexelCg, TexelCr, TexelCa
                TexelDb, TexelDg, TexelDr, TexelDa

                ...
                ...

               	__m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAr), _mm_mul_ps(l1, TexelBr)),
                                           _mm_add_ps(_mm_mul_ps(l2, TexelCr), _mm_mul_ps(l3, TexelDr)));
               	do the same for Texelg, Texelb, Texela

}


instead of squaring in 0~1 space, we can just square them in 0~255 space.



21:57
now we have the following code 

-	we square the our pixel value in the 0 ~ 255 space 

{
                
                TexelAr = mmSquare(TexelAr);
                TexelAg = mmSquare(TexelAg);
                TexelAb = mmSquare(TexelAb);

                do the same for 
                TexelBb, TexelBg, TexelBr, TexelBa
                TexelCb, TexelCg, TexelCr, TexelCa
                TexelDb, TexelDg, TexelDr, TexelDa


               	__m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAr), _mm_mul_ps(l1, TexelBr)),
                                           _mm_add_ps(_mm_mul_ps(l2, TexelCr), _mm_mul_ps(l3, TexelDr)));

               	do the same for Texelg, Texelb, Texela

}

recall in episode 119, during the Q/A section, where we examined the Nahalem processor diagram. 
for multiples we only have one unit, so we can only issue one multiply per cycle, which means, 
we dont get overlaps for multiplies. So the less multiplies, the better.


31:41 
Casey proceeds to further examine the code to get rid of more multiplies.
pretty much doing everything in 0~255 space. got rid of all the conversions between 0~255 and 0~1


36:01
apparently, despite doing all of that pruning, the IACA analysis indicates that we got more cycles.....
Casey deduces that whatever we were doing previously, the compiler was already optimizing it 


40:40
Casey comparing the IACA reports between the version before removing all the multiplications vs the version
after removing all the multiplicationss with WinDiff



48:04
Casey starting to consider other ways to optimize this code, and he is considering the integer mulplications
Take the example of this code: 

{
                TexelAg = mmSquare(TexelAg));
}

we are doing 4 operations wide. Casey says if we can do things in 16 bits,
that means we can do 8 wide.


especially now that we are doing things only in 0~65025 range (255 squared);, we only need 16 bits.

50:00

intiailly we have our values like below

				128 bits 
                 ____________________________________________________________________________________
                |     A3 B3 G3 R3     |     A2 B2 G2 R2    |    A1 B1 G1 R1     |    A0 B0 G0 R0     |
                |_____________________|____________________|____________________|____________________|

we get rid of two channels A and G, we keep the B and R Channel 
then for every 16 bit, we square the values 
                 ____________________________________________________________________________________
                |    B3*B3    R3*R3   |    B2*B2    R2*R2  |    B1*B1    R1*R1  |   B0*B0    R0*R0   |
                |_____________________|____________________|____________________|____________________|



52:26
Casey writing the actual code to do things in 16 bit 

initially we have this code 
{
                __m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(SampleA, MaskFF));
                __m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 8), MaskFF));
                __m128 TexelAr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 16), MaskFF));
                __m128 TexelAa = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(SampleA, 24), MaskFF));

                ...
                ...

                TexelAr = mmSquare(TexelAr));
}

now we have the following code.
-	we first get a bit Mask: Mask00FF00FF
	then we define TexelArb and TexelAag
	TexelArb has the red and blue channel 
	TexelAag has the alpha and green channel 

	we get the values for these two variables using the Mask00FF00FF mask.

	noticed just like the old code, when we get TexelAg, we shift it 8 bits to the right so we have 
	ARGB ---> 0ARG ---> 000G


-	then we get a squared version of TexelArb and TexelAag
	recall there is muliply lo and muliply hi: _mm_mullo_epi16 and _mm_mulhi_epi16
	for us our rgb values are always in the bottom 8 bits of the channel, so we will only use _mm_mul_lo

{				
				TexelArb = _mm_mullo_epi16(TexelArb, TexelArb);	
}
                


-	we do not want to square our alphas, so we got the TexelAa values separately.               


-	Notice we got rid of the "TexelAr = mmSquare(TexelAr);" as well. 
Instead, we replaced it with obtaining TexelAr, TexelAg and TexelAb


full code below 
{
                // NOTE(casey): Unpack bilinear samples
                __m128i TexelArb = _mm_and_si128(SampleA, MaskFF00FF);
                __m128i TexelAag = _mm_and_si128(_mm_srli_epi32(SampleA, 8), MaskFF00FF);
                TexelArb = _mm_mullo_epi16(TexelArb, TexelArb);
                __m128 TexelAa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelAag, 16));
                TexelAag = _mm_mullo_epi16(TexelAag, TexelAag);

                ...
                ...

                __m128 TexelAr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelArb, 16));
                __m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(TexelAag, MaskFFFF));
                __m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(TexelArb, MaskFFFF));
}




1:12:47
and we found out all this 16 bit squared optimizations is actually way worst


1:13:58
while Casey ran it through the IACA, while it is slower, the total number of Uops is 283, which is lower 
than before


But apparently although we went down in operations, but becuz of the specific operation we are running 
in the new version, it is actually worst for us




1:19:53
someone in the Q/A suggested pulling out the texture pointer dereferencing from the inner loop to 
local variables, the compiler is probably doing a read everytime becuz of aliasing.



previously we have 


                for(int I = 0; I < 4; ++I)
                {                
                    int32 FetchX = Mi(FetchX_4x, I);
                    int32 FetchY = Mi(FetchY_4x, I);

                    Assert((FetchX >= 0) && (FetchX < Texture->Width));
                    Assert((FetchY >= 0) && (FetchY < Texture->Height));

                    uint8 *TexelPtr = ((uint8 *)Texture->Memory) + FetchY*Texture->Pitch + FetchX*sizeof(uint32);
                    Mi(SampleA, I) = *(uint32 *)(TexelPtr);
                    Mi(SampleB, I) = *(uint32 *)(TexelPtr + sizeof(uint32));
                    Mi(SampleC, I) = *(uint32 *)(TexelPtr + Texture->Pitch);
                    Mi(SampleD, I) = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));
                }


now we have 

			    int32 TexturePitch = Texture->Pitch;
			    void *TextureMemory = Texture->Memory;

                for(int I = 0; I < 4; ++I)
                {                
                    int32 FetchX = Mi(FetchX_4x, I);
                    int32 FetchY = Mi(FetchY_4x, I);

                    Assert((FetchX >= 0) && (FetchX < Texture->Width));
                    Assert((FetchY >= 0) && (FetchY < Texture->Height));

                    uint8 *TexelPtr = ((uint8 *)TextureMemory) + FetchY*TexturePitch + FetchX*sizeof(uint32);
                    Mi(SampleA, I) = *(uint32 *)(TexelPtr);
                    Mi(SampleB, I) = *(uint32 *)(TexelPtr + sizeof(uint32));
                    Mi(SampleC, I) = *(uint32 *)(TexelPtr + TexturePitch);
                    Mi(SampleD, I) = *(uint32 *)(TexelPtr + TexturePitch + sizeof(uint32));
                }                

apparently that saved 10 cycles. Which is quite a bit.

For info on Aliasing 

https://www.youtube.com/watch?v=Tg6ID2uWjuY


real C++ example of aliasing
https://stackoverflow.com/questions/9709261/what-is-aliasing-and-how-does-it-affect-performance


essentially if you have a pointer in a loop, you would assume that we can just load the pointer value 
once outside the loop. But if the compiler thinks that the value of the pointer could change in the loop,
then it will have to load the pointer every iteration in the loop. Hence causing performance problems.

So we have to manually load the pointer outside the loop once.





1:25:52
someone in Q/A recommended using _mm_rsqrt_ps instead of _mm_sqrt_ps 

_mm_rsqrt_ps is the reciprocal square root 

https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=_mm_rsqrt_ps&expand=4770
you can see that _mm_rsqrt_ps has much lower CPI, cyclers per instruction 


whereas _mm_sqrt_ps has a much higher CPI
https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=_mm_sqrt_ps&expand=4770,5331

basically the idea is that, we have a number x, and we want to compute sqrt(x);

since 			_mm_rsqrt_ps = 1 / sqrt(x);

pretty much we want to replace sqrt(x); with  x * (1 / sqrt(x)) 

so previously we had 

{
                Blendedr = _mm_sqrt_ps(Blendedr);
                Blendedg = _mm_sqrt_ps(Blendedg);
                Blendedb = _mm_sqrt_ps(Blendedb);
}

now we have 
{
                Blendedr = _mm_mul_ps(Blendedr, _mm_rsqrt_ps(Blendedr));
                Blendedg = _mm_mul_ps(Blendedg, _mm_rsqrt_ps(Blendedg));
                Blendedb = _mm_mul_ps(Blendedb, _mm_rsqrt_ps(Blendedb));
}

and that indeed saved us cycles

