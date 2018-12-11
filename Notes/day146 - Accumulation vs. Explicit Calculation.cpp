Handmade Hero Day 146 - Accumulation vs. Explicit Calculation

Summary:

fixed some small details in OutputPlayingSounds regarding LoadedSound looping and streaming.

followed up on day 145, changed the main mixer loop from by 8 samples to by 4 samples

discussed the rounding error differences between using Accumulation vs. Explicit Calculation 

compared the assembly code of our SIMD clear zero vs C memset in Q/A

talked about whats wrong OOP in Q/A again

Keyword:
Audio, code clean up.


3:20
Casey mentioned a small detail in the code.
in our OutputPlayingSounds(); whenever we reach the condition 

                if((uint32)PlayingSound->SamplesPlayed >= LoadedSound->SampleCount)

if the NextIDToPlay is valid, we will continue to play that sound.

Casey mentioned that if we ever want to support looping, we can just have the LoadedSound point to himself. 

                handmade_audio.cpp

                internal void
                OutputPlayingSounds(audio_state *AudioState,
                                    game_sound_output_buffer *SoundBuffer, game_assets *Assets,
                                    memory_arena *TempArena)
                {    
                  
                    // NOTE(casey): Sum all sounds
                    for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
                        *PlayingSoundPtr;
                        )
                    {
                        ...
                        ...

                        while(TotalSamplesToMix8 && !SoundFinished)
                        {        
                            ...

                            if((uint32)PlayingSound->SamplesPlayed >= LoadedSound->SampleCount)
                            {
                                if(IsValid(Info->NextIDToPlay))
                                {
                                    PlayingSound->ID = Info->NextIDToPlay;
                                    PlayingSound->SamplesPlayed = 0;                    
                                }
                                else
                                {
                                    SoundFinished = true;
                                }
                            }

                        }

                    }

                }



5:46
another detail about the same snippet of code. 

                if((uint32)PlayingSound->SamplesPlayed >= LoadedSound->SampleCount)
                {
                    if(IsValid(Info->NextIDToPlay))
                    {
                        PlayingSound->ID = Info->NextIDToPlay;
                        PlayingSound->SamplesPlayed = 0;                    
                    }
                    else
                    {
                        SoundFinished = true;
                    }
                }

when SamplesPlayed runs over to the SamplesCount, and its time to go to the next sound. 
Setting PlayingSound->SamplesPlayed = 0; is only correct if we actually ended right on a sample boundary. 
which is going to be the case for 1 to 1 play back perhaps. But it will not be the case if we are doing some kind of pitch bend.
so what we rather have is 

                PlayingSound->SamplesPlayed -= LoadedSound->SampleCount;  

so it leaves any fractional leftover that was in there.

                if((uint32)PlayingSound->SamplesPlayed >= LoadedSound->SampleCount)
                {
                    if(IsValid(Info->NextIDToPlay))
                    {
                        PlayingSound->ID = Info->NextIDToPlay;
                        PlayingSound->SamplesPlayed -= LoadedSound->SampleCount;                    
                    }
                    else
                    {
                        SoundFinished = true;
                    }
                }




7:33
Casey proceeds to change the main mixer loop only process samples 4 at a time.

Casey also did some renaming. He replaced the 4 suffix with chunks.

Forexample
                SampleCount4 ----> ChunkCount
or 

                v2 Volume = PlayingSound->CurrentVolume;
                v2 dVolume = SecondsPerSample*PlayingSound->dCurrentVolume;
                v2 dVolume8 = 8.0f*dVolume;

                        |
                        |
                        V

                v2 Volume = PlayingSound->CurrentVolume;
                v2 dVolume = SecondsPerSample*PlayingSound->dCurrentVolume;
                v2 dVolumeChunk = 4.0f*dVolume;

or 

                TotalSamplesToMix8 -----> TotalChunksToMix

                ...
                ...


14:47
Casey now touching the mixing for loop 
a couple of things 
-   since we are now doing 4 pixels at a time 
                
                Dest0 += 2;
                Dest1 += 2;

is now 

                Dest0++;
                Dest1++;

[go read the day146 code. if you followed episode day 145, All the code changes here should be very straight forward.]




21:59
Casey trying to put the lerp section in the main mixing for loop back in 


the code should be straightforward. 

                for(u32 LoopIndex = 0;
                    LoopIndex < ChunksToMix;
                    ++LoopIndex)
                {
                    real32 SamplePosition = BeginSamplePosition + LoopIndexC*(r32)LoopIndex;
                    // TODO(casey): Move volume up here to explicit.
#if 1
                    __m128 SamplePos = _mm_setr_ps(SamplePosition + 0.0f*dSample,
                                                   SamplePosition + 1.0f*dSample,
                                                   SamplePosition + 2.0f*dSample,
                                                   SamplePosition + 3.0f*dSample);
                    __m128i SampleIndex = _mm_cvttps_epi32(SamplePos);
                    __m128 Frac = _mm_sub_ps(SamplePos, _mm_cvtepi32_ps(SampleIndex));
                    
                    __m128 SampleValueF = _mm_setr_ps(LoadedSound->Samples[0][((int32 *)&SampleIndex)[0]],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[1]],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[2]],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[3]]);
                    __m128 SampleValueC = _mm_setr_ps(LoadedSound->Samples[0][((int32 *)&SampleIndex)[0] + 1],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[1] + 1],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[2] + 1],
                                                      LoadedSound->Samples[0][((int32 *)&SampleIndex)[3] + 1]);

                    __m128 SampleValue = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(One, Frac), SampleValueF),
                                                    _mm_mul_ps(Frac, SampleValueC));
#else                    
                    __m128 SampleValue = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 0.0f*dSample)],
                                                     LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 1.0f*dSample)],
                                                     LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 2.0f*dSample)],
                                                     LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 3.0f*dSample)]);
#endif

                    ...
                    ...
                }           
    


Casey spent a bunch of time polishing and fixing bugs in the sound mixer code 


50:20
Casey explaining Accumulation vs Explicit/Inductive


there are two methods 
1.  f(i) = f(0) + i * deltaA 
2.  f(i) = f(i-1) + deltaA 
    

in terms of our code, we have 

(method 1, our new version)
                real32 BeginSamplePosition = PlayingSound->SamplesPlayed;
                ...
                real32 LoopIndexC = (EndSamplePosition - BeginSamplePosition) / (r32)ChunksToMix;
                for(u32 LoopIndex = 0;
                    LoopIndex < ChunksToMix;
                    ++LoopIndex)
                {
                    real32 SamplePosition = BeginSamplePosition + LoopIndexC*(r32)LoopIndex;

                    ...
                    ...
                }


vs 

(method 2, version day 145 changed to by 4 samples 
    )
                real32 SamplePosition = PlayingSound->SamplesPlayed;
                for(u32 LoopIndex = 0;
                    LoopIndex < SamplesToMix4;
                    ++LoopIndex)
                {
                    ...
                    ...
                    SamplePosition += dSample4;
                }


since we are doing floats, everytime you do an addition with the inductive method (method2);, you can have a rounding error 

if you use method 2, you will be gathering a lot of rounding error, especially if you are looping a bunch of interations. 
if you use method 1, the rounding error can only come from the addition, the multiplication

so method 1 just have 2 rounding errors where as method 2 has the "loop count" number of rounding errors. 



58:54
someone in the Q/A asked
is your SIMD clear to zero faster than memset?

Casey says he would assume they would be the same speed?
Ours might be slightly faster cuz we know ours is aligned
memset might have to test to see whether you address and your writes are aligned?

Casey wrote the two code below and proceeds to examine the assembly instructions

                // NOTE(casey): Clear out the mixer channels
                __m128 Zero = _mm_set1_ps(0.0f);
                {
                    __m128 *Dest0 = RealChannel0;
                    __m128 *Dest1 = RealChannel1;
                    for(u32 SampleIndex = 0;
                        SampleIndex < SampleCount4;
                        ++SampleIndex)
                    {
                        _mm_store_ps((float *)Dest0++, Zero);
                        _mm_store_ps((float *)Dest1++, Zero);
                    }
                }
                

                memset(Dest0, 0, ChunkCount*sizeof(__128));
                memset(Dest1, 0, ChunkCount*sizeof(__128));



1:31:26
I understand struct as bags of data instead of objects. OO is the wrong way to thing about programming.
the problem with object oriented is not the "object" part, but the "oriented" part.
