Handmade Hero Day 034 - Tile Map Memory

Summary:
Allocates permenant and transient memory

uses permenant memory to allocate the world

Keyword: 
memory



0:44
the game industry runs on C and C++


11:00 
adding smooth scrolling


43:42
starting to talk about how we will allocate memory for the Tile Map

since we want our world to persist, it needs to be in persistant storage


the game_memory struct is in handmade_platform.h


                typedef struct game_memory
                {
                    bool32 IsInitialized;

                    uint64 PermanentStorageSize;
                    void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

                    uint64 TransientStorageSize;
                    void *TransientStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

                    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
                    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
                    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
                } game_memory;


the first chunk of our PermanentStorage is dedicated to our "game_state" struct,
hence, we start initalize memory for the "game_state WorldArena" after our "game_state"








47:30
                GameState->World = PushStruct(&GameState->WorldArena, world);
                world *World = GameState->World;
                World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

we are saying that the "world" and "tile_map" is gonna come from the &GameState->WorldArena pool of memory




47:41
                InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                               (uint8 *)Memory->PermanentStorage + sizeof(game_state));

we then want to initialize this memory arena

for the arguments we pass in 

        size:
        Memory->PermanentStorageSize - sizeof(game_state),

        Essentially we are saying, you can use all the rest of the memory to our memory arena cuz we do not have anything else 
        other than the game_state



        base:   
        (uint8 *)Memory->PermanentStorage + sizeof(game_state));

        as said, the first chunk is for our game_state




                InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                               (uint8 *)Memory->PermanentStorage + sizeof(game_state));




50:46

typedef size_t memory_index;


size_t:
it is a type able to represent the size of any object in bytes: size_t is the type returned by the sizeof operator
and is widely used in standard library to represent sizes and counts; 

source: http://www.cplusplus.com/reference/cstring/size_t/




52:10
we define the function declaration, as you can see have "memory_size Size"
the Size could be uint32 (if size exceed 4GB) or uint64. so we define it to be size_t (memory_index)

                internal void
                InitializeArena(memory_arena *Arena, memory_index Size, uint8 *Base)
                {
                    Arena->Size = Size;
                    Arena->Base = Base;
                    Arena->Used = 0;
                }

memory_arena is just a memory tracker. nothing special


52:48


                #define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))

                void *
                PushSize_(memory_arena *Arena, memory_index Size)
                {
                    Assert((Arena->Used + Size) <= Arena->Size);
                    void *Result = Arena->Base + Arena->Used;
                    Arena->Used += Size;
                    
                    return(Result);
                }


we using the macro like templates. 

                GameState->World = PushStruct(&GameState->WorldArena, world);

                (world*)PushSize_(&GameState->WorldArena, sizeof(world))
here are type casting the void* we get from PushSize_ into a (world*)
so what we really have is 

                void* pointer = PushSize_(&GameState->WorldArena, sizeof(world))
                
                void* PushSize_(memory_arena *Arena, memory_index Size)
                {
                    Assert((Arena->Used + Size) <= Arena->Size);
                    void *Result = Arena->Base + Arena->Used;
                    Arena->Used += Size;
                    
                    return(Result);

                }



1:01:25

                #define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))

PushArray is essentially pushing an array of PushStruct



1:02:44

                TileMap->TileChunks = PushArray(&GameState->WorldArena,
                                                TileMap->TileChunkCountX*TileMap->TileChunkCountY,
                                                tile_chunk);

                for(uint32 Y = 0; Y < TileMap->TileChunkCountY; ++Y)
                {
                    for(uint32 X = 0; X < TileMap->TileChunkCountX; ++X)
                    {
                        TileMap->TileChunks[Y*TileMap->TileChunkCountX + X].Tiles =
                            PushArray(&GameState->WorldArena, TileMap->ChunkDim*TileMap->ChunkDim, uint32);
                    }
                }


TileMap has an array of TileChunks, and it essentially has TileChunkCountX * TileChunkCountY amount

TileChunkCountX x TileChunkCountY
     _______________________________
    |       |       |       |       |
    |       |       |       |       |
    |_______|_______|_______|_______|
    |       |       |       |       |
    |       |       |       |       |
    |_______|_______|_______|_______|
    |       |       |       |       |
    |       |       |       |       |
    |_______|_______|_______|_______|

each Chunk has an array of tiles, which is what we are doing in the double for loop


chunkDim vs chunkDim
     ________________________________
    |__|__|__|       |       |       |
    |__|__|__|       |       |       |
    |__|__|__|_______|_______|_______|
    |        |       |       |       |
    |        |       |       |       |
    |________|_______|_______|_______|
    |        |       |       |       |
    |        |       |       |       |
    |________|_______|_______|_______|

in each tile chunk, we have chunkDim vs ChunkDim tiles. and we are using uint32 to represent tiles 

hence PushArray(&GameState->WorldArena, TileMap->ChunkDim*TileMap->ChunkDim, uint32);



1:07:00
we initially thought that we need to clear zero, but we realized that we cleared zero for all of our whole memory


personally, as an API, i think you should still clear zero, cuz whoever is calling this function expects a zeroed cleared 
chunk of memory



1:26:38
have the fp:fast compiler flag cuz we do not want to call into the c runtime math library 


1:32:45
everything in memory is obviously on dimensional



1:35:13
we may decide to use 17 x 9 chunks and pay for the cost of look ups. cuz they are not powers of 2 (as oppose to 16 x 16 chunks)



1:41:01 
why not use look up tables to get sin and cosine


the reason why u do not use look up tables anymore for most math operations.

the most expensive operation on a computer nowadays is memory access

memory access is 100x slower than just doing some cycles

lookup tables are expensive. so sine and cosine are fairly fast.



1:43:47
memory access is the slowest operation out there. There is not a single operation that is as slow as memory access

SSE tend to be 2 clocks
memory access is usually 200 clcoks




1:46:24
x87 fpu is legacy part, do not ever use that

