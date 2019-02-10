Handmade Hero Day 210 - Consolidating Debug Data Storage

Summary:
code clean up of the debug system

Keyword:
debug system


10:33
Casey first addressed how he didnt like the code duplication of the different debug_variable data types 
onece in the debug_event_type 

                handmade_platform.h

                enum debug_event_type
                {
                    DebugEvent_FrameMarker,
                    DebugEvent_BeginBlock,
                    DebugEvent_EndBlock,

                    DebugEvent_OpenDataBlock,
                    DebugEvent_CloseDataBlock,
                    
                    DebugEvent_R32,
    ---------->     DebugEvent_U32,
                    DebugEvent_S32,
                    DebugEvent_V2,
                    DebugEvent_V3,
                    DebugEvent_V4,
                    DebugEvent_Rectangle2,
                    DebugEvent_Rectangle3,
                };


another in debug_variable

                handmade_debug.h

                struct debug_variable
                {
                    debug_variable_type Type;
                    char *Name;

                    union
                    {
                        b32 Bool32;
                        s32 Int32;
                        u32 UInt32;
    -------->           r32 Real32;
                        v2 Vector2;
                        v3 Vector3;
                        v4 Vector4;
                        debug_profile_settings Profile;
                        debug_bitmap_display BitmapDisplay;
                        debug_variable_link VarGroup;
                    };
                };

so Casey decided to make a change. 


                handmade_platform.h

                struct debug_event
                {
                    u64 Clock;
                    threadid_coreindex TC;
                    u16 DebugRecordIndex;
                    u8 TranslationUnit;
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


and for our debug_variable, it will just store a debug_event

                handmade_debug.h

                struct debug_variable
                {
                    debug_type Type;
                    char *Name;
                    debug_event Event;        
                };


17:06
then also, in the debug_variable_type, we dont need all these debug_variable_type anymore, 
at 30:58 Casey also merged in enums that was initially in debug_variable_type.

so right now, every single enum related to the debug system, are now all under the debug_type enum

                handmade_platform.h

                enum debug_type
                {
                    DebugType_FrameMarker,
                    DebugType_BeginBlock,
                    DebugType_EndBlock,

                    DebugType_OpenDataBlock,
                    DebugType_CloseDataBlock,

                    DebugType_B32,
                    DebugType_R32,
                    DebugType_U32,
                    DebugType_S32,
                    DebugType_V2,
                    DebugType_V3,
                    DebugType_V4,
                    DebugType_Rectangle2,
                    DebugType_Rectangle3,
                    DebugType_BitmapID,    
                    DebugType_SoundID,    
                    DebugType_FontID,    

                    //

                    DebugType_FirstUIType = 256,
                    DebugType_CounterThreadList,
                //    DebugVariableType_CounterFunctionList,
                    DebugType_VarGroup,
                };



27:04
with this change, in the CollateDebugRecords(); function, we just put all the other debug_event type in the default case,
in which we create a debug_variable

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {    
                    ...
                    ...

                        case DebugType_OpenDataBlock:
                        {
                            ...
                            ...
                        } break;

                        case DebugType_CloseDataBlock:
                        {
                            ...
                            ...
                        } break;

                        default:
                        {
                            debug_variable *Var = CollateCreateGroupedVariable(
                                DebugState, Thread->FirstOpenDataBlock,
                                (debug_type)Event->Type, Source->BlockName);
                            Var->Event = *Event;
                        } break;
                }



44:08
so Casey also got rid of the VarGroup in debug_variable
instead, he put it in debug_variable_link 

                struct debug_variable_group;
                struct debug_variable_link
                {
                    debug_variable_link *Next;
                    debug_variable_link *Prev;
                    debug_variable_group *Children;
                    debug_variable *Var;
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

                struct debug_variable
                {
                    debug_type Type;
                    char *Name;
                    debug_event Event;        
                };



[so essentialy with this change Casey has a hiearchy where each node is a linked list of nodes instead of just a single node


                                       _______
                                      |       |
                                      |       |
                                      |_______|
                                       /    \
                              ---------      ---------
                             /                        \
                            /                          \
                  ----------                            -------
                 /                                             \
                /                                               \
            _______         _______                           _______         _______         _______ 
           |       | ----> |       |                         |       | ----> |       | ----> |       |
           |       |       |       |                         |       |       |       |       |       |
           |_______|       |_______|                         |_______|       |_______|       |_______|       








also now debug_variable just represents data, purely data, and nothing else]



