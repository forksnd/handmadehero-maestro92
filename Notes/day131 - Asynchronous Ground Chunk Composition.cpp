Handmade Hero Day 131 - Asynchronous Ground Chunk Composition
Summary:
modified the PushSize(); function to be able to specify memory alignment

created the concept of TasksWithMemory.

allocated separate memory from the transient memory arena for these TasksWithMemory.

moved the FillGroundChunk(); work into these TasksWithMemory and move them in a LowPriorityQueue

removed the hiccup in the FillGroundChunk(); function

Keyword:
Memory, Asynchronous, multi-threading rendering


3:35
Casey showing that the framerate hiccup we are getting from ground chunk generation
1.  make the rendering easier
2.  multi-thread that



4:40
currently in the transient_state class, we have                

                struct transient_state
                {
                    bool32 IsInitialized;
                    memory_arena TranArena;    
                    uint32 GroundBufferCount;
                    ground_buffer *GroundBuffers;
                    platform_work_queue *HighPriorityQueue;
                    platform_work_queue *LowPriorityQueue;
                    uint64_t Pad;

                    ...
                    ...
                };

notice this uint64_t pad, in the last episode, we just put that pad there temporaily so that the 
GroundChunks are 16-byte aligned.

The goal is obviously is to move this Pad and the make the code more reliable

Recall for the transient_state struct, when we allocate memory for the transient_state,
we are just pushing groundChunks onto the TranState->TranArena.

we can see that we make the Transient->GroundBuffers = PushArray(); call

                handmade.cpp
                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {

                    ...
                    ...

                    // NOTE(casey): Transient initialization
                    transient_state *TranState = (transient_state *)Memory->TransientStorage;
                    if(!TranState->IsInitialized)
                    {
                        InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                                        (uint8 *)Memory->TransientStorage + sizeof(transient_state));

                        // TODO(casey): Pick a real number here!
                        TranState->HighPriorityQueue = Memory->HighPriorityQueue;
                        TranState->LowPriorityQueue = Memory->LowPriorityQueue;
                        TranState->GroundBufferCount = 256;
    --------------->    TranState->GroundBuffers = PushArray(&TranState->TranArena, TranState->GroundBufferCount, ground_buffer);

                        for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; ++GroundBufferIndex)
                        {
                            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
                            GroundBuffer->Bitmap = MakeEmptyBitmap(&TranState->TranArena, GroundBufferWidth, GroundBufferHeight, false);
                            GroundBuffer->P = NullPosition();
                        }
                                        ...
                        ...
                    }

                    ...
                    ...
                }

The MakeEmptyBitmap(); also calls the PushSize();

                handmade.cpp

                internal loaded_bitmap MakeEmptyBitmap(memory_arena *Arena, int32 Width, int32 Height, bool32 ClearToZero = true)
                {
                    loaded_bitmap Result = {};

                    Result.Width = Width;
                    Result.Height = Height;
                    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
                    int32 TotalBitmapSize = Width*Height*BITMAP_BYTES_PER_PIXEL;
                    Result.Memory = PushSize(Arena, TotalBitmapSize);
                    if(ClearToZero)
                    {
                        ClearBitmap(&Result);
                    }

                    return(Result);
                }


Graphically it looks like this 

                 ______________    <--- start of our transient memory  
                |______________|
                | trans        |  
                | state        |   
                |______________|   
                |              |   <--- PushStruct or PushArray
                | ground       |
                | chunk        |   
                |______________|      
                |              |   
                | ground       |
                | chunk        |   
                |______________|                  
                |              |   
                | ground       |
                | chunk        |   
                |______________|                 
                |              |   
                | ground       |
                | chunk        |   
                |______________|  


so as you can see, the size of transient_state effects where the ground chunks starts. And apparently, by adding a Pad in transient_state,
it will make our groundchunks start on a 16 byte aligned memory address


7:07
the goal is that when we do the call, MakeEmptyBitmap(); and the PushSize(); fnction inside,
to allocate memory for the Ground chunk, we want to make sure that it is 16 byte aligned

                handmade.cpp
                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...

                    // NOTE(casey): Transient initialization
                    transient_state *TranState = (transient_state *)Memory->TransientStorage;
                    if(!TranState->IsInitialized)
                    {
                        ...
                        ...

                        for(uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; ++GroundBufferIndex)
                        {

                            GroundBuffer->Bitmap = MakeEmptyBitmap(&TranState->TranArena, GroundBufferWidth, GroundBufferHeight, false);
                        }
                        ...
                    }
                    ...
                }


                internal loaded_bitmap MakeEmptyBitmap(memory_arena *Arena, int32 Width, int32 Height, bool32 ClearToZero = true)
                {
                    loaded_bitmap Result = {};

                    Result.Width = Width;
                    Result.Height = Height;
                    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
                    int32 TotalBitmapSize = Width*Height*BITMAP_BYTES_PER_PIXEL;
                    Result.Memory = PushSize(Arena, TotalBitmapSize);
                    if(ClearToZero)
                    {
                        ClearBitmap(&Result);
                    }

                    return(Result);
                }

7:50
so to accomplish that, Casey modifed the PushSize_ function to take in an extra argument: memory_index Alignment.
previously we have 

                inline void * PushSize_(memory_arena *Arena, memory_index Size)
                {
                    Assert((Arena->Used + Size) <= Arena->Size);
                    void *Result = Arena->Base + Arena->Used;
                    Arena->Used += Size;
                    
                    return(Result);
                }


essentially, what alignment do you want, or what bytes do you want your address to be aligned to
-   we will say that this has to be a power of 2
-   we will default it to 4
    the GetAlignmentOffset(); gives you the amount of byte padding you need to get to the next 
    memory alignment boundary. 

                handmade.h

                ...
                ...

                #define PushStruct(Arena, type, ...) (type *)PushSize_(Arena, sizeof(type), ## __VA_ARGS__)
                #define PushArray(Arena, Count, type, ...) (type *)PushSize_(Arena, (Count)*sizeof(type), ## __VA_ARGS__)
                #define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)

                inline void * PushSize_(memory_arena *Arena, memory_index SizeInit, memory_index Alignment = 4)
                {
                    memory_index Size = SizeInit;
                        
                    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
                    Size += AlignmentOffset;
                    
                    Assert((Arena->Used + Size) <= Arena->Size);
                    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
                    Arena->Used += Size;

                    Assert(Size >= SizeInit);
                    
                    return(Result);
                }


the GetAlignmentOffset(); function is below: 

-   the offset needed to get to the next alignment boundary
    if ResultPointer is 0x0007, and we want 16 byte alignment. We want 9
    if ResultPointer is 0x0004, and we want 16 byte alignment. We want 12

    the the example for 0x0007. Alignment mask is 15, which is 01111

    then (ResultPointer & AlignmentMask); returns the extra bits from your previous boundary.
    the previous boundary in this ciase is 0x0000, so the extra amount of memory is 7.

    after that, we just compute teh offset needed to the next alignemnt by doing Alignment - (ResultPointer & AlignmentMask);
    16 - 7 = 9

                inline memory_index GetAlignmentOffset(memory_arena *Arena, memory_index Alignment)
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



19:19
notice we also change 

                #define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
                #define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
                #define PushSize(Arena, Size) PushSize_(Arena, Size)

to 

                #define PushStruct(Arena, type, ...) (type *)PushSize_(Arena, sizeof(type), ## __VA_ARGS__)
                #define PushArray(Arena, Count, type, ...) (type *)PushSize_(Arena, (Count)*sizeof(type), ## __VA_ARGS__)
                #define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)


## __VA_ARGS__. Its a macro that declares to accept a variable number of arguments much as a function can



24:54
the idea is that within the PushBitmap calls will happen synchronously, then the TiledRenderGroupToOutput();
will happen Asynchronously


                internal void
                FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
                {
                    temporary_memory GroundMemory = BeginTemporaryMemory(&TranState->TranArena);

                    // TODO(casey): Decide what our pushbuffer size is!
                    render_group *RenderGroup = AllocateRenderGroup(&TranState->TranArena, Megabytes(4));
                    Orthographic(RenderGroup, Buffer->Width, Buffer->Height, (Buffer->Width - 2) / Width);
                    Clear(RenderGroup, V4(1.0f, 0.0f, 1.0f, 1.0f));
                    
                    for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
                    {
                        for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
                        {
                            ...
                            ...

                            v2 Center = V2(ChunkOffsetX*Width, ChunkOffsetY*Height);

                            ...
                            ...
                            PushBitmap(RenderGroup, Stamp, 2.0f, V3(P, 0.0f), Color);
                        }
                    }
                    
                    TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, Buffer);
                    EndTemporaryMemory(GroundMemory);
                }

the thing is that if TiledRenderGroupToOutput will be Asynchronous, we cant call EndTemporaryMemory(); to release the memory
right afterwards.

So what we need to do there is to have someway of temporary storage area for background tasks.


26:22
what we like to do is to introduce the concept of sub-arenas that work inside a memory arean
so the idea is that we can take one memory_arena and break it up into smaller arenas. 

its essentially just pre segment and roughly estimiate regions for tasks so people dont overlap each other  

so Casey created a memory_arena for ground chunk filing. 


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

                    ...
                    ...

                };


30:20
we initalize the task_with_memory Tasks at startup

notice we added this SubArena function. What we want to do is to take the Task->Arena,
and carve out some sub-arena from it. 

                handmade.cpp

                extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
                {
                    ...
                    ...

                    transient_state *TranState = (transient_state *)Memory->TransientStorage;
                    if(!TranState->IsInitialized)
                    {
                        InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                                        (uint8 *)Memory->TransientStorage + sizeof(transient_state));

                        for(uint32_t TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex)
                        {
                            task_with_memory *Task = TranState->Tasks + TaskIndex;

                            Task->BeingUsed = false;
                            SubArena(&Task->Arena, &TranState->TranArena, Megabytes(1));
                        }

                        ...
                        ...
                    }
                }



56:21
Casey writes the SubArena function.

                handmade.h

                inline void
                SubArena(memory_arena *Result, memory_arena *Arena, memory_index Size, memory_index Alignment = 16)
                {
                    Result->Size = Size;
                    Result->Base = (uint8 *)PushSize_(Arena, Size, Alignment);
                    Result->Used = 0;
                    Result->TempCount = 0;
                }







34:46
we did some refactoring in the FillGroundChunk(); function, 
the main concept is that we only begin a task if we are not too busy in the background. Then we only allocate 
the temporary memory for that background task if we started it. When it finishes it, we release that temporary memory. 


let us take a look at the FillGroundChunk(); function. 

-   we see that we have a BeginTaskWithMemory(); function that returns a boolean. 
The idea is that when we call BeginTaskWithMemory();, its possible that there are already too many tasks running. 
so we are limiting the number of tasks we can launch this way. So if you are trying to 
spawn work for the background queue, you cant just spam it, you have to think about it for a bit.  

-   as you can see, in FillGroundChunk();, we push all the rendering jobs synchronously. But to actually rendering it,
we do that Asynchronously. As you can see, we push the FillGroundChunkWork function on to the LowPriorityQueue.

                handmade.cpp

                internal void
                FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
                {
                    task_with_memory *Task = BeginTaskWithMemory(TranState);
                    if(Task)
                    {

                        ...
                        ...

                        for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
                        {
                            for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
                            {
                                ...
                                ...
                                PushBitmap(RenderGroup, Stamp, 0.1f, V3(P, 0.0f));
                            }
                        }

                        Work->RenderGroup = RenderGroup;
                        Work->Buffer = Buffer;
                        Work->Task = Task;

                        PlatformAddEntry(TranState->LowPriorityQueue, FillGroundChunkWork, Work);
                    }
                }


here we see the FillGroundChunkWork(); function. 
then once we finish, we call EndTaskWithMemory();

                internal PLATFORM_WORK_QUEUE_CALLBACK(FillGroundChunkWork)
                {
                    fill_ground_chunk_work *Work = (fill_ground_chunk_work *)Data;
                    
                    RenderGroupToOutput(Work->RenderGroup, Work->Buffer);

                    EndTaskWithMemory(Work->Task);
                }





36:01
and we proceed to write the BeginTaskWithMemory(); function
as you can see that we just loop through the tasks and find the first one that is not being used.

                handmade.cpp

                internal task_with_memory *
                BeginTaskWithMemory(transient_state *TranState)
                {
                    task_with_memory *FoundTask = 0;

                    for(uint32 TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex)
                    {
                        task_with_memory *Task = TranState->Tasks + TaskIndex;
                        if(!Task->BeingUsed)
                        {
                            FoundTask = Task;
                            Task->BeingUsed = true;
                            Task->MemoryFlush = BeginTemporaryMemory(&Task->Arena);
                            break;
                        }
                    }

                    return(FoundTask);
                }

                inline void
                EndTaskWithMemory(task_with_memory *Task)
                {
                    EndTemporaryMemory(Task->MemoryFlush);

                    CompletePreviousWritesBeforeFutureWrites;
                    Task->BeingUsed = false;
                }



45:55
lets go into the details of how the FillGroundChunk function allocates memory.

-   notice the AllocateRenderGroup(); and PushStruct(); takes Task->Arena as the argument. They are allocating memory 
from the task arena. 

-   first in the PushStruct(); function, we allocate enough memory for the fill_ground_chunk_work struct;
The fill_ground_chunk_work function is essentially the data for the job that the threads will have 

we will initialize the fill_ground_chunk_work task and then push it onto the LowPriorityQueue.
idle threads will then take jobs off of the LowPriorityQueue and find out what they need to do based on the data in here.


                struct fill_ground_chunk_work
                {
                    render_group *RenderGroup;
                    loaded_bitmap *Buffer;
                    task_with_memory *Task;
                };


full code FillGroundChunk(); function below:

as you can see, we then go through all the PushBitmap functions to do all the splat work for the RenderGroup, then 
we give it all to the fill_ground_chunk_work struct.

recall that a job consists of the job data and the actual job description.
here the job data is fill_ground_chunk_work, and the job description is FillGroundChunkWork();


                handmade.cpp

                internal void
                FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
                {
                    task_with_memory *Task = BeginTaskWithMemory(TranState);
                    if(Task)
                    {
                        fill_ground_chunk_work *Work = PushStruct(&Task->Arena, fill_ground_chunk_work);
                        ...
                        ...

                        render_group *RenderGroup = AllocateRenderGroup(&Task->Arena, 0);

                        ...
                        ...

                        for(int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
                        {
                            for(int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
                            {
                                ...
                                ...
                                PushBitmap(RenderGroup, Stamp, 0.1f, V3(P, 0.0f));
                            }
                        }

                        Work->RenderGroup = RenderGroup;
                        Work->Buffer = Buffer;
                        Work->Task = Task;

                        PlatformAddEntry(TranState->LowPriorityQueue, FillGroundChunkWork, Work);
                    }
                }



50:02
also for the AllocateRenderGroup(); function, we are allocating memory for the RenderGroup from the Task Arena. 

obviously we dont know how large the RenderGroup will be, so we will just allocate whatever is remaining 
in the arena, and give it all to the RenderGroup.


                handmade_render_group.cpp

                internal render_group *
                AllocateRenderGroup(memory_arena *Arena, uint32 MaxPushBufferSize)
                {
                    render_group *Result = PushStruct(Arena, render_group);

                    if(MaxPushBufferSize == 0)
                    {
                        // TODO(casey): Safe cast from memory_uint to uint32?
                        MaxPushBufferSize = (uint32)GetArenaSizeRemaining(Arena);
                    }
                    Result->PushBufferBase = (uint8 *)PushSize(Arena, MaxPushBufferSize);

                    Result->MaxPushBufferSize = MaxPushBufferSize;
                    Result->PushBufferSize = 0;
                    
                    Result->GlobalAlpha = 1.0f;

                    // NOTE(casey): Default transform
                    Result->Transform.OffsetP = V3(0.0f, 0.0f, 0.0f);
                    Result->Transform.Scale = 1.0f;

                    return(Result);
                }


50:14
here we create the GetArenaSizeRemaining(); function
recall we change the PushSize_ function so that it considers AlignmentOffset.
essentially the size remaining needs to consider the amount of offset that will push us till the next alignment 
boundary.

                handemade.h 

                inline memory_index
                GetArenaSizeRemaining(memory_arena *Arena, memory_index Alignment = 4)
                {
                    memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Alignment));

                    return(Result);
                }



1:10:27
Casey showing us that it works now, and we dont see the hiccups
a few problems Casey mentioned:
-   we drawing those bitmaps the sametime we are writing into them, which is why you see some purple/pink flashes.
we can add a flag saying that the ground chunk bitmaps are being filled. and dont draw me until im finished


1:13:21
Casey doing a drawing of what the transient memory looks like.

tasks has its own region of memory. Tasks memory are totaly segregated. 

