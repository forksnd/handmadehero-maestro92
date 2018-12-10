Handmade Hero Day 145 - SSE Mixer Main Loop

Summary:
Changing the Main Mixer loop into SIMD
[this episode feels completely chaotic...]

initially planned to do 8 samples at a time becuz he planned load 8 x 16 bit into the 128 bit SSE register.
But if we were to support pitch shifting, which means we cant assume we are advancing 1 sample at atime.
the 8 x 16 bit into the 128 bit SSE register plan no longer works.

Mentioned that tomorrow, he will just change the code back to by 4 instead of by 8

Keyword:
SIMD, audio

2:34
as we are converting the sound mixing loop to SIMD, one thing we would like to think of is having 4 or 8 samples together.
8 samples in 16 bit is 128 samples, which is the size that will fit in full width of the SSE registers 


3:18
so the first thing Casey will do is to make our sound buffer align8.
and since we are making that change, different loops will be working with either SampleCount4 or SampleCount8


                handmade_platform.h 

                #define AlignPow2(Value, Alignment) ((Value + ((Alignment) - 1)) & ~((Alignment) - 1))
                #define Align4(Value) ((Value + 3) & ~3)
                #define Align8(Value) ((Value + 7) & ~7)
                #define Align16(Value) ((Value + 15) & ~15)




                internal void
                OutputPlayingSounds(audio_state *AudioState,
                                    game_sound_output_buffer *SoundBuffer, game_assets *Assets,
                                    memory_arena *TempArena)
                {    
                    temporary_memory MixerMemory = BeginTemporaryMemory(TempArena);

                    u32 SampleCountAlign8 = Align8(SoundBuffer->SampleCount);
                    u32 SampleCount8 = SampleCountAlign8 / 8;
                    u32 SampleCount4 = SampleCountAlign8 / 4;

                    __m128 *RealChannel0 = PushArray(TempArena, SampleCount4, __m128, 16);
                    __m128 *RealChannel1 = PushArray(TempArena, SampleCount4, __m128, 16);

                                    



7:19
so recall in the win32 layer, we allocated memory from windows for the sound buffer

                win32_handmade.cpp 

                int CALLBACK
                WinMain(HINSTANCE Instance,
                        HINSTANCE PrevInstance,
                        LPSTR CommandLine,
                        int ShowCode)
                {



                    ...
                    ...

                    // TODO(casey): Pool with bitmap VirtualAlloc
                    u32 MaxPossibleOverrun = 2*4*sizeof(u16);
                    int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize + MaxPossibleOverrun,
                                                           MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);




                    game_sound_output_buffer SoundBuffer = {};
                    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                    SoundBuffer.Samples = Samples;

                }



11:14
Casey moved the align8 in section 3:18 out to the win32_handmade.cpp layer.             

                win32_handmade.cpp

                int CALLBACK
                WinMain(HINSTANCE Instance,
                        HINSTANCE PrevInstance,
                        LPSTR CommandLine,
                        int ShowCode)
                {
                    ...
                    ...

                    game_sound_output_buffer SoundBuffer = {};
                    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                    SoundBuffer.Samples = Samples;
                    if(Game.GetSoundSamples)
                    {
                        Game.GetSoundSamples(&GameMemory, &SoundBuffer);
                    }

                    ...
                    ...
                }


now we have 


                win32_handmade.cpp

                int CALLBACK
                WinMain(HINSTANCE Instance,
                        HINSTANCE PrevInstance,
                        LPSTR CommandLine,
                        int ShowCode)
                {
                    ...
                    ...

                    game_sound_output_buffer SoundBuffer = {};
                    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    SoundBuffer.SampleCount = Align8(BytesToWrite / SoundOutput.BytesPerSample);
                    BytesToWrite = SoundBuffer.SampleCount*SoundOutput.BytesPerSample;
                    SoundBuffer.Samples = Samples;
                    if(Game.GetSoundSamples)
                    {
                        Game.GetSoundSamples(&GameMemory, &SoundBuffer);
                    }

                    ...
                    ...
                }

then in OutputPlayingSounds(); we got rid of the Align8(); call.

                internal void
                OutputPlayingSounds(audio_state *AudioState,
                                    game_sound_output_buffer *SoundBuffer, game_assets *Assets,
                                    memory_arena *TempArena)
                {    
                    temporary_memory MixerMemory = BeginTemporaryMemory(TempArena);

                    Assert((SoundBuffer->SampleCount & 7) == 8);
                    u32 SampleCount8 = SampleCountAlign8 / 8;
                    u32 SampleCount4 = SampleCountAlign8 / 4;

                    __m128 *RealChannel0 = PushArray(TempArena, SampleCount4, __m128, 16);
                    __m128 *RealChannel1 = PushArray(TempArena, SampleCount4, __m128, 16);


                }




13:08
Casey proposing the idea of making loaded_sound SampleCount stored in batches of 8
in which he added

                struct loaded_sound
                {
                    uint32 SampleCount; // NOTE(casey): This is the sample count divided by 8
                    uint32 ChannelCount;
                    int16 *Samples[2];
                };

so when we load the sound, and we dont have it is not multiples of 8, we want to pad it out.  

so Casey did the following. To preserve the original code as much as possible,
Casey just added an extra bit at the end.

                internal loaded_sound
                DEBUGLoadWAV(char *FileName, u32 SectionFirstSampleIndex, u32 SectionSampleCount)
                {
                    loaded_sound Result = {};
                    
                    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);    
                    if(ReadResult.ContentsSize != 0)
                    {
                        ...
                        ...

                        // TODO(casey): Load right channels!
                        b32 AtEnd = true;
                        Result.ChannelCount = 1;
                        if(SectionSampleCount)
                        {
                            Assert((SectionFirstSampleIndex + SectionSampleCount) <= SampleCount);
                            AtEnd = ((SectionFirstSampleIndex + SectionSampleCount) == SampleCount);
                            SampleCount = SectionSampleCount;
                            for(uint32 ChannelIndex = 0; ChannelIndex < Result.ChannelCount; ++ChannelIndex)
                            {
                                Result.Samples[ChannelIndex] += SectionFirstSampleIndex;
                            }
                        }

                        if(AtEnd)
                        {
                            for(uint32 ChannelIndex = 0; ChannelIndex < Result.ChannelCount; ++ChannelIndex)
                            {
                                for(u32 SampleIndex = SampleCount; SampleIndex < (SampleCount + 8); ++SampleIndex)
                                {
                                    Result.Samples[ChannelIndex][SampleIndex] = 0;
                                }
                            }
                        }

                        Result.SampleCount = SampleCount;
                    }

                    return(Result);
                }




19:28
Casey moving on to change the mixing loop into SIMD. 







19:55
now we want to examine whole OutputPlayingSounds(); to see how do we change it so that we can process the main loop 8 samples at a time.

-   Casey first changed the 
                
                internal void OutputPlayingSounds(...)
                {    
                    ...

                    u32 SampleCountAlign4 = Align4(SoundBuffer->SampleCount);
                    u32 SampleCount4 = SampleCountAlign4 / 4;

                    for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
                        *PlayingSoundPtr;
                        )
                    {
                        while(TotalSamplesToMix && !SoundFinished)
                        {
                            ...
                        }
                    }
    to

                internal void OutputPlayingSounds(...)
                {    
                    ...

                    Assert((SoundBuffer->SampleCount & 7) == 0);
                    u32 SampleCount8 = SoundBuffer->SampleCount / 8;
                    u32 SampleCount4 = SoundBuffer->SampleCount / 4;

                    ...

                    for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
                        *PlayingSoundPtr;
                        )
                    {
                        while(TotalSamplesToMix8 && !SoundFinished)
                        {
                            ...
                        }
                    }

    recall that the SoundBuffer is aligned 8, so SampleCount8 is just gonna be the number of chunks. Each chunks is 8 samples.
    for the main while loop, we plan to do 8 samples at a time. Essentially the while loop is looping through chunks.



20:17 29:10
next, we go inside the while loop to deal with each sample of a chunk

previously, our structure was such that, we first determine the SamplesToMix variable. 
this is becuz we have pitch shifting, so we may not be advancing 1 sample at a time. 
We need to calculate how many samples we are mixing with this dSampleSpeed.
this is explained more detailed in day 143

                while(TotalSamplesToMix && !SoundFinished)
                {
                    ...........................................
                    ....... Determining SamplesToMix ..........
                    ...........................................
                    
                    // TODO(casey): Handle stereo!
                    real32 SamplePosition = PlayingSound->SamplesPlayed;
                    for(u32 LoopIndex = 0;
                        LoopIndex < SamplesToMix;
                        ++LoopIndex)
                    {
    #if 1
                        u32 SampleIndex = FloorReal32ToInt32(SamplePosition);
                        r32 Frac = SamplePosition - (r32)SampleIndex;
                        r32 Sample0 = (r32)LoadedSound->Samples[0][SampleIndex];
                        r32 Sample1 = (r32)LoadedSound->Samples[0][SampleIndex + 1];
                        r32 SampleValue = Lerp(Sample0, Frac, Sample1);
    #else                    
                        u32 SampleIndex = RoundReal32ToInt32(SamplePosition);
                        real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
    #endif
                        
                        *Dest0++ += AudioState->MasterVolume.E[0]*Volume.E[0]*SampleValue;
                        *Dest1++ += AudioState->MasterVolume.E[1]*Volume.E[1]*SampleValue;
                        
                        Volume += dVolume;
                        SamplePosition += dSample;
                    }



now that we are doing chunks of 8, we have to determine the number of SamplesToMix8
SamplesToMix8 is gonna be the number of chunks we are gonna do. Again each chunk is 8 samples 

-   also notice that since inside the main for loop, we are doing chunks of 8,
we will also increment Volume and SamplePosition by dVolume8 and dSample8 respectively.

                // NOTE(casey): Sum all sounds
                for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
                    *PlayingSoundPtr;
                    )
                {
                    playing_sound *PlayingSound = *PlayingSoundPtr;
                    bool32 SoundFinished = false;

                    uint32 TotalSamplesToMix8 = SampleCount8;
                    __m128 *Dest0 = RealChannel0;
                    __m128 *Dest1 = RealChannel1;
                                                    
                    while(TotalSamplesToMix8 && !SoundFinished)
                    {

                        v2 Volume = PlayingSound->CurrentVolume;
                        v2 dVolume = SecondsPerSample*PlayingSound->dCurrentVolume;
                        v2 dVolume8 = 8.0f*dVolume;
                        real32 dSample = PlayingSound->dSample;
                        real32 dSample8 = 8.0f*dSample;


                        ...........................................
                        ....... Determining SamplesToMix8 ..........
                        ...........................................
                            
                        // TODO(casey): Handle stereo!
                        real32 SamplePosition = PlayingSound->SamplesPlayed;
                        for(u32 LoopIndex = 0;
                            LoopIndex < SamplesToMix8;
                            ++LoopIndex)
                        {
                            ...
                            ...

                            Volume += dVolume8;
                            SamplePosition += dSample8;
                        }

21:03
Casey does mention a problem where the pitch shifting might occur in middle of your chunks 
so if we want to support variable sample rate, we cant have our samples nicely aligned reads 

recall what we are doing is to copy the loaded_sound samples Buffer into our intermediate buffer, then we 
interleave it to copy into the output buffer.

lets say our loaded_sound has a sample buffer, like below, and I drew a line at every sample 

if we are playing 1 sample at a time, what might happen is that when do our mixing in our main for loop
the first iteration may only touch sample 0 ~ 7 
the 2nd iteration will only touch sample 8 ~ 15

so then we can just load sample 0 ~ 7 to an SSE register, then we copied it to the intermediate buffer, 
then finally to the windows output sound buffer.
that way, we are doing an aligned read on the loaded_sound.Samples[0] or loaded_sound.Samples[1]
every thing aligns perfectly.


                loaded_sound 


                                128 bit                                  128 bit

Samples                                                 |
                 _______________________________________|_______________________________________
                |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 |
                |____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|
                                                        |
                                                        |
                    |
                    |
                    V

                intermediate buffer
                 _______________________________________
                | L0 | L1 | L2 | L3 | L4 | L5 | L6 | L7 |
                |____|____|____|____|____|____|____|____|

                 _______________________________________
                | R0 | R1 | R2 | R3 | R4 | R5 | R6 | R7 |
                |____|____|____|____|____|____|____|____|


                    |
                    |
                    V

                output buffer
                 _______________________________________
                | L0 | R0 | L1 | R1 | L2 | R2 | L3 | R3 |
                |____|____|____|____|____|____|____|____|


however, lets say we have we want to play it at 7/8 speed,
then what might happen is that when do our mixing in our main for loop

so then we can just load sample 0 ~ 7 to an SSE register,
the first iteration may only touch sample 0 ~ 6

for the 2nd iteration, it has to start on sample 7, so we will have to load sample 7 ~ 14 to an SSE register,

if this happens way, we are no longer doing aligned read on the loaded_sound.Samples[0] or loaded_sound.Samples[1]

                loaded_sound 
                                128 bit                                  128 bit
                                                        |
                 _______________________________________|_______________________________________
                |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 |
                |____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|
                                                        |
                                                        
                |.......................................|
                                                   |.......................................|



27:37
essentially Casey reiterated that in the main mixing loop
that we will have to load unaligned from the LoadedSound.Samples 
we do so cuz we want to support variable sample rate.


                real32 SamplePosition = PlayingSound->SamplesPlayed;
                for(u32 LoopIndex = 0;
                    LoopIndex < SamplesToMix8;
                    ++LoopIndex)
                {

                    u32 SampleIndex = FloorReal32ToInt32(SamplePosition8 * 8.0)  <------- this may not start at multiples of 8. unaligned reads
                    ...
                    ...
                }






33:42
Casey mentioning another problem, the similar problem we saw when we did the bilinear filtering 
recall that in bilinear filtering, we inserted a layer of 

lets say we have loaded_piece 1 and loaded_piece 2. 
assume you have sample A at the end of loaded_piece 1, due to the way we set up, we will have padding 
at the end of loaded_piece 1. 

so assume a sample required us to blend A and B. we need a way to skip all that padded zeros.


                loaded piece 1                              loaded piece 2

                                                        |
                 _______________________________________|_______________________________________
                |    |    |    |    |  A |  0 |  0 |  0 |  B |    |    |    |    |    |    |    |
                |____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|
                                                        |
                                                        |


so what we want to do is to replace the zeros with B 


               loaded piece 1                              loaded piece 2

                                                        |
                 _______________________________________|_______________________________________
                |    |    |    |    |  A |  B |  B |  B |  B |    |    |    |    |    |    |    |
                |____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|
                                                        |
                                                        |











40:05
Casey moving on to change the main loop to SIMD.
When Casey sets up the code to do things in SIMD, its always seems to be a 2 step process.
step 1 is to write the in to the double for loop. the 
step 2 is to replace the inner for loop into SIMD 

For example 

so originally, we are advancing one sample at a time. In the main mixing loop 


                // TODO(casey): Handle stereo!
                real32 SamplePosition = PlayingSound->SamplesPlayed;
                for(u32 LoopIndex = 0;
                    LoopIndex < SamplesToMix;
                    ++LoopIndex)
                {
#if 1
                    u32 SampleIndex = FloorReal32ToInt32(SamplePosition);
                    r32 Frac = SamplePosition - (r32)SampleIndex;
                    r32 Sample0 = (r32)LoadedSound->Samples[0][SampleIndex];
                    r32 Sample1 = (r32)LoadedSound->Samples[0][SampleIndex + 1];
                    r32 SampleValue = Lerp(Sample0, Frac, Sample1);
#else                    
                    u32 SampleIndex = RoundReal32ToInt32(SamplePosition);
                    real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
#endif
                    
                    *Dest0++ += AudioState->MasterVolume.E[0]*Volume.E[0]*SampleValue;
                    *Dest1++ += AudioState->MasterVolume.E[1]*Volume.E[1]*SampleValue;
                    
                    Volume += dVolume;
                    SamplePosition += dSample;
                }


since now we are doing are trying to process 8 16bit samples in the registers, so we are doing 8 at a time.
So casey makes it into a double for loop 



                // TODO(casey): Handle stereo!
                for(u32 LoopIndex = 0;
                    LoopIndex < SamplesToMix8;
                    ++LoopIndex)
                {
                    for(u32 SampleOffset = 0;
                        SampleOffset < 8
                        ++SampleOffset)
                    {
    #if 1
                        real32 OffsetSamplePosition = SamplePosition + SampleOffset * dSample;
                        u32 SampleIndex = FloorReal32ToInt32(OffsetSamplePosition);
                        r32 Frac = OffsetSamplePosition - (r32)SampleIndex;

                        r32 Sample0 = (r32)LoadedSound->Samples[0][SampleIndex];
                        r32 Sample1 = (r32)LoadedSound->Samples[0][SampleIndex + 1];
                        r32 SampleValue = Lerp(Sample0, Frac, Sample1);
    #else                    
                        u32 SampleIndex = RoundReal32ToInt32(SamplePosition);
                        real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
    #endif
                        
                        *Dest0++ += AudioState->MasterVolume.E[0]*Volume.E[0]*SampleValue;
                        *Dest1++ += AudioState->MasterVolume.E[1]*Volume.E[1]*SampleValue;
                    }

                    Volume += dVolume8;
                    SamplePosition += dSample8;
                }

Then Casey checks if the code runs fine.




41:27
then he goes on to do step2, which is to change the inner for loop into SIMD.
he first starts doing it for nearest sample case

-   so recall we are doing 8 samples at a time, so we are load into the intermediate buffer as 2 sets of 4 pixels 

    lets take channel 0 as an example. 
    we first call

                __m128 D0_0 = _mm_load_ps((float *)&Dest0[0]);
                __m128 D0_1 = _mm_load_ps((float *)&Dest0[1]);

                Dest0[0]            Dest0[1]
                  |                   |
                  |                   |
                  v                   v
                 _______________________________________
                | L0 | L1 | L2 | L3 | L4 | L5 | L6 | L7 |
                |____|____|____|____|____|____|____|____|

    D0_0 takes care of L0, L1, L2 and L3 
    D0_1 takes care of L4, L5, L6 and L7 

    then we store it back into the intermediate buffer. Also notice that 
    in the intermediate buffer, we can store it aligned.

                _mm_store_ps((float *)&Dest0[0], D0_0);
                _mm_store_ps((float *)&Dest0[1], D0_1);

    hence the 2 calls.

    we repeat it for channel 1.


-   previously, we did the following to our samples 

                *Dest0++ += AudioState->MasterVolume.E[0]*Volume.E[0]*SampleValue;

    the SIMD equivalent is 
                
                D0_0 = _mm_add_ps(D0_0, _mm_mul_ps(_mm_mul_ps(MasterVolume4_0, Volume4_0), SampleValue_0));
                D0_1 = _mm_add_ps(D0_1, _mm_mul_ps(_mm_mul_ps(MasterVolume4_0, _mm_add_ps(dVolume4_0, Volume4_0)), SampleValue_1));

-   we call at the end of the loop 

                Dest0 += 2;
                Dest1 += 2;

    becuz Dest0 is declared as 

                __m128 *Dest0 = RealChannel0;
                __m128 *Dest1 = RealChannel1;
                

    so Dest0 += 2, advances it by 256 bits. which is 8 samples. We are doing 8 pixels each iteration.

                Dest0               Dest0 + 1          Dest0 + 2 
                  |                   |                   |
                  |                   |                   |
                  v                 | v                 | v                 |
                 ___________________|___________________|___________________|___________________
                | L0 | L1 | L2 | L3 | L4 | L5 | L6 | L7 | L0 | L1 | L2 | L3 | L4 | L5 | L6 | L7 |
                |____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|____|
                                    |                   |                   |
                                    |                   |                   |

-   i suspect D1_0 and D1_1is initalized incorrectly.
    
    it should be 
                __m128 SampleValue_1_0 = _mm_setr_ps(LoadedSound->Samples[1][RoundReal32ToInt32(SamplePosition + 0.0f*dSample)],
                                                   LoadedSound->Samples[1][RoundReal32ToInt32(SamplePosition + 1.0f*dSample)],
                                                   LoadedSound->Samples[1][RoundReal32ToInt32(SamplePosition + 2.0f*dSample)],
                                                   LoadedSound->Samples[1][RoundReal32ToInt32(SamplePosition + 3.0f*dSample)]);
                __m128 SampleValue_1_1 = _mm_setr_ps(LoadedSound->Samples[1][RoundReal32ToInt32(SamplePosition + 4.0f*dSample)],
                                                   LoadedSound->Samples[1][RoundReal32ToInt32(SamplePosition + 5.0f*dSample)],
                                                   LoadedSound->Samples[1][RoundReal32ToInt32(SamplePosition + 6.0f*dSample)],
                                                   LoadedSound->Samples[1][RoundReal32ToInt32(SamplePosition + 7.0f*dSample)]);

                D1_0 = _mm_add_ps(D1_0, _mm_mul_ps(_mm_mul_ps(MasterVolume4_1, Volume4_1), SampleValue_1_0));
                D1_1 = _mm_add_ps(D1_1, _mm_mul_ps(_mm_mul_ps(MasterVolume4_1, _mm_add_ps(dVolume4_1, Volume4_1)), SampleValue_1_1));



-   full code below:

                // TODO(casey): Handle stereo!
                for(u32 LoopIndex = 0;
                    LoopIndex < SamplesToMix8;
                    ++LoopIndex)
                {

#if 0
                    real32 OffsetSamplePosition = SamplePosition + (r32)SampleOffset*dSample;
                    u32 SampleIndex = FloorReal32ToInt32(OffsetSamplePosition);
                    r32 Frac = OffsetSamplePosition - (r32)SampleIndex;

                    r32 Sample0 = (r32)LoadedSound->Samples[0][SampleIndex];
                    r32 Sample1 = (r32)LoadedSound->Samples[0][SampleIndex + 1];
                    r32 SampleValue = Lerp(Sample0, Frac, Sample1);
#else                    
                    __m128 SampleValue_0 = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 0.0f*dSample)],
                                                       LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 1.0f*dSample)],
                                                       LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 2.0f*dSample)],
                                                       LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 3.0f*dSample)]);
                    __m128 SampleValue_1 = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 4.0f*dSample)],
                                                       LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 5.0f*dSample)],
                                                       LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 6.0f*dSample)],
                                                       LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 7.0f*dSample)]);
#endif

                    __m128 D0_0 = _mm_load_ps((float *)&Dest0[0]);
                    __m128 D0_1 = _mm_load_ps((float *)&Dest0[1]);
                    __m128 D1_0 = _mm_load_ps((float *)&Dest1[0]);
                    __m128 D1_1 = _mm_load_ps((float *)&Dest1[1]);

                    D0_0 = _mm_add_ps(D0_0, _mm_mul_ps(_mm_mul_ps(MasterVolume4_0, Volume4_0), SampleValue_0));
                    D0_1 = _mm_add_ps(D0_1, _mm_mul_ps(_mm_mul_ps(MasterVolume4_0, _mm_add_ps(dVolume4_0, Volume4_0)), SampleValue_1));
                    D1_0 = _mm_add_ps(D1_0, _mm_mul_ps(_mm_mul_ps(MasterVolume4_1, Volume4_1), SampleValue_0));
                    D1_1 = _mm_add_ps(D1_1, _mm_mul_ps(_mm_mul_ps(MasterVolume4_1, _mm_add_ps(dVolume4_1, Volume4_1)), SampleValue_1));

                    _mm_store_ps((float *)&Dest0[0], D0_0);
                    _mm_store_ps((float *)&Dest0[1], D0_1);
                    _mm_store_ps((float *)&Dest1[0], D1_0);
                    _mm_store_ps((float *)&Dest1[1], D1_1);

                    Dest0 += 2;
                    Dest1 += 2;
                    Volume4_0 = _mm_add_ps(Volume4_0, dVolume84_0);
                    Volume4_1 = _mm_add_ps(Volume4_1, dVolume84_1);
                    Volume += dVolume8;
                    SamplePosition += dSample8;
                }




59:45
Casey goes on to edit and SIMDized the code that sets up our main for loop

-   recall that we are feeding dVolume8 and Volume4_0 into our "mixer for loop"
    so when we construct Volume4_0, it needs to be pre multipled with dVolume

                __m128 Volume4_0 = _mm_setr_ps(Volume.E[0] + 0.0f*dVolume.E[0],
                                               Volume.E[0] + 1.0f*dVolume.E[0],
                                               Volume.E[0] + 2.0f*dVolume.E[0],
                                               Volume.E[0] + 3.0f*dVolume.E[0]);



-   full code below:

                while(TotalSamplesToMix8 && !SoundFinished)
                {
                    loaded_sound *LoadedSound = GetSound(Assets, PlayingSound->ID);
                    if(LoadedSound)
                    {
                        asset_sound_info *Info = GetSoundInfo(Assets, PlayingSound->ID);
                        PrefetchSound(Assets, Info->NextIDToPlay);

                        v2 Volume = PlayingSound->CurrentVolume;
                        v2 dVolume = SecondsPerSample*PlayingSound->dCurrentVolume;
                        v2 dVolume8 = 8.0f*dVolume;
                        real32 dSample = PlayingSound->dSample;
                        real32 dSample8 = 8.0f*dSample;

                        __m128 MasterVolume4_0 = _mm_set1_ps(AudioState->MasterVolume.E[0]);
                        __m128 MasterVolume4_1 = _mm_set1_ps(AudioState->MasterVolume.E[1]);
                        __m128 Volume4_0 = _mm_setr_ps(Volume.E[0] + 0.0f*dVolume.E[0],
                                                       Volume.E[0] + 1.0f*dVolume.E[0],
                                                       Volume.E[0] + 2.0f*dVolume.E[0],
                                                       Volume.E[0] + 3.0f*dVolume.E[0]);
                        __m128 dVolume4_0 = _mm_set1_ps(dVolume.E[0]);
                        __m128 dVolume84_0 = _mm_set1_ps(dVolume8.E[0]);
                        __m128 Volume4_1 = _mm_setr_ps(Volume.E[1] + 0.0f*dVolume.E[1],
                                                       Volume.E[1] + 1.0f*dVolume.E[1],
                                                       Volume.E[1] + 2.0f*dVolume.E[1],
                                                       Volume.E[1] + 3.0f*dVolume.E[1]);
                        __m128 dVolume4_1 = _mm_set1_ps(dVolume.E[1]);
                        __m128 dVolume84_1 = _mm_set1_ps(dVolume8.E[1]);

                        ...
                        ...

                        .................................................................
                        ................... our mixer for loop ..........................
                        .................................................................
                
                        ...
                        ...
                    }
                
                    ...
                    ...
                }


1:07:21
Casey decided that go through the code, remove the by 8 and just do it by 4.
Casey initially thought we could do it by 8 is becuz he thought we could load 8 at a time out of the 16 bit sample buffer.

essentially this code.

                __m128 SampleValue_0 = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 0.0f*dSample)],
                                                   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 1.0f*dSample)],
                                                   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 2.0f*dSample)],
                                                   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 3.0f*dSample)]);
                __m128 SampleValue_1 = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 4.0f*dSample)],
                                                   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 5.0f*dSample)],
                                                   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 6.0f*dSample)],
                                                   LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 7.0f*dSample)]);


