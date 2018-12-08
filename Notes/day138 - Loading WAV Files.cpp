Handmade Hero Day 138 - Loading WAV Files
Summary:

Wrote the .wav file parser 
parsed the .wav file into the loaded_sound struct.

showed us an integer alignment trick. (this trick is used multiple times in previous episodes);

attempted to convert the interleaved the channel sample data in .wav file into separate channels 
and stored them inside loaded_sound.Samples variable.

Casey only loaded the left channel in-place. The right channel is left as a TODO in the future

mentioned the benefits of extern C in Q/A

touched upon what __stdcall is on windows

Keyword:
asset system, audio/sound, .wav file, .wav file parser, file loading, visual studio debugging



3:44
Casey moved all the struct we defined for the .wav file parsing into the pragma pack and pop section. 
recall what pragma pack and pop does is that it specifies the byte alignment. This is first done in day 36,
where we defined the bitmap_header.

we want to make sure these are all pragma pack 1, so the compiler doesnt insert any additional spacing.

                #pragma pack(push, 1)
                struct bitmap_header
                {
                    ...
                    ...
                };

                struct WAVE_header
                {
                    ...
                    ...
                };

                #define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))
                enum
                {
                    WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
                    WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
                    WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
                    WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
                };
                struct WAVE_chunk
                {
                    ...
                    ...
                };

                struct WAVE_fmt
                {
                    ...
                    ...
                };

                #pragma pack(pop)


6:11
Casey debugging the DEBUGLaodWAV function(); 
one smart thing he did during the debugging is that when he was checking the value for Header.RIFFID
which should be 'R' 'I' 'F' 'F'

he displayed Header.RIFFID in hexadecimal in the deubber Watch window,

then he typed 'R', 'I', 'F', 'F' into the debugger and checked if Header.RIFFID has the correct value.

just another small tip for debugging.




7:50
Casey mentioning how he wants to approach parsing/debugging the WAV file.
he says he wants to have cursor in the file that will helps us with the booking keeping in reading through the file

So Casey defined a struct for that 
-   uint8 *At;
    indicates where we currently are 

-   uint8 *Stop; 
    indicates where to stop


                struct riff_iterator
                {
                    uint8 *At;
                    uint8 *Stop;
                };




10:04
Casey using the riff_iterator in DEBUGLoadWAV();
the idea is that we are reading the file chunk by chunk.

so in the main for loop, we first get ParseChunkAt(Header + 1, (uint8 *)(Header + 1) + Header->Size - 4);
that is supposed to get all the content in the first chunk.

then we continue to read, and get the NextChunk();

essentially, we are reading chunk by chunk


                internal loaded_sound
                DEBUGLoadWAV(char *FileName)
                {
                    loaded_sound Result = {};
                    
                    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);    
                    if(ReadResult.ContentsSize != 0)
                    {
                        WAVE_header *Header = (WAVE_header *)ReadResult.Contents;
                        ...
                        ...

                        for(riff_iterator Iter = ParseChunkAt(Header + 1, (uint8 *)(Header + 1) + Header->Size - 4);
                            IsValid(Iter);
                            Iter = NextChunk(Iter))
                        {
                            ...
                            ...
                        }

                        ...
                        ...
                    }

                    return(Result);
                }




14:02
so casey wrote a ParseChunkAt function, which takes in the start and the end to parse

                inline riff_iterator
                ParseChunkAt(void *At, void *Stop)
                {
                    riff_iterator Iter;

                    Iter.At = (uint8 *)At;
                    Iter.Stop = (uint8 *)Stop;

                    return(Iter);
                }


notice up in the for loop, we pass "Header + 1" to the void *At argument 
for those of you who are not familar with struct pointer arithmetic

                Header + 1; 

is the same as 

                Header + sizeof(struct Header);
https://stackoverflow.com/questions/7623704/pointer-arithmetic-for-structs


so passing Header + 1, just means the byte after your WAVE_header struct.
assume all cc is the your WAVE_header content Header + 1 now points to ab
                
            Header                    Header + 1                    
                |                       |
                |                       |
                v                       v                                        
                cc cc cc cc cc cc cc cc ab cd ef gh
                |                     |  
                |.....WAVE_header.....|   




13:55
we also write the NextChunk(); function
we will just advance Iter.At by the chunk size, so that we start on the next chunk

-   Casey looked up the specifications of chunks at 26:33, which says it says 
                "If the chunk size is an odd number of bytes, a pad byte with value zero is written after ckData,
                Word aligning improces access speed (for chunks resident in memory) and maintains compatibility with EA IFF
                The ckSize value does not include the pad byte" 

    which is what the following line is doing
                uint32 Size = (Chunk->Size + 1) & ~1; 

-   (~ tilda) is the bitwise complement operator.
    which is essentially inverting bits. 
    so if you have ~2, you get -3
    2 is 0000 0010, by inverting it, you get 1111 1101, which is -3

    so here when we have ~1, 
    1 is 0000 0001
    ~1 gives you 1111 1110

    so n & ~1; is just getting all bits except for the last bit,
    which is flooring your n into an even number.  

    so for (Chunk->Size + 1) & ~1;
    if Chunk->Size is odd,
    (3 + 1) & ~1 = 4
    
    if chunk->Size is even
    (2 + 1) & ~1 = 2

    so this operation (Chunk->Size + 1) & ~1 to next even number;

    https://stackoverflow.com/questions/791328/how-does-the-bitwise-complement-operator-tilde-work


-   at 1:13:49 casey offers another explanation 
    
    (Chunk->Size + 1) & ~1; really is (Chunk->Size + (2 - 1)) & ~(2 - 1);

    where 2 here is the number you want to align to



    recall in day 127, we wrote the function          
            #define Align16(Value) ((Value + 15) & ~15)

    its the same idea.

    the generalized formula is 
            #define AlignN(Value, N) ((Value + (N-1)) & ~(N-1))


    Do Note that N has to be power of 2
    
                inline riff_iterator
                NextChunk(riff_iterator Iter)
                {
                    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
                    uint32 Size = (Chunk->Size + 1) & ~1;
                    Iter.At += sizeof(WAVE_chunk) + Size;

                    return(Iter);
                }









15:53
now Casey writes the IsValid function;


                inline bool32
                IsValid(riff_iterator Iter)
                {    
                    bool32 Result = (Iter.At < Iter.Stop);
                    
                    return(Result);
                }




then inside the for loop, once we have our chunk, we want to examine what type this chunk, 
so we do a switch statement checking the type of this chunk

we only care about the fmt chunk (format chunk); and a data chunk

-   notice that in the WAVE_ChunkID_fmt, the way we read in the data is just by casting our byte pointer
    into our WAVE_fmt struct. That is a very neat way of parsing.

    20:30 Casey does a debug pause in the debugger to check if we are actually getting the fmt chunk from the 
    GetChunkData(Iter); call

    with that, we do a bunch of sanity checks. (By doing a bunch of Asserts)

-   28:06 in the ParseChunkAt(); function, we pass in (uint8 *)(Header + 1) + Header->Size - 4);
    we have the - 4 becuz the specs says so.


                for(riff_iterator Iter = ParseChunkAt(Header + 1, (uint8 *)(Header + 1) + Header->Size - 4);
                    IsValid(Iter);
                    Iter = NextChunk(Iter))
                {
                    switch(GetType(Iter))
                    {
                        case WAVE_ChunkID_fmt:
                        {
                            WAVE_fmt *fmt = (WAVE_fmt *)GetChunkData(Iter);
                            ...
                            ...
                        } break;

                        case WAVE_ChunkID_data:
                        {
                            ...
                            ...
                        } break;
                    }
                }

17:31
Casey writes the GetChunkData();

                inline void *
                GetChunkData(riff_iterator Iter)
                {
                    void *Result = (Iter.At + sizeof(WAVE_chunk));

                    return(Result);
                }







29:34
Casey added a bunch of Asserts to make sure values fmt are what we expected

while parsing, we record three important data, ChannelCount, SampleDataSize and SampleData.
we will use these 3 later on.

                internal loaded_sound
                DEBUGLoadWAV(char *FileName)
                {
                    ...
                    ...

                    if(ReadResult.ContentsSize != 0)
                    {
                        WAVE_header *Header = (WAVE_header *)ReadResult.Contents;
                        ...
                        ...

                        uint32 ChannelCount = 0;
                        uint32 SampleDataSize = 0;
                        int16 *SampleData = 0;
                        for(riff_iterator Iter = ParseChunkAt(Header + 1, (uint8 *)(Header + 1) + Header->Size - 4);
                            IsValid(Iter);
                            Iter = NextChunk(Iter))
                        {
                            switch(GetType(Iter))
                            {
                                case WAVE_ChunkID_fmt:
                                {
                                    WAVE_fmt *fmt = (WAVE_fmt *)GetChunkData(Iter);
                                    Assert(fmt->wFormatTag == 1); // NOTE(casey): Only support PCM
                                    Assert(fmt->nSamplesPerSec == 48000);
                                    Assert(fmt->wBitsPerSample == 16);
                                    Assert(fmt->nBlockAlign == (sizeof(int16)*fmt->nChannels));
                                    ChannelCount = fmt->nChannels;
                                } break;

                                case WAVE_ChunkID_data:
                                {
                                    SampleData = (int16 *)GetChunkData(Iter);
                                    SampleDataSize = GetChunkDataSize(Iter);
                                } break;
                            }
                        }

                        Assert(ChannelCount && SampleData);
                        ...
                        ...
                    }
                }



33:03
now after getting the needed data, we start to generated a loaded_sound struct that we can use in our game

at 33:42 Casey redefines the loaded_sound was defined as follow:

Casey wants to introduce the concept that the loaded_sound multiple channels, (you can have a stereo or mono sound.)
musical sound will probably come in stereo, sound effects will probably want to come from mono
hence the ChannelCount variable

                handmade_asset.h

                struct loaded_sound
                {
                    uint32 SampleCount;
                    uint32 ChannelCount;
                    int16 *Samples[2];
                };


then we just proceed to get SampleCount, and process each channel

-   Note that we get the ChannelCount, and store it in our loaded_sound struct.
    also we calculate the Result.SampleCount by doing the division SampleDataSize / (ChannelCount*sizeof(int16));

    the reason why we are doing sizeof(int16) is becuz our file has (fmt->wBitsPerSample == 16); which is 2 bytes


-   we have the case if ChannelCount == 1 and if ChannelCount == 2, otherwise we treat it as invalid channel count in 
the WAV file 


                internal loaded_sound
                DEBUGLoadWAV(char *FileName)
                {
                    loaded_sound Result = {};

                    ...
                    if(ReadResult.ContentsSize != 0)
                    {
                        WAVE_header *Header = (WAVE_header *)ReadResult.Contents;
                        ...
                        ...

                        getting ... ChannelCount;
                                ... SampleDataSize;
                                ... SampleData;

                        ...
                        ...

                        Assert(ChannelCount && SampleData);
                        ...
                        ...


                        Result.ChannelCount = ChannelCount;
                        Result.SampleCount = SampleDataSize / (ChannelCount*sizeof(int16));
                        if(ChannelCount == 1)
                        {
                            ...
                            ...
                        }
                        else if(ChannelCount == 2)
                        {
                            ...
                            ...
                        }
                        else
                        {
                            Assert(!"Invalid channel count in WAV file");
                        }

                        // TODO(casey): Load right channels!
                        Result.ChannelCount = 1;
                    }

                    return result;
                }


37:06
if the channel count is one, then the parsing is rather simple 
    we put the sample data in our first channel, and set the other channel to 0.

                if(ChannelCount == 1)
                {
                    Result.Samples[0] = SampleData;
                    Result.Samples[1] = 0;
                }


if the channel count is 2, the audio files will have interleaved sample data for each channel 
    Left 0      Left 1      Left 2      Left 3 ...
        Right 0     Right 1     Right 2     Right 3 ... 

so what we want to do is un-interleaved the Channel data, and put it in Result.Samples[0] and Result.Samples[1] 
separately. 

Casey aims to do the un-interleaved in place 

so what happens here is that 
our original SampleData that we got from the .wav file is 

            SampleData 
                
                |
                |
                V

                l0 r0 l1 r1 l2 r2 l3 r3 l4 r4 l5 r5 

Result.SampleCount is half of this (recall this is becuz we divided SampleDataSize divided by the ChannelCount)

so SampleData + Result.SampleCount just points to the halfway of this sampleData array

            channel 0           channel 1
            SampleData          SampleData + Result.SampleCount;
                        
                |                 |
                |                 |
                V                 V  

                l0 r0 l1 r1 l2 r2 l3 r3 l4 r4 l5 r5 

                0  1  2  3  4  5  6  7  8  9  10 11


this way we dont have to allocate new array. We just do it in place.


-   the for loop goes from 0 to Result.SampleCount, so its just half of this sampleData array.
    
    what this for loop is doing is just getting the left channels all sorted.

    it goes from index 0 to 5, and tries to swap all the left channels data in. 

    so when sampleIndex == 0, we want L0 to be in SampleData[0], so it grabs L0 from SampleData[0]

    then sampleIndex == 1, we want L1 to be in SampleData[1], so it grabs L1 from SampleData[1]

    then sampleIndex == 2, we want L2 to be in SampleData[2], so it grabs L2 from SampleData[2]

    ...
    ...

    you get the idea.

    "int16 Source = SampleData[2*SampleIndex];"
    is where the left channel currently is 

    "SampleData[SampleIndex]" is where we want the left channel to be. 

    and we just swap the two


    of course this only sort our left channel. Our right channel is still unsorted.

    50:22
    Casey made the decision to only load the left channel for now. we will worry about the right channel later.


-   #if 0 and #endif part 
    is used to help debugging. Casey was populating the SampleData with 1 1 2 2 3 3 4 4 5 5 .... to help debug



                else if(ChannelCount == 2)
                {
                    Result.Samples[0] = SampleData;
                    Result.Samples[1] = SampleData + Result.SampleCount;

        #if 0
                    for(uint32 SampleIndex = 0;
                        SampleIndex < Result.SampleCount;
                        ++SampleIndex)
                    {
                        SampleData[2*SampleIndex + 0] = (int16)SampleIndex;
                        SampleData[2*SampleIndex + 1] = (int16)SampleIndex;
                    }
        #endif
                    
                    for(uint32 SampleIndex = 0;
                        SampleIndex < Result.SampleCount;
                        ++SampleIndex)
                    {
                        int16 Source = SampleData[2*SampleIndex];
                        SampleData[2*SampleIndex] = SampleData[SampleIndex];
                        SampleData[SampleIndex] = Source;
                    }
                }


55:09
to get the loaded_sound to play in game, 
we hooked in up with the GAME_GET_SOUND_SAMPLES(); function 

we first added a TestSound variable in the game_state struct

                struct game_state
                {
                    ...
                    ...
                    loaded_sound TestSound;
                };


after we loaded the TestSound,
we play it in the GAME_GET_SOUND_SAMPLES function.
as you can see, we just straight up populating the SoundBuffer with values from the TestSound loaded_sound asset.

afterwards, we give it back and return it to the win32 layer.

recall that SoundBuffer is the argument passed in 

                handmade_platform.h

                #define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
                typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);




-   at 56:25 Casey pointed out that sound values are encoded as signed values, so dont use uint16 SampleValue (what he initially had)
    use int16 SampleValue


                extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
                {
                    game_state *GameState = (game_state *)Memory->PermanentStorage;
                //    GameOutputSound(GameState, SoundBuffer, 400);

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


1:02:25
When Casey first played the audio in our game 
it seems like it is playing the right audio file,
but it didnt sound complete correct.

Casey instantly knew cuz we are not hitting the smooth framerate. 

so Casey changed it to optimized build from debug build.

Also Casey changed it back so that we are synced with the monitor frame rate instead of 60 hz.
For specific details on frame rate, look up day 018


1:06:15
Q/A some one asked why do we have to un-interleaved the audio sampleData? When you play it you still have to 
read both left and right values.

The reason is becuz we dont process the values the same way, and we want to be Wide since we plan to do SIMD.  



1:06:39
someone asked about the benefits of extern C in Q/A. What issues does it address?

-   it prevents the names from getting mangled. 
    normally when you link, it mangles the name for C++ so that you can have operator overloading work properly with linkers
    that dont know about it.  

    that goes for dynamic linkage too. So if you do a DLL export, the name will be mangled in the DLL important table 
    if we hadnt cleared extern c.

-   it implies a calling convention. on x64 on windows, there is only 1 calling convention, the standard call (stdcall);
    In general standard call is used everywhere. 


    [from the following link, it says 
            "stdcall is the standard calling convention for the Microsoft Win32 API and for Open Watcom C++"

        https://en.wikipedia.org/wiki/X86_calling_conventions


        also from this link 
            "__stdcall is the calling convetion used for the function. This tells the compiler the rules that apply 
            for setting up the stack, pushing arguments and getting a return value."

            "__stdcall is the standard calling convetion for Win32 system calls

            ...

            it primarily matters when you are calling a function outside of your code (e.g an OS API) or the OS is 
            calling you (for example WinMain). If the compiler doesnt know about the correct calling convention, then you
            will likely get very strange crashes as the stack will not be managed correctly.
            "]

        https://stackoverflow.com/questions/297654/what-is-stdcall



    this is not true on 32 bit. On 32 bit there is multiple calling conventions. Theres Pascal, theres C style and so on.
    extern c lets you know its a __cdecl 


    [from the following link:

        "__cdecl is the “C” calling convention whereas 
        extern “C” is the “C” naming convention. "

        http://muktvijay.blogspot.com/2009/07/extern-c-and-cdecl.html]


