Handmade Hero Day 214 - Collating Permanent Debug Values

Summary:
finishing up the DEBUGInitializeValue(); function which creates DebugEvent for GlobalConstants debug switches 
also made it so that we added these events to the debug system collation function, this way the debug system 
becomes aware of them

also added them to the debug UI through the Collation flow 

this way we can manipuate these GlobalConstants debug switches in the debug UI

Keyword:
debug system


5:47
Casey finishing up what we started in day 213. We will now write the DEBUGInitializeValue(); function.
This will create a debug_event for a debug switch in handmade_config.h

-   notice Casey invovled the use of a Comma operator when it calls the DEBUGInitializeValue(); function
[Casey calls it the sequence operator]

here is more info on the Comma operator.
https://stackoverflow.com/questions/52550/what-does-the-comma-operator-do


-   notice we also call RecordDebugEvent();. This is becuz we want to have a way to add to our debug system to allow us 
    to manipuate and change the values in this debug_event

    so the way we are doing this is through the collation system 

    so the idea is that we create a debug_event for this GlobalConstants debug swtich.
    then we create another debug_event, which we will add to the collation system 

    the second debug_event will then point to our GlobalConstants debug switch event.

    which is why we do 

                Event->Value_debug_event = SubEvent;

    SubEvent is our GlobalConstants debug switch event,

    Event is the event we add to the collation system

    [i assume the reason why we are doing this is becuz we are #define land, and RecordDebugEvent doesnt return a 
        debug_event. therefore have it set it up this way where we are creating two debug_variables
        I assume in the future, we can somehow combine these two into debug_event]

-   full code below:

                handmade_debug_interface.h

                inline debug_event DEBUGInitializeValue(debug_type Type, debug_event *SubEvent, char *Name, char *FileName, u32 LineNumber)
                {
                    RecordDebugEvent(DebugType_MarkDebugValue, "");
                    Event->Value_debug_event = SubEvent;

                    SubEvent->Clock = 0;
                    SubEvent->FileName = FileName;
                    SubEvent->BlockName = Name;
                    SubEvent->LineNumber = LineNumber;
                    SubEvent->ThreadID = 0;
                    SubEvent->CoreIndex = 0;
                    SubEvent->Type = (u8)Type;

                    return(*SubEvent);
                }

                #define DEBUG_IF__(Path)  \
                    local_persist debug_event DebugValue##Path = DEBUGInitializeValue((DebugValue##Path.Value_b32 = GlobalConstants_##Path, DebugType_b32), &DebugValue##Path, #Path, __FILE__, __LINE__); \
                    if(DebugValue##Path.Value_b32)

                #define DEBUG_VARIABLE__(type, Path, Variable) \
                    local_persist debug_event DebugValue##Variable = DEBUGInitializeValue((DebugValue##Variable.Value_##type = GlobalConstants_##Path##_##Variable, DebugType_##type), &DebugValue##Variable, #Path "_" #Variable,  __FILE__, __LINE__); \
                    type Variable = DebugValue##Variable.Value_##type;





12:19
we also write all of these DEBUGValueSetEventData(); that sets the value for the debug_event

                inline void DEBUGValueSetEventData(debug_event *Event, r32 Value)
                {
                    Event->Type = DebugType_r32;
                    Event->Value_r32 = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, u32 Value)
                {
                    Event->Type = DebugType_u32;
                    Event->Value_u32 = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, s32 Value)
                {
                    Event->Type = DebugType_s32;
                    Event->Value_s32 = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, v2 Value)
                {
                    Event->Type = DebugType_v2;
                    Event->Value_v2 = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, v3 Value)
                {
                    Event->Type = DebugType_v3;
                    Event->Value_v3 = Value;
                }

                inline void mDEBUGValueSetEventData(debug_event *Event, v4 Value)
                {
                    Event->Type = DebugType_v4;
                    Event->Value_v4 = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, rectangle2 Value)
                {
                    Event->Type = DebugType_rectangle2;
                    Event->Value_rectangle2 = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, rectangle3 Value)
                {
                    Event->Type = DebugType_rectangle3;
                    Event->Value_rectangle3 = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, bitmap_id Value)
                {
                    Event->Type = DebugType_bitmap_id;
                    Event->Value_bitmap_id = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, sound_id Value)
                {
                    Event->Type = DebugType_sound_id;
                    Event->Value_sound_id = Value;
                }

                inline void DEBUGValueSetEventData(debug_event *Event, font_id Value)
                {
                    Event->Type = DebugType_font_id;
                    Event->Value_font_id = Value;
                }




28:38
as mentioned in section 5:47, we have a new DebugType_MarkDebugValue for collating the GlobalConstants_ debug switches

so we added a new debug_type 
                
                handmade_debug_interface.h

                enum debug_type
                {
                    DebugType_Unknown,
                    
                    DebugType_FrameMarker,
                    DebugType_BeginBlock,
                    DebugType_EndBlock,

                    DebugType_OpenDataBlock,
                    DebugType_CloseDataBlock,

    --------->      DebugType_MarkDebugValue,

                    ...
                    ...
                }

and inside RecordDebugEvent, we added a debug_event pointer 

                struct debug_event
                {
                    u64 Clock;
                    // TODO(casey): To save space, we could put these two strings back-to-back with a null terminator in the middle
                    ...
                    ...
                    u16 CoreIndex;
                    u8 Type;
                    union
                    {
                        debug_id DebugID;
    ---------->         debug_event *Value_debug_event;
                        
                        b32 Value_b32;
                        s32 Value_s32;
                        u32 Value_u32;
                        r32 Value_r32;
                        ...
                        ...
                    };
                };


30:38
then in the CollateDebugRecords(); we handle the case for DebugType_MarkDebugValue.

recall we are implying our hiearchy through underscores

                #define GlobalConstants_Renderer_Camera_UseDebug 0
                #define GlobalConstants_Renderer_Camera_DebugDistance 25.0f

-   full code below:

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {  
                    ...
                    ...

                    for(u32 EventIndex = 0; EventIndex < GlobalDebugTable->EventCount[EventArrayIndex]; ++EventIndex)
                    {
                        debug_event *Event = GlobalDebugTable->Events[EventArrayIndex] + EventIndex;            

                        if(Event->Type == DebugType_MarkDebugValue)
                        {
                            CollateAddVariableToGroup(DebugState,
                                                      GetGroupForHierarchicalName(DebugState, Event->Value_debug_event->BlockName),
                                                      Event->Value_debug_event, true);
                        }
                        else if(Event->Type == DebugType_FrameMarker)
                        {
                            case DebugType_BeginBlock:
                            {
                                open_debug_block *DebugBlock = AllocateOpenDebugBlock(
                                    DebugState, FrameIndex, Event, &Thread->FirstOpenCodeBlock);
                            } break;

                            case DebugType_EndBlock:
                            {
                                ...
                                ....
                            } break;

                        }
                    }
                }

