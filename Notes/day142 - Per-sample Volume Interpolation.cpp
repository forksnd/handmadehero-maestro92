Handmade Hero Day 142 - Per-sample Volume Interpolation

Summary:
described the difference between audio programming and graphics programming. 

emphasized repeated that due to the high frequency demands of audio, we have to do the animation of volume and frequency 
inside the mixer composite loop. 

Wrote the API and functions for people to request volume animation

modified the sound mixing code to handle the animation in volume 

solved a problem of stopping the volume animation at the exact sample 

mentioned the potential of error accumulation in Volume += dVolume that is happening in the sound mixing loop
due to floating point math.

discussed a lerp percentage based approach to solve this problem. But also mentioned that it can be very expensive.

Casey showcasing how to skip an assertion by going into the disassembly during Q/A

mentioned what audio compressor does

Keyword:
volume Interpolation, audio



5:55
we have the concept of Volume in playing_sound
Casey wants to be able to animate the volume over time to achieve effects such as fading in or fading out.

                struct playing_sound
                {
                    real32 Volume[2];
                    sound_id ID;
                    int32 SamplesPlayed;
                    playing_sound *Next;
                };


theres ways that I would do animation in a graphics sense that I wouldnt do for audio sounds. 
theres are a lot of differences between the way sound programming works and graphics programming works. 

the reason is becuz graphics is actually a very low frequency phenomenon.

you are making a frame of information, at most 90 frames per second in a VR scenario, or as low as 30 frames per second. 
common 60 frames. 

audio happens primarily at 48000 times a second. When you think about things happening at that frequency, it leads you to do 
certain things differently. 

For example in the renderer, we tend batch up static sets of things into lists. and then we just draw them.
the renderer itself doesnt really need to know all that much about how things are animating.

that really doesnt work for something like sound mixing. for sound mixing, you are outputing 800 samples per frame, and that means 
we have to architect the audio system and the video system differently. 

typically in sound there are at least a few core animation components that will be sitting directly in the mixer. 
that is becuz theses things have to animate for every frame, but its happening so fast that they have to be done inside the mixer 
composite loops. (composite loop means the loop we are mixing all the playing_sounds.)

whereas in graphics, u tend to the layer above to animate.

what happens in sound mixing, you tend to have something like the animation of volume happening in the mixer.


9:25
in the sound system, volume and frequency animation are gonna be handled in the mixer out of necessity. not cuz it is 
architecturally conveninent for us to do so. 



9:59
Casey changing the playing_sound struct.

we made it so that it has the CurrentVolume and the TargetVolume
CurrentVolume changes towards the TargetVolume

                struct playing_sound
                {
                    real32 CurrentVolume[2];
                    real32 TargetVolume[2];

                    sound_id ID;
                    int32 SamplesPlayed;
                    playing_sound *Next;
                };



11:52
in the OutputPlayingSounds(); where we do the sound mixing, we make the corresponding changes 

initially we had something like below:

                internal void
                OutputPlayingSounds(audio_state *AudioState, game_sound_output_buffer *SoundBuffer, game_assets *Assets, memory_arena *TempArena)
                {
                    ...
                    ...
                    
                    // NOTE(casey): Sum all sounds
                    for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
                        *PlayingSoundPtr;)
                    {
                        ...
                        ...
                        
                        while(TotalSamplesToMix && !SoundFinished)
                        {
                            loaded_sound *LoadedSound = GetSound(Assets, PlayingSound->ID);
                            if(LoadedSound)
                            {
                                ...
                            
                                // TODO(casey): Handle stereo!
                                real32 Volume0 = PlayingSound->Volume[0];
                                real32 Volume1 = PlayingSound->Volume[1];

                                ...
                                ...

                                for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                                    SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                                    ++SampleIndex)
                                {                
                                    real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                                    *Dest0++ += Volume0*SampleValue;
                                    *Dest1++ += Volume1*SampleValue;
                                }


                                ...
                                ...
                            }
                        }

                        ...
                        ...
                    }

                    ...
                    ...
                    
                    EndTemporaryMemory(MixerMemory);
                }


Casey makes so that when we fetch the values for Volume0 and Volume1, it is reading from the PlayingSound->CurrentVolume

                real32 Volume0 = PlayingSound->CurrentVolume[0];
                real32 Volume1 = PlayingSound->CurrentVolume[1];


also inside the while loop, as we mix wav audio into the intermediate buffer,

                *Dest0++ += Volume0*SampleValue;
                *Dest1++ += Volume1*SampleValue;

we want to know the animated volume at each sample count. Becuz we are filling in 800 samples per frame, 
we will actually animate a cross lots of samples. So at each sample it is gonna use a slightly different volume. 





12:07
there are a couple of different ways we can do this. each with pros and cons.

Casey will start off with the most straight forward way first. 


Casey says he declared a ChangeVolume function where people external can make 

                handmade_audio.cpp

                internal void
                ChangeVolume(audio_state *AudioState, playing_sound *Sound, real32 FadeDurationInSeconds, v2 Volume)
                {
                    if(FadeDurationInSeconds <= 0.0f)
                    {
                        Sound->CurrentVolume = Sound->TargetVolume = Volume;
                    }
                    else
                    {
                        real32 OneOverFade = 1.0f / FadeDurationInSeconds;
                        Sound->TargetVolume = Volume;
                        Sound->dCurrentVolume = OneOverFade*(Sound->TargetVolume - Sound->CurrentVolume);
                    }
                }


Casey mentioned that there are two method we can use to specify how fast we want CurrentVolume to change to TargetVolume
1.  specifying the duration
2.  specifying the delta

Casey prefers the duration method cuz he thinks its more intuitive.

Casey designed the API so that if the FadeDurationInSeconds == 0, we want an instantaneous change.
hence the if condition you see in the function.

Casey also added the dCurrentVolume field. 

                struct playing_sound
                {
                    v2 CurrentVolume;
                    v2 dCurrentVolume;
                    v2 TargetVolume;

                    sound_id ID;
                    int32 SamplesPlayed;
                    playing_sound *Next;
                };


as you can see in the ChangeVolume function, we calculate the dCurrentVolume value for the playing_sound.
this dCurrentVolume is the amount of volume change per second 

                                   (TargetVolume - CurrentVolume)
        dCurrentVolume    =     _______________________________________

                                        FadeDurationInSeconds





20:52
back to the OutputPlayingSounds(); function, we want to modify the sound mixing in the while loop to refect the change in volume

-   from the section above, we have volume per second.
    we want volume per samples. 

    so the simple equation, we need seconds per sample 

                Volume              Seconds               Volume 
            _____________   x    _____________   =    ______________

                Seconds             Samples               Samples 


    so we have 48000 samples per second, which is just the inverse of seconds / samples. so we just need to do invert of 
    game_sound_output_buffer.SamplesPerSecond

    and as you can, we declared the SecondsPerSample at the very top of the OutputPlayingSounds(); function.


-   with SecondsPerSample, we get Volume per Sample, which is what the dVolume variable is for.

                v2 dVolume = SecondsPerSample*PlayingSound->dCurrentVolume;


-   in the for loop where we are actually mixing, you can see we do 

                for(uint32 SampleIndex = PlayingSound->SamplesPlayed; SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix); ++SampleIndex)
                { 
                    ...
                    ...
                    Volume += dVolume.
                }

    after every sample we process, we change the volume by volume per sample. Pretty straight forward 


                internal void
                OutputPlayingSounds(audio_state *AudioState, game_sound_output_buffer *SoundBuffer, game_assets *Assets, memory_arena *TempArena)
                {
                    ...
                    ...
                    real32 SecondsPerSample = 1.0f / (real32)SoundBuffer->SamplesPerSecond;

                    ...
                    ...

                    // NOTE(casey): Sum all sounds
                    for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
                        *PlayingSoundPtr;)
                    {
                        ...
                        ...
                        
                        while(TotalSamplesToMix && !SoundFinished)
                        {
                            loaded_sound *LoadedSound = GetSound(Assets, PlayingSound->ID);
                            if(LoadedSound)
                            {
                                ...
                            
                                // TODO(casey): Handle stereo!
                                v2 Volume = PlayingSound->CurrentVolume;
                                v2 dVolume = SecondsPerSample*PlayingSound->dCurrentVolume;




                                ...
                                ...

                                for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                                    SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                                    ++SampleIndex)
                                {                
                                    real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                                    *Dest0++ += Volume0*SampleValue;
                                    *Dest1++ += Volume1*SampleValue;

                                    Volume += dVolume;
                                }
                            
                            }
                        }
                    }

                    ...
                    ...
                    
                    EndTemporaryMemory(MixerMemory);
                }



24:40
Casey also mentioned another problem, which is that Volume += dVolume will kind of run indefinitely, so you kind of need a way to stop it 
if it reaches the TargetVolume.

Casey literally first did the most brute force way also incorrect way.

the correct implementation is to stop Volume += dVolume in side the mixing loop. 

Caseys implementation will overshoot. 

Nevertheless, his code is below:

-   we first do the following check:

                if(dVolume.E[ChannelIndex] != 0.0f)

    that checks whether we are animating or changing the volume                     



-   full code below 

                for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                    SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                    ++SampleIndex)
                {                
                    real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                    *Dest0++ += Volume0*SampleValue;
                    *Dest1++ += Volume1*SampleValue;

                    Volume += dVolume;
                }
                  
                b32 VolumeEnded[AudioStateOutputChannelCount] = {};
                for(... each channel ...)
                {
                    if(dVolume.E[ChannelIndex] > 0.0f)
                    {
                        if(PlayingSound->CurrentVolume >= PlayingSound->TargetVolume)   <----- Caseys actual code loops thru each Channel
                        {                                                                      it made it quite unreadable, so i just removed  
                            PlayingSound->CurrentVolume = PlayingSound->TargetVolume;          the channel accessing code just for here
                            PlayingSound->dCurrentVolume = 0.0f;
                        }
                    }
                    else if if(dVolume.E[ChannelIndex] < 0.0f)
                    {
                        .....................................................
                        ... same thing as above, but in opposte direction ...
                        .....................................................
                    }
                }
                









35:40
Casey attempting to fix the problem of volume changing not stopping where it should stop

so the Caseys idea is that we will clamp the value of SamplesToMix. 
                
recall in the for loop of sound mixing, we are going from 

                for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                    SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                    ++SampleIndex)
                {
                    ...
                }

which we are iterating SamplesToMix number of samples.
Casey is clamping this value to have the volume change stop at where it should


imagine below:

> denotes the samples we want to apply change 

SamplesToMix is initially at 7, since it has 7 aa samples to mix.                 

the TargetVolume will be reached at sample 3, so the SamplesToMix is clamped at 4


                    reach target Volume
                         |
                         |
                         V   

                aa aa aa aa aa aa aa bb bb bb bb

                >  >  >  >  

then recall that we are in a bigger while loop, and we will come inside to this while and process the rest of the samples,





-   so here want see how do we calculate SampleIndex
    we calculate how many samples it is gonna take us to reach TargetVolume from Volume 

            TargetVolume - Volume 
        ______________________________   =   VolumeSampleCount

                  dVolume 

    Note we are using Volume not CurrentVolume. Volume is the volume we are at during the mixing. 
    also note that we have VolumeSampleCount rounded up and truncated..

    and thats where we clamp SamplesToMix to.

    this is what the first for loop is doing. We are doing this for each channel.




-   if the dVolume was finished Ended on this while loop run, we change 

                PlayingSound->CurrentVolume = PlayingSound->TargetVolume;
                PlayingSound->dCurrentVolume = 0.0f;

    so that in the following while loop iteration, we dont apply dVolume anymore.

    this is what we are doing in the third for loop. We do this for each channel.



                while(... ran out of space in output sound buffer ... or playing is done playing )
                {
                    ...
                    ...

                    b32 VolumeEnded[AudioStateOutputChannelCount] = {};
                    for(... each channel ...)
                    {
                        if(dVolume.E[ChannelIndex] != 0.0f)
                        {
                            real32 DeltaVolume = (PlayingSound->TargetVolume.E[ChannelIndex] -
                                                  Volume.E[ChannelIndex]);

                            u32 VolumeSampleCount = (u32)((DeltaVolume / dVolume.E[ChannelIndex]) + 0.5f);
                            if(SamplesToMix > VolumeSampleCount)
                            {
                                SamplesToMix = VolumeSampleCount;
                                VolumeEnded[ChannelIndex] = true;
                            }
                        }
                    }


                    // TODO(casey): Handle stereo!
                    for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                        SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                        ++SampleIndex)
                    {                
                        real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                        *Dest0++ += *Volume.E[0]*SampleValue;
                        *Dest1++ += *Volume.E[1]*SampleValue;
                        
                        Volume += dVolume;
                    }

                    PlayingSound->CurrentVolume = Volume;

                    for(u32 ChannelIndex = 0;
                        ChannelIndex < ArrayCount(VolumeEnded);
                        ++ChannelIndex)
                    {
                        if(VolumeEnded[ChannelIndex])
                        {
                            PlayingSound->CurrentVolume.E[ChannelIndex] =
                                PlayingSound->TargetVolume.E[ChannelIndex];
                            PlayingSound->dCurrentVolume.E[ChannelIndex] = 0.0f;
                        }
                    }
                }





44:22
Casey pointed out another problem with out mixing code. 

one of the problems of doing Volume += inside the loop is that it may cause you to not hit your TargetVolume.
For example if you have a volume change over time of x, and dVolume is x / 1600 for 1600 times.
and this is in floating point math, which is im-percise. So the question is are we accumulating errors when you do this 



                    for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                        SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                        ++SampleIndex)
                    {                
                        real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                        *Dest0++ += *Volume.E[0]*SampleValue;
                        *Dest1++ += *Volume.E[1]*SampleValue;
                        
                        Volume += dVolume;
                    }

so another way we could do is to do this change to multiplication
the way you do that is to figure out where SampleIndex is along the progress towards Target
and then lerp.

So pretty much doing it through percentages 


          PlayingSound->SamplesPlayed                        PlayingSound->SamplesPlayed + SamplesToMix

                |                                                        |
                |                                                        |
                v                                                        v

                aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa aa 

                                           ^
                                           |
                                           |

                                      SampleIndex

The reason you dont want to do that is becuz that requires you to do a reciprocal every samples potentially.
which is very expensive. But these days, CPU are so fast so it might turn out fine.



53:30
Casey showcasing how to skip an assertion by going into the disassembly





1:02:01
someone mentioned in the Q/A whether it makes sense to have the concept MasterVolume

and Casey went on  to do that 

                handmade_audio.h

                struct audio_state
                {
                    memory_arena *PermArena;
                    playing_sound *FirstPlayingSound;
                    playing_sound *FirstFreePlayingSound;

                    v2 MasterVolume;
                };


                handmade_audio.cpp

                // TODO(casey): Handle stereo!
                for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                    SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                    ++SampleIndex)
                {                
                    real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                    *Dest0++ += AudioState->MasterVolume.E[0]*Volume.E[0]*SampleValue;
                    *Dest1++ += AudioState->MasterVolume.E[1]*Volume.E[1]*SampleValue;
                    
                    Volume += dVolume;
                }



1:11:35
what does an audio compressor do?
an audio compressor is something that attempts to squish the range of an audio in an non-linear way. 

we typically squish audio range linearly. Kind of like what HDR does for pixel colors. it maps wave values that are too high 
or too low and maps to a non-linear curve.



