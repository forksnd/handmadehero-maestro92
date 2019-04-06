Handmade Hero Day 252 - Allowing Debug Value Edits


Summary:
refactored the DEBUGValueSetEventData(); #define. now its all cleaned up
now we call edit debug values

nothing interesting. lots of debugging

Keyword:
Debug System



3:03
so to re-enable editing on the debug, what has to happen is that each debug event has a GUID
that we hash by. So what we do is to look up by GUID and see if there is a edit pending flag on it.

                handmade_debug_interface.h 

                #define DEBUG_VALUE(Value)  \
                 { \
                     RecordDebugEvent(DebugType_Unknown, DEBUG_NAME(#Value));                              \
                     DEBUGValueSetEventData(Event, Value);               \
                     DEBUGHandleValueEdit(Event, &Value);                   \
                 } 
            // TODO(casey):    DEBUGHandleValueEdit(Event, &Value);




35:40
added the "b32 ValueWasEdited;" variable in the debug_element struct 

                handmade_debug.h        

                struct debug_element
                {
                    char *OriginalGUID; // NOTE(casey): Can never be printed!  Might point into unloaded DLL.
                    char *GUID;
                    u32 FileNameCount;
                    u32 LineNumber;
                    u32 NameStartsAt;
                    
    ------------>    b32 ValueWasEdited;
                    
                    debug_element *NextInHash;

                    debug_stored_event *OldestEvent;
                    debug_stored_event *MostRecentEvent;
                };

45:15
Casey changed all the DEBUGValueSetEventData(); to a new function

                handmade_debug_interface.h 

                extern debug_event *DEBUGGlobalEditEvent;

                #define DEBUGValueSetEventData_(type) \
                inline void \
                DEBUGValueSetEventData(debug_event *Event, type Ignored, void *Value) \
                { \
                    Event->Type = DebugType_##type; \
                    if(GlobalDebugTable->EditEvent.GUID == Event->GUID) \
                    { \
                        *(type *)Value = GlobalDebugTable->EditEvent.Value_##type; \
                    } \
                    \
                    Event->Value_##type = *(type *)Value; \
                }

                DEBUGValueSetEventData_(r32);
                DEBUGValueSetEventData_(u32);
                DEBUGValueSetEventData_(s32);
                DEBUGValueSetEventData_(v2);
                DEBUGValueSetEventData_(v3);
                DEBUGValueSetEventData_(v4);
                DEBUGValueSetEventData_(rectangle2);
                DEBUGValueSetEventData_(rectangle3);
                DEBUGValueSetEventData_(bitmap_id);
                DEBUGValueSetEventData_(sound_id);
                DEBUGValueSetEventData_(font_id);


to replace all the other 


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

                ...
                ...


notice that Casey replaced all the separate function with all the macros.
The alternative way of doing templates is to abuse the #macros 



The idea behind the function

                #define DEBUGValueSetEventData_(type) \
                inline void \
                DEBUGValueSetEventData(debug_event *Event, type Ignored, void *Value) \
                { \
                    Event->Type = DebugType_##type; \
                    if(GlobalDebugTable->EditEvent.GUID == Event->GUID) \
                    { \
                        *(type *)Value = GlobalDebugTable->EditEvent.Value_##type; \
                    } \
                    \
                    Event->Value_##type = *(type *)Value; \
                }

is that "void *Value" is gonna be the data we will be editing. If the DEBUGGlobalEditEvent is the same 
as the debug_event we are looking at. then we write the value back to the variable Value 


For example: 
recall we had this code:

                if(DEBUG_REQUESTED(EntityDebugID))
                {
                    DEBUG_DATA_BLOCK("Simulation/Entity");
                    // TODO(casey): Do we want this DEBUG_VALUE(EntityDebugID); ??
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
                    DEBUG_VALUE(HeroBitmaps.Torso);

                }

and lets say for one the DEBUG_VALUE calls, we are editing Entity->P. if we happen to be editing the debug_UI item 
that represents that debug event, then we write back into the Entity->P variable 

                #define DEBUG_VALUE(Value)  \
                     { \
                         RecordDebugEvent(DebugType_Unknown, DEBUG_NAME(#Value));                              \
                         DEBUGValueSetEventData(Event, Value, (void *)&Value); \
                     } 

this way we dont have to store pointers 


55:21
Casey added the debug_event EditEvent in the debug_table struct 

                handmade_debug_interface.h

                struct debug_table
                {
    ----------->    debug_event EditEvent;
                    
                    // TODO(casey): No attempt is currently made to ensure that the final
                    // debug records being written to the event array actually complete
                    // their output prior to the swap of the event array index.    
                    u32 CurrentEventArrayIndex;
                    // TODO(casey): This could actually be a u32 atomic now, since we
                    // only need 1 bit to store which array we're using...
                    u64 volatile EventArrayIndex_EventIndex;
                    debug_event Events[2][16*65536];
                };

