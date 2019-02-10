Handmade Hero Day 209 - Displaying Buffered Debug Data

Summary:
create debug_varaibles and debug_variable groups for DebugEvent_OpenDataBlock

Keyword:
debug system


5:58
so in the CollateDebugRecords(); we officially write code to handle a 
DebugEvent_OpenDataBlock and DebugEvent_CloseDataBlock debug event

in DebugEvent_OpenDataBlock other than creating OpenDebugBlock (so that we can deallocate it when we get a CloseDataBlock debug event);
the other thing is that we create a VariableGroup. Notice that we store it in DebugBlock->Group.

this will be explained later.

-   full code below:

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {    
                    ...
                    ...

                    case DebugEvent_OpenDataBlock:
                    {
                        open_debug_block *DebugBlock = AllocateOpenDebugBlock(
                            DebugState, FrameIndex, Event, Source, &Thread->FirstOpenDataBlock);                        
                        
                        DebugBlock->Group = CollateCreateVariableGroup(DebugState, Source->BlockName);
                        CollateAddVariableToGroup(DebugState,
                                                  DebugBlock->Parent ? DebugBlock->Parent->Group : DebugState->CollationFrame->RootGroup,
                                                  DebugBlock->Group);
                    } break;

                    case DebugEvent_CloseDataBlock:
                    {
                        if(Thread->FirstOpenDataBlock)
                        {
                            open_debug_block *MatchingBlock = Thread->FirstOpenDataBlock;
                            debug_event *OpeningEvent = MatchingBlock->OpeningEvent;
                            if(EventsMatch(*OpeningEvent, *Event))
                            {
                                DeallocateOpenDebugBlock(DebugState, &Thread->FirstOpenDataBlock);
                            }
                            else
                            {
                                // TODO(casey): Record span that goes to the beginning of the frame series?
                            }
                        }
                    } break;

                    ...
                    ...
                }

and in the AllocateOpenDebugBlock(); and DeallocateOpenDebugBlock(); we just do all the operation we need.
should be pretty straightforward.

                inline open_debug_block *
                AllocateOpenDebugBlock(debug_state *DebugState, u32 FrameIndex, debug_event *Event,
                                       debug_record *Source, open_debug_block **FirstOpenBlock)
                {
                    open_debug_block *Result = DebugState->FirstFreeBlock;
                    if(Result)
                    {
                        DebugState->FirstFreeBlock = Result->NextFree;
                    }
                    else
                    {
                        Result = PushStruct(&DebugState->CollateArena, open_debug_block);
                    }

                    Result->StartingFrameIndex = FrameIndex;
                    Result->OpeningEvent = Event;
                    Result->Source = Source;
                    Result->NextFree = 0;

                    Result->Parent = *FirstOpenBlock;
                    *FirstOpenBlock = Result;

                    return(Result);
                }

                inline void
                DeallocateOpenDebugBlock(debug_state *DebugState, open_debug_block **FirstOpenBlock)
                {
                    open_debug_block *FreeBlock = *FirstOpenBlock;
                    *FirstOpenBlock = FreeBlock->Parent;

                    FreeBlock->NextFree = DebugState->FirstFreeBlock;
                    DebugState->FirstFreeBlock = FreeBlock;                            
                }


23:09
so once we have the DebugEvent_OpenDataBlock and DebugEvent_CloseDataBlock cases handled,
we can go on handle the data cases for the DebugEvent_OpenDataBlock and DebugEvent_CloseDataBlock
essentially we just create debug variables and add it into the debug_variable_group

                handmade_debug.cpp

                internal void CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
                {    


                    case DebugEvent_R32:
                    {
                        debug_variable *Var = CollateCreateGroupedVariable(
                            DebugState, Thread->FirstOpenDataBlock,
                            DebugVariableType_Real32, Source->BlockName);
                        Var->Real32 = Event->VecR32[0];
                    } break;

                    case DebugEvent_U32:
                    {
                        debug_variable *Var = CollateCreateGroupedVariable(
                            DebugState, Thread->FirstOpenDataBlock,
                            DebugVariableType_UInt32, Source->BlockName);
                        Var->UInt32 = Event->VecU32[0];
                    } break;

                    case DebugEvent_S32:
                    {
                        debug_variable *Var = CollateCreateGroupedVariable(
                            DebugState, Thread->FirstOpenDataBlock,
                            DebugVariableType_Int32, Source->BlockName);
                        Var->Int32 = Event->VecS32[0];
                    } break;

                    case DebugEvent_V2:
                    {
                        debug_variable *Var = CollateCreateGroupedVariable(
                            DebugState, Thread->FirstOpenDataBlock,
                            DebugVariableType_V2, Source->BlockName);
                        Var->Vector2.x = Event->VecR32[0];
                        Var->Vector2.y = Event->VecR32[1];
                    } break;

                    case DebugEvent_V3:
                    {
                        debug_variable *Var = CollateCreateGroupedVariable(
                            DebugState, Thread->FirstOpenDataBlock,
                            DebugVariableType_V3, Source->BlockName);
                        Var->Vector3.x = Event->VecR32[0];
                        Var->Vector3.y = Event->VecR32[1];
                        Var->Vector3.z = Event->VecR32[2];
                    } break;

                    case DebugEvent_V4:
                    {
                        debug_variable *Var = CollateCreateGroupedVariable(
                            DebugState, Thread->FirstOpenDataBlock,
                            DebugVariableType_V4, Source->BlockName);
                        Var->Vector4.x = Event->VecR32[0];
                        Var->Vector4.y = Event->VecR32[1];
                        Var->Vector4.z = Event->VecR32[2];
                        Var->Vector4.w = Event->VecR32[3];
                    } break;
                }






                internal debug_variable * CollateCreateGroupedVariable(debug_state *DebugState, open_debug_block *Block,
                                             debug_variable_type Type, char *Name)
                {
                    debug_variable *Result = CollateCreateVariable(DebugState, Type, Name);
                    Assert(Block);
                    Assert(Block->Group);

                    CollateAddVariableToGroup(DebugState, Block->Group, Result);

                    return(Result);
                }




27:15
so now Casey wants to address the problem of where the debug_variable created in all of these DebugEvent cases will live

Currently, the debug_state contains a "global" RootGroup where everything lives in 
                
                handmade_debug.h

                struct debug_state
                {
                    ...
                    ...

                    debug_variable *RootGroup;
                };

but what Casey rather do is to have the ability to build the debug_variables for these data blocks per frame 
so as frames come, we build up our debug_variables, but as frames go away, the debug_variables go away

so in the debug_frame, casey added a "debug_variable *RootGroup" variable

                handmade_debug.h

                #define MAX_REGIONS_PER_FRAME 2*4096
                struct debug_frame
                {
                    u64 BeginClock;
                    u64 EndClock;
                    r32 WallSecondsElapsed;

    ------------->  debug_variable *RootGroup;

                    u32 RegionCount;
                    debug_frame_region *Regions;
                };

so the idea is that lets say on a debug frame and we have multiple data blocks.
we build the hiearchy where the debug_frame has the root node of debug_variable_group
so if you have something like:

                debug_frame
                {

                    // block0
                    DEBUG_BEGIN_HOT_ELEMENT(Entity);    
                        ...
                        ...
                        // block 0.1
                        DEBUG_BEGIN_HOT_ELEMENT(Entity);


                        DEBUG_END_HOT_ELEMENT();
                        ...
                        ...
                    DEBUG_END_HOT_ELEMENT();


                    // block1
                    DEBUG_BEGIN_HOT_ELEMENT(Entity);
                        ...
                        ...
                        // block 0.2
                        DEBUG_BEGIN_HOT_ELEMENT(Entity);


                        DEBUG_END_HOT_ELEMENT();
                        ...
                        ...
                    DEBUG_END_HOT_ELEMENT();
                }

so visually you have something like:

                                debug_frame.RootGroup

                           /                                  \
                          /                                    \
                         /                                      \
                        /                                        \

            block1.RootGroup                                 bloc2.RootGroup

            /               \                                 /             \
           /                 \                               /               \
    
    v3 debug_varaible      block0_1.RootGroup          block0_2.RootGroup     v2 debug_varaible
                          
                              /          \                 /         \
                             /            \               /           \           


33:32
so then we add debug_variable* Group in the open_debug_block. While open_debug_block is shared among 
OpenDataBlock and the Profiling blocks, only the data blocks will use it 

                struct open_debug_block
                {
                    u32 StartingFrameIndex;
                    debug_record *Source;
                    debug_event *OpeningEvent;
                    open_debug_block *Parent;

                    // NOTE(casey): Only for data blocks?  Probably!
    ------------>   debug_variable *Group;
                    
                    open_debug_block *NextFree;
                };

and back to very beginning, in the CollateDebugRecords(); everytime we have a DebugEvent_OpenDataBlock debug event, 
we create a debug_variable group for the DebugBlock

you might also notice that we are checking for DebugBlock->Parent when we call CollateAddVariableToGroup();
the thing is that if you have nested data blocks 
                

                DEBUG_BEGIN_HOT_ELEMENT(Entity);
                    ...
                    ...
                    DEBUG_BEGIN_HOT_ELEMENT(Entity);


                    DEBUG_END_HOT_ELEMENT();
                    ...
                    ...
                DEBUG_END_HOT_ELEMENT();

we would want to preseve this hiearchy, if DebugBlock->Parent is valid, we would want to add our rootGroup to our parent_s 
root group. Otherwise, we just add it to the debug_frame root group.

-   full code below:

                case DebugEvent_OpenDataBlock:
                {
                    open_debug_block *DebugBlock = AllocateOpenDebugBlock(
                        DebugState, FrameIndex, Event, Source, &Thread->FirstOpenDataBlock);                        
                    
                    DebugBlock->Group = CollateCreateVariableGroup(DebugState, Source->BlockName);
                    CollateAddVariableToGroup(DebugState,
                                              DebugBlock->Parent ? DebugBlock->Parent->Group : DebugState->CollationFrame->RootGroup,
                                              DebugBlock->Group);
                } break;



46:24
