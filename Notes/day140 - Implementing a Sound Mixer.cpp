Handmade Hero Day 140 - Implementing a Sound Mixer

Summary:
wrote code for a sound mixer. 

wrote the playing_sound struct, which keeps track of the currently playing sound.

introduced the concept of memory for Meta objects. This is decoupled from memory_arena WorldArena, which is used 
for world simulation  

added the PlaySound(); which can be called anywhere.
the function takes users PlaySound(); audio request and appends it to our list of playing_sounds

Casey demonstarted it so that it PlaySounds whenever the hero throws his sword.

Casey explaining why the engine can playing multiple instances of the same sounds.
explained the difference between the playing_sound struct and the loaded_sound struct.
one is an asset data, the other instance data.

explained why Casey chooses a linked list over a vector to store the list of playing_sounds.


Keyword:
Sound mixer, audio, memory



3:02
Casey mentions that the plan for today is to take the GAME_GET_SOUND_SAMPLES(GameGetSoundSamples); function, 
convert it something that can play arbituary number of sounds, at arbituary times, and have them all mixed and sounds properly.


Casey is unsure whether we want to make GAME_GET_SOUND_SAMPLES(GameGetSoundSamples); something that can be called on a separate
thread.

just as the first draft, Casey will write this assuming that GAME_GET_SOUND_SAMPLES(); is synchornous with the rest of the game 


4:41
Casey wants to first introduce the idea that we want to keep track of all the sounds we are currently playing in the game. 
Casey plans to make this in a linked-list 

Casey first defined a struct for that 
-   we could be in stereo, so we can say we have two volumes, a left and a right volume. 

-   SamplesPlayed is to indicate where I am in the sound 

                struct playing_sound

                {
                    real32 Volume[2];
                    sound_id ID;
                    int32 SamplesPlayed;
                    playing_sound *Next;
                };



                struct game_state
                {
                    ...
                    ...

                    playing_sound *FirstPlayingSound;
                    playing_sound *FirstFreePlayingSound;
                };


-   here we define two linked list
    one is our current linked list of playing_sound 

    the other to recycle already used and finished ones. Pretty much the free list 
    (we have been doing this alot previously);


    if you call, we do the same with pairwise_collision_rule 


                struct game_state
                {
                    ...
                    ...
                    
                    pairwise_collision_rule *CollisionRuleHash[256];
                    pairwise_collision_rule *FirstFreeCollisionRule;
                    
                    ...
                    ...
                };








8:02
what we want to do here is that Casey will go through all the PlayingSound and sum them up.
As we mentioned in day 139, mixing sounds clip is literraly just summing all the sound waves up.

-   as mentioned above, we are doing a linked list, our for loop is just looping through the linked list.
Casey only wrote the skeleton up until now

                extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
                {

                    // NOTE(casey): Sum all sounds
                    for(playing_sound **PlayingSoundPtr = &GameState->FirstPlayingSound;
                        *PlayingSoundPtr;
                        )
                    {
                        ...........................................................
                        ....... Summing up all the sounds wave sample data ........
                        ...........................................................
                    }

                    ...
                    ...

                    ..................................................
                    ...... Copying it to the output SoundBuffer ......
                    ..................................................
                }






9:31
before we go on to write more code, Casey pointed out a problem in our original loop.
(the code itself is correct, but a concept in they way we are looping wont work for what we want to do later)

previously we can see that we are copying 16 bit sample data into our 16 bit output date 

16 bit sample data being 
                int16 SampleValue = GameState->TestSound.Samples[0][TestSoundSampleIndex];

16 bit output date being 
                int16 *SampleOut = SoundBuffer->Samples;

Casey pointed out that as we are accumulating values in the sampleData, we might get a lot more clipping than necessary

so the int16 SampleValue is our intermediate value. For this intermediate value, it is quite dangerous to just use a 
int16.
imagine we have 3 waves samples 

        sample points 
                    1 2 3 4 5  6   7 8 9 

        wave 1      1 1 1 1 1 20k  1 1 1
        wave 2      1 1 1 1 1 20k  1 1 1
        wave 3      1 1 1 1 1 -10k 1 1 1

when we are looping through the waves data, we accumulate the sum. 
when we get to sample point 6, we will first add wave1.20k + wav2.20k, which will overflow on a 16 bit 
but if you add the third point, it will only be 30 k, which is a valid un-clipped result.

so we can tuse int16 as an intermediate value 


                extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
                {


                    ...
                    ...


                    game_state *GameState = (game_state *)Memory->PermanentStorage;

                    int16 *SampleOut = SoundBuffer->Samples;
                    for(int SampleIndex = 0;
                        SampleIndex < SoundBuffer->SampleCount;
                        ++SampleIndex)
                    {
                        uint32 TestSoundSampleIndex = (GameState->TestSampleIndex + SampleIndex) %
                            GameState->TestSound.SampleCount;
                        int16 SampleValue = GameState->TestSound.Samples[0][TestSoundSampleIndex];
                        *SampleOut++ = SampleValue;
                        *SampleOut++ = SampleValue;
                    }

                    GameState->TestSampleIndex += SoundBuffer->SampleCount;
                }

so typically if you are doing 16 bit sound, you want to do it in 32-bit space.

what we will probably do here is just use float, cuz that way we can also do modulation, and rounding all that stuff.

so what we want to do is to convert 16 bit sample points to floats, do the mixing, and convert from float back to 16 bit.



11:41
Casey proceeds to write the code for it
-   So casey defined a real32 array for each channel 

                real32 *RealChannel0 = PushArray(&TranState->TranArena, SoundBuffer->SampleCount, real32);
                real32 *RealChannel1 = PushArray(&TranState->TranArena, SoundBuffer->SampleCount, real32);


-   also notice that if the LoadedSound is invalid, we load the sound 

                loaded_sound *LoadedSound = GetSound(TranState->Assets, PlayingSound->ID);
                if(LoadedSound)
                {
                    ...
                }
                else
                {
                    LoadSound(TranState->Assets, PlayingSound->ID);
                }

-   the idea here is that we got a double for loop
    the outer for loop goes through each wave 
    the inner for loop goes through eac sample point of the wave 

    assume the wave has samples points values corresponding to their index
    any of the waves could start from any sampleIndex and end at different sampleInex 
                       
                SampleIndex               
                wave 1      1   2   3   4   5   
                wave 2      4   5   6   7   8   9   10   11   1
                wave 3      7   8   9   10  11  12

    hence the      
                for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                ++SampleIndex)

    the "SamplesToMix" variable gets the number of samples we want to process
    
                uint32 SamplesToMix = SoundBuffer->SampleCount;
                uint32 SamplesRemainingInSound = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
                if(SamplesToMix > SamplesRemainingInSound)
                {
                    SamplesToMix = SamplesRemainingInSound;
                }

    as you can see, we first initializ SamplesToMix to be SoundBuffer->SoundBuffer->SampleCount
    if the the number of samples from the wave is less, we make "SamplesToMix = SamplesRemainingInSound;"



full code below

                extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
                {

                    temporary_memory MixerMemory = BeginTemporaryMemory(&TranState->TranArena);

                    real32 *RealChannel0 = PushArray(&TranState->TranArena, SoundBuffer->SampleCount, real32);
                    real32 *RealChannel1 = PushArray(&TranState->TranArena, SoundBuffer->SampleCount, real32);

                    ...
                    ...
                    
                    // NOTE(casey): Sum all sounds
                    for(playing_sound **PlayingSoundPtr = &GameState->FirstPlayingSound;
                        *PlayingSoundPtr;
                        )
                    {
                        ...
                        ...

                        loaded_sound *LoadedSound = GetSound(TranState->Assets, PlayingSound->ID);
                        if(LoadedSound)
                        {
                            ..
                            ...

                            uint32 SamplesToMix = SoundBuffer->SampleCount;
                            uint32 SamplesRemainingInSound = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
                            if(SamplesToMix > SamplesRemainingInSound)
                            {
                                SamplesToMix = SamplesRemainingInSound;
                            }

                            for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                                SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                                ++SampleIndex)
                            {                
                                ...
                                ...
                            }

                            ...
                        }
                        else
                        {
                            LoadSound(TranState->Assets, PlayingSound->ID);
                        }

                        ...
                        ...
                    }

                }




the GetSound(); function we call above is the same as its sister function GetBitmap();
instead of getting the bitmap, we get getting the sound. 

                handmade_asset.h

                inline loaded_sound *GetSound(game_assets *Assets, sound_id ID)
                {
                    loaded_sound *Result = Assets->Sounds[ID.Value].Sound;

                    return(Result);
                }





14:30
we delve further to see how do we mix the values 
you can see that we first grab the volume 

then when we populate the value in Dest0, we do 

                *Dest0++ += Volume0*SampleValue;
                *Dest1++ += Volume1*SampleValue;

assuming we have the following wave 

        volume[0] = 0.5
        
            waves   4   5   6   7   8   9   10   11   


the value we will store in our buffer will be 
                
                    2   2.5   3   3.5   4   4.5   5   5.5 

the volume is like a game setting, it something the user will control.
so when we submit our sound buffer to the windows layer, we just multiplying this user defined volume 
with the original sample value from the wav file.

-   then we determine whether we have finished playing the current sound 
    if so, we put the playingSound into the GameState->FirstFreePlayingSound list. (recycling finished playing sound structs)
    also we remove ourselves from the valid linked list. 

-   full code below:


                for(playing_sound **PlayingSoundPtr = &GameState->FirstPlayingSound;
                    *PlayingSoundPtr;
                    )
                {

                    // TODO(casey): Handle stereo!
                    real32 Volume0 = PlayingSound->Volume[0];
                    real32 Volume1 = PlayingSound->Volume[1];
                    real32 *Dest0 = RealChannel0;
                    real32 *Dest1 = RealChannel1;

                    for(uint32 SampleIndex = PlayingSound->SamplesPlayed;
                        SampleIndex < (PlayingSound->SamplesPlayed + SamplesToMix);
                        ++SampleIndex)
                    {                
                        real32 SampleValue = LoadedSound->Samples[0][SampleIndex];
                        *Dest0++ += Volume0*SampleValue;
                        *Dest1++ += Volume1*SampleValue;
                    }

                    SoundFinished = ((uint32)PlayingSound->SamplesPlayed == LoadedSound->SampleCount);

                    PlayingSound->SamplesPlayed += SamplesToMix;

                    ...
                    ...

                    if(SoundFinished)
                    {
                        *PlayingSoundPtr = PlayingSound->Next;
                        PlayingSound->Next = GameState->FirstFreePlayingSound;
                        GameState->FirstFreePlayingSound = PlayingSound;
                    }
                    else
                    {
                        PlayingSoundPtr = &PlayingSound->Next;
                    }
                }   




19:18
after we are done mixing, we just copy them from our intermediate buffer to our SoundBuffer
then we give it back to the win32 layer. Recall that we are converting from real32 to int16.

-   the +0.5 is just a trick to rounding our number to the closer integer. We have seen that multiple times 
    if you have 1.2, we want to round to 1 
    if you have 1.7, we want to round to 2

    so doing (int16)(*Source0++ + 0.5f) does the job;


                // NOTE(casey): Convert to 16-bit
                {
                    real32 *Source0 = RealChannel0;
                    real32 *Source1 = RealChannel1;

                    int16 *SampleOut = SoundBuffer->Samples;
                    for(int SampleIndex = 0;
                        SampleIndex < SoundBuffer->SampleCount;
                        ++SampleIndex)
                    {
                        *SampleOut++ = (int16)(*Source0++ + 0.5f);
                        *SampleOut++ = (int16)(*Source1++ + 0.5f);
                    }
                }


28:06
Casey wants to address some problems with memory_arena

we have things in the WorldArena, but we need a separate memory for non world object, as sounds do. 

there are things may persist across the creation or deletion of the world. 
For example, is music is playing when the player goes "Quit to Main Menu", you dont want those sounds to go away 
just becuz the world being played is destroyed. 

so sounds memory and World memory should be decoupled 

                struct game_state
                {
                    ...

                    memory_arena MetaArena;
                    memory_arena WorldArena;
                    world *World;

                    ...
                    ...
                };



31:51
Casey proceed to load the audio we files, 
should be pretty straightforward/

                handmade_asset.h

                enum asset_type_id
                {
                    Asset_None,

                    ...
                    ...

                    Asset_Bloop,
                    Asset_Crack,
                    Asset_Drop,
                    Asset_Glide,
                    Asset_Music,
                    Asset_Puhp,

                    ...

                    Asset_Count,
                };


                handmade_asset.cpp

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    ...
                    ...

                    BeginAssetType(Assets, Asset_Bloop);
                    AddSoundAsset(Assets, "test3/bloop_00.wav");
                    AddSoundAsset(Assets, "test3/bloop_01.wav");
                    AddSoundAsset(Assets, "test3/bloop_02.wav");
                    AddSoundAsset(Assets, "test3/bloop_03.wav");
                    AddSoundAsset(Assets, "test3/bloop_04.wav");
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Crack);
                    AddSoundAsset(Assets, "test3/crack_00.wav");
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Drop);
                    AddSoundAsset(Assets, "test3/drop_00.wav");
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Glide);
                    AddSoundAsset(Assets, "test3/glide_00.wav");
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Music);
                    AddSoundAsset(Assets, "test3/music_test.wav");
                    EndAssetType(Assets);

                    BeginAssetType(Assets, Asset_Puhp);
                    AddSoundAsset(Assets, "test3/puhp_00.wav");
                    AddSoundAsset(Assets, "test3/puhp_01.wav");
                    EndAssetType(Assets);   
                }



Casey also made the asset_slot shared among bitmap and sound.
this is done using a union trick

                struct asset_slot
                {
                    asset_state State;
                    union
                    {
                        loaded_bitmap *Bitmap;
                        loaded_sound *Sound;
                    };
                };



50:39
Casey then added the tech to PlaySound anywhere we want 
the PlaySound(); function is below
you can see that what we do here is that there is an available slot from the FreeList (GameState->FirstFreePlayingSound)

if there isnt an available slot, we allocate one on the spot.

then we just append our new playing sound to the GameState->FirstPlayingSound list. 
pretty straightforward

                handmade.cpp 

                internal playing_sound *
                PlaySound(game_state *GameState, sound_id SoundID)
                {
                    if(!GameState->FirstFreePlayingSound)
                    {
                        GameState->FirstFreePlayingSound = PushStruct(&GameState->WorldArena, playing_sound);
                        GameState->FirstFreePlayingSound->Next = 0;
                    }

                    playing_sound *PlayingSound = GameState->FirstFreePlayingSound;
                    GameState->FirstFreePlayingSound = PlayingSound->Next;

                    PlayingSound->SamplesPlayed = 0;
                    // TODO(casey): Should these default to 0.5f/0.5f for centerred?
                    PlayingSound->Volume[0] = 1.0f;
                    PlayingSound->Volume[1] = 1.0f;
                    PlayingSound->ID = SoundID;

                    PlayingSound->Next = GameState->FirstPlayingSound;
                    GameState->FirstPlayingSound = PlayingSound;

                    return(PlayingSound);
                }


54:02

Casey proceeds to use in in the following situation:
when the Hero throws a sword, 
we would call the PlaySound(); function.

                switch(Entity->Type)
                {
                    case EntityType_Hero:
                    {
                        // TODO(casey): Now that we have some real usage examples, let's solidify
                        // the positioning system!

                        ...
                        ...

                        if((ConHero->dSword.x != 0.0f) || (ConHero->dSword.y != 0.0f))
                        {
                            sim_entity *Sword = Entity->Sword.Ptr;
                            if(Sword && IsSet(Sword, EntityFlag_Nonspatial))
                            {
                                Sword->DistanceLimit = 5.0f;
                                MakeEntitySpatial(Sword, Entity->P,
                                                  Entity->dP + 5.0f*V3(ConHero->dSword, 0));
                                AddCollisionRule(GameState, Sword->StorageIndex, Entity->StorageIndex, false);

                                PlaySound(GameState, GetRandomSoundFrom(TranState->Assets, Asset_Music, &GameState->GeneralEntropy));
                            }
                        }
                        
                        
                    } break;



1:20:50
someone asked whether the engine can play the same sound audio at the same time 
Casey demonstrating that it certain can

the playing_sound is a different concept of loaded sound 

we have a list of playing_sound that all points to the loaded_sound


    playing_sound_0 -------------->  

    playing_sound_1 -------------->
                                            loaded_sound
    playing_sound_2 -------------->

    playing_sound_3 -------------->


this is a very important concept. We always want to separate our asset data with our instane data.
playing_sound is like an instance data
loaded_sound is the asset data



1:24:35
someone in the Q/A asked
for the list of sound, why do you use a linked list as oppose to vector or array or other data structure?

Casey says cuz in this case, we tend to add and remove entries randomly. 
vectors, you cant add and remove randomly without costs. When you add something into a vector,
you may have to resize it.

also in this case, we dont need random access. For example, we wont be accessing the third playing_sound 
in our list. (at least for now);

so if we have a situation where we have random addition and removal, without the need for random access,
linkedlist can do. 