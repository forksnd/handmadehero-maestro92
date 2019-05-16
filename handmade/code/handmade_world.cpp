/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

// TODO(casey): Think about what the real safe margin is!
#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILES_PER_CHUNK 8

#define TILE_CHUNK_UNINITIALIZED INT32_MAX

inline world_position
NullPosition(void)
{
    world_position Result = {};

    Result.ChunkX = TILE_CHUNK_UNINITIALIZED;

    return(Result);
}

inline bool32
IsValid(world_position P)
{
    bool32 Result = (P.ChunkX != TILE_CHUNK_UNINITIALIZED);
    return(Result);
}

inline bool32
IsCanonical(real32 ChunkDim, real32 TileRel)
{
    // TODO(casey): Fix floating point math so this can be exact?
    real32 Epsilon = 0.01f;
    bool32 Result = ((TileRel >= -(0.5f*ChunkDim + Epsilon)) &&
                     (TileRel <= (0.5f*ChunkDim + Epsilon)));

    return(Result);
}

inline bool32
IsCanonical(world *World, v3 Offset)
{
    bool32 Result = (IsCanonical(World->ChunkDimInMeters.x, Offset.x) &&
                     IsCanonical(World->ChunkDimInMeters.y, Offset.y) &&
                     IsCanonical(World->ChunkDimInMeters.z, Offset.z));

    return(Result);
}

inline bool32
AreInSameChunk(world *World, world_position *A, world_position *B)
{
    Assert(IsCanonical(World, A->Offset_));
    Assert(IsCanonical(World, B->Offset_));

    bool32 Result = ((A->ChunkX == B->ChunkX) &&
                     (A->ChunkY == B->ChunkY) &&
                     (A->ChunkZ == B->ChunkZ));

    return(Result);
}

inline void
ClearWorldEntityBlock(world_entity_block *Block)
{
    Block->EntityCount = 0;
    Block->Next = 0;
    Block->EntityDataSize = 0;
}

inline world_chunk **
GetWorldChunkInternal(world *World, int32 ChunkX, int32 ChunkY, int32 ChunkZ)
{
    TIMED_FUNCTION();

    Assert(ChunkX > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkY > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkZ > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkX < TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkY < TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkZ < TILE_CHUNK_SAFE_MARGIN);

    // TODO(casey): BETTER HASH FUNCTION!!!!
    uint32 HashValue = 19*ChunkX + 7*ChunkY + 3*ChunkZ;
    uint32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
    Assert(HashSlot < ArrayCount(World->ChunkHash));

    world_chunk **Chunk = &World->ChunkHash[HashSlot];
    while(*Chunk &&
           !((ChunkX == (*Chunk)->ChunkX) &&
             (ChunkY == (*Chunk)->ChunkY) &&
             (ChunkZ == (*Chunk)->ChunkZ)))
    {
        Chunk = &(*Chunk)->NextInHash;
    }

    return(Chunk);
}

inline world_chunk *
GetWorldChunk(world *World, int32 ChunkX, int32 ChunkY, int32 ChunkZ,
              memory_arena *Arena = 0)
{
    world_chunk **ChunkPtr = GetWorldChunkInternal(World, ChunkX, ChunkY, ChunkZ);
    world_chunk *Result = *ChunkPtr;
    if(!Result && Arena)
    {
        Result = PushStruct(Arena, world_chunk, NoClear());
        Result->FirstBlock = 0;
        Result->ChunkX = ChunkX;
        Result->ChunkY = ChunkY;
        Result->ChunkZ = ChunkZ;

        Result->NextInHash = *ChunkPtr;
        *ChunkPtr = Result;
    }

    return(Result);
}

internal world_chunk *
RemoveWorldChunk(world *World, int32 ChunkX, int32 ChunkY, int32 ChunkZ)
{
    world_chunk **ChunkPtr = GetWorldChunkInternal(World, ChunkX, ChunkY, ChunkZ);
    world_chunk *Result = *ChunkPtr;
    if(Result)
    {
        *ChunkPtr = Result->NextInHash;
    }

    return(Result);
}

internal world *
CreateWorld(v3 ChunkDimInMeters, memory_arena *ParentArena)
{
    world *World = PushStruct(ParentArena, world);

    World->ChunkDimInMeters = ChunkDimInMeters;
    World->FirstFree = 0;
    SubArena(&World->Arena, ParentArena, GetArenaSizeRemaining(ParentArena), NoClear());

    return(World);
}

inline void
RecanonicalizeCoord(real32 ChunkDim, int32 *Tile, real32 *TileRel)
{
    // TODO(casey): Need to do something that doesn't use the divide/multiply method
    // for recanonicalizing because this can end up rounding back on to the tile
    // you just came from.

    // NOTE(casey): Wrapping IS NOT ALLOWED, so all coordinates are assumed to be
    // within the safe margin!
    // TODO(casey): Assert that we are nowhere near the edges of the world.

    int32 Offset = RoundReal32ToInt32(*TileRel / ChunkDim);
    *Tile += Offset;
    *TileRel -= (r32)Offset*ChunkDim;

    Assert(IsCanonical(ChunkDim, *TileRel));
}

inline world_position
MapIntoChunkSpace(world *World, world_position BasePos, v3 Offset)
{
    world_position Result = BasePos;

    Result.Offset_ += Offset;
    
    RecanonicalizeCoord(World->ChunkDimInMeters.x, &Result.ChunkX, &Result.Offset_.x);
    RecanonicalizeCoord(World->ChunkDimInMeters.y, &Result.ChunkY, &Result.Offset_.y);
    RecanonicalizeCoord(World->ChunkDimInMeters.z, &Result.ChunkZ, &Result.Offset_.z);

    return(Result);
}

inline v3
Subtract(world *World, world_position *A, world_position *B)
{
    v3 dTile = {(real32)A->ChunkX - (real32)B->ChunkX,
                (real32)A->ChunkY - (real32)B->ChunkY,
                (real32)A->ChunkZ - (real32)B->ChunkZ};

    v3 Result = Hadamard(World->ChunkDimInMeters, dTile) + (A->Offset_ - B->Offset_);

    return(Result);
}

inline b32
HasRoomFor(world_entity_block *Block, u32 Size)
{
    b32 Result = ((Block->EntityDataSize + Size) <= sizeof(Block->EntityData));
    return(Result);
}

inline void
PackEntityReference(entity_reference *Ref)
{
    if(Ref->Ptr != 0)
    {
        Ref->Index = Ref->Ptr->ID;
    }
}

inline void
PackTraversableReference(traversable_reference *Ref)
{
    PackEntityReference(&Ref->Entity);
}

internal void
PackEntityIntoChunk(world *World, entity *Source, world_chunk *Chunk)
{
    u32 PackSize = sizeof(*Source);

    if(!Chunk->FirstBlock || !HasRoomFor(Chunk->FirstBlock, PackSize))
    {
        if(!World->FirstFreeBlock)
        {
            World->FirstFreeBlock = PushStruct(&World->Arena, world_entity_block);
            World->FirstFreeBlock->Next = 0;
        }

        Chunk->FirstBlock = World->FirstFreeBlock;
        World->FirstFreeBlock = Chunk->FirstBlock->Next;

        ClearWorldEntityBlock(Chunk->FirstBlock);
    }

    world_entity_block *Block = Chunk->FirstBlock;

    Assert(HasRoomFor(Block, PackSize));
    u8 *Dest = (Block->EntityData + Block->EntityDataSize);
    Block->EntityDataSize += PackSize;
    ++Block->EntityCount;

    entity *DestE = (entity *)Dest;
    *DestE = *Source;
    PackTraversableReference(&DestE->Occupying);
    PackTraversableReference(&DestE->CameFrom);
    PackEntityReference(&DestE->Head);
}

internal void
PackEntityIntoWorld(world *World, entity *Source, world_position At)
{
    world_chunk *Chunk = GetWorldChunk(World, At.ChunkX, At.ChunkY, At.ChunkZ, &World->Arena);
    Assert(Chunk);
    PackEntityIntoChunk(World, Source, Chunk);
}

inline void
AddBlockToFreeList(world *World, world_entity_block *Old)
{
    Old->Next = World->FirstFreeBlock;
    World->FirstFreeBlock = Old;
}

inline void
AddChunkToFreeList(world *World, world_chunk *Old)
{
    Old->NextInHash = World->FirstFreeChunk;
    World->FirstFreeChunk = Old;
}
