Handmade Hero Day 137 - Matching Periodic Tags
Summary:

continuing on day 136, solved the problme of Periodic Tag Matching
derived the neighborhood distance operator formula for parameters in the range of [-halfPeriod, halfPeriod]
added the period for each tag type in the asset system
implemented the Periodic Tag Matching in the BestMatchAsset function

added code to prase/load audio files.
talked about how WAVE files and IFF files format works

mentioned that MP3 is patented in Q/A, so Casey cant teach how to parse MP3 on stream

mentioned that even in a good case, FLAC compression can achieve 50% compression ratio,
it is still not good enough to be significant, and not worth it to introduce all the code and complexity for it.


Keyword:
math, asset system, audio/sound, .wav file, file loading, audio compression, FLAC, MP3


3:21 
in order to handle all tags uniformly, perhaps we can introduce the concept that all tags are periodic.
for tags that we dont want to treat it as periodic, we just give them a very large period.

on day 136, we use the atan2, which returns a value from [-pi, pi].
our value goes from -180 to 0 to 180. Then 180 wraps around to -180


                           pi/2
                      . . . ^
                   .        |
                .           |
         a   .              |
           .                |
          .                 |
         .                  |
        .                   |
        .                   |
    pi  .                   |
        ----------------------------------------> x
    -pi .                   |
        .                   |
        .                   |
         .                  |
        b  .                |
             .              |  
               .            |
                 .          |
                      . . . |
                           
                          -pi/2



8:08
Casey says to solve this problem, we will use the "neighborhood operator"
what this operator does is that it takes two input values, and it produces their closest possible proximity.

for example, in the example above, lets say a is 135 and b is -150.
if you want a to be placed in the neighborhood of b, you will want the negative version of a, which is 
-(180 + 45) = -225

this way a is gonna be in the neighborhood of b.


10:29
for the sake of us comparing two angles, we only need "neighborhood distance"
Sometimes, you need the actual "neighborhood-ed" value.

for example in interpolation, and we are interpolating from B to A,
we actually care about getting the "neighborhood-ed" value of a, which is -225


but here in this case, we just care the effective difference between two angles 



11:34
for outputs of atan2
if two angles are both positive, they are in the same neighborhood
if two angels are both negative, they are in the same neighborhood

this is becuz, if you have two positive angles a and b, there is no way that the negative version of a is closer 
to b. Therefore angles with the same sign are already in the same neighborhood.

example:
a is 135 or -225 (the negative version of itself)
b is 120

numerically abs(135 - 120); > abs(-225 - 120);
hence angles with the same sign are in the same neighborhood.



we only have to do a neighborhood operator if they have opposite signs.



here, if we are comparing a and b, we will convert one of them to the others neighborhood. 

                           pi/2
                      . . . ^
                   .        |
                .           |
         a   .              |
           .                |
          .                 |
         .                  |
        .                   |
        .                   |
    pi  .                   |
        ----------------------------------------> x
    -pi .                   |
        .                   |
        .                   |
         .                  |
        b  .                |
             .              |  
               .            |
                 .          |
                      . . . |
                           
                          -pi/2

recall our full period is from [-pi, pi]

here b is in the negative half of the period [-pi, 0], to map it to a_s neighborhood,
we just add the full period. 

so we get b + 2 * pi


maping a to b_s neighborhood is the opposite.
a is the upper half of the period [0, pi], so we just subtract the full period from it 

so we get a - 2 * pi


so the formula is 
so the neighorhood operator is: (a - range * sign(a));



19:09
so what we will eventually do is just compare 
and see which one is smaller 

        { abs (a - b) }
    min 
        { abs(a - (b - range * sign(b))}



*******************
note that this only works for angles within [-pi, pi]

in a generalized case, you have two values near both ends of your period. So one value is higher, the other lower.
as long as they are further apart than half of your period, we will encounter a wrap-around issue.

and you either map the lower one to the high neighborhood by adding the full range from that value 
or you map the higher one to the low neighborhood by subtracting the full range from that value 

here, we are using the sign of your angle to indicate which half of the period are you in.

if you are positive, that means you are in the [0, pi] half, which is the higher half.
if you are negative, that means youare in the [-pi, 0] half, which is the lower half.

and we use that to map yourself either to the higher/lower neighborhood

so if your angle is not in [-pi, pi] range, this formula falls apart.
for example, if your angle is in [0, 360]


[0, 360]

a = 330
b = 90 



                           b = 90
                      . . . ^ . . .
                   .        |        .
                .           |           .
             .              |              .
           .                |                .
          .                 |                 .
         .                  |                  .
        .                   |                   .
        .                   |                   .
        .                   |                   .
        ----------------------------------------> x
        .                   |                   .
        .                   |                   .
        .                   |                   .
         .                  |                  .   a = 330
           .                |                .
             .              |              .
               .            |            .
                 .          |          .
                      . . . | . . . 
                           
                          -pi/2


what we really want is 120 degress difference 

but if we use the formula for the [-pi, pi] 

abs(90 - 330) = 240;
abs( (90 - 360) - 330 ) = -600 

clearly incorrect



in a generalized case, if your period is symmetric around zero, where half of theres half period
above and below zero, then the below formula will work
   
                a = [-halfPeriod, halfPeriod]


so it is actually pretty smart to limit your input in such a range. That way your neighborhood distance 
formula can utilize the sign. Otherwise, in the case where your angle is [0, 360], it will involve quite a bit of 
if condition to test who is in the higher or lower value.


                    { abs (a - b) }
                min  
                    { abs(a - (b - fullRange * sign(b))}



21:01
Casey proceeds to implement this formula in the game 
first we are given the real32 TagRange[Tag_Count]

Note that TagCount is defined in asset_tag_id
                enum asset_tag_id
                {
                    Tag_Smoothness,
                    Tag_Flatness,
                    Tag_FacingDirection, // NOTE(casey): Angle in radians off of due right
                    
                    Tag_Count,
                };


the idea is this will be a straight up array, storing the period of each type of asset_Tag_id

                struct game_assets
                {
                    // TODO(casey): Not thrilled about this back-pointer
                    ...
                    ...
                    real32 TagRange[Tag_Count];
                    
                    ...
                    ...

                    uint32 TagCount;
                    asset_tag *Tags;

                    uint32 AssetCount;
                    asset *Assets;
                    
                    asset_type AssetTypes[Asset_Count];

                    ...
                    ...
                };





For example, using the table drawing from day 136

    asset_type AssetTypes[Asset_Count];                         
                                            asset *Assets;                  asset_tag              
                 ___________                                    
                | Asset_    |               hero0   -------------------->   Tag_FacingDirection: up  
                |  hero     |               hero1   -----------             Tag_Height: 180
                |___________|               hero2              \            Tag_Weight: 155 lbs
                | Asset_    |               hero3               \           Tag_Gender: male
                |  Shadow   |               shadow0              ------>    Tag_FacingDirection: left
                |___________|               shadow1                         Tag_Height: 180
                | Asset_    |  --------->   shadow2                         Tag_Weight: 155 lbs
                |  Tree     |               shadow3                         Tag_Gender: male
                |___________|               shadow4                         ...
                |           |               tree0   -------------------->   Tag_Color: green                          
                |  ...      |               tree1                           Tag_Type: oak
                |___________|               tree2   -------------------->   Tag_Color: yellow             
                |           |               tree3                           Tag_Type: oak
                |  ...      |               tree4
                |___________|               tree5
                |           |               ...
                |           |               ...
                |___________|



in the asset_array, we have a list of ID-Value Pair. To look up each asset_tag_s period,
we will just use ID to look it up inside the "real32 TagRange[Tag_Count];" array 
                

our TagRange looks something like below 

                TagRange
                index                                   Value
                (int)Tag_FacingDirection                360
                (int)Tag_Height                         1000000000
                (int)Tag_Weight                         1000000000
                (int)Tag_Gender                         1000000000




and to look it up the ranges, we just use ID to index into the TagRange array

                asset_tag *Tags;                                    real32 TagRange[Tag_Count];

                Tag_FacingDirection: up    --------------->         range = TagRange[Tag_FacingDirection];
                Tag_Height: 180                                     range = TagRange[Tag_Height];
                Tag_Weight: 155 lbs                                 range = TagRange[Tag_Weight];
                Tag_Gender: male                                    range = TagRange[Tag_Gender];
                Tag_FacingDirection: left                           range = TagRange[Tag_FacingDirection];
                Tag_Height: 180                                     ...
                Tag_Weight: 155 lbs                                 ...
                Tag_Gender: male
                ...
                Tag_Color: green         
                Tag_Type: oak
                Tag_Color: yellow        
                Tag_Type: oak
                           



21:16
in the AllocateGameAssets(); Casey proceeds to initalize the TagRanges
as you can see, we are init them with a gigantic near infinite value.
But for the Tag_FacingDirection, we set it to Tau32, which is 2 pi

                internal game_assets *
                AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
                {
                    game_assets *Assets = PushStruct(Arena, game_assets);
                    SubArena(&Assets->Arena, Arena, Size);
                    Assets->TranState = TranState;

                    for(uint32 TagType = 0; TagType < Tag_Count; ++TagType)
                    {
                        Assets->TagRange[TagType] = 1000000.0f;
                    }
                    Assets->TagRange[Tag_FacingDirection] = Tau32;  

                    ...
                    ...

                }



22:40
in the BestMatchAsset(); function we implement the neighborhood distance comparison formula

                internal bitmap_id
                BestMatchAsset(game_assets *Assets, asset_type_id TypeID,
                               asset_vector *MatchVector, asset_vector *WeightVector)
                {
                    bitmap_id Result = {};

                    real32 BestDiff = Real32Maximum;
                    asset_type *Type = Assets->AssetTypes + TypeID;
                    for(uint32 AssetIndex = Type->FirstAssetIndex; AssetIndex < Type->OnePastLastAssetIndex; ++AssetIndex)
                    {
                        asset *Asset = Assets->Assets + AssetIndex;

                        real32 TotalWeightedDiff = 0.0f;
                        for(uint32 TagIndex = Asset->FirstTagIndex; TagIndex < Asset->OnePastLastTagIndex; ++TagIndex)
                        {
                            asset_tag *Tag = Assets->Tags + TagIndex;

  --------------->          real32 A = MatchVector->E[Tag->ID];
                            real32 B = Tag->Value;
                            real32 D0 = AbsoluteValue(A - B);
                            real32 D1 = AbsoluteValue((A - Assets->TagRange[Tag->ID]*SignOf(A)) - B);
                            real32 Difference = Minimum(D0, D1);
                            
                            real32 Weighted = WeightVector->E[Tag->ID]*Difference;
                            TotalWeightedDiff += Weighted;
                        }

                        if(BestDiff > TotalWeightedDiff)
                        {
                            BestDiff = TotalWeightedDiff;
                            Result.Value = Asset->SlotID;
                        }
                    }

                    return(Result);
                }







37:30 43:40
starting to work on loading audio
the code for loading the audio, as well as background streaming to load the audio files,
is very similar to that of the loading the bitmap 

                handmade_asset.cpp

                internal PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork)
                {
                    load_sound_work *Work = (load_sound_work *)Data;

                    asset_sound_info *Info = Work->Assets->SoundInfos + Work->ID.Value;
                    *Work->Sound = DEBUGLoadWAV(Info->FileName);

                    CompletePreviousWritesBeforeFutureWrites;
                    
                    Work->Assets->Sounds[Work->ID.Value].Sound = Work->Sound;
                    Work->Assets->Sounds[Work->ID.Value].State = Work->FinalState;
                    
                    EndTaskWithMemory(Work->Task);
                }


                internal void
                LoadSound(game_assets *Assets, sound_id ID)
                {
                    if(ID.Value &&
                       (AtomicCompareExchangeUInt32((uint32 *)&Assets->Sounds[ID.Value].State, AssetState_Unloaded, AssetState_Queued) ==
                        AssetState_Unloaded))
                    {    
                        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
                        if(Task)        
                        {
                            load_sound_work *Work = PushStruct(&Task->Arena, load_sound_work);

                            Work->Assets = Assets;
                            Work->ID = ID;
                            Work->Task = Task;
                            Work->Sound = PushStruct(&Assets->Arena, loaded_sound);
                            Work->FinalState = AssetState_Loaded;

                            PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadSoundWork, Work);
                        }
                    }    
                }


39:11 41:30
Casey also defined the loaded_sound struct and the sound_id struct. These are pretty much parallels
of the bitmap functions 

                struct loaded_sound
                {
                    int32 SampleCount;
                    void *Memory;
                };

                struct sound_id
                {
                    uint32 Value;
                };


44:15
Casey writing the DEBUGLoadWAV(); function

                internal loaded_sound
                DEBUGLoadWAV(char *FileName)
                {
                    loaded_sound Result = {};
                    
                    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);    
                    if(ReadResult.ContentsSize != 0)
                    {
                        WAVE_header *Header = (WAVE_header *)ReadResult.Contents;
                        Assert(Header->RIFFID == WAVE_ChunkID_RIFF);
                        Assert(Header->WAVEID == WAVE_ChunkID_WAVE);
                    }

                    return(Result);
                }


45:40
Casey says, the way that the .wav file worked is that, there is this concept of a chunked file format. 

there is this concept of "IFF" Format, by EA
The way that they worked is that they try to create a generic way of encoding hiearchical data. 
essentially .wav file is just "chunks" after "chunks" or even "chunks" put together hiearchically.


within a chunk, it will first contain an "ID" field. The "ID" field are 4 bytes. 

The 4 bytes are mapped to actual ASCII characters. 
Some possible IDs such as "WAVE", "data", "RIFF" ... (these are defined in the offical documents)
So its actually 4 characters packed together. This specifies the type of chunk you are.

then it typically contains a "size" field. This indicates the size of your chunk.

so as a single chunks, it kind of looks like 

                     _________
                    |ID: WAVE |
                    |_________|
                    |   SIZE  |
                    |_________|
                    |         |
                    |         |               
                    |_________|





for example, you can chunks after each other, and you can also have nested chunks



                     _________
                    |   ID    |
                    |_________|
                    |   SIZE  |
                    |_________|
                    |         |
                    |   ____  |
                    |  | ID | |
                    |  |____| |
                    |  |SIZE| |
                    |  |____| |
                    |  |    | |
                    |  |____| |
                    |_________|
                    |   ID    |
                    |_________|
                    |   SIZE  |
                    |_________|
                    |         |
                    |         |
                    |         |
                    |         |
                    |         |  
                    |         |
                    |_________|

it is not a particular good way of doing file format. Not recommended. it is an old school way of doing things. 
you can think of it is like HTML in binary. 




49:21
Casey refers to the following Link, 
as you scroll through you can see all the different types of chunks: fmt chunk, fact chunk, data chunk, Non-PCM Data ....
http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html




Casey proceeds to write the code to parse a WAV file,  

Casey first wrote the WAVE_header struct that matches the table listed under "Wave File Format" 
(noticed that it says "The data is stored in little-endian byte order")

                struct WAVE_header
                {
                    uint32 RIFFID;
                    uint32 Size;
                    uint32 WAVEID;
                };

should be straight forward. pretty much a 1:1 mapping and copied over here.


50:33
then Casey wrote the struct that matches the table under the "fmt Chunk" section.
this part does have the first two fields: ckID and cksize

ckID means chunk ID, cksize means chunk size 

                struct WAVE_fmt
                {
                    uint16 wFormatTag;
                    uint16 nChannels;
                    uint32 nSamplesPerSec;
                    uint32 nAvgBytesPerSec;
                    uint32 nBlockAlign;
                    uint16 wBitsPerSample;
                    uint16 cbSize;
                    uint16 wValidBitsPerSample;
                    uint32 dwChannelMask;
                    uint8 SubFormat[16];
                };


52:23
Casey said that the "WAVE_FORMAT_PCM" format is the standard

52:38
Casey putting in the WAVE_Chunk. These two are gonna be ckID and cksize that we didnt include in the WAVE_fmt struct.

                struct WAVE_chunk
                {
                    uint32 ID;
                    uint32 Size;
                };



[*i guess the WAVE_header struct that Casey wrote above can also utilize this WAVE_chunk class

                struct WAVE_header
                {
                    WAVE_chunk WaveChunk;
                    uint32 WAVEID;
                };                             ]




53:16
as mentioned previously. The WAVE_Chunk id is just four characters packed together.
and within the IFF files, there are a few possibilities, such as "fmt ", "RIFF", "WAVE" or "data"

So Casey defining the enums for these possibilities

                #define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))
                enum
                {
                    WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
                    WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
                    WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
                };




55:13 
Casey then proceeding to write the parser

                internal loaded_sound
                DEBUGLoadWAV(char *FileName)
                {
                    loaded_sound Result = {};
                    
                    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);    
                    if(ReadResult.ContentsSize != 0)
                    {
                        WAVE_header *Header = (WAVE_header *)ReadResult.Contents;
                        Assert(Header->RIFFID == WAVE_ChunkID_RIFF);
                        Assert(Header->WAVEID == WAVE_ChunkID_WAVE);
                    }

                    return(Result);
                }

Note that inside the if condition,
Casey added two lines of assert, one checking if RIFFID == WAVE_ChunkID_RIFF, the other WAVEID == WAVE_ChunkID_WAVE.
we are doing this cuz that is what it was defined in the spec under the "Wave File Format" Section. 

56:15
Casey reiterated that this is not real loading code, this is just for our own debugging purposes. 


1:03:07
how is MP3 patented?

MP3 is literally covered by a dozens of patents.  
https://www.tunequest.org/a-big-list-of-mp3-patents/20070226/

they are not patenting the file extension, the patent is on the technique used to create, encode and decode the audio.

so if you want to parse what is in a mp3 file, you are violating the patent just by parsing it. 




1:06:26
Casey commenting on FLAC compression rate. 
FLAC typically achieves around 69 ~ 74% compression ratio.  (around 25%)
in a good case you can get around 50%

lets say the total footprint for you game is 2GB, how much of that is audio?
assuming you have 0.5 GB out of 2 GB is audio. and you using FLAC to achieve 25% compression,  

you are talking about a savings of 100 MB ish in a thing that is 2GB redistributable. Why would you bother..
you are really just adding a bunch of code for no significant savings. 

even achieving 50% is not that useful. If you are trying to get stuff down, you need to get alot more than that. 
you want to achieve 8 to 1 to get to the ballpark of useful compression that changes the use cases that you can ship this in. 

50% is just not enough to care about FLAC compression. 

if you are going to introduce all the complexity, you might as well choose Ogg Vorbis, something that can get you 8 to 1.
and you can actually tune it, so you can get more than that if you want. 