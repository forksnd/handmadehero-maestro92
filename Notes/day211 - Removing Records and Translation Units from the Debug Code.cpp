Handmade Hero Day 211 - Removing Records and Translation Units from the Debug Code

Summary:
Code Cleanup
removed the concept of debug_variable. debug_variable data is entirely in debug_event
also removed the concept of debug_record. the debug system now operates completely on debug_event
also combined the concept of DebugType_VarGroup and DebugType_OpenDataBlock. We removed DebugType_VarGroup

[Currently we commented out all the debug_variable from handmade_config.h, so Casey hasnt tested "removing DebugType_VarGroup" with 
debug variables from handmade_config.h]

Keyword:
debug system


3:08
Created the handmade_debug_interface.h file and threw all the debug related code in there


4:08
so what we want to try to do here is that debug_variable is no longer a thing anymore.
so all the data will be in debug_event, and we kind of want to remove the concept of debug_variable entirely.

so Casey removed debug_variable struct entirely.

so now the debug_variable_link will store a debug_event

                handmade_debug.h

                struct debug_variable_group;
                struct debug_variable_link
                {
                    debug_variable_link *Next;
                    debug_variable_link *Prev;
                    debug_variable_group *Children;
                    debug_event *Event;
                };

                struct debug_tree
                {
                    v2 UIP;
                    debug_variable_group *Group;

                    debug_tree *Next;
                    debug_tree *Prev;
                };

                struct debug_variable_group
                {
                    debug_variable_link Sentinel;
                };


6:33
on the UI side, we also completely removed debug_variable, we will just use debug_event

                handmade_debug.h

                struct debug_interaction
                {
                    debug_id ID;
                    debug_interaction_type Type;
                    union
                    {
                        void *Generic;
    ---------------->   debug_event *Event;
                        debug_tree *Tree;
                        v2 *P;
                    };
                };

22:39

So Casey didnt like the debug_record counter idea anymore, So Casey is beginning to completely remove DebugRecords
from the debug system.


Casey stuffed everything that was in debug_record in debug_event 
so now the debug_event has. So right now the debug system is completely using the debug_event


                "
                char *FileName;
                char *BlockName;    
                u32 LineNumber;
                "

-   full code below:

                struct debug_event
                {
                    u64 Clock;
                    // TODO(casey): To save space, we could put these two strings back-to-back with a null terminator in the middle
    ---------->     char *FileName;
                    char *BlockName;    
                    u32 LineNumber;
                    u16 ThreadID;
                    u16 CoreIndex;
                    u8 Type;
                    union
                    {
                        void *VecPtr[2];
                        
                        b32 Bool32;
                        s32 Int32;
                        u32 UInt32;
                        r32 Real32;
                        v2 Vector2;
                        v3 Vector3;
                        v4 Vector4;
                        rectangle2 Rectangle2;
                        rectangle3 Rectangle3;
                        bitmap_id BitmapID;
                        sound_id SoundID;
                        font_id FontID;
                    };
                };


23:05
Casey removed debug_records from the debug_table 

                handmade_debug.h

                struct debug_table
                {
                    // TODO(casey): No attempt is currently made to ensure that the final
                    // debug records being written to the event array actually complete
                    // their output prior to the swap of the event array index.
                    
                    u32 CurrentEventArrayIndex;
                    u64 volatile EventArrayIndex_EventIndex;
                    u32 EventCount[MAX_DEBUG_EVENT_ARRAY_COUNT];
                    debug_event Events[MAX_DEBUG_EVENT_ARRAY_COUNT][MAX_DEBUG_EVENT_COUNT];
                };


23:40
so previously, we had something like:
                    

                #define FRAME_MARKER(SecondsElapsedInit) \
                     { \
                     int Counter = __COUNTER__; \
                     RecordDebugEvent(Counter, DebugType_FrameMarker); \
                     Event->Real32 = SecondsElapsedInit; \
                     debug_record *Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
                     Record->FileName = __FILE__;                                        \
                     Record->LineNumber = __LINE__;                                    \
                     Record->BlockName = "Frame Marker";                                   \
                } 

now we are only operating on debug_event. As you can see, debug_event now stores all the fileName, LineNumber and BlockName information
so right now we dont have the compiler __COUNTER__ trick anymore. all the debug_event goes into a rolling array

                #define RecordDebugEvent(EventType, Block)           \
                    u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
                    u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;            \
                    Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);                     \
                    debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex; \
                    Event->Clock = __rdtsc();                       \
                    Event->Type = (u8)EventType;                                    \
                    Event->CoreIndex = 0;                                           \
                    Event->ThreadID = (u16)GetThreadID();                         \
    --------->      Event->FileName = __FILE__;                                      \
                    Event->LineNumber = __LINE__;                                   \
                    Event->BlockName = Block;                              \
                

                #define FRAME_MARKER(SecondsElapsedInit) \
                     { \
                     int Counter = __COUNTER__; \
                     RecordDebugEvent(DebugType_FrameMarker, "Frame Marker"); \
                     Event->Real32 = SecondsElapsedInit; \
                } 




40:46
so right now our debug_event looks like this, and Casey is thinking about how to compress this 

right now the storage space required is 


                struct debug_event          size 
                {
                    u64 Clock;              64
                    // TODO(casey): To save space, we could put these two strings back-to-back with a null terminator in the middle
                    char *FileName;         64 (64 bit machine)
                    char *BlockName;        64 (64 bit machine)
                    u32 LineNumber;         32
                    u16 ThreadID;           16
                    u16 CoreIndex;          16
                    u8 Type;                8
                    union
                    {
                        void *VecPtr[2];
                        
                        b32 Bool32;
                        s32 Int32;
                        u32 UInt32;
                        r32 Real32;
                        v2 Vector2;
                        v3 Vector3;
                        v4 Vector4;
                        rectangle2 Rectangle2;
                        rectangle3 Rectangle3;
                        bitmap_id BitmapID;
                        sound_id SoundID;
                        font_id FontID;
                    };
                };

Casey does mention that we can somehow combine FileName and BlockName into one string. His comments says it all 
// TODO(casey): To save space, we could put these two strings back-to-back with a null terminator in the middle


