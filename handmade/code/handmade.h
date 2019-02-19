#if !defined(HANDMADE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

/*
  TODO(casey):

  - Flush all thread queues before reloading DLL!

  - Debug code
    - Logging
    - Diagramming
    - (A LITTLE GUI, but only a little!) Switches / sliders / etc.
    - Draw tile chunks so we can verify that things are aligned / in the chunks we want them to be in / etc.
    - Thread visualization
  
  - Audio
    - FIX CLICKING BUG AT END OF SAMPLES!!!

  - Rendering
    - Get rid of "even" scan line notion?
    - Real projections with solid concept of project/unproject
    - Straighten out all coordinate systems!
      - Screen
      - World
      - Texture
    - Particle systems
    - Lighting
    - Final Optimization

  ARCHITECTURE EXPLORATION
  - Z!
    - Need to make a solid concept of ground levels so the camera can
      be freely placed in Z and have mulitple ground levels in one
      sim region
    - Concept of ground in the collision loop so it can handle
      collisions coming onto _and off of_ stairwells, for example.
    - Make sure flying things can go over low walls
    - How is this rendered?
      "Frinstances"!
      ZFudge!!!!
  - Collision detection?
    - Fix sword collisions!
    - Clean up predicate proliferation!  Can we make a nice clean
      set of flags/rules so that it's easy to understand how
      things work in terms of special handling?  This may involve
      making the iteration handle everything instead of handling
      overlap outside and so on.
    - Transient collision rules!  Clear based on flag.
      - Allow non-transient rules to override transient ones.
      - Entry/exit?
    - What's the plan for robustness / shape definition?
    - (Implement reprojection to handle interpenetration)
    - "Things pushing other things"
  - Animation
    - Skeletal animation
  - Implement multiple sim regions per frame
    - Per-entity clocking
    - Sim region merging?  For multiple players?
    - Simple zoomed-out view for testing?
  - AI
    - Rudimentary monstar behavior example
    * Pathfinding
    - AI "storage"
    
  PRODUCTION
  -> GAME
    - Entity system
    - Rudimentary world gen (no quality, just "what sorts of things" we do)
      - Placement of background things
      - Connectivity?
      - Non-overlapping?
      - Map display
        - Magnets - how they work???
  - Metagame / save game?
    - How do you enter "save slot"?
    - Persistent unlocks/etc.
    - Do we allow saved games?  Probably yes, just only for "pausing",
    * Continuous save for crash recovery?
*/

#include "handmade_platform.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_random.h"
#include "handmade_file_formats.h"
#include "handmade_meta.h"
#include "handmade_cutscene.h"

#define DLIST_INSERT(Sentinel, Element)         \
    (Element)->Next = (Sentinel)->Next;         \
    (Element)->Prev = (Sentinel);               \
    (Element)->Next->Prev = (Element);          \
    (Element)->Prev->Next = (Element); 

#define DLIST_INIT(Sentinel) \
    (Sentinel)->Next = (Sentinel); \
    (Sentinel)->Prev = (Sentinel);

#define FREELIST_ALLOCATE(Result, FreeListPointer, AllocationCode)             \
    (Result) = (FreeListPointer); \
    if(Result) {FreeListPointer = (Result)->NextFree;} else {Result = AllocationCode;}
#define FREELIST_DEALLOCATE(Pointer, FreeListPointer) \
    if(Pointer) {(Pointer)->NextFree = (FreeListPointer); (FreeListPointer) = (Pointer);}

struct memory_arena
{
    memory_index Size;
    uint8 *Base;
    memory_index Used;

    int32 TempCount;
};

struct temporary_memory
{
    memory_arena *Arena;
    memory_index Used;
};

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

inline b32
StringsAreEqual(char *A, char *B)
{
    b32 Result = (A == B);

    if(A && B)
    {
        while(*A && *B && (*A == *B))
        {
            ++A;
            ++B;
        }

        Result = ((*A == 0) && (*B == 0));
    }
    
    return(Result);
}

inline b32
StringsAreEqual(memory_index ALength, char *A, memory_index BLength, char *B)
{
    b32 Result = (ALength == BLength);

    if(Result)
    {
        Result = true;
        for(u32 Index = 0;
            Index < ALength;
            ++Index)
        {
            if(A[Index] != B[Index])
            {
                Result = false;
                break;
            }
        }
    }

    return(Result);
}

//
//
//

inline void
InitializeArena(memory_arena *Arena, memory_index Size, void *Base)
{
    Arena->Size = Size;
    Arena->Base = (uint8 *)Base;
    Arena->Used = 0;
    Arena->TempCount = 0;
}

inline memory_index
GetAlignmentOffset(memory_arena *Arena, memory_index Alignment)
{
    memory_index AlignmentOffset = 0;
    
    memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
    memory_index AlignmentMask = Alignment - 1;
    if(ResultPointer & AlignmentMask)
    {
        AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
    }

    return(AlignmentOffset);
}

inline memory_index
GetArenaSizeRemaining(memory_arena *Arena, memory_index Alignment = 4)
{
    memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Alignment));

    return(Result);
}

// TODO(casey): Optional "clear" parameter!!!!
#define DEFAULT_MEMORY_ALIGNMENT 4
#define PushStruct(Arena, type, ...) (type *)PushSize_(Arena, sizeof(type), ## __VA_ARGS__)
#define PushArray(Arena, Count, type, ...) (type *)PushSize_(Arena, (Count)*sizeof(type), ## __VA_ARGS__)
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)
#define PushCopy(Arena, Size, Source, ...) Copy(Size, Source, PushSize_(Arena, Size, ## __VA_ARGS__))
inline memory_index
GetEffectiveSizeFor(memory_arena *Arena, memory_index SizeInit, memory_index Alignment)
{
    memory_index Size = SizeInit;
        
    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    Size += AlignmentOffset;

    return(Size);
}

inline b32
ArenaHasRoomFor(memory_arena *Arena, memory_index SizeInit, memory_index Alignment = DEFAULT_MEMORY_ALIGNMENT)
{
    memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Alignment);
    b32 Result = ((Arena->Used + Size) <= Arena->Size);
    return(Result);
}

inline void *
PushSize_(memory_arena *Arena, memory_index SizeInit, memory_index Alignment = DEFAULT_MEMORY_ALIGNMENT)
{
    memory_index Size = GetEffectiveSizeFor(Arena, SizeInit, Alignment);
    
    Assert((Arena->Used + Size) <= Arena->Size);

    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
    Arena->Used += Size;

    Assert(Size >= SizeInit);
    
    return(Result);
}

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

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;

    Result.Arena = Arena;
    Result.Used = Arena->Used;

    ++Arena->TempCount;

    return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
    memory_arena *Arena = TempMem.Arena;
    Assert(Arena->Used >= TempMem.Used);
    Arena->Used = TempMem.Used;
    Assert(Arena->TempCount > 0);
    --Arena->TempCount;
}

inline void
Clear(memory_arena *Arena)
{
    InitializeArena(Arena, Arena->Size, Arena->Base);
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

inline void
SubArena(memory_arena *Result, memory_arena *Arena, memory_index Size, memory_index Alignment = 16)
{
    Result->Size = Size;
    Result->Base = (uint8 *)PushSize_(Arena, Size, Alignment);
    Result->Used = 0;
    Result->TempCount = 0;
}

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count*sizeof((Pointer)[0]), Pointer)
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    // TODO(casey): Check this guy for performance
    uint8 *Byte = (uint8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

inline void *
Copy(memory_index Size, void *SourceInit, void *DestInit)
{
    u8 *Source = (u8 *)SourceInit;
    u8 *Dest = (u8 *)DestInit;
    while(Size--) {*Dest++ = *Source++;}

    return(DestInit);
}

#include "handmade_world.h"
#include "handmade_sim_region.h"
#include "handmade_entity.h"
#include "handmade_world_mode.h"
#include "handmade_render_group.h"
#include "handmade_asset.h"
#include "handmade_audio.h"

struct controlled_hero
{
    uint32 EntityIndex;
    
    // NOTE(casey): These are the controller requests for simulation
    v2 ddP;
    v2 dSword;
    real32 dZ;
};

struct ground_buffer
{
    // NOTE(casey): An invalid P tells us that this ground_buffer has not been filled
    world_position P; // NOTE(casey): This is the center of the bitmap
    loaded_bitmap Bitmap;
};

struct hero_bitmap_ids
{
    bitmap_id Head;
    bitmap_id Cape;
    bitmap_id Torso;
};

enum game_mode
{
    GameMode_TitleScreen,
    GameMode_CutScene,
    GameMode_World,
};

struct game_state
{
    bool32 IsInitialized;

    memory_arena ModeArena;
    memory_arena AudioArena; // TODO(casey): Move this into the audio system proper!

    controlled_hero ControlledHeroes[ArrayCount(((game_input *)0)->Controllers)];

    loaded_bitmap TestDiffuse; // TODO(casey): Re-fill this guy with gray.
    loaded_bitmap TestNormal;

    audio_state AudioState;
    playing_sound *Music;

    game_mode GameMode;
    union
    {
        game_mode_title_screen *TitleScreen;
        game_mode_cutscene *CutScene;
        game_mode_world *WorldMode;
    };
};

struct task_with_memory
{
    bool32 BeingUsed;
    memory_arena Arena;

    temporary_memory MemoryFlush;
};

struct transient_state
{
    bool32 IsInitialized;
    memory_arena TranArena;    

    task_with_memory Tasks[4];

    game_assets *Assets;

    uint32 GroundBufferCount;
    ground_buffer *GroundBuffers;
    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;

    uint32 EnvMapWidth;
    uint32 EnvMapHeight;
    // NOTE(casey): 0 is bottom, 1 is middle, 2 is top
    environment_map EnvMaps[3];
};

global_variable platform_api Platform;

internal task_with_memory *BeginTaskWithMemory(transient_state *TranState);
internal void EndTaskWithMemory(task_with_memory *Task);
internal void SetGameMode(game_state *GameState, game_mode GameMode);

// TODO(casey): Get these into a more reasonable location?
#define GroundBufferWidth 256
#define GroundBufferHeight 256

#define HANDMADE_H
#endif
