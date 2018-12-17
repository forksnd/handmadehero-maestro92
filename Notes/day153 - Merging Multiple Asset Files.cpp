Handmade Hero Day 153 - Merging Multiple Asset Files

Summary:
Reiterated the notion that the first asset in our asset table is the NULL asset. 

refactored the asset generator to generate multiple .hha file.
tested the game side asset loading code and see if we are merging assets correctly. 

Keyword:
asset, asset system, asset files


6:35
Casey starts off by fixing the bug where shadows werent appearing.
turns out it is related to the concept of the first asset in our asset system should be the NULL asset 

in the test_asset_builder.cpp main code you can see that we did 

in which the AssetCount starts off at 1. That is us leaving the first asset as the NULL asset.

                int main(int ArgCount, char **Args)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;

                    ...
                    Assets->AssetCount = 1;

                    ...
                    ...
                }




9:32
so in the game side code, in the AllocateGameAssets(); function, we start the Assets->AssetCount = 1; as well.
We want to leave the first slot open.


                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);
                    
                    ...
                    ...

                    Assets->AssetCount = 1;

                    ...
                    ...
                }



10:15
we first make a NULL asset. Pretty much set the first entry Assets->Assets to be zero. 
completely clearing that part of the memory. 

                handmade_asset.cpp

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    ...
                    ...

                    u32 AssetCount = 0;
                    ZeroStruct(*(Assets->Assets + AssetCount));
                    ++AssetCount;

                    ...
                    ...

                }


Casey proceeds to make the first Tag the NULL tag as well.



19:35 
Casey beginning to refactor the test_asset_builder.cpp code to make it generate multiple .hha file.
This way we can test our game side asset loading code. 


pretty much we split our assets into three portions WriteHero();, WriteNonHero();, and WriteSounds();
with test1.hha, test2.hha, and test3.hha 

and we used all of these to test our game side asset loading code.

                internal void WriteHero(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    ...
                    ...

                    WriteHHA(Assets, "test1.hha");
                }

                internal void WriteNonHero(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    ...
                    ...

                    WriteHHA(Assets, "test2.hha");
                }

                internal void WriteSounds(void)
                {
                    game_assets Assets_;
                    game_assets *Assets = &Assets_;
                    Initialize(Assets);

                    ...
                    ...

                    WriteHHA(Assets, "test3.hha");
                }

                int main(int ArgCount, char **Args)
                {
                    WriteNonHero();
                    WriteHero();
                    WriteSounds();
                }

the rest of the stream is pretty much debugging


45:27
introduced the concept of different sound mode 
hha_sound now has a Chain field, which will indicate whether it is HHASoundChain_None, HHASoundChain_Loop or HHASoundChain_Advance

                enum hha_sound_chain
                {
                    HHASoundChain_None,
                    HHASoundChain_Loop,
                    HHASoundChain_Advance,
                };

                struct hha_bitmap
                {
                    u32 Dim[2];
                    r32 AlignPercentage[2];
                };

                struct hha_sound
                {
                    u32 SampleCount;
                    u32 ChannelCount;
                    u32 Chain; // NOTE(casey): hha_sound_chain
                };





45:44
we also modify the mixer to understand that as well. 

                handmade_audio.cpp

                // NOTE(casey): Sum all sounds
                for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
                    *PlayingSoundPtr;
                    )
                {
                    ...
                    ...

                    while(TotalChunksToMix && !SoundFinished)
                    {
                        loaded_sound *LoadedSound = GetSound(Assets, PlayingSound->ID);
                        if(LoadedSound)
                        {
                            sound_id NextSoundInChain = GetNextSoundInChain(Assets, PlayingSound->ID);
                            PrefetchSound(Assets, NextSoundInChain);

                            ...
                            ...
                        }
                    }
                }


we define the GetNextSoundInChain(); as below:

                handmade_asset.h

                inline sound_id GetNextSoundInChain(game_assets *Assets, sound_id ID)
                {
                    sound_id Result = {};

                    hha_sound *Info = GetSoundInfo(Assets, ID);
                    switch(Info->Chain)
                    {
                        case HHASoundChain_None:
                        {
                            // NOTE(casey): Nothing to do.
                        } break;

                        case HHASoundChain_Loop:
                        {
                            Result = ID;
                        } break;

                        case HHASoundChain_Advance:
                        {
                            Result.Value = ID.Value + 1;
                        } break;

                        default:
                        {
                            InvalidCodePath;
                        } break;
                    }

                    return(Result);
                }
