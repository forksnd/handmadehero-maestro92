Handmade Hero Day 118 - Wide Unpacking and Masking

Summary:
Changed the pixel unpacking and masking into SIMD code

Keyword:
SIMD, intrinsics


4:00
so now we have to fill up the framebuffer depending on the shouldFill flag.

one approach is that assuming you have two pixels that are new, two are old 
                 ____________________________________________________________________________________
                |     A3 B3 G3 R3     |     A2 B2 G2 R2    |    A1 B1 G1 R1     |    A0 B0 G0 R0     |
                |_____________________|____________________|____________________|____________________|

                			                   |                     |
                			                   |                     |
                		old	                   | new                 | new               old
                			                   |                     |
                			                   v                     v

                 ____________________________________________________________________________________
                |     A3 B3 G3 R3     |     A2 B2 G2 R2    |    A1 B1 G1 R1     |    A0 B0 G0 R0     |
                |_____________________|____________________|____________________|____________________|


you can just write to the destination buffer with the old values in the 1st and 4th pixel





7:07
One approach is to combine the vectors of new and old, and use bit operation to produce the resulting values 

                 ____________________________________________________________________________________
                |     new3            |     new2           |    new1            |       new0         |
                |_____________________|____________________|____________________|____________________|

                				bit operation 

                 ____________________________________________________________________________________
                |      old3           |     odl2           |    old1            |       old0         |
                |_____________________|____________________|____________________|____________________|



9:34
so Casey changed the code to be 
				
{
					...
					...

        	        __m128i MaskedOut = _mm_or_si128(_mm_and_si128(WriteMask, Out),
	                                                 _mm_andnot_si128(WriteMask, OriginalDest));
	                _mm_storeu_si128((__m128i *)Pixel, MaskedOut);
}


12:06
Casey now writes the values for OriginalDest. This loads the values from a memory to OriginalDest

{
                	__m128i OriginalDest = _mm_loadu_si128((__m128i *)Pixel);
}

this is pretty much the opposite of the storeu operation that we used to write to the (__m128i *)Pixel;


I had a question whether __m128 is on the stack or in the register?

answers from this link says:

	 These types force the compiler to load themselves into the appropriate register (XMM* in this case), 
	 but still give the compiler the freedom to choose which one, or store them locally on the stack if 
	 all appropriate registers are taken. They also ensure that any time they are stored on the stack, 
	 they maintain the correct alignment (16 byte alignment in this case);

https://stackoverflow.com/questions/22207021/relationship-between-physical-registers-and-intel-simd-variables

so therefore in this case __m128i is either in one of the register or on the stack, depending on what the compiler does.


14:19
Casey wrote the value for WriteMask();

{
					__m128i WriteMask = _mm_set1_epi32(0);

					for(int I = 0; i < 4; i++)
					{
						...
						if(shouldFill[i])
						{
							...
							...

							Mi(WriteMask, I) = 0xFFFFFFFF;
						}
					}
}

notice that we fill up the bits if it is a pixel that we want to fill				

recall that we had M(WriteMask, I); is 

				#define M(a, i) ((float *)&(a))[i]

Mi(WriteMask, I); is just the integer version of it.
				
				#define Mi(a, i) ((uint32 *)&(a))[i]
	


16:01
mentioned that his friend Fabian Giesen told him, the SSE rounding mode, unless we change it, will do round to nearest by default,

so so we are changing the code

{            
		            __m128i Intr = _mm_cvttps_epi32(_mm_add_ps(Blendedr, Half_4x));
		            __m128i Intg = _mm_cvttps_epi32(_mm_add_ps(Blendedg, Half_4x));
		            __m128i Intb = _mm_cvttps_epi32(_mm_add_ps(Blendedb, Half_4x));
		            __m128i Inta = _mm_cvttps_epi32(_mm_add_ps(Blendeda, Half_4x));
}

back to 

{            
		            __m128i Intr = _mm_cvtps_epi32(Blendedr);
		            __m128i Intg = _mm_cvtps_epi32(Blendedg);
		            __m128i Intb = _mm_cvtps_epi32(Blendedb);
		            __m128i Inta = _mm_cvtps_epi32(Blendeda);
}




17:53
Casey will now SIMD-zed the unpacks operations
previously our code looks like this 

we are loading the TexelAr here in the for loop.

	            for(int I = 0; I < 4; ++I)
	            {
	                ShouldFill[I] = ((M(U, I) >= 0.0f) && 
				                	(M(U, I) <= 1.0f) && 
				                	(M(V, I) >= 0.0f) && 
				                	(M(V, I) <= 1.0f));

	                if(ShouldFill[I])
	                {
	                	...
	                	...

	                    uint8 *TexelPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
	                    uint32 SampleA = *(uint32 *)(TexelPtr);
	                    uint32 SampleB = *(uint32 *)(TexelPtr + sizeof(uint32));
	                    uint32 SampleC = *(uint32 *)(TexelPtr + Texture->Pitch);
	                    uint32 SampleD = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));

	                    M(TexelAr, I) = (real32)((SampleA >> 16) & 0xFF);
	                    M(TexelAg, I) = (real32)((SampleA >> 8) & 0xFF);
	                    M(TexelAb, I) = (real32)((SampleA >> 0) & 0xFF);
	                    M(TexelAa, I) = (real32)((SampleA >> 24) & 0xFF);

	                    ...
	                    we do the same for 
						TexelBr, TexelBg, TexelBb, TexelBa
						TexelCr, TexelCg, TexelCb, TexelCa
						TexelDr, TexelDg, TexelDb, TexelDa
						...

	                    // NOTE(casey): Load destination
	                    M(Destr, I) = (real32)((*(Pixel + I) >> 16) & 0xFF);
	                    M(Destg, I) = (real32)((*(Pixel + I) >> 8) & 0xFF);
	                    M(Destb, I) = (real32)((*(Pixel + I) >> 0) & 0xFF);
	                    M(Desta, I) = (real32)((*(Pixel + I) >> 24) & 0xFF);
	                }
	            }


Casey will now move TexelAr outside of the for loop into SIMD code.
so essentially we explicitly wrote out the code that gets the red value for four pixels

				uint32 SampleA[4];

	            for(int I = 0; I < 4; ++I)
	            {
	                ShouldFill[I] = ((M(U, I) >= 0.0f) && 
				                	(M(U, I) <= 1.0f) && 
				                	(M(V, I) >= 0.0f) && 
				                	(M(V, I) <= 1.0f));

	                if(ShouldFill[I])
	                {
	                	...
	                	...

	                    uint8 *TexelPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
	                    SampleA[i] = *(uint32 *)(TexelPtr);
	                    uint32 SampleB = *(uint32 *)(TexelPtr + sizeof(uint32));
	                    uint32 SampleC = *(uint32 *)(TexelPtr + Texture->Pitch);
	                    uint32 SampleD = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));

	                    ...
	                    we do the same for 
						TexelBr, TexelBg, TexelBb, TexelBa
						TexelCr, TexelCg, TexelCb, TexelCa
						TexelDr, TexelDg, TexelDb, TexelDa
						...

	                    // NOTE(casey): Load destination
	                    M(Destr, I) = (real32)((*(Pixel + I) >> 16) & 0xFF);
	                    M(Destg, I) = (real32)((*(Pixel + I) >> 8) & 0xFF);
	                    M(Destb, I) = (real32)((*(Pixel + I) >> 0) & 0xFF);
	                    M(Desta, I) = (real32)((*(Pixel + I) >> 24) & 0xFF);
	                }
	            }


                M(TexelAr, 0) = (real32)((SampleA[0] >> 16) & 0xFF);
                M(TexelAg, 0) = (real32)((SampleA[0] >> 8) & 0xFF);
                M(TexelAb, 0) = (real32)((SampleA[0] >> 0) & 0xFF);
                M(TexelAa, 0) = (real32)((SampleA[0] >> 24) & 0xFF);

                M(TexelAr, 1) = (real32)((SampleA[1] >> 16) & 0xFF);
                M(TexelAg, 1) = (real32)((SampleA[1] >> 8) & 0xFF);
                M(TexelAb, 1) = (real32)((SampleA[1] >> 0) & 0xFF);
                M(TexelAa, 1) = (real32)((SampleA[1] >> 24) & 0xFF);

                M(TexelAr, 2) = (real32)((SampleA[2] >> 16) & 0xFF);
                M(TexelAg, 2) = (real32)((SampleA[2] >> 8) & 0xFF);
                M(TexelAb, 2) = (real32)((SampleA[2] >> 0) & 0xFF);
                M(TexelAa, 2) = (real32)((SampleA[2] >> 24) & 0xFF);

                M(TexelAr, 3) = (real32)((SampleA[3] >> 16) & 0xFF);
                M(TexelAg, 3) = (real32)((SampleA[3] >> 8) & 0xFF);
                M(TexelAb, 3) = (real32)((SampleA[3] >> 0) & 0xFF);
                M(TexelAa, 3) = (real32)((SampleA[3] >> 24) & 0xFF);


22:03
apparently just by flattening out this portion of the code, we already down to the low hundreds cycles/hit count 

that seems counter-intuitive, cuz we moved code from the if(ShouldFill) conditional to a part that is happening all the time.

things that compilers do.....


				uint32 SampleA[4];
				uint32 SampleB[4];
				uint32 SampleC[4];
				uint32 SampleD[4];


	            for(int I = 0; I < 4; ++I)
	            {
	                ShouldFill[I] = ((M(U, I) >= 0.0f) && 
				                	(M(U, I) <= 1.0f) && 
				                	(M(V, I) >= 0.0f) && 
				                	(M(V, I) <= 1.0f));

	                if(ShouldFill[I])
	                {
	                	...
	                	...

	                    uint8 *TexelPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
	                    SampleA[i] = *(uint32 *)(TexelPtr);
	                    SampleB[i] = *(uint32 *)(TexelPtr + sizeof(uint32));
	                    SampleC[i]= *(uint32 *)(TexelPtr + Texture->Pitch);
	                    SampleD[i] = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));

	                    // NOTE(casey): Load destination
	                    M(Destr, I) = (real32)((*(Pixel + I) >> 16) & 0xFF);
	                    M(Destg, I) = (real32)((*(Pixel + I) >> 8) & 0xFF);
	                    M(Destb, I) = (real32)((*(Pixel + I) >> 0) & 0xFF);
	                    M(Desta, I) = (real32)((*(Pixel + I) >> 24) & 0xFF);
	                }
	            }

	            ...
	            getting TexelAr TexelAg TexelAb TexelAa 0 1 2 3
	            getting TexelBr TexelBg TexelBb TexelBa 0 1 2 3
	            getting TexelCr TexelCg TexelCb TexelCa 0 1 2 3
	            getting TexelDr TexelDg TexelDb TexelDa 0 1 2 3
	            ...
	            ...

25:13
Casey aims to compute the WriteMask directly. WriteMask right now essentially rellies on ShouldFill();
we can just do the same computation that ShouldFill is doing and compute the WriteMask

{					
                ShouldFill[I] = ((M(U, I) >= 0.0f) && 
			                	(M(U, I) <= 1.0f) && 
			                	(M(V, I) >= 0.0f) && 
			                	(M(V, I) <= 1.0f));
}

the way we can SIMD-ized this part is to use SSE comparison operations


for example: 
				__m128 _mm_cmpge_ps (__m128 a, __m128 b);
https://software.intel.com/sites/landingpage/IntrinsicsGuide/#cats=Compare&techs=SSE,SSE2&expand=882


it compares a and b by lane, and fills it with either 32 zeros, or 32 ones

				a3    a2    a1    a0

				b3    b2    b1    b0 

			_____________________________

			   0000   1111  0000   0000


essentially this operation is perfect for our WriteMask


29:47
now our write mask looks like this:
				
{
            	__m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero),
	                                                                       _mm_cmple_ps(U, One)),
	                                                            _mm_and_ps(_mm_cmpge_ps(V, Zero),
	                                                                       _mm_cmple_ps(V, One))));

}

this is equivalent of our 4 ands 
{
				A = (M(U, I) >= 0.0f) && (M(U, I) <= 1.0f)
				B = (M(V, I) >= 0.0f) && (M(V, I) <= 1.0f))				
				WriteMask = A && B;
}





32:39
previously, we have this if ShouldFill condition. that is becuz we dont want to load invalid memory

{
	            for(int I = 0; I < 4; ++I)
	            {
	                ShouldFill[I] = ((M(U, I) >= 0.0f) &&
	                                 (M(U, I) <= 1.0f) &&
	                                 (M(V, I) >= 0.0f) &&
	                                 (M(V, I) <= 1.0f));
	                if(ShouldFill[I])
	                {
	                	...
	                	...

	                	tX = ... UV ...
	                	tY = ... UV ...

                        uint8 *TexelPtr = ((uint8 *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32);
	                    uint32 SampleA = *(uint32 *)(TexelPtr);
	                    uint32 SampleB = *(uint32 *)(TexelPtr + sizeof(uint32));
	                    uint32 SampleC = *(uint32 *)(TexelPtr + Texture->Pitch);
	                    uint32 SampleD = *(uint32 *)(TexelPtr + Texture->Pitch + sizeof(uint32));

	                   ...
	                   ...
}}}

what we can do is to always clamp U V, and get rid of this if condition. 
By clamping, we will never load invalid memory.

so now we have



37:31
so with the UV, we can proceed ot make tx, ty, Fx fy all SIMD-ized
{
                U = _mm_min_ps(_mm_max_ps(U, Zero), One);
                V = _mm_min_ps(_mm_max_ps(V, Zero), One);

                // TODO(casey): Formalize texture boundaries!!!
                __m128 tX = _mm_mul_ps(U, WidthM2);
                __m128 tY = _mm_mul_ps(V, HeightM2);
                
                __m128i FetchX_4x = _mm_cvttps_epi32(tX);
                __m128i FetchY_4x = _mm_cvttps_epi32(tY);
            
                __m128 fX = _mm_sub_ps(tX, _mm_cvtepi32_ps(FetchX_4x));
                __m128 fY = _mm_sub_ps(tY, _mm_cvtepi32_ps(FetchY_4x));

                __m128i SampleA;
                __m128i SampleB;
                __m128i SampleC;
                __m128i SampleD;
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
}




43:37
and we are now down to around 70 cycles



46:08
we got almost everything in SIMD except for the fetch, which is still in for loop.
There is not such thing as a SIMD fetch, so that thing will just have to stay in SIMD.



46:43
the last part Casey wants to convert to SIMD is loading the destination color.
{
				__m128 Destr, Destg, Destb, Desta 
				M(Destr, 0) = (real32)((*(Pixel + 0) >> 16) & 0xFF);
				M(Destg, 0) = (real32)((*(Pixel + 0) >> 8) & 0xFF);
				M(Destb, 0) = (real32)((*(Pixel + 0) >> 0) & 0xFF);
				M(Desta, 0) = (real32)((*(Pixel + 0) >> 24) & 0xFF);

				M(Destr, 1) = (real32)((*(Pixel + 1) >> 16) & 0xFF);
				M(Destg, 1) = (real32)((*(Pixel + 1) >> 8) & 0xFF);
				M(Destb, 1) = (real32)((*(Pixel + 1) >> 0) & 0xFF);
				M(Desta, 1) = (real32)((*(Pixel + 1) >> 24) & 0xFF);

				M(Destr, 2) = (real32)((*(Pixel + 2) >> 16) & 0xFF);
				M(Destg, 2) = (real32)((*(Pixel + 2) >> 8) & 0xFF);
				M(Destb, 2) = (real32)((*(Pixel + 2) >> 0) & 0xFF);
				M(Desta, 2) = (real32)((*(Pixel + 2) >> 24) & 0xFF);

				M(Destr, 3) = (real32)((*(Pixel + 3) >> 16) & 0xFF);
				M(Destg, 3) = (real32)((*(Pixel + 3) >> 8) & 0xFF);
				M(Destb, 3) = (real32)((*(Pixel + 3) >> 0) & 0xFF);
				M(Desta, 3) = (real32)((*(Pixel + 3) >> 24) & 0xFF);
}


47:31
again the idea is that we want to unpack 

                 ____________________________________________________________________________________
                |     A3 R3 G3 B3     |     A2 R2 G2 B2    |    A1 R1 G1 B1     |    A0 R0 G0 B0     |
                |_____________________|____________________|____________________|____________________|

into something like 

		__m128 DestB
                 ____________________________________________________________________________________
                |     B               |     B              |   B                |    B               |
                |_____________________|____________________|____________________|____________________|

B from the original dest is 0 ~ 255

we are converting it to float, between 0 ~ 1

so to get blue channels for DestB, we will just mask out the other channels 

{
                __m128 Destb = _mm_cvtepi32_ps(_mm_and_si128(OriginalDest, MaskFF));
                __m128 Destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 8), MaskFF));
                __m128 Destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 16), MaskFF));
                __m128 Desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 24), MaskFF));
}



58:21
we are now down to around 55 cycles/hit 



1:00:32
did a slight optmization with the WriteMask

Casey introduced this intrinsics _mm_movemask_epi8, which 
Create mask from the most significant bit of each 8-bit element in a, and store the result in dst.
				
{
            	__m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero),
	                                                                       _mm_cmple_ps(U, One)),
	                                                            _mm_and_ps(_mm_cmpge_ps(V, Zero),
	                                                                       _mm_cmple_ps(V, One))));


				if(_mm_movemask_epi8(WriteMask))
				{
					...
					...

					rendering stuff

					...
					...
				}
}



1:12:05
will it be a good idea to do SIMD to all our math operations in all of our programs?

Yes and No;
SIMD is only useful if you can do 4 things at once.


for example, operations like 

				Color.rgb *= Color.a;

this operation is done on the SIMD registers. Casey proceeds to show us the Disassembly.
It is using the registers, but it is not doing 4 wide.



1:15:42 
what does it mean if an intrinsics has no throughput defined

for example _mm_cmpgt_ps

Casey doesnt know the answer.


1:26:04
Casey mentioned that we havent done pre-caching and we are not even memory aligned yet.
He will address those issues later


1:27:28
we cannot use half float becuz the SSE operation do not support half float
meaning there are no intrinsics for half float