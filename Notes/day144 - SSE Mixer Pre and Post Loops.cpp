Handmade Hero Day 144 - SSE Mixer Pre and Post Loops

Summary:
wrote a few more Align functions

Changed the pre mixing loop and post mixing loop in OutputPlayingSounds();, into SIMD.

pre mixing loop is the loop that loads the loaded sounds into our float intermediate buffer 
post mixing loop is the loop that copies our float intermediate buffer into windows output sound buffer

spent a bunch of time talking about 2s complement

Keyword:
SIMD, Audio. 2s complement



6:15
Casey once again explaining, how align works 

                #define AlignN(Value, N) ((Value + (N-1)) & ~(N-1))

                where N is a power of 2

essentially it rounds your number up to the next closest multiple of N


Casey proceed to wrote a few more Align functions 

                #define AlignPow2(Value, Alignment) ((Value + ((Alignment) - 1)) & ~((Alignment) - 1))
                #define Align4(Value) ((Value + 3) & ~3)
                #define Align16(Value) ((Value + 15) & ~15)

it is not a power 2, u probably have to use a mod. 




7:45
Casey proceeds to make the sound mixing code SIMD 

recall we first created an intermediate buffer, ReadChannel0 and ReadChannel1

                handmade_audio.cpp

                internal void
                OutputPlayingSounds(audio_state *AudioState,
                                    game_sound_output_buffer *SoundBuffer, game_assets *Assets,
                                    memory_arena *TempArena)
                {
                    temporary_memory MixerMemory = BeginTemporaryMemory(TempArena);

                    real32 *RealChannel0 = PushArray(TempArena, SoundBuffer->SampleCount, real32);
                    real32 *RealChannel1 = PushArray(TempArena, SoundBuffer->SampleCount, real32);

                    real32 SecondsPerSample = 1.0f / (real32)SoundBuffer->SamplesPerSecond;

                    ...
                    ...
                }



now we make them in SIMD style 

                handmade_audio.cpp

                internal void
                OutputPlayingSounds(audio_state *AudioState,
                                    game_sound_output_buffer *SoundBuffer, game_assets *Assets,
                                    memory_arena *TempArena)
                {    
                    temporary_memory MixerMemory = BeginTemporaryMemory(TempArena);

                    u32 SampleCountAlign4 = Align4(SoundBuffer->SampleCount);
                    u32 SampleCount4 = SampleCountAlign4 / 4;
                    
                    __m128 *RealChannel0 = PushArray(TempArena, SampleCount4, __m128, 16);
                    __m128 *RealChannel1 = PushArray(TempArena, SampleCount4, __m128, 16);

                    ...
                    ...
                }


8:27
then to clear the buffers, the SIMD code looks like below:
pretty much we are just filling in Zeros

                handmade_audio.cpp

                internal void
                OutputPlayingSounds(audio_state *AudioState,
                                    game_sound_output_buffer *SoundBuffer, game_assets *Assets,
                                    memory_arena *TempArena)
                {
                    ..........................................................................
                    ....... Declaring RealChannel0, RealChannel1 intermediate buffers ........
                    ..........................................................................

                    // NOTE(casey): Clear out the mixer channels
                    __m128 Zero = _mm_set1_ps(0.0f);
                    {
                        __m128 *Dest0 = RealChannel0;
                        __m128 *Dest1 = RealChannel1;
                        for(u32 SampleIndex = 0; SampleIndex < SampleCount4; ++SampleIndex)
                        {
                            _mm_store_ps((float *)Dest0++, Zero);
                            _mm_store_ps((float *)Dest1++, Zero);
                        }
                    }
    




16:48
in integer math, they do this trick called 2s complement to make it simpler on the hardware.
basically you can do all the operations the same regardless whether you are signed or unsigned.  
and the math all works out exactly the same way. 












34:51
Casey then skips the middle loop to look at the conversion 

previously we had the following code 

                {
                    real32 *Source0 = RealChannel0;
                    real32 *Source1 = RealChannel1;

                    int16 *SampleOut = SoundBuffer->Samples;
                    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
                    {
                        // TODO(casey): Once this is in SIMD, clamp!

                        *SampleOut++ = (int16)(*Source0++ + 0.5f);
                        *SampleOut++ = (int16)(*Source1++ + 0.5f);
                    }
                }


now we have 

-   we first load the values 4 at a time from the intermediate buffer 

                __m128 S0 = _mm_load_ps((float *)Source0++);
                __m128 S1 = _mm_load_ps((float *)Source1++);


-   then we convert them from 32 bit floats to 32 bit integers 

                __m128i L = _mm_cvtps_epi32(S0);
                __m128i R = _mm_cvtps_epi32(S1);

which gives us 
                 ___________________    ___________________
                | L0 | L1 | L2 | L3 |  | R0 | R1 | R2 | R3 |
                |____|____|____|____|  |____|____|____|____|

    however, the memory order that windows expect is interleaved 
                 _______________________________________
                | L0 | R0 | L1 | R1 | L2 | R2 | L3 | R3 |
                |____|____|____|____|____|____|____|____|

    so we actually have to interleave it 
    we do that by calling 

                __m128i LR0 = _mm_unpacklo_epi32(L, R);
                __m128i LR1 = _mm_unpackhi_epi32(L, R);
                

-   then finally,, we combine the two and convert them to 16 bit
                                       
                _mm_packs_epi32  
    
    notice in the specs, it says 
    "Convert packed 32-bit integers from a and b to packed 16-bit integers using signed saturation, and store the results in dst."

    particularly the part is says "using signed saturation", which means we dont even have to do a clamp.



                // NOTE(casey): Convert to 16-bit
                {
                    __m128 *Source0 = RealChannel0;
                    __m128 *Source1 = RealChannel1;

                    __m128i *SampleOut = (__m128i *)SoundBuffer->Samples;
                    for(u32 SampleIndex = 0; SampleIndex < SampleCount4; ++SampleIndex)
                    {
                        __m128 S0 = _mm_load_ps((float *)Source0++);
                        __m128 S1 = _mm_load_ps((float *)Source1++);
                        
                        __m128i L = _mm_cvtps_epi32(S0);
                        __m128i R = _mm_cvtps_epi32(S1);
                        
                        __m128i LR0 = _mm_unpacklo_epi32(L, R);
                        __m128i LR1 = _mm_unpackhi_epi32(L, R);
                        
                        __m128i S01 = _mm_packs_epi32(LR0, LR1);

                        *SampleOut++ = S01;
                    }
                }


note that since we are doing this in SIMD, Casey noted that our game_sound_output_buffer must be padded to a multiple of 4 samples.
                
                typedef struct game_sound_output_buffer
                {
                    int SamplesPerSecond;
                    int SampleCount;

                    // IMPORTANT(casey): Samples must be padded to a multiple of 4 samples!
                    int16 *Samples;
                } game_sound_output_buffer;







49:11
Casey doing the trick to debug in SIMD again.

                // NOTE(casey): Convert to 16-bit
                {
                    __m128 *Source0 = RealChannel0;
                    __m128 *Source1 = RealChannel1;

                    __m128i *SampleOut = (__m128i *)SoundBuffer->Samples;
                    for(u32 SampleIndex = 0; SampleIndex < SampleCount4; ++SampleIndex)
                    {

                        __m128i L = _mm_cvtps_epi32(S0);
                        __m128i R = _mm_cvtps_epi32(S1);

                        L = _mm_set_epi(0xA0, 0xA1, 0xA2, 0xA3)
                        R = _mm_set_epi(0xB0, 0xB1, 0xB2, 0xB3)                        

                        __m128i LR0 = _mm_unpacklo_epi32(L, R);
                        __m128i LR1 = _mm_unpackhi_epi32(L, R);
                        
                        __m128i S01 = _mm_packs_epi32(LR0, LR1);

                        *SampleOut++ = S01;
                    }
                }



1:01:57
talked a lot about 2s complement



1:12:42
talked about how floating point works 


        sign     exponent       mantissa
         ____   __________   _________________________________
        | 1  | |    n     | |           m                     |
        |____| |__________| |_________________________________|

                                    ^
                                    |
                                    |
                            1. __________  x 2^(n)

the very first 1. is just given to you in float point representation
the mantissa follows after the 1.________



1:18:27
Casey proceeds to edit build.bat to make the rasterizer code compile in optimized mode, and compile the rest of the game logic code in debug mode.


1:26:06
following up on the 2s complement topic.
if 1111 is less than 0000, how do number comparisons work on the CPU level?

Casey says number comparisons are special cased?

So Casey wrote the following code and analyzed the disassembly code

                int8 A = -100;
                int8 B = 100;

                unsigned int8 C = 200;
                unsigned int8 D = 100;

                bool AB = A < B;
                bool CD = C < D;


essentially, it does a 
                
                cmp 
                jge 
                jmp 

Casey just says regular subtraction works        


turns out its not special cased.

