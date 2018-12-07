Handmade Hero Day 143 - Pitch Shifting in the Mixer

Summary:
as mentioned in day 139, we will be doing non-length preserving pitch shifting.
discussed how do we plan to do so.

wrote the code to do pitch shifting by changing play speed.

demonstrated this techinique using both nearest sample and lerping methods. 

demonstrating how to pan volume based on mouse position

briefly mentioned cores and threads in the Q/A

Keyword:
pitch shifting, audio, threads, core

1:57 
pretty described what are the use case of volume shifting and Pitch shifting. 
If you have one let say foot steps audio sample, adjusting volume and pitch gives you variety without additional memory footprint.



9:41
as mentioned in day 139, there is length preserving  pitch shifting and non-length preserving pitch shifting. We are gonna do the 
non-length preserving pitch shifting. So the idea is to just to play the sound slightly faster or slower.




The challenge though is that, we current have our wav in digit form, so you can imagine the real physical sound as a continuous wave, 
we have only have data point sampled on the wave at regular intervals.

below we have A B C D E F G H, and we play these notes in our sound output buffer

         |      |       |       |      |       |      |     |            
         |      B.      |       |      |       |      |    .H            
         |   .  |   .   |       |      |       |      | .   |  .
         | .    |     . |       |      |       |      G     |    .
         A      |       C       |      |       |    . |     |      .
       . |      |       |.      |      |       |   .  |     |        .
      .  |      |       | .     |      |       |  .   |     |         
    -----|------|-------|-------|------|-------|------|-----|---------------------->
         |      |       |   .   |      |       |.     |     | 
         |      |       |    .  |      |       F      |     |
         |      |       |     . |      |     . |      |     |
         |      |       |       D      |   .   |      |     |
         |      |       |       | .    | .     |      |     |
         |      |       |       |    . E       |      |     |
                                     
so for us, if we want to play it faster or slower, we dont actually have the original waves


so for us to play it faster, we will just sample points at bigger intervals

that way we get less sample points, but we finish playing the entire sound wave in a shorter time 
effectively playing the sound faster

below we will have a b c d e, and we play these notes in our sound output buffer
     
         |            |           |            |            |                
         |      ..    |           |            |           .e            
         |   .      . |           |            |        .   |  .
         | .          b           |            |      .     |    .
         a            | .         |            |    .       |      .
       . |            |  .        |            |   .        |        .
      .  |            |   .       |            |  .         |         .
    -----|------------|-----------|------------|------------|---------------------->
         |            |     .     |            |.           |
         |            |      .    |            d            |
         |            |       .   |          . |            |
         |            |         . |        .   |            |
         |            |           c      .     |            |
         |            |           |  . .       |            |
                                                                          




this will requires su to Interpolate between the original points 

the capital letters are the sample points from the original wav file 
lower case letters are the ones we want.

for example, for us to get sample b, we will have to interpolate between B and C 



                      |                                                      
                B.    |                                    .H            
             .      . |                                 .      .
           .          b                               G          .
         A            | C                           .              .
       .              |  .                         .                 .
      .               |   .                       .                   .
    ------------------|------------------------------------------------------------>
                      |     .                   .            
                      |      .                 F             
                      |       .              .               
                      |         D          .                 
                      |           .      .                   
                      |              . E                     



to get sample c, we will have to inpolate between D and E

                      |           |                                          
                B.    |           |                        .H            
             .      . |           |                     .      .
           .          b           |                   G          .
         A            | C         |                 .              .
       .              |  .        |                .                 .
      .               |   .       |               .                   .
    ------------------|-----------|------------------------------------------------>
                      |     .     |             .           
                      |      .    |            F            
                      |       .   |          .              
                      |         D |        .                
                      |           c      .                  
                      |           |  . E                    

this way we can take samples arbituary places of our wave.





17:26
Casey starts to change the sound mixer code for pitch shifting.
His first approach is literraly nearest point instead of doing interpolation. 

Casey first changed int SamplesPlayed to real32 SamplesPlayed

recall that SamplesPlayed is really CurrentSampledBeingPlayed
by changing it into a real32, we know where we are within the wave. 


                struct playing_sound
                {
                    v2 CurrentVolume;
                    v2 dCurrentVolume;
                    v2 TargetVolume;

                    real32 dSample;

                    sound_id ID;
                    real32 SamplesPlayed;
                    playing_sound *Next;
                };

for example


                      |                                                      
                B.    |                                    .H            
             .      . |                                 .      .
           .          b                               G          .
         A            | C                           .              .
       .              |  .                         .                 .
      .               |   .                       .                   .
    ------------------|------------------------------------------------------------>
                      |     .                   .            
                      |      .                 F             
                      |       .              .               
                      |         D          .                 
                      |           .      .                   
                      |              . E                     

if  A = sample 0, 
    B = sample 1, 
    C = sample 2, 
    D = sample 3
    E = sample 4
    ...
    ...

b will be around 1.8
since it is not an integer, we use a real32 to represent its position within the sound wave. 







20:50
So previously we are advancing the playing sound, 1 sample at a time
in the for loop, we are looping from 
    start = PlayingSound->SamplesPlayed         to
    end = PlayingSound->SamplesPlayed + SamplesToMix



                while(... ran out of space in output sound buffer ... or playing is done playing )
                {
                    ............................................
                    ......... getting SamplesToMix .............
                    ............................................

                    for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                        SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                        ++SampleIndex)
                    {                
                        real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                        *Dest0++ += *Volume.E[0]*SampleValue;
                        *Dest1++ += *Volume.E[1]*SampleValue;
                        
                        Volume += dVolume;
                    }


                    ...
                    ...
                }


now since we want to play it at different speed, Casey changed the code.
He reanmed SampleIndex to SamplePosition, and we now advances the samplePosition by dSample

-   notice now SamplesIndex is now being calculated by doing 

                u32 SampleIndex = FloorReal32ToInt32(SamplePosition);

    we are just flooring the position to get the nearest sample


-   also notice that since we changed the sample rate, we also have to change SamplesToMix
    SamplesToMix was previously calculated based on the assumption that we are reading one sample at a time.
    now we do the new calculation.

                r32 RealSampleRemainingInSound = (LoadedSound->SampleCount - RoundReal32ToInt32(PlayingSound->SamplesPlayed)) / dSample;                
                u32 SamplesRemainingInSound = RoundReal32ToInt32(RealSampleRemainingInSound);

    calculation should be straightforward.



-   notice that we are reading dSample from PlayingSound->dSample.

    dSample is just something we added into playing_sound

                struct playing_sound
                {
                    v2 CurrentVolume;
                    v2 dCurrentVolume;
                    v2 TargetVolume;

                    real32 dSample;

                    sound_id ID;
                    real32 SamplesPlayed;
                    playing_sound *Next;
                };


-   now the loop looks like 

                while(TotalSamplesToMix && !SoundFinished)
                {
                    loaded_sound *LoadedSound = GetSound(Assets, PlayingSound->ID);
                    if(LoadedSound)
                    {
                        ...
                        ...
                        real32 dSample = PlayingSound->dSample;
                        
                        ...
                        ...


                        u32 SamplesToMix = TotalSamplesToMix;
                        r32 RealSampleRemainingInSound =
                            (LoadedSound->SampleCount - RoundReal32ToInt32(PlayingSound->SamplesPlayed)) / dSample;

                        u32 SamplesRemainingInSound = RoundReal32ToInt32(RealSampleRemainingInSound);
                        if(SamplesToMix > SamplesRemainingInSound)
                        {
                            SamplesToMix = SamplesRemainingInSound;
                        }



                        
                        // TODO(casey): Handle stereo!
                        real32 SamplePosition = PlayingSound->SamplesPlayed;
                        for(u32 LoopIndex = 0;
                            LoopIndex < SamplesToMix;
                            ++LoopIndex)
                        {
                
                            u32 SampleIndex = RoundReal32ToInt32(SamplePosition);
                            real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                            
                            *Dest0++ += AudioState->MasterVolume.E[0]*Volume.E[0]*SampleValue;
                            *Dest1++ += AudioState->MasterVolume.E[1]*Volume.E[1]*SampleValue;
                            
                            Volume += dVolume;
                            SamplePosition += dSample;      
                        }

                        ...
                        ...
                    }

                }



26:12 
Casey proceeded to get live code editing to work with sounds. By messing with the Windows API. 






38:41
Casey adding the lerping in the sound mixing loop
the #if has one section with interpolating, the other using nearest


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





48:40
wrote a PushString function

-   we first iterate through Source, to get the Size. Since this is a NULL terminated string 
    we have to do it this way. 

    Casey set Size = 1 for the NULL terminator. 

-   then we call PushSize_ to get enough memory for ourselves

-   then we copy source to Dest.

                handmade.h

                // NOTE(casey): This is generally not for production use, this is probably
                // only really something we need during testing, but who knows
                inline char *
                PushString(memory_arena *Arena, char *Source)
                {
                    u32 Size = 1;
                    for(char *At = Source;
                        *At;
                        ++At)
                    {
                        ++Size;
                    }
                    
                    char *Dest = (char *)PushSize_(Arena, Size);
                    for(u32 CharIndex = 0;
                        CharIndex < Size;
                        ++CharIndex)
                    {
                        Dest[CharIndex] = Source[CharIndex];
                    }

                    return(Dest);
                }

Casey did mention that if this were production, and he anticipates this function being used more often, 
theres a trick he can do to avoid the double looping. 


                handmade.h

                // NOTE(casey): This is generally not for production use, this is probably
                // only really something we need during testing, but who knows
                inline char *
                PushString(memory_arena *Arena, char *Source)
                {
                    u32 Size = 1;
                    for(char *At = Source;
                        *At;
                        ++At)
                    {
                        ++Size;
                    }
                    
                    char *Dest = (char *)PushSize_(Arena, Size);
                    for(u32 CharIndex = 0;
                        CharIndex < Size;
                        ++CharIndex)
                    {
                        Dest[CharIndex] = Source[CharIndex];
                    }

                    return(Dest);
                }


57:03
Casey demonstarting volume panning based on mouse position 


                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    v2 MusicVolume;
                    MusicVolume.y = SafeRatio0((r32)Input->MouseX, (r32)Buffer->Width);
                    MusicVolume.x = 1.0f - MusicVolume.y;
                    ChangeVolume(&GameState->AudioState, GameState->Music, 0.01f, MusicVolume);
                
                    ...
                    ...

                }




1:03:07
someone in the Q/A Asked whether this pitch shift will also be animated?
Casey said, the pitch shift we are doing will be pretty subtle, you wont get alot of audio payoff from doing animation or ramping
so we dont need it and wont be doing it.




1:12:04
you dont get to choose what core a thread runs on. The OS determines that.
however, becuz that can be important for performance oriented programming, due to the fact that certain caches are local to certain core,

most OS (including windows);, provide ways to tell the OS which thread you want to run on a specific core. 

although Casey dont know if its a hard guarantee, it will create a strong preference in the scheduler

in short, you have some control indirectly.

