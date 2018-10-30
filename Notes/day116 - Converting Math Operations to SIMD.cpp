Handmade Hero Day 116 - Converting Math Operations to SIMD

Summary:


Keyword:





9:16
previously we had 

			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {

					    BEGIN_TIMED_BLOCK(ProcessPixel);
			        	
						...
						...

			        	END_TIMED_BLOCK(ProcessPixel);
			        }
			    }

Casey says we dont want to pollute the cache or ruin the loop so he 
moved the END_TIMED_BLOCK_TestPixel routine out 



			    BEGIN_TIMED_BLOCK(ProcessPixel);
			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {


			        	
						...
						...


			        }
			    }				
    			END_TIMED_BLOCK_COUNTED(ProcessPixel, (XMax - XMin + 1)*(YMax - YMin + 1));



and you might noticed that if we move it out of the block we wont know the number hits we got,
so we defined a new #define where we give its hit count cuz we know the number of pixels we will hit

#define END_TIMED_BLOCK_COUNTED(ID, Count) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += __rdtsc() - StartCycleCount##ID; DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount += (Count);




10:52 
gonna officially start converting the DrawRectangleHopefullyQuickly(); function to SIMD 

recall the idea is that we are just talking about registers that hold 4 values at once and do one operation



11:17
Casey is first declaring a __m128 variable. 

according to this link

https://msdn.microsoft.com/en-us/library/ayeb3ayc.aspx
The __m128 data type, for use with the Streaming SIMD Extensions and Streaming SIMD Extensions 2 instructions intrinsics, i
s defined in xmmintrin.h.

A variable of type __m128 maps to the XMM[0-7] registers.

so it is a variable that can loaded into one of XMM[0-7] registers





recall initially we have the code,
as we are doing batches of 4 pixels at a time, all of these arrays are just the four pixels r,g,b values 

			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        uint32 *Pixel = (uint32 *)Row;
			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {
			            real32 TexelAr[4];
			            real32 TexelAg[4];
			            real32 TexelAb[4];
			            real32 TexelAa[4];



here Casey is just replacing TexelAr[4] with the __m128 data type, so we got 


			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        uint32 *Pixel = (uint32 *)Row;
			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {
			            __m128 TexelAr;
			            real32 TexelAg[4];
			            real32 TexelAb[4];
			            real32 TexelAa[4];


pretty much exactly the same thing


later in 16:28, he initialized it to 
						
						__m128 TexelAr = _mm_set1_ps(0.0f);

because the compiler was complaining that it is not being intialized 




14:16
then previously, we have this piece of code 

the for loop of "for(int I = 0; I < 4; ++I)" is just iterating through each pixels


			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        uint32 *Pixel = (uint32 *)Row;

			        ...
			        ...

			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {
			        	...
			        	...

			            for(int I = 0; I < 4; ++I)
			            {
			                // NOTE(casey): Convert texture from sRGB to "linear" brightness space
			                TexelAr[I] = Inv255*TexelAr[I];
			                TexelAr[I] *= TexelAr[I];
			                TexelAg[I] = Inv255*TexelAg[I];
			                TexelAg[I] *= TexelAg[I];
			                TexelAb[I] = Inv255*TexelAb[I];
			                TexelAb[I] *= TexelAb[I];
			                TexelAa[I] = Inv255*TexelAa[I];


the original calculation on TexelAr[i] is just these two,

			                TexelAr[I] = Inv255*TexelAr[I];
			                TexelAr[I] *= TexelAr[I];


we multiply TexelAr[i] with Inv255, the we multiply TexelAr[i] by it self.

as we have already made TexelAr already a __128 data type, he will be replacing computations for that with 
the SIMD way, so we now have this 


			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        uint32 *Pixel = (uint32 *)Row;

			        ...
			        ...

			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {
			        	...
			        	...



			        	TexelAr = _mm_mul_ps(Inv255_4x, TexelAr);
			        	TexelAr = _mm_mul_ps(TexelAr, TexelAr);


			            for(int I = 0; I < 4; ++I)
			            {
			                // NOTE(casey): Convert texture from sRGB to "linear" brightness space
			                TexelAg[I] = Inv255*TexelAg[I];
			                TexelAg[I] *= TexelAg[I];
			                TexelAb[I] = Inv255*TexelAb[I];
			                TexelAb[I] *= TexelAb[I];
			                TexelAa[I] = Inv255*TexelAa[I];


remember TexelAr is now a __128 that now has the red values of our batches of four,
we are now calling the _mm_mul_ps(Inv255_4x, TexelAr) function, which multiplies Inv255_4x on all four lanes 

and we call _mm_mul_ps(TexelAr, TexelAr);. essentially this is the SIMD code that does exactly what we wanted.



15:23	
here we define what Inv255_4x is.

so we first declar Inv255, then we populate it to all 4 lanes 
in one of our SSE registers with that values


				real32 Inv255 = 1.0f / 255.0f
				__m128 Inv255_4x = _mm_set1_ps(Inv255);

and this will help us with the two operation we decalred in secion 14:16
		        
		        TexelAr = _mm_mul_ps(Inv255_4x, TexelAr);
				TexelAr = _mm_mul_ps(TexelAr, TexelAr);

ps means packed singles




18:23
Casey showing the __mm128 TexelAr value in the Visual studio Debugger 

as you can see, __mm128 is the a union. All this is is 128 bits, so it can be used anyway you want
depending on what instruction it is, it chooses to treat it as any of the values showned there


21:03
Casey showing us in the debugger that the instruction _mm_mul_ps(Inv255_4x, TexelAr) did indeed
multiplied TexelAr with Inv255_4x 


Very Simple. this is pretty much how you SIMD-ized code 


22:36
defined a macro to replace the squaring 

				#define mmSquare(a) _mm_mul_ps(a, a)
				
so you can make your own macros with intrinsics in it				



23:12
previously we had 

			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        uint32 *Pixel = (uint32 *)Row;

			        ...

			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {
			        	...

			            for(int I = 0; I < 4; ++I)
			            {
			                // NOTE(casey): Convert texture from sRGB to "linear" brightness space
			                TexelAr[I] = Inv255*TexelAr[I];
			                TexelAr[I] *= TexelAr[I];
			                TexelAg[I] = Inv255*TexelAg[I];
			                TexelAg[I] *= TexelAg[I];
			                TexelAb[I] = Inv255*TexelAb[I];
			                TexelAb[I] *= TexelAb[I];
			                TexelAa[I] = Inv255*TexelAa[I];

			                ...
			                ...
			            }
			        	...
			        }
			        ...
				}

is now replaced by                 

			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        uint32 *Pixel = (uint32 *)Row;

			        ...

			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {
			        	...

			        	TexelAr = mmSquare(_mm_mul_ps(Inv255_4x, TexelAr));
			        	TexelAg = mmSquare(_mm_mul_ps(Inv255_4x, TexelAg));
			        	TexelAb = mmSquare(_mm_mul_ps(Inv255_4x, TexelAb));
			        	TexelAa = _mm_mul_ps(Inv255_4x, TexelAa);


			            for(int I = 0; I < 4; ++I)
			            {
			                ...
			                ...
			            }
			        	...
			        }
			        ...
				}





24:02
and we do the same for TexelBr, TexelBg, TexelBb, TexelBa 
					   TexelCr, TexelCg, TexelCb, TexelCa 
					   TexelDr, TexelDg, TexelDb, TexelDa 




31:57
making the rest of the shading computations SIMD

40:40
mentiones the square root intrinsics.
there are multiple versions, and Casey is using the simplest one for us now _mm_Sqrt_ps();



44:05
now address the SIMD conversion for the clamp function.
Casey left this till the end becuz Clamp involves branching, so he didnt touch the clamp function at first.

previously the Clamp(); function looks like this: 

				handmade_math.h

				inline real32
				Clamp(real32 Min, real32 Value, real32 Max)
				{
				    real32 Result = Value;

				    if(Result < Min)
				    {
				        Result = Min;
				    }
				    else if(Result > Max)
				    {
				        Result = Max;
				    }

				    return(Result);
				}

Casey says he wants to show us what code is generated in the optmized build.

so he looked at the dissaembly code of the clamp function in the debugger.

46:24
apparently we see a bunch of jumps in the asembly code 
Casey says we dont want that 

The reason why it is doing clamps by jumps is becuz we wrote them that way

				    if(Result < Min)
				    {
				        Result = Min;
				    }
				    else if(Result > Max)
				    {
				        Result = Max;
				    }

with all the if comparisions, it is jumping. So the code is actually doing exactly what we wrote 


47:27
what we can do is to use the SIMD min and max function

apparently the _mm_min_ps() intrinsic can do a comparison and a replacement in one operation

https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=SSE,SSE2&text=_mm_min_ps&expand=3730

"Compare packed single-precision (32-bit) floating-point elements in a and b, and store packed minimum values in dst."

and as you can see the throughput is 1. so it is very fast.

same for _mm_max_ps();



49:42
with our Clamp01(); functions

	           for(int I = 0; I < 4; ++I)
	            {
	            	...
	            	...

	                Texel.r = Clamp01(Texel.r);
	                Texel.g = Clamp01(Texel.g);
	                Texel.b = Clamp01(Texel.b);

	                ...
	                ...
	            }


now they look like 

	            Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), One);
	            Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), One);
	            Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), One);

clamping is very cheap in SIMD. basically 2 cycles, one for min and one for max	            




50:29
and as you can see the code is already freaking fast

cycles per hit is arleady down to 179, and we havent even done everything


52:46 
will now start to tackle the loading the memory part
			
	            for(int I = 0; I < 4; ++I)
	            {
	                v2 PixelP = V2i(XI + I, Y);
	                v2 d = PixelP - Origin;
	                
	                real32 U = Inner(d, nXAxis);
	                real32 V = Inner(d, nYAxis);

	                ShouldFill[I] = ((U >= 0.0f) &&
	                                 (U <= 1.0f) &&
	                                 (V >= 0.0f) &&
	                                 (V <= 1.0f));
	                if(ShouldFill[I])
	                {
	                	...
	                	...


first Casey changes the v2 into scalar equations 


	            for(int I = 0; I < 4; ++I)
	            {
	                real32 PixelPx = XI + I;
	                real32 PixelPy = Y;

	                real32 dx = PixelPx - Origin.x;
	                real32 dy = PixelPy - Origin.y

	                real32 U = dx * nXAxis.x + dy * nXaxis.y;
					real32 V = dx * nYAxis.x + dy * nYaxis.y;

	                ShouldFill[I] = ((U >= 0.0f) &&
	                                 (U <= 1.0f) &&
	                                 (V >= 0.0f) &&
	                                 (V <= 1.0f));
	                if(ShouldFill[I])
	                {
	                	...
	                	...
            
55:35
for some reason, just by making this change, we went from 170 cycles/hit to 125 cycles/hit 
Casey proceeds to look at what the compiler is doing to put in 50 cycles of nonsense

and there are differences in the assembly code 


1:01:16
Casey is perplexed by the differences, and he says this is a lesson, 
ppl often joke about in C++ that doing something inline or doing certain things is zero cost.
but here we see somehing casting int to real32 literally confusing the compiler 50 cycles of optimization




1:03:43
anyways, Casey proceeds to make the above code SIMD.
-	Straightforward conversion, just like we have been doing in this episode

-	be careful with the order of XI + 3, XI + 2, XI + 1, XI + 0 for PixelPx

			    for(int Y = YMin; Y <= YMax; ++Y)
			    {
			        uint32 *Pixel = (uint32 *)Row;
			        ...

			        for(int XI = XMin; XI <= XMax; XI += 4)
			        {
			        	...
			        	...

			            __m128 PixelPx = _mm_set_ps((real32)(XI + 3),
			                                        (real32)(XI + 2),
			                                        (real32)(XI + 1),
			                                        (real32)(XI + 0));
			            __m128 PixelPy = _mm_set1_ps((real32)Y);

			            __m128 dx = _mm_sub_ps(PixelPx, Originx_4x);
			            __m128 dy = _mm_sub_ps(PixelPy, Originy_4x);
			            __m128 U = _mm_add_ps(_mm_mul_ps(dx, nXAxisx_4x), _mm_mul_ps(dy, nXAxisy_4x));
			            __m128 V = _mm_add_ps(_mm_mul_ps(dx, nYAxisx_4x), _mm_mul_ps(dy, nYAxisy_4x));

			            for(int I = 0; I < 4; ++I)
			            {

			                ShouldFill[I] = ((U >= 0.0f) &&
			                                 (U <= 1.0f) &&
			                                 (V >= 0.0f) &&
			                                 (V <= 1.0f));
			                if(ShouldFill[I])
			                {
			                	...
			                	...
		            


1:08:11
what is left that is not SIMD-ized is the loading the memory part


                if(ShouldFill[I])
                {
                    uint8 *TexelPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
                    uint32 SampleA = *(uint32 *)(TexelPtr);
                    uint32 SampleB = *(uint32 *)(TexelPtr + sizeof(uint32));
                    uint32 SampleC = *(uint32 *)(TexelPtr + Texture->Pitch);
                    uint32 SampleD = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));

                    TexelAr[I] = (real32)((SampleA >> 16) & 0xFF);
                    TexelAg[I] = (real32)((SampleA >> 8) & 0xFF);
                    TexelAb[I] = (real32)((SampleA >> 0) & 0xFF);
                    TexelAa[I] = (real32)((SampleA >> 24) & 0xFF);

                    TexelBr[i] = ...
                    TexelBg[i] = ...
                    ...
                    ...

                    same thing for the other three

                    // NOTE(casey): Load destination
                    Destr[I] = (real32)((*(Pixel + I) >> 16) & 0xFF);
                    Destg[I] = (real32)((*(Pixel + I) >> 8) & 0xFF);
                    Destb[I] = (real32)((*(Pixel + I) >> 0) & 0xFF);
                    Desta[I] = (real32)((*(Pixel + I) >> 24) & 0xFF);
                }


also the part of repacking the pixels 

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


Casey will do this for the next episode



1:10:21
Casey says when he optimizes, he doesnt do anything tricky such as operator overloading or what not 
becuz he is terrified at what the compiler will do.


1:13:10
someone asked in the Q/A that we went from 385 cycles to 130 cycles, and so if we can get it to 50 cycles?

Casey did some brief counting (literally looking at the number of operations in this code) 
and he says he is confident that he can get the math routine down to 200 cycles. 200 is for 4 pixels, so 50 cycles per pixel 


Casey says whether we can get the pixel pack and unpack down, that is the real question, and that is what he will tackle
in the next episode

pretty much, this is the hard part:

                uint8 *TexelPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
                uint32 SampleA = *(uint32 *)(TexelPtr);
                uint32 SampleB = *(uint32 *)(TexelPtr + sizeof(uint32));
                uint32 SampleC = *(uint32 *)(TexelPtr + Texture->Pitch);
                uint32 SampleD = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));



1:29:31 
someone says in the Q/A
why is there optimizations options on if you will end up SIMD-izing your code?

What optmization for the compiler means is that, try to generate good assembly code for I typed in.
and that does include vectorizing things. For the compiler to vectorize stuff, it is beyond current compiler technology



