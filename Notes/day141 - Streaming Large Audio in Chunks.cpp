Handmade Hero Day 141 - Streaming Large Audio in Chunks

Summary:
Casey discussing ways to incorporate streaming large audio in chunks into our current system with minimal effort.

chooses the method of chaining asset_sound_info through a NextIDToPlay variable.

Modified the DEBUGLoadWAV(); function to load audios in chunks.

Modifed the mixer to mix chained loaded_sounds. added the concept of prefetching in the function as well. 

created the handmade_audio.h and handmade_audio.cpp file, moved all the audio related logic there.

Casey explaning the design philosphy behind temporary memory and why we have them in the Q/A

mentioned whether we care about music compression in this game

Keyword:
audio loading, audio streaming, audio system game state, temporary memory 


2:32
Casey mentioned that he wants to support streaming sounds.
streaming here is not referring to asset streaming, which we have already implemented.
we mean like a single sound instead of being streamed in as a single chunk, it can be streamed as many chunks.

so if you have a music that is 30 MB long, we dont have to pull the whole thing in at once. 
you can take little pieces of it, which takes off some memory pressure and load pressure. 


3:16
the other thing Casey wants to look at is to add more functionality into the mixer, such as 
volume ramping over the time, so we can fade things in or fade things out. 

Casey also wants to make the mixer SSE in the future. 


Casey will address the streaming stuff first



5:03
Casey mentioned that we have this 

                struct asset_sound_info
                {
                    char* Filename;
                };

if we were to stream it in chunks, it would seem cheap for us just to add something like "Number of chunks to stream in as"

the problem with that is that our entire asset streaming system works off of an asset_slot is the fundamental thing 
that controls whether something is loaded or not

essentially we are streaming an asset based on its asset_slot status
if the asset_slot.State is unloaded, and the user requests it, we stream it. 


                struct asset_slot
                {
                    asset_state State;
                    union
                    {
                        loaded_bitmap *Bitmap;
                        loaded_sound *Sound;
                    };
                };


                struct game_assets
                {
                    ...
                    ...

                    uint32 SoundCount;
                    asset_sound_info *SoundInfos;
                    asset_slot *Sounds;

                    ...
                    ...
                };


6:38
what we can do is that every chunk gets an asset_slot, that way we can re-use what we currently have,
we dont have to special case it or do anything speciall essentially.

9:05
another thing we can do is to have mulitple loaded_sound struct, and chaining them together. 

essentially a linked list.

also if you set the sound_id to yourself, it will just be a sound that loops forever.

Casey proceeds to put the NextIDToPlay field in asset_sound_info

                struct asset_sound_info
                {
                    char *FileName;
                    sound_id NextIDToPlay;
                };



10:51
so previously inside the GAME_GET_SOUND_SAMPLES(); code as follow

Casey mentioned that whenever we call GetSound(); to load our sound, its very likely
that we would want the next sound to be loaded as well. 

                extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
                {
                    ...
                    ...
                    
                    // NOTE(casey): Sum all sounds
                    for(playing_sound **PlayingSoundPtr = &GameState->FirstPlayingSound;
                        *PlayingSoundPtr;
                        )
                    {
                        ...

                        loaded_sound *LoadedSound = GetSound(TranState->Assets, PlayingSound->ID);
                        if(LoadedSound)
                        {
                            ...
                            ...
                        }
                        else
                        {
                            LoadSound(TranState->Assets, PlayingSound->ID);
                        }

                        ...
                        ...
                    }

                    ...
                    ...
                }

so we want to go ahead and load the next one as well 

so Casey changed it to, this way we will Prefetch the sound asset for Info->NextIDToPlay;


                extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
                {
                    ...
                    ...
                    
                    // NOTE(casey): Sum all sounds
                    for(playing_sound **PlayingSoundPtr = &GameState->FirstPlayingSound;
                        *PlayingSoundPtr;
                        )
                    {
                        ...

                        loaded_sound *LoadedSound = GetSound(TranState->Assets, PlayingSound->ID);
                        if(LoadedSound)
                        {
                            asset_sound_info* Info = GetSoundInfo(TranState->Assets, PlayingSounds->ID);
                            PrefetchSound(TranState->Assets, Info->NextIDToPlay);
                            ...
                            ...
                        }
                        else
                        {
                            LoadSound(TranState->Assets, PlayingSound->ID);
                        }

                        ...
                        ...
                    }

                    ...
                    ...
                }



15:57
Casey writes the Prefetch() function; which surprisingly is just LoadSound(); for now.
Casey went on to write PrefetchBitmap(); as well. 

                internal void LoadBitmap(game_assets *Assets, bitmap_id ID);
                inline void PrefetchBitmap(game_assets *Assets, bitmap_id ID) {LoadBitmap(Assets, ID);}
                internal void LoadSound(game_assets *Assets, sound_id ID);
                inline void PrefetchSound(game_assets *Assets, sound_id ID) {LoadSound(Assets, ID);}

in the future we would want the asset system explicit know the difference.


12:37
Then if we get to the end of a loaded_sound, we want to continue and play the next sound. 

the challenge is to make the playback seamless.

On a first draft, Casey wrote the non-seamless version

PlayingSound has member variable called SampledPlayed.
that is really CurrentSampledBeingPlayed.

-   recall that the line 

                if((uint32)PlayingSound->SamplesPlayed == LoadedSound->SampleCount)

    is the condition we used to check whether we have finished a loaded_sound or not. 
    
    once we finished, we check whether Info->NextIDToPlay; is valid or not.

    if the NextIDToPlay is not valid, then we officially mark SoundFinished = true

    otherwise it should go on to play Info->NextIDToPlay.

                extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
                {
                    ...
                    ...
                    
                    // NOTE(casey): Sum all sounds
                    for(playing_sound **PlayingSoundPtr = &GameState->FirstPlayingSound;
                        *PlayingSoundPtr;
                        )
                    {
                        ...

                        loaded_sound *LoadedSound = GetSound(TranState->Assets, PlayingSound->ID);
                        if(LoadedSound)
                        {
                            asset_sound_info* Info = GetSoundInfo(TranState->Assets, PlayingSounds->ID);
                            PrefetchSound(TranState->Assets, Info->NextIDToPlay);
                     
                            .....................................................
                            .......... doing all the sound mixing shit ..........
                            .....................................................


                            if((uint32)PlayingSound->SamplesPlayed == LoadedSound->SampleCount)
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
                            else
                            {
                                Assert(TotalSamplesToMix == 0);
                            }

                        }
                        else
                        {
                            LoadSound(TranState->Assets, PlayingSound->ID);
                        }

                        ...
                        ...
                    }

                    ...
                    ...
                }


14:15
Casey writes the IsValid(); function.
essentially, we are just guarding against 0

                handmade_asset.h

                inline bool32
                IsValid(bitmap_id ID)
                {
                    bool32 Result = (ID.Value != 0);

                    return(Result);
                }

                inline bool32
                IsValid(sound_id ID)
                {
                    bool32 Result = (ID.Value != 0);

                    return(Result);
                }





 19:17
 Casey now says we need to extend our notion of asset_sound_info.

we added two new fields 
                u32 FirstSampleIndex;
                u32 SampleCount;

to indicate what section of the whole audio file are we looking at.                 

                struct asset_sound_info
                {
                    char *FileName;
                    u32 FirstSampleIndex;
                    u32 SampleCount;
                    sound_id NextIDToPlay;
                };

so visually we will have something like 

                asset_sound_info 1                      asset_sound_info 2                          asset_sound_info 3     
                {                                       {                                           {  
                    ...                                     ...                                         ...
                    FirstSampleIndex = 0                    FirstSampleIndex = 6                        FirstSampleIndex = 9
                    SampleCount = 5                         SampleCount = 3                             SampleCount = 4 
                    NextIDToPlay = 2  ----------->          NextIDToPlay = 3    --------------->        NextIDToPlay = 0                     
                }                                       }                                           }





19:51
then in the internal PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork) function, we make some changes.


initially we had 

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork)
                {
                    ...
                    ...
                    
                    *Work->Sound = DEBUGLoadWAV(Info->FileName);

                    ...
                    ...
                }

we modify the DEBUGLoadWAV(); function to load specific chunks


                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork)
                {
                    ...
                    ...

                    *Work->Sound = DEBUGLoadWAV(Info->FileName, Info->FirstSampleIndex, Info->SampleCount);

                    ...
                    ...
                }


20:38
Casey made the rule that if SampleCount == 0, that just means we are going to load the whole file
cuz you will never make an asset_sound_info to read zero SampleCount 

                struct asset_sound_info
                {
                    char *FileName;
                    u32 FirstSampleIndex;
                    u32 SampleCount;
                    sound_id NextIDToPlay;
                };











21:13 
Casey mdifying DEBUGLoadWAV(); to make it work with chunks of audio files 

what we do here is that we load the entire file, but we only want the portions that the caller is asking for 


-   we first call Result.SampleCount = SectionSampleCount.
that gets rid of the old SampleCount. The oldSampleCount contains the sampleCount for the entire audio fire.

-   then for each channel, we do a pointer arithemtic to move them up to where want them to be 


recall previously we had a sorted left channel and a unsorted right channel

            channel 0               channel 1
            SampleData              SampleData + Result.SampleCount;
                        
                |                       |
                |                       |
                V                       V  

                l0  l1  l2  l3  l4  l5  r?  r?  r?  r?  r?  r? 

where Results.Sample[0] points l0 of the Results.Samples array
and Samples[1] points to r? of the Results.Samples array

we do a pointer arithemtic to move them up. So if SectionFirstSampleIndex is 2, the graph becomes 

                    channel 0               channel 1
                    SampleData              SampleData + Result.SampleCount;
                        
                        |                       |
                        |                       |
                        V                       V  

                l0  l1  l2  l3  l4  l5  r?  r?  r?  r?  r?  r? 



full code below 


                internal loaded_sound
                DEBUGLoadWAV(char *FileName, u32 SectionFirstSampleIndex, u32 SectionSampleCount)
                {
                    loaded_sound Result = {};
                    
                    .........................................................
                    ....... loading entire audio file into Result ...........
                    .........................................................

                    // TODO(casey): Load right channels!
                    Result.ChannelCount = 1;
                    if(SectionSampleCount)
                    {
                        Assert((SectionFirstSampleIndex + SectionSampleCount) <= Result.SampleCount);
                        Result.SampleCount = SectionSampleCount;
                        for(uint32 ChannelIndex = 0;
                            ChannelIndex < Result.ChannelCount;
                            ++ChannelIndex)
                        {
                            Result.Samples[ChannelIndex] += SectionFirstSampleIndex;
                        }
                    }
                    

                    return(Result);
                }



30:28
to test his loading code, Casey tried to load the music_test.wav file into chunks 

this code is just temporary. Later on these code will all be removed. 

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    ...
                    ...
                    u32 OneMusicChunk = 10*48000;
                    BeginAssetType(Assets, Asset_Music);
                    asset *LastMusic = 0;
                    for(u32 FirstSampleIndex = 0;
                        FirstSampleIndex < TotalMusicSampleCount;
                        FirstSampleIndex += OneMusicChunk)
                    {
                        u32 SampleCount = TotalMusicSampleCount - FirstSampleIndex;
                        if(SampleCount > OneMusicChunk)
                        {
                            SampleCount = OneMusicChunk;
                        }
                        asset *ThisMusic = AddSoundAsset(Assets, "test3/music_test.wav", FirstSampleIndex, SampleCount);
                        if(LastMusic)
                        {
                            Assets->SoundInfos[LastMusic->SlotID].NextIDToPlay.Value = ThisMusic->SlotID;
                        }
                        LastMusic = ThisMusic;
                    }
                    EndAssetType(Assets);

                }

35:25
Becuz Casey had to constantly swtich back and forth between debug build and optimized build, he got annoyed.
so he mentioned that what we probably do is to just compile the blip code in optimized mode, the rest in 
debug mode. 



36:20
coming back to the mixer, we want to make it so that when we finished playing a sound, we want to continue 
playing it and fill up the sound output buffer. The implementation mentioned in 10:51 doesnt actually keep on
filling up the sound output buffer until next frame. We want to continue filling the sound output buffer this frame. 

-   so you can see that now the main structure is we loop through all the audio sound
    for each audio sound, we run a while loop to keep up mixing until the whole chain of that audio sound is done.

-   so there are two conditions where we would break out of the while loop,
    either the sound output buffer ran out of space for you to mix or
    we have finished playing thea audio.

    that is what each of TotalSamplesToMix and SoundFinished is for.

    TotalSamplesToMix keeps track of how much space we got left in the sound output buffer
    SoundFinished is whether our current chain of audio sound is finished playing or not.

    you can see that inside the while loop, we call 

                TotalSamplesToMix -= SamplesToMix;

    to keep track of TotalSamplesToMix. 

-   for example, assume we have 10 spots in the sound output buffer,
    and we got loaded_sound sound aa, bb and cc chained together.

    TotalSamplesToMix will start out at 10, cuz thats the total space in the output buffer 

                    __ __ __ __ __ __ __ __ __ __ 

                    aa aa aa aa aa bb bb bb bb cc cc cc 

    after we filled up loaded_sound aa, TotalSamplesToMix = 5,
    after we filled up loaded_sound bb, TotalSamplesToMix = 1,
    after we filled up loaded_sound bb, TotalSamplesToMix = 0, (cuz we clamp the SamplesToMix variable)

    and we break out of the while loop


-   2nd possible scenario is 

                    __ __ __ __ __ __ __ __ __ __ 

                    aa aa aa aa aa bb bb bb bb 

    that is when SoundFinished is true. and we break out of the while loop.



-   full code below 
      
                extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)(audio_state *AudioState,
                                    game_sound_output_buffer *SoundBuffer, game_assets *Assets,
                                    memory_arena *TempArena)
                {

                    ...
                    ...

                    // NOTE(casey): Sum all sounds
                    for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
                        *PlayingSoundPtr;
                        )
                    {
                        
                        ...
                        ...

                        while(TotalSamplesToMix && !SoundFinished)
                        {
                            loaded_sound *LoadedSound = GetSound(Assets, PlayingSound->ID);
                            if(LoadedSound)
                            {
                                asset_sound_info *Info = GetSoundInfo(Assets, PlayingSound->ID);
                                PrefetchSound(Assets, Info->NextIDToPlay);
                            
                                // TODO(casey): Handle stereo!
                                real32 Volume0 = PlayingSound->Volume[0];
                                real32 Volume1 = PlayingSound->Volume[1];

                                Assert(PlayingSound->SamplesPlayed >= 0);

                                u32 SamplesToMix = TotalSamplesToMix;
                                u32 SamplesRemainingInSound = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
                                if(SamplesToMix > SamplesRemainingInSound)
                                {
                                    SamplesToMix = SamplesRemainingInSound;
                                }
                            
                                for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                                    SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                                    ++SampleIndex)
                                {                
                                    real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                                    *Dest0++ += Volume0*SampleValue;
                                    *Dest1++ += Volume1*SampleValue;
                                }

                                Assert(TotalSamplesToMix >= SamplesToMix);
                                PlayingSound->SamplesPlayed += SamplesToMix;
                                TotalSamplesToMix -= SamplesToMix;

                                if((uint32)PlayingSound->SamplesPlayed == LoadedSound->SampleCount)
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
                                else
                                {
                                    Assert(TotalSamplesToMix == 0);
                                }
                            }
                            else
                            {
                                LoadSound(Assets, PlayingSound->ID);
                                break;
                            }
                        }

                        if(SoundFinished)
                        {
                            *PlayingSoundPtr = PlayingSound->Next;
                            PlayingSound->Next = AudioState->FirstFreePlayingSound;
                            AudioState->FirstFreePlayingSound = PlayingSound;
                        }
                        else
                        {
                            PlayingSoundPtr = &PlayingSound->Next;
                        }
                    }

                    ...
                    ...

                    EndTemporaryMemory(MixerMemory);
                }



41:54

Casey duscusses ways to make this easier to debug, he made the OneMusicChunk size in from 10*48000
to 2*48000.

around 42:05 Casey claims he still hears a bit of "skipping", which means our loop is not correct yet. 






46:55
Casey proceeds to move everything into a handmade_audio.h and handmade_audio.cpp file


49:31
created a new struct 

                struct audio_state
                {
                    memory_arena *PermArena;
                    playing_sound *FirstPlayingSound;
                    playing_sound *FirstFreePlayingSound;
                };


as you might have noticed, we moved the *FirstPlayingSound; and *FirstFreePlayingSound; from game_state to there.


                struct game_state
                {
                    ...
                    ...

                    playing_sound *FirstPlayingSound;
                    playing_sound *FirstFreePlayingSound;
                };


52:07
also noticed that we added the PermArena in the audio_state struct. That way, any audio related memory 
can come from there.




1:00:08
Someone in the Q/A asked what is the reason of having temproary memory 

The reason of is becuz it is memory that only exists during that function. 

we dont have the ability to talk about memory in a useful transient way. 
C++ used have this alloca_ that was poorly supported.

In C, if you think about, declaring temporary local variables is very simple.
you go into a function, you declare some integers. Once you finish with your functions, all that 
stack memory is unwinded and cleaned up.

the programmers who wrote C understand that it was important to have temporary scratch space 
where functions did their work and you didnt want to have to manage it all the time yourself.

also if you think about it, when you call functions, and unwind, then you call the 2nd function then unwind,
it is possible that both functions were using the same stack memory space. So this is a conecpt of having 
one or more overlapping temporary stacks of memory,

so what we are trying to do is to emulate that for supposed heap variables.
for example if you declare int array* = new int[there]
that memory will exist on the heap, and we dont want that 

we want to create dynamic arrays on our temporary memory that we can manage our selves with little costs. 

which is why Casey setup those arenas 

as a result, we can basically make our routines work the same with arbituary blocks of memory that gets unwinded at 
different rates or that are very large or we dont know how big they will be a head of time. 

all of these we can do it at a zero cost way, no overhead way. unlike garbage collection.


for example over here in the following piece of code 

when we declare RealChannel0 and RealChannel1, with the way we set up our temporary memory,
these are just like temproary stack local variables that you would have in C or C++.

we are imitating that with our temporary_memory setup.

                handmade_asset.cpp

                internal void
                OutputPlayingSounds(audio_state *AudioState,
                                    game_sound_output_buffer *SoundBuffer, game_assets *Assets,
                                    memory_arena *TempArena)
                {
                    temporary_memory MixerMemory = BeginTemporaryMemory(TempArena);

                    real32 *RealChannel0 = PushArray(TempArena, SoundBuffer->SampleCount, real32);
                    real32 *RealChannel1 = PushArray(TempArena, SoundBuffer->SampleCount, real32);


                    ...
                    ...

                    EndTemporaryMemory(TempArena);
                }


typically you will have three stacks that you will work with 

one stack is the function stack, the one C sets up for you, (the one where local variables go)

one stack is this temporary_memory stack, where you have large regions of memory that you work on 
temporarily 

and you then you have the final one where it persist across frames, 
such as our streaming system task memory, they persis across throughout the length of an operation, not just a function 

with our BeginTemporaryMemory and EndTemporaryMemory almost has no cost.



1:17:43
what is not true is that decompression is not always free. 
if you want all your sound files to compressed with heavy weight compression such as mp3,
that decompression can be expensive. So if you are decompressing in real time (lets say you want to save memory);,
you might start to care about CPU work load for audio work.

if youre just talking about sound mixing, CPU is more than enough.


1:19:25
do we care about the games disc footprint. we wouldnt very care about good music compression unless we care about disc footprint.

if you just actually care about disc footprint, such as download size smaller, patch size smaller. if you care about that 
    music compression is a big deal, cuz you can achieve 8 to 1, especially if you have tons of music in the game. 

the other thing you care about is transfer time, for example, even for us, we are doing asset streaming, all of our bitmaps,
all of our sound files. In the future, we always want to make sure that our asset streaming stays ahead of what we need.



