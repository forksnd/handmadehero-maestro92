Handmade Hero Day 117 - Packing Pixels for the Framebuffer

Summary:
did the packing the pixel in SIMD fashion
showed two ways to do the interleaving, one the unpack_high and unpack_lo way 
the other the bit shift way 
Caey showing us how to debug or verify that your are calling _mm_unpacklo_epi32 or _mm_unpackhi_epi32 correctly.

briefly mentioned writing aligned and writing un-aligned memories

Keyword:
SIMD, intrinsics


2:30
Casey says although we made our game faster, we didnt reall think much on how to optimize our code.
it is more of a translation cuz we are operating on 4 things at once. So this is a translation that happened
to make our code faster.




3:57
Casey says today we will just focus on the part where we pack pixels for the Framebuffer

	            Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
	            Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
	            Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
	            Blendeda = _mm_mul_ps(One255_4x, Blendeda);

	            for(int I = 0; I < 4; ++I)
	            {
	                if(ShouldFill[I])
	                {
	                    // NOTE(casey): Repack
	                    *(Pixel + I) = (((uint32)(M(Blendeda, I) + 0.5f) << 24) |
	                                    ((uint32)(M(Blendedr, I) + 0.5f) << 16) |
	                                    ((uint32)(M(Blendedg, I) + 0.5f) << 8) |
	                                    ((uint32)(M(Blendedb, I) + 0.5f) << 0));
	                }
	            }
	            
6:33
Casey starting to discuss the strategy to SIMD-ized this portion of the code 

we now have all the r g b for the 4 pixels 

				Blendedr

				128 bits 
				 ____________________________________________________________________________________
				|           r3        |      r2            |      r1            |     r0             |
				|_____________________|____________________|____________________|____________________|


				Blendedg

				128 bits 
				 ____________________________________________________________________________________
				|          g3         |       g2           |        g1          |      g0            |
				|_____________________|____________________|____________________|____________________|


				same for Blendedb and Blendeda



7:51
the pixels in our memory looks like this: 

				R0 G0 B0 A0         R1 G1 B1 A1          R2 G2 B2 A2        R3 G3 B3 A3

and notice all of these are 8 bits 

				R0 G0 B0 A0
				8  8  8  8     32 bits 



8:41
we will be boiling our values that are in the registers from 32 bit down to 8 bits

				128 bits 
				 ____________________________________________________________________________________
				|           r3        |      r2            |      r1            |     r0             |
				|_____________________|____________________|____________________|____________________|


11:30
if we were to load 

				R0 G0 B0 A0         R1 G1 B1 A1          R2 G2 B2 A2        R3 G3 B3 A3

into the our SIMD register, with intel little endian ordering, we will get a regsiter that looks like 

				 ____________________________________________________________________________________
				|     A3 B3 G3 R3     |     A2 B2 G2 R2    |    A1 B1 G1 R1     |    A0 B0 G0 R0     |
				|_____________________|____________________|____________________|____________________|


13:00
with this in mind, our code is gonna look something like this:


	            Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
	            Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
	            Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
	            Blendeda = _mm_mul_ps(One255_4x, Blendeda);

	            *(__m128*) pixels = ....



13:47
the question is literally turning 
				
				RRRR	
				GGGG			into            RGBA RGBA RGBA RGBA 
				BBBB                           (ABGR ABGR ABGR ABGR if you consider little endian order);
				AAAA


15:04
if you actually consider memory on windows, we will actually have 

	BGRA BGRA BGRA BGRA

so in the registers, we want it to look like 

	ARGB ARGB ARGB ARGB


RGBA RGBA RGBA RGBA is standard OpenGL Format 





15:57
start from babysteps. We first have 


				RGBA0 = 
				RGBA1 = 
				RGBA2 = 
				RGBA3 = 




Casey mentioned two intrinsinc functions: _mm_unpackhi_epi32 and _mm_unpacklo_epi32 
https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=_mm_unpack&techs=SSE,SSE2&expand=5969

what these two do is that, they take register values and interleve them depending if you are on the lower half 
or higher half 

for example, if you have these two values and you are doing unpack low 

				R3 R2 R1 R0 and 
				G3 G2 G1 G0


				 Hi      Lo

					  |
				R3 R2 | R1 R0
				G3 G2 | G1 G0
			          |

it first take R0 put them in the first slot
then puts G0 in the 2nd slot 
R1, then G1 

so we got: G1 R1 G0 R0



so our plan is to use these two functions to pack our pixels




20:00
so if weant to produce our ARGB format, we can just starting things like below  

				_mm_unpacklo_epi32(B3 B2 B1 B0, G3 G2 G1 G0) = G1 B1 G0 B0;

				_mm_unpackhi_epi32(B3 B2 B1 B0, G3 G2 G1 G0) = G3 B3 G2 B2;



				_mm_unpacklo_epi32(R3 R2 R1 R0, A3 A2 A1 A0) = A1 R1 A0 R0;

				_mm_unpackhi_epi32(R3 R2 R1 R0, A3 A2 A1 A0) = A3 R3 A2 R2;


so now when we want to combine these values, we have to use 64 bit unpack instead of 32 bit unpack 

recall these all 128 bit values, each poriton is 32 bit: G1 B1 G0 B0;
										
					
example:

			G1 B1 G0 B0;

			A1 R1 A0 R0;

if we use 64 bit unpack, we get 

			A1-R1 G1-B1 A0-R0 G0-B0

21:59
so one way we can do it is to use the _mm_unpackhi_epi64 function				





22:40
But Casey want to show us the other way to do it. 

insted of doing mixing BBBB and GGGG, RRRR and AAAA, 
we mix BBBB and RRRR, AAAA and GGGG:

BBBB and RRRR:
		R1 B1 R0 B0

GGGG and AAAA:
		A1 G1 A0 G0


then we combine 

				_mm_unpacklo_epi32(R1 B1 R0 B0, A1 G1 A0 G0); = A0 R0 G0 B0.


Why the hell did we even begin with the BBBB and GGGG, RRRR and AAAA example then......
shouldnt we derive our approach from our answer?

				one term 

                  \     \ 
				   \     \
				A0 R0 G0 B0 
                /     /
               /     /

               2nd term

if we do this in reverse, isnt that straight forward....
I guess he has to first introduce our _mm_unpacklo_epi32 works....


23:18
so as long as pick our mixing order correctly, we are in good shape





24:47
so now our code looks like:
                {
		            Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
		            Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
		            Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
		            Blendeda = _mm_mul_ps(One255_4x, Blendeda);

		            __m128i R1B1R0B0 = _mm_unpacklo_epi32(Blendedb, Blendedr);
		            __m128i A1G1A0G0 = _mm_unpacklo_epi32(Blendedg, Blendeda);
		        }

26:59
Casey mentioned there is a difference between __m128 and __m128i

it is actually a hint to the compiler, or a guide to the compiler, in terms picking the instructions you want to do 
because there is actually is a different unit in the CPU, for doing the 
interger operation vs the one doing the floating point operations.

Sometimes you lose a cycle here and there between you move between the types

so here we want to cast it into integers 

				{
	                Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
	                Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
	                Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
	                Blendeda = _mm_mul_ps(One255_4x, Blendeda);

	                __m128i R1B1R0B0 = _mm_unpacklo_epi32( _mm_castps_si(Blendedb), _mm_castps_si(Blendedr) );
	                __m128i A1G1A0G0 = _mm_unpacklo_epi32( _mm_castps_si(Blendedg), _mm_castps_si(Blendeda) );
	            }


31:24
Casey wanted show us in the visual studio debugger of what actually happens.
so he just put in random, easy to see values
				
				{
	                uint32 Rs = {0x00000050, 0x00000051, 0x00000052, 0x00000053}
	                uint32 Cs = {0x000000C0, 0x000000C1, 0x000000C2, 0x000000C3}
	                uint32 Bs = {0x000000B0, 0x000000B1, 0x000000B2, 0x000000B3}
	                uint32 As = {0x000000A0, 0x000000A1, 0x000000A2, 0x000000A3}

	                Blendedr = *(__m128 *)&Rs;
	                Blendedg = *(__m128 *)&Gs;
	                Blendedb = *(__m128 *)&Bs;
	                Blendeda = *(__m128 *)&As;

	                __m128i R1B1R0B0 = _mm_unpacklo_epi32( _mm_castps_si(Blendedb), _mm_castps_si(Blendedr) );
	                __m128i A1G1A0G0 = _mm_unpacklo_epi32( _mm_castps_si(Blendedg), _mm_castps_si(Blendeda) );
				}

proceeds to show us that in visual studio debugger, R1B1R0B0 is correct



40:05
after verifying, Casey finished up the code.

				{
	                Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
	                Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
	                Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
	                Blendeda = _mm_mul_ps(One255_4x, Blendeda);

	                __m128i R1B1R0B0 = _mm_unpacklo_epi32( _mm_castps_si(Blendedb), _mm_castps_si(Blendedr) );
	                __m128i A1G1A0G0 = _mm_unpacklo_epi32( _mm_castps_si(Blendedg), _mm_castps_si(Blendeda) );

	                __m128i R3B3R2B2 = _mm_unpackhi_epi32( _mm_castps_si(Blendedb), _mm_castps_si(Blendedr) );
	                __m128i A3G3A2G2 = _mm_unpackhi_epi32( _mm_castps_si(Blendedg), _mm_castps_si(Blendeda) );

	                __m128i ARGB0 = _mm_unpacklo_epi32( R1B1R0B0, A1G1A0G0 );
	                __m128i ARGB1 = _mm_unpackhi_epi32( R1B1R0B0, A1G1A0G0 );

	                __m128i ARGB2 = _mm_unpacklo_epi32( R3B3R2B2, A3G3A2G2 );
	                __m128i ARGB3 = _mm_unpackhi_epi32( R3B3R2B2, A3G3A2G2 );
				}

and Casey showned that in the debugger, we indeed have the right values.
the only problem is they are still in 32 bits (total 128 bits, each lane is 32 bits)

				{
					ARGB3.m128i_u32 = {0xb3b3b3b3, 0xc3c3c3c3, 0x53535353, 0xa3a3a3a3}
				}


41:58
like what Casey said, next thing is that we need to figure out to make this 32 bit floating thing into 
8 bit integers 


				Casey did not really finished, but he justed that you just convert 

				R1B1R0B0, A1G1A0G0 to integers and pack them down
	            
				{
	                __m128i ARGB0 = _mm_unpacklo_epi32( R1B1R0B0, A1G1A0G0 );
	                __m128i ARGB1 = _mm_unpackhi_epi32( R1B1R0B0, A1G1A0G0 );

	                __m128i ARGB2 = _mm_unpacklo_epi32( R3B3R2B2, A3G3A2G2 );
	                __m128i ARGB3 = _mm_unpackhi_epi32( R3B3R2B2, A3G3A2G2 );
	            }

So this is one way to do it, Casey will show us how to do it another way

##############################################################
#################### method 2 ################################
##############################################################

43:03
Casey mentioned the _mm_cvtps_epi32, the instructions that converts a "packed single-precision (32-bit)
floating-point elements to a packed 32-bit integer"

intially, we have this packing operation, where we casted our float to integer.

				{
		            Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
		            Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
		            Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
		            Blendeda = _mm_mul_ps(One255_4x, Blendeda);


		            for(int I = 0; I < 4; ++I)
		            {
		                if(ShouldFill[I])
		                {
		                    // NOTE(casey): Repack
		                    *(Pixel + I) = (((uint32)(M(Blendeda, I) + 0.5f) << 24) |
		                                    ((uint32)(M(Blendedr, I) + 0.5f) << 16) |
		                                    ((uint32)(M(Blendedg, I) + 0.5f) << 8) |
		                                    ((uint32)(M(Blendedb, I) + 0.5f) << 0));
		                }
		            }
            	}


now we have. 
Casey did mention that we need to be aware of how the _mm_cvtps_epi32 do covert.
if it using truncatiion, we still need the 0.5;
if it using nearest, then we dont 
				{
		            Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
		            Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
		            Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
		            Blendeda = _mm_mul_ps(One255_4x, Blendeda);

		            __m128i Intr = _mm_cvtps_epi32(blendedr);
		            __m128i Intg = _mm_cvtps_epi32(blendedg);
		            __m128i Intb = _mm_cvtps_epi32(blendedb);
		            __m128i Inta = _mm_cvtps_epi32(blendeda);
		        }

Unfortunately, there is no way to know unless we set and experiment with it. This is becuz the way the SSE register works
is that they do different things depending on their "mode". The mode can be set to any rounding methodologies. 



46:16
now we want to convert our 32 bit integers and make the into 8 bits. We are only going to use the bottom 
8 bits. note that all the parts before the 8 bits are all 0

				     r3                  r2                     r1                     r0 
				 ____________________________________________________________________________________
				|     0000    |8 bits |    00000   |8 bits |   00000000 |8 bits |   0000000  |8 bits |
				|_____________|_______|____________|_______|____________|_______|____________|_______|

The green channels have the same format 


				     g3                  g2                     g1                     g0 
				 ____________________________________________________________________________________
				|     0000    |8 bits |    00000   |8 bits |   00000000 |8 bits |   0000000  |8 bits |
				|_____________|_______|____________|_______|____________|_______|____________|_______|

so we can just bit shift SIMD to get 
	
				{
		            __m128i Sr = _mm_slli_epi32(Intr, 16);
		            __m128i Sg = _mm_slli_epi32(Intg, 8);
		            __m128i Sb = Intb;
		            __m128i Sa = _mm_slli_epi32(Inta, 24);
		            
		            __m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));
		        }


the bit shift SIMD function we used here is _mm_slli_epi32
https://software.intel.com/sites/landingpage/IntrinsicsGuide/#cats=Shift&techs=SSE,SSE2&expand=5240

the 32 is the width

notice that if you look at all the possible shift function in SSE, SSE2, you will see
all the left shift function is always sll something
				
				__m128i _mm_sll_epi32 (__m128i a, __m128i count)
				__m128i _mm_sll_epi64 (__m128i a, __m128i count)
				__m128i _mm_slli_epi16 (__m128i a, int imm8)
				__m128i _mm_slli_epi32 (__m128i a, int imm8)
				__m128i _mm_slli_epi64 (__m128i a, int imm8)
				__m128i _mm_slli_si128 (__m128i a, int imm8);

so all the shift left are logical. whereas shift right has srl and sra

when you shift left, you will fill it with 0s

when you shift right, there are two things you can fill with, 
either zeros or whatever the high bit is 

so if we have 11100000, you can have either 000111000
or 111111000

the reason they do that is becuz 2s compliement. so if you have a negative number
and if you want to keep the number negative, you keep the first bit negative. 
So the arithmetic bit shift does that.



1:00:31
using this bitshift way, we now have 


				{
		            Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
		            Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
		            Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
		            Blendeda = _mm_mul_ps(One255_4x, Blendeda);

		            // TODO(casey): Should we set the rounding mode to nearest and save the adds?
		            
		            __m128i Intr = _mm_cvtps_epi32(Blendedr);
		            __m128i Intg = _mm_cvtps_epi32(Blendedg);
		            __m128i Intb = _mm_cvtps_epi32(Blendedb);
		            __m128i Inta = _mm_cvtps_epi32(Blendeda);

		            __m128i Sr = _mm_slli_epi32(Intr, 16);
		            __m128i Sg = _mm_slli_epi32(Intg, 8);
		            __m128i Sb = Intb;
		            __m128i Sa = _mm_slli_epi32(Inta, 24);
		            
		            __m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));

		            // TODO(casey): Write only the pixels where ShouldFill[I] == true!
		            _mm_storeu_si128((__m128i *)Pixel, Out);
				}



1:03:02
Casey ran into a "writing to an address that is not aligned" bug. Casey says this is a topic for later.
but he just wants to briefly mention it

On intel processors, this is what happens 

1:04:31
Casey briefly explaning the cause.
essentially our values are 128 bits wide, which is 16 bytes.

Intel processors have instructions to write aligned and write un-aligned.

so if you are writing 16 bytes aligned, it checks by checking the address value. 
The bottom 4 bits of the address should always zero.

(i am a little confused about this. If the bottom 4 bits of an address is 0, shouldnt that be 16 bits aligned?)

Some processors you have to write aligned. but on intel processors, through intrinsincs, you can get around this.



1:19:33

someone mentinoed if it worth it to do all the shading in integer?

Casey says what we did here is we did all the shading calculation in floats. Doing it in integer 
may be slower becuz every multiplication requires a bit shift 

	F x F = F 
if both floats are in 0 ~ 1, our result will be 0 ~ 1

	I x I = I 

if you have int in 0 ~ 255, multiplying another int in 0 ~ 255, our final result will be in 0 ~ 65535
so to prevent it from blowing out the precision of the register, you have to shift it down.

so typically we convert from int the float at the begining of your pipeline,
do all your calculation in float, then shift it back to int. Then we are good.

Doing the pipeline in integers becomes very expensive.


1:25:35
usually you want to keep everything align if you can. Writing aligned is not that difficult anyway..
Also if we are not aligned on 16 byte, that also means we are not cache aligned.


1:30:39
someone in the Q/A mentioned that instead of messing with the rounding mode, you can just use the 
_mm_cvttps_epi32 intrinsics which says 

"convert packed single-precision floating-point elements in a to packed 32-bit integers with truncation"

Casey changed the code using this. 
but he did mention that, we are using an extra addition vs messing with the rounding mode

				{
		            Blendedr = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedr));
		            Blendedg = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedg));
		            Blendedb = _mm_mul_ps(One255_4x, _mm_sqrt_ps(Blendedb));
		            Blendeda = _mm_mul_ps(One255_4x, Blendeda);

		            // TODO(casey): Should we set the rounding mode to nearest and save the adds?
		            
		            __m128i Intr = _mm_cvttps_epi32(_mm_add_ps(Blendedr, Half_4x));
		            __m128i Intg = _mm_cvttps_epi32(_mm_add_ps(Blendedg, Half_4x));
		            __m128i Intb = _mm_cvttps_epi32(_mm_add_ps(Blendedb, Half_4x));
		            __m128i Inta = _mm_cvttps_epi32(_mm_add_ps(Blendeda, Half_4x));

		            __m128i Sr = _mm_slli_epi32(Intr, 16);
		            __m128i Sg = _mm_slli_epi32(Intg, 8);
		            __m128i Sb = Intb;
		            __m128i Sa = _mm_slli_epi32(Inta, 24);
		            
		            __m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));

		            // TODO(casey): Write only the pixels where ShouldFill[I] == true!
		            _mm_storeu_si128((__m128i *)Pixel, Out);
		        }