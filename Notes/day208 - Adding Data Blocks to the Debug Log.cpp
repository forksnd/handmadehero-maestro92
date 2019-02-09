Handmade Hero Day 208 - Adding Data Blocks to the Debug Log

Summary:
Decided to not use the introspection code to examine mouse picked entities in the debug system yet.
For now, will still just use the manually typed DEBUG_VALUE(); code.

added the concept of DebugEvent_OpenDataBlock and DebugEvent_CloseDataBlock in the debug system, which is very similar
to the DebugEvent_BeginBlock and DebugEvent_EndBlock for profiling blocks.

Keyword:
debug system 

4:59
Casey now thinking about how to add the introspection code to the debug system to actually inspect an entity mouse picked by the user 
Casey addressed a few problems:

first of all recall lin day 205, we have the following code 


    --------->  sim_entity *Entity = SimRegion->Entities + EntityIndex;

                for(uint32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; ++VolumeIndex)
                {
                    sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        

                    v3 LocalMouseP = Unproject(RenderGroup, MouseP);


                    if((LocalMouseP.x > -0.5f*Volume->Dim.x) && (LocalMouseP.x < 0.5f*Volume->Dim.x) &&
                       (LocalMouseP.y > -0.5f*Volume->Dim.y) && (LocalMouseP.y < 0.5f*Volume->Dim.y))
                    {
                        v4 OutlineColor = V4(1, 1, 0, 1);
                        PushRectOutline(RenderGroup, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z), Volume->Dim.xy, OutlineColor, 0.05f);

                        DEBUG_BEGIN_HOT_ELEMENT(Entity);
                        DEBUG_VALUE(Entity->StorageIndex);
                        DEBUG_VALUE(Entity->Updatable);
                        DEBUG_VALUE(Entity->Type);
                        DEBUG_VALUE(Entity->P);
                        DEBUG_VALUE(Entity->dP);
                        DEBUG_VALUE(Entity->DistanceLimit);
                        DEBUG_VALUE(Entity->FacingDirection);
                        DEBUG_VALUE(Entity->tBob);
                        DEBUG_VALUE(Entity->dAbsTileZ);
                        DEBUG_VALUE(Entity->HitPointMax);
                        DEBUG_BEGIN_ARRAY(Entity->HitPoint);
                        for(u32 HitPointIndex = 0;
                            HitPointIndex < Entity->HitPointMax;
                            ++HitPointIndex)
                        {
                            DEBUG_VALUE(Entity->HitPoint[HitPointIndex]);
                        }
                        DEBUG_END_ARRAY();
                        DEBUG_VALUE(Entity->Sword);
                        DEBUG_VALUE(Entity->WalkableDim);
                        DEBUG_VALUE(Entity->WalkableHeight);
                        DEBUG_END_HOT_ELEMENT();
                    }
                }

the problem is that sim_entity* Entity only exists transiently. Right now for just one frame, we unpack the sim_region,
we get all the sim_region_entity that we are interested, and then it goes away. So for that entity that the user wants 
inspect in the debug view, we have to somehow preserve its memory. 


8:50
so one thing we can do is that, in debug mode, when we do a sim_region simulation, instead of it using temporary memory,
we can have it use permenant memory. 



11:37
so the solution that Casey is going after is create a debug_event for capturing debug entity data.

so recall currently in our code base, 
we have DebugEvents for all of our other debug data, 
for example, when we call BEGIN_BLOCK(); and END_BLOCK(); we have call the RecordDebugEvent();


                #define BEGIN_BLOCK_(Counter, FileNameInit, LineNumberInit, BlockNameInit) {         \
                    ...
                    ...                                
    ------------>   RecordDebugEvent(Counter, DebugEvent_BeginBlock);}


                #define END_BLOCK_(Counter) \
    ------------>   RecordDebugEvent(Counter, DebugEvent_EndBlock);


also for the FRAME_MAKRER, we create a RecordDebugEvent();

                #define FRAME_MARKER(SecondsElapsedInit) \
                { \
                    int Counter = __COUNTER__; \
                    RecordDebugEventCommon(Counter, DebugEvent_FrameMarker); \
                    ...
                    ...                                 \
                } 


so Casey_s idea is that for capturing sim_entity data, we will also create DebugEvents for it.





as you can see in the section above, just like in profiling we have profiling blocks

                BEGIN_BLOCK();

                END_BLOCK();

now we will have data blocks

                DEBUG_BEGIN_HOT_ELEMENT(Entity);

                DEBUG_END_HOT_ELEMENT();

and inside these datablocks, we will have debug events for DebugEvent_V2, DebugEvent_V3 and so 


                DEBUG_BEGIN_HOT_ELEMENT(Entity);
                DEBUG_VALUE(Entity->StorageIndex);
                ...
                ...
                DEBUG_VALUE(Entity->WalkableHeight);
                DEBUG_END_HOT_ELEMENT();

so the idea is that all these DEBUG_VALUE(); calls creating debug_event will be store within a data block


12:08
So Casey added some more types of debug_event_type

notice the types for DebugEvent_OpenDataBlock and DebugEvent_CloseDataBlock,
as well as all the other DebugEvents types below:

                handmade_platform.h

                enum debug_event_type
                {
                    DebugEvent_FrameMarker,
                    DebugEvent_BeginBlock,
                    DebugEvent_EndBlock,

                    DebugEvent_OpenDataBlock,
                    DebugEvent_CloseDataBlock,
                    
                    DebugEvent_R32,
                    DebugEvent_U32,
                    DebugEvent_S32,
                    DebugEvent_V2,
                    DebugEvent_V3,
                    DebugEvent_V4,
                    DebugEvent_Rectangle2,
                    DebugEvent_Rectangle3,
                };





14:06
so within a debug_event, we want to add storage for all these possible types of data data 


                handmade_platform.h
                
                struct debug_event
                {
                    u64 Clock;
                    u16 DebugRecordIndex;
                    u8 TranslationUnit;
                    u8 Type;
                    union
                    {
                        threadid_coreindex TC;
                        r32 SecondsElapsed;

                        void *VecPtr[3];
                        s32 VecS32[6];
                        u32 VecU32[6];
                        r32 VecR32[6];
                    };
                };

since all the data are in a union, now when we create a debug_event, it will take as much space as a 
rectangle3, which is 6 floats. So this is going to massively bloat the storage space required for 
regular debug_events. 



21:20
Casey beginning to add a bunch of DEBUG #define

-   so essentially, if you have the HANDMADE_INTERNAL #define on, we would have these debug variables activated 

-   notice the DEBUG_BEGIN_DATA_BLOCK(); and DEBUG_END_DATA_BLOCK(); will call RecordDebugEventCommon(); to 
    create a debug_event

-   so notice how we use the C++ type discovery feature to have different DEBUGValueSetEventData(); function

    In the DEBUG_VALUE(); #define, we would call DEBUGValueSetEventData(); which will go on and set 
    the different values for debug_event

-   full code below:

                handmade_platform.h

                #if defined(__cplusplus) && defined(HANDMADE_INTERNAL)

                inline void DEBUGValueSetEventData(debug_event *Event, r32 Value)
                {
                    Event->Type = DebugEvent_R32;
                    Event->VecR32[0] = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, u32 Value)
                {
                    Event->Type = DebugEvent_U32;
                    Event->VecU32[0] = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, s32 Value)
                {
                    Event->Type = DebugEvent_S32;
                    Event->VecS32[0] = Value;
                }

                #define DEBUG_BEGIN_DATA_BLOCK(Name, Ptr0, Ptr1)    \
                     { \
                         int Counter = __COUNTER__;                                     \
                         RecordDebugEventCommon(Counter, DebugEvent_OpenDataBlock);     \
                         Event->VecPtr[0] = Ptr0;                                      \
                         Event->VecPtr[1] = Ptr1;                                      \
                         debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
                         Record->FileName = __FILE__;                                   \
                         Record->LineNumber = __LINE__;                                 \
                         Record->BlockName = Name;                               \
                     } 

                #define DEBUG_END_DATA_BLOCK()    \
                     { \
                         int Counter = __COUNTER__;                                     \
                         RecordDebugEventCommon(Counter, DebugEvent_CloseDataBlock);     \
                         debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
                         Record->FileName = __FILE__;                                   \
                         Record->LineNumber = __LINE__;                                 \
                     } 

                #define DEBUG_VALUE(Value)  \
                     { \
                         int Counter = __COUNTER__;                                     \
                         RecordDebugEventCommon(Counter, DebugEvent_R32);             \
                         DEBUGValueSetEventData(Event, Value);                          \
                         debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
                         Record->FileName = __FILE__;                                   \
                         Record->LineNumber = __LINE__;                                 \
                         Record->BlockName = "Value";                            \
                     } 

                #define DEBUG_BEGIN_ARRAY(...)
                #define DEBUG_END_ARRAY(...)


                #else

                #define DEBUG_BEGIN_DATA_BLOCK(...)
                #define DEBUG_END_DATA_BLOCK(...)
                #define DEBUG_VALUE(...)
                #define DEBUG_BEGIN_ARRAY(...)
                #define DEBUG_END_ARRAY(...)

                #endif


46:43
so notice in the DEBUG_BEGIN_DATA_BLOCK(); call, we passed in Ptr0 and Ptr1

also we store these two values in Event->VecPtr;

                #define DEBUG_BEGIN_DATA_BLOCK(Name, Ptr0, Ptr1)    \
                     { \
                         int Counter = __COUNTER__;                                     \
                         RecordDebugEventCommon(Counter, DebugEvent_OpenDataBlock);     \
                         Event->VecPtr[0] = Ptr0;                                      \
                         Event->VecPtr[1] = Ptr1;                                      \
                         debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
                         Record->FileName = __FILE__;                                   \
                         Record->LineNumber = __LINE__;                                 \
                         Record->BlockName = Name;                               \
                     } 

the idea is that we want to give this block a unique id that we can identify later. 


then in the game code where we put these data blocks, we just give it some id 

"GameState->LowEntities + Entity->StorageIndex". This way the debug system knows which entity it is examining

                handmade.cpp 

                sim_entity *Entity = SimRegion->Entities + EntityIndex;

                for(uint32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; ++VolumeIndex)
                {
                    sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;                        

                    v3 LocalMouseP = Unproject(RenderGroup, MouseP);


    ----------->    DEBUG_BEGIN_DATA_BLOCK("Hot Entity",
                                           GameState->LowEntities + Entity->StorageIndex,
                                           0);
                    DEBUG_VALUE(Entity->StorageIndex);
                    DEBUG_VALUE(Entity->Updatable);
                    DEBUG_VALUE(Entity->Type);
                    DEBUG_VALUE(Entity->P);
                    DEBUG_VALUE(Entity->dP);
                    DEBUG_VALUE(Entity->DistanceLimit);
                    DEBUG_VALUE(Entity->FacingDirection);
                    DEBUG_VALUE(Entity->tBob);
                    DEBUG_VALUE(Entity->dAbsTileZ);
                    DEBUG_VALUE(Entity->HitPointMax);

                    ...
                    ...

                    DEBUG_VALUE(Entity->WalkableDim);
                    DEBUG_VALUE(Entity->WalkableHeight);
                    DEBUG_END_DATA_BLOCK();
                }




29:28
then in handmade_math.h, we have all the other DEBUGValueSetEventData(); 

this is becuz in handmade_math.h, we defined v2, v3, rectangle2 and rectangle3, so we also 
put the DEBUGValueSetEventData(); functions for these types in the same file.
in the handmade_platform.h, we just define DEBUGValueSetEventData(); for the basic types 

                handmade_math.h

                inline void DEBUGValueSetEventData(debug_event *Event, v2 Value)
                {
                    Event->Type = DebugEvent_V2;
                    Event->VecR32[0] = Value.x;
                    Event->VecR32[1] = Value.y;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, v3 Value)
                {
                    Event->Type = DebugEvent_V3;
                    Event->VecR32[0] = Value.x;
                    Event->VecR32[1] = Value.y;
                    Event->VecR32[2] = Value.z;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, rectangle2 Value)
                {
                    Event->Type = DebugEvent_Rectangle2;
                    Event->VecR32[0] = Value.Min.x;
                    Event->VecR32[1] = Value.Min.y;
                    Event->VecR32[2] = Value.Max.x;
                    Event->VecR32[3] = Value.Max.y;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, rectangle3 Value)
                {
                    Event->Type = DebugEvent_Rectangle3;
                    Event->VecR32[0] = Value.Min.x;
                    Event->VecR32[1] = Value.Min.y;
                    Event->VecR32[2] = Value.Min.z;
                    Event->VecR32[3] = Value.Max.x;
                    Event->VecR32[4] = Value.Max.y;
                    Event->VecR32[5] = Value.Max.z;
                }



37:39
then in the CollateDebugRecords(); we would put in cases for the debugRecords

-   notice we added the two different Debug Events types:

                case DebugEvent_OpenDataBlock:
                {
                } break;

                case DebugEvent_CloseDataBlock:
                {
                } break;

    the idea is that we are openning a data region, writing data into it, and then finally closing it

-   full code below:

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {
                    if(Event->Type == DebugEvent_FrameMarker)
                    {
                        ...
                    } break;
                    case DebugEvent_BeginBlock:
                    {
                        ...
                        ...
                    } break;

                    case DebugEvent_EndBlock:
                    {

                    }
                    break;

                    case DebugEvent_OpenDataBlock:
                    {
                    } break;

                    case DebugEvent_CloseDataBlock:
                    {
                    } break;
                    
                    case DebugEvent_R32:
                    {
                    } break;

                    case DebugEvent_U32:
                    {
                    } break;

                    case DebugEvent_S32:
                    {
                    } break;

                    case DebugEvent_V2:
                    {
                    } break;

                    case DebugEvent_V3:
                    {
                    } break;

                    case DebugEvent_V4:
                    {
                    } break;

                    case DebugEvent_Rectangle2:
                    {
                    } break;

                    case DebugEvent_Rectangle3:
                    {
                    } break;

                    default:
                    {
                        Assert(!"Invalid event type");
                    } break;
                }    


41:36
so for each debug_thread, we will now store a linked list of open_debug_block for the profiling blocks 
as well as the data blocks 

                struct debug_thread
                {
                    u32 ID;
                    u32 LaneIndex;
                    open_debug_block *FirstOpenCodeBlock;
                    open_debug_block *FirstOpenDataBlock;
                    debug_thread *Next;    
                };

recall we first introduce this linked list as a stack concept in day 185. refer to that episode if you are confused


Q/A
58:25
when you compute very complicated math operations, for example some computation that includes 
sin, cos, sqrt or things of this nature, typically what you do is 
1.  series expansions, such as taylor series
2.  root finding by newton raphson method
    It is an iterative guess technique where if you have a function and its derivative, you can start with a guess
    that is close to the answer, and then you can refine it to get to the true answer 

    if you are interested in this kind of stuff, read Forman_s.Actons books
    https://en.wikipedia.org/wiki/Forman_S._Acton


so the person in the QA asked about 
https://en.wikipedia.org/wiki/Fast_inverse_square_root
